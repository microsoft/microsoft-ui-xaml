Param(
  [Parameter(Mandatory = $true, Position = 1)] [string] $pgoBranch,
  [Parameter(Mandatory = $true, Position = 2)] [string] $templatePath,
  [Parameter(Mandatory = $true, Position = 3)] [string] $outputPath)

. .\common.ps1
. .\get-version.ps1
. .\template.ps1

$version = "{0}.{1}" -f $releaseVersion, $( get_version $pgoBranch )
fill_out_template $templatePath $outputPath @{ "version" = $version; "id" = $packageId }