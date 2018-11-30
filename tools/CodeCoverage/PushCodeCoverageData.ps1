param(
[string]$MagellanInstallPath,
[string]$SqlConnectionString
)

Import-Module $PSScriptRoot\CodeCoverage.psm1 -Force -DisableNameChecking

Set-MagellanInstallPath -Path $MagellanInstallPath
Set-ConnectionString $SqlConnectionString

Import-CodeCoverageDataToDatabase