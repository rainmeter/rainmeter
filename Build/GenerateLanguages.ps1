[CmdletBinding()]
param(
	[string[]]$Locale,
	[string[]]$OutputDirectory,
	[switch]$RuntimeOnly
)

$ErrorActionPreference = 'Stop'

# NSIS uses its own language names, while Rainmeter uses LCID. Note that English must be first
# because it's the default language.
$languages = [ordered]@{
	'en'      = @{ nsis = 'English'; lcid = 1033 }
	'ar'      = @{ nsis = 'Arabic'; lcid = 1025 }
	'bg'      = @{ nsis = 'Bulgarian'; lcid = 1026 }
	'zh-CN'   = @{ nsis = 'SimpChinese'; lcid = 2052 }
	'zh-TW'   = @{ nsis = 'TradChinese'; lcid = 1028 }
	'cs'      = @{ nsis = 'Czech'; lcid = 1029 }
	'da'      = @{ nsis = 'Danish'; lcid = 1030 }
	'nl'      = @{ nsis = 'Dutch'; lcid = 1043 }
	'et'      = @{ nsis = 'Estonian'; lcid = 1061 }
	'fi'      = @{ nsis = 'Finnish'; lcid = 1035 }
	'fr'      = @{ nsis = 'French'; lcid = 1036 }
	'de'      = @{ nsis = 'German'; lcid = 1031 }
	'el'      = @{ nsis = 'Greek'; lcid = 1032 }
	'he'      = @{ nsis = 'Hebrew'; lcid = 1037 }
	'hu'      = @{ nsis = 'Hungarian'; lcid = 1038 }
	'id'      = @{ nsis = 'Indonesian'; lcid = 1057 }
	'it'      = @{ nsis = 'Italian'; lcid = 1040 }
	'ja'      = @{ nsis = 'Japanese'; lcid = 1041 }
	'ko'      = @{ nsis = 'Korean'; lcid = 1042 }
	'ms'      = @{ nsis = 'Malay'; lcid = 1086 }
	'nb'      = @{ nsis = 'Norwegian'; lcid = 1044 }
	'pl'      = @{ nsis = 'Polish'; lcid = 1045 }
	'pt-BR'   = @{ nsis = 'PortugueseBR'; lcid = 1046 }
	'pt-PT'   = @{ nsis = 'Portuguese'; lcid = 2070 }
	'ro'      = @{ nsis = 'Romanian'; lcid = 1048 }
	'ru'      = @{ nsis = 'Russian'; lcid = 1049 }
	'sr-Cyrl' = @{ nsis = 'Serbian'; lcid = 3098 }
	'sr-Latn' = @{ nsis = 'SerbianLatin'; lcid = 2074 }
	'sk'      = @{ nsis = 'Slovak'; lcid = 1051 }
	'sl'      = @{ nsis = 'Slovenian'; lcid = 1060 }
	'es'      = @{ nsis = 'SpanishInternational'; lcid = 3082 }
	'sv'      = @{ nsis = 'Swedish'; lcid = 1053 }
	'th'      = @{ nsis = 'Thai'; lcid = 1054 }
	'tr'      = @{ nsis = 'Turkish'; lcid = 1055 }
	'uk'      = @{ nsis = 'Ukrainian'; lcid = 1058 }
	'vi'      = @{ nsis = 'Vietnamese'; lcid = 1066 }
}

$utf8WithBom = New-Object System.Text.UTF8Encoding($true)
$scriptDirectory = Join-Path $PSScriptRoot '..\Language'
$resourceHeaderPath = Join-Path $PSScriptRoot '..\Library\resource.h'
$installerOutputDirectory = Join-Path $PSScriptRoot '..\BuildOut\Installer'
$languageOutputDirectories = @(
	(Join-Path $PSScriptRoot '..\BuildOut\Release32\Languages'),
	(Join-Path $PSScriptRoot '..\BuildOut\Release64\Languages'),
	(Join-Path $PSScriptRoot '..\BuildOut\Debug32\Languages'),
	(Join-Path $PSScriptRoot '..\BuildOut\Debug64\Languages')
)
if ($OutputDirectory) {
	$languageOutputDirectories = $OutputDirectory
}

