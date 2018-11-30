Write-Verbose "PSScriptRoot: $PSScriptRoot"
. $PSScriptRoot\ADALAuth.ps1

###############################################################################
# Retrieve-Headers
# Pops an authentication flow for VSTS
#
# This just wraps the base function from ADALAuth.ps1, and saves the 
# headers so that any future calls to this return immediately
###############################################################################

function Retrieve-Headers
{
    if ($global:vstsRestApiHeaders -eq $null)
    {
        $global:vstsRestApiHeaders = Get-AuthorizationHeaders
    }
}


###############################################################################
# GetAnswer
# Stolen from Jevan's CopyAndPrepVHD script
###############################################################################

function GetAnswer
{
    Param([string[]]$Choices)

    while ($true)
    {
        $choice = Read-Host -Prompt ("[" + ([string]::Join("|", $Choices)) + "]")
		foreach ($possibleChoice in $Choices)
		{
			if ($possibleChoice -like $choice)
			{
	            return $choice
		    }
		}
    }

    return $null
}

###############################################################################
# RunLocalization
# Run the localization script with a unique tag
###############################################################################

function RunLocalization
{
    Param([string] $branchName)
    $locWorkflowName = $branchName -replace '/','_'
    $locWorkflowName = "localization_nuget_release_" + $locWorkflowName
    $DateTime = Get-Date #or any other command to get DateTime object
    $locWorkflowName = $locWorkflowName + ([DateTimeOffset]$DateTime).ToUnixTimeSeconds();
    Write-Host $locWorkflowName
    
    pushd build/localization
    .\RunLocWorkflow.ps1 $locWorkflowName
    popd
}

###############################################################################
# New-BranchPullRequest
# Create a pull request from the specified branch into master
###############################################################################

function New-BranchPullRequest
{
    Param([string] $branchName)

    Retrieve-Headers

    $pullRequestParameters = @{
        "sourceRefName"="refs/heads/$branchname"
        "targetRefName"="refs/heads/master"
        "title"="Updating custom.props and localization in master (from nuget package creation)"
        "description"=""
        "reviewers"=@()
    }

    Invoke-RestMethod -Uri "https://microsoft.visualstudio.com/defaultcollection/os/_apis/git/repositories/dep.controls/pullRequests?api-version=3.0-preview&sourceRefName=refs/heads/$branchname" -Body (ConvertTo-Json $pullRequestParameters) -Method Post -ContentType "application/json" -Headers $global:vstsRestApiHeaders 2>&1> $null
}

function Get-BranchPullRequests
{
    Invoke-RestMethod -Uri "https://microsoft.visualstudio.com/defaultcollection/os/_apis/git/repositories/dep.controls/pullRequests?api-version=3.0-preview&sourceRefName=refs/heads/$branchname" -Method Get -Headers $global:vstsRestApiHeaders
}

function Lock-Branch
{
    Param([string] $branchName)

    Retrieve-Headers

    $parameters = @{
        "isLocked"="true"
    }

    Invoke-RestMethod -Uri "https://microsoft.visualstudio.com/defaultcollection/os/_apis/git/repositories/dep.controls/refs/heads/$($branchName)?api-version=1.0" -Body (ConvertTo-Json $parameters) -Method Patch -ContentType "application/json" -Headers $global:vstsRestApiHeaders 2>&1> $null
}

###############################################################################
# Queue-NewReleaseBuild
# Queue a build of the specified branch
###############################################################################

function Queue-NewReleaseBuild
{
    Param([string] $branchName)
    Retrieve-Headers

    $root = @{
        "sourceBranch" = "refs/heads/$branchName";
        "definition" = @{
            "id" = "11819" # depcontrols_release build defn
        };
        "project" = @{
            "id" = "8d47e068-03c8-4cdc-aa9b-fc6929290322" # OS
        };
        "repository" = @{
            "id" = "d39af991-db55-43c3-bdef-c6333a2b3264" # dep.controls
        };
    };

    $response = Invoke-RestMethod https://microsoft.visualstudio.com/DefaultCollection/os/_apis/build/builds?api-version=2.0 -Body (ConvertTo-Json $root) -Method Post -ContentType "application/json" -Headers $global:vstsRestApiHeaders

    $id = $response.id;
    $url = "https://microsoft.visualstudio.com/DefaultCollection/OS/_build/index?buildId=$id"
    Write-Host "New build at $url"
    Start-Process $url

}

