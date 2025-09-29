@echo off

cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE=D:/LibraryCode/vcpkg/scripts/buildsystems/vcpkg.cmake -D"CMAKE_EXPORT_COMPILE_COMMANDS=ON"
cmake --build build --config Debug

pause