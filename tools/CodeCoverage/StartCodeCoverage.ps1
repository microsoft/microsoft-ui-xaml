param(
[Parameter(Mandatory=$true, HelpMessage="Path to where the Magellan tools are located on test machine")]
[string]$MagellanInstallPath
)

Import-Module $PSScriptRoot\CodeCoverage.psm1 -Force -DisableNameChecking

Set-MagellanInstallPath -Path $magellanInstallPath
Invoke-CodeCoverage_PreTest