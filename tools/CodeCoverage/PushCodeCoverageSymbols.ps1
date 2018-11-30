param(
[string]$MagellanInstallPath,
[string]$SqlConnectionString,
[string]$CoverageSymPath
)

Import-Module $PSScriptRoot\CodeCoverage.psm1 -Force -DisableNameChecking

Set-MagellanInstallPath -Path $MagellanInstallPath
Set-ConnectionString $SqlConnectionString

Import-CodeCoverageSymbolsToDatabase $CoverageSymPath