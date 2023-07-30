@echo off

call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

echo ----------Starting Debug Build-------------
if not exist "builds\Windows(Debug)\" mkdir "builds\Windows(Debug)"

rmdir /Q /S builds\Windows(Debug)\resources\

if not exist "..\build temp\" mkdir "..\build temp"

pushd "..\build temp"

cl -nologo /D_HAS_EXCEPTIONS=0 /W3 /Zi "..\think\code\main.cpp" -I"..\think\libraries\include" /D"DEBUG" /D"WINDOWS" /link /OUT:"..\think\builds\Windows(Debug)\Think.exe" kernel32.lib user32.lib gdi32.lib SDL2.lib SDL2main.lib opengl32.lib glew32.lib shell32.lib /subsystem:console /LIBPATH:"..\think\libraries\lib" && Xcopy /I /E "..\think\resources" "..\think\builds\Windows(Debug)\resources\" /Q

popd

Xcopy /y "libraries\dll\SDL2.dll" "builds\Windows(Debug)\" /Q
Xcopy /y "libraries\dll\glew32.dll" "builds\Windows(Debug)\" /Q

if %ERRORLEVEL% == 0 (
   goto good
)

if ERRORLEVEL != 0 (
   goto bad
)

:good
   echo ----------Debug Build Finished-------------
   IF "%1"=="/r" run.bat
   goto end

:bad

   echo %ERRORLEVEL%
   echo ----------!Debug Build Unsuccessful!-------------
   goto end

:end


