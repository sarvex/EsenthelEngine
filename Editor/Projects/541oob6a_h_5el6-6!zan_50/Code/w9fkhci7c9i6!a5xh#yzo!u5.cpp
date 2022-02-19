/******************************************************************************/
Mesh     box ,
         ball;
ImageRTC rt  ; // Render Target Image (of IMAGE_RT mode), must be 'ImageRTC' and not 'Image'
/******************************************************************************/
void InitPre()
{
   INIT();
   Ms.hide();
   Ms.clip(null, 1);

   D.ambientPowerL(0);
}
/******************************************************************************/
void InitRT()
{
   int tex_size=256;
   rt.mustCreate(VecI2(tex_size, tex_size), IMAGE_R8G8B8A8_SRGB);
}
bool Init()
{
   Cam.dist=3;

   InitRT();

   MaterialPtr material=UID(2123216029, 1141820639, 615850919, 3316401700);

   box .parts.New().base.create( Box(4), VTX_TEX0|VTX_NRM|VTX_TAN).reverse(); // create mesh box, reverse it because it's meant to be viewed from inside
   ball.parts.New().base.create(Ball(1), VTX_TEX0|VTX_NRM|VTX_TAN)          ; // create mesh ball

   // set mesh materials, rendering versions and bounding boxes
   box .material(material).setRender().setBox();
   ball.material(material).setRender().setBox();

   return true;
}
void Shut()
{
}
/******************************************************************************/
bool Update()
{
   if(Kb.bp(KB_ESC))return false;
   Cam.transformByMouse(0.1, 10, CAMH_ZOOM|(Ms.b(1)?CAMH_MOVE:CAMH_ROT)); // move camera on right mouse button
   return true;
}
/******************************************************************************/
void Render()
{
   switch(Renderer())
   {
      case RM_PREPARE:
      {
         box .draw(MatrixIdentity);
         ball.draw(MatrixIdentity);

         LightPoint(25, Vec(0, 3, 0)).add();
      }break;
   }
}
void RenderToTexture()
{
   // render to texture
   Renderer.target=&rt;  // specify custom render target
   Renderer(Render);     // perform rendering
   Renderer.target=null; // disable custom render target
}
void Draw()
{
   // first handle rendering to texture before anyother drawing/rendering
   RenderToTexture();

   // render normally
   Renderer(Render);

   // now we can use previously rendered texture
   ALPHA_MODE alpha=D.alpha(ALPHA_NONE);                  // the rendering result's alpha channel is undefined so we can't use it, for that we need to disable alpha blending
   rt.drawRotate(Vec2(0, 0), Vec2(1, 1), Time.appTime()); // draw texture
   D.alpha(alpha);                                        // restore previously modified alpha mode
}
/******************************************************************************/
