param(
	[Parameter(Mandatory = $true)]
	[string]$SourceDirectory,

	[Parameter(Mandatory = $true)]
	[string]$BuildDirectory,

	[Parameter(Mandatory = $true)]
	[string]$OutputLibrary,

	[Parameter(Mandatory = $true)]
	[string]$Configuration,

	[Parameter(Mandatory = $true)]
	[string]$Platform
)

$ErrorActionPreference = 'Stop'

$sourceDirectory = [IO.Path]::GetFullPath($SourceDirectory)
$buildDirectory = [IO.Path]::GetFullPath($BuildDirectory)
$outputLibrary = [IO.Path]::GetFullPath($OutputLibrary)
$buildSourceDirectory = Join-Path $buildDirectory 'src'

function Copy-DirectoryContents([string]$Source, [string]$Destination)
{
	New-Item -ItemType Directory -Path $Destination -Force | Out-Null
	Get-ChildItem -LiteralPath $Source -Force |
		Copy-Item -Destination $Destination -Recurse -Force
}

# Disable FFI and suppress warnings from LuaJIT on newer MSVC toolsets.
$env:LUAJIT_DISABLE_FFI = '1'
$env:CL = ("$env:CL /wd4244 /wd5287").Trim()

Copy-DirectoryContents (Join-Path $sourceDirectory 'src') $buildSourceDirectory
Copy-DirectoryContents (Join-Path $sourceDirectory 'dynasm') (Join-Path $buildDirectory 'dynasm')
Copy-Item -LiteralPath (Join-Path $sourceDirectory '.relver') -Destination $buildDirectory -Force

# Keep Git from treating the staging directory as part of the parent worktree.
$env:GIT_CEILING_DIRECTORIES = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
& git -C $buildDirectory apply --no-index --whitespace=nowarn (Join-Path $PSScriptRoot 'RainmeterLuaJIT.patch')
if ($LASTEXITCODE -ne 0)
{
	exit $LASTEXITCODE
}

if (-not (Select-String -LiteralPath (Join-Path $buildSourceDirectory 'msvcbuild.bat') -SimpleMatch 'LUAJIT_DISABLE_FFI' -Quiet))
{
	throw 'RainmeterLuaJIT.patch was not applied to the staged LuaJIT sources.'
}

$builtLibrary = Join-Path $buildSourceDirectory 'lua51.lib'
Remove-Item -LiteralPath $builtLibrary -Force -ErrorAction SilentlyContinue

$buildArguments = if ($Configuration -eq 'Debug') { 'debug static' } else { 'static' }
$msvcBuild = Join-Path $buildSourceDirectory 'msvcbuild.bat'
$buildCommand = "call `"$msvcBuild`" $buildArguments"

# ARM64EC can link LuaJIT's supported x64 output.
if ($Platform -eq 'ARM64EC')
{
	$vsDevCmd = Join-Path $env:VSINSTALLDIR 'Common7\Tools\VsDevCmd.bat'
	$buildCommand = "call `"$vsDevCmd`" -arch=x64 -host_arch=x64 -no_logo && $buildCommand"
}

Push-Location $buildSourceDirectory
try
{
	& $env:ComSpec /d /s /c $buildCommand
	if ($LASTEXITCODE -ne 0)
	{
		exit $LASTEXITCODE
	}
}
finally
{
	Pop-Location
}

if (-not (Test-Path -LiteralPath $builtLibrary -PathType Leaf))
{
	throw 'LuaJIT did not produce lua51.lib.'
}

$outputDirectory = Split-Path -Parent $outputLibrary
New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null
Copy-Item -LiteralPath $builtLibrary -Destination $outputLibrary -Force
