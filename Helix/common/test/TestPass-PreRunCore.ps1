Write-Host "TestPass-PreRunCore.ps1"

if(Test-Path TestPass-PreRun.ps1)
{
    & ./TestPass-PreRun.ps1
}