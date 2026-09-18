[CmdletBinding()]
param(
    [Parameter(Mandatory)][string] $CollectionUri,
    [Parameter(Mandatory)][string] $Project,
    [Parameter(Mandatory)][string] $DefinitionId,
    [Parameter(Mandatory)][string] $SourceBranch,
    [Parameter(Mandatory)][string] $CurrentBuildId,
    [Parameter(Mandatory)][string] $ArtifactName,
    [string] $AccessToken = '',
    [string] $MainBranch = 'refs/heads/main',
    [string] $PullRequestNumber = '',
    [string] $GitHubRepository = 'microsoft/microsoft-ui-xaml',
    [string] $GitHubToken = ''
)

# Best effort by design. Without a baseline the comparison still runs and still reports
# real numbers, it just cannot draw a conclusion from them, so a lookup failure here must
# never take the stage down with it.
$ErrorActionPreference = 'Continue'

Import-Module (Join-Path $PSScriptRoot 'PRPerfResults.psm1') -Force
Import-Module (Join-Path $PSScriptRoot 'PRPerfGitHub.psm1') -Force

function Set-BaselineOutput {
    param([string] $BuildId = '', [string] $Commit = '', [string] $Kind = '')
    Write-Host "##vso[task.setvariable variable=perfBaselineBuildId]$BuildId"
    Write-Host "##vso[task.setvariable variable=perfBaselineCommit]$Commit"
    Write-Host "##vso[task.setvariable variable=perfBaselineKind]$Kind"
}

if ([string]::IsNullOrWhiteSpace($AccessToken)) {
    Write-Host '##vso[task.logissue type=warning]No access token was available, so no baseline build could be looked up.'
    Set-BaselineOutput
    return
}

try {
    $headers = @{ Authorization = "Bearer $AccessToken" }
    $base = $CollectionUri.TrimEnd('/') + '/' + [uri]::EscapeDataString($Project)

    function Get-CompletedBuilds {
        param([string] $Branch)
        $query = "$base/_apis/build/builds?definitions=$DefinitionId" +
            "&branchName=$([uri]::EscapeDataString($Branch))" +
            '&statusFilter=completed&queryOrder=finishTimeDescending&$top=20&api-version=7.0'
        return (Invoke-RestMethod -Uri $query -Headers $headers -Method Get).value
    }

    function Test-ArtifactStillExists {
        # Only a build that still has the artifact is usable. Retention quietly removes
        # them, and discovering that at download time would waste the whole run.
        param([string] $BuildId)
        try {
            $artifacts = (Invoke-RestMethod -Method Get -Headers $headers `
                -Uri "$base/_apis/build/builds/$BuildId/artifacts?api-version=7.0").value
        } catch {
            return $false
        }
        return @($artifacts | Where-Object { $_.name -eq $ArtifactName }).Count -gt 0
    }

    # Preferred baseline: a build of main that this pull request already contains. That is a
    # genuine "before this change" side, so a difference is this author's to explain. An
    # earlier build of the pull request itself only ever compares it with its own past.
    $baseCommit = ''
    if ($PullRequestNumber -match '^\d+$') {
        try {
            $baseCommit = [string](Get-GitHubPRPerfPullRequest `
                -Uri "https://api.github.com/repos/$GitHubRepository/pulls/$PullRequestNumber" `
                -Token $GitHubToken).base.sha
        } catch {
            Write-Host "##vso[task.logissue type=warning]Could not read pull request $PullRequestNumber to find the commit it branched from: $($_.Exception.Message)"
        }
    }

    if ($baseCommit -match '^[0-9a-fA-F]{40}$') {
        $gitHubHeaders = New-GitHubPRPerfHeaders -Token $GitHubToken
        $mainBuild = Select-PRPerfMainBaselineBuild -Builds (Get-CompletedBuilds -Branch $MainBranch) `
            -IsAncestor {
                param($sha)
                Test-GitHubPRPerfCommitIsAncestor -Candidate $sha -Descendant $baseCommit `
                    -Headers $gitHubHeaders -Repository $GitHubRepository
            } `
            -HasArtifact { param($id) Test-ArtifactStillExists -BuildId $id }

        if ($null -ne $mainBuild) {
            Write-Host "Baseline build $($mainBuild.id) on '$MainBranch' at commit $($mainBuild.sourceVersion) still has '$ArtifactName'."
            Set-BaselineOutput -BuildId ([string]$mainBuild.id) -Commit ([string]$mainBuild.sourceVersion) -Kind 'main'
            return
        }
        Write-Host "##vso[task.logissue type=warning]No build of '$MainBranch' that this pull request already contains still has a '$ArtifactName' artifact. Falling back to an earlier build of this pull request, which only compares it with its own earlier self."
    } else {
        Write-Host "##vso[task.logissue type=warning]The commit this pull request branched from could not be determined, so a main baseline cannot be shown to be honest. Falling back to an earlier build of this pull request."
    }

    foreach ($build in @(Get-CompletedBuilds -Branch $SourceBranch)) {
        if ([string]$build.id -eq [string]$CurrentBuildId) { continue }
        if ([string]::IsNullOrWhiteSpace([string]$build.sourceVersion)) { continue }
        if (-not (Test-ArtifactStillExists -BuildId ([string]$build.id))) { continue }

        Write-Host "Baseline build $($build.id) at commit $($build.sourceVersion) still has '$ArtifactName'."
        Set-BaselineOutput -BuildId ([string]$build.id) -Commit ([string]$build.sourceVersion) -Kind 'same-branch'
        return
    }

    Write-Host "##vso[task.logissue type=warning]No earlier build on '$SourceBranch' still has a '$ArtifactName' artifact, so there is no baseline to compare against."
    Set-BaselineOutput
} catch {
    Write-Host "##vso[task.logissue type=warning]Baseline lookup failed: $($_.Exception.Message)"
    Set-BaselineOutput
}
