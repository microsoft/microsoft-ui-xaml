<#
.SYNOPSIS
    Step 5 - Turn the downloaded .coverage file(s) into a component-scoped coverage report.

.DESCRIPTION
    Reuses the (unmodified) scripts from the SkillToGetCodeCoverageFromPipeline skill, copied into
    .\lib:
      * dotnet-coverage merge -f cobertura           -> merged.cobertura.xml
      * lib\Split-CoverageByModule.ps1               -> per-module, (file,line)-deduplicated numbers
                                                        (same correction that makes numbers match ADO)
      * lib\Build-CoverageTreeReport.ps1             -> single-file HTML tree for the component
      * lib\Get-FunctionCoverage.ps1                 -> per-function coverage (filtered to the component)
    and adds:
      * component filtering (-ComponentPathRegex, matched against the source path),
      * an optional before/after comparison against a baseline corrected-per-file.csv (e.g. from a
        CI coverage build), matched on the repo-relative path so build-agent prefixes
        (C:\__w\1\s\...) and flavors (x86chk/amd64chk) do not matter,
      * coverage-summary.md + component-per-file.csv + component-functions.csv.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string[]]$CoverageFiles,
    [Parameter(Mandatory)][string]$OutputDir,
    [string]$Module = 'Microsoft.UI.Xaml.Controls',
    # Regex on the source file path selecting the component's files.
    [string]$ComponentPathRegex = '\\controls\\dev\\TitleBar\\|\\Generated\\TitleBar|\\generated\\Controls\\TitleBar',
    [string]$ComponentName = 'TitleBar',
    [string]$BaselinePerFileCsv = '',
    [string]$BaselineLabel = 'Baseline',
    [string]$RunLabel = 'This run',
    [switch]$SkipFunctionCoverage
)
$ErrorActionPreference = 'Stop'
function Info($m) { Write-Host "[report] $m" }
$lib = Join-Path $PSScriptRoot 'lib'

$dc = (Get-Command dotnet-coverage -EA SilentlyContinue).Source
if (-not $dc) { throw 'dotnet-coverage not found: dotnet tool install --global dotnet-coverage' }
New-Item -ItemType Directory -Force $OutputDir | Out-Null
$OutputDir = (Resolve-Path $OutputDir).Path

# ---- merge -> cobertura -------------------------------------------------------------------------
$cob = Join-Path $OutputDir 'merged.cobertura.xml'
Info "merging $($CoverageFiles.Count) file(s) -> $cob"
& $dc merge @CoverageFiles -f cobertura -o $cob --nologo 2>&1 | Select-Object -Last 3 | ForEach-Object { Info "  $_" }
if (-not (Test-Path $cob)) { throw 'merge produced no cobertura file' }

# ---- per-module dedup (verified to match ADO's coverage tab) -------------------------------------
$perModule = Join-Path $OutputDir 'PerModule'
& (Join-Path $lib 'Split-CoverageByModule.ps1') -InputCobertura $cob -OutputDir $perModule -Modules $Module | Out-Null
$modDir = Get-ChildItem $perModule -Directory | Where-Object { $_.Name -ieq $Module } | Select-Object -First 1
if (-not $modDir) { throw "Module $Module not found in coverage (modules: $((Get-ChildItem $perModule -Directory).Name -join ', '))" }
$modCsv = Join-Path $modDir.FullName 'corrected-per-file.csv'
$modSummary = Get-Content (Join-Path $modDir.FullName 'corrected-summary.json') -Raw | ConvertFrom-Json

function Get-RepoKey([string]$p) {
    # repo-relative key: strip everything up to the repo root, and normalize build flavor dirs
    $k = $p.ToLowerInvariant() -replace '/', '\'
    $k = $k -replace '^.*?\\(?=(controls|dxaml|buildoutput|src|test)\\)', ''
    $k -replace '\\obj\\(x86|amd64|arm64)(chk|fre)\\', '\obj\<flavor>\'
}

