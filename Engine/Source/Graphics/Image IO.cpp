/******************************************************************************

   TODO: This could use some mem manager / ring buffer for streamed data to avoid frequent allocations

/******************************************************************************/
#include "stdafx.h"
namespace EE{
/******************************************************************************/
#define IMAGE_STREAM_FULL GPU_API(true, false) // load only small image, but stream the entire full image and replace the small with the full one
/******************************************************************************/
#define CC4_IMG CC4('I','M','G',0)
#define CC4_GFX CC4('G','F','X',0)
/******************************************************************************/
Bool ImageHeader::is()C
{
   return size.x>0 && size.y>0 && size.z>0 && mip_maps>0 && type;
}
/******************************************************************************/
static IMAGE_TYPE OldImageType3(Byte type)
{
   static const IMAGE_TYPE types[]=
   {
      IMAGE_NONE, IMAGE_R8G8B8A8, IMAGE_R8G8B8A8_SRGB, IMAGE_R8G8B8, IMAGE_R8G8B8_SRGB, IMAGE_R8G8, IMAGE_R8,
      IMAGE_A8, IMAGE_L8, IMAGE_L8_SRGB, IMAGE_L8A8, IMAGE_L8A8_SRGB,
      IMAGE_R10G10B10A2,
      IMAGE_I8, IMAGE_I16, IMAGE_I24, IMAGE_I32,
      IMAGE_F16, IMAGE_F32, IMAGE_F16_2, IMAGE_F32_2, IMAGE_F16_3, IMAGE_F32_3, IMAGE_F16_4, IMAGE_F32_4, IMAGE_F32_3_SRGB, IMAGE_F32_4_SRGB,
      IMAGE_BC1, IMAGE_BC1_SRGB, IMAGE_BC2, IMAGE_BC2_SRGB, IMAGE_BC3, IMAGE_BC3_SRGB, IMAGE_BC4, IMAGE_BC5, IMAGE_BC6, IMAGE_BC7, IMAGE_BC7_SRGB,
      IMAGE_ETC2_RGB, IMAGE_ETC2_RGB_SRGB, IMAGE_ETC2_RGBA1, IMAGE_ETC2_RGBA1_SRGB, IMAGE_ETC2_RGBA, IMAGE_ETC2_RGBA_SRGB,
      IMAGE_PVRTC1_2, IMAGE_PVRTC1_2_SRGB, IMAGE_PVRTC1_4, IMAGE_PVRTC1_4_SRGB,
   };
   return InRange(type, types) ? types[type] : IMAGE_NONE;
}
static IMAGE_TYPE OldImageType2(Byte type)
{
   static const IMAGE_TYPE types[]=
   {
      IMAGE_NONE, IMAGE_B8G8R8A8_SRGB, IMAGE_R8G8B8A8_SRGB, IMAGE_R8G8B8_SRGB, IMAGE_R8G8, IMAGE_R8,
      IMAGE_A8, IMAGE_L8_SRGB, IMAGE_L8A8_SRGB,
      IMAGE_BC1_SRGB, IMAGE_BC2_SRGB, IMAGE_BC3_SRGB,
      IMAGE_I8, IMAGE_I16, IMAGE_I24, IMAGE_I32, IMAGE_F16, IMAGE_F32, IMAGE_F16_2, IMAGE_F32_2, IMAGE_F16_3, IMAGE_F32_3, IMAGE_F16_4, IMAGE_F32_4,
      IMAGE_PVRTC1_2_SRGB, IMAGE_PVRTC1_4_SRGB,

      IMAGE_ETC1, IMAGE_ETC2_RGB_SRGB, IMAGE_ETC2_RGBA1_SRGB, IMAGE_ETC2_RGBA_SRGB,
      IMAGE_BC7_SRGB,
      IMAGE_R10G10B10A2,
   };
   return InRange(type, types) ? types[type] : IMAGE_NONE;
}
static IMAGE_TYPE OldImageType1(Byte type)
{
   static const IMAGE_TYPE types[]=
   {
      IMAGE_NONE, IMAGE_B8G8R8A8_SRGB, IMAGE_R8G8B8A8_SRGB, IMAGE_R8G8B8_SRGB, IMAGE_R8G8, IMAGE_R8,
      IMAGE_A8, IMAGE_L8_SRGB, IMAGE_L8A8_SRGB,
      IMAGE_BC2_SRGB, IMAGE_BC3_SRGB,
      IMAGE_I8, IMAGE_I16, IMAGE_I24, IMAGE_I32, IMAGE_F16, IMAGE_F32, IMAGE_F16_2, IMAGE_F32_2, IMAGE_F16_3, IMAGE_F32_3, IMAGE_F16_4, IMAGE_F32_4,
      IMAGE_PVRTC1_2_SRGB, IMAGE_PVRTC1_4_SRGB,

      IMAGE_ETC1,
   };
   return InRange(type, types) ? types[type] : IMAGE_NONE;
}
static IMAGE_TYPE OldImageType0(Byte type)
{
   static const IMAGE_TYPE types[]=
   {
      IMAGE_NONE, IMAGE_B8G8R8A8_SRGB, IMAGE_B8G8R8A8_SRGB, IMAGE_R8G8B8_SRGB, IMAGE_NONE, IMAGE_NONE,
      IMAGE_NONE, IMAGE_A8, IMAGE_A8, IMAGE_L8_SRGB, IMAGE_I16, IMAGE_NONE, IMAGE_L8A8_SRGB,
      IMAGE_BC2_SRGB, IMAGE_BC3_SRGB,
      IMAGE_I8, IMAGE_I16, IMAGE_I24, IMAGE_I32, IMAGE_F16, IMAGE_F32, IMAGE_F16_2, IMAGE_F32_2, IMAGE_F16_3, IMAGE_F32_3, IMAGE_F16_4, IMAGE_F32_4,
   };
   return InRange(type, types) ? types[type] : IMAGE_NONE;
}
/******************************************************************************/
static INLINE Bool CheckMipNum(Int mips) {return InRange(mips, MAX_MIP_MAPS+1);} // +1 because this checks number of elements
/******************************************************************************/
// MEMORY
/******************************************************************************/
static void _CopyImgData(C Byte *&src_data, Byte *&dest_data, Int src_pitch, Int dest_pitch, Int src_blocks_y, Int dest_blocks_y)
{
   Int copy_blocks_y=Min(src_blocks_y, dest_blocks_y);
   if(src_pitch==dest_pitch)
   {
      Int copy=copy_blocks_y*dest_pitch;
      CopyFast(dest_data, src_data, copy);
      dest_data+=copy;
       src_data+=copy;
   }else
   {
      Int copy_pitch=Min(src_pitch, dest_pitch);
      Int zero_pitch=dest_pitch-copy_pitch;
      REPD(y, copy_blocks_y)
      {
         CopyFast(dest_data           , src_data, copy_pitch);
         ZeroFast(dest_data+copy_pitch,           zero_pitch);
         dest_data+=dest_pitch;
          src_data+= src_pitch;
      }
   }
   if(dest_blocks_y>copy_blocks_y)
   {
      Int zero=(dest_blocks_y-copy_blocks_y)*dest_pitch;
      ZeroFast(dest_data, zero); dest_data+=zero;
   }
   src_data+=(src_blocks_y-copy_blocks_y)*src_pitch;
}
/******************************************************************************/
static void _CopyImgData(C Byte *&src_data, Byte *&dest_data, Int src_pitch, Int dest_pitch, Int src_blocks_y, Int dest_blocks_y, Int src_pitch2, Int dest_pitch2, Int src_d, Int dest_d)
{
   Int copy_d=Min(src_d, dest_d);
   if(src_pitch2==dest_pitch2 && src_pitch==dest_pitch && src_blocks_y==dest_blocks_y)
   {
      Int copy=copy_d*dest_pitch2;
      CopyFast(dest_data, src_data, copy);
      dest_data+=copy;
       src_data+=copy;
   }else
   REPD(z, copy_d)
   {
    C Byte * src_next= src_data+ src_pitch2;
      Byte *dest_next=dest_data+dest_pitch2;
     _CopyImgData(src_data, dest_data, src_pitch, dest_pitch, src_blocks_y, dest_blocks_y);
      if(dest_next>dest_data)
      {
         ZeroFast(dest_data, dest_next-dest_data); dest_data=dest_next;
      }
      src_data=src_next;
   }
   if(dest_d>copy_d)
   {
      Int zero=(dest_d-copy_d)*dest_pitch2;
      ZeroFast(dest_data, zero); dest_data+=zero;
   }
   src_data+=(src_d-copy_d)*src_pitch2;
}
/******************************************************************************/
void CopyImgData(C Byte *src_data, Byte *dest_data, Int src_pitch, Int dest_pitch, Int src_blocks_y, Int dest_blocks_y)
{
  _CopyImgData(src_data, dest_data, src_pitch, dest_pitch, src_blocks_y, dest_blocks_y);
}
void CopyImgData(C Byte *src_data, Byte *dest_data, Int src_pitch, Int dest_pitch, Int src_blocks_y, Int dest_blocks_y, Int src_pitch2, Int dest_pitch2, Int src_d, Int dest_d)
{
   DEBUG_ASSERT( src_pitch2>= src_pitch* src_blocks_y,  "src_pitch2>=src_pitch*src_blocks_y");
   DEBUG_ASSERT(dest_pitch2>=dest_pitch*dest_blocks_y, "dest_pitch2>=dest_pitch*dest_blocks_y");
  _CopyImgData(src_data, dest_data, src_pitch, dest_pitch, src_blocks_y, dest_blocks_y, src_pitch2, dest_pitch2, src_d, dest_d);
}
void CopyImgData(C Byte *src_data, Byte *dest_data, Int src_pitch, Int dest_pitch, Int src_blocks_y, Int dest_blocks_y, Int src_pitch2, Int dest_pitch2, Int src_d, Int dest_d, Int faces)
{
   DEBUG_ASSERT( src_pitch2>= src_pitch* src_blocks_y,  "src_pitch2>=src_pitch*src_blocks_y");
   DEBUG_ASSERT(dest_pitch2>=dest_pitch*dest_blocks_y, "dest_pitch2>=dest_pitch*dest_blocks_y");
   if(src_pitch2==dest_pitch2 && src_d==dest_d && src_pitch==dest_pitch && src_blocks_y==dest_blocks_y)
   {
      Int copy=dest_d*dest_pitch2*faces;
      CopyFast(dest_data, src_data, copy);
      dest_data+=copy;
       src_data+=copy;
   }else
   REPD(face, faces)
   {
     _CopyImgData(src_data, dest_data, src_pitch, dest_pitch, src_blocks_y, dest_blocks_y, src_pitch2, dest_pitch2, src_d, dest_d);
   }
}
/******************************************************************************/
// FILE
/******************************************************************************/
static void LoadImgData(File &f, Byte *&dest_data, Int src_pitch, Int dest_pitch, Int src_blocks_y, Int dest_blocks_y)
{
   Int copy_blocks_y=Min(src_blocks_y, dest_blocks_y);
   if(src_pitch==dest_pitch)
   {
      Int copy=copy_blocks_y*dest_pitch;
      f.getFast(dest_data, copy); dest_data+=copy;
   }else
   {
      Int copy_pitch=Min(src_pitch, dest_pitch);
      Int zero_pitch=dest_pitch-copy_pitch;
      Int skip_pitch= src_pitch-copy_pitch;
      REPD(y, copy_blocks_y)
      {
         f.getFast(dest_data, copy_pitch);
          ZeroFast(dest_data+ copy_pitch, zero_pitch);
         dest_data+=dest_pitch;
         f.skip(skip_pitch);
      }
   }
   if(dest_blocks_y>copy_blocks_y)
   {
      Int zero=(dest_blocks_y-copy_blocks_y)*dest_pitch;
      ZeroFast(dest_data, zero); dest_data+=zero;
   }
   f.skip((src_blocks_y-copy_blocks_y)*src_pitch);
}
/******************************************************************************/
static void LoadImgData(File &file, Byte *dest_data, Int src_pitch, Int dest_pitch, Int src_blocks_y, Int dest_blocks_y, Int src_pitch2, Int dest_pitch2, Int src_d, Int dest_d, Int faces)
{
   DEBUG_ASSERT( src_pitch2>= src_pitch* src_blocks_y,  "src_pitch2>=src_pitch*src_blocks_y");
   DEBUG_ASSERT(dest_pitch2>=dest_pitch*dest_blocks_y, "dest_pitch2>=dest_pitch*dest_blocks_y");
   if(src_pitch2==dest_pitch2 && src_d==dest_d && src_pitch==dest_pitch && src_blocks_y==dest_blocks_y)
   {
      Int copy=dest_d*dest_pitch2*faces;
      file.getFast(dest_data, copy); dest_data+=copy;
   }else
   REPD(face, faces)
   {
      Int copy_d=Min(src_d, dest_d);
      if(src_pitch2==dest_pitch2 && src_pitch==dest_pitch && src_blocks_y==dest_blocks_y)
      {
         Int copy=copy_d*dest_pitch2;
         file.getFast(dest_data, copy); dest_data+=copy;
      }else
      REPD(z, copy_d)
      {
         Byte *dest_next=dest_data+dest_pitch2;
         LoadImgData(file, dest_data, src_pitch, dest_pitch, src_blocks_y, dest_blocks_y);
         if(dest_next>dest_data)
         {
            ZeroFast(dest_data, dest_next-dest_data);
            dest_data=dest_next;
         }
      }
      if(dest_d>copy_d)
      {
         Int zero=(dest_d-copy_d)*dest_pitch2;
         ZeroFast(dest_data, zero); dest_data+=zero;
      }
      if(src_d>copy_d)file.skip((src_d-copy_d)*src_pitch2);
   }
}
/******************************************************************************/
// SAVE / LOAD
/******************************************************************************/
Bool Image::saveData(File &f)C
{
   IMAGE_TYPE file_type=T.type(); // set image type as to be stored in the file
   if(!CanCompress(file_type))file_type=T.hwType(); // if compressing to format which isn't supported then store as current 'hwType'
   if(file_type>=IMAGE_TYPES               // don't allow saving private formats
   || mode()   >=IMAGE_RT   )return false; // don't allow saving private modes
   ASSERT(IMAGE_2D==0 && IMAGE_3D==1 && IMAGE_CUBE==2 && IMAGE_SOFT==3 && IMAGE_SOFT_CUBE==4 && IMAGE_RT==5);

   f.putMulti(Byte(2), size3(), file_type, mode(), Byte(mipMaps())); // version

   const Bool ignore_gamma=false; // never ignore and always convert, because Esenthel formats are assumed to be in correct gamma already
   const Bool same_type   =CanDoRawCopy(hwType(), file_type, ignore_gamma);
   const UInt copy_flags  =(ignore_gamma ? IC_IGNORE_GAMMA : IC_CONVERT_GAMMA);
   const Int  faces       =T.faces();
   const VecI file_hw_size(PaddedWidth (w(), h(), 0, file_type),
                           PaddedHeight(w(), h(), 0, file_type), d()); // can't use 'hwSize' because that could be bigger (if image was created big and then 'size' was manually limited smaller), and also need to calculate based on 'file_type'
   const Bool soft_same_type=(soft() && same_type); // software with matching type

   /*if(mip_compression)
   {
      Int compression_level=255; Bool multi_threaded=false;
      if(!ConstCast(T).waitForStream())return false; // since we'll access 'softData' without locking, make sure stream is finished
      Bool direct=(soft_same_type && file_hw_size==hwSize3()); // if software, same type and HW size, we can read from directly
      UInt data_size=ImageSize(file_hw_size.x, file_hw_size.y, file_hw_size.z, file_type, mode(), mipMaps()), // this is size to store compressed image data that we're going to write to the file, make as big as total uncompressed data, if would need more than that, then disable compression and save uncompressed
           biggest_mip_size=0;
      if(!direct) // if we can't read directly from image, then we will need to write to temp buffer first before compression
      {
         biggest_mip_size=ImageFaceSize(file_hw_size.x, file_hw_size.y, file_hw_size.z, 0, file_type)*faces; // need room to write the biggest mip map
         data_size+=biggest_mip_size;
      }
      Memt<Byte> buffer; Byte *temp=buffer.setNum(data_size).data(), *save=temp; if(!direct)save+=biggest_mip_size; Byte *save_cur=save;
      File f_src, f_dest;
      REPD(mip, mipMaps()) // iterate all mip maps #MipOrder
      {
         Int face_size=ImageFaceSize(file_hw_size.x, file_hw_size.y, file_hw_size.z, mip, file_type),
              mip_size=face_size*faces;
       C Byte *src;
         if(direct)
         {
            src=softData(mip);
            TEST THIS
         }else
         {
            src=temp;
            Int file_pitch   =ImagePitch  (file_hw_size.x, file_hw_size.y, mip, file_type),
                file_blocks_y=ImageBlocksY(file_hw_size.x, file_hw_size.y, mip, file_type),
                file_d       =         Max(file_hw_size.z>>mip, 1);
            if(soft_same_type)
            {
               TEST THIS
               Int image_pitch   =softPitch  (mip),
                   image_blocks_y=softBlocksY(mip),
                   image_d       =Max(hwD()>>mip, 1);
               CopyImgData(softData(mip), temp, image_pitch, file_pitch, image_blocks_y, file_blocks_y, image_pitch*image_blocks_y, file_pitch*file_blocks_y, image_d, file_d, faces);
            }else
            {
               Byte *dest=temp;
               FREPD(face, faces) // iterate all faces
               {
                C Image *src=this;
                  Int    src_mip=mip, src_face=face;
                  if(!same_type){if(!extractMipMap(soft, file_type, mip, DIR_ENUM(face), FILTER_BEST, copy_flags))return false; src=&soft; src_mip=0; src_face=0;} // if 'hwType' is different than of file, then convert to 'file_type' IMAGE_SOFT, after extracting the mip map its Pitch and BlocksY may be different than of calculated from base (for example non-power-of-2 images) so write zeros to file to match the expected size
                  if(!src->lockRead(src_mip, DIR_ENUM(src_face)))return false;
                  CopyImgData(src->data(), dest, src->pitch(), file_pitch, src->softBlocksY(src_mip), file_blocks_y);
                  src->unlock();
                  dest+=face_size;
               }
            }
         }
         f_src . readMem    (src     , mip_size  );
         f_dest.writeMemDest(save_cur, mip_size-1); // set -1 because if output would be >=mip_size then there's no reduction, and we want to fallback to faster uncompressed mode
         UInt compressed_size;
         if(CompressRaw(f_src, f_dest, compress, compression_level, multi_threaded)) // if compressed and reduced size (because 'f_dest' size was limited)
         {
            compressed_size=f_dest.pos(); // remember compressed size
            save_cur+=compressed_size;
         }else
         {
            compressed_size=0; // set zero = uncompressed
            CopyFast(save_cur, src, mip_size); // copy uncompressed
            save_cur+=mip_size;
         }
         f.cmpUIntV(compressed_size);
      }
      UInt compressed_size=save_cur-save;
      DEBUG_ASSERT(compressed_size<=data_size, "compressed_size<=data_size");
      f.put(save, compressed_size);
   }else*/
   if(soft_same_type) // software with matching type, we can save without locking
   {
      if(!ConstCast(T).waitForStream())return false; // since we'll access 'softData' without locking, make sure stream is finished
      if(file_hw_size==hwSize3())f.put(softData(), memUsage());else // exact size, then we can save entire memory #MipOrder
      {
       C Byte *data=softData();
         REPD(mip, mipMaps()) // iterate all mip maps #MipOrder
         { // no need to use any "Min" or "f.put(null, ..)" because soft HW sizes are guaranteed to be >= file HW size
            Int file_pitch   =ImagePitch  (file_hw_size.x, file_hw_size.y, mip, file_type),
                file_blocks_y=ImageBlocksY(file_hw_size.x, file_hw_size.y, mip, file_type),
                file_d       =                         Max(file_hw_size.z>>mip, 1),
               image_pitch   = softPitch  (mip),
               image_blocks_y= softBlocksY(mip),
               image_d       =         Max(hwD()>>mip, 1),
                copy         =  file_blocks_y*file_pitch,
                skip         =(image_blocks_y-file_blocks_y)*image_pitch,
                skip2        =(image_d       -file_d       )*image_pitch*image_blocks_y;
            DEBUG_ASSERT(image_pitch   >=file_pitch   ,    "image_pitch>=file_pitch"   );
            DEBUG_ASSERT(image_blocks_y>=file_blocks_y, "image_blocks_y>=file_blocks_y");
            DEBUG_ASSERT(image_d       >=file_d       ,        "image_d>=file_d"       );
            REPD(face, faces) // iterate all faces
            {
               REPD(z, file_d)
               {
                  if(file_pitch==image_pitch){f.put(data, copy      ); data+=copy       ;} // if file pitch is the same as image pitch, then we can write both XY in one go
                  else REPD(y, file_blocks_y){f.put(data, file_pitch); data+=image_pitch;}
                  data+=skip;
               }
               data+=skip2;
            }
         }
      }
   }else
   {
      Image soft; // keep outside to reduce overhead
      REPD(mip, mipMaps()) // iterate all mip maps #MipOrder
      {
         Int file_pitch   =ImagePitch  (file_hw_size.x, file_hw_size.y, mip, file_type),
             file_blocks_y=ImageBlocksY(file_hw_size.x, file_hw_size.y, mip, file_type),
             file_d       =                      Max(1, file_hw_size.z>>mip);
         FREPD(face, faces) // iterate all faces
         {
          C Image *src=this;
            Int    src_mip=mip, src_face=face;
            if(!same_type){if(!extractMipMap(soft, file_type, mip, DIR_ENUM(face), FILTER_BEST, copy_flags))return false; src=&soft; src_mip=0; src_face=0;} // if 'hwType' is different than of file, then convert to 'file_type' IMAGE_SOFT, after extracting the mip map its Pitch and BlocksY may be different than of calculated from base (for example non-power-of-2 images) so write zeros to file to match the expected size
            if(!src->lockRead(src_mip, DIR_ENUM(src_face)))return false;
            Int  src_blocks_y=src->softBlocksY(src_mip);
            Int copy_pitch   =Min(file_pitch, src->pitch());
            Int zero_pitch   =    file_pitch-copy_pitch;
            Int copy_blocks_y=Min( src_blocks_y, file_blocks_y);
            Int zero_blocks_y=(   file_blocks_y-copy_blocks_y)*file_pitch;
            Int copy_pitch2  =    copy_pitch*copy_blocks_y;
            Int copy_d       =Min(file_d, src->ld());
            FREPD(z, copy_d)
            {
             C Byte *data=src->data() + z*src->pitch2();
               if(file_pitch==src->pitch()){f.put(data, copy_pitch2);                             data+=copy_pitch2 ;}
               else  REPD(y, copy_blocks_y){f.put(data, copy_pitch ); f.put(null, zero_pitch   ); data+=src->pitch();}
                                                                      f.put(null, zero_blocks_y);
            }
            f.put(null, (file_d-copy_d)*file_pitch*file_blocks_y);
            src->unlock();
         }
      }
   }
   return f.ok();
}
/******************************************************************************/
struct Mip
{
 //COMPRESS_TYPE compression;
   UInt          compressed_size,
               decompressed_size,
                 offset;
};
struct Loader
{
   static const Bool ignore_gamma=false; // never ignore and always convert, because Esenthel formats are assumed to be in correct gamma already
   static const UInt copy_flags  =(ignore_gamma ? IC_IGNORE_GAMMA : IC_CONVERT_GAMMA)|IC_CLAMP;

