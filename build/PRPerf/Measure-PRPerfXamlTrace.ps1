[CmdletBinding()]
param(
    [Parameter(Mandatory)][string] $PayloadRoot,
    [Parameter(Mandatory)][string] $OutputPath,
    [string] $AppPackageNamePattern = '*CppDesktopSampleApp*',
    [int] $SampleCount = 3,
    [int] $WarmupCount = 1,
    [int] $LaunchTimeoutSeconds = 60,
    [string] $WprpPath
)

<#
    Launches a real WinUI app and reads the XAML startup regions out of an ETW trace.

    This is the experimental half of the pull request perf check and it deliberately never
    fails. It depends on appx deployment, a trace session and a decoder on a lab agent, and
    if any of those is unavailable the useful thing to do is say nothing and leave the
    comparison that does work exactly as it was. So the script reports what it managed to
    measure, writes no file when it measured nothing, and always exits zero.

    Only the pull request build is measured. Comparing both sides would need two versions of
    the framework package installed at the same time, which cannot be done, so these numbers
    are an observation rather than a verdict.
#>

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
if ([string]::IsNullOrWhiteSpace($WprpPath)) { $WprpPath = Join-Path $root 'PRPerfXaml.wprp' }
Import-Module (Join-Path $root 'PRPerfTrace.psm1') -Force
Import-Module (Join-Path $root 'PRPerfResults.psm1') -Force

$script:ActivationSource = @'
using System;
using System.Runtime.InteropServices;

