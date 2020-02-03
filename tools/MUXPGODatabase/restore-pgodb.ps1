. .\version.ps1
. .\template.ps1
. .\config.ps1

$feedUri = "https://pkgs.dev.azure.com/ms/microsoft-ui-xaml/_packaging/MUX-Dependencies/nuget/v2"

# $packages = ( & git log --date=iso --pretty=format:"%ad" ) | foreach {
#   "2.0.{0}-master" -f ( Get-Date -Date $_ ).ToUniversalTime().ToString("yyMMddHHmm")
# } | Sort-Object -Descending
# $currentVersion = MakeVersion $releaseVersionMajor $releaseVersionMinor "1807261844-master"

$currentVersion = MakeVersion $releaseVersionMajor $releaseVersionMinor ( GetDatetimeStamp $pgoBranch $isReleaseBranch )

$packageSource = Register-PackageSource -Name MUX_Dependencies -Location $feedUri -ProviderName NuGet -Trusted
$packages = ( Find-Package $packageId -Source MUX_Dependencies -AllowPrereleaseVersions -AllVersions ) | Sort-Object -Property Version -Descending

$best = $null

foreach ( $existing in $packages )
{
    $existingVersion = MakeVersionFromString $existing.Version

    if ( ( CompareBranches $existingVersion $currentVersion ) -eq $False -or
         ( CompareReleases $existingVersion $currentVersion ) -ne 0 )
    {
        # If this is different release or branch, then skip it.
        continue
    }

    if ( ( CompareRevisions $existingVersion $currentVersion ) -le 0 )
    {
        # Version are sorted in descending order, the first one less than or equal to the current is the one we want.
        # NOTE: at this point the only difference between versions will be revision (date-time stamp)
        # which is formatted as a fixed-length string, so string comparison WILL sort it correctly.
        $best = $existing
        break
    }
}

if ( $best -eq $null )
{
    throw "Appropriate database cannot be found"
}

Write-Host ( "PGO OPTIMIZATION: picked {0} version {1}" -f $packageId, $best.Version )
#Write-Host ( "Picked {0} version {1}" -f $packageId, $best )

$best | Install-Package -Destination ..\..\packages -Force
$packageSource | Unregister-PackageSource

fill_out_template "PGO.version.props.template" "PGO.version.props" @{ "version" = $best.Version; "id" = $packageId }