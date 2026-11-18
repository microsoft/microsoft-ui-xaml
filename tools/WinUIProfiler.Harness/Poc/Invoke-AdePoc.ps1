# Stages a bounded sample and forwards governed fix operations to ADE.
#Requires -Version 7
[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [ValidateSet('Prepare', 'Propose', 'Approve', 'Run', 'Status', 'Verify', 'Report')]
    [string]$Action,
    [string]$StatePath,
    [string]$ProposalSha256,
    [ValidateSet('Synthetic', 'Control')]
    [string]$Scenario = 'Synthetic',
    [string]$ProfilerCli,
    [string]$DotNet,
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
    }
    Save-State $state
    Write-Output "POC-STATE: $StatePath"
    return
}

if (-not $StatePath) {
    throw "$Action requires -StatePath from a successful Prepare."
}
$StatePath = (Resolve-Path -LiteralPath $StatePath -ErrorAction Stop).Path
$state = Get-Content -LiteralPath $StatePath -Raw | ConvertFrom-Json
if ($state.Schema -ne 'winui-profiler-ade-poc/state-v1') {
    throw 'The state file is not a supported POC state.'
}

if ($Action -eq 'Report') {
    if (-not $state.Report) {
        throw 'This POC does not have an ADE proposal or report yet.'
    }
    Get-Content -LiteralPath $state.Report -Raw
    return
}

if ($Action -eq 'Propose') {
    if ($state.RunId) {
        throw 'This state already has an ADE run. Prepare a new POC for another proposal.'
    }
    $isControl = $state.PSObject.Properties['Scenario'] -and $state.Scenario -eq 'Control'
    $task = if ($isControl) {
        'Reduce redundant layout work in ScenarioWorkload.cs. Edit only that file. Preserve the complete ordered tree of 40 Buttons and the protected caller''s final UpdateLayout. The protected ten-iteration WinUIProfiler layout-event gate must pass and the extractor tests must remain passing. Report actual evidence paths and distinguish this deliberately constructed app inefficiency from a framework defect or a statistical latency claim.'
    } else {
        'Fix the repeated label-normalization work in Catalog. Edit only workload.py. Preserve all existing matching behavior. The protected work-count check must pass, and the correctness tests must remain passing. This is a synthetic ADE engine demonstration, not a WinUI performance claim.'
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
} else {
    if (-not $state.RunId) {
        throw "$Action requires a successful Propose first."
    }
    $arguments = @($Action.ToLowerInvariant(), '--run', $state.RunId)
    if ($Action -eq 'Approve') {
        if (-not $ProposalSha256 -or $ProposalSha256 -cne $state.ProposalSha256) {
            throw 'Approve requires the exact -ProposalSha256 shown in the proposal. Obtain human approval first.'
        }
        $arguments += @('--hash', $ProposalSha256)
    }
}

$response = Invoke-AdeCommand -Arguments $arguments
$state.LastResult = $response.Result
if ($response.ExitCode -eq 0 -and $Action -eq 'Propose') {
    $state.RunId = $response.Result.runId
    $state.ProposalSha256 = $response.Result.proposalSha256
}
$reportProperty = $response.Result.PSObject.Properties['report']
if ($reportProperty -and $reportProperty.Value) {
    $state.Report = $reportProperty.Value
}
Save-State $state
Write-Output "POC-STATE: $StatePath"
exit $response.ExitCode
