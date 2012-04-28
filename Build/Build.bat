@echo off
setlocal EnableDelayedExpansion

set VCVARSALL=%PROGRAMFILES%\Microsoft Visual Studio 10.0\VC\vcvarsall.bat
set MAKENSIS=%PROGRAMFILES%\NSIS\MakeNSIS.exe
set SUBWCREV=%PROGRAMFILES%\TortoiseSVN\bin\SubWCRev.exe
set GIT=%PROGRAMFILES%\Git\bin\git.exe
set VERSION=2.3.0
set REVISION=1
set ISBETA=true

if "%1" == "RELEASE" set ISBETA=false
if "%1" == "BUILDVERSION" goto BUILDVERSION
echo Rainmeter Build
echo ----------------------------------------------
echo.

if exist "%VCVARSALL%" goto VCFOUND
set VCVARSALL=%VCVARSALL:Program Files\=Program Files (x86)\%
if not exist "%VCVARSALL%" echo ERROR: vcvarsall.bat not found & goto END
:VCFOUND
call "%VCVARSALL%" x86 > nul
if "%1" == "BUILDLANGUAGES" goto BUILDLANGUAGES

if exist "%MAKENSIS%" goto NSISFOUND
set MAKENSIS=%MAKENSIS:Program Files\=Program Files (x86)\%
if not exist "%MAKENSIS%" echo ERROR: MakeNSIS.exe not found & goto END
:NSISFOUND

:BUILDVERSION

if exist "..\.svn" goto SVN
if exist "..\..\.svn" goto SVN
if not exist "..\.git" goto UPDATEVERSION

:: git
if exist "%GIT%" goto GITFOUND
set GIT=%GIT:Program Files\=Program Files (x86)\%
if not exist "%GIT%" echo ERROR: git.exe not found & goto END
:GITFOUND
set /a REVISION=0
for /f "usebackq delims= " %%G in (`"%GIT%" rev-list --all`) do set /a REVISION+=1
goto UPDATEVERSION

