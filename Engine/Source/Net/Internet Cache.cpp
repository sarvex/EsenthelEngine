/******************************************************************************/
#include "stdafx.h"
/******************************************************************************

   TODO: '_missing' is not saved

/******************************************************************************/
#define COMPARE  ComparePathCI
#define TIME     Time.realTime() // use 'realTime' because sometimes it's mixed with 'curTime'
#define CC4_INCH CC4('I','N','C','H')
#define PRECISE_MISSING_ACCESS_TIME 0 // currently not needed
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
static void ImportImageFunc(InternetCache::ImportImage &ii, InternetCache &ic, Int thread_index=0)
{
   File temp; if(File *src=ii.data.open(temp))
   {
      ThreadMayUseGPUData(); // keep this covering even 'Import' in case the file is Engine 'Image' which can be IMAGE_2D
      ii.image_temp.Import(*src, -1, IMAGE_2D, ic._image_mip_maps);
      ThreadFinishedUsingGPUData();
   }else ii.fail=true;
         ii.done=true; // !! DON'T DO ANYTHING AFTER THIS STEP BECAUSE 'ii' OBJECT CAN GET REMOVED !!
}
Bool InternetCache::ImportImage::isPak       ()C {return data.type==DataSource::PAK_FILE;}
Bool InternetCache::ImportImage::isDownloaded()C {return data.type==DataSource::MEM     ;}
/******************************************************************************/
InternetCache::InternetCache() : _missing(COMPARE), _downloaded(COMPARE) {_pak_modify_time_utc.zero();}
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

static Int CompareName      (C SrcFile &a, C SrcFile &b) {return COMPARE(a.name       , b.name       );}
static Int CompareName      (C SrcFile &a, C Str     &b) {return COMPARE(a.name       , b            );}
static Int CompareAccessTime(C SrcFile &a, C SrcFile &b) {return Compare(b.access_time, a.access_time);} // reverse order to list files with biggest access time first

struct PostHeader : PakPostHeader
{
   Memc<SrcFile>  files;
   InternetCache &ic;

   PostHeader(InternetCache &ic) : ic(ic) {}

   virtual void save(File &f, C Pak &pak)override
   {
      f.putMulti(UInt(CC4_INCH), Byte(0)); // version
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
      if(ic._save)ic._save(f);
   }
};

static void ICUpdate(InternetCache &ic);
void InternetCache::enable() {App.includeFuncCall(ICUpdate, T);}

