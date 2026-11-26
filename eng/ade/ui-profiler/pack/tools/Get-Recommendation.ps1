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
    [string]$ControlAnalysisPath,
    [string]$SupplementalEvidencePath,
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
$intentText = if ($scenario.PSObject.Properties['intent']) { $scenario.intent | ConvertTo-Json -Depth 20 } else {
    'Legacy scenario: no frozen intent specification. Preserve its actions/budget; label retrospective analysis questions as inferred.'
}
$intentQuestion = if ($scenario.PSObject.Properties['intent']) { $scenario.intent.question } else { 'Retrospective analysis of the frozen scenario.' }
$controlText = 'No control-scoped report was supplied. Query native evidence for the relevant control; do not attribute process totals to it.'
if ($ControlAnalysisPath) {
    [void](Invoke-LoggedProcess (Get-Command python -ErrorAction Stop).Path @("$PSScriptRoot\Analyze-Profiler.py",
        'verify', '--manifest', (Join-Path (Split-Path $ControlAnalysisPath -Parent) 'manifest.json')) `
        $Directory (Join-Path $Directory 'control-evidence-verify.log') 120)
    $controlText = "Control-scoped evidence: $ControlAnalysisPath`n" +
        (Get-Content -LiteralPath (Join-Path (Split-Path $ControlAnalysisPath -Parent) 'control-analysis.md') -Raw)
}
$supplement = if ($SupplementalEvidencePath) {
    Get-SupplementalEvidenceBinding $SupplementalEvidencePath $MeasurementPath $measurement
} else { $null }
if ($supplement) {
    $controlText += "`nAdditional hash-bound MUX WinUIProfiler discovery evidence:`n" +
        ($supplement.content | ConvertTo-Json -Depth 15) +
        "`nRead the referenced artifacts. They are supplemental discovery material, not another capture or a candidate comparison. " +
        "Distinguish symbols/metadata recovered on a derivative from raw measured events. " +
        "Keep original recording scope, missing samples, ownership limitations and elapsed-time versus sample-count distinctions explicit. " +
        "Verify any inferred mechanism in the currently selected source; do not assume the recorded runtime matches that source."
}
if ((Get-Sha256 $ScenarioPath) -ne $measurement.scenarioSha256) { throw 'Expert scenario differs from the measured plan.' }
Assert-FileHash $MeasurementPath $analysis.measurement.sha256
$analyzer = Join-Path $PSScriptRoot 'Analyze-Profiler.py'
$verifyArguments = @($analyzer, 'verify', '--manifest', (Join-Path (Split-Path $AnalysisPath -Parent) 'manifest.json'))
[void](Invoke-LoggedProcess (Get-Command python -ErrorAction Stop).Path $verifyArguments $Directory (Join-Path $Directory 'evidence-verify.log') 120)
$retrieval = ''
$queries = [ordered]@{
    application = "$($scenario.description) XAML application sample initialization deferred loading virtualization bindings accessibility performance"
    control = "$($scenario.description) control implementation OnApplyTemplate measure arrange pane visual states ItemsRepeater performance"
    framework = "$($scenario.description) $intentQuestion WinUI shared engine layout render property input resources lifetime performance"
}
foreach ($level in $queries.Keys) {
    $result = Invoke-LoggedProcess (Get-Command python -ErrorAction Stop).Path @($retriever, 'search',
        '--agent', 'WinUI_Expert_Agent', '--query', $queries[$level], '--top', '6', '--max-chars', '12000') `
        $ExpertHome (Join-Path $Directory "retrieval-$level.log")
    $retrieval += "`n## $level retrieval`n$result`n"
}
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
Additive intent specification (one run, not three repeats):
$intentText
Control-scoped evidence and attribution limits:
$controlText
Frozen replay steps: $($scenario.steps | ConvertTo-Json -Depth 10 -Compress)
Metrics: $($measurement.metrics | ConvertTo-Json -Compress)
Slowest scenario ticks: $($measurement.slowestTicks | ConvertTo-Json -Depth 8 -Compress)
Read matching full KB sections and relevant current application/framework source. Discover the source
for this user's scenario; no app-specific source paths or Gallery assumptions are supplied.
Investigate ALL THREE levels separately, not only app usage:
application: this app's usage, realization, bindings, event handlers and content.
control: relevant control implementation/templates under $KnowledgeRepository\controls\dev.
framework: shared engine/layout/render/property-system implementation under $KnowledgeRepository\dxaml\xcp.
Successfully view relevant code in all three source roots; a source-file list or directory listing is not a code read.
Use current source and the local KB to trace the scenario at every level. Do not equate aggregate
layout cost with a proven defect in a control/framework function. Measured runtime SHA-256:
$($measurement.frameworkSha256). The supplied MUX checkout is NOT proven to be the source revision
of that binary; explicitly report this runtime/source parity limitation unless concrete provenance proves it.
Independent app and native ADE trial preparation is available. Native trials build a source-matched
baseline/candidate pair and verify the actual private runtime/resource closure loaded by the app.
Do not omit control/framework recommendations because the historical recording lacks source parity;
that limits claims about the old binary, not a separately approved current-source paired experiment.
Separate confidence in a recommendation from whether its target build/validation workflow is available.
Return ONLY one JSON object with exactly these fields:
schema: "ui-profiler/recommendations/v3"; summary: text; risks: 1..16 strings;
coverage: object with exactly application, control, framework, each a nonempty source/evidence-cited
summary of that level's investigation, findings and missing attribution. Report honestly if no safe optimization follows.
runtimeSourceParity: text explaining whether measured runtime and inspected source are proven to match and why;
detailedAnalysis: object with exactly application, control, framework, EACH using the SAME layout:
{question: text, time: 1..16 strings, cpu: 1..16 strings, memory: 1..16 strings,
 modelAnalysis: 1..16 strings, unverified: 1..16 strings, validation: 1..16 strings}.
Give deep analysis at EVERY level, including the relevant control itself and shared engine mechanisms:
time: separate per-instance/control attribution from action-window/process/nested-span context;
cpu: source-grounded call paths, sampled functions, drop/symbol quality and unknown exclusive costs;
memory: per-instance realized subtrees/lifetimes/owned bytes ONLY where profiler evidence proves them,
with process working/private/commit and shared graphics costs explicitly separate.
Model-based analysis using profiler measurements AND code/KB is welcome. Label source/model inference
and estimates as such, never as measured exclusive CPU time or owned allocation bytes. All measurements
MUST come from MUX WinUIProfiler, not an alternate profiler or historical KB examples.
Unavailable attribution requires a useful explanation and precise missing telemetry, not omission of a level.
Report baseline-only evidence honestly: no fix, candidate improvement or draft PR exists until actually executed.
recommendations: a ranked list of 1..64 independently testable recommendations, each with exactly:
id: unique lowercase slug; level: "application", "control" or "framework"; title: text;
verdict: "propose", "recommend" or "insufficient-evidence"; summary: text;
observations: 1..16 strings citing actual queries/scopes and source/KB paths/lines;
hypotheses: 1..16 strings with uncertainty; validation: 1..16 concrete functional/performance checks;
files: objects {path: relative existing source path, change: source/KB-grounded minimal change};
risks: 1..16 strings.
Application "propose" scopes contain only existing app C#/XAML files, relative to Application source.
Control/framework "recommend" scopes are relative to the MUX repository, confined respectively to
controls\dev or dxaml\xcp existing C++/header/IDL/XAML/C# source. These are retained recommendations
requiring a separate target build/private-runtime/validation workflow, NOT approvable app trials.
Only application entries may say "propose"; only control/framework entries may say "recommend".
At every level "insufficient-evidence" has files []; do not substitute that verdict merely for unsupported execution.
Keep all distinct recommendations in this report. Ideas lacking performance/source evidence use
insufficient-evidence and files []; an unavailable execution workflow is not missing evidence.
An experiment is not a proven fix. A measured workload plus a concrete source-grounded avoidable
mechanism, bounded existing-file change and falsifiable correctness/regression plan can justify
propose/recommend even when exclusive CPU or owned bytes are unavailable. State that causal savings
are unverified until the candidate is measured; do not require a successful experiment before proposing it.
Do not manufacture recommendations to fill the list.
Every proposed trial starts from the ORIGINAL source, NOT another trial's fix. Split genuinely
independent changes into separate recommendations; a single fix can span all necessary explicitly
listed source files, without an arbitrary two-file cap. Shared-file recommendations are separate
variants, not patches to combine. A proposed item needs at least one existing code file.
If no justified safe fix exists, report insufficient evidence, not a speculative edit.
Preserve all application content, scenario semantics, transitions and accessibility.
Do not edit source, SDKs, build options, profiler, tests, or app/harness coupling. A control/framework
recommendation must specify native target build and measured-runtime parity checks before any claimed improvement.
Elapsed ticks are not CPU time or present/display timing. Dropped stacks and unresolved addresses do not
establish source function costs. Shared allocations are counted once; unknown links are not zero bytes.
Do not infer complete element-to-surface/texture ownership. Resource snapshots may be unsupported.
Recommend against the measured bottleneck, not only tick metrics; identify cross-dimensional tradeoffs
and later-work deferral. Do not assert that all dimensions improve.
"@
$promptPath = Join-Path $Directory 'prompt.txt'
[IO.File]::WriteAllText($promptPath, $prompt)
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
    $arguments = @('--agent', 'winui-expert', '--no-color',
        '--output-format', 'json', '--stream', 'off',
        '--disable-builtin-mcps', '--no-custom-instructions', '--additional-mcp-config', "@$McpConfigPath",
        '--allow-tool', 'read', '--allow-tool', 'winui-profiler', '--allow-tool', 'ui-profiler-evidence',
        '--deny-tool', 'write', '--deny-tool', 'shell',
        '--add-dir', $ExpertHome, '--add-dir', $KnowledgeRepository, '--add-dir', $ApplicationSource,
        '--add-dir', (Split-Path $MeasurementPath -Parent), '--add-dir', (Split-Path $AnalysisPath -Parent),
        '--add-dir', $Directory)
    if ($supplement) { $arguments += @('--add-dir', (Split-Path $supplement.path -Parent)) }
    if ([IO.Path]::GetExtension($cli) -eq '.ps1') {
        $arguments = @('-NoProfile', '-NonInteractive', '-File', $cli) + $arguments
        $cli = (Get-Command pwsh -ErrorAction Stop).Path
    }
    $events = Invoke-LoggedProcess $cli $arguments $Directory (Join-Path $Directory 'expert.log') -InputText $prompt
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
    $validation = @($parser, '--recommendation', $structuredPath, '--application-source', $ApplicationSource,
        '--knowledge-source', $KnowledgeRepository)
    $recommendation = (Invoke-LoggedProcess $python $validation $Directory (Join-Path $Directory 'recommendation-validation.log') 30) | ConvertFrom-Json
    if ($recommendation.schema -ne 'ui-profiler/recommendations/v3') { throw 'New investigations require identical deep analysis layouts at all three levels.' }
    $coverageArguments = @($parser, '--source-coverage', $eventPath, '--application-source', $ApplicationSource,
        '--knowledge-source', $KnowledgeRepository)
    $sourceCoverage = (Invoke-LoggedProcess $python $coverageArguments $Directory `
        (Join-Path $Directory 'source-coverage-verification.log') 30) | ConvertFrom-Json
    $answer = "# Actual WinUI expert recommendations`n`n$($recommendation.summary)`n"
    $answer += "`nRuntime/source parity: $($recommendation.runtimeSourceParity)`n"
    foreach ($level in @('application', 'control', 'framework')) {
        $answer += "`n## $level investigation`n`n$($recommendation.coverage.$level)`n"
        $layerAnalysis = $recommendation.detailedAnalysis.$level
        $answer += "`nQuestion: $($layerAnalysis.question)`n"
        foreach ($section in @('time', 'cpu', 'memory', 'modelAnalysis', 'unverified', 'validation')) {
            $answer += "`n### $section`n`n"
            foreach ($item in $layerAnalysis.$section) { $answer += "- $item`n" }
        }
    }
    foreach ($entry in $recommendation.recommendations) {
        $answer += "`n## $($entry.id): $($entry.title)`n`nVerdict: $($entry.verdict). $($entry.summary)`n"
        $answer += "`nLevel: $($entry.level). Repository: $(if ($entry.level -eq 'application') { 'application' } else { 'MUX' }).`n"
        foreach ($section in @('observations', 'hypotheses', 'files', 'validation', 'risks')) {
            $answer += "`n### $section`n`n"
            foreach ($item in $entry.$section) {
                $answer += if ($section -eq 'files') { "- $($item.path): $($item.change)`n" } else { "- $item`n" }
            }
        }
    }
    [IO.File]::WriteAllText((Join-Path $Directory 'recommendation.md'), $answer)
    if ($supplement) { Assert-SupplementalEvidenceBinding $supplement }
    Write-JsonFile (Join-Path $Directory 'provenance.json') @{
        agent = 'winui-expert'; profile = $profilePath; profileSha256 = Get-Sha256 $profilePath
        renderedProfileSha256 = Get-Sha256 (Join-Path $clientHome 'agents\winui-expert.agent.md')
        prompt = @{ path = $promptPath; sha256 = Get-Sha256 $promptPath; transport = 'utf8-stdin' }
        implementation = @{
            launcher = @{ path = $PSCommandPath; sha256 = Get-Sha256 $PSCommandPath }
            parser = @{ path = $parser; sha256 = Get-Sha256 $parser }
            module = @{ path = "$PSScriptRoot\ProfilerHarness.psm1"; sha256 = Get-Sha256 "$PSScriptRoot\ProfilerHarness.psm1" }
        }
        sourceCoverage = $sourceCoverage; investigationLevels = @('application', 'control', 'framework')
        requiresTargetWorkflow = @($recommendation.recommendations | Where-Object verdict -eq 'recommend' | ForEach-Object id)
        measurementSha256 = Get-Sha256 $MeasurementPath
        analysisSha256 = Get-Sha256 $AnalysisPath
        controlAnalysis = if ($ControlAnalysisPath) { @{ path = $ControlAnalysisPath; sha256 = Get-Sha256 $ControlAnalysisPath } } else { $null }
        supplementalEvidence = if ($supplement) { @{ path = $supplement.path; sha256 = $supplement.sha256 } } else { $null }
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
