if ( $env:BUILD_ARTIFACTSTAGINGDIRECTORY )
{
    # running in pipeline

    Import-Module .\log-pipeline.psm1 -DisableNameChecking

    $perfRootPath    = Resolve-Path "$PSScriptRoot\.."
    $taefPath        = Resolve-Path "$perfRootPath\..\..\Taef"
    $scriptsPath     = "$perfRootPath\scripts"
    $infraPath       = "$perfRootPath\infra"
    $profilesPath    = "$perfRootPath\profiles"
    $visPath         = "$perfRootPath\vis"
    $certsPath       = "$perfRootPath\scripts"
    $configFilePath  = "$profilesPath\config-pipeline.json"
}
elseif ( $env:BinRoot )
{
    # running in dev command line

    Import-Module .\log-console.psm1 -DisableNameChecking
 
    [xml]$packagesConfig = Get-Content "$env:RepoRoot\perf\packages.config"
    $taefVersion     = $packagesConfig.SelectNodes("/packages/package[@id='Microsoft.Taef']").version
    $infraVersion    = $packagesConfig.SelectNodes("/packages/package[@id='Microsoft.Internal.Performance.Infra']").version

    $perfRootPath    = Resolve-Path "$PSScriptRoot\.."
    $taefPath        = "$env:RepoRoot\packages\Microsoft.Taef\$taefVersion\build\binaries\x64"
    $scriptsPath     = "$perfRootPath\scripts"
    $infraPath       = "$env:RepoRoot\packages\microsoft.internal.performance.infra.$infraVersion\content"
    $profilesPath    = "$perfRootPath\profiles"
    $visPath         = "$perfRootPath\vis"
    $configFilePath  = "$profilesPath\config-user.json"
    $certsPath       = Resolve-Path "$perfRootPath\..\build"
    $env:perfAppsDir = [System.Environment]::ExpandEnvironmentVariables( "%BinRoot%\%_BuildArch%%_BuildType%\Test\perf\apps" )
}
else
{
    # running in regular command line

    Import-Module .\log-console.psm1 -DisableNameChecking

    $perfRootPath    = Resolve-Path "$PSScriptRoot\.."
    $taefPath        = Resolve-Path "$perfRootPath\Taef"
    $scriptsPath     = "$perfRootPath\scripts"
    $infraPath       = "$perfRootPath\infra"
    $profilesPath    = "$perfRootPath\profiles"
    $visPath         = "$perfRootPath\vis"
    $configFilePath  = "$profilesPath\config-user.json"
    $certsPath       = "$perfRootPath\scripts"
    $env:perfAppsDir = "$perfRootPath\apps"
}

function Dump-PathConfiguration ()
{
    Log-Debug "Path configuration:"
    Log-Debug "perfRootPath    = $perfRootPath"
    Log-Debug "taefPath        = $taefPath"
    Log-Debug "infraPath       = $infraPath"
    Log-Debug "scriptsPath     = $scriptsPath"
    Log-Debug "profilesPath    = $profilesPath"
    Log-Debug "visPath         = $visPath"
    Log-Debug "configFilePath  = $configFilePath"
    Log-Debug "certsPath       = $certsPath"
    Log-Debug "%perfAppsDir%   = $env:perfAppsDir"
}
