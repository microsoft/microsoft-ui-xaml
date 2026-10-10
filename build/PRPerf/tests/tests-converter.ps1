$root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$converter = Join-Path $root 'Convert-WinUIPerfResults.ps1'
$validator = Join-Path $root 'Assert-PRPerfArtifacts.ps1'

function Assert-Equal($Expected, $Actual, [string] $Message) {
    if ($Expected -ne $Actual) {
        throw "$Message Expected='$Expected' Actual='$Actual'"
    }
}

function Assert-Throws([scriptblock] $Action, [string] $Pattern, [string] $Message) {
    try {
        & $Action
    } catch {
        if ($_.Exception.Message -notmatch $Pattern) {
            throw "$Message Wrong error: $($_.Exception.Message)"
        }
        return
    }
    throw "$Message Expected an exception."
}

function Get-TestScratchDirectory {
    # Tests used to write their inputs and outputs next to themselves, which left
    # byproducts in the repository and made the working tree dirty just by running
    # the suite. Everything transient goes to a scratch directory instead.
    if (-not $script:TestScratchDirectory) {
        $script:TestScratchDirectory = Join-Path ([System.IO.Path]::GetTempPath()) "prperf-tests-$([guid]::NewGuid())"
        New-Item -ItemType Directory -Path $script:TestScratchDirectory -Force | Out-Null
    }
    return $script:TestScratchDirectory
}

function New-TestCsv([string] $Name, [string[]] $Lines) {
    $path = Join-Path (Get-TestScratchDirectory) $Name
    $Lines | Set-Content -LiteralPath $path -Encoding UTF8
    return $path
}

function Invoke-TestConversion([string[]] $InputCsv, [string] $Name = 'converted.json', [string] $ScenarioPattern) {
    $output = Join-Path (Get-TestScratchDirectory) $Name
    $extra = @{}
    if ($ScenarioPattern) { $extra['ScenarioPattern'] = $ScenarioPattern }
    & $converter `
        -InputCsv $InputCsv `
        -Commit ('a' * 40) `
        -BuildId '1001' `
        -AgentName 'PERF-01' `
        -OutputPath $output `
        @extra
    return Get-Content -LiteralPath $output -Raw | ConvertFrom-Json
}

function New-TestArtifactPair(
    [switch] $OmitTrialPackage,
    [switch] $MismatchedLayout,
    [switch] $PutTargetPackageAtArbitraryPath
) {
    $base = Join-Path (Get-TestScratchDirectory) "artifacts-$([guid]::NewGuid())"
    $target = Join-Path $base 'target'
    $trial = Join-Path $base 'trial'
    $relativeApps = 'drop_amd64fre\Test\perf\apps'
    $package = 'XAMLPerf.ButtonApp.Cpp.MUX_1.0.0.0_x64_Test'
    $targetRelativeApps = if ($PutTargetPackageAtArbitraryPath) {
        'drop_amd64fre\arbitrary\nested\apps'
    } else {
        $relativeApps
    }
    $targetPackage = Join-Path (Join-Path $target $targetRelativeApps) $package
    $trialPackage = Join-Path (Join-Path $trial $relativeApps) $package
    New-Item -ItemType Directory -Path $targetPackage -Force | Out-Null
    'target' | Set-Content -LiteralPath (Join-Path $targetPackage 'XAMLPerf.ButtonApp.Cpp.MUX_1.0.0.0_x64.appx')
    if (-not $OmitTrialPackage) {
        New-Item -ItemType Directory -Path $trialPackage -Force | Out-Null
        'trial' | Set-Content -LiteralPath (Join-Path $trialPackage 'XAMLPerf.ButtonApp.Cpp.MUX_1.0.0.0_x64.appx')
        if ($MismatchedLayout) {
            'extra' | Set-Content -LiteralPath (Join-Path $trialPackage 'unexpected.txt')
        }
    } else {
        New-Item -ItemType Directory -Path (Join-Path $trial $relativeApps) -Force | Out-Null
    }
    return [pscustomobject]@{ Base = $base; Target = $target; Trial = $trial }
}

