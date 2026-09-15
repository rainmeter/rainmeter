[CmdletBinding()]
param(
	[string[]]$Locale,
	[string[]]$OutputDirectory,
	[switch]$RuntimeOnly
)

$ErrorActionPreference = 'Stop'

# English must be first because it is the default language.
$languages = [ordered]@{
	'en' = 1033
	'ar' = 1025
	'bg' = 1026
	'zh-CN' = 2052
	'zh-TW' = 1028
	'cs' = 1029
	'da' = 1030
	'nl' = 1043
	'et' = 1061
	'fi' = 1035
	'fr' = 1036
	'de' = 1031
	'el' = 1032
	'he' = 1037
	'hr' = 1050
	'hu' = 1038
	'id' = 1057
	'it' = 1040
	'ja' = 1041
	'ko' = 1042
	'ms' = 1086
	'nb' = 1044
	'pl' = 1045
	'pt-BR' = 1046
	'pt-PT' = 2070
	'ro' = 1048
	'ru' = 1049
	'sr-Cyrl' = 3098
	'sr-Latn' = 2074
	'sk' = 1051
	'sl' = 1060
	'es' = 3082
	'sv' = 1053
	'th' = 1054
	'tr' = 1055
	'uk' = 1058
	'vi' = 1066
}

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

# Rainmeter strings use %1 and {0} style placeholders, while installer strings use $INSTDIR and
# ${VERSION_SHORT} style variables.
$placeholderRegex = [regex]'%[0-9]|\{[0-9]+\}|\$\{[A-Za-z0-9_]+\}|\$[A-Za-z0-9_]+'
$argumentPlaceholderRegex = [regex]'^(%[0-9]|\{[0-9]+\})$'

# Returns the distinct placeholders of a string.
function Get-Placeholder {
	param([string]$Value)

	return @($placeholderRegex.Matches($Value) | ForEach-Object { $_.Value } | Sort-Object -Unique)
}

# A translation that drops or misspells a placeholder leaves the text with an unsubstituted
# variable, and an argument placeholder that has no matching argument crashes Rainmeter when the
# string is formatted. Additional installer variables are allowed since they always expand.
function Assert-Placeholder {
	param(
		[string]$Key,
		[string]$Value,
		[string]$BaseValue,
		[string]$Path
	)

	$expected = Get-Placeholder -Value $BaseValue
	$actual = Get-Placeholder -Value $Value

	$missing = @($expected | Where-Object { $actual -notcontains $_ })
	if ($missing.Count -gt 0) {
		throw "Missing placeholder $($missing -join ', ') for $Key in $Path"
	}

	$unexpected = @($actual | Where-Object { $expected -notcontains $_ -and $argumentPlaceholderRegex.IsMatch($_) })
	if ($unexpected.Count -gt 0) {
		throw "Unexpected placeholder $($unexpected -join ', ') for $Key in $Path"
	}
}

function Read-LanguageFile {
	param(
		[string]$Path,
		[hashtable]$ResourceIds,
		[string]$ResourceHeaderPath,
		[object]$BaseLanguage
	)

	if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
		throw "Language file not found: $Path"
	}

	$installerStrings = [ordered]@{}
	$runtimeStrings = [ordered]@{}
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
			$installerStrings[$key] = $value
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
		$runtimeStrings[$key] = [pscustomobject]@{ Id = [uint32]$ResourceIds[$key]; Value = $value }
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

	if ($BaseLanguage) {
		$mergedInstallerStrings = New-Object System.Collections.Generic.List[object]
		foreach ($string in $BaseLanguage.InstallerStrings) {
			$value = if ($installerStrings.Contains($string.Key) -and $installerStrings[$string.Key].Length -gt 0) { $installerStrings[$string.Key] } else { $string.Value }
			Assert-Placeholder -Key $string.Key -Value $value -BaseValue $string.Value -Path $Path
			[void]$mergedInstallerStrings.Add([pscustomobject]@{ Key = $string.Key; Value = $value })
			$installerStrings.Remove($string.Key)
		}
		if ($installerStrings.Count -gt 0) {
			throw "Installer string not found in English language file: $($installerStrings.Keys -join ', ')"
		}

		$mergedRuntimeStrings = New-Object System.Collections.Generic.List[object]
		foreach ($string in $BaseLanguage.RuntimeStrings) {
			$value = if ($runtimeStrings.Contains($string.Key) -and $runtimeStrings[$string.Key].Value.Length -gt 0) { $runtimeStrings[$string.Key].Value } else { $string.Value }
			Assert-Placeholder -Key $string.Key -Value $value -BaseValue $string.Value -Path $Path
			[void]$mergedRuntimeStrings.Add([pscustomobject]@{ Key = $string.Key; Id = $string.Id; Value = $value })
			$runtimeStrings.Remove($string.Key)
		}
		if ($runtimeStrings.Count -gt 0) {
			throw "Runtime string not found in English language file: $($runtimeStrings.Keys -join ', ')"
		}

		return [pscustomobject]@{
			InstallerStrings = $mergedInstallerStrings
			RuntimeStrings = $mergedRuntimeStrings
			ButtonWidth = $buttonWidth
			LabelWidth = $labelWidth
			Rtl = $rtl
		}
	}

	return [pscustomobject]@{
		InstallerStrings = @($installerStrings.GetEnumerator() | ForEach-Object { [pscustomobject]@{ Key = $_.Key; Value = $_.Value } })
		RuntimeStrings = @($runtimeStrings.GetEnumerator() | ForEach-Object { [pscustomobject]@{ Key = $_.Key; Id = $_.Value.Id; Value = $_.Value.Value } })
		ButtonWidth = $buttonWidth
		LabelWidth = $labelWidth
		Rtl = $rtl
	}
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

