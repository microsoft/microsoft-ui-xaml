# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
#
# Split-CoverageByModule.ps1
#
# Companion to Repair-CoverageDuplicates.ps1. That script applies the duplicate-line dedup fix
# across the WHOLE merged Cobertura file, producing a single aggregate number for every
# instrumented module combined. When a coverage run instruments MORE THAN ONE product DLL (e.g.
# Microsoft.ui.xaml.dll AND Microsoft.UI.Xaml.Controls.dll), the raw merge keeps them in separate
# <package> elements - this script preserves that split, applying the exact same
# (file, line-number) dedup INDEPENDENTLY per package/module, so each DLL gets its own
# ADO-tab-accurate coverage number and per-file breakdown.
#
# Output (under -OutputDir):
#   <Module>\corrected-summary.json / .csv
#   <Module>\corrected-per-file.csv
#   <Module>\corrected.cobertura.xml
#   per-module-summary.csv            - one row per module, side by side
#
# NOTE on totals: summing the per-module line totals will NOT necessarily equal the combined
# (all-modules) total from Repair-CoverageDuplicates.ps1, because a source file (typically a
# shared header) can legitimately be compiled into more than one module - the combined run
# dedups that file's lines once globally, whereas each module counts its own copy. Both numbers
# are correct for what they measure; use the per-module number when asking "how well is THIS DLL
# covered".

