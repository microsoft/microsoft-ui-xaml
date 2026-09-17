# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
#
# PostTestRun aggregation for the lifetime stress suite.
#
# Every lifetime-stress work item writes its own per-work-item LifetimeNativeCrashReport.json (see
# Helix/common/test/RunHelixWorkItem.ps1 -> Report-LifetimeNativeCrash), recording how many native host crashes
# and native scenario warnings (thrown-exception / COMException reports) it produced. This script runs AFTER the
# test run and totals those records across every work item that landed on this shard/slice, so the pipeline shows
# a single "total native crash/warning" number instead of the reader having to scan every shard's console log by
# hand.
#
# The LifetimeStressTestSuite runs as a single isolated work item, so on the shard that ran it this total is the
# suite's native crash/warning total for the OS leg; shards that ran no lifetime work items simply total zero.
# The step is intentionally fail-open and NON-GATING: it never fails the stage. It emits a non-gating warning and
# sets the LifetimeNativeCrashTotal pipeline variable, and writes an aggregate LifetimeNativeCrashSummary.json.

[CmdletBinding()]
param (
    [Parameter(Mandatory = $true)]
    [string]$SearchRoot,        # Root scanned recursively for per-work-item LifetimeNativeCrashReport.json files.
    [string]$SummaryOutputPath  # Optional path for the aggregate summary; defaults to $SearchRoot\LifetimeNativeCrashSummary.json.
)

$ErrorActionPreference = 'Continue'

if (-not (Test-Path $SearchRoot))
{
    Write-Host "Lifetime stress PostTestRun: search root '$SearchRoot' does not exist; nothing to total."
    Write-Host "##vso[task.setvariable variable=LifetimeNativeCrashTotal]0"
    return
}

$reportFiles = @(Get-ChildItem -Path $SearchRoot -Filter 'LifetimeNativeCrashReport.json' -Recurse -ErrorAction SilentlyContinue)

# The LifetimeStressTestSuite runs only in the checked (chk) build flavor, because lifetime/TrackerHandle leak
# detection needs the reference-tracker instrumentation that free (fre) builds do not carry. This step, however,
# runs in every test-pass job (one per testOS x buildFlavor), so most jobs scan a $SearchRoot that never held a
# lifetime work item. Writing a workItemCount=0 summary in those jobs produced a scatter of empty
# LifetimeNativeCrashSummary.json files that look like the aggregation is broken. When no per-work-item report is
# present, total zero, set the variable, and return WITHOUT writing an empty summary so the only summary that ever
# lands in the artifacts is the populated one from the job that actually ran the suite.
if ($reportFiles.Count -eq 0)
{
    Write-Host "Lifetime stress PostTestRun: no LifetimeNativeCrashReport.json under '$SearchRoot' (no lifetime work items ran in this configuration - the suite runs only in the chk flavor). Nothing to total; not writing an empty summary."
    Write-Host "##vso[task.setvariable variable=LifetimeNativeCrashTotal]0"
    return
}

$totalCrashes  = 0
$totalWarnings = 0
$perWorkItem   = New-Object System.Collections.Generic.List[object]

foreach ($file in $reportFiles)
{
    try
    {
        $r = Get-Content $file.FullName -Raw | ConvertFrom-Json
    }
    catch
    {
        Write-Host "Lifetime stress PostTestRun: could not parse '$($file.FullName)' ($($_.Exception.Message)); skipping."
        continue
    }

    $c = [int]$r.nativeCrashCount
    $w = [int]$r.nativeWarningCount
    $totalCrashes  += $c
    $totalWarnings += $w
    $perWorkItem.Add([ordered]@{
        workItem           = $r.workItem
        nativeCrashCount   = $c
        nativeWarningCount = $w
        crashScenario      = $r.crashScenario
        dumps              = $r.dumps
        warningScenarios   = $r.warningScenarios
    })
}

$totalSignals = $totalCrashes + $totalWarnings

Write-Host "============================================================"
Write-Host " Lifetime stress - native crash/warning totals (this shard)"
Write-Host "============================================================"
Write-Host " Work items with a report : $($reportFiles.Count)"
Write-Host " Native host crashes      : $totalCrashes"
Write-Host " Native scenario warnings : $totalWarnings"
Write-Host " Total native signals     : $totalSignals"
foreach ($wi in $perWorkItem)
{
    $crashNote = if ($wi.crashScenario -and $wi.crashScenario -ne 'none') { " (host crash in '$($wi.crashScenario)')" } else { "" }
    Write-Host ("   - {0}: crashes={1}, warnings={2}{3}" -f $wi.workItem, $wi.nativeCrashCount, $wi.nativeWarningCount, $crashNote)
    if ($wi.warningScenarios -and @($wi.warningScenarios).Count -gt 0)
    {
        Write-Host ("       warning scenarios: {0}" -f (@($wi.warningScenarios) -join ", "))
    }
}
Write-Host "============================================================"

# Non-gating pipeline warning so the total is visible in the build summary without failing the stage.
if ($totalSignals -gt 0)
{
    Write-Host "##vso[task.logissue type=warning]Lifetime stress: $totalSignals native crash/warning signal(s) on this shard (crashes=$totalCrashes, warnings=$totalWarnings)."
}

# Publish the total as a pipeline variable so a downstream step/job can consume or roll it up across shards.
Write-Host "##vso[task.setvariable variable=LifetimeNativeCrashTotal]$totalSignals"

$summary = [ordered]@{
    generatedUtc        = (Get-Date).ToUniversalTime().ToString('o')
    searchRoot          = $SearchRoot
    workItemCount       = $reportFiles.Count
    totalNativeCrashes  = $totalCrashes
    totalNativeWarnings = $totalWarnings
    totalNativeSignals  = $totalSignals
    workItems           = $perWorkItem
}

if (-not $SummaryOutputPath) { $SummaryOutputPath = Join-Path $SearchRoot 'LifetimeNativeCrashSummary.json' }
try
{
    $summary | ConvertTo-Json -Depth 6 | Out-File -FilePath $SummaryOutputPath -Encoding utf8
    Write-Host "Lifetime stress PostTestRun: wrote aggregate summary to $SummaryOutputPath."
}
catch
{
    Write-Host "Lifetime stress PostTestRun: failed to write summary ($($_.Exception.Message))."
}
