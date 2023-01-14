/******************************************************************************/
enum CACHE_VERIFY : Byte
{
   CACHE_VERIFY_YES  , // always verify before returning cached result
   CACHE_VERIFY_DELAY, // allow returning cached result before verification completed
   CACHE_VERIFY_SKIP , // always skip verification
#if EE_PRIVATE
   CACHE_VERIFY_EXPIRED, // internal mode that always treats downloaded file as expired, forcing verification, and preventing from returning downloaded file
#endif
};
struct DataSourceTime : DataSource
{
   DateTime modify_time_utc;

   void zero() {super::zero(); modify_time_utc.zero();}
};
struct InternetCache
{
   void (*got)(C ImagePtr &image); // pointer to custom function called when cache has finished downloading and importing an image

   // manage
   void del   (); // delete manually
   void create(C Str &name, const_mem_addr Threads *threads=null, const_mem_addr Cipher *cipher=null, COMPRESS_TYPE compress=COMPRESS_LZ4, void (*save)(File &f)=null, void (*load)(File &f)=null); // create, 'name'=file name where the cache will be located, 'threads'=worker threads that will import the images (if null then importing will be done on the main thread), 'cipher'=cache file encryption, 'compress'=cache file compression, 'save' 'load' = callbacks allowing to save/load custom data

   // operations
   Bool     loading (C ImagePtr &image                                                     )C; // if image is still downloading or importing
   ImagePtr getImage(C Str &url                      , CACHE_VERIFY verify=CACHE_VERIFY_YES) ; // get image from the internet, image may be empty at start if it's not yet downloaded, it will be automatically updated once it completes downloading
   Bool     getFile (C Str &url, DataSourceTime &file, CACHE_VERIFY verify=CACHE_VERIFY_YES) ; // get file  from the internet, 'file' will contain a way to access this file, false is returned if file is not yet available and will be downloaded
   void     changed (C Str &url                                                            ) ; // notify the cache that the file on the internet has just been changed and needs updating
   Bool     flush   (                                                                      ) ; // flush updated files to disk, warning: this may invalidate all files obtained using 'getFile', false on fail

  ~InternetCache() {del();}
   InternetCache();

   // do not use
   static Int LOD(C Image &image);
   struct Lod
   {
      Int min, max;
   };
   Str (*image_lod_to_url)(C Str &name, Int  lod);
   Str (*url_to_image_lod)(C Str &url , Int &lod);
   Bool(*    is_image_lod)(C Str &name, Lod &lod);
   ImagePtr getImageLOD(C Str &name, Int lod, CACHE_VERIFY verify=CACHE_VERIFY_YES);

#if !EE_PRIVATE
private:
#endif
   struct ImportImage
   {
      enum TYPE : Byte
      {
         PAK       , // data comes from Pak File
         DOWNLOADED, // data comes from 'Downloaded.file_data'
         OTHER     , // data comes from 'temp' or importer thread stack memory
      };
      Bool           done=false, // if finished importing
                     fail=false; // if failed to open file
      SByte          lod=-1;
      TYPE           type;
      DataSourceTime data;
      ImagePtr       image_ptr;  // image into which import
      Image          image_temp; // temp image which will have the data
      Mems<Byte>     temp;

      // here don't check for 'data.type' because we need one single variable that's changed at the end once everything is ready
      Bool isPak       ()C {return type==PAK       ;}
      Bool isDownloaded()C {return type==DOWNLOADED;}

      void lockedRead();
      void import(InternetCache &ic);
   };
   struct FileTime
   {
      Flt access_time, verify_time;

      void zero() {access_time=-FLT_MAX; verify_time=INT_MIN;}
   };
   struct Downloaded : FileTime
   {
      Mems<Byte> file_data;
      DateTime   modify_time_utc;
   };

   COMPRESS_TYPE        _compress;
   Byte                 _compress_level;
   SByte                _image_mip_maps; // number of mip maps to be created when importing images (-1..Inf, -1=keep original value, 0=autodetect)
   Flt                  _verify_life;
   Int                  _max_missing;
   Long                 _max_file_size, _max_mem_size;
   Threads             *_threads;
   ReadWriteSync        _rws;
   Pak                  _pak;
   Long                 _pak_size;
   DateTime             _pak_modify_time_utc;
   Memb<DataRangeAbs>   _pak_used_file_ranges;
   Mems<FileTime>       _pak_files;
   Map<Str, FileTime>   _missing;
   Map<Str, Downloaded> _downloaded;
   Download             _downloading[6];
   Memc<Str>            _to_download, _to_verify;
   Memx<ImportImage>    _import_images; // use 'Memx' to have const_mem_addr needed for threads
   void               (*_save)(File &f);
   void               (*_load)(File &f);

#if EE_PRIVATE
   enum GET : Byte
   {
      NONE       , // no  file, not downloading
      DOWNLOADING, // no  file,     downloading
      FILE       , // got file
   };
   GET  getFileEx(C Str &url, DataSourceTime &file, CACHE_VERIFY verify=CACHE_VERIFY_YES, Bool access_download=true); // get file  from the internet, 'file' will contain a way to access this file, false is returned if file is not yet available and will be downloaded, 'access_download'=if adjust access time and allow download
   Bool busy  ()C;
   void enable();
   void update();
   void import  (ImportImage &ii);
   void cancel  (ImportImage &ii);
   void cancel  (C ImagePtr &image);
   void updating(Ptr data);
   FileTime& pakFile(C PakFile &pf) {return _pak_files[_pak.files().index(&pf)];}
   void   getPakFileInfo();
   void checkPakFileInfo();
   void resetPak(WriteLockEx *lock=null);
   void cleanMissing();
   Bool flush(Downloaded *keep, Mems<Byte> *keep_data);
   void zero();
#endif
};
/******************************************************************************/
