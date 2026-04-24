$helixCorrelationPath = $env:HELIX_CORRELATION_PAYLOAD

# We only need to run setup once per machine. We write a file into the payload dir to indicate that setup
# has been run for this machine.

$markerfile = Join-Path $helixCorrelationPath "setupcomplete.txt"
if(!(Test-Path($markerfile)))
{
    .\TestPass-OneTimeMachineSetupCore.ps1
    Set-Content $markerfile "done" 
}
else
{
    Write-Host "Setup already complete on this machine."
}