function Test-RawConverterDiscardsLaunchZeroAndEmitsSevenSamples {
    $output = Join-Path $PSScriptRoot 'converted-target.json'
    try {
        $result = Invoke-TestConversion `
            -InputCsv (Join-Path $root 'fixtures\winui-perf.target.raw.csv') `
            -Name 'converted-target.json'
        Assert-Equal 1 $result.schemaVersion 'Schema version mismatch.'
        Assert-Equal 'pr-smoke-v1' $result.benchmarkConfigVersion 'Config version mismatch.'
        Assert-Equal ('a' * 40) $result.commit 'Commit must come from arguments.'
        Assert-Equal '1001' $result.buildId 'Build ID must come from arguments.'
        Assert-Equal 'PERF-01' $result.machine.agentName 'Agent name must come from arguments.'
        Assert-Equal 'amd64' $result.machine.architecture 'Architecture mismatch.'
        Assert-Equal 'fre' $result.machine.flavor 'Flavor mismatch.'
        Assert-Equal 'Lifecycle-MinApp.Cpp.MUX' $result.scenarios[0].name 'Scenario name mismatch.'
        Assert-Equal 'CpuTimeMs' $result.scenarios[0].metrics[0].name 'Metric name mismatch.'
        Assert-Equal 'ms' $result.scenarios[0].metrics[0].unit 'Metric unit mismatch.'
        Assert-Equal 7 $result.scenarios[0].metrics[0].samples.Count 'Measured sample count mismatch.'
        Assert-Equal 250.0 $result.scenarios[0].metrics[0].samples[0] 'Run 1 must be first.'
        Assert-Equal 251.5 $result.scenarios[0].metrics[0].samples[6] 'Run 7 must be last.'
    } finally {
        Remove-Item -LiteralPath $output -Force -ErrorAction SilentlyContinue
    }
}

function Test-RawConverterAcceptsNumericRunWhenItAgreesWithGrouping {
    $csv = New-TestCsv 'numeric-run.csv' @(
        'scenario,metric,value,interval,grouping,run'
        'PRPerf-ObjectCreation,CPU/WallTime,999,Measured,Run:0,0'
        'PRPerf-ObjectCreation,CPU/WallTime,1,Measured,Run:1,1'
        'PRPerf-ObjectCreation,CPU/WallTime,2,Measured,Run:2,2'
        'PRPerf-ObjectCreation,CPU/WallTime,3,Measured,Run:3,3'
        'PRPerf-ObjectCreation,CPU/WallTime,4,Measured,Run:4,4'
        'PRPerf-ObjectCreation,CPU/WallTime,5,Measured,Run:5,5'
        'PRPerf-ObjectCreation,CPU/WallTime,6,Measured,Run:6,6'
        'PRPerf-ObjectCreation,CPU/WallTime,7,Measured,Run:7,7'
    )
    $output = Join-Path $PSScriptRoot 'numeric-run.json'
    try {
        $result = Invoke-TestConversion -InputCsv $csv -Name 'numeric-run.json'
        Assert-Equal 7 $result.scenarios[0].metrics[0].samples.Count 'Numeric run sample count mismatch.'
    } finally {
        Remove-Item -LiteralPath $csv, $output -Force -ErrorAction SilentlyContinue
    }
}

function Test-RawConverterRejectsRunDisagreement {
    $csv = New-TestCsv 'run-disagreement.csv' @(
        'scenario,metric,value,interval,grouping,run'
        'PRPerf-ObjectCreation,CPU/WallTime,1,Measured,Run:0,1'
    )
    try {
        Assert-Throws { Invoke-TestConversion -InputCsv $csv -Name 'run-disagreement.json' } 'does not agree' 'Run disagreement must fail.'
    } finally {
        Remove-Item -LiteralPath $csv, (Join-Path $PSScriptRoot 'run-disagreement.json') -Force -ErrorAction SilentlyContinue
    }
}

