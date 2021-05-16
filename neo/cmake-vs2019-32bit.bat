cd ..
del /s /q build
mkdir build
cd build
cmake -G "Visual Studio 16 2019" -A Win32 -DCMAKE_INSTALL_PREFIX=../bin/windows10-32 ../neo
pause