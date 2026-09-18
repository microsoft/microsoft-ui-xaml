# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License. See LICENSE in the project root for license information.

<#
    Reads XAML startup regions out of a decoded ETW trace.

    The lab image has wpr.exe and tracerpt.exe but neither xperf nor wpaexporter, so the
    only decoder available is tracerpt's XML dump. That rules out the regions XML format
    the OS performance gates use, which wpaexporter alone understands.

    It turns out not to matter. Every XAML region those gates define is a gap between two
    plain events on one thread, all sharing the same start:

        31 -> 17   application startup
        31 -> 42   first layout
        31 -> 65   first frame

    So no region definition file is needed. Any decoder that yields timestamped events with
    a provider, a process and a thread is enough, and tracerpt is in-box on Windows.
#>

$script:XamlProviderGuid = '{531a35ab-63ce-4bcf-aa98-f88c7a89e455}'
$script:RegionStartEventId = 31
$script:RegionStopEventIds = @(
    [pscustomobject]@{ EventId = 17; Metric = 'XamlInitializeMs' }
    [pscustomobject]@{ EventId = 42; Metric = 'XamlLayoutMs' }
    [pscustomobject]@{ EventId = 65; Metric = 'XamlFrameMs' }
)

function ConvertTo-PRPerfTraceTimestamp {
    <#
        tracerpt writes nine fractional digits. .NET parses at most seven, and refuses the
        rest outright. Losing an entire trace to a formatting detail would be absurd, so the
        extra digits are dropped: they are 10 nanoseconds of precision on a measurement whose
        threshold is measured in milliseconds.
    #>
    param([Parameter(Mandatory)][string] $SystemTime)

    $normalized = [regex]::Replace(
        $SystemTime,
        '\.(\d{7})\d+',
        { param($match) '.' + $match.Groups[1].Value })

    return [datetimeoffset]::Parse($normalized, [cultureinfo]::InvariantCulture)
}

function Get-PRPerfTraceEvents {
    param([Parameter(Mandatory)][string] $Xml)

    $document = [xml]$Xml
    $events = @()

    foreach ($node in $document.SelectNodes("//*[local-name()='Event']")) {
        $provider = $node.SelectSingleNode("*[local-name()='System']/*[local-name()='Provider']")
        if ($null -eq $provider) { continue }
        # Event ids are only unique within a provider, so another provider's event 17 would
        # otherwise be read as a XAML startup and produce a confidently wrong number.
        if ([string]$provider.GetAttribute('Guid') -ine $script:XamlProviderGuid) { continue }

        $eventId = $node.SelectSingleNode("*[local-name()='System']/*[local-name()='EventID']")
        $timeCreated = $node.SelectSingleNode("*[local-name()='System']/*[local-name()='TimeCreated']")
        $execution = $node.SelectSingleNode("*[local-name()='System']/*[local-name()='Execution']")
        if ($null -eq $eventId -or $null -eq $timeCreated -or $null -eq $execution) { continue }

        $events += [pscustomobject]@{
            EventId = [int]$eventId.InnerText
            ProcessId = [string]$execution.GetAttribute('ProcessID')
            ThreadId = [string]$execution.GetAttribute('ThreadID')
            Time = ConvertTo-PRPerfTraceTimestamp -SystemTime ([string]$timeCreated.GetAttribute('SystemTime'))
        }
    }

    return $events
}

function Get-PRPerfXamlRegionDurations {
    <#
    .SYNOPSIS
    The XAML startup regions found in a decoded trace, in milliseconds.

    .DESCRIPTION
    A region is only reported when both of its events were genuinely seen on the same
    thread, with the stop after the start. A region that cannot be measured is left out
    entirely rather than reported as zero: a zero would read as an impossibly fast startup
    and be presented as an enormous improvement.
    #>
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)][AllowEmptyString()][string] $Xml,

        # wpr records the whole machine, so the caller names the process it launched.
        # Without it, any other XAML app on the agent could be measured in its place.
        [string] $ProcessId
    )

    $regions = [ordered]@{}
    if ([string]::IsNullOrWhiteSpace($Xml)) { return $regions }

    $events = @(Get-PRPerfTraceEvents -Xml $Xml)
    if (-not [string]::IsNullOrWhiteSpace($ProcessId)) {
        $events = @($events | Where-Object { $_.ProcessId -eq $ProcessId })
    }

    $start = @($events | Where-Object { $_.EventId -eq $script:RegionStartEventId } |
        Sort-Object Time | Select-Object -First 1)
    if ($start.Count -eq 0) { return $regions }
    $start = $start[0]

    foreach ($region in $script:RegionStopEventIds) {
        $stop = @($events | Where-Object {
                $_.EventId -eq $region.EventId -and
                $_.ProcessId -eq $start.ProcessId -and
                $_.ThreadId -eq $start.ThreadId -and
                $_.Time -ge $start.Time
            } | Sort-Object Time | Select-Object -First 1)
        if ($stop.Count -eq 0) { continue }

        $regions[$region.Metric] =
            [math]::Round(($stop[0].Time - $start.Time).TotalMilliseconds, 4)
    }

    return $regions
}

Export-ModuleMember -Function Get-PRPerfXamlRegionDurations, ConvertTo-PRPerfTraceTimestamp
