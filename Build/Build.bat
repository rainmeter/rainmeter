@echo off
setlocal EnableDelayedExpansion

set VCVARSALL=C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat
set MAKENSIS=%PROGRAMFILES%\NSIS\MakeNSIS.exe
set GIT=%PROGRAMFILES%\Git\bin\git.exe

:: Set VERSION_REVISION to non-zero value to override
set VERSION_MAJOR=4
set VERSION_MINOR=1
set VERSION_SUBMINOR=0
set VERSION_REVISION=0
set ISBETA=true

if "%1" == "RELEASE" set ISBETA=false
if "%1" == "BUILDVERSION" goto BUILDVERSION
echo Rainmeter Build
echo ----------------------------------------------
echo.

if not exist "%VCVARSALL%" echo ERROR: vcvarsall.bat not found & goto END
call "%VCVARSALL%" x86 > nul

set MSBUILD="msbuild.exe" /nologo^
	/p:ExcludeTests=true^
	/p:TrackFileAccess=false^
	/p:Configuration=Release

if exist "Certificate.bat" call "Certificate.bat" > nul
:: http://time.certum.pl/
:: http://timestamp.comodoca.com/authenticode
:: http://timestamp.verisign.com/scripts/timestamp.dll
:: http://timestamp.globalsign.com/scripts/timestamp.dll
set SIGNTOOL="signtool.exe" sign /t http://timestamp.comodoca.com/authenticode /f "%CERTFILE%" /p "%CERTKEY%"

if "%1" == "BUILDLANGUAGES" goto BUILDLANGUAGES

if exist "%MAKENSIS%" goto NSISFOUND
set MAKENSIS=%MAKENSIS:Program Files\=Program Files (x86)\%
if not exist "%MAKENSIS%" echo ERROR: MakeNSIS.exe not found & goto END
:NSISFOUND

:BUILDVERSION

if not exist "..\.git" goto UPDATEVERSION
if not "%VERSION_REVISION%" == "0" goto UPDATEVERSION

