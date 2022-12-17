/******************************************************************************/
#include "stdafx.h"
/******************************************************************************/
#define COMPARE  ComparePathCI
#define TIME     Time.realTime() // use 'realTime' because sometimes it's mixed with 'curTime'
#define CC4_INCH CC4('I','N','C','H')
/******************************************************************************/
namespace EE{
/******************************************************************************/
static Str EatWWW(Str url) // convert "http://www.esenthel.com" -> "http://esenthel.com"
{
   Bool http =          StartsPath(url, "http://" ) ; if(http )url.remove(0, 7);
   Bool https=(!http && StartsPath(url, "https://")); if(https)url.remove(0, 8);
   if(Starts(url, "www."))url.remove(0, 4);
   if(https)url.insert(0, "https://");
   if(http )url.insert(0, "http://" );
   return url;
}
static Str ShortName(Str url) // convert "http://www.esenthel.com" -> "esenthel.com"
{
   if(StartsPath(url, "http://" ))url.remove(0, 7);else
   if(StartsPath(url, "https://"))url.remove(0, 8);
   if(Starts    (url, "www."    ))url.remove(0, 4);
   return url;
}
static void ImportImageFunc(InternetCache::ImportImage &ii, InternetCache &ic, Int thread_index=0)
{
   File temp; if(File *src=ii.data.open(temp))
   {
      ThreadMayUseGPUData();
      ii.image_temp.Import(*src, -1, IMAGE_2D, ic._image_mip_maps);
      ThreadFinishedUsingGPUData();
   }
   ii.done=true; // !! don't do anything after this step because the object can get removed !!
}
/******************************************************************************/
InternetCache::InternetCache() : _downloaded(COMPARE) {}
/******************************************************************************/
struct SrcFile : PakFileData, InternetCache::FileTime
{
   UInt compressed_size;

