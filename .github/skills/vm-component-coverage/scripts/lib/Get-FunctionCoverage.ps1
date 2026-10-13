# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
#
# Get-FunctionCoverage.ps1
#
# Produces a FUNCTION-LEVEL coverage breakdown ("which functions in this file were exercised by
# the tests, and by how much") - one row per C++ function/overload, not just one row per source
# file. This is a companion to (not a replacement for) Repair-CoverageDuplicates.ps1, which only
# ever reconstructs FILE-level totals because Cobertura (the format that script's input is in)
# collapses per-function detail down to one <class> per source file. To get real function
# boundaries this script uses a different raw input: the coverage merge engine's native "xml"
# output format (`dotnet-coverage merge -f xml` / `Microsoft.CodeCoverage.Console merge -f xml`),
# which - unlike Cobertura - keeps one <function> element per C++ function/template instantiation
# (name, namespace, containing class/type, and its own per-line <range> hit/miss list), with a
# <source_files> table mapping each line back to a real source path.
#
# THIS SCRIPT WORKS DIRECTLY FROM THE RAW PER-SLICE .coverage FILES - it does NOT need the
# product DLL/PDB. That symbol info (function names, namespaces, source file + line numbers) is
# already embedded in each .coverage file at collection time; this was explicitly verified by
# stripping the DLL/PDB out of a test run folder and re-merging - the function-level XML still
# resolved perfectly.
#
# PER-FUNCTION DEDUPLICATION: within a single function's own <ranges> list, this script keeps the
# max covered-state (yes > partial > no) seen per (source file, line number) - guarding against
# the same duplicate-range-on-merge defect Repair-CoverageDuplicates.ps1 fixes for Cobertura, in
# case it ever resurfaces for a given function. In practice, `dotnet-coverage merge -f xml` was
# observed to already correctly union same-function-id ranges across contributing slices without
# duplication (a real build's `AccessKey.cpp`, merged from all 20 slices, summed its 8 functions'
# corrected totals to exactly 52 - matching that file's independently-verified true line count) -
# so this per-function dedup is mostly a defensive safety net, not something that was seen to
# fire heavily in practice.
#
# IMPORTANT NUANCE - per-function totals do NOT sum to the file's true unique-line total, and
# THAT IS EXPECTED, NOT A BUG: in C++, many distinct COMPILED functions can legitimately share
# the same physical source line - template instantiations (e.g. `_tlgCreate1Sz<char>` and
# `_tlgCreate1Sz<wchar_t>` are two separate compiled functions that both report the identical 9
# header lines of their shared template body), inlined helpers, and macro expansions are the
# common causes. Verified on a real build: TraceLoggingProvider.h has 163 distinct instantiated
# functions whose own totals sum to 593 lines, even though the header itself has far fewer
# physical lines - each instantiation is legitimately "using" the same textual lines as its
# siblings. Because of this, DO NOT use this script's per-function rows to re-derive a file's
# true aggregate line coverage - use the existing, Azure-DevOps-verified `corrected-per-file.csv`
# (from Repair-CoverageDuplicates.ps1 / Split-CoverageByModule.ps1) for that; pass it via
# -ExistingPerFileCsv to have this script merge those authoritative file totals alongside the new
# per-function/function-count breakdown in corrected-per-file-function-summary.csv.
#
# OUTPUT (under -OutputDir)
#   NativeMerge\merged-native.xml       - raw dotnet-coverage -f xml merge.
#   corrected-per-function.csv          - one row per function: File, Namespace, Type, Function,
#                                          TotalLines, CoveredLines, FullyCoveredLines,
#                                          PartiallyCoveredLines, NotCoveredLines,
#                                          LineCoveragePct (all "own view" totals for that one
#                                          compiled function - see the nuance above before
#                                          summing this column across functions).
#                                          Sorted by File then Function.
#   corrected-per-file-function-summary.csv - one row per source file: TotalFunctions,
#                                          FunctionsWithAnyCoverage, FunctionsFullyCovered,
#                                          FunctionsNotCovered - plus, only when
#                                          -ExistingPerFileCsv is supplied, that file's
#                                          authoritative FileTotalLines/FileCoveredLines/
#                                          FileLineCoveragePct joined in from corrected-per-file.csv.
#
# EXAMPLE (all .coverage files under a folder, joining in the authoritative per-file totals that
# Split-CoverageByModule.ps1 already computed for them)
#   .\Get-FunctionCoverage.ps1 -SliceDir C:\cov\run1\raw `
#       -OutputDir C:\cov\run1\FunctionCoverage `
#       -ExistingPerFileCsv C:\cov\run1\PerModule\Microsoft.UI.Xaml.Controls\corrected-per-file.csv
#
# EXAMPLE (explicit list of .coverage files)
#   .\Get-FunctionCoverage.ps1 -CoverageFiles file1.coverage,file2.coverage -OutputDir .\out

