Param(
    [Parameter(Mandatory = $true)] 
    [string]$WttInputPath,

    [Parameter(Mandatory = $true)] 
    [string]$WttSingleRerunInputPath,

    [Parameter(Mandatory = $true)] 
    [string]$WttMultipleRerunInputPath,

    [Parameter(Mandatory = $true)] 
    [string]$WttMoreRerunInputPath,

    [Parameter(Mandatory = $true)] 
    [string]$XUnitOutputPath,

    [string]$TestNamePrefix
)

$WttInputPath = (Join-Path (Get-Location) $WttInputPath)
$WttSingleRerunInputPath = (Join-Path (Get-Location) $WttSingleRerunInputPath)
$WttMultipleRerunInputPath = (Join-Path (Get-Location) $WttMultipleRerunInputPath)
$WttMoreRerunInputPath = (Join-Path (Get-Location) $WttMoreRerunInputPath)
$XUnitOutputPath = (Join-Path (Get-Location) $XUnitOutputPath)

$rerunPassesRequiredToAvoidFailure = $env:rerunPassesRequiredToAvoidFailure

$testResultParser = [HelixTestHelpers.TestResultParser]::new($TestNamePrefix)
$testResultParser.ConvertWttLogToXUnitLog($WttInputPath, $WttSingleRerunInputPath, $WttMultipleRerunInputPath, $WttMoreRerunInputPath, $XUnitOutputPath, $rerunPassesRequiredToAvoidFailure, $true)