<#
.SYNOPSIS
    Regenerates a work-item durations file for duration-balanced slicing
    (RunTestPassSliceOnBuildAgent.ps1 -WorkItemDurationsFile).

.DESCRIPTION
    Each RunTests<OS> N agent job prints "Running <work item> work item" before every Helix work
    item, and Azure DevOps timestamps every log line. A work item's duration is the gap to the next
    item's line (the last item ends when the "Run Tests" step finishes), so it includes app
    deployment, TAEF start-up and in-item reruns. The median across the given builds is written.

    Requires `az login` with access to the WinUI Azure DevOps project.

.EXAMPLE
    .\Update-WorkItemDurations.ps1 -BuildIds 163791948,163141816,162768477 -OsJobToken 23H2 `
        -OutFile .\WorkItemDurations\DevTestSuite-Win11-23H2.json
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [long[]]$BuildIds,

    [string]$OsJobToken = '23H2', # job names look like 'RunTests23H2 7'

    [Parameter(Mandatory = $true)]
    [string]$OutFile,

    [string]$Organization = 'microsoft',
    [string]$Project = 'WinUI'
)

$ErrorActionPreference = 'Stop'

$token = az account get-access-token --resource 499b84ac-1321-427f-aa17-267ca6975798 --query accessToken -o tsv 2>$null
if (-not $token) { throw 'Could not get an Azure DevOps token. Run `az login` first.' }
$headers = @{ Authorization = "Bearer $token" }
$api = "https://dev.azure.com/$Organization/$Project/_apis"

# PowerShell 7 returns REST timestamps as DateTime (Kind=Utc); Windows PowerShell 5.1 returns strings.
function ConvertTo-UtcDateTime($value) {
    if ($value -is [datetime]) {
        if ($value.Kind -eq [DateTimeKind]::Unspecified) { return [datetime]::SpecifyKind($value, [DateTimeKind]::Utc) }
        return $value.ToUniversalTime()
    }
    return [datetimeoffset]::Parse([string]$value, [cultureinfo]::InvariantCulture).UtcDateTime
}

$samples = @{}
$usedBuilds = @()
foreach ($buildId in $BuildIds) {
    $timeline = (Invoke-RestMethod -Headers $headers -Uri "$api/build/builds/$buildId/timeline?api-version=7.1").records
    $jobs = @($timeline | Where-Object { $_.type -eq 'Job' -and $_.name -match "^RunTests$OsJobToken \d+$" -and $_.result -eq 'succeeded' })
    if (-not $jobs.Count) { Write-Warning "Build $($buildId): no succeeded 'RunTests$OsJobToken N' jobs; skipped."; continue }
    $usedBuilds += $buildId
    foreach ($job in $jobs) {
        $step = $timeline | Where-Object { $_.parentId -eq $job.id -and $_.type -eq 'Task' -and $_.name -eq 'Run Tests' } | Select-Object -First 1
        if (-not $step -or -not $step.log) { continue }
        $starts = @(foreach ($line in ((Invoke-RestMethod -Headers $headers -Uri $step.log.url) -split "`r?`n")) {
                if ($line -match '^(\S+Z) Running (.+) work item\s*$') {
                    [pscustomobject]@{ Time = ConvertTo-UtcDateTime $Matches[1]; Name = $Matches[2] }
                }
            })
        $stepEnd = ConvertTo-UtcDateTime $step.finishTime
        for ($i = 0; $i -lt $starts.Count; $i++) {
            $end = if ($i + 1 -lt $starts.Count) { $starts[$i + 1].Time } else { $stepEnd }
            $seconds = ($end - $starts[$i].Time).TotalSeconds
            if ($seconds -lt 0) { throw "Negative duration for $($starts[$i].Name) in build $buildId ($seconds s); check timestamp parsing." }
            if (-not $samples.ContainsKey($starts[$i].Name)) { $samples[$starts[$i].Name] = [System.Collections.Generic.List[object]]::new() }
            # The first work item on an agent also pays one-time machine setup (test prerun, cert/app install).
            $samples[$starts[$i].Name].Add([pscustomobject]@{ Seconds = $seconds; FirstOnAgent = ($i -eq 0) })
        }
    }
    Write-Host "Build $($buildId): $($jobs.Count) slices read"
}
if (-not $samples.Count) { throw 'No work-item durations found.' }

$workItems = [ordered]@{}
foreach ($name in ($samples.Keys | Sort-Object)) {
    # Prefer samples where the item was not first on its agent, so per-agent setup is not charged to it.
    $notFirst = @($samples[$name] | Where-Object { -not $_.FirstOnAgent })
    $v = @($(if ($notFirst.Count) { $notFirst } else { $samples[$name] }) | ForEach-Object { $_.Seconds } | Sort-Object)
    $median = if ($v.Count % 2) { $v[[int][math]::Floor($v.Count / 2)] } else { ($v[$v.Count / 2 - 1] + $v[$v.Count / 2]) / 2 }
    $workItems[$name] = [math]::Round($median, 1)
}

$doc = [ordered]@{
    description = "Median duration of each DevTestSuite Helix work item on the 'RunTests$OsJobToken' agent jobs, excluding runs where the item was first on its agent (one-time machine setup) when other samples exist. Used by RunTestPassSliceOnBuildAgent.ps1 -WorkItemDurationsFile. Regenerate with Helix/common/pipeline/Update-WorkItemDurations.ps1."
    unit        = 'seconds'
    sourceBuilds = $usedBuilds
    generatedUtc = (Get-Date).ToUniversalTime().ToString('yyyy-MM-ddTHH:mm:ssZ')
    workItems   = $workItems
}
New-Item -ItemType Directory -Force -Path (Split-Path $OutFile -Parent) | Out-Null
$doc | ConvertTo-Json -Depth 4 | Set-Content -Path $OutFile -Encoding utf8
Write-Host "Wrote $OutFile ($($workItems.Count) work items, total $([math]::Round((($workItems.Values | Measure-Object -Sum).Sum) / 60, 1)) minutes)"
