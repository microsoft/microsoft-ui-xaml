function Install-Dependency
{
    Param($dependencyFilePath)

    if(Test-Path $dependencyFilePath)
    {
        foreach($line in Get-Content $dependencyFilePath) 
        {
            Add-AppxPackage ..\$line
        }
    }
}

Install-Dependency ..\MUXControlsTestApp.dependencies.txt
Install-Dependency ..\IXMPTestApp.dependencies.txt
Install-Dependency ..\NugetPackageTestApp.dependencies.txt
Install-Dependency ..\NugetPackageTestAppCX.dependencies.txt