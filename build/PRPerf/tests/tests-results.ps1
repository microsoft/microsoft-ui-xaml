$root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
Import-Module (Join-Path $root 'PRPerfResults.psm1') -Force

function Assert-Equal($Expected, $Actual, [string] $Message) {
    if ($Expected -ne $Actual) {
        throw "$Message Expected='$Expected' Actual='$Actual'"
    }
}

function Assert-Inconclusive($Comparison, [string] $Message) {
    if ($null -eq $Comparison) {
        throw "$Message Comparison was null."
    }
    Assert-Equal 'Inconclusive' $Comparison.overallState $Message
    Assert-Equal 1 $Comparison.schemaVersion "$Message schemaVersion mismatch."
    foreach ($propertyName in @('target', 'trial', 'issues', 'scenarios')) {
        if ($null -eq $Comparison.PSObject.Properties[$propertyName]) {
            throw "$Message Missing comparison property '$propertyName'."
        }
    }
}

function Assert-ReadRejectsSamples([string] $SamplesJson, [string] $Message) {
    $invalidResultPath = Join-Path $PSScriptRoot "invalid-samples-$([guid]::NewGuid()).json"
    try {
        $json = New-TestResult | ConvertTo-Json -Depth 12
        $json = $json -replace '"samples"\s*:\s*\[[^\]]*\]', "`"samples`": $SamplesJson"
        $json | Set-Content -LiteralPath $invalidResultPath -Encoding UTF8

        $threw = $false
        try {
            Read-PRPerfResult -Path $invalidResultPath
        } catch {
            $threw = $true
        }
        Assert-Equal $true $threw $Message
    } finally {
        if (Test-Path -LiteralPath $invalidResultPath) {
            Remove-Item -LiteralPath $invalidResultPath -Force
        }
    }
}

function New-TestResult {
    param(
        [double[]] $Samples = @(100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0),
        [string] $MetricName = 'CpuTimeMs',
        [string] $Unit = 'ms',
        [string] $ScenarioName = 'ObjectCreation.BasicControls'
    )

    [pscustomobject]@{
        schemaVersion = 1
        benchmarkConfigVersion = 'pr-smoke-v1'
        commit = 'aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa'
        buildId = '1001'
        machine = [pscustomobject]@{
            agentName = 'PERF-01'
            osBuild = '26100'
            architecture = 'amd64'
            flavor = 'fre'
        }
        scenarios = @(
            [pscustomobject]@{
                name = $ScenarioName
                metrics = @(
                    [pscustomobject]@{
                        name = $MetricName
                        unit = $Unit
                        samples = $Samples
                    }
                )
            }
        )
    }
}

function Get-TestThresholds {
    param(
        [string[]] $RequiredMetrics = @('CpuTimeMs')
    )

    [pscustomobject]@{
        benchmarkConfigVersion = 'pr-smoke-v1'
        requiredSamples = 7
        percentRegression = 10.0
        absoluteRegressionMs = 5.0
        maxCoefficientOfVariation = 0.15
        requiredMetrics = $RequiredMetrics
    }
}

function Test-MedianUsesSortedMiddleSample {
    $stats = Get-PRPerfStatistics -Samples @(5.0, 1.0, 3.0, 2.0, 4.0)
    Assert-Equal 3.0 $stats.Median 'Median mismatch.'
}

function Test-PublicStatisticsUseContractRounding {
    $stats = Get-PRPerfStatistics -Samples @(1.11111, 2.22222, 4.44444, 8.88888)
    Assert-Equal 3.3333 $stats.Median 'Median must be rounded to four decimals.'
    Assert-Equal 4.1667 $stats.Mean 'Mean must be rounded to four decimals.'
    Assert-Equal 2.9788 $stats.StandardDeviation 'Standard deviation must be rounded to four decimals.'
    Assert-Equal 0.714920 $stats.CoefficientOfVariation 'Coefficient of variation must be rounded to six decimals.'
}

function Test-ExactThresholdBoundariesRegress {
    $comparison = Compare-PRPerfResults `
        -Target (New-TestResult -Samples @(50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0)) `
        -Trial (New-TestResult -Samples @(55.0, 55.0, 55.0, 55.0, 55.0, 55.0, 55.0)) `
        -Thresholds (Get-TestThresholds)
    Assert-Equal 'RegressionWarning' $comparison.overallState 'Exact threshold boundaries must regress.'
}

function Test-OnlyOneThresholdBoundaryPasses {
    $comparison = Compare-PRPerfResults `
        -Target (New-TestResult) `
        -Trial (New-TestResult -Samples @(105.0, 105.0, 105.0, 105.0, 105.0, 105.0, 105.0)) `
        -Thresholds (Get-TestThresholds)
    Assert-Equal 'Passed' $comparison.overallState 'Absolute threshold alone must not regress.'

    $comparison = Compare-PRPerfResults `
        -Target (New-TestResult -Samples @(40.0, 40.0, 40.0, 40.0, 40.0, 40.0, 40.0)) `
        -Trial (New-TestResult -Samples @(44.0, 44.0, 44.0, 44.0, 44.0, 44.0, 44.0)) `
        -Thresholds (Get-TestThresholds)
    Assert-Equal 'Passed' $comparison.overallState 'Percentage threshold alone must not regress.'
}

function Test-PercentageBelowThresholdDoesNotRegressAfterDisplayRounding {
    $comparison = Compare-PRPerfResults `
        -Target (New-TestResult) `
        -Trial (New-TestResult -Samples @(109.99996, 109.99996, 109.99996, 109.99996, 109.99996, 109.99996, 109.99996)) `
        -Thresholds (Get-TestThresholds)
    Assert-Equal 'Passed' $comparison.overallState 'A true percentage below 10% must not regress when it visually rounds to 10%.'
}

function Test-CVAboveThresholdIsInconclusiveAfterDisplayRounding {
    $deviation = 0.1500004 * [math]::Sqrt(7.0) * 50.0
    $trialSamples = @(
        (100.0 - $deviation),
        (100.0 - $deviation),
        100.0,
        100.0,
        100.0,
        (100.0 + $deviation),
        (100.0 + $deviation)
    )
    $comparison = Compare-PRPerfResults `
        -Target (New-TestResult) `
        -Trial (New-TestResult -Samples $trialSamples) `
        -Thresholds (Get-TestThresholds)
    Assert-Equal 'Inconclusive' $comparison.overallState 'A true CV above 0.15 must be inconclusive when it visually rounds to 0.15.'
}

function Test-PassRequiresBothThresholds {
    $comparison = Compare-PRPerfFiles `
        -TargetPath (Join-Path $root 'fixtures\target-pass.json') `
        -TrialPath (Join-Path $root 'fixtures\trial-pass.json') `
        -ThresholdPath (Join-Path $root 'pr-perf-thresholds.json')
    Assert-Equal 'Passed' $comparison.overallState 'Expected a passing comparison.'
}

function Test-RegressionExceedsPercentageAndAbsoluteThresholds {
    $comparison = Compare-PRPerfFiles `
        -TargetPath (Join-Path $root 'fixtures\target-pass.json') `
        -TrialPath (Join-Path $root 'fixtures\trial-regression.json') `
        -ThresholdPath (Join-Path $root 'pr-perf-thresholds.json')
    Assert-Equal 'RegressionWarning' $comparison.overallState 'Expected a regression warning.'
    Assert-Equal 'Regressed' $comparison.scenarios[0].metrics[0].classification 'Metric classification mismatch.'
}

function Test-NoisyTrialIsInconclusive {
    $comparison = Compare-PRPerfFiles `
        -TargetPath (Join-Path $root 'fixtures\target-pass.json') `
        -TrialPath (Join-Path $root 'fixtures\trial-noisy.json') `
        -ThresholdPath (Join-Path $root 'pr-perf-thresholds.json')
    Assert-Equal 'Inconclusive' $comparison.overallState 'Noisy data must not pass.'
}

function Test-NoisyTargetIsInconclusive {
    $comparison = Compare-PRPerfResults `
        -Target (New-TestResult -Samples @(40.0, 160.0, 50.0, 150.0, 60.0, 140.0, 70.0)) `
        -Trial (New-TestResult) `
        -Thresholds (Get-TestThresholds)
    Assert-Equal 'Inconclusive' $comparison.overallState 'Noisy target data must not pass.'
}

function Test-RequiredMetricMustBePresent {
    $comparison = Compare-PRPerfResults `
        -Target (New-TestResult -MetricName 'WallClockMs') `
        -Trial (New-TestResult -MetricName 'WallClockMs') `
        -Thresholds (Get-TestThresholds)
    Assert-Equal 'Inconclusive' $comparison.overallState 'Missing required metrics must be inconclusive.'
}

function Test-RequiredMetricsMustNotBeEmpty {
    $comparison = Compare-PRPerfResults `
        -Target (New-TestResult) `
        -Trial (New-TestResult) `
        -Thresholds (Get-TestThresholds -RequiredMetrics @())
    Assert-Equal 'Inconclusive' $comparison.overallState 'Empty requiredMetrics must be inconclusive.'
}

function Test-RequiredMetricsMustBeUniqueNonblankStrings {
    foreach ($requiredMetrics in @(
        $null,
        @('CpuTimeMs', $null),
        @('CpuTimeMs', ''),
        @('CpuTimeMs', '   '),
        @('CpuTimeMs', 'CpuTimeMs'),
        @('CpuTimeMs', 42),
        'CpuTimeMs'
    )) {
        $thresholds = Get-TestThresholds
        $thresholds.requiredMetrics = $requiredMetrics
        $comparison = Compare-PRPerfResults `
            -Target (New-TestResult) `
            -Trial (New-TestResult) `
            -Thresholds $thresholds
        Assert-Inconclusive $comparison "Invalid requiredMetrics '$requiredMetrics' must be inconclusive."
    }
}

function Test-ThresholdValuesMustBeTypedFiniteAndApproved {
    $invalidThresholds = @(
        @{ Name = 'percentRegression'; Values = @($null, '10.0', [double]::NaN, [double]::PositiveInfinity, 9.999) },
        @{ Name = 'absoluteRegressionMs'; Values = @($null, '5.0', [double]::NaN, [double]::PositiveInfinity, 4.999) },
        @{ Name = 'maxCoefficientOfVariation'; Values = @($null, '0.15', [double]::NaN, [double]::PositiveInfinity, -0.001, 0.151) },
        @{ Name = 'requiredSamples'; Values = @($null, '7', [double]::NaN, [double]::PositiveInfinity, 6, 7.5) }
    )

    foreach ($threshold in $invalidThresholds) {
        foreach ($value in $threshold.Values) {
            $thresholds = Get-TestThresholds
            $thresholds.($threshold.Name) = $value
            $comparison = Compare-PRPerfResults `
                -Target (New-TestResult) `
                -Trial (New-TestResult) `
                -Thresholds $thresholds
            Assert-Inconclusive $comparison "Invalid threshold $($threshold.Name)='$value' must be inconclusive."
        }
    }
}

