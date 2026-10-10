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
    <#
        Streams the decoded trace rather than loading it into a document.

        A few seconds of this provider decodes to tens of megabytes: a real capture of a
        Calculator launch produced 40 MB, which took 64 seconds to walk as a document and
        about two to stream. Only a handful of events in it matter, so there is no reason
        to hold the rest in memory at all.
    #>
    param([Parameter(Mandatory)][string] $Xml)

    $events = [System.Collections.Generic.List[object]]::new()

    $settings = [System.Xml.XmlReaderSettings]::new()
    $settings.IgnoreComments = $true
    $settings.IgnoreWhitespace = $true
    $settings.DtdProcessing = [System.Xml.DtdProcessing]::Ignore
    $settings.ConformanceLevel = [System.Xml.ConformanceLevel]::Fragment

    $stringReader = [System.IO.StringReader]::new($Xml)
    $reader = [System.Xml.XmlReader]::Create($stringReader, $settings)
    try {
        $isXaml = $false
        $eventId = $null
        $expectingEventId = $false
        $processId = $null
        $threadId = $null
        $time = $null

        while ($reader.Read()) {
            if ($reader.NodeType -eq [System.Xml.XmlNodeType]::Element) {
                switch ($reader.LocalName) {
                    'System' {
                        $isXaml = $false; $eventId = $null; $processId = $null; $threadId = $null; $time = $null
                    }
                    'Provider' {
                        # Event ids are only unique within a provider, so another provider's
                        # event 17 would otherwise be read as a XAML startup.
                        $isXaml = ([string]$reader.GetAttribute('Guid') -ieq $script:XamlProviderGuid)
                    }
                    'EventID' {
                        # The value arrives as the following text node. Reading the element
                        # content here would move the reader past elements this loop still
                        # needs to see, which is how a timestamp goes missing.
                        $expectingEventId = -not $reader.IsEmptyElement
                    }
                    'TimeCreated' { $time = [string]$reader.GetAttribute('SystemTime') }
                    'Execution' {
                        $processId = [string]$reader.GetAttribute('ProcessID')
                        $threadId = [string]$reader.GetAttribute('ThreadID')
                    }
                }
                continue
            }

            if ($expectingEventId -and
                ($reader.NodeType -eq [System.Xml.XmlNodeType]::Text -or
                 $reader.NodeType -eq [System.Xml.XmlNodeType]::CDATA)) {
                $parsed = 0
                if ([int]::TryParse($reader.Value.Trim(), [ref]$parsed)) { $eventId = $parsed }
                $expectingEventId = $false
                continue
            }

            if ($reader.NodeType -eq [System.Xml.XmlNodeType]::EndElement -and $reader.LocalName -eq 'System') {
                $expectingEventId = $false
                if (-not $isXaml) { continue }
                if ($null -eq $eventId -or $null -eq $time -or $null -eq $processId) { continue }
                # Only the events these regions are built from are worth keeping.
                if ($eventId -ne $script:RegionStartEventId -and
                    @($script:RegionStopEventIds.EventId) -notcontains $eventId) { continue }

                $events.Add([pscustomobject]@{
                    EventId = $eventId
                    ProcessId = $processId
                    ThreadId = $threadId
                    Time = ConvertTo-PRPerfTraceTimestamp -SystemTime $time
                })
            }
        }
    } finally {
        $reader.Dispose()
        $stringReader.Dispose()
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
