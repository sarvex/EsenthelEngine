/******************************************************************************/
#include "stdafx.h"
namespace EE{
/******************************************************************************/
#define SKIP_SPACE 1 // if skip drawing space that was at the end of the line but didn't fit
#define CC4_TXDS CC4('T','X','D','S')
#define            DefaultShade 230
static const Color DefaultSelectionColor(51, 153, 255, 64);
Memc<TextLineSplit8 > Tls8 ;
Memc<TextLineSplit16> Tls16;
DEFINE_CACHE(TextStyle, TextStyles, TextStylePtr, "Text Style");
/******************************************************************************/
static Bool Separatable(Char c)
{
   return Unsigned(c)>=3585; // starting from Thai, Korean=0x1100, see 'LanguageSpecific', careful this range also includes German 'ẞ'
}
/******************************************************************************/
// STR EX
/******************************************************************************/
Bool StrEx::Data::visible()C
{
   switch(type)
   {
      case TEXT : return text.is();
      case IMAGE: return image!=null;
      case PANEL: return panel!=null;
   }
   return false;
}
void StrEx::Data::del()
{
   switch(type)
   {
      case TEXT : DTOR(text ); break;
      case IMAGE: DTOR(image); break;
      case PANEL: DTOR(panel); break;
      case FONT : DTOR(font ); break;
   }
   type=NONE;
}
StrEx::Data& StrEx::Data::create(TYPE type)
{
   if(T.type!=type)
   {
      del(); switch(T.type=type)
      {
         case TEXT : CTOR(text ); break;
         case IMAGE: CTOR(image); break;
         case PANEL: CTOR(panel); break;
         case FONT : CTOR(font ); break;
      }
   }
   return T;
}
void StrEx::Data::operator=(C Data &src)
{
   if(this!=&src)
   {
      create(src.type); switch(type)
      {
         case TEXT : text =src.text ; break;
         case IMAGE: image=src.image; break;
         case PANEL: panel=src.panel; break;
         case FONT : font =src.font ; break;
      }
   }
}
/******************************************************************************/
Int StrEx::length()C
{
   Int l=0; FREPA(data)
   {
    C Data &d=data[i]; switch(d.type)
      {
         case Data::TEXT : l+=d.text.length(); break;
         case Data::IMAGE: l++; break;
      }
   }
   return l;
}
Int StrEx::strLength()C
{
   Int l=0; FREPA(data)
   {
    C Data &d=data[i]; if(d.type==Data::TEXT)l+=d.text.length();
   }
   return l;
}
Str StrEx::str()C
{
   Str s; FREPA(data)
   {
    C Data &d=data[i]; if(d.type==Data::TEXT)s+=d.text;
   }
   return s;
}
void StrEx::operator=(C Str &text) {if(text.is())data.setNum(1).first().create(Data::TEXT).text=text;else clear();}

StrEx& StrEx::text  (C Str           &text  ) {           data.New().create(Data::TEXT      ).text  =text  ; return T;}
StrEx& StrEx::image (C ImagePtr      &image ) {           data.New().create(Data::IMAGE     ).image =image ; return T;}
StrEx& StrEx::panel (C PanelImagePtr &panel ) {           data.New().create(Data::PANEL     ).panel =panel ; return T;}
StrEx& StrEx::font  (C FontPtr       &font  ) {           data.New().create(Data::FONT      ).font  =font  ; return T;}
StrEx& StrEx::color (C Color         &color ) {           data.New().create(Data::COLOR     ).color =color ; return T;}
StrEx& StrEx::shadow(  Byte           shadow) {           data.New().create(Data::SHADOW    ).shadow=shadow; return T;}
StrEx& StrEx::color (C Color         *color ) {if(!color )data.New().create(Data::COLOR_OFF );else T.color (*color ); return T;}
StrEx& StrEx::shadow(C Byte          *shadow) {if(!shadow)data.New().create(Data::SHADOW_OFF);else T.shadow(*shadow); return T;}

StrEx& StrEx::panelText (C PanelImagePtr &panel, C Str      &text ) {T.panel(panel); T+=text ; T.panel(null); return T;}
StrEx& StrEx::panelImage(C PanelImagePtr &panel, C ImagePtr &image) {T.panel(panel); T+=image; T.panel(null); return T;}
/******************************************************************************/
Bool StrEx::save(File &f, CChar *path)C
{
   f.cmpUIntV(0); // version
   f.cmpUIntV(data.elms());
   FREPA(data)
   {
    C Data &d=data[i]; f<<d.type; switch(d.type)
      {
         case Data::COLOR : f<<d.color ; break;
         case Data::SHADOW: f<<d.shadow; break;
         case Data::TEXT  : f<<d.text  ; break;
         case Data::IMAGE : f.putAsset(d.image.id()); break;
         case Data::PANEL : f.putAsset(d.panel.id()); break;
         case Data::FONT  : f.putAsset(d.font .id()); break;
      }
   }
   return f.ok();
}
Bool StrEx::load(File &f, CChar *path)
{
   switch(f.decUIntV()) // version
   {
      case 0:
      {
         data.setNumDiscard(f.decUIntV()); FREPA(data)
         {
            Data::TYPE type; f>>type; if(!InRange(type, Data::NUM))goto error;
            Data &d=data[i]; d.create(type); switch(d.type)
            {
               case Data::COLOR : f>>d.color ; break;
               case Data::SHADOW: f>>d.shadow; break;
               case Data::TEXT  : f>>d.text  ; break;
               case Data::IMAGE : d.image.require(f.getAssetID(), path); break;
               case Data::PANEL : d.panel.require(f.getAssetID(), path); break;
               case Data::FONT  : d.font .require(f.getAssetID(), path); break;
            }
         }
         if(f.ok())return true;
      }break;
   }
error:
   del(); return false;
}
static Bool VisibleData(C StrEx::Data *data, Int datas)
{
   REP(datas)if(data[i].visible())return true;
   return false;
}
static Bool PanelClosing(C StrEx::Data *data, Int datas) // if there are no visible elements (no text, images) until the panel (or data) ends
{
next:
   if(datas<=0)return true; // reached the end = close
   switch(data->type)
   {
      case StrEx::Data::IMAGE:                    return false;        // there's a visible element
      case StrEx::Data::TEXT : if(data->text.is())return false; break; // there's a visible element

      case StrEx::Data::PANEL: return true; // reached panel = close
   }
   data++; datas--; goto next;
}
/******************************************************************************/
struct TextSrc
{
   CChar8 *t8;
   CChar  *t16;

   Bool is()C {return Is(t8) || Is(t16);}

   void fix() {if(!t16 && !t8)t16=u"";} // don't allow null so we can assume "t8 || t16" in 'c' 'n', prefer 't16' to avoid 'Char8To16Fast'

   Char c()C {return t8 ? Char8To16Fast(*  t8) : *  t16;} // assumes that Str was already initialized and "t8 || t16"
   Char n()  {return t8 ? Char8To16Fast(*++t8) : *++t16;} // assumes that Str was already initialized and "t8 || t16"

   void operator+=(Int i) {if(t8)t8+=i;else t16+=i;} // assumes that "t8 || t16"
   void operator-=(Int i) {if(t8)t8-=i;else t16-=i;} // assumes that "t8 || t16"

   TextSrc() {}
   TextSrc(CChar8 *t) {t8 =t; t16=null;}
   TextSrc(CChar  *t) {t16=t; t8 =null;}
};
/******************************************************************************/
struct TextSplit
{
   Byte         shadow;
   Color        color;
   Int          offset;
   Int          length;
   Int          datas;
 C StrEx::Data *data;
 C Font        *font;
 C PanelImage  *panel;
   TextSrc      text;

   void end(Int end) {length=end-offset;}
};
static Memc<TextSplit> TextSplits;
/******************************************************************************/
// TEXT PROCESSOR
/******************************************************************************/
struct TextProcessor
{
   Byte          shadow;
   TEXT_POS_MODE tpm;
   SPACING_MODE  spacing;
   Char          prev_chr;
   Color         color;
   Flt           xsize, ysize, xsize_2, space, space_2, panel_r, panel_padd_r;
   Vec2          pos, size;
 C Font         *font, *default_font;
 C PanelImage   *panel;

   inline Bool spacingConst()C {return spacing==SPACING_CONST;}

   void setFontFast(C Font *font)
   {
      T.font=font;
      xsize=size.x/font->height(); // width of 1 font texel
   }

   Int     charWidth(Char      chr)C {return font->charWidth(chr);}
   Int nextCharWidth(Char next_chr)C {return font->charWidth(prev_chr, next_chr, spacing);}

   void advance     (Char next_chr) {pos.x+=space + xsize*nextCharWidth(next_chr);}
   void advanceSplit(Char next_chr) {pos.x-=space + xsize*nextCharWidth(next_chr);}
   Bool advanceFast (Char next_chr)
   {
      Flt w=xsize*nextCharWidth(next_chr);
      Flt test=(spacingConst() ? (tpm==TEXT_POS_DEFAULT) ? space_2 : space : (tpm==TEXT_POS_DEFAULT) ? w/2 : (tpm==TEXT_POS_OVERWRITE) ? w+space_2 : xsize*charWidth(prev_chr)); // for TEXT_POS_FIT we have to make sure that the 'prev_chr' fully fits
      if(pos.x<test)return true;
      pos.x-=space+w;
      return false;
   }

   inline void panelPadd(C PanelImage &panel, Flt &left, Flt &right)C
   {
              left =panel.defaultInnerPadding().min.x,
              right=panel.defaultInnerPadding().max.x;
      Flt scale; if(panel.getSideScaleH(size.y, scale)){left*=scale; right*=scale;}
   }

   Flt _width(TextSrc &text, C StrEx::Data *&data, Int &datas, Int &max_length, Bool stop_on_panel) // !! MAY CHANGE 'font' !! but doesn't change 'panel'
   {
      if(max_length)
      {
       //Char prev_chr='\0';
         Char chr=text.c();
         Int  chr_pixels=0, elements=(spacingConst() ? 0 : -1); // -1 because we will count only in between
         Flt  width=0;
      loop:
         if(!chr)
         {
         next_data:
            if(datas<=0)goto end;
               datas--; C StrEx::Data *d=data++; switch(d->type)
            {
               case StrEx::Data::TEXT: if(d->text.is()) // process only if have anything, if not then proceed to 'next_data'
               {
                  text=d->text(); chr=text.c(); goto have_char;
               }break;

               case StrEx::Data::IMAGE:
               {
                C Image *image=d->image(); if(image && image->is())
                  {
                     Flt w=size.y*image->aspect();
                     if(spacingConst()){width+=Max(w, space);}
                     else              {width+=    w; elements++;}
                  }else elements++;
                  if(!--max_length)goto end;
                  if(prev_chr){chr_pixels+=nextCharWidth('\0'); prev_chr='\0';} // can check this after 'max_length' because if it goes to 'end', it will calculate the same thing
               }break;

               case StrEx::Data::PANEL:
               {
                  if(stop_on_panel){datas++; data--; goto end;} // revert because we haven't processed this
                  if(prev_chr){chr_pixels+=nextCharWidth('\0'); prev_chr='\0';}
                  if(C PanelImage *panel=d->panel())
                  {
                     width+=chr_pixels*xsize; chr_pixels=0; // process what we have because 'xsize' might change
                     Flt w=_width(text, data, datas, max_length, true), panel_padd_l, panel_padd_r; panelPadd(*panel, panel_padd_l, panel_padd_r);
                         w=Max(w+panel_padd_l+panel_padd_r, size.y);
                     if(spacingConst()){width+=Max(w, space);}
                     else              {width+=    w; elements++;}
                     if(!max_length)goto end;
                  }
               }break;

               case StrEx::Data::FONT:
               {
                C Font *new_font=d->font(); if(!new_font)new_font=default_font; if(font!=new_font)
                  {
                     if(prev_chr){chr_pixels+=nextCharWidth('\0'); prev_chr='\0';}
                     width+=chr_pixels*xsize; chr_pixels=0;
                     setFontFast(new_font);
                  }
               }break;
            }
            goto next_data;
         }

      have_char:
         elements++;
         if(prev_chr)chr_pixels+=nextCharWidth(chr); prev_chr=chr;

         {
         combining:
            Char n=text.n();
            if(!--max_length)goto end; // check after advancing 'text' pointer
            if(CharFlagFast(n)&CHARF_COMBINING)goto combining;

            chr=n; goto loop;
         }

      end:
         if(prev_chr){chr_pixels+=nextCharWidth('\0'); prev_chr='\0';}

         if(elements>0)width+=  elements*space; // to skip -1 and 0
                       width+=chr_pixels*xsize;
         return width;
      }
      return 0;
   }
   Flt panelWidth(TextSrc text, C StrEx::Data *data, Int datas, Int max_length=-1)
   {
    C Font *start_font=font;
      Flt width=_width(text, data, datas, max_length, true);
      if(font!=start_font)setFontFast(start_font); // if font got changed, restore it
      return width;
   }
   Flt width(TextSrc text, C StrEx::Data *data, Int datas, Int max_length)
   {
    C Font *start_font=font;
      Flt width=0;
      if(panel)
      {
         width=_width(text, data, datas, max_length, true);
         Flt panel_padd_l, panel_padd_r; panelPadd(*panel, panel_padd_l, panel_padd_r);
         width=Max(width+panel_padd_l+panel_padd_r, size.y);
         if(spacingConst())MAX(width, space);else
         if(max_length && (text.c() || VisibleData(data, datas)))width+=space; // have to add 'space' only if there's something after
      }
      width+=_width(text, data, datas, max_length, false);
      if(font!=start_font)setFontFast(start_font); // if font got changed, restore it
      return width;
   }

   void processPanelFast(C TextSrc &text, C StrEx::Data *data, Int datas)
   {
      Flt w=panelWidth(text, data, datas);
      Flt panel_padd_l/*, panel_padd_r*/; panelPadd(*panel, panel_padd_l, panel_padd_r);
       w+=panel_padd_l +  panel_padd_r;
      if(w<size.y) // require min square
      {
         Flt d=(size.y-w)/2; w=size.y;
         panel_padd_l+=d;
         panel_padd_r+=d;
      }
      // here use "-" because we're decreasing
      if(spacingConst()){Flt o=Max(w, space); panel_r=pos.x-o      ; pos.x-=(o-w)/2;}
      else              {                     panel_r=pos.x-w-space;                } // calculate panel right side, it will be used to set position after finishing drawing it, this is needed to always apply 'space' after that panel, regardless if it's empty or not
      pos.x-=panel_padd_l;
      if(tpm==TEXT_POS_FIT)pos.x-=panel_padd_r; // in FIT mode we have to make sure we will have enough room to close the panel (display panel padding on the right side), this adjustment will be canceled out later when setting as 'panel_r'
   }

   Bool initFast(C TextStyleParams &style)
   {
      if(default_font=style.getFont())
      {
         prev_chr='\0';

         spacing=style.spacing;
         size   =style.size;
         space  =style.space.x*size.x; space_2=space/2;

         return true;
      }
      return false;
   }

