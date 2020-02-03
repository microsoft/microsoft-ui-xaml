Param(
  [Parameter(Mandatory = $true, Position = 1)] [string] $templatePath,
  [Parameter(Mandatory = $true, Position = 2)] [string] $outputPath)

. .\version.ps1
. .\template.ps1
. .\config.ps1

$version = FormatVersion ( MakeVersion $releaseVersionMajor $releaseVersionMinor ( GetDatetimeStamp $pgoBranch $isReleaseBranch ) )
Write-Host ( "PGO INSTRUMENT: generating {0} version {1}" -f $packageId, $version )
fill_out_template $templatePath $outputPath @{ "version" = $version; "id" = $packageId }