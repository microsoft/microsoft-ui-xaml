[CmdLetBinding()]
Param(
    [Parameter(Mandatory = $true)] 
    [string]$TestFilePattern,

    [Parameter(Mandatory = $true)] 
    [string]$TestBinaryDirectoryPath,

    [Parameter(Mandatory = $true)] 
    [string]$OutputProjFile,

    [Parameter(Mandatory = $true)] 
    [string]$JobTestSuiteName,

    [string]$TaefQuery
)

# $repoDirectory = Join-Path (Split-Path -Parent $script:MyInvocation.MyCommand.Path) "..\..\"
# $packagesDir = Join-Path $repoDirectory "packages"

$packagesDir = Join-Path (Split-Path -Parent $script:MyInvocation.MyCommand.Path) "packages"
$pipelinesScriptsDir = Join-Path $packagesDir "Microsoft.Internal.MUXTestInfra.Helix.0.0.1\scripts\pipeline"



$classificationQuery="@Classification='Integration'"
$taefBaseQuery = "$classificationQuery"

if($TaefQuery)
{
    $taefBaseQuery = "$taefBaseQuery AND $TaefQuery"
}



& $pipelinesScriptsDir\GenerateHelixWorkItemsCore.ps1 -TestFilePattern $TestFilePattern `
    -TestBinaryDirectoryPath $TestBinaryDirectoryPath `
    -OutputProjFile $OutputProjFile `
    -taefBaseQuery $taefBaseQuery `
    -testTimeout "00:31:00" `
    -testnameprefix fo1
