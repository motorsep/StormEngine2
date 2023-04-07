cd ..
del /s /q build
mkdir build
cd build
cmake -G "Visual Studio 17 2022" -A Win32 -DCMAKE_INSTALL_PREFIX=../bin/windows10-32 ../neo
pause