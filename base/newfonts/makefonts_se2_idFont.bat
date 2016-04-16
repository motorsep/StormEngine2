cd ..\..
SET BASEPATH=%CD%
cd base\newfonts

REM Do latin languages first
for %%i in ( *.?tf ) do idFont newfonts\%%i -fs_basepath %BASEPATH%