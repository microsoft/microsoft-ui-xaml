# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
#
# Repair-CoverageDuplicates.ps1
#
# Fixes a real, verified defect in Microsoft's native (C++ static-instrumentation) code coverage
# merge engine - the same engine backing BOTH the `dotnet-coverage` global tool and Visual
# Studio's bundled `Microsoft.CodeCoverage.Console.exe`.
#
# THE BUG: when `<tool> merge <n .coverage files> -f cobertura` merges multiple raw .coverage
# files that all instrument the SAME native module (e.g. one module instrumented once, then
# exercised by N separate collector processes on N separate machines - exactly what happens when
# a test pass is split into parallel Helix/lab slices with one collector per slice), the merge
# does NOT consolidate matching (file, line-number) entries into one. Instead it appends a
# fresh copy of that class's <line> elements per contributing input file. This was verified to
# be 100% systemic (every single <class> element in a real merged report exhibited it) and to
# exist even in Visual Studio's own native (non-Cobertura) merge output - so it is a merge-time
# defect, not a Cobertura-serialization-only quirk.
#
# EFFECT: any tool that iterates every <line> element and sums them (dotnet-coverage's own
# printed merge summary, reportgenerator's TextSummary, etc.) overcounts both total and covered
# lines, understating the true coverage percentage. Azure DevOps's own "Code Coverage" build tab
# is NOT affected, because it deduplicates by (file, line-number) when it renders - this script
# reproduces that same deduplication so a locally-computed number matches the ADO tab exactly.
# (Verified: deduplicating a real build's merged.cobertura.xml this way reproduced the ADO tab's
# 4,500 files / 451,305 total lines / 337,879 covered lines / 74.87% exactly.)
#
# THE FIX (this script): parse the buggy merged Cobertura XML, and for every unique source file
# (grouping across ALL <class> elements that map to it - some files have many classes, e.g. one
# per C++ template instantiation, that all reference the same physical source lines), keep only
# one entry per unique line number (hits = max across duplicates). Emit:
#   1. corrected-summary.json / corrected-summary.csv - overall + per-file corrected totals.
#   2. corrected.cobertura.xml - a schema-valid Cobertura file (one <class> per source file,
#      already deduplicated) that any standard Cobertura consumer (reportgenerator, ADO's own
#      publish task, etc.) will read back with the SAME correct numbers.
#
# This script is self-contained and works on ANY Cobertura XML produced by dotnet-coverage /
# Microsoft.CodeCoverage.Console's merge - not just ones produced by the sibling
# Get-PipelineCodeCoverage.ps1 script in this skill.

