@echo off
setlocal EnableDelayedExpansion

:: For example, to build release 4.4.1 r3500, run: Build.bat release 4 4 1 3500
:: Parameters: build_type version_major version_minor version_subminor version_revision
:: |build_type|: release, pre, languages
:: Examples:
::		Build.bat final 4 4 0 3520		-> Rainmeter-4.4.0.exe
::		Build.bat pre 4 4 1 3521		-> Rainmeter-4.4.1-prerelease.exe
::		Build.bat languages				-> No installer, just update the language .dll files

set BUILD_TYPE=%1

if "%BUILD_TYPE%" == "languages" goto VERSION_OK

set /A VERSION_MAJOR=%2
set /A VERSION_MINOR=%3
set /A VERSION_SUBMINOR=%4
set /A VERSION_REVISION=%5

if "%BUILD_TYPE%" == "pre" goto BUILD_TYPE_OK
if "%BUILD_TYPE%" == "release" goto BUILD_TYPE_OK
echo Unknown build type & exit /b 1
:BUILD_TYPE_OK

if "%VERSION_MAJOR%" == "" echo ERROR: VERSION_MAJOR parameter missing & exit /b 1
if "%VERSION_MINOR%" == "" echo ERROR: VERSION_MINOR parameter missing & exit /b 1
if "%VERSION_SUBMINOR%" == "" echo ERROR: VERSION_SUBMINOR parameter missing & exit /b 1
if "%VERSION_REVISION%" == "" echo ERROR: VERSION_REVISION parameter missing & exit /b 1
:VERSION_OK

:: Visual Studio no longer creates the |%VSxxxCOMNTOOLS%| environment variable during install, so link
:: directly to the default location of "vcvarsall.bat" (Visual Studio 2022 Community)
set VCVARSALL=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat
set MAKENSIS=%PROGRAMFILES%\NSIS\MakeNSIS.exe

if not exist "%VCVARSALL%" set VCVARSALL=%VCVARSALL:Community\=Enterprise\%
if not exist "%VCVARSALL%" echo ERROR: vcvarsall.bat not found & exit /b 1

if not exist "%MAKENSIS%" set MAKENSIS=%MAKENSIS:Program Files\=Program Files (x86)\%
if not exist "%MAKENSIS%" echo ERROR: MakeNSIS.exe not found & exit /b 1

:: Begin timestamp
for /F "tokens=1-4 delims=:.," %%a in ("%TIME%") do (
	set /A "BUILD_BEGIN_TIMESTAMP=(((%%a * 60) + 1%%b %% 100)* 60 + 1%%c %% 100) * 100 + 1%%d %% 100"
)

echo Rainmeter Build
echo ----------------------------------------------
echo.

set BUILD_YEAR=%date:~-4%
set BUILD_TIME=%BUILD_YEAR%-%date:~4,2%-%date:~7,2% %time:~0,2%:%time:~3,2%:%time:~6,2%

call "%VCVARSALL%" x86 > nul

set MSBUILD="msbuild.exe" /nologo^
	/p:ExcludeTests=true^
	/p:TrackFileAccess=false^
	/p:Configuration=Release

if exist "Certificate.bat" call "Certificate.bat" > nul
set SIGNTOOL_SHA1="signtool.exe" sign /t http://timestamp.comodoca.com /f "%CERTFILE%" /p "%CERTKEY%"
set SIGNTOOL_SHA2="signtool.exe" sign /fd sha256 /tr http://timestamp.comodoca.com/?td=sha256 /td sha256 /f "%CERTFILE%" /p "%CERTKEY%"

if "%BUILD_TYPE%" == "languages" goto BUILDLANGUAGES

set VERSION_SHORT=%VERSION_MAJOR%.%VERSION_MINOR%.%VERSION_SUBMINOR%
set VERSION_FULL=%VERSION_SHORT%.%VERSION_REVISION%

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

echo * Updated Version.{cs,h}

echo * Starting build for %VERSION_FULL%

:: Build Library
echo * Building 32-bit projects
%MSBUILD% /t:rebuild /p:Platform=Win32 /v:q /m ..\Rainmeter.sln || (echo   ERROR %ERRORLEVEL%: Build failed & exit /b 1)

echo * Building 64-bit projects
%MSBUILD% /t:rebuild /p:Platform=x64 /v:q /m ..\Rainmeter.sln || (echo   ERROR %ERRORLEVEL%: Build failed & exit /b 1)

:BUILDLANGUAGES
echo * Building languages

