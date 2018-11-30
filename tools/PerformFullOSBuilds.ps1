[CmdLetBinding()]
Param(
    )

Import-Module -Name $PSScriptRoot\BuildMachineUtils.psm1 -DisableNameChecking

$adalPath = Get-AdalPath

Write-Verbose "ADALPath: $adalPath"
Add-Type -Path "$adalPath\lib\net45\Microsoft.IdentityModel.Clients.ActiveDirectory.dll"

$result = Queue-BuildOnMachine -MachineName "PKGESWINUI24" -ClientAlias "winui2" -BuildId "32408" # Perform Full OS Build
Write-Host "PKGESWINUI24: $($result._links.web.href)"
$result = Queue-BuildOnMachine -MachineName "PKGESWINUI25" -ClientAlias "winui2" -BuildId "32408" # Perform Full OS Build
Write-Host "PKGESWINUI25: $($result._links.web.href)"
$result = Queue-BuildOnMachine -MachineName "PKGESWINUI44" -ClientAlias "winui" -BuildId "32408" # Perform Full OS Build
Write-Host "PKGESWINUI44: $($result._links.web.href)"
