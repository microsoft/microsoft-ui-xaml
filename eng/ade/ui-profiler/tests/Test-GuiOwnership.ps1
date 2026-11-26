# Exercise the driver's actual ownership guard before any UIA or window access.
#Requires -Version 7
$ErrorActionPreference = 'Stop'
$tokens = $null
$errors = $null
$ast = [Management.Automation.Language.Parser]::ParseFile(
    "$PSScriptRoot\..\pack\tools\Gui-Profiler.ps1", [ref]$tokens, [ref]$errors)
if ($errors.Count) { throw 'GUI driver has syntax errors.' }
$guard = $ast.Find({
    param($node)
    $node -is [Management.Automation.Language.FunctionDefinitionAst] -and $node.Name -eq 'Element'
}, $true)
if (!$guard) { throw 'Missing GUI ownership guard.' }
. ([scriptblock]::Create($guard.Extent.Text))
$GuiPath = 'C:\Fixture\WinUIProfiler.App.exe'
$created = [DateTime]::UtcNow
$guiIdentity = @{ creationTimeFileTime = $created.ToFileTimeUtc() }
foreach ($case in @('exited', 'reused-pid', 'different-executable')) {
    $gui = [pscustomobject]@{
        HasExited = $case -eq 'exited'
        StartTime = if ($case -eq 'reused-pid') { $created.AddSeconds(1) } else { $created }
        MainModule = @{ FileName = if ($case -eq 'different-executable') { 'C:\Other.exe' } else { $GuiPath } }
    }
    $gui | Add-Member ScriptMethod Refresh {}
    $failure = $null
    try { Element 'ProfilerSession' } catch { $failure = $_.Exception.Message }
    if ($failure -ne 'The owned profiler GUI exited or changed identity.') {
        throw "GUI ownership case $case reached UI access instead of refusing: $failure"
    }
}
Write-Output 'PASS GUI ownership: exited instance, reused PID and changed executable refused before UI access.'
$gui = [pscustomobject]@{
    HasExited = $false; StartTime = $created; MainModule = @{ FileName = $GuiPath }
}
$gui | Add-Member ScriptMethod Refresh {}
$script:lookupCalls = 0
$script:lookupMode = 'recover'
$token = [pscustomobject]@{ Id = 'owned-element' }
function Find-GuiElements([string]$Id) {
    $script:lookupCalls++
    if ($script:lookupMode -eq 'recover' -and $script:lookupCalls -ge 2) { return $token }
    if ($script:lookupMode -eq 'duplicate') { return @($token, $token) }
}
if ((Element 'ProfilerSession') -ne $token -or $script:lookupCalls -ne 2) {
    throw 'A temporarily absent GUI contract was not reacquired.'
}
foreach ($case in @('duplicate', 'missing', 'starting')) {
    $script:lookupMode = $case
    $script:lookupCalls = 0
    $failure = $null
    $result = $null
    try { $result = Element 'ProfilerSession' -Starting:($case -eq 'starting') -TimeoutMilliseconds 25 }
    catch { $failure = $_.Exception.Message }
    if ($case -eq 'duplicate' -and
        ($failure -ne 'Expected one owned GUI element: ProfilerSession; found 2.' -or $script:lookupCalls -ne 1)) {
        throw 'Ambiguous GUI contracts must fail immediately.'
    }
    if ($case -eq 'missing' -and $failure -ne
        'Expected one owned GUI element: ProfilerSession; found 0 after waiting 25 milliseconds.') {
        throw 'A permanently missing GUI contract must time out explicitly.'
    }
    if ($case -eq 'starting' -and ($failure -or $null -ne $result -or $script:lookupCalls -ne 1)) {
        throw 'Initial contract polling must retain its outer startup deadline.'
    }
}
$script:lookupCalls = 0
$script:refreshCalls = 0
$gui | Add-Member ScriptMethod Refresh {
    $script:refreshCalls++
    if ($script:refreshCalls -eq 2) { $this.StartTime = $this.StartTime.AddSeconds(1) }
} -Force
$failure = $null
try { Element 'ProfilerSession' } catch { $failure = $_.Exception.Message }
if ($failure -ne 'The owned profiler GUI exited or changed identity.' -or $script:lookupCalls -ne 1) {
    throw 'Ownership must be revalidated before every availability retry.'
}
Remove-Item Function:\Find-GuiElements
Write-Output 'PASS GUI availability: transient absence, bounded timeout, ambiguity, startup polling and retry ownership.'
Import-Module "$PSScriptRoot\..\pack\tools\ProfilerHarness.psm1" -Force -DisableNameChecking
$fixture = Join-Path ([IO.Path]::GetTempPath()) ('gui-bindings-' + [Guid]::NewGuid().ToString('N'))
try {
    New-Item -ItemType Directory "$fixture\payload" -Force | Out-Null
    foreach ($name in @('scenario.json', 'replay.ps1', 'driver.ps1', 'reader.py', 'payload\WinUIProfiler.App.exe')) {
        [IO.File]::WriteAllText((Join-Path $fixture $name), 'fixture')
    }
    foreach ($name in @('app', 'knowledge', 'expert')) {
        New-Item -ItemType Directory (Join-Path $fixture $name) | Out-Null
    }
    $request = @{
        applicationRepository = "$fixture\app"; knowledgeRepository = "$fixture\knowledge"
        expertHome = "$fixture\expert"; profilerCli = "$fixture\payload\WinUIProfiler.App.exe"
        scenario = 'Do not launch anything; exercise GUI input refusal.'
        captureBackend = 'Gui'
    }
    foreach ($case in @('missing-gui', 'missing-native-payload')) {
        if ($case -eq 'missing-native-payload') { $request.profilerGui = $request.profilerCli }
        Write-JsonFile "$fixture\arguments.json" @{ directory = "$fixture\run"; request = $request }
        $failure = $null
        try {
            & "$PSScriptRoot\..\pack\tools\Invoke-HarnessTool.ps1" -Operation Initialize -ArgumentsPath "$fixture\arguments.json"
        } catch { $failure = $_.Exception.Message }
        $expected = if ($case -eq 'missing-gui') { '*no CLI fallback*' } else { '*Incomplete GUI deployment*' }
        if ($failure -notlike $expected -or (Test-Path -LiteralPath "$fixture\run")) {
            throw "GUI input refusal case $case failed or created a run: $failure"
        }
    }
    Write-Output 'PASS GUI preparation: missing GUI and incomplete native deployment refuse before creating a run; no CLI fallback.'
    $context = [pscustomobject]@{
        scenarioPath = "$fixture\scenario.json"; scenarioSha256 = Get-Sha256 "$fixture\scenario.json"
        scenarioScript = "$fixture\replay.ps1"; scenarioScriptSha256 = Get-Sha256 "$fixture\replay.ps1"
        guiCapture = @{
            executable = "$fixture\payload\WinUIProfiler.App.exe"
            payloadSha256 = Get-ProfilerHash "$fixture\payload\WinUIProfiler.App.exe"
            driver = @{ path = "$fixture\driver.ps1"; sha256 = Get-Sha256 "$fixture\driver.ps1" }
            reader = @{ path = "$fixture\reader.py"; sha256 = Get-Sha256 "$fixture\reader.py" }
        }
    }
    Write-JsonFile "$fixture\request.json" @{ captureBackend = 'Gui'; profilerGui = $context.guiCapture.executable }
    $context | Add-Member NoteProperty request @{ path = "$fixture\request.json"; sha256 = Get-Sha256 "$fixture\request.json" }
    $missing = $context | ConvertTo-Json -Depth 20 | ConvertFrom-Json
    $missing.PSObject.Properties.Remove('guiCapture')
    $failed = $false
    try { Assert-FrozenScenario $missing } catch { $failed = $true }
    if (!$failed) { throw 'Missing GUI binding silently fell back to CLI.' }
    Assert-FrozenScenario $context
    Assert-FrozenScenario ($context | ConvertTo-Json -Depth 20 | ConvertFrom-Json)
    foreach ($name in @('driver.ps1', 'reader.py', 'payload\WinUIProfiler.App.exe')) {
        $path = Join-Path $fixture $name
        [IO.File]::WriteAllText($path, 'changed')
        $failed = $false
        try { Assert-FrozenScenario $context } catch { $failed = $true }
        if (!$failed) { throw "Changed GUI binding was accepted: $name" }
        [IO.File]::WriteAllText($path, 'fixture')
    }
    Write-Output 'PASS GUI bindings: cloned contexts preserve backend; changed driver, reader or payload refused.'
} finally {
    Remove-Item -LiteralPath $fixture -Recurse -Force
}
