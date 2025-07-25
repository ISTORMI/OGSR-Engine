@rem Script to build LuaJIT with MSVC.
@rem Copyright (C) 2005-2017 Mike Pall. See Copyright Notice in luajit.h
@rem
@rem Either open a "Visual Studio .NET Command Prompt"
@rem (Note that the Express Edition does not contain an x64 compiler)
@rem -or-
@rem Open a "Windows SDK Command Shell" and set the compiler environment:
@rem     setenv /release /x86
@rem   -or-
@rem     setenv /release /x64
@rem
@rem Then cd to this directory and run this script.

@if not defined INCLUDE goto :FAIL

@setlocal
@set LJCOMPILE=cl /nologo /MP /Zi /c /W3 /D_CRT_SECURE_NO_DEPRECATE /D_CRT_STDIO_INLINE=__declspec(dllexport)__inline
@set LJLINK=link /nologo /DEBUG
@set LJLIB=lib /nologo
@if "%3"=="Debug" goto :DBG_BUILD
@set LJCOMPILE=%LJCOMPILE% /O2 /Oi /Ot /GL /Gm- /Gy /Ob3 /MD
@set LJLINK=%LJLINK% /LARGEADDRESSAWARE /OPT:REF /OPT:ICF /LTCG /incremental:no
@set LJLIB=%LJLIB% /LTCG
@goto :COMMON_BUILD
:DBG_BUILD
@set LJCOMPILE=%LJCOMPILE% /Od /MTd
:COMMON_BUILD
@set LJMT=mt /nologo
@set DASMDIR=..\dynasm
@set DASM=%DASMDIR%\dynasm.lua
@set LJTARGETARCH=%1
@set DASC=vm_%LJTARGETARCH%.dasc
@set LJBINPATH=..\bin\
@if defined LJTARGETARCH (
  @set LJBINPATH=%LJBINPATH%%LJTARGETARCH%\
)
@if not exist %LJBINPATH% (
  @mkdir %LJBINPATH%
)
@set LJDLLNAME=%LJBINPATH%LuaJIT.dll
@set LJLIBNAME=%LJBINPATH%LuaJIT.lib
@set ALL_LIB=lib_base.c lib_math.c lib_bit.c lib_string.c lib_table.c lib_io.c lib_os.c lib_package.c lib_debug.c lib_jit.c lib_ffi.c lib_buffer.c

%LJCOMPILE% host\minilua.c
@if errorlevel 1 goto :BAD
%LJLINK% /out:minilua.exe minilua.obj
@if errorlevel 1 goto :BAD
if exist minilua.exe.manifest^
  %LJMT% -manifest minilua.exe.manifest -outputresource:minilua.exe

@set DASMFLAGS=-D WIN -D JIT -D FFI -D P64
@set LJARCH=x64
@minilua
@if errorlevel 8 goto :X64
@set DASMFLAGS=-D WIN -D JIT -D FFI
@set LJARCH=x86
:X64
@set LJCOMPILE=%LJCOMPILE% /DLUAJIT_ENABLE_GC64
minilua %DASM% -LN %DASMFLAGS% -o host\buildvm_arch.h %DASC%
@if errorlevel 1 goto :BAD

%LJCOMPILE% /I "." /I %DASMDIR% host\buildvm*.c
@if errorlevel 1 goto :BAD
%LJLINK% /out:buildvm.exe buildvm*.obj
@if errorlevel 1 goto :BAD
if exist buildvm.exe.manifest^
  %LJMT% -manifest buildvm.exe.manifest -outputresource:buildvm.exe

buildvm -m peobj -o lj_vm.obj
@if errorlevel 1 goto :BAD
buildvm -m bcdef -o lj_bcdef.h %ALL_LIB%
@if errorlevel 1 goto :BAD
buildvm -m ffdef -o lj_ffdef.h %ALL_LIB%
@if errorlevel 1 goto :BAD
buildvm -m libdef -o lj_libdef.h %ALL_LIB%
@if errorlevel 1 goto :BAD
buildvm -m recdef -o lj_recdef.h %ALL_LIB%
@if errorlevel 1 goto :BAD
buildvm -m vmdef -o jit\vmdef.lua %ALL_LIB%
@if errorlevel 1 goto :BAD
buildvm -m folddef -o lj_folddef.h lj_opt_fold.c
@if errorlevel 1 goto :BAD

@if "%2"=="amalg" goto :AMALGDLL
@if "%2"=="static" goto :STATIC
%LJCOMPILE% /DLUA_BUILD_AS_DLL lj_*.c lib_*.c
@if errorlevel 1 goto :BAD
%LJLINK% /DLL /out:%LJDLLNAME% lj_*.obj lib_*.obj
@if errorlevel 1 goto :BAD
@goto :MTDLL
:STATIC
%LJCOMPILE% lj_*.c lib_*.c
@if errorlevel 1 goto :BAD
%LJLIB% /OUT:%LJLIBNAME% lj_*.obj lib_*.obj
@if errorlevel 1 goto :BAD
@goto :MTDLL
:AMALGDLL
%LJCOMPILE% /DLUA_BUILD_AS_DLL ljamalg.c
@if errorlevel 1 goto :BAD
%LJLINK% /DLL /out:%LJDLLNAME% ljamalg.obj lj_vm.obj
@if errorlevel 1 goto :BAD
:MTDLL
if exist %LJDLLNAME%.manifest^
  %LJMT% -manifest %LJDLLNAME%.manifest -outputresource:%LJDLLNAME%;2

%LJCOMPILE% luajit.c
@if errorlevel 1 goto :BAD
%LJLINK% /out:%LJBINPATH%Lua_JIT.exe luajit.obj %LJLIBNAME%
@if errorlevel 1 goto :BAD
if exist %LJBINPATH%Lua_JIT.exe.manifest^
  %LJMT% -manifest %LJBINPATH%Lua_JIT.exe.manifest -outputresource:%LJBINPATH%Lua_JIT.exe

if not exist %LJBINPATH%lua\*.* (
	md %LJBINPATH%lua\
)
if not exist %LJBINPATH%lua\jit\*.* (
	md %LJBINPATH%lua\jit\
)
copy /Y jit\*.* %LJBINPATH%lua\jit\

@del *.obj *.manifest minilua.exe minilua.exp minilua.lib buildvm.exe buildvm.exp buildvm.lib jit\vmdef.lua *.ilk
@del host\buildvm_arch.h
@del lj_bcdef.h lj_ffdef.h lj_libdef.h lj_recdef.h lj_folddef.h
@echo.
@echo === Successfully built LuaJIT for Windows/%LJARCH% ===
@goto :END
:BAD
@echo.
@echo *******************************************************
@echo *** Build FAILED -- Please check the error messages ***
@echo *******************************************************
@goto :END
:FAIL
@echo You must open a "Visual Studio .NET Command Prompt" to run this script
:END
