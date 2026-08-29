$nugetVersion = ([xml](Get-Content -Raw $PSScriptRoot\MUXCustomBuildTasks.nuspec)).package.metadata.version
$projectRoot = Resolve-Path "$PSScriptRoot\..\..\..\.."

$packagesConfigReferences = @(
    "$projectRoot\packages.config",
    "$projectRoot\controls\dev\dll\packages.config"
)

$propsReferences = @(
    "$projectRoot\eng\versions.props"
)

foreach ($filePath in $packagesConfigReferences)
{
    Write-Host "Updating $filePath..."

    $fileContents = [System.IO.File]::ReadAllText($filePath)
    [System.IO.File]::WriteAllText($filePath, ($fileContents -replace "id=`"MUXCustomBuildTasks`" version=`".*?`"", "id=`"MUXCustomBuildTasks`" version=`"$nugetVersion`""))
}

foreach ($filePath in $propsReferences)
{
    Write-Host "Updating $filePath..."

    $fileContents = [System.IO.File]::ReadAllText($filePath)
    [System.IO.File]::WriteAllText($filePath, ($fileContents -replace "<MuxCustomBuildTasksPackageVersion>.*?</MuxCustomBuildTasksPackageVersion>", "<MuxCustomBuildTasksPackageVersion>$nugetVersion</MuxCustomBuildTasksPackageVersion>"))
}

Write-Host "$([Environment]::NewLine)All MUXCustomBuildTasks references updated to $nugetVersion."
