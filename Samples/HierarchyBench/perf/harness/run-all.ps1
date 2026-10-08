<#
.SYNOPSIS
  Multi-executable TableView perf benchmark orchestrator.

.DESCRIPTION
  Maps each requested control to the benchmark executable that implements it, runs
  sampler.py for every control/scenario/tier/run cell, appends to one CSV, then
  renders an HTML report with report.py. Supports -WhatIf for argument and matrix
  validation without launching benchmarks.
#>
[CmdletBinding(SupportsShouldProcess = $true)]
param(
  [string[]]$Controls = @("DataGrid", "ListBox", "TableView", "ListView", "ItemsView", "WinUITableView"),
  [string[]]$Scenarios = @("load", "scroll", "sort", "filter", "resize", "select"),
  [object[]]$Tiers = @("xlarge", "wide"),
  [int]$Runs = 5,
  [string]$Out,
  [string]$ReportOut,
  [int]$SettleMs = 1500,
  [switch]$Gpu,
  [switch]$NoReport,
  [ValidateSet("off", "on", "both")][string]$ColVirt = "off",
  [string]$ControlVersion = "TableView v1",
  # Hierarchy benches only (TableViewKeyBy/Combined/Children*): tree shape, forwarded as --shape.
  [ValidateSet("wide", "deep", "balanced", "shallow")][string]$Shape = "balanced"
)

# Results default to the shared OneDrive folder so an instance on another machine (same OneDrive)
# loads the same reports. Override with TVPERF_RESULTS, or -Out/-ReportOut. Falls back to ./results.
$ResultsDir = if ($env:TVPERF_RESULTS) { $env:TVPERF_RESULTS }
              elseif ($env:OneDrive) { Join-Path $env:OneDrive "Documents\PerfBenchResults" }
              else { Join-Path $PSScriptRoot "..\results" }
if (-not (Test-Path $ResultsDir)) { New-Item -ItemType Directory -Force -Path $ResultsDir | Out-Null }
if (-not $Out) { $Out = Join-Path $ResultsDir "results.csv" }
if (-not $ReportOut) { $ReportOut = Join-Path $ResultsDir "report.html" }

$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$python = (Get-Command python -ErrorAction Stop).Source