function Test-ThresholdValuesMustExactlyMatchApprovedConfiguration {
    $variants = @(
        @{ Name = 'requiredSamples'; Values = @(6, 8) },
        @{ Name = 'percentRegression'; Values = @(9.0, 11.0) },
        @{ Name = 'absoluteRegressionMs'; Values = @(4.0, 6.0) },
        @{ Name = 'maxCoefficientOfVariation'; Values = @(0.14, 0.16) }
    )

    foreach ($variant in $variants) {
        foreach ($value in $variant.Values) {
            $thresholds = Get-TestThresholds
            $thresholds.($variant.Name) = $value
            $comparison = Compare-PRPerfResults `
                -Target (New-TestResult) `
                -Trial (New-TestResult) `
                -Thresholds $thresholds
            Assert-Inconclusive $comparison "Threshold $($variant.Name)='$value' must exactly match the approved configuration."
        }
    }
}

function Test-RequiredMetricsMustContainExactlySevenSamples {
    foreach ($sampleCount in @(6, 8)) {
        $samples = @(
            1..$sampleCount | ForEach-Object { 100.0 }
        )
        $comparison = Compare-PRPerfResults `
            -Target (New-TestResult -Samples $samples) `
            -Trial (New-TestResult -Samples $samples) `
            -Thresholds (Get-TestThresholds)
        Assert-Inconclusive $comparison "Required metrics with $sampleCount samples must be inconclusive."
    }
}

function Test-ReadPRPerfResultRejectsInvalidSchemaDirectly {
    $invalidResultPath = Join-Path $PSScriptRoot "invalid-result-$([guid]::NewGuid()).json"
    try {
        $invalidResult = New-TestResult
        $invalidResult.schemaVersion = 2
        $invalidResult | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath $invalidResultPath -Encoding UTF8

        $threw = $false
        try {
            Read-PRPerfResult -Path $invalidResultPath
        } catch {
            $threw = $true
        }
        Assert-Equal $true $threw 'Read-PRPerfResult must throw for an invalid result schema.'
    } finally {
        if (Test-Path -LiteralPath $invalidResultPath) {
            Remove-Item -LiteralPath $invalidResultPath -Force
        }
    }
}

function Test-ReadPRPerfResultRejectsSixOrEightSamplesDirectly {
    Assert-ReadRejectsSamples `
        -SamplesJson '[100, 100, 100, 100, 100, 100]' `
        -Message 'Read-PRPerfResult must reject metrics with six samples.'
    Assert-ReadRejectsSamples `
        -SamplesJson '[100, 100, 100, 100, 100, 100, 100, 100]' `
        -Message 'Read-PRPerfResult must reject metrics with eight samples.'
}

function Test-ReadPRPerfResultRejectsMalformedSamplesDirectly {
    foreach ($case in @(
        @{ SamplesJson = '[100, 100, 100, 100, 100, 100, NaN]'; Label = 'NaN' },
        @{ SamplesJson = '[100, 100, 100, 100, 100, 100, Infinity]'; Label = 'infinite' },
        @{ SamplesJson = '[100, 100, 100, 100, 100, 100, "100"]'; Label = 'non-numeric' },
        @{ SamplesJson = '[100, 100, 100, 100, 100, 100, -1]'; Label = 'negative' }
    )) {
        Assert-ReadRejectsSamples `
            -SamplesJson $case.SamplesJson `
            -Message "Read-PRPerfResult must reject $($case.Label) samples."
    }
}

function Test-OptionalMetricsMustContainExactlySevenValidSamples {
    $target = New-TestResult
    $trial = New-TestResult
    $target.scenarios[0].metrics += (New-TestResult `
        -MetricName 'OptionalMetric' `
        -Samples @(100.0, 100.0, 100.0, 100.0, 100.0, 100.0)).scenarios[0].metrics[0]
    $trial.scenarios[0].metrics += (New-TestResult `
        -MetricName 'OptionalMetric' `
        -Samples @(100.0, 100.0, 100.0, 100.0, 100.0, 100.0)).scenarios[0].metrics[0]

    $comparison = Compare-PRPerfResults `
        -Target $target `
        -Trial $trial `
        -Thresholds (Get-TestThresholds)
    Assert-Inconclusive $comparison 'An optional metric with six samples must not coexist with Passed.'
}

function Test-MissingAndMalformedResultFilesAreInconclusive {
    $missingPath = Join-Path ([System.IO.Path]::GetTempPath()) "pr-perf-missing-$([guid]::NewGuid()).json"
    $malformedPath = Join-Path ([System.IO.Path]::GetTempPath()) "pr-perf-malformed-$([guid]::NewGuid()).json"
    try {
        '{not-json' | Set-Content -LiteralPath $malformedPath -Encoding UTF8
        foreach ($paths in @(
            @{ Target = $missingPath; Trial = (Join-Path $root 'fixtures\trial-pass.json') },
            @{ Target = (Join-Path $root 'fixtures\target-pass.json'); Trial = $missingPath },
            @{ Target = $malformedPath; Trial = (Join-Path $root 'fixtures\trial-pass.json') },
            @{ Target = (Join-Path $root 'fixtures\target-pass.json'); Trial = $malformedPath }
        )) {
            $comparison = Compare-PRPerfFiles `
                -TargetPath $paths.Target `
                -TrialPath $paths.Trial `
                -ThresholdPath (Join-Path $root 'pr-perf-thresholds.json')
            Assert-Inconclusive $comparison 'Missing or malformed result files must be inconclusive.'
        }
    } finally {
        if (Test-Path -LiteralPath $malformedPath) {
            Remove-Item -LiteralPath $malformedPath -Force
        }
    }
}

