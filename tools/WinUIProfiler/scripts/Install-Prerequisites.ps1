#requires -Version 7.4
<#
.SYNOPSIS
Checks the tools that build WinUI Profiler and installs the missing ones.

.DESCRIPTION
Checks for Visual Studio 2022 or 2026 (any edition, including Build Tools) with MSBuild and the C++ x64 and ARM64
build tools, Windows SDK 10.0.26100.0, a .NET SDK that global.json accepts, and Git. When some are missing, it lists
them and how it will install them, asks once, and installs them all after one UAC prompt: the .NET SDK and Git with
winget, and the Visual Studio parts with the Visual Studio Installer, or as Visual Studio Build Tools 2022 from
winget when no Visual Studio can take them.

Profile-App.ps1 runs it before it builds the profiler. The script fails when something is still missing, including
when the installation is declined or the script cannot ask.

.PARAMETER Force
Installs the missing tools without asking.

.EXAMPLE
.\scripts\Install-Prerequisites.ps1

.EXAMPLE
.\scripts\Install-Prerequisites.ps1 -WhatIf
#>
[CmdletBinding(SupportsShouldProcess)]
param(
    [switch] $Force,
    # Set when the script relaunches itself with administrator rights to install what was approved.
    [Parameter(DontShow)][string] $ElevatedLogPath,
    [Parameter(DontShow)][string] $WingetPackages,
    [Parameter(DontShow)][string] $VisualStudioPath,
    [Parameter(DontShow)][string] $VisualStudioComponents
)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$vsVersions = '[17.0,19.0)'
$components = [ordered]@{
    'Microsoft.Component.MSBuild' = 'MSBuild'
    'Microsoft.VisualStudio.Component.VC.Tools.x86.x64' = 'the C++ x64 build tools'
    'Microsoft.VisualStudio.Component.VC.Tools.ARM64' = 'the C++ ARM64 build tools'
}
$sdkVersion = '10.0.26100.0'
$sdkComponent = 'Microsoft.VisualStudio.Component.Windows11SDK.26100'
$dotnetVersion = [version](Get-Content -LiteralPath (Join-Path $root 'global.json') -Raw | ConvertFrom-Json).sdk.version
$dotnetBand = '{0}.{1}' -f $dotnetVersion.Major, $dotnetVersion.Minor
$dotnetPackage = "Microsoft.DotNet.SDK.$($dotnetVersion.Major)"
$gitPackage = 'Git.Git'
$buildToolsPackage = 'Microsoft.VisualStudio.2022.BuildTools'
$installer = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer'
$vswhere = Join-Path $installer 'vswhere.exe'
$installerProcesses = 'vs_installer', 'vs_installershell', 'vs_setup_bootstrapper', 'vs_bootstrapper', 'vs_buildtools', 'setup'
# winget exit codes for installed (or already installed) and for a restart that finishes or retries the install.
$wingetSucceeded = 0, 0x8A150061, 0x8A15002B
$wingetRestart = 0x8A150109, 0x8A15010A, 0x8A15010B
$installerErrors = @{
    740 = 'it needs administrator rights'
    1001 = 'the Visual Studio Installer is already running'
    1003 = 'Visual Studio is in use'
    1602 = 'it was canceled'
    1618 = 'another installation is running'
    5003 = 'it could not download the installer'
    5004 = 'it was canceled'
    5007 = 'this machine does not meet its requirements'
    8006 = 'Visual Studio is in use'
    8010 = 'this version of Windows is not supported'
    (-1073741510) = 'it was canceled'
    (-1073720687) = 'it could not connect to the internet'
}

function Write-Status([string] $Message, [string] $Color) {
    if ($Color) { Write-Host $Message -ForegroundColor $Color } else { Write-Host $Message }
    if ($ElevatedLogPath) { [IO.File]::AppendAllText($ElevatedLogPath, "$Message`r`n") }
}

function Write-Check([bool] $Ok, [string] $Text, [string[]] $Notes) {
    if ($Ok) { Write-Status "  OK       $Text" Green } else { Write-Status "  MISSING  $Text" Yellow }
    foreach ($note in $Notes) { Write-Status "           $note" }
}