function Get-ResourceIds {
	param([string]$Path)

	$ids = @{}
	foreach ($line in [System.IO.File]::ReadAllLines($Path)) {
		if ($line -match '^\s*#define\s+(IDS_[A-Za-z0-9_]+)\s+(\d+)\s*$') {
			$ids[$Matches[1].Substring(4)] = [Convert]::ToUInt32($Matches[2], 10)
		}
	}

	if ($ids.Count -eq 0) {
		throw "No string resource IDs found in $Path"
	}

	return $ids
}

function Read-LanguageFile {
	param(
		[string]$Path,
		[hashtable]$ResourceIds,
		[string]$ResourceHeaderPath
	)

	if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
		throw "Language file not found: $Path"
	}

	$installerStrings = New-Object System.Collections.Generic.List[object]
	$runtimeStrings = New-Object System.Collections.Generic.List[object]
	$buttonWidth = $null
	$labelWidth = $null
	$rtl = $null
	$currentSection = ''
	$installerSectionCount = 0

	foreach ($line in [System.IO.File]::ReadAllLines($Path)) {
		if ($line -match '^\s*\[([^]]+)\]\s*$') {
			$currentSection = $Matches[1]
			if ($currentSection -eq 'Installer') {
				++$installerSectionCount
			}
			continue
		}
		if ([string]::IsNullOrWhiteSpace($line) -or $line.TrimStart().StartsWith(';')) {
			continue
		}
		if ($line -notmatch '^([^=]+)=(.*)$') {
			throw "Invalid language string in ${Path}: $line"
		}

		$key = $Matches[1].Trim()
		$value = $Matches[2]
		if ($currentSection -eq 'Installer') {
			[void]$installerStrings.Add([pscustomobject]@{ Key = $key; Value = $value })
			continue
		}
		if ($currentSection -eq 'LanguageSettings') {
			switch ($key) {
				'BUTTONWIDTH' { $buttonWidth = [uint16]$value }
				'LABELWIDTH' { $labelWidth = [uint16]$value }
				'RTL' { $rtl = [byte]$value }
				default { throw "Unknown language setting in ${Path}: $key" }
			}
			continue
		}
		if (-not $ResourceIds.ContainsKey($key)) {
			throw "String resource ID not found for $key in $ResourceHeaderPath"
		}

		# Convert the escaping used by the INI source before UTF-16 serialization.
		$value = $value.Replace('\n', "`n").Replace('\\', '\')
		[void]$runtimeStrings.Add([pscustomobject]@{ Id = [uint32]$ResourceIds[$key]; Value = $value })
	}

	if ($installerSectionCount -ne 1 -or $installerStrings.Count -eq 0) {
		throw "Expected exactly one non-empty [Installer] section in $Path"
	}
	if ($null -eq $buttonWidth -or $null -eq $labelWidth -or $null -eq $rtl) {
		throw "Missing LanguageSettings value in $Path"
	}
	if ($rtl -ne 0 -and $rtl -ne 1) {
		throw "RTL must be 0 or 1 in $Path"
	}

	return [pscustomobject]@{
		InstallerStrings = $installerStrings
		RuntimeStrings = $runtimeStrings
		ButtonWidth = $buttonWidth
		LabelWidth = $labelWidth
		Rtl = $rtl
	}
}

function Write-InstallerLanguageFile {
	param(
		[string]$Path,
		[System.Collections.IEnumerable]$Strings,
		[System.Text.Encoding]$Encoding
	)

	$output = New-Object System.Collections.Generic.List[string]
	foreach ($string in $Strings) {
		[void]$output.Add(('${{LangFileString}} {0,-24} "{1}"' -f $string.Key, $string.Value))
	}

	$content = [string]::Join("`r`n", $output) + "`r`n"
	[System.IO.File]::WriteAllText($Path, $content, $Encoding)
}