   enum MODE : Byte
   {
      DIRECT_FAST, // can read entire mip map directly to memory
      DIRECT     , // can read entire mip map directly to memory however with alignment checking
      CONVERT    , // have to convert
   };

   ImageHeader  header;
 //COMPRESS_TYPE mip_compression=COMPRESS_NONE;
   MODE         load_mode;
   IMAGE_TYPE   want_hw_type;
   IMAGE_MODE   file_mode_soft;
   IMAGE_MODE   want_mode_soft;
   Byte         file_base_mip; // index of file mip map that matches the size if main mip in the image
   Byte        image_base_mip; // index of first valid/loaded mip map in the image (also the number of mips still need to be loaded)
   Int          file_faces;
   Int          want_faces;
   VecI         file_hw_size;
   VecI         want_hw_size;
   File        *f;
#if IMAGE_STREAM_FULL
   VecI         want_size;
   Int          want_mips;
#endif

   Int filePitch  (Int mip)C {return ImagePitch   (file_hw_size.x, file_hw_size.y,                 mip,  header.type);}
   Int fileBlocksY(Int mip)C {return ImageBlocksY (file_hw_size.x, file_hw_size.y,                 mip,  header.type);}
   Int wantPitch  (Int mip)C {return ImagePitch   (want_hw_size.x, want_hw_size.y,                 mip, want_hw_type);}
   Int wantBlocksY(Int mip)C {return ImageBlocksY (want_hw_size.x, want_hw_size.y,                 mip, want_hw_type);}
   Int wantMipSize(Int mip)C {return ImageFaceSize(want_hw_size.x, want_hw_size.y, want_hw_size.z, mip, want_hw_type)*want_faces;}