function Join-Text([string[]] $Items) {
    if ($Items.Count -lt 2) { return [string]$Items }
    return ($Items[0..($Items.Count - 2)] -join ', ') + ' and ' + $Items[-1]
}

function Get-ComponentName([string] $Id) {
    if ($Id -eq $sdkComponent) { return "Windows SDK $sdkVersion" }
    return $components[$Id]
}

function Test-Administrator {
    $identity = [Security.Principal.WindowsIdentity]::GetCurrent()
    try {
        return [Security.Principal.WindowsPrincipal]::new($identity).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
    }
    finally { $identity.Dispose() }
}

function ConvertTo-Argument([string] $Value) {
    # Quote for CommandLineToArgvW: double the backslashes before an escaped or closing quote.
    if ($Value -and $Value -notmatch '[\s"]') { return $Value }
    return '"' + (($Value -replace '(\\*)"', '$1$1\"') -replace '(\\+)$', '$1$1') + '"'
}

function Invoke-Utf8([scriptblock] $Command) {
    # vswhere -utf8 and winget write UTF-8, whatever the console code page is.
    $encoding = [Console]::OutputEncoding
    try { [Console]::OutputEncoding = [Text.UTF8Encoding]::new($false) } catch { $encoding = $null }
    try { & $Command }
    finally { if ($encoding) { [Console]::OutputEncoding = $encoding } }
}

function Get-VisualStudio {
    # Visual Studio 2022 and 2026 instances, newest first, with the required components that each one lacks.
    if (-not (Test-Path -LiteralPath $vswhere -PathType Leaf)) { return }
    $usable = @(& $vswhere -products '*' -version $vsVersions -property instanceId)
    $found = @{}
    foreach ($id in @($components.Keys) + $sdkComponent) {
        $found[$id] = @(& $vswhere -products '*' -version $vsVersions -requires $id -property instanceId)
    }
    $instances = Invoke-Utf8 { (& $vswhere -all -products '*' -version $vsVersions -format json -utf8) -join "`n" | ConvertFrom-Json }
    foreach ($instance in $instances | Sort-Object { [version]$_.installationVersion } -Descending) {
        [pscustomobject]@{
            Name = "$($instance.displayName) $($instance.catalog.productDisplayVersion)".Trim()
            Path = [string]$instance.installationPath
            Product = [string]$instance.productId
            Version = [version]$instance.installationVersion
            Usable = $usable -contains $instance.instanceId
            RebootRequired = $instance.isRebootRequired -eq $true
            Missing = @($components.Keys | Where-Object { $found[$_] -notcontains $instance.instanceId })
            HasSdk = $found[$sdkComponent] -contains $instance.instanceId
        }
    }
}

function Test-WindowsSdk {
    # The headers, libraries and tools that the native projects, the launcher and Profile-App.ps1 use.
    $tools = [Runtime.InteropServices.RuntimeInformation]::ProcessArchitecture.ToString().ToLowerInvariant()
    $files = @("Include\$sdkVersion\um\Windows.h", "Include\$sdkVersion\ucrt\stdio.h", "bin\$sdkVersion\x64\cppwinrt.exe",
        "bin\$sdkVersion\$tools\muirct.exe", "Platforms\UAP\$sdkVersion\Platform.xml", "References\$sdkVersion")
    foreach ($architecture in @('x64', 'arm64')) {
        $files += "Lib\$sdkVersion\um\$architecture\kernel32.lib", "Lib\$sdkVersion\ucrt\$architecture\ucrt.lib"
    }
    $roots = @(Join-Path ([Environment]::GetFolderPath('ProgramFilesX86')) 'Windows Kits\10')
    foreach ($key in @('HKLM:\SOFTWARE\Microsoft\Windows Kits\Installed Roots',
        'HKLM:\SOFTWARE\WOW6432Node\Microsoft\Windows Kits\Installed Roots')) {
        if (Test-Path -LiteralPath $key) {
            $property = (Get-ItemProperty -LiteralPath $key).PSObject.Properties['KitsRoot10']
            if ($property -and $property.Value) { $roots += [string]$property.Value }
        }
    }
    foreach ($kitsRoot in $roots) {
        if (-not ($files | Where-Object { -not (Test-Path -LiteralPath (Join-Path $kitsRoot $_)) })) { return $true }
    }
    return $false
}