   inline Bool insidePanelNearPos()C {return (tpm==TEXT_POS_OVERWRITE) && panel && panel_r<=(spacingConst() ? 0 : -space_2);} // if we're inside a panel that's closing near the requested position, this is used to prevent from advancing to the next element if still haven't finished current panel

   Int textPos(C TextStyleParams &style, Flt x, TEXT_POS_MODE tpm, TextSrc text, C StrEx::Data *data, Int datas, C Font *start_font, C PanelImage *start_panel)
   {
      if(text.is() || VisibleData(data, datas))
         if(initFast(style))
      {
         text.fix();
         pos.x=x;
         T.tpm=tpm;
         if(!start_font)start_font=default_font;
         setFontFast(start_font);
         if(panel=start_panel)processPanelFast(text, data, datas);

         Int  pos_i=0, chars=0;
         Char chr=text.c();
      loop:
         if(!chr)
         {
         next_data:
            if(datas<=0)goto end;
               datas--; C StrEx::Data *d=data++; switch(d->type)
            {
               case StrEx::Data::TEXT: if(d->text.is()) // process only if have anything, if not then proceed to 'next_data'
               {
                  text=d->text(); chr=text.c(); goto have_char;
               }break;

               case StrEx::Data::IMAGE:
               {
                  if(prev_chr){if(advanceFast('\0'))goto ret; pos_i+=chars; chars=0; prev_chr='\0';}

                  Flt w; C Image *image=d->image(); if(image && image->is())w=size.y*image->aspect();else w=0;
                  Flt test=(spacingConst() ? (tpm==TEXT_POS_DEFAULT) ? Max(w, space)/2 : Max(w, space) : (tpm==TEXT_POS_DEFAULT) ? w/2 : (tpm==TEXT_POS_OVERWRITE) ? w+space_2 : w);
                  if(pos.x<test)goto ret;
                  const Bool FAST_CHECK=true;
                  if(!FAST_CHECK && insidePanelNearPos() && PanelClosing(data, datas))goto ret;
                  pos.x-=(spacingConst() ? Max(w, space) : w+space);
                  if( FAST_CHECK && insidePanelNearPos() && pos.x/*-w-space : already applied this above*/-panel_padd_r<=panel_r+EPS)goto ret; // if this is the last element, "image_left_pos-image_w-panel_padd_r<=panel_r+space+EPS" 'panel_r' had 'space' applied so have to revert it
                  pos_i++;
               }break;

               case StrEx::Data::PANEL:
               {
                  if(prev_chr){if(insidePanelNearPos() || advanceFast('\0'))goto ret; pos_i+=chars; chars=0; prev_chr='\0';}
                  if(panel)pos.x=panel_r;
                  if(panel=d->panel())processPanelFast(text, data, datas);
               }break;

               case StrEx::Data::FONT:
               {
                C Font *new_font=d->font(); if(!new_font)new_font=default_font; if(font!=new_font)
                  {
                     if(prev_chr){if(advanceFast('\0'))goto ret; pos_i+=chars; chars=0; prev_chr='\0';}
                     setFontFast(new_font);
                  }
               }break;
            }
            goto next_data;
         }

      have_char:

         if(prev_chr){if(advanceFast(chr))goto ret; pos_i+=chars; chars=0;} prev_chr=chr;

         chars++;

         {
         combining:
            Char n=text.n();
            if(CharFlagFast(n)&CHARF_COMBINING){chars++; goto combining;}

            chr=n; goto loop;
         }

      end:
         if(prev_chr){if(advanceFast('\0'))goto ret; pos_i+=chars; /*chars=0; prev_chr='\0';*/}

      ret:
       /*prev_chr='\0';*/ return pos_i;
      // no need to clear because this is a standalone function to be called outside of others
      }
      return 0;
   }

   void separator(TextSplit &separator, C TextSrc &text, C StrEx::Data *data, Int datas, Int pos_i)
   {
      separator.shadow=shadow;
      separator.color =color;
      separator.datas =datas;
    //separator.length=-1; this may be called several times, don't set it here, instead call it only one time later
      separator.offset=pos_i;
      separator.text  =text;
      separator.data  =data;
      separator.font  =font;
      separator.panel =panel;
   }

   void split(MemPtr<TextSplit> splits, C TextStyleParams &style, Flt width, Bool auto_line, TextSrc text, C StrEx::Data *data, Int datas) // have to set at least one line to support drawing cursor when text is empty
   {
      splits.clear();
      if(initFast(style))
      {
         text.fix();
         pos.x=width;
         T.tpm=TEXT_POS_FIT;
         setFontFast(default_font);
         panel=null;
         shadow=style.shadow;
         color =style.color;

         TextSplit *split=&splits.New(); separator(*split, text, data, datas, 0);

         Int pos_i=0;

         Char chr=text.c();
         if(!auto_line)
         {
         aln_loop:
            if(!chr)
            {
            aln_next_data:
               if(datas<=0)goto aln_end;
                  datas--; C StrEx::Data *d=data++; switch(d->type)
               {
                  case StrEx::Data::TEXT      : if(d->text.is()){text=d->text(); chr=text.c(); goto aln_have_char;} break; // process only if have anything, if not then proceed to 'next_data'
                  case StrEx::Data::IMAGE     : pos_i++; break;
                  case StrEx::Data::PANEL     : panel =d   ->panel(); break;
                  case StrEx::Data::FONT      : font  =d   ->font (); break;
                  case StrEx::Data::COLOR     : color =d   ->color  ; break;
                  case StrEx::Data::COLOR_OFF : color =style.color  ; break;
                  case StrEx::Data::SHADOW    : shadow=d   ->shadow ; break;
                  case StrEx::Data::SHADOW_OFF: shadow=style.shadow ; break;
               }
               goto aln_next_data;
            }

         aln_have_char:
            {
               Char n=text.n(); pos_i++;
               if(chr=='\n')
               {
                  split->end(pos_i-1);
                  split=&splits.New(); separator(*split, text, data, datas, pos_i);
               }
               chr=n;
               goto aln_loop;
            }

         aln_end:
            split->length=-1; // unlimited, end(pos_i);
         }else
         {
            TextSplit *next=&splits.New(); split=&splits[splits.elms()-2]; split->length=-1;
         loop:
            if(!chr)
            {
            next_data:
               if(datas<=0)goto end;
                  datas--; C StrEx::Data *d=data++; switch(d->type)
               {
                  case StrEx::Data::TEXT: if(d->text.is()) // process only if have anything, if not then proceed to 'next_data'
                  {
                     text=d->text(); chr=text.c(); goto have_char;
                  }break;

                  case StrEx::Data::IMAGE:
                  {
                     if(prev_chr){advanceSplit('\0'); prev_chr='\0';}

                     Flt w; C Image *image=d->image(); if(image && image->is())w=size.y*image->aspect();else w=0;

                     // 'pos_i' is at image, but 'data', 'datas' after it
                     if(pos_i>split->offset // require at least length=1
                     && pos.x<(spacingConst() ? Max(w, space) : w)) // image doesn't fit
                     {
                        split->end(pos_i);
                        separator(*next, text, data-1, datas+1, pos_i);

                        next=&splits.New(); split=&splits[splits.elms()-2]; split->length=-1;
                        pos.x=width;
                        if(panel)processPanelFast(text, data, datas);
                     }

                     pos.x-=(spacingConst() ? Max(w, space) : w+space);
                     pos_i++;

                     split->end(pos_i);
                     separator(*next, text, data, datas, pos_i);
                  }break;

                  case StrEx::Data::PANEL:
                  {
                     if(prev_chr){advanceSplit('\0'); prev_chr='\0';}
                     if(panel)pos.x=panel_r;
                     if(panel=d->panel())processPanelFast(text, data, datas);
                  }break;

                  case StrEx::Data::FONT:
                  {
                   C Font *new_font=d->font(); if(!new_font)new_font=default_font; if(font!=new_font)
                     {
                        if(prev_chr){advanceSplit('\0'); prev_chr='\0';}
                        setFontFast(new_font);
                     }
                  }break;

                  case StrEx::Data::COLOR     : color =d   ->color ; break;
                  case StrEx::Data::COLOR_OFF : color =style.color ; break;
                  case StrEx::Data::SHADOW    : shadow=d   ->shadow; break;
                  case StrEx::Data::SHADOW_OFF: shadow=style.shadow; break;
               }
               goto next_data;
            }

         have_char:
            {
               if(prev_chr)advanceSplit(chr);

               Int pos_start=pos_i; // 'pos_start' is located at 'chr'

               // combining
               Int chars=1;
            combining:
               Char n=text.n();
               if(CharFlagFast(n)&CHARF_COMBINING){chars++; goto combining;}
               pos_i+=chars;
               // 'text' and 'pos_i' are now after ('chr' and combined)

               Bool min_length=(pos_start>split->offset); // require at least length=1

               Bool new_line=(chr=='\n' // manual new line
                          || (min_length // require at least length=1
                           && pos.x<(spacingConst() ? space : xsize*charWidth(chr)))); // character doesn't fit, for TEXT_POS_FIT we have to make sure that the 'chr' fully fits

               Bool skippable=(chr==' ' || chr=='\n');
               if(  skippable // found separator
               ||   new_line && split->length<0 // or going to create a new line, but separator wasn't found yet
               ||   min_length && (Separatable(prev_chr) || Separatable(chr))
               )
               {
                  split->end(pos_start);
                  if(skippable)
                  {
                     if(new_line /*&& skippable already checked*/ && panel && !n && PanelClosing(data, datas))panel=null; // if we've skipped 'chr', creating new line, inside panel, and there's no visible element until panel closes, then close it already, so we don't start the new line with this panel
                       separator(*next, text, data, datas, pos_i    );}
                  else{separator(*next, text, data, datas, pos_start); next->text-=chars;}
               }

               prev_chr=chr;

               if(new_line)
               {
                  if(pos_start>next->offset) // if went too far, and separator is behind, we have to revert (this condition works well for 'skippable' on/off)
                  { // restart from last remembered position
                     shadow=next->shadow;
                     color =next->color;
                     pos_i =next->offset;
                     text  =next->text;
                     data  =next->data;
                     datas =next->datas;
                     panel =next->panel;
                     if(next->font!=font)setFontFast(next->font);
                     n=text.c(); prev_chr='\0';
                  }else
                  if(skippable)prev_chr='\0'; // if not reverting, but character is skippable, then clear it
                  next=&splits.New(); split=&splits[splits.elms()-2]; split->length=-1;
                  pos.x=width;
                  if(panel)processPanelFast(split->text, data, datas);
               }

               chr=n; goto loop;
            }

         end:
            split->length=-1; splits.removeLast(); // unlimited, end(pos_i); remove after adjusting 'split' because it might change memory address
            prev_chr='\0'; // have to make sure to clear 'prev_chr', because this class can still be used after this function
         }
      }
   }
};
/******************************************************************************/
struct TextDrawer : TextProcessor
{
   Bool sub_pixel;
   Int  font_offset_i_y;
   Flt  x_align_mul, y_align_mul;
   Vec2 font_offset;

   void initDraw(C TextStyleParams &style)
   {
      sub_pixel=default_font->_sub_pixel; // 'sub_pixel' before 'setColor'

      x_align_mul=style.align.x/2-0.5f;
      y_align_mul=style.align.y/2+0.5f;
   }
   void setFont(C Font *font)
   {
      T.font=font;
      xsize=size.x/font->height(); // width  of 1 font texel
      ysize=size.y/font->height(); // height of 1 font texel
      xsize_2=xsize/2;

      font_offset_i_y=       font->paddingT();
      font_offset.set(-xsize*font->paddingL(),
                       ysize*font_offset_i_y);
   }
};
/******************************************************************************/
struct TextDrawerHW : TextDrawer
{
   ALPHA_MODE   alpha;
   Int          cur, sel;
   ShaderImage *shader_image;
 C Shader      *shader;

   static inline Flt ByteToFontLum(Byte b) {return Min(b, 128)/128.0f;} // calculate text brightness 0..1, multiply by "2" (/128.0f instead of /255.0f) will give better results for grey text color (reaches 1.0 already at grey 128 byte value)
   void setColor(C Color &color)
   {
      VI.color(T.color=color);
      if(!sub_pixel)
      {
      #if LINEAR_GAMMA
         Sh.FontLum->set(ByteToFontLum(color.lum()));
      #endif
      }else
      {
      #if LINEAR_GAMMA
         Sh.FontLum->set(Vec(ByteToFontLum(color.r), ByteToFontLum(color.g), ByteToFontLum(color.b)));
      #endif
         D.alphaFactor(color); // 'MaterialClear' called at start only one time instead of here which can be called multiple times
      }
   }
   void setShadow(Byte shadow)
   {
      Sh.FontShadow->set(ByteToFlt(T.shadow=shadow));
   }
   void changeColor (C Color &color ) {if(T.color !=color ){VI.flush(); setColor (color );}}
   void changeShadow(  Byte   shadow) {if(T.shadow!=shadow){VI.flush(); setShadow(shadow);}}

   void initDraw(C TextStyleParams &style)
   {
      super::initDraw(style);

      setColor (style.color ); // after 'sub_pixel'
      setShadow(style.shadow);

      cur=-1; sel=-1; if(style.edit){cur=style.edit->cur; sel=style.edit->sel;}

      // font params
      Flt  contrast;
      Byte lum=style.color.lum(); if(!lum)contrast=1;else
      {
         Flt pixels =Renderer.screenToPixelSize(size).min();
         if( pixels>=32)contrast=1;else
         {
            contrast=32/pixels;
            contrast=Sqrt(contrast); // or alternative: contrast=Log2(contrast+1);
            contrast=Max(1, contrast);
            contrast=Lerp(1.0f, contrast, ByteToFlt(lum));
         }
      }
      Sh.FontContrast->set(contrast);
      Sh.FontShade   ->set(ByteSRGBToDisplay(style.shade));

      shader_image=(sub_pixel ? Sh.Img[0] : Sh.ImgXY[0]);

      // alpha
      if(sub_pixel){alpha=D.alpha(Renderer.inside() ? ALPHA_FONT_DEC : ALPHA_FONT); VI.shader(Sh.FontCurSP); MaterialClear();}else // if drawing text while rendering, then decrease the alpha channel (glow), for sub-pixel we will be changing 'D.alphaFactor' for which we have to call 'MaterialClear'
      if(Renderer.inside())D.alpha(ALPHA_RENDER_BLEND); // if drawing text while rendering, set special alpha mode, but don't bother to restore it, as in Rendering, alpha blending is always set for each call

      // font depth
      if(D._text_depth) // apply new state
      {
         D .depthLock (true );
         D .depthWrite(false); Renderer.needDepthTest(); // !! 'needDepthTest' after 'depthWrite' !!
         VI.shader    (Sh.Font[true][D._linear_gamma]);
      }

      shader=VI._shader;
      VI._image=null; // clear to make sure 'VI.imageConditional' below will work properly
   }
   Bool init(C TextStyleParams &style)
   {
      if(initFast(style))
      {
         initDraw(style);
         return true;
      }
      return false;
   }
   void shut()
   {
      VI.end();

      // alpha
      if(sub_pixel)D.alpha(alpha); // restore alpha

      // font depth
      if(D._text_depth) // reset old state
      {
         D.depthUnlock();
         D.depthWrite (true);
      }
   }

