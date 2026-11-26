# Exercise real recovery refusals against isolated files; never initialize, launch or capture.
#Requires -Version 7
$ErrorActionPreference = 'Stop'
$plugin = Split-Path $PSScriptRoot -Parent
Import-Module "$plugin\pack\tools\ProfilerHarness.psm1" -Force -DisableNameChecking
$temp = Join-Path ([IO.Path]::GetTempPath()) ('ui-profiler-continuation-' + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory $temp | Out-Null
try {
    $repo = Join-Path $temp 'repo'
    New-Item -ItemType Directory $repo | Out-Null
    [IO.File]::WriteAllText((Join-Path $repo 'App.cs'), 'original')
    git -C $repo init -q
    git -C $repo add -- App.cs
    git -C $repo -c user.name='Harness Test' -c user.email='test@example.invalid' -c core.hooksPath=NUL commit -qm "Synthetic recovery baseline`n`nCo-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>`nCopilot-Session: fe7e0679-84a3-41a3-9fa9-3342fe4afcda"
    if ($LASTEXITCODE -ne 0) { throw 'Could not commit the isolated source fixture.' }
    $sourceHash = Get-SourceHash $repo
    $revision = git -C $repo rev-parse HEAD
    $cases = @{
        state = 'Only an unmeasured planned investigation'
        measured = 'Only an unmeasured planned investigation'
        native = 'Only an unmeasured planned investigation'
        trials = 'Only an unmeasured planned investigation'
        path = 'Only an unmeasured planned investigation'
        reserved = 'A baseline directory exists'
        consent = 'Measurement permission is required'
        module = 'Evidence or input changed'
        scenario = 'Evidence or input changed'
        replay = 'Evidence or input changed'
        contracts = 'Evidence or input changed'
        source = 'original application source changed'
        preview = 'closed, observed preview'
        valid = 'The repaired ADE client must exist'
    }
    foreach ($case in $cases.Keys) {
        $directory = Join-Path $temp $case
        $destination = Join-Path $temp "$case-new"
        New-Item -ItemType Directory $directory | Out-Null
        $requestPath = Join-Path $directory 'request.json'
        $request = @{
            allowMeasurements = $true
            contractsSha256 = Get-Sha256 "$plugin\CONTRACTS.md"
            intentCatalogSha256 = Get-Sha256 "$plugin\pack\knowledge\intent-catalog.md"
        }
        if ($case -eq 'consent') { $request.allowMeasurements = $false }
        if ($case -eq 'contracts') { $request.contractsSha256 = '0' * 64 }
        Write-JsonFile $requestPath $request
        $scenarioPath = Join-Path $directory 'scenario.json'
        Write-JsonFile $scenarioPath @{ steps = @(@{ action = 'assert'; settleMilliseconds = 0 }) }
        $replayPath = Join-Path $directory 'replay.ps1'
        [IO.File]::WriteAllText($replayPath, '# inert fixture, never executed')
        $reference = @{ path = $replayPath; sha256 = Get-Sha256 $replayPath }
        $context = @{
            schema = 'ui-profiler/investigation/v3'; directory = $directory; state = 'planned'
            baseline = $null; analysis = $null; nativeMcp = $null; recommendations = $null; trials = @()
            request = @{ path = $requestPath; sha256 = Get-Sha256 $requestPath }
            contracts = $reference; adapter = $reference; module = $reference
            scenarioPath = $scenarioPath; scenarioSha256 = Get-Sha256 $scenarioPath
            scenarioScript = "$plugin\pack\tools\Invoke-Scenario.ps1"
            scenarioScriptSha256 = Get-Sha256 "$plugin\pack\tools\Invoke-Scenario.ps1"
            originalRepository = $repo; originalRevision = $revision; originalSourceSha256 = $sourceHash
            preview = @{ closed = $true; inspections = @($reference) }
        }
        switch ($case) {
            state { $context.state = 'failed' }
            measured { $context.baseline = @{ path = 'already-measured' } }
            native { $context.nativeMcp = @{ path = 'preflight-started' } }
            trials { $context.trials = @(@{ id = 'existing-trial' }) }
            path { $context.directory = $temp }
            reserved { New-Item -ItemType Directory (Join-Path $directory 'baseline') | Out-Null }
            module { $context.module = @{ path = $replayPath; sha256 = '0' * 64 } }
            scenario { [IO.File]::AppendAllText($scenarioPath, 'changed') }
            replay { $context.scenarioScriptSha256 = '0' * 64 }
            source { [IO.File]::WriteAllText((Join-Path $repo 'App.cs'), 'changed') }
            preview { $context.preview.inspections = @() }
        }
        $contextPath = Join-Path $directory 'context.json'
        Write-JsonFile $contextPath $context
        $contextHash = Get-Sha256 $contextPath
        $requestHash = Get-Sha256 $requestPath
        $failure = $null
        try {
            & "$plugin\scripts\Restart-UnmeasuredInvestigation.ps1" -SourceInvestigation $directory `
                -Directory $destination -AdeCopilotCli (Join-Path $temp 'missing-client.exe') -Python 'never-used'
        } catch { $failure = $_.Exception.Message }
        if (!$failure -or !$failure.Contains($cases[$case], [StringComparison]::OrdinalIgnoreCase)) {
            throw "Recovery case $case did not refuse at its intended guard: $failure"
        }
        if ((Test-Path -LiteralPath $destination) -or (Get-Sha256 $contextPath) -ne $contextHash -or
            (Get-Sha256 $requestPath) -ne $requestHash) {
            throw "Recovery case $case modified original inputs or created a continuation."
        }
        [IO.File]::WriteAllText((Join-Path $repo 'App.cs'), 'original')
        Write-Output "PASS continuation refusal: $case"
    }
    Write-Output "$($cases.Count) recovery guards passed; zero previews, builds or captures."
} finally {
    Remove-Item -LiteralPath $temp -Recurse -Force
}
