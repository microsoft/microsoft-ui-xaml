# Invoke the actual installed/source WinUI expert with local retrieval and read-only tools.
#Requires -Version 7
param(
    [Parameter(Mandatory)][string]$ExpertHome,
    [Parameter(Mandatory)][string]$KnowledgeRepository,
    [Parameter(Mandatory)][string]$ApplicationSource,
    [Parameter(Mandatory)][string]$MeasurementPath,
    [Parameter(Mandatory)][string]$AnalysisPath,
    [Parameter(Mandatory)][string]$ScenarioPath,
    [Parameter(Mandatory)][string]$McpConfigPath,
    [Parameter(Mandatory)][string]$ContractsPath,
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
$scenario = Read-JsonFile $ScenarioPath
if ((Get-Sha256 $ScenarioPath) -ne $measurement.scenarioSha256) { throw 'Expert scenario differs from the measured plan.' }
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
# The private copy retains the real expert instructions; runtime permissions deny shell and writes.
$profileText = [regex]::Replace($profileText, '(?m)^tools:.*(?:\r?\n[ \t]+[^\r\n]*)*', 'tools: ["*"]')
[IO.File]::WriteAllText((Join-Path $clientHome 'agents\winui-expert.agent.md'), $profileText)
Write-JsonFile (Join-Path $clientHome 'settings.json') @{ customAgents = @{ defaultLocalOnly = $true }; disableAllHooks = $true }
$prompt = @"
Analyze only; do not edit application or knowledge, refresh collectors, invoke another agent, or run a capture.
Mandatory harness contracts:
$(Get-Content -LiteralPath $ContractsPath -Raw)

You are the supplied winui-expert agent. This is one real recording, not statistical evidence.
Current WinUI source overrides the bundle's dated repo_path: $KnowledgeRepository
Application source: $ApplicationSource
Read its AGENTS.md and .github\copilot-instructions.md where present before proposing a change.
Measured evidence: $MeasurementPath
Multidimensional analysis: $AnalysisPath
Read sibling analysis.md first. Query the unchanged winui-profiler MCP for original session/tick/object
evidence. Query ui-profiler-evidence/evidence capabilities and relevant metrics/details for the complete
derived report. Both MCP servers must be used successfully; tools are read-only and bound in $McpConfigPath.
The evidence server belongs to this team plugin, NOT WinUIProfiler. It verifies report/input hashes.
Use bounded paging and detail pointers, rather than dumping analysis.json.
Its capability matrix distinguishes unavailable/unsupported evidence from measured zero.
Inspect ALL available families: action timing, UI ticks/cadence, layout/render spans, regions/navigation,
provider events, CPU utilization/stacks and drop/symbol quality, all three memory gauges, GPU utilization,
GPU usage/budget, DirectX/texture lifetimes/descriptors/known bytes, trees/object histories, resources and launch.
Original query exports may be paged; the hashed SQLite and analysis drilldowns retain the full evidence.
The caller performed your required local retrieval; do not claim you ran it yourself:
$retrieval

User scenario, preserved verbatim: $($scenario.description)
Frozen replay steps: $($scenario.steps | ConvertTo-Json -Depth 10 -Compress)
Metrics: $($measurement.metrics | ConvertTo-Json -Compress)
Slowest scenario ticks: $($measurement.slowestTicks | ConvertTo-Json -Depth 8 -Compress)
Read matching full KB sections and relevant current application/framework source. Discover the source
for this user's scenario; no app-specific source paths or Gallery assumptions are supplied.
Return ONLY one JSON object with exactly these fields:
schema: "ui-profiler/recommendations/v1"; summary: text; risks: 1..16 strings;
recommendations: a ranked list of 1..64 independently testable recommendations, each with exactly:
id: unique lowercase slug; title: text; verdict: "propose" or "insufficient-evidence"; summary: text;
observations: 1..16 strings citing actual queries/scopes and source/KB paths/lines;
hypotheses: 1..16 strings with uncertainty; files: objects {path: relative existing app C#/XAML,
change: source/KB-grounded minimal change}; risks: 1..16 strings.
Keep all distinct recommendations in this report, including unsupported ideas with
insufficient-evidence and files []. Do not manufacture recommendations to fill the list.
Every proposed trial starts from the ORIGINAL source, NOT another trial's fix. Split genuinely
independent changes into separate recommendations; a single fix can span all necessary explicitly
listed source files, without an arbitrary two-file cap. Shared-file recommendations are separate
variants, not patches to combine. A proposed item needs at least one existing code file.
If no justified safe fix exists, report insufficient evidence, not a speculative edit.
Preserve all application content, scenario semantics, transitions and accessibility.
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
    $arguments = @('--agent', 'winui-expert', '--prompt', $prompt, '--no-color',
        '--output-format', 'json', '--stream', 'off',
        '--disable-builtin-mcps', '--no-custom-instructions', '--additional-mcp-config', "@$McpConfigPath",
        '--allow-tool', 'read', '--allow-tool', 'winui-profiler', '--allow-tool', 'ui-profiler-evidence',
        '--deny-tool', 'write', '--deny-tool', 'shell',
        '--add-dir', $ExpertHome, '--add-dir', $KnowledgeRepository, '--add-dir', $ApplicationSource,
        '--add-dir', (Split-Path $MeasurementPath -Parent), '--add-dir', (Split-Path $AnalysisPath -Parent),
        '--add-dir', $Directory)
    if ([IO.Path]::GetExtension($cli) -eq '.ps1') {
        $arguments = @('-NoProfile', '-NonInteractive', '-File', $cli) + $arguments
        $cli = (Get-Command pwsh -ErrorAction Stop).Path
    }
    $events = Invoke-LoggedProcess $cli $arguments $Directory (Join-Path $Directory 'expert.log')
    $eventPath = Join-Path $Directory 'expert-events.jsonl'
    [IO.File]::WriteAllText($eventPath, $events)
    $parser = Join-Path $PSScriptRoot 'Harness-Mcp.py'
    $python = (Get-Command python -ErrorAction Stop).Path
    $parseArguments = @($parser, '--parse-agent-events', $eventPath, '--required-server', 'winui-profiler',
        '--expected-database', $measurement.database.path)
    $native = (Invoke-LoggedProcess $python $parseArguments $Directory (Join-Path $Directory 'native-query-verification.log') 30) | ConvertFrom-Json
    $parseArguments = @($parser, '--parse-agent-events', $eventPath, '--required-server', 'ui-profiler-evidence', '--required-tool', 'evidence')
    $derived = (Invoke-LoggedProcess $python $parseArguments $Directory (Join-Path $Directory 'derived-query-verification.log') 30) | ConvertFrom-Json
    $structuredPath = Join-Path $Directory 'recommendation.json'
    [IO.File]::WriteAllText($structuredPath, $derived.answer)
    $validation = @($parser, '--recommendation', $structuredPath, '--application-source', $ApplicationSource)
    $recommendation = (Invoke-LoggedProcess $python $validation $Directory (Join-Path $Directory 'recommendation-validation.log') 30) | ConvertFrom-Json
    $answer = "# Actual WinUI expert recommendations`n`n$($recommendation.summary)`n"
    foreach ($entry in $recommendation.recommendations) {
        $answer += "`n## $($entry.id): $($entry.title)`n`nVerdict: $($entry.verdict). $($entry.summary)`n"
        foreach ($section in @('observations', 'hypotheses', 'files', 'risks')) {
            $answer += "`n### $section`n`n"
            foreach ($item in $entry.$section) {
                $answer += if ($section -eq 'files') { "- $($item.path): $($item.change)`n" } else { "- $item`n" }
            }
        }
    }
    [IO.File]::WriteAllText((Join-Path $Directory 'recommendation.md'), $answer)
    Write-JsonFile (Join-Path $Directory 'provenance.json') @{
        agent = 'winui-expert'; profile = $profilePath; profileSha256 = Get-Sha256 $profilePath
        renderedProfileSha256 = Get-Sha256 (Join-Path $clientHome 'agents\winui-expert.agent.md')
        measurementSha256 = Get-Sha256 $MeasurementPath
        analysisSha256 = Get-Sha256 $AnalysisPath
        scenarioSha256 = Get-Sha256 $ScenarioPath
        contracts = @{ path = $ContractsPath; sha256 = Get-Sha256 $ContractsPath }
        events = @{ path = $eventPath; sha256 = Get-Sha256 $eventPath }
        structuredRecommendation = @{ path = $structuredPath; sha256 = Get-Sha256 $structuredPath }
        actionable = @($recommendation.recommendations | Where-Object verdict -eq 'propose' | ForEach-Object id)
        mcpConfiguration = @{ path = $McpConfigPath; sha256 = Get-Sha256 $McpConfigPath }
        mcpQueries = @($native.queries) + @($derived.queries)
        retrievalSha256 = Get-Sha256 (Join-Path $Directory 'retrieval.txt')
        recommendationSha256 = Get-Sha256 (Join-Path $Directory 'recommendation.md')
        sourceRepository = $KnowledgeRepository
        allowed = @('read', 'winui-profiler', 'ui-profiler-evidence')
        denied = @('write', 'shell'); writes = $false; collectorRefresh = $false
    }
    $answer
} finally {
    foreach ($name in $saved.Keys) { [Environment]::SetEnvironmentVariable($name, $saved[$name]) }
}
