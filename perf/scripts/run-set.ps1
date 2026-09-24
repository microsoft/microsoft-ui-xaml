#Requires -RunAsAdministrator

Param(
    [Parameter(Position = 0, Mandatory = $true)]
    [string] $runSetAndProfile,
    [string] $appsDir
)

$ErrorActionPreference = "Stop"

. .\config-paths.ps1

Import-Module .\config-helpers.psm1 -DisableNameChecking
Import-Module .\helpers.psm1 -DisableNameChecking

function Match-Directory( $appsDirectory, $matchRe )
{
    $directory = $null

    Get-ChildItem -Directory $appsDirectory | foreach {
        if ( $_.Name -match $matchRe )
        {
            if ( $directory -eq $null )
            {
                $directory = $_.Name
            }
            else
            {
                Log-ErrorAndThrow "Multiple matching directories for '$matchRe'."
            }
        }
    }

    return $directory
}

function Set-WinUIVersionPrefix( $appsDirectory )
{
    $prefix = Match-Directory $appsDirectory "^\d\.\d\.\d.*"

    if ( $prefix -ne $null )
    {
        $env:WinUIPrefix = $prefix + "\"
    }
    else
    {
        $env:WinUIPrefix = ".\"
    }

    Log-Debug "%WinUIPrefix%   = $env:WinUIPrefix"
}

function Set-WUXPrefix( $appsDirectory )
{
    $prefix = Match-Directory $appsDirectory "^WUX$"

    if ( $prefix -ne $null )
    {
        $env:WUXPrefix = $prefix + "\"
    }
    else
    {
        $env:WUXPrefix = ".\"
    }

    Log-Debug "%WUXPrefix%     = $env:WUXPrefix"
}

if ( $appsDir )
{
    if ( !( Test-Path $appsDir ) )
    {
        Log-ErrorAndThrow "Specified appsDir ($appsDir) does not exist."
    }

    $env:perfAppsDir = $appsDir
}

Dump-PathConfiguration

$configuration = Get-ResolvedConfiguration $configFilePath

Set-WinUIVersionPrefix $configuration.InstallersRootDirectory
Set-WUXPrefix $configuration.InstallersRootDirectory

$toolArgs = @( "--config-file", $configFilePath, "--legacy-query", $runSetAndProfile )

Run-Tool "$infraPath\ScenarioRunner.exe" $toolArgs
