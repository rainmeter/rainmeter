@echo off
setlocal EnableDelayedExpansion

:: Parameters: type version
::
:: Available build types:
::		full          -> build everything
::		rainmeter-32  -> build 32-bit Rainmeter
::		rainmeter-64  -> build 64-bit Rainmeter
::		languages     -> build language .dll files for all targets
::		installer     -> build installer

set BUILD_TYPE=%1
set VERSION=%2

if "%BUILD_TYPE%" == "full" goto BUILD_TYPE_OK
if "%BUILD_TYPE%" == "rainmeter-32" goto BUILD_TYPE_OK
if "%BUILD_TYPE%" == "rainmeter-64" goto BUILD_TYPE_OK
if "%BUILD_TYPE%" == "languages" set VERSION=0.0.0.0 & goto BUILD_TYPE_OK
if "%BUILD_TYPE%" == "installer" goto BUILD_TYPE_OK
echo Unknown build type & exit /b 1
:BUILD_TYPE_OK

if "%VERSION%" == "" echo Invalid version & exit /b 1

for /F "tokens=1-4 delims=:.-" %%a in ("%VERSION%") do (
	set /A VERSION_MAJOR=%%a
	set /A VERSION_MINOR=%%b
	set /A VERSION_SUBMINOR=%%c
	set /A VERSION_REVISION=%%d
)

set VERSION_SHORT=%VERSION_MAJOR%.%VERSION_MINOR%.%VERSION_SUBMINOR%
set VERSION_FULL=%VERSION_SHORT%.%VERSION_REVISION%

set BUILD_YEAR=%date:~-4%
set BUILD_TIME=%BUILD_YEAR%-%date:~4,2%-%date:~7,2% %time:~0,2%:%time:~3,2%:%time:~6,2%

:: Visual Studio no longer creates the |%VSxxxCOMNTOOLS%| environment variable during install, so link
:: directly to the default location of "vcvarsall.bat" (Visual Studio 2022 Community)
set VCVARSALL=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat
if not exist "%VCVARSALL%" set VCVARSALL=%VCVARSALL:Community\=Enterprise\%
if not exist "%VCVARSALL%" echo ERROR: vcvarsall.bat not found & exit /b 1

:: Speed up build by bypassing VS telemetry
set VSCMD_SKIP_SENDTELEMETRY=1

call "%VCVARSALL%" x86 > nul

set MSBUILD="msbuild.exe" /nologo^
	/p:ExcludeTests=true^
	/p:TrackFileAccess=false^
	/p:Configuration=Release

if "%BUILD_TYPE%" == "languages" goto BUILD_LANGUAGES
if "%BUILD_TYPE%" == "installer" goto BUILD_INSTALLER

echo * Starting %BUILD_TYPE% build for %VERSION_FULL%

:: Update Version.h
> "..\Version.h" (
	echo #pragma once
	echo #define FILEVER %VERSION_MAJOR%,%VERSION_MINOR%,%VERSION_SUBMINOR%,%VERSION_REVISION%
	echo #define PRODUCTVER FILEVER
	echo #define STRFILEVER "%VERSION_FULL%"
	echo #define STRPRODUCTVER STRFILEVER
	echo #define APPVERSION L"%VERSION_SHORT%"
	echo #define RAINMETER_VERSION ((%VERSION_MAJOR% * 1000000^) + (%VERSION_MINOR% * 1000^) + %VERSION_SUBMINOR%^)
	echo #define BUILD_TIME L"%BUILD_TIME%"
	echo #define STRCOPYRIGHT "%BUILD_YEAR% Rainmeter Team"
	echo const int revision_number = %VERSION_REVISION%;
)
if not "%GITHUB_SHA%" == "" echo #define COMMIT_HASH L"%GITHUB_SHA%">> "..\Version.h"

:: Update Version.cs
> "..\Version.cs" (
	echo namespace Rainmeter
	echo {
	echo     public class Version
	echo     {
	echo #if X64
	echo         public const string Informational = "%VERSION_FULL% (64-bit^)";
	echo #else
	echo         public const string Informational = "%VERSION_FULL% (32-bit^)";
	echo #endif
	echo     }
	echo }
)