function Test-RawConverterRejectsUnknownOrMissingColumns {
    $unknown = New-TestCsv 'unknown-column.csv' @(
        'scenario,metric,value,interval,grouping,statistic'
        'PRPerf-ObjectCreation,CPU/WallTime,1,Measured,Run:0,fAvg'
    )
    $missing = New-TestCsv 'missing-column.csv' @(
        'scenario,metric,value,interval'
        'PRPerf-ObjectCreation,CPU/WallTime,1,Measured'
    )
    try {
        Assert-Throws { Invoke-TestConversion -InputCsv $unknown -Name 'unknown-column.json' } 'Unsupported CSV column' 'Aggregate/statistic columns must fail.'
        Assert-Throws { Invoke-TestConversion -InputCsv $missing -Name 'missing-column.json' } 'Missing required CSV column' 'Missing grouping must fail.'
    } finally {
        Remove-Item -LiteralPath $unknown, $missing, (Join-Path $PSScriptRoot 'unknown-column.json'), (Join-Path $PSScriptRoot 'missing-column.json') -Force -ErrorAction SilentlyContinue
    }
}

function Test-RawConverterRequiresRunsOneThroughSevenExactlyOnce {
    $duplicate = New-TestCsv 'duplicate-run.csv' @(
        'scenario,metric,value,interval,grouping'
        'PRPerf-ObjectCreation,CPU/WallTime,0,Measured,Run:0'
        'PRPerf-ObjectCreation,CPU/WallTime,1,Measured,Run:1'
        'PRPerf-ObjectCreation,CPU/WallTime,2,Measured,Run:2'
        'PRPerf-ObjectCreation,CPU/WallTime,3,Measured,Run:3'
        'PRPerf-ObjectCreation,CPU/WallTime,4,Measured,Run:4'
        'PRPerf-ObjectCreation,CPU/WallTime,5,Measured,Run:5'
        'PRPerf-ObjectCreation,CPU/WallTime,6,Measured,Run:6'
        'PRPerf-ObjectCreation,CPU/WallTime,7,Measured,Run:6'
    )
    $missing = New-TestCsv 'missing-run.csv' @(
        'scenario,metric,value,interval,grouping'
        'PRPerf-ObjectCreation,CPU/WallTime,0,Measured,Run:0'
        'PRPerf-ObjectCreation,CPU/WallTime,1,Measured,Run:1'
        'PRPerf-ObjectCreation,CPU/WallTime,2,Measured,Run:2'
        'PRPerf-ObjectCreation,CPU/WallTime,3,Measured,Run:3'
        'PRPerf-ObjectCreation,CPU/WallTime,4,Measured,Run:4'
        'PRPerf-ObjectCreation,CPU/WallTime,5,Measured,Run:5'
        'PRPerf-ObjectCreation,CPU/WallTime,6,Measured,Run:6'
    )
    try {
        Assert-Throws { Invoke-TestConversion -InputCsv $duplicate -Name 'duplicate-run.json' } 'duplicate.*6|Run 6.*duplicate' 'Duplicate run must fail.'
        Assert-Throws { Invoke-TestConversion -InputCsv $missing -Name 'missing-run.json' } 'missing required Run 7' 'Missing run must fail.'
    } finally {
        Remove-Item -LiteralPath $duplicate, $missing, (Join-Path $PSScriptRoot 'duplicate-run.json'), (Join-Path $PSScriptRoot 'missing-run.json') -Force -ErrorAction SilentlyContinue
    }
}

