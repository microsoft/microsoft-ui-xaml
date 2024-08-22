Param(
    [Parameter(Position=0, Mandatory=$true)]
    [string] $NuGetConfigPath
)

. .\version.ps1
. .\template.ps1
. .\config.ps1

function Get-AvailablePackages ( $package )
{
    $result = @()

    $output = ( & nuget.exe list $package -prerelease -allversions -configfile $NuGetConfigPath )

    if ( $LastExitCode -ne 0 )
    {
        throw "FAILED: nuget.exe list"
    }

    foreach ( $line in $output )
    {
        $name, $version = $line.Split(" ")

        if ( $name -eq $package )
        {
            $result += ( MakeVersionFromString $version )
        }
    }

    return $result
}

function Install-Package ( $package, $version )
{
    & nuget.exe install $package -prerelease -version $version -configfile $NuGetConfigPath

    if ( $LastExitCode -ne 0 )
    {
        throw "FAILED: nuget.exe install"
    }
}

# We are no longer running the PGO pipeline. WinUI2 development has switched from 'main' branch to 'winui2/main'. 
# To keep things working without having to re-enable the PGO pipeline we hard-code the last version of MUXPGODatabase and skip generating it from git history.
# $forkPoint = ( GetForkPoint $pgoBranch )
# $requestedVersion = MakeVersion $releaseVersionMajor $releaseVersionMinor $rel4873563b7953e2431caed4efcdc3b8261d1405cdaseVersionPatch $releaseVersionPrerelease $forkPoint.DateString $forkPoint.BranchString

$requestedVersion = MakeVersion $releaseVersionMajor $releaseVersionMinor $releaseVersionPatch $releaseVersionPrerelease $forkPointDateString $pgoBranch

Write-Host ( "PGO OPTIMIZE: requesting {0} version {1} ({2})" -f $packageId, ( FormatVersion $requestedVersion ), $forkPoint.SHA )

$packageVersions = ( Get-AvailablePackages $packageId ) | Sort-Object -Descending -Property Major, Minor, Patch, Prerelease, Branch, Revision

$bestVersion = $null

foreach ( $existingVersion in $packageVersions )
{
    if ( ( CompareReleaseAndBranch $existingVersion $requestedVersion ) -eq $False )
    {
        # If this is different release number, pre-release tag or branch, then skip it.
        continue
    }

    # Sorting guarantees that all eligible entries are consecutive and sorted according to decreasing revision time stamp.
    # Revisions are fixed, 10 character strings containing numbers (YYMMHHhhmm).  Sorting will arrange them reverse-chronologically.
    # Once we are here, we are beginning of that segment.

    if ( ( CompareRevisions $existingVersion $requestedVersion ) -le 0 )
    {
        # Revisions are sorted in descending order, the first one less than or equal to the current is the one we want.
        $bestVersion = $existingVersion
        break
    }
}

if ( $bestVersion -eq $null )
{
    throw "Appropriate database cannot be found"
}

$bestVersionAsString = ( FormatVersion $bestVersion )

Write-Host ( "PGO OPTIMIZE: picked {0} version {1}" -f $packageId, $bestVersionAsString )

Install-Package $packageId $bestVersionAsString

FillOut-Template "PGO.version.props.template" "PGO.version.props" @{ "version" = $bestVersionAsString; "id" = $packageId }