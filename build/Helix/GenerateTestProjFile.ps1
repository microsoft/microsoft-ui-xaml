[CmdLetBinding()]
Param(
    [Parameter(Mandatory = $true)] 
    [string]$TestFilePattern,

    [Parameter(Mandatory = $true)] 
    [string]$TestBinaryDirectoryPath,

    [Parameter(Mandatory = $true)] 
    [string]$OutputProjFile,

    [string]$TaefQuery
)

[xml]$pkgVerData = (Get-Content "$PSScriptRoot\packages.config")
$muxTestInfraHelixVer = $pkgVerData.SelectSingleNode("//packages/package[@id=`"Microsoft.Internal.MUXTestInfra.Helix`"]").version

$packagesDir = Join-Path (Split-Path -Parent $script:MyInvocation.MyCommand.Path) "packages"
$pipelinesScriptsDir = Join-Path $packagesDir "Microsoft.Internal.MUXTestInfra.Helix.$muxTestInfraHelixVer\scripts\pipeline"

& $pipelinesScriptsDir\GenerateHelixWorkItems.ps1 -TestFilePattern $TestFilePattern `
    -TestBinaryDirectoryPath $TestBinaryDirectoryPath `
    -OutputProjFile $OutputProjFile `
    -TaefBaseQuery $TaefQuery `
    -TestTimeout "00:30:00" 