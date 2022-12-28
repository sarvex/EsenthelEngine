rmdir /s /q Windows\.vs
rmdir /s /q Windows\avif.dir
rmdir /s /q Windows\CMakeFiles
rmdir /s /q Windows\contrib
rmdir /s /q Windows\x64

rmdir /s /q Android\.vs
rmdir /s /q Android\Android-arm64-v8a
rmdir /s /q Android\avif.dir
rmdir /s /q Android\CMakeFiles
rmdir /s /q Android\contrib
rmdir /s /q Android\x64

rmdir /s /q Mac\build
rmdir /s /q Mac\CMakeFiles
rmdir /s /q Mac\CMakeScripts
rmdir /s /q Mac\contrib
rmdir /s /q Mac\Debug
del Mac\cmake_install.cmake
del Mac\CMakeCache.txt
del Mac\libavif.pc
del Mac\libavif-config-version.cmake
