$root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
Import-Module (Join-Path $root 'PRPerfTrace.psm1') -Force

function Assert-Equal($Expected, $Actual, [string] $Message) {
    if ($Expected -ne $Actual) {
        throw "$Message Expected='$Expected' Actual='$Actual'"
    }
}

function New-TraceEvent {
    param(
        [string] $Provider = '{531a35ab-63ce-4bcf-aa98-f88c7a89e455}',
        [int] $EventId,
        [string] $Time,
        [int] $ProcessId = 100,
        [int] $ThreadId = 200
    )

    @"
<Event xmlns="http://schemas.microsoft.com/win/2004/08/events/event">
  <System>
    <Provider Guid="$Provider" />
    <EventID>$EventId</EventID>
    <TimeCreated SystemTime="$Time" />
    <Execution ProcessID="$ProcessId" ThreadID="$ThreadId" ProcessorID="0" />
  </System>
</Event>
"@
}

function New-TraceDocument {
    param([string[]] $Events)
    return "<Events>`n$($Events -join "`n")`n</Events>"
}

function Test-TraceReportsTheThreeXamlRegionsAsMilliseconds {
    # Every XAML region is the gap between event 31 and one later event on the same thread.
    # 17 is application startup, 42 is first layout, 65 is first frame.
    $xml = New-TraceDocument -Events @(
        (New-TraceEvent -EventId 31 -Time '2026-09-14T08:50:11.0000000+00:00')
        (New-TraceEvent -EventId 17 -Time '2026-09-14T08:50:11.0250000+00:00')
        (New-TraceEvent -EventId 42 -Time '2026-09-14T08:50:11.1000000+00:00')
        (New-TraceEvent -EventId 65 -Time '2026-09-14T08:50:11.2500000+00:00')
    )

    $regions = Get-PRPerfXamlRegionDurations -Xml $xml

    Assert-Equal 25.0 $regions.XamlInitializeMs 'Event 31 to 17 must be reported in milliseconds.'
    Assert-Equal 100.0 $regions.XamlLayoutMs 'Event 31 to 42 must be reported in milliseconds.'
    Assert-Equal 250.0 $regions.XamlFrameMs 'Event 31 to 65 must be reported in milliseconds.'
}

function Test-TraceIgnoresEventsFromOtherProviders {
    # Event ids are only unique within a provider. Another provider's event 17 would
    # otherwise be read as a XAML startup and produce a confidently wrong number.
    $xml = New-TraceDocument -Events @(
        (New-TraceEvent -EventId 31 -Time '2026-09-14T08:50:11.0000000+00:00')
        (New-TraceEvent -EventId 17 -Time '2026-09-14T08:50:11.0050000+00:00' -Provider '{11111111-1111-1111-1111-111111111111}')
        (New-TraceEvent -EventId 17 -Time '2026-09-14T08:50:11.0250000+00:00')
    )

    $regions = Get-PRPerfXamlRegionDurations -Xml $xml

    Assert-Equal 25.0 $regions.XamlInitializeMs 'Another provider''s event must not be used.'
}

function Test-TraceIgnoresAStopOnADifferentThread {
    # The regions are defined per thread. A stop raised on another UI thread would
    # otherwise be paired with this thread's start.
    $xml = New-TraceDocument -Events @(
        (New-TraceEvent -EventId 31 -Time '2026-09-14T08:50:11.0000000+00:00' -ThreadId 200)
        (New-TraceEvent -EventId 17 -Time '2026-09-14T08:50:11.0050000+00:00' -ThreadId 999)
        (New-TraceEvent -EventId 17 -Time '2026-09-14T08:50:11.0250000+00:00' -ThreadId 200)
    )

    $regions = Get-PRPerfXamlRegionDurations -Xml $xml

    Assert-Equal 25.0 $regions.XamlInitializeMs 'A stop on another thread must not be paired with this start.'
}

function Test-TraceOmitsARegionWhoseStopNeverArrived {
    # A missing region must be absent, so the caller reports it as missing. Substituting
    # a zero would read as an impossibly fast startup and look like a huge improvement.
    $xml = New-TraceDocument -Events @(
        (New-TraceEvent -EventId 31 -Time '2026-09-14T08:50:11.0000000+00:00')
        (New-TraceEvent -EventId 17 -Time '2026-09-14T08:50:11.0250000+00:00')
    )

    $regions = Get-PRPerfXamlRegionDurations -Xml $xml

    Assert-Equal 25.0 $regions.XamlInitializeMs 'The region that did complete must still be reported.'
    if ($regions.Contains('XamlFrameMs')) { throw 'A region with no stop event must not be reported at all.' }
}

function Test-TraceReadsHundredNanosecondTimestamps {
    # tracerpt writes nine fractional digits, which is finer than .NET parses. Truncating
    # must not throw, because losing the whole trace to a formatting detail would be absurd.
    $xml = New-TraceDocument -Events @(
        (New-TraceEvent -EventId 31 -Time '2026-09-14T08:50:11.705617100+00:00')
        (New-TraceEvent -EventId 17 -Time '2026-09-14T08:50:11.715617100+00:00')
    )

    $regions = Get-PRPerfXamlRegionDurations -Xml $xml

    Assert-Equal 10.0 $regions.XamlInitializeMs 'A nine digit fraction must still be read.'
}

function Test-TraceIgnoresAStopThatPrecedesTheStart {
    # A leftover stop from a previous activity would otherwise produce a negative duration,
    # which would be reported as an enormous improvement.
    $xml = New-TraceDocument -Events @(
        (New-TraceEvent -EventId 17 -Time '2026-09-14T08:50:10.0000000+00:00')
        (New-TraceEvent -EventId 31 -Time '2026-09-14T08:50:11.0000000+00:00')
        (New-TraceEvent -EventId 17 -Time '2026-09-14T08:50:11.0250000+00:00')
    )

    $regions = Get-PRPerfXamlRegionDurations -Xml $xml

    Assert-Equal 25.0 $regions.XamlInitializeMs 'A stop before the start must be ignored.'
}

function Test-TraceReturnsNothingWhenTheProviderNeverAppeared {
    # An empty trace means the app was never actually measured. Reporting no regions lets
    # the caller say so, rather than inventing a result.
    $xml = New-TraceDocument -Events @(
        (New-TraceEvent -EventId 31 -Time '2026-09-14T08:50:11.0000000+00:00' -Provider '{11111111-1111-1111-1111-111111111111}')
    )

    $regions = Get-PRPerfXamlRegionDurations -Xml $xml

    Assert-Equal 0 $regions.Count 'A trace without the XAML provider must yield no regions.'
}