:: git
if exist "%GIT%" goto GITFOUND
set GIT=%GIT:Program Files\=Program Files (x86)\%
if exist "%GIT%" goto GITFOUND
set GIT=%LOCALAPPDATA%\Programs\Git\bin\GIT.exe
if not exist "%GIT%" echo ERROR: git.exe not found & goto END
:GITFOUND
set /a VERSION_REVISION=0
:: We really shouldn't be including revs from gh-pages, but it is too late to
:: change that now. We are also using `gh-page[s]` instead of just `gh-pages`
:: because Git adds ´/*´ to the end of the pattern unless it contains a
:: special glob char like [.
for /f "usebackq delims= " %%G in (`"%GIT%" rev-list --remotes^=origin/gh-page[s] --remotes^=origin/maste[r] --count`) do set VERSION_REVISION=%%G

:UPDATEVERSION

set VERSION_FULL=%VERSION_MAJOR%.%VERSION_MINOR%.%VERSION_SUBMINOR%.%VERSION_REVISION%
set VERSION_SHORT=%VERSION_MAJOR%.%VERSION_MINOR%
if not "%VERSION_SUBMINOR%" == "0" set VERSION_SHORT=!VERSION_SHORT!.%VERSION_SUBMINOR%

:: Update Version.h
> "..\Version.h" (
	echo #pragma once
	echo #define FILEVER %VERSION_MAJOR%,%VERSION_MINOR%,%VERSION_SUBMINOR%,%VERSION_REVISION%
	echo #define PRODUCTVER FILEVER
	echo #define STRFILEVER "%VERSION_FULL%"
	echo #define STRPRODUCTVER STRFILEVER
	echo #define APPVERSION L"%VERSION_MAJOR%.%VERSION_MINOR%.%VERSION_SUBMINOR%"
	echo #define RAINMETER_VERSION ((%VERSION_MAJOR% * 1000000^) + (%VERSION_MINOR% * 1000^) + %VERSION_SUBMINOR%^)
	echo const int revision_number = %VERSION_REVISION%;
	echo const bool revision_beta = %ISBETA%;
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


if "%1" == "BUILDVERSION" goto :eof
echo * Updated Version.h

:: Set vcbuild environment variables and begin build
echo * Starting build for %VERSION_FULL%
for /F "tokens=1-4 delims=:.," %%a in ("%TIME%") do (
	set /A "BUILD_BEGIN_TIMESTAMP=(((%%a * 60) + 1%%b %% 100)* 60 + 1%%c %% 100) * 100 + 1%%d %% 100"
)

:: Build Library
echo * Building 32-bit projects
%MSBUILD% /t:rebuild /p:Platform=Win32 /v:q /m ..\Rainmeter.sln > "BuildLog.txt"
if not %ERRORLEVEL% == 0 echo   ERROR %ERRORLEVEL%: Build failed & goto END

echo * Building 64-bit projects
%MSBUILD% /t:rebuild /p:Platform=x64 /v:q /m ..\Rainmeter.sln > "BuildLog.txt"
if not %ERRORLEVEL% == 0 echo   ERROR %ERRORLEVEL%: Build failed & goto END

:BUILDLANGUAGES
echo * Building languages

:: Build all language libraries
>".\Installer\Languages.nsh" echo.
for /f "tokens=1,2,3 delims=," %%a in (..\Language\List) do (
	> "..\Language\Language.rc" echo #include "%%a.h"
	>>"..\Language\Language.rc" echo #include "Resource.rc"
	>>".\Installer\Languages.nsh" echo ${IncludeLanguage} "%%b" "%%a"
	set LANGDLL_PARAMS='%%a -  ${LANGFILE_%%b_NAME}' '${LANG_%%b}' '${LANG_%%b_CP}' !LANGDLL_PARAMS!
	set LANGUAGE_IDS=${LANG_%%b},!LANGUAGE_IDS!

	%MSBUILD% /t:Language /p:Platform=Win32;TargetName=%%c /v:q ..\Rainmeter.sln > "BuildLog.txt"
	if not %ERRORLEVEL% == 0 echo   ERROR: Building language %%a failed & goto END
)
>>".\Installer\Languages.nsh" echo ^^!define LANGDLL_PARAMS "%LANGDLL_PARAMS%"
>>".\Installer\Languages.nsh" echo ^^!define LANGUAGE_IDS "%LANGUAGE_IDS%"

:: Restore English
echo #include "English.h"> "..\Language\Language.rc"
echo #include "Resource.rc">> "..\Language\Language.rc"
if "%1" == "BUILDLANGUAGES" (
	xcopy /Q /S /Y ..\x32-Release\Languages\*.dll ..\x64-Release\Languages\ > nul
	xcopy /Q /S /Y ..\x32-Release\Release\Languages\*.dll ..\x32-Debug\Languages\ > nul
	xcopy /Q /S /Y ..\x32-Release\Release\Languages\*.dll ..\x64-Debug\Languages\ > nul
	if exist "BuildLog.txt" del "BuildLog.txt"
	goto END
)

:: Sign binaries
if not "%CERTFILE%" == "" (
	echo * Signing binaries
	for %%Z in (Rainmeter.dll Rainmeter.exe SkinInstaller.exe) do (
		%SIGNTOOL% ..\x32-Release\%%Z > BuildLog.txt
		if not %ERRORLEVEL% == 0 echo   ERROR %ERRORLEVEL%: Signing x32\%%Z failed & goto END
		%SIGNTOOL% ..\x64-Release\%%Z > BuildLog.txt
		if not %ERRORLEVEL% == 0 echo   ERROR %ERRORLEVEL%: Signing x64\%%Z failed & goto END
	)
)

:: Build installer
echo * Building installer

set INSTALLER_NAME=Rainmeter-%VERSION_SHORT%.exe
if not "%1" == "RELEASE" set INSTALLER_NAME=Rainmeter-%VERSION_SHORT%-r%VERSION_REVISION%-beta.exe

set INSTALLER_DEFINES=^
	/DOUTFILE="%INSTALLER_NAME%"^
	/DVERSION_FULL="%VERSION_FULL%"^
	/DVERSION_SHORT="%VERSION_SHORT%"^
	/DVERSION_REVISION="%VERSION_REVISION%"
if not "%1" == "RELEASE" set INSTALLER_DEFINES=!INSTALLER_DEFINES! /DBETA

"%MAKENSIS%" %INSTALLER_DEFINES% .\Installer\Installer.nsi > "BuildLog.txt"
if not %ERRORLEVEL% == 0 echo   ERROR %ERRORLEVEL%: Building installer failed & goto END

:: Sign installer
if not "%CERTFILE%" == "" (
	echo * Signing installer
	%SIGNTOOL% %INSTALLER_NAME% > BuildLog.txt
	if not %ERRORLEVEL% == 0 echo   ERROR %ERRORLEVEL%: Signing installer failed & goto END
)

:: If we got here, build was successful so delete BuildLog.txt
if exist "BuildLog.txt" del "BuildLog.txt"

for /F "tokens=1-4 delims=:.," %%a in ("%TIME%") do (
	set /A "BUILD_END_TIMESTAMP=(((%%a * 60) + 1%%b %% 100)* 60 + %%c %% 100) * 100 + 1%%d %% 100"
)
set /A "BUILD_ELAPSED_TIME=(%BUILD_END_TIMESTAMP% - %BUILD_BEGIN_TIMESTAMP%) / 100"
echo * Build complete. Elapsed time: %BUILD_ELAPSED_TIME% sec

:END
if exist ".\Installer\Languages.nsh" del ".\Installer\Languages.nsh"
echo.
pause
