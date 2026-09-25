$ErrorActionPreference = 'Stop'

. (Join-Path $PSScriptRoot 'VisualStudioBuildTools.ps1')

$vsInstallDir = @('Community', 'Enterprise', 'BuildTools') |
	ForEach-Object { "$($env:ProgramW6432)\Microsoft Visual Studio\18\$_\" } |
	Where-Object { Test-Path -LiteralPath (Join-Path $_ 'VC\Auxiliary\Build\vcvarsall.bat') -PathType Leaf } |
	Select-Object -First 1
if (-not $vsInstallDir) {
	throw 'vcvarsall.bat not found'
}

$original = @{}
Get-ChildItem Env: | ForEach-Object { $original[$_.Name] = $_.Value }

$targetedTimer = [System.Diagnostics.Stopwatch]::StartNew()
Add-VisualStudioBuildToolsToPath
$targetedTimer.Stop()

$targeted = @{}
Get-ChildItem Env: | ForEach-Object { $targeted[$_.Name] = $_.Value }

Get-ChildItem Env: | Where-Object { -not $original.ContainsKey($_.Name) } | ForEach-Object {
	Remove-Item -LiteralPath "Env:$($_.Name)"
}
foreach ($name in $original.Keys) {
	Set-Item -LiteralPath "Env:$name" -Value $original[$name]
}

$vcVarsAll = Join-Path $vsInstallDir 'VC\Auxiliary\Build\vcvarsall.bat'
$batchFile = Join-Path ([System.IO.Path]::GetTempPath()) ([System.IO.Path]::GetRandomFileName() + '.cmd')
$vcVarsAllTimer = [System.Diagnostics.Stopwatch]::StartNew()
try {
	Set-Content -LiteralPath $batchFile -Encoding Ascii -Value "@echo off`r`ncall `"$vcVarsAll`" x64 > nul || exit /b 1`r`nset"
	$referenceLines = & $batchFile
	$vcVarsAllTimer.Stop()
	if ($LASTEXITCODE -ne 0) {
		throw "ERROR $($LASTEXITCODE): vcvarsall.bat failed"
	}
}
finally {
	Remove-Item -LiteralPath $batchFile -ErrorAction SilentlyContinue
}

$reference = @{}
foreach ($line in $referenceLines) {
	if ($line -match '^([^=]+)=(.*)$') {
		$reference[$Matches[1]] = $Matches[2]
	}
}

Write-Host ('Targeted setup:      {0:N0} ms' -f $targetedTimer.Elapsed.TotalMilliseconds)
Write-Host ('vcvarsall.bat setup: {0:N0} ms' -f $vcVarsAllTimer.Elapsed.TotalMilliseconds)

$names = @(
	'INCLUDE', 'LIB', 'LIBPATH', 'VCINSTALLDIR', 'VCToolsInstallDir', 'VCToolsVersion',
	'WindowsSdkDir', 'WindowsSDKVersion', 'UniversalCRTSdkDir', 'UCRTVersion',
	'VisualStudioVersion', 'VSINSTALLDIR', 'VSCMD_ARG_HOST_ARCH', 'VSCMD_ARG_TGT_ARCH'
)
foreach ($name in $names) {
	$current = $targeted[$name]
	if ($reference[$name] -cne $current) {
		Write-Host "${name}:"
		Write-Host "  vcvarsall: $($reference[$name])"
		Write-Host "  targeted:  $current"
	}
}

$currentPaths = @($targeted['Path'] -split ';' | ForEach-Object { ($_ -replace '\\+', '\').TrimEnd('\') })
foreach ($path in ($reference['Path'] -split ';')) {
	$normalizedPath = ($path -replace '\\+', '\').TrimEnd('\')
	if ($path -match 'Microsoft Visual Studio|Windows Kits|Microsoft\.NET|MSBuild' -and $normalizedPath -notin $currentPaths) {
		Write-Host "PATH missing from targeted setup: $path"
	}
}
