/******************************************************************************/
#include "stdafx.h"

#if SUPPORT_JPG && !SWITCH
   #include "../../../../ThirdPartyLibs/begin.h"

   #undef EXTERN
   #include <setjmp.h>
   #if 1 // JpegTurbo
      #include "../../../../ThirdPartyLibs/JpegTurbo/lib/jpeglib.h"
   #else // Jpeg
      #include "../../../../ThirdPartyLibs/Jpeg/jpeglib.h"
   #endif

   #include "../../../../ThirdPartyLibs/end.h"
#endif

namespace EE{
/******************************************************************************/
#if !SWITCH
#if SUPPORT_JPG
struct jpeg_error_mgr_ex : jpeg_error_mgr
{
   jmp_buf jump_buffer;
};
static void my_error_exit(j_common_ptr cinfo)
{
   jpeg_error_mgr_ex &err=*(jpeg_error_mgr_ex*)cinfo->err;
 //(*cinfo->err->output_message)(cinfo);
   longjmp(err.jump_buffer, 1);
}

#define EXIF_JPEG_MARKER   JPEG_APP0+1
#define EXIF_IDENT_STRING  "Exif\000\000"

static const Byte LETH[]={0X49, 0X49, 0X2A, 0X00}; // Little endian TIFF header
static const Byte BETH[]={0X4D, 0X4D, 0X00, 0X2A}; // Big    endian TIFF header

static U16 Get16(void *ptr, Bool big_endian)
{
   U16 val=*(U16*)ptr;
   if(big_endian)SwapEndian(val);
   return val;
}
static U32 Get32(void *ptr, Bool big_endian)
{
   U32 val=*(U32*)ptr;
   if(big_endian)SwapEndian(val);
   return val;
}
static Int Orientation(j_decompress_ptr cinfo)
{
   for(jpeg_saved_marker_ptr marker=cinfo->marker_list; marker; marker=marker->next)
      if(marker->marker==EXIF_JPEG_MARKER
      && marker->data_length>=32
      && !memcmp(marker->data, EXIF_IDENT_STRING, 6))
   for(UInt i=0; i<16; i++)
   { /* Skip data until TIFF header - it should be within 16 bytes from marker start.
        Normal structure relative to APP1 marker -
        0x0000: APP1 marker entry = 2 bytes
        0x0002: APP1 length entry = 2 bytes
        0x0004: Exif Identifier entry = 6 bytes
        0x000A: Start of TIFF header (Byte order entry) - 4 bytes - This is what we look for, to determine endianess.
        0x000E: 0th IFD offset pointer - 4 bytes

        marker->data points to the first data after the APP1 marker
        and length entries, which is the exif identification string.
        The TIFF header should thus normally be found at i=6, below,
        and the pointer to IFD0 will be at 6+4 = 10 */
      Bool big_endian;
      if(!memcmp(&marker->data[i], LETH, 4))big_endian=false;else // Little endian TIFF header
      if(!memcmp(&marker->data[i], BETH, 4))big_endian=true ;else // Big    endian TIFF header
         continue;
      // We have found either big or little endian TIFF header
      U16 orient_tag_id=0x112; if(big_endian)SwapEndian(orient_tag_id); // Endian the orientation tag ID, to locate it more easily
      i+=Get32(&marker->data[i]+4, big_endian); // Read out the offset pointer to IFD0
      if(i+2>marker->data_length)return 0; // Check that we still are within the buffer and can read the tag count
      U16 tags=Get16(&marker->data[i], big_endian); // Find out how many tags we have in IFD0. As per the TIFF spec, the first two bytes of the IFD contain a count of the number of tags.
      i+=2;
      if(i+tags*12>marker->data_length)return 0; // Check that we still have enough data for all tags to check. The tags are listed in consecutive 12-byte blocks. The tag ID, type, size, and a pointer to the actual value, are packed into these 12 byte entries
      while(tags--) // Check through IFD0 for tags of interest
      {
         U16  type =Get16(&marker->data[i+2], big_endian);
         UInt count=Get32(&marker->data[i+4], big_endian);
         if(!memcmp(&marker->data[i], &orient_tag_id, 2)) // Is this the orientation tag?
         {
            if(type!=3 || count!=1)return 0; // Check that type and count fields are OK. The orientation field will consist of a single (count=1) 2-byte integer (type=3)
            U16 ret=Get16(&marker->data[i+8], big_endian); // Return the orientation value. Within the 12-byte block, the pointer to the actual data is at offset 8
            return ret<=8 ? ret : 0;
         }
         i+=12; // move the pointer to the next 12-byte tag field
      }
      break;
   }
   return 0;
}
#if 0
#define INPUT_VARS(cinfo)  \
        struct jpeg_source_mgr *datasrc = (cinfo)->src;  \
        const JOCTET *next_input_byte = datasrc->next_input_byte;  \
        size_t bytes_in_buffer = datasrc->bytes_in_buffer

#define INPUT_SYNC(cinfo)  \
        ( datasrc->next_input_byte = next_input_byte,  \
          datasrc->bytes_in_buffer = bytes_in_buffer )

#define INPUT_RELOAD(cinfo)  \
        ( next_input_byte = datasrc->next_input_byte,  \
          bytes_in_buffer = datasrc->bytes_in_buffer )

#define MAKE_BYTE_AVAIL(cinfo,action)  \
        if (bytes_in_buffer == 0) {  \
          if (! (*datasrc->fill_input_buffer) (cinfo))  \
            { action; }  \
          INPUT_RELOAD(cinfo);  \
        }

#define INPUT_2BYTES(cinfo,V,action)  \
        MAKESTMT( MAKE_BYTE_AVAIL(cinfo,action); \
                  bytes_in_buffer--; \
                  V = ((unsigned int) GETJOCTET(*next_input_byte++)) << 8; \
                  MAKE_BYTE_AVAIL(cinfo,action); \
                  bytes_in_buffer--; \
                  V += GETJOCTET(*next_input_byte++); )

static boolean MarkerParser(j_decompress_ptr cinfo)
{
   JLONG length;
   INPUT_VARS(cinfo);

   INPUT_2BYTES(cinfo, length, return FALSE);
   length -= 2;

   TRACEMS2(cinfo, 1, JTRC_MISC_MARKER, cinfo->unread_marker, (int) length);

   INPUT_SYNC(cinfo);            /* do before skip_input_data */
   if (length > 0)
   (*cinfo->src->skip_input_data) (cinfo, (long) length);

   return TRUE;
}
#endif
#endif
/******************************************************************************/
Bool Image::ImportJPG(File &f)
{
#if SUPPORT_JPG
   struct JPGReader : jpeg_source_mgr
   {
      File &f;
      Byte  data[4096];
      Bool  start_of_file;
      Long  start_pos;

      static void InitSource(j_decompress_ptr cinfo)
      {
         JPGReader &src=*(JPGReader*)cinfo->src;
         src.start_of_file=true;
      }
      static boolean FillInputBuffer(j_decompress_ptr cinfo)
      {
         JPGReader &src   =*(JPGReader*)cinfo->src;
         Int        nbytes=src.f.getReturnSize(src.data, SIZE(src.data));
         if(nbytes<=0) // if no more data available
         {
            if(src.start_of_file)return false; // if just started reading then fail
            // otherwise insert a fake EOI marker, and allowing reading incomplete JPG files
            src.data[0]=(JOCTET)0xFF;
            src.data[1]=(JOCTET)JPEG_EOI;
            nbytes=2;
         }
         src.next_input_byte=src.data;
         src.bytes_in_buffer=nbytes;
         src.start_of_file  =false;
         return true;
      }
      static void SkipInputData(j_decompress_ptr cinfo, long num_bytes)
      {
         JPGReader &src=*(JPGReader*)cinfo->src;
         if(num_bytes>0)
         {
            while(num_bytes>src.bytes_in_buffer)
            {
               num_bytes-=(long)src.bytes_in_buffer;
               FillInputBuffer(cinfo);
            }
            src.next_input_byte+=num_bytes;
            src.bytes_in_buffer-=num_bytes;
         }
      }
      static void TermSource(j_decompress_ptr cinfo) {}

      JPGReader(File &f) : f(f)
      {
         init_source      =InitSource;
         fill_input_buffer=FillInputBuffer;
         skip_input_data  =SkipInputData;
         resync_to_restart=jpeg_resync_to_restart; // use libJpeg method
         term_source      =TermSource;
         bytes_in_buffer  =0;
         next_input_byte  =null;
         start_pos        =f.pos();
      }
     ~JPGReader()
      {
         f.pos(Max(start_pos, f.pos()-(Int)bytes_in_buffer)); // go back with any unprocessed data left, to be able to read them after reading the JPEG (this is needed because 'FillInputBuffer' reads ahead of what's needed)
      }
   };

   	Bool  ok=false, created=false;
      JPGReader                  jpg(f);
      jpeg_error_mgr_ex          jerr ;
      jpeg_decompress_struct     cinfo; cinfo.err=jpeg_std_error(&jerr); jerr.error_exit=my_error_exit; if(setjmp(jerr.jump_buffer))goto error; // setup jump position that will be reached upon jpeg error
      jpeg_create_decompress   (&cinfo); cinfo.src=&jpg;
   #if 0 // needed for orientation
      jpeg_set_marker_processor(&cinfo, EXIF_JPEG_MARKER, MarkerParser);
   #else
      jpeg_save_markers        (&cinfo, EXIF_JPEG_MARKER, USHORT_MAX);
   #endif
   if(jpeg_read_header         (&cinfo, true))
   {
   	jpeg_start_decompress(&cinfo);

      if(cinfo.output_components==1 && cinfo.out_color_space==JCS_GRAYSCALE
      || cinfo.output_components==3 && cinfo.out_color_space==JCS_RGB)
         if(createSoft(cinfo.output_width, cinfo.output_height, 1, (cinfo.output_components==1) ? IMAGE_L8_SRGB : IMAGE_R8G8B8_SRGB))
      {
         created=true;
         for(; cinfo.output_scanline<cinfo.output_height; ){JSAMPROW row=T.data()+cinfo.output_scanline*pitch(); jpeg_read_scanlines(&cinfo, &row, 1);}
         switch(Orientation(&cinfo)) // http://sylvana.net/jpegcrop/exif_orientation.html   https://sirv.com/help/articles/rotate-photos-to-be-upright/
         {
            case 2: mirrorX (); break;
            case 3: mirrorXY(); break;
            case 4: mirrorY (); break;
            case 5: rotateR ().mirrorX(); break; // TODO: optimize
            case 6: rotateR (); break;
            case 7: rotateL ().mirrorX(); break; // TODO: optimize
            case 8: rotateL (); break;
         }
         ok=true;
      }
   	jpeg_finish_decompress(&cinfo);
   }
error:
   jpeg_destroy_decompress(&cinfo);
   if(!ok && !created)del(); return ok; // if failed but the image was created, then it's possible that some data was read, keep that in case user wants to preview what was read
#else
   del(); return false;
#endif
}
/******************************************************************************/
Bool Image::ExportJPG(File &f, Flt quality, Int sub_sample)C
{
#if SUPPORT_JPG
   struct JPGWriter : jpeg_destination_mgr
   {
      File &f;
      Byte  data[4096];

      static void InitDestination(j_compress_ptr cinfo)
      {
         JPGWriter &dest=*(JPGWriter*)cinfo->dest;
         dest.next_output_byte=dest.data;
         dest.free_in_buffer  =SIZE(dest.data);
      }
      static boolean EmptyOutputBuffer(j_compress_ptr cinfo)
      {
         JPGWriter &dest=*(JPGWriter*)cinfo->dest;
         dest.f<<dest.data;
         dest.next_output_byte=dest.data;
         dest.free_in_buffer  =SIZE(dest.data);
         return true;
      }
      static void TermDestination(j_compress_ptr cinfo)
      {
         JPGWriter &dest=*(JPGWriter*)cinfo->dest;
         dest.f.put(dest.data, SIZE(dest.data)-(Int)dest.free_in_buffer);
      }

      JPGWriter(File &f) : f(f)
      {
         next_output_byte   =null;
         free_in_buffer     =0;
         init_destination   =InitDestination;
         empty_output_buffer=EmptyOutputBuffer;
         term_destination   =TermDestination;
      }
   };

   Int q=RoundPos(quality*100); if(q<0)q=80;else Clamp(q, 1, 100); // default to 80, min is 1, max is 100

 C Image *src=this;
   Image temp;

   if(!src->is  ())return false;
   if( src->cube())if(temp.fromCube(*src, IMAGE_R8G8B8_SRGB))src=&temp;else return false; // JPEG doesn't have alpha

   if(src->hwType()!=IMAGE_L8 && src->hwType()!=IMAGE_L8_SRGB && src->hwType()!=IMAGE_A8 && src->hwType()!=IMAGE_I8
   && src->hwType()!=IMAGE_R8G8B8
   && src->hwType()!=IMAGE_R8G8B8_SRGB
#if JCS_EXTENSIONS
   && src->hwType()!=IMAGE_R8G8B8A8
   && src->hwType()!=IMAGE_R8G8B8A8_SRGB
   && src->hwType()!=IMAGE_B8G8R8
   && src->hwType()!=IMAGE_B8G8R8_SRGB
   && src->hwType()!=IMAGE_B8G8R8A8
   && src->hwType()!=IMAGE_B8G8R8A8_SRGB
#endif
   )
      if(src->copy(temp, -1, -1, -1, (src->type()==IMAGE_I16) ? IMAGE_L8_SRGB : IMAGE_R8G8B8_SRGB, IMAGE_SOFT, 1))src=&temp;else return false;

   if(src->lockRead())
   {
      JPGWriter             jpg(f);
      jpeg_error_mgr_ex     jerr ;
      jpeg_compress_struct  cinfo; cinfo.err=jpeg_std_error(&jerr); jerr.error_exit=my_error_exit; if(setjmp(jerr.jump_buffer)){jpeg_destroy_compress(&cinfo); return false;} // setup jump position that will be reached upon jpeg error
      jpeg_create_compress(&cinfo);

      switch(src->hwType())
      {
         case IMAGE_A8      :
         case IMAGE_I8      :                    
         case IMAGE_L8      : case IMAGE_L8_SRGB      : cinfo.in_color_space=JCS_GRAYSCALE; cinfo.input_components=1; break;

         case IMAGE_R8G8B8  : case IMAGE_R8G8B8_SRGB  : cinfo.in_color_space=JCS_RGB      ; cinfo.input_components=3; break;
      #if JCS_EXTENSIONS
         case IMAGE_R8G8B8A8: case IMAGE_R8G8B8A8_SRGB: cinfo.in_color_space=JCS_EXT_RGBX ; cinfo.input_components=4; break;
         case IMAGE_B8G8R8  : case IMAGE_B8G8R8_SRGB  : cinfo.in_color_space=JCS_EXT_BGR  ; cinfo.input_components=3; break;
         case IMAGE_B8G8R8A8: case IMAGE_B8G8R8A8_SRGB: cinfo.in_color_space=JCS_EXT_BGRX ; cinfo.input_components=4; break;
      #endif
      }
      cinfo.image_width =src->w();
      cinfo.image_height=src->h();
      cinfo.dest        =&jpg;

      jpeg_set_defaults(&cinfo);

      if(cinfo.comp_info && cinfo.in_color_space!=JCS_GRAYSCALE)switch(sub_sample)
      {
                  case 0: cinfo.comp_info[0].h_samp_factor=1; cinfo.comp_info[0].v_samp_factor=1; break;
                  case 1: cinfo.comp_info[0].h_samp_factor=2; cinfo.comp_info[0].v_samp_factor=1; break;
         default: case 2: cinfo.comp_info[0].h_samp_factor=2; cinfo.comp_info[0].v_samp_factor=2; break;
      }

      jpeg_set_quality     (&cinfo, q, true);
      jpeg_start_compress  (&cinfo, true); for(; cinfo.next_scanline<cinfo.image_height; ){JSAMPROW data=ConstCast(src->data()+cinfo.next_scanline*src->pitch()); jpeg_write_scanlines(&cinfo, &data, 1);}
      jpeg_finish_compress (&cinfo);
      jpeg_destroy_compress(&cinfo);

      src->unlock();
      return f.ok();
   }
#endif
   return false;
}
#endif
/******************************************************************************/
Bool Image::ExportJPG(C Str &name, Flt quality, Int sub_sample)C
{
#if SUPPORT_JPG
   File f; if(f.write(name)){if(ExportJPG(f, quality, sub_sample) && f.flush())return true; f.del(); FDelFile(name);}
#endif
   return false;
}
Bool Image::ImportJPG(C Str &name)
{
#if SUPPORT_JPG
   File f; if(f.read(name))return ImportJPG(f);
#endif
   del(); return false;
}
/******************************************************************************/
}
/******************************************************************************/
