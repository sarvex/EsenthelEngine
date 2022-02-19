/******************************************************************************

   1. Run this application on your computer (Windows) to configure Editor Paths
   2. Run this application on your Nintendo Switch device to precompile shaders and store them in the Editor Path
   3. Building Nintendo Switch applications will automatically use this cache

/******************************************************************************/
Str EditorPath="C:\\Esenthel\\Editor";
Edit.EditorInterface EI;
Str ShaderCachePath, ShaderCacheSize;
int ProcessedShaders, TotalShaders, Stage;
const bool CacheOnComputer=true;
/******************************************************************************/
void InitPre()
{
   INIT();
   App.flag=APP_WORK_IN_BACKGROUND|APP_SHADER_CACHE_MAX_COMPRESS|APP_IGNORE_PRECOMPILED_SHADER_CACHE; // ignore "PrecompiledShaderCache" to make sure we always use "ShaderCache", this will always save shaders in "ShaderCache" even if they're already included in the "PrecompiledShaderCache"
   if(DESKTOP)
   {
      Str msg; if(!EI.connect(msg))
      {
         Exit(S+"Can't connect to Esenthel Editor:\n"+msg+"\nConnection is required in order to configure Editor Path.");
      }
      Str editor_path=EI.editorPath(); if(!editor_path.is())Exit("Failed to get Editor Path");
      Str code;
      DYNAMIC_ASSERT(EI.getCode(UID(2369338331, 1171751015, 441055395, 4089946543), code), "Can't get code");
      Str text="Str EditorPath";
      int pos=TextPosI(code, text); DYNAMIC_ASSERT(pos>=0, S+"Can't find text:"+text);
      pos+=text.length();
      for(int end=pos; ; end++)
      {
         char c=code[end]; if(c==';')
         {
            code.remove(pos, end-pos);
            break;
         }
         if(c=='\n' || !c)Exit("Invalid code");
      }
      code.insert(pos, S+"=\""+CString(editor_path)+'"');
      DYNAMIC_ASSERT(EI.setCode(UID(2369338331, 1171751015, 441055395, 4089946543), code), "Can't set code");
      Exit("Path set successfully, you can now run this application on Nintendo Switch");
   }else
   if(SWITCH)
   {
   #if SWITCH
      ASSERT(!DEBUG); // for compiling shaders we need Release mode, because it affects their binary format (shaders compiled in Release can only be used in Release mode, and Debug only in Debug)
   #endif
      DYNAMIC_ASSERT(MountHost(), "Can't connect to the Computer, please check your USB cable connection");
      DYNAMIC_ASSERT(EditorPath.is(), "'EditorPath' was not set, please run this application on your computer (Windows) first");
      DYNAMIC_ASSERT(FileInfoSystem(EditorPath).type==FSTD_DIR, "Editor Path was not found, please run this application on your computer (Windows) to auto setup path");
      if(CacheOnComputer)
      {
         ShaderCachePath=GetPath(EditorPath).tailSlash(true)+"NintendoSwitch";
      }else
      {
         ShaderCachePath=SystemPath(SP_APP_CACHE); DYNAMIC_ASSERT(ShaderCachePath.is(), "Application Cache path unavailable, please enable Cache in the NMETA file");
      }
      ShaderCachePath.tailSlash(true)+="ShaderCache";
      D.shaderCache(ShaderCachePath);
      App.stayAwake(AWAKE_SYSTEM);
   }
#if !DESKTOP && !SWITCH
   #error Only Desktop and Switch
#endif
}
/******************************************************************************/
bool ShaderCompiler(Thread &thread)
{
   ThreadMayUseGPUData();
   REPA(ShaderFiles)
   {
      ShaderFile &shader_file=ShaderFiles.lockedData(i);
      REP(shader_file.shaders())
      {
         shader_file.shader(i); // access i-th shader, this will compile it
         ProcessedShaders++;
         if(thread.wantStop())return false;
      }
      REP(shader_file.computeShaders())
      {
         shader_file.computeShader(i); // access i-th shader, this will compile it
         ProcessedShaders++;
         if(thread.wantStop())return false;
      }
   }

   Stage++;
   Str path=EditorPath;
   path.tailSlash(true)+="Bin/Nintendo/";
   DYNAMIC_ASSERT(FCreateDirs(path), S+"Can't create path:\n"+path);
   DYNAMIC_ASSERT(PakCreate(ShaderCachePath, path+"Switch ShaderCache.pak"), "Can't create ShaderCache");

   return false;
}
Thread ShaderCompilerThread;
bool Init()
{
   if(SWITCH)
   {
    C PaksFile *shaders=Paks.find("Shader/GL");
      DYNAMIC_ASSERT(shaders, "Can't find shaders");
      REP(shaders.children_num)
      {
         Str name=Paks.file(shaders.children_offset+i).file.name;
         if(name!="Forward"
         && name!="Bone Highlight"
         && name!="World Editor"
         )ShaderFiles(name); // load all shaders to the cache
      }
      REPA(ShaderFiles)
      {
         ShaderFile &sf=ShaderFiles.lockedData(i);
         TotalShaders+=sf.shaders()+sf.computeShaders(); // calculate total shaders
      }
      ShaderCompilerThread.create(ShaderCompiler);
   }
   return true;
}
void Shut()
{
   ShaderCompilerThread.del();
}
/******************************************************************************/
bool Update()
{
   if(ShaderCompilerThread.wait(1000/30)) // wait a little to pause this thread and give more CPU power to shader compiler
   {
      Exit("Shader Cache created successfully");
   }
   //if(Touches.elms() && Touches[0].pd())ShaderCacheSize=S+"ShaderCache Size: "+TextInt(FSize(ShaderCachePath), -1, 3);
   return true;
}
/******************************************************************************/
void Draw()
{
   D.clear(BLACK);
   switch(Stage)
   {
      case 0:
      {
         D.text(0, 0.1, "Compiling Shaders");
         D.text(0, 0, TextInt(ProcessedShaders, -1, 3)+" / "+TextInt(TotalShaders, -1, 3));
         D.text(0, -0.1, ShaderCacheSize);
      }break;
      
      case 1:
      {
         D.text(0, 0, "Creating PAK");
      }break;
   }
}
/******************************************************************************/
