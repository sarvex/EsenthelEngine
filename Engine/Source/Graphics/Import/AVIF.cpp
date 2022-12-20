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
      Memt<Byte> data; data.setNumDiscard(f.size());
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
Bool Image::ExportAVIF(File &f, Flt rgb_quality, Flt alpha_quality)C
{
   Bool ok=false;
#if SUPPORT_AVIF
 C Image *src=this;
   Image  temp;
   if(src->cube  ()                                                      )if(temp.fromCube(*src ,             IMAGE_R8G8B8A8_SRGB               ))src=&temp;else return false;
   if(src->hwType()!=IMAGE_R8G8B8A8 && src->hwType()!=IMAGE_R8G8B8A8_SRGB)if(src->copy    ( temp, -1, -1, -1, IMAGE_R8G8B8A8_SRGB, IMAGE_SOFT, 1))src=&temp;else return false; // AVIF uses RGBA
#endif
   return ok;
}
Bool Image::ExportAVIF(C Str &name, Flt rgb_quality, Flt alpha_quality)C
{
#if SUPPORT_AVIF
   File f; if(f.write(name)){if(ExportAVIF(f, rgb_quality, alpha_quality) && f.flush())return true; f.del(); FDelFile(name);}
#endif
   return false;
}
/******************************************************************************/
}
/******************************************************************************/
