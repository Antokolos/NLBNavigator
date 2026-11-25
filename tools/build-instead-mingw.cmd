@echo off
rem Сборка INSTEAD 3.5.2 с настоящим SDL2 для локальных дифференциальных прогонов.
rem Требуется: NLB_TOOLS (каталог зависимостей), MinGW-w64, cmake в PATH,
rem исходники Lua 5.1.5, zlib 1.3.1 и INSTEAD 3.5.2 (см. WINDOWS_LOCAL.md).
setlocal
if "%NLB_TOOLS%"=="" (
  echo Задайте NLB_TOOLS — каталог с lua, zlib, sdl2-mingw и instead.
  exit /b 2
)
set "SRC=%NLB_TOOLS%\instead"
set "BUILD=%SRC%\build-mingw"
set "MINGW=%NLB_TOOLS%\mingw\mingw64\bin"
set "PATH=%MINGW%;%PATH%"

if not exist "%NLB_TOOLS%\lua\src-5.1.5\src\lapi.c" (
  echo Нет %NLB_TOOLS%\lua\src-5.1.5 — распакуйте lua-5.1.5.tar.gz.
  exit /b 2
)
if not exist "%SRC%\CMakeLists.txt" (
  echo Нет исходников INSTEAD в %SRC%.
  exit /b 2
)

echo === Lua 5.1.5 ===
cd /d "%NLB_TOOLS%\lua\src-5.1.5\src"
del /q *.o liblua51.a 2>nul
gcc -O2 -c lapi.c lcode.c ldebug.c ldo.c ldump.c lfunc.c lgc.c llex.c lmem.c lobject.c lopcodes.c lparser.c lstate.c lstring.c ltable.c ltm.c lundump.c lvm.c lzio.c lauxlib.c lbaselib.c ldblib.c liolib.c lmathlib.c loslib.c ltablib.c lstrlib.c loadlib.c linit.c > ..\build-lua.log 2>&1
if errorlevel 1 exit /b 1
ar rcs liblua51.a lapi.o lcode.o ldebug.o ldo.o ldump.o lfunc.o lgc.o llex.o lmem.o lobject.o lopcodes.o lparser.o lstate.o lstring.o ltable.o ltm.o lundump.o lvm.o lzio.o lauxlib.o lbaselib.o ldblib.o liolib.o lmathlib.o loslib.o ltablib.o lstrlib.o loadlib.o linit.o >> ..\build-lua.log 2>&1
if errorlevel 1 exit /b 1

echo === zlib ===
if exist "%NLB_TOOLS%\zlib\build-mingw" rmdir /s /q "%NLB_TOOLS%\zlib\build-mingw"
cmake -S "%NLB_TOOLS%\zlib\src\zlib-1.3.1" -B "%NLB_TOOLS%\zlib\build-mingw" -G "MinGW Makefiles" ^
  -DCMAKE_BUILD_TYPE=Release -DZLIB_BUILD_EXAMPLES=OFF ^
  -DCMAKE_MAKE_PROGRAM="%MINGW%\mingw32-make.exe" ^
  -DCMAKE_C_COMPILER="%MINGW%\gcc.exe" ^
  -DCMAKE_INSTALL_PREFIX="%NLB_TOOLS%\zlib\install-mingw" > "%NLB_TOOLS%\zlib\build-mingw.log" 2>&1
if errorlevel 1 exit /b 1
cmake --build "%NLB_TOOLS%\zlib\build-mingw" --target install >> "%NLB_TOOLS%\zlib\build-mingw.log" 2>&1
if errorlevel 1 exit /b 1