function ConvertTo-CppByteString {
	param([string]$Value)

	$bytes = [System.Text.Encoding]::UTF8.GetBytes($Value)
	return ($bytes | ForEach-Object { '\x{0:x2}' -f $_ }) -join ''
}

function Write-NativeInstallerLanguagesFile {
	param(
		[string]$Path,
		[System.Collections.IDictionary]$Languages,
		[System.Collections.IDictionary]$Definitions
	)

	$keys = @($Definitions['en'].InstallerStrings | ForEach-Object { $_.Key })
	$output = [System.Collections.Generic.List[string]]::new()
	[void]$output.Add('#pragma once')
	[void]$output.Add('')
	[void]$output.Add('enum class InstallerString : unsigned short')
	[void]$output.Add('{')
	foreach ($key in $keys) {
		[void]$output.Add("`t$key,")
	}
	[void]$output.Add("`tCount")
	[void]$output.Add('};')
	[void]$output.Add('')
	[void]$output.Add('struct InstallerLanguage')
	[void]$output.Add('{')
	[void]$output.Add("`tunsigned short lcid;")
	[void]$output.Add("`tbool rtl;")
	[void]$output.Add("`tconst char* name;")
	[void]$output.Add("`tconst char* strings;")
	[void]$output.Add('};')
	[void]$output.Add('')

	$index = 0
	foreach ($language in $Languages.GetEnumerator()) {
		$locale = $language.Key
		$definition = $Definitions[$locale]
		$localeInfo = [System.Globalization.CultureInfo]::GetCultureInfo($locale)
		$displayName = $localeInfo.NativeName
		if ($locale -ne 'en') {
			$displayName += " - $($localeInfo.EnglishName.split(' ')[0])"
		}
		$blob = [string]::Join([char]0, @($definition.InstallerStrings | ForEach-Object { $_.Value })) + [char]0
		[void]$output.Add(('static const char g_languageStrings{0}[] = "{1}";' -f $index, (ConvertTo-CppByteString -Value $blob)))
		[void]$output.Add(('static const char g_languageName{0}[] = "{1}";' -f $index, (ConvertTo-CppByteString -Value $displayName)))
		++$index
	}

	[void]$output.Add('')
	[void]$output.Add('static const InstallerLanguage g_languages[] = {')
	$index = 0
	foreach ($language in $Languages.GetEnumerator()) {
		$definition = $Definitions[$language.Key]
		[void]$output.Add(("`t{{ {0}, {1}, g_languageName{2}, g_languageStrings{2} }}," -f $language.Value, $(if ($definition.Rtl) { 'true' } else { 'false' }), $index))
		++$index
	}
	[void]$output.Add('};')

	$utf8NoBom = New-Object System.Text.UTF8Encoding($false)
	[System.IO.File]::WriteAllText($Path, ([string]::Join("`r`n", $output) + "`r`n"), $utf8NoBom)
}

$locales = if ($Locale) { $Locale } else { @($languages.Keys) }
$resourceIds = Get-ResourceIds -Path $resourceHeaderPath
$englishIniPath = Join-Path $scriptDirectory 'en.ini'
$englishDefinition = Read-LanguageFile -Path $englishIniPath -ResourceIds $resourceIds -ResourceHeaderPath $resourceHeaderPath -BaseLanguage $null
foreach ($directory in $languageOutputDirectories) {
	[System.IO.Directory]::CreateDirectory($directory) | Out-Null
}
if (-not $RuntimeOnly) {
	[System.IO.Directory]::CreateDirectory($installerOutputDirectory) | Out-Null
}

$definitions = [ordered]@{}
foreach ($localeName in $locales) {
	if (-not $languages.Contains($localeName)) {
		throw "Unknown language locale: $localeName"
	}

	$iniPath = Join-Path $scriptDirectory ($localeName + '.ini')
	$definition = if ($localeName -eq 'en') {
		$englishDefinition
	} else {
		Read-LanguageFile -Path $iniPath -ResourceIds $resourceIds -ResourceHeaderPath $resourceHeaderPath -BaseLanguage $englishDefinition
	}
	$definitions[$localeName] = $definition
	Write-RuntimeLanguageFile -Locale $localeName -Lcid $languages[$localeName] -Language $definition -OutputDirectories $languageOutputDirectories
}

if (-not $RuntimeOnly) {
	if (-not $Locale) {
		$nativeLanguagesPath = Join-Path $installerOutputDirectory 'InstallerLanguages.generated.h'
		Write-NativeInstallerLanguagesFile -Path $nativeLanguagesPath -Languages $languages -Definitions $definitions
	}
}
