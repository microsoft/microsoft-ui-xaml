foreach($line in Get-Content ..\NugetPackageTestApp.dependencies.txt) {
    Add-AppxPackage ..\$line
}

foreach($line in Get-Content ..\NugetPackageTestAppCX.dependencies.txt) {
    Add-AppxPackage ..\$line
}