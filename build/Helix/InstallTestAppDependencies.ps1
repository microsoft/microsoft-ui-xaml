Push-Location ..\

$dependencyFiles = Get-ChildItem -Filter "*dependencies.txt"

foreach($file in $dependencyFiles)
{
    foreach($line in Get-Content $file)
    {
        Add-AppxPackage $line
    }
}

Pop-Location