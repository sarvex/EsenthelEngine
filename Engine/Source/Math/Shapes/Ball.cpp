/******************************************************************************/
#include "stdafx.h"
namespace EE{
/******************************************************************************/
#define SCALE 1.5f // max assumed stretch (for example if the original box is set to a character in rest pose, but due to animation it moves its arms around, then the box after animation might be bigger than original non-animated box, this allows to specify max allowed scale of the original box)
/******************************************************************************
Ball& Ball::setAnimated(C Box &box, C Matrix &matrix)
{
   r  =LengthMul(box.size  (), matrix.x)*(0.5f*SCALE);
   pos=          box.center()* matrix;
   return T;
}
Ball& Ball::setAnimated(C Box &box, C AnimatedSkeleton &anim_skel)
{
   r  =LengthMul(box.size  (), anim_skel.matrix().x)*(0.5f*SCALE);
   pos=          box.center()* anim_skel.matrix();
   return T;
}
/******************************************************************************/
Ball& Ball::setAnimated(C Extent &ext, C Matrix &matrix)
{
   r  =LengthMul(ext.ext, matrix.x)*SCALE;
   pos=          ext.pos* matrix;
   return T;
}
Ball& Ball::setAnimated(C Extent &ext, C AnimatedSkeleton &anim_skel)
{
   r  =LengthMul(ext.ext, anim_skel.matrix().x)*SCALE;
   pos=          ext.pos* anim_skel.matrix();
   return T;
}
/******************************************************************************/
Ball ::Ball (C Extent   &ext    ) {set(     ext.ext   .length()     , ext    .pos     );}
Ball ::Ball (C Box      &box    ) {set(     box.size().length()*0.5f, box    .center());}
Ball ::Ball (C OBox     &obox   ) {set(obox.box.size().length()*0.5f, obox   .center());}
Ball ::Ball (C Capsule  &capsule) {set(     capsule.ballRSafe()     , capsule.pos     );}
BallM::BallM(C CapsuleM &capsule) {set(     capsule.ballRSafe()     , capsule.pos     );}
Ball ::Ball (C Shape    &shape  )
{
   switch(shape.type)
   {
      case SHAPE_BOX    : T=shape.box    ; break;
      case SHAPE_OBOX   : T=shape.obox   ; break;
      case SHAPE_BALL   : T=shape.ball   ; break;
      case SHAPE_CAPSULE: T=shape.capsule; break;
   }
}
Ball::Ball(C MeshBase &mesh) {from(mesh.vtx.pos(), mesh.vtxs());}

Ball& Ball::operator*=(  Flt      f) {pos*=f; r*=f; return T;}
Ball& Ball::operator/=(  Flt      f) {pos/=f; r/=f; return T;}
Ball& Ball::operator*=(C Matrix3 &m)
{
   pos*=m;
   r  *=m.maxScale();
   return T;
}
Ball& Ball::operator*=(C Matrix &m)
{
   pos*=m;
   r  *=m.maxScale();
   return T;
}
Ball& Ball::operator/=(C Matrix3 &m)
{
   pos/=m;
   r  /=m.maxScale();
   return T;
}
Ball& Ball::operator/=(C Matrix &m)
{
   pos/=m;
   r  /=m.maxScale();
   return T;
}
/******************************************************************************/
Bool Ball::from(C Vec *point, Int points)
{
   Box box; if(box.from(point, points))
   {
      pos=box.center();
      Flt dist2=0; REP(points)MAX(dist2, Dist2(point[i], pos));
      r=SqrtFast(dist2);
      return true;
   }
   zero(); return false;
}
Bool Ball::from(C Vec *point, Int points, C Matrix &matrix)
{
   Box box; if(box.from(point, points))
   {
      pos=box.center()*matrix;
      Flt dist2=0; REP(points)MAX(dist2, Dist2(point[i]*matrix, pos));
      r=SqrtFast(dist2);
      return true;
   }
   zero(); return false;
}
/******************************************************************************/
Vec Ball::nearest(C Vec &normal)C {return pos-normal*r;}
/******************************************************************************/
void Ball::drawVI(Bool fill, C VecI2 &resolution)C
{
   Int res_x=((resolution.x<0) ? 24 : ((resolution.x<3) ? 3 : resolution.x)),
       res_y=((resolution.y<0) ? 24 : ((resolution.y<3) ? 3 : resolution.y));
   Flt y1_sr=r,
       y1_cr=0;

   REPD(y, res_y-1)
   {
      Flt y2_cr, y2_sr; CosSin(y2_cr, y2_sr, -PI_2+PI*y/(res_y-1)); y2_sr*=r; y2_cr*=r;
      Flt x1_s=0,
          x1_c=1;
      REPD(x, res_x)
      {
         Flt x2_c, x2_s; CosSin(x2_c, x2_s, (PI2*x)/res_x);
         if(fill)
         {
            VI.quad(pos+Vec(x2_c*y1_cr, y1_sr, x2_s*y1_cr),
                    pos+Vec(x1_c*y1_cr, y1_sr, x1_s*y1_cr),
                    pos+Vec(x1_c*y2_cr, y2_sr, x1_s*y2_cr),
                    pos+Vec(x2_c*y2_cr, y2_sr, x2_s*y2_cr));
         }else
         {
                 VI.line(pos+Vec(x1_c*y1_cr, y1_sr, x1_s*y1_cr), pos+Vec(x1_c*y2_cr, y2_sr, x1_s*y2_cr));
            if(y)VI.line(pos+Vec(x1_c*y2_cr, y2_sr, x1_s*y2_cr), pos+Vec(x2_c*y2_cr, y2_sr, x2_s*y2_cr));
         }
         x1_s=x2_s;
         x1_c=x2_c;
      }
      y1_sr=y2_sr;
      y1_cr=y2_cr;
   }
}
void Ball::draw(C Color &color, Bool fill, C VecI2 &resolution)C
{
   VI.color(color); drawVI(fill, resolution);
   VI.end  (     );
}
void Ball::drawAngle(C Color &color, Flt from, Flt to, C Vec &up, Bool fill, C VecI2 &resolution)C
{
   to-=from;
   Matrix m; m.setPosUp(VecZero, up).scale(r).move(pos);
   Int    res_x=((resolution.x<0) ? 24 : ((resolution.x<3) ? 3 : resolution.x)),
          res_y=((resolution.y<0) ? 24 : ((resolution.y<3) ? 3 : resolution.y));
   Flt    y1_cr, y1_sr; CosSin(y1_cr, y1_sr, from+to);

   VI.color(color);
   REPD(y, res_y-1)
   {
      Flt y2_cr, y2_sr; CosSin(y2_cr, y2_sr, from+to*y/(res_y-1));
      Flt x1_s=0,
          x1_c=1;
      REPD(x, res_x)
      {
         Flt x2_c, x2_s; CosSin(x2_c, x2_s, (PI2*x)/res_x);
         if(fill)
         {
            VI.quad(Vec(x2_c*y1_cr, y1_sr, x2_s*y1_cr)*m,
                    Vec(x1_c*y1_cr, y1_sr, x1_s*y1_cr)*m,
                    Vec(x1_c*y2_cr, y2_sr, x1_s*y2_cr)*m,
                    Vec(x2_c*y2_cr, y2_sr, x2_s*y2_cr)*m);
         }else
         {
                 VI.line(Vec(x1_c*y1_cr, y1_sr, x1_s*y1_cr)*m, Vec(x1_c*y2_cr, y2_sr, x1_s*y2_cr)*m);
            if(y)VI.line(Vec(x1_c*y2_cr, y2_sr, x1_s*y2_cr)*m, Vec(x2_c*y2_cr, y2_sr, x2_s*y2_cr)*m);
         }
         x1_s=x2_s;
         x1_c=x2_c;
      }
      y1_sr=y2_sr;
      y1_cr=y2_cr;
   }
   VI.end();
}
void Ball::draw2(C Color &color, Bool fill, Int resolution)C
{
   Matrix m[6]; REP(6)m[i].setPosOrient(pos, DIR_ENUM(i));
   m[DIR_BACK].y.chs();
   m[DIR_LEFT].y.chs();
   m[DIR_DOWN].x.set(0, 0,-1);
   m[DIR_DOWN].y.set(1, 0, 0);
   m[DIR_DOWN].z.set(0,-1, 0);

   if(resolution<0)resolution=12;else if(resolution<1)resolution=1;
   Vec t(-resolution/2.0f, -resolution/2.0f, resolution/2.0f);

   VI.color(color);
   REPD(y, resolution)
   REPD(x, resolution)
   {
      Vec bp[4]=
      {
         Vec(x  , y+1, 0),
         Vec(x+1, y+1, 0),
         Vec(x+1, y  , 0),
         Vec(x  , y  , 0),
      };
      REP(4){bp[i]+=t; bp[i].setLength(r);}
      REPD(s, 6)
      {
         Vec p[4]; Copy(p, bp); Transform(p, m[s], 4);
         if(fill)VI.quad(p[0], p[1], p[2], p[3]);else
         {
            VI.line(p[0], p[1]);
            VI.line(p[0], p[3]);
         }
      }
   }
   VI.end();
}
/******************************************************************************/
void SphereConvert::init(Int res)
{
   T.res=res;
   pos_to_cell_mul=res/PI_2; pos_to_cell_add=res*0.5f;
   cell_to_pos_mul=PI_2/res; //cell_to_pos_add=-PI_4;
   tans.setNumDiscard(res+1); REPAO(tans)=cellToPos(i);
}
void SphereConvertEx::init(Int res)
{
   super::init(res);
   tan_mids.setNumDiscard(res); REPAO(tan_mids)=cellToPos(i+0.5f);
}
/******************************************************************************/
DIR_ENUM DirToCubeFace(C Vec &dir)
{
#if 1 // faster
   Vec abs=Abs(dir); if(abs.x>=abs.z)
   {
      if(abs.x>=abs.y)return (dir.x>=0) ? DIR_RIGHT   : DIR_LEFT; // X
                   Y: return (dir.y>=0) ? DIR_UP      : DIR_DOWN; // Y
   }
      if(abs.y>=abs.z)goto Y;
                      return (dir.z>=0) ? DIR_FORWARD : DIR_BACK; // Z
#else
   switch(Abs(dir).maxI())
   {
      case 0: return (dir.x>=0) ? DIR_RIGHT   : DIR_LEFT; // X
      case 1: return (dir.y>=0) ? DIR_UP      : DIR_DOWN; // Y
      case 2: return (dir.z>=0) ? DIR_FORWARD : DIR_BACK; // Z
   }
#endif
}
/******************************************************************************/
void PosToSphereTerrainPos(DIR_ENUM dir, Vec2 &dest, C Vec &src)
{
   switch(dir) // #TerrainOrient
   {
      case DIR_RIGHT  : dest.set( src.z,  src.y); break;
      case DIR_LEFT   : dest.set(-src.z,  src.y); break;
      case DIR_UP     : dest.set( src.x,  src.z); break;
      case DIR_DOWN   : dest.set( src.x, -src.z); break;
      case DIR_FORWARD: dest.set(-src.x,  src.y); break;
      case DIR_BACK   : dest.set( src.x,  src.y); break;
   }
}
void PosToSphereTerrainPos(DIR_ENUM dir, Vec &dest, C Vec &src)
{
   switch(dir) // #TerrainOrient
   {
      case DIR_RIGHT  : dest.set( src.z,  src.y,  src.x); break;
      case DIR_LEFT   : dest.set(-src.z,  src.y, -src.x); break;
      case DIR_UP     : dest.set( src.x,  src.z,  src.y); break;
      case DIR_DOWN   : dest.set( src.x, -src.z, -src.y); break;
      case DIR_FORWARD: dest.set(-src.x,  src.y,  src.z); break;
      case DIR_BACK   : dest.set( src.x,  src.y, -src.z); break;
   }
}
void PosToSphereTerrainPos(DIR_ENUM dir, VecD *dest, C VecD *src, Int elms) // convert world space position 'src' to 'dest' where XY=plane position, Z=height
{
   switch(dir) // #TerrainOrient
   {
      case DIR_RIGHT  : REP(elms){C auto &s=src[i]; dest[i].set( s.z,  s.y,  s.x);} break;
      case DIR_LEFT   : REP(elms){C auto &s=src[i]; dest[i].set(-s.z,  s.y, -s.x);} break;
      case DIR_UP     : REP(elms){C auto &s=src[i]; dest[i].set( s.x,  s.z,  s.y);} break;
      case DIR_DOWN   : REP(elms){C auto &s=src[i]; dest[i].set( s.x, -s.z, -s.y);} break;
      case DIR_FORWARD: REP(elms){C auto &s=src[i]; dest[i].set(-s.x,  s.y,  s.z);} break;
      case DIR_BACK   : REP(elms){C auto &s=src[i]; dest[i].set( s.x,  s.y, -s.z);} break;
   }
}
/******************************************************************************
DIR_ENUM DirToCubeFace(C Vec &dir, Vec2 &xy)
{
   Vec abs=Abs(dir); if(abs.x>=abs.z)
   {
      if(abs.x>=abs.y)
      {
         Flt div=abs.x;
         if(!div    ){xy.zero(                     ); return DIR_RIGHT;} // only this case can have zero, because we've checked x>=z && x>=y, any other case will have non-zero
         if(dir.x>=0){xy.set(-dir.z/div, -dir.y/div); return DIR_RIGHT;}
                     {xy.set( dir.z/div, -dir.y/div); return DIR_LEFT ;}
      }
      Y: Flt div=abs.y;
         if(dir.y>=0){xy.set( dir.x/div,  dir.z/div); return DIR_UP   ;}
                     {xy.set( dir.x/div, -dir.z/div); return DIR_DOWN ;}
   }
      if(abs.y>=abs.z)goto Y;
         Flt div=abs.z;
         if(dir.z>=0){xy.set( dir.x/div, -dir.y/div); return DIR_FORWARD;}
                     {xy.set(-dir.x/div, -dir.y/div); return DIR_BACK   ;}
}*/
DIR_ENUM DirToCubeFacePixel(C Vec &dir, Int res, Vec2 &xy) // this matches exact same results as drawing Cube on GPU, 'xy' has to be passed to 'Image.colorFLinear' (for exact results Image also needs to have linear gamma)
{
   // Vec n=dir/Abs(dir).max();
   // xy.x=(n.x+1)/2*res-0.5
   // xy.x=(n.x+1)*res/2-0.5
   // xy.x=n.x*res/2 + res/2-0.5
   Flt mul=res*0.5f, add=mul-0.5f;
   Vec abs=Abs(dir); if(abs.x>=abs.z)
   {
      if(abs.x>=abs.y)
      {
         if( !abs.x ){xy.zero(                             ); return DIR_RIGHT;} // only this case can have zero, because we've checked x>=z && x>=y, any other case will have non-zero
         mul/=abs.x;
         if(dir.x>=0){xy.set(-dir.z*mul+add, -dir.y*mul+add); return DIR_RIGHT;}
                     {xy.set( dir.z*mul+add, -dir.y*mul+add); return DIR_LEFT ;}
      }
      Y: mul/=abs.y;
         if(dir.y>=0){xy.set( dir.x*mul+add,  dir.z*mul+add); return DIR_UP   ;}
                     {xy.set( dir.x*mul+add, -dir.z*mul+add); return DIR_DOWN ;}
   }
      if(abs.y>=abs.z)goto Y;
         mul/=abs.z;
         if(dir.z>=0){xy.set( dir.x*mul+add, -dir.y*mul+add); return DIR_FORWARD;}
                     {xy.set(-dir.x*mul+add, -dir.y*mul+add); return DIR_BACK   ;}
}
DIR_ENUM DirToSphereTerrainPixel(C Vec &dir, Int res, Vec2 &xy) // #TerrainOrient
{
#if 0
   Flt mul=res/PI_2, add=res*0.5f; // ((Atan(..)+PI_4)/PI_2)*res = (Atan(..)/PI_2+0.5)*res = Atan(..)*(res/PI_2)+(res*0.5)
   Vec abs=Abs(dir); if(abs.x>=abs.z)
   {
      if(abs.x>=abs.y)
      {
         if( !abs.x ){xy.zero(); return DIR_RIGHT;} // only this case can have zero, because we've checked x>=z && x>=y, any other case will have non-zero
         Flt y=Atan(dir.y/abs.x), z=Atan(dir.z/abs.x);
         if(dir.x>=0){xy.set( z*mul+add,  y*mul+add); return DIR_RIGHT;}
                     {xy.set(-z*mul+add,  y*mul+add); return DIR_LEFT ;}
      }
      Y: Flt x=Atan(dir.x/abs.y), z=Atan(dir.z/abs.y);
         if(dir.y>=0){xy.set( x*mul+add,  z*mul+add); return DIR_UP   ;}
                     {xy.set( x*mul+add, -z*mul+add); return DIR_DOWN ;}
   }
      if(abs.y>=abs.z)goto Y;
         Flt x=Atan(dir.x/abs.z), y=Atan(dir.y/abs.z);
         if(dir.z>=0){xy.set(-x*mul+add,  y*mul+add); return DIR_FORWARD;}
                     {xy.set( x*mul+add,  y*mul+add); return DIR_BACK   ;}
#else
   Flt x, y; DIR_ENUM ret;
   Vec abs=Abs(dir); if(abs.x>=abs.z)
   {
      if(abs.x>=abs.y)
      {
         if( !abs.x ){xy.zero(); return DIR_RIGHT;} // only this case can have zero, because we've checked x>=z && x>=y, any other case will have non-zero
         x=dir.z/abs.x; y=dir.y/abs.x;
         if(dir.x>=0){        ret=DIR_RIGHT;}
         else        {CHS(x); ret=DIR_LEFT ;}
      }else
      {
      Y: x=dir.x/abs.y; y=dir.z/abs.y;
         if(dir.y>=0){        ret=DIR_UP   ;}
         else        {CHS(y); ret=DIR_DOWN ;}
      }
   }else
   {
      if(abs.y>=abs.z)goto Y;
         x=dir.x/abs.z; y=dir.y/abs.z;
         if(dir.z>=0){CHS(x); ret=DIR_FORWARD;}
         else        {        ret=DIR_BACK   ;}
   }
   Flt mul=res/PI_2, add=res*0.5f; // ((Atan(..)+PI_4)/PI_2)*res = (Atan(..)/PI_2+0.5)*res = Atan(..)*(res/PI_2)+(res*0.5)
   xy.set(Atan(x)*mul+add, Atan(y)*mul+add); return ret;
#endif
}
DIR_ENUM SphereConvert::dirToSphereTerrainPixel(C Vec &dir, Vec2 &xy)C
{
   Flt x, y; DIR_ENUM ret;
   Vec abs=Abs(dir); if(abs.x>=abs.z)
   {
      if(abs.x>=abs.y)
      {
         if( !abs.x ){xy.zero(); return DIR_RIGHT;} // only this case can have zero, because we've checked x>=z && x>=y, any other case will have non-zero
         x=dir.z/abs.x; y=dir.y/abs.x;
         if(dir.x>=0){        ret=DIR_RIGHT;}
         else        {CHS(x); ret=DIR_LEFT ;}
      }else
      {
      Y: x=dir.x/abs.y; y=dir.z/abs.y;
         if(dir.y>=0){        ret=DIR_UP   ;}
         else        {CHS(y); ret=DIR_DOWN ;}
      }
   }else
   {
      if(abs.y>=abs.z)goto Y;
         x=dir.x/abs.z; y=dir.y/abs.z;
         if(dir.z>=0){CHS(x); ret=DIR_FORWARD;}
         else        {        ret=DIR_BACK   ;}
   }
   xy.set(posToCell(x), posToCell(y)); return ret;
}
/******************************************************************************/
Vec CubeFacePixelToDir(Flt x, Flt y, Int res, DIR_ENUM cube_face) // this matches exact same results as drawing Cube on GPU
{
   // x=(dir.x+1)/2*res-0.5
   // (x+0.5)*2/res-1=dir.x
   // dir.x=x*2/res + 0.5*2/res - 1
   // dir.x=x*2/res + 1/res-1
 //if(res>0)
   {
      Flt inv_res=1.0f/res, mul=2*inv_res, add=inv_res-1;
      x=x*mul+add;
      y=y*mul+add;
      switch(cube_face)
      {
         case DIR_RIGHT  : return Vec( 1, -y, -x);
         case DIR_LEFT   : return Vec(-1, -y,  x);
         case DIR_UP     : return Vec( x,  1,  y);
         case DIR_DOWN   : return Vec( x, -1, -y);
         case DIR_FORWARD: return Vec( x, -y,  1);
         case DIR_BACK   : return Vec(-x, -y, -1);
      }
   }
   return VecZero;
}
Vec SphereTerrainPixelToDir(Flt x, Flt y, Int res, DIR_ENUM cube_face) // #TerrainOrient
{
 //if(res>0)
   {
      Flt mul=PI_2/res, add=-PI_4;
      x=Tan(x*mul+add);
      y=Tan(y*mul+add);
      switch(cube_face)
      {
         case DIR_RIGHT  : return Vec( 1,  y,  x);
         case DIR_LEFT   : return Vec(-1,  y, -x);
         case DIR_UP     : return Vec( x,  1,  y);
         case DIR_DOWN   : return Vec( x, -1, -y);
         case DIR_FORWARD: return Vec(-x,  y,  1);
         case DIR_BACK   : return Vec( x,  y, -1);
      }
   }
   return VecZero;
}
Vec SphereConvert::sphereTerrainPixelToDir(Flt x, Flt y, DIR_ENUM cube_face)C // #TerrainOrient
{
 //if(res>0)
   {
      x=cellToPos(x);
      y=cellToPos(y);
      switch(cube_face)
      {
         case DIR_RIGHT  : return Vec( 1,  y,  x);
         case DIR_LEFT   : return Vec(-1,  y, -x);
         case DIR_UP     : return Vec( x,  1,  y);
         case DIR_DOWN   : return Vec( x, -1, -y);
         case DIR_FORWARD: return Vec(-x,  y,  1);
         case DIR_BACK   : return Vec( x,  y, -1);
      }
   }
   return VecZero;
}
Vec SphereConvert::_sphereTerrainPixelCenterToDir(Int xi, Int yi, DIR_ENUM cube_face)C // #TerrainOrient
{
 //if(res>0)
 //if(InRange(xi, res))
 //if(InRange(yi, res))
   {
      Flt x=Avg(_cellToPos(xi), _cellToPos(xi+1));
      Flt y=Avg(_cellToPos(yi), _cellToPos(yi+1));
      switch(cube_face)
      {
         case DIR_RIGHT  : return Vec( 1,  y,  x);
         case DIR_LEFT   : return Vec(-1,  y, -x);
         case DIR_UP     : return Vec( x,  1,  y);
         case DIR_DOWN   : return Vec( x, -1, -y);
         case DIR_FORWARD: return Vec(-x,  y,  1);
         case DIR_BACK   : return Vec( x,  y, -1);
      }
   }
   return VecZero;
}
Vec SphereConvertEx::_sphereTerrainPixelCenterToDir(Int xi, Int yi, DIR_ENUM cube_face)C // #TerrainOrient
{
 //if(res>0)
 //if(InRange(xi, res))
 //if(InRange(yi, res))
   {
      Flt x=_cellCenterToPos(xi);
      Flt y=_cellCenterToPos(yi);
      switch(cube_face)
      {
         case DIR_RIGHT  : return Vec( 1,  y,  x);
         case DIR_LEFT   : return Vec(-1,  y, -x);
         case DIR_UP     : return Vec( x,  1,  y);
         case DIR_DOWN   : return Vec( x, -1, -y);
         case DIR_FORWARD: return Vec(-x,  y,  1);
         case DIR_BACK   : return Vec( x,  y, -1);
      }
   }
   return VecZero;
}
/******************************************************************************/
Bool ClipZ(Ball &ball, Flt min_z) // clip ball to retain information only above 'min_z'
{
/* if Ball is fully above  'min_z' then keep it fully
   if Ball is fully below  'min_z' then clip fully (ret false)
   if Ball intersects with 'min_z' then clip it to smaller radius and adjust pos.z (height) (this will extend the ball upwards, but will reduce the radius)

                          XXXXXXXX
                        X         X
                     *****************
                 ****  X           X  ****
               *      X             X      *
             *        X             X        *
min_height - *--------X-------------X---------*
             *         X           X          *
             *          X         X           *
             *            X     X             *
              *             XXX              *
               *                            *
                 *                        *
                   *                    *
                      *              *
                          *  *  *
*/
   if(ball.pos.z>=min_z)return true;
   Flt d=min_z-ball.pos.z; if(d>=ball.r)return false;
   Flt cos=CosSin(d/ball.r);
   ball.r    *=cos;
   ball.pos.z+=d;
   return true;
}
static Bool PosToCellX(C SphereConvert &sc, C Vec &pos, Int &cell) {if(pos.z>0){cell=sc.posToCellI(pos.x/pos.z); return true;} return false;}
static Bool PosToCellY(C SphereConvert &sc, C Vec &pos, Int &cell) {if(pos.z>0){cell=sc.posToCellI(pos.y/pos.z); return true;} return false;}
void SphereConvert::getIntersectingSphereAreas(MemPtr<SphereArea> area_pos, C Ball &ball, Flt min_radius)C
{
   /* min_radius is treated as min_height
                    *
               *         *
            *               *
          *                   *
          \                  /
           \                /
            \              /
min_height - \------------/
           |  \          /
           |   \        /
           |    \      /
           |     \    /
           |      \  /
           -       \/
   */
   area_pos.clear();
   SphereArea ap;
   for(ap.side=DIR_ENUM(0); ; )
   {
      Ball oriented_ball; PosToSphereTerrainPos(ap.side, oriented_ball.pos, ball.pos); oriented_ball.r=ball.r;
      if(ClipZ(oriented_ball, min_radius))
      {
         RectI rect;
      #if 0 // simple but incorrect, because it treats ball as flat Circle
         oriented_ball/=oriented_ball.pos.z; // project to plane XY with Z=1
         rect.set(posToCellI(oriented_ball.pos.xy-oriented_ball.r),
                  posToCellI(oriented_ball.pos.xy+oriented_ball.r));
         rect.clampX(0, res-1);
         rect.clampY(0, res-1);
      #else
         // code based on 'ToScreenRect'
         Flt len2, sin2, cos, r2=Sqr(oriented_ball.r);
         Vec zd, d;

         zd.set(oriented_ball.pos.x, 0, oriented_ball.pos.z); len2=zd.length2();
         if(r2>=len2)rect.setX(0, res-1);else
         {
            sin2=r2/len2; cos=Sqrt(1-sin2); d=CrossUp(zd); d.setLength(cos*oriented_ball.r); zd*=-sin2; zd+=oriented_ball.pos;
            if(PosToCellX(T, zd-d, rect.min.x))MAX(rect.min.x,     0);else rect.min.x=    0;
            if(PosToCellX(T, zd+d, rect.max.x))MIN(rect.max.x, res-1);else rect.max.x=res-1;
            if(!rect.validX())goto next;
         }

         zd.set(0, oriented_ball.pos.y, oriented_ball.pos.z); len2=zd.length2();
         if(r2>=len2)rect.setY(0, res-1);else
         {
            sin2=r2/len2; cos=Sqrt(1-sin2); d=CrossRight(zd); d.setLength(cos*oriented_ball.r); zd*=-sin2; zd+=oriented_ball.pos;
            if(PosToCellY(T, zd+d, rect.min.y))MAX(rect.min.y,     0);else rect.min.y=    0;
            if(PosToCellY(T, zd-d, rect.max.y))MIN(rect.max.y, res-1);else rect.max.y=res-1;
            if(!rect.validY())goto next;
         }
      #endif
         VecI2 ball_cell=posToCellI(oriented_ball.pos.xy/oriented_ball.pos.z);
         for(ap.y=rect.min.y; ap.y<=rect.max.y; ap.y++)
         {
            // for distance, we just have to check corners, if ball.pos distance from Line(VecZero, area.corner) is >= ball.r
            // which corner, it depends on which side of the area the ball.pos is
            //
            //       O <- if ball.pos is here then we can't use corner, because it's not on left or right side
            // LU +----+ RU
            //    |    |
            //    |    |
            // LD +----+ RD
            //
            //                O <- if ball.pos is here, use RightDown corner

            Flt cell_pos_y=_cellToPos(ap.y+(ap.y<ball_cell.y)); // (ap.y<ball_cell.y) ? _cellToPos(ap.y+1) : _cellToPos(ap.y)
            for(ap.x=rect.min.x; ap.x<=rect.max.x; ap.x++)
            {
               if(ap.y!=ball_cell.y  // if ball is on down or up    side
               && ap.x!=ball_cell.x) // if ball is on left or right side
               {
                //left =(ap.x>ball_cell.x); if ball is on the left  side of area
                //right=(ap.x<ball_cell.x); if ball is on the right side of area
                //down =(ap.y>ball_cell.y); if ball is on the down  side of area
                //up   =(ap.y<ball_cell.y); if ball is on the up    side of area
                  Vec dir(_cellToPos(ap.x+(ap.x<ball_cell.x)), // (ap.x<ball_cell.x) ? _cellToPos(ap.x+1) : _cellToPos(ap.x)
                           cell_pos_y                        , // (ap.y<ball_cell.y) ? _cellToPos(ap.y+1) : _cellToPos(ap.y)
                           1);
                  dir.normalize();
                  if(Dist2PointLine(oriented_ball.pos, dir)>=r2)continue;
               }
               area_pos.add(ap);
            }
         }
      }
   next:
      if(ap.side==DIR_NUM-1)break; ap.side=DIR_ENUM(ap.side+1);
   }
}
/******************************************************************************/
void SphereConvertEx::sort(MemPtr<SphereArea> areas, C Vec &pos, Bool reverse)C
{
   Memt<SphereAreaDist> area_dist; area_dist.setNum(areas.elms()); REPA(area_dist)
   {
      SphereAreaDist &area=area_dist[i];
      SCAST(SphereArea, area)=areas[i];
      Vec  dir=_sphereTerrainPixelCenterToDir(area.x, area.y, area.side);
      area.dist=Dot   (pos, dir) // treat 'pos' as direction and reverse 'Compare' below
               *RSqrt0(dir.length2()); // '_sphereTerrainPixelCenterToDir' should be normalized, but here we can just use fast approximation
    //area.dist=Dist2 (pos, dir*RSqrt0(dir.length2())); this requires NOT reversing 'Compare' below // slower alternative
   }
   if(reverse)  area_dist.sort(Compare ); // 'Compare' are reversed because 'pos' is treated as direction instead of position
   else         area_dist.sort(CompareR);
   REPAO(areas)=area_dist[i];
}
/******************************************************************************/
void SphereConvert::draw()C
{
   REPA(tans)
   {
      D.lineX(GREY, tans[i], -1, 1);
      D.lineY(GREY, tans[i], -1, 1);
   }
   Rect(-1,-1,1,1).draw(WHITE, false);
}
void SphereConvert::drawCell(C Color &color, C VecI2 &cell)C
{
   if(InRange(cell.x, res))
   if(InRange(cell.y, res))
   {
      Rect r(_cellToPos(cell), _cellToPos(cell+1));
      r.draw(color, false);
   }
}
void SphereConvert::drawCell(C Color &color, C SphereArea &area, Flt radius)C
{
   if(InRange(area.x, res))
   if(InRange(area.y, res))
   {
      Quad q;
      q.p[0]=sphereTerrainPixelToDir(area.x  , area.y  , area.side);
      q.p[1]=sphereTerrainPixelToDir(area.x  , area.y+1, area.side);
      q.p[2]=sphereTerrainPixelToDir(area.x+1, area.y+1, area.side);
      q.p[3]=sphereTerrainPixelToDir(area.x+1, area.y  , area.side);
      REPAO(q.p).setLength(radius);
      q.draw(color, false);
   }
}
/******************************************************************************/
// BALL
/******************************************************************************/
Ball Avg(C Ball &a, C Ball &b) {return Ball(Avg(a.r, b.r), Avg(a.pos, b.pos));}
/******************************************************************************/
Flt Dist(C Vec &point, C Ball &ball)
{
   return Max(0, Dist(point, ball.pos)-ball.r);
}
Dbl Dist(C VecD &point, C BallM &ball)
{
   return Max(0, Dist(point, ball.pos)-ball.r);
}
Flt Dist(C Edge &edge, C Ball &ball)
{
   return Max(0, Dist(ball.pos, edge)-ball.r);
}
Flt Dist(C TriN &tri, C Ball &ball)
{
   return Max(0, Dist(ball.pos, tri)-ball.r);
}
Flt Dist(C Box &box, C Ball &ball)
{
   Flt dist2=0;

	if(ball.pos.x<box.min.x)dist2+=Sqr(ball.pos.x-box.min.x);else
	if(ball.pos.x>box.max.x)dist2+=Sqr(ball.pos.x-box.max.x);

	if(ball.pos.y<box.min.y)dist2+=Sqr(ball.pos.y-box.min.y);else
	if(ball.pos.y>box.max.y)dist2+=Sqr(ball.pos.y-box.max.y);

	if(ball.pos.z<box.min.z)dist2+=Sqr(ball.pos.z-box.min.z);else
	if(ball.pos.z>box.max.z)dist2+=Sqr(ball.pos.z-box.max.z);

   return Max(0, SqrtFast(dist2)-ball.r);
}
Flt Dist(C OBox &obox, C Ball &ball)
{
   Ball   temp=ball; temp.pos.divNormalized(obox.matrix);
   return Dist(obox.box, temp);
}
Flt Dist(C Extent &ext, C Ball &ball)
{
   return Max(0, Dist(ball.pos, ext)-ball.r);
}
Flt Dist(C Ball &a, C Ball &b)
{
   return Max(0, Dist(a.pos, b.pos)-a.r-b.r);
}
Flt DistBallPlane(C Ball &ball, C Vec &plane, C Vec &normal)
{
   return DistPointPlane(ball.pos, plane, normal)-ball.r;
}
Dbl DistBallPlane(C Ball &ball, C VecD &plane, C Vec &normal)
{
   return DistPointPlane(ball.pos, plane, normal)-ball.r;
}
Dbl DistBallPlane(C BallM &ball, C VecD &plane, C Vec &normal)
{
   return DistPointPlane(ball.pos, plane, normal)-ball.r;
}
/******************************************************************************/
Bool Cuts(C Vec &point, C Ball &ball)
{
   return Dist2(point, ball.pos)<=Sqr(ball.r);
}
Bool Cuts(C VecD &point, C Ball &ball)
{
   return Dist2(point, ball.pos)<=Sqr(ball.r);
}
Bool Cuts(C VecD &point, C BallM &ball)
{
   return Dist2(point, ball.pos)<=Sqr(ball.r);
}
Bool Cuts(C Edge &edge, C Ball &ball)
{
   return Dist(ball.pos, edge)<=ball.r;
}
Bool Cuts(C Tri &tri, C Ball &ball)
{
   return Dist(ball.pos, tri)<=ball.r;
}
Bool Cuts(C TriN &tri, C Ball &ball)
{
   return Dist(ball.pos, tri)<=ball.r;
}
Bool Cuts(C Ball &a, C Ball &b)
{
   return Dist2(a.pos, b.pos)<=Sqr(a.r+b.r);
}
Bool Cuts(C Ball &a, C BallM &b)
{
   return Dist2(a.pos, b.pos)<=Sqr(a.r+b.r);
}
Bool Cuts(C BallM &a, C BallM &b)
{
   return Dist2(a.pos, b.pos)<=Sqr(a.r+b.r);
}
Bool Cuts(C Box &box, C Ball &ball)
{
#if 1 // fastest
	if(ball.pos.x<box.min.x-ball.r || ball.pos.x>box.max.x+ball.r)return false;
	if(ball.pos.y<box.min.y-ball.r || ball.pos.y>box.max.y+ball.r)return false;
	if(ball.pos.z<box.min.z-ball.r || ball.pos.z>box.max.z+ball.r)return false;

   Flt dist2=0;

	if(ball.pos.x<box.min.x)dist2 =Sqr(ball.pos.x-box.min.x);else
	if(ball.pos.x>box.max.x)dist2 =Sqr(ball.pos.x-box.max.x);

	if(ball.pos.y<box.min.y)dist2+=Sqr(ball.pos.y-box.min.y);else
	if(ball.pos.y>box.max.y)dist2+=Sqr(ball.pos.y-box.max.y);

	if(ball.pos.z<box.min.z)dist2+=Sqr(ball.pos.z-box.min.z);else
	if(ball.pos.z>box.max.z)dist2+=Sqr(ball.pos.z-box.max.z);

   return dist2<=Sqr(ball.r);
#elif 1 // medium
   Flt dist2=0, r2=Sqr(ball.r);

	if(ball.pos.x<box.min.x){dist2 =Sqr(box.min.x-ball.pos.x); if(dist2>r2)return false;}else
	if(ball.pos.x>box.max.x){dist2 =Sqr(ball.pos.x-box.max.x); if(dist2>r2)return false;}

	if(ball.pos.y<box.min.y){dist2+=Sqr(box.min.y-ball.pos.y); if(dist2>r2)return false;}else
	if(ball.pos.y>box.max.y){dist2+=Sqr(ball.pos.y-box.max.y); if(dist2>r2)return false;}

	if(ball.pos.z<box.min.z){dist2+=Sqr(box.min.z-ball.pos.z); if(dist2>r2)return false;}else
	if(ball.pos.z>box.max.z){dist2+=Sqr(ball.pos.z-box.max.z); if(dist2>r2)return false;}

   return true;
#else // slowest
   Flt dist2=0, d;

	if(ball.pos.x<box.min.x){d=box.min.x-ball.pos.x; if(d>ball.r)return false; dist2 =Sqr(d);}else
	if(ball.pos.x>box.max.x){d=ball.pos.x-box.max.x; if(d>ball.r)return false; dist2 =Sqr(d);}

	if(ball.pos.y<box.min.y){d=box.min.y-ball.pos.y; if(d>ball.r)return false; dist2+=Sqr(d);}else
	if(ball.pos.y>box.max.y){d=ball.pos.y-box.max.y; if(d>ball.r)return false; dist2+=Sqr(d);}

	if(ball.pos.z<box.min.z){d=box.min.z-ball.pos.z; if(d>ball.r)return false; dist2+=Sqr(d);}else
	if(ball.pos.z>box.max.z){d=ball.pos.z-box.max.z; if(d>ball.r)return false; dist2+=Sqr(d);}

   return dist2<=Sqr(ball.r);
#endif
}
Bool Cuts(C Extent &ext, C Ball &ball)
{
#if 1 // faster
   return Dist2(Max(0, Abs(ext.pos.x-ball.pos.x)-ext.ext.x),
                Max(0, Abs(ext.pos.y-ball.pos.y)-ext.ext.y),
                Max(0, Abs(ext.pos.z-ball.pos.z)-ext.ext.z))<=Sqr(ball.r);
#else // slower
   Flt dist2=0, d;
   d=Abs(ext.pos.x-ball.pos.x)-ext.ext.x; if(d>0)dist2 =Sqr(d);
   d=Abs(ext.pos.y-ball.pos.y)-ext.ext.y; if(d>0)dist2+=Sqr(d);
   d=Abs(ext.pos.z-ball.pos.z)-ext.ext.z; if(d>0)dist2+=Sqr(d);
   return dist2<=Sqr(ball.r);
#endif
}
Bool Cuts(C Extent &ext, C BallM &ball)
{
   return Dist2(Max(0, Abs(ext.pos.x-ball.pos.x)-ext.ext.x),
                Max(0, Abs(ext.pos.y-ball.pos.y)-ext.ext.y),
                Max(0, Abs(ext.pos.z-ball.pos.z)-ext.ext.z))<=Sqr(ball.r);
}
Bool Cuts(C OBox &obox, C Ball &ball)
{
   Ball temp; // 'ball' in 'obox' space
   temp.r=ball.r;
   temp.pos.fromDivNormalized(ball.pos, obox.matrix);
   return Cuts(obox.box, temp);
}
/******************************************************************************/
Int CutsLineBall(C Vec &line_pos, C Vec &line_dir, C Ball &ball, Vec *contact_a, Vec *contact_b)
{
   Vec p=PointOnPlane(line_pos, ball.pos, line_dir);
   Flt s=Dist        (p       , ball.pos          );
   if(s> ball.r)return 0;
   if(s==ball.r){if(contact_a)*contact_a=p; return 1;}
   if(contact_a || contact_b)
   {
      s=CosSin(s/ball.r)*ball.r;
      if(contact_a)*contact_a=p-s*line_dir;
      if(contact_b)*contact_b=p+s*line_dir;
   }
   return 2;
}
Int CutsLineBall(C VecD &line_pos, C VecD &line_dir, C BallM &ball, VecD *contact_a, VecD *contact_b)
{
   VecD p=PointOnPlane(line_pos, ball.pos, line_dir);
   Dbl  s=Dist        (p       , ball.pos          );
   if(s> ball.r)return 0;
   if(s==ball.r){if(contact_a)*contact_a=p; return 1;}
   if(contact_a || contact_b)
   {
      s=CosSin(s/ball.r)*ball.r;
      if(contact_a)*contact_a=p-s*line_dir;
      if(contact_b)*contact_b=p+s*line_dir;
   }
   return 2;
}
Int CutsEdgeBall(C Vec &edge_start, C Vec &edge_end, C Ball &ball, Vec *contact_a, Vec *contact_b)
{
   Vec c[2], dir=!(edge_end-edge_start);
   if(Int hit=CutsLineBall(edge_start, dir, ball, &c[0], &c[1]))
   {
      Bool in[2]={(hit>=1), (hit>=2)};
      REP(2)if(in[i])if(DistPointPlane(c[i], edge_start, dir)<0 || DistPointPlane(c[i], edge_end, dir)>0)in[i]=false;
      if(in[0] && in[1]){if(contact_a)*contact_a=c[    0]; if(contact_b)*contact_b=c[1]; return 2;}
      if(in[0] || in[1]){if(contact_a)*contact_a=c[in[1]];                               return 1;}
   }
   return 0;
}
/******************************************************************************/
Bool Inside(C Box &a, C Ball &b)
{
#if 0 // this is slower
   return Dist2(Max(Abs(a.max.x-b.pos.x), Abs(a.min.x-b.pos.x)),
                Max(Abs(a.max.y-b.pos.y), Abs(a.min.y-b.pos.y)),
                Max(Abs(a.max.z-b.pos.z), Abs(a.min.z-b.pos.z)))<=Sqr(b.r);
#else // this is faster
   return Max(Sqr(a.max.x-b.pos.x), Sqr(a.min.x-b.pos.x))
         +Max(Sqr(a.max.y-b.pos.y), Sqr(a.min.y-b.pos.y))
         +Max(Sqr(a.max.z-b.pos.z), Sqr(a.min.z-b.pos.z))<=Sqr(b.r);
#endif
}
Bool Inside(C Extent &a, C Ball &b)
{
   return Dist2(((a.pos.x>b.pos.x) ? a.maxX() : a.minX())-b.pos.x,
                ((a.pos.y>b.pos.y) ? a.maxY() : a.minY())-b.pos.y,
                ((a.pos.z>b.pos.z) ? a.maxZ() : a.minZ())-b.pos.z)<=Sqr(b.r);
}
/******************************************************************************/
Bool SweepPointBall(C Vec &point, C Vec &move, C Ball &ball, Flt *hit_frac, Vec *hit_normal)
{
   Vec dir =move; Flt length=dir.normalize();
   Vec p   =PointOnPlane(point, ball.pos, dir);
   Flt s   =Dist(p, ball.pos)/ball.r; if(s>1)return false;
       p  -=dir*(CosSin(s)*ball.r);
   Flt dist=DistPointPlane(p, point, dir); if(dist<0 || dist>length)return false;
   if(hit_frac  )*hit_frac  =dist/length;
   if(hit_normal)*hit_normal=(p-ball.pos)/ball.r;
   return true;
}
Bool SweepPointBall(C VecD &point, C VecD &move, C BallD &ball, Dbl *hit_frac, VecD *hit_normal)
{
   VecD dir =move; Dbl length=dir.normalize();
   VecD p   =PointOnPlane(point, ball.pos, dir);
   Dbl  s   =Dist(p, ball.pos)/ball.r; if(s>1)return false;
        p  -=dir*(CosSin(s)*ball.r);
   Dbl  dist=DistPointPlane(p, point, dir); if(dist<0 || dist>length)return false;
   if(hit_frac  )*hit_frac  =dist/length;
   if(hit_normal)*hit_normal=(p-ball.pos)/ball.r;
   return true;
}
/******************************************************************************/
Bool SweepBallPoint(C Ball &ball, C Vec &move, C Vec &point, Flt *hit_frac, Vec *hit_normal)
{
   Vec dir =move; Flt length=dir.normalize();
   Vec p   =PointOnPlane(ball.pos, point, dir);
   Flt s   =Dist(p, point)/ball.r; if(s>1)return false;
       p  -=dir*(CosSin(s)*ball.r);
   Flt dist=DistPointPlane(p, ball.pos, dir); if(dist<0 || dist>length)return false;
   if(hit_frac  )*hit_frac  =dist/length;
   if(hit_normal)*hit_normal=(p-point)/ball.r;
   return true;
}
Bool SweepBallPoint(C BallD &ball, C VecD &move, C VecD &point, Dbl *hit_frac, VecD *hit_normal)
{
   VecD dir =move; Dbl length=dir.normalize();
   VecD p   =PointOnPlane(ball.pos, point, dir);
   Dbl  s   =Dist(p, point)/ball.r; if(s>1)return false;
        p  -=dir*(CosSin(s)*ball.r);
   Dbl  dist=DistPointPlane(p, ball.pos, dir); if(dist<0 || dist>length)return false;
   if(hit_frac  )*hit_frac  =dist/length;
   if(hit_normal)*hit_normal=(p-point)/ball.r;
   return true;
}
/******************************************************************************/
Bool SweepBallEdge(C Ball &ball, C Vec &move, C Edge &edge, Flt *hit_frac, Vec *hit_normal) // safe in case 'edge' is zero length
{
   Int point_test;
   Vec dir=edge.delta(); if(dir.normalize()) // check if 'edge' has length
   {
      Matrix matrix;        matrix.    setPosDir(edge.p[0], dir  );
      Circle circle(ball.r, matrix.      convert(ball.pos , true));
      Vec2   move2 =        matrix.orn().convert(move     , true) ;
      Vec2   normal; Flt frac;
      if(SweepCirclePoint(circle, move2, Vec2Zero, &frac, &normal))
      {
         Vec point=ball.pos+frac*move;
         if(DistPointPlane(point, edge.p[0], matrix.z)<0)point_test=0;else
         if(DistPointPlane(point, edge.p[1], matrix.z)>0)point_test=1;else
         {
            if(hit_frac  )*hit_frac  =frac;
            if(hit_normal)*hit_normal=matrix.orn().convert(normal);
            return true;
         }
      }else point_test=Closer(ball.pos, edge.p[0], edge.p[1]);
   }else point_test=0; // if 'edge' is zero length then check first point
   return SweepBallPoint(ball, move, edge.p[point_test], hit_frac, hit_normal);
}
Bool SweepBallEdge(C BallD &ball, C VecD &move, C EdgeD &edge, Dbl *hit_frac, VecD *hit_normal) // safe in case 'edge' is zero length
{
   Int  point_test;
   VecD dir=edge.delta(); if(dir.normalize()) // check if 'edge' has length
   {
      MatrixD matrix;        matrix.    setPosDir(edge.p[0], dir  );
      CircleD circle(ball.r, matrix.      convert(ball.pos , true));
      VecD2   move2 =        matrix.orn().convert(move     , true) ;
      VecD2   normal; Dbl frac;
      if(SweepCirclePoint(circle, move2, VecD2Zero, &frac, &normal))
      {
         VecD point=ball.pos+frac*move;
         if(DistPointPlane(point, edge.p[0], matrix.z)<0)point_test=0;else
         if(DistPointPlane(point, edge.p[1], matrix.z)>0)point_test=1;else
         {
            if(hit_frac  )*hit_frac  =frac;
            if(hit_normal)*hit_normal=matrix.orn().convert(normal);
            return true;
         }
      }else point_test=Closer(ball.pos, edge.p[0], edge.p[1]);
   }else point_test=0; // if 'edge' is zero length then check first point
   return SweepBallPoint(ball, move, edge.p[point_test], hit_frac, hit_normal);
}
/******************************************************************************/
Bool SweepBallBall(C Ball &ball, C Vec &move, C Ball &ball2, Flt *hit_frac, Vec *hit_normal)
{
   return SweepPointBall(ball.pos, move, Ball(ball.r+ball2.r, ball2.pos), hit_frac, hit_normal); // 'hit_normal' is OK when merging 2 balls, this was tested, it will always be in the line of ball positions when they are in contact ('ball' 'move'd by 'hit_frac')
}
Bool SweepBallBall(C BallD &ball, C VecD &move, C BallD &ball2, Dbl *hit_frac, VecD *hit_normal)
{
   return SweepPointBall(ball.pos, move, BallD(ball.r+ball2.r, ball2.pos), hit_frac, hit_normal); // 'hit_normal' is OK when merging 2 balls, this was tested, it will always be in the line of ball positions when they are in contact ('ball' 'move'd by 'hit_frac')
}
/******************************************************************************/
}
/******************************************************************************/
