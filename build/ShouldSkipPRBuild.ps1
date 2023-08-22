# Determine if the current PR payload requires a build.
# We skip the build if the only files changed are .md files.

function AllChangedFilesAreSkippable
{
    Param($files)

    $skipExts = @(".md", ".png", ".PNG", ".jpg", ".ics")
    $skipFiles = @(".github/ISSUE_TEMPLATE/bug_report.yaml")
    $allFilesAreSkippable = $true

    foreach($file in $files)
    {
        Write-Host "Checking '$file'"
        try
        {
            $ext = [System.IO.Path]::GetExtension($file)
            $fileIsSkippable = $ext -in $skipExts

            if(!$fileIsSkippable)
            {
                $fileIsSkippable = $file -in $skipFiles
            }
        }
        catch
        {
            $fileIsSkippable = $false
        }

        Write-Host "File '$file' is skippable: '$fileIsSkippable'"

        if(!$fileIsSkippable)
        {
            $allFilesAreSkippable = $false
        }
    }

    return $allFilesAreSkippable
}

$shouldSkipBuild = $false

if($env:BUILD_REASON -eq "PullRequest")
{
    # Azure DevOps sets this variable with refs/heads/ at the beginning.
    # This trims it so the $gitCommandLine is formatted properly
    if ($env:SYSTEM_PULLREQUEST_TARGETBRANCH.StartsWith("refs/heads/"))
    {
        $systemPullRequestTargetBranch = $env:SYSTEM_PULLREQUEST_TARGETBRANCH.Substring("11")
        
    }
    else 
    {
        $systemPullRequestTargetBranch = $env:SYSTEM_PULLREQUEST_TARGETBRANCH
    }

    $targetBranch = "origin/$systemPullRequestTargetBranch"

    $gitCommandLine = "git diff $targetBranch --name-only"
    Write-Host "$gitCommandLine"

    $diffOutput = Invoke-Expression $gitCommandLine
    Write-Host $diffOutput

    $files = $diffOutput.Split([Environment]::NewLine)

    Write-Host "Files changed: $files"
    

    $shouldSkipBuild = AllChangedFilesAreSkippable($files)
}

Write-Host "shouldSkipBuild = $shouldSkipBuild"

Write-Host "##vso[task.setvariable variable=shouldSkipPRBuild;isOutput=true]$shouldSkipBuild"