foreach($line in Get-Content ..\MUXControlsTestApp.dependencies.txt) {
    Add-AppxPackage ..\$line
}

foreach($line in Get-Content ..\IXMPTestApp.dependencies.txt) {
    Add-AppxPackage ..\$line
}