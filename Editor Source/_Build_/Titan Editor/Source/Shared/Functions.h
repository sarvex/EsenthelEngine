﻿/******************************************************************************/
extern cchar8 *SizeSuffix[];
extern cchar8 *FormatSuffixes[]
;
extern int FormatSuffixElms;
extern const Color LitSelColor, SelColor, LitColor, DefColor, InvalidColor;
/******************************************************************************/
bool SafeDel(C Str &name, ReadWriteSync &rws);
bool SafeOverwrite(File &src, C Str &dest, ReadWriteSync &rws);
bool SafeOverwriteChunk(File &src, C Str &dest, ReadWriteSync &rws);
bool SafeCopy(C Str &src, C Str &dest);
void RemoveChunk(C Str &file, C Str &chunk, ReadWriteSync &rws);
Str FileSize(long size, char dot=',');
Str FileSizeKB(long size);
void SavedImage        (C Str &name);
void SavedImageAtlas   (C Str &name);
void SavedEditSkel     (C Str &name);
void SavedSkel         (C Str &name);
void SavedAnim         (C Str &name);
void SavedMesh         (C Str &name);
void SavedEditMtrl     (C Str &name);
void SavedEditWaterMtrl(C Str &name);
void SavedEditPhysMtrl (C Str &name);
void SavedMtrl         (C Str &name);
void SavedWaterMtrl    (C Str &name);
void SavedPhysMtrl     (C Str &name);
void SavedEditPhys     (C Str &name);
void SavedPhys         (C Str &name);
void SavedEnum         (C Str &name);
void SavedEditEnum     (C Str &name);
void SavedEditObjPar   (C Str &name);
void SavedGameObjPar   (C Str &name);
void SavedGameWayp     (C Str &name);
void SavedFont         (C Str &name);
void SavedTextStyle    (C Str &name);
void SavedPanelImage   (C Str &name);
void SavedPanel        (C Str &name);
void SavedGuiSkin      (C Str &name);
void SavedGui          (C Str &name);
void SavedEnv          (C Str &name);
void Saved(C Image           &img , C Str &name);
void Saved(C ImageAtlas      &img , C Str &name);
void Saved(C IconSettings    &icon, C Str &name);
void Saved(C EditSkeleton    &skel, C Str &name);
void Saved(C     Skeleton    &skel, C Str &name);
void Saved(C Animation       &anim, C Str &name);
void Saved(C Mesh            &mesh, C Str &name);
void Saved(C EditMaterial    &mtrl, C Str &name);
void Saved(C EditWaterMtrl   &mtrl, C Str &name);
void Saved(C EditPhysMtrl    &mtrl, C Str &name);
void Saved(C Material        &mtrl, C Str &name);
void Saved(C WaterMtrl       &mtrl, C Str &name);
void Saved(C PhysMtrl        &mtrl, C Str &name);
void Saved(C PhysBody        &phys, C Str &name);
void Saved(C Enum            &enm , C Str &name);
void Saved(C EditEnums       &enms, C Str &name);
void Saved(C EditObject      &obj , C Str &name);
void Saved(C     Object      &obj , C Str &name);
void Saved(C EditWaypoint    &wp  , C Str &name);
void Saved(C Game::Waypoint   &wp  , C Str &name);
void Saved(C EditFont        &font, C Str &name);
void Saved(C Font            &font, C Str &name);
void Saved(C EditTextStyle   &ts  , C Str &name);
void Saved(C EditPanelImage  &pi  , C Str &name);
void Saved(C PanelImage      &pi  , C Str &name);
void Saved(C TextStyle       &ts  , C Str &name);
void Saved(C EditPanel       &panl, C Str &name);
void Saved(C Panel           &panl, C Str &name);
void Saved(C EditGuiSkin     &skin, C Str &name);
void Saved(C     GuiSkin     &skin, C Str &name);
void Saved(C GuiObjs         &gui , C Str &name);
void Saved(C Lake            &lake, C Str &name);
void Saved(C River           &rivr, C Str &name);
void Saved(C EditEnv         &env , C Str &name);
void Saved(C Environment     &env , C Str &name);
void Saved(C Game::WorldSettings &s, C Str &name);
bool Save (C Image           &img , C Str &name                        );
void Save (C ImageAtlas      &img , C Str &name                        );
void Save (C IconSettings    &icon, C Str &name                        );
void Save (C EditSkeleton    &skel, C Str &name                        );
void Save (C     Skeleton    &skel, C Str &name                        );
void Save (C Animation       &anim, C Str &name                        );
void Save (C Mesh            &mesh, C Str &name, C Str &resource_path=S);
void Save (C EditMaterial    &mtrl, C Str &name                        );
void Save (C EditWaterMtrl   &mtrl, C Str &name                        );
void Save (C EditPhysMtrl    &mtrl, C Str &name                        );
void Save (C Material        &mtrl, C Str &name, C Str &resource_path=S);
void Save (C WaterMtrl       &mtrl, C Str &name, C Str &resource_path=S);
void Save (C PhysMtrl        &mtrl, C Str &name                        );
void Save (C PhysBody        &phys, C Str &name, C Str &resource_path=S);
void Save (C Enum            &enm , C Str &name                        );
void Save (C EditEnums       &enms, C Str &name                        );
void Save (C EditObject      &obj , C Str &name, C Str &resource_path=S);
void Save (C     Object      &obj , C Str &name, C Str &resource_path=S);
void Save (C EditWaypoint    &wp  , C Str &name                        );
void Save (C Game::Waypoint   &wp  , C Str &name                        );
void Save (C EditFont        &font, C Str &name                        );
void Save (C Font            &font, C Str &name                        );
void Save (C EditTextStyle   &ts  , C Str &name                        );
void Save (C TextStyle       &ts  , C Str &name, C Str &resource_path=S);
void Save (C EditPanelImage  &pi  , C Str &name                        );
void Save (C PanelImage      &pi  , C Str &name                        );
void Save (C EditPanel       &panl, C Str &name                        );
void Save (C Panel           &panl, C Str &name, C Str &resource_path=S);
void Save (C EditGuiSkin     &skin, C Str &name                        );
void Save (C     GuiSkin     &skin, C Str &name, C Str &resource_path=S);
void Save (C GuiObjs         &gui , C Str &name, C Str &resource_path=S);
void Save (C Lake            &lake, C Str &name                        );
void Save (C River           &rivr, C Str &name                        );
void Save (C EditEnv         &env , C Str &name                        );
void Save (C Environment     &env , C Str &name, C Str &resource_path=S);
void Save (C Game::WorldSettings &s, C Str &name, C Str &resource_path=S);
bool Load (Mesh       &mesh, C Str &name, C Str &resource_path=S);
bool Load (Material   &mtrl, C Str &name, C Str &resource_path=S);
bool Load (WaterMtrl  &mtrl, C Str &name, C Str &resource_path=S);
bool Load (EditObject &obj , C Str &name, C Str &resource_path=S);
bool SaveCode(C Str &code, C Str &name);
Edit::ERROR_TYPE LoadCode(Str &code, C Str &name);
void SavedBase(ELM_TYPE type, C Str &path);
void SavedCode(C Str &path);
void SavedEdit(ELM_TYPE type, C Str &path);
void SavedGame(ELM_TYPE type, C Str &path);
void MAX1(TimeStamp &time, C TimeStamp &src_time);
bool Sync(TimeStamp &time, C TimeStamp &src_time);
bool Undo(TimeStamp &time, C TimeStamp &src_time);
template<typename TYPE> bool Sync(TimeStamp &time, C TimeStamp &src_time, TYPE &data, C TYPE &src_data);
template<typename TYPE> bool UndoByTime(TimeStamp &time, C TimeStamp &src_time, TYPE &data, C TYPE &src_data);
template<typename TYPE> bool SyncByValue(TimeStamp &time, C TimeStamp &src_time, TYPE &data, C TYPE &src_data);
template<typename TYPE> bool SyncByValueEqual(TimeStamp &time, C TimeStamp &src_time, TYPE &data, C TYPE &src_data);
template<typename TYPE> bool UndoByValue(TimeStamp &time, C TimeStamp &src_time, TYPE &data, C TYPE &src_data);
template<typename TYPE> bool Undo(TimeStamp &time, C TimeStamp &src_time, TYPE &data, C TYPE &src_data);
void SetUndo(C Edit::_Undo &undos, Button &undo, Button &redo);
DIR_ENUM GetCubeDir(int face);
Str GetCubeFile(C Str &files, int face);
Str SetCubeFile(Str files, int face, C Str &file);
bool HasAlpha(C Image &image);
bool HasColor(C Image &image);
bool NeedFullAlpha(Image &image, int dest_type);
bool SetFullAlpha(Image &image, IMAGE_TYPE dest_type);
void ImageProps(C Image &image, UID *hash, IMAGE_TYPE *best_type=null, uint flags=SRGB, Edit::Material::TEX_QUALITY quality=Edit::Material::MEDIUM);
void LoadTexture(C Project &proj, C UID &tex_id, Image &image);
void ExtractBaseTextures(C Project &proj, C UID &base_0, C UID &base_1, C UID &base_2, Image *color, Image *alpha, Image *bump, Image *normal, Image *smooth, Image *metal, Image *glow);
void ExtractWaterBaseTextures(C Project &proj, C UID &base_0, C UID &base_1, C UID &base_2, Image *color, Image *alpha, Image *bump, Image *normal, Image *smooth, Image *reflect, Image *glow);
void ExtractDetailTextures(C Project &proj, C UID &detail_tex, Image *color, Image *bump, Image *normal, Image *smooth);
UID MergedBaseTexturesID(C UID &base_0, C UID &base_1, C UID &base_2);
VecI ImageSize(C VecI &src, C VecI2 &custom, bool pow2);
VecI2 GetSize(C Str &name, C Str &value, C VecI &src);
int GetFilterI(C Str &name);
FILTER_TYPE GetFilter(C Str &name);
bool GetClampWrap(C Str &name, bool default_clamp);
bool GetAlphaWeight(C Str &name);
bool GetKeepEdges  (C Str &name);
bool EditToGameImage(Image &edit, Image &game, bool pow2, bool srgb, bool alpha_lum, ElmImage::TYPE type, int mode, int mip_maps, bool has_color, bool has_alpha, bool ignore_alpha, bool env, C VecI2 &custom_size=0, C int *force_type=null);
bool EditToGameImage(Image &edit, Image &game, C ElmImage &data, C int *force_type=null);
void DrawPanelImage(C PanelImage &pi, C Rect &rect, bool draw_lines=false);
bool UpdateMtrlBase1Tex(C Image &src, Image &dest);
bool ImportImage(Image &image, C Str &name, int type=-1, int mode=-1, int mip_maps=-1, bool decompress=false);
char IndexChannel(int i);
int ChannelIndex(char c);
bool ChannelMonoTransform(C Str &value);
bool PartialTransform   (C TextParam &p   );
bool  LinearTransform   (C Str       &name);
bool  ResizeTransformAny(C Str       &name);
bool  ResizeTransform   (C Str       &name);
bool    MonoTransform   (C TextParam &p   );
bool NonMonoTransform   (C TextParam &p   );
bool HighPrecTransform(C Str &name);
bool SizeDependentTransform(C TextParam &p);
bool ForcesMono(C Str &file);
Str BumpFromColTransform(C Str &color_map, int blur);
bool ExtractResize(MemPtr<FileParams> files, TextParam &resize);
bool ExtractLinearTransform(MemPtr<FileParams> files, Vec &mul, Vec &add);
void AdjustImage(Image &image, bool rgb, bool alpha, bool high_prec);
void ContrastLum(Image &image, flt contrast, flt avg_lum, C BoxI &box);
void AvgContrastLum(Image &image, flt contrast, dbl avg_lum, C BoxI &box);
void ContrastHue(Image &image, flt contrast, C Vec &avg_col, C BoxI &box, bool photo=false);
void AddHue(Image &image, flt hue, C BoxI &box, bool photo=false);
void ContrastSat(Image &image, flt contrast, flt avg_sat, C BoxI &box, bool photo=false);
void MulRGBH(Image &image, flt red, flt yellow, flt green, flt cyan, flt blue, flt purple, C BoxI &box);
void MulRGBHS(Image &image, flt red, flt yellow, flt green, flt cyan, flt blue, flt purple, C BoxI &box);
void GammaSat(Image &image, flt gamma, C BoxI &box, bool photo=false);
void MulAddSat(Image &image, flt mul, flt add, C BoxI &box, bool photo=false);
void MulSatH(Image &image, flt red, flt yellow, flt green, flt cyan, flt blue, flt purple, bool sat, bool photo, C BoxI &box);
flt HueDelta(flt a, flt b);
Vec2  LerpToMad(flt from, flt to);
Vec2 ILerpToMad(flt from, flt to);
flt   FloatSelf(flt x);
flt   PowMax   (flt x, flt y);
void Crop(Image &image, int x, int y, int w, int h, C Color &background, bool hp=true);
void TransformImage(Image &image, TextParam param, bool clamp, C Color &background=TRANSPARENT);
void TransformImage(Image &image, C MemPtr<TextParam> &params, bool clamp, C Color &background=TRANSPARENT);
bool LoadImage(C Project *proj, Image &image, TextParam *image_resize, C FileParams &fp, bool srgb, bool clamp=false, C Color &background=TRANSPARENT, C Image *color=null, C TextParam *color_resize=null, C Image *smooth=null, C TextParam *smooth_resize=null, C Image *bump=null, C TextParam *bump_resize=null);
bool HighPrecTransform(APPLY_MODE mode);
bool LoadImages(C Project *proj, Image &image, TextParam *image_resize, C Str &src, bool srgb=true, bool clamp=false, C Color &background=TRANSPARENT, C Image *color=null, C TextParam *color_resize=null, C Image *smooth=null, C TextParam *smooth_resize=null, C Image *bump=null, C TextParam *bump_resize=null);
bool ValidChar(char c);
bool ValidText(C Str &text, int min=1, int max=-1);
bool ValidFileName(C Str &name);
bool ValidFileNameForUpload(C Str &name, int max=128);
bool ValidPass(C Str &pass);
bool ValidEnum(C Str &name);
bool ValidSupport(C Str &support);
bool ValidVideo(C Str &video);
Str YouTubeEmbedToFull(C Str &video);
Str YouTubeFullToEmbed(C Str &video);
UID PassToMD5(C Str &pass);
Str NameToEnum(C Str &name);
Str TimeAgo(C DateTime &date);
char CountS(int n);
Str  Plural(Str name);
int Occurrences(C Str &s, char c);
inline VecI2 TextVecI2Ex(cchar *t);
inline Vec2  TextVec2Ex (cchar *t);
inline Vec   TextVecEx  (cchar *t);
Str TextVecI2Ex(C VecI2 &v);
Str TextVec2Ex(C Vec2 &v);
Str TextVecEx(C Vec &v, int precision=-3);
bool TextVecVecEx(cchar *t, Vec &a, Vec &b);
Str TextVecVecEx(C Vec &a, C Vec &b, int precision=-3);
Str RelativePath  (C Str &path);
Str EditToGamePath(  Str  path);
cchar8* FormatSuffixSimple();
cchar8* FormatSuffix(IMAGE_TYPE type);
Str8 ImageDownSizeSuffix(int size);
TextParam* FindTransform(MemPtr<FileParams> files, C Str &name);
void DelTransform(MemPtr<FileParams> files, C Str &name);
void SetTransform(MemPtr<FileParams> files, C Str &name, C Str &value=S);
void AddTransform(MemPtr<FileParams> files, C Str &name, C Str &value=S);
void SetResizeTransform(MemPtr<FileParams> files, C Str &name, C Str &value=S);
void SetTransform(Str &file, C Str &name, C Str &value=S);
void AddTransform(Str &file, C Str &name, C Str &value=S);
SOUND_CODEC TextSoundCodec(C Str &t);
int       VisibleVtxs      (C MeshLod &mesh);
int       VisibleTris      (C MeshLod &mesh);
int       VisibleTrisTotal (C MeshLod &mesh);
int       VisibleQuads     (C MeshLod &mesh);
int       VisibleSize      (C MeshLod &mesh);
MESH_FLAG VisibleFlag      (C MeshLod &mesh);
MESH_FLAG VisibleFlag      (C Mesh    &mesh);
flt       VisibleLodQuality(C Mesh    &mesh, int lod_index);
void KeepParams(C Mesh &src, Mesh &dest);
void RemovePartsAndLods(Mesh &mesh);
void EditToGameMesh(C Mesh &edit, Mesh &game, Skeleton *skel, Enum *draw_group, C Matrix *matrix);
bool HasMaterial(C MeshPart &part, C MaterialPtr &material);
void CleanMesh(Mesh &mesh);
bool FixVtxNrm(MeshBase &base);
void FixMesh(Mesh &mesh);
bool SamePartInAllLods(C Mesh &mesh, int part);
void SetDrawGroup(Mesh &mesh, MeshLod &lod, int part, int group, Enum *draw_group_enum);
bool DisableLQLODs(Mesh &mesh);
Str BoneNeutralName(C Str &name);
bool OverrideMeshSkel(C Mesh *mesh, C Skeleton *skel);
bool OverridePhys    (C PhysBody *body              );
int CompareObj(C Game::Area::Data::AreaObj &a, C Game::Area::Data::AreaObj &b);
void SetRootMoveRot(Animation &anim, C Vec *root_move, C Vec *root_rot);
bool DelEndKeys(Animation &anim);
inline bool NegativeSB(flt  x);
inline void      CHSSB(flt &x);
int UniquePairs(int elms);
bool Distance2D(C Vec2 &point, C Edge &edge, flt &dist, flt min_length=0.025f);
int MatrixAxis(C Vec2 &screen_pos, C Matrix &matrix);
void MatrixAxis(Edit::Viewport4 &v4, C Matrix &matrix, int &axis, Vec *axis_vec=null);
int GetNearestAxis(C Matrix &matrix, C Vec &dir);
bool UniformScale(C Matrix3 &m);
bool UniformScale(C Vec     &s);
flt CamMoveScale(bool perspective=true    );
Vec2   MoveScale(Edit::Viewport4::View &view);
flt AlignDirToCamEx(C Vec &dir, C Vec2 &delta, C Vec &cam_x=ActiveCam.matrix.x, C Vec &cam_y=ActiveCam.matrix.y);
Vec AlignDirToCam  (C Vec &dir, C Vec2 &delta, C Vec &cam_x=ActiveCam.matrix.x, C Vec &cam_y=ActiveCam.matrix.y);
flt MatrixLength(C Vec &x, C Vec &y, C Vec &z, C Vec &dir);
void Include(RectI &rect,           C VecI2 &x);
void Include(RectI &rect,           C RectI &x);
void Include(Rect  &rect,           C Rect  &x);
void Include(Rect  &rect, bool &is, C Vec2  &x);
void Include(Rect  &rect, bool &is, C Rect  &x);
void Include(Box   &box , bool &is, C Vec   &v);
void Include(Box   &box , bool &is, C Box   &b);
void DrawMatrix(C Matrix &matrix, int bold_axis);
void Hide(GuiObj &go);
Rect GetRect(C Rect &rect, Memt<Rect> &rects);
void Include(MemPtr<UID> ids, C UID &id);
bool Same(C Memc<UID> &a, C Memc<UID> &b);
bool Same(C Memc<ObjData> &a, C Memc<ObjData> &b);
void GetNewer(C Memc<ObjData> &a, C Memc<ObjData> &b, Memc<UID> &newer);
bool EmbedObject(C Box &obj_box, C VecI2 &area_xy, flt area_size);
bool SameOS(OS_VER a, OS_VER b);
UID GetFileNameID(Str name);
UID AsID(C Elm *elm);
void SetPath(WindowIO &win_io, C Str &path, bool clear=false);
bool ParamTypeID        (PARAM_TYPE type           );
bool ParamTypeCompatible(PARAM_TYPE a, PARAM_TYPE b);
bool ParamCompatible    (C Param   &a, C Param   &b);
UNIT_TYPE UnitType(C Str &s);
UNIT_TYPE GetUnitType(C Str &s);
flt ConvertUnitType(flt value, flt full, UNIT_TYPE unit);
Color BackgroundColor();
Color BackgroundColorLight();
Color GuiListTextColor();
Color GetLitSelCol(bool lit, bool sel, C Color &none=DefColor);
bool ErrorCopy(C Str &src, C Str &dest);
bool ErrorRecycle(C Str &name);
bool ErrorCreateDir(C Str &name);
bool RecycleLoud  (C Str &name            );
bool CreateDirLoud(C Str &name            );
bool SafeCopyLoud (C Str &src, C Str &dest);
flt VorbisBitRateToQuality(int rel_bit_rate);
int DecIntV(File &f);
void GetStr2(File &f, Str &s);
Str  GetStr2(File &f);
void PutStr(File &f, C Str &s);
Str GetStr(File &f);
void GetStr(File &f, Str &s);
template<typename TYPE      > bool Save(File &f, C Memc<TYPE> &m              );
template<typename TYPE, typename USER> bool Save(File &f, C Memc<TYPE> &m, C USER &user);
template<typename TYPE      > bool Save(File &f, C Memx<TYPE> &m              );
template<typename TYPE      > bool Load(File &f,   Memc<TYPE> &m              );
template<typename TYPE, typename USER> bool Load(File &f,   Memc<TYPE> &m, C USER &user);
template<typename TYPE      > bool Load(File &f,   Memx<TYPE> &m              );
Mems<FileParams> _DecodeFileParams(C Str &str);
/******************************************************************************/