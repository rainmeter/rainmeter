<#
.SYNOPSIS
Builds RainLexer release artifacts.

.PARAMETER BuildType
The build target to run. Valid values are full, rainlexer-32, rainlexer-64, and installer.

.PARAMETER Version
The RainLexer version in major.minor.patch format.

.EXAMPLE
.\Build.ps1 full 2.22.0

Builds both RainLexer DLLs and the installer.

.EXAMPLE
.\Build.ps1 installer 2.22.0

Builds only the RainLexer installer using the existing signed DLL outputs.
#>
[CmdletBinding()]
param(
	[Parameter(Position = 0)]
	[ValidateSet('full', 'rainlexer-32', 'rainlexer-64', 'installer')]
	[string]$BuildType,

	[Parameter(Position = 1)]
	[string]$Version
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
		[string]$WorkingDirectory = $PSScriptRoot
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
	$env:VSCMD_SKIP_SENDTELEMETRY = '1'

	$vcVarsAll = @('Community', 'Enterprise', 'BuildTools') |
		ForEach-Object {
			"C:\Program Files\Microsoft Visual Studio\18\$_\VC\Auxiliary\Build\vcvarsall.bat"
		} |
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

function Write-VersionHeader {
	param([version]$ParsedVersion)

	Write-Utf8File (Join-Path $PSScriptRoot '..\RainLexer\Version.h') @(
		'#pragma once',
		(
			"#define RAINLEXER_VERSION_RC " +
			"$($ParsedVersion.Major),$($ParsedVersion.Minor),$($ParsedVersion.Build),0"
		),
		(
			"#define RAINLEXER_VERSION_STRING " +
			"`"$($ParsedVersion.Major).$($ParsedVersion.Minor).$($ParsedVersion.Build).0`""
		),
		(
			"#define RAINLEXER_TITLE " +
			"L`"RainLexer $($ParsedVersion.Major).$($ParsedVersion.Minor).$($ParsedVersion.Build)`""
		)
	)
}

if ([string]::IsNullOrWhiteSpace($Version)) {
	Write-UsageError 'Invalid version'
}

if ($Version -notmatch '^\d+\.\d+\.\d+$') {
	Write-UsageError 'Invalid version'
}

$parsedVersion = [version]$Version
$versionString = "$($parsedVersion.Major).$($parsedVersion.Minor).$($parsedVersion.Build)"

Add-VisualStudioBuildToolsToPath
Write-VersionHeader $parsedVersion

$msBuildArgs = @(
	'..\RainLexer.sln',
	'/nologo',
	'/m',
	'/t:Rebuild',
	'/p:Configuration=Release',
	'/v:q'
)

if ($BuildType -eq 'full' -or $BuildType -eq 'rainlexer-32') {
	Write-Host '* Building 32-bit RainLexer'
	Invoke-NativeCommand 'msbuild.exe' ($msBuildArgs + '/p:Platform=Win32')
}

if ($BuildType -eq 'full' -or $BuildType -eq 'rainlexer-64') {
	Write-Host '* Building 64-bit RainLexer'
	Invoke-NativeCommand 'msbuild.exe' ($msBuildArgs + '/p:Platform=x64')
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

	Invoke-NativeCommand $makeNsis @(
		"/DOUTFILE=RainLexer-$versionString.exe",
		"/DVERSION=$versionString",
		'..\Installer\Installer.nsi'
	)
}

Write-Host
if (-not $env:CI) {
	Read-Host 'Press Enter to continue'
}
