[CmdletBinding()]
param(
    [String]$Platform = $env:testbuildplatform
)

$ErrorActionPreference = 'Stop'

if(!$Platform)
{
    $Platform = "x86"
    if(Test-Path ./buildconfig.json)
    {
        $buildConfig = Get-Content ./buildconfig.json | Out-String | ConvertFrom-Json
        $Platform = $buildConfig.Platform
    }
}

Write-Host "TestPass-OneTimeMachineSetup.ps1"

reg add HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\WinUI\XAML /v UsePrivateHeap /t REG_DWORD /d 1 /f /reg:32
reg add HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\WinUI\XAML /v UsePrivateHeap /t REG_DWORD /d 1 /f /reg:64
reg add HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\WinUI\XAML /v ErrorLogDirectory /t REG_SZ /d C:\ProductErrorLogs /f /reg:32
reg add HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\WinUI\XAML /v ErrorLogDirectory /t REG_SZ /d C:\ProductErrorLogs /f /reg:64
reg add "HKEY_CURRENT_USER\Keyboard Layout\Toggle" /v "Layout Hotkey" /t REG_SZ /d 3 /f /reg:32
reg add "HKEY_CURRENT_USER\Keyboard Layout\Toggle" /v "Layout Hotkey" /t REG_SZ /d 3 /f /reg:64

# UWP window creation has been disabled. To enable it using runtime feature, we use this regkey
reg add HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\WinUI\XAML /v EnableUWPWindow /t REG_DWORD /d 1 /f /reg:32
reg add HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\WinUI\XAML /v EnableUWPWindow /t REG_DWORD /d 1 /f /reg:64

# We'll extract the signing certificates from each test app we need to install and add them as trusted certificates.
foreach ($testAppInstaller in (Get-ChildItem $PSScriptRoot\* -Recurse -Include "*.appx", "*.appxbundle", "*.msix", "*.msixbundle"))
{
    [System.IO.FileSystemInfo]$testAppInstaller = $testAppInstaller

    Write-Host "Adding test cert for $($testAppInstaller.FullName)..."

    $certificateBytes = (Get-AuthenticodeSignature $testAppInstaller.FullName).SignerCertificate.Export([System.Security.Cryptography.X509Certificates.X509ContentType]::Cert)
    $certificatePath = [System.IO.Path]::Combine($env:TEMP, [System.IO.Path]::ChangeExtension($testAppInstaller.Name, ".cer"))
    [System.IO.File]::WriteAllBytes($certificatePath, $certificateBytes)
    certutil -addstore TrustedPeople $certificatePath

    Remove-Item $certificatePath
}

$scriptDirectory = $script:MyInvocation.MyCommand.Path | Split-Path -Parent


Write-Host "Install dotnet runtime"
.\dotnet-windowsdesktop-runtime-installer.exe /quiet /install /norestart /log dotnetinstalllog.txt |Out-Null
$dotNetInstallResult = $?

Get-Content .\dotnetinstalllog.txt
if($env:HELIX_WORKITEM_UPLOAD_ROOT)
{
    Copy-Item .\dotnetinstalllog.txt $env:HELIX_WORKITEM_UPLOAD_ROOT -Force
}

if (!$dotNetInstallResult)
{
    Write-Host "##vso[task.logissue type=error;]Failed to install dotnet runtime.  Copied log to artifacts dir as dotnetinstalllog.txt."
    Write-Error "Failed to install dotnet runtime.  Copied log to artifacts dir as dotnetinstalllog.txt."
    exit 1
}


$redistPlatform = $Platform
if($Platform -eq "arm64ec")
{
    $redistPlatform = "x64"
}

Write-Host "Installing VC Redist"
& ./vc_redist.$redistPlatform.exe /install /quiet /norestart

function UninstallTestApps {
    Param([string[]]$appsToUninstall)

    foreach($pkgName in $appsToUninstall)
    {
        foreach($pkg in (Get-AppxPackage $pkgName).PackageFullName) 
        {
            Write-Output "Removing: $pkg" 
            Remove-AppxPackage $pkg
        }

        # Sometimes an app can get into a state where it is no longer returned by Get-AppxPackage, but it is still present
        # which prevents other versions of the app from being installed.
        # To handle this, we can directly call Remove-AppxPackage against the full name of the package. However, without
        # Get-AppxPackage to find the PackageFullName, we just have to manually construct the name.
        foreach ($publisherHashes in @("8wekyb3d8bbwe", "6f07fta6qpts2"))
        {
            $packageFullName = "$($pkgName)_1.0.0.0_$($redistPlatform)__$publisherHashes" 
            Write-Host "Removing $packageFullName if installed"
            Remove-AppxPackage $packageFullName -ErrorVariable appxerror -ErrorAction SilentlyContinue 
            if($appxerror)
            {
                foreach($error in $appxerror)
                {
                    # In most cases, Remove-AppPackage will fail due to the package not being found. Don't treat this as an error.
                    if(!($error.Exception.Message -match "0x80073CF1"))
                    {
                        Write-Error $error
                    }
                }
            }
            else
            {
                Write-Host "Sucessfully removed $packageFullName"
            }
        }
    }
}

Write-Host "Uninstall any of our test apps that may have been left over from previous test runs"
UninstallTestApps("NugetPackageTestApp", 
    "NugetPackageTestAppCX", 
    "IXMPTestApp", 
    "MUXControlsTestApp", 
    "MUXControlsTestApp.Desktop",
    "WinUICsDesktopSampleApp", 
    "Microsoft.WinUIGallery.UWP.WinUI3Preview", 
    "Microsoft.WinUIGallery.Desktop.WinUI3Prev", 
    "Microsoft.WinUI3ControlsGallery.UWP",
    "Microsoft.WinUI3ControlsGallery.Debug",
    "Microsoft.WinUI3ControlsGallery",
    "WinUICppDesktopSampleApp")

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

# enable dump collection for our test apps:
$namesOfProcessesForDumpCollection = @(
    "MUXControlsTestApp.exe",
    "MUXControlsTestApp.TAEF.Desktop.exe",
    "IXMPTestApp.exe",
    "NugetPackageTestApp.exe",
    "NugetPackageTestAppCX.exe",
    "te.exe",
    "te.processhost.exe",
    "AppThatUsesMUXIndirectly.exe",
    "taefhostapp.exe",
    "TaefHostAppManaged.exe",
    "AppUIBasics.exe",
    "WinUIGallery.Desktop.exe",    
    "WinUICsDesktopSampleApp.exe",
    "WinUICppDesktopSampleApp.exe")

Enable-CrashDumpsForProcesses $namesOfProcessesForDumpCollection