[CmdletBinding()]
param(
    # Path to the (buggy, duplicate-line) merged Cobertura XML to repair.
    [Parameter(Mandatory = $true)]
    [string]$InputCobertura,

    # Where to write corrected-summary.json/.csv and corrected.cobertura.xml.
    [Parameter(Mandatory = $true)]
    [string]$OutputDir,

    # Name(s) of the product DLL(s) that were instrumented to produce this coverage data (e.g.
    # "Microsoft.ui.xaml.dll", or multiple space-separated names if more than one module was
    # instrumented). Purely descriptive - recorded in corrected-summary.json's
    # "InstrumentedModules" field and used as the <package name="..."> in the corrected Cobertura
    # output, so downstream reports (e.g. Build-CoverageTreeReport.ps1's HTML) can show which
    # binary this coverage percentage actually measures. Optional - falls back to a generic
    # placeholder if not supplied.
    [string[]]$ModuleNames = @()
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Xml

if (-not (Test-Path $InputCobertura)) {
    throw "Input Cobertura file not found: $InputCobertura"
}
New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null

Write-Host "Parsing and deduplicating $InputCobertura ..."
Write-Host "(This streams the XML rather than loading a full DOM, so large multi-hundred-MB merges are fine.)"
$sw = [System.Diagnostics.Stopwatch]::StartNew()

# fileName -> Dictionary<lineNumber, hits>  (hits = max seen across all duplicate/class entries)
$fileLineMap = New-Object 'System.Collections.Generic.Dictionary[string,System.Collections.Generic.Dictionary[int,int]]'
# Preserve first-seen file order for stable, readable output.
$fileOrder = New-Object 'System.Collections.Generic.List[string]'

$reader = [System.Xml.XmlReader]::Create($InputCobertura)
$currentDict = $null
$classElementCount = 0
$rawLineElementCount = 0
try {
    while ($reader.Read()) {
        if ($reader.NodeType -eq [System.Xml.XmlNodeType]::Element -and $reader.Name -eq 'class') {
            $classElementCount++
            $fn = $reader.GetAttribute('filename')
            if (-not $fn) { $fn = $reader.GetAttribute('name') }
            if (-not $fileLineMap.TryGetValue($fn, [ref]$currentDict)) {
                $currentDict = New-Object 'System.Collections.Generic.Dictionary[int,int]'
                $fileLineMap[$fn] = $currentDict
                $fileOrder.Add($fn)
            }
        }
        elseif ($reader.NodeType -eq [System.Xml.XmlNodeType]::Element -and $reader.Name -eq 'line') {
            $rawLineElementCount++
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
} finally {
    $reader.Close()
}

Write-Host ("Parsed {0} raw <class> elements and {1} raw <line> elements in {2:N1}s." -f $classElementCount, $rawLineElementCount, $sw.Elapsed.TotalSeconds)

# Aggregate corrected per-file and overall totals.
$perFile = New-Object System.Collections.Generic.List[object]
$overallTotal = 0
$overallCovered = 0
foreach ($fn in $fileOrder) {
    $dict = $fileLineMap[$fn]
    $total = $dict.Count
    $covered = 0
    foreach ($h in $dict.Values) { if ($h -gt 0) { $covered++ } }
    $overallTotal += $total
    $overallCovered += $covered
    $perFile.Add([pscustomobject]@{
        File          = $fn
        CoveredLines  = $covered
        TotalLines    = $total
        LineCoverage  = if ($total -gt 0) { [math]::Round(100.0 * $covered / $total, 2) } else { 0 }
    })
}

$overallRate = if ($overallTotal -gt 0) { $overallCovered / $overallTotal } else { 0 }
$moduleDisplayName = if ($ModuleNames -and $ModuleNames.Count -gt 0) { $ModuleNames -join ', ' } else { '(not specified)' }

$summary = [pscustomobject]@{
    Modules             = $fileOrder.Count
    TotalLines          = $overallTotal
    CoveredLines        = $overallCovered
    LineCoverage        = [math]::Round(100.0 * $overallRate, 2)
    InstrumentedModules = @($ModuleNames)
    RawClassCount       = $classElementCount
    RawLineCount        = $rawLineElementCount
    SourceFile          = (Resolve-Path $InputCobertura).Path
    GeneratedAt   = (Get-Date).ToString('o')
}

$summaryJsonPath = Join-Path $OutputDir 'corrected-summary.json'
$summaryCsvPath  = Join-Path $OutputDir 'corrected-summary.csv'
$perFileCsvPath  = Join-Path $OutputDir 'corrected-per-file.csv'
$correctedXmlPath = Join-Path $OutputDir 'corrected.cobertura.xml'

$summary | ConvertTo-Json | Set-Content $summaryJsonPath
$summary | Export-Csv $summaryCsvPath -NoTypeInformation
$perFile | Sort-Object File | Export-Csv $perFileCsvPath -NoTypeInformation

Write-Host "`nWriting corrected, deduplicated Cobertura XML (one <class> per source file) -> $correctedXmlPath"
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
    $writer.WriteEndElement() # sources

    $writer.WriteStartElement('packages')
    $writer.WriteStartElement('package')
    $writer.WriteAttributeString('name', $moduleDisplayName)
    $writer.WriteAttributeString('line-rate', $overallRate.ToString('G17', [System.Globalization.CultureInfo]::InvariantCulture))
    $writer.WriteAttributeString('branch-rate', '0')
    $writer.WriteAttributeString('complexity', '0')
    $writer.WriteStartElement('classes')

    foreach ($fn in $fileOrder) {
        $dict = $fileLineMap[$fn]
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
        $writer.WriteEndElement() # methods
        $writer.WriteStartElement('lines')
        foreach ($num in ($dict.Keys | Sort-Object)) {
            $writer.WriteStartElement('line')
            $writer.WriteAttributeString('number', $num)
            $writer.WriteAttributeString('hits', $dict[$num])
            $writer.WriteAttributeString('branch', 'false')
            $writer.WriteEndElement() # line
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

Write-Host "`n=== Corrected coverage (bug-fixed, matches Azure DevOps's Code Coverage tab) ==="
Write-Host ("  Instrumented DLL(s): {0}" -f $moduleDisplayName)
Write-Host ("  Modules (files):  {0}" -f $summary.Modules)
Write-Host ("  Total lines:      {0}" -f $summary.TotalLines)
Write-Host ("  Covered lines:    {0}" -f $summary.CoveredLines)
Write-Host ("  Line coverage:    {0}%" -f $summary.LineCoverage)
Write-Host "`nWrote:"
Write-Host "  $summaryJsonPath"
Write-Host "  $summaryCsvPath"
Write-Host "  $perFileCsvPath"
Write-Host "  $correctedXmlPath"