:: Build all language libraries
>".\Installer\Languages.nsh" echo.
for /f "tokens=1,2,3 delims=," %%a in (..\Language\List) do (
	> "..\Language\Language.rc" echo #include "%%a.h"
	>>"..\Language\Language.rc" echo #include "Resource.rc"
	>>".\Installer\Languages.nsh" echo ${IncludeLanguage} "%%b" "%%a"
	set "LANGDLL_PARAMS=!LANGDLL_PARAMS!'%%a -  ${LANGFILE_%%b_NAME}' '${LANG_%%b}' '${LANG_%%b_CP}' "
	set "LANGUAGE_IDS=!LANGUAGE_IDS!${LANG_%%b},"

	%MSBUILD% /t:Language /p:Platform=Win32;TargetName=%%c /v:q ..\Rainmeter.sln || (echo   ERROR: Building language %%a failed & exit /b 1)
)
>>".\Installer\Languages.nsh" echo ^^!define LANGDLL_PARAMS "%LANGDLL_PARAMS%"
>>".\Installer\Languages.nsh" echo ^^!define LANGUAGE_IDS "%LANGUAGE_IDS%"

:: Restore English
echo #include "English.h"> "..\Language\Language.rc"
echo #include "Resource.rc">> "..\Language\Language.rc"
if "%BUILD_TYPE%" == "languages" (
	xcopy /Q /S /Y ..\x32-Release\Languages\*.dll ..\x64-Release\Languages\ > nul
	xcopy /Q /S /Y ..\x32-Release\Languages\*.dll ..\x32-Debug\Languages\ > nul
	xcopy /Q /S /Y ..\x32-Release\Languages\*.dll ..\x64-Debug\Languages\ > nul
	goto DONE
)

:: Sign binaries
if not "%CERTFILE%" == "" (
	echo * Signing binaries
	for %%Z in (Rainmeter.dll Rainmeter.exe RestartRainmeter.exe SkinInstaller.exe) do (
		%SIGNTOOL_SHA2% ..\x32-Release\%%Z || (echo   ERROR %ERRORLEVEL%: Signing x32-Release\%%Z failed & exit /b 1)
		timeout 2 > nul
		%SIGNTOOL_SHA2% ..\x64-Release\%%Z || (echo   ERROR %ERRORLEVEL%: Signing x64-Release\%%Z failed & exit /b 1)
		timeout 2 > nul
	)
)

:: Build installer
echo * Building installer

set INSTALLER_PATH=Rainmeter-%VERSION_SHORT%.exe
if "%BUILD_TYPE%" == "pre" set INSTALLER_PATH=Rainmeter-%VERSION_FULL%-prerelease.exe

set INSTALLER_DEFINES=^
	/DOUTFILE="%INSTALLER_PATH%"^
	/DVERSION_FULL="%VERSION_FULL%"^
	/DVERSION_SHORT="%VERSION_SHORT%"^
	/DVERSION_REVISION="%VERSION_REVISION%"^
	/DVERSION_MAJOR="%VERSION_MAJOR%"^
	/DVERSION_MINOR="%VERSION_MINOR%"^
	/DBUILD_YEAR="%BUILD_YEAR%"

"%MAKENSIS%" %INSTALLER_DEFINES% /WX .\Installer\Installer.nsi || (echo   ERROR %ERRORLEVEL%: Building installer failed & exit /b 1)

:: Sign installer
if not "%CERTFILE%" == "" (
	echo * Signing installer
	%SIGNTOOL_SHA1% %INSTALLER_PATH% || (echo   ERROR %ERRORLEVEL%: Signing installer failed & exit /b 1)
	timeout 2 > nul
	%SIGNTOOL_SHA2% /as %INSTALLER_PATH% || (echo   ERROR %ERRORLEVEL%: Signing installer failed & exit /b 1)
)

:: Create winget manifest
if not "%BUILD_TYPE%" == "release" goto DONE
if exist "Manifest.bat" call "Manifest.bat" > nul
if not "%MANIFEST_PATH%" == "" call ".\Winget\BuildManifest.bat"

:DONE
for /F "tokens=1-4 delims=:.," %%a in ("%TIME%") do (
	set /A "BUILD_END_TIMESTAMP=(((%%a * 60) + 1%%b %% 100)* 60 + %%c %% 100) * 100 + 1%%d %% 100"
)
set /A "BUILD_ELAPSED_TIME=(%BUILD_END_TIMESTAMP% - %BUILD_BEGIN_TIMESTAMP%) / 100"
echo * Build complete. Elapsed time: %BUILD_ELAPSED_TIME% sec

:END
if exist ".\Installer\Languages.nsh" del ".\Installer\Languages.nsh"
echo.
pause