function Test-RawConverterRejectsNonfiniteAndNegativeValues {
    foreach ($case in @(
        @{ Name = 'nan'; Value = 'NaN'; Pattern = 'finite' },
        @{ Name = 'negative'; Value = '-1'; Pattern = 'negative' }
    )) {
        $lines = @('scenario,metric,value,interval,grouping')
        foreach ($run in 0..7) {
            $value = if ($run -eq 4) { $case.Value } else { [string]$run }
            $lines += "PRPerf-ObjectCreation,CPU/WallTime,$value,Measured,Run:$run"
        }
        $csv = New-TestCsv "$($case.Name)-value.csv" $lines
        try {
            Assert-Throws { Invoke-TestConversion -InputCsv $csv -Name "$($case.Name)-value.json" } $case.Pattern "$($case.Name) value must fail."
        } finally {
            Remove-Item -LiteralPath $csv, (Join-Path $PSScriptRoot "$($case.Name)-value.json") -Force -ErrorAction SilentlyContinue
        }
    }
}

function Test-RawConverterFiltersMixedFilesAndUnrelatedCpuRows {
    $mixed = New-TestCsv 'mixed-relevant.csv' @(
        'scenario,metric,value,interval,grouping'
        'OtherScenario,CPU/WallTime,100,Measured,Run:1'
        'PRPerf-ObjectCreation,GPU/Time,200,Measured,Run:1'
        'PRPerf-ObjectCreation,CPU/WallTime,999,Total,Run:1'
        'PRPerf-ObjectCreation,CPU/WallTime,0,Measured,Run:0'
        'PRPerf-ObjectCreation,CPU/WallTime,1,Measured,Run:1'
        'PRPerf-ObjectCreation,CPU/WallTime,2,Measured,Run:2'
        'PRPerf-ObjectCreation,CPU/WallTime,3,Measured,Run:3'
        'PRPerf-ObjectCreation,CPU/WallTime,4,Measured,Run:4'
        'PRPerf-ObjectCreation,CPU/WallTime,5,Measured,Run:5'
        'PRPerf-ObjectCreation,CPU/WallTime,6,Measured,Run:6'
        'PRPerf-ObjectCreation,CPU/WallTime,7,Measured,Run:7'
    )
    $unrelated = New-TestCsv 'unrelated.csv' @(
        'scenario,metric,value,interval,grouping,diagnostic'
        'OtherScenario,CPU/WallTime,123,Measured,Run:1,ignore-me'
    )
    $output = Join-Path $PSScriptRoot 'mixed-files.json'
    try {
        $result = Invoke-TestConversion -InputCsv @($unrelated, $mixed) -Name 'mixed-files.json' -ScenarioPattern '^PRPerf-'
        Assert-Equal 1 $result.scenarios.Count 'Only the scenario matching the requested pattern must be emitted.'
        Assert-Equal 7 $result.scenarios[0].metrics[0].samples.Count 'Seven matching runs must be emitted.'
        Assert-Equal 1.0 $result.scenarios[0].metrics[0].samples[0] 'Unrelated CPU rows must be ignored.'
    } finally {
        Remove-Item -LiteralPath $mixed, $unrelated, $output -Force -ErrorAction SilentlyContinue
    }
}

