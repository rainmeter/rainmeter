[CmdletBinding()]
param(
	[string[]]$Locale,
	[string[]]$OutputDirectory,
	[switch]$RuntimeOnly
)

$ErrorActionPreference = 'Stop'

# NSIS uses its own language names, while Codepage contains the Windows LCID
# persisted by the installer. English must be first because it's the default
# language.
$languageCodepages = [ordered]@{
	'en'      = @{ NsisLanguage = 'English'; Codepage = 1033 }
	'ar'      = @{ NsisLanguage = 'Arabic'; Codepage = 1025 }
	'bg'      = @{ NsisLanguage = 'Bulgarian'; Codepage = 1026 }
	'zh-CN'   = @{ NsisLanguage = 'SimpChinese'; Codepage = 2052 }
	'zh-TW'   = @{ NsisLanguage = 'TradChinese'; Codepage = 1028 }
	'hr'      = @{ NsisLanguage = 'Croatian'; Codepage = $null }
	'cs'      = @{ NsisLanguage = 'Czech'; Codepage = 1029 }
	'da'      = @{ NsisLanguage = 'Danish'; Codepage = 1030 }
	'nl'      = @{ NsisLanguage = 'Dutch'; Codepage = 1043 }
	'et'      = @{ NsisLanguage = 'Estonian'; Codepage = 1061 }
	'fi'      = @{ NsisLanguage = 'Finnish'; Codepage = 1035 }
	'fr'      = @{ NsisLanguage = 'French'; Codepage = 1036 }
	'de'      = @{ NsisLanguage = 'German'; Codepage = 1031 }
	'el'      = @{ NsisLanguage = 'Greek'; Codepage = 1032 }
	'he'      = @{ NsisLanguage = 'Hebrew'; Codepage = 1037 }
	'hu'      = @{ NsisLanguage = 'Hungarian'; Codepage = 1038 }
	'id'      = @{ NsisLanguage = 'Indonesian'; Codepage = 1057 }
	'it'      = @{ NsisLanguage = 'Italian'; Codepage = 1040 }
	'ja'      = @{ NsisLanguage = 'Japanese'; Codepage = 1041 }
	'ko'      = @{ NsisLanguage = 'Korean'; Codepage = 1042 }
	'lv'      = @{ NsisLanguage = 'Latvian'; Codepage = $null }
	'lt'      = @{ NsisLanguage = 'Lithuanian'; Codepage = $null }
	'ms'      = @{ NsisLanguage = 'Malay'; Codepage = 1086 }
	'nb'      = @{ NsisLanguage = 'Norwegian'; Codepage = 1044 }
	'pl'      = @{ NsisLanguage = 'Polish'; Codepage = 1045 }
	'pt-BR'   = @{ NsisLanguage = 'PortugueseBR'; Codepage = 1046 }
	'pt-PT'   = @{ NsisLanguage = 'Portuguese'; Codepage = 2070 }
	'ro'      = @{ NsisLanguage = 'Romanian'; Codepage = 1048 }
	'ru'      = @{ NsisLanguage = 'Russian'; Codepage = 1049 }
	'sr-Cyrl' = @{ NsisLanguage = 'Serbian'; Codepage = 3098 }
	'sr-Latn' = @{ NsisLanguage = 'SerbianLatin'; Codepage = 2074 }
	'sk'      = @{ NsisLanguage = 'Slovak'; Codepage = 1051 }
	'sl'      = @{ NsisLanguage = 'Slovenian'; Codepage = 1060 }
	'es'      = @{ NsisLanguage = 'SpanishInternational'; Codepage = 3082 }
	'sv'      = @{ NsisLanguage = 'Swedish'; Codepage = 1053 }
	'th'      = @{ NsisLanguage = 'Thai'; Codepage = 1054 }
	'tr'      = @{ NsisLanguage = 'Turkish'; Codepage = 1055 }
	'uk'      = @{ NsisLanguage = 'Ukrainian'; Codepage = 1058 }
	'vi'      = @{ NsisLanguage = 'Vietnamese'; Codepage = 1066 }
}

