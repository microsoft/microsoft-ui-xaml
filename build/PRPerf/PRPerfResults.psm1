Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Get-PRPerfPropertyValue {
    param(
        $InputObject,
        [Parameter(Mandatory)][string] $Name
    )

    if ($null -eq $InputObject) { return $null }
    $property = $InputObject.PSObject.Properties[$Name]
    if ($null -eq $property) { return $null }
    return $property.Value
}

function Test-PRPerfDataFailed {
    param($InputObject)

    $failed = Get-PRPerfPropertyValue -InputObject $InputObject -Name 'failed'
    if ($null -ne $failed) {
        if ($failed -is [bool] -and $failed) { return $true }
        if ($failed -is [string] -and $failed.Trim() -match '^(?i:true|failed|failure|error)$') { return $true }
        if ($failed -isnot [string] -and $failed -isnot [bool] -and $failed -ne 0) { return $true }
    }

    $status = Get-PRPerfPropertyValue -InputObject $InputObject -Name 'status'
    if ($null -ne $status -and -not [string]::IsNullOrWhiteSpace([string]$status)) {
        $successfulStatuses = @('pass', 'passed', 'success', 'succeeded', 'complete', 'completed', 'ok')
        if ($successfulStatuses -notcontains ([string]$status).Trim().ToLowerInvariant()) { return $true }
    }

    return $false
}

function Test-PRPerfSample {
    param($Sample)

    if (-not (Test-PRPerfFiniteNumber -Value $Sample)) { return $false }
    return [double]$Sample -ge 0
}

function Test-PRPerfFiniteNumber {
    param($Value)

    $numericTypes = @(
        [byte], [sbyte], [int16], [uint16], [int32], [uint32],
        [int64], [uint64], [single], [double], [decimal]
    )
    $isNumber = $false
    foreach ($numericType in $numericTypes) {
        if ($Value -is $numericType) {
            $isNumber = $true
            break
        }
    }
    if (-not $isNumber) { return $false }

    $doubleValue = [double]$Value
    return -not [double]::IsNaN($doubleValue) -and -not [double]::IsInfinity($doubleValue)
}

function Test-PRPerfInteger {
    param($Value)

    if (-not (Test-PRPerfFiniteNumber -Value $Value)) { return $false }
    $doubleValue = [double]$Value
    return $doubleValue -eq [math]::Truncate($doubleValue)
}

function Test-PRPerfNonblankString {
    param($Value)

    return $Value -is [string] -and -not [string]::IsNullOrWhiteSpace($Value)
}

