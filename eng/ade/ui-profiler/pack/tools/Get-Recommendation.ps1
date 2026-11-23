# Invoke the actual installed/source WinUI expert with local retrieval and read-only tools.
#Requires -Version 7
param(
    [Parameter(Mandatory)][string]$ExpertHome,
    [Parameter(Mandatory)][string]$KnowledgeRepository,
    [Parameter(Mandatory)][string]$ApplicationSource,
    [Parameter(Mandatory)][string]$MeasurementPath,
    [Parameter(Mandatory)][string]$AnalysisPath,
    [Parameter(Mandatory)][string]$Directory,
    [string]$GitHubAccount
)
$ErrorActionPreference = 'Stop'
Import-Module "$PSScriptRoot\ProfilerHarness.psm1" -Force -DisableNameChecking
$profilePath = Join-Path $ExpertHome 'winui-expert.agent.md'
$retriever = Join-Path $ExpertHome 'rag\rag.py'
if (!(Test-Path -LiteralPath $profilePath) -or !(Test-Path -LiteralPath $retriever)) {
    throw 'Supply the actual WinUI_Expert_Agent bundle, including its local RAG/KB; no substitute agent is used.'
}
New-Item -ItemType Directory -Force $Directory | Out-Null
$measurement = Read-JsonFile $MeasurementPath
Assert-Measurement $measurement
$analysis = Read-JsonFile $AnalysisPath
Assert-FileHash $MeasurementPath $analysis.measurement.sha256
$analyzer = Join-Path $PSScriptRoot 'Analyze-Profiler.py'
$verifyArguments = @($analyzer, 'verify', '--manifest', (Join-Path (Split-Path $AnalysisPath -Parent) 'manifest.json'))
[void](Invoke-LoggedProcess (Get-Command python -ErrorAction Stop).Path $verifyArguments $Directory (Join-Path $Directory 'evidence-verify.log') 120)
$retrieval = Invoke-LoggedProcess (Get-Command python -ErrorAction Stop).Path @($retriever, 'search',
    '--agent', 'WinUI_Expert_Agent', '--query',
    'WinUI CPU layout rendering working set private commit GPU texture allocation virtualization deferred loading performance',
    '--top', '8', '--max-chars', '16000') $ExpertHome (Join-Path $Directory 'retrieval.log')
[IO.File]::WriteAllText((Join-Path $Directory 'retrieval.txt'), $retrieval)
$clientHome = Join-Path $Directory 'copilot-home'
New-Item -ItemType Directory -Force (Join-Path $clientHome 'agents') | Out-Null
$profileText = [IO.File]::ReadAllText($profilePath).Replace('{{AGENT_INSTALL_DIR}}', $ExpertHome)
[IO.File]::WriteAllText((Join-Path $clientHome 'agents\winui-expert.agent.md'), $profileText)
Write-JsonFile (Join-Path $clientHome 'settings.json') @{ customAgents = @{ defaultLocalOnly = $true }; disableAllHooks = $true }
$prompt = @"
Analyze only; do not edit application or knowledge, refresh collectors, invoke another agent, or run a capture.
You are the supplied winui-expert agent. This is one real recording, not statistical evidence.
Current WinUI source overrides the bundle's dated repo_path: $KnowledgeRepository
Application source: $ApplicationSource
Read its AGENTS.md and .github\copilot-instructions.md where present before proposing a change.
Measured evidence: $MeasurementPath
Multidimensional analysis: $AnalysisPath
Read sibling analysis.md first, then relevant full metrics and drilldowns in analysis.json.
Its capability matrix distinguishes unavailable/unsupported evidence from measured zero.
Inspect ALL available families: action timing, UI ticks/cadence, layout/render spans, regions/navigation,
provider events, CPU utilization/stacks and drop/symbol quality, all three memory gauges, GPU utilization,
GPU usage/budget, DirectX/texture lifetimes/descriptors/known bytes, trees/object histories, resources and launch.
Original query exports may be paged; the hashed SQLite and analysis drilldowns retain the full evidence.
The caller performed your required local retrieval; do not claim you ran it yourself:
$retrieval