if "%BUILD_TYPE%" == "rainmeter-64" goto BUILD_RAINMETER_64

:BUILD_RAINMETER_32
echo * Building 32-bit projects
%MSBUILD% /t:rebuild /p:Platform=Win32 /v:q /m ..\Rainmeter.sln || (echo   ERROR %ERRORLEVEL%: Build failed & exit /b 1)

if "%BUILD_TYPE%" == "rainmeter-32" goto BUILD_LANGUAGES

:BUILD_RAINMETER_64
echo * Building 64-bit projects
%MSBUILD% /t:rebuild /p:Platform=x64 /v:q /m ..\Rainmeter.sln || (echo   ERROR %ERRORLEVEL%: Build failed & exit /b 1)

:BUILD_LANGUAGES
echo * Building languages

for /f "tokens=1,2,3 delims=," %%a in (..\Language\List) do (
	> "..\Language\Language.rc" echo #include "%%a.h"
	>>"..\Language\Language.rc" echo #include "Resource.rc"
	%MSBUILD% /t:Language /p:Platform=Win32;TargetName=%%c /v:q ..\Rainmeter.sln || (echo   ERROR: Building language %%a failed & exit /b 1)
)

:: Restore English
echo #include "English.h"> "..\Language\Language.rc"
echo #include "Resource.rc">> "..\Language\Language.rc"
if "%BUILD_TYPE%" == "languages" (
	xcopy /Q /S /Y ..\x32-Release\Languages\*.dll ..\x64-Release\Languages\ > nul
	xcopy /Q /S /Y ..\x32-Release\Languages\*.dll ..\x32-Debug\Languages\ > nul
	xcopy /Q /S /Y ..\x32-Release\Languages\*.dll ..\x64-Debug\Languages\ > nul
	goto DONE
)

if "%BUILD_TYPE%" == "rainmeter-32" goto DONE
if "%BUILD_TYPE%" == "rainmeter-64" goto DONE

:BUILD_INSTALLER
echo * Building installer

set MAKENSIS=%PROGRAMFILES%\NSIS\MakeNSIS.exe
if not exist "%MAKENSIS%" set MAKENSIS=%MAKENSIS:Program Files\=Program Files (x86)\%
if not exist "%MAKENSIS%" echo ERROR: MakeNSIS.exe not found & exit /b 1

set INSTALLER_PATH=Rainmeter-%VERSION_FULL%.exe

set INSTALLER_DEFINES=^
	/DOUTFILE="%INSTALLER_PATH%"^
	/DVERSION_FULL="%VERSION_FULL%"^
	/DVERSION_SHORT="%VERSION_SHORT%"^
	/DVERSION_REVISION="%VERSION_REVISION%"^
	/DVERSION_MAJOR="%VERSION_MAJOR%"^
	/DVERSION_MINOR="%VERSION_MINOR%"^
	/DBUILD_YEAR="%BUILD_YEAR%"

>".\Installer\Languages.nsh" echo.
for /f "tokens=1,2,3 delims=," %%a in (..\Language\List) do (
	>>".\Installer\Languages.nsh" echo ${IncludeLanguage} "%%b" "%%a"
	set "LANGDLL_PARAMS=!LANGDLL_PARAMS!'%%a -  ${LANGFILE_%%b_NAME}' '${LANG_%%b}' '${LANG_%%b_CP}' "
	set "LANGUAGE_IDS=!LANGUAGE_IDS!${LANG_%%b},"
)
>>".\Installer\Languages.nsh" echo ^^!define LANGDLL_PARAMS "%LANGDLL_PARAMS%"
>>".\Installer\Languages.nsh" echo ^^!define LANGUAGE_IDS "%LANGUAGE_IDS%"

"%MAKENSIS%" %INSTALLER_DEFINES% /WX .\Installer\Installer.nsi || (echo   ERROR %ERRORLEVEL%: Building installer failed & exit /b 1)

:DONE
echo.
if "%CI%" == "" pause
