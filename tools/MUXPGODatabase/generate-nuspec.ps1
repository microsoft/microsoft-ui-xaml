Param(
  [Parameter(Mandatory = $true, Position = 1)] [string] $templatePath,
  [Parameter(Mandatory = $true, Position = 2)] [string] $outputPath)

. .\version.ps1
. .\template.ps1
. .\config.ps1

$forkPoint = ( GetForkPoint $pgoBranch )
$version = FormatVersion ( MakeVersion $releaseVersionMajor $releaseVersionMinor $releaseVersionPatch $releaseVersionPrerelease $forkPoint.DateString $forkPoint.BranchString )
Write-Host ( "PGO INSTRUMENT: generating {0} version {1} ({2})" -f $packageId, $version, $forkPoint.SHA )
FillOut-Template $templatePath $outputPath @{ "version" = $version; "id" = $packageId }