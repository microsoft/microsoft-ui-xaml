# Prepare a measured expert-backed ADE proposal, or run/verify an already approved proposal.
#Requires -Version 7
param(
    [ValidateSet('Prepare', 'Run', 'Verify')][string]$Phase = 'Prepare',
    [Parameter(Mandatory)][string]$Directory,
    [string]$ApplicationRepository,
    [string]$ScenarioPath,
    [string]$KnowledgeRepository,
    [string]$ExpertHome,
    [string]$ProfilerCli,
    [string]$Dotnet,
    [string]$AdeCopilotCli,
    [string]$GitHubAccount
)
$ErrorActionPreference = 'Stop'
$plugin = Split-Path $PSScriptRoot -Parent
Import-Module "$plugin\pack\tools\ProfilerHarness.psm1" -Force -DisableNameChecking
$Directory = [IO.Path]::GetFullPath($Directory)
$contextPath = Join-Path $Directory 'context.json'
$runs = Join-Path $Directory 'ade-runs'
function Invoke-AdeRuntime([string[]]$Arguments, $Context, [switch]$Probe) {
    $saved = @{}
    foreach ($name in @('COPILOT_CLI_PATH', 'COPILOT_CLI_RESOLVED_DIST_DIR', 'COPILOT_HOME',
        'COPILOT_AUTO_UPDATE', 'COPILOT_GITHUB_TOKEN')) {
        $saved[$name] = [Environment]::GetEnvironmentVariable($name)
    }
    try {
        if ($Context.adeCopilotCli) { $env:COPILOT_CLI_PATH = $Context.adeCopilotCli }
        $env:COPILOT_CLI_RESOLVED_DIST_DIR = $null
        $env:COPILOT_HOME = Join-Path $Directory 'ade-client'
        $env:COPILOT_AUTO_UPDATE = 'false'
        if ($GitHubAccount) {
            $token = gh auth token --hostname github.com --user $GitHubAccount
            if ($LASTEXITCODE -ne 0) { throw 'The selected existing GitHub credential is unavailable.' }
            $env:COPILOT_GITHUB_TOKEN = $token.Trim()
            $token = $null
        } elseif (!$env:COPILOT_GITHUB_TOKEN -and !$env:GH_TOKEN -and !$env:GITHUB_TOKEN) {
            throw 'Run the isolated ADE client with -GitHubAccount or an existing token in the environment.'
        }
        if ($Probe) {
            $version = (Read-JsonFile "$plugin\pack\ade.team.json").runtimeVersion
            $runtime = if ($env:ADE_RUNTIME_DIR) { $env:ADE_RUNTIME_DIR } else {
                Join-Path $env:LOCALAPPDATA "ade\runtime\$version"
            }
            Add-Type -Path (Join-Path $runtime 'GitHub.Copilot.SDK.dll')
            $options = [GitHub.Copilot.CopilotClientOptions]::new()
            $options.UseLoggedInUser = $true
            $options.WorkingDirectory = $ApplicationRepository
            $client = [GitHub.Copilot.CopilotClient]::new($options)
            $stop = [Threading.CancellationTokenSource]::new(60000)
            try {
                [void]$client.StartAsync($stop.Token).GetAwaiter().GetResult()
                $auth = $client.GetAuthStatusAsync($stop.Token).GetAwaiter().GetResult()
                if (!$auth.IsAuthenticated) { throw 'ADE provider is not authenticated.' }
            } finally {
                [void]$client.DisposeAsync().AsTask().GetAwaiter().GetResult()
                $stop.Dispose()
            }
        } else {
            & "$PSScriptRoot\ade.ps1" @Arguments
            if ($LASTEXITCODE -ne 0) { throw "ADE failed with exit $LASTEXITCODE." }
        }
    } finally {
        foreach ($name in $saved.Keys) { [Environment]::SetEnvironmentVariable($name, $saved[$name]) }
    }
}
if ($Phase -ne 'Prepare') {
    $run = Read-JsonFile (Join-Path $Directory 'ade-run.json')
    if ($Phase -eq 'Run') {
        $context = Read-JsonFile $contextPath
        Invoke-AdeRuntime @('run', '--run', $run.runId, '--runs-dir', $runs) $context
    } else {
        & "$PSScriptRoot\ade.ps1" verify --run $run.runId --runs-dir $runs
        if ($LASTEXITCODE -ne 0) { throw 'ADE event-chain verification failed.' }
    }
    exit 0
}
foreach ($required in @('ApplicationRepository', 'ScenarioPath', 'KnowledgeRepository', 'ExpertHome', 'ProfilerCli')) {
    if ([string]::IsNullOrWhiteSpace((Get-Variable $required -ValueOnly))) { throw "Prepare requires -$required" }
}
if (Test-Path -LiteralPath (Join-Path $Directory 'ade-run.json')) { throw 'This investigation already has a proposal; use Run or Verify.' }
$ApplicationRepository = [IO.Path]::GetFullPath($ApplicationRepository)
$ScenarioPath = [IO.Path]::GetFullPath($ScenarioPath)
$ExpertHome = [IO.Path]::GetFullPath($ExpertHome)
$KnowledgeRepository = [IO.Path]::GetFullPath($KnowledgeRepository)
$ProfilerCli = [IO.Path]::GetFullPath($ProfilerCli)
if (!$Dotnet) { $Dotnet = (Get-Command dotnet -ErrorAction Stop).Path }
if ($AdeCopilotCli) { $AdeCopilotCli = [IO.Path]::GetFullPath($AdeCopilotCli) }
$status = @(git -C $ApplicationRepository status --porcelain)
if ($LASTEXITCODE -ne 0 -or $status.Count -ne 0) { throw 'Use a clean application repository; this adapter never discards or silently omits edits.' }
$scenario = Read-JsonFile $ScenarioPath
if ($scenario.schema -ne 'ui-profiler/scenario/v1' -or @($scenario.steps).Count -eq 0) { throw 'Invalid external scenario.' }
$files = @(Get-RepositoryFiles $ApplicationRepository)
$sourceHash = Get-SourceHash $ApplicationRepository $files
New-Item -ItemType Directory -Force $Directory | Out-Null
$copy = Join-Path $Directory 'baseline-source'
$pack = Join-Path $Directory 'pack'
if (!(Test-Path -LiteralPath $contextPath)) {
    Copy-ApplicationSource $ApplicationRepository $copy $files
    Copy-Item -LiteralPath "$plugin\pack" -Destination $pack -Recurse
    $scenarioCopy = Join-Path $Directory 'scenario.json'
    Copy-Item -LiteralPath $ScenarioPath -Destination $scenarioCopy
    $script = Join-Path $pack 'tools\Invoke-Scenario.ps1'
    $context = [pscustomobject]@{
        schema = 'ui-profiler/investigation/v1'
        directory = $Directory
        originalRepository = $ApplicationRepository
        originalSourceSha256 = $sourceHash
        originalRevision = (git -C $ApplicationRepository rev-parse HEAD)
        scenario = $scenario
        scenarioPath = $scenarioCopy
        scenarioSha256 = Get-Sha256 $scenarioCopy
        scenarioScript = $script
        scenarioScriptSha256 = Get-Sha256 $script
        profilerCli = $ProfilerCli
        profilerSha256 = Get-ProfilerHash $ProfilerCli
        dotnet = [IO.Path]::GetFullPath($Dotnet)
        adeCopilotCli = $AdeCopilotCli
        baseline = $null
    }
    Write-JsonFile $contextPath $context
} else {
    $context = Read-JsonFile $contextPath
    if ($context.originalSourceSha256 -ne $sourceHash -or $context.scenarioSha256 -ne (Get-Sha256 $ScenarioPath)) {
        throw 'The unfinished investigation belongs to different source/scenario inputs.'
    }
}
Invoke-AdeRuntime @() $context -Probe
$baseline = Invoke-Measurement $context $copy 'baseline' $files
$baselinePath = Join-Path $Directory 'baseline\measurement.json'
$context.baseline = @{ path = $baselinePath; sha256 = Get-Sha256 $baselinePath }
Write-JsonFile $contextPath $context
$baselineAnalysis = Export-MeasurementAnalysis $baselinePath (Join-Path $Directory 'baseline-analysis') $ProfilerCli
$expertDirectory = Join-Path $Directory 'expert'
$recommendationPath = Join-Path $expertDirectory 'recommendation.md'
if (!(Test-Path -LiteralPath $recommendationPath)) {
    $expertArguments = @{
        ExpertHome = $ExpertHome
        KnowledgeRepository = $KnowledgeRepository
        ApplicationSource = $copy
        MeasurementPath = $baselinePath
        AnalysisPath = $baselineAnalysis.path
        Directory = $expertDirectory
        GitHubAccount = $GitHubAccount
    }
    [void](& "$pack\tools\Get-Recommendation.ps1" @expertArguments)
}
$provenance = Read-JsonFile (Join-Path $expertDirectory 'provenance.json')
Assert-FileHash $recommendationPath $provenance.recommendationSha256
if ($null -eq $provenance.PSObject.Properties['analysisSha256'] -or
    $provenance.analysisSha256 -ne $baselineAnalysis.sha256) {
    throw 'The cached expert recommendation is not bound to this multidimensional evidence.'
}
$cfg = Read-JsonFile (Join-Path $pack 'ade.team.json')
$quoteContext = $contextPath.Replace("'", "''")
$quoteCheck = (Join-Path $pack 'tools\Measure.ps1').Replace("'", "''")
$cfg.checks = @(@{
    id = 'profile-scenario'
    command = "& '$quoteCheck' -RepositoryPath (Get-Location).Path -ContextPath '$quoteContext' -ContextSha256 '$(Get-Sha256 $contextPath)'"
    timeoutSeconds = 900
})
$cfg.protectedPaths = @('.github', 'tests') + @($files | Where-Object {
    $_ -match '\.(csproj|vcxproj|props|targets|sln|slnx|appxmanifest|manifest)$|(^|/)(AGENTS\.md|nuget\.config|global\.json|packages\.lock\.json)$'
})
Write-JsonFile (Join-Path $pack 'ade.team.json') $cfg
$workstreams = Join-Path $Directory 'workstreams'
$name = 'profile-' + $scenario.name
$goal = 'Apply one minimal, behavior-preserving performance fix grounded in the WinUI expert recommendation, then verify the identical scenario once.'
$workstreamArguments = @('workstream', 'new', '--pack', $pack, '--name', $name, '--repo', $ApplicationRepository,
    '--type', 'change', '--goal', $goal, '--workstreams-dir', $workstreams)
