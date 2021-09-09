@echo off
setlocal EnableDelayedExpansion

if not "%BUILD_TYPE%" == "release" echo   Notice: Only releases are uploaded to winget & goto :EOF

:: This is called from Build.bat (only if a Manifest.bat file exists)
if not exist "%INSTALLER_PATH%" echo   Invalid installer path: %INSTALLER_PATH% & goto :EOF

echo * Building winget manifest

:: Get parent folder
for %%a in ("%~dp0.") do for %%b in ("%%~dpa") do set INSTALLER_FULLPATH=%%~b
set INSTALLER_FULLPATH=!INSTALLER_FULLPATH!%INSTALLER_PATH%
if not exist "%INSTALLER_FULLPATH%" echo   ERROR: "%INSTALLER_FULLPATH%" not found & exit /b 1

:: Build powershell script
> "create-manifest.ps1" (
	echo $global:hash = ^(Get-FileHash -Algorithm SHA256 -Path '%INSTALLER_FULLPATH%' ^| Select -ExpandProperty Hash^)
	echo function ReplaceValues^($template_file, $output_file^)
	echo {
	echo   ^(Get-Content $template_file^) ^| Foreach-Object {
	echo     $_ -replace '{VERSION}', '%VERSION_SHORT%' `
	echo        -replace '{INSTALLERURL}', 'https://github.com/rainmeter/rainmeter/releases/download/v%VERSION_FULL%/Rainmeter-%VERSION_SHORT%.exe' `
	echo        -replace '{INSTALLERSHA256}', $hash `
	echo        -replace '{YEAR}', '%BUILD_YEAR%'
	echo   } ^| Set-Content $output_file
	echo }
	echo New-Item -ItemType Directory -Force -Path %MANIFEST_PATH%\%VERSION_SHORT%
	echo ReplaceValues -template_file '.\Winget\installer.yaml' -output_file '%MANIFEST_PATH%\%VERSION_SHORT%\Rainmeter.Rainmeter.installer.yaml'
	echo ReplaceValues -template_file '.\Winget\locale.yaml' -output_file '%MANIFEST_PATH%\%VERSION_SHORT%\Rainmeter.Rainmeter.locale.en-US.yaml'
	echo ReplaceValues -template_file '.\Winget\id.yaml' -output_file '%MANIFEST_PATH%\%VERSION_SHORT%\Rainmeter.Rainmeter.yaml'
)

powershell.exe -ExecutionPolicy Unrestricted -Command ". '.\create-manifest.ps1'" > nul
if exist ".\create-manifest.ps1" del ".\create-manifest.ps1"

start "" explorer.exe /e,/select,"%MANIFEST_PATH%\%VERSION_SHORT%"
