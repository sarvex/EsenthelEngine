/******************************************************************************

   Use 'Ball'  to handle ball shapes, Flt type
   Use 'BallD' to handle ball shapes, Dbl type

/******************************************************************************/
struct Ball // Ball Shape
{
   Flt r  ; // radius
   Vec pos; // center position

   Ball& zero(                         ) {T.r=0; T.pos.zero(); return T;}
   Ball& set (Flt r, C Vec &pos=VecZero) {T.r=r; T.pos=pos   ; return T;}

#if EE_PRIVATE
   Ball& setAnimated(C Extent &ext, C Matrix           &matrix   ); // set ball from 'ext' animated by 'matrix'
#endif
   Ball& setAnimated(C Extent &ext, C AnimatedSkeleton &anim_skel); // set approximate ball encapsulating 'ext' animated by 'anim_skel' skeleton

   Ball& operator+=(C Vec     &v) {pos+=v; return T;}
   Ball& operator-=(C Vec     &v) {pos-=v; return T;}
   Ball& operator*=(  Flt      f);
   Ball& operator/=(  Flt      f);
   Ball& operator*=(C Matrix3 &m);
   Ball& operator/=(C Matrix3 &m);
   Ball& operator*=(C Matrix  &m);
   Ball& operator/=(C Matrix  &m);

   friend Ball operator+ (C Ball &ball, C Vec     &v) {return Ball(ball)+=v;}
   friend Ball operator- (C Ball &ball, C Vec     &v) {return Ball(ball)-=v;}
   friend Ball operator* (C Ball &ball,   Flt      f) {return Ball(ball)*=f;}
   friend Ball operator/ (C Ball &ball,   Flt      f) {return Ball(ball)/=f;}
   friend Ball operator* (C Ball &ball, C Matrix3 &m) {return Ball(ball)*=m;}
   friend Ball operator/ (C Ball &ball, C Matrix3 &m) {return Ball(ball)/=m;}
   friend Ball operator* (C Ball &ball, C Matrix  &m) {return Ball(ball)*=m;}
   friend Ball operator/ (C Ball &ball, C Matrix  &m) {return Ball(ball)/=m;}

   // get
   Flt area  ()C {return (PI*4  )*Sqr (r);} // get surface area
   Flt volume()C {return (PI*4/3)*Cube(r);} // get volume

   Vec nearest(C Vec &normal)C; // get nearest point on ball towards normal

   Str asText()C {return S+"Radius: "+r+", Pos: "+pos;} // get text description

   // operations
   Ball& extend(Flt e) {r+=e; return T;} // extend
   Bool  from  (C Vec *point, Int points                  ); // set ball from an array of points                        , false on fail (if there are no points)
   Bool  from  (C Vec *point, Int points, C Matrix &matrix); // set ball from an array of points transformed by 'matrix', false on fail (if there are no points)

   // draw
#if EE_PRIVATE
   void drawVI   (                                                   Bool fill=false, C VecI2 &resolution=VecI2(-1))C; // draw, this relies on active object matrix which can be set using 'SetMatrix' function
#endif
   void draw     (C Color &color=WHITE,                              Bool fill=false, C VecI2 &resolution=VecI2(-1))C; // draw                  , this relies on active object matrix which can be set using 'SetMatrix' function
   void drawAngle(C Color &color      , Flt from, Flt to, C Vec &up, Bool fill=false, C VecI2 &resolution=VecI2(-1))C; // draw with angle ranges, this relies on active object matrix which can be set using 'SetMatrix' function
   void draw2    (C Color &color=WHITE,                              Bool fill=false,   Int    resolution=      -1 )C; // draw box based        , this relies on active object matrix which can be set using 'SetMatrix' function

              Ball() {}
              Ball(Flt r, C Vec &pos=VecZero) {set(r, pos);}
   CONVERSION Ball(C Box      &box    );
   CONVERSION Ball(C OBox     &obox   );
   CONVERSION Ball(C Extent   &ext    );
   CONVERSION Ball(C Capsule  &capsule);
   CONVERSION Ball(C Shape    &shape  );
   CONVERSION Ball(C MeshBase &mesh   );
};
/******************************************************************************/
struct BallM // Ball Shape (mixed precision)
{
   Flt  r  ; // radius
   VecD pos; // center position

   BallM& set(Flt r, C VecD &pos) {T.r=r; T.pos=pos; return T;}

   BallM& extend(Flt e) {r+=e; return T;} // extend