$utf8WithBom = New-Object System.Text.UTF8Encoding($true)
$scriptDirectory = Join-Path $PSScriptRoot '..\Language'
$resourceHeaderPath = Join-Path $PSScriptRoot '..\Library\resource.h'
$languageOutputDirectories = @(
	(Join-Path $PSScriptRoot '..\x32-Release\Languages'),
	(Join-Path $PSScriptRoot '..\x64-Release\Languages'),
	(Join-Path $PSScriptRoot '..\x32-Debug\Languages'),
	(Join-Path $PSScriptRoot '..\x64-Debug\Languages')
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
		[object]$Language,
		[string[]]$OutputDirectories
	)

	$binaryPath = Join-Path $OutputDirectories[0] ($Locale + '.rmlang')
	$stream = [System.IO.File]::Open($binaryPath, [System.IO.FileMode]::Create, [System.IO.FileAccess]::Write)
	$writer = New-Object System.IO.BinaryWriter($stream)
	try {
		# BinaryWriter emits little-endian integers. Encoding.Unicode is UTF-16LE.
		# String lengths exclude the null terminator, matching the runtime reader.
		$writer.Write([System.Text.Encoding]::ASCII.GetBytes('RMLANG'))
		$writer.Write([byte]1)
		$writer.Write($Language.Rtl)
		$writer.Write([uint16]$Language.ButtonWidth)
		$writer.Write([uint16]$Language.LabelWidth)
		foreach ($string in $Language.RuntimeStrings) {
			$writer.Write($string.Id)
			$writer.Write([uint32]$string.Value.Length)
			$writer.Write([System.Text.Encoding]::Unicode.GetBytes($string.Value + [char]0))
		}
	} finally {
		$writer.Dispose()
	}

	# Each build configuration runs from its own output directory.
	if ($OutputDirectories.Length -gt 1) {
		foreach ($directory in $OutputDirectories[1..($OutputDirectories.Length - 1)]) {
			$copyPath = Join-Path $directory ($Locale + '.rmlang')
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
		if ($null -eq $language.Value.Codepage) {
			continue
		}

		$locale = $language.Key
		$nsisLanguage = $language.Value.NsisLanguage
		[void]$output.Add(('${{IncludeLanguage}} "{0}" "{1}"' -f $nsisLanguage, $locale))
		$langDllParams += "'$locale -  `${LANGFILE_${nsisLanguage}_NAME}' '`${LANG_${nsisLanguage}}' '`${LANG_${nsisLanguage}_CP}' "
		$languageIds += "$($language.Value.Codepage),"
	}

	[void]$output.Add(('!define LANGDLL_PARAMS "{0}"' -f $langDllParams))
	[void]$output.Add(('!define LANGUAGE_IDS "{0}"' -f $languageIds))
	$content = [string]::Join("`r`n", $output) + "`r`n"
	[System.IO.File]::WriteAllText($Path, $content, $Encoding)
}

$locales = if ($Locale) { $Locale } else { @($languageCodepages.Keys) }
$resourceIds = Get-ResourceIds -Path $resourceHeaderPath
foreach ($directory in $languageOutputDirectories) {
	[System.IO.Directory]::CreateDirectory($directory) | Out-Null
}

Write-Host "Generating language files..."

foreach ($localeName in $locales) {
	if (-not $languageCodepages.Contains($localeName)) {
		throw "Unknown language locale: $localeName"
	}

	$iniPath = Join-Path $scriptDirectory ($localeName + '.ini')
	$definition = Read-LanguageFile -Path $iniPath -ResourceIds $resourceIds -ResourceHeaderPath $resourceHeaderPath
	if (-not $RuntimeOnly) {
		$nshPath = Join-Path $scriptDirectory ($localeName + '.nsh')
		Write-InstallerLanguageFile -Path $nshPath -Strings $definition.InstallerStrings -Encoding $utf8WithBom
	}
	Write-RuntimeLanguageFile -Locale $localeName -Language $definition -OutputDirectories $languageOutputDirectories
}

if (-not $RuntimeOnly) {
	$installerLanguagesPath = Join-Path $PSScriptRoot 'Installer\Languages.nsh'
	Write-InstallerLanguagesFile -Path $installerLanguagesPath -Languages $languageCodepages -Encoding $utf8WithBom
}
