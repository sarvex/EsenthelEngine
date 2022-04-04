﻿/******************************************************************************/
#include "stdafx.h"
/******************************************************************************/
// THIS IS AN AUTOMATICALLY GENERATED FILE
// THIS FILE WAS GENERATED BY TITAN EDITOR, ACCORDING TO PROJECT SETTINGS
/******************************************************************************/
// CONSTANTS
/******************************************************************************/
#define    STEAM   0 // if Application properties have Steam enabled
#define    OPEN_VR 0 // if Application properties have OpenVR enabled
const bool PUBLISH          =true; // this is set to true when compiling for publishing
const bool EMBED_ENGINE_DATA=(true && !WINDOWS_NEW && !MOBILE && !WEB); // this is set to true when "Embed Engine Data" was enabled in application settings, this is always disabled for WindowsNew, Mobile and Web builds
const bool ANDROID_EXPANSION=false; // this is set to true when auto-download of Android Expansion Files is enabled
cchar *C    ENGINE_DATA_PATH=((WINDOWS_NEW || MOBILE || WEB) ? u"Engine.pak"  : PUBLISH ? u"Bin/Engine.pak"  : u"C:/Esenthel/Editor/Bin/Engine.pak");
cchar *C   PROJECT_DATA_PATH=((WINDOWS_NEW || MOBILE || WEB) ? u"Project.pak" : PUBLISH ? u"Bin/Project.pak" : u"C:/Esenthel/Editor Source/h3kv^1fvcwe4ri0#ll#761m7/Game");
cchar *C   PROJECT_NAME     =u"Editor";
cchar *C   APP_NAME         =u"Titan Editor";
const UID  APP_GUI_SKIN     =UID(0, 0, 0, 0);
Cipher *C  PROJECT_CIPHER   =null;
/******************************************************************************/
// FUNCTIONS
/******************************************************************************/
void INIT(bool load_engine_data, bool load_project_data)
{
   App.name(APP_NAME); // set application name
   INIT_OBJ_TYPE(); // initialize object type enum
   LoadEmbeddedPaks(PROJECT_CIPHER); // load data embedded in Application executable file
   if(load_engine_data )if(!EMBED_ENGINE_DATA)Paks.add(ENGINE_DATA_PATH); // load engine data
   if(load_project_data) // load project data
   {
      if(WINDOWS_NEW || MOBILE || WEB || PUBLISH)Paks.add(PROJECT_DATA_PATH, PROJECT_CIPHER);else DataPath(PROJECT_DATA_PATH);
      if(ANDROID && ANDROID_EXPANSION)
      {
         REP(APP_BUILD+1)if(Paks.addTry(AndroidExpansionFileName(i), PROJECT_CIPHER))goto added;
         Exit("Can't load Project Data");
      added:;
      }
   }
}
void INIT_OBJ_TYPE() // this function will setup 'ObjType' enum used for object types
{
}
/******************************************************************************/
// ENUMS
/******************************************************************************/

/******************************************************************************/