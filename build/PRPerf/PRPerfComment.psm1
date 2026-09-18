Set-StrictMode -Version Latest
$script:Marker = '<!-- winui-pr-perf-result -->'

function Format-PRPerfNumber($Value, [string] $Suffix = '') {
    if ($null -eq $Value) { return 'n/a' }
    return ([double]$Value).ToString('N2', [System.Globalization.CultureInfo]::InvariantCulture) + $Suffix
}

function ConvertTo-PRPerfMarkdownCell($Value) {
    $cell = [regex]::Replace([string]$Value, '\r\n|\r|\n', ' ')
    return $cell.Replace('\', '\\').Replace('|', '\|')
}

function Get-PRPerfXamlRegions {
    <#
    .SYNOPSIS
    The measured XAML regions, if a trace produced any.

    .DESCRIPTION
    Every failure here is silent and yields nothing. The regions are an extra observation
    on top of a comparison that already works, so losing the whole comment because the
    trace file was missing or half written would trade a useful result for no result.
    #>
    [CmdletBinding()]
    param([Parameter(Mandatory)][AllowEmptyString()][string] $Path)

    $regions = [ordered]@{}
    if ([string]::IsNullOrWhiteSpace($Path) -or -not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        return $regions
    }

    try {
        $parsed = Get-Content -LiteralPath $Path -Raw | ConvertFrom-Json
    } catch {
        Write-Host "Ignoring unreadable XAML region file '$Path': $($_.Exception.Message)"
        return $regions
    }
    if ($null -eq $parsed) { return $regions }

    foreach ($property in $parsed.PSObject.Properties) {
        # Anything that is not a plain number cannot be a duration, and rendering it would
        # put a meaningless figure in front of a reviewer as though it had been measured.
        $value = $property.Value -as [double]
        if ($null -eq $value -or -not [double]::IsFinite($value)) { continue }
        $regions[$property.Name] = $value
    }
    return $regions
}

function Get-PRPerfXamlTraceApp {
    <#
    .SYNOPSIS
    The app the trace measured, if one was recorded.
    #>
    [CmdletBinding()]
    param([Parameter(Mandatory)][AllowEmptyString()][string] $Path)

    if ([string]::IsNullOrWhiteSpace($Path) -or -not (Test-Path -LiteralPath $Path -PathType Leaf)) { return '' }
    try {
        $parsed = Get-Content -LiteralPath $Path -Raw | ConvertFrom-Json
    } catch {
        return ''
    }
    if ($null -eq $parsed) { return '' }

    $property = $parsed.PSObject.Properties['app']
    if ($null -eq $property) { return '' }
    return [string]$property.Value
}

function Get-PRPerfXamlRegionSection {
    param($XamlRegions, [string] $XamlTraceApp)

    if ($null -eq $XamlRegions) { return @() }
    $names = @($XamlRegions.Keys)
    # Nothing measured means the trace was switched off or did not produce usable events.
    # The comparison that did work must then read exactly as it would have anyway.
    if ($names.Count -eq 0) { return @() }

    # Naming the app is what keeps the section honest. A reader who sees a XAML number
    # beside a pull request verdict will otherwise take it as this pull request's XAML,
    # and an app that uses the system XAML has not touched the branch's code at all.
    $heading = '### XAML startup regions (informational, not attributed to this pull request)'
    $lines = @('', $heading, '')
    if (-not [string]::IsNullOrWhiteSpace($XamlTraceApp)) {
        $lines += @("Measured by launching ``$(ConvertTo-PRPerfMarkdownCell $XamlTraceApp)``, with no baseline to compare against.", '')
    } else {
        $lines += @('Measured from a single app launch, with no baseline to compare against.', '')
    }
    $lines += @(
        '| Region | Measured |',
        '|---|---:|'
    )
    foreach ($name in $names) {
        $lines += "| $(ConvertTo-PRPerfMarkdownCell $name) | $(Format-PRPerfNumber $XamlRegions[$name] ' ms') |"
    }
    return $lines
}

function New-PRPerfMarkdown {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)] $Comparison,
        [Parameter(Mandatory)][AllowEmptyString()][string] $ArtifactUrl,
        [Parameter(Mandatory)][AllowEmptyString()][string] $PipelineUrl,

        # Measured on the pull request build alone. There is deliberately no baseline: the
        # two sides need different framework packages installed, which cannot both be
        # present at once, so these are reported as an observation and never as a verdict.
        $XamlRegions = $null,

        # The app whose launch produced those regions. Naming it is what stops a reader
        # taking a number beside a pull request verdict as this pull request's XAML.
        [AllowEmptyString()][string] $XamlTraceApp = ''
    )

    $title = switch ($Comparison.overallState) {
        'Passed' { 'Perf regression test passed' }
        'RegressionWarning' { 'Performance regression warning' }
        default { 'Performance result inconclusive' }
    }
    $lines = @(
        $script:Marker,
        "## $title",
        '',
        '| Scenario | Metric | Target median | PR median | Delta | Delta % | Target CV | PR CV | Result |',
        '|---|---:|---:|---:|---:|---:|---:|---:|---|'
    )
    foreach ($scenario in $Comparison.scenarios) {
        foreach ($metric in $scenario.metrics) {
            $scenarioName = ConvertTo-PRPerfMarkdownCell $scenario.name
            $metricName = ConvertTo-PRPerfMarkdownCell $metric.name
            $unitSuffix = ' ' + (ConvertTo-PRPerfMarkdownCell $metric.unit)
            $classification = ConvertTo-PRPerfMarkdownCell $metric.classification
            $lines += "| $scenarioName | $metricName | $(Format-PRPerfNumber $metric.target.Median $unitSuffix) | $(Format-PRPerfNumber $metric.trial.Median $unitSuffix) | $(Format-PRPerfNumber $metric.absoluteDelta $unitSuffix) | $(Format-PRPerfNumber $metric.percentDelta '%') | $(Format-PRPerfNumber (100 * $metric.target.CoefficientOfVariation) '%') | $(Format-PRPerfNumber (100 * $metric.trial.CoefficientOfVariation) '%') | $classification |"
        }
    }
    if ($null -ne $Comparison.issues -and @($Comparison.issues).Count -gt 0) {
        $lines += @(
            '',
            '### Inconclusive details'
        )
        foreach ($issue in @($Comparison.issues)) {
            $lines += "- $(ConvertTo-PRPerfMarkdownCell $issue)"
        }
    }
    $baselineKind = ''
    $kindProperty = $Comparison.target.PSObject.Properties['baselineKind']
    # Only kinds this code actually produces are rendered. The compare step runs on paths
    # where the pipeline variable was never set, and printing an unexpanded literal to a
    # pull request as though it were a fact would be worse than saying nothing.
    if ($null -ne $kindProperty -and @('main', 'same-branch') -contains [string]$kindProperty.Value) {
        # A main baseline attributes the delta to this pull request. A same-branch baseline
        # only compares the pull request with its own earlier self. Saying which one was
        # used keeps the reader from over-reading the number.
        $baselineKind = ", $([string]$kindProperty.Value) baseline"
    }
    $lines += @(
        '',
        "Target: ``$($Comparison.target.commit)`` (build $($Comparison.target.buildId)$baselineKind)",
        "PR: ``$($Comparison.trial.commit)`` (build $($Comparison.trial.buildId))"
    )
    $lines += Get-PRPerfXamlRegionSection -XamlRegions $XamlRegions -XamlTraceApp $XamlTraceApp
    $lines += @(
        '',
        "[Artifacts]($ArtifactUrl) | [Pipeline run]($PipelineUrl)"
    )
    return $lines -join "`n"
}

function Find-PRPerfComment {
    [CmdletBinding()]
    param([Parameter(Mandatory)][AllowEmptyCollection()][object[]] $Threads)

    foreach ($thread in $Threads) {
        foreach ($comment in $thread.comments) {
            if ($comment.content -like "*$script:Marker*") {
                return [pscustomobject]@{
                    ThreadId = $thread.id
                    CommentId = $comment.id
                }
            }
        }
    }
    return $null
}

Export-ModuleMember -Function New-PRPerfMarkdown, Find-PRPerfComment, Get-PRPerfXamlRegions, Get-PRPerfXamlTraceApp
