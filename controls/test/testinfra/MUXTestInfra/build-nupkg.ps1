[CmdLetBinding()]
Param(
    [string]$BuildBinDir =  $(Resolve-Path "$PSScriptRoot\..\..\..\..\BuildOutput\bin"),
    [string]$OutputDir = $(Resolve-Path "$PSScriptRoot\..\..\..\..\PackageStore"),
    [string]$VersionOverride
)

$RootDir = $(Resolve-Path "$PSScriptRoot\..\..\..\..")

$scriptDirectory = $script:MyInvocation.MyCommand.Path | Split-Path -Parent

pushd $scriptDirectory

if (!$OutputDir)
{
    $OutputDir = $scriptDirectory
}

if (!$env:NUGETCMD) {

    cmd /c where /Q nuget.exe
    if ($lastexitcode -ne 0) {
        Write-Host "nuget not found on path. Either add it to path or set NUGETCMD environment variable." -ForegroundColor Red
        Exit 1
    }

    $env:NUGETCMD = "nuget.exe"
}

if ($VersionOverride)
{
    $version = $VersionOverride
}
else
{
    $version = "$env:versionFinal"

    if (!$version)
    {
        Write-Error "Expected versionFinal environment variable to have been set"
        Exit 1
    }

    Write-Verbose "Version = $version"
}

if (!(Test-Path $OutputDir)) { mkdir $OutputDir }


$CommonNugetArgs = "-properties `"BuildBinDir=$BuildBinDir``;RootDir=$RootDir``;Version=$version`""
$NugetArgs = "$CommonNugetArgs -OutputDirectory $(Resolve-Path $OutputDir)"


$nuspecs = Get-ChildItem $scriptDirectory -Filter *.nuspec

foreach ($nuspec in $nuspecs)
{
    $NugetCmdLine = "$env:NUGETCMD pack $nuspec $NugetArgs"
    Write-Host $NugetCmdLine
    Invoke-Expression $NugetCmdLine
    if ($lastexitcode -ne 0)
    {
        Write-Host "Nuget returned $lastexitcode"
        Exit $lastexitcode;
    }
}

popd