              BallM() {}
              BallM(Flt r, C VecD &pos=VecDZero) {set(r, pos);}
   CONVERSION BallM(C CapsuleM &capsule);
};
/******************************************************************************/
struct BallD // Ball Shape (double precision)
{
   Dbl  r  ; // radius
   VecD pos; // center position

   BallD& set(Dbl r, C VecD &pos) {T.r=r; T.pos=pos; return T;}

   BallD& extend(Dbl e) {r+=e; return T;} // extend

   BallD() {}
   BallD(Dbl r, C VecD &pos=VecDZero) {set(r, pos);}
};
/******************************************************************************/
struct SphereArea : VecI2
{
   DIR_ENUM side;

   SphereArea& operator=(C VecI2 &xy) {super::operator=(xy); return T;}
};
#if EE_PRIVATE
struct SphereAreaDist : SphereArea
{
   Flt dist;
};
inline Int Compare (C SphereAreaDist &a, C SphereAreaDist &b) {return Compare(a.dist, b.dist);}
inline Int CompareR(C SphereAreaDist &a, C SphereAreaDist &b) {return Compare(b.dist, a.dist);}
#endif
struct SphereConvert
{
   Int       res;
   Flt       pos_to_cell_mul, pos_to_cell_add, cell_to_pos_mul/*, cell_to_pos_add=-PI_4*/;
   Mems<Flt> tans;

   void init(Int res);

   Flt   posToCell    (  Flt   pos)C {return Atan(pos)*pos_to_cell_mul+pos_to_cell_add;}
   Int   posToCellI   (  Flt   pos)C {return     Floor(posToCell(pos));}
   Int   posToCellIMid(  Flt   pos)C {return Mid(Trunc(posToCell(pos)), 0, res-1);}
   Vec2  posToCell    (C Vec2 &pos)C {return Vec2 (posToCell    (pos.x), posToCell    (pos.y));}
   VecI2 posToCellI   (C Vec2 &pos)C {return VecI2(posToCellI   (pos.x), posToCellI   (pos.y));}
   VecI2 posToCellIMid(C Vec2 &pos)C {return VecI2(posToCellIMid(pos.x), posToCellIMid(pos.y));}

   Flt   cellToPos(  Flt    cell)C {return Tan (cell*cell_to_pos_mul+(-PI_4));}
   Flt   cellToPos(  Int    cell)C {return Tan (cell*cell_to_pos_mul+(-PI_4));}
   Flt  _cellToPos(  Int    cell)C {return tans[cell]                                  ;} // !! 'cell' MUST BE IN RANGE "0..res" !!
   Vec2  cellToPos(C Vec2  &cell)C {return Vec2( cellToPos(cell.x),  cellToPos(cell.y));}
   Vec2  cellToPos(C VecI2 &cell)C {return Vec2( cellToPos(cell.x),  cellToPos(cell.y));}
   Vec2 _cellToPos(C VecI2 &cell)C {return Vec2(_cellToPos(cell.x), _cellToPos(cell.y));} // !! 'cell' MUST BE IN RANGE "0..res" !!

   DIR_ENUM dirToSphereTerrainPixel      (C Vec &dir, Vec2 &xy            )C; // convert vector direction (doesn't need to be normalized) to cube face and spherical terrain coordinates, 'xy'=image pixel coordinates (0..res-1)
   Vec2     dirToSphereTerrainPixel      (C Vec &dir,   DIR_ENUM cube_face)C; // convert vector direction (doesn't need to be normalized) to               spherical terrain coordinates, 'xy'=image pixel coordinates (0..res-1), 'cube_face'=cube face that 'dir' belongs to
   Vec      sphereTerrainPixelToDir      (Flt x, Flt y, DIR_ENUM cube_face)C; // convert spherical terrain coordinates to vector direction, 'x,y'=terrain pixel coordinates (0..res-1)  , 'cube_face'=terrain cube face, returned vector is not normalized, however it's on a cube with radius=1 ("Abs(dir).max()=1")
   Vec     _sphereTerrainPixelCenterToDir(Int x, Int y, DIR_ENUM cube_face)C; // convert spherical terrain coordinates to vector direction, 'x,y'=terrain pixel coordinates (0..res-1)  , 'cube_face'=terrain cube face, returned vector is not normalized, however it's on a cube with radius=1 ("Abs(dir).max()=1"), !! 'x' 'y' MUST BE IN RANGE "0..res-1" !! THIS IS FAST APPROXIMATION !!