[CmdletBinding(DefaultParameterSetName = 'FromSliceDir')]
param(
    # Directory to search recursively for raw *.coverage files (e.g. a folder holding one
    # .coverage file per test run or slice). All matches are merged together.
    [Parameter(Mandatory = $true, ParameterSetName = 'FromSliceDir')]
    [string]$SliceDir,

    # Explicit list of raw .coverage files to merge (alternative to -SliceDir).
    [Parameter(Mandatory = $true, ParameterSetName = 'FromFiles')]
    [string[]]$CoverageFiles,

    # Where to write outputs. Created if it doesn't exist.
    [Parameter(Mandatory = $true)]
    [string]$OutputDir,

    # Explicit path to dotnet-coverage / Microsoft.CodeCoverage.Console.exe. Auto-resolved
    # (dotnet-coverage on PATH, else VS's Microsoft.CodeCoverage.Console.exe, else auto-installs
    # dotnet-coverage) if not supplied.
    [string]$CoverageToolPath = "",

    # Reuse an already-produced NativeMerge\merged-native.xml from a prior run instead of
    # re-merging (useful for iterating on the dedup/CSV logic without re-running the - often
    # multi-minute - merge step).
    [switch]$SkipMerge,

    # Count a "partially covered" line (some but not all blocks on that line executed - common
    # with multiple statements/branches per line in C++) as covered when computing CoveredLines
    # and LineCoveragePct. On by default (matches the common-sense "was this line touched at
    # all" reading). Fully-covered and partially-covered are still also broken out into their
    # own separate columns regardless of this switch, so no information is lost either way.
    [bool]$TreatPartialAsCovered = $true,

    # Keep NativeMerge\merged-native.xml after the run instead of leaving it (it is NOT deleted
    # by default - this file is the expensive-to-regenerate one, so it is kept unless this switch
    # is explicitly reversed by passing -KeepIntermediateXml:$false).
    [bool]$KeepIntermediateXml = $true,

    # Optional path to an existing corrected-per-file.csv (as produced by
    # Repair-CoverageDuplicates.ps1 / Split-CoverageByModule.ps1). When supplied, that file's
    # already Azure-DevOps-verified per-file TotalLines/CoveredLines/LineCoverage are joined into
    # corrected-per-file-function-summary.csv as FileTotalLines/FileCoveredLines/
    # FileLineCoveragePct, alongside this script's own function-count columns. Recommended -
    # without it, the summary only has function counts (no line totals), by design, to avoid
    # presenting a second, non-equivalent "file total" next to the one everyone already trusts
    # (see the "IMPORTANT NUANCE" note above).
    [string]$ExistingPerFileCsv = ""
)

$ErrorActionPreference = 'Stop'

$nativeMergeDir = Join-Path $OutputDir 'NativeMerge'
New-Item -ItemType Directory -Force -Path $OutputDir, $nativeMergeDir | Out-Null
$nativeXmlPath = Join-Path $nativeMergeDir 'merged-native.xml'

