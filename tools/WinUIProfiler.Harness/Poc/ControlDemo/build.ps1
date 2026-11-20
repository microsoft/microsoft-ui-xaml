# Builds in a child shell so product initialization cannot replace the capture process's .NET environment.
#Requires -Version 7
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$InitRun,
    [Parameter(Mandatory)][string]$DotNet,
    [Parameter(Mandatory)][string]$OutputRoot
)

$ErrorActionPreference = 'Stop'
Push-Location $PSScriptRoot
try {
    & $InitRun -Command @(
        $DotNet, 'build', (Join-Path $PSScriptRoot 'ControlDemo.csproj'), '--configuration', 'Release',
        "-p:PocOutputRoot=`"$OutputRoot`"", '-p:RestoreLockedMode=true',
        '--verbosity', 'quiet', '--nologo'
    )
} finally {
    Pop-Location
}
exit $LASTEXITCODE