function Write-RuntimeLanguageFile {
	param(
		[string]$Locale,
		[uint32]$Lcid,
		[object]$Language,
		[string[]]$OutputDirectories
	)

	$fileName = $Lcid.ToString() + '.rmlang'
	foreach ($directory in $OutputDirectories) {
		$legacyPath = Join-Path $directory ($Locale + '.rmlang')
		[System.IO.File]::Delete($legacyPath)
	}

	$binaryPath = Join-Path $OutputDirectories[0] $fileName
	$stream = [System.IO.File]::Open($binaryPath, [System.IO.FileMode]::Create, [System.IO.FileAccess]::Write)
	$writer = New-Object System.IO.BinaryWriter($stream)
	try {
		# BinaryWriter emits little-endian integers. Encoding.Unicode is UTF-16LE.
		# String lengths exclude the null terminator, matching the runtime reader.
		$writer.Write([System.Text.Encoding]::ASCII.GetBytes('RMLANG'))
		$writer.Write([byte]1)
		$writer.Write([byte]$Language.Rtl)
		$writer.Write([uint16]$Language.ButtonWidth)
		$writer.Write([uint16]$Language.LabelWidth)
		foreach ($string in $Language.RuntimeStrings) {
			$writer.Write([uint32]$string.Id)
			$writer.Write([uint32]$string.Value.Length)
			$writer.Write([System.Text.Encoding]::Unicode.GetBytes($string.Value + [char]0))
		}
	} finally {
		$writer.Dispose()
	}

	# Each build configuration runs from its own output directory.
	if ($OutputDirectories.Length -gt 1) {
		foreach ($directory in $OutputDirectories[1..($OutputDirectories.Length - 1)]) {
			$copyPath = Join-Path $directory $fileName
			[System.IO.File]::Copy($binaryPath, $copyPath, $true)
		}
	}
}

function Write-InstallerLanguagesFile {
	param(
		[string]$Path,
		[System.Collections.IDictionary]$Languages,
		[System.Text.Encoding]$Encoding
	)

	$output = New-Object System.Collections.Generic.List[string]
	$langDllParams = ''
	$languageIds = ''
	[void]$output.Add('')

	foreach ($language in $Languages.GetEnumerator()) {
		$locale = $language.Key
		$localeInfo = [System.Globalization.CultureInfo]::GetCultureInfo($locale)
		$displayName = $localeInfo.NativeName
		if ($locale -ne 'en') {
			$displayName += " - $($localeInfo.EnglishName.split(' ')[0])"
		}

		$nsisLanguage = $language.Value.nsis
		[void]$output.Add(('${{IncludeLanguage}} "{0}" "{1}"' -f $nsisLanguage, $locale))
		$langDllParams += "'${displayName}' '`${LANG_${nsisLanguage}}' '`${LANG_${nsisLanguage}_CP}' "
		$languageIds += "$($language.Value.lcid),"
	}

	[void]$output.Add(('!define LANGDLL_PARAMS "{0}"' -f $langDllParams))
	[void]$output.Add(('!define LANGUAGE_IDS "{0}"' -f $languageIds))
	$content = [string]::Join("`r`n", $output) + "`r`n"
	[System.IO.File]::WriteAllText($Path, $content, $Encoding)
}

$locales = if ($Locale) { $Locale } else { @($languages.Keys) }
$resourceIds = Get-ResourceIds -Path $resourceHeaderPath
foreach ($directory in $languageOutputDirectories) {
	[System.IO.Directory]::CreateDirectory($directory) | Out-Null
}
if (-not $RuntimeOnly) {
	[System.IO.Directory]::CreateDirectory($installerOutputDirectory) | Out-Null
}

Write-Host "Generating language files..."

foreach ($localeName in $locales) {
	if (-not $languages.Contains($localeName)) {
		throw "Unknown language locale: $localeName"
	}

	$iniPath = Join-Path $scriptDirectory ($localeName + '.ini')
	$definition = Read-LanguageFile -Path $iniPath -ResourceIds $resourceIds -ResourceHeaderPath $resourceHeaderPath
	if (-not $RuntimeOnly) {
		$nshPath = Join-Path $installerOutputDirectory ($localeName + '.nsh')
		Write-InstallerLanguageFile -Path $nshPath -Strings $definition.InstallerStrings -Encoding $utf8WithBom
	}
	Write-RuntimeLanguageFile -Locale $localeName -Lcid $languages[$localeName].lcid -Language $definition -OutputDirectories $languageOutputDirectories
}

if (-not $RuntimeOnly) {
	$installerLanguagesPath = Join-Path $installerOutputDirectory 'Languages.nsh'
	Write-InstallerLanguagesFile -Path $installerLanguagesPath -Languages $languages -Encoding $utf8WithBom
}
