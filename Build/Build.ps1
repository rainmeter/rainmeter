<#
.SYNOPSIS
Builds Rainmeter release artifacts.

.PARAMETER BuildType
The build target to run. Valid values are full, rainmeter-32, rainmeter-64, test-64, languages, plugin-api, and installer.

.PARAMETER Version
The release version in major.minor.subminor.revision format. Required for all build types except test-64.

.PARAMETER TestMode
Optional test file handling mode. Use include-tests to include unit tests in project builds; omitted by default.

.PARAMETER MSBuildTarget
Optional MSBuild target. Valid values are build and rebuild; defaults to rebuild.

.PARAMETER LTCG
Enables whole-program optimization and link-time code generation for release builds.

.PARAMETER X64Only
Builds an installer containing only 64-bit files.

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
	[string]$TestMode,

	[ValidateSet('build', 'rebuild')]
	[string]$MSBuildTarget = 'rebuild',

	[switch]$LTCG,

	[switch]$X64Only
)

$ErrorActionPreference = 'Stop'

. (Join-Path $PSScriptRoot 'VisualStudioBuildTools.ps1')

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
		$errorLines = [System.Collections.Generic.List[string]]::new()
		& $FilePath @Arguments 2>&1 | ForEach-Object {
			$line = $_.ToString()
			Write-Host $line
			if ($line -match '(?i)(?:^|:\s)(?:fatal\s+)?error(?:\s+[A-Z]+\d+)?\s*:') {
				$errorLines.Add($line)
				if ($errorLines.Count -gt 10) {
					$errorLines.RemoveAt(0)
				}
			}
		}
		$exitCode = $LASTEXITCODE
		if ($exitCode -ne 0) {
			if ($env:GITHUB_ACTIONS -eq 'true') {
				if ($errorLines.Count -gt 0) {
					$annotationMessage = [string]::Join("`n", $errorLines)
				} else {
					$annotationMessage = "$FilePath exited with code $exitCode"
				}
				$annotationMessage = $annotationMessage.Replace('%', '%25').Replace("`r", '%0D').Replace("`n", '%0A')
				Write-Host "::error title=${ErrorMessage}::$annotationMessage"
			}
			Write-Error "ERROR ${exitCode}: $ErrorMessage"
			exit 1
		}
	} finally {
		Pop-Location
	}
}

