/******************************************************************************/
#include "stdafx.h"
#define SUPPORT_KARTRIDGE 0//WINDOWS_OLD
#if     SUPPORT_KARTRIDGE
//#define KONG_STATIC_LIBRARY 1
#include "../../../ThirdPartyLibs/Kartridge/kongregate.h"
#endif
namespace EE{
/******************************************************************************/
KARTRIDGE Kartridge;
KARTRIDGE::~KARTRIDGE()
{
#if SUPPORT_KARTRIDGE
   KongregateAPI_Shutdown();
#endif
}
/*static void EventHandler(void* context, const char *event, const char* const payload)
{
   if(event==KONGREGATE_EVENT_READY)
   {
   }else
   if(event==KONGREGATE_EVENT_CONNECTED)
   {
   }else
   if(event==KONGREGATE_EVENT_USER)
   {
   }
}*/
Bool KARTRIDGE::init(UInt game_id)
{
#if SUPPORT_KARTRIDGE
    //KongregateAPI_SetEventCallback(EventHandler, this);
   if(KongregateAPI_Initialize(null))
   {
      REP(1000){if(KongregateAPI_IsConnected())goto connected; Time.wait(1); KongregateAPI_Update();} return false; connected:;
      REP(1000){if(KongregateAPI_IsReady    ())goto ready    ; Time.wait(1); KongregateAPI_Update();} return false; ready    :;
     _user_name=KongregateServices_GetUsername();
     _user_id  =KongregateServices_GetUserId();
      return    KongregateLibrary_IsGameOwned(game_id);
   }
#endif
   return false;
}
/******************************************************************************/
}
/******************************************************************************/
