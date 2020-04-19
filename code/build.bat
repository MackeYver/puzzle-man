@ECHO OFF
SETLOCAL ENABLEEXTENSIONS
SETLOCAL ENABLEDELAYEDEXPANSION
cls

SET DebugBuild=1

IF %1.==. (
echo To build in release configuration, enter 'release' as the first argument to build.bat.
echo - example: build.bat release
echo(
GOTO Bypass
)

IF %1==release (
SET DebugBuild=0
) ELSE (
echo To build in release configuration, enter 'release' as the first argument to build.bat.
echo - example: build.bat release
echo(
)

:Bypass

IF %errorlevel% NEQ 0 EXIT /b %errorlevel%

REM ----------------------------------------------------------------------------------
REM Compiler Options
REM https://docs.microsoft.com/en-us/cpp/build/reference/compiler-options-listed-alphabetically?view=vs-2017

SET IgnoredWarnings=/wd4100 /wd4201 /wd4505
SET CompilerOptions=/DWIN32=1 /nologo /MP /fp:fast /fp:except- /EHsc /Gm- /Oi /WX /W4 /Zi !IgnoredWarnings!

IF %DebugBuild%==1 (
SET CompilerOptions=/Od /MTd /DDEBUG=1 !CompilerOptions!
ECHO Building debug build.
) ELSE (
ECHO Building release build.
SET CompilerOptions=/O2 /DRELEASE=1 !CompilerOptions!
)


REM WL	       One line diagonostics
REM MP         Build with Multiple Processes
REM Ox	       Code generation x E [d = Debug, 1 = small code, 2 = fast code]
REM fp:fast    Fast floating point code generated
REM fp:except- No floating point exceptions
REM EHsc       Catches C++ exceptions only
REM GM-        Enables minimal rebuild (- disables it, we want all files compiled all the time)
REM Zi         Produces separate PDB file that contains all the symbolic debugging information for use with the debugger
REM Zo         Generate enhanced debugging information for optimized code in non-debug builds (enabled with Zi)
REM Oi	       Generates intrinsic functions.
REM WX	       Treats all warnings as errors
REM W4         All warnings
REM wx	       Except...
REM	           4100 'identifier': unreferenced formal parameter
REM		   4201 nonstandard extension used: nameless struct/union
REM		   4505 unreferenced local function
REM FC         Display full path of source code files passed to cl.exe in diagnostic text
REM GS	       Buffer security check
REM Gs	       Control stack checking calls
REM MTd        Defines _DEBUG and _MT, uses debug version of the crt



REM ----------------------------------------------------------------------------------
REM Linker Options
REM https://docs.microsoft.com/en-us/cpp/build/reference/linker-options?view=vs-2017

SET LinkerLibs=user32.lib gdi32.lib
REM Temp: winmm.lib kernel32.lib ole32.lib

SET AdditionalLinkerLibs=xaudio2_9redist.lib
SET AdditionalLinkerLibsPath="C:\developer\projects\Windows\puzzle-man\build\lib"

IF %DebugBuild%==1 (
SET LinkerOptions=/DEBUG:FULL /OPT:NOREF /OPT:NOICF /PROFILE
) ELSE (
SET LinkerOptions=/OPT:REF /OPT:ICF
)
SET LinkerOptions=!LinkerOptions! /MANIFEST:NO /INCREMENTAL:NO !LinkerLibs! /LIBPATH:!AdditionalLinkerLibsPath! !AdditionalLinkerLibs!

REM
REM We are setting /OPT for the linker explicitily just to be clear about what we are doing.
REM (OPT:REF and OPT:ICF is default for the linker and when using /DEBUG it changes to /OPT:NOREF and /OPT:NOICF)
REM
REM DEBUG:FULL     single PDB, also changes to opt:noref and opt:noicf
REM OPT:NOREF      keeps functions and data that are never referenced
REM OPT:NOICF      does not perform identical COMDAT folding
REM
REM MANIFEST:NO    Prevents the linker from adding a manifest to the exe (this is done "manually" further down)
REM INCREMENTAL:NO Link Incrementally is not selected

REM ----------------------------------------------------------------------------------
REM Build

IF NOT EXIST ..\build mkdir ..\build
PUSHD ..\build

ECHO Removing all old files...
del /Q *.*

ECHO Building...
cl %CompilerOptions% ../code/win32_main.cpp /link /SUBSYSTEM:windows %LinkerOptions% /out:puzzle-man.exe

IF %errorlevel% NEQ 0 (
  popd
  EXIT /b %errorlevel%
)

ECHO Embedding manifest...
mt.exe -nologo -manifest "../build/manifest/a.manifest" -outputresource:"puzzle-man.exe;#1"

IF %errorlevel% NEQ 0 (
  popd
  EXIT /b %errorlevel%
)

REM Move the resulting exe to the run_tree
IF NOT EXIST ..\run_tree mkdir ..\run_tree
move puzzle-man.exe ..\run_tree

ECHO All done.
POPD
