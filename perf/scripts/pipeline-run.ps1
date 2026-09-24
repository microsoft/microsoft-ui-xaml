Param(
    [Parameter(Position = 0, Mandatory = $true)]
    [string] $runSetAndProfile,
    [Parameter(Position = 1, Mandatory = $true)]
    [string] $experimentName,
    [string[]] $copyArtifactsTo = @()
)

$ErrorActionPreference = "Stop"

Import-Module .\config-helpers.psm1 -DisableNameChecking

. .\config-paths.ps1

function Compress-ETL( $etlPath )
{
    $temporaryEtl = $etlPath + ".tmp"

    $toolArgs = @( "-merge", $etlPath, $temporaryEtl, "-compress", "-mergeonly" )
    Run-Tool "xperf.exe" $toolArgs

    Remove-Item $etlPath
    Rename-Item $temporaryEtl $etlPath
}

function Generate-Visualizations( $dataPath )
{
    Push-Location $visPath
    
    try
    {
        Run-Tool "c:\python311\python.exe" @( "-m", "pip", "install", "--requirement", "requirements.txt" )
        Run-Tool "c:\python311\python.exe" @( "dashboard.py", "--data_path", $dataPath )
    }
    catch
    {
        Log-Warning $_
    }

    Pop-Location
}

$configuration = Get-ResolvedConfiguration $configFilePath

$userExperimentsDirectory = $configuration.CsvCollator.ExperimentsDirectory
$experimentDirectory = Join-Path $userExperimentsDirectory $experimentName

if ( Test-Path $configuration.ShiftsDirectory -PathType Container )
{
    Remove-Item ( Join-Path $configuration.ShiftsDirectory "*" ) -Recurse -Force
}

. .\run-set.ps1 $runSetAndProfile

# Since version of WPR installed on perf machine does not support -compress switch, we need to do it here.
Get-ChildItem -Path $configuration.ShiftsDirectory -Filter "*.etl" -Recurse | foreach { Compress-ETL $_.FullName }

. .\process.ps1 $experimentName

Generate-Visualizations $experimentDirectory

foreach ( $copy in $copyArtifactsTo )
{
    Log-Info "Copying artifacts from '$userExperimentsDirectory' to '$copy'."
    Copy-Item $userExperimentsDirectory -Destination $copy -Recurse -Force -ErrorAction Continue
}

$processedDataArchiveDirectory = Join-Path $experimentDirectory $configuration.CsvCollator.ProcessedDataArchiveDirectory

# We don't want to store all processed traces on perf machine, they are stored in other locations.  Delete them.
Log-Info "Removing '$processedDataArchiveDirectory'."
Remove-Item $processedDataArchiveDirectory -Recurse -Force
