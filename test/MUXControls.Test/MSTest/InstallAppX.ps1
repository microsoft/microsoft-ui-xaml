[CmdletBinding()]
param(
    [Parameter(Position=0,mandatory=$true)]
    [String]$AppXDir
)

Write-Host "Checking if the app's certificate is installed..." -ForegroundColor Magenta
Write-Host "" 

# If the certificate for the app is not present, installing it requires elevation.
# We'll run Add-AppDevPackage.ps1 without -Force in that circumstance so the user
# can be prompted to allow elevation.  We don't want to run it without -Force all the time,
# as that prompts the user to hit enter at the end of the install, which is an annoying
# and unnecessary step. The parameter is the SHA-1 hash of the certificate.
certutil.exe -verifystore TrustedPeople fd1d6927f4521242f00b20c9df66ea4f97175ee2
Write-Host "" 

if ($LASTEXITCODE -ge 0)
{
    Write-Host "Certificate is installed. Installing app..." -ForegroundColor Magenta
    Write-Host "" 
    & $AppXDir\Add-AppDevPackage.ps1 -Force
}
else
{
    Write-Host "Certificate is not installed. Installing app and certificate..." -ForegroundColor Magenta
    Write-Host "" 
    & $AppXDir\Add-AppDevPackage.ps1
}