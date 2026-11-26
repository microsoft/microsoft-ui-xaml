# Start the tool-using planner from natural language, or execute/verify a human-approved ADE proposal.
#Requires -Version 7
param(
    [ValidateSet('Prepare', 'Investigate', 'Run', 'Verify', 'Report')][string]$Phase = 'Prepare',
    [Parameter(Mandatory)][string]$Directory,
    [string]$ApplicationRepository,
    [string]$Scenario,
    [string]$KnowledgeRepository,
    [string]$ExpertHome,
    [string]$ProfilerCli,
    [ValidateSet('Gui', 'Cli')][string]$CaptureBackend = 'Gui',
    [string]$ProfilerGui,
    [string]$Dotnet,
    [string]$Python,
    [string]$AdeCopilotCli,
    [string]$GitHubAccount,
    [string]$SourceInvestigation,
    [string]$SupplementalEvidencePath,
    [string]$TargetControl,
    [string[]]$RecommendationId = @(),
    [switch]$AllowPreview,
    [switch]$AllowMeasurements
)
$ErrorActionPreference = 'Stop'
$plugin = Split-Path $PSScriptRoot -Parent
Import-Module "$plugin\pack\tools\ProfilerHarness.psm1" -Force -DisableNameChecking

function Get-InvestigationKnowledgeSource([string]$Repository) {
    $root = [IO.Path]::GetFullPath($Repository).TrimEnd('\')
    $gitRoot = git -C $root rev-parse --show-toplevel
    if ($LASTEXITCODE -ne 0 -or [IO.Path]::GetFullPath($gitRoot.Replace('/', '\')).TrimEnd('\') -ne $root) {
        throw 'Use the actual hypothesis Git repository root, not a nested source directory.'
    }
    $revision = git -C $root rev-parse HEAD
    if ($LASTEXITCODE -ne 0) { throw 'The hypothesis source must be a Git checkout.' }
    $files = @(Get-NativeRepositoryFiles $root)
    @{
        path = $root; revision = $revision; files = $files
        sourceSha256 = Get-SourceHash $root $files
        excludedSampleSubmodules = @(Get-NativeSampleSubmodules $root)
    }
}

function Assert-InvestigationKnowledgeSource($Knowledge) {
    $current = Get-InvestigationKnowledgeSource $Knowledge.path
    if ($current.revision -cne $Knowledge.revision -or $current.sourceSha256 -cne $Knowledge.sourceSha256 -or
        (ConvertTo-Json @($current.files) -Compress) -cne (ConvertTo-Json @($Knowledge.files) -Compress) -or
        (ConvertTo-Json @($current.excludedSampleSubmodules | Select-Object path, revision) -Compress) -cne
        (ConvertTo-Json @($Knowledge.excludedSampleSubmodules | Select-Object path, revision) -Compress)) {
        throw 'The expert hypothesis source changed during read-only investigation.'
    }
}

function Resolve-InvestigationKnowledgeSource($Request, [string]$Override) {
    Get-InvestigationKnowledgeSource $(if ($Override) { $Override } else { $Request.knowledgeRepository })
}

$Directory = [IO.Path]::GetFullPath($Directory)
$contextPath = Join-Path $Directory 'context.json'
$runs = Join-Path $Directory 'ade-runs'
if ($Phase -eq 'Investigate') {
    if (!$SourceInvestigation) { throw 'Investigate requires -SourceInvestigation with an existing valid baseline.' }
    $sourceContextPath = Join-Path ([IO.Path]::GetFullPath($SourceInvestigation)) 'context.json'
    $sourceContextSha256 = Get-Sha256 $sourceContextPath
    $source = Read-JsonFile $sourceContextPath
    if ($source.schema -ne 'ui-profiler/investigation/v3' -or !$source.baseline -or !$source.analysis -or !$source.nativeMcp) {
        throw 'Investigate requires a completed baseline, analysis and frozen native MCP; it never records.'
    }
    if ([IO.Path]::GetFullPath($source.directory) -ne [IO.Path]::GetFullPath($SourceInvestigation)) {
        throw 'The source context belongs to a different investigation directory.'
    }
    foreach ($reference in @($source.request, $source.contracts, $source.module, $source.baseline, $source.analysis, $source.nativeMcp)) {
        Assert-FileHash $reference.path $reference.sha256
    }
    Assert-FrozenScenario $source
    Assert-OriginalSource $source
    if ((Get-ProfilerHash $source.nativeMcp.path) -ne $source.nativeMcp.deploymentSha256 -or
        (Get-SourceHash $source.nativeMcp.source $source.nativeMcp.files) -ne $source.nativeMcp.sourceSha256) {
        throw 'The frozen native MCP implementation changed.'
    }
    $applicationSource = Join-Path $source.directory 'baseline-source'
    $applicationFiles = @(Get-RepositoryFiles $source.originalRepository)
    if ((Get-SourceHash $applicationSource $applicationFiles) -ne $source.originalSourceSha256) {
        throw 'The source inspected by the expert must match the measured original application.'
    }
    $request = Read-JsonFile $source.request.path
    $knowledge = Resolve-InvestigationKnowledgeSource $request $KnowledgeRepository
    $supplemental = if ($SupplementalEvidencePath) {
        Get-SupplementalEvidenceBinding $SupplementalEvidencePath $source.baseline.path (Read-JsonFile $source.baseline.path)
    } else { $null }
    Assert-RunDirectory $Directory @($source.directory, $request.applicationRepository, $request.knowledgeRepository,
        $knowledge.path, $request.expertHome, (Split-Path $request.profilerCli -Parent), (Split-Path $request.dotnet -Parent))
    if ($supplemental) { Assert-RunDirectory $Directory @((Split-Path $supplemental.path -Parent)) }
    if (Test-Path -LiteralPath $Directory) { throw 'Use a new report directory; never overwrite sealed investigations.' }
    if (!$Python) { $Python = (Get-Command python -ErrorAction Stop).Path }
    $previousPath = $env:PATH
    try {
        $env:PATH = "$(Split-Path $Python -Parent);$env:PATH"
        New-Item -ItemType Directory -Path $Directory | Out-Null
        $mcpPath = Join-Path $Directory 'profiler-mcp.json'
        # Discovery MCP provenance stays frozen; an override changes hypothesis source, not recorded runtime identity.
        Write-JsonFile $mcpPath (New-ProfilerMcpConfiguration $request.knowledgeRepository $source.dotnet `
            $source.analysis.path (Join-Path $source.directory 'pack\tools\Evidence-Mcp.py') $source.nativeMcp.path)
        [void](Invoke-LoggedProcess $Python @("$plugin\pack\tools\Harness-Mcp.py", '--probe-mcp', $mcpPath) `
            $Directory (Join-Path $Directory 'mcp-preflight.log') 120)
        if (!$TargetControl -and $source.scenario.PSObject.Properties['intent']) {
            $TargetControl = $source.scenario.intent.target.control
        }
        $controlAnalysis = if ($TargetControl) {
            Export-ControlAnalysis $source.analysis.path $TargetControl (Join-Path $Directory 'control-analysis') $source.profilerCli
        } else { $null }
        $account = if ($GitHubAccount) { $GitHubAccount } else { $source.githubAccount }
        $expertArguments = @{
            ExpertHome = $request.expertHome; KnowledgeRepository = $knowledge.path; ApplicationSource = $applicationSource
            MeasurementPath = $source.baseline.path; AnalysisPath = $source.analysis.path; ScenarioPath = $source.scenarioPath
            McpConfigPath = $mcpPath; ContractsPath = "$plugin\CONTRACTS.md"; Directory = (Join-Path $Directory 'expert')
            ControlAnalysisPath = $(if ($controlAnalysis) { $controlAnalysis.path } else { '' }); GitHubAccount = $account
        }
        if ($supplemental) { $expertArguments.SupplementalEvidencePath = $supplemental.path }
        [void](& "$plugin\pack\tools\Get-Recommendation.ps1" @expertArguments)
        Assert-InvestigationKnowledgeSource $knowledge
        $provenance = Read-JsonFile (Join-Path $Directory 'expert\provenance.json')
        if ([IO.Path]::GetFullPath($provenance.sourceRepository).TrimEnd('\') -ne $knowledge.path) {
            throw 'The fresh expert inspected a different hypothesis source checkout.'
        }
        if ($supplemental) {
            Assert-SupplementalEvidenceBinding $supplemental
            if ($provenance.supplementalEvidence.path -ne $supplemental.path -or
                $provenance.supplementalEvidence.sha256 -cne $supplemental.sha256) {
                throw 'The fresh expert did not bind the supplied supplemental discovery evidence.'
            }
        }
        Assert-OriginalSource $source
        Assert-FileHash $sourceContextPath $sourceContextSha256
        if ((Get-SourceHash $applicationSource $applicationFiles) -ne $source.originalSourceSha256) {
            throw 'The inspected application source changed during investigation.'
        }
        $recommendationsPath = Join-Path $Directory 'expert\recommendation.json'
        $context = [pscustomobject]@{
            schema = 'ui-profiler/read-only-investigation/v1'; directory = $Directory
            sourceInvestigation = @{ path = $sourceContextPath; sha256 = $sourceContextSha256 }
            knowledgeRepository = $knowledge
            contracts = @{ path = "$plugin\CONTRACTS.md"; sha256 = Get-Sha256 "$plugin\CONTRACTS.md" }
            baseline = $source.baseline; analysis = $source.analysis; originalRevision = $source.originalRevision
            recommendations = @{ path = $recommendationsPath; sha256 = Get-Sha256 $recommendationsPath }
            targetControl = $TargetControl; controlAnalysis = $controlAnalysis
            provenance = @{ path = (Join-Path $Directory 'expert\provenance.json')
                sha256 = Get-Sha256 (Join-Path $Directory 'expert\provenance.json') }
            trials = @(); report = $null; captures = 0; editsAuthorized = $false
        }
        if ($supplemental) {
            $context | Add-Member -NotePropertyName supplementalEvidence -NotePropertyValue @{
                path = $supplemental.path; sha256 = $supplemental.sha256
            }
        }
        $result = Write-RecommendationReport $context
        $context.report = @{ path = $result.path; sha256 = $result.sha256 }
        Write-JsonFile $contextPath $context
        [void](Assert-RecommendationReport $context)
        @{ status = 'ReadOnlyRecommendations'; report = $context.report; captures = 0; editsAuthorized = $false } |
            ConvertTo-Json -Depth 10
    } finally { $env:PATH = $previousPath }
    return
}
if ($SupplementalEvidencePath) { throw '-SupplementalEvidencePath is only valid with -Phase Investigate.' }
if ($SourceInvestigation) { throw '-SourceInvestigation is only valid with -Phase Investigate.' }
if ($Phase -ne 'Prepare') {
    $selectedIds = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    foreach ($id in $RecommendationId) {
        if (!$selectedIds.Add($id)) { throw 'Recommendation ids must be unique; no trial has been started.' }
    }
    $context = Read-JsonFile $contextPath
    if ($context.schema -eq 'ui-profiler/read-only-investigation/v1') {
        if ($Phase -ne 'Report') { throw 'Read-only investigations cannot approve, run or capture a candidate.' }
        if ([IO.Path]::GetFullPath($context.directory) -ne $Directory) { throw 'Mismatched read-only report directory.' }
        foreach ($reference in @($context.sourceInvestigation, $context.baseline, $context.analysis,
            $context.recommendations, $context.provenance)) {
            Assert-FileHash $reference.path $reference.sha256
        }
        if ($context.PSObject.Properties['knowledgeRepository'] -and $context.knowledgeRepository) {
            Assert-InvestigationKnowledgeSource $context.knowledgeRepository
        }
        if ($context.PSObject.Properties['supplementalEvidence'] -and $context.supplementalEvidence) {
            Assert-FileHash $context.supplementalEvidence.path $context.supplementalEvidence.sha256
            [void](Get-SupplementalEvidenceBinding $context.supplementalEvidence.path $context.baseline.path `
                (Read-JsonFile $context.baseline.path))
        }
        if ($context.PSObject.Properties['controlAnalysis'] -and $context.controlAnalysis) {
            Assert-FileHash $context.controlAnalysis.path $context.controlAnalysis.sha256
            if (!$Python) { $Python = (Get-Command python -ErrorAction Stop).Path }
            [void](Invoke-LoggedProcess $Python @("$plugin\pack\tools\Analyze-Profiler.py", 'verify', '--manifest',
                (Join-Path (Split-Path $context.controlAnalysis.path -Parent) 'manifest.json')) `
                $Directory (Join-Path $Directory 'control-report-verify.log') 120)
        }
        Assert-RecommendationReport $context | ConvertTo-Json -Depth 60
        return
    }
    if ($context.schema -ne 'ui-profiler/investigation/v3') {
        if ($Phase -ne 'Verify') { throw 'Use the original sealed adapter for an older investigation; do not expand its approved capture budgets.' }
        $run = Read-JsonFile (Join-Path $Directory 'ade-run.json')
        & "$PSScriptRoot\ade.ps1" verify --run $run.runId --runs-dir $runs
        if ($LASTEXITCODE -ne 0) { throw 'ADE event-chain verification failed.' }
        return
    }
    if ([IO.Path]::GetFullPath($context.directory) -ne $Directory) { throw 'Mismatched investigation directory.' }
    Assert-FileHash $context.module.path $context.module.sha256
    Import-Module $context.module.path -Force -DisableNameChecking
    Assert-FileHash $context.request.path $context.request.sha256
    Assert-FileHash $context.contracts.path $context.contracts.sha256
    Assert-FileHash $context.recommendations.path $context.recommendations.sha256
    Assert-FileHash $context.report.path $context.report.sha256
    Assert-FrozenScenario $context
    Assert-OriginalSource $context
    if ($Phase -eq 'Report') {
        Assert-RecommendationReport $context | ConvertTo-Json -Depth 60
        return
    }
    $selected = if ($RecommendationId.Count) {
        foreach ($id in $RecommendationId) {
            $matches = @($context.trials | Where-Object id -eq $id)
            if ($matches.Count -ne 1) { throw "No prepared independent trial for recommendation: $id" }
            $matches[0]
        }
    } else {
        if ($Phase -eq 'Run') { throw 'Select the approved recommendation ids with -RecommendationId; preparation is not execution approval.' }
        @($context.trials)
    }
    $lock = $null
    $lockPath = Join-Path $Directory 'operation.lock'
    try {
        try { $lock = [IO.File]::Open($lockPath, [IO.FileMode]::CreateNew, [IO.FileAccess]::Write, [IO.FileShare]::None) }
        catch [IO.IOException] { throw 'Another investigation operation is active or interrupted; no automatic lock reset.' }
        foreach ($reference in $selected) {
            $trial = Assert-TrialReference $context $reference
            Assert-TrialContext $trial
            $trialRuns = Join-Path $reference.directory 'ade-runs'
            if ($Phase -eq 'Verify') {
                Assert-FileHash $trial.adeLauncher $trial.adeLauncherSha256
                & $trial.adeLauncher verify --run $reference.runId --runs-dir $trialRuns
                if ($LASTEXITCODE -ne 0) { throw 'ADE event-chain verification failed.' }
                continue
            }
            $outcomePath = Join-Path $reference.directory 'outcome.json'
            if (Test-Path -LiteralPath $outcomePath) { throw "Recommendation $($reference.id) already has an outcome; no repeated coding or capture." }
            $approvalPath = Join-Path (Join-Path $trialRuns $reference.runId) 'approval.json'
            if (!(Test-Path -LiteralPath $approvalPath) -or
                (Read-JsonFile $approvalPath).proposalSha256 -ne $reference.proposalSha256) {
                throw "Approve recommendation $($reference.id)'s exact proposal hash before execution. This trial remains unspent."
            }
            $outcome = @{
                schema = 'ui-profiler/trial-outcome/v1'; recommendationId = $reference.id; state = 'failed'
                baselineSha256 = $context.baseline.sha256; contextSha256 = $reference.context.sha256
                proposalSha256 = $reference.proposalSha256; adeRun = $reference.runId
                adeChecksPassed = $false; chainVerified = $false; originalRepositoryUnchanged = $false
                comparison = $null; counts = $null; error = $null
            }
            try {
                if ($GitHubAccount) { $trial.githubAccount = $GitHubAccount }
                $result = Invoke-AdeRuntime @('run', '--run', $reference.runId, '--runs-dir', $trialRuns) $trial
                if ($result.stopReason -ne 'Passed') { throw 'Required ADE checks did not pass.' }
                $outcome.adeChecksPassed = $true
                [void](Invoke-AdeRuntime @('verify', '--run', $reference.runId, '--runs-dir', $trialRuns) $trial)
                $outcome.chainVerified = $true
                $comparisonDirectory = Join-Path $reference.directory 'multidimensional'
                $comparison = Compare-Measurements $context.baseline.path `
                    (Join-Path $reference.directory 'candidate\measurement.json') $comparisonDirectory $context.profilerCli
                Assert-OriginalSource $context
                $outcome.originalRepositoryUnchanged = $true
                $outcome.state = 'verified-comparison'
                $comparisonPath = Join-Path $comparisonDirectory 'comparison.json'
                $outcome.comparison = @{ path = $comparisonPath; sha256 = Get-Sha256 $comparisonPath }
                $outcome.counts = $comparison.counts
            } catch {
                $outcome.error = $_.Exception.Message
            } finally {
                Write-JsonFile $outcomePath $outcome
                Assert-OriginalSource $context
                $report = Write-RecommendationReport $context
                $context.report = @{ path = $report.path; sha256 = $report.sha256 }
                Write-JsonFile $contextPath $context
            }
        }
        if ($Phase -eq 'Run') {
            $report = Read-JsonFile $context.report.path
            $pending = @($report.rows | Where-Object { $_.verdict -eq 'propose' -and $_.state -notin @('verified-comparison', 'failed') })
            if (!$pending.Count) { $context.state = 'completed'; Write-JsonFile $contextPath $context }
            $failed = @($report.rows | Where-Object state -eq 'failed')
            Write-JsonFile (Join-Path $Directory 'completion.json') @{
                schema = 'ui-profiler/completion/v2'; state = if ($failed.Count) { 'failed-trials' } elseif ($pending.Count) { 'partial' } else { 'verified-comparisons' }
                report = $context.report; strategy = 'independent'; originalRepositoryUnchanged = $true
                interpretation = 'All recommendation outcomes are retained. Each measured variant is compared with the original baseline, not combined or assumed faster.'
            }
            $report | ConvertTo-Json -Depth 60
            if ($failed.Count) { throw 'One or more independent trials failed; the report retains them and no overall success is claimed.' }
        }
    } finally {
        if ($lock) { $lock.Dispose(); Remove-Item -LiteralPath $lockPath }
    }
    return
}
foreach ($required in @('ApplicationRepository', 'Scenario', 'KnowledgeRepository', 'ExpertHome', 'ProfilerCli')) {
    if ([string]::IsNullOrWhiteSpace((Get-Variable $required -ValueOnly))) { throw "Prepare requires -$required" }
}
if ($CaptureBackend -eq 'Gui' -and !$ProfilerGui) {
    throw 'GUI capture requires -ProfilerGui with the existing WinUIProfiler.App.exe. There is no CLI capture fallback.'
}
if (!(Test-Path variable:Python) -or !$Python) { $Python = (Get-Command python -ErrorAction Stop).Path }
& $Python -c 'from mcp.server.fastmcp import FastMCP; from mcp.types import ToolAnnotations'
if ($LASTEXITCODE -ne 0) { throw 'Install requirements.txt into an activated local Python virtual environment first.' }
if (!$Dotnet) { $Dotnet = (Get-Command dotnet -ErrorAction Stop).Path }
$request = [ordered]@{
    schema = 'ui-profiler/request/v2'; directory = $Directory
    strategy = 'independent'; candidatePolicy = 'one-per-approved-recommendation'
    applicationRepository = [IO.Path]::GetFullPath($ApplicationRepository); scenario = $Scenario
    knowledgeRepository = [IO.Path]::GetFullPath($KnowledgeRepository)
    expertHome = [IO.Path]::GetFullPath($ExpertHome); profilerCli = [IO.Path]::GetFullPath($ProfilerCli)
    captureBackend = $CaptureBackend; profilerGui = $ProfilerGui
    dotnet = [IO.Path]::GetFullPath($Dotnet); adeCopilotCli = $AdeCopilotCli; githubAccount = $GitHubAccount
    allowPreview = [bool]$AllowPreview; allowMeasurements = [bool]$AllowMeasurements
    contractsSha256 = Get-Sha256 "$plugin\CONTRACTS.md"
    intentPolicy = 'single-run/v1'
    intentCatalogSha256 = Get-Sha256 "$plugin\pack\knowledge\intent-catalog.md"
    targetControl = $TargetControl
}
Assert-RunDirectory $Directory @($request.applicationRepository, $request.knowledgeRepository,
    $request.expertHome, (Split-Path $request.profilerCli -Parent), (Split-Path $request.dotnet -Parent))
if (Test-Path -LiteralPath $Directory) { throw 'Use a fresh investigation directory; never reset an existing request or capture budget.' }
New-Item -ItemType Directory $Directory | Out-Null
$requestPath = Join-Path $Directory 'request.json'
Write-JsonFile $requestPath $request
$mcpPath = Join-Path $Directory 'harness-mcp.json'
Write-JsonFile $mcpPath @{
    mcpServers = @{
        'ui-profiler-harness' = @{
            type = 'local'; command = $Python
            args = @("$plugin\pack\tools\Harness-Mcp.py", '--request', $requestPath)
            tools = @('*'); timeout = 1800000
        }
    }
}
$clientHome = Join-Path $Directory 'controller-home'
New-Item -ItemType Directory (Join-Path $clientHome 'agents') | Out-Null
$skill = [IO.File]::ReadAllText("$plugin\skills\work\SKILL.md")
$skill = [regex]::Replace($skill, '(?s)\A---\r?\n.*?\r?\n---\r?\n', '')
$skill += "`n# Mandatory harness contracts`n`n" + [IO.File]::ReadAllText("$plugin\CONTRACTS.md")
$skill += "`n# Additive single-run intent catalog`n`n" +
    [IO.File]::ReadAllText("$plugin\pack\knowledge\intent-catalog.md")
Assert-FileHash "$plugin\CONTRACTS.md" $request.contractsSha256
$profile = "---`nname: ui-profiler-controller`ndescription: Plan a user scenario through bounded profiling tools.`ntools: [`"*`"]`n---`n`n$skill"
[IO.File]::WriteAllText((Join-Path $clientHome 'agents\ui-profiler-controller.agent.md'), $profile)
Write-JsonFile (Join-Path $clientHome 'settings.json') @{ customAgents = @{ defaultLocalOnly = $true }; disableAllHooks = $true }
$prompt = @"
Prepare this investigation using ui-profiler-harness MCP tools, not shell scripts or hand-authored fixtures.
The server is bound to the exact original request at $requestPath; read harness_contracts,
then begin with no arguments. Call harness operations sequentially, not in parallel.
User scenario: $Scenario
Application repository: $ApplicationRepository
Preview permission: $([bool]$AllowPreview). Measurement permission: $([bool]$AllowMeasurements).
Inspect source and the authorized unrecorded preview, choose build/steps/selectors, freeze, measure once,
analyze, invoke the actual expert, and propose through the tools. Respect all state/permission failures.
Before freezing, translate the user's intent into one focused question and the validated intent specification.
Use the additive intent catalog, discover the actual app/control and source; no built-in app-specific flow.
Keep one run per baseline/candidate, actual step count/waits, valid-run checks, relevant metrics and falsifiable
healthy result. Declare unsupported presented-frame/allocation/ownership telemetry, not substitute ticks.
Model/source reasoning is allowed; only the declared WinUIProfiler supplies measurements.
Keep the complete three-level ranked recommendation report. Call propose_fix once for EACH application/propose recommendation
id; each prepares a fresh isolated ADE coding subagent/trial starting from original source and reusing
the shared baseline. Do not combine fixes. Stop at the per-proposal exact-hash human approval boundary.
Never approve, run, apply, edit source or invoke another
agent yourself. If inputs/permissions/evidence are insufficient, stop and state what blocked preparation.
"@
$saved = @{}
foreach ($name in @('COPILOT_HOME', 'COPILOT_AUTO_UPDATE', 'COPILOT_GITHUB_TOKEN', 'PATH')) {
    $saved[$name] = [Environment]::GetEnvironmentVariable($name)
}
try {
    $env:PATH = "$(Split-Path $Python -Parent);$env:PATH"
    $env:COPILOT_HOME = $clientHome
    $env:COPILOT_AUTO_UPDATE = 'false'
    if ($GitHubAccount) {
        $token = gh auth token --hostname github.com --user $GitHubAccount
        if ($LASTEXITCODE -ne 0) { throw 'The selected existing GitHub credential is unavailable.' }
        $env:COPILOT_GITHUB_TOKEN = $token.Trim()
        $token = $null
    }
    $cli = (Get-Command copilot -ErrorAction Stop).Path
    $arguments = @('--agent', 'ui-profiler-controller', '--prompt', $prompt, '--no-color',
        '--output-format', 'json', '--stream', 'off', '--no-custom-instructions', '--disable-builtin-mcps',
        '--additional-mcp-config', "@$mcpPath", '--allow-tool', 'ui-profiler-harness', '--allow-tool', 'read',
        '--deny-tool', 'shell', '--deny-tool', 'write', '--add-dir', $Directory,
        '--add-dir', $request.applicationRepository, '--add-dir', $request.knowledgeRepository)
    if ([IO.Path]::GetExtension($cli) -eq '.ps1') {
        $arguments = @('-NoProfile', '-NonInteractive', '-File', $cli) + $arguments
        $cli = (Get-Command pwsh -ErrorAction Stop).Path
    }
    $events = Invoke-LoggedProcess $cli $arguments $Directory (Join-Path $Directory 'controller.log') 1800
    [IO.File]::WriteAllText((Join-Path $Directory 'controller-events.jsonl'), $events)
    if (!(Test-Path -LiteralPath $contextPath)) {
        throw 'The planner stopped before preparing an investigation; no success is claimed.'
    }
    $context = Read-JsonFile $contextPath
    if (!$context.report) { throw 'The planner stopped without the required recommendation report; inspect diagnostics.' }
    $report = Read-JsonFile $context.report.path
    $pending = @($report.rows | Where-Object { $_.verdict -eq 'propose' -and $_.state -eq 'pending' })
    if ($pending.Count) { throw 'The report retains recommendations with proposals still pending; continue preparation through propose_fix, not another baseline recording.' }
    @{ report = $context.report; trials = $context.trials
        status = if ($context.trials.Count) { 'AwaitingApproval' }
            elseif (@($report.rows | Where-Object verdict -eq 'recommend').Count) { 'RecommendationsReady' }
            else { 'InsufficientEvidence' }
    } | ConvertTo-Json -Depth 20
} finally {
    foreach ($name in $saved.Keys) { [Environment]::SetEnvironmentVariable($name, $saved[$name]) }
}
