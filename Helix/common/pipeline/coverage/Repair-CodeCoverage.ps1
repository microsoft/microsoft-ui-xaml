# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
#
# Repair-CodeCoverage.ps1 - Turn the raw (duplicate-line) merged Cobertura into correct
# per-module and combined coverage numbers, plus the Cobertura file published to the ADO tab.
#
# WHY: the native coverage merge engine behind both dotnet-coverage and
# Microsoft.CodeCoverage.Console.exe does not consolidate duplicate (file, line) entries when it
# merges .coverage files collected by separate collector processes for the same module (one per
# test slice here). It appends another copy of each class's <line> elements per input file, so
# anything that sums <line> elements (the tool's own printed summary, reportgenerator, the root
# line-rate attribute) overcounts total and covered lines and understates the percentage. The ADO
# Code Coverage tab dedups by (file, line) when it renders; this script applies that same dedup:
# group every <line> by (source file, line number) across ALL classes mapping to that file (C++
# template instantiations produce many classes per file), keeping the max hit count.
#
# OUTPUT (under -OutputDir):
#   <Module>\corrected-per-file.csv / corrected-summary.json / corrected.cobertura.xml
#       One folder per instrumented DLL (one Cobertura <package> each), deduped independently.
#   Combined\corrected-per-file.csv / corrected-summary.json
#       All DLLs together, deduped globally by (file, line). A source file compiled into more
#       than one DLL (typically a shared header) is counted once here, so the per-module totals
#       need not sum to the combined total.
#   published.cobertura.xml
#       What gets published to the Code Coverage tab. With -PublishLayout PerModuleFolders (the
#       default) every DLL's files are re-rooted under their own top-level folder (e.g.
#       "MUX - Microsoft.ui.xaml.dll\dxaml\..."), so the tab's Files tree shows one folder per
#       DLL with that DLL's own coverage, and the tab's overview is all DLLs together. ADO
#       recomputes the tab's numbers from the per-file data (merging entries with the same
#       path), so this folder split is the only way to keep the DLLs apart in the tab; the
#       publish task always reports an empty build configuration/platform and OneBranch does not
#       let it attach its HTML report. Trade-off: re-rooted paths no longer match the repository,
#       and a file compiled into several DLLs is counted once per DLL in the overview (the
#       Combined view below counts it once). -PublishLayout Combined keeps real paths instead,
#       with all DLLs merged into one tree.
#   coverage-summary.json / coverage-summary.csv
#       One row per view (Combined first, then each module) - consumed by
#       New-CoverageHtmlReport.ps1 and Merge-CodeCoverage.ps1.
#
# EXPERIMENTAL QUALITY.

