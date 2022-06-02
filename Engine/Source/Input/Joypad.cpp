/******************************************************************************/
#include "stdafx.h"
namespace EE{
/******************************************************************************/
#define SORT_JOYPADS_BY_ID 0 // don't use this, so user can do things like manually changing order of joypads "Joypads.swapOrder"

// for JP_GAMEPAD_INPUT _raw_game_controller
#define MAX_BUTTONS 32
#define MAX_AXES     6
#define MAX_SWITCHES 1
/******************************************************************************/
static Bool CalculateJoypadSensors;
CChar8* Joypad::_button_name[32+4]; // 32 DirectInput + 4xDPad
MemtN<Joypad, 4> Joypads;
/******************************************************************************/
#if JP_DIRECT_INPUT
static Bool IsGamePadInput(U16 vendor_id, U16 product_id)
{
#if JP_GAMEPAD_INPUT
   REPA(Joypads)
   {
      Joypad &jp=Joypads[i]; if(jp._vendor_id==vendor_id && jp._product_id==product_id && (jp._gamepad || jp._raw_game_controller))return true;
   }
#endif
   return false;
}
#endif
#if JP_GAMEPAD_INPUT && WINDOWS_OLD
static Microsoft::WRL::ComPtr<ABI::Windows::Gaming::Input::IGamepadStatics          > GamepadStatics;
static Microsoft::WRL::ComPtr<ABI::Windows::Gaming::Input::IGamepadStatics2         > GamepadStatics2;
static Microsoft::WRL::ComPtr<ABI::Windows::Gaming::Input::IRawGameControllerStatics> RawGameControllerStatics;
static EventRegistrationToken GamePadAddedToken, GamePadRemovedToken, RawGameControllerAddedToken, RawGameControllerRemovedToken;

static Int FindJoypadI(ABI::Windows::Gaming::Input::IGamepad          * gamepad            ) {REPA(Joypads)if(Joypads[i]._gamepad            .Get()==gamepad            )return i; return -1;}
static Int FindJoypadI(ABI::Windows::Gaming::Input::IRawGameController* raw_game_controller) {REPA(Joypads)if(Joypads[i]._raw_game_controller.Get()==raw_game_controller)return i; return -1;}

struct GamePadChange
{
   Bool                                                                    added;
   Microsoft::WRL::ComPtr<ABI::Windows::Gaming::Input::IGamepad          > gamepad;
   Microsoft::WRL::ComPtr<ABI::Windows::Gaming::Input::IRawGameController> raw_game_controller;