Scenario: open Gallery, search TreeView, expand/collapse its first sample.
Metrics: $($measurement.metrics | ConvertTo-Json -Compress)
Slowest scenario ticks: $($measurement.slowestTicks | ConvertTo-Json -Depth 8 -Compress)
Read matching full KB sections and the relevant current application/framework source. In the application,
start with WinUIGallery\MainWindow.xaml.cs and WinUIGallery\Samples\TreeView\TreeViewPage.xaml and .xaml.cs.
Return concise Markdown with: observations citing query/tick evidence; hypotheses with explicit uncertainty;
one minimal behavior-preserving fix (at most two existing application code files) citing current source/KB;
and verification risks. Preserve all sample content, search semantics, tree transitions and accessibility.
Do not change SDKs, build options, profiler, tests, or app/harness coupling.
Elapsed ticks are not CPU time or present/display timing. Dropped stacks and unresolved addresses do not
establish source function costs. Shared allocations are counted once; unknown links are not zero bytes.
Do not infer complete element-to-surface/texture ownership. Resource snapshots may be unsupported.
Recommend against the measured bottleneck, not only tick metrics; identify cross-dimensional tradeoffs
and later-work deferral. Do not assert that all dimensions improve.
"@
[IO.File]::WriteAllText((Join-Path $Directory 'prompt.txt'), $prompt)
$saved = @{}
foreach ($name in @('COPILOT_HOME', 'COPILOT_AUTO_UPDATE', 'COPILOT_GITHUB_TOKEN')) {
    $saved[$name] = [Environment]::GetEnvironmentVariable($name)
}
try {
    $env:COPILOT_HOME = $clientHome
    $env:COPILOT_AUTO_UPDATE = 'false'
    if ($GitHubAccount) {
        $token = gh auth token --hostname github.com --user $GitHubAccount
        if ($LASTEXITCODE -ne 0) { throw 'The selected existing GitHub credential is unavailable.' }
        $env:COPILOT_GITHUB_TOKEN = $token.Trim()
        $token = $null
    } elseif (!$env:COPILOT_GITHUB_TOKEN -and !$env:GH_TOKEN -and !$env:GITHUB_TOKEN) {
        throw 'Use an existing GitHub account with -GitHubAccount, or supply a Copilot token via the environment.'
    }
    $cli = (Get-Command copilot -ErrorAction Stop).Path
    $arguments = @('--agent', 'winui-expert', '--prompt', $prompt, '--silent', '--no-color',
        '--disable-builtin-mcps', '--no-custom-instructions', '--available-tools', 'view',
        '--allow-tool', 'read', '--deny-tool', 'write', '--deny-tool', 'shell',
        '--add-dir', $ExpertHome, '--add-dir', $KnowledgeRepository, '--add-dir', $ApplicationSource,
        '--add-dir', (Split-Path $MeasurementPath -Parent), '--add-dir', (Split-Path $AnalysisPath -Parent))
    if ([IO.Path]::GetExtension($cli) -eq '.ps1') {
        $arguments = @('-NoProfile', '-NonInteractive', '-File', $cli) + $arguments
        $cli = (Get-Command pwsh -ErrorAction Stop).Path
    }
    $answer = Invoke-LoggedProcess $cli $arguments $ApplicationSource (Join-Path $Directory 'expert.log')
    if ([string]::IsNullOrWhiteSpace($answer)) { throw 'The actual WinUI expert returned no recommendation.' }
    [IO.File]::WriteAllText((Join-Path $Directory 'recommendation.md'), $answer)
    Write-JsonFile (Join-Path $Directory 'provenance.json') @{
        agent = 'winui-expert'; profile = $profilePath; profileSha256 = Get-Sha256 $profilePath
        measurementSha256 = Get-Sha256 $MeasurementPath
        analysisSha256 = Get-Sha256 $AnalysisPath
        retrievalSha256 = Get-Sha256 (Join-Path $Directory 'retrieval.txt')
        recommendationSha256 = Get-Sha256 (Join-Path $Directory 'recommendation.md')
        sourceRepository = $KnowledgeRepository
        tools = @('view'); writes = $false; collectorRefresh = $false
    }
    $answer
} finally {
    foreach ($name in $saved.Keys) { [Environment]::SetEnvironmentVariable($name, $saved[$name]) }
}
