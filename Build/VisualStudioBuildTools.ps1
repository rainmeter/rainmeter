function Add-VisualStudioBuildToolsToPath {
	param(
		[ValidateSet('x86', 'x64')]
		[string]$Architecture = 'x64'
	)

	$env:VSCMD_SKIP_SENDTELEMETRY = '1'

	$vcVarsAll = @('Community', 'Enterprise', 'BuildTools') |
		ForEach-Object { "C:\Program Files\Microsoft Visual Studio\18\$_\VC\Auxiliary\Build\vcvarsall.bat" } |
		Where-Object { Test-Path -LiteralPath $_ -PathType Leaf } |
		Select-Object -First 1
	if (-not $vcVarsAll) {
		throw 'vcvarsall.bat not found'
	}

	$vars = & cmd.exe /D /S /C ('"{0}" {1} > nul && set' -f $vcVarsAll, $Architecture)
	if ($LASTEXITCODE -ne 0) {
		throw "ERROR $($LASTEXITCODE): vcvarsall.bat failed"
	}

	$vars | ForEach-Object {
		$_ | Select-String -Pattern '^([^=]+)=(.*)$' | ForEach-Object {
			$var = $_.Matches[0].Groups[1].Value
			$value = $_.Matches[0].Groups[2].Value
			Set-Item -Path "Env:$var" -Value $value
		}
	}
}