   void processPanel(C TextSrc &text, C StrEx::Data *data, Int datas, Int max_length)
   {
      Flt w=panelWidth(text, data, datas, max_length);
      Flt panel_padd_l, panel_padd_r; panelPadd(*panel, panel_padd_l, panel_padd_r);
       w+=panel_padd_l+ panel_padd_r;
      if(w<size.y) // require min square
      {
         Flt d=(size.y-w)/2; w=size.y;
         panel_padd_l+=d;
         panel_padd_r+=d;
      }
      if(spacingConst()){Flt o=Max(w, space); panel_r=pos.x+o      ; pos.x+=(o-w)/2;}
      else              {                     panel_r=pos.x+w+space;                } // calculate panel right side, it will be used to set position after finishing drawing it, this is needed to always apply 'space' after that panel, regardless if it's empty or not
      panel->draw(Rect_LU(pos, w, size.y));
      pos.x+=panel_padd_l;
   }

   void _draw(C TextStyleParams &style, Flt x, Flt y, TextSrc text, C StrEx::Data *data, Int datas, Int max_length, C Color &start_color, Byte start_shadow, C Font *start_font, C PanelImage *start_panel, Int offset=0, Int next_offset=-1) // assumes: 'text.fix'
   {
      if(!start_font)start_font=default_font;
      setFont(start_font); // BEFORE 'width'
      panel=start_panel;

      Flt total_width=(Equal(x_align_mul, 0) ? 0 : width(text, data, datas, max_length)); // don't calculate 'width' when not needed, AFTER 'setFont' and 'panel'
      x+= total_width*x_align_mul;
      y+= size.y     *y_align_mul;

      pos.set(x, y);
      if(style.pixel_align)D.alignScreenToPixel(pos);

      if(panel)
      {
         VI.flush();
         VI.shader(null);
         processPanel(text, data, datas, max_length);
         VI.shader(shader);
         VI.color (color);
      }

      Int pos_i=offset;
      Flt cur_w=-1, cur_x, sel_x;
   #if DEBUG
      cur_x=sel_x=0; // to prevent run-time check exceptions in debug mode
   #endif

      if(max_length)
      {
         changeColor (start_color );
         changeShadow(start_shadow);

         Char chr=text.c();
      loop:
         if(!chr)
         {
         next_data:
            if(datas<=0)goto end;
               datas--; C StrEx::Data *d=data++; switch(d->type)
            {
               case StrEx::Data::COLOR     : changeColor (d   ->color ); break;
               case StrEx::Data::COLOR_OFF : changeColor (style.color ); break;
               case StrEx::Data::SHADOW    : changeShadow(d   ->shadow); break;
               case StrEx::Data::SHADOW_OFF: changeShadow(style.shadow); break;
               case StrEx::Data::TEXT      : if(d->text.is()) // process only if have anything, if not then proceed to 'next_data'
               {
                  text=d->text(); chr=text.c(); goto have_char;
               }break;

               case StrEx::Data::IMAGE:
               {
                  if(prev_chr){advance('\0'); prev_chr='\0';}
                  if(sel==pos_i){sel_x=pos.x;}

                C Image *image=d->image(); if(image && image->is())
                  {
                     VI.flush();
                     VI.shader(null);

                     Flt w=size.y*image->aspect();
                     if(cur==pos_i){cur_x=pos.x; cur_w=w;}
                     Flt x=pos.x;
                     if(spacingConst()){Flt o=Max(w, space); pos.x+=o      ; x+=(o-w)/2;}
                     else              {                     pos.x+=w+space;}
                     image->draw(Rect_LU(x, pos.y, w, size.y));
                     VI.shader(shader);
                   //VI.color (color); not needed, 'image->draw' doesn't change it
                  }else
                  {
                     if(cur==pos_i){cur_x=pos.x; cur_w=size.y/2;}
                     pos.x+=space;
                  }
                  pos_i++;
                  if(!--max_length)goto end;
               }break;

               case StrEx::Data::PANEL:
               {
                  if(prev_chr){advance('\0'); prev_chr='\0';}
                  if(panel)pos.x=panel_r;
                  if(panel=d->panel())
                  {
                     VI.flush();
                     VI.shader(null);

                     processPanel(text, data, datas, max_length);

                     VI.shader(shader);
                     VI.color (color);

                     if(!max_length)goto end;
                  }
               }break;

               case StrEx::Data::FONT:
               {
                C Font *new_font=d->font(); if(!new_font)new_font=default_font; if(font!=new_font)
                  {
                     if(prev_chr){advance('\0'); prev_chr='\0';}
                     // no need to 'VI.flush' because this will be done in 'VI.imageConditional'
                     setFont(new_font);
                  }
               }break;
            }
            goto next_data;
         }
      have_char:

         if(prev_chr)advance(chr); prev_chr=chr;

         // update positions
         if(cur==pos_i){cur_x=pos.x; cur_w=xsize*charWidth(chr);}
         if(sel==pos_i){sel_x=pos.x;}
                 pos_i++;

         // draw character
         if(chr!=' ') // don't draw space sign, potentially we could check it below using "fc.height" instead, however this is faster
         {
            auto index=font->_wide_to_font[U16(chr)]; if(InRange(index, font->_chrs))
            {
             C Font::Chr &fc=font->_chrs[index];
             //if(fc.height) // potentially we could check for empty characters (full width space, half width space, nbsp, however in most cases we will have something to draw, so this check will slow down things
               {
                  Vec2 chr_pos=pos;
                  if(spacingConst())chr_pos.x+=space_2-xsize_2*                 fc.width  ; // move back by half of the character width
                                    chr_pos.x+=                 font_offset.x             ;
                                    chr_pos.y+=        ysize  *(font_offset_i_y-fc.offset);
                  if(style.pixel_align)D.alignScreenXToPixel(chr_pos.x);

                  Rect_LU rect(chr_pos, xsize*fc.width_padd, ysize*fc.height_padd);
                  VI.imageConditional(&font->_images[fc.image], *shader_image);
                  if(sub_pixel)VI.imagePart(rect, fc.tex);
                  else         VI.font     (rect, fc.tex);

                  // combining
                  if(!--max_length)goto end; // !! THIS CAN CHECK BEFORE ADVANCING 'text' AS LONG AS WE'RE NOT GOING TO USE IT AFTERWARD !!
                  Char n=text.n();
                  UInt flag=CharFlagFast(n); if(flag&CHARF_COMBINING)
                  {
                     chr_pos.x+=xsize_2*fc.width ; // after 'chr_pos' was pixel aligned, move by half of the character width to put it at centerX of 'chr' character
                     chr_pos.y+=ysize  *fc.offset; // reset Y pos
                     Bool skipped_bottom_shadow_padding=false;

                  draw_combining:
                     // update positions
                     if(cur==pos_i){cur_x=pos.x; cur_w=xsize*charWidth(chr);}
                     if(sel==pos_i){sel_x=pos.x;}
                             pos_i++;

                     auto index=font->_wide_to_font[U16(n)]; if(InRange(index, font->_chrs))
                     {
                      C Font::Chr &fc=font->_chrs[index];

                        if((flag&CHARF_STACK) && !skipped_bottom_shadow_padding) // if found a first stacking character, then skip shadow padding at the bottom, because next character is drawn after base character and on top, so its bottom shadow must not overlap the base
                        {
                           skipped_bottom_shadow_padding=true; // do this only once
                           chr_pos.y+=ysize*font->paddingB();
                        }

                        Vec2 n_pos=chr_pos;
                        n_pos.x-=xsize_2*fc.width; // move back by half of the character width
                        n_pos.y-=ysize  *fc.offset;
                      //if(style.pixel_align)D.alignScreenXToPixel(n_pos.x); 'n_pos' based on 'chr_pos' which was already aligned

                        Flt   n_height_padd=ysize*fc.height_padd;
                        Rect_LU rect(n_pos, xsize*fc. width_padd, n_height_padd);
                        VI.imageConditional(&font->_images[fc.image], *shader_image);
                        if(sub_pixel)VI.imagePart(rect, fc.tex);
                        else         VI.font     (rect, fc.tex);
                        if(flag&CHARF_STACK)chr_pos.y+=n_height_padd; // if the drawn character is stacking, then move position higher for next character, to stack combining characters on top of each other (needed for THAI)
                     }

                     if(!--max_length)goto end; // !! THIS CAN CHECK BEFORE ADVANCING 'text' AS LONG AS WE'RE NOT GOING TO USE IT AFTERWARD !!
                     n=text.n();
                     flag=CharFlagFast(n); if(flag&CHARF_COMBINING)goto draw_combining; // if next character is combining too
                  }
                  chr=n; goto loop;
               }
            }
         }

      skip_combining:
         if(!--max_length)goto end; // !! THIS CAN CHECK BEFORE ADVANCING 'text' AS LONG AS WE'RE NOT GOING TO USE IT AFTERWARD !!
         Char n=text.n();
         if(CharFlagFast(n)&CHARF_COMBINING)
         {
            // update positions
            if(cur==pos_i){cur_x=pos.x; cur_w=xsize*charWidth(chr);}
            if(sel==pos_i){sel_x=pos.x;}
                    pos_i++;

            goto skip_combining;
         }
         chr=n; goto loop;
      }
   end:
      if(prev_chr){advance('\0'); prev_chr='\0';}

      // selection
      if(sel>=0 && cur>=0) // have some selection
      {
         Int min, max; Rect rect;
         if(sel<cur)
         {
            min=sel; rect.min.x=sel_x;
            max=cur; rect.max.x=cur_x;
         }else
         {
            min=cur; rect.min.x=cur_x;
            max=sel; rect.max.x=sel_x;
         }
         if(max>offset && min<pos_i) // if selection intersects with this text
         {
            if(min< offset)rect.min.x=    x; // if minimum is before the start of this text, then use starting position
            if(max>=pos_i )rect.max.x=pos.x; // if maximum is after  the end   of this text, then use current  position
            Flt h=style.lineHeight();
            rect. setY(y-h, pos.y); D.alignScreenYToPixel(rect.min.y); // use "y-h" instead of "pos.y-h" to get exact value of the next line
            rect.moveY((h-size.y)/2); // adjust rectangle so text is at selection center
            rect.draw (style.selection);
         }
      }

      // cursor
      if(cur==pos_i && cur!=next_offset){cur_x=pos.x; cur_w=size.y/2;} // allow drawing cursor at the end only if it's not going to be drawn in the next line
      if(cur_w>=0 && !Kb._cur_hidden && App.active()) // it's faster to check 'App.active' here instead of adjusting 'Kb._cur_hidden' all the time based on 'App.active'
      {
         VI.flush();
         VI.shader(null);

         if(style.edit->overwrite && sel<0)DrawKeyboardCursorOverwrite(Vec2(cur_x, pos.y), size.y, spacingConst() ? Max(space, cur_w) : cur_w); // draw overwrite only without selection
         else                              DrawKeyboardCursor         (Vec2(cur_x, pos.y), size.y);

         VI.shader(shader);
         VI.color (color);
      }
   }
   void draw(C TextStyleParams &style, Flt x, Flt y, TextSrc text, C StrEx::Data *data, Int datas, Int max_length)
   {
      if(text.is() || VisibleData(data, datas) || (style.edit && (style.edit->cur>=0 || style.edit->sel>=0)))
         if(init(style))
      {
         text.fix();
        _draw(style, x, y, text, data, datas, max_length, style.color, style.shadow, default_font, null);
         shut();
      }
   }
   void draw(C TextStyleParams &style, C Rect &rect, TextSrc text, C StrEx::Data *data, Int datas, Bool auto_line)
   {
      if(text.is() || VisibleData(data, datas) || (style.edit && (style.edit->cur>=0 || style.edit->sel>=0)))
      {
         auto &splits=TextSplits;
         split(splits, style, rect.w(), auto_line, text, data, datas); if(splits.elms())
         {
            Flt  h=style.lineHeight();
            Vec2 p(rect.lerpX(style.align.x*-0.5f+0.5f),
                   Lerp(rect.min.y+(splits.elms()-1)*h, rect.max.y, style.align.y*-0.5f+0.5f));
            if(style.pixel_align)D.alignScreenToPixel(p); // align here to prevent jittering between lines when moving the whole text

          C Rect &clip=D._clip ? D._clip_rect : D.viewRect();
            Vec2 range; style.posY(p.y, range);
            Int  start=Max(              0, Trunc((range.y-clip.max.y)/h)), // can use 'Trunc' instead of 'Floor' because we use "Max(0"
                 end  =Min(splits.elms()-1, Ceil ((range.x-clip.min.y)/h));

         #if DEBUG && 0
            D.clip(null);
            D.lineX(RED, range.y                    , -D.w(), D.w()); // max
            D.lineX(RED, range.x-(splits.elms()-1)*h, -D.w(), D.w()); // min
            D.text(0, D.h()*0.9f, S+start+' '+end);
         #endif

            if(start<=end)
            {
               initDraw(style);
               TextSplit *split=&splits[start];
            next:
               TextSplit *next=splits.addr(start+1);
              _draw(style, p.x, p.y-start*h, split->text, split->data, split->datas, split->length, split->color, split->shadow, split->font, split->panel, split->offset, next ? next->offset : -1);
               if(++start<=end){split=next; goto next;}
               shut();
            }
         }
      }
   }
};
/******************************************************************************/
struct TextDrawerSoft : TextDrawer
{
   struct Rect2
   {
      Rect src, dest;
   };
   Int    rects, mip_map;
   Flt    contrast, shadow;
   Vec4   color;
   RectI  clip;
   Rect2  rect[65536/SIZE(Rect2)];
 C Image *src;
   Image *dest;

   void setFont(C Font *font)
   {
      super::setFont(font);

      Flt max_size=Max(Abs(xsize), Abs(ysize)), mip_map=0.5f*Log2(Sqr(1/max_size));
      T.mip_map=Max(0, Round(mip_map));

      CHS(ysize); CHS(font_offset.y); // #TextSoft
   }
   void setColor (C Color &color ) {T.color =          super::color =color  ;}
   void setShadow(  Byte   shadow) {T.shadow=ByteToFlt(super::shadow=shadow);}

   void changeColor (C Color &color ) {if(super::color !=color ){flush(); setColor (color );}}
   void changeShadow(  Byte   shadow) {if(super::shadow!=shadow){flush(); setShadow(shadow);}}

