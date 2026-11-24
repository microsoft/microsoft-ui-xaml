# Private deterministic adapters for the agent's investigation tools; no approval or coding operation.
#Requires -Version 7
param(
    [Parameter(Mandatory)][ValidateSet('Initialize', 'Inspect', 'Preview', 'InspectUi', 'Act', 'Freeze',
        'ClosePreview', 'Measure', 'Analyze', 'Recommend', 'Propose', 'Status', 'Contracts', 'Report')][string]$Operation,
    [Parameter(Mandatory)][string]$ArgumentsPath
)
$ErrorActionPreference = 'Stop'
$a = Get-Content -LiteralPath $ArgumentsPath -Raw | ConvertFrom-Json -AsHashtable
$directory = [IO.Path]::GetFullPath($a.directory)
$contextPath = Join-Path $directory 'context.json'
if ($Operation -ne 'Initialize') {
    if (!(Test-Path -LiteralPath $contextPath -PathType Leaf)) { throw 'Begin an investigation before calling this tool.' }
    $metadata = Get-Content -LiteralPath $contextPath -Raw | ConvertFrom-Json
    $tools = Join-Path $directory 'pack\tools'
    foreach ($name in @('adapter', 'module')) {
        $path = Join-Path $tools $(if ($name -eq 'adapter') { 'Invoke-HarnessTool.ps1' } else { 'ProfilerHarness.psm1' })
        if (!$metadata.$name.sha256 -or $metadata.$name.path -ne $path -or
            (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant() -ne $metadata.$name.sha256) {
            throw "The frozen harness $name changed; no operation is permitted."
        }
    }
    if ($PSScriptRoot -ne $tools) {
        & (Join-Path $tools 'Invoke-HarnessTool.ps1') -Operation $Operation -ArgumentsPath $ArgumentsPath
        return
    }
}
Import-Module "$PSScriptRoot\ProfilerHarness.psm1" -Force -DisableNameChecking
$failurePath = Join-Path $directory 'last-failure.json'
$plugin = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$python = (Get-Command python -ErrorAction Stop).Path
$lock = $null
$lockPath = Join-Path $directory 'operation.lock'
$context = $null

function Save-Context { Write-JsonFile $contextPath $script:context }
function Get-Projects { @($script:files | Where-Object { $_ -like '*.csproj' }) }
function Require-Preview {
    Assert-InvestigationState $script:context @('planning')
    if (!$script:request.allowPreview) { throw 'The user did not authorize preview interaction.' }
    if (!$script:context.preview -or $script:context.preview.closed) { throw 'Launch the owned unrecorded preview first.' }
    if ([DateTime]::Parse($script:context.preview.openedUtc).ToUniversalTime().AddMinutes(10) -lt [DateTime]::UtcNow) {
        Close-OwnedApplication $script:context.preview.identity $script:context.preview.executable
        $script:context.preview.closed = $true
        Save-Context
        throw 'The ten-minute preview expired; no further UI action is permitted.'
    }
}
function Invoke-PreviewUi([string]$PlanPath, [string]$OutputPath, [switch]$Inspect) {
    Require-Preview
    Assert-InteractiveDesktop
    $arguments = @('-NoProfile', '-NonInteractive', '-File', $script:context.scenarioScript,
        '-ProcessId', [string]$script:context.preview.identity.processId,
        '-CreationTimeFileTime', [string]$script:context.preview.identity.creationTimeFileTime,
        '-ScenarioPath', $PlanPath, '-OutputPath', $OutputPath,
        '-ExpectedExecutable', $script:context.preview.executable)
    if ($Inspect) { $arguments += '-Inspect' }
    [void](Invoke-LoggedProcess 'powershell.exe' $arguments $directory "$OutputPath.log" 90)
    Read-JsonFile $OutputPath
}
function Close-Preview {
    if ($script:context.preview -and !$script:context.preview.closed) {
        Close-OwnedApplication $script:context.preview.identity $script:context.preview.executable
        $script:context.preview.closed = $true
        Save-Context
    }
}

if ($Operation -eq 'Initialize') {
    $request = $a.request
    foreach ($key in @('applicationRepository', 'scenario', 'knowledgeRepository', 'expertHome', 'profilerCli')) {
        if ([string]::IsNullOrWhiteSpace($request[$key])) { throw "Required user input: $key" }
    }
    if ($request.scenario.Length -gt 8000) { throw 'The natural-language scenario is too long.' }
    foreach ($key in @('applicationRepository', 'knowledgeRepository', 'expertHome', 'profilerCli')) {
        if (![IO.Path]::IsPathFullyQualified($request[$key]) -or !(Test-Path -LiteralPath $request[$key])) {
            throw "Supply an existing absolute input path: $key"
        }
        $request[$key] = [IO.Path]::GetFullPath($request[$key])
    }
    if (!$request.dotnet) { $request.dotnet = (Get-Command dotnet -ErrorAction Stop).Path }
    foreach ($key in @('adeCopilotCli', 'githubAccount')) {
        if (!$request.ContainsKey($key)) { $request[$key] = '' }
    }
    foreach ($key in @('allowPreview', 'allowMeasurements')) {
        if (!$request.ContainsKey($key)) { $request[$key] = $false }
        if ($request[$key] -isnot [bool]) { throw "A boolean authorization is required: $key" }
    }
    if ($request.ContainsKey('strategy') -and $request.strategy -ne 'independent') { throw 'Only independent trials are supported.' }
    if ($request.ContainsKey('candidatePolicy') -and $request.candidatePolicy -ne 'one-per-approved-recommendation') {
        throw 'The recorded candidate authorization differs from the independent trial policy.'
    }
    $request.schema = 'ui-profiler/request/v2'
    $request.strategy = 'independent'
    $request.candidatePolicy = 'one-per-approved-recommendation'
    $request.directory = $directory
    if (!$request.contractsSha256) { $request.contractsSha256 = Get-Sha256 (Join-Path $plugin 'CONTRACTS.md') }
    Assert-FileHash (Join-Path $plugin 'CONTRACTS.md') $request.contractsSha256
    Assert-RunDirectory $directory @($request.applicationRepository, $request.knowledgeRepository,
        $request.expertHome, (Split-Path $request.profilerCli -Parent), (Split-Path $request.dotnet -Parent))
    foreach ($relative in @('winui-expert.agent.md', 'rag\rag.py')) {
        if (!(Test-Path -LiteralPath (Join-Path $request.expertHome $relative) -PathType Leaf)) {
            throw 'The actual WinUI expert profile and local RAG bundle are required.'
        }
    }
    $status = @(git -C $request.applicationRepository status --porcelain)
    if ($LASTEXITCODE -ne 0 -or $status.Count -ne 0) { throw 'Use a clean application Git repository; no changes are discarded.' }
    [void](New-ProfilerMcpConfiguration $request.knowledgeRepository $request.dotnet)
    New-Item -ItemType Directory -Force $directory | Out-Null
} elseif (!(Test-Path -LiteralPath $contextPath -PathType Leaf)) {
    throw 'Begin an investigation before calling this tool.'
}

try {
    try { $lock = [IO.File]::Open($lockPath, [IO.FileMode]::CreateNew, [IO.FileAccess]::Write, [IO.FileShare]::None) }
    catch [IO.IOException] { throw 'Another operation is active or was interrupted. No automatic retry or lock reset.' }
    if ($Operation -ne 'Initialize') {
        $context = Read-JsonFile $contextPath
        if ($context.schema -ne 'ui-profiler/investigation/v3' -or $context.directory -ne $directory) {
            throw 'Unsupported or mismatched investigation context.'
        }
        Assert-FileHash $context.request.path $context.request.sha256
        Assert-FileHash $context.contracts.path $context.contracts.sha256
        Assert-FileHash $context.scenarioScript $context.scenarioScriptSha256
        if ($context.recommendations) { Assert-FileHash $context.recommendations.path $context.recommendations.sha256 }
        if ($context.report) { Assert-FileHash $context.report.path $context.report.sha256 }
        if ($context.nativeMcp) {
            Assert-FileHash $context.nativeMcp.path $context.nativeMcp.sha256
            if ((Get-ProfilerHash $context.nativeMcp.path) -ne $context.nativeMcp.deploymentSha256) {
                throw 'The frozen native MCP deployment changed.'
            }
            if ((Get-SourceHash $context.nativeMcp.source $context.nativeMcp.files) -ne $context.nativeMcp.sourceSha256) {
                throw 'The private native MCP source snapshot changed.'
            }
        }
        $request = Read-JsonFile $context.request.path
        $files = @(Get-RepositoryFiles $context.originalRepository)
        if ($Operation -ne 'Status') { Assert-OriginalSource $context }
        if ($context.scenarioPath) { Assert-FrozenScenario $context }
    }
    $data = switch ($Operation) {
        'Contracts' {
            @{ path = $context.contracts.path; sha256 = $context.contracts.sha256
                text = Get-Content -LiteralPath $context.contracts.path -Raw }
        }
        'Initialize' {
            if (Test-Path -LiteralPath $contextPath) { throw 'This directory already has an investigation; use its status, not a second request.' }
            $files = @(Get-RepositoryFiles $request.applicationRepository)
            $sourceHash = Get-SourceHash $request.applicationRepository $files
            $requestPath = Join-Path $directory 'request.json'
            if (Test-Path -LiteralPath $requestPath) {
                $existing = Get-Content -LiteralPath $requestPath -Raw | ConvertFrom-Json -AsHashtable
                foreach ($key in $request.Keys) {
                    if (($request[$key] | ConvertTo-Json -Compress) -cne ($existing[$key] | ConvertTo-Json -Compress)) {
                        throw "The recorded user request differs: $key"
                    }
                }
            } else { Write-JsonFile $requestPath $request }
            $pack = Join-Path $directory 'pack'
            Copy-ApplicationSource $request.applicationRepository (Join-Path $directory 'baseline-source') $files
            Copy-Item -LiteralPath (Split-Path $PSScriptRoot -Parent) -Destination $pack -Recurse
            $contractsPath = Join-Path $pack 'knowledge\harness-contracts.md'
            Copy-Item -LiteralPath (Join-Path $plugin 'CONTRACTS.md') -Destination $contractsPath
            Assert-FileHash $contractsPath $request.contractsSha256
            $script = Join-Path $pack 'tools\Invoke-Scenario.ps1'
            $launcher = Join-Path $directory 'scripts\ade.ps1'
            New-Item -ItemType Directory (Split-Path $launcher -Parent) | Out-Null
            Copy-Item -LiteralPath (Join-Path $plugin 'scripts\ade.ps1') -Destination $launcher
            $context = [pscustomobject]@{
                schema = 'ui-profiler/investigation/v3'; state = 'planning'; directory = $directory
                strategy = 'independent'
                request = @{ path = $requestPath; sha256 = Get-Sha256 $requestPath }
                contracts = @{ path = $contractsPath; sha256 = Get-Sha256 $contractsPath }
                adapter = @{ path = Join-Path $pack 'tools\Invoke-HarnessTool.ps1'
                    sha256 = Get-Sha256 (Join-Path $pack 'tools\Invoke-HarnessTool.ps1') }
                module = @{ path = Join-Path $pack 'tools\ProfilerHarness.psm1'
                    sha256 = Get-Sha256 (Join-Path $pack 'tools\ProfilerHarness.psm1') }
                originalRepository = $request.applicationRepository; originalSourceSha256 = $sourceHash
                originalRevision = (git -C $request.applicationRepository rev-parse HEAD)
                scenario = $null; scenarioPath = $null; scenarioSha256 = $null
                scenarioScript = $script; scenarioScriptSha256 = Get-Sha256 $script
                profilerCli = $request.profilerCli; profilerSha256 = Get-ProfilerHash $request.profilerCli
                dotnet = $request.dotnet; adeCopilotCli = $request.adeCopilotCli; githubAccount = $request.githubAccount
                adeLauncher = $launcher; adeLauncherSha256 = Get-Sha256 $launcher
                baseline = $null; analysis = $null; expert = $null; preview = $null; protectedPaths = @()
                nativeMcp = $null
                recommendations = $null; trials = @(); report = $null
            }
            Save-Context
            @{ directory = $directory; userScenario = $request.scenario; projects = @(Get-Projects)
                requestSha256 = $context.request.sha256; sourceSha256 = $sourceHash
                allowPreview = $request.allowPreview; allowMeasurements = $request.allowMeasurements }
        }
        'Inspect' {
            @{ projects = @(Get-Projects); source = Join-Path $directory 'baseline-source'
                instructions = @($files | Where-Object { $_ -match '(^|/)(AGENTS\.md|copilot-instructions\.md)$' })
                scenario = $request.scenario; buildAdapter = 'unpackaged self-contained .NET WinUI, win-x64'
                actions = @('type-text', 'keys', 'invoke', 'assert', 'expand', 'collapse') }
        }
        'Preview' {
            Assert-InvestigationState $context @('planning')
            if (!$request.allowPreview) { throw 'The user did not authorize an unrecorded UI preview.' }
            if ($context.preview) { throw 'Only one preview launch is permitted; do not silently replay a scenario.' }
            if ($a.build.project.Replace('\', '/') -notin @(Get-Projects)) { throw 'The selected project is not tracked.' }
            Assert-InteractiveDesktop
            $source = Join-Path $directory 'preview-source'
            Copy-ApplicationSource $context.originalRepository $source $files
            $previewDirectory = Join-Path $directory 'preview'
            $previewPlan = @{ build = $a.build; window = $a.window; steps = @() }
            $context.scenario = $previewPlan
            $executable = Build-Application $context $source (Join-Path $previewDirectory 'app')
            $context.scenario = $null
            if ((Get-SourceHash $source $files) -ne $context.originalSourceSha256) { throw 'Preview build changed tracked source.' }
            $info = [Diagnostics.ProcessStartInfo]::new($executable)
            $info.WorkingDirectory = Split-Path $executable -Parent
            $info.UseShellExecute = $true
            $process = [Diagnostics.Process]::Start($info)
            try {
                $context.preview = @{
                    identity = @{ processId = $process.Id; creationTimeFileTime = $process.StartTime.ToUniversalTime().ToFileTimeUtc() }
                    executable = $executable; openedUtc = [DateTime]::UtcNow.ToString('O'); closed = $false
                    build = $a.build; window = $a.window; actions = @(); inspections = @()
                }
            } finally { $process.Dispose() }
            Save-Context
            $previewPath = Join-Path $previewDirectory 'scenario.json'
            Write-JsonFile $previewPath $previewPlan
            Save-Context
            @{ process = $context.preview.identity; executable = $executable; recorded = $false; expiresMinutes = 10 }
        }
        'InspectUi' {
            Require-Preview
            if ($context.preview.inspections.Count -ge 64) { throw 'The preview inspection limit is spent.' }
            $path = Join-Path $directory "preview\inspection-$($context.preview.inspections.Count).json"
            $observation = Invoke-PreviewUi (Join-Path $directory 'preview\scenario.json') $path -Inspect
            $context.preview.inspections += @{ path = $path; sha256 = Get-Sha256 $path }
            Save-Context
            $observation
        }
        'Act' {
            Require-Preview
            if ($context.preview.actions.Count -ge 64) { throw 'The preview action limit is spent.' }
            $ordinal = $context.preview.actions.Count
            $planPath = Join-Path $directory "preview\step-$ordinal.json"
            $auditPath = Join-Path $directory "preview\action-$ordinal.json"
            Write-JsonFile $planPath @{ build = $context.preview.build; window = $context.preview.window; steps = @($a.step) }
            $context.preview.actions += @{ path = $auditPath; state = 'reserved' }
            Save-Context
            $audit = Invoke-PreviewUi $planPath $auditPath
            if ($audit.state -ne 'complete') { throw 'The preview step did not pass.' }
            $context.preview.actions[$ordinal] = @{ path = $auditPath; sha256 = Get-Sha256 $auditPath; state = 'complete' }
            Save-Context
            $audit
        }
        'Freeze' {
            Require-Preview
            foreach ($name in @('project', 'configuration', 'executable')) {
                if ($a.plan.build[$name] -ne $context.preview.build.$name) { throw 'Freeze must use the previewed build.' }
            }
            foreach ($name in @('width', 'height')) {
                if ($a.plan.window[$name] -ne $context.preview.window.$name) { throw 'Freeze must use the previewed window size.' }
            }
            $observations = @(
                foreach ($inspection in $context.preview.inspections) {
                    Assert-FileHash $inspection.path $inspection.sha256
                    Read-JsonFile $inspection.path
                }
            )
            $bundle = Join-Path $directory 'plan-inputs.json'
            Write-JsonFile $bundle @{ plan = $a.plan; request = $request; projects = @(Get-Projects); observations = $observations }
            $validated = Invoke-LoggedProcess $python @("$PSScriptRoot\Harness-Mcp.py", '--validate-plan', $bundle) `
                $directory (Join-Path $directory 'plan-validation.log') 30
            $plan = $validated | ConvertFrom-Json
            $path = Join-Path $directory 'scenario.json'
            if (Test-Path -LiteralPath $path) { throw 'A frozen scenario cannot be overwritten.' }
            Close-Preview
            Write-JsonFile $path $plan
            $context.scenario = $plan; $context.scenarioPath = $path; $context.scenarioSha256 = Get-Sha256 $path
            $context.state = 'planned'
            Save-Context
            @{ scenarioPath = $path; sha256 = $context.scenarioSha256; scenario = $plan; recorded = $false }
        }
        'ClosePreview' {
            Close-Preview
            @{ closed = $true }
        }
        'Measure' {
            Assert-InvestigationState $context @('planned')
            if (!$request.allowMeasurements) { throw 'The user did not authorize the baseline and independently approved candidate trials.' }
            Assert-FrozenScenario $context
            if ((Get-SourceHash (Join-Path $directory 'baseline-source') $files) -ne $context.originalSourceSha256) {
                throw 'The baseline source copy changed before measurement.'
            }
            Invoke-AdeRuntime @() $context -Probe
            if (!$context.nativeMcp) {
                $context.nativeMcp = Build-ProfilerMcp $context $request.knowledgeRepository
                Save-Context
            }
            $preflightPath = Join-Path $directory 'mcp-preflight.json'
            Write-JsonFile $preflightPath (New-ProfilerMcpConfiguration $request.knowledgeRepository $context.dotnet `
                -NativeAssembly $context.nativeMcp.path)
            [void](Invoke-LoggedProcess $python @("$PSScriptRoot\Harness-Mcp.py", '--probe-mcp', $preflightPath) `
                $directory (Join-Path $directory 'mcp-preflight.log') 120)
            $measurement = Invoke-Measurement $context (Join-Path $directory 'baseline-source') 'baseline' $files
            Assert-OriginalSource $context
            $path = Join-Path $directory 'baseline\measurement.json'
            $context.baseline = @{ path = $path; sha256 = Get-Sha256 $path }
            $context.state = 'measured'
            Save-Context
            @{ measurement = $context.baseline; metrics = $measurement.metrics; health = $measurement.health }
        }
        'Analyze' {
            Assert-InvestigationState $context @('measured')
            Assert-FileHash $context.baseline.path $context.baseline.sha256
            $context.analysis = Export-MeasurementAnalysis $context.baseline.path (Join-Path $directory 'baseline-analysis') $context.profilerCli
            $context.state = 'analyzed'
            Save-Context
            @{ analysis = $context.analysis; summary = Join-Path $directory 'baseline-analysis\analysis.md' }
        }
        'Recommend' {
            Assert-InvestigationState $context @('analyzed')
            $expertDirectory = Join-Path $directory 'expert'
            $mcpPath = Join-Path $directory 'profiler-mcp.json'
            Write-JsonFile $mcpPath (New-ProfilerMcpConfiguration $request.knowledgeRepository $context.dotnet `
                $context.analysis.path (Join-Path $directory 'pack\tools\Evidence-Mcp.py') $context.nativeMcp.path)
            $arguments = @{
                ExpertHome = $request.expertHome; KnowledgeRepository = $request.knowledgeRepository
                ApplicationSource = Join-Path $directory 'baseline-source'; MeasurementPath = $context.baseline.path
                AnalysisPath = $context.analysis.path; Directory = $expertDirectory; GitHubAccount = $context.githubAccount
                ScenarioPath = $context.scenarioPath; McpConfigPath = $mcpPath
                ContractsPath = $context.contracts.path
            }
            [void](& "$PSScriptRoot\Get-Recommendation.ps1" @arguments)
            $path = Join-Path $expertDirectory 'provenance.json'
            $context.expert = @{ path = $path; sha256 = Get-Sha256 $path }
            $recommendationsPath = Join-Path $expertDirectory 'recommendation.json'
            $context.recommendations = @{ path = $recommendationsPath; sha256 = Get-Sha256 $recommendationsPath }
            $recommendations = Read-JsonFile $recommendationsPath
            $context.state = if (@($recommendations.recommendations | Where-Object verdict -eq 'propose').Count) {
                'recommended'
            } else { 'completed' }
            $report = Write-RecommendationReport $context
            $context.report = @{ path = $report.path; sha256 = $report.sha256 }
            Save-Context
            @{ report = $context.report; provenance = $context.expert
                recommendations = @($report.rows | ForEach-Object {
                    @{ id = $_.id; title = $_.title; verdict = $_.verdict; state = $_.state; fileCount = $_.files.Count }
                }) }
        }
        'Propose' {
            Assert-InvestigationState $context @('recommended', 'awaiting-approval')
            Assert-FileHash $context.baseline.path $context.baseline.sha256
            Assert-Measurement (Read-JsonFile $context.baseline.path)
            Assert-FileHash $context.analysis.path $context.analysis.sha256
            Assert-FileHash $context.expert.path $context.expert.sha256
            $expert = Read-JsonFile $context.expert.path
            $recommendationPath = Join-Path $directory 'expert\recommendation.md'
            Assert-FileHash $recommendationPath $expert.recommendationSha256
            if ($expert.agent -ne 'winui-expert' -or $expert.analysisSha256 -ne $context.analysis.sha256 -or
                @($expert.mcpQueries).Count -lt 2) { throw 'A real, evidence-bound WinUI expert MCP recommendation is required.' }
            Assert-FileHash $expert.structuredRecommendation.path $expert.structuredRecommendation.sha256
            if ($expert.structuredRecommendation.sha256 -ne $context.recommendations.sha256) {
                throw 'The report differs from the actual expert output.'
            }
            $recommendations = Read-JsonFile $context.recommendations.path
            $entries = @($recommendations.recommendations | Where-Object id -eq $a.recommendationId)
            if ($entries.Count -ne 1 -or $entries[0].verdict -ne 'propose') {
                throw 'Select an actionable id from the frozen expert recommendation report.'
            }
            $entry = $entries[0]
            $trialDirectory = Join-Path (Join-Path $directory 'trials') $entry.id
            if (Test-Path -LiteralPath $trialDirectory) {
                throw 'This recommendation already has a proposed, partial or spent trial; do not overwrite or retry it.'
            }
            $pack = Join-Path $trialDirectory 'pack'
            New-Item -ItemType Directory -Force $pack | Out-Null
            foreach ($name in @('tools', 'knowledge', 'skills')) {
                Copy-Item -LiteralPath (Join-Path (Join-Path $directory 'pack') $name) -Destination $pack -Recurse
            }
            Copy-Item -LiteralPath (Join-Path $directory 'pack\ade.team.json') -Destination $pack
            Write-JsonFile (Join-Path $pack 'mcp.json') (New-ProfilerMcpConfiguration $request.knowledgeRepository `
                $context.dotnet $context.analysis.path (Join-Path $directory 'pack\tools\Evidence-Mcp.py') $context.nativeMcp.path)
            $allowed = @($entry.files | ForEach-Object { $_.path.Replace('\', '/') })
            $trial = $context | ConvertTo-Json -Depth 60 | ConvertFrom-Json -AsHashtable
            $trial.schema = 'ui-profiler/trial/v1'
            $trial.directory = $trialDirectory
            $trial.investigationDirectory = $directory
            $trial.recommendationId = $entry.id
            $trial.allowedPaths = @($entry.files | ForEach-Object path)
            $trial.protectedPaths = @('.github', 'tests', 'test') + @($files | Where-Object { $_ -notin $allowed })
            foreach ($name in @('trials', 'report', 'preview', 'adapter', 'state')) { $trial.Remove($name) }
            $trial.module = @{ path = Join-Path $pack 'tools\ProfilerHarness.psm1'
                sha256 = Get-Sha256 (Join-Path $pack 'tools\ProfilerHarness.psm1') }
            $trial.adeLauncher = Join-Path $trialDirectory 'scripts\ade.ps1'
            Assert-FileHash $context.adeLauncher $context.adeLauncherSha256
            New-Item -ItemType Directory (Split-Path $trial.adeLauncher -Parent) | Out-Null
            Copy-Item -LiteralPath $context.adeLauncher -Destination $trial.adeLauncher
            $trial.adeLauncherSha256 = Get-Sha256 $trial.adeLauncher
            $trialContextPath = Join-Path $pack 'knowledge\trial-context.json'
            Write-JsonFile $trialContextPath $trial
            $trial = Read-JsonFile $trialContextPath
            $cfg = Read-JsonFile (Join-Path $pack 'ade.team.json')
            $quotedContext = $trialContextPath.Replace("'", "''")
            $quotedCheck = (Join-Path $pack 'tools\Measure.ps1').Replace("'", "''")
            $cfg.checks = @(@{
                id = 'profile-scenario'
                command = "& '$quotedCheck' -RepositoryPath (Get-Location).Path -ContextPath '$quotedContext' -ContextSha256 '$(Get-Sha256 $trialContextPath)'"
                timeoutSeconds = 900
            })
            $cfg.protectedPaths = $trial.protectedPaths
            Write-JsonFile (Join-Path $pack 'ade.team.json') $cfg
            $workstreams = Join-Path $trialDirectory 'workstreams'
            $name = "trial-$($entry.id)"
            $goal = "Independently test recommendation $($entry.id): $($entry.title). Start from original source; compare one candidate with the shared frozen baseline."
            $ws = Invoke-AdeRuntime @('workstream', 'new', '--pack', $pack, '--name', $name,
                '--repo', $context.originalRepository, '--type', 'change', '--goal', $goal,
                '--workstreams-dir', $workstreams) $trial
            $notes = @"
# User request and immutable scenario

$($request.scenario)

Original: $($context.originalRepository) at $($context.originalRevision).
Scenario: $($context.scenarioPath), SHA-256 $($context.scenarioSha256).
Baseline: $($context.baseline.path), SHA-256 $($context.baseline.sha256).
Analysis: $($context.analysis.path), SHA-256 $($context.analysis.sha256).
Use the declared winui-profiler and ui-profiler-evidence MCPs for scoped read-only evidence queries.

$(Get-Content -LiteralPath (Join-Path $directory 'baseline-analysis\analysis.md') -Raw)

# Actual WinUI expert recommendation

$($entry | ConvertTo-Json -Depth 20)

# Constraints

Read the mandatory harness contracts: $($context.contracts.path) (SHA-256 $($context.contracts.sha256)).
Contract violations do not authorize relaxing a check, editing frozen inputs or taking another capture.

This is a fresh coding subagent/working copy for recommendation $($entry.id), independent of every
other trial. Start from original source; do not import another recommendation's patch or agent history.
Modify only these explicitly approved existing application C#/XAML files:
$($entry.files | ForEach-Object { "- $($_.path): $($_.change)" } | Out-String)
There is no arbitrary two-file cap. Preserve content, semantics and accessibility.
Do not edit tests, instructions, dependencies, project/build configuration or profiler/harness integration.
Do not build, launch, capture or invoke another agent in the coding session. Shell/URL access is disabled.
ADE verifies the identical frozen scenario after this subagent's sole coding round, with one
candidate capture for this recommendation. Every trial reuses the SAME original baseline recording.
Never combine trial fixes or profile concurrently with another trial.
A passing check means functional and comparable complete evidence, NOT that all dimensions improved.
Report regressions, missing capabilities and uncertainty; one pair is observational, not significance.
UI ticks are elapsed UI-thread time, not CPU work or presented frames. Automation waits remain included.
"@
            foreach ($instruction in @('AGENTS.md', '.github/copilot-instructions.md')) {
                if ($instruction -in $files) {
                    $notes += "`n# App instructions: $instruction`n" +
                        (Get-Content -LiteralPath (Join-Path (Join-Path $directory 'baseline-source') $instruction.Replace('/', '\')) -Raw)
                }
            }
            $notes += "`nRead additional scoped instructions before editing; current project values govern framework versions.`n"
            [IO.File]::WriteAllText((Join-Path $ws.workstream 'notes.md'), $notes)
            $result = Invoke-AdeRuntime @('propose', '--pack', $pack, '--repo', $context.originalRepository,
                '--type', 'change', '--task', $goal, '--workstream', $name, '--workstreams-dir', $workstreams,
                '--runs-dir', (Join-Path $trialDirectory 'ade-runs')) $trial
            if ($result.status -ne 'AwaitingApproval' -or $result.proposalSha256 -notmatch '^[a-f0-9]{64}$') {
                throw 'ADE did not produce an approvable exact-hash proposal.'
            }
            Assert-OriginalSource $context
            $proposalPath = Join-Path $trialDirectory 'ade-run.json'
            Write-JsonFile $proposalPath $result
            $context.trials = @($context.trials) + @(@{
                id = $entry.id; directory = $trialDirectory
                context = @{ path = $trialContextPath; sha256 = Get-Sha256 $trialContextPath }
                proposal = @{ path = $proposalPath; sha256 = Get-Sha256 $proposalPath }
                runId = $result.runId; proposalSha256 = $result.proposalSha256
            })
            $context.state = 'awaiting-approval'
            $report = Write-RecommendationReport $context
            $context.report = @{ path = $report.path; sha256 = $report.sha256 }
            Save-Context
            @{ recommendationId = $entry.id; proposal = $result; report = $context.report }
        }
        'Report' {
            if (!$context.report) { throw 'Obtain the expert recommendation report first.' }
            $report = Assert-RecommendationReport $context
            @{ report = $context.report; rows = @($report.rows | ForEach-Object {
                @{ id = $_.id; title = $_.title; state = $_.state; fileCount = $_.files.Count
                    comparison = if ($_.outcome) { $_.outcome.comparison } else { $null } }
            }) }
        }
        'Status' {
            @{ directory = $directory; scenarioSha256 = $context.scenarioSha256; baseline = $context.baseline
                contracts = $context.contracts
                recommendations = $context.recommendations; trials = $context.trials; report = $context.report
                budgets = @(
                    $path = Join-Path $directory 'baseline\capture-budget.json'
                    if (Test-Path -LiteralPath $path) { @{ role = 'baseline'; budget = Read-JsonFile $path } }
                    foreach ($trial in $context.trials) {
                        $path = Join-Path $trial.directory 'candidate\capture-budget.json'
                        if (Test-Path -LiteralPath $path) { @{ role = 'candidate'; recommendationId = $trial.id; budget = Read-JsonFile $path } }
                    }
                )
                lastFailure = if (Test-Path -LiteralPath $failurePath) { Read-JsonFile $failurePath } else { $null } }
        }
    }
    Write-Output ('UI-PROFILER-RESULT: ' + (@{
        schema = 'ui-profiler/tool-result/v1'; operation = $Operation; state = $context.state; data = $data
    } | ConvertTo-Json -Depth 60 -Compress))
} catch {
    if ($null -ne $lock) {
        if ($Operation -eq 'Preview' -and $context -and $context.preview -and !$context.preview.closed) {
            Close-Preview
        }
        Write-JsonFile $failurePath @{ operation = $Operation; error = $_.Exception.Message; utc = [DateTime]::UtcNow.ToString('O') }
    }
    throw
} finally {
    if ($null -ne $lock) {
        $lock.Dispose()
        Remove-Item -LiteralPath $lockPath
    }
}