#####################################################
#               START ACTUAL SCRIPT                 #
#####################################################

Write-Host "Make sure this branch is clear of changes!"

Write-Host "ReleaseHelper, select your action:"
Write-Host "1. Create and prepare release branch"
# Write-Host "2. Deploy nuget package to feed"

$scriptAction = GetAnswer("1", "2");

if ($scriptAction -eq "1")
{
    Write-Host "Do you want me to switch to master and pull?"
    if ((GetAnswer("Y", "N")) -eq "Y")
    {
        Write-Host "YEAH BOY";
        git checkout master
        git pull
    }

    $branchName = git name-rev --name-only HEAD

    Write-Host "Create a release branch [Y]? Or use existing branch $branchName [N]" 
    if ((GetAnswer("Y", "N")) -eq "Y")
    {
        Write-Host "Pick a name for the release branch, it should look something like release/1801/nuget-18013-180123"
        Write-Host "That's release/[YY][MM]/nuget-[YY][MM][release # for this month]-[YY][MM][todays date]"
    
        $branchName = Read-Host "Release branch name"
    
        git checkout -b $choice        
    }
    
    $gitChanges = $false;

    Write-Host ""
    Write-Host "Run localization?"
    Write-Host ""
    Write-Host "Remember, if you are adding new strings, you'll need to re-run localization (after the translations are done)"
    Write-Host "to actually get the newly translated strings pulled back down into our project. Since we are releasing a new"
    Write-Host "package every week-ish, this only matters if for some reason we add new/update strings in a release end-game,"
    Write-Host "but just something to keep in mind!"
    if ((GetAnswer("Y", "N")) -eq "Y")
    {
        RunLocalization($branchName);
        $gitChanges = $true;
    }

    Write-Host "Time to update custom.props, usually you set the VersionMinor to the [YY][MM][release # for this month] of your branch name"
    Write-Host "Update custom.props?"
    if ((GetAnswer("Y", "N")) -eq "Y")
    {
        notepad custom.props
        git add custom.props
        $gitChanges = $true;
    }

    if ($gitChanges -eq $true)
    {
        git commit -m "Updated custom.props"
        git push --set-upstream origin $branchName
    }

    Write-Host "Lock the branch? This will prevent further commits (including by future runs of this script)"
    if ((GetAnswer("Y", "N")) -eq "Y")
    {
        Lock-Branch($branchName)
    }

    Write-Host "Create pull request to merge changes back to master?"
    if ((GetAnswer("Y", "N")) -eq "Y")
    {
        Write-Host "You may be asked to install some nuget packages the first time you go through this flow"

        New-BranchPullRequest($branchName); # rename function

        $BranchPullRequests = Get-BranchPullRequests
        if ($BranchPullRequests.count -eq 0)
        {
            Write-Error "Could not create pull request!"
            exit 1
        }
        else
        {
            $pullRequest = $BranchPullRequests.value[0];
            $pullRequestID  = $pullRequest.pullRequestId;
            $url = "https://microsoft.visualstudio.com/OS/OS%20Team/_git/dep.controls/pullrequest/$pullRequestID#_a=overview"
            Write-Host "New pull request: $url"
            Start-Process $url
        }
    }

    Write-Host "Spin a new depcontrols_release build?"
    if ((GetAnswer("Y", "N")) -eq "Y")
    {
        Queue-NewReleaseBuild($branchName);
    }

    Write-Host "---"
    Write-Host ""
    Write-Host "Remember go to https://microsoft.visualstudio.com/DefaultCollection/OS/ft_xamlcon/_git/dep.controls/branches and LOCK your new branch"
}
Elseif ($scriptAction -eq "2")
{
    # Write-Host "NUYGETSFDASD"
}