function Test-RawConverterRejectsAmbiguousMatchingIntervalsAcrossFiles {
    $measured = New-TestCsv 'measured-interval.csv' @(
        'scenario,metric,value,interval,grouping'
        'PRPerf-ObjectCreation,CPU/WallTime,0,Measured,Run:0'
        'PRPerf-ObjectCreation,CPU/WallTime,1,Measured,Run:1'
        'PRPerf-ObjectCreation,CPU/WallTime,2,Measured,Run:2'
        'PRPerf-ObjectCreation,CPU/WallTime,3,Measured,Run:3'
        'PRPerf-ObjectCreation,CPU/WallTime,4,Measured,Run:4'
        'PRPerf-ObjectCreation,CPU/WallTime,5,Measured,Run:5'
        'PRPerf-ObjectCreation,CPU/WallTime,6,Measured,Run:6'
        'PRPerf-ObjectCreation,CPU/WallTime,7,Measured,Run:7'
    )
    $alternate = New-TestCsv 'alternate-interval.csv' @(
        'scenario,metric,value,interval,grouping'
        'PRPerf-ObjectCreation,CPU/WallTime,0,Alternate,Run:0'
        'PRPerf-ObjectCreation,CPU/WallTime,1,Alternate,Run:1'
        'PRPerf-ObjectCreation,CPU/WallTime,2,Alternate,Run:2'
        'PRPerf-ObjectCreation,CPU/WallTime,3,Alternate,Run:3'
        'PRPerf-ObjectCreation,CPU/WallTime,4,Alternate,Run:4'
        'PRPerf-ObjectCreation,CPU/WallTime,5,Alternate,Run:5'
        'PRPerf-ObjectCreation,CPU/WallTime,6,Alternate,Run:6'
        'PRPerf-ObjectCreation,CPU/WallTime,7,Alternate,Run:7'
    )
    try {
        Assert-Throws {
            Invoke-TestConversion -InputCsv @($measured, $alternate) -Name 'ambiguous-interval.json'
        } 'ambiguous|measurement groups|exactly one' 'Multiple matching intervals for one scenario must fail explicitly.'
    } finally {
        Remove-Item -LiteralPath $measured, $alternate, (Join-Path $PSScriptRoot 'ambiguous-interval.json') -Force -ErrorAction SilentlyContinue
    }
}

function Test-ArtifactValidatorAcceptsMatchingSmokeAppLayouts {
    $pair = New-TestArtifactPair
    try {
        $summary = & $validator -TargetRoot $pair.Target -TrialRoot $pair.Trial -ExpectedArtifactName 'drop_amd64fre' | ConvertFrom-Json
        Assert-Equal $true $summary.compatible 'Matching artifacts must be compatible.'
        Assert-Equal 'Test\perf\apps' $summary.appsRelativePath 'Apps subtree mismatch.'
    } finally {
        Remove-Item -LiteralPath $pair.Base -Recurse -Force -ErrorAction SilentlyContinue
    }
}

function Test-ArtifactValidatorRejectsPackageAtArbitraryRecursivePath {
    $pair = New-TestArtifactPair -PutTargetPackageAtArbitraryPath
    try {
        Assert-Throws {
            & $validator -TargetRoot $pair.Target -TrialRoot $pair.Trial -ExpectedArtifactName 'drop_amd64fre'
        } 'expected relative artifact layout|required.*Test\\perf\\apps|missing' 'A matching package at an arbitrary recursive path must fail.'
    } finally {
        Remove-Item -LiteralPath $pair.Base -Recurse -Force -ErrorAction SilentlyContinue
    }
}

function Test-ArtifactValidatorRejectsMissingPackageAndLayoutMismatch {
    $missing = New-TestArtifactPair -OmitTrialPackage
    $mismatch = New-TestArtifactPair -MismatchedLayout
    try {
        Assert-Throws { & $validator -TargetRoot $missing.Target -TrialRoot $missing.Trial -ExpectedArtifactName 'drop_amd64fre' } 'required.*ButtonApp|ButtonApp.*missing' 'Missing app package must fail.'
        Assert-Throws { & $validator -TargetRoot $mismatch.Target -TrialRoot $mismatch.Trial -ExpectedArtifactName 'drop_amd64fre' } 'relative file sets|unexpected.txt' 'Mismatched app layout must fail.'
    } finally {
        Remove-Item -LiteralPath $missing.Base, $mismatch.Base -Recurse -Force -ErrorAction SilentlyContinue
    }
}