   void flush()
   {
      FREP(rects)
      {
       C Rect2 &r=rect[i];
         RectI dest;
         dest.set(Trunc(r.dest.min.x), Trunc(r.dest.max.y), Ceil(r.dest.max.x), Ceil(r.dest.min.y));
         dest&=clip;
         Vec2 mul_add_x((r.src.max.x-r.src.min.x)                         / (r.dest.max.x - r.dest.min.x)  * src->lw(),
         (r.src.min.x + (r.src.max.x-r.src.min.x) * (0.5f - r.dest.min.x) / (r.dest.max.x - r.dest.min.x)) * src->lw() - 0.5f);
         Vec2 mul_add_y((r.src.min.y-r.src.max.y)                         / (r.dest.max.y - r.dest.min.y)  * src->lh(),
         (r.src.max.y + (r.src.min.y-r.src.max.y) * (0.5f - r.dest.min.y) / (r.dest.max.y - r.dest.min.y)) * src->lh() - 0.5f);

         for(Int y=dest.min.y; y<dest.max.y; y++)
         {
          //Flt fy=LerpR(r.dest.min.y, r.dest.max.y, y+0.5f); // fy=(y+0.5f - r.dest.min.y)/(r.dest.max.y - r.dest.min.y)
          //Flt ty=Lerp (r.src .max.y, r.src .min.y, fy)*src->lh()-0.5f; // ty=(r.src.max.y + (r.src.min.y-r.src.max.y) * fy)*src->lh()-0.5f
            // ty=(r.src.max.y + (r.src.min.y-r.src.max.y) * (y+0.5f - r.dest.min.y)/(r.dest.max.y - r.dest.min.y))*src->lh()-0.5f
            // ty=y *                (r.src.min.y-r.src.max.y)                         / (r.dest.max.y - r.dest.min.y)  * src->lh()
            //      + (r.src.max.y + (r.src.min.y-r.src.max.y) * (0.5f - r.dest.min.y) / (r.dest.max.y - r.dest.min.y)) * src->lh() - 0.5f
            Flt ty=y*mul_add_y.x+mul_add_y.y;
            for(Int x=dest.min.x; x<dest.max.x; x++)
            {
             //Flt fx=LerpR(r.dest.min.x, r.dest.max.x, x+0.5f);
             //Flt tx=Lerp (r.src .min.x, r.src .max.x, fx)*src->lw()-0.5f;
               Flt tx=x*mul_add_x.x+mul_add_x.y;

               Vec4 c=src->colorFLinear(tx, ty);

               if(sub_pixel)
               {
                  c*=color.w;
                  Vec4 d=T.dest->colorF(x, y), out;
                  out.xyz=c.xyz*color.xyz + d.xyz*(1-c.xyz);
                  out.w  =c.w             + d.w  *(1-c.w  );
                  T.dest->colorF(x, y, out);
               }else
               {
                  // #FontImageLayout
                  Flt a=Min(c.x*contrast, 1), // font opacity, scale up by 'contrast' to improve quality when font is very small
                      s=    c.y*shadow      ; // font shadow

                  // Flt final_alpha=1-(1-s)*(1-a);
                  // 1-(1-s)*(1-a)
                  // 1-(1-a-s+sa)
                  // 1-1+a+s-sa
                  // a + s - s*a
                  Flt final_alpha=a+s-s*a;

                  // ALPHA_MERGE:
                  Vec4 out;
                  out.xyz=color.xyz*(a*color.w);
                  out.w  =color.w*final_alpha;

                  T.dest->mergeF(x, y, out);
               }
            }
         }
      }
      rects=0;
   }
   void unlock() {if(src){flush(); src->unlock();}}
   Bool draw(C Image &image, C Rect &rect_src, C Rect &rect_dest)
   {
      if(src!=&image)
      {
         unlock();
         if(image.lockRead(Min(mip_map, image.mipMaps()-1)))
         {
            src=&image;
         }else
         {
            src=null; return false;
         }
      }
      if(!InRange(rects, rect))flush();
      Rect2 &r=rect[rects++];
      r.src =rect_src;
      r.dest=rect_dest;
      return true;
   }

   void initDraw(C TextStyleParams &style, Image &dest)
   {
      super::initDraw(style);

      setColor (style.color ); // after 'sub_pixel'
      setShadow(style.shadow);

      rects=0;
      T.src = null;
      T.dest=&dest;
      clip.set(0, 0, dest.w(), dest.h());

      // font params
      Byte lum=style.color.lum(); if(!lum)contrast=1;else
      {
         Flt pixels =size.min();
         if( pixels>=32)contrast=1;else
         {
            contrast=32/pixels;
            contrast=Sqrt(contrast); // or alternative: contrast=Log2(contrast+1);
            contrast=Max(1, contrast);
            contrast=Lerp(1.0f, contrast, ByteToFlt(lum));
         }
      }
   }
   Bool init(C TextStyleParams &style, Image &dest)
   {
      if(initFast(style))
      {
         initDraw(style, dest);
         return true;
      }
      return false;
   }
   void shut()
   {
      unlock();
   }

   void processPanel(C TextSrc &text, C StrEx::Data *data, Int datas, Int max_length)
   {
      Flt w=panelWidth(text, data, datas, max_length);
      Flt panel_padd_l, panel_padd_r; panelPadd(*panel, panel_padd_l, panel_padd_r);
       w+=panel_padd_l+ panel_padd_r;
      if(w<size.y) // require min square
      {
         Flt d=(size.y-w)/2; w=size.y;
         panel_padd_l+=d;
         panel_padd_r+=d;
      }
      if(spacingConst()){Flt o=Max(w, space); panel_r=pos.x+o      ; pos.x+=(o-w)/2;}
      else              {                     panel_r=pos.x+w+space;                } // calculate panel right side, it will be used to set position after finishing drawing it, this is needed to always apply 'space' after that panel, regardless if it's empty or not
    //panel->draw(Rect_LU(pos, w, size.y)); FIXME #TextSoft
      pos.x+=panel_padd_l;
   }

