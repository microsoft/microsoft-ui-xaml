Param(
    [string]$SourceFolder,
    [string]$OutputFolder
)

$pgcFiles = Get-ChildItem -Recurse $SourceFolder | where { $_.Name.EndsWith(".pgc") }

foreach($pgcFile in $pgcFiles)
{
    $flavorPath = $pgcFile.Name.Split('.')[0]
    $archPath = $pgcFile.Name.Split('.')[1]
    $fileName = $pgcFile.Name.Remove(0, $flavorPath.length + $archPath.length + 2)
    $fullPath = "$OutputFolder\PGO\$archPath"
    $destination = "$fullPath\$fileName"

    Write-Host "Copying $($pgcFile.Name) to $destination"

    if (-Not (Test-Path $fullPath))
    {
        New-Item $fullPath -ItemType Directory
    }
    Write-Host "Copy $($pgcFile.FullName) to $destination"
    Copy-Item $pgcFile.FullName $destination
}