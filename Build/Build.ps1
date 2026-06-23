<#
.SYNOPSIS
Builds Rainmeter release artifacts.

.PARAMETER BuildType
The build target to run. Valid values are full, rainmeter-32, rainmeter-64, test-64, languages, and installer.

.PARAMETER Version
The release version in major.minor.subminor.revision format. Required for all build types except test-64.

.PARAMETER TestMode
Optional test file handling mode. Use include-tests to include unit tests in project builds; omitted by default.

.EXAMPLE
.\Build.ps1 full 1.2.3.4

Builds 32-bit Rainmeter, 64-bit Rainmeter, runs 64-bit tests, builds languages, and creates the installer.

.EXAMPLE
.\Build.ps1 rainmeter-64 1.2.3.4 include-tests

Builds only 64-bit Rainmeter and includes unit tests in the project build.
#>
[CmdletBinding()]
param(
	[Parameter(Position = 0)]
	[string]$BuildType,

	[Parameter(Position = 1)]
	[string]$Version,

	[Parameter(Position = 2)]
	[string]$TestMode
)

$ErrorActionPreference = 'Stop'

function Write-UsageError {
	param([string]$Message)

	Write-Error $Message
	exit 1
}

function Invoke-NativeCommand {
	param(
		[string]$FilePath,
		[string[]]$Arguments,
		[string]$WorkingDirectory = $PSScriptRoot,
		[string]$ErrorMessage
	)

	Push-Location $WorkingDirectory
	try {
		& $FilePath @Arguments
		if ($LASTEXITCODE -ne 0) {
			Write-Error "ERROR ${LASTEXITCODE}: Failed to run $FilePath"
			exit 1
		}
	} finally {
		Pop-Location
	}
}

function Add-VisualStudioBuildToolsToPath {
	# Speed up build by bypassing VS telemetry.
	$env:VSCMD_SKIP_SENDTELEMETRY = '1'

	$vcVarsAll = @('Community', 'Enterprise', 'BuildTools') |
		ForEach-Object { "C:\Program Files\Microsoft Visual Studio\18\$_\VC\Auxiliary\Build\vcvarsall.bat" } |
		Where-Object { Test-Path -LiteralPath $_ -PathType Leaf } |
		Select-Object -First 1
	if (-not $vcVarsAll) {
		Write-Error 'ERROR: vcvarsall.bat not found'
		exit 1
	}

	$vars = & cmd.exe /D /S /C "`"$vcVarsAll`" x64 > nul && set"
	if ($LASTEXITCODE -ne 0) {
		Write-Error "ERROR ${LASTEXITCODE}: vcvarsall.bat failed"
		exit 1
	}

	$vars | ForEach-Object {
		$_ | Select-String -Pattern '^([^=]+)=(.*)$' | ForEach-Object {
			$var = $_.Matches[0].Groups[1].Value
			$value = $_.Matches[0].Groups[2].Value
			Set-Item -Path "Env:$var" -Value $value
		}
	}
}

function Write-Utf8File {
	param(
		[string]$Path,
		[string[]]$Lines
	)

	$utf8NoBom = New-Object System.Text.UTF8Encoding($false)
	$content = [string]::Join("`r`n", $Lines) + "`r`n"
	[System.IO.File]::WriteAllText($Path, $content, $utf8NoBom)
}

$excludeTests = 'true'

if ($TestMode) {
	if ($TestMode -eq 'include-tests') {
		$excludeTests = 'false'
	} else {
		Write-UsageError 'Unknown test mode'
	}
}

switch ($BuildType) {
	'full' {}
	'rainmeter-32' {}
	'rainmeter-64' {}
	'test-64' {}
	'languages' { $Version = '0.0.0.0' }
	'installer' {}
	default { Write-UsageError 'Unknown build type' }
}

if ($BuildType -ne 'test-64') {
	if ([string]::IsNullOrWhiteSpace($Version)) {
		Write-UsageError 'Invalid version'
	}
	if ($Version -notmatch '^(\d+)[:.-](\d+)[:.-](\d+)[:.-](\d+)$') {
		Write-UsageError 'Invalid version'
	}

	$versionMajor = [int]$Matches[1]
	$versionMinor = [int]$Matches[2]
	$versionSubminor = [int]$Matches[3]
	$versionRevision = [int]$Matches[4]
	$versionShort = "$versionMajor.$versionMinor.$versionSubminor"
	$versionFull = "$versionShort.$versionRevision"
}

$now = Get-Date
$buildYear = $now.ToString('yyyy')
$buildTime = $now.ToString('yyyy-MM-dd HH:mm:ss')

Add-VisualStudioBuildToolsToPath

$msBuildArgs = @(
	'/nologo',
	"/p:ExcludeTests=$excludeTests",
	'/p:TrackFileAccess=false',
	'/p:Configuration=Release'
)

if ($BuildType -ne 'test-64' -and $BuildType -ne 'languages' -and $BuildType -ne 'installer') {
	Write-Host "* Starting $BuildType build for $versionFull"

	$versionHeaderLines = @(
		'#pragma once',
		"#define FILEVER $versionMajor,$versionMinor,$versionSubminor,$versionRevision",
		'#define PRODUCTVER FILEVER',
		"#define STRFILEVER `"$versionFull`"",
		'#define STRPRODUCTVER STRFILEVER',
		"#define APPVERSION L`"$versionShort`"",
		"#define RAINMETER_VERSION (($versionMajor * 1000000) + ($versionMinor * 1000) + $versionSubminor)",
		"#define BUILD_TIME L`"$buildTime`"",
		"#define STRCOPYRIGHT `"$buildYear Rainmeter Team`"",
		"const int revision_number = $versionRevision;"
	)
	if ($env:GITHUB_SHA) {
		$versionHeaderLines += "#define COMMIT_HASH L`"$env:GITHUB_SHA`""
	}

	Write-Utf8File (Join-Path $PSScriptRoot '..\Version.h') $versionHeaderLines

	Write-Utf8File (Join-Path $PSScriptRoot '..\Version.cs') @(
		'namespace Rainmeter',
		'{',
		'    public class Version',
		'    {',
		'#if X64',
		"        public const string Informational = `"$versionFull (64-bit)`";",
		'#else',
		"        public const string Informational = `"$versionFull (32-bit)`";",
		'#endif',
		'    }',
		'}'
	)
}