   SrcFile& set(C Str &name, C InternetCache::Downloaded &src)
   {
      type=FSTD_FILE; compress_mode=(Compressable(GetExt(name)) ? COMPRESS_ENABLE : COMPRESS_DISABLE); decompressed_size=compressed_size=src.file_data.elms(); T.name=name; data.set(src.file_data.data(), src.file_data.elms()); modify_time_utc=src.modify_time_utc; access_time=src.access_time; verify_time=src.verify_time; return T;
   }
   SrcFile& set(C Str &name, C Pak &pak, C PakFile &pf, C InternetCache::FileTime &time)
   {
      type=pf.type(); compress_mode=COMPRESS_KEEP_ORIGINAL; compressed=pf.compression; decompressed_size=pf.data_size; compressed_size=pf.data_size_compressed; T.name=name; data.set(pf, pak); xxHash64_32=pf.data_xxHash64_32; modify_time_utc=pf.modify_time_utc; access_time=time.access_time; verify_time=time.verify_time; return T;
   }
};
typedef Memc<SrcFile> SrcFiles;

static Int CompareName      (C SrcFile &a, C SrcFile &b) {return COMPARE(a.name       , b.name       );}
static Int CompareName      (C SrcFile &a, C Str     &b) {return COMPARE(a.name       , b            );}
static Int CompareAccessTime(C SrcFile &a, C SrcFile &b) {return Compare(b.access_time, a.access_time);} // reverse order to list files with biggest access time first

static void ICUpdate(InternetCache &ic);

static void SavePostHeader(File &f, C Pak &pak, Ptr user)
{
   f.putMulti(UInt(CC4_INCH), Byte(0)); // version
   SrcFiles &files=*(SrcFiles*)user;
   Dbl time=Time.curTime(); Long now=DateTime().getUTC().seconds(); // calculate times at same moment
   FREPA(pak)
   {
      Flt  access_time; // store as relative to current time, it will be -Inf..0
      Long verify_time; // store as absolute DateTime.seconds
      if(C SrcFile *file=files.binaryFind(pak.fullName(i), CompareName))
      {
         access_time=       file->access_time-time;
         verify_time=RoundL(file->verify_time-time)+now;
      }else
      {
         access_time=-FLT_MAX; verify_time=0;
      }
      f.putMulti(access_time, verify_time);
   }
}
void InternetCache::del()
{
   REPAO(_downloading).stop();
   if(_threads)REPA(_import_images)_threads->cancel(_import_images[i], ImportImageFunc, T); // cancel importing

   flush();

   if(_threads)
   {
      REPA(_import_images)_threads->wait(_import_images[i], ImportImageFunc, T);
     _threads=null;
   }

   got=null;
  _downloaded          .del();
  _import_images       .del();
  _to_download         .del();
  _to_verify           .del();
  _pak                 .del();
  _pak_used_file_ranges.del();
  _pak_files           .del();
   App._callbacks.exclude(ICUpdate, T);

   REPAO(_downloading).del();
}
void InternetCache::create(C Str &name, Threads *threads, Cipher *cipher, COMPRESS_TYPE compress)
{
   del();

   if(D.canUseGPUDataOnSecondaryThread())_threads=threads; // setup worker threads only if we can operate on GPU on secondary threads
  _compress      =compress;
//_image_mip_maps=image_mip_maps;
   if(name.is())
   {
      File f; if(f.readStd(name, cipher))
      {
         if(_pak.loadHeader(f, null, null, _pak_used_file_ranges)==PAK_LOAD_OK)
         {
           _pak.pakFileName(name);
            if(_pak._cipher_per_file)f.cipherOffsetClear(); // post-header is right after header
            Dbl time=Time.curTime(); Long now=DateTime().getUTC().seconds(); // calculate times at same moment
            if(f.getUInt()==CC4_INCH)switch(f.decUIntV()) // version
            {
               case 0:
               {
                 _pak_files.setNum(_pak.totalFiles()); FREPA(_pak_files)
                  {
                     FileTime &file_time=_pak_files[i];
                     Flt access_time; Long verify_time; f.getMulti(access_time, verify_time);
                     file_time.access_time=Min(0, access_time    )     ; // prevent setting to the future in case of corrupt data
                     file_time.verify_time=Min(0, verify_time-now)+time; // prevent setting to the future in case of corrupt data
                  }
                  if(f.ok())return;
               }break;
            }
         }
        _pak.del(); _pak_used_file_ranges.clear(); _pak_files.clear();
      }
      SrcFiles files; PakPostHeader post_header(SavePostHeader, &files);
     _pak.create(CMemPtr<PakFileData>(), name, 0, cipher, COMPRESS_NONE, 0, null, null, null, _pak_used_file_ranges, &post_header); // create an empty pak
   }
}
/******************************************************************************/
Bool InternetCache::flush()
{
 //if(_downloaded.elms()) always save because we need to save 'access_time' and 'verify_time' which can change without new '_downloaded'
   {
      if(_pak.pakFileName().is()) // we want to save data
      {
         if(_threads)REPA(_import_images)_threads->wait(_import_images[i], ImportImageFunc, T); // wait until the worker threads finish importing, as they operate on '_downloaded' and '_pak' which we're about to update

         SrcFiles files; Long file_size=0;
         FREPA(_downloaded)                                                            file_size+=files.New().set(_downloaded.key(i), _downloaded[i]     ).compressed_size;
         FREPA(_pak       )if(i){Str name=_pak.fullName(i); if(!_downloaded.find(name))file_size+=files.New().set(name, _pak, _pak.file(i), _pak_files[i]).compressed_size;} // skip post-header #PostHeaderFileIndex and files already included from '_downloaded'
         if(_max_file_size>=0 && file_size>_max_file_size) // limit file size
         {
            files.sort(CompareAccessTime);
            do{file_size-=files.last().compressed_size; files.removeLast();}while(file_size>_max_file_size && files.elms());
         }
         files.sort(CompareName); // needed for 'binaryFind' in 'SavePostHeader' and below
         PakPostHeader post_header(SavePostHeader, &files);

         if(((_max_file_size<0 || FSize(_pak.pakFileName())/2<_max_file_size) // if pak size is smaller than 2x limit, allow some tolerance because 'replaceInPlace' does not generate compact paks but they may have holes
           && _pak.replaceInPlace(_pak_used_file_ranges, SCAST(Memc<PakFileData>, files), 0, _compress, _compress_level, null, null, &post_header)) // replace in place
         ||   _pak.replace       (_pak_used_file_ranges, SCAST(Memc<PakFileData>, files), 0, _compress, _compress_level, null, null, &post_header)) // recreate
         {
           _downloaded.del();
           _pak_files.setNumDiscard(_pak.totalFiles());
            REPA(_pak_files)
            {
                   FileTime &file=_pak_files[i];
               if(C SrcFile *src =files.binaryFind(_pak.fullName(i), CompareName))
               {
                  file.access_time=src->access_time;
                  file.verify_time=src->verify_time;
               }else
               {
                  file.access_time=-FLT_MAX;
                  file.verify_time= INT_MIN;
               }
            }
         }else return false;
      }else
      if(_max_mem_size>=0)
      {
         SrcFiles files; Long size=0;
         FREPA(_downloaded)size+=files.New().set(_downloaded.key(i), _downloaded[i]).compressed_size;
         if(size>_max_mem_size) // limit mem size
         {
            files.sort(CompareAccessTime);
            if(_threads)REPA(_import_images)_threads->wait(_import_images[i], ImportImageFunc, T); // wait until the worker threads finish importing, as they operate on '_downloaded' and '_pak' which we're about to update
            do
            {
               SrcFile &file=files.last();
              _downloaded.removeKey(file.name);
               size-=file.compressed_size;
               files.removeLast();
            }while(size>_max_mem_size && files.elms());
         }
      }
   }
   return true;
}
/******************************************************************************/
void InternetCache::changed(C Str &url)
{
   if(url.is())
   {
      Str name=ShortName(url); if(name.is())
      {
         if(Downloaded *down=_downloaded.find(name       ))down       ->verify_time=INT_MIN;
         if(C PakFile  *pf  =_pak       .find(name, false))pakFile(*pf).verify_time=INT_MIN;
         REPA(_downloading)
         {
            Download &down=_downloading[i]; if(EqualPath(down.url(), url))
            { // restart the download
            #if 1
               down.create(url);
            #else
               down.del(); _to_download.binaryInclude(url, COMPARE);
            #endif
               return;
            }
         }
         if(_to_verify  .binaryHas(url, COMPARE))return; // it will be checked
         if(_to_download.binaryHas(url, COMPARE))return; // it will be downloaded
         if(ImagePtr().find(url)) // download if currently referenced TODO: could be replaced with Images.has
         {
           _to_download.binaryInclude(url, COMPARE); enable();
         }
      }
   }
}
Bool InternetCache::getFile(C Str &url, DataSource &file, CACHE_VERIFY verify)
{
   file.set();
   if(!url.is())return true;
   Str name=ShortName(url); if(!name.is())return false;

   // find in cache
   Flt *verify_time;
   if(Downloaded *down=_downloaded.find(name))
   {
                   down->access_time=TIME;
      verify_time=&down->verify_time;
      file.set(down->file_data.data(), down->file_data.elms());
   }else
   if(C PakFile *pf=_pak.find(name, false))
   {
      FileTime &time=pakFile(*pf);
                   time.access_time=TIME;
      verify_time=&time.verify_time;
      file.set(*pf, _pak);
   }else
   { // not found
      REPA(_downloading)if(EqualPath   (_downloading[i].url(), url))goto downloading;
                        if(_to_download.binaryInclude(url, COMPARE))enable();
   downloading:
      return false;
   }

   // found
   if(verify==CACHE_VERIFY_SKIP)return true;
   if(TIME-*verify_time<=_verify_life)return true; // verification still acceptable
   // verify
   REPA(_downloading)if(EqualPath   (_downloading[i].url(), url))goto verifying; // downloading now
                     if(_to_download.binaryHas    (url, COMPARE))goto verifying; // will download soon
                     if(_to_verify  .binaryInclude(url, COMPARE))enable();       // verify
verifying:
   return verify==CACHE_VERIFY_DELAY;
}
ImagePtr InternetCache::getImage(C Str &url, CACHE_VERIFY verify)
{
   ImagePtr img; if(url.is())
   {
      DataSource file; Bool get_file=getFile(url, file, verify); // always call 'getFile' to adjust 'access_time' and request verification if needed
      if(!img.find(url))
      {
         CACHE_MODE mode=Images.mode(CACHE_DUMMY); img=url;
                         Images.mode(mode       );
         if(get_file)
         {
            ImportImage &ii=_import_images.New();
            Swap(ii.data, file);
            ii.image_ptr=img;
            import(ii);
            enable();
         }
      }
   }
   return img;
}
/******************************************************************************/
void InternetCache::import(ImportImage &ii)
{
   if(_threads)_threads->queue(ii, ImportImageFunc, T);else ImportImageFunc(ii, T);
}
void InternetCache::cancel(C ImagePtr &image) // canceling is needed to make sure we won't replace newer data with older
{
   REPA(_import_images)
   {
      ImportImage &ii=_import_images[i]; if(ii.image_ptr==image)
      {
         if(ii.done                                                                             // finished
         || _threads && _threads->cancel(ii, ImportImageFunc, T))_import_images.removeValid(i); // canceled, just remove
         else ii.image_ptr=null; // now processing, clear 'image_ptr' so we will ignore it
      }
   }
}
Bool InternetCache::busy()C
{
   if(_to_download.elms() || _to_verify.elms() || _import_images.elms())return true;
   REPA(_downloading)if(_downloading[i].state()!=DWNL_NONE)return true;
   return false;
}
inline void InternetCache::update()
{
   // update imported images
   REPA(_import_images)
   {
      ImportImage &ii=_import_images[i]; if(ii.done)
      {
         if(ii.image_ptr) // not canceled
         {
            Swap(*ii.image_ptr, ii.image_temp);
            if(got)got(ii.image_ptr);
         }
        _import_images.removeValid(i);
      }
   }

   // process downloaded data
   REPA(_downloading)
   {
      Download &down=_downloading[i]; switch(down.state())
      {
         case DWNL_NONE:
         {
         again:
            if(_to_download.elms()){down.create(_to_download.last()                       ); _to_download.removeLast();}else
            if(_to_verify  .elms()){down.create(_to_verify  .last(), null, null, -1, -1, 0); _to_verify  .removeLast();} // use offset as -1 to encode special mode of verification
         }break;

         case DWNL_DONE: // finished downloading
         {
            Str name=ShortName(down.url());
            if(down.offset()<0) // if this was verification
            {
               Flt        *verify_time=null;
               Downloaded * downloaded=_downloaded.find(name);
             C PakFile    *         pf=null;
               if(downloaded        ){if(downloaded->file_data.elms()==down.totalSize() && downloaded->modify_time_utc==down.modifyTimeUTC()){verify_time=& downloaded->verify_time;}}else
               if(pf=_pak.find(name)){if(        pf->data_size       ==down.totalSize() &&         pf->modify_time_utc==down.modifyTimeUTC()){verify_time=&pakFile(*pf).verify_time;}}

               if(verify_time)
               {
                 *verify_time=TIME;
                  // it's possible the image was not yet loaded due to CACHE_VERIFY_YES
                  ImagePtr img; if(img.find(down.url()))if(!img->is()) // if image empty, load it
                  {
                     cancel(img);
                     ImportImage &ii=_import_images.New();
                     if(downloaded)ii.data.set(downloaded->file_data.data(), downloaded->file_data.elms());
                     else          ii.data.set(*pf, _pak);
                     Swap(ii.image_ptr, img);
                     import(ii);
                  }
                  goto next;
               }
               down.create(Str(down.url())); // it's different so download it fully, copy the 'url' because it might get deleted in 'create'
            }else // move to 'downloaded'
            {
               Downloaded *downloaded=_downloaded(name);
               downloaded->file_data.setNumDiscard(down.size()).copyFrom((Byte*)down.data());
               downloaded->modify_time_utc=down.modifyTimeUTC();
               downloaded->access_time=downloaded->verify_time=TIME;

               if(_max_mem_size>=0)
               {
                  Long mem_size=0; REPA(_downloaded)mem_size+=_downloaded[i].file_data.elms();
                  if(  mem_size>_max_mem_size)
                  {
                     flush();
                     downloaded=null;
                  }
               }

               ImagePtr img; if(img.find(down.url())) // reload image
               {
                  cancel(img);

                C PakFile *pf=null;
                  if(!downloaded)
                  {
                                    downloaded=_downloaded.find(name);
                     if(!downloaded)pf        =_pak       .find(name);
                  }
                  if(downloaded || pf)
                  {
                     ImportImage &ii=_import_images.New();
                     Swap(ii.image_ptr, img);
                     if(downloaded)ii.data.set(downloaded->file_data.data(), downloaded->file_data.elms());
                     else          ii.data.set(*pf, _pak);
                     import(ii);
                  }
               }
               goto next;
            }
         }break;

         case DWNL_ERROR: next: down.del(); goto again; break; // failed, so just ignore it
      }
   }

   if(busy())enable();
}
static void ICUpdate(InternetCache &ic) {ic.update();}
void InternetCache::enable() {App.includeFuncCall(ICUpdate, T);}
/******************************************************************************/
}
/******************************************************************************/
