$pgoBranch           = "main"
$packageId           = "MUXPGODatabase"

# Get release version
[xml] $customProps   = ( Get-Content "..\..\version.props" )
# $releaseVersionMajor      = ( [int64]::Parse( $customProps.GetElementsByTagName("MUXVersionMajor").'#text' ) )
# $releaseVersionMinor      = ( [int64]::Parse( $customProps.GetElementsByTagName("MUXVersionMinor").'#text' ) )
# Temporarily hard-code to 2.8 until we can fix the PGO Pipeline for 2.9
$releaseVersionMajor      = [int64]2
$releaseVersionMinor      = [int64]8
$releaseVersionPatch      = [int64] 0
$releaseVersionPrerelease = $null