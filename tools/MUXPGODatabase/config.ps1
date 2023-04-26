$pgoBranch           = "user/kmahone/1estest"
$packageId           = "MUXPGODatabase-test3"

# Get release version
[xml] $customProps   = ( Get-Content "..\..\version.props" )
$releaseVersionMajor      = ( [int64]::Parse( $customProps.GetElementsByTagName("MUXVersionMajor").'#text' ) )
$releaseVersionMinor      = ( [int64]::Parse( $customProps.GetElementsByTagName("MUXVersionMinor").'#text' ) )
$releaseVersionPatch      = [int64] 0
$releaseVersionPrerelease = $null