function Test-ArtifactValidatorRejectsUnsanitizedOrWrongArchitectureArtifactNames {
    $pair = New-TestArtifactPair
    try {
        Assert-Throws { & $validator -TargetRoot $pair.Target -TrialRoot $pair.Trial -ExpectedArtifactName '..\drop_amd64fre' } 'safe artifact name' 'Unsanitized artifact name must fail.'
        Assert-Throws { & $validator -TargetRoot $pair.Target -TrialRoot $pair.Trial -ExpectedArtifactName 'drop_x86fre' } 'x64|amd64' 'Wrong architecture must fail.'
    } finally {
        Remove-Item -LiteralPath $pair.Base -Recurse -Force -ErrorAction SilentlyContinue
    }
}

function Test-RunYamlPreservesFixturesAndRunsRealTargetThenTrial {
    $yamlPath = Join-Path (Split-Path -Parent $root) 'AzurePipelinesTemplates\WinUI-PRPerf-Run.yml'
    $yaml = Get-Content -LiteralPath $yamlPath -Raw
    foreach ($required in @(
        'name: ${{ parameters.perfPoolName }}',
        'targetBuildId',
        'sourceBuildId',
        'drop_amd64fre',
        'Assert-PRPerfArtifacts.ps1',
        'pipeline-run.ps1',
        'pr-smoke/cpu-pr',
        '$(Build.ArtifactStagingDirectory)\current',
        'Convert-WinUIPerfResults.ps1',
        'target-results.json',
        'pr-results.json',
        'Copy PR perf fixtures',
        'condition: always()'
    )) {
        if ($yaml -notlike "*$required*") {
            throw "Run YAML is missing '$required'."
        }
    }
    if ($yaml -match '-InputCsv\s+[^\r\n]*shift\.agg\.csv') {
        throw 'Run YAML must consume selected *.raw.csv, not shift.agg.csv.'
    }
    if ($yaml -match 'Select (target|trial) raw performance CSV' -or $yaml -match '(target|trial)\.raw\.csv') {
        throw 'Run YAML must pass the raw source glob to the converter instead of copying mixed CSV files.'
    }
    $rawSourceUses = @([regex]::Matches($yaml, '-InputCsv\s+"?\$\(Build\.ArtifactStagingDirectory\)\\current"?')).Count
    if ($rawSourceUses -ne 2) {
        throw "Run YAML must pass the raw source directory to both converter calls; found $rawSourceUses uses."
    }
    $targetRun = $yaml.IndexOf('target-results.json', [System.StringComparison]::Ordinal)
    $trialRun = $yaml.IndexOf('pr-results.json', [System.StringComparison]::Ordinal)
    if ($targetRun -lt 0 -or $trialRun -le $targetRun) {
        throw 'Target conversion must appear before trial conversion in the same job.'
    }
}

function Test-RawConverterAcceptsRealHarnessScenarioName {
    # perf\profiles\scenarios.json names the PR smoke scenario Lifecycle-MinApp.Cpp.MUX.
    # Nothing in the real harness emits a PRPerf- prefix.
    $csv = New-TestCsv 'real-name.raw.csv' @(
        'scenario,metric,value,interval,grouping'
        'Lifecycle-MinApp.Cpp.MUX,CPU/WallTime,999,Measured,Run:0'
        'Lifecycle-MinApp.Cpp.MUX,CPU/WallTime,250,Measured,Run:1'
        'Lifecycle-MinApp.Cpp.MUX,CPU/WallTime,252,Measured,Run:2'
        'Lifecycle-MinApp.Cpp.MUX,CPU/WallTime,251,Measured,Run:3'
        'Lifecycle-MinApp.Cpp.MUX,CPU/WallTime,249,Measured,Run:4'
        'Lifecycle-MinApp.Cpp.MUX,CPU/WallTime,253,Measured,Run:5'
        'Lifecycle-MinApp.Cpp.MUX,CPU/WallTime,250.5,Measured,Run:6'
        'Lifecycle-MinApp.Cpp.MUX,CPU/WallTime,251.5,Measured,Run:7'
    )
    try {
        $result = Invoke-TestConversion -InputCsv @($csv)
        Assert-Equal 'Lifecycle-MinApp.Cpp.MUX' $result.scenarios[0].name 'Real harness scenario name must be preserved.'
        Assert-Equal 7 $result.scenarios[0].metrics[0].samples.Count 'Real harness rows must yield seven samples.'
    } finally {
        Remove-Item -LiteralPath $csv -Force -ErrorAction SilentlyContinue
    }
}

