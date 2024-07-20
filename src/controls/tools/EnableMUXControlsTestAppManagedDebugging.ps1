#requires -version 4.0
#requires –RunAsAdministrator

$muxControlsTestAppPath = (Get-AppxPackage MUXControlsTestApp.Desktop).InstallLocation

if ($muxControlsTestAppPath)
{
    Write-Host "Taking ownership of the path $muxControlsTestAppPath..."
    & takeown /F "$muxControlsTestAppPath\*" /R | Out-Null

    Write-Host "Granting administrator rights..."
    & icacls "$muxControlsTestAppPath\*" /grant:r administrators:f /T | Out-Null

    Write-Host "Successfully enabled managed debugging!"
    Write-Host
    Write-Host "NOTE: MAKE SURE you use Visual Studio 2022 and start the remote debugger as administrator! Managed debugging will not work otherwise!"
}
else
{
    Write-Error "MUXControlsTestApp is not installed!"
}