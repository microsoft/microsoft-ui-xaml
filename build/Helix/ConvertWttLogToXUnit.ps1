Param(
    [Parameter(Mandatory = $true)] 
    [string]$TaefInput,

    [Parameter(Mandatory = $true)] 
    [string]$XUnitOutput
)

Add-Type -Language CSharp -ReferencedAssemblies System.Xml,System.Xml.Linq (Get-Content .\ConvertWttLogToXUnit.cs -Raw)

[HelixTestHelpers.TestResultParser]::ConvertWttLogToXUnitLog($TaefInput, $XUnitOutput)