function Test-RawConverterToleratesRealHarnessColumns {
    # perf\vis\reports.py works over arch/version/shift alongside the value columns,
    # so real raw CSVs carry them and the converter must not reject the file.
    $csv = New-TestCsv 'real-columns.raw.csv' @(
        'shift,arch,version,scenario,metric,value,interval,grouping'
        '0,amd64,1.0.0,Lifecycle-MinApp.Cpp.MUX,CPU/WallTime,999,Measured,Run:0'
        '0,amd64,1.0.0,Lifecycle-MinApp.Cpp.MUX,CPU/WallTime,250,Measured,Run:1'
        '0,amd64,1.0.0,Lifecycle-MinApp.Cpp.MUX,CPU/WallTime,252,Measured,Run:2'
        '0,amd64,1.0.0,Lifecycle-MinApp.Cpp.MUX,CPU/WallTime,251,Measured,Run:3'
        '0,amd64,1.0.0,Lifecycle-MinApp.Cpp.MUX,CPU/WallTime,249,Measured,Run:4'
        '0,amd64,1.0.0,Lifecycle-MinApp.Cpp.MUX,CPU/WallTime,253,Measured,Run:5'
        '0,amd64,1.0.0,Lifecycle-MinApp.Cpp.MUX,CPU/WallTime,250.5,Measured,Run:6'
        '0,amd64,1.0.0,Lifecycle-MinApp.Cpp.MUX,CPU/WallTime,251.5,Measured,Run:7'
    )
    try {
        $result = Invoke-TestConversion -InputCsv @($csv)
        Assert-Equal 7 $result.scenarios[0].metrics[0].samples.Count 'Real harness columns must not be rejected.'
    } finally {
        Remove-Item -LiteralPath $csv -Force -ErrorAction SilentlyContinue
    }
}

function Test-RawConverterStillRejectsGenuinelyUnknownColumns {
    $csv = New-TestCsv 'bogus-column.raw.csv' @(
        'scenario,metric,value,interval,grouping,bogus'
        'Lifecycle-MinApp.Cpp.MUX,CPU/WallTime,250,Measured,Run:1,x'
    )
    try {
        $threw = $false
        try { Invoke-TestConversion -InputCsv @($csv) } catch { $threw = $true }
        Assert-Equal $true $threw 'An unrecognized column must still be rejected.'
    } finally {
        Remove-Item -LiteralPath $csv -Force -ErrorAction SilentlyContinue
    }
}


function Test-RawConverterScopesToRequestedRealScenarioOnly {
    # A real run set emits every scenario sharing the selected tag, so the pipeline must be
    # able to narrow the comparison to the exact scenario it intends to measure.
    $csv = New-TestCsv 'real-multi.raw.csv' @(
        'scenario,metric,value,interval,grouping'
        'Lifecycle-MinApp.Cpp.MUX,CPU/WallTime,250,Measured,Run:1'
        'Lifecycle-MinApp.Cpp.MUX,CPU/WallTime,252,Measured,Run:2'
        'Lifecycle-MinApp.Cpp.MUX,CPU/WallTime,251,Measured,Run:3'
        'Lifecycle-MinApp.Cpp.MUX,CPU/WallTime,249,Measured,Run:4'
        'Lifecycle-MinApp.Cpp.MUX,CPU/WallTime,253,Measured,Run:5'
        'Lifecycle-MinApp.Cpp.MUX,CPU/WallTime,250.5,Measured,Run:6'
        'Lifecycle-MinApp.Cpp.MUX,CPU/WallTime,251.5,Measured,Run:7'
        'Lifecycle-MinApp.Cs.WUX,CPU/WallTime,900,Measured,Run:1'
    )
    try {
        $result = Invoke-TestConversion -InputCsv @($csv) -Name 'real-multi.json' -ScenarioPattern '^Lifecycle-MinApp\.Cpp\.MUX$'
        Assert-Equal 1 $result.scenarios.Count 'Sibling scenarios must be excluded.'
        Assert-Equal 'Lifecycle-MinApp.Cpp.MUX' $result.scenarios[0].name 'Wrong scenario selected.'
    } finally {
        Remove-Item -LiteralPath $csv -Force -ErrorAction SilentlyContinue
        Remove-Item -LiteralPath (Join-Path $PSScriptRoot 'real-multi.json') -Force -ErrorAction SilentlyContinue
    }
}