function Assert-PRPerfResultSchema {
    param([Parameter(Mandatory)] $Result)

    $schemaVersion = Get-PRPerfPropertyValue -InputObject $Result -Name 'schemaVersion'
    if (-not (Test-PRPerfInteger -Value $schemaVersion) -or [double]$schemaVersion -ne 1) {
        throw "Unsupported schemaVersion '$schemaVersion'."
    }

    $configVersion = Get-PRPerfPropertyValue -InputObject $Result -Name 'benchmarkConfigVersion'
    if (-not (Test-PRPerfNonblankString -Value $configVersion) -or $configVersion -ne 'pr-smoke-v1') {
        throw "Unsupported benchmarkConfigVersion '$configVersion'."
    }

    $commit = Get-PRPerfPropertyValue -InputObject $Result -Name 'commit'
    if (-not (Test-PRPerfNonblankString -Value $commit) -or $commit -notmatch '^[0-9a-fA-F]{40}$') {
        throw 'commit must be a 40-character hexadecimal string.'
    }
    $buildId = Get-PRPerfPropertyValue -InputObject $Result -Name 'buildId'
    if (-not (Test-PRPerfNonblankString -Value $buildId)) {
        throw 'buildId must be a non-empty string.'
    }

    $machine = Get-PRPerfPropertyValue -InputObject $Result -Name 'machine'
    if ($machine -isnot [pscustomobject]) {
        throw 'machine must be an object.'
    }
    foreach ($field in @('agentName', 'osBuild', 'architecture', 'flavor')) {
        if (-not (Test-PRPerfNonblankString -Value (Get-PRPerfPropertyValue -InputObject $machine -Name $field))) {
            throw "machine.$field must be a non-empty string."
        }
    }

    $scenariosProperty = $Result.PSObject.Properties['scenarios']
    if ($null -eq $scenariosProperty -or
        $scenariosProperty.Value -isnot [System.Array] -or
        $scenariosProperty.Value.Count -eq 0) {
        throw 'scenarios must be a non-empty array.'
    }
    foreach ($scenario in $scenariosProperty.Value) {
        if ($scenario -isnot [pscustomobject]) {
            throw 'Each scenario must be an object.'
        }
        if (-not (Test-PRPerfNonblankString -Value (Get-PRPerfPropertyValue -InputObject $scenario -Name 'name'))) {
            throw 'Each scenario name must be a non-empty string.'
        }
        $metricsProperty = $scenario.PSObject.Properties['metrics']
        if ($null -eq $metricsProperty -or
            $metricsProperty.Value -isnot [System.Array] -or
            $metricsProperty.Value.Count -eq 0) {
            throw 'Each scenario metrics value must be a non-empty array.'
        }
        foreach ($metric in $metricsProperty.Value) {
            if ($metric -isnot [pscustomobject]) {
                throw 'Each metric must be an object.'
            }
            foreach ($field in @('name', 'unit')) {
                if (-not (Test-PRPerfNonblankString -Value (Get-PRPerfPropertyValue -InputObject $metric -Name $field))) {
                    throw "Each metric $field must be a non-empty string."
                }
            }
            $samplesProperty = $metric.PSObject.Properties['samples']
            if ($null -eq $samplesProperty -or $samplesProperty.Value -isnot [System.Array]) {
                throw 'Each metric samples value must be an array.'
            }
            if ($samplesProperty.Value.Count -ne 7) {
                throw 'Each metric samples array must contain exactly 7 entries.'
            }
            foreach ($sample in $samplesProperty.Value) {
                if (-not (Test-PRPerfSample -Sample $sample)) {
                    throw 'Each metric sample must be numeric, finite, and nonnegative.'
                }
            }
        }
    }
}

function New-PRPerfInconclusiveComparison {
    param(
        $Target,
        $Trial,
        [string[]] $Issues
    )

    [pscustomobject]@{
        schemaVersion = 1
        benchmarkConfigVersion = Get-PRPerfPropertyValue -InputObject $Target -Name 'benchmarkConfigVersion'
        target = [pscustomobject]@{
            commit = Get-PRPerfPropertyValue -InputObject $Target -Name 'commit'
            buildId = Get-PRPerfPropertyValue -InputObject $Target -Name 'buildId'
        }
        trial = [pscustomobject]@{
            commit = Get-PRPerfPropertyValue -InputObject $Trial -Name 'commit'
            buildId = Get-PRPerfPropertyValue -InputObject $Trial -Name 'buildId'
        }
        overallState = 'Inconclusive'
        issues = @($Issues)
        scenarios = @()
    }
}

function Get-PRPerfRawStatistics {
    param([Parameter(Mandatory)][double[]] $Samples)

    if ($Samples.Count -eq 0) { throw 'At least one sample is required.' }
    $sorted = @($Samples | Sort-Object)
    $middle = [math]::Floor($sorted.Count / 2)
    $median = if ($sorted.Count % 2) {
        [double]$sorted[$middle]
    } else {
        ([double]$sorted[$middle - 1] + [double]$sorted[$middle]) / 2.0
    }
    $mean = ($Samples | Measure-Object -Average).Average
    $variance = 0.0
    foreach ($sample in $Samples) { $variance += [math]::Pow($sample - $mean, 2) }
    $stdDev = [math]::Sqrt($variance / $Samples.Count)
    $cv = if ($mean -eq 0) { [double]::PositiveInfinity } else { $stdDev / [math]::Abs($mean) }

    [pscustomobject]@{
        Median = $median
        Mean = $mean
        StandardDeviation = $stdDev
        CoefficientOfVariation = $cv
    }
}

function ConvertTo-PRPerfPublicStatistics {
    param([Parameter(Mandatory)] $Statistics)

    [pscustomobject]@{
        Median = [math]::Round($Statistics.Median, 4)
        Mean = [math]::Round($Statistics.Mean, 4)
        StandardDeviation = [math]::Round($Statistics.StandardDeviation, 4)
        CoefficientOfVariation = [math]::Round($Statistics.CoefficientOfVariation, 6)
    }
}

