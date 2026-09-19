[CmdletBinding()]
param(
    [Parameter(Mandatory)][string] $Owner,
    [Parameter(Mandatory)][string] $Repository,
    [Parameter(Mandatory)][int] $PullRequestNumber,
    [Parameter(Mandatory)][string] $ComparisonPath,
    [Parameter(Mandatory)][string] $ArtifactUrl,
    [Parameter(Mandatory)][string] $PipelineUrl,
    [Parameter(Mandatory)][string] $Token,
    [ValidatePattern('^[0-9a-fA-F]{40}$')][string] $ExpectedSourceCommit,
    [string] $XamlRegionsPath
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
Import-Module (Join-Path $root 'PRPerfComment.psm1') -Force
Import-Module (Join-Path $root 'PRPerfGitHub.psm1') -Force


$comparison = Get-Content -LiteralPath $ComparisonPath -Raw | ConvertFrom-Json
$xamlRegions = Get-PRPerfXamlRegions -Path $XamlRegionsPath
$xamlTraceApp = Get-PRPerfXamlTraceApp -Path $XamlRegionsPath
$markdown = New-PRPerfMarkdown `
    -Comparison $comparison `
    -ArtifactUrl $ArtifactUrl `
    -PipelineUrl $PipelineUrl `
    -XamlRegions $xamlRegions `
    -XamlTraceApp $xamlTraceApp

function Invoke-GitHubWrite {
    param(
        [Parameter(Mandatory)] [string] $Operation,
        [Parameter(Mandatory)] [scriptblock] $Action
    )

    try {
        & $Action
    } catch {
        $detail = Get-GitHubPRPerfErrorDetailFromRecord -ErrorRecord $_
        throw "Could not $Operation. $detail"
    }
}

$title = switch ($comparison.overallState) {
    'Passed' { 'Perf regression test passed' }
    'RegressionWarning' { 'Performance regression warning' }
    default { 'Performance result inconclusive' }
}
$state = switch ($comparison.overallState) {
    default { ConvertTo-GitHubPRPerfStatusState -OverallState $comparison.overallState }
}

$escapedOwner = [uri]::EscapeDataString($Owner)
$escapedRepository = [uri]::EscapeDataString($Repository)
$baseUrl = "https://api.github.com/repos/$escapedOwner/$escapedRepository"
$headers = New-GitHubPRPerfHeaders -Token $Token
$sourceSha = [string]$comparison.trial.commit

if (-not [string]::IsNullOrWhiteSpace($ExpectedSourceCommit)) {
    $currentPR = Get-GitHubPRPerfPullRequest -Uri "$baseUrl/pulls/$PullRequestNumber" -Token $Token
    $isSuperseded = -not (Test-GitHubPRPerfRequestCurrent `
        -ExpectedSourceCommit $ExpectedSourceCommit `
        -CurrentPullRequest $currentPR)
    if ($isSuperseded) {
        # Regenerate the report from a comparison whose verdict has actually been downgraded,
        # rather than prefixing a heading onto the original. Prefixing left the full passing
        # report intact below the heading, so a reader still saw a green table for results the
        # code had already decided no longer apply.
        $comparison.overallState = 'Inconclusive'
        $supersededIssue = 'Superseded by a newer commit - perf results no longer apply.'
        if ($null -eq $comparison.issues) {
            $comparison | Add-Member -NotePropertyName 'issues' -NotePropertyValue @($supersededIssue) -Force
        } else {
            $comparison.issues = @($comparison.issues) + $supersededIssue
        }
        $markdown = New-PRPerfMarkdown `
            -Comparison $comparison `
            -ArtifactUrl $ArtifactUrl `
            -PipelineUrl $PipelineUrl `
            -XamlRegions $xamlRegions `
            -XamlTraceApp $xamlTraceApp
        $marker = '<!-- winui-pr-perf-result -->'
        $markdownWithoutMarker = [regex]::Replace($markdown, "^\s*$([regex]::Escape($marker))\s*", '', 1)
        $markdown = "$marker`n## Superseded by a newer PR commit`n`n$supersededIssue`n`n$markdownWithoutMarker"
        $title = 'Superseded by a newer commit - perf results no longer apply.'
        $state = ConvertTo-GitHubPRPerfStatusState -OverallState 'Inconclusive'
    }
}

$marker = '<!-- winui-pr-perf-result -->'
$existing = $null
$page = 1
do {
    $commentsUrl = "$baseUrl/issues/$PullRequestNumber/comments?per_page=100&page=$page"
    [object[]] $comments = @(Invoke-RestMethod -Method Get -Uri $commentsUrl -Headers $headers)
    foreach ($comment in $comments) {
        if ([string]$comment.body -like "*$marker*") {
            $existing = $comment
            break
        }
    }
    $page++
} while ($null -eq $existing -and $comments.Count -eq 100)

if ($null -ne $existing) {
    $body = @{
        body = $markdown
    } | ConvertTo-Json -Depth 10
    Invoke-GitHubWrite -Operation 'update the PR perf comment' -Action {
        Invoke-RestMethod `
            -Method Patch `
            -Uri "$baseUrl/issues/comments/$($existing.id)" `
            -Headers $headers `
            -Body $body `
            -ContentType 'application/json' | Out-Null
    }
} else {
    $body = @{
        body = $markdown
    } | ConvertTo-Json -Depth 10
    Invoke-GitHubWrite -Operation 'post the PR perf comment' -Action {
        Invoke-RestMethod `
            -Method Post `
            -Uri "$baseUrl/issues/$PullRequestNumber/comments" `
            -Headers $headers `
            -Body $body `
            -ContentType 'application/json' | Out-Null
    }
}

$statusBody = @{
    state = $state
    description = $title
    target_url = $PipelineUrl
    context = 'winui-perf'
} | ConvertTo-Json -Depth 10
Invoke-GitHubWrite -Operation 'publish the PR perf commit status' -Action {
    Invoke-RestMethod `
        -Method Post `
        -Uri "$baseUrl/statuses/$sourceSha" `
        -Headers $headers `
        -Body $statusBody `
        -ContentType 'application/json' | Out-Null
}
