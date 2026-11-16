<#
.SYNOPSIS
    Assigns Helix work items to pipeline slices so that the slowest slice finishes as early as
    possible (longest-processing-time-first greedy scheduling).

.DESCRIPTION
    RunTestPassSliceOnBuildAgent.ps1 runs once per agent job (slice). Every slice calls this
    script with the same work-item list and the same durations file, so every slice computes the
    same plan and then runs only its own items - no coordination between agents is needed.

    Algorithm (deterministic):
      1. Cost of each work item = its measured duration from the durations file, or the median of
         the known durations when the item is not in the file (new/renamed work items).
      2. Sort by cost descending, then by name (ordinal) to break ties.
      3. Give each item to the slice with the smallest planned total so far (lowest slice number
         on ties).

    Durations file format (JSON):
      { "unit": "seconds", "workItems": { "<HelixWorkItem Include>": <seconds>, ... } }

.OUTPUTS
    One object per work item: WorkItem (the input object), Name, Slice (1-based), Seconds,
    Estimated (true when the median default was used).
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [object[]]$WorkItems, # Objects with an 'Include' property (HelixWorkItem XML elements)

    [Parameter(Mandatory = $true)]
    [int]$TotalSlices,

    [Parameter(Mandatory = $true)]
    [string]$DurationsFile
)

$ErrorActionPreference = 'Stop'

if ($TotalSlices -lt 1) { throw "TotalSlices must be at least 1 (got $TotalSlices)." }

$durations = @{}
$json = Get-Content -Path $DurationsFile -Raw | ConvertFrom-Json
foreach ($p in $json.workItems.PSObject.Properties) { $durations[$p.Name] = [double]$p.Value }
if ($durations.Count -eq 0) { throw "Durations file '$DurationsFile' has no workItems." }

$known = @($durations.Values | Sort-Object)
$defaultSeconds = if ($known.Count % 2) { $known[[int][math]::Floor($known.Count / 2)] } else { ($known[$known.Count / 2 - 1] + $known[$known.Count / 2]) / 2 }

$items = [System.Collections.Generic.List[object]]::new()
foreach ($wi in $WorkItems) {
    $name = [string]$wi.Include
    $isKnown = $durations.ContainsKey($name)
    $items.Add([pscustomobject]@{
            WorkItem  = $wi
            Name      = $name
            Slice     = 0
            Seconds   = $(if ($isKnown) { $durations[$name] } else { $defaultSeconds })
            Estimated = -not $isKnown
        })
}

# Ordinal tie-break so every agent sorts identically regardless of culture settings.
$items.Sort([System.Comparison[object]] {
        param($a, $b)
        $c = $b.Seconds.CompareTo($a.Seconds)
        if ($c -ne 0) { return $c }
        return [string]::CompareOrdinal($a.Name, $b.Name)
    })

$loads = New-Object double[] $TotalSlices
foreach ($item in $items) {
    $best = 0
    for ($s = 1; $s -lt $TotalSlices; $s++) {
        if ($loads[$s] -lt $loads[$best]) { $best = $s }
    }
    $loads[$best] += $item.Seconds
    $item.Slice = $best + 1
}

$items