   void _draw(C TextStyleParams &style, Flt x, Flt y, TextSrc text, C StrEx::Data *data, Int datas, Int max_length, C Color &start_color, Byte start_shadow, C Font *start_font, C PanelImage *start_panel) // assumes: 'text.fix'
   {
      if(!start_font)start_font=default_font;
      setFont(start_font); // BEFORE 'width'
      panel=start_panel;

      Flt total_width=(Equal(x_align_mul, 0) ? 0 : width(text, data, datas, max_length)); // don't calculate 'width' when not needed, AFTER 'setFont' and 'panel'
      x+= total_width*x_align_mul;
      y-= size.y     *y_align_mul; // #TextSoft

      pos.set(x, y);
      if(style.pixel_align)pos=Round(pos); // #TextSoft

      if(panel)processPanel(text, data, datas, max_length);

      if(max_length)
      {
         changeColor (start_color );
         changeShadow(start_shadow);

         Char chr=text.c();
      loop:
         if(!chr)
         {
         next_data:
            if(datas<=0)goto end;
               datas--; C StrEx::Data *d=data++; switch(d->type)
            {
               case StrEx::Data::COLOR     : changeColor (d   ->color ); break;
               case StrEx::Data::COLOR_OFF : changeColor (style.color ); break;
               case StrEx::Data::SHADOW    : changeShadow(d   ->shadow); break;
               case StrEx::Data::SHADOW_OFF: changeShadow(style.shadow); break;
               case StrEx::Data::TEXT      : if(d->text.is()) // process only if have anything, if not then proceed to 'next_data'
               {
                  text=d->text(); chr=text.c(); goto have_char;
               }break;

               case StrEx::Data::IMAGE:
               {
                  if(prev_chr){advance('\0'); prev_chr='\0';}

                C Image *image=d->image(); if(image && image->is())
                  {
                     Flt w=size.y*image->aspect();
                     Flt x=pos.x;
                     if(spacingConst()){Flt o=Max(w, space); pos.x+=o      ; x+=(o-w)/2;}
                     else              {                     pos.x+=w+space;}
                   //image->draw(Rect_LU(x, pos.y, w, size.y)); FIXME #TextSoft
                  }else
                  {
                     pos.x+=space;
                  }
                  if(!--max_length)goto end;
               }break;

               case StrEx::Data::PANEL:
               {
                  if(prev_chr){advance('\0'); prev_chr='\0';}
                  if(panel)pos.x=panel_r;
                  if(panel=d->panel())
                  {
                     processPanel(text, data, datas, max_length);
                     if(!max_length)goto end;
                  }
               }break;

               case StrEx::Data::FONT:
               {
                C Font *new_font=d->font(); if(!new_font)new_font=default_font; if(font!=new_font)
                  {
                     if(prev_chr){advance('\0'); prev_chr='\0';}
                     setFont(new_font);
                  }
               }break;
            }
            goto next_data;
         }
      have_char:

         if(prev_chr)advance(chr); prev_chr=chr;

         // draw character
         if(chr!=' ') // don't draw space sign, potentially we could check it below using "fc.height" instead, however this is faster
         {
            auto index=font->_wide_to_font[U16(chr)]; if(InRange(index, font->_chrs))
            {
             C Font::Chr &fc=font->_chrs[index];
             //if(fc.height) // potentially we could check for empty characters (full width space, half width space, nbsp, however in most cases we will have something to draw, so this check will slow down things
               {
                  Vec2 chr_pos=pos;
                  if(spacingConst())chr_pos.x+=space_2-xsize_2*                 fc.width  ; // move back by half of the character width
                                    chr_pos.x+=                 font_offset.x             ;
                                    chr_pos.y+=        ysize  *(font_offset_i_y-fc.offset);
                  if(style.pixel_align)chr_pos.x=Round(chr_pos.x); // #TextSoft

                  Rect_LU rect(chr_pos, xsize*fc.width_padd, ysize*fc.height_padd);
                  draw(font->_images[fc.image], fc.tex, rect); // #TextSoft

                  // combining
                  if(!--max_length)goto end; // !! THIS CAN CHECK BEFORE ADVANCING 'text' AS LONG AS WE'RE NOT GOING TO USE IT AFTERWARD !!
                  Char n=text.n();
                  UInt flag=CharFlagFast(n); if(flag&CHARF_COMBINING)
                  {
                     chr_pos.x+=xsize_2*fc.width ; // after 'chr_pos' was pixel aligned, move by half of the character width to put it at centerX of 'chr' character
                     chr_pos.y+=ysize  *fc.offset; // reset Y pos
                     Bool skipped_bottom_shadow_padding=false;

                  draw_combining:
                     auto index=font->_wide_to_font[U16(n)]; if(InRange(index, font->_chrs))
                     {
                      C Font::Chr &fc=font->_chrs[index];

                        if((flag&CHARF_STACK) && !skipped_bottom_shadow_padding) // if found a first stacking character, then skip shadow padding at the bottom, because next character is drawn after base character and on top, so its bottom shadow must not overlap the base
                        {
                           skipped_bottom_shadow_padding=true; // do this only once
                           chr_pos.y+=ysize*font->paddingB();
                        }

                        Vec2 n_pos=chr_pos;
                        n_pos.x-=xsize_2*fc.width; // move back by half of the character width
                        n_pos.y-=ysize  *fc.offset;
                      //if(style.pixel_align)n_pos.x=Round(n_pos.x); 'n_pos' based on 'chr_pos' which was already aligned #TextSoft

                        Flt   n_height_padd=ysize*fc.height_padd;
                        Rect_LU rect(n_pos, xsize*fc. width_padd, n_height_padd);
                        draw(font->_images[fc.image], fc.tex, rect); // #TextSoft
                        if(flag&CHARF_STACK)chr_pos.y+=n_height_padd; // if the drawn character is stacking, then move position higher for next character, to stack combining characters on top of each other (needed for THAI)
                     }

                     if(!--max_length)goto end; // !! THIS CAN CHECK BEFORE ADVANCING 'text' AS LONG AS WE'RE NOT GOING TO USE IT AFTERWARD !!
                     n=text.n();
                     flag=CharFlagFast(n); if(flag&CHARF_COMBINING)goto draw_combining; // if next character is combining too
                  }
                  chr=n; goto loop;
               }
            }
         }

      skip_combining:
         if(!--max_length)goto end; // !! THIS CAN CHECK BEFORE ADVANCING 'text' AS LONG AS WE'RE NOT GOING TO USE IT AFTERWARD !!
         Char n=text.n();
         if(CharFlagFast(n)&CHARF_COMBINING)goto skip_combining;
         chr=n; goto loop;
      }
   end:
      if(prev_chr){advance('\0'); prev_chr='\0';}
   }
   void draw(C TextStyleParams &style, Image &image, Flt x, Flt y, TextSrc text, C StrEx::Data *data, Int datas, Int max_length)
   {
      if(text.is() || VisibleData(data, datas))
         if(init(style, image))
      {
         text.fix();
        _draw(style, x, y, text, data, datas, max_length, style.color, style.shadow, default_font, null);
         shut();
      }
   }
   void draw(C TextStyleParams &style, Image &image, C Rect &rect, TextSrc text, C StrEx::Data *data, Int datas, Bool auto_line)
   {
      if(text.is() || VisibleData(data, datas))
      {
         Memt<TextSplit> splits; // use temp because this can be called outside of Draw #TextSoft
         split(splits, style, rect.w(), auto_line, text, data, datas); if(splits.elms())
         {
            Flt  h=style.lineHeight();
            Vec2 p(rect.lerpX(style.align.x*-0.5f+0.5f),
                   Lerp(rect.max.y-(splits.elms()-1)*h, rect.min.y, style.align.y*-0.5f+0.5f)); // #TextSoft
            if(style.pixel_align)p=Round(p); // align here to prevent jittering between lines when moving the whole text

            // #TextSoft
            Rect clip(0, image.size());
            Vec2 range; style.posYI(p.y, range);
            Int  start=Max(              0, Trunc((clip.min.y-range.x)/h)), // can use 'Trunc' instead of 'Floor' because we use "Max(0"
                 end  =Min(splits.elms()-1, Ceil ((clip.max.y-range.y)/h));

            if(start<=end)
            {
               initDraw(style, image);
               TextSplit *split=&splits[start];
            next:
               TextSplit *next=splits.addr(start+1);
              _draw(style, p.x, p.y+start*h, split->text, split->data, split->datas, split->length, split->color, split->shadow, split->font, split->panel); // #TextSoft
               if(++start<=end){split=next; goto next;}
               shut();
            }
         }
      }
   }
};
/******************************************************************************/
static Int Length(CChar8 *text, AUTO_LINE_MODE auto_line, Int max_length)
{
   Int length=-1;
   MAX (max_length, 1); // always allow drawing at least 1 character
   FREP(max_length+1) // iterate those characters (and 1 more) to check if within this range (or right after) we have special characters
   {
      Char8 c=text[i];
                                    if(c=='\0' || c=='\n')return i; // if we've encountered end or new line, then we have to stop there, return length exclusive because we won't draw this character
      if(auto_line!=AUTO_LINE_SPLIT)if(c==' '            )length=i; // if we want to avoid splitting words into multiple lines, then remember last space that's in this range
   }
   if(length>=0)length+=(length<max_length);else // to support drawing cursors and selections, draw the space too (increase length to include it) if it can fit in the 'max_length' range
   { // if nothing was found
      length=max_length; // draw all characters that fit in this space
      if(auto_line==AUTO_LINE_SPACE)for(; ; length++) // if we're not splitting words then we have to keep going forward outside of the range that fits in the space until we find first character that can split
      {
         Char8 c=text[length];
         if(c=='\0'
         || c=='\n'
         || c==' ' )break; // and find a first character that can split text, set length exclusive because we won't draw this character
      }
   }
   return length;
}
static Int Length(CChar *text, AUTO_LINE_MODE auto_line, Int max_length)
{
   Int length=-1;
   MAX (max_length, 1); // always allow drawing at least 1 character
   FREP(max_length+1) // iterate those characters (and 1 more) to check if within this range (or right after) we have special characters
   {
      Char c=text[i];
                                    if(c=='\0' || c=='\n')return i; // if we've encountered end or new line, then we have to stop there, return length exclusive because we won't draw this character
      if(auto_line!=AUTO_LINE_SPLIT)if(c==' '            )length=i; // if we want to avoid splitting words into multiple lines, then remember last space that's in this range
   }
   if(length>=0)length+=(length<max_length);else // to support drawing cursors and selections, draw the space too (increase length to include it) if it can fit in the 'max_length' range
   { // if nothing was found
      length=max_length; // draw all characters that fit in this space
      if(auto_line==AUTO_LINE_SPACE)for(; ; length++) // if we're not splitting words then we have to keep going forward outside of the range that fits in the space until we find first character that can split
      {
         Char c=text[length];
         if(c=='\0'
         || c=='\n'
         || c==' ' )break; // and find a first character that can split text, set length exclusive because we won't draw this character
      }
   }
   return length;
}
/******************************************************************************/
void Set(MemPtr<TextLineSplit8> tls, CChar8 *text, C TextStyleParams &text_style, Flt width, AUTO_LINE_MODE auto_line) // have to set at least one line to support drawing cursor when text is empty
{
   tls.clear();
   if(CChar8 *t=text)switch(auto_line)
   {
      default: // AUTO_LINE_NONE
      {
         tls.New().set(t, -1, 0);
         for(Int length=0; t[0]; t++)
         {
            if(t[0]!='\n')length++;else
            {
               tls.last().length=length; length=0;
               tls.New ().set(t+1, -1, (t+1)-text);
            }
         }
      }break;

      case AUTO_LINE_SPACE:
      case AUTO_LINE_SPACE_SPLIT:
      case AUTO_LINE_SPLIT:
      {
         for(width+=EPS; ; ) // needed when drawing text in 'width' calculated from 'textWidth'
         {
            Int length=Length(t, auto_line, text_style.textPos(t, width, TEXT_POS_FIT)); // check how many characters can we fit in this space
            tls.New().set(t, length, t-text);
            t+=length;
            if(*t=='\0')break;
            if(*t=='\n'/* || !length*/ // if next character is a new line, or we didn't advance ('length' doesn't need to be checked because it can be 0 only for '\0' or '\n' which are already checked)
            || SKIP_SPACE && *t==' ' && text_style.spacing!=SPACING_CONST
            )t++; // then skip this character 
         }
      }break;
   }else tls.New().set(null, 0, 0); // have to set at least one line
}
void Set(MemPtr<TextLineSplit16> tls, CChar *text, C TextStyleParams &text_style, Flt width, AUTO_LINE_MODE auto_line) // have to set at least one line to support drawing cursor when text is empty
{
   tls.clear();
   if(CChar *t=text)switch(auto_line)
   {
      default: // AUTO_LINE_NONE
      {
         tls.New().set(t, -1, 0);
         for(Int length=0; t[0]; t++)
         {
            if(t[0]!='\n')length++;else
            {
               tls.last().length=length; length=0;
               tls.New ().set(t+1, -1, (t+1)-text);
            }
         }
      }break;
      
      case AUTO_LINE_SPACE:
      case AUTO_LINE_SPACE_SPLIT:
      case AUTO_LINE_SPLIT:
      {
         for(width+=EPS; ; ) // needed when drawing text in 'width' calculated from 'textWidth'
         {
            Int length=Length(t, auto_line, text_style.textPos(t, width, TEXT_POS_FIT)); // check how many characters can we fit in this space
            tls.New().set(t, length, t-text);
            t+=length;
            if(*t=='\0')break;
            if(*t=='\n'/* || !length*/ // if next character is a new line, or we didn't advance ('length' doesn't need to be checked because it can be 0 only for '\0' or '\n' which are already checked)
            || SKIP_SPACE && *t==' ' && text_style.spacing!=SPACING_CONST
            )t++; // then skip this character 
         }
      }break;
   }else tls.New().set(null, 0, 0); // have to set at least one line
}
static Bool SetLine(TextLineSplit16 &tls, CChar *text, C TextStyleParams &text_style, Flt width, AUTO_LINE_MODE auto_line, Int line)
{
   if(CChar *t=text)if(line>=0)for(width+=EPS; ; ) // needed when drawing text in 'width' calculated from 'textWidth'
   {
      Int length=Length(t, auto_line, (auto_line==AUTO_LINE_NONE) ? INT_MAX-1 : text_style.textPos(t, width, TEXT_POS_FIT)); // check how many characters can we fit in this space, use "INT_MAX-1" so "FREP(max_length+1)" inside 'Length' can finish
      if(!line){tls.set(t, length, t-text); return true;}
      line--;
      t+=length;
      if(*t=='\0')break;
      if(*t=='\n'/* || !length*/ // if next character is a new line, or we didn't advance ('length' doesn't need to be checked because it can be 0 only for '\0' or '\n' which are already checked)
      || SKIP_SPACE && *t==' ' && text_style.spacing!=SPACING_CONST && auto_line!=AUTO_LINE_NONE
      )t++; // then skip this character 
   }
   return false;
}
Vec2 TextStyleParams::textIndex(CChar *text, Int index, Flt width, AUTO_LINE_MODE auto_line)C
{
   if(index<=0 || !text)return 0;
   CChar *t=text; Int line=0; for(width+=EPS; ; line++) // needed when drawing text in 'width' calculated from 'textWidth'
   {
      Int length=Length(t, auto_line, (auto_line==AUTO_LINE_NONE) ? INT_MAX-1 : textPos(t, width, TEXT_POS_FIT)); // check how many characters can we fit in this space, use "INT_MAX-1" so "FREP(max_length+1)" inside 'Length' can finish
      Int offset=t-text;
      t+=length;
   #if 1 // this check will set the cursor for the next line for split lines
      if(*t!='\n')length--;
   #endif
      if(index<=offset+length || *t=='\0')
         return Vec2(textWidth(text+offset, index-offset), line*lineHeight());
      if(*t=='\n'/* || !length*/ // if next character is a new line, or we didn't advance ('length' doesn't need to be checked because it can be 0 only for '\0' or '\n' which are already checked)
      || SKIP_SPACE && *t==' ' && spacing!=SPACING_CONST && auto_line!=AUTO_LINE_NONE
      )t++; // then skip this character 
   }
}
/******************************************************************************/
Font* TextStyleParams::getFont()C
{
   if(_font)return _font;
   if(GuiSkin *skin=Gui.skin())return skin->font(); // copy to temp 'skin' in case of multi-thread issues
   return null;
}
Flt TextStyleParams::posY(Flt y)C
{
   if(C Font *font=getFont())
   {
      Flt ysize      =size .y/font->height(), // height of 1 font texel
          y_align_mul=align.y*0.5f+0.5f;
       y+=size.y*y_align_mul+ysize*font->paddingT();
   }
   return y;
}
void TextStyleParams::posY(Flt y, Vec2 &range)C
{
   if(C Font *font=getFont())
   {
      Flt ysize      =size .y/font->height(), // height of 1 font texel
          y_align_mul=align.y*0.5f+0.5f;
      y+=size.y*y_align_mul;
      range.x=y-size.y-ysize*font->paddingB(); // min
      range.y=y       +ysize*font->paddingT(); // max
   }
}
void TextStyleParams::posYI(Flt y, Vec2 &range)C
{
   if(C Font *font=getFont())
   {
      Flt ysize      =size .y/font->height(), // height of 1 font texel
          y_align_mul=align.y*0.5f+0.5f;
      y-=size.y*y_align_mul;
      range.x=y       -ysize*font->paddingT(); // min
      range.y=y+size.y+ysize*font->paddingB(); // max
   }
}
/******************************************************************************/
void TextStyleParams::setPerPixelSize()
{
   if(C Font *font=getFont())size=D.pixelToScreenSize(font->height());
}
Flt TextStyleParams::textWidth(C Str &str, Int max_length)C
{
   if(Int length=str.length())
      if(C Font *font=getFont())
   {
      if(max_length>=0 && max_length<length){if(!max_length)return 0; length=max_length;}

      Flt    xsize=size.x/font->height(),
             space=size.x*T.space.x;
      Int    base_chars, width=font->textWidth(base_chars, spacing, str, max_length);
      return width*xsize + space*(base_chars-(spacing!=SPACING_CONST)); // calculate spacing only between base characters (ignoring combining), we're including spacing between the characters, so we need to set 1 less (except the case for SPACING_CONST where we need to have spacing for all characters)
   }
   return 0;
}
Flt TextStyleParams::textWidth(C Str8 &str, Int max_length)C
{
   if(Int length=str.length())
      if(C Font *font=getFont())
   {
      if(max_length>=0 && max_length<length){if(!max_length)return 0; length=max_length;}

      Flt    xsize=size.x/font->height(),
             space=size.x*T.space.x;
      Int    base_chars, width=font->textWidth(base_chars, spacing, str, max_length);
      return width*xsize + space*(base_chars-(spacing!=SPACING_CONST)); // calculate spacing only between base characters (ignoring combining), we're including spacing between the characters, so we need to set 1 less (except the case for SPACING_CONST where we need to have spacing for all characters)
   }
   return 0;
}
Flt TextStyleParams::textWidth(CChar *text, Int max_length)C
{
   if(Int length=Length(text))
      if(C Font *font=getFont())
   {
      if(max_length>=0 && max_length<length){if(!max_length)return 0; length=max_length;}

      Flt    xsize=size.x/font->height(),
             space=size.x*T.space.x;
      Int    base_chars, width=font->textWidth(base_chars, spacing, text, max_length);
      return width*xsize + space*(base_chars-(spacing!=SPACING_CONST)); // calculate spacing only between base characters (ignoring combining), we're including spacing between the characters, so we need to set 1 less (except the case for SPACING_CONST where we need to have spacing for all characters)
   }
   return 0;
}
Flt TextStyleParams::textWidth(CChar8 *text, Int max_length)C
{
   if(Int length=Length(text))
      if(C Font *font=getFont())
   {
      if(max_length>=0 && max_length<length){if(!max_length)return 0; length=max_length;}

      Flt    xsize=size.x/font->height(),
             space=size.x*T.space.x;
      Int    base_chars, width=font->textWidth(base_chars, spacing, text, max_length);
      return width*xsize + space*(base_chars-(spacing!=SPACING_CONST)); // calculate spacing only between base characters (ignoring combining), we're including spacing between the characters, so we need to set 1 less (except the case for SPACING_CONST where we need to have spacing for all characters)
   }
   return 0;
}
/******************************************************************************/
Int TextStyleParams::textPos(CChar8 *text, Flt x, TEXT_POS_MODE tpm)C
{
   Int pos=0;
   if(Is(text))
      if(C Font *font=getFont())
   {
      Flt space=size.x*T.space.x;
      if(spacing==SPACING_CONST)
      {
         x/=space; if(tpm==TEXT_POS_DEFAULT)x+=0.5f;
         pos=Trunc(x);
      #if 0 // fast
         Clamp(pos, 0, Length(text));
      #else // CHARF_COMBINING
         CChar8 *start=text; for(Int base_chars=0; ; base_chars++)
         {
            Char8 c=*text; if(!c || base_chars>=pos){pos=text-start; break;}
         skip:
            Char8 next=*++text;
            if(CharFlagFast(next)&CHARF_COMBINING)goto skip;
         }
      #endif
      }else
      for(Flt xsize=size.x/font->height(), space_2=space/2; ; )
      {
          Char8 c=*text; if(!c)break;
         CChar8 *start=text;
      skip1:
         Char8 next=*++text;
         if(CharFlagFast(next)&CHARF_COMBINING)goto skip1;
         Flt w=font->charWidth(c, next, spacing)*xsize;
         if(x<((tpm==TEXT_POS_DEFAULT) ? w/2 : (tpm==TEXT_POS_OVERWRITE) ? w+space_2 : xsize*font->charWidth(c)))break; // for TEXT_POS_FIT we have to make sure that the 'c' fully fits
         x-=w+space;
         pos+=text-start; // advance by how many characters were processed
      }
   }
   return pos;
}
Int TextStyleParams::textPos(CChar *text, Flt x, TEXT_POS_MODE tpm)C
{
   Int pos=0;
   if(Is(text))
      if(C Font *font=getFont())
   {
      Flt space=size.x*T.space.x;
      if(spacing==SPACING_CONST)
      {
         x/=space; if(tpm==TEXT_POS_DEFAULT)x+=0.5f;
         pos=Trunc(x);
      #if 0 // fast
         Clamp(pos, 0, Length(text));
      #else // CHARF_COMBINING
         CChar *start=text; for(Int base_chars=0; ; base_chars++)
         {
            Char c=*text; if(!c || base_chars>=pos){pos=text-start; break;}
         skip:
            Char next=*++text;
            if(CharFlagFast(next)&CHARF_COMBINING)goto skip;
         }
      #endif
      }else
      for(Flt xsize=size.x/font->height(), space_2=space/2; ; )
      {
          Char c=*text; if(!c)break;
         CChar *start=text;
      skip1:
         Char next=*++text;
         if(CharFlagFast(next)&CHARF_COMBINING)goto skip1;
         Flt w=font->charWidth(c, next, spacing)*xsize;
         if(x<((tpm==TEXT_POS_DEFAULT) ? w/2 : (tpm==TEXT_POS_OVERWRITE) ? w+space_2 : xsize*font->charWidth(c)))break; // for TEXT_POS_FIT we have to make sure that the 'c' fully fits
         x-=w+space;
         pos+=text-start; // advance by how many characters were processed
      }
   }
   return pos;
}
Int TextStyleParams::textPos(CChar *text, Flt x, Flt y, TEXT_POS_MODE tpm, Flt width, AUTO_LINE_MODE auto_line, Bool &eol)C
{
   Int line=Trunc(y/lineHeight()); if(line<0){eol=false; return 0;}
   TextLineSplit16 tls; if(!SetLine(tls, text, T, width, auto_line, line)){eol=true; return Length(text);}
   Int pos=textPos(text+tls.offset, x, tpm);
   if(eol=(pos>=tls.length))pos=tls.length; // yes this must check ">=" and not ">" because we need to set "eol=(pos>=tls.length)" because we need it for correct double-clicking word selection
   return tls.offset+pos;
}
/******************************************************************************/
Int TextStyleParams::textLines(CChar8 *text, Flt width, AUTO_LINE_MODE auto_line, Flt *actual_width)C
{
   Memt<TextLineSplit8> tls; Set(tls, text, T, width, auto_line);
   if(actual_width)
   {
     *actual_width=0; REPA(tls){TextLineSplit8 &t=tls[i]; MAX(*actual_width, textWidth(t.text, t.length));}
   }
   return tls.elms();
}
Int TextStyleParams::textLines(CChar *text, Flt width, AUTO_LINE_MODE auto_line, Flt *actual_width)C
{
   Memt<TextLineSplit16> tls; Set(tls, text, T, width, auto_line);
   if(actual_width)
   {
     *actual_width=0; REPA(tls){TextLineSplit16 &t=tls[i]; MAX(*actual_width, textWidth(t.text, t.length));}
   }
   return tls.elms();
}
/******************************************************************************/
TextStyleParams& TextStyleParams::resetColors(Bool gui)
{
   // use 'Gui.skin.text.text_style', 'Gui.skin.text_style' if available
   if(GuiSkin *skin=Gui.skin())
      if(TextStyle *text_style=(gui ? skin->text.text_style() : skin->text_style()))
   {
      shadow   =text_style->shadow;
      shade    =text_style->shade;
      color    =text_style->color;
      selection=text_style->selection;
      return T;
   }

   // otherwise set defaults
   shadow   =255;
   shade    =DefaultShade;
   color    =WHITE;
   selection=DefaultSelectionColor;
   return T;
}
TextStyleParams& TextStyleParams::reset(Bool gui)
{
   // use 'Gui.skin.text.text_style', 'Gui.skin.text_style' if available
   if(GuiSkin *skin=Gui.skin())
      if(TextStyle *text_style=(gui ? skin->text.text_style() : skin->text_style()))
         {T=*text_style; return T;}

   // otherwise set defaults
   pixel_align=true;
   align.set(0    , 0    );
   size .set(0.08f, 0.08f);
   space.set(0.06f, 1    );
   spacing  =SPACING_NICE;
   shadow   =255;
   shade    =DefaultShade;
   color    =WHITE;
   selection=DefaultSelectionColor;
  _font     =null; // keep as null to always use the current value of 'Gui.skin.font'
   edit     =null;
   return T;
}
/******************************************************************************/
TextStyle& TextStyle::font       (C FontPtr &font) {_font=font; super::font(_font()); return T;}
TextStyle& TextStyle::resetColors(  Bool     gui ) {super::resetColors(gui); return T;}
TextStyle& TextStyle::reset      (  Bool     gui ) {super::reset      (gui); if(GuiSkin *skin=Gui.skin())if(TextStyle *text_style=(gui ? skin->text.text_style() : skin->text_style())){font(text_style->font()); return T;} font(null); return T;} // copy to temp 'skin' and 'text_style' in case of multi-thread issues, call 'font' to always make sure that '_font' from 'TextStyle' and 'TextStyleParams' is synchronized
TextStyle::TextStyle             (               ) {                         if(GuiSkin *skin=Gui.skin())if(TextStyle *text_style=                                 skin->text_style() ){font(text_style->font()); return  ;} font(null);          } // copy to temp 'skin' and 'text_style' in case of multi-thread issues, call 'font' to always make sure that '_font' from 'TextStyle' and 'TextStyleParams' is synchronized
/******************************************************************************/
// DRAW
/******************************************************************************/
struct TextInput
{
   CChar8 *t8 ;
   CChar  *t16;