:: svn
:SVN
if exist "%SUBWCREV%" goto SUBWCREVFOUND
set SUBWCREV=%SUBWCREV:Program Files\=Program Files (x86)\%
if not exist "%SUBWCREV%" echo ERROR: SubWCRev.exe (TortoiseSVN) not found & goto END
:SUBWCREVFOUND
for /f "usebackq tokens=5 delims= " %%G in (`"%SUBWCREV%" ..\`) do set REVISION=%%G

:UPDATEVERSION

:: Update Version.h
> "..\Version.h" echo #pragma once
>>"..\Version.h" echo #define FILEVER %VERSION:~0,1%,%VERSION:~2,1%,%VERSION:~4,1%,%REVISION%
>>"..\Version.h" echo #define PRODUCTVER FILEVER
>>"..\Version.h" echo #define STRFILEVER "%VERSION%.%REVISION%"
>>"..\Version.h" echo #define STRPRODUCTVER STRFILEVER
>>"..\Version.h" echo #define APPVERSION L"%VERSION%"
>>"..\Version.h" echo #define RAINMETER_VERSION ((%VERSION:~0,1% * 1000000) + (%VERSION:~2,1% * 1000) + %VERSION:~4,1%)
>>"..\Version.h" echo const int revision_number = %REVISION%;
>>"..\Version.h" echo const bool revision_beta = %ISBETA%;

:: Update Version.cs
> "..\Version.cs" echo namespace Rainmeter
>>"..\Version.cs" echo {
>>"..\Version.cs" echo     public class Version
>>"..\Version.cs" echo     {
>>"..\Version.cs" echo #if X64
>>"..\Version.cs" echo         public const string Informational = "%VERSION%.%REVISION% (64-bit)";
>>"..\Version.cs" echo #else
>>"..\Version.cs" echo         public const string Informational = "%VERSION%.%REVISION% (32-bit)";
>>"..\Version.cs" echo #endif
>>"..\Version.cs" echo     }
>>"..\Version.cs" echo }


if "%1" == "BUILDVERSION" goto :eof
echo * Updated Version.h

:: Set vcbuild environment variables and begin build
echo * Starting build for %VERSION% r%REVISION%

:: Build Library
echo * Building 32-bit projects
"msbuild.exe" /t:rebuild /p:Configuration=Release;Platform=Win32 /m ..\Rainmeter.sln > "BuildLog.txt"
if not %ERRORLEVEL% == 0 echo   ERROR %ERRORLEVEL%: Build failed & goto END

echo * Building 64-bit projects
"msbuild.exe" /t:rebuild /p:Configuration=Release;Platform=x64 /m ..\Rainmeter.sln > "BuildLog.txt"
if not %ERRORLEVEL% == 0 echo   ERROR %ERRORLEVEL%: Build failed & goto END

:BUILDLANGUAGES
echo * Building languages

:: Build all language libraries
>".\Installer\Languages.nsh" echo.
for /f "tokens=1,2,3 delims=," %%a in (..\Language\List) do (
	> "..\Language\Language.rc" echo #include "%%a.h"
	>>"..\Language\Language.rc" echo #include "Resource.rc"
	>>".\Installer\Languages.nsh" echo ${IncludeLanguage} "%%b" "%%a"
	set LANGUAGES='%%a -  ${LANGFILE_%%b_NAME}' '${LANG_%%b}' '${LANG_%%b_CP}' !LANGUAGES!

	"msbuild.exe" /t:Language /p:Configuration=Release;Platform=Win32;TargetName=%%c ..\Rainmeter.sln > "BuildLog.txt"
	if not %ERRORLEVEL% == 0 echo   ERROR: Building language %%a failed & goto END
)
>>".\Installer\Languages.nsh" echo ^^!define LANGUAGES "%LANGUAGES%"

goto :eof

:: Restore English
echo #include "English.h"> "..\Language\Language.rc"
echo #include "Resource.rc">> "..\Language\Language.rc"
if "%1" == "BUILDLANGUAGES" (
	xcopy /Q /S /Y ..\TestBench\x32\Release\Languages\*.dll ..\TestBench\x64\Release\Languages\ > nul
	xcopy /Q /S /Y ..\TestBench\x32\Release\Languages\*.dll ..\TestBench\x32\Debug\Languages\ > nul
	xcopy /Q /S /Y ..\TestBench\x32\Release\Languages\*.dll ..\TestBench\x64\Debug\Languages\ > nul
	if exist "BuildLog.txt" del "BuildLog.txt"
	goto END
)

:: Sign binaries
if exist "Certificate.bat" (
	call "Certificate.bat" > nul
)
set SIGNTOOL="signtool.exe" sign /t http://time.certum.pl /f "%CERTFILE%" /p "%CERTKEY%"

if not "%CERTFILE%" == "" (
	echo * Signing binaries
	for %%Z in (Rainmeter.dll Rainmeter.exe SkinInstaller.exe) do (
		%SIGNTOOL% ..\TestBench\x32\Release\%%Z > BuildLog.txt
		if not %ERRORLEVEL% == 0 echo   ERROR %ERRORLEVEL%: Signing x32\%%Z failed & goto END
		%SIGNTOOL% ..\TestBench\x64\Release\%%Z > BuildLog.txt
		if not %ERRORLEVEL% == 0 echo   ERROR %ERRORLEVEL%: Signing x64\%%Z failed & goto END
	)
)

:: Build installer
echo * Building installer
if "%1" == "RELEASE" (
	"%MAKENSIS%" /DREV="%REVISION%" /DVER="%VERSION:~0,1%.%VERSION:~2,1%" .\Installer\Installer.nsi > "BuildLog.txt"
) else (
	"%MAKENSIS%" /DBETA /DREV="%REVISION%" /DVER="%VERSION:~0,1%.%VERSION:~2,1%" .\Installer\Installer.nsi > "BuildLog.txt"
)
if not %ERRORLEVEL% == 0 echo   ERROR %ERRORLEVEL%: Building installer failed & goto END

:: Sign installer
if not "%CERTFILE%" == "" (
	echo * Signing installer
	if "%1" == "RELEASE" (
		%SIGNTOOL% Rainmeter-%VERSION:~0,1%.%VERSION:~2,1%.exe > BuildLog.txt
	) else (
		%SIGNTOOL% Rainmeter-%VERSION:~0,1%.%VERSION:~2,1%-r%REVISION%-beta.exe > BuildLog.txt
	)
	if not %ERRORLEVEL% == 0 echo   ERROR %ERRORLEVEL%: Signing installer failed & goto END
)

:: If we got here, build was successful so delete BuildLog.txt
if exist "BuildLog.txt" del "BuildLog.txt"
echo * Build complete.

:END
if exist ".\Installer\Languages.nsh" del ".\Installer\Languages.nsh"
echo.
pause
