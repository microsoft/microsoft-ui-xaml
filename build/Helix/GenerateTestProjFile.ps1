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

# $repoDirectory = Join-Path (Split-Path -Parent $script:MyInvocation.MyCommand.Path) "..\..\"
# $packagesDir = Join-Path $repoDirectory "packages"

$packagesDir = Join-Path (Split-Path -Parent $script:MyInvocation.MyCommand.Path) "packages"
$pipelinesScriptsDir = Join-Path $packagesDir "Microsoft.Internal.MUXTestInfra.Helix.0.0.2.2\scripts\pipeline"



# $classificationQuery="@Classification='Integration'"
# $taefBaseQuery = "$classificationQuery"

$taefBaseQuery ="@Name='*ColorPicker*'"

if($TaefQuery)
{
    $taefBaseQuery = "$taefBaseQuery AND $TaefQuery"
}


& $pipelinesScriptsDir\GenerateHelixWorkItems.ps1 -TestFilePattern $TestFilePattern `
    -TestBinaryDirectoryPath $TestBinaryDirectoryPath `
    -OutputProjFile $OutputProjFile `
    -TaefBaseQuery $taefBaseQuery `
    -TestTimeout "00:30:00" `

    # -testnameprefix fo1