function Test-MissingAndMalformedThresholdFilesAreInconclusive {
    $missingPath = Join-Path ([System.IO.Path]::GetTempPath()) "pr-perf-missing-$([guid]::NewGuid()).json"
    $malformedPath = Join-Path ([System.IO.Path]::GetTempPath()) "pr-perf-malformed-$([guid]::NewGuid()).json"
    try {
        '{not-json' | Set-Content -LiteralPath $malformedPath -Encoding UTF8
        foreach ($thresholdPath in @($missingPath, $malformedPath)) {
            $comparison = Compare-PRPerfFiles `
                -TargetPath (Join-Path $root 'fixtures\target-pass.json') `
                -TrialPath (Join-Path $root 'fixtures\trial-pass.json') `
                -ThresholdPath $thresholdPath
            Assert-Inconclusive $comparison 'Missing or malformed threshold files must be inconclusive.'
        }
    } finally {
        if (Test-Path -LiteralPath $malformedPath) {
            Remove-Item -LiteralPath $malformedPath -Force
        }
    }
}

function Test-InvalidSamplesAreInconclusive {
    foreach ($invalidSample in @(-1.0, [double]::NaN, [double]::PositiveInfinity)) {
        $samples = @(100.0, 100.0, 100.0, 100.0, 100.0, 100.0, $invalidSample)
        $comparison = Compare-PRPerfResults `
            -Target (New-TestResult) `
            -Trial (New-TestResult -Samples $samples) `
            -Thresholds (Get-TestThresholds)
        Assert-Equal 'Inconclusive' $comparison.overallState "Invalid sample '$invalidSample' must be inconclusive."
    }
}

function Test-NonArrayTargetSamplesAreInconclusive {
    $sampleList = [System.Collections.Generic.List[double]]::new()
    $sampleList.AddRange([double[]]@(100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0))
    $sampleQueue = [System.Collections.Generic.Queue[double]]::new(
        [double[]]@(100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0))

    foreach ($sampleCase in @(
        [pscustomobject]@{ Name = 'scalar'; Value = 100.0 },
        [pscustomobject]@{ Name = 'List[double]'; Value = $sampleList },
        [pscustomobject]@{ Name = 'Queue[double]'; Value = $sampleQueue }
    )) {
        $target = New-TestResult
        $target.scenarios[0].metrics[0].samples = $sampleCase.Value
        $comparison = Compare-PRPerfResults -Target $target -Trial (New-TestResult) -Thresholds (Get-TestThresholds)
        Assert-Inconclusive $comparison "Non-array target samples ($($sampleCase.Name)) must be inconclusive."
    }
}

function Test-NonArrayTrialSamplesAreInconclusive {
    $sampleList = [System.Collections.Generic.List[double]]::new()
    $sampleList.AddRange([double[]]@(100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0))
    $sampleQueue = [System.Collections.Generic.Queue[double]]::new(
        [double[]]@(100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0))

    foreach ($sampleCase in @(
        [pscustomobject]@{ Name = 'scalar'; Value = 100.0 },
        [pscustomobject]@{ Name = 'List[double]'; Value = $sampleList },
        [pscustomobject]@{ Name = 'Queue[double]'; Value = $sampleQueue }
    )) {
        $trial = New-TestResult
        $trial.scenarios[0].metrics[0].samples = $sampleCase.Value
        $comparison = Compare-PRPerfResults -Target (New-TestResult) -Trial $trial -Thresholds (Get-TestThresholds)
        Assert-Inconclusive $comparison "Non-array trial samples ($($sampleCase.Name)) must be inconclusive."
    }
}

function Test-ExplicitFailedDataIsInconclusive {
    $trial = New-TestResult
    $trial.scenarios[0].metrics[0] | Add-Member -NotePropertyName failed -NotePropertyValue $true
    $comparison = Compare-PRPerfResults -Target (New-TestResult) -Trial $trial -Thresholds (Get-TestThresholds)
    Assert-Equal 'Inconclusive' $comparison.overallState 'Explicit failed data must be inconclusive.'

    $trial = New-TestResult
    $trial.scenarios[0].metrics[0] | Add-Member -NotePropertyName status -NotePropertyValue 'Failed'
    $comparison = Compare-PRPerfResults -Target (New-TestResult) -Trial $trial -Thresholds (Get-TestThresholds)
    Assert-Equal 'Inconclusive' $comparison.overallState 'Failed status data must be inconclusive.'
}

function Test-ThresholdConfigMismatchIsInconclusive {
    $thresholds = Get-TestThresholds
    $thresholds.benchmarkConfigVersion = 'different-config'
    $comparison = Compare-PRPerfResults -Target (New-TestResult) -Trial (New-TestResult) -Thresholds $thresholds
    Assert-Equal 'Inconclusive' $comparison.overallState 'Threshold config mismatch must be inconclusive.'
}

function Test-ResultMetadataMismatchIsInconclusive {
    foreach ($mismatch in @('schemaVersion', 'benchmarkConfigVersion', 'agentName', 'osBuild', 'architecture', 'flavor')) {
        $trial = New-TestResult
        switch ($mismatch) {
            'schemaVersion' { $trial.schemaVersion = 2 }
            'benchmarkConfigVersion' { $trial.benchmarkConfigVersion = 'different-config' }
            'agentName' { $trial.machine.agentName = 'PERF-02' }
            'osBuild' { $trial.machine.osBuild = '26200' }
            'architecture' { $trial.machine.architecture = 'arm64' }
            'flavor' { $trial.machine.flavor = 'chk' }
        }
        $comparison = Compare-PRPerfResults -Target (New-TestResult) -Trial $trial -Thresholds (Get-TestThresholds)
        Assert-Equal 'Inconclusive' $comparison.overallState "$mismatch mismatch must be inconclusive."
    }
}

function Test-SchemaStringFieldsRequireActualNonblankStrings {
    $fieldCases = @(
        @{ Path = 'benchmarkConfigVersion'; Values = @($null, '', '   ', 42, $true, [pscustomobject]@{ value = 'pr-smoke-v1' }) },
        @{ Path = 'machine.agentName'; Values = @($null, '', '   ', 42, $true, [pscustomobject]@{ value = 'PERF-01' }) },
        @{ Path = 'machine.osBuild'; Values = @($null, '', '   ', 26100, $true, [pscustomobject]@{ value = '26100' }) },
        @{ Path = 'machine.architecture'; Values = @($null, '', '   ', 64, $true, [pscustomobject]@{ value = 'amd64' }) },
        @{ Path = 'machine.flavor'; Values = @($null, '', '   ', 1, $true, [pscustomobject]@{ value = 'fre' }) },
        @{ Path = 'scenario.name'; Values = @($null, '', '   ', 42, $true, [pscustomobject]@{ value = 'ObjectCreation.BasicControls' }) },
        @{ Path = 'metric.name'; Values = @($null, '', '   ', 42, $true, [pscustomobject]@{ value = 'CpuTimeMs' }) },
        @{ Path = 'metric.unit'; Values = @($null, '', '   ', 1, $true, [pscustomobject]@{ value = 'ms' }) }
    )

    foreach ($fieldCase in $fieldCases) {
        foreach ($value in $fieldCase.Values) {
            $target = New-TestResult
            $trial = New-TestResult
            switch ($fieldCase.Path) {
                'benchmarkConfigVersion' {
                    $target.benchmarkConfigVersion = $value
                    $trial.benchmarkConfigVersion = $value
                    $thresholds = Get-TestThresholds
                    $thresholds.benchmarkConfigVersion = $value
                }
                'machine.agentName' {
                    $target.machine.agentName = $value
                    $trial.machine.agentName = $value
                    $thresholds = Get-TestThresholds
                }
                'machine.osBuild' {
                    $target.machine.osBuild = $value
                    $trial.machine.osBuild = $value
                    $thresholds = Get-TestThresholds
                }
                'machine.architecture' {
                    $target.machine.architecture = $value
                    $trial.machine.architecture = $value
                    $thresholds = Get-TestThresholds
                }
                'machine.flavor' {
                    $target.machine.flavor = $value
                    $trial.machine.flavor = $value
                    $thresholds = Get-TestThresholds
                }
                'scenario.name' {
                    $target.scenarios[0].name = $value
                    $trial.scenarios[0].name = $value
                    $thresholds = Get-TestThresholds
                }
                'metric.name' {
                    $target.scenarios[0].metrics[0].name = $value
                    $trial.scenarios[0].metrics[0].name = $value
                    $thresholds = Get-TestThresholds
                }
                'metric.unit' {
                    $target.scenarios[0].metrics[0].unit = $value
                    $trial.scenarios[0].metrics[0].unit = $value
                    $thresholds = Get-TestThresholds
                }
            }

            $comparison = Compare-PRPerfResults -Target $target -Trial $trial -Thresholds $thresholds
            Assert-Inconclusive $comparison "Malformed $($fieldCase.Path) value '$value' must be inconclusive."
        }
    }
}

function Test-CommitAndBuildIdentifiersRequireValidStrings {
    $identifierCases = @(
        @{ Name = 'commit'; Values = @($null, '', '   ', 42, $true, [pscustomobject]@{ value = 'aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa' }, 'not-a-commit', 'gggggggggggggggggggggggggggggggggggggggg') },
        @{ Name = 'buildId'; Values = @($null, '', '   ', 1001, $true, [pscustomobject]@{ value = '1001' }) }
    )

    foreach ($identifierCase in $identifierCases) {
        foreach ($value in $identifierCase.Values) {
            $target = New-TestResult
            $trial = New-TestResult
            $target.($identifierCase.Name) = $value
            $trial.($identifierCase.Name) = $value
            $comparison = Compare-PRPerfResults -Target $target -Trial $trial -Thresholds (Get-TestThresholds)
            Assert-Inconclusive $comparison "Malformed $($identifierCase.Name) value '$value' must be inconclusive."
        }
    }
}

function Test-AsymmetricScenarioSetsAreInconclusive {
    $trial = New-TestResult
    $trial.scenarios += (New-TestResult -ScenarioName 'Extra.Scenario').scenarios[0]
    $comparison = Compare-PRPerfResults -Target (New-TestResult) -Trial $trial -Thresholds (Get-TestThresholds)
    Assert-Equal 'Inconclusive' $comparison.overallState 'Extra trial scenarios must be inconclusive.'

    $trial = New-TestResult
    $trial.scenarios = @()
    $comparison = Compare-PRPerfResults -Target (New-TestResult) -Trial $trial -Thresholds (Get-TestThresholds)
    Assert-Equal 'Inconclusive' $comparison.overallState 'Missing trial scenarios must be inconclusive.'

    $trial = New-TestResult
    $trial.scenarios += $trial.scenarios[0]
    $comparison = Compare-PRPerfResults -Target (New-TestResult) -Trial $trial -Thresholds (Get-TestThresholds)
    Assert-Equal 'Inconclusive' $comparison.overallState 'Duplicated trial scenarios must be inconclusive.'

    $target = New-TestResult
    $target.scenarios += $target.scenarios[0]
    $comparison = Compare-PRPerfResults -Target $target -Trial (New-TestResult) -Thresholds (Get-TestThresholds)
    Assert-Equal 'Inconclusive' $comparison.overallState 'Duplicated target scenarios must be inconclusive.'
}

function Test-ScalarTargetScenariosAreInconclusive {
    $target = New-TestResult
    $target.scenarios = $target.scenarios[0]
    $comparison = Compare-PRPerfResults -Target $target -Trial (New-TestResult) -Thresholds (Get-TestThresholds)
    Assert-Inconclusive $comparison 'Scalar target scenarios must be inconclusive.'
}

function Test-ScalarTrialScenariosAreInconclusive {
    $trial = New-TestResult
    $trial.scenarios = $trial.scenarios[0]
    $comparison = Compare-PRPerfResults -Target (New-TestResult) -Trial $trial -Thresholds (Get-TestThresholds)
    Assert-Inconclusive $comparison 'Scalar trial scenarios must be inconclusive.'
}

function Test-AsymmetricMetricSetsAndUnitsAreInconclusive {
    $trial = New-TestResult
    $trial.scenarios[0].metrics += (New-TestResult -MetricName 'ExtraMetric').scenarios[0].metrics[0]
    $comparison = Compare-PRPerfResults -Target (New-TestResult) -Trial $trial -Thresholds (Get-TestThresholds)
    Assert-Equal 'Inconclusive' $comparison.overallState 'Extra trial metrics must be inconclusive.'

    $trial = New-TestResult
    $trial.scenarios[0].metrics = @()
    $comparison = Compare-PRPerfResults -Target (New-TestResult) -Trial $trial -Thresholds (Get-TestThresholds)
    Assert-Equal 'Inconclusive' $comparison.overallState 'Missing trial metrics must be inconclusive.'

    $trial = New-TestResult
    $trial.scenarios[0].metrics += $trial.scenarios[0].metrics[0]
    $comparison = Compare-PRPerfResults -Target (New-TestResult) -Trial $trial -Thresholds (Get-TestThresholds)
    Assert-Equal 'Inconclusive' $comparison.overallState 'Duplicated trial metrics must be inconclusive.'

    $target = New-TestResult
    $target.scenarios[0].metrics += $target.scenarios[0].metrics[0]
    $comparison = Compare-PRPerfResults -Target $target -Trial (New-TestResult) -Thresholds (Get-TestThresholds)
    Assert-Equal 'Inconclusive' $comparison.overallState 'Duplicated target metrics must be inconclusive.'

    $comparison = Compare-PRPerfResults `
        -Target (New-TestResult) `
        -Trial (New-TestResult -Unit 'ticks') `
        -Thresholds (Get-TestThresholds)
    Assert-Equal 'Inconclusive' $comparison.overallState 'Metric unit mismatch must be inconclusive.'
}

function Test-ScalarTargetMetricsAreInconclusive {
    $target = New-TestResult
    $target.scenarios[0].metrics = $target.scenarios[0].metrics[0]
    $comparison = Compare-PRPerfResults -Target $target -Trial (New-TestResult) -Thresholds (Get-TestThresholds)
    Assert-Inconclusive $comparison 'Scalar target metrics must be inconclusive.'
}

function Test-ScalarTrialMetricsAreInconclusive {
    $trial = New-TestResult
    $trial.scenarios[0].metrics = $trial.scenarios[0].metrics[0]
    $comparison = Compare-PRPerfResults -Target (New-TestResult) -Trial $trial -Thresholds (Get-TestThresholds)
    Assert-Inconclusive $comparison 'Scalar trial metrics must be inconclusive.'
}

function Test-CLIWritesComparisonJson {
    $outputDirectory = Join-Path ([System.IO.Path]::GetTempPath()) "pr-perf-$([guid]::NewGuid())"
    try {
        $scriptPath = Join-Path $root 'Compare-PRPerfResults.ps1'
        & powershell -NoProfile -ExecutionPolicy Bypass -File $scriptPath `
            -TargetPath (Join-Path $root 'fixtures\target-pass.json') `
            -TrialPath (Join-Path $root 'fixtures\trial-pass.json') `
            -ThresholdPath (Join-Path $root 'pr-perf-thresholds.json') `
            -OutputDirectory $outputDirectory
        if ($LASTEXITCODE -ne 0) {
            throw "CLI exited with code $LASTEXITCODE."
        }

        $comparisonPath = Join-Path $outputDirectory 'comparison.json'
        if (-not (Test-Path -LiteralPath $comparisonPath -PathType Leaf)) {
            throw "CLI did not create '$comparisonPath'."
        }

        $comparison = Get-Content -LiteralPath $comparisonPath -Raw | ConvertFrom-Json
        Assert-Equal 'Passed' $comparison.overallState 'CLI comparison state mismatch.'
    } finally {
        if (Test-Path -LiteralPath $outputDirectory) {
            Remove-Item -LiteralPath $outputDirectory -Recurse -Force
        }
    }
}

function Test-CLIWritesInconclusiveComparisonJson {
    $outputDirectory = Join-Path ([System.IO.Path]::GetTempPath()) "pr-perf-$([guid]::NewGuid())"
    $trialPath = Join-Path ([System.IO.Path]::GetTempPath()) "pr-perf-trial-$([guid]::NewGuid()).json"
    try {
        $trial = Get-Content -LiteralPath (Join-Path $root 'fixtures\trial-pass.json') -Raw | ConvertFrom-Json
        $trial.machine.architecture = 'arm64'
        $trial | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath $trialPath -Encoding UTF8

        $scriptPath = Join-Path $root 'Compare-PRPerfResults.ps1'
        & powershell -NoProfile -ExecutionPolicy Bypass -File $scriptPath `
            -TargetPath (Join-Path $root 'fixtures\target-pass.json') `
            -TrialPath $trialPath `
            -ThresholdPath (Join-Path $root 'pr-perf-thresholds.json') `
            -OutputDirectory $outputDirectory
        if ($LASTEXITCODE -ne 0) {
            throw "CLI exited with code $LASTEXITCODE."
        }

        $comparisonPath = Join-Path $outputDirectory 'comparison.json'
        $comparison = Get-Content -LiteralPath $comparisonPath -Raw | ConvertFrom-Json
        Assert-Equal 'Inconclusive' $comparison.overallState 'CLI must serialize an inconclusive comparison.'
    } finally {
        if (Test-Path -LiteralPath $outputDirectory) {
            Remove-Item -LiteralPath $outputDirectory -Recurse -Force
        }
        if (Test-Path -LiteralPath $trialPath) {
            Remove-Item -LiteralPath $trialPath -Force
        }
    }
}

function Test-CLIWritesInconclusiveComparisonJsonForMalformedInput {
    $outputDirectory = Join-Path ([System.IO.Path]::GetTempPath()) "pr-perf-$([guid]::NewGuid())"
    $trialPath = Join-Path ([System.IO.Path]::GetTempPath()) "pr-perf-trial-$([guid]::NewGuid()).json"
    try {
        '{not-json' | Set-Content -LiteralPath $trialPath -Encoding UTF8

        $scriptPath = Join-Path $root 'Compare-PRPerfResults.ps1'
        & powershell -NoProfile -ExecutionPolicy Bypass -File $scriptPath `
            -TargetPath (Join-Path $root 'fixtures\target-pass.json') `
            -TrialPath $trialPath `
            -ThresholdPath (Join-Path $root 'pr-perf-thresholds.json') `
            -OutputDirectory $outputDirectory
        if ($LASTEXITCODE -ne 0) {
            throw "CLI exited with code $LASTEXITCODE."
        }

        $comparisonPath = Join-Path $outputDirectory 'comparison.json'
        if (-not (Test-Path -LiteralPath $comparisonPath -PathType Leaf)) {
            throw "CLI did not create '$comparisonPath'."
        }

        $comparison = Get-Content -LiteralPath $comparisonPath -Raw | ConvertFrom-Json
        Assert-Inconclusive $comparison 'Malformed CLI input must serialize an inconclusive comparison.'
    } finally {
        if (Test-Path -LiteralPath $outputDirectory) {
            Remove-Item -LiteralPath $outputDirectory -Recurse -Force
        }
        if (Test-Path -LiteralPath $trialPath) {
            Remove-Item -LiteralPath $trialPath -Force
        }
    }
}

function Invoke-CompareCli([hashtable] $Extra, [string] $TrialFile = 'fixtures\trial-pass.json') {
    $outputDirectory = Join-Path ([System.IO.Path]::GetTempPath()) "pr-perf-$([guid]::NewGuid())"
    try {
        $scriptPath = Join-Path $root 'Compare-PRPerfResults.ps1'
        $args = @(
            '-TargetPath', (Join-Path $root 'fixtures\target-pass.json'),
            '-TrialPath', (Join-Path $root $TrialFile),
            '-ThresholdPath', (Join-Path $root 'pr-perf-thresholds.json'),
            '-OutputDirectory', $outputDirectory
        )
        foreach ($k in $Extra.Keys) { $args += @("-$k", $Extra[$k]) }
        & powershell -NoProfile -ExecutionPolicy Bypass -File $scriptPath @args | Out-Null
        return Get-Content -LiteralPath (Join-Path $outputDirectory 'comparison.json') -Raw | ConvertFrom-Json
    } finally {
        if (Test-Path -LiteralPath $outputDirectory) { Remove-Item -LiteralPath $outputDirectory -Recurse -Force }
    }
}

function Test-CLIRejectsResultsMeasuredAtADifferentSourceCommit {
    # A result file left over from an earlier run, or fixture data, must never be reported as a
    # clean pass for the commit the run was actually requested for.
    $comparison = Invoke-CompareCli @{
        SourceCommit = ('c' * 40)
        TargetCommit = ('a' * 40)
    }
    Assert-Equal 'Inconclusive' $comparison.overallState 'A trial measured at another commit must be inconclusive.'
}

function Test-CLIRejectsResultsMeasuredAtADifferentTargetCommit {
    $comparison = Invoke-CompareCli @{
        SourceCommit = ('b' * 40)
        TargetCommit = ('d' * 40)
    }
    Assert-Equal 'Inconclusive' $comparison.overallState 'A target measured at another commit must be inconclusive.'
}

function Test-CLIRejectsComparisonOfACommitAgainstItself {
    # Comparing a commit with itself always passes and says nothing about the change.
    $comparison = Invoke-CompareCli @{
        SourceCommit = ('a' * 40)
        TargetCommit = ('a' * 40)
    }
    Assert-Equal 'Inconclusive' $comparison.overallState 'Comparing a commit against itself must be inconclusive.'
}

function Test-CLIStillPassesWhenRequestedCommitsMatchTheMeasuredResults {
    $comparison = Invoke-CompareCli @{
        SourceCommit = ('b' * 40)
        TargetCommit = ('a' * 40)
    }
    Assert-Equal 'Passed' $comparison.overallState 'Matching commits must still be able to pass.'
}

function Test-CLIPassesWhenNoCommitsAreSupplied {
    $comparison = Invoke-CompareCli @{}
    Assert-Equal 'Passed' $comparison.overallState 'Omitting commit context must not change the verdict.'
}

function Test-ProvenanceInconclusiveStillReportsTheMeasuredNumbers {
    # A provenance problem (such as both sides being measured at the same commit)
    # invalidates the verdict, not the measurements. Discarding the scenarios left
    # a reader with an empty table and no way to see what was actually measured,
    # which makes a real run indistinguishable from one that collected nothing.
    $dir = Join-Path $PSScriptRoot "provenance-$([guid]::NewGuid())"
    New-Item -ItemType Directory -Path $dir -Force | Out-Null
    try {
        $targetPath = Join-Path $dir 'target.json'
        $trialPath = Join-Path $dir 'trial.json'
        $thresholdPath = Join-Path $dir 'thresholds.json'
        New-TestResult | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath $targetPath -Encoding UTF8
        New-TestResult -Samples @(101.0, 101.0, 101.0, 101.0, 101.0, 101.0, 101.0) |
            ConvertTo-Json -Depth 12 | Set-Content -LiteralPath $trialPath -Encoding UTF8
        Get-TestThresholds | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath $thresholdPath -Encoding UTF8

        $comparison = Compare-PRPerfFiles `
            -TargetPath $targetPath -TrialPath $trialPath -ThresholdPath $thresholdPath

        Assert-Equal 'Inconclusive' $comparison.overallState 'Same-commit comparison must stay inconclusive.'
        if (-not (@($comparison.issues) -match 'both measured at commit')) {
            throw 'The same-commit reason must still be reported.'
        }
        if (@($comparison.scenarios).Count -eq 0) {
            throw 'The measured scenarios must still be reported so the numbers are visible.'
        }
        $metric = $comparison.scenarios[0].metrics[0]
        Assert-Equal 100.0 $metric.target.Median 'Target median must be reported.'
        Assert-Equal 101.0 $metric.trial.Median 'Trial median must be reported.'
        if ($metric.classification -eq 'Passed') {
            throw 'A metric inside an inconclusive report must never read as Passed.'
        }
    } finally {
        Remove-Item -LiteralPath $dir -Recurse -Force -ErrorAction SilentlyContinue
    }
}

