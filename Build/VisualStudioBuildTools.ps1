function Add-VisualStudioBuildToolsToPath {
	param(
		[ValidateSet('x86', 'x64')]
		[string]$Architecture = 'x64'
	)

	$env:VSCMD_SKIP_SENDTELEMETRY = '1'
	$env:VSCMD_ARG_HOST_ARCH = 'x64'
	$env:VSCMD_ARG_TGT_ARCH = $Architecture
	$env:VSCMD_ARG_APP_PLAT = 'Desktop'

	$vsInstallDir = @('Community', 'Enterprise', 'BuildTools') |
		ForEach-Object { "$($env:ProgramW6432)\Microsoft Visual Studio\18\$_\" } |
		Where-Object {
			(Test-Path -LiteralPath (Join-Path $_ 'Common7\Tools\vsdevcmd\ext\vcvars.bat') -PathType Leaf) -and
			(Test-Path -LiteralPath (Join-Path $_ 'Common7\Tools\vsdevcmd\core\winsdk.bat') -PathType Leaf)
		} |
		Select-Object -First 1
	if (-not $vsInstallDir) {
		throw 'Visual Studio build tools not found'
	}
	$env:VSINSTALLDIR = $vsInstallDir

	# These scripts set up the compiler and SDK faster than vcvarsall.bat.
	$batchFile = Join-Path ([System.IO.Path]::GetTempPath()) ([System.IO.Path]::GetRandomFileName() + '.cmd')
	try {
		Set-Content -LiteralPath $batchFile -Encoding Ascii -Value @'
@echo off
call "%VSINSTALLDIR%Common7\Tools\vsdevcmd\ext\vcvars.bat" > nul || exit /b 1
call "%VSINSTALLDIR%Common7\Tools\vsdevcmd\core\winsdk.bat" > nul || exit /b 1
if not defined INCLUDE set "INCLUDE=%__VSCMD_VCVARS_INCLUDE%%__VSCMD_WINSDK_INCLUDE%%__VSCMD_NETFX_INCLUDE%%INCLUDE%"
set
'@
		$vars = & $batchFile
		if ($LASTEXITCODE -ne 0) {
			throw "ERROR $($LASTEXITCODE): Visual Studio build tools setup failed"
		}
	}
	finally {
		Remove-Item -LiteralPath $batchFile -ErrorAction SilentlyContinue
	}

	$vars | ForEach-Object {
		$_ | Select-String -Pattern '^([^=]+)=(.*)$' | ForEach-Object {
			$var = $_.Matches[0].Groups[1].Value
			$value = $_.Matches[0].Groups[2].Value
			Set-Item -Path "Env:$var" -Value $value
		}
	}

	$msBuild = @('MSBuild\Current\Bin\amd64\MSBuild.exe', 'MSBuild\Current\Bin\MSBuild.exe') |
		ForEach-Object { Join-Path $vsInstallDir $_ } |
		Where-Object { Test-Path -LiteralPath $_ -PathType Leaf } |
		Select-Object -First 1
	if (-not $msBuild) {
		throw 'MSBuild.exe not found'
	}
	$env:MSBUILD_EXE = $msBuild
	$env:PATH = "$(Split-Path $msBuild);$env:PATH"
}
