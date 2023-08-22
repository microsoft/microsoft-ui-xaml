$REVISION_FMT = 'yyMMddHHmm'

function MakeVersion ( $major, $minor, $patch, $prerelease, $revision, $branch )
{
    return [PSCustomObject] @{
        Major = [int64] $major
        Minor = [int64] $minor
        Patch = [int64] $patch
        Prerelease = [string] $prerelease
        Revision = [string] $revision
        Branch = [string] $branch
    }
}

function MakeVersionFromString ( $str )
{
    $match = [Regex]::match($str, '^(?<major>0|[1-9]\d*)\.(?<minor>0|[1-9]\d*)\.(?<patch>0|[1-9]\d*)(?:-(?<prerelease>(?:0|[1-9]\d*|\d*[a-zA-Z-][0-9a-zA-Z-]*)(?:\.(?:0|[1-9]\d*|\d*[a-zA-Z-][0-9a-zA-Z-]*))*))?(?:\+(?<buildmetadata>[0-9a-zA-Z-]+(?:\.[0-9a-zA-Z-]+)*))?$')

    if ( -not $match.Success )
    {
        throw "Failed to parse version string."
    }

    $REVISION_RE = '(?<revision>[0-9]{10})'
    $BRANCH_RE   = '(?<branch>[a-zA-Z0-9-]+)'

    $prereleaseMatch = [Regex]::match($match.Groups["prerelease"].Value, "^((?<actualprerelease>.*)-)?$REVISION_RE-$BRANCH_RE$")

    if ( $prereleaseMatch.Success )
    {
        # new way: revision + branch encoded in prerelease tag
        return MakeVersion $match.Groups["major"].Value `
                           $match.Groups["minor"].Value `
                           $match.Groups["patch"].Value `
                           $prereleaseMatch.Groups["actualprerelease"].Value `
                           $prereleaseMatch.Groups["revision"].Value `
                           $prereleaseMatch.Groups["branch"].Value
    }
    else
    {
        # old way: revision in patch and branch is prerelease
        $patchMatch = [Regex]::match($match.Groups["patch"].Value, "^$REVISION_RE$") 
        $prereleaseMatch = [Regex]::match($match.Groups["prerelease"].Value, "^$BRANCH_RE$")

        if ( -not ( $patchMatch.Success -and $prereleaseMatch.Success ) )
        {
            throw "Failed to parse fork point form version string."
        }

        return MakeVersion $match.Groups["major"].Value `
                           $match.Groups["minor"].Value `
                           0 `
                           $null `
                           $patchMatch.Groups["revision"].Value `
                           $prereleaseMatch.Groups["branch"].Value
    }
}

function FormatVersion ( $version )
{
    $branch = ""

    if ( $version.Branch -and $version.Branch -ne "" )
    {
        $branch = "-{0}" -f $version.Branch
    }

    $prerelease = ""

    if ( $version.Prerelease -and $version.Prerelease -ne "" )
    {
        $prerelease = "-{0}" -f $version.Prerelease
    }

    return "{0}.{1}.{2}{3}-{4}{5}" -f $version.Major, $version.Minor, $version.Patch, $prerelease, $version.Revision, $branch
}

function CompareReleaseAndBranch ( $version1, $version2 )
{
    return ( $version1.Major -eq $version2.Major ) -and
           ( $version1.Minor -eq $version2.Minor ) -and
           ( $version1.Patch -eq $version2.Patch ) -and
           ( $version1.Prerelease -like $version2.Prerelease ) -and
           ( $version1.Branch -like $version2.Branch )
}

function CompareRevisions ( $version1, $version2 )
{
    return [DateTime]::ParseExact($version1.Revision, $REVISION_FMT, $null) - [DateTime]::ParseExact($version2.Revision, $REVISION_FMT, $null)
}

function GetForkPoint ( $pgoBranch )
{
    $forkSHA = $( git merge-base origin/$pgoBranch HEAD )

    if ( $LastExitCode -ne 0 )
    {
        throw "FAILED: git merge-base"
    }

    $forkDate = ( Get-Date -Date $( git log -1 $forkSHA --date=iso --pretty=format:"%ad" ) ).ToUniversalTime().ToString($REVISION_FMT)

    if ( $LastExitCode -ne 0 )
    {
        throw "FAILED: Get forkDate"
    }

    return [PSCustomObject] @{
        DateString = $forkDate
        BranchString = ( $pgoBranch -replace "(/|\.|@|>|<)", "-" )
        SHA = $forkSHA
    }
}