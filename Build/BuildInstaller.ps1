[CmdletBinding()]
param(
	[Parameter(Mandatory = $true)]
	[string]$Version,

	[Parameter(Mandatory = $true)]
	[string]$VersionShort,

	[Parameter(Mandatory = $true)]
	[string]$BuildYear,

	[switch]$UninstallerOnly
)

$ErrorActionPreference = 'Stop'

. (Join-Path $PSScriptRoot 'VisualStudioBuildTools.ps1')

function Invoke-NativeCommand {
	param([string]$FilePath, [string[]]$Arguments, [string]$ErrorMessage, [switch]$Quiet)

	$output = @(& $FilePath @Arguments 2>&1)
	$exitCode = $LASTEXITCODE
	if (-not $Quiet -or $exitCode -ne 0) {
		$output | ForEach-Object { Write-Host $_ }
	}
	if ($exitCode -ne 0) {
		throw "ERROR ${exitCode}: $ErrorMessage"
	}
}

function Add-ArchiveFile {
	param([System.Collections.Generic.List[object]]$Files, [string]$Source, [string]$Destination, [byte]$Architecture, [byte]$Flags = 0)

	if (-not (Test-Path -LiteralPath $Source -PathType Leaf)) {
		throw "Payload file not found: $Source"
	}
	[void]$Files.Add([pscustomobject]@{ Source = $Source; Destination = $Destination.Replace('\', '/'); Architecture = $Architecture; Flags = $Flags })
}

function Add-ArchiveDirectory {
	param([System.Collections.Generic.List[object]]$Files, [string]$Source, [string]$Destination)

	if (-not (Test-Path -LiteralPath $Source -PathType Container)) {
		throw "Payload directory not found: $Source"
	}
	$sourcePath = (Resolve-Path -LiteralPath $Source).Path.TrimEnd('\\')
	Get-ChildItem -LiteralPath $sourcePath -File -Recurse | Where-Object Name -ne '.DS_Store' | Sort-Object FullName | ForEach-Object {
		$relative = $_.FullName.Substring($sourcePath.Length + 1)
		Add-ArchiveFile -Files $Files -Source $_.FullName -Destination "$Destination/$relative" -Architecture 0
	}
}

function New-PayloadArchive {
	param([string]$Path, [string]$Uninstaller)

	$files = [System.Collections.Generic.List[object]]::new()
	$release32 = Join-Path $repoRoot 'BuildOut\Release32'
	$release64 = Join-Path $repoRoot 'BuildOut\Release64'
	foreach ($file in @('Rainmeter.exe', 'Rainmeter.dll', 'SkinInstaller.exe')) {
		Add-ArchiveFile -Files $files -Source (Join-Path $release32 $file) -Destination $file -Architecture 1
		Add-ArchiveFile -Files $files -Source (Join-Path $release64 $file) -Destination $file -Architecture 2
	}
	Add-ArchiveFile -Files $files -Source $Uninstaller -Destination 'uninst.exe' -Architecture 0 -Flags 1
	Add-ArchiveFile -Files $files -Source (Join-Path $repoRoot 'Application\Rainmeter.exe.config') -Destination 'Rainmeter.exe.config' -Architecture 0
	Add-ArchiveDirectory -Files $files -Source (Join-Path $release32 'Languages') -Destination 'Languages'
	Add-ArchiveDirectory -Files $files -Source (Join-Path $PSScriptRoot 'Skins') -Destination 'Defaults/Skins'
	Add-ArchiveDirectory -Files $files -Source (Join-Path $PSScriptRoot 'Layouts') -Destination 'Defaults/Layouts'

	$stream = [System.IO.File]::Open($Path, [System.IO.FileMode]::Create, [System.IO.FileAccess]::Write)
	$writer = [System.IO.BinaryWriter]::new($stream)
	try {
		$writer.Write([uint32]0x524D4252)
		$writer.Write([uint32]1)
		$writer.Write([uint32]$files.Count)
		foreach ($file in $files) {
			$pathBytes = [System.Text.Encoding]::UTF8.GetBytes($file.Destination)
			if ($pathBytes.Length -gt [uint16]::MaxValue) {
				throw "Payload path is too long: $($file.Destination)"
			}
			$contents = [System.IO.File]::ReadAllBytes($file.Source)
			$writer.Write([uint16]$pathBytes.Length)
			$writer.Write([byte]$file.Architecture)
			$writer.Write([byte]$file.Flags)
			$writer.Write([uint64]$contents.Length)
			$writer.Write($pathBytes)
			$writer.Write($contents)
		}
	} finally {
		$writer.Dispose()
	}
}

function New-MsvcrtImportLibrary {
	param([string]$OutputDirectory)

	# Visual Studio no longer ships this import library. The checked-in definition uses the stable C
	# export surface from the 32-bit Windows XP msvcrt.dll and omits its C++ runtime exports.
	$definition = Join-Path $PSScriptRoot 'msvcrt.def'
	$importLibrary = Join-Path $OutputDirectory 'msvcrt.lib'
	Invoke-NativeCommand -FilePath 'lib.exe' -Arguments @('/nologo', '/machine:x86', "/def:$definition", "/out:$importLibrary") -ErrorMessage 'Failed to create the msvcrt import library' -Quiet
	return $importLibrary
}

function Build-BrotliEncoder {
	param([string]$OutputDirectory)

	$objectDirectory = Join-Path $OutputDirectory 'Obj\Brotli'
	New-Item -ItemType Directory -Path $objectDirectory -Force | Out-Null
	$objects = [System.Collections.Generic.List[string]]::new()
	$sources = @(
		Get-ChildItem -LiteralPath (Join-Path $brotliDirectory 'common') -Filter '*.c' -File
		Get-ChildItem -LiteralPath (Join-Path $brotliDirectory 'dec') -Filter '*.c' -File
		Get-ChildItem -LiteralPath (Join-Path $brotliDirectory 'enc') -Filter '*.c' -File
		Get-Item -LiteralPath (Join-Path $brotliDirectory 'tools\brotli.c')
	)
	foreach ($source in $sources) {
		$object = Join-Path $objectDirectory ("Brotli-$($objects.Count).obj")
		$arguments = @('/nologo', '/c', '/O2', '/GL', '/MT', '/W3', '/DBROTLI_NO_STATIC_DICTIONARY', "/I$(Join-Path $brotliDirectory 'include')", "/Fo$object", $source.FullName)
		Invoke-NativeCommand -FilePath 'cl.exe' -Arguments $arguments -ErrorMessage "Compilation failed for $($source.FullName)" -Quiet
		[void]$objects.Add($object)
	}

	$output = Join-Path $OutputDirectory 'brotli.exe'
	Invoke-NativeCommand -FilePath 'link.exe' -Arguments (@('/nologo', '/LTCG', '/OPT:REF', '/OPT:ICF', "/OUT:$output") + $objects.ToArray()) -ErrorMessage 'Linking failed for brotli.exe' -Quiet
	return $output
}

function Build-Executable {
	param([string]$Name, [string[]]$Definitions)

	$objectDirectory = Join-Path $outputDirectory "Obj\$Name"
	New-Item -ItemType Directory -Path $objectDirectory -Force | Out-Null
	$objects = [System.Collections.Generic.List[string]]::new()
	$sources = @(
		(Join-Path $nativeDirectory 'Installer.cpp')
	)
	if ($Definitions -notcontains 'RM_UNINSTALLER') {
		$sources += @(
			(Join-Path $nativeDirectory 'BrotliDictionary.c'),
			(Join-Path $brotliDirectory 'common\constants.c'),
			(Join-Path $brotliDirectory 'common\context.c'),
			(Join-Path $brotliDirectory 'common\platform.c'),
			(Join-Path $brotliDirectory 'common\shared_dictionary.c'),
			(Join-Path $brotliDirectory 'common\transform.c'),
			(Join-Path $brotliDirectory 'dec\bit_reader.c'),
			(Join-Path $brotliDirectory 'dec\decode.c'),
			(Join-Path $brotliDirectory 'dec\huffman.c'),
			(Join-Path $brotliDirectory 'dec\prefix.c'),
			(Join-Path $brotliDirectory 'dec\state.c'),
			(Join-Path $brotliDirectory 'dec\static_init.c')
		)
	}
	foreach ($source in $sources) {
		$sourceName = [System.IO.Path]::GetFileNameWithoutExtension($source)
		$object = Join-Path $objectDirectory ($sourceName + '-' + $objects.Count + '.obj')
		$arguments = @('/nologo', '/c', '/O1', '/GL', '/GS-', '/Gy', '/Gw', '/Zl', '/W3', '/WX', '/wd4244', '/wd4334', '/DUNICODE', '/D_UNICODE', '/DWIN32_LEAN_AND_MEAN', '/DNOMINMAX', "/I$nativeDirectory", "/I$(Join-Path $repoRoot 'BuildOut\Installer')", "/I$(Join-Path $brotliDirectory 'include')", "/Fo$object")
		if ([System.IO.Path]::GetExtension($source) -eq '.cpp') {
			$arguments += @('/GR-', '/EHs-c-')
		}
		$arguments += $Definitions | ForEach-Object { "/D$_" }
		$arguments += $source
		Invoke-NativeCommand -FilePath 'cl.exe' -Arguments $arguments -ErrorMessage "Compilation failed for $source" -Quiet
		[void]$objects.Add($object)
	}

	$resource = Join-Path $objectDirectory 'Installer.res'
	$resourceDefinitions = @("/dVERSION_FULL=`"$Version`"", "/dVERSION_NUMERIC=$($Version.Replace('.', ','))", "/dBUILD_YEAR=`"$BuildYear`"")
	if ($Definitions -contains 'RM_UNINSTALLER') {
		$resourceDefinitions += '/dRM_UNINSTALLER'
	}
	Push-Location $nativeDirectory
	try {
		Invoke-NativeCommand -FilePath 'rc.exe' -Arguments (@('/nologo') + $resourceDefinitions + @("/fo$resource", 'Installer.rc')) -ErrorMessage 'Installer resource compilation failed'
	} finally {
		Pop-Location
	}
	$output = Join-Path $outputDirectory "$Name.exe"
	$arguments = @('/nologo', '/SUBSYSTEM:WINDOWS,6.02', '/ENTRY:wWinMainCRTStartup', '/LTCG', '/OPT:REF', '/OPT:ICF,9', '/NOCOFFGRPINFO', "/OUT:$output") + $objects.ToArray() + @($resource, 'kernel32.lib', 'user32.lib', 'advapi32.lib', 'shell32.lib', 'ole32.lib', 'oleaut32.lib', 'comctl32.lib', 'shlwapi.lib', 'uuid.lib', $msvcrtImportLibrary, 'libcmt.lib', '/NODEFAULTLIB')
	Invoke-NativeCommand -FilePath 'link.exe' -Arguments $arguments -ErrorMessage "Linking failed for $Name.exe" -Quiet
	return $output
}

Add-VisualStudioBuildToolsToPath -Architecture x86

$repoRoot = Split-Path -Parent $PSScriptRoot
$nativeDirectory = Join-Path $repoRoot 'Installer'
$brotliDirectory = Join-Path $repoRoot 'ThirdParty\brotli\c'
$outputDirectory = Join-Path $repoRoot 'BuildOut\Installer'
New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null
$msvcrtImportLibrary = New-MsvcrtImportLibrary -OutputDirectory $outputDirectory

$versionDefinitions = @("VERSION_FULL_W=L\`"$Version\`"", "VERSION_SHORT_W=L\`"$VersionShort\`"")
if ($UninstallerOnly) {
	# The uninstaller is built and signed first because its signed bytes are stored inside the final installer.
	$builtUninstaller = Build-Executable -Name 'uninst' -Definitions ($versionDefinitions + @('RM_UNINSTALLER'))
	Write-Host "Uninstaller for signing: $builtUninstaller"
	exit 0
}

$installerStub = Build-Executable -Name 'InstallerStub' -Definitions $versionDefinitions
$uninstaller = Join-Path $outputDirectory 'uninst.exe'
if (-not (Test-Path -LiteralPath $uninstaller -PathType Leaf)) {
	$uninstaller = Build-Executable -Name 'uninst' -Definitions ($versionDefinitions + @('RM_UNINSTALLER'))
}

$brotliPath = Build-BrotliEncoder -OutputDirectory $outputDirectory

$rawArchive = Join-Path $outputDirectory 'Payload.bin'
$compressedArchive = Join-Path $outputDirectory 'Payload.br'
New-PayloadArchive -Path $rawArchive -Uninstaller $uninstaller
$installer = Join-Path $PSScriptRoot "Rainmeter-$Version.exe"
Invoke-NativeCommand -FilePath $brotliPath -Arguments @('--quality=11', '--force', "--output=$compressedArchive", $rawArchive) -ErrorMessage 'Brotli compression failed' -Quiet
$output = [System.IO.File]::Open($installer, [System.IO.FileMode]::Create, [System.IO.FileAccess]::Write)
$writer = [System.IO.BinaryWriter]::new($output)
try {
	$stub = [System.IO.File]::ReadAllBytes($installerStub)
	$archive = [System.IO.File]::ReadAllBytes($compressedArchive)
	$writer.Write($stub)
	$writer.Write($archive)
	$writer.Write([uint32]0x524D454E)
	$writer.Write([uint32]1)
	$writer.Write([uint64]$stub.Length)
	$writer.Write([uint64]$archive.Length)
	$writer.Write([uint64](Get-Item -LiteralPath $rawArchive).Length)
} finally {
	$writer.Dispose()
}

$verification = Start-Process -FilePath $installer -ArgumentList '/VERIFYARCHIVE' -Wait -PassThru
if ($verification.ExitCode -ne 0) {
	throw 'The reduced Brotli decoder could not verify the installer archive'
}

Write-Host "Created $installer"
