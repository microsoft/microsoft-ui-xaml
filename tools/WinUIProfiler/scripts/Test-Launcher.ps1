#requires -Version 7.4
# Runs the private WinUI launcher against a fixture app with fake MUX and MUXC DLLs.
[CmdletBinding()]
param(
    [ValidateSet('x64', 'ARM64')][string]$Architecture = 'x64',
    # Also registers, activates and removes a fixture package (requires Developer Mode or sideloading).
    [switch]$Packaged,
    # Fails when the machine already honors DotLocal redirection, so the test cannot prove the launcher works alone.
    [switch]$RequireInactiveOverride,
    # Rebuilds the launcher and its fixtures first; they are built when missing.
    [switch]$Build
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$Architecture = $Architecture.ToLowerInvariant()
if ($Architecture -eq 'arm64' -and [Runtime.InteropServices.RuntimeInformation]::OSArchitecture -ne 'Arm64') {
    throw 'The ARM64 launcher test needs an ARM64 device; building for ARM64 is not a runtime test.'
}
$root = Split-Path -Parent $PSScriptRoot
$native = Join-Path $root "artifacts\launcher\win-$Architecture"
$launcher = Join-Path $native 'ProfilerLauncher.exe'
if ($Build -or !(Test-Path -LiteralPath (Join-Path $native 'tests\StaticFixture.exe') -PathType Leaf)) {
    & (Join-Path $PSScriptRoot 'Build-Launcher.ps1') -Architecture $Architecture -IncludeTests
}
$output = Join-Path $native "tests\runs\$([guid]::NewGuid().ToString('N'))"
$hostDirectory = Join-Path $output "app space $([char]0x65e5)"
$null = New-Item -ItemType Directory -Path $hostDirectory
$fixture = Join-Path $hostDirectory 'Fixture.exe'
Copy-Item -LiteralPath (Join-Path $native 'tests\Fixture.exe') -Destination $fixture
foreach ($directory in @('original', 'private', 'other')) {
    $destination = Join-Path $hostDirectory $directory
    $null = New-Item -ItemType Directory -Path $destination
    foreach ($name in @('Microsoft.UI.Xaml.dll', 'Microsoft.UI.Xaml.Controls.dll')) {
        Copy-Item -LiteralPath (Join-Path $native "tests\$directory\$name") -Destination $destination
    }
}
$source = Join-Path $hostDirectory 'original'
$private = Join-Path $hostDirectory 'private'
$tests = [Collections.Generic.List[object]]::new()
if (-not ('ProfileFixtureErrorMode' -as [type])) {
    Add-Type -TypeDefinition @'
using System.Runtime.InteropServices;
public static class ProfileFixtureErrorMode {
    [DllImport("kernel32.dll")] public static extern uint GetErrorMode();
    [DllImport("kernel32.dll")] public static extern uint SetErrorMode(uint mode);
}
'@
}

function Invoke-FixtureProcess([string] $Executable, [string[]] $Arguments, [switch] $AllowFailure) {
    $start = [Diagnostics.ProcessStartInfo]::new($Executable)
    $start.UseShellExecute = $false
    foreach ($argument in $Arguments) { $start.ArgumentList.Add($argument) }
    $mode = [ProfileFixtureErrorMode]::GetErrorMode()
    try {
        $null = [ProfileFixtureErrorMode]::SetErrorMode($mode -bor 3)
        $process = [Diagnostics.Process]::Start($start)
    }
    finally { $null = [ProfileFixtureErrorMode]::SetErrorMode($mode) }
    try {
        if (-not $process.WaitForExit(45000)) {
            $process.Kill()
            throw "Owned fixture process timed out: $Executable"
        }
        if ($process.ExitCode -and -not $AllowFailure) {
            throw "Fixture failed ($($process.ExitCode)): $Executable; diagnostics: $output"
        }
        return $process.ExitCode
    }
    finally { $process.Dispose() }
}

function Read-FixtureResult([string] $Path) {
    $deadline = [DateTime]::UtcNow.AddSeconds(10)
    while (-not [IO.File]::Exists($Path)) {
        if ([DateTime]::UtcNow -ge $deadline) { throw "The fixture did not finish: $Path" }
        Start-Sleep -Milliseconds 50
    }
    return Get-Content -LiteralPath $Path -Raw | ConvertFrom-Json
}

function Invoke-RedirectCase([string] $Name, [string] $Mode = 'full', [bool] $Controls = $true,
    [string] $Aumid = '', [string] $PackageFullName = '', [string] $Executable = $fixture,
    [string] $Original = $source) {
    $caseDirectory = Join-Path $output $Name
    $null = New-Item -ItemType Directory -Path $caseDirectory
    $result = Join-Path $caseDirectory 'launch.json'
    $appResult = if ($Aumid) { Join-Path (Split-Path -Parent $Executable) 'fixture-result.json' }
        else { Join-Path $caseDirectory 'app.json' }
    $arguments = @('--exe', $Executable, '--source', $Original, '--private', $private,
        '--controls', $(if ($Controls) { '1' } else { '0' }),
        '--log', (Join-Path $caseDirectory 'redirect.jsonl'), '--result', $result)
    if ($Aumid) { $arguments += '--aumid', $Aumid, '--package', $PackageFullName, '--expected-aumid', $Aumid }
    else { $arguments += '--arguments', "`"$appResult`" $Mode" }
    $null = Invoke-FixtureProcess $launcher $arguments
    $launch = Read-FixtureResult $result
    $app = Read-FixtureResult $appResult
    if ($launch.error -ne 0 -or $launch.stage -cne 'ready' -or $app.mux -ne 2 -or
        $app.muxc -ne $(if ($Controls) { 2 } else { 1 })) {
        throw "Incorrect native selection in $Name; see $caseDirectory."
    }
    if ($Mode -eq 'unrelated' -and $app.unrelated -ne 3) { throw 'An unrelated same-named DLL was redirected.' }
    if ($Aumid -and ($launch.aumid -cne $Aumid -or $launch.packageFullName -cne $PackageFullName)) {
        throw 'The packaged fixture lost its package identity.'
    }
    try { $child = [Diagnostics.Process]::GetProcessById([int]$launch.pid) }
    catch [ArgumentException] { $child = $null }
    if ($child) {
        try { if (-not $child.WaitForExit(10000)) { throw "Fixture PID $($child.Id) failed to exit." } }
        finally { $child.Dispose() }
    }
    $tests.Add(@{ name = $Name; pid = $launch.pid; mux = $app.mux; muxc = $app.muxc; package = $launch.packageFullName })
}

$baselinePath = Join-Path $output 'baseline.json'
$null = Invoke-FixtureProcess $fixture @($baselinePath)
$baseline = Read-FixtureResult $baselinePath
if ($baseline.mux -ne 1 -or $baseline.muxc -ne 1) { throw 'The baseline did not load both original DLLs.' }

# Measure effective DotLocal behavior without changing its registry value or rebooting.
$marker = "$fixture.local"
$null = New-Item -ItemType Directory -Path $marker
try {
    foreach ($name in @('Microsoft.UI.Xaml.dll', 'Microsoft.UI.Xaml.Controls.dll')) {
        Copy-Item -LiteralPath (Join-Path $private $name) -Destination $marker
    }
    $controlPath = Join-Path $output 'dotlocal-control.json'
    $null = Invoke-FixtureProcess $fixture @($controlPath)
    $control = Read-FixtureResult $controlPath
    $dotLocalActive = $control.mux -eq 2 -and $control.muxc -eq 2
    if (-not $dotLocalActive -and ($control.mux -ne 1 -or $control.muxc -ne 1)) {
        throw 'Unexpected module identities in the DotLocal negative control.'
    }
    if ($RequireInactiveOverride -and $dotLocalActive) {
        throw 'This runner has effective DotLocal overrides; it cannot establish fresh-machine coverage.'
    }
}
finally {
    foreach ($name in @('Microsoft.UI.Xaml.dll', 'Microsoft.UI.Xaml.Controls.dll')) {
        $path = Join-Path $marker $name
        if ([IO.File]::Exists($path)) { [IO.File]::Delete($path) }
    }
    [IO.Directory]::Delete($marker)
}
foreach ($mode in @('full', 'bare', 'kernelbase', 'loadlibrary', 'extended', 'unrelated', 'threaded')) {
    Invoke-RedirectCase "native-$mode" $mode
}
Invoke-RedirectCase 'core-only' -Controls $false
if (Test-Path -LiteralPath $marker) { throw 'Native selection must not leave a DotLocal marker.' }

foreach ($case in @(
    @{ Name = 'invalid-controls'; Exe = $fixture; Controls = 'invalid'; Error = 87 },
    @{ Name = 'missing-executable'; Exe = (Join-Path $hostDirectory 'missing.exe'); Controls = '1'; Error = 2 }
)) {
    $caseDirectory = Join-Path $output $case.Name
    $null = New-Item -ItemType Directory -Path $caseDirectory
    $result = Join-Path $caseDirectory 'launch.json'
    $code = Invoke-FixtureProcess $launcher @('--exe', $case.Exe, '--source', $source,
        '--private', $private, '--controls', $case.Controls,
        '--log', (Join-Path $caseDirectory 'redirect.jsonl'), '--result', $result) -AllowFailure
    $failure = Read-FixtureResult $result
    if ($code -ne $case.Error -or $failure.error -ne $case.Error -or $failure.pid -ne 0) {
        throw "Native input failure did not remain explicit: $($case.Name)"
    }
    $tests.Add(@{ name = $case.Name; error = $failure.error })
}

$staticFixture = Join-Path $hostDirectory 'StaticFixture.exe'
Copy-Item -LiteralPath (Join-Path $native 'tests\StaticFixture.exe') -Destination $staticFixture
Copy-Item -LiteralPath (Join-Path $source 'Microsoft.UI.Xaml.dll') -Destination $hostDirectory
foreach ($case in @(
    @{ Name = 'missing-private-runtime'; Exe = $fixture; Private = (Join-Path $output 'absent'); Event = 'missing_private_runtime' },
    @{ Name = 'already-loaded-runtime'; Exe = $staticFixture; Private = $private; Event = 'runtime_already_loaded' }
)) {
    $caseDirectory = Join-Path $output $case.Name
    $null = New-Item -ItemType Directory -Path $caseDirectory
    $result = Join-Path $caseDirectory 'launch.json'
    $appResult = Join-Path $caseDirectory 'app.json'
    $redirectLog = Join-Path $caseDirectory 'redirect.jsonl'
    $code = Invoke-FixtureProcess $launcher @('--exe', $case.Exe, '--source', $source,
        '--private', $case.Private, '--controls', '1', '--log', $redirectLog,
        '--result', $result, '--arguments', "`"$appResult`"") -AllowFailure
    $failure = Read-FixtureResult $result
    $events = @(Get-Content -LiteralPath $redirectLog | ConvertFrom-Json)
    if (-not $code -or -not $failure.error -or $failure.stage -cne 'helper_ready' -or
        $case.Event -cnotin $events.event -or [IO.File]::Exists($appResult)) {
        throw "A startup helper rejection did not fail before the app entry point: $($case.Name)"
    }
    $tests.Add(@{ name = $case.Name; pid = $failure.pid; error = $failure.error; diagnostic = $case.Event })
}

if ($Packaged) {
    Import-Module Appx -UseWindowsPowerShell -WarningAction SilentlyContinue
    $name = "WinUIProfilerTest.$([guid]::NewGuid().ToString('N'))"
    $packageDirectory = Join-Path $output 'package'
    $assets = Join-Path $packageDirectory 'Assets'
    $packageOriginal = Join-Path $packageDirectory 'original'
    $null = New-Item -ItemType Directory -Path $assets, $packageOriginal
    Copy-Item -LiteralPath $fixture -Destination (Join-Path $packageDirectory 'Fixture.exe')
    foreach ($dll in @('Microsoft.UI.Xaml.dll', 'Microsoft.UI.Xaml.Controls.dll')) {
        Copy-Item -LiteralPath (Join-Path $source $dll) -Destination $packageOriginal
    }
    Add-Type -AssemblyName System.Drawing
    foreach ($size in @(44, 50, 150)) {
        $bitmap = [Drawing.Bitmap]::new($size, $size)
        try { $bitmap.Save((Join-Path $assets "Logo$size.png"), [Drawing.Imaging.ImageFormat]::Png) }
        finally { $bitmap.Dispose() }
    }
    $manifest = @"
<?xml version="1.0" encoding="utf-8"?>
<Package xmlns="http://schemas.microsoft.com/appx/manifest/foundation/windows10"
 xmlns:uap="http://schemas.microsoft.com/appx/manifest/uap/windows10"
 xmlns:rescap="http://schemas.microsoft.com/appx/manifest/foundation/windows10/restrictedcapabilities"
 IgnorableNamespaces="uap rescap">
 <Identity Name="$name" Publisher="CN=WinUIProfiler.LauncherTest" Version="1.0.0.0" ProcessorArchitecture="$Architecture" />
 <Properties><DisplayName>WinUI profiler native test</DisplayName><PublisherDisplayName>WinUI Profiler</PublisherDisplayName><Logo>Assets\Logo50.png</Logo></Properties>
 <Dependencies><TargetDeviceFamily Name="Windows.Desktop" MinVersion="10.0.17763.0" MaxVersionTested="10.0.26100.0" /></Dependencies>
 <Resources><Resource Language="en-us" /></Resources>
 <Applications><Application Id="App" Executable="Fixture.exe" EntryPoint="Windows.FullTrustApplication">
  <uap:VisualElements DisplayName="WinUI profiler native test" Description="Owned native launch test" BackgroundColor="transparent"
   Square150x150Logo="Assets\Logo150.png" Square44x44Logo="Assets\Logo44.png" />
 </Application></Applications>
 <Capabilities><rescap:Capability Name="runFullTrust" /></Capabilities>
</Package>
"@
    $manifestPath = Join-Path $packageDirectory 'AppxManifest.xml'
    $manifest | Set-Content -LiteralPath $manifestPath -Encoding utf8
    $registered = $false
    try {
        Add-AppxPackage -Register $manifestPath
        $registered = $true
        $package = @(Get-AppxPackage -Name $name)
        if ($package.Count -ne 1) { throw 'The owned package fixture was not registered uniquely.' }
        $aumid = "$($package[0].PackageFamilyName)!App"
        Invoke-RedirectCase 'packaged' -Aumid $aumid -PackageFullName $package[0].PackageFullName `
            -Executable (Join-Path $packageDirectory 'Fixture.exe') -Original $packageOriginal
        if (-not ('ProfileFixtureActivation' -as [type])) {
            Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class ProfileFixtureActivation {
    [ComImport, Guid("2e941141-7f97-4756-ba1d-9decde894a3d"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    private interface IActivation {
        [PreserveSig] int ActivateApplication([MarshalAs(UnmanagedType.LPWStr)] string id,
            [MarshalAs(UnmanagedType.LPWStr)] string args, uint options, out uint pid);
    }
    [ComImport, Guid("45ba127d-10a8-46ea-8ab7-56ea9078943c")]
    private class ActivationManager { }
    public static int Launch(string id) {
        var manager = (IActivation)new ActivationManager();
        try {
            uint pid;
            Marshal.ThrowExceptionForHR(manager.ActivateApplication(id, null, 2, out pid));
            return checked((int)pid);
        } finally { Marshal.ReleaseComObject(manager); }
    }
}
'@
        }
        $ordinaryPath = Join-Path $packageDirectory 'fixture-result.json'
        [IO.File]::Delete($ordinaryPath)
        $ordinaryPid = [ProfileFixtureActivation]::Launch($aumid)
        $ordinary = Read-FixtureResult $ordinaryPath
        if ($ordinary.mux -ne 1 -or $ordinary.muxc -ne 1) {
            throw 'Package debugger settings or private DLL selection survived the owned launch.'
        }
        try { $ordinaryProcess = [Diagnostics.Process]::GetProcessById($ordinaryPid) }
        catch [ArgumentException] { $ordinaryProcess = $null }
        if ($ordinaryProcess) {
            try { if (-not $ordinaryProcess.WaitForExit(10000)) { throw 'Ordinary fixture activation did not exit.' } }
            finally { $ordinaryProcess.Dispose() }
        }
        $tests.Add(@{ name = 'package-debug-options-released'; pid = $ordinaryPid; mux = 1; muxc = 1 })
    }
    finally {
        if ($registered) {
            Get-AppxPackage -Name $name | Where-Object Name -CEQ $name |
                ForEach-Object { Remove-AppxPackage -Package $_.PackageFullName }
        }
    }
}

[ordered]@{
    architecture = $Architecture; hostArchitecture = [Runtime.InteropServices.RuntimeInformation]::OSArchitecture.ToString()
    dotLocalActive = $dotLocalActive; noDotLocalMarkers = -not (Test-Path -LiteralPath $marker)
    cases = $tests.ToArray()
} | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $output 'results.json') -Encoding utf8
Write-Host "Verified $($tests.Count) private WinUI launcher cases ($Architecture; DotLocal effective=$dotLocalActive): $output"