function Test-RawConverterRejectsMultipleShiftsInOneFile {
    # perf\vis\reports.py separates baseline from trial with data_frame['shift'] == shifts[0]
    # vs shifts[1]. A file holding two shifts is a baseline+trial pair, and blending both into
    # one sample series would silently compare a commit against itself.
    $csv = New-TestCsv 'multi-shift.raw.csv' @(
        'shift,arch,version,scenario,metric,value,interval,grouping'
        '0,amd64,1.0.0,Lifecycle-MinApp.Cpp.MUX,CPU/WallTime,250,Measured,Run:1'
        '1,amd64,2.0.0,Lifecycle-MinApp.Cpp.MUX,CPU/WallTime,900,Measured,Run:1'
    )
    try {
        Assert-Throws { Invoke-TestConversion -InputCsv @($csv) -Name 'multi-shift.json' } 'shift' 'Two shifts in one file must be rejected.'
    } finally {
        Remove-Item -LiteralPath $csv -Force -ErrorAction SilentlyContinue
        Remove-Item -LiteralPath (Join-Path $PSScriptRoot 'multi-shift.json') -Force -ErrorAction SilentlyContinue
    }
}

function Test-RawConverterRejectsMultipleArchitecturesInOneFile {
    # reports.py merges on arch, so a frame can legitimately span architectures. Averaging
    # amd64 and x86 timings together would be meaningless.
    $csv = New-TestCsv 'multi-arch.raw.csv' @(
        'shift,arch,version,scenario,metric,value,interval,grouping'
        '0,amd64,1.0.0,Lifecycle-MinApp.Cpp.MUX,CPU/WallTime,250,Measured,Run:1'
        '0,x86,1.0.0,Lifecycle-MinApp.Cpp.MUX,CPU/WallTime,900,Measured,Run:1'
    )
    try {
        Assert-Throws { Invoke-TestConversion -InputCsv @($csv) -Name 'multi-arch.json' } 'arch' 'Two architectures in one file must be rejected.'
    } finally {
        Remove-Item -LiteralPath $csv -Force -ErrorAction SilentlyContinue
        Remove-Item -LiteralPath (Join-Path $PSScriptRoot 'multi-arch.json') -Force -ErrorAction SilentlyContinue
    }
}

function Test-RawConverterRejectsBlankScenarioPattern {
    # -cmatch '' matches every row, which would defeat explicit scenario selection.
    $csv = New-TestCsv 'blank-pattern.raw.csv' @(
        'scenario,metric,value,interval,grouping'
        'Lifecycle-MinApp.Cpp.MUX,CPU/WallTime,250,Measured,Run:1'
    )
    try {
        Assert-Throws { Invoke-TestConversion -InputCsv @($csv) -Name 'blank-pattern.json' -ScenarioPattern '   ' } 'ScenarioPattern' 'A blank scenario pattern must be rejected.'
    } finally {
        Remove-Item -LiteralPath $csv -Force -ErrorAction SilentlyContinue
        Remove-Item -LiteralPath (Join-Path $PSScriptRoot 'blank-pattern.json') -Force -ErrorAction SilentlyContinue
    }
}