   Bool load(Int file_mip, Image &image)C
   {
      VecI file_mip_size(Max(1, header.size.x>>file_mip), Max(1, header.size.y>>file_mip), Max(1, header.size.z>>file_mip));
      if(image.createEx(file_mip_size.x, file_mip_size.y, file_mip_size.z, header.type, file_mode_soft, 1))
      {
         Int file_pitch   =filePitch  (file_mip),
             file_blocks_y=fileBlocksY(file_mip);
     //C Mip &mip=mips[file_mip]; if(mip.compression)todo;else
         {
            LoadImgData(*f, image.data(), file_pitch, image.pitch(), file_blocks_y, image.softBlocksY(0), file_pitch*file_blocks_y, image.pitch2(), file_mip_size.z, image.d(), file_faces);
         }
         return f->ok();
      }
      return false;
   }
   Bool load(Int file_mip, Int img_mip, Byte *img_data)C
   {
      Int img_pitch   =wantPitch  (img_mip),
          img_blocks_y=wantBlocksY(img_mip),
          img_d       =Max(1, want_hw_size.z>>img_mip);
      if(load_mode!=CONVERT)
      {
         Int file_pitch   =filePitch  (file_mip),
             file_blocks_y=fileBlocksY(file_mip),
             file_d       =Max(1, file_hw_size.z>>file_mip);
     //C Mip &mip=mips[file_mip]; if(mip.compression)todo;else
         {
            LoadImgData(*f, img_data, file_pitch, img_pitch, file_blocks_y, img_blocks_y, file_pitch*file_blocks_y, img_pitch*img_blocks_y, file_d, img_d, file_faces);
         }
         return f->ok();
      }else
      {
         Image soft;
         if(!load(file_mip, soft))return false;
         if(!soft.copyTry(soft, -1, -1, -1, want_hw_type, want_mode_soft, -1, FILTER_BEST, copy_flags|IC_NO_ALT_TYPE))return false;
         CopyImgData(soft.data(), img_data, soft.pitch(), img_pitch, soft.softBlocksY(0), img_blocks_y, soft.pitch2(), img_pitch*img_blocks_y, soft.d(), img_d, want_faces);
         return true;
      }
   }
   Bool load  (Image &image, C Str &name, Bool can_del_f);
   void update();
};
/******************************************************************************/
static void Cancel(Image *&image) {if(image){image->_streaming=false; image=null;}} // use when want to cancel something
struct StreamData
{
   Image *image;

   inline void cancel() {Cancel(image);} // use when want to cancel something
};
struct StreamLoad : StreamData
{
   Loader loader;
   File   f;
};
struct StreamSet : StreamData
{
   Int        mip;
   Mems<Byte> mip_data;

