.\CopyVisualTreeVerificationFiles.ps1

$uploadRoot = $env:HELIX_WORKITEM_UPLOAD_ROOT
$testnameprefix = $env:testnameprefix 

$pgcFiles = Get-ChildItem $uploadRoot | where { $_.Name.EndsWith(".pgc") }
foreach($pgcFile in $pgcFiles)
{
    $newName = "$testnameprefix.$($pgcFile.Name)"
    $newPath = Join-Path $pgcFile.Directory $newName
    Write-Host "Move-Item $($pgcFile.FullName) $newPath"
    Move-Item $pgcFile.FullName $newPath
}