function Get-DotnetSdkVersion {
    # The SDK that global.json selects, when one is installed.
    if (-not (Get-Command dotnet -CommandType Application -ErrorAction SilentlyContinue)) { return }
    Push-Location -LiteralPath $root
    try { $output = @(& dotnet --version 2>&1 | ForEach-Object { "$_".Trim() } | Where-Object { $_ }) }
    finally { Pop-Location }
    if ($LASTEXITCODE -eq 0 -and $output.Count) { return $output[-1] }
}

function Get-GitVersion {
    if (-not (Get-Command git -CommandType Application -ErrorAction SilentlyContinue)) { return }
    $output = @(& git --version 2>&1 | ForEach-Object { "$_" })
    if ($LASTEXITCODE -eq 0 -and $output.Count) { return $output[0] -replace '^git version\s*', '' }
}

function Get-Plan {
    # Prints the checklist and returns what is missing and how this script installs it.
    $plan = [ordered]@{ Missing = [ordered]@{}; Manual = @(); Packages = @(); VisualStudioPath = ''; Components = @(); Steps = @() }
    Write-Status 'Checking the WinUI Profiler build prerequisites:'

    $instances = @(Get-VisualStudio)
    $usable = @($instances | Where-Object Usable)
    $ready = $usable | Where-Object { -not $_.Missing.Count } | Select-Object -First 1
    $target = if ($ready) { $ready } else {
        # The edition that lacks the fewest components, newest first; products such as Team Explorer can't take them.
        $usable | Where-Object Product -Match '\.(Community|Professional|Enterprise|BuildTools)$' |
            Sort-Object { $_.Missing.Count }, @{ Expression = 'Version'; Descending = $true } | Select-Object -First 1
    }
    if ($ready) { Write-Check $true "$($ready.Name) with MSBuild and the C++ x64 and ARM64 build tools" }
    else {
        $notes = foreach ($instance in $instances | Where-Object { -not $_.Usable }) {
            $fix = if ($instance.RebootRequired) { 'restart Windows to finish its installation' } else { 'repair it in the Visual Studio Installer' }
            "$($instance.Name) is not finished: $fix."
        }
        $text = if ($target) { "$(Join-Text @($target.Missing | ForEach-Object { $components[$_] })) in $($target.Name)" }
        else { 'Visual Studio 2022 or 2026 with MSBuild and the C++ x64 and ARM64 build tools' }
        Write-Check $false $text $notes
        $plan.Missing['Visual Studio C++ build tools'] = 'https://aka.ms/vs/17/release/vs_buildtools.exe'
        $plan.Components = @(if ($target) { $target.Missing } else { $components.Keys })
    }

    $sdkLink = 'https://developer.microsoft.com/windows/downloads/windows-sdk/'
    if (Test-WindowsSdk) { Write-Check $true "Windows SDK $sdkVersion" }
    else {
        $plan.Missing["Windows SDK $sdkVersion"] = $sdkLink
        if ($target -and $target.HasSdk) {
            $plan.Manual += "Repair $($target.Name) in the Visual Studio Installer, or install Windows SDK $sdkVersion from $sdkLink."
            Write-Check $false "Windows SDK $sdkVersion" $plan.Manual[-1]
        }
        else {
            Write-Check $false "Windows SDK $sdkVersion"
            $plan.Components += $sdkComponent
        }
    }

    $dotnet = Get-DotnetSdkVersion
    if ($dotnet) { Write-Check $true ".NET SDK $dotnet" }
    else {
        Write-Check $false ".NET SDK $dotnetVersion or a later $dotnetBand SDK"
        $plan.Missing[".NET SDK $dotnetVersion"] = "https://dotnet.microsoft.com/download/dotnet/$dotnetBand"
        $plan.Packages += $dotnetPackage
        $plan.Steps += "install the latest .NET $dotnetBand SDK (winget package $dotnetPackage)"
    }

    $git = Get-GitVersion
    if ($git) { Write-Check $true "Git $git" }
    else {
        Write-Check $false 'Git'
        $plan.Missing['Git'] = 'https://git-scm.com/download/win'
        $plan.Packages += $gitPackage
        $plan.Steps += "install Git (winget package $gitPackage)"
    }

    if ($plan.Components) {
        $names = Join-Text @($plan.Components | ForEach-Object { Get-ComponentName $_ })
        if ($target) {
            $plan.VisualStudioPath = $target.Path
            $plan.Steps += "add $names to $($target.Name) with the Visual Studio Installer"
        }
        else {
            $plan.Packages += $buildToolsPackage
            $plan.Steps += "install Visual Studio Build Tools 2022 with $names (winget package $buildToolsPackage)"
        }
    }
    return $plan
}

