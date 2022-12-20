cmake
disable AVIF_ENABLE_GTEST
disable AVIF_ENABLE_WERROR
disable AVIF_CODEC_AOM_*
 enable AVIF_CODEC_DAV1D
disable BUILD_SHARED_LIBS

DAV1D_INCLUDE_DIR = C:\Esenthel\ThirdPartyLibs\dav1d\Windows\include
DAV1D_LIBRARY = C:\Esenthel\ThirdPartyLibs\dav1d\Windows\src\libdav1d.lib

C/C++ Code Gen
Multi-Threaded /MT