function Verify-RuntimeDependencies {
	param([string]$ReleaseDirectory)

	$binaries = Get-ChildItem -LiteralPath $ReleaseDirectory -Recurse -File |
		Where-Object { $_.Extension -match '(?i)^\.(?:dll|exe)$' }
	foreach ($binary in $binaries) {
		$dependencies = & dumpbin.exe /dependents $binary.FullName 2>&1
		if ($LASTEXITCODE -ne 0) {
			throw "Could not inspect DLL dependencies for '$($binary.FullName)'."
		}
		$unexpected = @($dependencies | Where-Object { $_ -match '(?i)\b(?:MSVCP|VCRUNTIME)[^\s]*\.dll\b' })
		if ($unexpected.Count -gt 0) {
			$names = $unexpected | ForEach-Object { $_.Trim() }
			throw "$($binary.FullName) links to disallowed runtime DLL(s): $([string]::Join(', ', $names))"
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

function Install-SignedInstallerPlugins {
	$pluginDir = Join-Path $PSScriptRoot '..\BuildOut\Installer\Plugins\x86-unicode'
	$pluginHashes = [ordered]@{
		'LangDLL.dll' = '15c83bf9daaa9e0fe20ee0f3ea4fc4b80bf6044173b5115dbba27c7c358bbafe'
		'nsDialogs.dll' = 'b06545d2dd59935da7c4c7f821503c4d06946224195a0e5a130573720563f38b'
		'System.dll' = '0f3b34e416967137f71da4b43975716b22546441ae3c8c730d14ff9882442d36'
		'UAC.dll' = '8319e659616ac168caa66f3d8d54eb0ac402f780ad618c10bfb6b50064f6ce75'
	}
	$pluginsNeedDownload = @($pluginHashes.GetEnumerator() | Where-Object {
		$path = Join-Path $pluginDir $_.Key
		-not (Test-Path -LiteralPath $path -PathType Leaf) -or (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash -ne $_.Value
	}).Count -gt 0

	if ($pluginsNeedDownload) {
		Write-Host '* Downloading signed NSIS plugins'
		New-Item -ItemType Directory -Path $pluginDir -Force | Out-Null
		$archive = Join-Path ([System.IO.Path]::GetTempPath()) ([System.IO.Path]::ChangeExtension([System.IO.Path]::GetRandomFileName(), '.zip'))
		try {
			Invoke-WebRequest 'https://github.com/rainmeter/build-tools/releases/download/v1/plugins.zip' -OutFile $archive
			Expand-Archive $archive -DestinationPath $pluginDir -Force
		} finally {
			Remove-Item -LiteralPath $archive -Force -ErrorAction SilentlyContinue
		}
	}

	foreach ($plugin in $pluginHashes.GetEnumerator()) {
		$path = Join-Path $pluginDir $plugin.Key
		if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
			throw "Signed NSIS plugin archive does not contain $($plugin.Key)"
		}
		$actualHash = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash
		if ($actualHash -ne $plugin.Value) {
			throw "SHA-256 mismatch for $($plugin.Key)"
		}
	}
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
	'plugin-api' {}
	'installer' {}
	default { Write-UsageError 'Unknown build type' }
}

if ($BuildType -ne 'test-64' -and $BuildType -ne 'plugin-api') {
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
	'/p:Configuration=Release'
)
if ($LTCG) {
	$msBuildArgs += '/p:EnableLTCG=true'
}

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
		"#define STRCOPYRIGHT `"\xA9 $buildYear Rainmeter Team`"",
		"const int revision_number = $versionRevision;"
	)
	if ($env:GITHUB_SHA) {
		$versionHeaderLines += "#define COMMIT_HASH L`"$env:GITHUB_SHA`""
	}

	Write-Utf8File (Join-Path $PSScriptRoot '..\Version.h') $versionHeaderLines
}

if ($BuildType -eq 'full' -or $BuildType -eq 'rainmeter-32') {
	Write-Host '* Building 32-bit projects'
	Invoke-NativeCommand 'msbuild.exe' ($msBuildArgs + @("/t:$MSBuildTarget", '/p:Platform=Win32', '/v:q', '/m', '..\Rainmeter.sln')) -ErrorMessage '32-bit project build failed'
	Verify-RuntimeDependencies (Join-Path $PSScriptRoot '..\BuildOut\Release32')
}

if ($BuildType -eq 'full' -or $BuildType -eq 'rainmeter-64') {
	Write-Host '* Building 64-bit projects'
	Invoke-NativeCommand 'msbuild.exe' ($msBuildArgs + @("/t:$MSBuildTarget", '/p:Platform=x64', '/v:q', '/m', '..\Rainmeter.sln')) -ErrorMessage '64-bit project build failed'
	Verify-RuntimeDependencies (Join-Path $PSScriptRoot '..\BuildOut\Release64')
}

if ($BuildType -eq 'full' -or $BuildType -eq 'test-64') {
	Write-Host '* Testing 64-bit projects'
	Invoke-NativeCommand 'vstest.console.exe' @('..\BuildOut\Release64\Obj\Common_Test\Common_Test.dll', '..\BuildOut\Release64\Rainmeter.dll', '/Platform:x64') -ErrorMessage '64-bit tests failed'
}

if ($BuildType -eq 'full' -or $BuildType -eq 'plugin-api') {
	Write-Host '* Building plugin API'

	$pluginApiDir = Join-Path $PSScriptRoot '..\BuildOut\PluginAPI\API'
	$solutionDir = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path + '\'

	# The import libraries come from the PluginAPI stub rather than from Exports.def directly,
	# since lib.exe cannot tell how the __stdcall functions are decorated from a name alone.
	foreach ($arch in ([ordered]@{ x32 = 'Win32'; x64 = 'x64' }).GetEnumerator()) {
		Invoke-NativeCommand 'msbuild.exe' ($msBuildArgs + @('/t:rebuild', "/p:Platform=$($arch.Value)", "/p:SolutionDir=$solutionDir", '/v:q', '..\PluginAPI\PluginAPI.vcxproj')) -ErrorMessage "$($arch.Key) Plugin API build failed"

		$libDir = Join-Path $pluginApiDir $arch.Key
		New-Item -ItemType Directory -Path $libDir -Force | Out-Null
		$outDirRoot = if ($arch.Value -eq 'Win32') { 'Release32' } else { 'Release64' }
		Copy-Item (Join-Path $solutionDir "BuildOut\$outDirRoot\Obj\PluginAPI\Rainmeter.lib") $libDir
	}

	Copy-Item (Join-Path $PSScriptRoot '..\Library\RainmeterAPI.h'), (Join-Path $PSScriptRoot '..\Library\RainmeterAPI.cs') $pluginApiDir
}

if ($BuildType -eq 'full' -or $BuildType -eq 'languages' -or $BuildType -eq 'installer') {
	Write-Host '* Building languages'
	Invoke-NativeCommand 'powershell.exe' @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', '.\GenerateLanguages.ps1') -ErrorMessage 'Language build failed'
}

if ($BuildType -eq 'full' -or $BuildType -eq 'installer') {
	Install-SignedInstallerPlugins

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
	if ($X64Only) {
		$installerDefines += '/DX64ONLY'
	}

	Invoke-NativeCommand $makeNsis ($installerDefines + @('/WX', '.\Installer\Installer.nsi')) -ErrorMessage 'Installer build failed'
}

Write-Host
if (-not $env:CI) {
	Read-Host 'Press Enter to continue'
}