# The L1 sampler depends on psutil; ensure it's present (portable across machines).
& $python -c "import psutil" 2>$null
if ($LASTEXITCODE -ne 0) {
  Write-Host "psutil not found - installing (python -m pip install --user psutil) ..."
  & $python -m pip install --quiet --user psutil 2>&1 | Out-Null
  & $python -c "import psutil" 2>$null
  if ($LASTEXITCODE -ne 0) { throw "psutil is required for the L1 sampler. Install it manually: `"$python`" -m pip install psutil" }
  Write-Host "psutil installed."
}
$sampler = Join-Path $PSScriptRoot "sampler.py"
$report = Join-Path $PSScriptRoot "report.py"
$HierarchyControls = @("TableViewKeyBy", "TableViewCombined", "TableViewChildren3", "TableViewChildren4")
# Flat baselines on the same data/scenarios: TableViewFlat/ListViewFlat/ItemsViewFlat run from the
# KeyBy HierarchyBench build (mode picked by control name); WinUITableViewFlat is the OSS
# WinUI.TableView bench (C:\perf\ossbench) built to C:\perf\hikbench\WinUITableView.
$FlatBaselineControls = @("TableViewFlat", "ListViewFlat", "ItemsViewFlat", "WinUITableViewFlat")

function Resolve-BenchExe {
  param([string]$Control)

  if ($Control -in $FlatBaselineControls) {
    $root = if ($env:TVPERF_HIERARCHY_APPS) { $env:TVPERF_HIERARCHY_APPS } else { "C:\perf\hikbench" }
    $exe = if ($Control -eq "WinUITableViewFlat") { Join-Path $root "WinUITableView\WinUITableViewBench.exe" } else { Join-Path $root "TableViewKeyBy\HierarchyBench.exe" }
    if (Test-Path $exe) { return (Resolve-Path $exe).Path }
    throw "Flat baseline bench for '$Control' not found: $exe"
  }

  # Hierarchy variant benches (Samples\HierarchyBench in microsoft-ui-xaml, one build per WinUI
  # package). Base dir: TVPERF_HIERARCHY_APPS, default C:\perf\hikbench\<control>\HierarchyBench.exe.
  if ($Control -in $HierarchyControls) {
    $root = if ($env:TVPERF_HIERARCHY_APPS) { $env:TVPERF_HIERARCHY_APPS } else { "C:\perf\hikbench" }
    $exe = Join-Path $root "$Control\HierarchyBench.exe"
    if (Test-Path $exe) { return (Resolve-Path $exe).Path }
    throw "Hierarchy bench for '$Control' not found: $exe"
  }

  # Prefer the OneDrive-synced common folder so any machine sharing this OneDrive resolves the apps.
  # Override the base dir with the TVPERF_BENCH_APPS env var.
  $odRoot = if ($env:TVPERF_BENCH_APPS) { $env:TVPERF_BENCH_APPS }
            elseif ($env:OneDrive) { Join-Path $env:OneDrive "Documents\PerfBenchApps" }
            else { $null }
  if ($odRoot) {
    $odRel = switch ($Control) {
      { $_ -in @("TableView", "ListView", "ItemsView") } { "WinUIBench\TableViewBench.exe"; break }
      "TableViewV1" { "TableViewV1Bench\TableViewV1Bench.exe"; break }
      "WinUITableView" { "OssTableViewBench\WinUITableViewBench.exe"; break }
      { $_ -in @("DataGrid", "ListBox") } { "WpfBench\TvPerfBench.exe"; break }
      default { $null }
    }
    if ($odRel) {
      $odExe = Join-Path $odRoot $odRel
      if (Test-Path $odExe) { return (Resolve-Path $odExe).Path }
    }
  }

  $preferred = switch ($Control) {
    { $_ -in @("TableView", "ListView", "ItemsView") } { "C:\Users\hik\tv-perf-winui\TableViewBench\bin\x64\Release\net8.0-windows10.0.19041.0\win-x64\TableViewBench.exe"; break }
    "TableViewV1" { "C:\Users\hik\tv-perf-winui\TableViewV1Bench\bin\x64\Release\net8.0-windows10.0.19041.0\win-x64\TableViewV1Bench.exe"; break }
    "WinUITableView" { "C:\Users\hik\tv-perf-wahmad\WinUITableViewBench\bin\x64\Release\net8.0-windows10.0.19041.0\win-x64\WinUITableViewBench.exe"; break }
    { $_ -in @("DataGrid", "ListBox") } { Join-Path $repoRoot "app\bin\Release\net8.0-windows\TvPerfBench.exe"; break }
    default { throw "Unknown control '$Control'. Valid controls: DataGrid, ListBox, TableView, TableViewV1, ListView, ItemsView, WinUITableView" }
  }

  if (Test-Path $preferred) { return (Resolve-Path $preferred).Path }

  $searchRoot = switch ($Control) {
    { $_ -in @("TableView", "ListView", "ItemsView") } { "C:\Users\hik\tv-perf-winui"; break }
    "TableViewV1" { "C:\Users\hik\tv-perf-winui\TableViewV1Bench"; break }
    "WinUITableView" { "C:\Users\hik\tv-perf-wahmad"; break }
    default { Join-Path $repoRoot "app\bin" }
  }
  $candidate = Get-ChildItem -Path $searchRoot -Filter "*Bench.exe" -Recurse -ErrorAction SilentlyContinue |
    Sort-Object FullName | Select-Object -First 1
  if ($candidate) { return $candidate.FullName }

  throw "Benchmark exe for control '$Control' not found. Expected '$preferred' or a *Bench.exe under '$searchRoot'."
}

$tierPresets = @{
  tiny    = @{ Rows = 100;    Cols = 10 }
  medium  = @{ Rows = 1000;   Cols = 20 }
  large   = @{ Rows = 10000;  Cols = 50 }
  xlarge  = @{ Rows = 50000;  Cols = 30 }
  wide    = @{ Rows = 1000;   Cols = 100 }
  extreme = @{ Rows = 100000; Cols = 100 }
  h10k    = @{ Rows = 10000;  Cols = 10 }
  h50k    = @{ Rows = 50000;  Cols = 10 }
}

function Convert-Tier {
  param([object]$Tier)

  if ($Tier -is [string]) {
    $name = $Tier.Trim()
    if (-not $tierPresets.ContainsKey($name)) {
      throw "Unknown tier '$name'. Valid tiers: $((($tierPresets.Keys | Sort-Object) -join ', '))"
    }
    $preset = $tierPresets[$name]
    return [pscustomobject]@{ Name = $name; Rows = [int]$preset.Rows; Cols = [int]$preset.Cols }
  }

  if ($Tier -is [hashtable] -or $Tier.PSObject.Properties["Rows"]) {
    $rows = [int]$Tier.Rows
    $cols = [int]$Tier.Cols
    return [pscustomobject]@{ Name = "${rows}x${cols}"; Rows = $rows; Cols = $cols }
  }

  throw "Invalid tier '$Tier'. Use a preset name or @{Rows=<n>;Cols=<n>}."
}

if ($Runs -lt 1) { throw "-Runs must be >= 1" }
if (-not (Test-Path $sampler)) { throw "sampler.py not found: $sampler" }

$resolvedTiers = @($Tiers | ForEach-Object { Convert-Tier $_ })
$resolvedControls = @{}
foreach ($control in $Controls) { $resolvedControls[$control] = Resolve-BenchExe $control }

# Expand each control into one or more execution variants. Only TableView supports the
# column-virtualization A/B (controlled by the TVPERF_COLVIRT env var, read once by
# SharedHelpers::IsTableViewColumnVirtualizationEnabled at first call inside the bench).
# Non-TableView controls always get a single variant matching today's behavior.
function Get-Variants {
  param([string]$Control)
  if ($Control -ne "TableView" -or $ColVirt -eq "off") {
    return , @([pscustomobject]@{ Label = $Control; ColVirt = $null })
  }
  if ($ColVirt -eq "on") {
    return , @([pscustomobject]@{ Label = "TableView (ColVirt ON)"; ColVirt = "1" })
  }
  return , @(
    [pscustomobject]@{ Label = "TableView (ColVirt OFF)"; ColVirt = "0" }
    [pscustomobject]@{ Label = "TableView (ColVirt ON)";  ColVirt = "1" }
  )
}
$controlVariants = @{}
foreach ($control in $Controls) { $controlVariants[$control] = Get-Variants $control }

$outDir = Split-Path -Parent $Out
if ($outDir -and -not (Test-Path $outDir) -and $PSCmdlet.ShouldProcess($outDir, "Create output directory")) {
  New-Item -ItemType Directory -Path $outDir | Out-Null
}
$reportDir = Split-Path -Parent $ReportOut
if ($reportDir -and -not (Test-Path $reportDir) -and $PSCmdlet.ShouldProcess($reportDir, "Create report directory")) {
  New-Item -ItemType Directory -Path $reportDir | Out-Null
}

if ((Test-Path $Out) -and $PSCmdlet.ShouldProcess($Out, "Remove previous output CSV")) {
  Remove-Item $Out -Force
}

$variantTotal = 0
foreach ($control in $Controls) { $variantTotal += $controlVariants[$control].Count }
$total = $variantTotal * $Scenarios.Count * $resolvedTiers.Count * $Runs
$i = 0
$sw = [System.Diagnostics.Stopwatch]::StartNew()
Write-Host "=== Perf run-all: $total cells ===" -ForegroundColor Cyan
Write-Host "Controls: $($Controls -join ', ')"
Write-Host "Scenarios: $($Scenarios -join ', ')"
$tierSummary = ($resolvedTiers | ForEach-Object { "$($_.Name)=$($_.Rows)x$($_.Cols)" }) -join ', '
Write-Host "Tiers: $tierSummary"
Write-Host "Control version: $ControlVersion"
Write-Host "ColVirt: $ColVirt"
foreach ($pair in $resolvedControls.GetEnumerator() | Sort-Object Name) {
  Write-Host "Exe[$($pair.Key)] = $($pair.Value)"
}

# Temp CSV used per cell when we need to relabel the row's control field (i.e. when
# producing ColVirt OFF/ON variants of TableView). For untouched controls we still
# write straight to $Out to preserve today's behavior and avoid the extra IO.
$relabelTmp = Join-Path ([System.IO.Path]::GetTempPath()) ("runall_cell_" + [guid]::NewGuid().ToString("N") + ".csv")

foreach ($control in $Controls) {
  $exe = $resolvedControls[$control]
  foreach ($variant in $controlVariants[$control]) {
    $relabel = ($variant.Label -ne $control)
    foreach ($scenario in $Scenarios) {
      foreach ($tier in $resolvedTiers) {
        for ($run = 1; $run -le $Runs; $run++) {
          $i++
          Write-Host ("[{0,3}/{1}] {2,-26} {3,-7} tier={4,-7} rows={5,-7} cols={6,-4} run={7}" -f $i, $total, $variant.Label, $scenario, $tier.Name, $tier.Rows, $tier.Cols, $run)
          $cellOut = if ($relabel) { $relabelTmp } else { $Out }
          if ($relabel -and (Test-Path $cellOut)) { Remove-Item $cellOut -Force }
          $argsList = @($sampler, "--exe", $exe, "--control", $control, "--scenario", $scenario,
            "--rows", $tier.Rows, "--cols", $tier.Cols, "--run", $run, "--out", $cellOut, "--settle-ms", $SettleMs)
          if ($Gpu) { $argsList += "--gpu" }
          if ($control -in $HierarchyControls -or $control -in $FlatBaselineControls) { $argsList += @("--shape", $Shape) }
          if ($PSCmdlet.ShouldProcess("$($variant.Label)/$scenario/$($tier.Name)/run$run", "Launch sampler")) {
            if ($variant.ColVirt) { $env:TVPERF_COLVIRT = $variant.ColVirt } else { Remove-Item Env:\TVPERF_COLVIRT -ErrorAction SilentlyContinue }
            try {
              $cellSw = [System.Diagnostics.Stopwatch]::StartNew()
              $line = & $python @argsList 2>&1
              $cellSw.Stop()
              Write-Host ("  -> " + ($line -join ' '))
              # Machine-parseable per-cell wall-clock for config-based time estimates (server folds into run-history.json).
              Write-Host ("CELLTIME|scenario={0}|rows={1}|cols={2}|sec={3}" -f $scenario, $tier.Rows, $tier.Cols, [math]::Round($cellSw.Elapsed.TotalSeconds, 2))
            } finally {
              Remove-Item Env:\TVPERF_COLVIRT -ErrorAction SilentlyContinue
            }
            if ($relabel -and (Test-Path $cellOut)) {
              $rows = Import-Csv $cellOut
              foreach ($r in $rows) { $r.control = $variant.Label }
              if (Test-Path $Out) {
                $rows | ConvertTo-Csv -NoTypeInformation | Select-Object -Skip 1 | Add-Content $Out
              } else {
                $rows | Export-Csv $Out -NoTypeInformation
              }
            }
          }
        }
      }
    }
  }
}
if (Test-Path $relabelTmp) { Remove-Item $relabelTmp -Force }

$sw.Stop()
Write-Host "=== planned/captured $total cells in $([math]::Round($sw.Elapsed.TotalSeconds, 1))s -> $Out ===" -ForegroundColor Green

if (-not $NoReport -and (Test-Path $report)) {
  $envScript = Join-Path $PSScriptRoot "write-env.ps1"
  if ((Test-Path $envScript) -and $PSCmdlet.ShouldProcess("env.json/footprint.json", "Write environment + footprint sidecars")) {
    & $envScript -OutDir (Split-Path -Parent $Out) -ControlVersion $ControlVersion
  }
  if ($PSCmdlet.ShouldProcess($ReportOut, "Render HTML report")) {
    $title = if (@($Controls | Where-Object { $_ -in $HierarchyControls }).Count -gt 0) { "TableView Hierarchy Variants - shape=$Shape" } else { "TableView Perf Framework - Multi-Benchmark Report" }
    & $python $report --in $Out --out $ReportOut --title $title
    Write-Host "report -> $ReportOut" -ForegroundColor Green
  }
}
