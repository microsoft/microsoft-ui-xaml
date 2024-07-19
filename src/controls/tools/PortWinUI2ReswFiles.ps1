Param(
    [Parameter(Mandatory = $true)] 
    [string]$WinUI2RepoRoot
)

if ($env:RepoRoot)
{
    $repoRoot = $env:RepoRoot
}
else
{
    $repoRoot = [System.IO.Path]::GetFullPath("$PSScriptRoot\..\..")
}

# First, we'll delete all of the existing resw files.
Write-Host "Deleting existing resw files..."

foreach ($projectFile in (Get-ChildItem "$repoRoot\controls\dev" -Filter "*.vcxitems" -Recurse))
{
    Write-Host "    Deleting resw files from $($projectFile.FullName)..."

    $pathToProject = $projectFile.DirectoryName
    $reswFiles = Get-ChildItem $pathToProject -Filter "*.resw" -Recurse | Sort-Object -Property @{Expression = {$_.Name.Length}; Descending = $True}

    foreach ($reswFile in $reswFiles)
    {
        Remove-Item $reswFile.FullName | Out-Null
    }
}

# Next, we'll bring over the new resw files.
Write-Host "Copying over new resw files..."

foreach ($projectFile in (Get-ChildItem "$WinUI2RepoRoot\dev" -Filter "*.vcxitems" -Recurse))
{
    Write-Host "    Copying resw files from $($projectFile.FullName)..."

    $pathToProject = $projectFile.DirectoryName
    $reswFiles = Get-ChildItem $pathToProject -Filter "*.resw" -Recurse | Sort-Object -Property @{Expression = {$_.Name.Length}; Descending = $True}

    foreach ($reswFile in $reswFiles)
    {
        $targetPath = $reswFile.FullName.ToLower().Replace($WinUI2RepoRoot.ToLower(), "$($repoRoot.ToLower())\controls")
        $targetDirectory = [System.IO.Path]::GetDirectoryName($targetPath)

        if (-not [System.IO.Directory]::Exists($targetDirectory))
        {
            New-Item -Path $targetDirectory -ItemType Directory | Out-Null
        }

        Copy-Item $reswFile.FullName $targetPath | Out-Null
    }
}