# ---------------------------------------------------------------------------------------------
# 1. Resolve a coverage merge tool (dotnet-coverage on PATH, else VS's console tool, else install).
# ---------------------------------------------------------------------------------------------
function Resolve-CoverageTool {
    param([string]$Explicit)
    if ($Explicit -and (Test-Path $Explicit)) { return $Explicit }
    $dc = Get-Command 'dotnet-coverage' -ErrorAction SilentlyContinue
    if ($dc) { return $dc.Source }
    $vs = Get-ChildItem -Path @(
        "${env:ProgramFiles}\Microsoft Visual Studio",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio"
    ) -Recurse -Filter 'Microsoft.CodeCoverage.Console.exe' -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($vs) { return $vs.FullName }
    Write-Host "dotnet-coverage not found; installing the global tool..."
    & dotnet tool install --global dotnet-coverage --add-source https://api.nuget.org/v3/index.json --ignore-failed-sources | Write-Host
    $env:PATH = "$env:PATH;$env:USERPROFILE\.dotnet\tools"
    $dc = Get-Command 'dotnet-coverage' -ErrorAction SilentlyContinue
    if ($dc) { return $dc.Source }
    throw "Could not find or install a code coverage merge tool (dotnet-coverage or Microsoft.CodeCoverage.Console.exe)."
}

# ---------------------------------------------------------------------------------------------
# 2. Merge raw .coverage files into the native (function-granular) xml format.
# ---------------------------------------------------------------------------------------------
if (-not $SkipMerge) {
    $inputFiles = if ($PSCmdlet.ParameterSetName -eq 'FromSliceDir') {
        Write-Host "=== Discovering raw .coverage files under $SliceDir ==="
        @(Get-ChildItem $SliceDir -Recurse -Filter '*.coverage' -ErrorAction SilentlyContinue | Select-Object -ExpandProperty FullName)
    } else {
        @($CoverageFiles)
    }
    if ($inputFiles.Count -eq 0) {
        throw "No .coverage files found (checked $(if ($PSCmdlet.ParameterSetName -eq 'FromSliceDir') { $SliceDir } else { '-CoverageFiles list' }))."
    }
    Write-Host "Merging $($inputFiles.Count) raw .coverage file(s) into the native function-granular xml format..."
    Write-Host "(This does NOT require the product DLL/PDB - function/line symbol info is already embedded in each .coverage file.)"

    $covTool = Resolve-CoverageTool -Explicit $CoverageToolPath
    Write-Host "Using: $covTool"

    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    & $covTool merge @inputFiles -f xml -o $nativeXmlPath --nologo | Write-Host
    if ($LASTEXITCODE -ne 0) { throw "Native xml merge failed (exit $LASTEXITCODE)." }
    Write-Host ("Merge complete in {0:N1}s -> {1} ({2:N0} MB)" -f $sw.Elapsed.TotalSeconds, $nativeXmlPath, ((Get-Item $nativeXmlPath).Length / 1MB))
    Write-Host "NOTE: this raw merged file still has the duplicate-range bug - do not read its <module>/<function> line/block-rate attributes directly; this script re-derives corrected values below."
} else {
    if (-not (Test-Path $nativeXmlPath)) { throw "-SkipMerge was set but $nativeXmlPath does not exist." }
    Write-Host "=== -SkipMerge set: reusing existing $nativeXmlPath ==="
}

# ---------------------------------------------------------------------------------------------
# 3. Stream-parse the native xml once, deduplicating as we go:
#      - source_files: source_id -> file path
#      - functions: function id -> aggregated { Name, Namespace, TypeName, per-(sourceId,line)
#        max covered-state, per-sourceId line-count (to pick the function's "primary" file) }
#    A single forward-only XmlReader pass keeps this feasible on multi-hundred-MB inputs without
#    loading a full DOM.
# ---------------------------------------------------------------------------------------------
Write-Host "`n=== Parsing and deduplicating $nativeXmlPath ==="
$sw = [System.Diagnostics.Stopwatch]::StartNew()

Add-Type -Language CSharp -TypeDefinition @"
using System.Collections.Generic;
public class FuncAgg {
    public string Name;
    public string Namespace;
    public string TypeName;
    // "sourceId|line" -> state (2=yes, 1=partial, 0=no)
    public Dictionary<string,int> LineStates = new Dictionary<string,int>();
    // sourceId -> count of distinct lines seen for that source, used to pick the "primary" file.
    public Dictionary<string,int> SourceLineCounts = new Dictionary<string,int>();
}
"@

