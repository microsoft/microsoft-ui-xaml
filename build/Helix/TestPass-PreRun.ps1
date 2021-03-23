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