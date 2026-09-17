[CmdletBinding()]
param(
    [Parameter(Mandatory)][string] $CollectionUri,
    [Parameter(Mandatory)][string] $Project,
    [Parameter(Mandatory)][string] $DefinitionId,
    [Parameter(Mandatory)][string] $SourceBranch,
    [Parameter(Mandatory)][string] $CurrentBuildId,
    [Parameter(Mandatory)][string] $ArtifactName,
    [string] $AccessToken = ''
)

# Best effort by design. Without a baseline the comparison still runs and still reports
# real numbers, it just cannot draw a conclusion from them, so a lookup failure here must
# never take the stage down with it.
$ErrorActionPreference = 'Continue'

function Set-BaselineOutput {
    param([string] $BuildId = '', [string] $Commit = '')
    Write-Host "##vso[task.setvariable variable=perfBaselineBuildId]$BuildId"
    Write-Host "##vso[task.setvariable variable=perfBaselineCommit]$Commit"
}

if ([string]::IsNullOrWhiteSpace($AccessToken)) {
    Write-Host '##vso[task.logissue type=warning]No access token was available, so no baseline build could be looked up.'
    Set-BaselineOutput
    return
}

try {
    $headers = @{ Authorization = "Bearer $AccessToken" }
    $base = $CollectionUri.TrimEnd('/') + '/' + [uri]::EscapeDataString($Project)
    $query = "$base/_apis/build/builds?definitions=$DefinitionId" +
        "&branchName=$([uri]::EscapeDataString($SourceBranch))" +
        '&statusFilter=completed&queryOrder=finishTimeDescending&$top=20&api-version=7.0'
    $builds = (Invoke-RestMethod -Uri $query -Headers $headers -Method Get).value

    foreach ($build in @($builds)) {
        if ([string]$build.id -eq [string]$CurrentBuildId) { continue }
        if ([string]::IsNullOrWhiteSpace([string]$build.sourceVersion)) { continue }

        # Only a build that still has the artifact is usable. Retention quietly removes
        # them, and discovering that at download time would waste the whole run.
        try {
            $artifacts = (Invoke-RestMethod -Method Get -Headers $headers `
                -Uri "$base/_apis/build/builds/$($build.id)/artifacts?api-version=7.0").value
        } catch {
            continue
        }
        if (@($artifacts | Where-Object { $_.name -eq $ArtifactName }).Count -eq 0) { continue }

        Write-Host "Baseline build $($build.id) at commit $($build.sourceVersion) still has '$ArtifactName'."
        Set-BaselineOutput -BuildId ([string]$build.id) -Commit ([string]$build.sourceVersion)
        return
    }

    Write-Host "##vso[task.logissue type=warning]No earlier build on '$SourceBranch' still has a '$ArtifactName' artifact, so there is no baseline to compare against."
    Set-BaselineOutput
} catch {
    Write-Host "##vso[task.logissue type=warning]Baseline lookup failed: $($_.Exception.Message)"
    Set-BaselineOutput
}