function Test-VisualStudioBusy([string] $Path) {
    # True while the Visual Studio Installer, or Visual Studio from $Path, runs.
    foreach ($process in @(Get-Process -Name ($installerProcesses + 'devenv') -ErrorAction SilentlyContinue)) {
        $file = try { $process.Path } catch { $null }
        if ($process.ProcessName -eq 'devenv') {
            if ($Path -and $file -and $file.StartsWith("$Path\", [StringComparison]::OrdinalIgnoreCase)) { return $true }
        }
        elseif ($process.ProcessName -ne 'setup' -or ($file -and $file.StartsWith("$installer\", [StringComparison]::OrdinalIgnoreCase))) {
            return $true
        }
    }
    return $false
}

function Wait-VisualStudioInstaller([string] $Path, [string] $Message, [int] $Minutes) {
    $deadline = [DateTime]::UtcNow.AddMinutes($Minutes)
    $shown = $false
    while (Test-VisualStudioBusy $Path) {
        if ([DateTime]::UtcNow -ge $deadline) { return $false }
        if (-not $shown) { Write-Status $Message; $shown = $true }
        Start-Sleep -Seconds 5
    }
    return $true
}

function Invoke-Winget([string[]] $Arguments) {
    Invoke-Utf8 {
        & winget @Arguments 2>&1 | ForEach-Object {
            # Keep the final text of lines that winget redraws, and drop spinners, progress bars and sizes.
            $line = @(("$_" -replace '\x1b\[[0-9;?]*[ -/]*[@-~]', '') -split "`r" | ForEach-Object Trim | Where-Object { $_ })
            if ($line.Count -and $line[-1] -notmatch '^([-\\|/]|[\u2588\u2592\u2501].*|[\d.]+\s*[KMG]?B\s*/\s*[\d.]+\s*[KMG]?B|\d+%)$') {
                Write-Status "    $($line[-1])"
            }
        }
    }
    return $LASTEXITCODE
}

function Invoke-Process([string] $FilePath, [string[]] $Arguments) {
    $info = [Diagnostics.ProcessStartInfo]::new($FilePath)
    foreach ($argument in $Arguments) { $info.ArgumentList.Add($argument) }
    $info.UseShellExecute = $false
    $info.WorkingDirectory = [IO.Path]::GetTempPath()
    $process = [Diagnostics.Process]::Start($info)
    try {
        $process.WaitForExit()
        return $process.ExitCode
    }
    finally { $process.Dispose() }
}

function Install-Prerequisite([string[]] $Packages, [string] $Path, [string[]] $Add) {
    # Installs the winget packages, then adds $Add to the Visual Studio at $Path. Returns 0, 3010 when Windows must
    # restart to finish, or 1 when something failed.
    $restart = $false
    $failed = $false
    if ($Packages -and -not (Get-Command winget -CommandType Application -ErrorAction SilentlyContinue)) {
        Write-Status 'winget was not found, so nothing was installed with it.' Red
        $failed = $true
        $Packages = @()
    }
    foreach ($package in $Packages) {
        $arguments = @('install', '--id', $package, '--exact', '--source', 'winget', '--scope', 'machine',
            '--accept-package-agreements', '--accept-source-agreements', '--disable-interactivity')
        if ($package -eq $buildToolsPackage) {
            if (-not (Wait-VisualStudioInstaller '' 'Close the Visual Studio Installer to continue.' 10)) {
                Write-Status 'The Visual Studio Installer is still running, so Build Tools was not installed.' Red
                $failed = $true
                continue
            }
            $options = @('Microsoft.VisualStudio.Workload.VCTools') + $Add | ForEach-Object { "--add $_" }
            $arguments += '--override', "--passive --wait --norestart $($options -join ' ')"
        }
        else { $arguments += '--silent' }
        Write-Status "Installing $package with winget."
        $code = Invoke-Winget $arguments
        if ($package -eq $buildToolsPackage) {
            $null = Wait-VisualStudioInstaller '' 'Waiting for the Visual Studio Installer to finish.' 60
        }
        if ($code -in $wingetRestart) { $restart = $true }
        elseif ($code -notin $wingetSucceeded) {
            Write-Status ('winget could not install {0} (exit code 0x{1:X8}).' -f $package, $code) Red
            $failed = $true
        }
    }
    if ($Path) {
        if (Wait-VisualStudioInstaller $Path 'Close Visual Studio and the Visual Studio Installer to continue.' 10) {
            Write-Status "Adding $(Join-Text @($Add | ForEach-Object { Get-ComponentName $_ })) to $Path with the Visual Studio Installer."
            $arguments = @('modify', '--installPath', $Path) + @($Add | ForEach-Object { '--add', $_ }) + @('--passive', '--norestart')
            $code = Invoke-Process (Join-Path $installer 'setup.exe') $arguments
            $null = Wait-VisualStudioInstaller '' 'Waiting for the Visual Studio Installer to finish.' 60
            if ($code -in 3010, 1641) { $restart = $true }
            elseif ($code -ne 0) {
                $reason = if ($installerErrors.ContainsKey($code)) { " because $($installerErrors[$code])" }
                Write-Status "The Visual Studio Installer failed$reason (exit code $code). Its logs are the dd_*.log files in $([IO.Path]::GetTempPath())." Red
                $failed = $true
            }
        }
        else {
            Write-Status 'Visual Studio or the Visual Studio Installer is still running, so nothing was added to Visual Studio.' Red
            $failed = $true
        }
    }
    if ($failed) { return 1 }
    if ($restart) { return 3010 }
    return 0
}

function Invoke-Elevated([Collections.IDictionary] $Parameters) {
    # Relaunches this script through UAC to install, streams its log, and returns its exit code.
    $log = Join-Path ([IO.Path]::GetTempPath()) "WinUIProfiler\install-prerequisites-$([guid]::NewGuid().ToString('N')).log"
    $null = New-Item -ItemType Directory -Force -Path (Split-Path -Parent $log)
    $arguments = @('-NoProfile', '-ExecutionPolicy', [string](Get-ExecutionPolicy), '-File', $PSCommandPath)
    foreach ($entry in $Parameters.GetEnumerator()) {
        if ($entry.Value) { $arguments += "-$($entry.Key):", [string]$entry.Value }
    }
    $arguments += '-ElevatedLogPath:', $log
    Write-Status 'Installing needs administrator rights. Approve the Windows UAC prompt to continue.'
    try {
        $child = Start-Process -FilePath ([Environment]::ProcessPath) -Verb RunAs -WindowStyle Hidden -PassThru `
            -ArgumentList (($arguments | ForEach-Object { ConvertTo-Argument $_ }) -join ' ')
    }
    catch { throw "Administrator rights were not granted, so nothing was installed: $($_.Exception.Message)" }
    $reader = $null
    try {
        do {
            $exited = $child.WaitForExit(1000)
            if ($null -eq $reader -and [IO.File]::Exists($log)) {
                $reader = [IO.StreamReader]::new([IO.FileStream]::new($log, 'Open', 'Read', 'ReadWrite, Delete'))
            }
            while ($null -ne $reader -and $null -ne ($line = $reader.ReadLine())) { Write-Host $line }
        } until ($exited)
    }
    finally {
        if ($null -ne $reader) { $reader.Dispose() }
        Remove-Item -LiteralPath $log -Force -ErrorAction SilentlyContinue
    }
    if ($null -eq $reader) { Write-Warning 'The administrator run wrote no log.' }
    return $child.ExitCode
}

function Import-RegistryPath {
    # Adds the PATH entries that the installers registered, after this session's own entries.
    $seen = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    $entries = @($env:Path -split ';') + @([Environment]::GetEnvironmentVariable('Path', 'Machine') -split ';') +
        @([Environment]::GetEnvironmentVariable('Path', 'User') -split ';')
    $env:Path = @($entries | Where-Object { $_ -and $seen.Add($_.TrimEnd('\')) }) -join ';'
}

if ($ElevatedLogPath) {
    $code = 1
    try {
        if (-not (Test-Administrator)) { throw 'The relaunched script does not have administrator rights.' }
        $packages = @($WingetPackages -split ',' | Where-Object { $_ })
        $add = @($VisualStudioComponents -split ',' | Where-Object { $_ })
        # Install only what this script offers.
        if ($packages | Where-Object { $_ -cnotin $dotnetPackage, $gitPackage, $buildToolsPackage }) {
            throw "Unexpected winget package in $WingetPackages."
        }
        if ($add | Where-Object { $_ -cnotin (@($components.Keys) + $sdkComponent) }) {
            throw "Unexpected Visual Studio component in $VisualStudioComponents."
        }
        if ($VisualStudioPath -and @(Get-VisualStudio | Where-Object Usable | ForEach-Object Path) -notcontains $VisualStudioPath) {
            throw "Visual Studio was not found at $VisualStudioPath."
        }
        $code = Install-Prerequisite $packages $VisualStudioPath $add
    }
    catch { Write-Status "Installing failed: $($_.Exception.Message)" Red }
    exit $code
}

try {
    if (-not $IsWindows) { throw 'Install-Prerequisites.ps1 requires Windows.' }
    $plan = Get-Plan
    if (-not $plan.Missing.Count) { return }
    $missing = Join-Text @($plan.Missing.Keys)
    $links = -join @($plan.Missing.GetEnumerator() | ForEach-Object { "`n  $($_.Key): $($_.Value)" })
    if (-not $plan.Steps.Count) { throw "Missing: $missing. $($plan.Manual -join ' ')" }
    if ($plan.Packages.Count -and -not (Get-Command winget -CommandType Application -ErrorAction SilentlyContinue)) {
        throw "Missing: $missing. Installing them needs winget (https://aka.ms/getwinget); or install them yourself:$links"
    }
    Write-Status 'To install them, the script will:'
    foreach ($step in $plan.Steps) { Write-Status "  - $step" }
    $administrator = Test-Administrator
    $notes = @(if ($plan.Components) { 'can download several GB' }; if (-not $administrator) { 'asks for administrator rights once' })
    if ($notes) { Write-Status "This $(Join-Text $notes)." }
    if (-not $PSCmdlet.ShouldProcess($missing, 'Install')) { return }
    if (-not $Force) {
        $choice = try { $Host.UI.PromptForChoice($null, 'Install them now?', @('&Yes', '&No'), 0) } catch { 1 }
        if ($choice -ne 0) {
            throw "Nothing was installed. Missing: $missing. Run .\scripts\Install-Prerequisites.ps1 to install them (add -Force where it cannot ask), or install them yourself:$links"
        }
    }
    $code = if ($administrator) { Install-Prerequisite $plan.Packages $plan.VisualStudioPath $plan.Components }
    else {
        Invoke-Elevated ([ordered]@{
                WingetPackages = $plan.Packages -join ','
                VisualStudioPath = $plan.VisualStudioPath
                VisualStudioComponents = $plan.Components -join ','
            })
    }
    Import-RegistryPath
    $plan = Get-Plan
    if ($plan.Missing.Count) {
        $message = "Still missing: $(Join-Text @($plan.Missing.Keys))."
        if ($code -eq 3010) { $message += ' Restart Windows, then run the script again.' }
        throw "$message $($plan.Manual -join ' ')".Trim()
    }
    if ($code -eq 3010) { Write-Warning 'Restart Windows soon to finish the installation.' }
}
catch {
    # A new exception shows as "Install-Prerequisites.ps1: <message>" instead of "Exception: <message>".
    $PSCmdlet.ThrowTerminatingError([Management.Automation.ErrorRecord]::new(
            [InvalidOperationException]::new($_.Exception.Message), 'InstallPrerequisites', 'NotInstalled', $null))
}
