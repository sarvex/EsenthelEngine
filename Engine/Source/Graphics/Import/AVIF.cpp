/******************************************************************************/
#include "stdafx.h"

#if SUPPORT_AVIF
   #include "../../../../ThirdPartyLibs/begin.h"

   #include "../../../../ThirdPartyLibs/AVIF/lib/include/avif/avif.h"

   #include "../../../../ThirdPartyLibs/end.h"
#endif

namespace EE{
/******************************************************************************/
Bool Image::ImportAVIF(File &f)
{
   Bool ok=false;
#if SUPPORT_AVIF
   const Byte sig[]={0,0,0,' ','f','t','y','p','a','v','i','f'};
         Byte data[Elms(sig)];
   if(f.getFast(data) && EqualMem(sig, data) && f.skip(-SIZEI(sig)))
   {
      Memt<Byte> data; data.setNumDiscard(f.left());
      if(f.getFast(data.data(), data.elms()))
      if(avifDecoder *decoder=avifDecoderCreate())
      {
         // here can override decoder defaults (codecChoice, requestedSource, ignoreExif, ignoreXMP, etc)
         if(avifDecoderSetIOMemory(decoder, data.data(), data.elms())==AVIF_RESULT_OK)
         if(avifDecoderParse(decoder)==AVIF_RESULT_OK)
         {
            avifRGBImage rgb; avifRGBImageSetDefaults(&rgb, decoder->image);
            if(avifDecoderNextImage(decoder)==AVIF_RESULT_OK)
            {
               Bool alpha= decoder->alphaPresent,
                       hp=(decoder->image->depth>8);
               if(createSoft(decoder->image->width, decoder->image->height, 1, hp ? (alpha ? IMAGE_F16_4 : IMAGE_F16_3) : (alpha ? IMAGE_R8G8B8A8_SRGB : IMAGE_R8G8B8_SRGB)))
               {
                  rgb.pixels  =T.data();
                  rgb.rowBytes=T.pitch();
                  rgb.format  =(alpha ? AVIF_RGB_FORMAT_RGBA : AVIF_RGB_FORMAT_RGB);
                //rgb.isFloat =(decoder->image->depth==16); need to test gamma
                  Flt mul=1.0f/((1<<decoder->image->depth)-1);

                  if(avifImageYUVToRGB(decoder->image, &rgb)==AVIF_RESULT_OK)
                  {
                     if(hp)// && !rgb.isFloat)
                     REPD(y, h())
                     {
                        Byte *dy=T.data() + y*pitch();
                        REPD(x, w())
                        {
                           Byte *d=dy + x*bytePP();
                                    ((Half*)d)[0]=SRGBToLinear(((U16*)d)[0]*mul);
                                    ((Half*)d)[1]=SRGBToLinear(((U16*)d)[1]*mul);
                                    ((Half*)d)[2]=SRGBToLinear(((U16*)d)[2]*mul);
                           if(alpha)((Half*)d)[3]=            (((U16*)d)[3]*mul);
                        }
                     }
                     ok=true;
                  }
               }
            }
         }
         avifDecoderDestroy(decoder);
      }
   }
#endif
   if(!ok)del(); return ok;
}
Bool Image::ImportAVIF(C Str &name)
{
#if SUPPORT_AVIF
   File f; if(f.read(name))return ImportAVIF(f);
#endif
   del(); return false;
}
/******************************************************************************/
Bool Image::ExportAVIF(File &f, Flt rgb_quality, Flt alpha_quality, Flt compression_level)C
{
   Bool ok=false;
#if SUPPORT_AVIF
 C Image *src=this;
   Image  temp;

 C auto &type_info=src->typeInfo();
   Bool  hp   =(type_info.high_precision),
         alpha=(type_info.a>0);
   IMAGE_TYPE type=(hp ? (alpha ? IMAGE_F16_4 : IMAGE_F16_3) : (alpha ? IMAGE_R8G8B8A8_SRGB : IMAGE_R8G8B8_SRGB));

   if(src->cube())if(temp.fromCube(*src, type))src=&temp;else return false;

   if(hp // always copy for 'hp' because there we have to convert values
   || !CanDoRawCopy(src->hwType(), type, IgnoreGamma(0, src->hwType(), type)))
      if(src->copy(temp, -1, -1, -1, type, IMAGE_SOFT, 1))src=&temp;else return false;

   if(hp)
   {
      Image &img=temp;
      if(!img.lock())return false;
      REPD(y, img.h())
      {
         Byte *dy=img.data() + y*img.pitch();
         REPD(x, img.w())
         {
            Byte *d=dy + x*img.bytePP();
                     ((U16*)d)[0]=FltToU16(LinearToSRGB(((Half*)d)[0]));
                     ((U16*)d)[1]=FltToU16(LinearToSRGB(((Half*)d)[1]));
                     ((U16*)d)[2]=FltToU16(LinearToSRGB(((Half*)d)[2]));
            if(alpha)((U16*)d)[3]=FltToU16(             ((Half*)d)[3] );
         }
      }
      img.unlock();
   }
   if(!src->lockRead())return false;

   avifEncoder *encoder=null;
   avifImage     *image=avifImageCreate(src->w(), src->h(), hp ? 16 : 8, AVIF_PIXEL_FORMAT_YUV444); // these values dictate what goes into the final AVIF
   avifRWData    output=AVIF_DATA_EMPTY;
   avifRGBImage  rgb; avifRGBImageSetDefaults(&rgb, image);
   rgb.pixels  =ConstCast(src->data ());
   rgb.rowBytes=          src->pitch();
   rgb.format  =(alpha ? AVIF_RGB_FORMAT_RGBA : AVIF_RGB_FORMAT_RGB);
 //rgb.depth;
 //rgb.chromaDownsampling;
   if(avifImageRGBToYUV(image, &rgb)!=AVIF_RESULT_OK)goto error;
   encoder=avifEncoderCreate();
    // Configure your encoder here (see avif/avif.h):
    // * speed
    // * maxThreads
    // * quality
    // * qualityAlpha
   encoder->quality     =60;
   encoder->qualityAlpha=AVIF_QUALITY_LOSSLESS;

   if(avifEncoderAddImage(encoder, image, 1, AVIF_ADD_IMAGE_FLAG_SINGLE)!=AVIF_RESULT_OK)goto error;
   if(avifEncoderFinish  (encoder, &output                             )!=AVIF_RESULT_OK)goto error;

   ok=f.put(output.data, (Int)output.size);
error:
    if(image  )avifImageDestroy  (image);
    if(encoder)avifEncoderDestroy(encoder);
               avifRWDataFree    (&output);

   src->unlock();
#endif
   return ok;
}
Bool Image::ExportAVIF(C Str &name, Flt rgb_quality, Flt alpha_quality, Flt compression_level)C
{
#if SUPPORT_AVIF
   File f; if(f.write(name)){if(ExportAVIF(f, rgb_quality, alpha_quality, compression_level) && f.flush())return true; f.del(); FDelFile(name);}
#endif
   return false;
}
/******************************************************************************/
}
/******************************************************************************/
