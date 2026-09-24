Param(
  [Parameter(Mandatory = $true, Position = 1)]
  [string] $token,
  [Parameter(Mandatory = $true, Position = 2)]
  [string] $startSha
)

$azureDevOpsRestApiHeaders = @{
    "Accept"="application/json"
    "Authorization"="Basic $([System.Convert]::ToBase64String([System.Text.ASCIIEncoding]::ASCII.GetBytes(":$($token)")))"
}

$uri = $env:WinUIPerfArtifactsPackageApiUri
if ( [string]::IsNullOrWhiteSpace( $uri ) )
{
    throw "WinUIPerfArtifactsPackageApiUri environment variable is not set. Set it to the Azure DevOps Package API base URI."
}
if ( -not [System.Uri]::IsWellFormedUriString( $uri, [System.UriKind]::Absolute ) )
{
    throw "WinUIPerfArtifactsPackageApiUri must be an absolute URI. Current value: '$uri'"
}

$testPoints = @( Get-Content testpoints.txt )

$gitHistory = @()
( & git rev-list --first-parent --format='%H' "$startSha~1..origin/main" ) -split "`n" | Where-Object { $i % 2 -eq 1; $i++ } | ForEach-Object { $gitHistory += [PSCustomObject] @{ SHA = $_; Artifacts = @(); } }

# Get commits for all versions of artifacts.

$artifactVersions = Invoke-RestMethod "$( $uri )/Versions" -Headers $azureDevOpsRestApiHeaders

foreach ( $artifact in $artifactVersions.value )
{
    if ( $artifact.isListed -and $artifact.version -match "3\.0\.0-zmain\.\d{6}\.\d+-CI" )
    {
        $provenance = ( Invoke-RestMethod "$($artifact.url)/Provenance" -Headers $azureDevOpsRestApiHeaders ).provenance.data
        $branch = $provenance | Select-String -Pattern "Build.SourceBranch=([^\s;]+)" | ForEach-Object { $_.Matches.Groups[1].Value }

        if ( $branch -eq "refs/heads/master" -or $branch -eq "refs/heads/main" )
        {
            $sha = $provenance | Select-String -Pattern "Build.SourceVersion=([0-9a-f]+)" | ForEach-Object { $_.Matches.Groups[1].Value }
            $knownItem = ( $gitHistory | Where-Object { $_.SHA -eq $sha } )

            if ( $null -ne $knownItem )
            {
                $knownItem.Artifacts += [PSCustomObject] @{ Version = $artifact.version; TestPointIndex = [array]::IndexOf( $testPoints, $artifact.version ); }
            }
        }
    }
}

function Dump-CheckinInfo ( $testPointVersion, $artifactVersion, $sha )
{
    if ( $null -ne $testPointVersion )
    {
        Write-Host -NoNewline "$testPointVersion"
    }

    Write-Host -NoNewline ","

    if ( $null -ne $artifactVersion )
    {
        Write-Host -NoNewline "$artifactVersion"
    }

    $gitOutput = ( & git show --first-parent --format='\"%cd\",\"%h\",\"%cn\",\"%s\"' --abbrev-commit --date=local --no-patch $sha )

    Write-Host ",$gitOutput"
}

Write-Host "TestPointVersion,ArtifactVersion,CommitDate,CommitSha,CommitAuthor,CommitComment"

for ( $checkinIndex = 0 ; $checkinIndex -lt $gitHistory.count ; $checkinIndex++ )
{
    $currentCheckin = $gitHistory[ $checkinIndex ]

    foreach ( $artifact in $currentCheckin.Artifacts )
    {
        $index = $artifact.TestPointIndex

        if ( $index -ne -1 )
        {
            $lastArtifactVersion = $artifact.Version

            Dump-CheckinInfo $artifact.Version $artifact.Version $currentCheckin.SHA

            for ( $i = $checkinIndex + 1 ; $i -lt $gitHistory.count ; $i++ )
            {
                if ( $gitHistory[ $i ].Artifacts | Where-Object { $_.TestPointIndex -ne -1 -and $_.TestPointIndex -lt $index } )
                {
                    break
                }

                $artifactVersion = ( $gitHistory[ $i ].Artifacts | Where-Object { $_.TestPointIndex -eq -1 } )

                if ( $null -ne $artifactVersion )
                {
                    $lastArtifactVersion = $artifactVersion.Version
                }

                Dump-CheckinInfo $artifact.Version $lastArtifactVersion $gitHistory[ $i ].SHA
            }
        }
    }
}