void InternetCache::del()
{
   REPAO(_downloading).stop();
   if(_threads)_threads->cancelFuncUser(ImportImageFunc, T); // cancel importing

   flush();

   if(_threads)
   {
     _threads->waitFuncUser(ImportImageFunc, T);
     _threads=null;
   }

   got=null;
  _save=_load=null;
  _missing             .del();
  _downloaded          .del();
  _import_images       .del();
  _to_download         .del();
  _to_verify           .del();
  _pak                 .del();
  _pak_size            =-1;
  _pak_modify_time_utc .zero();
  _pak_used_file_ranges.del();
  _pak_files           .del();
   App._callbacks.exclude(ICUpdate, T);

   REPAO(_downloading).del();
}
void InternetCache::create(C Str &name, Threads *threads, Cipher *cipher, COMPRESS_TYPE compress, void (*save)(File &f), void (*load)(File &f))
{
   del();

   if(D.canUseGPUDataOnSecondaryThread())_threads=threads; // setup worker threads only if we can operate on GPU on secondary threads
  _compress      =compress;
//_image_mip_maps=image_mip_maps;
  _save=save;
  _load=load;
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
                 _pak_files.setNumDiscard(_pak.totalFiles()); FREPA(_pak_files)
                  {
                     FileTime &file_time=_pak_files[i];
                     Flt access_time; Long verify_time; f.getMulti(access_time, verify_time);
                     file_time.access_time=Min(0, access_time    )     ; // prevent setting to the future in case of corrupt data
                     file_time.verify_time=Min(0, verify_time-now)+time; // prevent setting to the future in case of corrupt data
                  }
                  if(f.ok())
                  {
                     if(load)load(f);
                     getPakFileInfo();
                     return;
                  }
               }break;
            }
         }
      //_pak.del(); _pak_used_file_ranges.clear(); _pak_files.clear(); ignore because we recreate in 'reset'
      }
     _pak.pakFileName(name);
     _pak._file_cipher=cipher;
      reset();
   }
}
/******************************************************************************/
static void GetFileInfo(C Str &name, Long &size, DateTime &modify_time_utc)
{
   FileInfo fi; if(fi.getSystem(name) && fi.type==FSTD_FILE)
   {
      size           =fi.size;
      modify_time_utc=fi.modify_time_utc;
   }else
   {
      size           =-1;
      modify_time_utc.zero();
   }
}
void InternetCache::getPakFileInfo()
{
   GetFileInfo(_pak.pakFileName(), _pak_size, _pak_modify_time_utc);
}
void InternetCache::checkPakFileInfo()
{
            Long size;                DateTime modify_time_utc; GetFileInfo(_pak.pakFileName(), size, modify_time_utc);
   if(_pak_size!=size || _pak_modify_time_utc!=modify_time_utc)reset();
}
/******************************************************************************/
Bool InternetCache::flush()
{
 //if(_downloaded.elms()) always save because we need to save 'access_time' and 'verify_time' which can change without new '_downloaded'
   {
      if(_pak.pakFileName().is()) // we want to save data
      {
         checkPakFileInfo(); // check before waiting, because potential 'reset' might create '_import_images' as well, for which we need to wait

         PostHeader post_header(T); auto &files=post_header.files; Long file_size=0;
         FREPA(_downloaded)     {C Str &name=_downloaded.key(i); if(                           !_missing.find(name))file_size+=files.New().set(name, _downloaded[i]                   ).compressed_size;}
         FREPA(_pak       )if(i){  Str  name=_pak  .fullName(i); if(!_downloaded.find(name) && !_missing.find(name))file_size+=files.New().set(name, _pak, _pak.file(i), _pak_files[i]).compressed_size;} // skip post-header #PostHeaderFileIndex and files already included from '_downloaded'
         if(_max_file_size>=0 && file_size>_max_file_size) // limit file size
         {
            files.sort(CompareAccessTime);
            do{file_size-=files.last().compressed_size; files.removeLast();}while(file_size>_max_file_size && files.elms());
         }
         files.sort(CompareName); // needed for 'binaryFind' in 'SavePostHeader' and below

         if(_threads) // wait for Import Threads processing PAK files which we're about to update
         {
            Memt<Threads::Call> calls; REPA(_import_images){auto &ii=_import_images[i]; if(!ii.done && ii.isPak())calls.New().set(ii, ImportImageFunc, T);}
           _threads->wait(calls);
         }

         if(((_max_file_size<0 || _pak_size/2<_max_file_size) // if pak size is smaller than 2x limit "_pak_size<_max_file_size*2", allow some tolerance because 'replaceInPlace' does not generate compact paks but they may have holes
           && _pak.replaceInPlace(_pak_used_file_ranges, SCAST(Memc<PakFileData>, files), 0, _compress, _compress_level, null, null, &post_header)) // replace in place
         ||   _pak.replace       (_pak_used_file_ranges, SCAST(Memc<PakFileData>, files), 0, _compress, _compress_level, null, null, &post_header)) // recreate
         {
            getPakFileInfo();
           _pak_files.setNumDiscard(_pak.totalFiles());
            REPA(_pak_files)
            {
                   FileTime &file=_pak_files[i];
               if(C SrcFile *src =files.binaryFind(_pak.fullName(i), CompareName))
               {
                  file.access_time=src->access_time;
                  file.verify_time=src->verify_time;
               }else file.zero();
            }

            // delete '_downloaded' as its data was moved to PAK
            if(_threads) // wait for Import Threads processing Downloaded files which we're about to delete
            {
               Memt<Threads::Call> calls; REPA(_import_images){auto &ii=_import_images[i]; if(!ii.done && ii.isDownloaded())calls.New().set(ii, ImportImageFunc, T);}
              _threads->wait(calls);
            }
           _downloaded.del();
         }else return false;
      }else
      if(_max_mem_size>=0)
      {
         Memc<SrcFile> files; Long size=0;
         FREPA(_downloaded)size+=files.New().set(_downloaded.key(i), _downloaded[i]).compressed_size;
         if(size>_max_mem_size) // limit mem size
         {
            files.sort(CompareAccessTime);
            do
            {
               SrcFile &file=files.last();
               Downloaded *downloaded=_downloaded.find(file.name);
               if(_threads)REPA(_import_images){auto &ii=_import_images[i]; if(!ii.done && ii.data.type==DataSource::MEM && ii.data.memory==downloaded->file_data.data())_threads->wait(ii, ImportImageFunc, T);} // we're going to remove 'downloaded' so wait for any thread using its data to finish
              _downloaded.removeData(downloaded); // _downloaded.removeKey(file.name);
               size-=file.compressed_size;
               files.removeLast();
            }while(size>_max_mem_size && files.elms());
         }
      }
   }
   return true;
}
/******************************************************************************/
Bool InternetCache::missing(C Str &name)
{
   if(FileTime *missing=_missing.find(name))
   {
      if(TIME-missing->verify_time<=_verify_life) // verification still acceptable
      {
         missing->access_time=TIME;
         return true;
      }
     _missing.removeData(missing); // verification expired, status unknown, so just remove missing info
   }
   return false;
}
Bool InternetCache::getFile(C Str &url, DataSource &file, CACHE_VERIFY verify)
{
   file.set();
   if(!url.is())return true;
   Str name=SkipHttpWww(url); if(!name.is())return false;

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
      if(missing(name))return false;
      REPA(_downloading)if(EqualPath   (_downloading[i].url(), url))goto downloading;
                        if(_to_download.binaryInclude(url, COMPARE))enable();
   downloading:
      return false;
   }

   // found
   if(verify==CACHE_VERIFY_SKIP      )return true; // verification not   needed
   if(TIME-*verify_time<=_verify_life)return true; // verification still acceptable
   if(missing(name))return false;
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
void InternetCache::changed(C Str &url)
{
   if(url.is())
   {
      Str name=SkipHttpWww(url); if(name.is())
      {
      #if !PRECISE_MISSING_ACCESS_TIME
                             _missing   .removeKey(name      );
      #else
         if(FileTime   *miss=_missing   .find     (name      ))miss       ->verify_time=INT_MIN;
      #endif
         if(Downloaded *down=_downloaded.find     (name      ))down       ->verify_time=INT_MIN;
         if(C PakFile  *pf  =_pak       .find     (name, true))pakFile(*pf).verify_time=INT_MIN;
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
         if(Images.has(url)) // download if currently referenced
         {
           _to_download.binaryInclude(url, COMPARE); enable();
         }
      }
   }
}
/******************************************************************************/
void InternetCache::import(ImportImage &ii)
{
   if(_threads)_threads->queue(ii, ImportImageFunc, T);else ImportImageFunc(ii, T);
}
Bool InternetCache::busy()C
{
   if(_to_download.elms() || _to_verify.elms() || _import_images.elms())return true;
   REPA(_downloading)if(_downloading[i].state()!=DWNL_NONE)return true;
   return false;
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
void InternetCache::cancelWait(Ptr data)
{
   if(data)
   REPA(_import_images)
   {
      ImportImage &ii=_import_images[i]; if(ii.data.type==DataSource::MEM && ii.data.memory==data)
      {
         if(!ii.done  // not yet finished
         &&  _threads // have threads
         && !_threads->cancel(ii, ImportImageFunc, T)) // couldn't cancel
             _threads->wait  (ii, ImportImageFunc, T); // wait for finish
        _import_images.removeValid(i); // remove
      }
   }
}
void InternetCache::reset()
{
   if(_threads)
   {
     _threads->cancelFuncUser(ImportImageFunc, T); // cancel   all
     _threads->  waitFuncUser(ImportImageFunc, T); // wait for all
   }
   Memc<Str> retry;
   REPA(_import_images)
   {
      ImportImage &ii=_import_images[i]; if(ii.image_ptr) // not canceled
      {
         if(ii.done && !ii.fail) // success
         {
            Swap(*ii.image_ptr, ii.image_temp);
            if(got)got(ii.image_ptr);
         }else retry.add(ii.image_ptr.name());
      }
   }
  _import_images.clear();

   PostHeader post_header(T);
  _pak.create(CMemPtr<PakFileData>(), _pak.pakFileName(), 0, _pak._file_cipher, COMPRESS_NONE, 0, null, null, null, _pak_used_file_ranges, &post_header); // create an empty pak
  _pak_files.setNumDiscard(_pak.totalFiles()); REPAO(_pak_files).zero();
   getPakFileInfo();

   if(retry.elms())
   {
      Bool enable=false;
      CACHE_MODE mode=Images.mode(CACHE_DUMMY);
      REPA(retry)
      {
       C Str &url=retry[i];
         DataSource file; if(getFile(url, file, CACHE_VERIFY_SKIP)) // always call 'getFile' to adjust 'access_time' and request verification if needed
         {
            ImportImage &ii=_import_images.New();
            Swap(ii.data, file);
            ii.image_ptr=url;
            import(ii);
            enable=true;
         }
      }
      Images.mode(mode);
      if(enable)T.enable();
   }
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
            if(ii.fail){reset(); break;} // if failed to open file, then we have to reset, break because 'reset' will handle '_import_images'

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
            if(_to_download.elms()){down.create(_to_download.last()                   ); _to_download.removeLast();}else
            if(_to_verify  .elms()){down.create(_to_verify  .last(), null, null, -1, 0); _to_verify  .removeLast();} // use offset as -1 to encode special mode of verification
         }break;

         case DWNL_DONE: // finished downloading
         {
            Str name=SkipHttpWww(down.url());
      #if PRECISE_MISSING_ACCESS_TIME
           _missing.removeKey(name);
      #endif
            if(down.offset()<0) // if this was verification
            {
               Flt        *verify_time=null;
               Downloaded * downloaded=_downloaded.find(name);
             C PakFile    *         pf=null;
               if(downloaded               ){if(downloaded->file_data.elms()==down.totalSize() && downloaded->modify_time_utc==down.modifyTimeUTC()){verify_time=& downloaded->verify_time;}}else
               if(pf=_pak.find(name, false)){if(        pf->data_size       ==down.totalSize() &&         pf->modify_time_utc==down.modifyTimeUTC()){verify_time=&pakFile(*pf).verify_time;}}

               if(verify_time)
               {
                 *verify_time=TIME;
                  // it's possible the image was not yet loaded due to CACHE_VERIFY_YES
                  ImagePtr img; if(img.find(down.url()))if(!img->is()) // if image empty, load it
                  {
                     REPA(_import_images)if(_import_images[i].image_ptr==img)goto next; // first check if it's importing already, but just not yet finished
                     // if not yet importing, then import
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
               Bool just_created;
               Downloaded *downloaded=_downloaded(name, just_created);
               cancelWait(downloaded->file_data.data()); // we're going to modify 'downloaded->file_data' so we have to make sure no importers are using that data
               downloaded->file_data.setNumDiscard(down.done()).copyFrom((Byte*)down.data());
               downloaded->modify_time_utc=down.modifyTimeUTC();
               downloaded->verify_time=TIME;
               if(just_created)downloaded->access_time=downloaded->verify_time;

               ImagePtr img; img.find(down.url());

               if(_max_mem_size>=0)
               {
                  Long mem_size=0; REPA(_downloaded)mem_size+=_downloaded[i].file_data.elms();
                  if(  mem_size>_max_mem_size)
                  {
                     // TODO: if we're going to reload image if(img) then prevent 'downloaded' from being removed in flush: do Downloaded *except=downloaded; or downloaded.access_time=; or copy memory to importer from 'down'
                     flush();
                     downloaded=null;
                  }
               }

               if(img) // reload image
               {
                C PakFile *pf=null;
                  if(!downloaded)
                  {
                                    downloaded=_downloaded.find(name);
                     if(!downloaded)pf        =_pak       .find(name, false);
                  }
                  if(downloaded || pf)
                  {
                     cancel(img);
                     ImportImage &ii=_import_images.New();
                     if(downloaded)ii.data.set(downloaded->file_data.data(), downloaded->file_data.elms());
                     else          ii.data.set(*pf, _pak);
                     Swap(ii.image_ptr, img);
                     import(ii);
                  }
               }
               goto next;
            }
         }break;

         case DWNL_ERROR: // failed
         {
            if(down.code()==404) // confirmed that file is missing
            {
               Str name=SkipHttpWww(down.url());
               Bool just_created;
               FileTime &missing=*_missing(name, just_created);
               missing.verify_time=TIME;
               if(just_created)
               {
                  missing.access_time=missing.verify_time;
                  ImagePtr img; if(img.find(down.url())) // delete image
                  {
                     cancel(img);
                     if(img->is())
                     {
                        img->del();
                        if(got)got(img);
                     }
                  }
               }
            }
            next: down.del(); goto again;
         }break;
      }
   }

   if(busy())enable();
}
static void ICUpdate(InternetCache &ic) {ic.update();}
/******************************************************************************/
}
/******************************************************************************/