   Bool is()C {return Is(t8) || Is(t16);}
   Char c ()C {return t8 ? Char8To16Fast(*  t8) : t16 ? *  t16 : 0;} // we can assume that Str was already initialized
   Char n ()  {return t8 ? Char8To16Fast(*++t8) :       *++t16;}
   CPtr p ()C {return t8 ? CPtr(t8) : CPtr(t16);}

   void operator++(int) {if(t8)t8++; if(t16)t16++;}

   TextInput(CChar8 *t) {t8 =t; t16=null;}
   TextInput(CChar  *t) {t16=t; t8 =null;}
};
static Int CompareCode(C TextCodeData &code, C CPtr &pos) {return ComparePtr(code.pos, pos);}
static Int    FindCode(C TextCodeData *code, Int codes, CPtr cur_pos)
{
   Int    index;
   Bool   found=BinarySearchFirst(code, codes, cur_pos, index, CompareCode); // find first in case many codes would point to the same place
   return found ? index : index-1; // if we haven't found at exact position, then we need to grab the one before selected position like this: "<code>some tex|t here" when starting drawing of | we need to use the <code>
}
static inline Flt ByteToFontLum(Byte b) {return Min(b, 128)/128.0f;} // calculate text brightness 0..1, multiply by "2" (/128.0f instead of /255.0f) will give better results for grey text color (reaches 1.0 already at grey 128 byte value)
static void SetCode(C TextCodeData *code, C TextStyleParams &text_style, Bool sub_pixel)
{
   VI.flush();
                                      Color c=((code && code-> color_mode!=TextCodeData::DEFAULT) ? code->color  : text_style.color ); VI.color(c);
   if(!sub_pixel){Sh.FontShadow->set(ByteToFlt((code && code->shadow_mode!=TextCodeData::DEFAULT) ? code->shadow : text_style.shadow));
   #if LINEAR_GAMMA
      Sh.FontLum->set(ByteToFontLum(c.lum()));
   #endif
   }else
   {
   #if LINEAR_GAMMA
      Sh.FontLum->set(Vec(ByteToFontLum(c.r), ByteToFontLum(c.g), ByteToFontLum(c.b)));
   #endif
      D.alphaFactor(c); // 'MaterialClear' called below only one time instead of here which can be called multiple times
   }
}
void DrawKeyboardCursor(C Vec2 &pos, Flt height)
{
   ALPHA_MODE alpha=D.alpha(ALPHA_INVERT); Rect_U(pos, height/11.0f, height).draw(WHITE);
                    D.alpha(alpha       );
}
void DrawKeyboardCursorOverwrite(C Vec2 &pos, Flt height, Flt width)
{
   const Flt min_w=height/5.0f; // use min width because some characters are just too thin
   ALPHA_MODE alpha=D.alpha(ALPHA_INVERT); Rect_LU r(pos, width, height); if(min_w>width)r.extendX((min_w-width)*0.5f); r.draw(WHITE); // extend both left and right
                    D.alpha(alpha       );
}
void TextStyleParams::drawMain(Flt x, Flt y, TextInput ti, Int max_length, C TextCodeData *code, Int codes, Int offset)C
{
   Int cur=SIGN_BIT, sel=SIGN_BIT; ASSERT(Int(SIGN_BIT)<0); // these must be negative
   if(edit && App.active()){if(edit->cur>=0)cur=edit->cur-offset; if(edit->sel>=0)sel=edit->sel-offset;} // set only if valid (so we keep SIGN_BIT otherwise)
   if(ti.is() || cur>=0 || sel>=0) // we have some text to draw, or we may encounter cursor or selection ahead
      if(C Font *font=getFont())
   {
      Int pos=0;
      Flt cur_x, cur_w=-1, sel_x;
   #if DEBUG
      cur_x=sel_x=0; // to prevent run-time check exceptions on debug mode
   #endif

      // set
      Flt xsize       =size.x/font->height(), // width  of 1 font texel
          ysize       =size.y/font->height(), // height of 1 font texel
          space       =size.x*T.space.x,
          x_align_mul =align.x*0.5f-0.5f,
          y_align_mul =align.y*0.5f+0.5f,
          total_width =(Equal(x_align_mul, 0) ? 0 : ti.t8 ? textWidth(ti.t8, max_length) : textWidth(ti.t16, max_length)), // don't calculate text width when not needed
          total_height=size.y;
          x          +=total_width *x_align_mul - xsize*font->paddingL();
          y          +=total_height*y_align_mul + ysize*font->paddingT();

      if(spacing==SPACING_CONST)x+=space*0.5f; // put the cursor at the center where characters will be drawn, later this will be repositioned by half char width
      Vec2 p(x, y); if(pixel_align)D.alignScreenToPixel(p);

      // draw
      if(ti.is())
      {
         Flt xsize_2=xsize*0.5f;

         // sub-pixel rendering
         ALPHA_MODE   alpha;
         Bool         sub_pixel=font->_sub_pixel;
         ShaderImage &shader_image=(sub_pixel ? *Sh.Img[0] : *Sh.ImgXY[0]);
         if(sub_pixel){alpha=D.alpha(Renderer.inside() ? ALPHA_FONT_DEC : ALPHA_FONT); VI.shader(Sh.FontCurSP); MaterialClear();}else // if drawing text while rendering, then decrease the alpha channel (glow), for sub-pixel we will be changing 'D.alphaFactor' for which we have to call 'MaterialClear'
         if(Renderer.inside())D.alpha(ALPHA_RENDER_BLEND); // if drawing text while rendering, then decrease the alpha channel (glow), but don't bother to restore it, as in Rendering, alpha blending is always set for each call

         VI._image=null; // clear to make sure 'VI.imageConditional' below will work properly

         Int            cur_code_index=FindCode(code, codes, ti.p());
       C TextCodeData * cur_code      =(InRange(cur_code_index  , codes) ? &code[cur_code_index  ] : null),
                      *next_code      =(InRange(cur_code_index+1, codes) ? &code[cur_code_index+1] : null);

         // font params
         Flt  contrast;
         Byte lum=color.lum(); if(!lum)contrast=1;else
         {
            Flt pixels =Renderer.screenToPixelSize(size).min();
            if( pixels>=32)contrast=1;else
            {
               contrast=32/pixels;
               contrast=Sqrt(contrast); // or alternative: contrast=Log2(contrast+1);
               contrast=Max(1, contrast);
               contrast=Lerp(1.0f, contrast, ByteToFlt(lum));
            }
         }
         Sh.FontContrast->set(contrast);
         Sh.FontShade   ->set(ByteSRGBToDisplay(shade));
         SetCode(cur_code, T, sub_pixel);

         // font depth
         if(D._text_depth) // apply new state
         {
            D .depthLock (true );
            D .depthWrite(false); Renderer.needDepthTest(); // !! 'needDepthTest' after 'depthWrite' !!
            VI.shader    (Sh.Font[true][D._linear_gamma]);
         }

         for(Char c=ti.c(); c; pos++)
         {
            if(!max_length--)break;

            // check if encountered next code
            for(; next_code && ti.p()>=next_code->pos; )
            {
                cur_code_index++;
                cur_code=next_code;
               next_code=(InRange(cur_code_index+1, codes) ? &code[cur_code_index+1] : null);
               SetCode(cur_code, T, sub_pixel);
            }

            // next character
            Char n=ti.n();

            // draw character
            if(c!=' ') // don't draw space sign, potentially we could check it below using "fc.height" instead, however this is faster
            {
               UInt index=font->_wide_to_font[U16(c)]; if(InRange(index, font->_chrs))
               {
                C Font::Chr &fc=font->_chrs[index];
                //if(fc.height) // potentially we could check for empty characters (full width space, half width space, nbsp, however in most cases we will have something to draw, so this check will slow down things
                  {
                     Vec2 c_pos=p;
                     if(spacing==SPACING_CONST)c_pos.x-=xsize_2*fc.width ; // move back by half of the character width
                                               c_pos.y-=ysize  *fc.offset;
                     if(pixel_align)D.alignScreenXToPixel(c_pos.x);

                     VI.imageConditional(&font->_images[fc.image], shader_image);
                     Rect_LU rect(c_pos, xsize*fc.width_padd, ysize*fc.height_padd);
                     if(sub_pixel)VI.imagePart(rect, fc.tex);
                     else         VI.font     (rect, fc.tex);

                     // combining
                     UInt flag=CharFlagFast(n); if(flag&CHARF_COMBINING)
                     {
                        c_pos.x+=xsize_2*fc.width; // after 'c_pos' was pixel aligned, move by half of the character width to put it at centerX of 'c' character
                        c_pos.y =p.y; // reset Y pos
                        Bool skipped_bottom_shadow_padding=false;

                     again:
                        UInt index=font->_wide_to_font[U16(n)]; if(InRange(index, font->_chrs))
                        {
                           if(!max_length--)break;

                           // update positions
                           if(cur==pos){cur_x=p.x; cur_w=xsize*font->charWidth(c);}
                           if(sel==pos){sel_x=p.x;}
                           pos++;

                         C Font::Chr &fc=font->_chrs[index];

                           if((flag&CHARF_STACK) && !skipped_bottom_shadow_padding) // if found a first stacking character, then skip shadow padding at the bottom, because next character is drawn after base character and on top, so its bottom shadow must not overlap the base
                           {
                              skipped_bottom_shadow_padding=true; // do this only once
                              c_pos.y+=ysize*font->paddingB();
                           }

                           Vec2 n_pos=c_pos;
                           n_pos.x-=xsize_2*fc.width; // move back by half of the character width
                           n_pos.y-=ysize  *fc.offset;
                           if(pixel_align)D.alignScreenXToPixel(n_pos.x);

                           VI.imageConditional(&font->_images[fc.image], shader_image);
                           Rect_LU rect(n_pos, xsize*fc.width_padd, ysize*fc.height_padd);
                           if(sub_pixel)VI.imagePart(rect, fc.tex);
                           else         VI.font     (rect, fc.tex);

                           n=ti.n();
                           UInt flag_next=CharFlagFast(n); if(flag_next&CHARF_COMBINING) // if next character is combining too
                           {
                              if(flag&CHARF_STACK)c_pos.y+=ysize*fc.height_padd; // if the drawn character is stacking, then move position higher, to stack combining characters on top of each other (needed for THAI)
                              flag=flag_next;
                              goto again;
                           }
                        }
                     }
                  }
               }
            }

            // update positions
            if(cur==pos){cur_x=p.x; cur_w=xsize*font->charWidth(c);}
            if(sel==pos){sel_x=p.x;}
            p.x+=space + xsize*font->charWidth(c, n, spacing);
            c=n;
         }
         VI.end();

         // sub-pixel
         if(sub_pixel)D.alpha(alpha); // restore alpha
         
         // font depth
         if(D._text_depth) // reset old state
         {
            D.depthUnlock();
            D.depthWrite (true);
         }
      }

      // selection
      if(sel!=SIGN_BIT && cur!=SIGN_BIT) // have some selection
      {
         Int min, max; Rect rect;
         if(sel<cur)
         {
            min=sel; rect.min.x=sel_x;
            max=cur; rect.max.x=cur_x;
         }else
         {
            min=cur; rect.min.x=cur_x;
            max=sel; rect.max.x=sel_x;
         }
         if(max>=0 && min<pos) // if selection intersects with this text
         {
            if(min<=0  )rect.min.x=  x; // if minimum is before the start of this text, then use starting position
            if(max>=pos)rect.max.x=p.x; // if maximum is after  the end   of this text, then use current  position
            if(spacing==SPACING_CONST){rect.min.x-=space*0.5f; rect.max.x-=space*0.5f;} // revert what we've applied
            Flt h=lineHeight();
            rect. setY(y-h, p.y); D.alignScreenYToPixel(rect.min.y); // use "y-h" instead of "p.y-h" to get exact value of the next line
            rect.moveY((h-size.y)/2); // adjust rectangle so text is at selection center
            rect.draw(selection);
         }
      }

      // cursor
      if(cur==pos){Char c=ti.c(); if(!c || c=='\n' || (SKIP_SPACE && c==' ' && spacing!=SPACING_CONST /*&& auto_line!=AUTO_LINE_NONE*/)){cur_x=p.x; cur_w=total_height/2;}} // allow drawing cursor at the end only if it's followed by new line or null (this prevents drawing cursors 2 times for split lines - at the end of 1st line and at the beginning of 2nd line)
      if(cur_w>=0 && !Kb._cur_hidden)
      {
         if(spacing==SPACING_CONST)cur_x-=space*0.5f; // revert what we've applied
         if(edit->overwrite)DrawKeyboardCursorOverwrite(Vec2(cur_x, p.y), total_height, (spacing==SPACING_CONST) ? colWidth() : cur_w);
         else               DrawKeyboardCursor         (Vec2(cur_x, p.y), total_height);
      }
   }
}
struct FontDraw
{
   struct Rect2
   {
      Rect src, dest;
   };
   Rect2  rect[65536/SIZE(Rect2)];
   Int    rects, mip_map;
   Flt    contrast, shadow;
   Vec4   color;
   RectI  clip;
   Bool   sub_pixel;
 C Image *src;
   Image &dest;

