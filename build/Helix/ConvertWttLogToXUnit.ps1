Param(
    [Parameter(Mandatory = $true)] 
    [string]$WttInputPath,

    [Parameter(Mandatory = $true)] 
    [string]$XUnitOutputPath,

    [Parameter(Mandatory = $true)] 
    [string]$testNamePrefix,

    [Parameter(Mandatory = $true)] 
    [string]$linkToUploadedWtlLog,

    [Parameter(Mandatory = $true)] 
    [string]$helixResultsContainerUri,

    [Parameter(Mandatory = $true)] 
    [string]$helixResultsContainerRsas
)

Add-Type -Language CSharp -ReferencedAssemblies System.Xml,System.Xml.Linq (Get-Content .\ConvertWttLogToXUnit.cs -Raw)

[HelixTestHelpers.TestResultParser]::ConvertWttLogToXUnitLog($WttInputPath, $XUnitOutputPath, $testNamePrefix, $linkToUploadedWtlLog, $helixResultsContainerUri, $helixResultsContainerRsas)