@echo off

cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=D:/LibraryCode/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Debug

pause