echo === INSTEAD ===
if exist "%BUILD%" rmdir /s /q "%BUILD%"
cmake -S "%SRC%" -B "%BUILD%" -G "MinGW Makefiles" ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_MAKE_PROGRAM="%MINGW%\mingw32-make.exe" ^
  -DCMAKE_C_COMPILER="%MINGW%\gcc.exe" ^
  -DSYSTEMWIDE=OFF -DSTANDALONE=OFF ^
  -DWITH_LUAJIT=OFF -DWITH_ICONV=OFF -DWITH_GTK2=OFF -DWITH_GTK3=OFF ^
  -DLUA_INCLUDE_DIR="%NLB_TOOLS%\lua\src-5.1.5\src" ^
  -DLUA_LIBRARY="%NLB_TOOLS%\lua\src-5.1.5\src\liblua51.a" ^
  -DLUA_LIBRARIES="%NLB_TOOLS%\lua\src-5.1.5\src\liblua51.a" ^
  -DSDL2_INCLUDE_DIR="%NLB_TOOLS%\sdl2-mingw\SDL2-2.32.10\x86_64-w64-mingw32\include\SDL2" ^
  -DSDL2_LIBRARY="%NLB_TOOLS%\sdl2-mingw\SDL2-2.32.10\x86_64-w64-mingw32\lib\libSDL2.dll.a" ^
  -DSDL2IMAGE_INCLUDE_DIR="%NLB_TOOLS%\sdl2-mingw\SDL2_image-2.8.12\x86_64-w64-mingw32\include\SDL2" ^
  -DSDL2IMAGE_LIBRARY="%NLB_TOOLS%\sdl2-mingw\SDL2_image-2.8.12\x86_64-w64-mingw32\lib\libSDL2_image.dll.a" ^
  -DSDL2TTF_INCLUDE_DIR="%NLB_TOOLS%\sdl2-mingw\SDL2_ttf-2.24.0\x86_64-w64-mingw32\include\SDL2" ^
  -DSDL2TTF_LIBRARY="%NLB_TOOLS%\sdl2-mingw\SDL2_ttf-2.24.0\x86_64-w64-mingw32\lib\libSDL2_ttf.dll.a" ^
  -DSDL2MIXER_INCLUDE_DIR="%NLB_TOOLS%\sdl2-mingw\SDL2_mixer-2.8.1\x86_64-w64-mingw32\include\SDL2" ^
  -DSDL2MIXER_LIBRARY="%NLB_TOOLS%\sdl2-mingw\SDL2_mixer-2.8.1\x86_64-w64-mingw32\lib\libSDL2_mixer.dll.a" ^
  -DZLIB_LIBRARY="%NLB_TOOLS%\zlib\install-mingw\lib\libzlibstatic.a" ^
  -DZLIB_INCLUDE_DIR="%NLB_TOOLS%\zlib\install-mingw\include" ^
  -DMATH_LIBRARY="%MINGW%\..\x86_64-w64-mingw32\lib\libm.a" > "%SRC%\build-mingw.configure.log" 2>&1
if errorlevel 1 exit /b 1
cmake --build "%BUILD%" --parallel 4 >> "%SRC%\build-mingw.build.log" 2>&1
if errorlevel 1 exit /b 1
copy /y "%NLB_TOOLS%\sdl2-mingw\SDL2-2.32.10\x86_64-w64-mingw32\bin\*.dll" "%BUILD%" >nul 2>&1
copy /y "%NLB_TOOLS%\sdl2-mingw\SDL2_image-2.8.12\x86_64-w64-mingw32\bin\*.dll" "%BUILD%" >nul 2>&1
copy /y "%NLB_TOOLS%\sdl2-mingw\SDL2_ttf-2.24.0\x86_64-w64-mingw32\bin\*.dll" "%BUILD%" >nul 2>&1
copy /y "%NLB_TOOLS%\sdl2-mingw\SDL2_mixer-2.8.1\x86_64-w64-mingw32\bin\*.dll" "%BUILD%" >nul 2>&1
copy /y "%MINGW%\libgcc_s_seh-1.dll" "%MINGW%\libstdc++-6.dll" "%MINGW%\libwinpthread-1.dll" "%BUILD%" >nul 2>&1
echo BUILD_OK: %BUILD%\src\sdl-instead.exe