[CmdletBinding()]
param(
    # Raw merged Cobertura file(s) from `dotnet-coverage merge ... -f cobertura`. Several files
    # may be passed (e.g. one per module); they are combined before deduping.
    [Parameter(Mandatory = $true)]
    [string[]]$InputCobertura,

    [Parameter(Mandatory = $true)]
    [string]$OutputDir,

    # Shape of published.cobertura.xml - see OUTPUT above.
    [ValidateSet('PerModuleFolders', 'Combined')]
    [string]$PublishLayout = 'PerModuleFolders',

    # Matches the build-agent sources root at the start of a path (C:\__w\1\s\, D:\a\1\s\, ...).
    # PerModuleFolders inserts the DLL folder right after it so the tab still strips the root.
    [string]$SourcesRootPattern = '^[A-Za-z]:[\\/]+(__w|a|_work)[\\/]+\d+[\\/]+s[\\/]+'
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Xml

$inv = [System.Globalization.CultureInfo]::InvariantCulture
New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null

function New-FileMap {
    [pscustomobject]@{
        Files = New-Object 'System.Collections.Generic.Dictionary[string,System.Collections.Generic.Dictionary[int,int]]' ([System.StringComparer]::OrdinalIgnoreCase)
        Order = New-Object 'System.Collections.Generic.List[string]'
    }
}

function Get-LineDict {
    param($FileMap, [string]$File)
    $dict = $null
    if (-not $FileMap.Files.TryGetValue($File, [ref]$dict)) {
        $dict = New-Object 'System.Collections.Generic.Dictionary[int,int]'
        $FileMap.Files[$File] = $dict
        $FileMap.Order.Add($File)
    }
    return , $dict
}

function Get-SafeName {
    param([string]$Name)
    return ($Name -replace '[\\/:*?"<>|]', '_')
}

# -- Parse: module (Cobertura package) -> file -> line -> max hits. ---------------------------
$modules = New-Object 'System.Collections.Generic.Dictionary[string,object]' ([System.StringComparer]::OrdinalIgnoreCase)
$rawClassCount = 0
$rawLineCount = 0
$sw = [System.Diagnostics.Stopwatch]::StartNew()

foreach ($inputPath in $InputCobertura) {
    if (-not (Test-Path $inputPath)) { throw "Input Cobertura file not found: $inputPath" }
    Write-Host "Streaming $inputPath ..."
    $reader = [System.Xml.XmlReader]::Create($inputPath)
    $currentModule = $null
    $currentDict = $null
    $existing = 0
    try {
        while ($reader.Read()) {
            if ($reader.NodeType -ne [System.Xml.XmlNodeType]::Element) { continue }
            switch ($reader.Name) {
                'package' {
                    $name = $reader.GetAttribute('name')
                    if (-not $name) { $name = '(unnamed)' }
                    if (-not $modules.ContainsKey($name)) { $modules[$name] = New-FileMap }
                    $currentModule = $modules[$name]
                    $currentDict = $null
                }
                'class' {
                    $rawClassCount++
                    if ($null -eq $currentModule) {
                        # Defensive: a class outside any <package>. Attribute it to a placeholder.
                        if (-not $modules.ContainsKey('(unnamed)')) { $modules['(unnamed)'] = New-FileMap }
                        $currentModule = $modules['(unnamed)']
                    }
                    $fn = $reader.GetAttribute('filename')
                    if (-not $fn) { $fn = $reader.GetAttribute('name') }
                    $currentDict = Get-LineDict -FileMap $currentModule -File $fn
                }
                'line' {
                    if ($null -eq $currentDict) { continue }
                    $rawLineCount++
                    # Inlined (rather than calling Add-Hits) - this runs once per raw <line>.
                    $num = [int]$reader.GetAttribute('number')
                    $hits = [int]$reader.GetAttribute('hits')
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
}

Write-Host ("Parsed {0} module(s), {1} <class> and {2} <line> elements in {3:N1}s." -f $modules.Count, $rawClassCount, $rawLineCount, $sw.Elapsed.TotalSeconds)
if ($modules.Count -eq 0) {
    throw "No <package> elements found in $($InputCobertura -join ', ') - nothing to report."
}

$moduleNames = @($modules.Keys | Sort-Object { $_.ToLowerInvariant() })

# Combined view: union of every module's lines, deduped globally by (file, line).
$combined = New-FileMap
foreach ($m in $moduleNames) {
    $map = $modules[$m]
    foreach ($fn in $map.Order) {
        $dst = Get-LineDict -FileMap $combined -File $fn
        $existing = 0
        foreach ($kv in $map.Files[$fn].GetEnumerator()) {
            if ($dst.TryGetValue($kv.Key, [ref]$existing)) {
                if ($kv.Value -gt $existing) { $dst[$kv.Key] = $kv.Value }
            } else {
                $dst[$kv.Key] = $kv.Value
            }
        }
    }
}

# -- Writers ----------------------------------------------------------------------------------
function Get-FileStats {
    param($FileMap)
    $rows = New-Object System.Collections.Generic.List[object]
    $total = 0; $covered = 0
    foreach ($fn in $FileMap.Order) {
        $dict = $FileMap.Files[$fn]
        $t = $dict.Count
        $c = 0
        foreach ($h in $dict.Values) { if ($h -gt 0) { $c++ } }
        $total += $t; $covered += $c
        $rows.Add([pscustomobject]@{
            File         = $fn
            CoveredLines = $c
            TotalLines   = $t
            LineCoverage = if ($t -gt 0) { [math]::Round(100.0 * $c / $t, 2) } else { 0 }
        })
    }
    [pscustomobject]@{ Rows = $rows; Total = $total; Covered = $covered }
}

function Write-Cobertura {
    # $Packages: ordered list of @{ Name; FileMap; Stats }
    # $MapPath: optional { param($packageName, $file) } returning the filename to write.
    param([string]$Path, [object[]]$Packages, [long]$Total, [long]$Covered, [scriptblock]$MapPath = $null)

    $rate = if ($Total -gt 0) { $Covered / $Total } else { 0 }
    $settings = New-Object System.Xml.XmlWriterSettings
    $settings.Indent = $false
    $settings.Encoding = [System.Text.UTF8Encoding]::new($false)
    $w = [System.Xml.XmlWriter]::Create($Path, $settings)
    try {
        $w.WriteStartDocument()
        $w.WriteStartElement('coverage')
        $w.WriteAttributeString('line-rate', $rate.ToString('G17', $inv))
        $w.WriteAttributeString('branch-rate', '0')
        $w.WriteAttributeString('lines-covered', $Covered.ToString($inv))
        $w.WriteAttributeString('lines-valid', $Total.ToString($inv))
        $w.WriteAttributeString('branches-covered', '0')
        $w.WriteAttributeString('branches-valid', '0')
        $w.WriteAttributeString('complexity', '0')
        $w.WriteAttributeString('version', '1.9')
        $w.WriteAttributeString('timestamp', [DateTimeOffset]::UtcNow.ToUnixTimeSeconds().ToString($inv))
        $w.WriteStartElement('sources')
        $w.WriteElementString('source', '.')
        $w.WriteEndElement()
        $w.WriteStartElement('packages')
        foreach ($pkg in $Packages) {
            $pkgRate = if ($pkg.Stats.Total -gt 0) { $pkg.Stats.Covered / $pkg.Stats.Total } else { 0 }
            $w.WriteStartElement('package')
            $w.WriteAttributeString('name', $pkg.Name)
            $w.WriteAttributeString('line-rate', $pkgRate.ToString('G17', $inv))
            $w.WriteAttributeString('branch-rate', '0')
            $w.WriteAttributeString('complexity', '0')
            $w.WriteStartElement('classes')
            foreach ($row in $pkg.Stats.Rows) {
                $dict = $pkg.FileMap.Files[$row.File]
                $classRate = if ($row.TotalLines -gt 0) { $row.CoveredLines / $row.TotalLines } else { 0 }
                $fileName = if ($MapPath) { & $MapPath $pkg.Name $row.File } else { $row.File }
                $w.WriteStartElement('class')
                $w.WriteAttributeString('name', $fileName)
                $w.WriteAttributeString('filename', $fileName)
                $w.WriteAttributeString('line-rate', $classRate.ToString('G17', $inv))
                $w.WriteAttributeString('branch-rate', '0')
                $w.WriteAttributeString('complexity', '0')
                $w.WriteStartElement('methods')
                $w.WriteEndElement()
                $w.WriteStartElement('lines')
                foreach ($num in ($dict.Keys | Sort-Object)) {
                    $w.WriteStartElement('line')
                    $w.WriteAttributeString('number', $num.ToString($inv))
                    $w.WriteAttributeString('hits', $dict[$num].ToString($inv))
                    $w.WriteAttributeString('branch', 'false')
                    $w.WriteEndElement()
                }
                $w.WriteEndElement() # lines
                $w.WriteEndElement() # class
            }
            $w.WriteEndElement() # classes
            $w.WriteEndElement() # package
        }
        $w.WriteEndElement() # packages
        $w.WriteEndElement() # coverage
        $w.WriteEndDocument()
    } finally {
        $w.Close()
    }
}

function Write-View {
    param([string]$Id, [string]$Label, [string[]]$Dlls, $Stats)
    $dir = Join-Path $OutputDir (Get-SafeName $Id)
    New-Item -ItemType Directory -Force -Path $dir | Out-Null
    $pct = if ($Stats.Total -gt 0) { [math]::Round(100.0 * $Stats.Covered / $Stats.Total, 2) } else { 0 }
    $summary = [pscustomobject]@{
        View                = $Id
        Label               = $Label
        InstrumentedModules = @($Dlls)
        Files               = $Stats.Rows.Count
        TotalLines          = $Stats.Total
        CoveredLines        = $Stats.Covered
        LineCoverage        = $pct
        PerFileCsv          = (Join-Path (Get-SafeName $Id) 'corrected-per-file.csv')
    }
    $summary | ConvertTo-Json | Set-Content (Join-Path $dir 'corrected-summary.json') -Encoding UTF8
    $Stats.Rows | Sort-Object File | Export-Csv (Join-Path $dir 'corrected-per-file.csv') -NoTypeInformation -Encoding UTF8
    return $summary
}

function Get-DllName {
    param([string]$Module)
    if ($Module -match '\.(dll|exe)$') { return $Module }
    return "$Module.dll"
}

# Folder a DLL's files are re-rooted under in the PerModuleFolders layout.
$friendlyModuleNames = @{ 'microsoft.ui.xaml' = 'MUX'; 'microsoft.ui.xaml.controls' = 'MUXC' }
function Get-ModuleFolder {
    param([string]$Module)
    $dll = Get-DllName $Module
    $alias = $friendlyModuleNames[($Module -replace '\.(dll|exe)$', '').ToLowerInvariant()]
    $folder = if ($alias) { "$alias - $dll" } else { $dll }
    return (Get-SafeName $folder)
}

# -- Emit -------------------------------------------------------------------------------------
$views = New-Object System.Collections.Generic.List[object]
$packages = New-Object System.Collections.Generic.List[object]
$publishedTotal = 0; $publishedCovered = 0

$combinedStats = Get-FileStats -FileMap $combined
$allDlls = @($moduleNames | ForEach-Object { Get-DllName $_ })
$combinedLabel = if ($moduleNames.Count -gt 1) { "Combined ($($allDlls -join ' + '))" } else { "Combined ($($allDlls[0]))" }
$views.Add((Write-View -Id 'Combined' -Label $combinedLabel -Dlls $allDlls -Stats $combinedStats))

foreach ($m in $moduleNames) {
    $stats = Get-FileStats -FileMap $modules[$m]
    $summary = Write-View -Id $m -Label (Get-DllName $m) -Dlls @(Get-DllName $m) -Stats $stats
    $views.Add($summary)
    $pkg = [pscustomobject]@{ Name = $m; FileMap = $modules[$m]; Stats = $stats }
    $packages.Add($pkg)
    $publishedTotal += $stats.Total; $publishedCovered += $stats.Covered
    Write-Cobertura -Path (Join-Path $OutputDir (Join-Path (Get-SafeName $m) 'corrected.cobertura.xml')) -Packages @($pkg) -Total $stats.Total -Covered $stats.Covered
}

$publishedPath = Join-Path $OutputDir 'published.cobertura.xml'
if ($PublishLayout -eq 'PerModuleFolders') {
    # Keep the agent's sources root (so the tab still strips it) and insert the DLL folder after
    # it. Paths outside the sources root (SDK/OS sources, intermediate dirs) go under
    # <DLL folder>\_outside-sources\ instead of being dropped by the tab.
    $sourcesRegex = New-Object System.Text.RegularExpressions.Regex($SourcesRootPattern, 'IgnoreCase')
    $sourcesRoot = ''
    foreach ($m in $moduleNames) {
        $first = $modules[$m].Order | Where-Object { $sourcesRegex.IsMatch($_) } | Select-Object -First 1
        if ($first) { $sourcesRoot = $sourcesRegex.Match($first).Value; break }
    }
    $folders = @{}
    foreach ($m in $moduleNames) { $folders[$m] = Get-ModuleFolder $m }
    $mapPath = {
        param($packageName, $file)
        $folder = $folders[$packageName]
        $match = $sourcesRegex.Match($file)
        if ($match.Success) {
            return $sourcesRoot + $folder + '\' + $file.Substring($match.Length)
        }
        $rest = ($file -replace '^[.\\/]+', '' -replace ':', '').TrimStart('\', '/')
        return $sourcesRoot + $folder + '\_outside-sources\' + $rest
    }.GetNewClosure()
    # ADO sums the re-rooted files, so the file's own totals use the per-DLL sum too.
    Write-Cobertura -Path $publishedPath -Packages $packages.ToArray() -Total $publishedTotal -Covered $publishedCovered -MapPath $mapPath
}
else {
    # Real paths, one package per DLL. ADO merges same-path entries, so the tab shows the
    # Combined (globally deduped) numbers.
    Write-Cobertura -Path $publishedPath -Packages $packages.ToArray() -Total $combinedStats.Total -Covered $combinedStats.Covered
}

$views | ConvertTo-Json -Depth 4 | Set-Content (Join-Path $OutputDir 'coverage-summary.json') -Encoding UTF8
$views | Select-Object View, Label, Files, TotalLines, CoveredLines, LineCoverage |
    Export-Csv (Join-Path $OutputDir 'coverage-summary.csv') -NoTypeInformation -Encoding UTF8

Write-Host "`n=== Corrected coverage (deduped by file+line; matches the ADO Code Coverage tab) ==="
foreach ($v in $views) {
    Write-Host ("  {0,-60} {1,6:N2}%  ({2:N0} / {3:N0} lines, {4:N0} files)" -f $v.Label, $v.LineCoverage, $v.CoveredLines, $v.TotalLines, $v.Files)
}
if ($moduleNames.Count -gt 1) {
    $sumFiles = ($views | Where-Object { $_.View -ne 'Combined' } | Measure-Object -Property Files -Sum).Sum
    $shared = $sumFiles - $combinedStats.Rows.Count
    if ($shared -gt 0) {
        Write-Host "  ($shared source file(s) are compiled into more than one DLL; Combined counts each of them once.)"
    }
}
if ($PublishLayout -eq 'PerModuleFolders' -and $moduleNames.Count -gt 1) {
    $tabPct = if ($publishedTotal -gt 0) { 100.0 * $publishedCovered / $publishedTotal } else { 0 }
    Write-Host ("  Code Coverage tab overview (each DLL's files under its own folder, summed): {0:N2}% ({1:N0} / {2:N0} lines)" -f $tabPct, $publishedCovered, $publishedTotal)
}
Write-Host "Published Cobertura ($PublishLayout): $publishedPath"