   void getIntersectingSphereAreas(MemPtr<SphereArea> area_pos, C Ball &ball, Flt min_radius)C;

#if EE_PRIVATE
   void draw()C;
   void drawCell(C Color &color, C VecI2 &cell)C;
   void drawCell(C Color &color, C SphereArea &area, Flt radius)C;
#endif
};
struct SphereConvertEx : SphereConvert
{
   Mems<Flt> tan_mids;

   Flt  _cellCenterToPos(  Int    cell)C {return tan_mids[cell]                              ;} // !! 'cell' MUST BE IN RANGE "0..res-1" !!
   Vec2 _cellCenterToPos(C VecI2 &cell)C {return Vec2(_cellToPos(cell.x), _cellToPos(cell.y));} // !! 'cell' MUST BE IN RANGE "0..res-1" !!

   void init(Int res);

   Vec _sphereTerrainPixelCenterToDir(Int x, Int y, DIR_ENUM cube_face)C; // convert spherical terrain coordinates to vector direction, 'x,y'=terrain pixel coordinates (0..res-1), 'cube_face'=terrain cube face, returned vector is not normalized, however it's on a cube with radius=1 ("Abs(dir).max()=1"), !! 'x' 'y' MUST BE IN RANGE "0..res-1" !!

   void sort(MemPtr<SphereArea> areas, C Vec &pos, Bool reverse=false)C; // sort 'areas' based on distance to 'pos', 'reverse'=if reverse sort order
};

DIR_ENUM DirToCubeFace          (C Vec &dir                               ); // convert vector direction (doesn't need to be normalized) to cube face
DIR_ENUM DirToCubeFacePixel     (C Vec &dir, Int res, Vec2 &xy            ); // convert vector direction (doesn't need to be normalized) to cube face and image             coordinates, 'res'=cube image resolution, 'xy'=image pixel coordinates (0..res-1)
DIR_ENUM DirToSphereTerrainPixel(C Vec &dir, Int res, Vec2 &xy            ); // convert vector direction (doesn't need to be normalized) to cube face and spherical terrain coordinates, 'res'=terrain    resolution, 'xy'=image pixel coordinates (0..res-1)
Vec           CubeFacePixelToDir(Flt x, Flt y, Int res, DIR_ENUM cube_face); // convert        cube image coordinates to vector direction, 'x,y'=image   pixel coordinates (0..res-1)  , 'res'=cube image resolution, 'cube_face'=image   cube face, returned vector is not normalized, however it's on a cube with radius=1 ("Abs(dir).max()=1")
Vec      SphereTerrainPixelToDir(Flt x, Flt y, Int res, DIR_ENUM cube_face); // convert spherical terrain coordinates to vector direction, 'x,y'=terrain pixel coordinates (0..res-1)  , 'res'=terrain    resolution, 'cube_face'=terrain cube face, returned vector is not normalized, however it's on a cube with radius=1 ("Abs(dir).max()=1")

void PosToSphereTerrainPos(DIR_ENUM dir, Vec2 &dest, C Vec  &src          ); // convert world space position 'src' to 'dest' spherical terrain position where XY=plane position
void PosToSphereTerrainPos(DIR_ENUM dir, Vec  &dest, C Vec  &src          ); // convert world space position 'src' to 'dest' spherical terrain position where XY=plane position, Z=height
void PosToSphereTerrainPos(DIR_ENUM dir, VecD *dest, C VecD *src, Int elms); // convert world space position 'src' to 'dest' spherical terrain position where XY=plane position, Z=height
/******************************************************************************/
Ball Avg(C Ball &a, C Ball &b);

// distance
       Flt Dist         (C Vec    &point, C Ball   &ball                ); // distance between point    and a ball
       Dbl Dist         (C VecD   &point, C BallM  &ball                ); // distance between point    and a ball
       Flt Dist         (C Edge   &edge , C Ball   &ball                ); // distance between edge     and a ball
       Flt Dist         (C TriN   &tri  , C Ball   &ball                ); // distance between triangle and a ball
       Flt Dist         (C Box    &box  , C Ball   &ball                ); // distance between box      and a ball
       Flt Dist         (C OBox   &obox , C Ball   &ball                ); // distance between box      and a ball
       Flt Dist         (C Extent &ext  , C Ball   &ball                ); // distance between extent   and a ball
       Flt Dist         (C Ball   &a    , C Ball   &b                   ); // distance between ball     and a ball
       Flt DistBallPlane(C Ball   &ball , C Vec    &plane, C Vec &normal); // distance between ball     and a plane
       Dbl DistBallPlane(C Ball   &ball , C VecD   &plane, C Vec &normal); // distance between ball     and a plane
       Dbl DistBallPlane(C BallM  &ball , C VecD   &plane, C Vec &normal); // distance between ball     and a plane
