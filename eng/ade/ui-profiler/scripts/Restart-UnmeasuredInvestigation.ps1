# Continue a validated, never-recorded scenario with a repaired orchestration client.
#Requires -Version 7
param(
    [Parameter(Mandatory)][string]$SourceInvestigation,
    [Parameter(Mandatory)][string]$Directory,
    [Parameter(Mandatory)][string]$AdeCopilotCli,
    [string]$Python = (Get-Command python -ErrorAction Stop).Path
)
$ErrorActionPreference = 'Stop'
$plugin = Split-Path $PSScriptRoot -Parent
Import-Module "$plugin\pack\tools\ProfilerHarness.psm1" -Force -DisableNameChecking
$SourceInvestigation = [IO.Path]::GetFullPath($SourceInvestigation)
$Directory = [IO.Path]::GetFullPath($Directory)
$sourcePath = Join-Path $SourceInvestigation 'context.json'
$sourceHash = Get-Sha256 $sourcePath
$source = Read-JsonFile $sourcePath
if ($source.schema -ne 'ui-profiler/investigation/v3' -or
    [IO.Path]::GetFullPath($source.directory) -ne $SourceInvestigation -or
    $source.state -ne 'planned' -or $source.baseline -or $source.analysis -or
    $source.nativeMcp -or $source.recommendations -or $source.trials.Count) {
    throw 'Only an unmeasured planned investigation that stopped before native preflight can be continued.'
}
foreach ($reference in @($source.request, $source.contracts, $source.adapter, $source.module)) {
    Assert-FileHash $reference.path $reference.sha256
}
Assert-FrozenScenario $source
Assert-OriginalSource $source
if (!$source.preview.closed -or !$source.preview.inspections.Count) {
    throw 'A closed, observed preview and a frozen validated scenario are required.'
}
if (Test-Path -LiteralPath (Join-Path $SourceInvestigation 'baseline')) {
    throw 'A baseline directory exists; never continue a reserved, failed or completed capture.'
}
$request = Read-JsonFile $source.request.path
if (!$request.allowMeasurements) { throw 'Measurement permission is required and cannot be granted by recovery.' }
Assert-FileHash "$plugin\CONTRACTS.md" $request.contractsSha256
Assert-FileHash "$plugin\pack\knowledge\intent-catalog.md" $request.intentCatalogSha256
Assert-FileHash "$plugin\pack\tools\Invoke-Scenario.ps1" $source.scenarioScriptSha256
if (!(Test-Path -LiteralPath $AdeCopilotCli -PathType Leaf)) { throw 'The repaired ADE client must exist.' }
Assert-RunDirectory $Directory @($SourceInvestigation, $request.applicationRepository,
    $request.knowledgeRepository, $request.expertHome, (Split-Path $request.profilerCli -Parent),
    (Split-Path $request.dotnet -Parent), (Split-Path $AdeCopilotCli -Parent))
if (Test-Path -LiteralPath $Directory) { throw 'Use a new continuation directory; preserve previous attempts.' }
$lockPath = Join-Path $SourceInvestigation 'operation.lock'
$lock = $null
try {
    $lock = [IO.File]::Open($lockPath, [IO.FileMode]::CreateNew, [IO.FileAccess]::Write, [IO.FileShare]::None)
    if (Test-Path -LiteralPath (Join-Path $SourceInvestigation 'baseline')) {
        throw 'A baseline appeared before recovery; no budget may be transferred.'
    }
    Assert-FileHash $sourcePath $sourceHash
    $observations = @(
        foreach ($inspection in $source.preview.inspections) {
            Assert-FileHash $inspection.path $inspection.sha256
            Read-JsonFile $inspection.path
        }
    )
    $files = @(Get-RepositoryFiles $source.originalRepository)
    if ((Get-SourceHash (Join-Path $SourceInvestigation 'baseline-source') $files) -ne $source.originalSourceSha256) {
        throw 'The source used to validate the original preview changed.'
    }
    $previousCli = $request.adeCopilotCli
    $request.directory = $Directory
    $request.adeCopilotCli = [IO.Path]::GetFullPath($AdeCopilotCli)
    $request.allowPreview = $false
    $request | Add-Member -NotePropertyName planningSource -NotePropertyValue @{
        path = $sourcePath; sha256 = $sourceHash
        scenarioSha256 = $source.scenarioSha256; previousAdeCopilotCli = $previousCli
        reason = 'Repair orchestration-client compatibility before any reserved or recorded capture.'
    }
    New-Item -ItemType Directory -Path $Directory | Out-Null
    $requestPath = Join-Path $Directory 'request.json'
    Write-JsonFile $requestPath $request
    $argumentsPath = Join-Path $Directory 'initialize-inputs.json'
    Write-JsonFile $argumentsPath @{ directory = $Directory; request = $request }
    [void](& "$plugin\pack\tools\Invoke-HarnessTool.ps1" -Operation Initialize -ArgumentsPath $argumentsPath)
    $contextPath = Join-Path $Directory 'context.json'
    $context = Read-JsonFile $contextPath
    if ($context.originalSourceSha256 -ne $source.originalSourceSha256 -or
        $context.originalRevision -ne $source.originalRevision -or
        $context.profilerSha256 -ne $source.profilerSha256 -or
        $context.scenarioScriptSha256 -ne $source.scenarioScriptSha256) {
        throw 'App, profiler or replay inputs differ from the validated scenario.'
    }
    Invoke-AdeRuntime @() $context -Probe
    $plan = @{ build = $source.scenario.build; window = $source.scenario.window; steps = $source.scenario.steps }
    if ($source.scenario.PSObject.Properties['intent']) { $plan.intent = $source.scenario.intent }
    $validationPath = Join-Path $Directory 'plan-inputs.json'
    Write-JsonFile $validationPath @{
        request = $request; plan = $plan; projects = @($files | Where-Object { $_ -like '*.csproj' })
        observations = $observations
    }
    [void](Invoke-LoggedProcess $Python @("$plugin\pack\tools\Harness-Mcp.py", '--validate-plan', $validationPath) `
        $Directory (Join-Path $Directory 'plan-validation.log') 120)
    Assert-FileHash $source.scenarioPath $source.scenarioSha256
    Assert-FileHash $sourcePath $sourceHash
    $scenarioPath = Join-Path $Directory 'scenario.json'
    Copy-Item -LiteralPath $source.scenarioPath -Destination $scenarioPath
    Assert-FileHash $scenarioPath $source.scenarioSha256
    $context.scenario = $source.scenario
    $context.scenarioPath = $scenarioPath
    $context.scenarioSha256 = $source.scenarioSha256
    $context.state = 'planned'
    $context | Add-Member -NotePropertyName planningSource -NotePropertyValue $request.planningSource
    Write-JsonFile $contextPath $context
    Write-JsonFile (Join-Path $Directory 'harness-mcp.json') @{
        mcpServers = @{
            'ui-profiler-harness' = @{
                type = 'local'; command = $Python
                args = @("$plugin\pack\tools\Harness-Mcp.py", '--request', $requestPath)
                tools = @('*'); timeout = 1800000
            }
        }
    }
    @{ status = 'ValidatedUnmeasuredContinuation'; directory = $Directory; planningSource = $context.planningSource
        captures = 0; previewLaunches = 0; editsAuthorized = $false } | ConvertTo-Json -Depth 8
} finally {
    if ($lock) { $lock.Dispose(); Remove-Item -LiteralPath $lockPath }
}