if ($BuildType -eq 'full' -or $BuildType -eq 'rainmeter-32') {
	Write-Host '* Building 32-bit projects'
	Invoke-NativeCommand 'msbuild.exe' ($msBuildArgs + @('/t:rebuild', '/p:Platform=Win32', '/v:q', '/m', '..\Rainmeter.sln'))
}

if ($BuildType -eq 'full' -or $BuildType -eq 'rainmeter-64') {
	Write-Host '* Building 64-bit projects'
	Invoke-NativeCommand 'msbuild.exe' ($msBuildArgs + @('/t:rebuild', '/p:Platform=x64', '/v:q', '/m', '..\Rainmeter.sln'))
}

if ($BuildType -eq 'full' -or $BuildType -eq 'test-64') {
	Write-Host '* Testing 64-bit projects'
	Invoke-NativeCommand 'vstest.console.exe' @('..\x64-Release\Obj\Common_Test\Common_Test.dll', '..\x64-Release\Rainmeter.dll', '/Platform:x64')
}

if ($BuildType -eq 'full' -or $BuildType -eq 'languages' -or $BuildType -eq 'installer') {
	Write-Host '* Building languages'
	Invoke-NativeCommand 'powershell.exe' @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', '.\GenerateLanguages.ps1')
}

if ($BuildType -eq 'full' -or $BuildType -eq 'installer') {
	Write-Host '* Building installer'

	$makeNsis = Join-Path $env:ProgramFiles 'NSIS\MakeNSIS.exe'
	if (-not (Test-Path -LiteralPath $makeNsis -PathType Leaf)) {
		$makeNsis = $makeNsis.Replace('Program Files\', 'Program Files (x86)\')
	}
	if (-not (Test-Path -LiteralPath $makeNsis -PathType Leaf)) {
		Write-Error 'ERROR: MakeNSIS.exe not found'
		exit 1
	}

	$installerPath = "Rainmeter-$versionFull.exe"
	$installerDefines = @(
		"/DOUTFILE=$installerPath",
		"/DVERSION_FULL=$versionFull",
		"/DVERSION_SHORT=$versionShort",
		"/DVERSION_REVISION=$versionRevision",
		"/DVERSION_MAJOR=$versionMajor",
		"/DVERSION_MINOR=$versionMinor",
		"/DBUILD_YEAR=$buildYear"
	)

	Invoke-NativeCommand $makeNsis ($installerDefines + @('/WX', '.\Installer\Installer.nsi'))
}

Write-Host
if (-not $env:CI) {
	Read-Host 'Press Enter to continue'
}