function Get-PRPerfStatistics {
    [CmdletBinding()]
    param([Parameter(Mandatory)][double[]] $Samples)

    $statistics = Get-PRPerfRawStatistics -Samples $Samples
    ConvertTo-PRPerfPublicStatistics -Statistics $statistics
}

function Read-PRPerfResult {
    [CmdletBinding()]
    param([Parameter(Mandatory)][string] $Path)

    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { throw "Result file not found: $Path" }
    $result = Get-Content -LiteralPath $Path -Raw | ConvertFrom-Json
    Assert-PRPerfResultSchema -Result $result
    return $result
}

function Compare-PRPerfResults {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)] $Target,
        [Parameter(Mandatory)] $Trial,
        [Parameter(Mandatory)] $Thresholds
    )

    $issues = @()
    $targetSchemaVersion = Get-PRPerfPropertyValue -InputObject $Target -Name 'schemaVersion'
    $trialSchemaVersion = Get-PRPerfPropertyValue -InputObject $Trial -Name 'schemaVersion'
    if (-not (Test-PRPerfInteger -Value $targetSchemaVersion) -or
        -not (Test-PRPerfInteger -Value $trialSchemaVersion) -or
        [double]$targetSchemaVersion -ne 1 -or
        [double]$trialSchemaVersion -ne 1) {
        $issues += 'Target and trial schemaVersion must both be 1.'
    } elseif ($targetSchemaVersion -ne $trialSchemaVersion) {
        $issues += 'Target/trial schemaVersion mismatch.'
    }

    $targetConfigVersion = Get-PRPerfPropertyValue -InputObject $Target -Name 'benchmarkConfigVersion'
    $trialConfigVersion = Get-PRPerfPropertyValue -InputObject $Trial -Name 'benchmarkConfigVersion'
    $thresholdConfigVersion = Get-PRPerfPropertyValue -InputObject $Thresholds -Name 'benchmarkConfigVersion'
    if (-not (Test-PRPerfNonblankString -Value $targetConfigVersion) -or
        -not (Test-PRPerfNonblankString -Value $trialConfigVersion) -or
        -not (Test-PRPerfNonblankString -Value $thresholdConfigVersion)) {
        $issues += 'benchmarkConfigVersion must be a non-empty string for target, trial, and thresholds.'
    } elseif ($targetConfigVersion -ne $trialConfigVersion -or $targetConfigVersion -ne $thresholdConfigVersion) {
        $issues += 'Target, trial, and threshold benchmarkConfigVersion values must match.'
    }

    foreach ($entry in @(
        [pscustomobject]@{ label = 'Target'; result = $Target },
        [pscustomobject]@{ label = 'Trial'; result = $Trial }
    )) {
        $commit = Get-PRPerfPropertyValue -InputObject $entry.result -Name 'commit'
        $buildId = Get-PRPerfPropertyValue -InputObject $entry.result -Name 'buildId'
        if (-not (Test-PRPerfNonblankString -Value $commit) -or $commit -notmatch '^[0-9a-fA-F]{40}$') {
            $issues += "$($entry.label) commit must be a 40-character hexadecimal string."
        }
        if (-not (Test-PRPerfNonblankString -Value $buildId)) {
            $issues += "$($entry.label) buildId must be a non-empty string."
        }
    }

    $targetMachine = Get-PRPerfPropertyValue -InputObject $Target -Name 'machine'
    $trialMachine = Get-PRPerfPropertyValue -InputObject $Trial -Name 'machine'
    foreach ($field in @('agentName', 'osBuild', 'architecture', 'flavor')) {
        $targetValue = Get-PRPerfPropertyValue -InputObject $targetMachine -Name $field
        $trialValue = Get-PRPerfPropertyValue -InputObject $trialMachine -Name $field
        if (-not (Test-PRPerfNonblankString -Value $targetValue) -or
            -not (Test-PRPerfNonblankString -Value $trialValue) -or
            $targetValue -ne $trialValue) {
            $issues += "Target/trial machine $field values must be non-empty strings and match."
        }
    }

    if (Test-PRPerfDataFailed -InputObject $Target) { $issues += 'Target result reports a failed status.' }
    if (Test-PRPerfDataFailed -InputObject $Trial) { $issues += 'Trial result reports a failed status.' }

    $requiredMetricsProperty = if ($null -eq $Thresholds) {
        $null
    } else {
        $Thresholds.PSObject.Properties['requiredMetrics']
    }
    $requiredMetricsValue = $null
    if ($null -ne $requiredMetricsProperty) {
        $requiredMetricsValue = $requiredMetricsProperty.Value
    }
    $requiredMetrics = @()
    if ($requiredMetricsValue -isnot [System.Array] -or $requiredMetricsValue.Count -eq 0) {
        $issues += 'thresholds.requiredMetrics must contain at least one metric.'
    } else {
        $requiredMetricNames = [System.Collections.Generic.HashSet[string]]::new(
            [System.StringComparer]::OrdinalIgnoreCase)
        foreach ($requiredMetric in $requiredMetricsValue) {
            if ($requiredMetric -isnot [string] -or
                [string]::IsNullOrWhiteSpace($requiredMetric) -or
                -not $requiredMetricNames.Add($requiredMetric)) {
                $issues += 'thresholds.requiredMetrics must contain unique, non-empty metric names.'
                break
            }
            $requiredMetrics += $requiredMetric
        }
        if ($requiredMetrics.Count -ne $requiredMetricsValue.Count) {
            $requiredMetrics = @()
        }
    }

    $requiredSamples = Get-PRPerfPropertyValue -InputObject $Thresholds -Name 'requiredSamples'
    $percentRegression = Get-PRPerfPropertyValue -InputObject $Thresholds -Name 'percentRegression'
    $absoluteRegressionMs = Get-PRPerfPropertyValue -InputObject $Thresholds -Name 'absoluteRegressionMs'
    $maxCoefficientOfVariation = Get-PRPerfPropertyValue -InputObject $Thresholds -Name 'maxCoefficientOfVariation'
    if (-not (Test-PRPerfInteger -Value $requiredSamples) -or [double]$requiredSamples -ne 7) {
        $issues += 'thresholds.requiredSamples must equal 7.'
    }
    if (-not (Test-PRPerfFiniteNumber -Value $percentRegression) -or [double]$percentRegression -ne 10.0) {
        $issues += 'thresholds.percentRegression must equal 10.0.'
    }
    if (-not (Test-PRPerfFiniteNumber -Value $absoluteRegressionMs) -or [double]$absoluteRegressionMs -ne 5.0) {
        $issues += 'thresholds.absoluteRegressionMs must equal 5.0.'
    }
    if (-not (Test-PRPerfFiniteNumber -Value $maxCoefficientOfVariation) -or
        [double]$maxCoefficientOfVariation -ne 0.15) {
        $issues += 'thresholds.maxCoefficientOfVariation must equal 0.15.'
    }

    $targetScenariosProperty = if ($null -eq $Target) { $null } else { $Target.PSObject.Properties['scenarios'] }
    $trialScenariosProperty = if ($null -eq $Trial) { $null } else { $Trial.PSObject.Properties['scenarios'] }
    $targetScenariosValue = $null
    $trialScenariosValue = $null
    if ($null -ne $targetScenariosProperty) { $targetScenariosValue = $targetScenariosProperty.Value }
    if ($null -ne $trialScenariosProperty) { $trialScenariosValue = $trialScenariosProperty.Value }
    $targetScenarios = @()
    $trialScenarios = @()
    if ($targetScenariosValue -isnot [System.Array]) {
        $issues += 'Target scenarios must be an array.'
    } else {
        $targetScenarios = $targetScenariosValue
        if ($targetScenarios.Count -eq 0) {
            $issues += 'Target must contain at least one scenario.'
        }
    }
    if ($trialScenariosValue -isnot [System.Array]) {
        $issues += 'Trial scenarios must be an array.'
    } else {
        $trialScenarios = $trialScenariosValue
        if ($trialScenarios.Count -eq 0) {
            $issues += 'Trial must contain at least one scenario.'
        }
    }

    $targetScenarioMap = @{}
    $trialScenarioMap = @{}
    foreach ($entry in @(
        [pscustomobject]@{ label = 'Target'; scenarios = $targetScenarios; map = $targetScenarioMap },
        [pscustomobject]@{ label = 'Trial'; scenarios = $trialScenarios; map = $trialScenarioMap }
    )) {
        foreach ($scenario in $entry.scenarios) {
            $scenarioName = Get-PRPerfPropertyValue -InputObject $scenario -Name 'name'
            if (-not (Test-PRPerfNonblankString -Value $scenarioName)) {
                $issues += "$($entry.label) contains a scenario whose name is not a non-empty string."
            } elseif ($entry.map.ContainsKey($scenarioName)) {
                $issues += "$($entry.label) scenario '$scenarioName' is duplicated."
            } else {
                $entry.map[$scenarioName] = $scenario
            }
            if (Test-PRPerfDataFailed -InputObject $scenario) {
                $issues += "$($entry.label) scenario '$scenarioName' reports a failed status."
            }
        }
    }

    $targetScenarioNames = @($targetScenarioMap.Keys | Sort-Object)
    $trialScenarioNames = @($trialScenarioMap.Keys | Sort-Object)
    if (Compare-Object -ReferenceObject $targetScenarioNames -DifferenceObject $trialScenarioNames) {
        $issues += 'Target/trial scenario sets do not match.'
    }

    $scenarioMetricMaps = @{}
    foreach ($scenarioName in $targetScenarioNames) {
        if (-not $trialScenarioMap.ContainsKey($scenarioName)) { continue }

        $targetMetricsProperty = $targetScenarioMap[$scenarioName].PSObject.Properties['metrics']
        $trialMetricsProperty = $trialScenarioMap[$scenarioName].PSObject.Properties['metrics']
        $targetMetricsValue = $null
        $trialMetricsValue = $null
        if ($null -ne $targetMetricsProperty) { $targetMetricsValue = $targetMetricsProperty.Value }
        if ($null -ne $trialMetricsProperty) { $trialMetricsValue = $trialMetricsProperty.Value }
        $targetMetrics = @()
        $trialMetrics = @()
        if ($targetMetricsValue -isnot [System.Array]) {
            $issues += "Target scenario '$scenarioName' metrics must be an array."
        } else {
            $targetMetrics = $targetMetricsValue
        }
        if ($trialMetricsValue -isnot [System.Array]) {
            $issues += "Trial scenario '$scenarioName' metrics must be an array."
        } else {
            $trialMetrics = $trialMetricsValue
        }
        $targetMetricMap = @{}
        $trialMetricMap = @{}
        foreach ($entry in @(
            [pscustomobject]@{ label = 'Target'; metrics = $targetMetrics; map = $targetMetricMap },
            [pscustomobject]@{ label = 'Trial'; metrics = $trialMetrics; map = $trialMetricMap }
        )) {
            foreach ($metric in $entry.metrics) {
                $metricName = Get-PRPerfPropertyValue -InputObject $metric -Name 'name'
                if (-not (Test-PRPerfNonblankString -Value $metricName)) {
                    $issues += "$($entry.label) scenario '$scenarioName' contains a metric whose name is not a non-empty string."
                } elseif ($entry.map.ContainsKey($metricName)) {
                    $issues += "$($entry.label) metric '$scenarioName/$metricName' is duplicated."
                } else {
                    $entry.map[$metricName] = $metric
                }
                if (Test-PRPerfDataFailed -InputObject $metric) {
                    $issues += "$($entry.label) metric '$scenarioName/$metricName' reports a failed status."
                }
            }
        }

        $targetMetricNames = @($targetMetricMap.Keys | Sort-Object)
        $trialMetricNames = @($trialMetricMap.Keys | Sort-Object)
        if (Compare-Object -ReferenceObject $targetMetricNames -DifferenceObject $trialMetricNames) {
            $issues += "Target/trial metric sets for scenario '$scenarioName' do not match."
        }
        foreach ($requiredMetric in $requiredMetrics) {
            if (-not $targetMetricMap.ContainsKey($requiredMetric) -or -not $trialMetricMap.ContainsKey($requiredMetric)) {
                $issues += "Required metric '$scenarioName/$requiredMetric' is missing."
            }
        }
        foreach ($metricName in $targetMetricNames) {
            if (-not $trialMetricMap.ContainsKey($metricName)) { continue }
            $targetUnit = Get-PRPerfPropertyValue -InputObject $targetMetricMap[$metricName] -Name 'unit'
            $trialUnit = Get-PRPerfPropertyValue -InputObject $trialMetricMap[$metricName] -Name 'unit'
            if (-not (Test-PRPerfNonblankString -Value $targetUnit) -or
                -not (Test-PRPerfNonblankString -Value $trialUnit) -or
                $targetUnit -ne $trialUnit) {
                $issues += "Target/trial unit for metric '$scenarioName/$metricName' must be non-empty strings and match."
            }
        }
        $scenarioMetricMaps[$scenarioName] = [pscustomobject]@{
            target = $targetMetricMap
            trial = $trialMetricMap
        }
    }

    if ($issues.Count -gt 0) {
        return New-PRPerfInconclusiveComparison -Target $Target -Trial $Trial -Issues $issues
    }

    $rows = @()
    foreach ($scenarioName in $targetScenarioNames) {
        $metricRows = @()
        $metricMaps = $scenarioMetricMaps[$scenarioName]
        foreach ($metricName in @($metricMaps.target.Keys | Sort-Object)) {
            $targetMetric = $metricMaps.target[$metricName]
            $trialMetric = $metricMaps.trial[$metricName]
            $targetSamplesProperty = $targetMetric.PSObject.Properties['samples']
            $trialSamplesProperty = $trialMetric.PSObject.Properties['samples']
            $targetSamplesValue = $null
            $trialSamplesValue = $null
            if ($null -ne $targetSamplesProperty) { $targetSamplesValue = $targetSamplesProperty.Value }
            if ($null -ne $trialSamplesProperty) { $trialSamplesValue = $trialSamplesProperty.Value }
            $targetSamples = @()
            $trialSamples = @()
            $validSamples = $targetSamplesValue -is [System.Array] -and $trialSamplesValue -is [System.Array]
            if ($targetSamplesValue -is [System.Array]) { $targetSamples = $targetSamplesValue }
            if ($trialSamplesValue -is [System.Array]) { $trialSamples = $trialSamplesValue }
            foreach ($sample in ($targetSamples + $trialSamples)) {
                if (-not (Test-PRPerfSample -Sample $sample)) {
                    $validSamples = $false
                    break
                }
            }
            if (-not $validSamples -or
                $targetSamples.Count -ne $requiredSamples -or
                $trialSamples.Count -ne $requiredSamples) {
                $classification = 'Inconclusive'
                $targetStats = $null
                $trialStats = $null
                $absoluteDelta = $null
                $percentDelta = $null
            } else {
                $targetRawStats = Get-PRPerfRawStatistics -Samples ([double[]]$targetSamples)
                $trialRawStats = Get-PRPerfRawStatistics -Samples ([double[]]$trialSamples)
                $targetStats = ConvertTo-PRPerfPublicStatistics -Statistics $targetRawStats
                $trialStats = ConvertTo-PRPerfPublicStatistics -Statistics $trialRawStats
                $absoluteDelta = $trialRawStats.Median - $targetRawStats.Median
                $percentDelta = if ($targetRawStats.Median -eq 0) {
                    $null
                } else {
                    100.0 * $absoluteDelta / $targetRawStats.Median
                }
                $noisy = $targetRawStats.CoefficientOfVariation -gt $maxCoefficientOfVariation -or
                    $trialRawStats.CoefficientOfVariation -gt $maxCoefficientOfVariation
                if ($noisy -or $null -eq $percentDelta) {
                    $classification = 'Inconclusive'
                } elseif ($absoluteDelta -ge $absoluteRegressionMs -and $percentDelta -ge $percentRegression) {
                    $classification = 'Regressed'
                } elseif ($absoluteDelta -le -$absoluteRegressionMs -and $percentDelta -le -$percentRegression) {
                    $classification = 'Improved'
                } else {
                    $classification = 'Passed'
                }
            }

            $metricRows += [pscustomobject]@{
                name = $metricName
                unit = Get-PRPerfPropertyValue -InputObject $targetMetric -Name 'unit'
                target = $targetStats
                trial = $trialStats
                absoluteDelta = $absoluteDelta
                percentDelta = $percentDelta
                classification = $classification
            }
        }
        $rows += [pscustomobject]@{ name = $scenarioName; metrics = $metricRows }
    }

    $classes = @($rows.metrics.classification)
    $overall = if ($classes -contains 'Regressed') { 'RegressionWarning' }
        elseif ($classes -contains 'Inconclusive') { 'Inconclusive' }
        else { 'Passed' }

    [pscustomobject]@{
        schemaVersion = 1
        benchmarkConfigVersion = $Target.benchmarkConfigVersion
        target = [pscustomobject]@{ commit = $Target.commit; buildId = $Target.buildId }
        trial = [pscustomobject]@{ commit = $Trial.commit; buildId = $Trial.buildId }
        overallState = $overall
        issues = @()
        scenarios = $rows
    }
}

