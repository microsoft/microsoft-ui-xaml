. .\common.ps1
. .\get-version.ps1
. .\template.ps1

$feedUri = "https://pkgs.dev.azure.com/ms/microsoft-ui-xaml/_packaging/MUX-Dependencies/nuget/v2"

# $packages = ( & git log --date=iso --pretty=format:"%ad" ) | foreach {
#   "2.0.{0}-mm" -f ( Get-Date -Date $_ ).ToUniversalTime().ToString("yyMMddHHmm")
# } | Sort-Object -Descending
# $currentDateBranch = "2001300148"

$currentDateBranch = $( get_version $pgoBranch )
$currentDateBranchParts = $currentDateBranch.Split("-")

$packageSource = Register-PackageSource -Name MUX_Dependencies -Location $feedUri -ProviderName NuGet -Trusted
$packages = ( Find-Package $packageId -Source MUX_Dependencies -MinimumVersion $releaseVersion -MaximumVersion $releaseVersion -AllowPrereleaseVersions ) | Sort-Object -Property Version -Descending

$best = $null

foreach ( $existing in $packages )
{
    $existingPackageVersionParts = $existing.Version.Split(".")[2].Split("-")
    #$existingPackageVersionParts = $existing.Split(".")[2].Split("-")

    if ( ( $existingPackageVersionParts.Length -ne $currentDateBranchParts.Length ) -or
         ( $existingPackageVersionParts.Length -eq 2 -and $existingPackageVersionParts[1] -ne $currentDateBranchParts[1] ) )
    {
        # skip it if branches mismatch
        continue
    }

    if ( $existingPackageVersionParts[0] -le $currentDateBranchParts[0] )
    {
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