$rows = Import-Csv $modCsv | Where-Object { $_.File -match $ComponentPathRegex }
if (-not $rows) { throw "No files matched -ComponentPathRegex '$ComponentPathRegex' in $Module coverage." }
$base = @{}
if ($BaselinePerFileCsv) {
    Import-Csv $BaselinePerFileCsv | Where-Object { $_.File -match $ComponentPathRegex } | ForEach-Object { $base[(Get-RepoKey $_.File)] = $_ }
}
$comp = foreach ($r in $rows) {
    $k = Get-RepoKey $r.File
    $b = $base[$k]
    [pscustomobject]@{
        File = $k
        CoveredLines = [int]$r.CoveredLines; TotalLines = [int]$r.TotalLines
        LineCoverage = if ([int]$r.TotalLines) { [math]::Round(100.0 * $r.CoveredLines / $r.TotalLines, 2) } else { 0 }
        BaselineCovered = if ($b) { [int]$b.CoveredLines } else { $null }
        BaselineTotal = if ($b) { [int]$b.TotalLines } else { $null }
        BaselineCoverage = if ($b -and [int]$b.TotalLines) { [math]::Round(100.0 * $b.CoveredLines / $b.TotalLines, 2) } else { $null }
        SourcePath = $r.File
    }
}
$comp = $comp | Sort-Object File
$comp | Export-Csv (Join-Path $OutputDir 'component-per-file.csv') -NoTypeInformation
$tot = ($comp | Measure-Object TotalLines -Sum).Sum
$cov = ($comp | Measure-Object CoveredLines -Sum).Sum
$pct = if ($tot) { [math]::Round(100.0 * $cov / $tot, 2) } else { 0 }

