Param(
    [Parameter(Mandatory = $true)] 
    [string]$WttInputPath
)

Add-Type -Language CSharp -ReferencedAssemblies System.Xml,System.Xml.Linq (Get-Content .\HelixTestHelpers.cs -Raw)

[HelixTestHelpers.FailedTestDetector]::OutputFailedTests($WttInputPath)