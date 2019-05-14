Push-Location ..\

$dependencyFiles = Get-ChildItem (Get-Item (Get-Location).Path).Parent -Filter "*dependencies.txt"

foreach ($file in $dependencyFiles)
{
    foreach ($line in Get-Content $file)
    {
        Add-AppxPackage $line
    }
}

Pop-Location