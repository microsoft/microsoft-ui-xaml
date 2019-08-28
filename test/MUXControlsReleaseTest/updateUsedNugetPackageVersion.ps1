[CmdLetBinding()]
Param(
    [Parameter(mandatory=$true)]
    [string]$newVersion,

    [string]$currentPackageVersion = "2.1" + "190606001"
)

$scriptDirectory = $script:MyInvocation.MyCommand.Path | Split-Path -Parent
pushd $scriptDirectory

$numFilesReplaced = 0
Get-ChildItem $scriptDirectory -r -File |
  ForEach-Object {
    $path = $_.FullName
    $contents = [System.IO.File]::ReadAllText($path)
    $newContents = $contents.Replace("$currentPackageVersion", "$newVersion")
    if ($contents -ne $newContents) {
      Write-Host "Updating version in $path"
      $newContents | Set-Content $path -Encoding UTF8
      $numFilesReplaced += 1
    }
  }

if ($numFilesReplaced -eq 0)
{
  Write-Host "##vso[task.logissue type=error]No files found with '$env:currentPackageVersion' in them, make sure to update *.yml files when retargeting ReleaseTest projects"
  Exit 1
}