   FontDraw(Image &dest, Int mip_map, Bool sub_pixel) : mip_map(Max(0, mip_map)), sub_pixel(sub_pixel), dest(dest) {rects=0; clip.set(0, 0, dest.w(), dest.h()); clear();}
  ~FontDraw() {unlock();}

   void setCode(C TextCodeData *code, C TextStyleParams &text_style)
   {
      flush();
      color =         ((code && code-> color_mode!=TextCodeData::DEFAULT) ? code->color  : text_style.color );
      shadow=ByteToFlt((code && code->shadow_mode!=TextCodeData::DEFAULT) ? code->shadow : text_style.shadow);
   }
   void flush()
   {
      FREP(rects)
      {
       C Rect2 &r=rect[i];
         RectI dest;
         dest.set(Trunc(r.dest.min.x), Trunc(r.dest.max.y), Ceil(r.dest.max.x), Ceil(r.dest.min.y));
         dest&=clip;
         Vec2 mul_add_x((r.src.max.x-r.src.min.x)                         / (r.dest.max.x - r.dest.min.x)  * src->lw(),
         (r.src.min.x + (r.src.max.x-r.src.min.x) * (0.5f - r.dest.min.x) / (r.dest.max.x - r.dest.min.x)) * src->lw() - 0.5f);
         Vec2 mul_add_y((r.src.min.y-r.src.max.y)                         / (r.dest.max.y - r.dest.min.y)  * src->lh(),
         (r.src.max.y + (r.src.min.y-r.src.max.y) * (0.5f - r.dest.min.y) / (r.dest.max.y - r.dest.min.y)) * src->lh() - 0.5f);

         for(Int y=dest.min.y; y<dest.max.y; y++)
         {
          //Flt fy=LerpR(r.dest.min.y, r.dest.max.y, y+0.5f); // fy=(y+0.5f - r.dest.min.y)/(r.dest.max.y - r.dest.min.y)
          //Flt ty=Lerp (r.src .max.y, r.src .min.y, fy)*src->lh()-0.5f; // ty=(r.src.max.y + (r.src.min.y-r.src.max.y) * fy)*src->lh()-0.5f
            // ty=(r.src.max.y + (r.src.min.y-r.src.max.y) * (y+0.5f - r.dest.min.y)/(r.dest.max.y - r.dest.min.y))*src->lh()-0.5f
            // ty=y *                (r.src.min.y-r.src.max.y)                         / (r.dest.max.y - r.dest.min.y)  * src->lh()
            //      + (r.src.max.y + (r.src.min.y-r.src.max.y) * (0.5f - r.dest.min.y) / (r.dest.max.y - r.dest.min.y)) * src->lh() - 0.5f
            Flt ty=y*mul_add_y.x+mul_add_y.y;
            for(Int x=dest.min.x; x<dest.max.x; x++)
            {
             //Flt fx=LerpR(r.dest.min.x, r.dest.max.x, x+0.5f);
             //Flt tx=Lerp (r.src .min.x, r.src .max.x, fx)*src->lw()-0.5f;
               Flt tx=x*mul_add_x.x+mul_add_x.y;

               Vec4 c=src->colorFLinear(tx, ty);

               if(sub_pixel)
               {
                  c*=color.w;
                  Vec4 d=T.dest.colorF(x, y), out;
                  out.xyz=c.xyz*color.xyz + d.xyz*(1-c.xyz);
                  out.w  =c.w             + d.w  *(1-c.w  );
                  T.dest.colorF(x, y, out);
               }else
               {
                  // #FontImageLayout
                  Flt a=Min(c.x*contrast, 1), // font opacity, scale up by 'contrast' to improve quality when font is very small
                      s=    c.y*shadow      ; // font shadow

                  // Flt final_alpha=1-(1-s)*(1-a);
                  // 1-(1-s)*(1-a)
                  // 1-(1-a-s+sa)
                  // 1-1+a+s-sa
                  // a + s - s*a
                  Flt final_alpha=a+s-s*a;

                  // ALPHA_MERGE:
                  Vec4 out;
                  out.xyz=color.xyz*(a*color.w);
                  out.w  =color.w*final_alpha;

                  T.dest.mergeF(x, y, out);
               }
            }
         }
      }
      rects=0;
   }
   void clear () {src=null;}
   void unlock() {if(src){flush(); src->unlock();}}
   Bool draw(C Image &image, C Rect &rect_src, C Rect &rect_dest)
   {
      if(src!=&image)
      {
         unlock();
         if(image.lockRead(Min(mip_map, image.mipMaps()-1)))
         {
            src=&image;
         }else
         {
            clear(); return false;
         }
      }
      if(!InRange(rects, rect))flush();
      Rect2 &r=rect[rects++];
      r.src =rect_src;
      r.dest=rect_dest;
      return true;
   }
};
void TextStyleParams::drawMainSoft(Image &image, Flt x, Flt y, TextInput ti, Int max_length, C TextCodeData *code, Int codes, Int offset)C
{
   if(ti.is()) // we have some text to draw
      if(C Font *font=getFont())
   {
      // set
      Flt xsize       = size.x/font->height(), // width  of 1 font texel
          ysize       =-size.y/font->height(), // height of 1 font texel
          space       = size.x*T.space.x,
          x_align_mul =align.x*0.5f-0.5f,
          y_align_mul =align.y*0.5f+0.5f,
          total_width =(Equal(x_align_mul, 0) ? 0 : ti.t8 ? textWidth(ti.t8, max_length) : textWidth(ti.t16, max_length)), // don't calculate text width when not needed
          total_height=-size.y;
          x          +=total_width *x_align_mul - xsize*font->paddingL();
          y          +=total_height*y_align_mul + ysize*font->paddingT();

      if(spacing==SPACING_CONST)x+=space*0.5f; // put the cursor at the center where characters will be drawn, later this will be repositioned by half char width
      Vec2 p(x, y); if(pixel_align)p=Round(p);

      // draw
    //if(ti.is())
      {
         Flt xsize_2=xsize*0.5f;

         // sub-pixel rendering
         Flt      max_size=Max(Abs(xsize), Abs(ysize)), mip_map=0.5f*Log2(Sqr(1/max_size));
         FontDraw draw(image, Round(mip_map), font->_sub_pixel);

         Int            cur_code_index=FindCode(code, codes, ti.p());
       C TextCodeData * cur_code      =(InRange(cur_code_index  , codes) ? &code[cur_code_index  ] : null),
                      *next_code      =(InRange(cur_code_index+1, codes) ? &code[cur_code_index+1] : null);

         // font params
         Byte lum=color.lum(); if(!lum)draw.contrast=1;else
         {
            Flt pixels =size.min();
            if( pixels>=32)draw.contrast=1;else
            {
               draw.contrast=32/pixels;
               draw.contrast=Sqrt(draw.contrast); // or alternative: draw.contrast=Log2(draw.contrast+1);
               draw.contrast=Max(1, draw.contrast);
               draw.contrast=Lerp(1.0f, draw.contrast, ByteToFlt(lum));
            }
         }

         draw.setCode(cur_code, T);

         for(Char c=ti.c(); c; )
         {
            if(!max_length--)break;

            // check if encountered next code
            for(; next_code && ti.p()>=next_code->pos; )
            {
                cur_code_index++;
                cur_code=next_code;
               next_code=(InRange(cur_code_index+1, codes) ? &code[cur_code_index+1] : null);
               draw.setCode(cur_code, T);
            }

            // next character
            Char n=ti.n();

            // draw character
            if(c!=' ') // don't draw space sign, potentially we could check it below using "fc.height" instead, however this is faster
            {
               UInt index=font->_wide_to_font[U16(c)]; if(InRange(index, font->_chrs))
               {
                C Font::Chr &fc=font->_chrs[index];
                //if(fc.height) // potentially we could check for empty characters (full width space, half width space, nbsp, however in most cases we will have something to draw, so this check will slow down things
                  {
                     Vec2 c_pos=p;
                     if(spacing==SPACING_CONST)c_pos.x-=xsize_2*fc.width ; // move back by half of the character width
                                               c_pos.y-=ysize  *fc.offset;
                     if(pixel_align)c_pos.x=Round(c_pos.x);

                     draw.draw(font->_images[fc.image], fc.tex, Rect_LU(c_pos, xsize*fc.width_padd, ysize*fc.height_padd));

                     // combining
                     UInt flag=CharFlagFast(n); if(flag&CHARF_COMBINING)
                     {
                        c_pos.x+=xsize_2*fc.width; // after 'c_pos' was pixel aligned, move by half of the character width to put it at centerX of 'c' character
                        c_pos.y =p.y; // reset Y pos
                        Bool skipped_bottom_shadow_padding=false;

                     again:
                        UInt index=font->_wide_to_font[U16(n)]; if(InRange(index, font->_chrs))
                        {
                           if(!max_length--)break;

                         C Font::Chr &fc=font->_chrs[index];

                           if((flag&CHARF_STACK) && !skipped_bottom_shadow_padding) // if found a first stacking character, then skip shadow padding at the bottom, because next character is drawn after base character and on top, so its bottom shadow must not overlap the base
                           {
                              skipped_bottom_shadow_padding=true; // do this only once
                              c_pos.y+=ysize*font->paddingB();
                           }

                           Vec2 n_pos=c_pos;
                           n_pos.x-=xsize_2*fc.width; // move back by half of the character width
                           n_pos.y-=ysize  *fc.offset;
                           if(pixel_align)n_pos.x=Round(n_pos.x);

                           draw.draw(font->_images[fc.image], fc.tex, Rect_LU(n_pos, xsize*fc.width_padd, ysize*fc.height_padd));

                           n=ti.n();
                           UInt flag_next=CharFlagFast(n); if(flag_next&CHARF_COMBINING) // if next character is combining too
                           {
                              if(flag&CHARF_STACK)c_pos.y+=ysize*fc.height_padd; // if the drawn character is stacking, then move position higher, to stack combining characters on top of each other (needed for THAI)
                              flag=flag_next;
                              goto again;
                           }
                        }
                     }
                  }
               }
            }

            // update positions
            p.x+=space + xsize*font->charWidth(c, n, spacing);
            c=n;
         }
      }
   }
}
/******************************************************************************/
void TextStyleParams::drawSplit(C Rect &rect, Memc<TextLineSplit8> &tls, C TextCodeData *code, Int codes)C
{
   Flt  h=lineHeight();
   Vec2 p(rect.lerpX(align.x*-0.5f+0.5f),
          Lerp(rect.min.y+(tls.elms()-1)*h, rect.max.y, align.y*-0.5f+0.5f));
   if(pixel_align)D.alignScreenToPixel(p); // align here to prevent jittering between lines when moving the whole text

 C Rect &clip=D._clip ? D._clip_rect : D.viewRect();
   Vec2 range; posY(p.y, range);
   Int  start=Max(           0, Floor((range.y-clip.max.y)/h)),
        end  =Min(tls.elms()-1, Ceil ((range.x-clip.min.y)/h));

#if DEBUG && 0
   D.clip(null);
   D.lineX(RED, range.y                 , -D.w(), D.w()); // max
   D.lineX(RED, range.x-(tls.elms()-1)*h, -D.w(), D.w()); // min
   D.text(0, D.h()*0.9f, S+start+' '+end);
#endif

   for(; start<=end; start++){auto &t=tls[start]; drawMain(p.x, p.y-start*h, t.text, t.length, code, codes, t.offset);}
}
void TextStyleParams::drawSplit(C Rect &rect, Memc<TextLineSplit16> &tls, C TextCodeData *code, Int codes)C
{
   Flt  h=lineHeight();
   Vec2 p(rect.lerpX(align.x*-0.5f+0.5f),
          Lerp(rect.min.y+(tls.elms()-1)*h, rect.max.y, align.y*-0.5f+0.5f));
   if(pixel_align)D.alignScreenToPixel(p); // align here to prevent jittering between lines when moving the whole text

 C Rect &clip=D._clip ? D._clip_rect : D.viewRect();
   Vec2 range; posY(p.y, range);
   Int  start=Max(           0, Floor((range.y-clip.max.y)/h)),
        end  =Min(tls.elms()-1, Ceil ((range.x-clip.min.y)/h));

#if DEBUG && 0
   D.clip(null);
   D.lineX(RED, range.y                 , -D.w(), D.w()); // max
   D.lineX(RED, range.x-(tls.elms()-1)*h, -D.w(), D.w()); // min
   D.text(0, D.h()*0.9f, S+start+' '+end);
#endif

   for(; start<=end; start++){auto &t=tls[start]; drawMain(p.x, p.y-start*h, t.text, t.length, code, codes, t.offset);}
}
/******************************************************************************/
void TextStyleParams::drawSplitSoft(Image &image, C Rect &rect, Memt<TextLineSplit8> &tls, C TextCodeData *code, Int codes)C
{
   Flt  h=lineHeight();
   Vec2 p(rect.lerpX(align.x*-0.5f+0.5f),
          Lerp(rect.max.y-(tls.elms()-1)*h, rect.min.y, align.y*-0.5f+0.5f));
   if(pixel_align)p=Round(p); // align here to prevent jittering between lines when moving the whole text

   Rect clip(0, image.size());
   Vec2 range; posYI(p.y, range);
   Int  start=Max(           0, Floor((clip.min.y-range.x)/h)),
        end  =Min(tls.elms()-1, Ceil ((clip.max.y-range.y)/h));

   for(; start<=end; start++){auto &t=tls[start]; drawMainSoft(image, p.x, p.y+start*h, t.text, t.length, code, codes, t.offset);}
}
void TextStyleParams::drawSplitSoft(Image &image, C Rect &rect, Memt<TextLineSplit16> &tls, C TextCodeData *code, Int codes)C
{
   Flt  h=lineHeight();
   Vec2 p(rect.lerpX(align.x*-0.5f+0.5f),
          Lerp(rect.max.y-(tls.elms()-1)*h, rect.min.y, align.y*-0.5f+0.5f));
   if(pixel_align)p=Round(p); // align here to prevent jittering between lines when moving the whole text

   Rect clip(0, image.size());
   Vec2 range; posYI(p.y, range);
   Int  start=Max(           0, Floor((clip.min.y-range.x)/h)),
        end  =Min(tls.elms()-1, Ceil ((clip.max.y-range.y)/h));

   for(; start<=end; start++){auto &t=tls[start]; drawMainSoft(image, p.x, p.y+start*h, t.text, t.length, code, codes, t.offset);}
}
/******************************************************************************/
void TextStyleParams::drawCode(C Rect &rect, CChar8 *t, AUTO_LINE_MODE auto_line, C TextCodeData *code, Int codes)C {Set(Tls8 , t, T, rect.w(), auto_line); drawSplit(rect, Tls8 , code, codes);}
void TextStyleParams::drawCode(C Rect &rect, CChar  *t, AUTO_LINE_MODE auto_line, C TextCodeData *code, Int codes)C {Set(Tls16, t, T, rect.w(), auto_line); drawSplit(rect, Tls16, code, codes);}
/******************************************************************************/
void DisplayDraw::text(C TextStyleParams &text_style, Flt x, Flt y, CChar8 *t) {text_style.drawMain(x, y, t);}
void DisplayDraw::text(C TextStyleParams &text_style, Flt x, Flt y, CChar  *t) {text_style.drawMain(x, y, t);}

void DisplayDraw::text(                               Flt x, Flt y, CChar8 *t) {if(Gui.skin && Gui.skin->text_style)Gui.skin->text_style->drawMain(x, y, t);}
void DisplayDraw::text(                               Flt x, Flt y, CChar  *t) {if(Gui.skin && Gui.skin->text_style)Gui.skin->text_style->drawMain(x, y, t);}

void DisplayDraw::text(C TextStyleParams &text_style, C Rect &rect, CChar8 *t, AUTO_LINE_MODE auto_line) {text_style.drawCode(rect, t, auto_line);}
void DisplayDraw::text(C TextStyleParams &text_style, C Rect &rect, CChar  *t, AUTO_LINE_MODE auto_line) {text_style.drawCode(rect, t, auto_line);}

void DisplayDraw::text(                               C Rect &rect, CChar8 *t, AUTO_LINE_MODE auto_line) {if(Gui.skin && Gui.skin->text_style)Gui.skin->text_style->drawCode(rect, t, auto_line);}
void DisplayDraw::text(                               C Rect &rect, CChar  *t, AUTO_LINE_MODE auto_line) {if(Gui.skin && Gui.skin->text_style)Gui.skin->text_style->drawCode(rect, t, auto_line);}
/******************************************************************************/
void TextStyleParams::drawSoft(Image &image, Flt x, Flt y, CChar8 *t)C {drawMainSoft(image, x, y, t);}
void TextStyleParams::drawSoft(Image &image, Flt x, Flt y, CChar  *t)C {drawMainSoft(image, x, y, t);}

void TextStyleParams::drawSoft(Image &image, C Rect &rect, CChar8 *t, AUTO_LINE_MODE auto_line)C {Memt<TextLineSplit8 > tls8 ; Set(tls8 , t, T, rect.w(), auto_line); drawSplitSoft(image, rect, tls8 );} // use temp because this can be called outside of Draw
void TextStyleParams::drawSoft(Image &image, C Rect &rect, CChar  *t, AUTO_LINE_MODE auto_line)C {Memt<TextLineSplit16> tls16; Set(tls16, t, T, rect.w(), auto_line); drawSplitSoft(image, rect, tls16);} // use temp because this can be called outside of Draw
/******************************************************************************/
// IO
/******************************************************************************/
#pragma pack(push, 1)
struct TextStyleDesc
{
   SPACING_MODE spacing;
   Byte         shadow, shade;
   Color        color, selection;
   Vec2         align, size, space;
};
#pragma pack(pop)

Bool TextStyle::save(File &f, CChar *path)C
{
   f.putMulti(UInt(CC4_TXDS), Byte(4)); // version

   TextStyleDesc desc;
   Unaligned(desc.spacing  , spacing);
   Unaligned(desc.shadow   , shadow);
   Unaligned(desc.shade    , shade);
   Unaligned(desc.color    , color);
   Unaligned(desc.selection, selection);
   Unaligned(desc.align    , align);
   Unaligned(desc.size     , size);
   Unaligned(desc.space    , space);
   f<<desc;
   f._putAsset(font().name(path));
   return f.ok();
}
Bool TextStyle::load(File &f, CChar *path)
{
   pixel_align=true;
   edit=null;
   if(f.getUInt()==CC4_TXDS)switch(f.decUIntV()) // version
   {
      case 4:
      {
         TextStyleDesc desc; if(f.getFast(desc))
         {
            Unaligned(spacing  , desc.spacing);
            Unaligned(shadow   , desc.shadow);
            Unaligned(shade    , desc.shade);
            Unaligned(color    , desc.color);
            Unaligned(selection, desc.selection);
            Unaligned(align    , desc.align);
            Unaligned(size     , desc.size);
            Unaligned(space    , desc.space);
           _font.require(f._getAsset(), path); super::font(_font());
            if(f.ok())return true;
         }
      }break;

      case 3:
      {
         #pragma pack(push, 1)
         struct TextStyleDesc
         {
            SPACING_MODE spacing;
            Byte         shadow, shade;
            Color        color;
            Vec2         align, size, space;
         };
         #pragma pack(pop)
         TextStyleDesc desc; if(f.get(desc))
         {
            Unaligned(spacing, desc.spacing);
            Unaligned(shadow , desc.shadow);
            Unaligned(shade  , desc.shade);
            Unaligned(color  , desc.color);
            Unaligned(align  , desc.align);
            Unaligned(size   , desc.size);
            Unaligned(space  , desc.space);
           _font.require(f._getStr(), path); super::font(_font());
            selection=DefaultSelectionColor;
            if(f.ok())return true;
         }
      }break;

      case 2:
      {
         #pragma pack(push, 4)
         struct TextStyleDesc
         {
            SPACING_MODE spacing;
            Byte         shadow, shade;
            Color        color;
            Vec2         align, size, space;
         };
         #pragma pack(pop)
         TextStyleDesc desc; if(f.get(desc))
         {
            Unaligned(spacing, desc.spacing);
            Unaligned(shadow , desc.shadow);
            Unaligned(shade  , desc.shade);
            Unaligned(color  , desc.color);
            Unaligned(align  , desc.align);
            Unaligned(size   , desc.size);
            Unaligned(space  , desc.space);
           _font.require(f._getStr(), path); super::font(_font());
            selection=DefaultSelectionColor;
            if(f.ok())return true;
         }
      }break;

      case 1:
      {
         #pragma pack(push, 4)
         struct TextStyleDesc
         {
            SPACING_MODE spacing;
            Byte         shadow, shade;
            VecB4        color;
            Vec2         align, size, space;
         };
         #pragma pack(pop)
         TextStyleDesc desc; if(f.get(desc))
         {
            Unaligned(spacing, desc.spacing);
            Unaligned(shadow , desc.shadow);
            Unaligned(shade  , desc.shade);
                      color  .set(Unaligned(desc.color.z), Unaligned(desc.color.y), Unaligned(desc.color.x), Unaligned(desc.color.w));
            Unaligned(align  , desc.align);
            Unaligned(size   , desc.size);
            Unaligned(space  , desc.space);
           _font.require(f._getStr(), path); super::font(_font());
            selection=DefaultSelectionColor;
            if(f.ok())return true;
         }
      }break;

      case 0:
      {
         #pragma pack(push, 4)
         struct TextStyleDesc
         {
            SPACING_MODE spacing;
            Byte         shadow, shade;
            VecB4        color;
            Vec2         align, size, space;
         };
         #pragma pack(pop)
         TextStyleDesc desc; if(f.get(desc))
         {
            Unaligned(spacing, desc.spacing);
            Unaligned(shadow , desc.shadow);
                      shade  =DefaultShade;
                      color  .set(Unaligned(desc.color.z), Unaligned(desc.color.y), Unaligned(desc.color.x), Unaligned(desc.color.w));
            Unaligned(align  , desc.align);
            Unaligned(size   , desc.size);
            Unaligned(space  , desc.space);
           _font.require(f._getStr8(), path); super::font(_font());
            selection=DefaultSelectionColor;
            if(f.ok())return true;
         }
      }break;
   }
   reset(); return false;
}

Bool TextStyle::save(C Str &name)C
{
   File f; if(f.writeTry(name)){if(save(f, _GetPath(name)) && f.flush())return true; f.del(); FDelFile(name);}
   return false;
}
Bool TextStyle::load(C Str &name)
{
   File f; if(f.readTry(name))return load(f, _GetPath(name));
   reset(); return false;
}
void TextStyle::operator=(C Str &name)
{
   if(!load(name))Exit(MLT(S+"Can't load Text Style \""         +name+"\"",
                       PL,S+u"Nie można wczytać stylu tekstu \""+name+"\""));
}
/******************************************************************************/
// TEXT CODE
/******************************************************************************/
static TextCodeData& NewCode(MemPtr<TextCodeData> codes, Str &text)
{
   TextCodeData &code=codes.New(); code.pos=(CPtr)text.length();
   if(codes.elms()>=2)
   {
      TextCodeData &last=codes[codes.elms()-2];
      code.shadow_mode=((last.shadow_mode==TextCodeData::DEFAULT) ? TextCodeData::DEFAULT : TextCodeData::PREV); code.shadow=last.shadow;
      code. color_mode=((last. color_mode==TextCodeData::DEFAULT) ? TextCodeData::DEFAULT : TextCodeData::PREV); code.color =last.color ;
      code.nocode_mode=((last.nocode_mode==TextCodeData::DEFAULT) ? TextCodeData::DEFAULT : TextCodeData::PREV);
   }else
   {
      code.shadow_mode=code.color_mode=code.nocode_mode=TextCodeData::DEFAULT;
      code.shadow=255;
      code.color =WHITE;
   }
   return code;
}
static Bool GetDig(Byte &dig, CChar* &text)
{
   Int i=CharInt(*text);
   if( i>=0){dig=i; text++; return true;}
   return false;
}
static Color GetColor(CChar *text)
{
   text=_SkipWhiteChars(text);
   Byte dig[8], digs=0; FREP(8)if(GetDig(dig[i], text))digs++;else break;
   switch(digs)
   {
      case  3: return Color( dig[0]*255/0xF   ,  dig[1]*255/0xF   ,  dig[2]*255/0xF                       ); // RGB
      case  4: return Color( dig[0]*255/0xF   ,  dig[1]*255/0xF   ,  dig[2]*255/0xF   ,  dig[3]*255/0xF   ); // RGBA
      case  6: return Color((dig[0]<<4)|dig[1], (dig[2]<<4)|dig[3], (dig[4]<<4)|dig[5]                    ); // RRGGBB
      case  8: return Color((dig[0]<<4)|dig[1], (dig[2]<<4)|dig[3], (dig[4]<<4)|dig[5], (dig[6]<<4)|dig[7]); // RRGGBBAA
      default: return WHITE;
   }
}
static Byte GetByte(CChar *text)
{
   text=_SkipWhiteChars(text);
   Byte dig[2], digs=0; FREP(2)if(GetDig(dig[i], text))digs++;else break;
   switch(digs)
   {
      case  1: return  dig[0]*255/0xF;
      case  2: return (dig[0]<<4)|dig[1];
      default: return 255;
   }
}
void SetTextCode(C Str &code, Str &text, MemPtr<TextCodeData> codes)
{
   // 'codes' initially uses 'pos' as Int
   text .clear();
   codes.clear();

   Bool               nocode=false;
   MemtN<Color, 1024> colors ;
   MemtN<Byte , 1024> shadows;

   for(CChar *cc=code(); cc; )
   {
      Char c=*cc++; if(!c)break;
      if(c=='[') // code sign
      {
         if(!nocode && (Starts(cc, "col=") || Starts(cc, "color=")))
         {
            if(cc=TextPos(cc, '='))
            {
               Color         col =GetColor(cc+1);
               TextCodeData &code=NewCode(codes, text);
               code.color_mode=TextCodeData::NEW;
               code.color     =col; colors.New()=col;
            }
            if(cc=TextPos(cc, ']'))cc++;
         }else
         if(!nocode && (Starts(cc, "shd=") || Starts(cc, "shadow=")))
         {
            if(cc=TextPos(cc, '='))
            {
               Byte          shd =GetByte(cc+1);
               TextCodeData &code=NewCode(codes, text);
               code.shadow_mode=TextCodeData::NEW;
               code.shadow     =shd; shadows.New()=shd;
            }
            if(cc=TextPos(cc, ']'))cc++;
         }else
         if(!nocode && Starts(cc, "/col"))
         {
            TextCodeData &code=NewCode(codes, text);
            colors.removeLast();
            if(colors.elms())
            {
               code.color_mode=TextCodeData::OLD;
               code.color     =colors.last();
            }else
            {
               code.color_mode=TextCodeData::DEFAULT;
               code.color     =WHITE;
            }
            if(cc=TextPos(cc, ']'))cc++;
         }else
         if(!nocode && Starts(cc, "/sh"))
         {
            TextCodeData &code=NewCode(codes, text);
            shadows.removeLast();
            if(shadows.elms())
            {
               code.shadow_mode=TextCodeData::OLD;
               code.shadow     =shadows.last();
            }else
            {
               code.shadow_mode=TextCodeData::DEFAULT;
               code.shadow     =255;
            }
            if(cc=TextPos(cc, ']'))cc++;
         }else
         if(Starts(cc, "nocode"))
         {
            nocode=true;
            TextCodeData &code=NewCode(codes, text);
            code.nocode_mode=TextCodeData::NEW;
            if(cc=TextPos(cc, ']'))cc++;
         }else
         if(Starts(cc, "/nocode"))
         {
            nocode=false;
            TextCodeData &code=NewCode(codes, text);
            code.nocode_mode=TextCodeData::DEFAULT;
            if(cc=TextPos(cc, ']'))cc++;
         }else
         {
            text+='[';
         }
      }else
      {
         text+=c;
      }
   }

   // update codes
   REPA(codes)codes[i].pos=CPtr(text()+UIntPtr(codes[i].pos)); // convert Int to Ptr
}
Str GetTextCode(C Str &text, C TextCodeData *code, Int codes)
{
   Str out;
   Int cur_code=0, colors=0, shadows=0, nocodes=0;
   for(Int i=0; ; i++)
   {
      for(; InRange(cur_code, codes) && text()+i>=code[cur_code].pos; )
      {
       C TextCodeData &c=code[cur_code++];

         switch(c.color_mode)
         {
            case TextCodeData::NEW: out+="[color="; out+=c.color.asHex(); out+=']'; colors++; break;

            case TextCodeData::DEFAULT: if(!colors)break; // on purpose without break
            case TextCodeData::OLD    : colors--; out+="[/color]"; break;
         }

         switch(c.shadow_mode)
         {
            case TextCodeData::NEW: out+="[shadow="; out+=TextHex(U32(c.shadow), 2); out+=']'; shadows++; break;

            case TextCodeData::DEFAULT: if(!shadows)break; // on purpose without break
            case TextCodeData::OLD    : shadows--; out+="[/shadow]"; break;
         }

         switch(c.nocode_mode)
         {
            case TextCodeData::NEW: out+="[nocode]"; nocodes++; break;

            case TextCodeData::DEFAULT: if(!nocodes)break; // on purpose without break
            case TextCodeData::OLD    : nocodes--; out+="[/nocode]"; break;
         }
      }
      if(i>=text.length())break;
       out+=text[i];
   }
   return out;
}
/******************************************************************************/
}
/******************************************************************************/
