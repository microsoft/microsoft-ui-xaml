<#
.SYNOPSIS
    Run system quieting scripts and cleanup before test run.
.DESCRIPTION
    .
.PARAMETER help
    Get help on command.
#>

#Requires -RunAsAdministrator

Param(
    [switch] $help
)

$ErrorActionPreference = "Stop"

. .\config-paths.ps1

Import-Module .\helpers.psm1 -DisableNameChecking

if ( $help )
{
    Get-Help .\pre-run.ps1 -detailed
    exit
}

Dump-PathConfiguration

$args = @(
    "-addstore"
    "TrustedPeople"
    "$certsPath\WinUITest.cer"
)

reg add HKLM\SOFTWARE\Microsoft\WinUI\XAML /v EnableUWPWindow /t REG_DWORD /d 1 /f

Run-Tool "certutil.exe" $args

Log-Info "Copy quieting script to where configuration files are ($scriptsPath)."
Copy-Item "c:\perf\test\Microsoft.PerfGates.Test.Config.dll" -Destination "$scriptsPath" -Force -Verbose

$args = @(
    "$scriptsPath\Microsoft.PerfGates.Test.Config.dll"
    "/unicodeOutput:false"
    "/select:@Features='MachineConfig'"
)

Run-Tool "$taefPath\te.exe" $args
