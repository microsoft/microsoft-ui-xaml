Param(
    [Parameter(Position=0, Mandatory = $true)]
    [string] $pathToProvision
)

. .\config-paths.ps1

if ( -not $env:BinRoot )
{
    Log-ErrorAndThrow "This script must be run from initialized dev environment."
}

$platform = $env:_BuildArch

if ( $platform -eq "amd64" )
{
    $platform = "x64"
}

if ( $pathToProvision )
{
    if ( -not ( Test-Path -LiteralPath $pathToProvision ) )
    {
        New-Item -Path $pathToProvision -ItemType Directory -Verbose | Out-Null
    }
    else
    {
        Log-Info "Removing contents of $pathToProvision"
        Remove-Item -Path "$pathToProvision\*" -Force -Recurse | Out-Null
    }

    # new infra -- provision only if running outside of dev prompt
    & msbuild.exe /p:Platform=$platform /p:PublishDestination="$pathToProvision" $perfRootPath\PublishInfrastructure.proj
}
