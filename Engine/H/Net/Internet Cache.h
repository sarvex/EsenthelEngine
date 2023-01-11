/******************************************************************************/
enum CACHE_VERIFY : Byte
{
   CACHE_VERIFY_YES  , // always verify before returning cached result
   CACHE_VERIFY_DELAY, // allow returning cached result before verification completed
   CACHE_VERIFY_SKIP , // always skip verification
};
struct InternetCache
{
   void (*got)(C ImagePtr &image)=null; // pointer to custom function called when cache has finished downloading and importing an image

   // manage
   void del   (); // delete manually
   void create(C Str &name, const_mem_addr Threads *threads=null, const_mem_addr Cipher *cipher=null, COMPRESS_TYPE compress=COMPRESS_LZ4, void (*save)(File &f)=null, void (*load)(File &f)=null); // create, 'name'=file name where the cache will be located, 'threads'=worker threads that will import the images (if null then importing will be done on the main thread), 'cipher'=cache file encryption, 'compress'=cache file compression, 'save' 'load' = callbacks allowing to save/load custom data

   // operations
   ImagePtr getImage(C Str &url                  , CACHE_VERIFY verify=CACHE_VERIFY_YES); // get image from the internet, image may be empty at start if it's not yet downloaded, it will be automatically updated once it completes downloading
   Bool     getFile (C Str &url, DataSource &file, CACHE_VERIFY verify=CACHE_VERIFY_YES); // get file  from the internet, 'file' will contain a way to access this file, false is returned if file is not yet available and will be downloaded
   void     changed (C Str &url                                                        ); // notify the cache that the file on the internet has just been changed and needs updating
   Bool     flush   (                                                                  ); // flush updated files to disk, warning: this may invalidate all files obtained using 'getFile', false on fail

  ~InternetCache() {del();}
   InternetCache();

#if !EE_PRIVATE
private:
#endif
   struct ImportImage
   {
      Bool       done=false, // if finished importing
                 fail=false; // if failed to open file
      DataSource data;
      ImagePtr   image_ptr;  // image into which import
      Image      image_temp; // temp image which will have the data
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

   COMPRESS_TYPE        _compress=COMPRESS_NONE;
   Byte                 _compress_level=255;
   SByte                _image_mip_maps=0; // number of mip maps to be created when importing images (-1..Inf, -1=keep original value, 0=autodetect)
   Flt                  _verify_life=60*60;
   Long                 _max_file_size=512<<20, _max_mem_size=16<<20;
   Threads             *_threads=null;
   Pak                  _pak;
   Long                 _pak_size=-1;
   DateTime             _pak_modify_time_utc;
   Memb<DataRangeAbs>   _pak_used_file_ranges;
   Mems<FileTime>       _pak_files;
   Map<Str, FileTime>   _missing;
   Map<Str, Downloaded> _downloaded;
   Download             _downloading[6];
   Memc<Str>            _to_download, _to_verify;
   Memx<ImportImage>    _import_images; // use 'Memx' to have const_mem_addr needed for threads
   void               (*_save)(File &f)=null;
   void               (*_load)(File &f)=null;

#if EE_PRIVATE
   Bool busy  ()C;
   void enable();
   void update();
   void import(ImportImage &ii);
   void cancel    (C ImagePtr &image);
   void cancelWait(Ptr data);
   FileTime& pakFile(C PakFile &pf) {return _pak_files[_pak.files().index(&pf)];}
   void   getPakFileInfo();
   void checkPakFileInfo();
   void reset();
   Bool missing(C Str &name);
#endif
};
/******************************************************************************/
