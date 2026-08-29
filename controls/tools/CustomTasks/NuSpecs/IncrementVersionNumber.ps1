$nugetVersionString = ([xml](Get-Content -Raw $PSScriptRoot\MUXCustomBuildTasks.nuspec)).package.metadata.version.Replace("-winui3", "")
$nugetVersion = [version]$nugetVersionString
$newNugetVersion = "$($nugetVersion.Major).$($nugetVersion.Minor).$($nugetVersion.Build + 1)"

$versionReferences = @(
    (Resolve-Path "$PSScriptRoot\..\Properties\AssemblyInfo.cs"),
    (Resolve-Path "$PSScriptRoot\MUXCustomBuildTasks.nuspec")
)

foreach ($filePath in $versionReferences)
{
    Write-Host "Incrementing version number in $filePath from $nugetVersionString to $newNugetVersion..."

    $fileContents = [System.IO.File]::ReadAllText($filePath)
    [System.IO.File]::WriteAllText($filePath, ($fileContents -replace $nugetVersionString, $newNugetVersion))
}
