param(
    [Parameter(Mandatory = $true)][string]$Executable,
    [string]$TargetExecutable,
    [switch]$CloseDuringDisconnect
)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName UIAutomationClient
Add-Type -AssemblyName UIAutomationTypes
$process = Start-Process -FilePath $Executable -WorkingDirectory (Split-Path $Executable) -PassThru
$targetProcess = $null
try {
    $deadline = [DateTime]::UtcNow.AddSeconds(30)
    do {
        Start-Sleep -Milliseconds 100
        $process.Refresh()
        if ($process.HasExited) { throw "Profiler exited before UI readiness: $($process.ExitCode)" }
    } while ($process.MainWindowHandle -eq [IntPtr]::Zero -and [DateTime]::UtcNow -lt $deadline)
    if ($process.MainWindowHandle -eq [IntPtr]::Zero) { throw 'Profiler window did not become ready.' }
    $window = [System.Windows.Automation.AutomationElement]::FromHandle($process.MainWindowHandle)
    function Find-Control([string]$name) {
        $condition = [System.Windows.Automation.PropertyCondition]::new(
            [System.Windows.Automation.AutomationElement]::NameProperty, $name)
        $control = $window.FindFirst([System.Windows.Automation.TreeScope]::Descendants, $condition)
        if (!$control) { throw "Missing live connection control: $name" }
        return $control
    }
    foreach ($name in @('Launch or attach mode', 'Executable path, application AUMID, or process ID',
        'Trust local injection binaries', 'Launch and connect', 'Disconnect', 'Live connection status')) {
        $null = Find-Control $name
    }
    if ($TargetExecutable) {
        $target = Find-Control 'Executable path, application AUMID, or process ID'
        $target.GetCurrentPattern([System.Windows.Automation.ValuePattern]::Pattern).SetValue($TargetExecutable)
        $trust = Find-Control 'Trust local injection binaries'
        $trust.GetCurrentPattern([System.Windows.Automation.TogglePattern]::Pattern).Toggle()
        (Find-Control 'Launch and connect').GetCurrentPattern([System.Windows.Automation.InvokePattern]::Pattern).Invoke()
        $deadline = [DateTime]::UtcNow.AddSeconds(55)
        $state = Find-Control 'Live connection status'
        do {
            Start-Sleep -Milliseconds 200
            $status = $state.Current.ItemStatus
            if ($status -eq 'Failed') { throw "UI connection failed: $($window.Current.Name)" }
        } while ($status -ne 'Connected' -and [DateTime]::UtcNow -lt $deadline)
        if ($status -ne 'Connected') { throw "UI did not connect; final state: $status" }
        $deadline = [DateTime]::UtcNow.AddSeconds(10)
        do {
            Start-Sleep -Milliseconds 200
            $all = $window.FindAll([System.Windows.Automation.TreeScope]::Descendants,
                [System.Windows.Automation.Condition]::TrueCondition)
            $statistics = @($all | Where-Object { $_.Current.ItemStatus -match '^PID \d+ \| ETW events [1-9]' })
        } while (!$statistics.Count -and [DateTime]::UtcNow -lt $deadline)
        if (!$statistics.Count) { throw 'UI did not show received target ETW events.' }
        $text = $statistics[0].Current.ItemStatus
        if ($text -notmatch '^PID (\d+)') { throw "Unexpected statistics: $text" }
        $targetProcess = Get-Process -Id ([int]$Matches[1])
        $hello = @($all | Where-Object { $_.Current.Name -like 'hello received from target PID*' })
        if (!$hello.Count) { throw 'UI did not confirm the real target hello round trip.' }
        $database = (Find-Control 'Live session database').Current.ItemStatus
        if (!(Test-Path -LiteralPath $database)) { throw 'UI did not expose a real session database.' }
        (Find-Control 'Disconnect').GetCurrentPattern([System.Windows.Automation.InvokePattern]::Pattern).Invoke()
        if ($CloseDuringDisconnect) {
            $null = $process.CloseMainWindow()
            $null = $process.CloseMainWindow()
            if (!$process.WaitForExit(30000)) { throw 'Repeated window close did not await and finish disconnect.' }
        }
        else {
            $deadline = [DateTime]::UtcNow.AddSeconds(25)
            do { Start-Sleep -Milliseconds 200 } while (
                $state.Current.ItemStatus -ne 'Disconnected' -and [DateTime]::UtcNow -lt $deadline)
            if ($state.Current.ItemStatus -ne 'Disconnected') { throw 'UI disconnect did not drain cleanly.' }
        }
        $cli = Join-Path (Split-Path (Split-Path $PSScriptRoot)) 'WinUIProfiler.Cli\bin\Release\net10.0\WinUIProfiler.Cli.dll'
        $session = dotnet $cli session --db $database
        if ($LASTEXITCODE -ne 0) { throw 'Live UI session was not durably closed/checkpointed for the read-only CLI.' }
        $saved = $session | ConvertFrom-Json
        if ($saved.result.value.status -ne 'Stopped' -or $saved.result.context.watermark.ingestOrdinal -le 0) {
            throw "UI connection did not persist its terminal recording: $session"
        }
        $targetProcess.Refresh()
        if ($targetProcess.HasExited) { throw 'Profiler disconnect terminated its target.' }
        Write-Output "LIVE_CONNECTION_UI_HELLO_ETW_PASS $text"
    }
    if (!$process.HasExited -and (!$process.CloseMainWindow() -or !$process.WaitForExit(10000))) {
        throw 'Profiler did not close cleanly after UI validation.'
    }
    if ($process.ExitCode -ne 0) { throw "Profiler exited $($process.ExitCode)" }
    Write-Output 'LIVE_CONNECTION_UI_CONTROLS_PASS'
}
finally {
    if (!$targetProcess -and $TargetExecutable) {
        $children = @(Get-CimInstance Win32_Process -Filter "ParentProcessId=$($process.Id)" |
            Where-Object { $_.ExecutablePath -eq $TargetExecutable })
        foreach ($child in $children) {
            $owned = Get-Process -Id $child.ProcessId -ErrorAction Stop
            try {
                $null = $owned.CloseMainWindow()
                if (!$owned.WaitForExit(10000)) { Stop-Process -Id $owned.Id }
            }
            finally { $owned.Dispose() }
        }
    }
    if ($targetProcess) {
        $targetProcess.Refresh()
        if (!$targetProcess.HasExited) {
            $null = $targetProcess.CloseMainWindow()
            if (!$targetProcess.WaitForExit(10000)) { Stop-Process -Id $targetProcess.Id }
        }
        $targetProcess.Dispose()
    }
    $process.Refresh()
    if (!$process.HasExited) { Stop-Process -Id $process.Id }
    $process.Dispose()
}
