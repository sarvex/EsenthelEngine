/******************************************************************************/
const_mem_addr struct Text : GuiObj // Gui Text !! must be stored in constant memory address !!
{
   Bool          auto_line; // automatically calculate new lines when drawing text, default=false
   TextStylePtr text_style; // text style   , default=null (if set to null then current value of 'skin.text.text_style' is used)
   GuiSkinPtr         skin; // skin override, default=null (if set to null then current value of 'Gui.skin'             is used)
   Str                text;
   StrEx             extra;

   // manage
   virtual Text& del   (                                                    )override;                                 // delete
           Text& create(              C Str &text=S, C TextStylePtr &ts=null);                                         // create, 'ts'=text style (the object is not copied, only the pointer to the object is remembered, therefore it must point to a constant memory address !!)
           Text& create(C Vec2 &rect, C Str &text=S, C TextStylePtr &ts=null) {create(text, ts).rect(rect); return T;} // create, 'ts'=text style (the object is not copied, only the pointer to the object is remembered, therefore it must point to a constant memory address !!)
           Text& create(C Rect &rect, C Str &text=S, C TextStylePtr &ts=null) {create(text, ts).rect(rect); return T;} // create, 'ts'=text style (the object is not copied, only the pointer to the object is remembered, therefore it must point to a constant memory address !!)
           Text& create(C Text &src                                         );                                         // create from 'src'

   // get / set
   Text& clear(           );                                     // clear   text value
   Text& set  (C Str &text); C Str& operator()()C {return text;} // set/get text value

   GuiSkin  * getSkin     ()C {return skin ? skin() : Gui.skin();} // get actual skin
   TextStyle* getTextStyle()C;                                     // get actual text style

   Vec2 textSize()C; // get visible text size

   // main
   virtual void draw(C GuiPC &gpc)override; // draw object

#if EE_PRIVATE
   void zero();
#endif

  ~Text() {del();}
   Text();

protected:
   virtual Bool save(File &f, CChar *path=null)C override;
   virtual Bool load(File &f, CChar *path=null)  override;

private:
   NO_COPY_CONSTRUCTOR(Text);
};
/******************************************************************************/
struct TextNoTest : Text
{
   virtual GuiObj* test(C GuiPC &gpc, C Vec2 &pos, GuiObj* &mouse_wheel)override {return null;}
   virtual void nearest(C GuiPC &gpc, GuiObjNearest &gon               )override {}
};
/******************************************************************************/