$sourceFiles = New-Object 'System.Collections.Generic.Dictionary[string,string]'
$functions = New-Object 'System.Collections.Generic.Dictionary[string,FuncAgg]'

$reader = [System.Xml.XmlReader]::Create($nativeXmlPath)
$currentFunc = $null
$rawFunctionElementCount = 0
$rawRangeElementCount = 0
try {
    while ($reader.Read()) {
        if ($reader.NodeType -ne [System.Xml.XmlNodeType]::Element) { continue }

        switch ($reader.Name) {
            'source_file' {
                $id = $reader.GetAttribute('id')
                $path = $reader.GetAttribute('path')
                if ($id -and -not $sourceFiles.ContainsKey($id)) { $sourceFiles[$id] = $path }
            }
            'function' {
                $rawFunctionElementCount++
                $id = $reader.GetAttribute('id')
                $agg = $null
                if (-not $functions.TryGetValue($id, [ref]$agg)) {
                    $agg = New-Object FuncAgg
                    $agg.Name = $reader.GetAttribute('name')
                    $agg.Namespace = $reader.GetAttribute('namespace')
                    $agg.TypeName = $reader.GetAttribute('type_name')
                    $functions[$id] = $agg
                }
                $currentFunc = $agg
            }
            'range' {
                $rawRangeElementCount++
                if (-not $currentFunc) { continue }
                $sid = $reader.GetAttribute('source_id')
                $sln = [int]$reader.GetAttribute('start_line')
                $eln = [int]$reader.GetAttribute('end_line')
                $cov = $reader.GetAttribute('covered')
                $state = if ($cov -eq 'yes') { 2 } elseif ($cov -eq 'partial') { 1 } else { 0 }
                for ($ln = $sln; $ln -le $eln; $ln++) {
                    $key = "$sid|$ln"
                    $existing = 0
                    if ($currentFunc.LineStates.TryGetValue($key, [ref]$existing)) {
                        if ($state -gt $existing) { $currentFunc.LineStates[$key] = $state }
                    } else {
                        $currentFunc.LineStates[$key] = $state
                        $cnt = 0
                        [void]$currentFunc.SourceLineCounts.TryGetValue($sid, [ref]$cnt)
                        $currentFunc.SourceLineCounts[$sid] = $cnt + 1
                    }
                }
            }
        }
    }
} finally {
    $reader.Close()
}
Write-Host ("Parsed {0:N0} raw <function> elements ({1:N0} unique) and {2:N0} raw <range> elements in {3:N1}s." -f $rawFunctionElementCount, $functions.Count, $rawRangeElementCount, $sw.Elapsed.TotalSeconds)

# ---------------------------------------------------------------------------------------------
# 4. Compute corrected per-function totals, attributing each function to its "primary" file
#    (whichever source_id contributed the most distinct lines) and only counting that file's
#    lines toward the function's own totals - this keeps a constructor's one stray reference to
#    a header's default-member-initializer line, for example, from polluting its own .cpp totals.
# ---------------------------------------------------------------------------------------------
Write-Host "`n=== Computing corrected per-function totals ==="
$perFunctionRows = New-Object System.Collections.Generic.List[object]
foreach ($kvp in $functions.GetEnumerator()) {
    $agg = $kvp.Value
    if ($agg.SourceLineCounts.Count -eq 0) { continue }
    $primarySourceId = ($agg.SourceLineCounts.GetEnumerator() | Sort-Object -Property Value -Descending | Select-Object -First 1).Key
    $filePath = $null
    if (-not $sourceFiles.TryGetValue($primarySourceId, [ref]$filePath)) { $filePath = "<unknown source_id $primarySourceId>" }

    $total = 0; $fully = 0; $partial = 0
    foreach ($entry in $agg.LineStates.GetEnumerator()) {
        if (-not $entry.Key.StartsWith("$primarySourceId|")) { continue }
        $total++
        if ($entry.Value -eq 2) { $fully++ }
        elseif ($entry.Value -eq 1) { $partial++ }
    }
    $notCovered = $total - $fully - $partial
    $covered = if ($TreatPartialAsCovered) { $fully + $partial } else { $fully }
    $pct = if ($total -gt 0) { [math]::Round(100.0 * $covered / $total, 2) } else { 0 }

    $perFunctionRows.Add([pscustomobject]@{
        File                  = $filePath
        Namespace             = $agg.Namespace
        Type                  = $agg.TypeName
        Function              = $agg.Name
        TotalLines            = $total
        CoveredLines          = $covered
        FullyCoveredLines     = $fully
        PartiallyCoveredLines = $partial
        NotCoveredLines       = $notCovered
        LineCoveragePct       = $pct
    })
}
Write-Host "Computed totals for $($perFunctionRows.Count) function(s)."

