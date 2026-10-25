param(
    [Parameter(Mandatory = $true)][string]$Executable,
    [string]$ApplicationAumid
)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName UIAutomationClient
Add-Type -AssemblyName UIAutomationTypes
Add-Type -ReferencedAssemblies UIAutomationClient,UIAutomationTypes @'
using System;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Automation;
public static class PickerTestWindows {
    [DllImport("user32.dll")] public static extern IntPtr GetWindow(IntPtr window, uint command);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)] private static extern int GetClassName(IntPtr window, StringBuilder name, int maximum);
    [DllImport("user32.dll")] private static extern bool EnumWindows(EnumWindow callback, IntPtr parameter);
    [DllImport("user32.dll")] private static extern bool IsWindowVisible(IntPtr window);
    [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr window, uint message, IntPtr wparam, IntPtr lparam);
    [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr window, IntPtr after, int x, int y, int width, int height, uint flags);
    [DllImport("user32.dll")] private static extern bool EnumChildWindows(IntPtr parent, EnumWindow callback, IntPtr parameter);
    [DllImport("user32.dll")] private static extern int GetDlgCtrlID(IntPtr window);
    [DllImport("user32.dll", EntryPoint = "SendMessageW", CharSet = CharSet.Unicode)] private static extern IntPtr SetControlText(IntPtr window, uint message, IntPtr wparam, string text);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)] private static extern IntPtr SendMessage(IntPtr window, uint message, IntPtr maximum, StringBuilder text);
    private delegate bool EnumWindow(IntPtr window, IntPtr parameter);
    public static bool IsOwnedBy(IntPtr window, IntPtr owner) {
        for (int depth = 0; window != IntPtr.Zero && depth < 16; depth++) {
            window = GetWindow(window, 4);
            if (window == owner) return true;
        }
        return false;
    }
    public static Task InvokeAsync(AutomationElement element) {
        return Task.Run(() => ((InvokePattern)element.GetCurrentPattern(InvokePattern.Pattern)).Invoke());
    }
    public static IntPtr FindDialog(IntPtr owner) {
        IntPtr result = IntPtr.Zero;
        EnumWindows((window, parameter) => {
            var name = new StringBuilder(256);
            GetClassName(window, name, name.Capacity);
            if (name.ToString() == "#32770" && IsWindowVisible(window) && IsOwnedBy(window, owner)) {
                result = window;
                return false;
            }
            return true;
        }, IntPtr.Zero);
        return result;
    }
    public static bool SetFileName(IntPtr dialog, string path) {
        IntPtr filename = IntPtr.Zero;
        EnumChildWindows(dialog, (window, parameter) => {
            if (GetDlgCtrlID(window) == 1148) { filename = window; return false; }
            return true;
        }, IntPtr.Zero);
        if (filename == IntPtr.Zero) return false;
        var edit = filename;
        EnumChildWindows(filename, (window, parameter) => {
            var name = new StringBuilder(256);
            GetClassName(window, name, name.Capacity);
            if (name.ToString() == "Edit") { edit = window; return false; }
            return true;
        }, IntPtr.Zero);
        return SetControlText(edit, 0x000C, IntPtr.Zero, path) != IntPtr.Zero;
    }
    public static string DescribeDialog(IntPtr dialog) {
        var result = new StringBuilder();
        EnumChildWindows(dialog, (window, parameter) => {
            var name = new StringBuilder(256);
            var text = new StringBuilder(1024);
            GetClassName(window, name, name.Capacity);
            SendMessage(window, 0x000D, (IntPtr)text.Capacity, text);
            result.AppendLine(GetDlgCtrlID(window) + " | " + name + " | " + text);
            return true;
        }, IntPtr.Zero);
        return result.ToString();
    }
    public static bool AcceptDialog(IntPtr dialog) {
        IntPtr button = IntPtr.Zero;
        EnumChildWindows(dialog, (window, parameter) => {
            if (GetDlgCtrlID(window) == 1) { button = window; return false; }
            return true;
        }, IntPtr.Zero);
        return button != IntPtr.Zero && PostMessage(button, 0x00F5, IntPtr.Zero, IntPtr.Zero);
    }
}
'@
$profiler = Start-Process -FilePath $Executable -WorkingDirectory (Split-Path $Executable) -PassThru
$targetProcess = $null
try {
    $deadline = [DateTime]::UtcNow.AddSeconds(30)
    do {
        Start-Sleep -Milliseconds 100
        $profiler.Refresh()
        if ($profiler.HasExited) { throw "Profiler exited before picker validation: $($profiler.ExitCode)" }
    } while ($profiler.MainWindowHandle -eq [IntPtr]::Zero -and [DateTime]::UtcNow -lt $deadline)
    if ($profiler.MainWindowHandle -eq [IntPtr]::Zero) { throw 'Profiler window is unavailable.' }
    $window = [System.Windows.Automation.AutomationElement]::FromHandle($profiler.MainWindowHandle)
    function Find-Control([string]$name, $root = $window) {
        $condition = [System.Windows.Automation.PropertyCondition]::new(
            [System.Windows.Automation.AutomationElement]::NameProperty, $name)
        $deadline = [DateTime]::UtcNow.AddSeconds(10)
        do {
            $control = $root.FindFirst([System.Windows.Automation.TreeScope]::Descendants, $condition)
            if (!$control) { Start-Sleep -Milliseconds 100 }
        } while (!$control -and [DateTime]::UtcNow -lt $deadline)
        if (!$control) { throw "Missing target selection control: $name" }
        return $control
    }
    function Invoke-Control([string]$name, $root = $window) {
        (Find-Control $name $root).GetCurrentPattern([System.Windows.Automation.InvokePattern]::Pattern).Invoke()
    }
    function Set-Text([string]$name, [string]$value) {
        (Find-Control $name).GetCurrentPattern([System.Windows.Automation.ValuePattern]::Pattern).SetValue($value)
    }
    function Target-Value {
        return (Find-Control 'Executable path, application AUMID, or process ID').GetCurrentPattern(
            [System.Windows.Automation.ValuePattern]::Pattern).Current.Value
    }
    function Select-Mode([string]$name) {
        $combo = Find-Control 'Launch or attach mode'
        $combo.GetCurrentPattern([System.Windows.Automation.ExpandCollapsePattern]::Pattern).Expand()
        (Find-Control $name).GetCurrentPattern([System.Windows.Automation.SelectionItemPattern]::Pattern).Select()
    }
    function Select-TargetRow([string]$search, [string]$expected) {
        Set-Text 'Search targets' $search
        $deadline = [DateTime]::UtcNow.AddSeconds(30)
        do {
            Start-Sleep -Milliseconds 100
            $items = (Find-Control 'Available targets').FindAll([System.Windows.Automation.TreeScope]::Children,
                [System.Windows.Automation.PropertyCondition]::new(
                    [System.Windows.Automation.AutomationElement]::ControlTypeProperty,
                    [System.Windows.Automation.ControlType]::ListItem))
            $matches = @($items | Where-Object { $_.Current.Name -like "*$expected*" })
        } while (!$matches.Count -and [DateTime]::UtcNow -lt $deadline)
        if (!$matches.Count) { throw "Discovery did not show the expected real target: $expected" }
        $matches[0].GetCurrentPattern([System.Windows.Automation.SelectionItemPattern]::Pattern).Select()
        $deadline = [DateTime]::UtcNow.AddSeconds(3)
        do { Start-Sleep -Milliseconds 50 } while (
            !(Find-Control 'Select').Current.IsEnabled -and [DateTime]::UtcNow -lt $deadline)
        if (!(Find-Control 'Select').Current.IsEnabled) { throw "Selected target is unavailable: $expected" }
    }
    $null = Find-Control 'Browse or select target'
    Set-Text 'Executable path, application AUMID, or process ID' 'preserve-cancelled-input'
    Select-Mode 'Launch packaged'
    Invoke-Control 'Browse or select target'
    $null = Find-Control 'Search targets'
    Invoke-Control 'Cancel'
    if ((Target-Value) -ne 'preserve-cancelled-input') { throw 'Cancelling selection changed the manual target.' }

    if (!$ApplicationAumid) {
        $ApplicationAumid = (Get-StartApps | Where-Object { $_.AppID -like '*!*' } | Select-Object -First 1).AppID
        if (!$ApplicationAumid) { throw 'No installed packaged app is available for real discovery qualification.' }
    }
    Invoke-Control 'Browse or select target'
    Select-TargetRow $ApplicationAumid $ApplicationAumid
    Invoke-Control 'Select'
    if ((Target-Value) -ne $ApplicationAumid) { throw 'Packaged selection did not fill the actual AUMID.' }
    Write-Output "PACKAGED_APP_SELECTION_PASS aumid=$ApplicationAumid"

    $targetProcess = Start-Process -FilePath $Executable -WorkingDirectory (Split-Path $Executable) -PassThru
    Select-Mode 'Attach to process ID'
    Invoke-Control 'Browse or select target'
    Select-TargetRow ([string]$targetProcess.Id) ([string]$targetProcess.Id)
    Invoke-Control 'Refresh list'
    if ((Find-Control 'Select').Current.IsEnabled) { throw 'Refresh retained a potentially stale selected process.' }
    Select-TargetRow ([string]$targetProcess.Id) ([string]$targetProcess.Id)
    Invoke-Control 'Select'
    if ((Target-Value) -ne [string]$targetProcess.Id) { throw 'Process selection did not fill the target PID.' }
    if ((Find-Control 'Live connection status').Current.ItemStatus -ne 'Disconnected') {
        throw 'Selecting a target must not automatically connect.'
    }
    Write-Output "RUNNING_PROCESS_SELECTION_PASS pid=$($targetProcess.Id)"
    (Find-Control 'Trust local injection binaries').GetCurrentPattern(
        [System.Windows.Automation.TogglePattern]::Pattern).Toggle()
    Invoke-Control 'Launch and connect'
    $deadline = [DateTime]::UtcNow.AddSeconds(55)
    $state = Find-Control 'Live connection status'
    do {
        Start-Sleep -Milliseconds 200
        if ($state.Current.ItemStatus -eq 'Failed') { throw 'Selected-process connection failed.' }
    } while ($state.Current.ItemStatus -ne 'Connected' -and [DateTime]::UtcNow -lt $deadline)
    if ($state.Current.ItemStatus -ne 'Connected') { throw 'Selected process did not connect.' }
    if ((Find-Control 'Browse or select target').Current.IsEnabled) { throw 'Browse remained enabled while connected.' }
    $hello = $window.FindAll([System.Windows.Automation.TreeScope]::Descendants,
        [System.Windows.Automation.Condition]::TrueCondition) | Where-Object {
        $_.Current.Name -like "hello received from target PID $($targetProcess.Id);*"
    }
    if (!$hello) { throw 'Selected process did not return its real TAP hello.' }
    $targetProcess.Refresh()
    if (![PickerTestWindows]::SetWindowPos($targetProcess.MainWindowHandle, [IntPtr]::Zero, 0, 0, 1180, 780, 0x16)) {
        throw 'Cannot drive a real target frame for ETW validation.'
    }
    $deadline = [DateTime]::UtcNow.AddSeconds(10)
    do {
        Start-Sleep -Milliseconds 100
        $statistics = (Find-Control 'Live event statistics').Current.ItemStatus
    } while ($statistics -notmatch "^PID $($targetProcess.Id) \| ETW events [1-9]" -and [DateTime]::UtcNow -lt $deadline)
    if ($statistics -notmatch "^PID $($targetProcess.Id) \| ETW events [1-9]") {
        throw 'Selected target did not deliver real XAML ETW events.'
    }
    $database = (Find-Control 'Live session database').Current.ItemStatus
    Invoke-Control 'Disconnect'
    $deadline = [DateTime]::UtcNow.AddSeconds(25)
    do { Start-Sleep -Milliseconds 100 } while (
        $state.Current.ItemStatus -ne 'Disconnected' -and [DateTime]::UtcNow -lt $deadline)
    if ($state.Current.ItemStatus -ne 'Disconnected') { throw 'Selected-process disconnect did not complete.' }
    $targetProcess.Refresh()
    if ($targetProcess.HasExited) { throw 'Disconnect terminated the selected process.' }
    $cli = Join-Path (Split-Path (Split-Path $PSScriptRoot)) 'WinUIProfiler.Cli\bin\Release\net10.0\WinUIProfiler.Cli.dll'
    $saved = dotnet $cli session --db $database
    if ($LASTEXITCODE -ne 0) { throw 'Selected-target recording did not close durably for read-only reopen.' }
    $session = $saved | ConvertFrom-Json
    if ($session.result.value.status -ne 'Stopped' -or $session.result.context.watermark.ingestOrdinal -le 0) {
        throw "Selected-target ETW recording did not preserve a terminal nonempty watermark: $saved"
    }
    Write-Output "SELECTED_PROCESS_TAP_HELLO_ETW_DURABLE_PASS $statistics"

    Select-Mode 'Launch unpackaged'
    $nativeRequest = [PickerTestWindows]::InvokeAsync((Find-Control 'Browse or select target'))
    $deadline = [DateTime]::UtcNow.AddSeconds(15)
    do {
        Start-Sleep -Milliseconds 100
        $fileHandle = [PickerTestWindows]::FindDialog($profiler.MainWindowHandle)
    } while ($fileHandle -eq [IntPtr]::Zero -and [DateTime]::UtcNow -lt $deadline)
    if ($fileHandle -eq [IntPtr]::Zero) { throw 'The owner-initialized executable file picker did not appear.' }
    if (![PickerTestWindows]::SetFileName($fileHandle, $Executable)) { throw 'Native file-name entry failed.' }
    if (![PickerTestWindows]::AcceptDialog($fileHandle)) { throw 'Native file dialog has no Open button.' }
    if (!$nativeRequest.Wait(10000)) {
        Write-Output ([PickerTestWindows]::DescribeDialog($fileHandle))
        throw 'Executable picker invocation did not return after selection.'
    }
    $deadline = [DateTime]::UtcNow.AddSeconds(10)
    do { Start-Sleep -Milliseconds 100 } while (
        (Target-Value) -ne $Executable -and [DateTime]::UtcNow -lt $deadline)
    if ((Target-Value) -ne $Executable) { throw 'Executable file selection did not fill the target path.' }
    Write-Output "EXECUTABLE_SELECTION_PASS path=$Executable"
    $nativeRequest = [PickerTestWindows]::InvokeAsync((Find-Control 'Browse or select target'))
    $deadline = [DateTime]::UtcNow.AddSeconds(15)
    do {
        Start-Sleep -Milliseconds 100
        $fileHandle = [PickerTestWindows]::FindDialog($profiler.MainWindowHandle)
    } while ($fileHandle -eq [IntPtr]::Zero -and [DateTime]::UtcNow -lt $deadline)
    if ($fileHandle -eq [IntPtr]::Zero) { throw 'Executable picker did not reopen for cancellation.' }
    $null = [PickerTestWindows]::PostMessage($fileHandle, 0x0111, [IntPtr]2, [IntPtr]::Zero)
    if (!$nativeRequest.Wait(10000)) { throw 'Executable picker cancellation did not complete.' }
    if ((Target-Value) -ne $Executable) { throw 'Cancelling the executable picker changed the target path.' }
    Write-Output 'EXECUTABLE_CANCEL_PRESERVES_TARGET_PASS'
    if (!$profiler.CloseMainWindow() -or !$profiler.WaitForExit(15000)) { throw 'Profiler failed to close after picker validation.' }
    if ($profiler.ExitCode -ne 0) { throw "Profiler exited $($profiler.ExitCode)" }
    Write-Output 'TARGET_SELECTION_UI_PASS'
}
catch {
    Write-Output $_.ScriptStackTrace
    if ($window -and (!$nativeRequest -or $nativeRequest.IsCompleted)) {
        $window.FindAll([System.Windows.Automation.TreeScope]::Descendants,
            [System.Windows.Automation.Condition]::TrueCondition) | ForEach-Object {
            Write-Output ("{0} | {1} | enabled={2}" -f $_.Current.ControlType.ProgrammaticName, $_.Current.Name, $_.Current.IsEnabled)
        }
    }
    throw
}
finally {
    $fileHandle = [PickerTestWindows]::FindDialog($profiler.MainWindowHandle)
    if ($fileHandle -ne [IntPtr]::Zero) {
        $null = [PickerTestWindows]::PostMessage($fileHandle, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero)
    }
    foreach ($owned in @($targetProcess, $profiler)) {
        if (!$owned) { continue }
        $owned.Refresh()
        if (!$owned.HasExited) {
            $null = $owned.CloseMainWindow()
            if (!$owned.WaitForExit(10000)) { Stop-Process -Id $owned.Id }
        }
        $owned.Dispose()
    }
}
