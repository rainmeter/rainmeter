<#
.SYNOPSIS
Builds Rainmeter release artifacts.

.PARAMETER BuildTargets
Comma-separated build targets to run. Valid values are full, rainmeter, test, languages, plugin-api, and installer. The full target expands to all targets.

.PARAMETER Version
The release version in major.minor.subminor.revision format. Required for rainmeter and installer targets.

.PARAMETER IncludeTests
Includes unit tests in project builds; omitted by default.

.PARAMETER Rebuild
Rebuilds projects instead of building them incrementally; off by default.

.PARAMETER LTCG
Enables whole-program optimization and link-time code generation for release builds.

.PARAMETER OfficialBuild
Uses solid LZMA compression for the official installer.

.EXAMPLE
.\Build.ps1 full 1.2.3.4 -IncludeTests

Builds all Rainmeter components and creates the installer.

.EXAMPLE
.\Build.ps1 rainmeter 1.2.3.4 -IncludeTests

Builds Rainmeter and includes unit tests in the project build.
#>
[CmdletBinding()]
param(
	[Parameter(Position = 0)]
	[string[]]$BuildTargets,

	[Parameter(Position = 1)]
	[string]$Version,

	[switch]$IncludeTests,

	[switch]$Rebuild,

	[switch]$LTCG,

	[switch]$OfficialBuild
)

$ErrorActionPreference = 'Stop'

$msBuildTarget = if ($Rebuild) { 'rebuild' } else { 'build' }

$fullBuildTypes = @('rainmeter', 'test', 'plugin-api', 'languages', 'installer')
$BuildTypes = @(
	foreach ($buildTargetGroup in $BuildTargets) {
		foreach ($buildTarget in ($buildTargetGroup -split ',')) {
			$buildTarget = $buildTarget.Trim()
			if ($buildTarget -eq 'full') {
				$fullBuildTypes
			} else {
				$buildTarget
			}
		}
	}
)

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

$excludeTests = if ($IncludeTests) { 'false' } else { 'true' }

foreach ($buildType in $BuildTypes) {
	switch ($buildType) {
		'rainmeter' {}
		'test' {}
		'languages' {}
		'plugin-api' {}
		'installer' {}
		default { Write-UsageError "Unknown build type '$buildType'" }
	}
}

$versionedBuildTypes = @('rainmeter', 'installer')
$requiresVersion = @($BuildTypes | Where-Object { $versionedBuildTypes -contains $_ }).Count -gt 0

if ($requiresVersion) {
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

if ($BuildTypes -contains 'rainmeter') {
	Write-Host "* Starting $BuildTargets build for $versionFull"

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

if ($BuildTypes -contains 'rainmeter') {
	Write-Host '* Building 64-bit projects'
	Invoke-NativeCommand 'msbuild.exe' ($msBuildArgs + @("/t:$msBuildTarget", '/p:Platform=x64', '/v:q', '/m', '..\Rainmeter.sln')) -ErrorMessage '64-bit project build failed'
	Verify-RuntimeDependencies (Join-Path $PSScriptRoot '..\BuildOut\Release64')
}

if ($BuildTypes -contains 'test') {
	Write-Host '* Testing 64-bit projects'
	Invoke-NativeCommand 'vstest.console.exe' @('..\BuildOut\Release64\Obj\Common_Test\Common_Test.dll', '..\BuildOut\Release64\Rainmeter.dll', '/Platform:x64') -ErrorMessage '64-bit tests failed'
}

if ($BuildTypes -contains 'plugin-api') {
	Write-Host '* Building plugin API'

	$pluginApiDir = Join-Path $PSScriptRoot '..\BuildOut\PluginAPI\API'
	$solutionDir = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path + '\'

	# The import libraries come from the PluginAPI stub rather than from Exports.def directly,
	# since lib.exe cannot tell how the __stdcall functions are decorated from a name alone.
	Invoke-NativeCommand 'msbuild.exe' ($msBuildArgs + @('/t:rebuild', '/p:Platform=x64', "/p:SolutionDir=$solutionDir", '/v:q', '..\PluginAPI\PluginAPI.vcxproj')) -ErrorMessage 'x64 Plugin API build failed'
	$libDir = Join-Path $pluginApiDir 'x64'
	New-Item -ItemType Directory -Path $libDir -Force | Out-Null
	Copy-Item (Join-Path $solutionDir 'BuildOut\Release64\Obj\PluginAPI\Rainmeter.lib') $libDir

	Copy-Item (Join-Path $PSScriptRoot '..\Library\RainmeterAPI.h'), (Join-Path $PSScriptRoot '..\Library\RainmeterAPI.cs') $pluginApiDir
}

if ($BuildTypes -contains 'languages') {
	Write-Host '* Building languages'
	Invoke-NativeCommand 'powershell.exe' @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', '.\GenerateLanguages.ps1') -ErrorMessage 'Language build failed'
}

if ($BuildTypes -contains 'installer') {
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
	if ($OfficialBuild) {
		$installerDefines += '/DOFFICIALBUILD'
	}

	Invoke-NativeCommand $makeNsis ($installerDefines + @('/WX', '.\Installer\Installer.nsi')) -ErrorMessage 'Installer build failed'
}

Write-Host
if (-not $env:CI) {
	Read-Host 'Press Enter to continue'
}
