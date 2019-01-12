Param(
    [Parameter(Mandatory = $true)] 
    [string]$WttInputPath,

    [Parameter(Mandatory = $true)] 
    [string]$XUnitOutputPath,

    [Parameter(Mandatory = $true)] 
    [string]$testNamePrefix
)

# Ideally these would be passed as parameters to the script. However ps makes it difficult to deal with string literals containing '&', so we just 
# read the values directly from the environment variables
$helixResultsContainerUri = $Env:HELIX_RESULTS_CONTAINER_URI
$helixResultsContainerRsas = $Env:HELIX_RESULTS_CONTAINER_RSAS
$linkToUploadedWtlLog = "$helixResultsContainerUri/te.wtl$helixResultsContainerRsas"

Add-Type -Language CSharp -ReferencedAssemblies System.Xml,System.Xml.Linq (Get-Content .\ConvertWttLogToXUnit.cs -Raw)

[HelixTestHelpers.TestResultParser]::ConvertWttLogToXUnitLog($WttInputPath, $XUnitOutputPath, $testNamePrefix, $linkToUploadedWtlLog, $helixResultsContainerUri, $helixResultsContainerRsas)