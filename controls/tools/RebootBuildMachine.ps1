[CmdLetBinding()]
param(
    [Parameter(mandatory=$true)]
    [string]$MachineName
    )

Import-Module -Name $PSScriptRoot\BuildMachineUtils.psm1 -DisableNameChecking

$adalPath = Get-AdalPath

Write-Verbose "ADALPath: $adalPath"
Add-Type -Path "$adalPath\lib\net45\Microsoft.IdentityModel.Clients.ActiveDirectory.dll"

$result = Queue-BuildOnMachine -MachineName $MachineName -BuildId "33711" # Reboot build machine build
Write-Host "$($MachineName): $($result._links.web.href)"