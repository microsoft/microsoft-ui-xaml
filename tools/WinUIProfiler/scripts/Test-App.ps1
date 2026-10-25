param(
    [Parameter(Mandatory = $true)][string]$Deployment,
    [Parameter(Mandatory = $true)][string]$SessionPath,
    [Parameter(Mandatory = $true)][string]$ExpectedSource,
    [Parameter(Mandatory = $true)][string]$ExpectedValue,
    [string]$ExpectedNextValue,
    [switch]$VerifyReadError
)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName UIAutomationClient
Add-Type -AssemblyName UIAutomationTypes
$manifest = Get-Content $Deployment -Raw | ConvertFrom-Json
if ($manifest.Architecture -eq 'ARM64' -and $env:PROCESSOR_ARCHITECTURE -ne 'ARM64') {
    throw 'An ARM64 cross-build cannot satisfy native ARM64 UI execution.'
}
$directory = [IO.Path]::GetDirectoryName($manifest.Executable)
$selected = (Resolve-Path $SessionPath).Path
$sessionHash = (Get-FileHash -LiteralPath $selected -Algorithm SHA256).Hash
$process = Start-Process $manifest.Executable -ArgumentList @('--session', "`"$selected`"") `
    -WorkingDirectory $directory -PassThru `
    -RedirectStandardOutput (Join-Path $directory 'ProfilerApp.stdout.log') `
    -RedirectStandardError (Join-Path $directory 'ProfilerApp.stderr.log')
try {
    $null = $process.Handle
    $deadline = [DateTime]::UtcNow.AddSeconds(45)
    $sourceElement = $null
    do {
        Start-Sleep -Milliseconds 100
        $process.Refresh()
        if ($process.HasExited) { throw "Profiler application exited before readiness: $($process.ExitCode)" }
        if ($process.MainWindowHandle -ne [IntPtr]::Zero) {
            $window = [System.Windows.Automation.AutomationElement]::FromHandle($process.MainWindowHandle)
            $condition = [System.Windows.Automation.PropertyCondition]::new(
                [System.Windows.Automation.AutomationElement]::NameProperty, $ExpectedSource)
            $sourceElement = $window.FindFirst([System.Windows.Automation.TreeScope]::Descendants, $condition)
        }
    } while (!$sourceElement -and [DateTime]::UtcNow -lt $deadline)
    if (!$sourceElement) { throw 'The actual WinUI list never exposed the expected saved metric source through UI Automation.' }
    function Wait-AppText([string]$text) {
        $until = [DateTime]::UtcNow.AddSeconds(15)
        do {
            if ($process.HasExited) { throw "Profiler application exited during UI validation: $($process.ExitCode)" }
            $condition = [System.Windows.Automation.PropertyCondition]::new(
                [System.Windows.Automation.AutomationElement]::NameProperty, $text)
            $found = $window.FindFirst([System.Windows.Automation.TreeScope]::Descendants, $condition)
            if ($found -and !$found.Current.IsOffscreen) { return $found }
            Start-Sleep -Milliseconds 100
        } while ([DateTime]::UtcNow -lt $until)
        throw "The saved-session UI did not expose expected visible text: $text"
    }
    function Invoke-AppButton([string]$name) {
        $button = Wait-AppText $name
        if (!$button.Current.IsEnabled) { throw "Expected an enabled button: $name" }
        $pattern = $button.GetCurrentPattern([System.Windows.Automation.InvokePattern]::Pattern)
        $pattern.Invoke()
    }
    function Assert-Navigation([bool]$previous, [bool]$next) {
        $previousButton = Wait-AppText 'Previous page'
        $nextButton = Wait-AppText 'Next page'
        if ($previousButton.Current.IsEnabled -ne $previous -or $nextButton.Current.IsEnabled -ne $next) {
            throw "Incorrect navigation state: expected previous=$previous next=$next"
        }
    }
    foreach ($text in @($ExpectedValue, 'Measured / Partial', 'Next page')) {
        $condition = [System.Windows.Automation.PropertyCondition]::new(
            [System.Windows.Automation.AutomationElement]::NameProperty, $text)
        $element = $window.FindFirst([System.Windows.Automation.TreeScope]::Descendants, $condition)
        if (!$element) { throw "The saved-session UI did not expose expected text: $text" }
        if ($text -eq 'Next page' -and $element.Current.IsEnabled -ne [bool]$ExpectedNextValue) {
            throw "Incorrect initial Next state: expected enabled=$([bool]$ExpectedNextValue)"
        }
    }
    Assert-Navigation $false ([bool]$ExpectedNextValue)
    if ($ExpectedNextValue) {
        Invoke-AppButton 'Next page'
        $null = Wait-AppText $ExpectedNextValue
        Assert-Navigation $true $false
        Invoke-AppButton 'Previous page'
        $null = Wait-AppText $ExpectedValue
        Assert-Navigation $false $true
    }
    if ($VerifyReadError) {
        $pathField = Wait-AppText 'Saved session database path'
        $valuePattern = $pathField.GetCurrentPattern([System.Windows.Automation.ValuePattern]::Pattern)
        $missing = "$selected.missing.$([Guid]::NewGuid().ToString('N'))"
        $valuePattern.SetValue($missing)
        Invoke-AppButton 'Open'
        $null = Wait-AppText 'Session query failed'
        $null = Wait-AppText $ExpectedValue
        Assert-Navigation $false ([bool]$ExpectedNextValue)
        if (Test-Path -LiteralPath $missing) { throw 'Opening a missing saved session created a database.' }
        $valuePattern.SetValue($selected)
        Invoke-AppButton 'Open'
        $until = [DateTime]::UtcNow.AddSeconds(15)
        do {
            $errorCondition = [System.Windows.Automation.PropertyCondition]::new(
                [System.Windows.Automation.AutomationElement]::NameProperty, 'Session query failed')
            $errorElement = $window.FindFirst([System.Windows.Automation.TreeScope]::Descendants, $errorCondition)
            $openButton = Wait-AppText 'Open'
            if ((!$errorElement -or $errorElement.Current.IsOffscreen) -and $openButton.Current.IsEnabled) { break }
            Start-Sleep -Milliseconds 100
        } while ([DateTime]::UtcNow -lt $until)
        if (($errorElement -and !$errorElement.Current.IsOffscreen) -or !$openButton.Current.IsEnabled) {
            throw 'The actual UI did not recover after reopening the valid session.'
        }
        $null = Wait-AppText $ExpectedValue
        Assert-Navigation $false ([bool]$ExpectedNextValue)
    }
    $expected = @{}
    foreach ($file in $manifest.RuntimePayloads) {
        $expected[[IO.Path]::GetFileName($file.Path)] = $file
    }
    $loaded = @()
    $process.Refresh()
    foreach ($module in $process.Modules) {
        if (!$expected.ContainsKey($module.ModuleName)) { continue }
        $entry = $expected[$module.ModuleName]
        $actual = [IO.Path]::GetFullPath($module.FileName)
        if ($actual -ine [IO.Path]::GetFullPath($entry.Path) -or
            (Get-FileHash $actual -Algorithm SHA256).Hash -ne $entry.SHA256) {
            throw "The actual process loaded an unexpected runtime payload: $actual"
        }
        $loaded += [ordered]@{ Path = $actual; SHA256 = $entry.SHA256 }
    }
    if (!($loaded | Where-Object { [IO.Path]::GetFileName($_.Path) -ieq 'Microsoft.UI.Xaml.dll' })) {
        throw 'The actual profiler did not load the manifest-verified local WinUI runtime.'
    }
    $loaded | ConvertTo-Json -Depth 4 | Set-Content (Join-Path $directory 'ProfilerAppLoadedModules.json') -Encoding UTF8
    if (!$process.CloseMainWindow() -or !$process.WaitForExit(10000)) { throw 'The actual profiler failed clean window shutdown.' }
    if ($process.ExitCode -ne 0) { throw "Profiler shutdown failed: $($process.ExitCode)" }
    if ((Get-FileHash -LiteralPath $selected -Algorithm SHA256).Hash -ne $sessionHash) {
        throw 'Read-only App inspection changed the selected database file.'
    }
    Write-Output "APP_UI_PASS: $($process.Id) source=$ExpectedSource value=$ExpectedValue quality=Measured/Partial paging=$([bool]$ExpectedNextValue) recovery=$VerifyReadError exit=$($process.ExitCode)"
    $loaded | ForEach-Object { Write-Output "APP_RUNTIME_VERIFIED: $($_.Path) SHA256=$($_.SHA256)" }
}
finally {
    try { if (!$process.HasExited) { Stop-Process -Id $process.Id } }
    finally { $process.Dispose() }
}