public static class PRPerfActivator
{
    [ComImport, Guid("2e941141-7f97-4756-ba1d-9decde894a3d"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    private interface IApplicationActivationManager
    {
        int ActivateApplication(
            [MarshalAs(UnmanagedType.LPWStr)] string appUserModelId,
            [MarshalAs(UnmanagedType.LPWStr)] string arguments,
            uint options,
            out uint processId);
    }

    [ComImport, Guid("45BA127D-10A8-46EA-8AB7-56EA9078943C")]
    private class ApplicationActivationManager { }

    public static uint Activate(string appUserModelId)
    {
        var manager = (IApplicationActivationManager)new ApplicationActivationManager();
        uint processId;
        int hr = manager.ActivateApplication(appUserModelId, null, 0, out processId);
        if (hr < 0) { Marshal.ThrowExceptionForHR(hr); }
        return processId;
    }
}
'@

function Write-Note([string] $Message) { Write-Host "[xaml-trace] $Message" }

function Invoke-PRPerfNative {
    <#
        wpr and tracerpt both write ordinary progress to the error stream, which this script
        would otherwise treat as a failure and abort on. Their exit code is the only thing
        worth believing, and even that is only advisory here.
    #>
    param([string] $FileName, [string[]] $Arguments)

    $previous = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try {
        & $FileName @Arguments 2>&1 | Out-Null
        return $LASTEXITCODE
    } catch {
        Write-Note "$FileName $($Arguments -join ' ') did not run: $($_.Exception.Message)"
        return -1
    } finally {
        $ErrorActionPreference = $previous
    }
}

function Install-PayloadCertificates {
    param([string] $Root)

    foreach ($certificate in @(Get-ChildItem -LiteralPath $Root -Filter '*.cer' -Recurse -ErrorAction SilentlyContinue)) {
        & certutil.exe -addstore TrustedPeople $certificate.FullName | Out-Null
        Write-Note "Trusted $($certificate.Name)."
    }
}

function Install-PayloadPackages {
    param([string] $Root)

    # Sideloading a test signed package needs the machine to allow it. The scenario test
    # jobs in this repository set exactly the same policy before deploying their payload.
    & reg.exe add 'HKLM\Software\Policies\Microsoft\Windows\Appx' /v AllowDevelopmentWithoutDevLicense /t REG_DWORD /d 1 /f | Out-Null
    Install-PayloadCertificates -Root $Root

    $packages = @(Get-ChildItem -LiteralPath $Root -Include '*.msix', '*.appx' -Recurse -ErrorAction SilentlyContinue)
    if ($packages.Count -eq 0) { throw "No appx or msix packages were found under '$Root'." }

    # The framework package has to be present before the app that depends on it, and there
    # is no reliable ordering in the payload, so every package is simply offered twice.
    foreach ($pass in 1..2) {
        foreach ($package in $packages) {
            try {
                Add-AppxPackage -Path $package.FullName -ForceUpdateFromAnyVersion -ErrorAction Stop
                Write-Note "Installed $($package.Name)."
            } catch {
                if ($pass -eq 2) { Write-Note "Could not install $($package.Name): $($_.Exception.Message)" }
            }
        }
    }
}

function Get-MeasuredAppUserModelId {
    param([string] $NamePattern)

    $package = @(Get-AppxPackage | Where-Object { $_.Name -like $NamePattern -or $_.PackageFullName -like $NamePattern }) |
        Select-Object -First 1
    if ($null -eq $package) { throw "No installed package matched '$NamePattern'." }

    $entry = @(Get-AppxPackageManifest -Package $package.PackageFullName).Package.Applications.Application |
        Select-Object -First 1
    if ($null -eq $entry) { throw "Package '$($package.PackageFullName)' declares no application to launch." }

    return [pscustomobject]@{
        PackageFullName = $package.PackageFullName
        AppUserModelId = "$($package.PackageFamilyName)!$($entry.Id)"
    }
}

function Assert-ProcessUsesMeasuredXaml {
    <#
        Without this the numbers could come from a XAML that has nothing to do with the
        pull request, and would look perfectly reasonable while measuring the wrong code.
    #>
    param([int] $ProcessId)

    try {
        $modules = (Get-Process -Id $ProcessId -ErrorAction Stop).Modules
    } catch {
        Write-Note "Could not read the loaded modules of process $ProcessId."
        return
    }

    $xaml = @($modules | Where-Object { $_.ModuleName -ilike 'Microsoft.UI.Xaml.dll' }) | Select-Object -First 1
    if ($null -eq $xaml) {
        Write-Note 'The launched app has not loaded Microsoft.UI.Xaml.dll.'
        return
    }
    Write-Note "The launched app loaded $($xaml.FileName)."
}

function Measure-OneLaunch {
    param([string] $AppUserModelId, [string] $WorkingDirectory, [int] $Index)

    $etl = Join-Path $WorkingDirectory "launch-$Index.etl"
    $xml = Join-Path $WorkingDirectory "launch-$Index.xml"

    $null = Invoke-PRPerfNative wpr.exe @('-cancel')
    $started = Invoke-PRPerfNative wpr.exe @('-start', "$WprpPath!PRPerfXaml", '-filemode')
    if ($started -ne 0) { throw "wpr could not start a trace (exit $started)." }

    $processId = 0
    try {
        $processId = [int][PRPerfActivator]::Activate($AppUserModelId)
        Write-Note "Launched $AppUserModelId as process $processId."

        # The regions all complete during startup. Waiting for the process to go idle is
        # the closest thing to "the window is up" that needs no window handle.
        $deadline = (Get-Date).AddSeconds($LaunchTimeoutSeconds)
        while ((Get-Date) -lt $deadline) {
            Start-Sleep -Milliseconds 500
            $process = Get-Process -Id $processId -ErrorAction SilentlyContinue
            if ($null -eq $process) { break }
            if ($process.MainWindowHandle -ne [IntPtr]::Zero) { Start-Sleep -Seconds 2; break }
        }
        if ($Index -eq 0) { Assert-ProcessUsesMeasuredXaml -ProcessId $processId }
    } finally {
        $null = Invoke-PRPerfNative wpr.exe @('-stop', $etl)
        if ($processId -ne 0) { Stop-Process -Id $processId -Force -ErrorAction SilentlyContinue }
    }

    # tracerpt is the only decoder on the lab image. Its restricted mode drops events whose
    # payload does not match the schema, which for these regions would mean measuring
    # nothing at all, so the relaxed dump is used as a fallback.
    $null = Invoke-PRPerfNative tracerpt.exe @($etl, '-o', $xml, '-of', 'XML', '-y')
    if (-not (Test-Path -LiteralPath $xml)) {
        $null = Invoke-PRPerfNative tracerpt.exe @($etl, '-o', $xml, '-of', 'XML', '-lr', '-y')
    }
    if (-not (Test-Path -LiteralPath $xml)) { throw "tracerpt produced no output for '$etl'." }

    return Get-PRPerfXamlRegionDurations -Xml (Get-Content -LiteralPath $xml -Raw) -ProcessId $processId
}

$workingDirectory = Join-Path ([System.IO.Path]::GetTempPath()) "prperf-xaml-$([guid]::NewGuid())"
$measurements = @{}
try {
    if (-not (Test-Path -LiteralPath $WprpPath -PathType Leaf)) { throw "Recording profile not found: $WprpPath" }
    New-Item -ItemType Directory -Path $workingDirectory -Force | Out-Null
    Add-Type -TypeDefinition $script:ActivationSource -Language CSharp

    Install-PayloadPackages -Root $PayloadRoot
    $app = Get-MeasuredAppUserModelId -NamePattern $AppPackageNamePattern
    Write-Note "Measuring $($app.AppUserModelId)."

    for ($run = 0; $run -lt ($WarmupCount + $SampleCount); $run++) {
        try {
            $regions = Measure-OneLaunch -AppUserModelId $app.AppUserModelId -WorkingDirectory $workingDirectory -Index $run
        } catch {
            Write-Note "Launch $run did not produce a trace: $($_.Exception.Message)"
            continue
        }

        # The first launch pages the app and its framework in from disk, which measures the
        # file system rather than XAML, so it is discarded rather than averaged in.
        if ($run -lt $WarmupCount) {
            Write-Note "Discarded warmup launch $run ($(@($regions.Keys) -join ', '))."
            continue
        }

        foreach ($name in @($regions.Keys)) {
            if (-not $measurements.ContainsKey($name)) { $measurements[$name] = @() }
            $measurements[$name] += [double]$regions[$name]
        }
        Write-Note "Launch $run measured $(@($regions.Keys) -join ', ')."
    }
} catch {
    Write-Note "No XAML regions were measured: $($_.Exception.Message)"
} finally {
    $null = Invoke-PRPerfNative wpr.exe @('-cancel')
    Remove-Item -LiteralPath $workingDirectory -Recurse -Force -ErrorAction SilentlyContinue
}

$report = [ordered]@{}
foreach ($name in @($measurements.Keys | Sort-Object)) {
    $samples = @($measurements[$name])
    if ($samples.Count -eq 0) { continue }
    $report[$name] = [math]::Round((Get-PRPerfStatistics -Samples $samples).Median, 2)
}

if ($report.Count -eq 0) {
    # Writing an empty file would be indistinguishable from a real measurement of nothing.
    # Leaving it absent is what tells the comment to carry on without this section.
    Write-Note 'Nothing was measured, so no result file was written.'
    exit 0
}

$outputDirectory = Split-Path -Parent $OutputPath
if (-not [string]::IsNullOrWhiteSpace($outputDirectory)) {
    New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null
}
$report | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $OutputPath -Encoding UTF8
Write-Note "Wrote $OutputPath : $(($report.Keys | ForEach-Object { "$_=$($report[$_])" }) -join ', ')"
exit 0


