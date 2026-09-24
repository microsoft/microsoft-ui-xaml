Param(
    [Parameter(Position = 0, Mandatory = $true)]
    [string] $experimentName
)

$ErrorActionPreference = "Stop"

. .\config-paths.ps1

Import-Module .\helpers.psm1 -DisableNameChecking

Dump-PathConfiguration

$toolArgs = @( "shift", "--config-file", $configFilePath )

Run-Tool "$infraPath\TraceProcessor.exe" $toolArgs

$toolArgs = @( "--config-file", $configFilePath, "--experiment-name", $experimentName )

Run-Tool "$infraPath\CSVCollator.exe" $toolArgs
