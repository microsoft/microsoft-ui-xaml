[CmdLetBinding()]
Param(
    )

Import-Module -Name $PSScriptRoot\BuildMachineUtils.psm1 -DisableNameChecking

$adalPath = Get-AdalPath

Write-Verbose "ADALPath: $adalPath"
Add-Type -Path "$adalPath\lib\net45\Microsoft.IdentityModel.Clients.ActiveDirectory.dll"

for ($i = 30; $i -le 43; $i++)
{
    $machineName = "PKGESWINUI$i"
    $result = Queue-BuildOnMachine -MachineName $machineName -ClientAlias "depcontrols2" -BuildId "32303" # Update VS installs build
    Write-Host "$($machineName): $($result._links.web.href)"
}
