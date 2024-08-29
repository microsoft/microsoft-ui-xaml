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

# We are no longer running the PGO pipeline. WinUI2 development has switched from 'main' branch to 'winui2/main'. 
# To keep things working without having to re-enable the PGO pipeline we hard-code the last version of MUXPGODatabase here.
$forkPointDateString = "2310202140"