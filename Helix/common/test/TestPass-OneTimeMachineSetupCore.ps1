[CmdletBinding()]
param(
    [String]$Platform = $env:testbuildplatform
)

Write-Host "TestPass-OneTimeMachineSetupCore.ps1"


if(!$Platform)
{
    $Platform = "x86"
    if(Test-Path ./buildconfig.json)
    {
        $buildConfig = Get-Content ./buildconfig.json | Out-String | ConvertFrom-Json
        $Platform = $buildConfig.Platform
    }
}

Write-Host "Platform = $Platform"

# Displaying progress is unnecessary and is just distracting.
$ProgressPreference = "SilentlyContinue"

$scriptDirectory = $script:MyInvocation.MyCommand.Path | Split-Path -Parent


reg add HKLM\Software\Policies\Microsoft\Windows\Appx /v AllowDevelopmentWithoutDevLicense /t REG_DWORD /d 1 /f

Write-Host "Setting regkey to disable network flyout."
reg add HKLM\System\CurrentControlSet\Control\Network\NewNetworkWindowOff /f
Write-Host "Restarting explorer to clear any existing network flyout."
Stop-Process -ProcessName explorer
$explorerProcess = Get-Process -Name explorer -ErrorAction SilentlyContinue
if ($explorerProcess)
{
    Write-Host "explorer.exe automatically restarted."
}
else
{
    Write-Host "Manual restarting explorer.exe"
    Start-Process explorer.exe
}

# Add this test directory as an exclusion for Windows Defender
Add-MpPreference -ExclusionPath $scriptDirectory
Get-MpPreference
Get-MpComputerStatus

# Kill processes by name that are known to interfere with our tests:
$processNamesToStop = @("Microsoft.Photos", "WinStore.App", "SkypeApp", "SkypeBackgroundHost", "dhandler", "YourPhone", "SurfaceAppDt", "TextInputHost", "WebViewHost")
foreach($procName in $processNamesToStop)
{
    Write-Host "Attempting to kill $procName if it is running"
    Stop-Process -ProcessName $procName  -ErrorAction Ignore
}
Write-Host "All processes running after attempting to kill unwanted processes:"
Get-Process | Format-Table -AutoSize

$redistPlatform = $Platform
if($Platform -eq "arm64ec")
{
    $redistPlatform = "x64"
}

$packagesToInstall = @(
    "Microsoft.VCLibs.$redistPlatform.Debug.14.00.appx",
    "Microsoft.VCLibs.$redistPlatform.14.00.appx",
    "Microsoft.VCLibs.$redistPlatform.Debug.14.00.Desktop.appx",
    "Microsoft.VCLibs.$redistPlatform.14.00.Desktop.appx",
    "Microsoft.NET.CoreRuntime.1.1.appx",
    "Microsoft.NET.CoreRuntime.2.2.appx",
    "Microsoft.NET.CoreFramework.Debug.2.2.appx",
    "Microsoft.NET.Native.Framework.2.2.appx",
    "Microsoft.NET.Native.Framework.Debug.2.2.appx",
    "Microsoft.NET.Native.Runtime.2.2.appx"
)

foreach($package in $packagesToInstall)
{
    if(Test-Path Test\$package)
    {
        Write-Host "Installing $package"
        Add-AppxPackage Test\$package -ErrorVariable appxerror -ErrorAction SilentlyContinue
        if($appxerror)
        {
            foreach($error in $appxerror)
            {
                # In the case where the package does not install becasuse a higher version is already installed
                # we don't want to print an error message, since that is just noise. Filter out such errors.
                if(($error.Exception.Message -match "0x80073D06") -or ($error.Exception.Message -match "0x80073CFB"))
                {
                    Write-Host "The same or higher version of this package is already installed."
                }
                else
                {
                    Write-Error $error
                }
            }
        }
    }
}

function UninstallApps {
    Param([string[]]$appsToUninstall)

    foreach($pkgName in $appsToUninstall)
    {
        foreach($pkg in (Get-AppxPackage $pkgName).PackageFullName)
        {
            Write-Output "Removing: $pkg"
            Remove-AppxPackage $pkg
        }
    }
}

$runningInHelix = $false
if($env:HELIX_CORRELATION_PAYLOAD)
{
    $runningInHelix = $true
}

# If we are not running in Helix we are probably running on a dev's machine, so we should skip uninstalling these apps.
if($runningInHelix)
{
    Write-Host "Uninstall AppX packages that are known to cause issues with our tests"
    UninstallApps("*Skype*", "*Windows.Photos*")
}


# If we set the registry from a 32-bit process on a 64-bit machine, we will set the "virtualized" syswow registry.
# For crash dump collection we always want to set the "native" registry, so we make sure to invoke the native cmd.exe
$nativeCmdPath = "$env:SystemRoot\system32\cmd.exe"
if([Environment]::Is64BitOperatingSystem -and ![Environment]::Is64BitProcess)
{
    # The "sysnative" path is a 'magic' path that allows a 32-bit process to invoke the native 64-bit cmd.exe.
    $nativeCmdPath = "$env:SystemRoot\sysnative\cmd.exe"
}

$dumpFolder = $env:HELIX_DUMP_FOLDER
if(!$dumpFolder)
{
    $dumpFolder = "C:\dumps"
}

function Enable-CrashDumpsForProcesses {
    Param([string[]]$namesOfProcessesForDumpCollection)

    foreach($procName in $namesOfProcessesForDumpCollection )
    {
        Write-Host "Enabling local crash dumps for $procName"
        & $nativeCmdPath /c reg add "HKLM\Software\Microsoft\Windows\Windows Error Reporting\LocalDumps\$procName" /v DumpFolder /t REG_EXPAND_SZ /d $dumpFolder /f
        & $nativeCmdPath /c reg add "HKLM\Software\Microsoft\Windows\Windows Error Reporting\LocalDumps\$procName" /v DumpType /t REG_DWORD /d 2 /f
        & $nativeCmdPath /c reg add "HKLM\Software\Microsoft\Windows\Windows Error Reporting\LocalDumps\$procName" /v DumpCount /t REG_DWORD /d 3 /f
    }
}


Enable-CrashDumpsForProcesses @("te.exe", "te.processhost.exe")


if(Test-Path TestPass-OneTimeMachineSetup.ps1)
{
    & ./TestPass-OneTimeMachineSetup.ps1
}