[CmdletBinding()]
param(
    # Path to the (buggy, duplicate-line) merged Cobertura XML produced by
    # `dotnet-coverage merge ... -f cobertura` / Microsoft.CodeCoverage.Console.exe.
    [Parameter(Mandatory = $true)]
    [string]$InputCobertura,

    # Where to write the per-module folders and per-module-summary.csv.
    [Parameter(Mandatory = $true)]
    [string]$OutputDir,

    # Optional filter: only emit modules whose <package name> matches one of these (matching is
    # case-insensitive and ignores a trailing ".dll", so "Microsoft.UI.Xaml.Controls.dll" matches
    # a package named "Microsoft.ui.xaml.controls"). Omit to emit every module found.
    [string[]]$Modules = @()
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Xml

if (-not (Test-Path $InputCobertura)) {
    throw "Input Cobertura file not found: $InputCobertura"
}
New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null

function Get-NormalizedModuleName {
    param([string]$Name)
    return ($Name -replace '\.dll$', '').ToLowerInvariant()
}

$wanted = @()
foreach ($m in $Modules) { if ($m) { $wanted += (Get-NormalizedModuleName $m) } }

Write-Host "Streaming and deduplicating (per module) $InputCobertura ..."
$sw = [System.Diagnostics.Stopwatch]::StartNew()

# module -> (file -> (line -> hits))
$moduleMap = New-Object 'System.Collections.Generic.Dictionary[string,object]'
$moduleOrder = New-Object 'System.Collections.Generic.List[string]'
# module -> ordered file list
$moduleFileOrder = New-Object 'System.Collections.Generic.Dictionary[string,object]'

$reader = [System.Xml.XmlReader]::Create($InputCobertura)
$currentModule = $null
$currentFileMap = $null
$currentFileOrder = $null
$currentDict = $null
$classCount = 0
$lineCount = 0
try {
    while ($reader.Read()) {
        if ($reader.NodeType -ne [System.Xml.XmlNodeType]::Element) { continue }

        switch ($reader.Name) {
            'package' {
                $currentModule = $reader.GetAttribute('name')
                if (-not $currentModule) { $currentModule = '(unnamed)' }
                if (-not $moduleMap.ContainsKey($currentModule)) {
                    $moduleMap[$currentModule] = New-Object 'System.Collections.Generic.Dictionary[string,System.Collections.Generic.Dictionary[int,int]]'
                    $moduleFileOrder[$currentModule] = New-Object 'System.Collections.Generic.List[string]'
                    $moduleOrder.Add($currentModule)
                }
                $currentFileMap = $moduleMap[$currentModule]
                $currentFileOrder = $moduleFileOrder[$currentModule]
                $currentDict = $null
            }
            'class' {
                $classCount++
                $fn = $reader.GetAttribute('filename')
                if (-not $fn) { $fn = $reader.GetAttribute('name') }
                if ($null -eq $currentFileMap) { continue }
                if (-not $currentFileMap.TryGetValue($fn, [ref]$currentDict)) {
                    $currentDict = New-Object 'System.Collections.Generic.Dictionary[int,int]'
                    $currentFileMap[$fn] = $currentDict
                    $currentFileOrder.Add($fn)
                }
            }
            'line' {
                if ($null -eq $currentDict) { continue }
                $lineCount++
                $num = [int]$reader.GetAttribute('number')
                $hits = [int]$reader.GetAttribute('hits')
                $existing = 0
                if ($currentDict.TryGetValue($num, [ref]$existing)) {
                    if ($hits -gt $existing) { $currentDict[$num] = $hits }
                } else {
                    $currentDict[$num] = $hits
                }
            }
        }
    }
} finally {
    $reader.Close()
}

Write-Host ("Parsed {0} module(s), {1} <class> and {2} <line> elements in {3:N1}s." -f $moduleOrder.Count, $classCount, $lineCount, $sw.Elapsed.TotalSeconds)
Write-Host ("Modules found: {0}" -f ($moduleOrder -join ', '))

$perModuleRows = New-Object System.Collections.Generic.List[object]

foreach ($module in $moduleOrder) {
    if ($wanted.Count -gt 0 -and -not ($wanted -contains (Get-NormalizedModuleName $module))) {
        Write-Host "Skipping module '$module' (not in -Modules filter)."
        continue
    }

    $fileMap = $moduleMap[$module]
    $fileOrder = $moduleFileOrder[$module]
    $safeName = ($module -replace '[\\/:*?"<>|]', '_')
    $moduleDir = Join-Path $OutputDir $safeName
    New-Item -ItemType Directory -Force -Path $moduleDir | Out-Null

    $perFile = New-Object System.Collections.Generic.List[object]
    $overallTotal = 0
    $overallCovered = 0
    foreach ($fn in $fileOrder) {
        $dict = $fileMap[$fn]
        $total = $dict.Count
        $covered = 0
        foreach ($h in $dict.Values) { if ($h -gt 0) { $covered++ } }
        $overallTotal += $total
        $overallCovered += $covered
        $perFile.Add([pscustomobject]@{
            File         = $fn
            CoveredLines = $covered
            TotalLines   = $total
            LineCoverage = if ($total -gt 0) { [math]::Round(100.0 * $covered / $total, 2) } else { 0 }
        })
    }

    $overallRate = if ($overallTotal -gt 0) { $overallCovered / $overallTotal } else { 0 }
    $summary = [pscustomobject]@{
        Modules             = $fileOrder.Count
        TotalLines          = $overallTotal
        CoveredLines        = $overallCovered
        LineCoverage        = [math]::Round(100.0 * $overallRate, 2)
        InstrumentedModules = @($module)
        SourceFile          = (Resolve-Path $InputCobertura).Path
        GeneratedAt         = (Get-Date).ToString('o')
    }

    $summary | ConvertTo-Json | Set-Content (Join-Path $moduleDir 'corrected-summary.json')
    $summary | Export-Csv (Join-Path $moduleDir 'corrected-summary.csv') -NoTypeInformation
    $perFile | Sort-Object File | Export-Csv (Join-Path $moduleDir 'corrected-per-file.csv') -NoTypeInformation

    $correctedXmlPath = Join-Path $moduleDir 'corrected.cobertura.xml'
    $xmlSettings = New-Object System.Xml.XmlWriterSettings
    $xmlSettings.Indent = $false
    $xmlSettings.Encoding = [System.Text.UTF8Encoding]::new($false)
    $writer = [System.Xml.XmlWriter]::Create($correctedXmlPath, $xmlSettings)
    try {
        $writer.WriteStartDocument()
        $writer.WriteStartElement('coverage')
        $writer.WriteAttributeString('line-rate', $overallRate.ToString('G17', [System.Globalization.CultureInfo]::InvariantCulture))
        $writer.WriteAttributeString('branch-rate', '0')
        $writer.WriteAttributeString('lines-covered', $overallCovered)
        $writer.WriteAttributeString('lines-valid', $overallTotal)
        $writer.WriteAttributeString('branches-covered', '0')
        $writer.WriteAttributeString('branches-valid', '0')
        $writer.WriteAttributeString('complexity', '0')
        $writer.WriteAttributeString('version', '1.9')
        $writer.WriteAttributeString('timestamp', [DateTimeOffset]::UtcNow.ToUnixTimeSeconds().ToString())
        $writer.WriteStartElement('sources')
        $writer.WriteElementString('source', '.')
        $writer.WriteEndElement()
        $writer.WriteStartElement('packages')
        $writer.WriteStartElement('package')
        $writer.WriteAttributeString('name', $module)
        $writer.WriteAttributeString('line-rate', $overallRate.ToString('G17', [System.Globalization.CultureInfo]::InvariantCulture))
        $writer.WriteAttributeString('branch-rate', '0')
        $writer.WriteAttributeString('complexity', '0')
        $writer.WriteStartElement('classes')
        foreach ($fn in $fileOrder) {
            $dict = $fileMap[$fn]
            $total = $dict.Count
            $covered = 0
            foreach ($h in $dict.Values) { if ($h -gt 0) { $covered++ } }
            $rate = if ($total -gt 0) { $covered / $total } else { 0 }
            $writer.WriteStartElement('class')
            $writer.WriteAttributeString('name', $fn)
            $writer.WriteAttributeString('filename', $fn)
            $writer.WriteAttributeString('line-rate', $rate.ToString('G17', [System.Globalization.CultureInfo]::InvariantCulture))
            $writer.WriteAttributeString('branch-rate', '0')
            $writer.WriteAttributeString('complexity', '0')
            $writer.WriteStartElement('methods')
            $writer.WriteEndElement()
            $writer.WriteStartElement('lines')
            foreach ($num in ($dict.Keys | Sort-Object)) {
                $writer.WriteStartElement('line')
                $writer.WriteAttributeString('number', $num)
                $writer.WriteAttributeString('hits', $dict[$num])
                $writer.WriteAttributeString('branch', 'false')
                $writer.WriteEndElement()
            }
            $writer.WriteEndElement() # lines
            $writer.WriteEndElement() # class
        }
        $writer.WriteEndElement() # classes
        $writer.WriteEndElement() # package
        $writer.WriteEndElement() # packages
        $writer.WriteEndElement() # coverage
        $writer.WriteEndDocument()
    } finally {
        $writer.Close()
    }

    $perModuleRows.Add([pscustomobject]@{
        Module       = $module
        Files        = $fileOrder.Count
        TotalLines   = $overallTotal
        CoveredLines = $overallCovered
        LineCoverage = $summary.LineCoverage
        OutputFolder = $moduleDir
    })

    Write-Host ("`n=== {0} ===" -f $module)
    Write-Host ("  Files:         {0}" -f $fileOrder.Count)
    Write-Host ("  Total lines:   {0}" -f $overallTotal)
    Write-Host ("  Covered lines: {0}" -f $overallCovered)
    Write-Host ("  Line coverage: {0}%" -f $summary.LineCoverage)
}

if ($perModuleRows.Count -eq 0) {
    throw "No modules were emitted. Modules present in the input were: $($moduleOrder -join ', '). Check the -Modules filter."
}

$perModuleCsv = Join-Path $OutputDir 'per-module-summary.csv'
$perModuleRows | Export-Csv $perModuleCsv -NoTypeInformation
Write-Host "`nWrote per-module summary: $perModuleCsv"
