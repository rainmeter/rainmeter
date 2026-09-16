<#
.SYNOPSIS
Prepares restored MSBuild intermediates for an incremental build.

.PARAMETER CacheDirectory
The intermediate directory restored by the CI cache.
#>
[CmdletBinding()]
param(
	[Parameter(Mandatory)]
	[string]$CacheDirectory
)

$ErrorActionPreference = 'Stop'

function ConvertTo-FileMap([string[]]$Entries) {
	$files = @{}
	foreach ($entry in $Entries) {
		$separator = $entry.IndexOf("`t")
		if ($separator -lt 0) {
			throw "Invalid git index entry: $entry"
		}

		$path = $entry.Substring($separator + 1)
		$files[$path] = $entry.Substring(0, $separator)
	}
	return $files
}

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$cacheDirectoryPath = [IO.Path]::GetFullPath((Join-Path $repoRoot $CacheDirectory))
$manifestPath = Join-Path $cacheDirectoryPath '.git-index'

$currentEntries = @(& git -C $repoRoot ls-files --stage)
if ($LASTEXITCODE -ne 0) {
	throw 'Failed to read the git index'
}

if (Test-Path -LiteralPath $manifestPath -PathType Leaf) {
	$cachedFiles = ConvertTo-FileMap ([IO.File]::ReadAllLines($manifestPath))
	$currentFiles = ConvertTo-FileMap $currentEntries
	$cacheTime = [DateTime]::UtcNow
	$changedFiles = 0

	Get-ChildItem -LiteralPath $cacheDirectoryPath -File -Recurse |
		Where-Object { $_.FullName -ne $manifestPath } |
		ForEach-Object { $_.LastWriteTimeUtc = $cacheTime }

	$changedTime = $cacheTime.AddSeconds(1)
	foreach ($file in $currentFiles.GetEnumerator()) {
		if (-not $cachedFiles.ContainsKey($file.Key) -or $cachedFiles[$file.Key] -ne $file.Value) {
			++$changedFiles
			$path = Join-Path $repoRoot $file.Key
			if (Test-Path -LiteralPath $path -PathType Leaf) {
				(Get-Item -LiteralPath $path).LastWriteTimeUtc = $changedTime
			}
		}
	}
	Write-Host "* Prepared build cache with $changedFiles changed files"
} else {
	Write-Host '* No reusable build cache found'
}

New-Item -ItemType Directory -Path $cacheDirectoryPath -Force | Out-Null
$utf8NoBom = New-Object System.Text.UTF8Encoding($false)
[IO.File]::WriteAllLines($manifestPath, $currentEntries, $utf8NoBom)
