[CmdletBinding()]
param(
    [Parameter(Mandatory)][string] $CollectionUri,
    [Parameter(Mandatory)][string] $Project,
    [Parameter(Mandatory)][string] $RepositoryId,
    [Parameter(Mandatory)][int] $PullRequestId,
    [Parameter(Mandatory)][string] $ComparisonPath,
    [Parameter(Mandatory)][string] $ArtifactUrl,
    [Parameter(Mandatory)][string] $PipelineUrl,
    [Parameter(Mandatory)][string] $AccessToken,
    [ValidatePattern('^[0-9a-fA-F]{40}$')][string] $ExpectedSourceCommit
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
Import-Module (Join-Path $root 'PRPerfComment.psm1') -Force
Import-Module (Join-Path $root 'PRPerfRequest.psm1') -Force

$comparison = Get-Content -LiteralPath $ComparisonPath -Raw | ConvertFrom-Json
$markdown = New-PRPerfMarkdown `
    -Comparison $comparison `
    -ArtifactUrl $ArtifactUrl `
    -PipelineUrl $PipelineUrl

$title = switch ($comparison.overallState) {
    'Passed' { 'Perf regression test passed' }
    'RegressionWarning' { 'Performance regression warning' }
    default { 'Performance result inconclusive' }
}
$state = switch ($comparison.overallState) {
    'Passed' { 'succeeded' }
    'RegressionWarning' { 'failed' }
    default { 'error' }
}

$collection = $CollectionUri.TrimEnd('/')
$escapedProject = [uri]::EscapeDataString($Project)
$escapedRepository = [uri]::EscapeDataString($RepositoryId)
$baseUrl = "$collection/$escapedProject/_apis/git/repositories/$escapedRepository/pullRequests/$PullRequestId"
$threadsUrl = "$baseUrl/threads?api-version=7.1-preview.1"
$headers = New-PRPerfAuthorizationHeaders -AccessToken $AccessToken

$isSuperseded = $false
if (-not [string]::IsNullOrWhiteSpace($ExpectedSourceCommit)) {
    $currentPR = Invoke-RestMethod -Method Get -Uri "$baseUrl?api-version=7.1" -Headers $headers
    $isSuperseded = -not (Test-PRPerfRequestCurrent `
        -ExpectedSourceCommit $ExpectedSourceCommit `
        -CurrentPR $currentPR)
    if ($isSuperseded) {
        $markdown = "Superseded by a newer PR commit`n`n$markdown"
        $title = 'Superseded by a newer PR commit'
        $state = 'notApplicable'
    }
}

$threadResponse = Invoke-RestMethod -Method Get -Uri $threadsUrl -Headers $headers
[object[]] $threads = @()
if ($null -ne $threadResponse.value) {
    $threads = @($threadResponse.value)
}
$existing = Find-PRPerfComment -Threads $threads

if ($null -ne $existing) {
    $body = @{
        content = $markdown
        commentType = 'text'
    } | ConvertTo-Json -Depth 10
    $commentUrl = "$baseUrl/threads/$($existing.ThreadId)/comments/$($existing.CommentId)?api-version=7.1-preview.1"
    Invoke-RestMethod `
        -Method Patch `
        -Uri $commentUrl `
        -Headers $headers `
        -Body $body `
        -ContentType 'application/json' | Out-Null
} else {
    $body = @{
        comments = @(
            @{
                parentCommentId = 0
                content = $markdown
                commentType = 'text'
            }
        )
        status = 'active'
    } | ConvertTo-Json -Depth 10
    Invoke-RestMethod `
        -Method Post `
        -Uri $threadsUrl `
        -Headers $headers `
        -Body $body `
        -ContentType 'application/json' | Out-Null
}

$statusBody = @{
    state = $state
    description = $title
    targetUrl = $PipelineUrl
    context = @{
        name = 'pr-smoke'
        genre = 'winui-perf'
    }
} | ConvertTo-Json -Depth 10
$statusUrl = "$baseUrl/statuses?api-version=7.1-preview.1"
Invoke-RestMethod `
    -Method Post `
    -Uri $statusUrl `
    -Headers $headers `
    -Body $statusBody `
    -ContentType 'application/json' | Out-Null
