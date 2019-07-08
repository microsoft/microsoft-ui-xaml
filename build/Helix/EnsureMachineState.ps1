# List all processes to aid debugging:
Write-Host "All processes running:"
Get-Process

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

# Uninstall AppX packages that are known to cause issues with our tests:
$packagesToUninstall = @("*Skype*", "*Windows.Photos*")
foreach($pkgName in $packagesToUninstall)
{
    foreach($pkg in (Get-AppxPackage $pkgName).PackageFullName) 
    {
        Write-Output "Removing: $pkg" 
        Remove-AppxPackage $pkg
    } 
}

