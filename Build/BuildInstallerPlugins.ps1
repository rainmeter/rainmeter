<#
.SYNOPSIS
Builds the NSIS plugins used by the Rainmeter installer.
#>
[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'

. (Join-Path $PSScriptRoot 'VisualStudioBuildTools.ps1')

function Invoke-NativeCommand {
	param(
		[string]$FilePath,
		[string[]]$Arguments,
		[string]$ErrorMessage
	)

	& $FilePath @Arguments 2>&1 | ForEach-Object { Write-Host $_ }
	$exitCode = $LASTEXITCODE
	if ($exitCode -ne 0) {
		throw "ERROR $($exitCode): $ErrorMessage"
	}
}

function Compile-CSource {
	param(
		[string]$Source,
		[string]$Object,
		[string[]]$Definitions = @()
	)

	$args = @(
		'/nologo', '/c', '/TC', '/O1', '/W3', '/GS-', '/Gy', '/Zl',
		'/D_UNICODE', '/DUNICODE', '/DNSISCALL=__stdcall', '/D_CRT_SECURE_NO_WARNINGS',
		'/D_CRT_NONSTDC_NO_WARNINGS', '/D_CRT_SECURE_NO_DEPRECATE', '/D_CRT_NON_CONFORMING_SWPRINTFS',
		"/I$script:includeRoot",
		"/Fo$Object"
	)
	$args += $Definitions | ForEach-Object { "/D$_" }
	$args += $Source
	Invoke-NativeCommand -FilePath 'cl.exe' -Arguments $args -ErrorMessage "Compilation failed for $Source"
	if (-not (Test-Path -LiteralPath $Object -PathType Leaf)) {
		throw "Compiler did not create $Object"
	}
}

function Compile-CppSource {
	param(
		[string]$Source,
		[string]$Object
	)

	$args = @(
		'/nologo', '/c', '/TP', '/O1', '/W3', '/wd4996', '/GS-', '/Gy', '/GR-', '/EHs-c-', '/Zl',
		'/D_UNICODE', '/DUNICODE', "/I$script:includeRoot", "/Fo$Object", $Source
	)
	Invoke-NativeCommand -FilePath 'cl.exe' -Arguments $args -ErrorMessage "Compilation failed for $Source"
	if (-not (Test-Path -LiteralPath $Object -PathType Leaf)) {
		throw "Compiler did not create $Object"
	}
}

function Compile-PluginSources {
	param(
		[string]$Name,
		[string[]]$Sources,
		[string[]]$Definitions = @()
	)

	$objectDir = Join-Path $script:objectRoot $Name
	New-Item -ItemType Directory -Path $objectDir -Force | Out-Null

	$objects = foreach ($source in $Sources) {
		$sourceFile = Get-Item -LiteralPath $source
		$object = Join-Path $objectDir ($sourceFile.BaseName + '.obj')
		Compile-CSource -Source $sourceFile.FullName -Object $object -Definitions $Definitions
		$object
	}

	$pluginApiObject = Join-Path $objectDir 'pluginapi.obj'
	Compile-CSource -Source (Join-Path $script:sourceRoot 'pluginapi.c') -Object $pluginApiObject
	@($objects) + @($pluginApiObject)
}

function Link-Plugin {
	param(
		[string]$Name,
		[string[]]$Objects,
		[string[]]$Libraries,
		[string[]]$Options = @(),
		[string]$EntryPoint = 'DllMain'
	)

	$output = Join-Path $script:outputDir "$Name.dll"
	$map = Join-Path $script:objectRoot "$Name.map"
	$arguments = [System.Collections.Generic.List[string]]::new()
	@(
		'/nologo',
		'/DLL',
		"/ENTRY:$EntryPoint",
		'/OPT:REF',
		'/OPT:ICF,9',
		'/NOCOFFGRPINFO',
		"/OUT:$output",
		"/MAP:$map"
	) | ForEach-Object { $arguments.Add($_) }
	$Options | ForEach-Object { $arguments.Add($_) }
	$Objects | ForEach-Object {
		$inputFile = (Resolve-Path -LiteralPath $_).Path
		$arguments.Add($inputFile)
	}
	$Libraries | ForEach-Object { $arguments.Add($_) }
	Invoke-NativeCommand -FilePath 'link.exe' -Arguments $arguments.ToArray() -ErrorMessage "Linking failed for $Name.dll"
}

Add-VisualStudioBuildToolsToPath -Architecture x86

$repoRoot = Split-Path -Parent $PSScriptRoot
$sourceRoot = Join-Path $repoRoot 'ThirdParty\NSIS'
$includeRoot = Join-Path $repoRoot 'ThirdParty'
$outputDir = Join-Path $repoRoot 'BuildOut\Installer\Plugins\x86-unicode'
$objectRoot = Join-Path $repoRoot 'BuildOut\Installer\Plugins\Obj'
New-Item -ItemType Directory -Path $outputDir -Force | Out-Null
New-Item -ItemType Directory -Path $objectRoot -Force | Out-Null
Write-Host '* Building LangDLL'
$langDllDir = Join-Path $sourceRoot 'LangDLL'
$langDllObjects = Compile-PluginSources -Name 'LangDLL' -Sources @((Join-Path $langDllDir 'LangDLL.c'))
$langDllResource = Join-Path $objectRoot 'LangDLL.res'
Invoke-NativeCommand -FilePath 'rc.exe' -Arguments @('/nologo', "/fo$langDllResource", (Join-Path $langDllDir 'resource.rc')) -ErrorMessage 'Resource compilation failed for LangDLL'
[string[]]$langDllInputs = @($langDllObjects) + @($langDllResource)
Link-Plugin -Name 'LangDLL' -Objects $langDllInputs -Libraries @('kernel32.lib', 'user32.lib', 'gdi32.lib') -Options @('/NODEFAULTLIB')

Write-Host '* Building nsDialogs'
$nsDialogsDir = Join-Path $sourceRoot 'nsDialogs'
$nsDialogsObjects = Compile-PluginSources -Name 'nsDialogs' -Sources @(
	(Join-Path $nsDialogsDir 'browse.c'),
	(Join-Path $nsDialogsDir 'input.c'),
	(Join-Path $nsDialogsDir 'nsDialogs.c'),
	(Join-Path $nsDialogsDir 'rtl.c')
)
$nsDialogsResource = Join-Path $objectRoot 'nsDialogs.res'
Invoke-NativeCommand -FilePath 'rc.exe' -Arguments @('/nologo', "/fo$nsDialogsResource", (Join-Path $nsDialogsDir 'dialog.rc')) -ErrorMessage 'Resource compilation failed for nsDialogs'
[string[]]$nsDialogsInputs = @($nsDialogsObjects) + @($nsDialogsResource)
Link-Plugin -Name 'nsDialogs' -Objects $nsDialogsInputs -Libraries @('kernel32.lib', 'user32.lib', 'gdi32.lib', 'shell32.lib', 'comdlg32.lib', 'ole32.lib') -Options @('/NODEFAULTLIB', "/DEF:$(Join-Path $nsDialogsDir 'nsDialogs.def')")

Write-Host '* Building System'
$systemDir = Join-Path $sourceRoot 'System'
$systemObjects = Compile-PluginSources -Name 'System' -Sources @(
	(Join-Path $systemDir 'Buffers.c'),
	(Join-Path $systemDir 'Plugin.c'),
	(Join-Path $systemDir 'System.c')
) -Definitions @('SYSTEM_EXPORTS')
$callObject = Join-Path $objectRoot 'System\Call.obj'
Invoke-NativeCommand -FilePath 'ml.exe' -Arguments @('/nologo', '/c', '/coff', '/D_UNICODE', "/Fo$callObject", "/Ta$(Join-Path $systemDir 'Call.S')") -ErrorMessage 'Assembly failed for System'
[string[]]$systemInputs = @($systemObjects) + @($callObject)
Link-Plugin -Name 'System' -Objects $systemInputs -Libraries @('libcmt.lib', 'libvcruntime.lib', 'kernel32.lib', 'user32.lib', 'ole32.lib') -Options @('/NODEFAULTLIB')

Write-Host '* Building UAC'
$uacDir = Join-Path $sourceRoot 'UAC'
$uacObjectDir = Join-Path $objectRoot 'UAC'
New-Item -ItemType Directory -Path $uacObjectDir -Force | Out-Null
$uacObjects = foreach ($sourceName in @('uac.cpp', 'util.cpp')) {
	$source = Join-Path $uacDir $sourceName
	$object = Join-Path $uacObjectDir ([System.IO.Path]::ChangeExtension($sourceName, '.obj'))
	Compile-CppSource -Source $source -Object $object
	$object
}
Link-Plugin -Name 'UAC' -Objects $uacObjects -Libraries @('libcmt.lib', 'libvcruntime.lib', 'kernel32.lib', 'user32.lib', 'gdi32.lib', 'shell32.lib', 'advapi32.lib', 'ole32.lib') -Options @('/NODEFAULTLIB') -EntryPoint '_DllMainCRTStartup@12'

Write-Host ''
Write-Host '* Generated plugin sizes'
@('LangDLL.dll', 'nsDialogs.dll', 'System.dll', 'UAC.dll') | ForEach-Object {
	$dll = Get-Item -LiteralPath (Join-Path $outputDir $_)
	Write-Host ('  {0}: {1:N0} bytes ({2:N2} KiB)' -f $dll.Name, $dll.Length, ($dll.Length / 1KB))
}
