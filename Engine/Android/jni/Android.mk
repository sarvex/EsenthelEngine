LOCAL_PATH := $(call my-dir)

# Engine
include $(CLEAR_VARS)
LOCAL_MODULE           := Engine
LOCAL_SRC_FILES        := Animation.cpp Edit.cpp Code.cpp Code_Func_List.cpp File.cpp Game.cpp Game_Objects.cpp Game_Objects2.cpp Graphics.cpp Graphics2.cpp Graphics_Import.cpp Gui.cpp Gui_Objects.cpp Input.cpp Math.cpp Math_Shapes.cpp Memory.cpp Mesh.cpp Mesh_Import.cpp Mesh_MeshBase.cpp Misc.cpp Net.cpp Physics.cpp Platforms.cpp Sound.cpp Custom.cpp ../../Source/Graphics/Import/BC_Compress.cpp ../../Source/Graphics/Import/ETC_Compress.cpp ../../Source/Graphics/Import/PVRTC_Compress.cpp ../../Source/Graphics/ImageFilterWaifu.cpp ../../Source/Sound/VorbisEncoder.cpp ../../Source/Net/Sql.cpp
LOCAL_STATIC_LIBRARIES := android_native_app_glue cpufeatures
LOCAL_CPPFLAGS         := -I.. -I../H/_ -I../../ThirdPartyLibs -I../../ThirdPartyLibs/Bullet/lib/src -I../../ThirdPartyLibs/Ogg/include -I../../ThirdPartyLibs/Opus/lib/include -I../../ThirdPartyLibs/Opus/file/include -I../../ThirdPartyLibs/Vorbis/include -I../../ThirdPartyLibs/Theora/include -I../../ThirdPartyLibs/PhysX/physx/include -I../../ThirdPartyLibs/PhysX/pxshared/include -I../../ThirdPartyLibs/PhysX/physx/source/common/src -I../../ThirdPartyLibs/PhysX/physx/source/foundation/include -I../../ThirdPartyLibs/PhysX/physx/source/geomutils/src -I../../ThirdPartyLibs/FDK-AAC/lib/libAACdec/include -I../../ThirdPartyLibs/FDK-AAC/lib/libAACenc/include -I../../ThirdPartyLibs/FDK-AAC/lib/libFDK/include -I../../ThirdPartyLibs/FDK-AAC/lib/libMpegTPDec/include -I../../ThirdPartyLibs/FDK-AAC/lib/libMpegTPEnc/include -I../../ThirdPartyLibs/FDK-AAC/lib/libPCMutils/include -I../../ThirdPartyLibs/FDK-AAC/lib/libSBRdec/include -I../../ThirdPartyLibs/FDK-AAC/lib/libSBRenc/include -I../../ThirdPartyLibs/FDK-AAC/lib/libSYS/include -I../../ThirdPartyLibs/VP/libvpx/third_party/libwebm -fshort-wchar -ffast-math -fomit-frame-pointer -fpermissive -ffunction-sections -fdata-sections -fvisibility=hidden -ffriend-injection -fms-extensions -s -std=c++17 -Wno-invalid-offsetof -Wno-comment -Wno-parentheses -Wno-switch -Wno-address-of-temporary -Wno-null-dereference -Wno-int-to-void-pointer-cast -Wno-dynamic-class-memaccess -Wno-undefined-bool-conversion
LOCAL_CPP_FEATURES     := rtti
LOCAL_ARM_NEON         := true # force NEON usage for all files

include $(BUILD_STATIC_LIBRARY)

$(call import-module, android/native_app_glue)
$(call import-module, android/cpufeatures)
