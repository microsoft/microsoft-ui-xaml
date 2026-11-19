[CmdletBinding()]
param(
    [Parameter(Mandatory)][string] $TargetRoot,
    [Parameter(Mandatory)][string] $TrialRoot,
    [Parameter(Mandatory)][string] $ExpectedArtifactName
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$requiredPackage = 'XAMLPerf.ButtonApp.Cpp.MUX_1.0.0.0_x64_Test'
$requiredAppx = 'XAMLPerf.ButtonApp.Cpp.MUX_1.0.0.0_x64.appx'
$requiredAppsRelativePath = 'Test\perf\apps'
$issues = [Collections.Generic.List[string]]::new()

function Get-RelativePath {
    param([Parameter(Mandatory)][string] $Root, [Parameter(Mandatory)][string] $Path)

    $rootFull = [IO.Path]::GetFullPath($Root).TrimEnd('\') + '\'
    $pathFull = [IO.Path]::GetFullPath($Path)
    if (-not $pathFull.StartsWith($rootFull, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Path '$Path' is not below '$Root'."
    }
    return $pathFull.Substring($rootFull.Length)
}

function Get-ArtifactLayout {
    param([Parameter(Mandatory)][string] $Root, [Parameter(Mandatory)][string] $Label)

    if (-not (Test-Path -LiteralPath $Root -PathType Container)) {
        $issues.Add("$Label root is missing: $Root")
        return $null
    }

    $artifactRoot = Join-Path $Root $ExpectedArtifactName
    if (-not (Test-Path -LiteralPath $artifactRoot -PathType Container)) {
        $issues.Add("$Label artifact '$ExpectedArtifactName' is missing below '$Root'.")
        return $null
    }

    $expectedAppsRoot = Join-Path $artifactRoot $requiredAppsRelativePath
    $expectedPackageRoot = Join-Path $expectedAppsRoot $requiredPackage
    $packages = @(
        Get-ChildItem -LiteralPath $artifactRoot -Directory -Recurse |
            Where-Object Name -ceq $requiredPackage
    )
    $packagesAtExpectedPath = @(
        $packages | Where-Object {
            [IO.Path]::GetFullPath($_.FullName) -ceq [IO.Path]::GetFullPath($expectedPackageRoot)
        }
    )
    if ($packages.Count -ne 1 -or $packagesAtExpectedPath.Count -ne 1) {
        $foundPaths = @($packages | ForEach-Object {
            Get-RelativePath -Root $artifactRoot -Path $_.FullName
        })
        $foundDescription = if ($foundPaths.Count -eq 0) { '<none>' } else { $foundPaths -join ', ' }
        $issues.Add("$Label artifact must contain the required $requiredPackage exactly at the expected relative artifact layout '$requiredAppsRelativePath\$requiredPackage'; found: $foundDescription.")
        return $null
    }

    $packageRoot = $expectedPackageRoot
    $appsRoot = $expectedAppsRoot
    $appxPath = Join-Path $packageRoot $requiredAppx
    if (-not (Test-Path -LiteralPath $appxPath -PathType Leaf)) {
        $issues.Add("$Label required app package payload is missing: $requiredAppx")
    }

    $relativeFiles = @(
        Get-ChildItem -LiteralPath $packageRoot -File -Recurse |
            ForEach-Object { Get-RelativePath -Root $packageRoot -Path $_.FullName } |
            Sort-Object
    )
    if ($relativeFiles.Count -eq 0) {
        $issues.Add("$Label required app package contains no files.")
    }

    return [pscustomobject]@{
        ArtifactRoot = $artifactRoot
        AppsRoot = $appsRoot
        AppsRelativePath = $requiredAppsRelativePath
        RelativeFiles = $relativeFiles
    }
}

if ($ExpectedArtifactName -cnotmatch '^[A-Za-z0-9._-]+$') {
    throw "ExpectedArtifactName must be a safe artifact name without path separators: '$ExpectedArtifactName'."
}
if ($ExpectedArtifactName -notmatch '(?i)(?:x64|amd64)(?:release|fre)$') {
    throw "ExpectedArtifactName must identify x64/amd64 Release/fre output: '$ExpectedArtifactName'."
}

$targetLayout = Get-ArtifactLayout -Root $TargetRoot -Label 'Target'
$trialLayout = Get-ArtifactLayout -Root $TrialRoot -Label 'Trial'

if ($null -ne $targetLayout -and $null -ne $trialLayout) {
    if ($targetLayout.AppsRelativePath -cne $trialLayout.AppsRelativePath) {
        $issues.Add("Target and trial scenario-app subtrees differ: '$($targetLayout.AppsRelativePath)' versus '$($trialLayout.AppsRelativePath)'.")
    }

    $targetFiles = @($targetLayout.RelativeFiles)
    $trialFiles = @($trialLayout.RelativeFiles)
    $differences = @(
        Compare-Object -ReferenceObject $targetFiles -DifferenceObject $trialFiles |
            ForEach-Object { "$($_.SideIndicator) $($_.InputObject)" }
    )
    if ($differences.Count -gt 0) {
        $issues.Add("Target and trial relative file sets for the PR smoke app do not match: $($differences -join ', ')")
    }
}

if ($issues.Count -gt 0) {
    throw "PR perf artifacts are incompatible:`n - $($issues -join "`n - ")"
}

[pscustomobject]@{
    compatible = $true
    artifactName = $ExpectedArtifactName
    architecture = 'amd64'
    configuration = 'fre'
    appsRelativePath = $targetLayout.AppsRelativePath
    targetAppsRoot = $targetLayout.AppsRoot
    trialAppsRoot = $trialLayout.AppsRoot
} | ConvertTo-Json -Depth 4
