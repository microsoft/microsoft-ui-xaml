Write-Host "TestPass-PostRunCore.ps1"

if(Test-Path TestPass-PostRun.ps1)
{
    & ./TestPass-PostRun.ps1
}