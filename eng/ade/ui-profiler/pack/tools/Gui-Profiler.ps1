# Drive only one owned profiler GUI. Recording itself is performed by that application.
#Requires -Version 7
param(
    [Parameter(Mandatory)][string]$GuiPath,
    [Parameter(Mandatory)][string]$Executable,
    [Parameter(Mandatory)][string]$Directory,
    [Parameter(Mandatory)][int]$LifetimeSeconds
)
$ErrorActionPreference = 'Stop'
Import-Module "$PSScriptRoot\ProfilerHarness.psm1" -Force -DisableNameChecking
Add-Type -AssemblyName UIAutomationClient, UIAutomationTypes
$gui = $null
$guiIdentity = $null
$completed = $false
$snapshots = [Collections.Generic.List[string]]::new()
$clock = [Diagnostics.Stopwatch]::StartNew()
function Emit([string]$Type, $Data) {
    [Console]::Out.WriteLine((@{ type = $Type; data = $Data } | ConvertTo-Json -Depth 40 -Compress))
    [Console]::Out.Flush()
}
function Find-GuiElements([string]$Id) {
    if ($gui.MainWindowHandle -eq 0) { return }
    $root = [System.Windows.Automation.AutomationElement]::FromHandle($gui.MainWindowHandle)
    $condition = [System.Windows.Automation.AndCondition]::new(
        [System.Windows.Automation.PropertyCondition]::new(
            [System.Windows.Automation.AutomationElement]::ProcessIdProperty, $gui.Id),
        [System.Windows.Automation.PropertyCondition]::new(
            [System.Windows.Automation.AutomationElement]::AutomationIdProperty, $Id))
    $root.FindAll([System.Windows.Automation.TreeScope]::Subtree, $condition)
}
function Element([string]$Id, [switch]$Starting,
    [ValidateRange(1, 30000)][int]$TimeoutMilliseconds = 5000) {
    $wait = [Diagnostics.Stopwatch]::StartNew()
    while ($true) {
        $gui.Refresh()
        if ($gui.HasExited -or $gui.StartTime.ToUniversalTime().ToFileTimeUtc() -ne $guiIdentity.creationTimeFileTime -or
            $gui.MainModule.FileName -ne $GuiPath) { throw 'The owned profiler GUI exited or changed identity.' }
        $found = @(Find-GuiElements $Id)
        if ($found.Count -eq 1) { return $found[0] }
        if ($found.Count -gt 1) { throw "Expected one owned GUI element: $Id; found $($found.Count)." }
        if ($Starting) { return $null }
        if ($wait.Elapsed.TotalMilliseconds -ge $TimeoutMilliseconds) {
            throw "Expected one owned GUI element: $Id; found 0 after waiting $TimeoutMilliseconds milliseconds."
        }
        Start-Sleep -Milliseconds ([Math]::Max(1, [Math]::Min(100,
            $TimeoutMilliseconds - [int]$wait.Elapsed.TotalMilliseconds)))
    }
}
function State([switch]$Starting) {
    $element = Element 'ProfilerSession' -Starting:$Starting
    if ($Starting -and ($null -eq $element -or !$element.Current.ItemStatus)) { return $null }
    $value = $element.Current.ItemStatus | ConvertFrom-Json
    if ($value.schema -ne 'winui-profiler/gui-session/v1') { throw 'The GUI does not expose the supported session contract.' }
    if ($value.state -eq 'Failed' -or $value.error) { throw "Profiler GUI failed: $($value.error)" }
    $value
}
function Invoke-Button([string]$Id) {
    $element = Element $Id
    if (!$element.Current.IsEnabled) { throw "Owned GUI button is disabled: $Id" }
    $element.GetCurrentPattern([System.Windows.Automation.InvokePattern]::Pattern).Invoke()
}
try {
    $gui = Start-Process -FilePath $GuiPath -WorkingDirectory (Split-Path $GuiPath -Parent) -PassThru
    $guiIdentity = @{ processId = $gui.Id; creationTimeFileTime = $gui.StartTime.ToUniversalTime().ToFileTimeUtc()
        executable = $GuiPath; sha256 = Get-Sha256 $GuiPath }
    Write-JsonFile (Join-Path $Directory 'gui-identity.json') $guiIdentity
    while ($gui.MainWindowHandle -eq 0 -and !$gui.HasExited -and $clock.Elapsed.TotalSeconds -lt 30) {
        Start-Sleep -Milliseconds 100
        $gui.Refresh()
    }
    if ($gui.HasExited -or $gui.MainWindowHandle -eq 0) { throw 'The owned profiler GUI did not open a window.' }
    while ($null -eq (State -Starting)) {
        if ($clock.Elapsed.TotalSeconds -gt 30) { throw 'The owned GUI session contract did not become available.' }
        Start-Sleep -Milliseconds 100
    }
    (Element 'TargetText').GetCurrentPattern([System.Windows.Automation.ValuePattern]::Pattern).SetValue($Executable)
    (Element 'WorkingDirectoryText').GetCurrentPattern([System.Windows.Automation.ValuePattern]::Pattern).SetValue(
        (Split-Path $Executable -Parent))
    Invoke-Button 'ConnectButton'
    do {
        $state = State
        if ($clock.Elapsed.TotalSeconds -gt 90) { throw 'The profiler GUI did not complete connection and its initial snapshot within90seconds.' }
        Start-Sleep -Milliseconds 100
    } until ($state.state -eq 'Connected' -and $state.ready)
    if ($state.etwState -ne 'available' -or $state.recordingState -ne 'available' -or !$state.snapshot.path) {
        throw 'GUI ETW, SQLite or initial snapshot is unavailable.'
    }
    $identity = $state.scope.process
    $target = Get-Process -Id $identity.processId
    try {
        if ($target.MainModule.FileName -ne $Executable -or
            $target.StartTime.ToUniversalTime().ToFileTimeUtc() -ne $identity.creationTimeFileTime) {
            throw 'The GUI connected to a different target instance.'
        }
    } finally { $target.Dispose() }
    $database = $state.recording.path
    Write-JsonFile (Join-Path $Directory 'gui-session-connected.json') $state
    $snapshots.Add($state.snapshot.path)
    Emit 'snapshot' @{ path = $state.snapshot.path }
    Emit 'status' @{ state = 'Connected'; scope = $state.scope; etwState = $state.etwState
        recordingState = $state.recordingState; captureBackend = 'gui'; gui = $guiIdentity }
    $pendingInput = [Console]::In.ReadLineAsync()
    while ($true) {
        if ($clock.Elapsed.TotalSeconds -gt $LifetimeSeconds) { throw 'The GUI capture exceeded its bounded lifetime.' }
        $state = State
        if ($state.state -ne 'Connected') { throw "The GUI disconnected during capture: $($state.state)" }
        if ($state.scope.process.processId -ne $identity.processId -or
            $state.scope.process.creationTimeFileTime -ne $identity.creationTimeFileTime -or
            $state.scope.process.instanceId -ne $identity.instanceId) {
            throw 'The GUI switched to another target process instance.'
        }
        if ($pendingInput.IsCompleted) {
            $command = $pendingInput.GetAwaiter().GetResult()
            if ($null -eq $command) { throw 'The harness closed GUI control input before finalization.' }
            if ($command -eq 'stop') { break }
            if ($command -ne 'snapshot') { throw 'Unsupported GUI capture control command.' }
            $before = $state.snapshot.path
            Invoke-Button 'SnapshotButton'
            $wait = [Diagnostics.Stopwatch]::StartNew()
            do {
                $state = State
                if ($wait.Elapsed.TotalSeconds -gt 30) { throw 'The GUI end snapshot timed out.' }
                Start-Sleep -Milliseconds 100
            } until ($state.ready -and $state.snapshot.path -and $state.snapshot.path -ne $before)
            $snapshots.Add($state.snapshot.path)
            Emit 'snapshot' @{ path = $state.snapshot.path }
            $pendingInput = [Console]::In.ReadLineAsync()
        }
        Start-Sleep -Milliseconds 100
    }
    $final = $state
    Invoke-Button 'DisconnectButton'
    $wait = [Diagnostics.Stopwatch]::StartNew()
    do {
        $state = State
        if ($wait.Elapsed.TotalSeconds -gt 30) { throw 'The GUI did not finalize its SQLite within30seconds.' }
        Start-Sleep -Milliseconds 100
    } until ($state.state -eq 'Disconnected')
    Write-JsonFile (Join-Path $Directory 'gui-session-final.json') $state
    # Closing normally also drains/cancels the GUI's post-recording symbol writer.
    if (!$gui.CloseMainWindow() -or !$gui.WaitForExit(30000)) {
        throw 'The owned GUI did not close its post-recording SQLite writers.'
    }
    $hash = Get-Sha256 $database
    $copy = Join-Path $Directory ([IO.Path]::GetFileName($database))
    [IO.File]::Copy($database, $copy, $false)
    Assert-FileHash $copy $hash
    $read = & python -B "$PSScriptRoot\Read-GuiRecording.py" --database $copy --process-id $identity.processId `
        --creation-time $identity.creationTimeFileTime --instance-id $identity.instanceId
    if ($LASTEXITCODE -ne 0) { throw 'The finalized GUI SQLite failed evidence validation.' }
    foreach ($line in $read) {
        [Console]::Out.WriteLine($line)
    }
    Emit 'summary' @{ state = 'completed'; exitCode = 0; recordingPath = $copy
        snapshotPaths = $snapshots.ToArray(); process = $identity; targetTerminated = $false
        captureBackend = 'gui'; gui = $guiIdentity; etw = $final.etw
        sourceRecording = @{ path = $database; sha256 = $hash } }
    $completed = $true
} catch {
    Write-JsonFile (Join-Path $Directory 'gui-failure.json') @{
        error = $_.Exception.Message; state = $state; gui = $guiIdentity
    }
    throw
} finally {
    if (!$completed -and $guiIdentity) {
        Close-OwnedApplication $guiIdentity $GuiPath
    }
    if ($gui) { $gui.Dispose() }
}
