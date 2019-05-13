Push-Location ..\

$payloadFolder = Resolve-Path "$PSScriptRoot\.."
$dependencyFiles = Get-ChildItem -Filter "$payloadFolder\*dependencies.txt"

foreach($file in $dependencyFiles)
{
    foreach($line in Get-Content $file)
    {
        Add-AppxPackage $line
    }
}

Pop-Location