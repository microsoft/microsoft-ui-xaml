[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [ValidateSet('Discover', 'Single')]
    [string] $Mode,

    [int] $PullRequestId,

    [string] $DryRunOutputPath,

    [string] $FixtureDirectory,

    [switch] $Force
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
Import-Module (Join-Path $root 'PRPerfRequest.psm1') -Force

function Get-RequiredSetting {
    param([Parameter(Mandatory)][string] $Name)

    $value = [Environment]::GetEnvironmentVariable($Name)
    if ([string]::IsNullOrWhiteSpace($value)) {
        throw "Required environment variable '$Name' is not set."
    }
    return $value
}

function Read-Fixture {
    param([Parameter(Mandatory)][string] $Name)

    $path = Join-Path $FixtureDirectory $Name
    return Get-Content -LiteralPath $path -Raw | ConvertFrom-Json
}

function Invoke-PRPerfApi {
    param(
        [Parameter(Mandatory)][ValidateSet('Get', 'Post')][string] $Method,
        [Parameter(Mandatory)][string] $Uri,
        [string] $Body
    )

    $parameters = @{
        Method = $Method
        Uri = $Uri
        Headers = $script:headers
    }
    if ($Method -eq 'Post') {
        $parameters.Body = $Body
        $parameters.ContentType = 'application/json'
    }
    return Invoke-RestMethod @parameters
}

if ($Mode -eq 'Single' -and $PullRequestId -le 0) {
    throw 'PullRequestId must be supplied for Single mode.'
}

$repositoryId = Get-RequiredSetting 'PRPerfRepositoryId'
$sourcePipelineId = Get-RequiredSetting 'PRPerfSourcePipelineId'
$targetPipelineId = Get-RequiredSetting 'PRPerfTargetPipelineId'
$perfPipelineId = Get-RequiredSetting 'PRPerfPipelineId'
$artifactName = [Environment]::GetEnvironmentVariable('PRPerfArtifactName')
if ([string]::IsNullOrWhiteSpace($artifactName)) {
    $artifactName = 'drop_amd64fre'
}

$useFixtureApi = -not [string]::IsNullOrWhiteSpace($FixtureDirectory)
if ($useFixtureApi) {
    $activePrFixture = Read-Fixture 'active-prs.json'
    [object[]] $allPullRequests = @($activePrFixture.value)
    if ($Mode -eq 'Single') {
        [object[]] $pullRequests = @($allPullRequests | Where-Object pullRequestId -eq $PullRequestId)
        if ($pullRequests.Count -ne 1) {
            throw "Fixture pull request '$PullRequestId' was not found exactly once."
        }
    } else {
        $labelFixture = Read-Fixture 'pr-labels.json'
        $pullRequests = @(
            foreach ($pr in $allPullRequests) {
                $entry = @($labelFixture.value | Where-Object pullRequestId -eq $pr.pullRequestId)
                if ($entry.Count -ne 1) {
                    throw "Fixture labels for pull request '$($pr.pullRequestId)' were not found exactly once."
                }
                if (Test-PRPerfLabel -Labels @($entry[0].labels)) {
                    $pr
                }
            }
        )
    }
    $buildFixture = Read-Fixture 'builds.json'
    [object[]] $allBuilds = @($buildFixture.value)
    $perfRunPath = Join-Path $FixtureDirectory 'perf-runs.json'
    if (Test-Path -LiteralPath $perfRunPath -PathType Leaf) {
        $perfRunFixture = Read-Fixture 'perf-runs.json'
        [object[]] $existingPerfRuns = @($perfRunFixture.value)
    } else {
        [object[]] $existingPerfRuns = @()
    }
} else {
    $accessToken = Get-RequiredSetting 'SYSTEM_ACCESSTOKEN'
    $collectionUri = (Get-RequiredSetting 'SYSTEM_COLLECTIONURI').TrimEnd('/')
    $project = [uri]::EscapeDataString((Get-RequiredSetting 'SYSTEM_TEAMPROJECT'))
    $escapedRepository = [uri]::EscapeDataString($repositoryId)
    $script:headers = New-PRPerfAuthorizationHeaders -AccessToken $accessToken
    $repositoryApi = "$collectionUri/$project/_apis/git/repositories/$escapedRepository"

    if ($Mode -eq 'Single') {
        $prUrl = "$repositoryApi/pullRequests/$PullRequestId" + '?api-version=7.1'
        $pullRequests = @(Invoke-PRPerfApi -Method Get -Uri $prUrl)
    } else {
        $prsUrl = "$repositoryApi/pullRequests?searchCriteria.status=active&api-version=7.1"
        $prResponse = Invoke-PRPerfApi -Method Get -Uri $prsUrl
        $pullRequests = @(
            foreach ($pr in @($prResponse.value)) {
                $labelsUrl = "$repositoryApi/pullRequests/$($pr.pullRequestId)/labels?api-version=7.1-preview.1"
                $labelResponse = Invoke-PRPerfApi -Method Get -Uri $labelsUrl
                if (Test-PRPerfLabel -Labels @($labelResponse.value)) {
                    $pr
                }
            }
        )
    }

    $buildApi = "$collectionUri/$project/_apis/build"
    $recentRunsUrl = "$buildApi/builds?definitions=$perfPipelineId&queryOrder=queueTimeDescending&`$top=100&api-version=7.1"
    $recentRunsResponse = Invoke-PRPerfApi -Method Get -Uri $recentRunsUrl
    [object[]] $existingPerfRuns = @($recentRunsResponse.value)
}

$queueBodies = @(
    foreach ($pr in $pullRequests) {
        $sourceCommit = [string]$pr.lastMergeSourceCommit.commitId
        $targetCommit = [string]$pr.lastMergeTargetCommit.commitId

        if ($useFixtureApi) {
            [object[]] $sourceBuildCandidates = @(
                $allBuilds | Where-Object { [string]$_.definition.id -eq [string]$sourcePipelineId }
            )
            [object[]] $targetBuildCandidates = @(
                $allBuilds | Where-Object { [string]$_.definition.id -eq [string]$targetPipelineId }
            )
        } else {
            $buildApi = "$collectionUri/$project/_apis/build"
            $sourceBuildsUrl = "$buildApi/builds?definitions=$sourcePipelineId&sourceVersion=$sourceCommit&statusFilter=completed&resultFilter=succeeded&queryOrder=finishTimeDescending&api-version=7.1"
            $targetBuildsUrl = "$buildApi/builds?definitions=$targetPipelineId&sourceVersion=$targetCommit&statusFilter=completed&resultFilter=succeeded&queryOrder=finishTimeDescending&api-version=7.1"
            $sourceBuildResponse = Invoke-PRPerfApi -Method Get -Uri $sourceBuildsUrl
            $targetBuildResponse = Invoke-PRPerfApi -Method Get -Uri $targetBuildsUrl
            [object[]] $sourceBuildCandidates = @($sourceBuildResponse.value)
            [object[]] $targetBuildCandidates = @($targetBuildResponse.value)

            foreach ($build in @($sourceBuildCandidates) + @($targetBuildCandidates)) {
                $artifactsUrl = "$buildApi/builds/$($build.id)/artifacts?api-version=7.1"
                $artifactResponse = Invoke-PRPerfApi -Method Get -Uri $artifactsUrl
                Add-Member -InputObject $build -NotePropertyName artifacts -NotePropertyValue @($artifactResponse.value) -Force
            }
        }

        $sourceBuild = Select-ExactPRPerfBuild `
            -Builds $sourceBuildCandidates `
            -Commit $sourceCommit `
            -ArtifactName $artifactName
        $targetBuild = Select-ExactPRPerfBuild `
            -Builds $targetBuildCandidates `
            -Commit $targetCommit `
            -ArtifactName $artifactName
        $requestIdentity = New-PRPerfRequestIdentity `
            -RepositoryId ([string]$pr.repository.id) `
            -PullRequestId ([int]$pr.pullRequestId) `
            -SourceCommit $sourceCommit `
            -TargetCommit $targetCommit `
            -ConfigVersion 'pr-smoke-v1'
        $duplicateRequest = -not $Force -and @($existingPerfRuns | Where-Object {
            Test-PRPerfExistingRun -Run $_ -RequestIdentity $requestIdentity
        }).Count -gt 0
        if ($duplicateRequest) {
            Write-Host "Skipping PR $($pr.pullRequestId) because request identity '$requestIdentity' is already queued or completed."
        } else {
            $queueBodyObject = @{
                resources = @{
                    repositories = @{
                        self = @{
                            refName = $pr.sourceRefName
                            version = $sourceCommit
                        }
                    }
                }
                templateParameters = @{
                    pullRequestId = [string]$pr.pullRequestId
                    repositoryId = [string]$pr.repository.id
                    sourceCommit = $sourceCommit
                    targetCommit = $targetCommit
                    sourceBuildId = [string]$sourceBuild.id
                    targetBuildId = [string]$targetBuild.id
                    requestIdentity = $requestIdentity
                    useFixtures = $useFixtureApi
                }
                variables = @{
                    PRPerfTag = @{ value = 'PRPerf' }
                    PRPerfPullRequestTag = @{ value = "PRPerf-PR-$($pr.pullRequestId)" }
                    PRPerfRequestTag = @{ value = "PRPerf-Request-$($requestIdentity.Substring(0, 16))" }
                }
            }
            $queueBody = $queueBodyObject | ConvertTo-Json -Depth 20

            if (-not $DryRunOutputPath) {
                $queueUrl = "$collectionUri/$project/_apis/pipelines/$perfPipelineId/runs?api-version=7.1-preview.1"
                Invoke-PRPerfApi -Method Post -Uri $queueUrl -Body $queueBody | Out-Null
            }

            $queueBodyObject
        }
    }
)

if ($DryRunOutputPath) {
    if ($queueBodies.Count -gt 1) {
        throw "DryRunOutputPath requires exactly one resolved request; found $($queueBodies.Count)."
    }
    $dryRunJson = if ($queueBodies.Count -eq 1) {
        $queueBodies[0] | ConvertTo-Json -Depth 20
    } else {
        '[]'
    }
    Set-Content -LiteralPath $DryRunOutputPath -Value $dryRunJson -Encoding UTF8
}
