function AllChangedFilesAreSkippable
{
    Param($files)

    #$skipExts = @(".md")
    $skipExts = @(".md", ".ps1", ".yml")
    $allFilesAreSkippable = $true

    foreach($file in $files)
    {
        Write-Host "Checking '$file'"
        $ext = (Get-Item $file).Extension
        $fileIsSkippable = $ext -in $skipExts
        Write-Host "File '$file' is skippable: '$fileIsSkippable'"

        if(!$fileIsSkippable)
        {
            $allFilesAreSkippable = $false
        }
    }

    return $allFilesAreSkippable
}

$shouldSkipBuild = $false

#if($env:BUILD_REASON -eq "PullRequest")
#{
    #$targetBranch = "origin/$env:SYSTEM_PULLREQUEST_TARGETBRANCH"

    $targetBranch = "origin/master"

    $gitCommandLine = "git diff $targetBranch --name-only"
    Write-Host "$gitCommandLine"

    $diffOutput = Invoke-Expression $gitCommandLine
    Write-Host $diffOutput

    $files = $diffOutput.Split([Environment]::NewLine)

    Write-Host "Files changed: $files"
    

    $shouldSkipBuild = AllChangedFilesAreSkippable($files)
#}


Write-Host $shouldSkipBuild

Write-Host "##vso[task.setvariable variable=shouldSkipPRBuild;isOutput=true]$shouldSkipBuild"