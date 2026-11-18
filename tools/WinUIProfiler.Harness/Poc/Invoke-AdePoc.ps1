# Stages a bounded sample and forwards governed fix operations to ADE.
#Requires -Version 7
[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [ValidateSet('Prepare', 'Investigate', 'Show', 'Evidence', 'Propose', 'Approve', 'Run', 'Status', 'Verify', 'Report')]
    [string]$Action,
    [string]$StatePath,
    [string]$ProposalSha256,
    [ValidateSet('Synthetic', 'Control')]
    [string]$Scenario = 'Synthetic',
    [string]$ProfilerCli,
    [string]$DotNet,
    [ValidateSet('Fix', 'Investigation')]
    [string]$Phase = 'Fix',
    [string]$Problem = 'Investigate the cost of constructing the control tree in this scenario. Identify evidence-backed hypotheses or request additional measurements.',
    [string]$CandidateManifest,
    [ValidateSet('Baseline', 'Candidate')]
    [string]$Variant = 'Baseline',
    [ValidateRange(1, 300)][int]$DisplaySeconds = 30,
    [string]$ArtifactsRoot = (Join-Path $env:LOCALAPPDATA 'WinUIProfilerHarness\Poc')
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$PSNativeCommandUseErrorActionPreference = $false
$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..\..'))
$pluginRoot = Join-Path $repoRoot 'eng\ade\ui-profiler'
$launcher = Join-Path $pluginRoot 'scripts\ade.ps1'

function Invoke-AdeCommand {
    param([string[]]$Arguments)

    $lines = @()
    & $launcher @Arguments | Tee-Object -Variable lines | ForEach-Object { Write-Host $_ }
    $exitCode = $LASTEXITCODE
    $results = @($lines | Where-Object { $_ -is [string] -and $_.StartsWith('ADE-RESULT: ') })
    if ($results.Count -eq 0) {
        throw "ADE returned exit code $exitCode without an ADE-RESULT record."
    }
    return [pscustomobject]@{
        ExitCode = $exitCode
        Result = $results[-1].Substring('ADE-RESULT: '.Length) | ConvertFrom-Json
    }
}

function Save-State {
    param($State)

    $temporaryPath = "$StatePath.tmp"
    [IO.File]::WriteAllText($temporaryPath, ($State | ConvertTo-Json -Depth 20))
    Move-Item -LiteralPath $temporaryPath -Destination $StatePath -Force
}

if ($Action -eq 'Prepare') {
    if ($StatePath) {
        throw 'Prepare creates a new state file; do not supply -StatePath.'
    }
    $artifacts = [IO.Path]::GetFullPath($ArtifactsRoot)
    if ($artifacts.TrimEnd('\') -eq $repoRoot.TrimEnd('\') -or
        $artifacts.StartsWith("$($repoRoot.TrimEnd('\'))\", [StringComparison]::OrdinalIgnoreCase)) {
        throw 'ArtifactsRoot must be outside the WinUI source repository.'
    }
    Get-Command python, git -ErrorAction Stop | Out-Null
    $pack = if ($Scenario -eq 'Control') { Join-Path $pluginRoot 'control-pack' } else { Join-Path $pluginRoot 'pack' }
    $doctor = Invoke-AdeCommand -Arguments @('doctor', '--pack', $pack)
    if ($doctor.ExitCode -ne 0) {
        exit $doctor.ExitCode
    }

    $name = '{0}-{1}' -f [DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss'), [guid]::NewGuid().ToString('N')
    $sessionRoot = Join-Path $artifacts $name
    $sampleRoot = Join-Path $sessionRoot 'sample'
    if ($Scenario -eq 'Control') {
        Get-Command dotnet, logman -ErrorAction Stop | Out-Null
        $sourceSample = Join-Path $PSScriptRoot 'ControlDemo'
        if (-not $ProfilerCli) {
            $ProfilerCli = Join-Path $repoRoot 'tools\WinUIProfiler\WinUIProfiler.Cli\bin\Release\net10.0\WinUIProfiler.Cli.exe'
        }
        $ProfilerCli = (Resolve-Path -LiteralPath $ProfilerCli -ErrorAction Stop).Path
        if (-not $DotNet) {
            $DotNet = Join-Path $env:ProgramFiles 'dotnet\dotnet.exe'
        }
        $DotNet = (Resolve-Path -LiteralPath $DotNet -ErrorAction Stop).Path
        $files = @(
            '.gitignore', 'README.md', 'App.xaml', 'App.xaml.cs', 'ScenarioWorkload.cs',
            'ControlDemo.csproj', 'Directory.Build.props', 'Directory.Build.targets',
            'app.manifest', 'global.json', 'NuGet.Config', 'packages.lock.json',
            'markers.json', 'providers.txt', 'build.ps1', 'capture.ps1', 'metrics.py', 'verify.py',
            'profiler_query.py',
            'tests\__init__.py', 'tests\test_metrics.py'
        )
        $workload = 'ScenarioWorkload.cs'
    } else {
        $sourceSample = Join-Path $PSScriptRoot 'sample'
        $files = @(
            '.gitignore', 'README.md', 'workload.py', 'verify.py',
            'tests\__init__.py', 'tests\test_workload.py'
        )
        $workload = 'workload.py'
    }
    foreach ($relativePath in $files) {
        $destination = Join-Path $sampleRoot $relativePath
        New-Item -ItemType Directory -Path (Split-Path $destination -Parent) -Force | Out-Null
        Copy-Item -LiteralPath (Join-Path $sourceSample $relativePath) -Destination $destination
    }

    $baselinePath = Join-Path $sessionRoot 'baseline-measurement.json'
    if ($Scenario -eq 'Control') {
        $contract = [ordered]@{
            schema = 'winui-profiler-control-poc/contract-v1'
            profilerCli = $ProfilerCli
            profilerSha256 = (Get-FileHash -LiteralPath $ProfilerCli).Hash.ToLowerInvariant()
            initRun = Join-Path $repoRoot 'initrun.ps1'
            dotnet = $DotNet
            artifactsRoot = Join-Path $sessionRoot 'measurements'
            expectedControls = 40
            maxLayoutPasses = 4
        }
        [IO.File]::WriteAllText(
            (Join-Path $sampleRoot 'measurement-contract.json'),
            ($contract | ConvertTo-Json -Depth 10))
    }
    Push-Location $sampleRoot
    try {
        & python -B -m unittest discover -s tests -t .
        if ($LASTEXITCODE -ne 0) {
            throw 'The seed correctness tests failed; this is not the expected POC baseline.'
        }
        if ($Scenario -eq 'Control') {
            & python -B verify.py | Tee-Object -FilePath $baselinePath
        } else {
            & python -B verify.py --output $baselinePath
        }
        if ($LASTEXITCODE -ne 1) {
            throw 'The seed must fail the work-budget check with exit code 1.'
        }
    } finally {
        Pop-Location
    }
    $baseline = Get-Content -LiteralPath $baselinePath -Raw | ConvertFrom-Json
    if ($baseline.status -ne 'Failed' -or
        ($Scenario -eq 'Synthetic' -and $baseline.reason -ne 'WorkBudgetExceeded')) {
        throw 'The baseline failed for an unexpected reason.'
    }
    $StatePath = Join-Path $sessionRoot 'state.json'
    $state = [pscustomobject]@{
        Schema = 'winui-profiler-ade-poc/state-v1'
        Scenario = $Scenario
        PackRoot = $pack
        SampleRoot = $sampleRoot
        BaselineMeasurement = $baselinePath
        SourceWorkloadSha256 = (Get-FileHash -LiteralPath (Join-Path $sourceSample $workload)).Hash
        RunId = $null
        ProposalSha256 = $null
        Report = $null
        LastResult = $null
        Investigation = $null
    }
    Save-State $state
    Write-Output "POC-STATE: $StatePath"
    exit 0
}

if (-not $StatePath) {
    throw "$Action requires -StatePath from a successful Prepare."
}
$StatePath = (Resolve-Path -LiteralPath $StatePath -ErrorAction Stop).Path
$state = Get-Content -LiteralPath $StatePath -Raw | ConvertFrom-Json
if ($state.Schema -ne 'winui-profiler-ade-poc/state-v1') {
    throw 'The state file is not a supported POC state.'
}

$isControl = $state.PSObject.Properties['Scenario'] -and $state.Scenario -eq 'Control'
if ($Action -in 'Investigate', 'Show', 'Evidence' -and -not $isControl) {
    throw "$Action requires a Control state."
}
if ($Action -eq 'Evidence') {
    if (-not $CandidateManifest) {
        throw 'Evidence requires an explicit -CandidateManifest; it never selects the latest capture.'
    }
    $baseline = Get-Content -LiteralPath $state.BaselineMeasurement -Raw | ConvertFrom-Json
    $output = Join-Path (Split-Path $StatePath -Parent) ('comparison-' + [guid]::NewGuid().ToString('N') + '.md')
    & python -B (Join-Path $PSScriptRoot 'ControlDemo\metrics.py') `
        --baseline $baseline.captureManifest --candidate $CandidateManifest --output $output
    if ($LASTEXITCODE -eq 0) {
        Write-Output "POC-EVIDENCE: $output"
    }
    exit $LASTEXITCODE
}
if ($Action -eq 'Show') {
    $sourceRoot = $state.SampleRoot
    if ($Variant -eq 'Candidate') {
        if (-not $state.PSObject.Properties['FixRunDir'] -or -not $state.FixRunDir) {
            throw 'Showing the candidate requires a completed successful fix run from this adapter.'
        }
        $sourceRoot = Join-Path $state.FixRunDir 'workcopy'
    }
    $contract = Get-Content -LiteralPath (Join-Path $sourceRoot 'measurement-contract.json') -Raw | ConvertFrom-Json
    $output = Join-Path (Split-Path $StatePath -Parent) ('observation-' + [guid]::NewGuid().ToString('N'))
    Write-Host "Showing $Variant for $DisplaySeconds seconds after the scenario. This is not a ten-run acceptance check."
    & pwsh -NoProfile -File (Join-Path $sourceRoot 'capture.ps1') `
        -ProfilerCli $contract.profilerCli -InitRun $contract.initRun -DotNet $contract.dotnet `
        -RunDirectory $output -Iterations 1 -DisplaySeconds $DisplaySeconds
    exit $LASTEXITCODE
}

$active = $state
if ($Action -eq 'Investigate') {
    if ($state.PSObject.Properties['Investigation'] -and $state.Investigation -and $state.Investigation.RunId) {
        throw 'This state already has an investigation. Prepare a new POC for another investigation.'
    }
    $pack = Join-Path $pluginRoot 'investigation-pack'
    $doctor = Invoke-AdeCommand -Arguments @('doctor', '--pack', $pack)
    if ($doctor.ExitCode -ne 0) {
        exit $doctor.ExitCode
    }
    $baseline = Get-Content -LiteralPath $state.BaselineMeasurement -Raw | ConvertFrom-Json
    $manifestPath = $baseline.captureManifest
    $manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
    $inputRoot = Join-Path (Split-Path $StatePath -Parent) ('investigation-' + [guid]::NewGuid().ToString('N'))
    New-Item -ItemType Directory -Path $inputRoot | Out-Null
    foreach ($name in @('App.xaml.cs', 'ScenarioWorkload.cs')) {
        Copy-Item -LiteralPath (Join-Path $state.SampleRoot $name) -Destination (Join-Path $inputRoot $name)
    }
    foreach ($name in @('metrics.py', 'profiler_query.py')) {
        Copy-Item -LiteralPath (Join-Path $PSScriptRoot "ControlDemo\$name") -Destination (Join-Path $inputRoot $name)
    }
    $paths = @($manifestPath, $manifest.profilerCli)
    $paths += @('App.xaml.cs', 'ScenarioWorkload.cs') | ForEach-Object { Join-Path $state.SampleRoot $_ }
    foreach ($run in $manifest.iterations) {
        $paths += @($run.database, $run.appResult)
    }
    $inputs = @($paths | ForEach-Object {
        [ordered]@{ path = $_; sha256 = (Get-FileHash -LiteralPath $_).Hash.ToLowerInvariant() }
    })
    $contract = [ordered]@{
        schema = 'winui-profiler-control-poc/investigation-v1'
        problem = $Problem
        captureManifest = $manifestPath
        profilerCli = $manifest.profilerCli
        inputs = $inputs
    }
    [IO.File]::WriteAllText(
        (Join-Path $inputRoot 'investigation.json'), ($contract | ConvertTo-Json -Depth 10))
    $active = [pscustomobject]@{
        RunId = $null
        ProposalSha256 = $null
        Report = $null
        LastResult = $null
        Evidence = $null
    }
    $state | Add-Member -NotePropertyName Investigation -NotePropertyValue $active -Force
    $arguments = @(
        'propose', '--type', 'perf-investigate', '--repo', $inputRoot, '--pack', $pack,
        '--task', "$Problem In proposal planning, plan queries only; do not read app source or diagnose yet. In approved execution, start with the profiler queries, not app source. Follow the perf-investigate workflow. Do not apply a fix."
    )
} elseif ($Phase -eq 'Investigation') {
    if (-not $isControl -or -not $state.PSObject.Properties['Investigation'] -or -not $state.Investigation) {
        throw 'No investigation proposal exists. Run Investigate first.'
    }
    $active = $state.Investigation
}

if ($Action -eq 'Report') {
    if (-not $active.Report) {
        throw 'This POC does not have an ADE proposal or report yet.'
    }
    Get-Content -LiteralPath $active.Report -Raw
    if ($Phase -eq 'Investigation' -and $active.Evidence) {
        Get-Content -LiteralPath $active.Evidence.diagnosis -Raw
    }
    return
}

if ($Action -eq 'Propose') {
    if ($Phase -ne 'Fix') {
        throw 'Use Investigate to create the investigation proposal.'
    }
    if ($active.RunId) {
        throw 'This state already has an ADE run. Prepare a new POC for another proposal.'
    }
    $task = if ($isControl) {
        'Reduce redundant layout work in ScenarioWorkload.cs. Edit only that file. Preserve the complete ordered tree of 40 Buttons and the protected caller''s final UpdateLayout. The protected ten-iteration WinUIProfiler layout-event gate must pass and the extractor tests must remain passing. Report actual evidence paths and distinguish this deliberately constructed app inefficiency from a framework defect or a statistical latency claim.'
    } else {
        'Fix the repeated label-normalization work in Catalog. Edit only workload.py. Preserve all existing matching behavior. The protected work-count check must pass, and the correctness tests must remain passing. This is a synthetic ADE engine demonstration, not a WinUI performance claim.'
    }
    if ($isControl -and $state.PSObject.Properties['Investigation'] -and $state.Investigation) {
        $evidence = $state.Investigation.Evidence
        if (-not $evidence -or $evidence.decision.status -ne 'RecommendationReady') {
            throw 'The investigation must complete with verified evidence and a recommendation before proposing a fix.'
        }
        if ((Get-FileHash -LiteralPath $evidence.diagnosis).Hash.ToLowerInvariant() -ne $evidence.diagnosisSha256) {
            throw 'The investigation deliverable changed after verification.'
        }
        $queryRoot = Split-Path (Split-Path $evidence.diagnosis -Parent) -Parent
        $validation = & python -B (Join-Path $queryRoot 'profiler_query.py') verify
        if ($LASTEXITCODE -ne 0) {
            throw 'Investigation inputs or query evidence changed; do not propose a fix from stale evidence.'
        }
        $currentEvidence = ($validation -join [Environment]::NewLine) | ConvertFrom-Json
        if ($currentEvidence.decision.status -ne 'RecommendationReady' -or
            $currentEvidence.diagnosisSha256 -ne $evidence.diagnosisSha256) {
            throw 'The verified investigation no longer matches the recommendation.'
        }
        $task = 'Implement the evidence-backed recommendation below. Edit only ScenarioWorkload.cs. Preserve correctness and the protected measurement contract. Validate the hypothesis with the ten-run verifier; do not claim a confirmed root cause unless the controlled change supports it.' +
            [Environment]::NewLine + (Get-Content -LiteralPath $evidence.diagnosis -Raw)
    }
    $arguments = @(
        'propose',
        '--type', 'perf-fix',
        '--repo', $state.SampleRoot,
        '--task', $task,
        '--check', 'python -B verify.py'
    )
    if ($state.PSObject.Properties['PackRoot']) {
        $arguments += @('--pack', $state.PackRoot)
    }
} elseif ($Action -ne 'Investigate') {
    if (-not $active.RunId) {
        throw "$Action requires a successful Propose first."
    }
    $arguments = @($Action.ToLowerInvariant(), '--run', $active.RunId)
    if ($Action -eq 'Approve') {
        if (-not $ProposalSha256 -or $ProposalSha256 -cne $active.ProposalSha256) {
            throw 'Approve requires the exact -ProposalSha256 shown in the proposal. Obtain human approval first.'
        }
        $arguments += @('--hash', $ProposalSha256)
    }
}

$response = Invoke-AdeCommand -Arguments $arguments
$active.LastResult = $response.Result
if ($response.ExitCode -eq 0 -and $Action -in 'Propose', 'Investigate') {
    $active.RunId = $response.Result.runId
    $active.ProposalSha256 = $response.Result.proposalSha256
}
$reportProperty = $response.Result.PSObject.Properties['report']
if ($reportProperty -and $reportProperty.Value) {
    $active.Report = $reportProperty.Value
}
Save-State $state
if ($response.ExitCode -eq 0 -and $Action -eq 'Run' -and $Phase -eq 'Fix' -and
    $response.Result.stopReason -eq 'Passed') {
    $state | Add-Member -NotePropertyName FixRunDir -NotePropertyValue $response.Result.runDir -Force
    Save-State $state
}
if ($response.ExitCode -eq 0 -and $Action -eq 'Run' -and $Phase -eq 'Investigation') {
    $queryScript = Join-Path $response.Result.runDir 'workcopy\profiler_query.py'
    $validation = & python -B $queryScript verify
    if ($LASTEXITCODE -ne 0) {
        throw 'ADE completed, but profiler evidence verification failed. Do not use this diagnosis for a fix.'
    }
    $active.Evidence = ($validation -join [Environment]::NewLine) | ConvertFrom-Json
    Save-State $state
}
Write-Output "POC-STATE: $StatePath"
exit $response.ExitCode
