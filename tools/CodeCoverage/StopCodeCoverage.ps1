param(
[Parameter(Mandatory=$true, HelpMessage="Name of test suite. Ex: UNIT, BVT, FVT, etc")]
[string]$testSuite,
[Parameter(Mandatory=$true, HelpMessage="Path to where the Magellan tools are located on test machine")]
[string]$MagellanInstallPath,
[Parameter(Mandatory=$false, HelpMessage="Optional. SQL data connection string for using a Magellan Database. If not provided then covdata files need to be sent to build machine.")]
[string]$SqlConnectionString
)

Import-Module $PSScriptRoot\CodeCoverage.psm1 -Force -DisableNameChecking

Set-MagellanInstallPath -Path $magellanInstallPath

# save the captured data as files to be transported back to the build machine for publishing
Save-CodeCoverageData -testSuite $testSuite

# import the data into a Magellan SQL database if using one
# this is necessary if you need to merge coverage results from multiple test runs.
if ($SqlConnectionString -ne $null -and $SqlConnectionString -ne '')
{
    Set-ConnectionString -DBConn $SqlConnectionString
    Import-CodeCoverageDataToDatabase
}