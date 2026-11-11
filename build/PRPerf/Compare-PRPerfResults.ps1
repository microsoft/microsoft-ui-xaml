[CmdletBinding()]
param(
    [Parameter(Mandatory)][string] $TargetPath,
    [Parameter(Mandatory)][string] $TrialPath,
    [Parameter(Mandatory)][string] $ThresholdPath,
    [Parameter(Mandatory)][string] $OutputDirectory,
    [string] $ArtifactUrl = '',
    [string] $PipelineUrl = '',
    [string] $SourceCommit = '',
    [string] $TargetCommit = '',
    [string] $SourceBuildId = '',
    [string] $TargetBuildId = '',
    [string] $BaselineKind = '',
    [string] $RequestIdentity = ''
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
Import-Module (Join-Path $root 'PRPerfResults.psm1') -Force
Import-Module (Join-Path $root 'PRPerfComment.psm1') -Force

$comparison = Compare-PRPerfFiles `
    -TargetPath $TargetPath `
    -TrialPath $TrialPath `
    -ThresholdPath $ThresholdPath `
    -ExpectedTargetCommit $TargetCommit `
    -ExpectedTrialCommit $SourceCommit

# Record which side the baseline actually came from so the comment cannot imply the delta
# is attributable to this pull request when it was only compared with its own earlier build.
if (-not [string]::IsNullOrWhiteSpace($BaselineKind)) {
    $comparison.target | Add-Member -NotePropertyName baselineKind -NotePropertyValue $BaselineKind -Force
}
if ($comparison.overallState -eq 'Inconclusive' -and
    (-not [string]::IsNullOrWhiteSpace($SourceCommit) -or
     -not [string]::IsNullOrWhiteSpace($TargetCommit) -or
     -not [string]::IsNullOrWhiteSpace($RequestIdentity))) {
    $comparison.issues += "Request context: target commit '$TargetCommit' (build '$TargetBuildId'), source commit '$SourceCommit' (build '$SourceBuildId'), request identity '$RequestIdentity'."
}

New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
$comparison |
    ConvertTo-Json -Depth 12 |
    Set-Content -LiteralPath (Join-Path $OutputDirectory 'comparison.json') -Encoding UTF8

New-PRPerfMarkdown `
    -Comparison $comparison `
    -ArtifactUrl $ArtifactUrl `
    -PipelineUrl $PipelineUrl |
    Set-Content -LiteralPath (Join-Path $OutputDirectory 'comparison.md') -Encoding UTF8
