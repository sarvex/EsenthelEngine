/******************************************************************************/
Mesh box ,
     ball;
/******************************************************************************/
void InitPre()
{
   INIT();
   Ms.hide();
   Ms.clip(null, 1);

   D.ambientPowerL(0);
}
/******************************************************************************/
bool Init()
{
   Cam.dist=3;

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
void Draw()
{
   // first handle rendering to texture before anyother drawing/rendering
   ImageRTPtr rt=Renderer.get(Render);

   // render normally
   Renderer(Render);

   // now we can use previously rendered texture
   ALPHA_MODE alpha=D.alpha(ALPHA_NONE);                  // the rendering result's alpha channel is undefined so we can't use it, for that we need to disable alpha blending
   rt->drawRotate(Vec2(0, 0), Vec2(1, 1), Time.appTime()); // draw texture
   D.alpha(alpha);                                        // restore previously modified alpha mode
}
/******************************************************************************/