# HTML tree for the component (reuses the pipeline skill renderer; it needs File,CoveredLines,TotalLines)
$treeCsv = Join-Path $OutputDir 'component-tree-input.csv'
$comp | Select-Object @{n='File';e={$_.SourcePath}}, CoveredLines, TotalLines, LineCoverage | Export-Csv $treeCsv -NoTypeInformation
& (Join-Path $lib 'Build-CoverageTreeReport.ps1') -InputCsv $treeCsv -OutputDir (Join-Path $OutputDir 'TreeReport') `
    -Title "$ComponentName coverage ($RunLabel)" -ModuleNames "$Module.dll" | Out-Null

# ---- function coverage (optional) -----------------------------------------------------------------
$funcRows = @()
if (-not $SkipFunctionCoverage) {
    $fcDir = Join-Path $OutputDir 'FunctionCoverage'
    try {
        # Pre-merge here and call the reused script with -SkipMerge: its own merge passes the inputs
        # via "@inputFiles" after an if-expression, which unwraps a SINGLE file to a scalar string and
        # splats nothing (empty <results/>). The pipeline always had 20 slices, so it never hit this.
        $native = Join-Path $fcDir 'NativeMerge\merged-native.xml'
        New-Item -ItemType Directory -Force (Split-Path $native) | Out-Null
        & $dc merge @CoverageFiles -f xml -o $native --nologo 2>&1 | Select-Object -Last 1 | ForEach-Object { Info "  $_" }
        & (Join-Path $lib 'Get-FunctionCoverage.ps1') -CoverageFiles $CoverageFiles -OutputDir $fcDir -ExistingPerFileCsv $modCsv -SkipMerge | Out-Null
        $fcCsv = Join-Path $fcDir 'corrected-per-function.csv'
        if (Test-Path $fcCsv) {
            $funcRows = Import-Csv $fcCsv | Where-Object { $_.File -match $ComponentPathRegex } |
                Sort-Object File, @{e={[double]$_.LineCoveragePct}}
            $funcRows | Export-Csv (Join-Path $OutputDir 'component-functions.csv') -NoTypeInformation
        }
    } catch { Write-Warning "function coverage failed: $_" }
}

# ---- summary markdown -----------------------------------------------------------------------------
$md = New-Object Text.StringBuilder
[void]$md.AppendLine("# $ComponentName code coverage - $RunLabel")
[void]$md.AppendLine()
[void]$md.AppendLine("Module: ``$Module.dll`` | Coverage file(s): $($CoverageFiles.Count) | Generated: $(Get-Date -Format 'yyyy-MM-dd HH:mm')")
[void]$md.AppendLine()
[void]$md.AppendLine("**$ComponentName total: $cov / $tot lines = $pct %**  (whole module in this run: $($modSummary.CoveredLines) / $($modSummary.TotalLines) = $($modSummary.LineCoverage) %)")
if ($BaselinePerFileCsv) {
    $bt = ($comp | Where-Object { $null -ne $_.BaselineTotal } | Measure-Object BaselineTotal -Sum).Sum
    $bc = ($comp | Where-Object { $null -ne $_.BaselineTotal } | Measure-Object BaselineCovered -Sum).Sum
    if ($bt) { [void]$md.AppendLine("`n$BaselineLabel for the same files: $bc / $bt = $([math]::Round(100.0*$bc/$bt,2)) %") }
}
[void]$md.AppendLine()
[void]$md.AppendLine("| File | Covered / Total | $RunLabel % | $BaselineLabel % |")
[void]$md.AppendLine('|---|---|---|---|')
foreach ($c in $comp) {
    $bp = if ($null -ne $c.BaselineCoverage) { "$($c.BaselineCoverage) % ($($c.BaselineCovered)/$($c.BaselineTotal))" } else { 'n/a' }
    [void]$md.AppendLine("| ``$($c.File)`` | $($c.CoveredLines) / $($c.TotalLines) | **$($c.LineCoverage) %** | $bp |")
}
if ($funcRows) {
    $hand = $funcRows | Where-Object { $_.File -match '\\controls\\dev\\' + [regex]::Escape($ComponentName) + '\\[^\\]+\.cpp$' }
    $unc = @($hand | Where-Object { [int]$_.CoveredLines -eq 0 })
    $part = @($hand | Where-Object { [int]$_.CoveredLines -gt 0 -and [int]$_.CoveredLines -lt [int]$_.TotalLines })
    [void]$md.AppendLine()
    [void]$md.AppendLine("## Functions in $ComponentName .cpp files")
    [void]$md.AppendLine("$(@($hand).Count) functions: $(@($hand).Count - $unc.Count - $part.Count) fully covered, $($part.Count) partially, $($unc.Count) not covered.")
    if ($unc) {
        [void]$md.AppendLine("`n**Not covered:**")
        $unc | ForEach-Object { [void]$md.AppendLine("- ``$($_.Function)`` ($(Split-Path $_.File -Leaf), $($_.TotalLines) lines)") }
    }
    if ($part) {
        [void]$md.AppendLine("`n**Partially covered (lowest first):**")
        $part | Select-Object -First 25 | ForEach-Object { [void]$md.AppendLine("- ``$($_.Function)`` $($_.CoveredLines)/$($_.TotalLines) ($($_.LineCoveragePct) %)") }
    }
}
[void]$md.AppendLine()
[void]$md.AppendLine('Files: `component-per-file.csv`, `component-functions.csv`, `TreeReport\coverage-tree.html`, `PerModule\`, `merged.cobertura.xml`.')
$md.ToString() | Set-Content (Join-Path $OutputDir 'coverage-summary.md') -Encoding UTF8
Info "$ComponentName : $cov / $tot = $pct %  -> $(Join-Path $OutputDir 'coverage-summary.md')"
[pscustomobject]@{ component = $ComponentName; coveredLines = $cov; totalLines = $tot; lineCoveragePct = $pct
                   summary = (Join-Path $OutputDir 'coverage-summary.md'); perFile = (Join-Path $OutputDir 'component-per-file.csv') }
