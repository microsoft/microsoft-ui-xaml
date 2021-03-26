Write-Host "TestPass-PreRun.ps1"

$Platform = $env:testbuildplatform
if(!$Platform)
{
    $Platform = "x86"
}

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
        $packageFullName = "$($pkgName)_1.0.0.0_$($Platform)__8wekyb3d8bbwe" 
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

Write-Host "Uninstall any of our test apps that may have been left over from previous test runs"
UninstallTestApps("NugetPackageTestApp", "NugetPackageTestAppCX", "IXMPTestApp", "MUXControlsTestApp")

Write-Host "Uninstall MUX Framework package that may have been left over from previous test runs"
# We don't want to uninstall all versions of the MUX Framework package, as there may be other apps preinstalled on the system 
# that depend on it. We only uninstall the Framework package that corresponds to the version of MUX that we are testing.
[xml]$versionData = (Get-Content "version.props")
$versionMajor = $versionData.GetElementsByTagName("MUXVersionMajor").'#text'
$versionMinor = $versionData.GetElementsByTagName("MUXVersionMinor").'#text'
UninstallTestApps("Microsoft.UI.Xaml.$versionMajor.$versionMinor")


.\InstallTestAppDependencies.ps1



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

Enable-CrashDumpsForProcesses @("MUXControlsTestApp.exe,IXMPTestApp.exe","NugetPackageTestApp.exe","NugetPackageTestAppCX.exe","AppThatUsesMUXIndirectly.exe")