$new = @(Invoke-AdeRuntime $workstreamArguments $context)
$ws = ($new | Where-Object { $_ -like 'ADE-RESULT:*' } | Select-Object -Last 1).Substring(12) | ConvertFrom-Json
$notes = @"
# Measured WinUI investigation

Original source: $ApplicationRepository at $($context.originalRevision).
One immutable baseline recording: $($baseline.database.path) (SHA-256 $($baseline.database.sha256)).
Scenario SHA-256: $($context.scenarioSha256).
Measured scenario tick metrics: $($baseline.metrics | ConvertTo-Json -Compress).
Slowest scenario ticks: $($baseline.slowestTicks | ConvertTo-Json -Depth 8 -Compress).

# Full profiler evidence and availability

$(Get-Content -LiteralPath (Join-Path (Split-Path $baselineAnalysis.path -Parent) 'analysis.md') -Raw)

# Actual WinUI expert recommendations

$(Get-Content -LiteralPath $recommendationPath -Raw)

# Execution constraints

Change at most two existing application code files. Preserve all sample content, search semantics,
tree transitions, accessibility, build settings, dependencies, and release configuration.
No app/profiler coupling. No shell/build/capture in the coding session.
The runtime verifies after the only coding round. One candidate capture; no repair measurements.
Passing verifies functional behavior and comparable, complete evidence; it does not imply a speedup.
Report negative changes as regressions and this single pair as observed, not statistically validated.
Compare every available profiler evidence family, not only ticks. Report unavailable resources, dropped
CPU stacks, unknown texture/allocation links and deferred work honestly. No scalar score or success check
establishes that memory, GPU, launch, layout and latency all improved.
"@
foreach ($instruction in @('AGENTS.md', '.github/copilot-instructions.md')) {
    if ($instruction -in $files) {
        $notes += "`n# Application instructions: $instruction`n`n" +
            (Get-Content -LiteralPath (Join-Path $copy $instruction.Replace('/', '\')) -Raw)
    }
}
$notes += "`nRead any additional scoped instructions before editing. Actual project/props values govern build framework versions.`n"
$wsDirectory = $ws.workstream
[IO.File]::WriteAllText((Join-Path $wsDirectory 'notes.md'), $notes)
$proposalArguments = @('propose', '--pack', $pack, '--repo', $ApplicationRepository, '--type', 'change',
    '--task', $goal, '--workstream', $name, '--workstreams-dir', $workstreams, '--runs-dir', $runs)
$proposal = @(Invoke-AdeRuntime $proposalArguments $context)
$result = ($proposal | Where-Object { $_ -like 'ADE-RESULT:*' } | Select-Object -Last 1).Substring(12) | ConvertFrom-Json
Write-JsonFile (Join-Path $Directory 'ade-run.json') $result
Write-Output ($result | ConvertTo-Json -Depth 12)
Write-Output "Awaiting human approval of this exact hash: $($result.proposalSha256). This script never approves."
