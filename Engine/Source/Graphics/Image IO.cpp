/******************************************************************************/
#include "stdafx.h"
namespace EE{
/******************************************************************************/
#define CC4_IMG CC4('I','M','G',0)
#define CC4_GFX CC4('G','F','X',0)
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
// SAVE / LOAD
/******************************************************************************/
Bool Image::saveData(File &f)C
{
   IMAGE_TYPE file_type=T.type(); // set image type as to be stored in the file
   if(!CanCompress(file_type))file_type=T.hwType(); // if compressing to format which isn't supported then store as current 'hwType'
   if(file_type>=IMAGE_TYPES               // don't allow saving private formats
   || mode()   >=IMAGE_RT   )return false; // don't allow saving private modes
   ASSERT(IMAGE_2D==0 && IMAGE_3D==1 && IMAGE_CUBE==2 && IMAGE_SOFT==3 && IMAGE_SOFT_CUBE==4 && IMAGE_RT==5);

   f.putMulti(Byte(1), size3(), Byte(file_type), Byte(mode()), Byte(mipMaps())); // version

   if(soft() && CanDoRawCopy(hwType(), file_type)) // software with matching type, we can save without locking
   {
      if(hwSize3()==size3())f.put(softData(), memUsage());else // exact size, then we can save entire memory
      {
       C Byte *data =softData();
         Int   faces=T.faces();
         FREPD(mip, mipMaps()) // iterate all mip maps
         {
            // here no need to use any "Min" because soft HW sizes are guaranteed to be >= file sizes
            Int file_pitch   =ImagePitch  (  w(),   h(), mip, file_type), // use "w(), h()" instead of "hwW(), hwH()" because we want to write only valid pixels
                file_blocks_y=ImageBlocksY(  w(),   h(), mip, file_type), // use "w(), h()" instead of "hwW(), hwH()" because we want to write only valid pixels
               image_pitch   =ImagePitch  (hwW(), hwH(), mip, file_type),
               image_blocks_y=ImageBlocksY(hwW(), hwH(), mip, file_type),
                file_d       =             Max(1,   d()>>mip)           , // use "d()"      instead of "hwD()"        because we want to write only valid pixels
               image_d       =             Max(1, hwD()>>mip)           ,
                write        =  file_blocks_y*file_pitch,
               skip          =(image_blocks_y-file_blocks_y)*image_pitch,
               skip2         =(image_d       -file_d       )*image_pitch*image_blocks_y;
            FREPD(face, faces) // iterate all faces
            {
               FREPD(z, file_d)
               {
                  if(file_pitch==image_pitch) // if file pitch is the same as image pitch
                  {
                     f.put(data, write); data+=write; // we can write both XY in one go
                  }else
                  FREPD(y, file_blocks_y){f.put(data, file_pitch); data+=image_pitch;}
                  data+=skip;
               }
               data+=skip2;
            }
         }
      }
   }else
   {
      Image soft;
      Int   faces=T.faces();
      FREPD(mip, mipMaps()) // iterate all mip maps
      {
         Int file_pitch   =ImagePitch  (w(), h(), mip, file_type), // use "w(), h()" instead of "hwW(), hwH()" because we want to write only valid pixels
             file_blocks_y=ImageBlocksY(w(), h(), mip, file_type); // use "w(), h()" instead of "hwW(), hwH()" because we want to write only valid pixels
         FREPD(face, faces) // iterate all faces
         {
          C Image *src=this;
            Int    src_mip=mip, src_face=face;
            if(!CanDoRawCopy(hwType(), file_type)){if(!extractMipMap(soft, file_type, mip, DIR_ENUM(face)))return false; src=&soft; src_mip=0; src_face=0;} // if 'hwType' is different than of file, then convert to 'file_type' IMAGE_SOFT, after extracting the mip map its Pitch and BlocksY may be different than of calculated from base (for example non-power-of-2 images) so write zeros to file to match the expected size

            if(!src->lockRead(src_mip, DIR_ENUM(src_face)))return false;
            Int write_pitch   =Min(src->pitch()                                            , file_pitch   ),
                write_blocks_y=Min(ImageBlocksY(src->w(), src->h(), src_mip, src->hwType()), file_blocks_y); // use "w(), h()" instead of "hwW(), hwH()" because we want to write only valid pixels
            FREPD(z, src->ld())
            {
             C Byte *src_data_z=src->data() + z*src->pitch2();
               if(file_pitch==src->pitch()) // if file pitch is the same as image pitch !! compare 'src->pitch' and not 'write_pitch' !!
                  f.put(src_data_z, write_blocks_y*write_pitch);else // we can write both XY in one go !! use "write_blocks_y*write_pitch" and not 'pitch2', because 'pitch2' may be bigger !!
               {
                  Int skip=file_pitch-write_pitch;
                  FREPD(y, write_blocks_y){f.put(src_data_z + y*src->pitch(), write_pitch); f.put(null, skip);} // write each line separately
               }
               f.put(null, (file_blocks_y-write_blocks_y)*file_pitch); // unwritten blocksY * pitch
            }
            src->unlock();
         }
      }
   }
   return f.ok();
}
/******************************************************************************/
static INLINE Bool SizeFits (  Int   src,   Int   dest) {return src<dest*2;} // this is OK for src=7, dest=4 (7<4*2), but NOT OK for src=8, dest=4 (8<4*2)
static INLINE Bool SizeFits1(  Int   src,   Int   dest) {return src>1 && SizeFits(src, dest);} // only if 'src>1', if we don't check this, then 1024x1024x1 src will fit into 16x16x1 dest because of Z=1
static        Bool SizeFits (C VecI &src, C VecI &dest) {return SizeFits1(src.x, dest.x) || SizeFits1(src.y, dest.y) || SizeFits1(src.z, dest.z) || (src.x==1 && src.y==1 && src.z==1);}
static Bool Load(Image &image, File &f, C ImageHeader &header, C Str &name)
{
   if(!f.ok())return false;

   ImageHeader want=header;
   Int         shrink=0;
   if(Int (*image_load_shrink)(ImageHeader &image_header, C Str &name)=D.image_load_shrink) // copy to temp variable to avoid multi-threading issues
   {
      shrink=image_load_shrink(want, name);

      // adjust mip-maps, we will need this for load from file memory
      Int total_mip_maps=TotalMipMaps(want.size.x, want.size.y, want.size.z, want.type); // don't use hardware texture size hwW(), hwH(), hwD(), so that number of mip-maps will always be the same (and not dependant on hardware capabilities like TexPow2 sizes), also because 1x1 image has just 1 mip map, but if we use padding then 4x4 block would generate 3 mip maps
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
            FREPD(i, file_mip)f_data+=ImageMipSize(header.size.x, header.size.y, header.size.z, i, header.type)*file_faces; // skip data that we don't want
            if(image.createEx(want.size.x, want.size.y, want.size.z, want.type, want.mode, want.mip_maps, 1, f_data))
            {
               for(; file_mip<header.mip_maps; file_mip++)f_data+=ImageMipSize(header.size.x, header.size.y, header.size.z, file_mip, header.type)*file_faces; // skip remaining mip maps
               f.skip(f_data-(Byte*)f.memFast()); // skip to the end of the image
               return true;
            }
         }
         break;
      }else
      if(file_mip_size.x<want.size.x
      || file_mip_size.y<want.size.y
      || file_mip_size.z<want.size.z)break; // if any dimension of processed file mip is smaller than what we want then stop
   }

   // create
   if(image.createTry(want.size.x, want.size.y, want.size.z, want.type, create_from_soft ? (IsCube(want.mode) ? IMAGE_SOFT_CUBE : IMAGE_SOFT) : want.mode, want.mip_maps)) // don't use 'want' after this call, instead operate on 'image' members
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
            /*
            Watch out for following cases:
               image.width=497, image.type=BC1
               file.mip0.padded_width=Ceil4(497   )=500
               file.mip1.padded_width=Ceil4(497>>1)=248
              image.mip0.padded_width=Ceil4(497   )=500
              image.mip1.padded_width=Ceil4(500>>1)=252, image mip pitch is calculated based on HW size, so it may be bigger than file mip
            */
            if(fast_load && image_mip_size==file_mip_size) // if this is a software image with same type/cube and mip size matches
            {
               if(image_mip_size.x==Max(1, image.hwW()>>image_mip)
               && image_mip_size.y==Max(1, image.hwH()>>image_mip)
               && image_mip_size.z==Max(1, image.hwD()>>image_mip)) // if image mip size is the same as image HW mip size, then we can read all remaining data in one go
               {
                  mip_count=Min(image.mipMaps()-image_mip, header.mip_maps-file_mip); // how many mip-maps we can read = Min(available in 'image', available in file)
                  f.getFast(image.softData(image_mip), ImageSize(file_mip_size.x, file_mip_size.y, file_mip_size.z, header.type, header.mode, mip_count));
                  file_mip+=mip_count-1; // -1 because 1 is already added at the end of FREPD loop
               }else
               {
                  mip_count=1;
                  // here no need to use any "Min" because soft HW sizes are guaranteed to be >= file sizes
            const Int file_pitch   =ImagePitch  (header.size.x, header.size.y,  file_mip, header.type),
                      file_blocks_y=ImageBlocksY(header.size.x, header.size.y,  file_mip, header.type),
                     image_pitch   =ImagePitch  ( image.hwW() ,  image.hwH() , image_mip, header.type),
                     image_blocks_y=ImageBlocksY( image.hwW() ,  image.hwH() , image_mip, header.type),
                      file_d       =      Max(1, header.size.z>> file_mip),
                     image_d       =      Max(1,  image.hwD() >>image_mip),
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

         const Int file_pitch   =ImagePitch  (header.size.x, header.size.y, file_mip,  header.type  ),
                   file_blocks_y=ImageBlocksY(header.size.x, header.size.y, file_mip,  header.type  ),
                   dest_blocks_y=ImageBlocksY(  dest->hwW(),   dest->hwH(), dest_mip, dest->hwType()),
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
            f.skip(ImageMipSize(header.size.x, header.size.y, header.size.z, file_mip, header.type)*file_faces);
         }
      }
      if(image_mip)image.updateMipMaps(filter, IC_CLAMP, image_mip-1); // set any missing mip maps, this is needed for example if file had 1 mip map, but we've requested to create more
      else         image.clear(); // or if we didn't load anything, then clear to zero

      if(image.mode()!=want.mode) // if created as SOFT, then convert to HW
      {
         Swap(soft, image); // can't create from self
         if(!image.createEx(soft.w(), soft.h(), soft.d(), soft.type(), want.mode, soft.mipMaps(), soft.samples(), null, &soft
            #if GL_ES
               , true // allow deleting 'soft' src
            #endif
         ))
         {
            VecI size=soft.size3(); IMAGE_TYPE type=soft.type(); // remember params for adjust later
            soft.adjustInfo(soft.hwW(), soft.hwH(), soft.d(), soft.type()); // adjust internal size to 'hwW', 'hwH' because we must allocate entire HW size for 'type' to have enough room for its data, for example 48x48 PVRTC requires 64x64 size 7 mip maps, while RGBA would give us 48x48 size 6 mip maps, this is to achieve consistent results (have the same sizes, and mip maps) and it's also a requirement for saving, doing this instead of using 'copyTry' with FILTER_NO_STRETCH allows for faster 'Decompress' (directly into 'target'), no need to explicitly specify mip maps instead of using -1 because they could change due to different sizes)
            for(IMAGE_TYPE alt_type=type; ; )
            {
               alt_type=ImageTypeOnFail(alt_type); if(!alt_type)return false;
               if(ImageSupported (alt_type, want.mode, soft.samples()) // do a quick check before 'copyTry' to avoid it if we know creation will fail
               && soft .  copyTry(soft, -1, -1, -1, alt_type, -1, -1, FILTER_BEST, IC_CONVERT_GAMMA) // we have to keep depth, soft mode, mip maps, make sure gamma conversion is performed
               && image.createEx (soft.w(), soft.h(), soft.d(), soft.type(), want.mode, soft.mipMaps(), soft.samples(), null, &soft
                  #if GL_ES
                     , true // allow deleting 'soft' src
                  #endif
               ))break; // success
            }
            image.adjustInfo(size.x, size.y, size.z, type);
         }
      }

      return f.ok();
   }
   return false;
}
#pragma pack(push, 1)
struct ImageFileHeader
{
   VecI       size;
   IMAGE_TYPE type;
   IMAGE_MODE mode;
   Byte       mips;
};
#pragma pack(pop)
Bool Image::loadData(File &f, ImageHeader *header, C Str &name)
{
   ImageHeader ih;
   switch(f.decUIntV())
   {
      case 1:
      {
         ImageFileHeader fh; f>>fh;
         Unaligned(ih.size    , fh.size);
         Unaligned(ih.type    , fh.type);
         Unaligned(ih.mode    , fh.mode);
        _Unaligned(ih.mip_maps, fh.mips);
         if(header)goto set_header;
         if(Load(T, f, ih, name))goto ok;
      }break;

      case 0:
      {
         ImageFileHeader fh; f>>fh;
         Unaligned(ih.size    , fh.size);
         Unaligned(ih.type    , fh.type); ih.type=OldImageType3(ih.type);
         Unaligned(ih.mode    , fh.mode);
        _Unaligned(ih.mip_maps, fh.mips);
         if(header)goto set_header;
         if(Load(T, f, ih, name))goto ok;
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
   File f; if(f.readTry(name))switch(f.getUInt())
   {
      case CC4_IMG: return  loadData(f, null, name);
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
ok:;
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
   File f; if(f.readTry(name))return ImageLoadHeader(f, header);
   header.zero(); return false;
}
/******************************************************************************/
}
/******************************************************************************/