   void process()
   {
      if(added)
      {
         if(gamepad            .Get() && FindJoypadI(gamepad            .Get())>=0         // make sure it's not already listed
         || raw_game_controller.Get() && FindJoypadI(raw_game_controller.Get())>=0)return; // make sure it's not already listed

         U16  vendor_id=0, product_id=0;
         UInt joypad_id=0;
         Microsoft::WRL::ComPtr<ABI::Windows::Gaming::Input::IRawGameController > raw_game_controller;
         Microsoft::WRL::ComPtr<ABI::Windows::Gaming::Input::IRawGameController2> raw_game_controller2;
         Microsoft::WRL::ComPtr<ABI::Windows::Gaming::Input::IGameController    >     game_controller;
         if(gamepad) // gamepad callback
         {
            gamepad->QueryInterface(IID_PPV_ARGS(&game_controller)); if(game_controller)
            {
               if(RawGameControllerStatics)RawGameControllerStatics->FromGameController(game_controller.Get(), &raw_game_controller);
            }
         }else // raw_game_controller callback
         if(raw_game_controller=T.raw_game_controller)
         {
            raw_game_controller->QueryInterface(IID_PPV_ARGS(&game_controller)); if(game_controller)
            {
               if(GamepadStatics2)
               {
                  GamepadStatics2->FromGameController(game_controller.Get(), &gamepad);
                  if(gamepad)return; // if this is a gamepad, then ignore this 'raw_game_controller' callback, as it will be processed using 'gamepad' callback, to avoid having the same gamepad listed twice
               }
            }
         }

         if(raw_game_controller)
         {
            raw_game_controller->get_HardwareVendorId (& vendor_id);
            raw_game_controller->get_HardwareProductId(&product_id);
         #if JP_DIRECT_INPUT // if we use DirectInput then have to remove all DirectInput joypads with same vendor/product ID as they will be processed using this API instead
            REPA(Joypads) // go from back because we remove
            {
               Joypad &jp=Joypads[i]; if(jp._vendor_id==vendor_id && jp._product_id==product_id && jp._device)Joypads.remove(i, true); // remove all (not just one)
            }
         #endif
            raw_game_controller.As(&raw_game_controller2); if(raw_game_controller2)
            {
               HSTRING id=null; raw_game_controller2->get_NonRoamableId(&id); C wchar_t *controller_id=WindowsGetStringRawBuffer(id, null); joypad_id=xxHash64_32Mem(controller_id, SIZE(*controller_id)*Length(controller_id));
            }
         }

         joypad_id=NewJoypadID(joypad_id); // make sure it's not used yet !! set this before creating new 'Joypad' !!
         Bool added; Joypad &joypad=GetJoypad(joypad_id, added); if(added)
         {
            joypad._gamepad            =gamepad;
            joypad._raw_game_controller=raw_game_controller;

            if(raw_game_controller)
            {
               raw_game_controller->get_ButtonCount(&joypad._buttons ); MIN(joypad._buttons , MAX_BUTTONS);
               raw_game_controller->get_SwitchCount(&joypad._switches); MIN(joypad._switches, MAX_SWITCHES);
               raw_game_controller->get_AxisCount  (&joypad._axes    ); MIN(joypad._axes    , MAX_AXES);
               /* were empty on Logitech F310 in DirectInput mode, Nintendo JoyCons
               FREP(buttons)
               {
                  ABI::Windows::Gaming::Input::GameControllerButtonLabel label=ABI::Windows::Gaming::Input::GameControllerButtonLabel_None;
                 _raw_game_controller->GetButtonLabel(i, &label);
               }*/
            #if JOYPAD_VENDOR_PRODUCT_ID
               joypad. _vendor_id= vendor_id;
               joypad._product_id=product_id;
            #endif
               Microsoft::WRL::ComPtr<ABI::Windows::Foundation::Collections::IVectorView<ABI::Windows::Gaming::Input::ForceFeedback::ForceFeedbackMotor*>> motors; raw_game_controller->get_ForceFeedbackMotors(&motors); if(motors)
               {
                  unsigned motors_count=0; motors->get_Size(&motors_count); joypad._vibrations=(motors_count>0);
               }
            }
            if(raw_game_controller2)
            {
               HSTRING display_name=null; raw_game_controller2->get_DisplayName(&display_name); joypad._name=WindowsGetStringRawBuffer(display_name, null);
            }
         }
      }else
      {
         if(gamepad            .Get())Joypads.remove(FindJoypadI(gamepad            .Get()), true);
         if(raw_game_controller.Get())Joypads.remove(FindJoypadI(raw_game_controller.Get()), true);
      }
   }
};
static MemcThreadSafe<GamePadChange> GamePadChanges;

static void GamePadChanged()
{
   MemcThreadSafeLock lock(GamePadChanges); FREPA(GamePadChanges)GamePadChanges.lockedElm(i).process(); GamePadChanges.lockedClear(); // process in order
}
static void GamePadChanged(Bool added, ABI::Windows::Gaming::Input::IGamepad* gamepad, ABI::Windows::Gaming::Input::IRawGameController* raw_game_controller)
{
   {MemcThreadSafeLock lock(GamePadChanges); GamePadChange &gpc=GamePadChanges.lockedNew(); gpc.added=added; gpc.gamepad=gamepad; gpc.raw_game_controller=raw_game_controller;}
   App.addFuncCall(GamePadChanged);
}

// !! THESE ARE CALLED ON SECONDARY THREADS !!
static struct GamePadAddedClass   : Microsoft::WRL::RuntimeClass<Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>, __FIEventHandler_1_Windows__CGaming__CInput__CGamepad> {virtual HRESULT Invoke(IInspectable*, ABI::Windows::Gaming::Input::IGamepad* gamepad)override {GamePadChanged(true , gamepad, null); return S_OK;}}GamePadAdded;
static struct GamePadRemovedClass : Microsoft::WRL::RuntimeClass<Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>, __FIEventHandler_1_Windows__CGaming__CInput__CGamepad> {virtual HRESULT Invoke(IInspectable*, ABI::Windows::Gaming::Input::IGamepad* gamepad)override {GamePadChanged(false, gamepad, null); return S_OK;}}GamePadRemoved;

static struct RawGameControllerAddedClass   : Microsoft::WRL::RuntimeClass<Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>, __FIEventHandler_1_Windows__CGaming__CInput__CRawGameController> {virtual HRESULT Invoke(IInspectable*, ABI::Windows::Gaming::Input::IRawGameController* raw_game_controller)override {GamePadChanged(true , null, raw_game_controller); return S_OK;}}RawGameControllerAdded;
static struct RawGameControllerRemovedClass : Microsoft::WRL::RuntimeClass<Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>, __FIEventHandler_1_Windows__CGaming__CInput__CRawGameController> {virtual HRESULT Invoke(IInspectable*, ABI::Windows::Gaming::Input::IRawGameController* raw_game_controller)override {GamePadChanged(false, null, raw_game_controller); return S_OK;}}RawGameControllerRemoved;

#elif MAC
ASSERT(MEMBER_SIZE(Joypad::Elm, cookie)==SIZE(UInt)); // because it's stored as UInt for EE_PRIVATE
static IOHIDManagerRef HidManager;
static UInt JoypadsID;
/******************************************************************************/
static Int Compare(C Joypad::Elm &a, C Joypad::Elm        &b) {return Compare(UIntPtr(a.cookie), UIntPtr(b.cookie));}
static Int Compare(C Joypad::Elm &a, C IOHIDElementCookie &b) {return Compare(UIntPtr(a.cookie), UIntPtr(b       ));}

static NSMutableDictionary* JoypadCriteria(UInt32 inUsagePage, UInt32 inUsage)
{
   NSMutableDictionary* dict=[[NSMutableDictionary alloc] init];
   [dict setObject: [NSNumber numberWithInt: inUsagePage] forKey: (NSString*)CFSTR(kIOHIDDeviceUsagePageKey)];
   [dict setObject: [NSNumber numberWithInt: inUsage    ] forKey: (NSString*)CFSTR(kIOHIDDeviceUsageKey    )];
   return dict;
} 
static void JoypadAdded(void *inContext, IOReturn inResult, void *inSender, IOHIDDeviceRef device) // this is called on the main thread
{
   Memt<Joypad::Elm> elms;
   Int               buttons=0, axes=0;
   NSArray          *elements=(NSArray*)IOHIDDeviceCopyMatchingElements(device, null, kIOHIDOptionsTypeNone);
   FREP([elements count]) // process in order
   {
      IOHIDElementRef element=(IOHIDElementRef)[elements objectAtIndex: i];
      Int type =IOHIDElementGetType       (element),
          usage=IOHIDElementGetUsage      (element),
          page =IOHIDElementGetUsagePage  (element),
           min =IOHIDElementGetPhysicalMin(element),
           max =IOHIDElementGetPhysicalMax(element),
          lmin =IOHIDElementGetLogicalMin (element),
          lmax =IOHIDElementGetLogicalMax (element);
      IOHIDElementCookie cookie=IOHIDElementGetCookie(element);
    //CFStringRef      elm_name=IOHIDElementGetName  (element); NSLog(@"%@", (NSString*)elm_name);

      if(type==kIOHIDElementTypeInput_Misc || type==kIOHIDElementTypeInput_Axis || type==kIOHIDElementTypeInput_Button)
      {
         if((max-min==1) || page==kHIDPage_Button || type==kIOHIDElementTypeInput_Button){if(InRange(buttons, MEMBER(Joypad, _mac_button)))elms.New().setButton(cookie, buttons++, min, max);}else
         if(usage>=0x30 && usage<0x36                                                   )                                                  elms.New().setAxis  (cookie, axes   ++, min, max); else
         if(usage==0x39                                                                 )                                                  elms.New().setPad   (cookie,                lmax);
      }
   }

   if(elms.elms())
   {
      elms.sort(Compare); // sort so 'binaryFind' can be used later

      NSString *name  =(NSString*)IOHIDDeviceGetProperty(device, CFSTR(kIOHIDProductKey     )); // do not release this !!
    //NSString *serial=(NSString*)IOHIDDeviceGetProperty(device, CFSTR(kIOHIDSerialNumberKey)); // do not release this ? this was null on "Logitech Rumblepad 2"
	   Int    vendorId=[(NSNumber*)IOHIDDeviceGetProperty(device, CFSTR(kIOHIDVendorIDKey    )) intValue];
	   Int   productId=[(NSNumber*)IOHIDDeviceGetProperty(device, CFSTR(kIOHIDProductIDKey   )) intValue];

      Bool added; Joypad &jp=GetJoypad(JoypadsID++, added); if(added)
      {
         jp._name  =name;
         jp._device=device;
         jp._elms  =elms;
      }
   }
}
static void JoypadRemoved(void *inContext, IOReturn inResult, void *inSender, IOHIDDeviceRef device) // this is called on the main thread
{
   REPA(Joypads)if(Joypads[i]._device==device){Joypads.remove(i, true); break;}
}
static void JoypadAction(void *inContext, IOReturn inResult, void *inSender, IOHIDValueRef value) // this is called on the main thread
{
   IOHIDElementRef element=IOHIDValueGetElement (value  );
   IOHIDDeviceRef  device =IOHIDElementGetDevice(element); // or IOHIDQueueGetDevice((IOHIDQueueRef)inSender);
   REPA(Joypads)
   {
      Joypad &jp=Joypads[i]; if(jp._device==device)
      {
         if(C Joypad::Elm *elm=jp._elms.binaryFind(IOHIDElementGetCookie(element), Compare))
         {
            Int val=IOHIDValueGetIntegerValue(value);
            switch(elm->type)
            {
               case Joypad::Elm::PAD:
               {
                  if(InRange(val, elm->max))
                  {
                     CosSin(jp.dir.x, jp.dir.y, val*elm->mul+elm->add);
                     jp.setDiri(Round(jp.dir.x), Round(jp.dir.y));
                  }else
                  {
                     jp.dir.zero();
                     jp.setDiri(0, 0);
                  }
               }break;

               case Joypad::Elm::BUTTON:
               {
                  jp._mac_button[elm->index]=(val>elm->avg);
               }break;

               case Joypad::Elm::AXIS:
               {
                  switch(elm->index)
                  {
                     case 0: jp.dir_a[0].x=val*elm->mul+elm->add; break;
                     case 1: jp.dir_a[0].y=val*elm->mul+elm->add; break;
                     case 2: jp.dir_a[1].x=val*elm->mul+elm->add; break;
                     case 3: jp.dir_a[1].y=val*elm->mul+elm->add; break;
                  }
               }break;
            }
         }
         break;
      }
   }
}
#endif
/******************************************************************************/
Joypad::~Joypad()
{
#if JP_DIRECT_INPUT
   if(_device){_device->Unacquire(); _device->Release(); _device=null;}
#endif

   // adjust 'Inputs', because it holds a 'device' index, so have to adjust those indexes
   Int device=index(); if(device>=0)REPA(Inputs) // go from the end because we're removing elements
   {
      Input &input=Inputs[i]; if(input.type==INPUT_JOYPAD)
      {
         if(input.device> device)input .device--;else    // adjust the index
         if(input.device==device)Inputs.remove(i, true); // if this input is for joypad being deleted, then delete the input and keep order
      }
   }
}
Joypad::Joypad()
{
   ASSERT(ELMS(_last_t)==ELMS(_button));

#if SWITCH
   Zero(_vibration_handle); Zero(_sensor_handle);
#elif MAC
   Zero(_mac_button);
#endif

  _color_left .zero();
  _color_right.zero();

   zero();

   // adjust 'Inputs', because it holds a 'device' index, so have to adjust those indexes
   Int device=index(); if(device>=0 && device<Joypads.elms()-1)REPA(Inputs) // if it's in 'Joypads' and it's not the last one, then it means we've just moved some existing joypads to the right
   {
      Input &input=Inputs[i]; if(input.type==INPUT_JOYPAD && input.device>=device)input.device++;
   }
}
CChar8* Joypad::buttonName(Int b)C {return InRange(b, _button_name) ? _button_name[b] : null;}
CChar8* Joypad::ButtonName(Int b)  {return InRange(b, _button_name) ? _button_name[b] : null;}
/******************************************************************************/
Bool Joypad::supportsVibrations()C
{
#if JP_X_INPUT
   return _xinput!=0xFF;
#elif JP_GAMEPAD_INPUT
   return _vibrations;
#elif SWITCH
   return _vibration_handle[0];// || _vibration_handle[1]; check only first because second will be available only if first is
#endif
   return false;
}
Bool Joypad::supportsSensors()C
{
#if SWITCH
   return _sensor_handle[0];// || _sensor_handle[1]; check only first because second will be available only if first is
#else
   return false;
#endif
}
Int Joypad::index()C {return Joypads.index(this);}
/******************************************************************************/
#if !SWITCH
Joypad& Joypad::vibration(C Vec2 &vibration)
{
#if JP_X_INPUT
   if(_xinput!=0xFF)
   {
      XINPUT_VIBRATION xvibration;
      xvibration. wLeftMotorSpeed=RoundU(Sat(vibration.x)*0xFFFF);
      xvibration.wRightMotorSpeed=RoundU(Sat(vibration.y)*0xFFFF);
      XInputSetState(_xinput, &xvibration);
   }
#elif JP_GAMEPAD_INPUT
   if(_gamepad)
   {
   #if WINDOWS_OLD
      ABI::Windows::Gaming::Input::GamepadVibration v;
   #else
      Windows::Gaming::Input::GamepadVibration v;
   #endif
      v. LeftMotor=vibration.x;
      v.RightMotor=vibration.y;
      v.LeftTrigger=v.RightTrigger=0;
   #if WINDOWS_OLD
     _gamepad->put_Vibration(v);
   #else
     _gamepad->Vibration=v;
   #endif
   }
#endif
   return T;
}
Joypad& Joypad::vibration(C Vibration &left, C Vibration &right)
{
   return vibration(Vec2(Max(left .motor[0].intensity, left .motor[1].intensity),
                         Max(right.motor[0].intensity, right.motor[1].intensity)));
}
#endif
/******************************************************************************/
void Joypad::zero()
{
   Zero(_button);
   REPAO(_last_t )=-FLT_MAX;
          _dir_t  =-FLT_MAX;
   REPAO( _dir_at)=-FLT_MAX;
   REPAO(diri_ar).zero();
         diri_r  .zero();
         diri    .zero();
         dir     .zero();
   REPAO(dir_a  ).zero();
   REPAO(trigger)=0;
  _sensor_left .reset();
  _sensor_right.reset();
}
void Joypad::clear()
{
   REPAO(_button)&=~BS_NOT_ON;
         diri_r  .zero();
   REPAO(diri_ar).zero();
}
void Joypad::update(C Byte *on, Int elms)
{
   MIN(elms, Elms(_button));
   REP(elms){Byte o=on[i]; if((o!=0)!=ButtonOn(_button[i])){if(o)push(i);else release(i);}}
}
#if WINDOWS_NEW
static inline Bool FlagOn(Windows::Gaming::Input::GamepadButtons flags, Windows::Gaming::Input::GamepadButtons f) {return (flags&f)!=Windows::Gaming::Input::GamepadButtons::None;}
#endif
static void UpdateDirRep(VecSB2 &diri_r, C VecSB2 &diri, Flt &time, Flt first_time, Flt repeat_time)
{
   if(diri.any())
   {
      if(Time.appTime()>=time)
      {
         diri_r=diri;
         time  =Time.appTime()+((time<0) ? first_time : repeat_time); // if first press, then wait longer
      }
   }else
   {
      time=-FLT_MAX;
   }
}
static VecSB2 DirToDirI(C Vec2 &d)
{
#if 0
   const Flt tan=0.414213568f; // Tan(PI_4/2) correct value
#else
   const Flt tan=0.5f; // use higher value to minimize chance of moving diagonally by accident
#endif
   const Flt dead=0.333f;
   Flt v=Abs(d).max(); if(v>dead)return SignEpsB(d/v, tan);
   return 0;
}
inline void Joypad::updateOK()
{
   UpdateDirRep(diri_r    ,           diri     , _dir_t    , FirstRepeatPressTime,       RepeatPressTime);
   UpdateDirRep(diri_ar[0], DirToDirI(dir_a[0]), _dir_at[0], FirstRepeatPressTime, AnalogRepeatPressTime);
   UpdateDirRep(diri_ar[1], DirToDirI(dir_a[1]), _dir_at[1], FirstRepeatPressTime, AnalogRepeatPressTime);
}
void Joypad::update()
{
#if WINDOWS
#if JP_X_INPUT
   if(_xinput!=0xFF)
   {
      XINPUT_STATE state; if(XInputGetState(_xinput, &state)==ERROR_SUCCESS)
      {
         // buttons
         Byte button[JB_NUM];
         button[JB_A     ]=FlagOn(state.Gamepad.wButtons     , XINPUT_GAMEPAD_A                );
         button[JB_B     ]=FlagOn(state.Gamepad.wButtons     , XINPUT_GAMEPAD_B                );
         button[JB_X     ]=FlagOn(state.Gamepad.wButtons     , XINPUT_GAMEPAD_X                );
         button[JB_Y     ]=FlagOn(state.Gamepad.wButtons     , XINPUT_GAMEPAD_Y                );
         button[JB_L1    ]=FlagOn(state.Gamepad.wButtons     , XINPUT_GAMEPAD_LEFT_SHOULDER    );
         button[JB_R1    ]=FlagOn(state.Gamepad.wButtons     , XINPUT_GAMEPAD_RIGHT_SHOULDER   );
         button[JB_L2    ]=      (state.Gamepad. bLeftTrigger>=XINPUT_GAMEPAD_TRIGGER_THRESHOLD);
         button[JB_R2    ]=      (state.Gamepad.bRightTrigger>=XINPUT_GAMEPAD_TRIGGER_THRESHOLD);
         button[JB_LTHUMB]=FlagOn(state.Gamepad.wButtons     , XINPUT_GAMEPAD_LEFT_THUMB       );
         button[JB_RTHUMB]=FlagOn(state.Gamepad.wButtons     , XINPUT_GAMEPAD_RIGHT_THUMB      );
         button[JB_BACK  ]=FlagOn(state.Gamepad.wButtons     , XINPUT_GAMEPAD_BACK             );
         button[JB_START ]=FlagOn(state.Gamepad.wButtons     , XINPUT_GAMEPAD_START            );
         ASSERT(ELMS(button)<=ELMS(T._button));
         update(button, Elms(button));

         // digital pad
         setDiri(FlagOn(state.Gamepad.wButtons, XINPUT_GAMEPAD_DPAD_RIGHT)-FlagOn(state.Gamepad.wButtons, XINPUT_GAMEPAD_DPAD_LEFT),
                 FlagOn(state.Gamepad.wButtons, XINPUT_GAMEPAD_DPAD_UP   )-FlagOn(state.Gamepad.wButtons, XINPUT_GAMEPAD_DPAD_DOWN));
         dir=diri;
         if(diri.x && diri.y)dir/=SQRT2; // dir.clipLength(1)

         // analog pad
         dir_a[0].x=state.Gamepad.sThumbLX/32768.0f;
         dir_a[0].y=state.Gamepad.sThumbLY/32768.0f;
         dir_a[1].x=state.Gamepad.sThumbRX/32768.0f;
         dir_a[1].y=state.Gamepad.sThumbRY/32768.0f;

         // triggers
         trigger[0]=state.Gamepad. bLeftTrigger/255.0f;
         trigger[1]=state.Gamepad.bRightTrigger/255.0f;

         updateOK(); return;
      }
   }
#elif JP_GAMEPAD_INPUT
   if(_gamepad)
   {
   #if WINDOWS_OLD
      ABI::Windows::Gaming::Input::GamepadReading state; if(OK(_gamepad->GetCurrentReading(&state)))
   #else
      auto state=_gamepad->GetCurrentReading();
   #endif
      {
         // buttons
         Byte button[JB_UWP_NUM];
      #if WINDOWS_OLD
         button[JB_A      ]=FlagOn(state.Buttons, ABI::Windows::Gaming::Input::GamepadButtons::GamepadButtons_A);
         button[JB_B      ]=FlagOn(state.Buttons, ABI::Windows::Gaming::Input::GamepadButtons::GamepadButtons_B);
         button[JB_X      ]=FlagOn(state.Buttons, ABI::Windows::Gaming::Input::GamepadButtons::GamepadButtons_X);
         button[JB_Y      ]=FlagOn(state.Buttons, ABI::Windows::Gaming::Input::GamepadButtons::GamepadButtons_Y);
         button[JB_L1     ]=FlagOn(state.Buttons, ABI::Windows::Gaming::Input::GamepadButtons::GamepadButtons_LeftShoulder);
         button[JB_R1     ]=FlagOn(state.Buttons, ABI::Windows::Gaming::Input::GamepadButtons::GamepadButtons_RightShoulder);
         button[JB_LTHUMB ]=FlagOn(state.Buttons, ABI::Windows::Gaming::Input::GamepadButtons::GamepadButtons_LeftThumbstick);
         button[JB_RTHUMB ]=FlagOn(state.Buttons, ABI::Windows::Gaming::Input::GamepadButtons::GamepadButtons_RightThumbstick);
         button[JB_BACK   ]=FlagOn(state.Buttons, ABI::Windows::Gaming::Input::GamepadButtons::GamepadButtons_View);
         button[JB_START  ]=FlagOn(state.Buttons, ABI::Windows::Gaming::Input::GamepadButtons::GamepadButtons_Menu);
         button[JB_PADDLE1]=FlagOn(state.Buttons, ABI::Windows::Gaming::Input::GamepadButtons::GamepadButtons_Paddle1);
         button[JB_PADDLE2]=FlagOn(state.Buttons, ABI::Windows::Gaming::Input::GamepadButtons::GamepadButtons_Paddle2);
         button[JB_PADDLE3]=FlagOn(state.Buttons, ABI::Windows::Gaming::Input::GamepadButtons::GamepadButtons_Paddle3);
         button[JB_PADDLE4]=FlagOn(state.Buttons, ABI::Windows::Gaming::Input::GamepadButtons::GamepadButtons_Paddle4);
      #else
         button[JB_A      ]=FlagOn(state.Buttons, Windows::Gaming::Input::GamepadButtons::A);
         button[JB_B      ]=FlagOn(state.Buttons, Windows::Gaming::Input::GamepadButtons::B);
         button[JB_X      ]=FlagOn(state.Buttons, Windows::Gaming::Input::GamepadButtons::X);
         button[JB_Y      ]=FlagOn(state.Buttons, Windows::Gaming::Input::GamepadButtons::Y);
         button[JB_L1     ]=FlagOn(state.Buttons, Windows::Gaming::Input::GamepadButtons:: LeftShoulder);
         button[JB_R1     ]=FlagOn(state.Buttons, Windows::Gaming::Input::GamepadButtons::RightShoulder);
         button[JB_LTHUMB ]=FlagOn(state.Buttons, Windows::Gaming::Input::GamepadButtons:: LeftThumbstick);
         button[JB_RTHUMB ]=FlagOn(state.Buttons, Windows::Gaming::Input::GamepadButtons::RightThumbstick);
         button[JB_BACK   ]=FlagOn(state.Buttons, Windows::Gaming::Input::GamepadButtons::View);
         button[JB_START  ]=FlagOn(state.Buttons, Windows::Gaming::Input::GamepadButtons::Menu);
         button[JB_PADDLE1]=FlagOn(state.Buttons, Windows::Gaming::Input::GamepadButtons::Paddle1);
         button[JB_PADDLE2]=FlagOn(state.Buttons, Windows::Gaming::Input::GamepadButtons::Paddle2);
         button[JB_PADDLE3]=FlagOn(state.Buttons, Windows::Gaming::Input::GamepadButtons::Paddle3);
         button[JB_PADDLE4]=FlagOn(state.Buttons, Windows::Gaming::Input::GamepadButtons::Paddle4);
      #endif
         button[JB_L2]=(state. LeftTrigger>=30.0/255); // matches XINPUT_GAMEPAD_TRIGGER_THRESHOLD
         button[JB_R2]=(state.RightTrigger>=30.0/255); // matches XINPUT_GAMEPAD_TRIGGER_THRESHOLD
         ASSERT(ELMS(button)<=ELMS(T._button));
         update(button, Elms(button));

         // digital pad
      #if WINDOWS_OLD
         setDiri(FlagOn(state.Buttons, ABI::Windows::Gaming::Input::GamepadButtons::GamepadButtons_DPadRight)-FlagOn(state.Buttons, ABI::Windows::Gaming::Input::GamepadButtons::GamepadButtons_DPadLeft),
                 FlagOn(state.Buttons, ABI::Windows::Gaming::Input::GamepadButtons::GamepadButtons_DPadUp   )-FlagOn(state.Buttons, ABI::Windows::Gaming::Input::GamepadButtons::GamepadButtons_DPadDown));
      #else
         setDiri(FlagOn(state.Buttons, Windows::Gaming::Input::GamepadButtons::DPadRight)-FlagOn(state.Buttons, Windows::Gaming::Input::GamepadButtons::DPadLeft),
                 FlagOn(state.Buttons, Windows::Gaming::Input::GamepadButtons::DPadUp   )-FlagOn(state.Buttons, Windows::Gaming::Input::GamepadButtons::DPadDown));
      #endif
         dir=diri;
         if(diri.x && diri.y)dir/=SQRT2; // dir.clipLength(1)

         // analog pad
         dir_a[0].set(state. LeftThumbstickX, state. LeftThumbstickY);
         dir_a[1].set(state.RightThumbstickX, state.RightThumbstickY);

         // triggers
         trigger[0]=state. LeftTrigger;
         trigger[1]=state.RightTrigger;

         updateOK(); return;
      }
   }else
   if(_raw_game_controller)
   {
   #if WINDOWS_OLD
      boolean                                                   button[MAX_BUTTONS ]; ASSERT(ELMS(button)<=ELMS(T._button));
      DOUBLE                                                    axis  [MAX_AXES    ];
      ABI::Windows::Gaming::Input::GameControllerSwitchPosition Switch[MAX_SWITCHES];
      UINT64 timestamp;
      if(OK(_raw_game_controller->GetCurrentReading(_buttons, button, _switches, Switch, _axes, axis, &timestamp)))
      {
         update(button, _buttons);
         if(_axes>=2)dir_a[0].set(axis[0]*2-1, axis[1]*-2+1);
         if(_axes>=4)dir_a[1].set(axis[2]*2-1, axis[3]*-2+1);
         if(_axes>=6){trigger[0]=axis[4]; trigger[1]=axis[5];}
         if(_switches)switch(Switch[0])
         {
            case ABI::Windows::Gaming::Input::GameControllerSwitchPosition_Center   : setDiri( 0,  0); dir.zero(                  ); break;
            case ABI::Windows::Gaming::Input::GameControllerSwitchPosition_Up       : setDiri( 0,  1); dir.set (       0,        1); break;
            case ABI::Windows::Gaming::Input::GameControllerSwitchPosition_UpRight  : setDiri( 1,  1); dir.set ( SQRT2_2,  SQRT2_2); break;
            case ABI::Windows::Gaming::Input::GameControllerSwitchPosition_Right    : setDiri( 1,  0); dir.set (       1,        0); break;
            case ABI::Windows::Gaming::Input::GameControllerSwitchPosition_DownRight: setDiri( 1, -1); dir.set ( SQRT2_2, -SQRT2_2); break;
            case ABI::Windows::Gaming::Input::GameControllerSwitchPosition_Down     : setDiri( 0, -1); dir.set (       0,       -1); break;
            case ABI::Windows::Gaming::Input::GameControllerSwitchPosition_DownLeft : setDiri(-1, -1); dir.set (-SQRT2_2, -SQRT2_2); break;
            case ABI::Windows::Gaming::Input::GameControllerSwitchPosition_Left     : setDiri(-1,  0); dir.set (      -1,        0); break;
            case ABI::Windows::Gaming::Input::GameControllerSwitchPosition_UpLeft   : setDiri(-1,  1); dir.set (-SQRT2_2,  SQRT2_2); break;
         }
         updateOK(); return;
      }
   #else
     _raw_game_controller->GetCurrentReading(_array_button, _array_switch, _array_axis);

      bool *button=_array_button->Data; ASSERT(SIZE(*button)==SIZE(Byte)); update((Byte*)button, _buttons);

      double *axis=_array_axis->Data;
      if(_axes>=2)dir_a[0].set(axis[0]*2-1, axis[1]*-2+1);
      if(_axes>=4)dir_a[1].set(axis[2]*2-1, axis[3]*-2+1);
      if(_axes>=6){trigger[0]=axis[4]; trigger[1]=axis[5];}

      if(_switches)switch(*_array_switch->Data)
      {
         case Windows::Gaming::Input::GameControllerSwitchPosition::Center   : setDiri( 0,  0); dir.zero(                  ); break;
         case Windows::Gaming::Input::GameControllerSwitchPosition::Up       : setDiri( 0,  1); dir.set (       0,        1); break;
         case Windows::Gaming::Input::GameControllerSwitchPosition::UpRight  : setDiri( 1,  1); dir.set ( SQRT2_2,  SQRT2_2); break;
         case Windows::Gaming::Input::GameControllerSwitchPosition::Right    : setDiri( 1,  0); dir.set (       1,        0); break;
         case Windows::Gaming::Input::GameControllerSwitchPosition::DownRight: setDiri( 1, -1); dir.set ( SQRT2_2, -SQRT2_2); break;
         case Windows::Gaming::Input::GameControllerSwitchPosition::Down     : setDiri( 0, -1); dir.set (       0,       -1); break;
         case Windows::Gaming::Input::GameControllerSwitchPosition::DownLeft : setDiri(-1, -1); dir.set (-SQRT2_2, -SQRT2_2); break;
         case Windows::Gaming::Input::GameControllerSwitchPosition::Left     : setDiri(-1,  0); dir.set (      -1,        0); break;
         case Windows::Gaming::Input::GameControllerSwitchPosition::UpLeft   : setDiri(-1,  1); dir.set (-SQRT2_2,  SQRT2_2); break;
      }

      updateOK(); return;
   #endif
   }
#endif
#if JP_DIRECT_INPUT
   if(_device)
   {
      DIJOYSTATE state; if(OK(_device->Poll()) && OK(_device->GetDeviceState(SIZE(state), &state)))
      {
         // buttons
         ASSERT(ELMS( T._button)==ELMS(state.rgbButtons));
         update(state.rgbButtons, Elms(state.rgbButtons));

         // digital pad
         switch(state.rgdwPOV[0])
         {
            case UINT_MAX: setDiri( 0,  0); dir.zero(                  ); break;
            case        0: setDiri( 0,  1); dir.set (       0,        1); break;
            case     4500: setDiri( 1,  1); dir.set ( SQRT2_2,  SQRT2_2); break;
            case     9000: setDiri( 1,  0); dir.set (       1,        0); break;
            case    13500: setDiri( 1, -1); dir.set ( SQRT2_2, -SQRT2_2); break;
            case    18000: setDiri( 0, -1); dir.set (       0,       -1); break;
            case    22500: setDiri(-1, -1); dir.set (-SQRT2_2, -SQRT2_2); break;
            case    27000: setDiri(-1,  0); dir.set (      -1,        0); break;
            case    31500: setDiri(-1,  1); dir.set (-SQRT2_2,  SQRT2_2); break;
            default      : CosSin(dir.x, dir.y, PI_2-DegToRad(state.rgdwPOV[0]/100.0f)); setDiri(Round(dir.x), Round(dir.y)); break;
         }

         // analog pad
         const Flt mul=1.0f/32768;
         dir_a[0].x= (state.lX-32768)*mul;
         dir_a[0].y=-(state.lY-32768)*mul;
         if(_offset_x && _offset_y)
         {
            ASSERT(SIZE(state.lZ)==SIZE(Int));
            dir_a[1].x= (*(Int*)(((Byte*)&state)+_offset_x)-32768)*mul;
            dir_a[1].y=-(*(Int*)(((Byte*)&state)+_offset_y)-32768)*mul;
         }

         // triggers
         trigger[0]=(state.rglSlider[0]-32768)*mul;
         trigger[1]=(state.rglSlider[1]-32768)*mul;

         updateOK(); return;
      }
      if(App.active())acquire(true); // if failed then try to re-acquire
   }
#endif
#elif MAC
   ASSERT(ELMS(T._button)==ELMS(T._mac_button));
   update( T._mac_button,  Elms(T._mac_button));
   updateOK(); return;
#else
   updateOK(); return; // updated externally
#endif
   zero();
}
void Joypad::setDiri(Int x, Int y)
{
   if(diri.x!=x || diri.y!=y)
   {
      Int device=index();
      if(diri.x!=x)
      {
         if(diri.x && device>=0)Inputs.New().set(false, INPUT_JOYPAD, (diri.x>0) ? JB_DPAD_RIGHT : JB_DPAD_LEFT, device); // release
            diri.x=x;
         if(diri.x && device>=0)Inputs.New().set(true , INPUT_JOYPAD, (diri.x>0) ? JB_DPAD_RIGHT : JB_DPAD_LEFT, device); // push
      }
      if(diri.y!=y)
      {
         if(diri.y && device>=0)Inputs.New().set(false, INPUT_JOYPAD, (diri.y>0) ? JB_DPAD_UP : JB_DPAD_DOWN, device); // release
            diri.y=y;
         if(diri.y && device>=0)Inputs.New().set(true , INPUT_JOYPAD, (diri.y>0) ? JB_DPAD_UP : JB_DPAD_DOWN, device); // push
      }
   }
}
void Joypad::push(Byte b)
{
   if(InRange(b, _button) && !(_button[b]&BS_ON))
   {
     _button[b]|=BS_PUSHED|BS_ON;
      if(Time.appTime()-_last_t[b]<=DoubleClickTime+Time.ad())
      {
        _button[b]|=BS_DOUBLE;
        _last_t[b] =-FLT_MAX;
      }else
      {
        _last_t[b]=Time.appTime();
      }
      Int device=index(); if(device>=0)Inputs.New().set(true, INPUT_JOYPAD, b, device);
   }
}
void Joypad::release(Byte b)
{
   if(InRange(b, _button) && (_button[b]&BS_ON))
   {
     _button[b]&=~BS_ON;
     _button[b]|= BS_RELEASED;
      Int device=index(); if(device>=0)Inputs.New().set(false, INPUT_JOYPAD, b, device);
   }
}
void Joypad::eat(Int b)
{
   if(InRange(b, _button))FlagDisable(_button[b], BS_NOT_ON);
}
/******************************************************************************/
void Joypad::acquire(Bool on)
{
#if JP_DIRECT_INPUT
   if(_device){if(on)_device->Acquire();else _device->Unacquire();}
#endif
   if(!on)zero();
}
#if !SWITCH
void Joypad::sensors(Bool calculate) {}
#endif
/******************************************************************************/
void ApplyDeadZone(Vec2 &v, Flt dead_zone)
{
   Flt len =v.length();
   if( len<=dead_zone)v.zero();else // zero
   if( len>=1        )v/=len  ;else // normalize
                      v*=LerpR(dead_zone, 1, len)/len; // scale from dead_zone..1 -> 0..1
}
/******************************************************************************/
Bool JoypadSensors() {return CalculateJoypadSensors;}
void JoypadSensors(Bool calculate)
{
   if(CalculateJoypadSensors!=calculate)
   {
      CalculateJoypadSensors^=1;
      REPAO(Joypads).sensors(CalculateJoypadSensors);
   }
}
/******************************************************************************/
#if !SWITCH
void ConfigureJoypads(Int min_players, Int max_players, C CMemPtr<Str> &player_names, C CMemPtr<Color> &player_colors) {}
#endif
/******************************************************************************/
static Int Compare(C Joypad &a, C UInt &b) {return Compare(a.id(), b);}
Joypad* FindJoypad(UInt id)
{
#if SORT_JOYPADS_BY_ID
   return Joypads.binaryFind(id, Compare);
#else
   REPA(Joypads)if(Joypads[i].id()==id)return &Joypads[i];
#endif
   return null;
}
Joypad& GetJoypad(UInt id, Bool &added)
{
   added=false;
#if SORT_JOYPADS_BY_ID
   Joypad *joypad; Int index; if(Joypads.binarySearch(id, index, Compare))joypad=&Joypads[index];else{joypad=&Joypads.NewAt(index); joypad->_id=id; added=true;}
#else
   Joypad *joypad=FindJoypad(id);                                                         if(!joypad){joypad=&Joypads.New  (     ); joypad->_id=id; added=true;}
#endif
   joypad->_connected=true;
   return *joypad;
}
UInt NewJoypadID(UInt id)
{
   for(;;)
   {
      if(!FindJoypad(id))return id; // if no joypad uses this ID, then we can use it
      id++; // increase
   }
}
#if JP_DIRECT_INPUT
static Bool IsXInputDevice(C GUID &pGuidProductFromDirectInput) // !! Warning: this might trigger calling 'WindowMsg' !!
{
   Bool xinput=false;

   // Create WMI
   IWbemLocator *pIWbemLocator=null; CoCreateInstance(__uuidof(WbemLocator), null, CLSCTX_INPROC_SERVER, __uuidof(IWbemLocator), (Ptr*)&pIWbemLocator);
   if(           pIWbemLocator)
   {
      // Using the locator, connect to WMI in the given namespace
      if(BSTR Namespace=SysAllocString(L"root\\cimv2"))
      {
         IWbemServices *pIWbemServices=null; pIWbemLocator->ConnectServer(Namespace, null, null, null, 0, null, null, &pIWbemServices);
         if(            pIWbemServices)
         {
            if(BSTR Win32_PNPEntity=SysAllocString(L"Win32_PNPEntity"))
            {
               CoSetProxyBlanket(pIWbemServices, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, null, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, null, EOAC_NONE); // Switch security level to IMPERSONATE
               IEnumWbemClassObject *pEnumDevices=null; pIWbemServices->CreateInstanceEnum(Win32_PNPEntity, 0, null, &pEnumDevices);
               if(                   pEnumDevices)
               {
                  if(BSTR DeviceID=SysAllocString(L"DeviceID"))
                  {
                     IWbemClassObject *pDevices[16];
                  again:
                     DWORD returned=0; if(OK(pEnumDevices->Next(10000, Elms(pDevices), pDevices, &returned)) && returned)
                     {
                        FREP(returned) // check each device
                        {
                           if(!xinput)
                           {
                              VARIANT var; if(OK(pDevices[i]->Get(DeviceID, 0, &var, null, null)))
                              {
                                 if(var.vt==VT_BSTR && var.bstrVal!=null && wcsstr(var.bstrVal, L"IG_")) // Check if the device ID contains "IG_". If it does, then it's an XInput device
                                 {
                                    DWORD vendor_id=0, product_id=0; // If it does, then get the VID/PID from var.bstrVal
                                 #pragma warning(push)
                                 #pragma warning(disable:4996)
                                    WCHAR *strVid=wcsstr(var.bstrVal, L"VID_"); if(strVid && swscanf(strVid, L"VID_%4X", & vendor_id)!=1) vendor_id=0;
                                    WCHAR *strPid=wcsstr(var.bstrVal, L"PID_"); if(strPid && swscanf(strPid, L"PID_%4X", &product_id)!=1)product_id=0;
                                 #pragma warning(pop)

                                    if(MAKELONG(vendor_id, product_id)==pGuidProductFromDirectInput.Data1) // Compare the VID/PID to the DInput device
                                       xinput=true;
                                 }
                                 VariantClear(&var);
                              }
                           }
                           RELEASE(pDevices[i]);
                        }
                        if(!xinput)goto again;
                     }
                     SysFreeString(DeviceID);
                  }
                  pEnumDevices->Release();
               }
               SysFreeString(Win32_PNPEntity);
            }
            pIWbemServices->Release();
         }
         SysFreeString(Namespace);
      }
      pIWbemLocator->Release();
   }
   return xinput;
}
static BOOL CALLBACK EnumAxes(const DIDEVICEOBJECTINSTANCE *pdidoi, VOID *user)
{
   Joypad &joypad=*(Joypad*)user;

   // Logitech RumblePad 2                     uses: (x0=lX, y0=lY, x1=lZ , y1=lRz)
   // Logitech F310        in DirectInput mode uses: (x0=lX, y0=lY, x1=lZ , y1=lRz)
   // Logitech F310        in XInput      mode uses: (x0=lX, y0=lY, x1=lRx, y1=lRy)
   Int offset=0;
   if(pdidoi->guidType==GUID_ZAxis )offset=OFFSET(DIJOYSTATE, lZ );else
   if(pdidoi->guidType==GUID_RxAxis)offset=OFFSET(DIJOYSTATE, lRx);else
   if(pdidoi->guidType==GUID_RyAxis)offset=OFFSET(DIJOYSTATE, lRy);else
   if(pdidoi->guidType==GUID_RzAxis)offset=OFFSET(DIJOYSTATE, lRz);

   if(offset)
   {
      if(!joypad._offset_x || offset<joypad._offset_x) // X axis is assumed to be specified before Y axis
      {
         joypad._offset_y=joypad._offset_x;
         joypad._offset_x=        offset;
      }else
      if(!joypad._offset_y || offset<joypad._offset_y)// Y axis is assumed to be specified before other axes
      {
         joypad._offset_y=offset;
      }
   }

   return DIENUM_CONTINUE;
}
static BOOL CALLBACK EnumJoypads(const DIDEVICEINSTANCE *DIDevInst, void*)
{
   U16  vendor_id=LOWORD(DIDevInst->guidProduct.Data1);
   U16 product_id=HIWORD(DIDevInst->guidProduct.Data1);
#if JP_X_INPUT
   if(!IsXInputDevice(DIDevInst->guidProduct)) // if we're using XInput then don't list this device here, it will be processed by XInput
#endif
#if JP_GAMEPAD_INPUT
   if(!IsGamePadInput(vendor_id, product_id)) // if we're using GamePadInput then don't list this device here, it will be processed by GamePadInput
#endif
   {
      UInt id=0; ASSERT(SIZE(DIDevInst->guidInstance)==SIZE(UID)); C UID &uid=(UID&)DIDevInst->guidInstance; REPA(uid.i)id^=uid.i[i];
      Bool added; Joypad &joypad=GetJoypad(id, added); if(added)
      {
         IDirectInputDevice8 *did=null;
         if(OK(InputDevices.DI->CreateDevice(DIDevInst->guidInstance, &did, null)))
         if(OK(did->SetDataFormat      (&c_dfDIJoystick)))
         if(OK(did->SetCooperativeLevel(App.window(), DISCL_EXCLUSIVE|DISCL_FOREGROUND)))
         {
            Swap(joypad._device, did);
            joypad.      _name=DIDevInst->tszProductName;
         #if JOYPAD_VENDOR_PRODUCT_ID
            joypad. _vendor_id= vendor_id;
            joypad._product_id=product_id;
         #endif

            // disable auto centering ?
            DIPROPDWORD dipdw; Zero(dipdw);
            dipdw.diph.dwSize      =SIZE(dipdw);
            dipdw.diph.dwHeaderSize=SIZE(DIPROPHEADER);
            dipdw.diph.dwHow       =DIPH_DEVICE;
            joypad._device->SetProperty(DIPROP_AUTOCENTER, &dipdw.diph);

            joypad._device->EnumObjects(EnumAxes, &joypad, DIDFT_AXIS);
         }
         if(!joypad._device)Joypads.removeData(&joypad, true); // if failed to create it then remove it
         RELEASE(did);
      }
   }
   return DIENUM_CONTINUE;
}
#endif
/******************************************************************************/
void ListJoypads()
{
#if JP_X_INPUT || JP_DIRECT_INPUT
   REPA(Joypads) // assume that all are disconnected
   {
      Joypad &jp=Joypads[i];
   #if JP_X_INPUT
      if(jp._xinput!=0xFF)jp._connected=false;
   #endif
   #if JP_DIRECT_INPUT
      if(jp._device)jp._connected=false;
   #endif
   }

   #if JP_GAMEPAD_INPUT && WINDOWS_OLD && 0
      if(GamepadStatics) this is handled in GamePadAdded/GamePadRemoved
      {
         Microsoft::WRL::ComPtr<ABI::Windows::Foundation::Collections::IVectorView<ABI::Windows::Gaming::Input::Gamepad*>> gamepads; GamepadStatics->get_Gamepads(&gamepads); if(gamepads)
         {
            unsigned gamepad_count=0; gamepads->get_Size(&gamepad_count); for(unsigned i=0; i<gamepad_count; i++)
            {
               Microsoft::WRL::ComPtr<ABI::Windows::Gaming::Input::IGamepad> gamepad; gamepads->GetAt(i, &gamepad); if(gamepad)
               {
               }
            }
         }
      }
   #endif
   #if JP_X_INPUT
      ASSERT(XUSER_MAX_COUNT==4);
      FREP  (XUSER_MAX_COUNT) // XInput supports only 4 gamepads (process in order)
      {
         XINPUT_STATE state; if(XInputGetState(i, &state)==ERROR_SUCCESS) // if returned valid input
         {
            Bool added; Joypad &joypad=GetJoypad(i, added); if(added) // index is used for the ID for XInput gamepads
            {
               joypad._xinput=i;
               joypad._name  =S+"X Gamepad #"+(i+1);
            }
         }
      }
   #endif
   #if JP_DIRECT_INPUT
      if(InputDevices.DI)InputDevices.DI->EnumDevices(DI8DEVCLASS_GAMECTRL, EnumJoypads, null, DIEDFL_ATTACHEDONLY/*|DIEDFL_FORCEFEEDBACK*/); // this would enumerate only devices with ForceFeedback
   #endif

   REPA(Joypads)if(!Joypads[i]._connected)Joypads.remove(i, true); // remove disconnected joypads
#elif MAC
	if(HidManager=IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone))
	{
      NSMutableDictionary *criteria[]=
      {
         JoypadCriteria(kHIDPage_GenericDesktop, kHIDUsage_GD_Joystick),
         JoypadCriteria(kHIDPage_GenericDesktop, kHIDUsage_GD_GamePad),
         JoypadCriteria(kHIDPage_GenericDesktop, kHIDUsage_GD_MultiAxisController),
      };
      NSArray *criteria_array=[NSArray arrayWithObjects: criteria[0], criteria[1], criteria[2], __null];
      IOHIDManagerSetDeviceMatchingMultiple(HidManager, (CFArrayRef)criteria_array);
      REPA(criteria)[criteria[i] release];

      IOHIDManagerScheduleWithRunLoop(HidManager, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
      IOReturn hid_open=IOHIDManagerOpen(HidManager, kIOHIDOptionsTypeNone);

      IOHIDManagerRegisterDeviceMatchingCallback(HidManager, JoypadAdded  , null);
      IOHIDManagerRegisterDeviceRemovalCallback (HidManager, JoypadRemoved, null);
      IOHIDManagerRegisterInputValueCallback    (HidManager, JoypadAction , null);
   }
#endif
}
void InitJoypads()
{
   if(LogInit)LogN("InitJoypads");

   ASSERT(JB_NUM         <=ELMS(Joypad::_button_name)
   &&     JB_UWP_NUM     <=ELMS(Joypad::_button_name)
   &&     JB_NINTENDO_NUM<=ELMS(Joypad::_button_name)
   &&     JB_DPAD_LEFT   < ELMS(Joypad::_button_name)
   &&     JB_DPAD_RIGHT  < ELMS(Joypad::_button_name)
   &&     JB_DPAD_DOWN   < ELMS(Joypad::_button_name)
   &&     JB_DPAD_UP     < ELMS(Joypad::_button_name)
   );
   // set this first so other codes can overwrite it
   Joypad::_button_name[ 0]="1";
   Joypad::_button_name[ 1]="2";
   Joypad::_button_name[ 2]="3";
   Joypad::_button_name[ 3]="4";
   Joypad::_button_name[ 4]="5";
   Joypad::_button_name[ 5]="6";
   Joypad::_button_name[ 6]="7";
   Joypad::_button_name[ 7]="8";
   Joypad::_button_name[ 8]="9";
   Joypad::_button_name[ 9]="10";
   Joypad::_button_name[10]="11";
   Joypad::_button_name[11]="12";
   Joypad::_button_name[12]="13";
   Joypad::_button_name[13]="14";
   Joypad::_button_name[14]="15";
   Joypad::_button_name[15]="16";
   Joypad::_button_name[16]="17";
   Joypad::_button_name[17]="18";
   Joypad::_button_name[18]="19";
   Joypad::_button_name[19]="20";
   Joypad::_button_name[20]="21";
   Joypad::_button_name[21]="22";
   Joypad::_button_name[22]="23";
   Joypad::_button_name[23]="24";
   Joypad::_button_name[24]="25";
   Joypad::_button_name[25]="26";
   Joypad::_button_name[26]="27";
   Joypad::_button_name[27]="28";
   Joypad::_button_name[28]="29";
   Joypad::_button_name[29]="30";
   Joypad::_button_name[30]="31";
   Joypad::_button_name[31]="32";

   // set universal first
   Joypad::_button_name[JB_A]="A";
   Joypad::_button_name[JB_B]="B";
   Joypad::_button_name[JB_X]="X";
   Joypad::_button_name[JB_Y]="Y";

   Joypad::_button_name[JB_L1]=SWITCH ?  "L" : "LB";
   Joypad::_button_name[JB_R1]=SWITCH ?  "R" : "RB";
   Joypad::_button_name[JB_L2]=SWITCH ? "ZL" : "LT";
   Joypad::_button_name[JB_R2]=SWITCH ? "ZR" : "RT";

   Joypad::_button_name[JB_LTHUMB]="LThumb";
   Joypad::_button_name[JB_RTHUMB]="RThumb";

   Joypad::_button_name[JB_BACK ]="Back";
   Joypad::_button_name[JB_START]="Start";

   // set platform specific
#if WINDOWS
   Joypad::_button_name[JB_PADDLE1]="Paddle1";
   Joypad::_button_name[JB_PADDLE2]="Paddle2";
   Joypad::_button_name[JB_PADDLE3]="Paddle3";
   Joypad::_button_name[JB_PADDLE4]="Paddle4";
#endif

#if SWITCH
   Joypad::_button_name[JB_LSL]= "Left SL";
   Joypad::_button_name[JB_LSR]= "Left SR";
   Joypad::_button_name[JB_RSL]="Right SL";
   Joypad::_button_name[JB_RSR]="Right SR";

   Joypad::_button_name[JB_MINUS]="-",
   Joypad::_button_name[JB_PLUS ]="+",
#endif

   Joypad::_button_name[JB_DPAD_LEFT ]="DPadLeft" ;
   Joypad::_button_name[JB_DPAD_RIGHT]="DPadRight";
   Joypad::_button_name[JB_DPAD_DOWN ]="DPadDown" ;
   Joypad::_button_name[JB_DPAD_UP   ]="DPadUp"   ;

#if JP_GAMEPAD_INPUT && WINDOWS_OLD
   // !! SET ALL BEFORE ADDING CALLBACKS !!
   RoGetActivationFactory(Microsoft::WRL::Wrappers::HStringReference(RuntimeClass_Windows_Gaming_Input_RawGameController).Get(), __uuidof(ABI::Windows::Gaming::Input::IRawGameControllerStatics), &RawGameControllerStatics);
   RoGetActivationFactory(Microsoft::WRL::Wrappers::HStringReference(RuntimeClass_Windows_Gaming_Input_Gamepad          ).Get(), __uuidof(ABI::Windows::Gaming::Input::IGamepadStatics          ), &GamepadStatics          );
   RoGetActivationFactory(Microsoft::WRL::Wrappers::HStringReference(RuntimeClass_Windows_Gaming_Input_Gamepad          ).Get(), __uuidof(ABI::Windows::Gaming::Input::IGamepadStatics2         ), &GamepadStatics2         );
   // !! ADD CALLBACKS AFTER !!
   if(GamepadStatics)
   {
      GamepadStatics->add_GamepadAdded  (&GamePadAdded  , &GamePadAddedToken);
      GamepadStatics->add_GamepadRemoved(&GamePadRemoved, &GamePadRemovedToken);
   }
   if(RawGameControllerStatics)
   {
      RawGameControllerStatics->add_RawGameControllerAdded  (&RawGameControllerAdded  , &RawGameControllerAddedToken);
      RawGameControllerStatics->add_RawGameControllerRemoved(&RawGameControllerRemoved, &RawGameControllerRemovedToken);
   }
#endif

   ListJoypads();
}
void ShutJoypads()
{
#if JP_GAMEPAD_INPUT && WINDOWS_OLD
   // first remove callbacks
   if(GamepadStatics)
   {
      GamepadStatics->remove_GamepadAdded  (GamePadAddedToken);
      GamepadStatics->remove_GamepadRemoved(GamePadRemovedToken);
   }
   if(RawGameControllerStatics)
   {
      RawGameControllerStatics->remove_RawGameControllerAdded  (RawGameControllerAddedToken);
      RawGameControllerStatics->remove_RawGameControllerRemoved(RawGameControllerRemovedToken);
   }
   // now clear
   GamepadStatics          =null;
   GamepadStatics2         =null;
   RawGameControllerStatics=null;
#endif

   Joypads.del();

#if MAC
   if(HidManager)
   {
      IOHIDManagerClose(HidManager, kIOHIDOptionsTypeNone);
      CFRelease(HidManager); HidManager=null;
   }
#endif
}
/******************************************************************************/
}
/******************************************************************************/
