Set-StrictMode -Version Latest

function Test-PRPerfLabel {
    param(
        [Parameter(Mandatory)][object[]] $Labels
    )

    return @($Labels | Where-Object { $_.name -ieq 'run-perf' }).Count -gt 0
}

function New-PRPerfAuthorizationHeaders {
    param(
        [Parameter(Mandatory)][string] $AccessToken
    )

    return @{
        Authorization = 'Bearer ' + $AccessToken
    }
}

function New-PRPerfRequestIdentity {
    param(
        [Parameter(Mandatory)][string] $RepositoryId,
        [Parameter(Mandatory)][int] $PullRequestId,
        [Parameter(Mandatory)][ValidatePattern('^[0-9a-fA-F]{40}$')][string] $SourceCommit,
        [Parameter(Mandatory)][ValidatePattern('^[0-9a-fA-F]{40}$')][string] $TargetCommit,
        [Parameter(Mandatory)][string] $ConfigVersion
    )

    $raw = "$RepositoryId|$PullRequestId|$($SourceCommit.ToLowerInvariant())|$($TargetCommit.ToLowerInvariant())|$ConfigVersion"
    $sha = [System.Security.Cryptography.SHA256]::Create()
    try {
        return ([BitConverter]::ToString(
            $sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($raw))
        )).Replace('-', '').ToLowerInvariant()
    } finally {
        $sha.Dispose()
    }
}

function Test-PRPerfRequestCurrent {
    param(
        [Parameter(Mandatory)][ValidatePattern('^[0-9a-fA-F]{40}$')][string] $ExpectedSourceCommit,
        [Parameter(Mandatory)] $CurrentPR
    )

    $currentCommit = $null
    $sourceCommitProperty = $CurrentPR.PSObject.Properties['lastMergeSourceCommit']
    if ($null -ne $sourceCommitProperty -and $null -ne $sourceCommitProperty.Value) {
        $commitProperty = $sourceCommitProperty.Value.PSObject.Properties['commitId']
        if ($null -ne $commitProperty) {
            $currentCommit = $commitProperty.Value
        }
    }
    return -not [string]::IsNullOrWhiteSpace([string]$currentCommit) -and
        [string]$currentCommit -ieq $ExpectedSourceCommit
}

function Get-PRPerfRunRequestIdentity {
    param($Run)

    if ($null -eq $Run) { return $null }
    $templateParameters = $Run.PSObject.Properties['templateParameters']
    if ($null -ne $templateParameters -and $null -ne $templateParameters.Value) {
        $requestIdentity = $templateParameters.Value.PSObject.Properties['requestIdentity']
        if ($null -ne $requestIdentity -and -not [string]::IsNullOrWhiteSpace([string]$requestIdentity.Value)) {
            return [string]$requestIdentity.Value
        }
    }

    $parameters = $Run.PSObject.Properties['parameters']
    if ($null -ne $parameters -and -not [string]::IsNullOrWhiteSpace([string]$parameters.Value)) {
        try {
            $parsed = [string]$parameters.Value | ConvertFrom-Json
            $requestIdentity = $parsed.PSObject.Properties['requestIdentity']
            if ($null -ne $requestIdentity -and -not [string]::IsNullOrWhiteSpace([string]$requestIdentity.Value)) {
                return [string]$requestIdentity.Value
            }
        } catch {
        }
    }

    return $null
}

function Test-PRPerfExistingRun {
    param(
        [Parameter(Mandatory)] $Run,
        [Parameter(Mandatory)][ValidatePattern('^[0-9a-fA-F]{64}$')][string] $RequestIdentity
    )

    $identity = Get-PRPerfRunRequestIdentity -Run $Run
    if (-not [string]::IsNullOrWhiteSpace($identity) -and $identity -ieq $RequestIdentity) {
        return $true
    }

    $tagsProperty = $Run.PSObject.Properties['tags']
    if ($null -ne $tagsProperty) {
        $requestTag = 'PRPerf-Request-' + $RequestIdentity.Substring(0, 16)
        if (@($tagsProperty.Value | Where-Object { [string]$_ -ieq $requestTag }).Count -gt 0) {
            return $true
        }
    }

    return $false
}

function Select-ExactPRPerfBuild {
    param(
        [Parameter(Mandatory)][object[]] $Builds,
        [Parameter(Mandatory)][string] $Commit,
        [Parameter(Mandatory)][string] $ArtifactName
    )

    $matches = @($Builds | Where-Object {
        $artifactsProperty = $_.PSObject.Properties['artifacts']
        $_.sourceVersion -ieq $Commit -and
            $_.status -eq 'completed' -and
            $_.result -eq 'succeeded' -and
            $null -ne $artifactsProperty -and
            @($artifactsProperty.Value | Where-Object { $_.name -eq $ArtifactName }).Count -eq 1
    } | Sort-Object finishTime -Descending)

    if ($matches.Count -eq 0) {
        throw "No succeeded build with artifact '$ArtifactName' exists for commit '$Commit'."
    }
    return $matches[0]
}

Export-ModuleMember -Function Test-PRPerfLabel, New-PRPerfAuthorizationHeaders, New-PRPerfRequestIdentity, Test-PRPerfRequestCurrent, Test-PRPerfExistingRun, Select-ExactPRPerfBuild