# ---------------------------------------------------------------------------------------------
# 5. Roll up to a per-file function summary. This intentionally reports FUNCTION COUNTS only
#    (not a re-derived line total - see the "IMPORTANT NUANCE" note at the top of this script for
#    why summing per-function line totals is not a valid stand-in for a file's true unique-line
#    total in C++). If -ExistingPerFileCsv was supplied, that already-verified file-level total
#    is joined in alongside the function counts.
# ---------------------------------------------------------------------------------------------
Write-Host "`n=== Rolling up to per-file function summary ==="
$existingPerFile = $null
if ($ExistingPerFileCsv) {
    if (-not (Test-Path $ExistingPerFileCsv)) { throw "-ExistingPerFileCsv path not found: $ExistingPerFileCsv" }
    $existingPerFile = @{}
    foreach ($row in (Import-Csv $ExistingPerFileCsv)) { $existingPerFile[$row.File] = $row }
    Write-Host "Joining authoritative file totals from $ExistingPerFileCsv ($($existingPerFile.Count) file(s))."
}

$perFileSummary = $perFunctionRows | Group-Object -Property File | ForEach-Object {
    $rows = $_.Group
    $row = [ordered]@{
        File                     = $_.Name
        TotalFunctions           = $rows.Count
        FunctionsWithAnyCoverage = ($rows | Where-Object { $_.CoveredLines -gt 0 }).Count
        FunctionsFullyCovered    = ($rows | Where-Object { $_.TotalLines -gt 0 -and $_.CoveredLines -eq $_.TotalLines }).Count
        FunctionsNotCovered      = ($rows | Where-Object { $_.CoveredLines -eq 0 }).Count
    }
    if ($existingPerFile) {
        $match = if ($existingPerFile.ContainsKey($_.Name)) { $existingPerFile[$_.Name] } else { $null }
        $row['FileTotalLines'] = if ($match) { $match.TotalLines } else { '' }
        $row['FileCoveredLines'] = if ($match) { $match.CoveredLines } else { '' }
        $row['FileLineCoveragePct'] = if ($match) { $match.LineCoverage } else { '' }
    }
    [pscustomobject]$row
}

# ---------------------------------------------------------------------------------------------
# 6. Write outputs.
# ---------------------------------------------------------------------------------------------
$perFunctionCsvPath = Join-Path $OutputDir 'corrected-per-function.csv'
$perFileSummaryCsvPath = Join-Path $OutputDir 'corrected-per-file-function-summary.csv'

$perFunctionRows | Sort-Object File, Function | Export-Csv $perFunctionCsvPath -NoTypeInformation
$perFileSummary | Sort-Object File | Export-Csv $perFileSummaryCsvPath -NoTypeInformation

if (-not $KeepIntermediateXml) {
    Remove-Item $nativeXmlPath -Force -ErrorAction SilentlyContinue
}

Write-Host "`n=== Done ==="
Write-Host "  $perFunctionCsvPath"
Write-Host "  $perFileSummaryCsvPath"
if ($KeepIntermediateXml) { Write-Host "  $nativeXmlPath (kept; pass -KeepIntermediateXml `$false to discard)" }