function Compare-PRPerfFiles {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)][string] $TargetPath,
        [Parameter(Mandatory)][string] $TrialPath,
        [Parameter(Mandatory)][string] $ThresholdPath,

        # When supplied, the measured results must actually come from these commits. A result
        # file left over from an earlier run, or fixture data, otherwise compares cleanly and
        # reports a pass for a commit that was never measured.
        [string] $ExpectedTargetCommit = '',
        [string] $ExpectedTrialCommit = ''
    )
    $issues = @()
    $target = $null
    $trial = $null
    $thresholds = $null

    try {
        $target = Read-PRPerfResult -Path $TargetPath
    } catch {
        $issues += "Unable to read target result '$TargetPath': $($_.Exception.Message)"
    }
    try {
        $trial = Read-PRPerfResult -Path $TrialPath
    } catch {
        $issues += "Unable to read trial result '$TrialPath': $($_.Exception.Message)"
    }
    try {
        if (-not (Test-Path -LiteralPath $ThresholdPath -PathType Leaf)) {
            throw "Threshold file not found: $ThresholdPath"
        }
        $thresholds = Get-Content -LiteralPath $ThresholdPath -Raw | ConvertFrom-Json
    } catch {
        $issues += "Unable to read thresholds '$ThresholdPath': $($_.Exception.Message)"
    }

    if ($issues.Count -gt 0) {
        return New-PRPerfInconclusiveComparison -Target $target -Trial $trial -Issues $issues
    }

    $measuredTargetCommit = Get-PRPerfPropertyValue -InputObject $target -Name 'commit'
    $measuredTrialCommit = Get-PRPerfPropertyValue -InputObject $trial -Name 'commit'
    $provenanceIssues = @()
    if (-not [string]::IsNullOrWhiteSpace($ExpectedTargetCommit) -and
        $measuredTargetCommit -ine $ExpectedTargetCommit) {
        $provenanceIssues += "Target result was measured at commit '$measuredTargetCommit' but the run was requested for '$ExpectedTargetCommit'."
    }
    if (-not [string]::IsNullOrWhiteSpace($ExpectedTrialCommit) -and
        $measuredTrialCommit -ine $ExpectedTrialCommit) {
        $provenanceIssues += "Trial result was measured at commit '$measuredTrialCommit' but the run was requested for '$ExpectedTrialCommit'."
    }
    if ($measuredTargetCommit -ieq $measuredTrialCommit) {
        $provenanceIssues += "Target and trial were both measured at commit '$measuredTargetCommit', so the comparison says nothing about the change."
    }
    if ($provenanceIssues.Count -gt 0) {
        return New-PRPerfInconclusiveComparison -Target $target -Trial $trial -Issues $provenanceIssues
    }

    try {
        Compare-PRPerfResults -Target $target -Trial $trial -Thresholds $thresholds
    } catch {
        New-PRPerfInconclusiveComparison `
            -Target $target `
            -Trial $trial `
            -Issues @("Unable to compare PR performance results: $($_.Exception.Message)")
    }
}

Export-ModuleMember -Function Get-PRPerfStatistics, Read-PRPerfResult, Compare-PRPerfResults, Compare-PRPerfFiles