function Test-LocalResultCarriesEverySampleAndItsProvenance {
    $result = New-PRPerfLocalResult `
        -Commit ('c' * 40) -BuildId '4242' -AgentName 'LAB-07' `
        -ScenarioName 'Startup.LoadMicrosoftUiXaml' `
        -Samples @(8.1, 8.4, 8.2, 8.6, 8.3, 8.5, 8.0)

    Assert-Equal ('c' * 40) $result.commit 'Result must record the measured commit.'
    Assert-Equal '4242' $result.buildId 'Result must record the build.'
    Assert-Equal 'LAB-07' $result.machine.agentName 'Result must record the agent that measured it.'
    Assert-Equal 'Startup.LoadMicrosoftUiXaml' $result.scenarios[0].name 'Scenario name mismatch.'
    Assert-Equal 'WallTimeMs' $result.scenarios[0].metrics[0].name 'The metric must be named for what was measured.'
    Assert-Equal 7 @($result.scenarios[0].metrics[0].samples).Count 'Every sample must be reported.'
}

function Test-LocalResultRefusesToInventSamples {
    # Emitting a result with no samples would let a run that measured nothing flow
    # into the comparer as though it were data.
    $failed = $false
    try {
        New-PRPerfLocalResult -Commit ('c' * 40) -BuildId '1' -AgentName 'LAB-07' `
            -ScenarioName 'Startup.LoadMicrosoftUiXaml' -Samples @()
    } catch {
        $failed = $true
        if ($_.Exception -is [System.Management.Automation.CommandNotFoundException]) {
            throw 'New-PRPerfLocalResult must exist.'
        }
    }
    if (-not $failed) { throw 'A result with no samples must not be produced.' }
}

