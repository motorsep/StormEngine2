cd ..
del /s /q build64
mkdir build64
cd build64
cmake -G "Visual Studio 16 2019" -A x64 -DCMAKE_INSTALL_PREFIX=../bin/windows10-64 ../neo
pause