inline Flt Dist         (C Ball   &ball , C Plane  &plane               ) {return DistBallPlane(ball, plane.pos, plane.normal);} // distance between ball and a plane
inline Dbl Dist         (C Ball   &ball , C PlaneM &plane               ) {return DistBallPlane(ball, plane.pos, plane.normal);} // distance between ball and a plane
inline Dbl Dist         (C BallM  &ball , C PlaneM &plane               ) {return DistBallPlane(ball, plane.pos, plane.normal);} // distance between ball and a plane

// cuts
Bool Cuts(C Vec    &point, C Ball  &ball); // if point    cuts a ball
Bool Cuts(C VecD   &point, C Ball  &ball); // if point    cuts a ball
Bool Cuts(C VecD   &point, C BallM &ball); // if point    cuts a ball
Bool Cuts(C Edge   &edge , C Ball  &ball); // if edge     cuts a ball
Bool Cuts(C Tri    &tri  , C Ball  &ball); // if triangle cuts a ball
Bool Cuts(C TriN   &tri  , C Ball  &ball); // if triangle cuts a ball
Bool Cuts(C Box    &box  , C Ball  &ball); // if box      cuts a ball
Bool Cuts(C OBox   &obox , C Ball  &ball); // if box      cuts a ball
Bool Cuts(C Extent &ext  , C Ball  &ball); // if extent   cuts a ball
Bool Cuts(C Extent &ext  , C BallM &ball); // if extent   cuts a ball
Bool Cuts(C Ball   &a    , C Ball  &b   ); // if ball     cuts a ball
Bool Cuts(C Ball   &a    , C BallM &b   ); // if ball     cuts a ball
Bool Cuts(C BallM  &a    , C BallM &b   ); // if ball     cuts a ball

Int CutsLineBall(C Vec  &line_pos  , C Vec  &line_dir, C Ball  &ball, Vec  *contact_a=null, Vec  *contact_b=null); // if infinite straight line cuts a ball, return number of contacts, 'line_dir'=straight line direction (must be normalized)
Int CutsLineBall(C VecD &line_pos  , C VecD &line_dir, C BallM &ball, VecD *contact_a=null, VecD *contact_b=null); // if infinite straight line cuts a ball, return number of contacts, 'line_dir'=straight line direction (must be normalized)
Int CutsEdgeBall(C Vec  &edge_start, C Vec  &edge_end, C Ball  &ball, Vec  *contact_a=null, Vec  *contact_b=null); // if edge                   cuts a ball, return number of contacts

Bool Inside(C Box    &a, C Ball &b); // if 'a' is fully inside 'b'
Bool Inside(C Extent &a, C Ball &b); // if 'a' is fully inside 'b'

// sweep
Bool SweepPointBall(C Vec  &point, C Vec  &move, C Ball  &ball , Flt *hit_frac=null, Vec  *hit_normal=null); // if moving point cuts through a static ball
Bool SweepPointBall(C VecD &point, C VecD &move, C BallD &ball , Dbl *hit_frac=null, VecD *hit_normal=null); // if moving point cuts through a static ball

Bool SweepBallPoint(C Ball  &ball, C Vec  &move, C Vec   &point, Flt *hit_frac=null, Vec  *hit_normal=null); // if moving ball cuts through a static point
Bool SweepBallPoint(C BallD &ball, C VecD &move, C VecD  &point, Dbl *hit_frac=null, VecD *hit_normal=null); // if moving ball cuts through a static point
Bool SweepBallEdge (C Ball  &ball, C Vec  &move, C Edge  &edge , Flt *hit_frac=null, Vec  *hit_normal=null); // if moving ball cuts through a static edge
Bool SweepBallEdge (C BallD &ball, C VecD &move, C EdgeD &edge , Dbl *hit_frac=null, VecD *hit_normal=null); // if moving ball cuts through a static edge
Bool SweepBallBall (C Ball  &ball, C Vec  &move, C Ball  &ball2, Flt *hit_frac=null, Vec  *hit_normal=null); // if moving ball cuts through a static ball
Bool SweepBallBall (C BallD &ball, C VecD &move, C BallD &ball2, Dbl *hit_frac=null, VecD *hit_normal=null); // if moving ball cuts through a static ball

#if EE_PRIVATE
Bool ClipZ(Ball &ball, Flt min_z); // clip ball to retain information only above 'min_z'
#endif
/******************************************************************************/
