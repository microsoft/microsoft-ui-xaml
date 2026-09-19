# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
#
# Totals the per-work-item LifetimeNativeCrashReport.json files under a search root: native host crashes, native
# scenario warnings, and managed leak warnings ("object still alive after forced collection" WeakReference probes).
# Non-gating: emits a warning, sets the LifetimeNativeCrashTotal / LifetimeManagedWarningTotal / LifetimeSignalTotal
# variables, and writes LifetimeNativeCrashSummary.json.

[CmdletBinding()]
param (
    [Parameter(Mandatory = $true)]
    [string]$SearchRoot,        # Scanned recursively for LifetimeNativeCrashReport.json files.
    [string]$SummaryOutputPath  # Defaults to $SearchRoot\LifetimeNativeCrashSummary.json.
)

$ErrorActionPreference = 'Continue'

if (-not (Test-Path $SearchRoot))
{
    Write-Host "Lifetime stress PostTestRun: search root '$SearchRoot' does not exist; nothing to total."
    Write-Host "##vso[task.setvariable variable=LifetimeNativeCrashTotal]0"
    return
}

$reportFiles = @(Get-ChildItem -Path $SearchRoot -Filter 'LifetimeNativeCrashReport.json' -Recurse -ErrorAction SilentlyContinue)

# No reports here means no lifetime work item ran in this configuration (the suite is chk-only). Total zero
# and return without writing an empty summary, so only the job that ran the suite leaves one behind.
if ($reportFiles.Count -eq 0)
{
    Write-Host "Lifetime stress PostTestRun: no LifetimeNativeCrashReport.json under '$SearchRoot' (no lifetime work items ran in this configuration - the suite runs only in the chk flavor). Nothing to total; not writing an empty summary."
    Write-Host "##vso[task.setvariable variable=LifetimeNativeCrashTotal]0"
    return
}

$totalCrashes  = 0
$totalWarnings = 0
$totalManagedWarnings = 0
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
    $m = [int]$r.managedWarningCount
    $totalCrashes  += $c
    $totalWarnings += $w
    $totalManagedWarnings += $m
    $perWorkItem.Add([ordered]@{
        workItem              = $r.workItem
        nativeCrashCount      = $c
        nativeWarningCount    = $w
        managedWarningCount   = $m
        crashScenario         = $r.crashScenario
        dumps                 = $r.dumps
        warningScenarios      = $r.warningScenarios
        managedWarningObjects = $r.managedWarningObjects
    })
}

$totalNativeSignals = $totalCrashes + $totalWarnings
$totalSignals = $totalNativeSignals + $totalManagedWarnings

Write-Host "============================================================"
Write-Host " Lifetime stress - crash/warning totals (this shard)"
Write-Host "============================================================"
Write-Host " Work items with a report : $($reportFiles.Count)"
Write-Host " Native host crashes      : $totalCrashes"
Write-Host " Native scenario warnings : $totalWarnings"
Write-Host " Managed leak warnings    : $totalManagedWarnings"
Write-Host " Total native signals     : $totalNativeSignals"
Write-Host " Total signals (all)      : $totalSignals"
foreach ($wi in $perWorkItem)
{
    $crashNote = if ($wi.crashScenario -and $wi.crashScenario -ne 'none') { " (host crash in '$($wi.crashScenario)')" } else { "" }
    Write-Host ("   - {0}: crashes={1}, nativeWarnings={2}, managedWarnings={3}{4}" -f $wi.workItem, $wi.nativeCrashCount, $wi.nativeWarningCount, $wi.managedWarningCount, $crashNote)
    if ($wi.warningScenarios -and @($wi.warningScenarios).Count -gt 0)
    {
        Write-Host ("       native warning scenarios: {0}" -f (@($wi.warningScenarios) -join ", "))
    }
    if ($wi.managedWarningObjects -and @($wi.managedWarningObjects).Count -gt 0)
    {
        Write-Host ("       managed leak warnings   : {0}" -f (@($wi.managedWarningObjects) -join ", "))
    }
}
Write-Host "============================================================"

# Surface the total as a non-gating warning.
if ($totalSignals -gt 0)
{
    Write-Host "##vso[task.logissue type=warning]Lifetime stress: $totalSignals lifetime signal(s) on this shard (native crashes=$totalCrashes, native warnings=$totalWarnings, managed leak warnings=$totalManagedWarnings)."
}

# Publish the totals for downstream steps to consume. LifetimeNativeCrashTotal keeps its original native-only
# meaning for existing consumers; managed and combined totals get their own variables.
Write-Host "##vso[task.setvariable variable=LifetimeNativeCrashTotal]$totalNativeSignals"
Write-Host "##vso[task.setvariable variable=LifetimeManagedWarningTotal]$totalManagedWarnings"
Write-Host "##vso[task.setvariable variable=LifetimeSignalTotal]$totalSignals"

$summary = [ordered]@{
    generatedUtc         = (Get-Date).ToUniversalTime().ToString('o')
    searchRoot           = $SearchRoot
    workItemCount        = $reportFiles.Count
    totalNativeCrashes   = $totalCrashes
    totalNativeWarnings  = $totalWarnings
    totalManagedWarnings = $totalManagedWarnings
    totalNativeSignals   = $totalNativeSignals
    totalSignals         = $totalSignals
    workItems            = $perWorkItem
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
