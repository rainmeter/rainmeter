<#
.SYNOPSIS
Builds RainLexer release artifacts.

.PARAMETER BuildType
The build target to run. Valid values are full, test, rainlexer, rainlexer-32, rainlexer-64, and installer.

.PARAMETER Version
The RainLexer version in major.minor.patch format.

.EXAMPLE
.\Build.ps1 full 2.22.0

Builds both RainLexer DLLs and the installer.

.EXAMPLE
.\Build.ps1 installer 2.22.0

Builds only the RainLexer installer using the existing signed DLL outputs.

.EXAMPLE
.\Build.ps1 test 2.22.0

Builds and runs the lexer tests without producing any release artifacts.
#>
[CmdletBinding()]
param(
	[Parameter(Position = 0)]
	[ValidateSet('full', 'test', 'rainlexer', 'rainlexer-32', 'rainlexer-64', 'installer')]
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
		[string[]]$Lines,
		[string]$NewLine = "`r`n"
	)

	$utf8NoBom = New-Object System.Text.UTF8Encoding($false)
	$content = [string]::Join($NewLine, $Lines) + $NewLine
	[System.IO.File]::WriteAllText($Path, $content, $utf8NoBom)
}

function Write-LexerConfigs {
	$configRoot = Join-Path $PSScriptRoot '..\Config'
	$languages = Get-Content -LiteralPath (Join-Path $configRoot 'Languages.xml')

	# The keyword lists are the same for every theme, so each
	# Config\Generated\<Theme>\RainLexer.xml is stitched together from the shared
	# Languages.xml and the theme's own styles.
	$styleFiles = @(
		Get-ChildItem -Path $configRoot -Filter 'LexerStyles-*.xml' |
			Sort-Object -Property Name
	)
	if ($styleFiles.Count -eq 0) {
		Write-Error 'ERROR: No lexer config themes found'
		exit 1
	}

	foreach ($styleFile in $styleFiles) {
		$styles = Get-Content -LiteralPath $styleFile.FullName

		$themeName = $styleFile.BaseName.Substring('LexerStyles-'.Length)
		$themeDir = Join-Path (Join-Path $configRoot 'Generated') $themeName
		New-Item -ItemType Directory -Force -Path $themeDir | Out-Null

		Write-Utf8File (Join-Path $themeDir 'RainLexer.xml') (
			@('<?xml version="1.0" encoding="utf-8" ?>', '<NotepadPlus>') +
			$languages +
			$styles +
			@('</NotepadPlus>')
		) -NewLine "`n"
	}
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
Write-LexerConfigs

$msBuildArgs = @(
	'..\RainLexer.sln',
	'/nologo',
	'/m',
	'/t:Rebuild',
	'/p:Configuration=Release',
	'/v:q'
)

if ($BuildType -eq 'full' -or $BuildType -eq 'test') {
	Write-Host '* Building lexer tests'

	$testOutDir = Join-Path $PSScriptRoot '..\x64-Test'
	New-Item -ItemType Directory -Force -Path $testOutDir | Out-Null

	# Compiled straight with cl.exe rather than through a project of its own: the test
	# harness is three translation units and needs none of the DLL's link settings.
	# Asserts are deliberately left enabled, because Scintilla's LexAccessor checks its
	# own preconditions with them and those have caught real lexer faults.
	Invoke-NativeCommand 'cl.exe' @(
		'/nologo',
		'/std:c++20',
		'/permissive-',
		'/W4',
		'/O2',
		'/MT',
		'/EHsc',
		'/DUNICODE',
		'/D_UNICODE',
		'/DWIN32_LEAN_AND_MEAN',
		'/D_CRT_SECURE_NO_WARNINGS',
		'/I..\ThirdParty\Scintilla\include',
		'/I..\ThirdParty\lexilla\include',
		'/I..\ThirdParty\lexilla\lexlib',
		'/I..\RainLexer',
		'/I..\Test',
		'..\Test\TestRunner.cpp',
		'..\RainLexer\Lexer.cpp',
		'..\ThirdParty\lexilla\lexlib\WordList.cxx',
		'/Fo..\x64-Test\',
		'/Fe..\x64-Test\Test.exe'
	)

	Write-Host '* Running lexer tests'

	# Wrapped in @() so that a single case still comes back as an array.
	$cases = @(
		Get-ChildItem -Path (Join-Path $PSScriptRoot '..\Test\Cases') -Filter '*.ini' |
			Sort-Object -Property Name |
			ForEach-Object { $_.FullName }
	)
	if ($cases.Count -eq 0) {
		Write-Error 'ERROR: No lexer test cases found'
		exit 1
	}

	Invoke-NativeCommand '..\x64-Test\Test.exe' (
		@('..\Config\Languages.xml') + $cases
	)
}

if ($BuildType -eq 'full' -or $BuildType -eq 'rainlexer' -or $BuildType -eq 'rainlexer-32') {
	Write-Host '* Building 32-bit RainLexer'
	Invoke-NativeCommand 'msbuild.exe' ($msBuildArgs + '/p:Platform=Win32')
}

if ($BuildType -eq 'full' -or $BuildType -eq 'rainlexer' -or $BuildType -eq 'rainlexer-64') {
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