   void setMipData()
   {
      if(image && mip>=0) // not canceled && not error
         image->lockedSetMipData(mip_data.data(), mip); // set mip data
      mip_data.del(); // delete now to release memory, because it's possible driver would make its own allocation, and since we delete objects only after all of them are processed, a lot of memory could get allocated
   }
   void baseMip()
   {
      if(image && InRange(mip, image->baseMip())) // mip>=0 && mip<image->baseMip(), not canceled && not error && smaller, because this can be called for biggest mips first
         image->lockedBaseMip(mip);
   }
   void finish()
   {
      if(image && mip<=0) // not canceled && error or last mip
         image->_streaming=false; // stream finished !! THIS CAN BE SET ONLY FOR THE LAST 'StreamSet' FOR THIS 'image' !!
   }
   void canceled(Image &image) // use when something got canceled and we need to clear it
   {
       if(T.image==&image){T.image=null; mip_data.del();} // can already release memory
   }
};
static MemcThreadSafe<StreamLoad> StreamLoads;
static MemcThreadSafe<StreamSet > StreamSets; // !! MUST BE PROCESSED IN ORDER, BECAUSE WE CAN LOWER 'image.baseMip' ONLY 1 BY 1 DOWN TO ZERO, CAN'T SKIP !!
static Image                     *StreamLoadCur;
static SyncLock                   StreamLoadCurLock;
static SyncEvent                  StreamLoadEvent;
static SyncCounter                StreamLoaded;
static Int                        StreamLoadWaiting;
static Thread                     StreamLoadThread;
static Bool                       StreamLoadFunc(Thread &thread);
static void                       StreamSetsFunc();
/******************************************************************************/
static INLINE Bool SizeFits (  Int   src,   Int   dest) {return src<dest*2;} // this is OK for src=7, dest=4 (7<4*2), but NOT OK for src=8, dest=4 (8<4*2)
static INLINE Bool SizeFits1(  Int   src,   Int   dest) {return src>1 && SizeFits(src, dest);} // only if 'src>1', if we don't check this, then 1024x1024x1 src will fit into 16x16x1 dest because of Z=1
static        Bool SizeFits (C VecI &src, C VecI &dest) {return SizeFits1(src.x, dest.x) || SizeFits1(src.y, dest.y) || SizeFits1(src.z, dest.z) || (src.x==1 && src.y==1 && src.z==1);}

static void Shrink(Int  &mips, Int shrink) {mips=Max(1, mips-shrink);}
static void Shrink(VecI &size, Int shrink)
{
   size.set(Max(1, size.x>>shrink),
            Max(1, size.y>>shrink),
            Max(1, size.z>>shrink));
}
/******************************************************************************/
Bool Loader::load(Image &image, C Str &name, Bool can_del_f)
{
   if(!header.is()){image.del(); return true;}
   ImageHeader want=header;
   Int         shrink=0;
   if(Int (*image_load_shrink)(ImageHeader &image_header, C Str &name)=D.image_load_shrink) // copy to temp variable to avoid multi-threading issues
   {
      shrink=image_load_shrink(want, name);

      // adjust mip-maps, we will need this for load from file memory
      Int total_mip_maps=TotalMipMaps(want.size.x, want.size.y, want.size.z); // don't use hardware size hwW(), hwH(), hwD(), so that number of mip-maps will always be the same (and not dependant on hardware capabilities like TexPow2 sizes), also because 1x1 image has just 1 mip map, but if we use padding then 4x4 block would generate 3 mip maps
      if(want.mip_maps<=0)want.mip_maps=total_mip_maps ; // if mip maps not specified (or we want multiple mip maps with type that requires full chain) then use full chain
      else            MIN(want.mip_maps,total_mip_maps); // don't use more than maximum allowed
   }

   file_faces  =    ImageFaces  (header.mode);
   file_hw_size.set(PaddedWidth (header.size.x, header.size.y, 0, header.type),
                    PaddedHeight(header.size.x, header.size.y, 0, header.type), header.size.z);

   // set mips info
   UInt compressed_size=0; //, image_size=0;
   Mip  mips[MAX_MIP_MAPS]; if(!CheckMipNum(header.mip_maps))return false;
   REP(header.mip_maps) // #MipOrder
   {
      Mip &mip=mips[i];
           mip.decompressed_size=ImageFaceSize(file_hw_size.x, file_hw_size.y, file_hw_size.z, i, header.type)*file_faces;
    /*if(  mip.compression=mip_compression)
      {
         f->decUIntV(mip.compressed_size); if(!mip.compressed_size)
         {
            mip.compression    =COMPRESS_NONE;
            mip.compressed_size=mip.decompressed_size;
         }
      }else */mip.compressed_size=mip.decompressed_size;
              mip.offset         =      compressed_size;
      compressed_size+=mip.  compressed_size;
         //image_size+=mip.decompressed_size;
   }
   if(!f->ok  ()                 // if any 'mip.compressed_size' failed to load
   ||  f->left()<compressed_size // or don't have enough data (this is needed loading directly from FILE_MEM memory, and also to make sure that any streaming on other thread will succeed)
   )return false;
   if(!want.is()){image.del(); return can_del_f || f->skip(compressed_size);} // check before shrinking, because "Max(1" might validate it, no need to seek if 'f' isn't needed later (important if 'f' is compressed)

   // shrink
   if(shrink>0)
   {
      Shrink(want.size    , shrink);
      Shrink(want.mip_maps, shrink);
   }
   if(IsHW(want.mode) && D.maxTexSize()>0) // apply 'D.maxTexSize' only for hardware textures (not for software images)
      for(; want.size.max()>D.maxTexSize(); )
   {
      Shrink(want.size    , 1);
      Shrink(want.mip_maps, 1);
   }

   // detect HW type that we will use
   want_hw_type=want.type;
   for(; !ImageSupported(want_hw_type, want.mode); )
   {
          want_hw_type=ImageTypeOnFail(want_hw_type); // use replacement
      if(!want_hw_type)return false; // there isn't any then fail
   }

   want_faces  =    ImageFaces  (want.mode);
   want_hw_size.set(PaddedWidth (want.size.x, want.size.y, 0, want_hw_type),
                    PaddedHeight(want.size.x, want.size.y, 0, want_hw_type), want.size.z);
   file_mode_soft=AsSoft(header.mode);
   want_mode_soft=AsSoft(  want.mode);

   // base mip = find smallest file mip that's >= biggest image mip
         file_base_mip=0;
   VecI  file_base_mip_size=header.size;
   for(; file_base_mip<header.mip_maps-1 && !SizeFits(file_base_mip_size, want.size); )
   {
      file_base_mip++;
      file_base_mip_size.set(Max(1, file_base_mip_size.x>>1), Max(1, file_base_mip_size.y>>1), Max(1, file_base_mip_size.z>>1));
   }

   Long f_end=f->pos()+compressed_size;
   if(file_base_mip_size==want.size) // if found exact mip match
   {
      const VecI file_base_mip_hw_size_no_pad(Max(1, file_hw_size.x>>file_base_mip), Max(1, file_hw_size.y>>file_base_mip), Max(1, file_hw_size.z>>file_base_mip));
            Bool same_alignment=(file_base_mip_hw_size_no_pad==want_hw_size); // file mip hw size without pad exactly matches wanted texture, only this will guarantee all mip maps will have same sizes, since they're exactly the same, then "file_base_mip_hw_size_no_pad>>mip==want_hw_size>>mip" for any 'mip'. This is for cases when loading image size=257, which mip1 size=Ceil4(Ceil4(257)>>1)=Ceil4(130)=132, and doing shrink=1, giving size=128
            Bool same_type     =CanDoRawCopy(header.type, want_hw_type, ignore_gamma);
            Bool direct        =(same_type && want_faces==file_faces); // type and cube are the same
      load_mode=(direct ? (same_alignment /*&& !mip_compression*/) ? DIRECT_FAST : DIRECT : CONVERT);

      CPtr mip_data[MAX_MIP_MAPS]; if(!CheckMipNum(want.mip_maps))return false;

      // try to create directly from file memory
      if( f->_type==FILE_MEM // file data is already available and in continuous memory
      && !f->_cipher         // no cipher
      && header.mip_maps-file_base_mip>=want.mip_maps // have all mip maps that we want
      && load_mode==DIRECT_FAST
    //&& !mip_compression   // no mip compression
      )
      {
         REP(want.mip_maps)mip_data[i]=(Byte*)f->memFast()+mips[file_base_mip+i].offset;
         if(image.createEx(want.size.x, want.size.y, want.size.z, want_hw_type, want.mode, want.mip_maps, 1, mip_data))
         {
            image.adjustInfo(image.w(), image.h(), image.d(), want.type);
            return can_del_f || f->skip(compressed_size); // skip image data, no need to seek if 'f' isn't needed later (important if 'f' is compressed)
         }
      }

   /* Load all mip maps that are already in file buffer without requiring to do any more reads
      but at least 1 (to make sure we have something, even if a read is needed)
      if(FILE_MEM || can't stream) then read all. Can stream if 'can_del_f' and 'image' belongs to 'Images' cache
      when creating image, use src data for fast create (can ignore smallest/biggest mip-maps)
      if there are missing smaller mip-maps than read, then make them with 'updateMipMaps'
      schedule loading of remaining mip-maps */
      Int can_read_mips=Min(header.mip_maps-file_base_mip, want.mip_maps); // mips that we can read
      if(!f->skip(mips[file_base_mip+can_read_mips-1].offset))return false; // #MipOrder

      Bool stream=false;
      Int  read_mips=0;
      UInt data_size=0;
      if(f->_type!=FILE_MEM // if file not in memory
      && f->_type!=FILE_MEMB
      && can_del_f)        // and can delete 'f' file
      {
         Int buffer=f->_buf_len; // how much data in the buffer
         if(f->left()>buffer) // only if there's still more data left to read (we haven't already read the entire file)
         {
            REP(can_read_mips) // #MipOrder, start from the smallest
            {
               Int mip_size=mips[file_base_mip+i].compressed_size;
               if(buffer<mip_size)break; // not in buffer
               buffer-=mip_size;
               read_mips++;
            }
            MAX(read_mips, 1); // always read at least one
            if( read_mips<can_read_mips && Images.has(&image)) // check 'Images.has' at the end because it's slow, this check is performed because we must be sure that the image will remain in constant memory address, if it's not in cache, maybe user can move it
            {
               stream=true;
            #if IMAGE_STREAM_FULL
               REP(can_read_mips)data_size+=wantMipSize(i); // already allocate enough for full image

               // remember current for later
               T.want_size=want.size;
               T.want_mips=want.mip_maps;

               // adjust reading for now
               shrink=can_read_mips-read_mips;
               file_base_mip+=shrink;
                      want.mip_maps=can_read_mips=read_mips; // create only those that we will read, all others will be set on the loader thread
               Shrink(want.size, shrink);
               want_hw_size.set(PaddedWidth (want.size.x, want.size.y, 0, want_hw_type),
                                PaddedHeight(want.size.x, want.size.y, 0, want_hw_type), want.size.z);
               goto has_read_mips_and_data_size;
            #else
               goto has_read_mips;
            #endif
            }
         }
      }
      read_mips=can_read_mips; // if not streaming then read all here
#if !IMAGE_STREAM_FULL
   has_read_mips:
#endif
      // calculate data needed for loaded mip maps
      FREP(read_mips){Int img_mip=can_read_mips-1-i; data_size+=wantMipSize(img_mip);}
   has_read_mips_and_data_size:

      const Bool need_valid_ptr=GPU_API(true, false); // DX requires that all mip data pointers are valid
      if(need_valid_ptr)MAX(data_size, wantMipSize(0)); // need enough room for biggest mip map
      Memt<Byte> img_data_memt; Byte *img_data;
   #if IMAGE_STREAM_FULL
      Mems<Byte> img_data_mems; if(stream)img_data=img_data_mems.setNum(data_size).data();else
   #endif
                                          img_data=img_data_memt.setNum(data_size).data();

      REP(want.mip_maps)mip_data[i]=(need_valid_ptr ? img_data : null);
      if(load_mode==DIRECT_FAST)
      {
         Byte *img_data_start=img_data;
         FREP(read_mips)
         {
            Int  img_mip=can_read_mips-1-i;
            Int file_mip=file_base_mip+img_mip;
            mip_data[img_mip]=img_data;
            img_data+=mips[file_mip].decompressed_size; // can use file mip here only because it's exactly the same as image mip
         }
         if(!f->getFast(img_data_start, img_data-img_data_start))return false;
      }else
      {
         FREP(read_mips)
         {
            Int  img_mip=can_read_mips-1-i;
            Int file_mip=file_base_mip+img_mip;
            mip_data[img_mip]=img_data;
            if(!load(file_mip, img_mip, img_data))return false;
            img_data+=wantMipSize(img_mip);
         }
      }
      image_base_mip=can_read_mips-read_mips;
      if(!image.createEx(want.size.x, want.size.y, want.size.z, want_hw_type, want.mode, want.mip_maps, 1, mip_data, image_base_mip))
      {
         if(!ImageTypeInfo::usageKnown()) // only if usage is unknown, because if known, then we've already selected correct 'want_hw_type'
            if(IMAGE_TYPE alt_type=ImageTypeOnFail(want_hw_type)) // there's replacement
         {
            Image soft; if(soft.createEx(want.size.x, want.size.y, want.size.z, want_hw_type, want_mode_soft, want.mip_maps, 1, mip_data)) // first create as soft
            {
               want_hw_type=alt_type;
            again:
               if(soft.copyTry(soft, -1, -1, -1, want_hw_type, -1, -1, FILTER_BEST, copy_flags|IC_NO_ALT_TYPE)) // perform conversion
               {
                  REP(soft.mipMaps())mip_data[i]=soft.softData(i);
                  if(image.createEx(soft.w(), soft.h(), soft.d(), soft.hwType(), want.mode, soft.mipMaps(), soft.samples(), mip_data, image_base_mip))
                  {
                     // these could've changed if converted to another type
                     want_hw_size  =image.hwSize3();
                     same_alignment=(file_base_mip_hw_size_no_pad==want_hw_size);
                     same_type     =CanDoRawCopy(header.type, want_hw_type, ignore_gamma);
                     direct        =(same_type && want_faces==file_faces);
                     load_mode     =(direct ? (same_alignment /*&& !mip_compression*/) ? DIRECT_FAST : DIRECT : CONVERT);
                  #if IMAGE_STREAM_FULL
                     if(stream)
                     {
                        // need this for calculation of full size
                        want_hw_size.set(PaddedWidth (want_size.x, want_size.y, 0, want_hw_type),
                                         PaddedHeight(want_size.x, want_size.y, 0, want_hw_type), want_size.z);
                        data_size=0;
                        Int full_mips=read_mips+shrink;
                        REP(full_mips)data_size+=wantMipSize(i); // already allocate enough for entire image

                        Int copy=soft.memUsage();
                        MAX(data_size, copy);
                        img_data_mems.setNumDiscard(data_size);
                        CopyFast(img_data_mems.data(), soft.softData(), copy);
                     }
                  #endif
                     goto ok;
                  }
                  if(want_hw_type=ImageTypeOnFail(want_hw_type))goto again; // there's another replacement
               }
            }
         }
         return false;
      }
   ok:
      image.adjustInfo(image.w(), image.h(), image.d(), want.type);
      image.updateMipMaps(FILTER_BEST, copy_flags, can_read_mips-1);
      if(stream)
      {
      #if IMAGE_STREAM_FULL
         file_base_mip-=shrink;
         // adjust members in case user wants to access them, this is a bit dangerous, care must be taken
         {
            image.   _size=want_size;
            image._hw_size=want_hw_size.set(PaddedWidth (want_size.x, want_size.y, 0, want_hw_type),
                                            PaddedHeight(want_size.x, want_size.y, 0, want_hw_type), want_size.z);
            image._mips=want_mips;
            image.setPartial();
         }
      #endif
         image._streaming=true; // !! THIS CAN BE SET ONLY IF WE'RE GOING TO PUT THIS FOR FURTHER PROCESSING !!
         {
            MemcThreadSafeLock lock(StreamLoads);
            StreamLoad &sl=StreamLoads.lockedNew();
                 sl.image=&image;
            Swap(sl.f     , *f);
            Swap(sl.loader,  T); // 'sl.loader.f' will be adjusted in the processing thread
         #if IMAGE_STREAM_FULL
            Swap(sl.img_data, img_data_mems);
         #endif
            if(!StreamLoadThread.created())StreamLoadThread.create(StreamLoadFunc, null, 0, false, "EE.StreamLoad");
         }
         StreamLoadEvent.on();
         return true;
      }else
      {
         if(can_del_f || f->pos(f_end))return true; // no need to seek if 'f' isn't needed later (important if 'f' is compressed)
      }
   }else // load the base mip, and re-create the whole image out of it
   {
      Image soft;
      if(f->skip(mips[file_base_mip].offset))
         if(load(file_base_mip, soft))
            if(can_del_f || f->pos(f_end)) // no need to seek if 'f' isn't needed later (important if 'f' is compressed)
               return soft.copyTry(image, want.size.x, want.size.y, want.size.z, want.type, want.mode, want.mip_maps, FILTER_BEST, copy_flags);
   }
   return false;
}
static inline Bool Submit(StreamSet &set)
{
   {
      SyncLocker lock(StreamLoadCurLock);
      if(!StreamLoadCur)return false; // canceled
      StreamSets.swapAdd(set);
   }
   App._callbacks.include(StreamSetsFunc); // have to schedule after every swap, and not just one time
   if(StreamLoadWaiting)StreamLoaded+=StreamLoadWaiting; // wake up all those that are waiting
   return true;
}
void Loader::update()
{
#if IMAGE_STREAM_FULL
   if(load_mode==DIRECT_FAST) // recalculate if we can still use DIRECT_FAST due to full size
   {
      const VecI file_base_mip_hw_size_no_pad(Max(1, file_hw_size.x>>file_base_mip), Max(1, file_hw_size.y>>file_base_mip), Max(1, file_hw_size.z>>file_base_mip));
            Bool same_alignment=(file_base_mip_hw_size_no_pad==want_hw_size); // file mip hw size without pad exactly matches wanted texture, only this will guarantee all mip maps will have same sizes, since they're exactly the same, then "file_base_mip_hw_size_no_pad>>mip==want_hw_size>>mip" for any 'mip'. This is for cases when loading image size=257, which mip1 size=Ceil4(Ceil4(257)>>1)=Ceil4(130)=132, and doing shrink=1, giving size=128
      if(!same_alignment)load_mode=DIRECT;
   }
   // FIXME might need to realign 'img_data'
#else
   StreamSet set;
   REPD(img_mip, image_base_mip) // iterate all mips that need to be loaded
   {
      Int file_mip=file_base_mip+img_mip;
      Int  img_mip_size=wantMipSize(img_mip);
      set.mip_data.setNumDiscard(img_mip_size);
      switch(load_mode)
      {
         case DIRECT_FAST: if(!f->getFast(set.mip_data.data(), img_mip_size))goto error; break;
         default         : if(!load(file_mip, img_mip, set.mip_data.data()) )goto error; break;
      }
      // have to set members for every mip, because due to swap they become invalid
      set.mip  =img_mip;
      set.image=StreamLoadCur;

      // !! ANY DATA OUTPUT HERE MUST BE PROCESSED IN ORDER, BECAUSE WE CAN LOWER 'image.baseMip' ONLY 1 BY 1 DOWN TO ZERO, CAN'T SKIP !!
      if(!Submit(set))return; // canceled
   }
   return;
error:
#if 1 // allow to continue
   set.mip  =-1; // error
   set.image=StreamLoadCur;
   set.mip_data.clear();
   Submit(set);
#else // exit
   Exit("Can't stream Image");
#endif
#endif
}
/******************************************************************************/
static Bool Load(Image &image, File &f, C ImageHeader &header, C Str &name)
{
   ImageHeader want=header;
   Int         shrink=0;
   if(Int (*image_load_shrink)(ImageHeader &image_header, C Str &name)=D.image_load_shrink) // copy to temp variable to avoid multi-threading issues
   {
      shrink=image_load_shrink(want, name);

      // adjust mip-maps, we will need this for load from file memory
      Int total_mip_maps=TotalMipMaps(want.size.x, want.size.y, want.size.z); // don't use hardware texture size hwW(), hwH(), hwD(), so that number of mip-maps will always be the same (and not dependant on hardware capabilities like TexPow2 sizes), also because 1x1 image has just 1 mip map, but if we use padding then 4x4 block would generate 3 mip maps
      if(want.mip_maps<=0)want.mip_maps=total_mip_maps ; // if mip maps not specified (or we want multiple mip maps with type that requires full chain) then use full chain
      else            MIN(want.mip_maps,total_mip_maps); // don't use more than maximum allowed
   }
   // shrink
   for(; --shrink>=0 || (IsHW(want.mode) && want.size.max()>D.maxTexSize() && D.maxTexSize()>0); ) // apply 'D.maxTexSize' only for hardware textures (not for software images)
   {
      want.size.x  =Max(1, want.size.x >>1);
      want.size.y  =Max(1, want.size.y >>1);
      want.size.z  =Max(1, want.size.z >>1);
      want.mip_maps=Max(1, want.mip_maps-1);
   }

   const Bool create_from_soft=true; // if want to load into SOFT and then create HW from it, to avoid locking 'D._lock', use this because it's much faster
   const Bool file_cube =IsCube    (header.mode);
   const Int  file_faces=ImageFaces(header.mode);

   /* #MipOrder
   // try to create directly from file memory
   if(create_from_soft // can create from soft
   &&  f._type==FILE_MEM // file data is already available and in continuous memory
   && !f._cipher         // no cipher
   && IsHW        (want.mode) // want HW mode
   && CanDoRawCopy(want.type, header.type) // type is the same
   && IsCube      (want.mode)==file_cube   // cube is the same
   && want.size.x==PaddedWidth (want.size.x, want.size.y, 0, want.type) // can do this only if size is same as hwSize (if not same, then it means HW image has some gaps between lines/blocks that are not present in file data, and we must insert them and zero before create)
   && want.size.y==PaddedHeight(want.size.x, want.size.y, 0, want.type) // can do this only if size is same as hwSize (if not same, then it means HW image has some gaps between lines/blocks that are not present in file data, and we must insert them and zero before create)
   )
      FREPD(file_mip, header.mip_maps) // iterate all mip maps in the file
   {
      VecI file_mip_size(Max(1, header.size.x>>file_mip), Max(1, header.size.y>>file_mip), Max(1, header.size.z>>file_mip));
      if(  file_mip_size==want.size) // found exact match
      {
         if(header.mip_maps-file_mip>=want.mip_maps) // if have all mip maps that we want
         {
            Byte *f_data=(Byte*)f.memFast();
            FREPD(i, file_mip)f_data+=ImageFaceSize(header.size.x, header.size.y, header.size.z, i, header.type)*file_faces; // skip data that we don't want
            if(image.createEx(want.size.x, want.size.y, want.size.z, want.type, want.mode, want.mip_maps, 1, f_data this needs to be fixed))
            {
               for(; file_mip<header.mip_maps; file_mip++)f_data+=ImageFaceSize(header.size.x, header.size.y, header.size.z, file_mip, header.type)*file_faces; // skip remaining mip maps
               f.skip(f_data-(Byte*)f.memFast()); // skip to the end of the image
               return true;
            }
         }
         break;
      }else
      if(file_mip_size.x<want.size.x
      || file_mip_size.y<want.size.y
      || file_mip_size.z<want.size.z)break; // if any dimension of processed file mip is smaller than what we want then stop
   }*/

   // create
   if(image.createTry(want.size.x, want.size.y, want.size.z, want.type, create_from_soft ? AsSoft(want.mode) : want.mode, want.mip_maps)) // don't use 'want' after this call, instead operate on 'image' members
   {
const FILTER_TYPE filter=FILTER_BEST;
      Image       soft; // store outside the loop to avoid overhead
const Bool        fast_load=(image.soft() && CanDoRawCopy(image.hwType(), header.type) && image.cube()==file_cube);
      Int         image_mip=0; // how many mip-maps have already been set in the image
      FREPD(file_mip, header.mip_maps) // iterate all mip maps in the file
      {
         VecI file_mip_size(Max(1, header.size.x>>file_mip), Max(1, header.size.y>>file_mip), Max(1, header.size.z>>file_mip));
         Bool      mip_fits=SizeFits(file_mip_size, image.size3()); // if this mip-map fits into the image
         if(image_mip<image.mipMaps() // if we can set another mip-map, because it fits into 'image' mip map array
         && (mip_fits || file_mip==header.mip_maps-1)) // if fits or this is the last mip-map in the file
         {
            Int  mip_count;
            VecI image_mip_size(Max(1, image.w()>>image_mip), Max(1, image.h()>>image_mip), Max(1, image.d()>>image_mip));
            if(fast_load && image_mip_size==file_mip_size) // if this is a software image with same type/cube and mip size matches
            {
            /* #MipOrder
               if(image_mip_size.x==Max(1, image.hwW()>>image_mip)
               && image_mip_size.y==Max(1, image.hwH()>>image_mip)
               && image_mip_size.z==Max(1, image.hwD()>>image_mip)) // if image mip size is the same as image HW mip size, then we can read all remaining data in one go
               {
                  mip_count=Min(image.mipMaps()-image_mip, header.mip_maps-file_mip); // how many mip-maps we can read = Min(available in 'image', available in file)
                  f.getFast(image.softData(image_mip), ImageSize(file_mip_size.x, file_mip_size.y, file_mip_size.z, header.type, header.mode, mip_count));
                  file_mip+=mip_count-1; // -1 because 1 is already added at the end of FREPD loop
               }else*/
               {
                  mip_count=1;
                  // here no need to use any "Min" because soft HW sizes are guaranteed to be >= file sizes
            const Int file_pitch   =ImagePitch  (header.size.x, header.size.y, file_mip, header.type),
                      file_blocks_y=ImageBlocksY(header.size.x, header.size.y, file_mip, header.type),
                     image_pitch   =image.softPitch  (image_mip),
                     image_blocks_y=image.softBlocksY(image_mip),
                      file_d       =Max(1, header.size.z>> file_mip),
                     image_d       =Max(1,  image.hwD() >>image_mip),
                      zero_pitch   = image_pitch   -file_pitch,
                      read         =  file_blocks_y*file_pitch,
                      zero         =(image_blocks_y-file_blocks_y)*image_pitch, // how much to zero = total - what was set
                      zero2        =(image_d       -file_d       )*image_pitch*image_blocks_y;
                  Byte *data=image.softData(image_mip);
                  FREPD(face, file_faces) // iterate all faces
                  {
                     FREPD(z, file_d)
                     {
                        if(file_pitch==image_pitch) // if file pitch is the same as image pitch
                        {  // we can read both XY in one go
                           f.getFast(data, read);
                           data+=read;
                        }else
                        FREPD(y, file_blocks_y) // read each line separately
                        {
                           f.getFast(data, file_pitch);
                           if(zero_pitch>0)ZeroFast(data+file_pitch, zero_pitch); // zero remaining data to avoid garbage
                           data+=image_pitch;
                        }
                        if(zero>0)ZeroFast(data, zero); data+=zero; // zero remaining data to avoid garbage
                     }
                     if(zero2>0)ZeroFast(data, zero2); data+=zero2; // zero remaining data to avoid garbage
                  }
               }
            }else
            {
               Bool   temp=(!CanDoRawCopy(image.hwType(), header.type) || file_mip_size!=image_mip_size); // we need to load the mip-map into temporary image first, if the hardware types don't match, or if the mip-map size doesn't match
               Image *dest; Int dest_mip;
               mip_count=1;
               if(temp) // if need to use a temporary image
               {
                  if(!soft.createTry(file_mip_size.x, file_mip_size.y, file_mip_size.z, header.type, IMAGE_SOFT, 1, false))return false;
                  dest=&soft; dest_mip=0;
                  if(!image_mip) // if file is 64x64 but 'image' is 256x256 then we need to write the first file mip map into 256x256, 128x128, 64x64 'image' mip maps, this check is needed only at the start, when we still haven't written any mip-maps
                     REP(image.mipMaps()-1) // -1 because we already have 'mip_count'=1
                  {
                     image_mip_size.set(Max(1, image_mip_size.x>>1), Max(1, image_mip_size.y>>1), Max(1, image_mip_size.z>>1)); // calculate next image mip size
                     if(SizeFits(file_mip_size, image_mip_size))mip_count++;else break; // if file mip still fits into the next image mip size, then increase the counter, else stop
                  }
               }else // we can write directly to 'image'
               {
                  dest=&image; dest_mip=image_mip;
               }

         const Int file_pitch   =ImagePitch  (header.size.x, header.size.y, file_mip, header.type),
                   file_blocks_y=ImageBlocksY(header.size.x, header.size.y, file_mip, header.type),
                   dest_blocks_y=dest->softBlocksY(dest_mip),
                   read_blocks_y=Min(dest_blocks_y, file_blocks_y),
                   read         = read_blocks_y*file_pitch, // !! use "read_blocks_y*file_pitch" and not 'pitch2', because 'pitch2' may be bigger !!
                   skip         =(file_blocks_y-read_blocks_y)*file_pitch; // unread blocksY * pitch
               FREPD(face, file_faces) // iterate all faces
               {
                  if(!dest->lock(LOCK_WRITE, dest_mip, DIR_ENUM(temp ? 0 : face)))return false;
            const Int   read_pitch=Min(dest->pitch (), file_pitch),
                        zero      =    dest->pitch2()- read_blocks_y*dest->pitch(); // how much to zero = total - what was set
                  Byte *dest_data =    dest->data  ();
                  FREPD(z, dest->ld())
                  {
                     if(file_pitch==dest->pitch()) // if file pitch is the same as image pitch !! compare 'dest->pitch' and not 'read_pitch' !!
                     {  // we can read both XY in one go
                        f.getFast(dest_data, read);
                        dest_data+=read;
                     }else
                     {
                        const Int skip_pitch= file_pitch  -read_pitch, // even though this could be calculated above outside of the loop, most likely this will not be needed because most likely "file_pitch==dest->pitch()" and we can read all in one go
                                  zero_pitch=dest->pitch()-read_pitch;
                        FREPD(y, read_blocks_y) // read each line separately
                        {
                           f.getFast(dest_data, read_pitch); f.skip(skip_pitch);
                           if(zero_pitch>0)ZeroFast(dest_data+read_pitch, zero_pitch); // zero remaining data to avoid garbage
                           dest_data+=dest->pitch();
                        }
                     }
                     f.skip(skip);
                     if(zero>0)ZeroFast(dest_data, zero); // zero remaining data to avoid garbage
                     dest_data+=zero;
                  }
                  dest->unlock();

                  if(temp)REP(mip_count)if(!image.injectMipMap(*dest, image_mip+i, DIR_ENUM(face), filter))return false;
               }
            }
            image_mip+=mip_count;
         }else // skip this mip-map
         {
            f.skip(ImageFaceSize(header.size.x, header.size.y, header.size.z, file_mip, header.type)*file_faces);
         }
      }
      if(image_mip)image.updateMipMaps(filter, IC_CLAMP, image_mip-1); // set any missing mip maps, this is needed for example if file had 1 mip map, but we've requested to create more
      else         image.clear(); // or if we didn't load anything, then clear to zero

      if(image.mode()!=want.mode) // if created as SOFT, then convert to HW
      {
         Swap(soft, image); // can't create from self
         CPtr mip_data[MAX_MIP_MAPS];
         if(!CheckMipNum(soft.mipMaps()))return false; REP(soft.mipMaps())mip_data[i]=soft.softData(i);
         if(!image.createEx(soft.w(), soft.h(), soft.d(), soft.type(), want.mode, soft.mipMaps(), soft.samples(), mip_data))
         {
            IMAGE_TYPE type=soft.type(); // remember for adjust later
            for(IMAGE_TYPE alt_type=type; ; )
            {
               alt_type=ImageTypeOnFail(alt_type); if(!alt_type)return false;
               if(ImageSupported(alt_type, want.mode, soft.samples()) // do a quick check before 'copyTry' to avoid it if we know creation will fail
               && soft.copyTry(soft, -1, -1, -1, alt_type, -1, -1, FILTER_BEST, IC_CONVERT_GAMMA)) // we have to keep depth, soft mode, mip maps, make sure gamma conversion is performed
               {
                  if(!CheckMipNum(soft.mipMaps()))return false; REP(soft.mipMaps())mip_data[i]=soft.softData(i);
                  if(image.createEx(soft.w(), soft.h(), soft.d(), soft.type(), want.mode, soft.mipMaps(), soft.samples(), mip_data))break; // success
               }
            }
            image.adjustInfo(image.w(), image.h(), image.d(), type);
         }
      }

      return f.ok();
   }
   return false;
}
/******************************************************************************/
#pragma pack(push, 1)
struct ImageFileHeader
{
   VecI       size;
   IMAGE_TYPE type;
   IMAGE_MODE mode;
   Byte       mips;
};
#pragma pack(pop)
/******************************************************************************/
Bool Image::loadData(File &f, ImageHeader *header, C Str &name, Bool can_del_f)
{
   ImageFileHeader fh;
   switch(f.decUIntV())
   {
      case 2:
      {
         f>>fh; if(f.ok())
         {
            Loader loader;
            Unaligned(loader.header.size    , fh.size);
            Unaligned(loader.header.type    , fh.type);
            Unaligned(loader.header.mode    , fh.mode);
           _Unaligned(loader.header.mip_maps, fh.mips);
            if(header){*header=loader.header; return true;}
            loader.f=&f;
            if(loader.load(T, name, can_del_f))goto ok;
         }
      }break;

      case 1:
      {
         f>>fh; if(f.ok())
         {
            ImageHeader ih;
            Unaligned(ih.size    , fh.size);
            Unaligned(ih.type    , fh.type);
            Unaligned(ih.mode    , fh.mode);
           _Unaligned(ih.mip_maps, fh.mips);
            if(header){*header=ih; return true;}
            if(Load(T, f, ih, name))goto ok;
         }
      }break;

      case 0:
      {
         f>>fh; if(f.ok())
         {
            ImageHeader ih;
            Unaligned(ih.size    , fh.size);
            Unaligned(ih.type    , fh.type); ih.type=OldImageType3(ih.type);
            Unaligned(ih.mode    , fh.mode);
           _Unaligned(ih.mip_maps, fh.mips);
            if(header){*header=ih; return true;}
            if(Load(T, f, ih, name))goto ok;
         }
      }break;
   }

//error:
   if(header)header->zero();
   del(); return false;

ok:
   if(App.flag&APP_AUTO_FREE_IMAGE_OPEN_GL_ES_DATA)freeOpenGLESData();
   return true;
}
Bool Image::_loadData(File &f, ImageHeader *header, C Str &name)
{
   ImageHeader ih;
   switch(f.decUIntV())
   {
      case 4:
      {
         ImageFileHeader fh; f>>fh;
         Unaligned(ih.size    , fh.size);
         Unaligned(ih.type    , fh.type); ih.type=OldImageType2(ih.type);
         Unaligned(ih.mode    , fh.mode);
        _Unaligned(ih.mip_maps, fh.mips);
         if(header)goto set_header;
         if(Load(T, f, ih, name))goto ok;
      }break;

      case 3:
      {
         ImageFileHeader fh; f>>fh;
         Unaligned(ih.size    , fh.size);
         Unaligned(ih.type    , fh.type); ih.type=OldImageType1(ih.type);
         Unaligned(ih.mode    , fh.mode);
        _Unaligned(ih.mip_maps, fh.mips);
         if(header)goto set_header;
         if(Load(T, f, ih, name))goto ok;
      }break;

      case 2:
      {
         f.getByte(); // old U16 version part
         ih.size.x  =f.getInt();
         ih.size.y  =f.getInt();
         ih.size.z  =f.getInt();
         Byte byte_pp =f.getByte();
         Byte old_type=f.getByte(); ih.type=OldImageType0(old_type);
         ih.mode    =IMAGE_MODE(f.getByte()); if(ih.mode==IMAGE_CUBE && ih.size.z==6)ih.size.z=1;
         ih.mip_maps=           f.getByte();
         if(header)goto set_header;
         if(old_type==6)f.skip(SIZE(Color)*256); // palette
         if(createTry(ih.size.x, ih.size.y, ih.size.z, ih.type, ih.mode, ih.mip_maps))
         {
            Image soft;
            FREPD(mip ,  ih.mip_maps                 ) // iterate all mip maps
            FREPD(face, (ih.mode==IMAGE_CUBE) ? 6 : 1) // iterate all faces
            {
               Image *dest=this;
               Int    dest_mip=mip, dest_face=face;
               if(hwType()!=ih.type){if(!soft.createTry(Max(1, ih.size.x>>mip), Max(1, ih.size.y>>mip), Max(1, ih.size.z>>mip), ih.type, IMAGE_SOFT, 1, false))return false; dest=&soft; dest_mip=0; dest_face=0;} // if 'hwType' is different than of file, then load into 'file_type' IMAGE_SOFT, after creating the mip map its Pitch and BlocksY may be different than of calculated from base (for example non-power-of-2 images) so skip some data from file to read only created

               if(!dest->lock(LOCK_WRITE, dest_mip, DIR_ENUM(dest_face)))return false;
               Int file_pitch   =ImagePitch  (ih.size.x, ih.size.y, mip, ih.type), dest_pitch   =Min(dest->pitch()                                                   , file_pitch   ),
                   file_blocks_y=ImageBlocksY(ih.size.x, ih.size.y, mip, ih.type), dest_blocks_y=Min(ImageBlocksY(dest->hwW(), dest->hwH(), dest_mip, dest->hwType()), file_blocks_y);
               FREPD(z, dest->ld())
               {
                  FREPD(y, dest_blocks_y){f.getFast(dest->data() + y*dest->pitch() + z*dest->pitch2(), dest_pitch); f.skip(file_pitch-dest_pitch);}
                  f.skip((file_blocks_y-dest_blocks_y)*file_pitch); // unread blocksY * pitch
               }
               if(ih.mode==IMAGE_SOFT && (ih.type==IMAGE_R8G8B8 || ih.type==IMAGE_R8G8B8_SRGB)){Byte *bgr=dest->data(); REP(dest->lw()*dest->lh()*dest->ld()){Swap(bgr[0], bgr[2]); bgr+=3;}}
               dest->unlock();

               if(hwType()!=ih.type)if(!injectMipMap(*dest, mip, DIR_ENUM(face)))return false;
            }
            if(f.ok())goto ok;
         }
      }break;

      case 1:
      {
         f.getByte(); // old U16 version part
         ih.size.x=f.getInt();
         ih.size.y=f.getInt();
         ih.size.z=f.getInt();
         Byte byte_pp =f.getByte();
         Byte old_type=f.getByte(); ih.type=OldImageType0(old_type);
         ih.mode    =IMAGE_MODE(f.getByte()); if(ih.mode==IMAGE_CUBE && ih.size.z==6)ih.size.z=1;
         ih.mip_maps=           f.getByte();
         if(header)goto set_header;
         if(old_type==6)f.skip(SIZE(Color)*256); // palette
         if(createTry(ih.size.x, ih.size.y, ih.size.z, ih.type, ih.mode, ih.mip_maps))
         {
            Image soft;
            FREPD(mip ,  ih.mip_maps                 ) // iterate all mip maps
            FREPD(face, (ih.mode==IMAGE_CUBE) ? 6 : 1) // iterate all faces
            {
               Image *dest=this;
               Int    dest_mip=mip, dest_face=face;
               if(hwType()!=ih.type){if(!soft.createTry(Max(1, ih.size.x>>mip), Max(1, ih.size.y>>mip), Max(1, ih.size.z>>mip), ih.type, IMAGE_SOFT, 1, false))return false; dest=&soft; dest_mip=0; dest_face=0;} // if 'hwType' is different than of file, then load into 'file_type' IMAGE_SOFT

               if(!dest->lock(LOCK_WRITE, dest_mip, DIR_ENUM(dest_face)))return false;
               Int file_pitch   =dest->lw(),
                   file_blocks_y=dest->lh(); if(ImageTI[ih.type].compressed){file_blocks_y=DivCeil4(file_blocks_y); file_pitch*=4; if(file_pitch<16)file_pitch=16;} file_pitch*=ImageTI[ih.type].bit_pp; file_pitch/=8;
               Int dest_pitch   =Min(dest->pitch()                                                   , file_pitch   ),
                   dest_blocks_y=Min(ImageBlocksY(dest->hwW(), dest->hwH(), dest_mip, dest->hwType()), file_blocks_y);
               FREPD(z, dest->ld())
               {
                  FREPD(y, dest_blocks_y){f.getFast(dest->data() + y*dest->pitch() + z*dest->pitch2(), dest_pitch); f.skip(file_pitch-dest_pitch);}
                  f.skip((file_blocks_y-dest_blocks_y)*file_pitch); // unread blocksY * pitch
               }
               if(ih.mode==IMAGE_SOFT && (ih.type==IMAGE_R8G8B8 || ih.type==IMAGE_R8G8B8_SRGB)){Byte *bgr=dest->data(); REP(dest->lw()*dest->lh()*dest->ld()){Swap(bgr[0], bgr[2]); bgr+=3;}}
               dest->unlock();

               if(hwType()!=ih.type)if(!injectMipMap(*dest, mip, DIR_ENUM(face)))return false;
            }
            if(f.ok())goto ok;
         }
      }break;

      case 0:
      {
         f.getByte(); // old U16 version part
         ih.size.x=f.getInt();
         ih.size.y=f.getInt();
         ih.size.z=1;
         Byte byte_pp =f.getByte();
         Byte old_type=f.getByte(); ih.type=OldImageType0(old_type);
         ih.mip_maps  =f.getByte();
         ih.mode      =IMAGE_MODE(f.getByte()); if(ih.mode==1)ih.mode=IMAGE_SOFT;else if(ih.mode==0)ih.mode=IMAGE_2D; if(ih.mode==IMAGE_CUBE && ih.size.z==6)ih.size.z=1;
         if(header)goto set_header;
         if(old_type==6)f.skip(SIZE(Color)*256); // palette
         if(createTry(ih.size.x, ih.size.y, ih.size.z, ih.type, ih.mode, ih.mip_maps))
         {
            Image soft;
            FREPD(mip ,  ih.mip_maps                 ) // iterate all mip maps
            FREPD(face, (ih.mode==IMAGE_CUBE) ? 6 : 1) // iterate all faces
            {
               Image *dest=this;
               Int    dest_mip=mip, dest_face=face;
               if(hwType()!=ih.type){if(!soft.createTry(Max(1, ih.size.x>>mip), Max(1, ih.size.y>>mip), Max(1, ih.size.z>>mip), ih.type, IMAGE_SOFT, 1, false))return false; dest=&soft; dest_mip=0; dest_face=0;} // if 'hwType' is different than of file, then load into 'file_type' IMAGE_SOFT

               if(!dest->lock(LOCK_WRITE, dest_mip, DIR_ENUM(dest_face)))return false;
               Int file_pitch   =dest->lw(),
                   file_blocks_y=dest->lh(); if(ImageTI[ih.type].compressed){file_blocks_y=DivCeil4(file_blocks_y); file_pitch*=4; if(file_pitch<16)file_pitch=16;} file_pitch*=ImageTI[ih.type].bit_pp; file_pitch/=8;
               Int dest_pitch   =Min(dest->pitch()                                                   , file_pitch   ),
                   dest_blocks_y=Min(ImageBlocksY(dest->hwW(), dest->hwH(), dest_mip, dest->hwType()), file_blocks_y);
               FREPD(z, dest->ld())
               {
                  FREPD(y, dest_blocks_y){f.getFast(dest->data() + y*dest->pitch() + z*dest->pitch2(), dest_pitch); f.skip(file_pitch-dest_pitch);}
                  f.skip((file_blocks_y-dest_blocks_y)*file_pitch); // unread blocksY * pitch
               }
               if(ih.mode==IMAGE_SOFT && (ih.type==IMAGE_R8G8B8 || ih.type==IMAGE_R8G8B8_SRGB)){Byte *bgr=dest->data(); REP(dest->lw()*dest->lh()*dest->ld()){Swap(bgr[0], bgr[2]); bgr+=3;}}
               dest->unlock();

               if(hwType()!=ih.type)if(!injectMipMap(*dest, mip, DIR_ENUM(face)))return false;
            }
            if(f.ok())goto ok;
         }
      }break;
   }
error:
   if(header)header->zero();
   del(); return false;

ok:
   if(App.flag&APP_AUTO_FREE_IMAGE_OPEN_GL_ES_DATA)freeOpenGLESData();
   return true;

set_header:
   if(f.ok()){*header=ih; return true;}
   goto error;
}
/******************************************************************************/
Bool Image::save(File &f)C
{
   f.putUInt(CC4_IMG);
   return saveData(f);
}
Bool Image::load(File &f)
{
   switch(f.getUInt())
   {
      case CC4_IMG: return  loadData(f);
      case CC4_GFX: return _loadData(f);
   }
   del(); return false;
}

Bool Image::save(C Str &name)C
{
   File f; if(f.writeTry(name)){if(save(f) && f.flush())return true; f.del(); FDelFile(name);}
   return false;
}
Bool Image::load(C Str &name)
{
   File f; if(f.readTryEx(name, null, null, true))switch(f.getUInt()) // allow streaming
   {
      case CC4_IMG: return  loadData(f, null, name, true); // can delete 'f' because it's a temporary
      case CC4_GFX: return _loadData(f, null, name);
   }
   del(); return false;
}
void Image::operator=(C UID &id  ) {T=_EncodeFileName(id);}
void Image::operator=(C Str &name)
{
   if(!load(name))Exit(MLT(S+"Can't load Image \""         +name+ "\", possible reasons:\n-Video card doesn't support required format\n-File not found\n-Out of memory\n-Engine not yet initialized",
                       PL,S+u"Nie można wczytać Obrazka \""+name+u"\", możliwe przyczyny:\n-Karta graficzna nie obsługuje wczytywanego formatu\n-Nie odnaleziono pliku\n-Skończyła się pamięć\n-Silnik nie został jeszcze zainicjalizowany"));
}
/******************************************************************************/
// IMPORT
/******************************************************************************/
Bool Image::Export(C Str &name, Flt rgb_quality, Flt alpha_quality, Flt compression_level, Int sub_sample)C
{
   CChar   *ext=_GetExt(name);
   if(Equal(ext, "img" ))return save      (name);
   if(Equal(ext, "bmp" ))return ExportBMP (name);
   if(Equal(ext, "png" ))return ExportPNG (name, compression_level);
   if(Equal(ext, "jpg" ))return ExportJPG (name, rgb_quality, sub_sample);
   if(Equal(ext, "webp"))return ExportWEBP(name, rgb_quality, alpha_quality);
   if(Equal(ext, "heif"))return ExportHEIF(name, rgb_quality);
   if(Equal(ext, "tga" ))return ExportTGA (name);
   if(Equal(ext, "tif" ))return ExportTIF (name, compression_level);
   if(Equal(ext, "dds" ))return ExportDDS (name);
   if(Equal(ext, "ico" ))return ExportICO (name);
   if(Equal(ext, "icns"))return ExportICNS(name);
                         return false;
}
/******************************************************************************/
Bool Image::ImportTry(File &f, Int type, Int mode, Int mip_maps)
{
   Long pos=f.pos();
                         if(load      (f))goto ok;
   f.resetOK().pos(pos); if(ImportBMP (f))goto ok;
   f.resetOK().pos(pos); if(ImportPNG (f))goto ok;
   f.resetOK().pos(pos); if(ImportJPG (f))goto ok;
   f.resetOK().pos(pos); if(ImportWEBP(f))goto ok;
   f.resetOK().pos(pos); if(ImportHEIF(f))goto ok;
   f.resetOK().pos(pos); if(ImportTIF (f))goto ok; // import after PNG/JPG in case LibTIFF tries to decode them too
   f.resetOK().pos(pos); if(ImportDDS (f, type, mode, mip_maps))goto ok;
   f.resetOK().pos(pos); if(ImportPSD (f))goto ok;
   f.resetOK().pos(pos); if(ImportHDR (f))goto ok;
   f.resetOK().pos(pos); if(ImportICO (f))goto ok;
 //f.resetOK().pos(pos); if(ImportTGA (f, type, mode, mip_maps))goto ok; TGA format doesn't contain any special signatures, so we can't check it
   del(); return false;
ok:
   return copyTry(T, -1, -1, -1, type, mode, mip_maps);
}
Bool Image::ImportTry(C Str &name, Int type, Int mode, Int mip_maps)
{
   if(!name.is()){del(); return true;}
   File f; if(f.readTry(name))
   {
      if(ImportTry(f, type, mode, mip_maps))return true;
      CChar *ext=_GetExt(name);
      if(Equal(ext, "tga") && f.resetOK().pos(0) && ImportTGA(f, type, mode, mip_maps))return true; // TGA format doesn't contain any special signatures, so check extension instead
   }
   del(); return false;
}
Image& Image::mustImport(File &f, Int type, Int mode, Int mip_maps)
{
   if(!ImportTry(f, type, mode, mip_maps))Exit(MLT(S+"Can't import image", PL,S+u"Nie można zaimportować obrazka"));
   return T;
}
Image& Image::mustImport(C Str &name, Int type, Int mode, Int mip_maps)
{
   if(!ImportTry(name, type, mode, mip_maps))Exit(MLT(S+"Can't import image \""+name+'"', PL,S+u"Nie można zaimportować obrazka \""+name+'"'));
   return T;
}
/******************************************************************************/
Bool Image::ImportCubeTry(C Image &right, C Image &left, C Image &up, C Image &down, C Image &forward, C Image &back, Int type, Bool soft, Int mip_maps, Bool resize_to_pow2, FILTER_TYPE filter)
{
   Int size= Max(right  .w(), right  .h(), left.w(), left.h()) ;
   MAX(size, Max(up     .w(), up     .h(), down.w(), down.h()));
   MAX(size, Max(forward.w(), forward.h(), back.w(), back.h()));
   if(resize_to_pow2)size=NearestPow2(size);
   if(createTry(size, size, 1, IMAGE_TYPE((type<=0) ? right.type() : type), soft ? IMAGE_SOFT_CUBE : IMAGE_CUBE, mip_maps))
   {
      injectMipMap(right  , 0, DIR_RIGHT  , filter);
      injectMipMap(left   , 0, DIR_LEFT   , filter);
      injectMipMap(up     , 0, DIR_UP     , filter);
      injectMipMap(down   , 0, DIR_DOWN   , filter);
      injectMipMap(forward, 0, DIR_FORWARD, filter);
      injectMipMap(back   , 0, DIR_BACK   , filter);
      updateMipMaps(); return true;
   }
   del(); return false;
}
Bool Image::ImportCubeTry(C Str &right, C Str &left, C Str &up, C Str &down, C Str &forward, C Str &back, Int type, Bool soft, Int mip_maps, Bool resize_to_pow2, FILTER_TYPE filter)
{
   Image r, l, u, d, f, b;
   if(r.ImportTry(right  , -1, IMAGE_SOFT, 1) // use OR to proceed if at least one was imported
   |  l.ImportTry(left   , -1, IMAGE_SOFT, 1) // use single OR "|" to call import for all images (double OR "||" would continue on first success)
   |  u.ImportTry(up     , -1, IMAGE_SOFT, 1)
   |  d.ImportTry(down   , -1, IMAGE_SOFT, 1)
   |  f.ImportTry(forward, -1, IMAGE_SOFT, 1)
   |  b.ImportTry(back   , -1, IMAGE_SOFT, 1))return ImportCubeTry(r, l, u, d, f, b, type, soft, mip_maps, resize_to_pow2, filter);
   del(); return false;
}
Image& Image::mustImportCube(C Str &right, C Str &left, C Str &up, C Str &down, C Str &forward, C Str &back, Int type, Bool soft, Int mip_maps, Bool resize_to_pow2, FILTER_TYPE filter)
{
   if(!ImportCubeTry(right, left, up, down, forward, back, type, soft, mip_maps, resize_to_pow2, filter))Exit(MLT(S+"Can't import images as Cube Texture \""              +right+"\", \""+left+"\", \""+up+"\", \""+down+"\", \""+forward+"\", \""+back+'"',
                                                                                                              PL,S+u"Nie można zaimportować obrazków jako Cube Texture \""+right+"\", \""+left+"\", \""+up+"\", \""+down+"\", \""+forward+"\", \""+back+'"'));
   return T;
}
/******************************************************************************/
Bool ImageLoadHeader(File &f, ImageHeader &header)
{
   Image temp;
   Long  pos=f.pos(); // remember file position
   Bool  ok =false;
   switch(f.getUInt())
   {
      case CC4_IMG: ok=temp. loadData(f, &header); break;
      case CC4_GFX: ok=temp._loadData(f, &header); break;
   }
   f.pos(pos); // reset file position
   if(ok)return true;
   header.zero(); return false;
}
Bool ImageLoadHeader(C Str &name, ImageHeader &header)
{
   File f; if(f.readTryEx(name, null, null, true))return ImageLoadHeader(f, header); // allow streaming
   header.zero(); return false;
}
/******************************************************************************/
// STREAM
/******************************************************************************/
static Bool StreamLoadFunc(Thread &thread)
{
   StreamLoadEvent.wait();
   StreamLoad sl; // outside loop to minimize overhead
   for(; StreamLoads.elms(); )
   {
      {
         MemcThreadSafeLock lock(StreamLoads);
     if(!StreamLoads.elms())break; // if removed before got lock then try again later
         StreamLoads.lockedSwapPop(sl);
         StreamLoadCur=sl.image;
      }
    //if(StreamLoadCur) now this isn't needed because we remove canceled 'StreamLoads' instead of canceling them // if wasn't canceled, we don't need 'StreamLoadCurLock' here, because we don't access it yet, that can wait
      {
         sl.loader.f=&sl.f; // have to adjust because memory address got changed due to swap
         sl.loader.update();
         StreamLoadCur=null;
      }
   }
   return true;
}
static void StreamSetsFunc()
{
   SyncLocker         d_lock(D._lock);
   MemcThreadSafeLock s_lock(StreamSets);
   // this must be separated into 3 steps for best efficiency. When setting 'baseMip' from end to start, it can't be merged with 'setMipData', because when it adjusts 'baseMip' other threads might check 'baseMip', and access some mip maps, even though they would still need to be processed via 'setMipData' here. So do all 'setMipData' first, and then adjust 'baseMip' for biggest mips that we have first. Separating like this requires that all 'StreamSets' that we have will be processed.
   FREPA(StreamSets)StreamSets.lockedElm(i).setMipData(); // set all mip data, order is not important much, but probably better go forward, so next step going back will have elements already in RAM cache
    REPA(StreamSets)StreamSets.lockedElm(i).baseMip   (); // now we have to set 'baseMip', which recreates SRV, it's best if we go from end, and already create for the biggest mip we have, further smaller mips we'll just ignore, remember that when setting 'baseMip' then all mips for that range must have its data already set (because other threads might access it)
   FREPA(StreamSets)StreamSets.lockedElm(i).finish    (); // this have to process in order, because once '_streaming' is disabled, we can't access that 'image' any more, it could get deleted and removed from memory
   StreamSets.clear();
}
void CancelAllStreamLoads() // this force cancels all when we want to shut down
{
   // cancellation order is important! First the 'StreamLoads' source, then next steps
   {MemcThreadSafeLock lock(StreamLoads      ); REPA(StreamLoads)StreamLoads.lockedElm(i).cancel(); StreamLoads.lockedDel();} // have to cancel before del
   {SyncLocker         lock(StreamLoadCurLock);                                           Cancel(   StreamLoadCur);}
   {MemcThreadSafeLock lock(StreamSets       ); REPA(StreamSets )StreamSets .lockedElm(i).cancel(); StreamSets .lockedDel();} // have to cancel before del
}
void Image::cancelStream() // called when image is deleted
{
   if(_streaming)
   {
      // cancellation order is important! First the 'StreamLoads' source, then next steps
      {MemcThreadSafeLock lock(StreamLoads      ); REPA(StreamLoads)if(StreamLoads.lockedElm(i).image==this)StreamLoads.lockedRemove(i);} // no need to keep order
      {SyncLocker         lock(StreamLoadCurLock);                                   if(StreamLoadCur==this)StreamLoadCur=null;}
      {MemcThreadSafeLock lock(StreamSets       ); REPA(StreamSets )   StreamSets .lockedElm(i).canceled(T );} // cancel instead of remove, this will be faster !! ALSO WE NEED TO KEEP ORDER !!
     _streaming=false;
   }
}
static inline Bool Wait(Image &image, Int mip) {return mip<image._base_mip && image._streaming;}
Bool Image::waitForStream(Int mip)
{
   if(Wait(T, mip))
   {
      AtomicInc(StreamLoadWaiting);
   again:
      if(StreamSets.elms())StreamSetsFunc(); // process all sets
      if(Wait(T, mip))
      {
         StreamLoaded.wait(); // wait until next got loaded
         goto again;
      }
      AtomicDec(StreamLoadWaiting);
   }
   return mip>=_base_mip;
}
void ShutStreamLoads()
{
   StreamLoadThread.stop(); // request stop
   CancelAllStreamLoads (); // cancel all loads
   StreamLoadEvent .on  (); // wake up to exit
   StreamLoadThread.del (); // delete
}
/******************************************************************************/
}
/******************************************************************************/
