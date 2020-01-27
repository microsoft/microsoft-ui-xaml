# List all processes to aid debugging:
Write-Host "All processes running:"
Get-Process


# Minimize all windows:
$shell = New-Object -ComObject "Shell.Application"
$shell.minimizeall()

# Kill any instances of Windows Security Alert:
$windowTitleToMatch = "*Windows Security Alert*"
$procs = Get-Process | Where {$_.MainWindowTitle -like "*Windows Security Alert*"}
foreach ($proc in $procs)
{
    Write-Host "Found process with '$windowTitleToMatch' title: $proc"
    $proc.Kill();
}

# Kill processes by name that are known to interfere with our tests:
$processNamesToStop = @("Microsoft.Photos", "WinStore.App", "SkypeApp", "SkypeBackgroundHost")
foreach($procName in $processNamesToStop)
{
    Write-Host "Attempting to kill $procName if it is running"
    Stop-Process -ProcessName $procName -Verbose     
}
Write-Host "All processes running after attempting to kill unwanted processes:"
Get-Process

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

Write-Host "List all installed apps:"
Get-AppxPackage

Write-Host "Uninstall AppX packages that are known to cause issues with our tests"
UninstallApps("*Skype*", "*Windows.Photos*")

Write-Host "Uninstall any of our test apps that may have been left over from previous test runs"
UninstallApps("NugetPackageTestApp", "NugetPackageTestAppCX", "IXMPTestApp", "MUXControlsTestApp")

Write-Host "Uninstall MUX Framework package that may have been left over from previous test runs"
# We don't want to uninstall all versions of the MUX Framework package, as there may be other apps preinstalled on the system 
# that depend on it. We only uninstall the Framework package that corresponds to the version of MUX that we are testing.
[xml]$versionData = (Get-Content "version.props")
$versionMajor = $versionData.GetElementsByTagName("MUXVersionMajor").'#text'
$versionMinor = $versionData.GetElementsByTagName("MUXVersionMinor").'#text'
UninstallApps("Microsoft.UI.Xaml.$versionMajor.$versionMinor")

Write-Host "List all installed apps:"
Get-AppxPackage