function Test-LocalResultPassesTheSchemaTheComparerEnforces {
    $result = New-PRPerfLocalResult `
        -Commit ('c' * 40) -BuildId '4242' -AgentName 'LAB-07' `
        -ScenarioName 'Startup.LoadMicrosoftUiXaml' `
        -Samples @(8.1, 8.4, 8.2, 8.6, 8.3, 8.5, 8.0)
    $roundTripped = $result | ConvertTo-Json -Depth 12 | ConvertFrom-Json

    try {
        Assert-PRPerfResultSchema -Result $roundTripped
    } catch {
        throw "Locally measured results must satisfy the comparer schema: $($_.Exception.Message)"
    }
}

function Test-MeasureRefusesAMissingBinary {
    # Measuring a binary that is not there must fail loudly rather than quietly
    # producing a result the comparer would treat as real data.
    $out = Join-Path $PSScriptRoot "measure-$([guid]::NewGuid()).json"
    $failed = $false
    try {
        & (Join-Path $root 'Measure-PRPerfLocalScenario.ps1') `
            -BinaryPath (Join-Path $PSScriptRoot 'no-such-binary.dll') `
            -Commit ('c' * 40) -BuildId '1' -AgentName 'LAB-07' -OutputPath $out
    } catch {
        $failed = $true
        if ($_.Exception -is [System.Management.Automation.CommandNotFoundException]) {
            throw 'Measure-PRPerfLocalScenario.ps1 must exist.'
        }
    }
    try {
        if (-not $failed) { throw 'Measuring a missing binary must fail.' }
        if (Test-Path -LiteralPath $out) { throw 'No result file may be written when measurement fails.' }
    } finally {
        Remove-Item -LiteralPath $out -Force -ErrorAction SilentlyContinue
    }
}

function Test-MeasureProducesSevenRealSamplesForARealBinary {
    $out = Join-Path $PSScriptRoot "measure-$([guid]::NewGuid()).json"
    try {
        & (Join-Path $root 'Measure-PRPerfLocalScenario.ps1') `
            -BinaryPath 'C:\Windows\System32\shlwapi.dll' `
            -Commit ('c' * 40) -BuildId '99' -AgentName 'LAB-07' -OutputPath $out | Out-Null

        if (-not (Test-Path -LiteralPath $out)) { throw 'A result file must be written.' }
        $result = Get-Content -LiteralPath $out -Raw | ConvertFrom-Json
        Assert-PRPerfResultSchema -Result $result
        $samples = @($result.scenarios[0].metrics[0].samples)
        Assert-Equal 7 $samples.Count 'Exactly seven samples are required.'
        foreach ($sample in $samples) {
            if ($sample -le 0) { throw "A measured sample must be positive. Got '$sample'." }
        }
        # Distinct timings prove these were measured rather than copied from one reading.
        if (@($samples | Select-Object -Unique).Count -lt 2) {
            throw 'Samples that are all identical are not real measurements.'
        }
    } finally {
        Remove-Item -LiteralPath $out -Force -ErrorAction SilentlyContinue
    }
}

function New-PRPerfBinaryTree {
    param([string[]] $RelativePaths)
    $dir = Join-Path $PSScriptRoot "bin-$([guid]::NewGuid())"
    foreach ($relative in $RelativePaths) {
        $full = Join-Path $dir $relative
        New-Item -ItemType Directory -Path (Split-Path -Parent $full) -Force | Out-Null
        Set-Content -LiteralPath $full -Value 'x' -Encoding ASCII
    }
    return $dir
}

function Test-MeasurementBinarySelectionPrefersTheWinUICore {
    $dir = New-PRPerfBinaryTree @('a\Microsoft.UI.Xaml.dll', 'b\Microsoft.UI.Xaml.Controls.dll')
    try {
        $selected = Select-PRPerfMeasurementBinary -Root $dir
        Assert-Equal 'Microsoft.UI.Xaml.dll' (Split-Path -Leaf $selected) 'The WinUI core binary must be preferred.'
    } finally { Remove-Item -LiteralPath $dir -Recurse -Force -ErrorAction SilentlyContinue }
}

function Test-MeasurementBinarySelectionIsStableAcrossDuplicates {
    # Two agents measuring different copies of the same name would compare different
    # files without saying so, so selection must not depend on enumeration order.
    $dir = New-PRPerfBinaryTree @('z\Microsoft.UI.Xaml.dll', 'a\Microsoft.UI.Xaml.dll')
    try {
        $first = Select-PRPerfMeasurementBinary -Root $dir
        $second = Select-PRPerfMeasurementBinary -Root $dir
        Assert-Equal $first $second 'Selection must be deterministic.'
        if ($first -notmatch '\\a\\') { throw "Selection must be the lexicographically first path. Got '$first'." }
    } finally { Remove-Item -LiteralPath $dir -Recurse -Force -ErrorAction SilentlyContinue }
}

function Test-MeasurementBinarySelectionFallsBackThroughCandidates {
    $dir = New-PRPerfBinaryTree @('b\Microsoft.UI.Xaml.Controls.dll')
    try {
        $selected = Select-PRPerfMeasurementBinary -Root $dir
        Assert-Equal 'Microsoft.UI.Xaml.Controls.dll' (Split-Path -Leaf $selected) 'Selection must fall back to the next candidate.'
    } finally { Remove-Item -LiteralPath $dir -Recurse -Force -ErrorAction SilentlyContinue }
}

function Test-MeasurementBinarySelectionExplainsWhatItFound {
    # An empty or unexpected drop must say what was there, because the alternative is
    # a bare "not found" against a tree nobody can inspect after the agent is gone.
    $dir = New-PRPerfBinaryTree @('b\Something.Else.dll')
    try {
        $message = $null
        try { Select-PRPerfMeasurementBinary -Root $dir } catch { $message = $_.Exception.Message }
        if ($null -eq $message) { throw 'Selection must fail when no candidate is present.' }
        if ($message -notmatch 'Something\.Else\.dll') {
            throw "The failure must report what was actually present. Got: $message"
        }
    } finally { Remove-Item -LiteralPath $dir -Recurse -Force -ErrorAction SilentlyContinue }
}

function Test-LocalComparisonWithoutABaselineStillMeasuresBothSides {
    # With no baseline the run must still produce both result files, because measuring
    # one side only leaves an empty table that reads exactly like a run that collected
    # nothing. The comparer is what refuses to draw a conclusion from it.
    $dir = Join-Path $PSScriptRoot "local-$([guid]::NewGuid())"
    $trialRoot = Join-Path $dir 'trial\drop'
    $out = Join-Path $dir 'out'
    New-Item -ItemType Directory -Path $trialRoot -Force | Out-Null
    Copy-Item 'C:\Windows\System32\shlwapi.dll' (Join-Path $trialRoot 'Microsoft.UI.Xaml.dll')
    try {
        & (Join-Path $root 'Invoke-PRPerfLocalComparison.ps1') `
            -TrialRoot $trialRoot -TargetRoot (Join-Path $dir 'no-baseline') `
            -TrialCommit ('a' * 40) -TrialBuildId '500' `
            -AgentName 'LAB-07' -OutputDirectory $out `
            -ThresholdPath (Join-Path $root 'pr-perf-thresholds-local.json') | Out-Null

        foreach ($name in @('pr-results.json', 'target-results.json')) {
            $path = Join-Path $out $name
            if (-not (Test-Path -LiteralPath $path)) { throw "$name must be written." }
            Assert-PRPerfResultSchema -Result (Get-Content -LiteralPath $path -Raw | ConvertFrom-Json)
        }

        $comparison = Compare-PRPerfFiles `
            -TargetPath (Join-Path $out 'target-results.json') `
            -TrialPath (Join-Path $out 'pr-results.json') `
            -ThresholdPath (Join-Path $root 'pr-perf-thresholds-local.json')

        Assert-Equal 'Inconclusive' $comparison.overallState 'A run with no baseline must not conclude anything.'
        if (@($comparison.scenarios).Count -eq 0) {
            throw 'The measured numbers must still be reported when there is no baseline.'
        }
        if (@($comparison.scenarios[0].metrics[0].target.Median) -le 0) {
            throw 'The reported median must be a real measurement.'
        }
    } finally {
        Remove-Item -LiteralPath $dir -Recurse -Force -ErrorAction SilentlyContinue
    }
}
