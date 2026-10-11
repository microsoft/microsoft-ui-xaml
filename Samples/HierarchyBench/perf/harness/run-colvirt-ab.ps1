<#
.SYNOPSIS
  Column-virtualization A/B benchmark: runs the TableView bench with column virtualization
  OFF and ON and produces a side-by-side comparison report.

.DESCRIPTION
  Drives two MATCHED self-contained bench apps (built from the same source, differing only in the
  IsTableViewColumnVirtualizationEnabled compile-time flag):
    - OFF app: column virtualization disabled (v1 horizontal StackPanel cell host)
    - ON  app: column virtualization enabled (TwoAxisLayout)
  For each variant it runs the L1 sampler (CPU/working-set) + L2 in-app metrics
  (realized rows/cols/cells, FPS, render-inclusive sort/filter/select latency) across the
  column-heavy tiers where virtualization matters most, relabels the control to expose the
  variant, merges to one CSV, and renders an HTML report.

  The two app folders are resolved from OneDrive by default so this runs unchanged on any
  synced machine. Override with -OffExe / -OnExe or the TVPERF_COLVIRT_OFF / _ON env vars.

.EXAMPLE
  .\run-colvirt-ab.ps1
  .\run-colvirt-ab.ps1 -Tiers wide,xlarge -Runs 5
#>
param(
  [string]$OffExe,
  [string]$OnExe,
  [string[]]$Scenarios = @("load", "scroll", "sort", "filter", "select"),
  [object[]]$Tiers = @("wide", "xlarge"),
  [int]$Runs = 5,
  [string]$Out,
  [int]$SettleMs = 1500
)

$ErrorActionPreference = "Stop"
$here = $PSScriptRoot
$python = (Get-Command python -ErrorAction Stop).Source
$sampler = Join-Path $here "sampler.py"
$report = Join-Path $here "report.py"
$resultsDir = Resolve-Path (Join-Path $here "..\results") -ErrorAction SilentlyContinue
if (-not $resultsDir) { $resultsDir = (New-Item -ItemType Directory -Force -Path (Join-Path $here "..\results")).FullName }
if (-not $Out) { $Out = Join-Path $resultsDir "colvirt-ab.csv" }
$reportOut = [System.IO.Path]::ChangeExtension($Out, ".html")

# Ensure the L1 sampler dependency (portable across machines).
& $python -c "import psutil" 2>$null
if ($LASTEXITCODE -ne 0) { & $python -m pip install --quiet --user psutil 2>&1 | Out-Null }

# --- resolve the two matched bench apps (OneDrive-synced by default) ---
$odApps = if ($env:OneDrive) { Join-Path $env:OneDrive "Documents\PerfBenchApps" } else { $null }
function ResolveExe { param([string]$Given, [string]$EnvVar, [string]$OdFolder)
  if ($Given) { return $Given }
  if ($EnvVar -and (Test-Path $EnvVar)) { return $EnvVar }
  if ($odApps) { $c = Join-Path $odApps "$OdFolder\TableViewBench.exe"; if (Test-Path $c) { return $c } }
  return $null
}
if (-not $OffExe) { $OffExe = ResolveExe $OffExe $env:TVPERF_COLVIRT_OFF "TableViewBench-ColVirtOFF" }
if (-not $OnExe)  { $OnExe  = ResolveExe $OnExe  $env:TVPERF_COLVIRT_ON  "TableViewBench-ColVirtON" }
foreach ($pair in @(@{n = "OFF"; e = $OffExe }, @{n = "ON"; e = $OnExe })) {
  if (-not $pair.e -or -not (Test-Path $pair.e)) {
    throw "Col-virt $($pair.n) bench exe not found. Pass -$($pair.n)Exe or place it at %OneDrive%\Documents\PerfBenchApps\TableViewBench-ColVirt$($pair.n)\TableViewBench.exe"
  }
}

$tierPresets = @{
  tiny = @{ Rows = 100; Cols = 10 }; medium = @{ Rows = 1000; Cols = 20 }; large = @{ Rows = 10000; Cols = 50 }
  xlarge = @{ Rows = 50000; Cols = 30 }; wide = @{ Rows = 1000; Cols = 100 }; extreme = @{ Rows = 100000; Cols = 100 }
}
$resolvedTiers = foreach ($t in $Tiers) {
  if ($t -is [hashtable]) { @{ Name = "$($t.Rows)x$($t.Cols)"; Rows = $t.Rows; Cols = $t.Cols } }
  elseif ($tierPresets.ContainsKey([string]$t)) { @{ Name = [string]$t; Rows = $tierPresets[$t].Rows; Cols = $tierPresets[$t].Cols } }
  else { throw "Unknown tier '$t'. Valid: $($tierPresets.Keys -join ', ')" }
}

$variants = @(@{ Tag = "TableView ColVirt OFF"; Exe = $OffExe }, @{ Tag = "TableView ColVirt ON"; Exe = $OnExe })
$total = $variants.Count * $Scenarios.Count * $resolvedTiers.Count * $Runs
Write-Host "=== Col-virt A/B: $total cells ($($variants.Count) variants x $($Scenarios.Count) scenarios x $($resolvedTiers.Count) tiers x $Runs runs) -> $Out ==="

if (Test-Path $Out) { Remove-Item $Out -Force }
$tmp = Join-Path ([System.IO.Path]::GetTempPath()) ("colvirt_cell_" + [guid]::NewGuid().ToString("N") + ".csv")
$i = 0
foreach ($v in $variants) {
  foreach ($scenario in $Scenarios) {
    foreach ($tier in $resolvedTiers) {
      for ($run = 1; $run -le $Runs; $run++) {
        $i++
        Write-Host ("[{0,4}/{1}] {2,-22} {3,-7} {4,-6} rows={5,-7} cols={6,-4} run={7}" -f $i, $total, $v.Tag, $scenario, $tier.Name, $tier.Rows, $tier.Cols, $run)
        if (Test-Path $tmp) { Remove-Item $tmp -Force }
        $argsList = @($sampler, "--exe", $v.Exe, "--control", "TableView", "--scenario", $scenario,
          "--rows", $tier.Rows, "--cols", $tier.Cols, "--run", $run, "--out", $tmp, "--settle-ms", $SettleMs)
        & $python @argsList 2>&1 | ForEach-Object { Write-Host "  -> $_" }
        if (Test-Path $tmp) {
          # relabel the control column to the variant tag, then append (header once)
          $rows = Import-Csv $tmp
          foreach ($r in $rows) { $r.control = $v.Tag }
          if (Test-Path $Out) { $rows | ConvertTo-Csv -NoTypeInformation | Select-Object -Skip 1 | Add-Content $Out }
          else { $rows | Export-Csv $Out -NoTypeInformation }
        }
      }
    }
  }
}
if (Test-Path $tmp) { Remove-Item $tmp -Force }

Write-Host "`nRendering report -> $reportOut"
$env:PYTHONIOENCODING = "utf-8"
& $python $report --in $Out --out $reportOut --title "TableView - Column Virtualization A/B (OFF vs ON, render-inclusive)"
Write-Host "Done. CSV: $Out  |  Report: $reportOut"
