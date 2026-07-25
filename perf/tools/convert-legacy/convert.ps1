Param(
    [string] $legacyDataPath
)

$ErrorActionPreference = "Stop"

Import-Module ..\..\scripts\log-console.psm1 -DisableNameChecking
Import-Module ..\..\scripts\helpers.psm1 -DisableNameChecking

$dateTimeFormat = "yyyy-MM-dd_HH-mm-ss"

$sideCarFiles = Get-ChildItem -Path $legacyDataPath -Filter "*.json" -Recurse

function Compress-ETL( $etlPath )
{
    $temporaryEtl = $etlPath + ".tmp"

    $toolArgs = @( "-merge", $etlPath, $temporaryEtl, "-compress", "-mergeonly" )
    Run-Tool "xperf.exe" $toolArgs

    Remove-Item $etlPath
    Rename-Item $temporaryEtl $etlPath
}

foreach ( $sideCarFile in $sideCarFiles)
{
    if ( Test-Path ( $sideCarFile.FullName + ".old" ) -PathType Leaf )
    {
        Remove-Item $sideCarFile.FullName
        Rename-Item ( $sideCarFile.FullName + ".old" ) $sideCarFile.FullName
    }

    $scenariosMap = ( Get-Content ( Join-Path $PSScriptRoot "scenarios.json" ) | ConvertFrom-JSON )
    $profilesMap = ( Get-Content ( Join-Path $PSScriptRoot "profiles.json" ) | ConvertFrom-JSON )

    $legacyData = ( Get-Content $sideCarFile.FullName | ConvertFrom-JSON )
    $newData = ( Get-Content ( Join-Path $PSScriptRoot "skeleton.json" ) | ConvertFrom-JSON )

    $newData.StartTime = [Datetime]::ParseExact( $legacyData.Start, $dateTimeFormat, $null ).ToString( "O" )
    $newData.EndTime = [Datetime]::ParseExact( $legacyData.End, $dateTimeFormat, $null ).ToString( "O" )
    $newData.WasSuccessful = ( $legacyData.TEExitCode -eq 0 )
    $newData.FullPath = Join-Path ( [System.IO.Path]::GetDirectoryName( $sideCarFile.FullName ) ) $legacyData.ETLFile

    if ( ! ( Test-Path -LiteralPath $newData.FullPath -PathType Leaf ) )
    {
        throw "ETL file does not exist"
    }

    $scenarioName = $legacyData.App.ScenarioName

    if ( ! $scenariosMap.$scenarioName )
    {
        throw "unknown scenario: $scenarioName"
    }

    $profileName = $legacyData.Profile.Analysis.Profile

    if ( ! $profilesMap.$profileName )
    {
        throw "unknown profile: $profileName"
    }

    # Compress-ETL $newData.FullPath

    $newData.Scenario = $scenariosMap.$scenarioName
    $newData.Scenario.App.Deployment.InstallerBaseDirectory = $legacyData.AppSourcePath
    $newData.Scenario.PreRunDelay = [TimeSpan]::FromMilliseconds( $legacyData.Profile.Execution.PreRunTime ).ToString() + "."
    $newData.Scenario.PostRunDelay = [TimeSpan]::FromMilliseconds( $legacyData.Profile.Execution.PostRunTime ).ToString() + "."
    $newData.Scenario.EndCondition.Timeout = [TimeSpan]::FromMilliseconds( $legacyData.Profile.Execution.AppActiveTime ).ToString() + "."

    $versionFromPath = $legacyData.AppSourcePath -match "3\.0\.0-.*\.\d{6}\.\d+-\w+"

    if ( $versionFromPath )
    {
        $fixedVersionSource = ( Get-Content ( Join-Path $PSScriptRoot "fixed-version-source.json" ) | ConvertFrom-JSON )
        $fixedVersionSource.Value = $Matches.0
        $newData.Scenario.App.VersionSource = $fixedVersionSource
    }

    $newData.Profile = $profilesMap.$profileName
    $newData.Profile.ScenarioLaunchCount = $legacyData.Profile.Execution.LaunchCount

    $newData.Name = $newData.Scenario.App.AppId + "." + $newData.Scenario.Name + "." + $newData.Profile.Name

    if ( $legacyData.Profile.Execution.AppArgs )
    {
        $newData.Scenario.Args += $legacyData.Profile.Execution.AppArgs
    }

    Rename-Item $sideCarFile.FullName ( $sideCarFile.FullName + ".old" )
    $newData | ConvertTo-Json -Depth 100 | Add-Content $sideCarFile.FullName
}