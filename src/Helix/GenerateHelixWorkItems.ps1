[CmdLetBinding()]
Param(
    [Parameter(Mandatory = $true)] 
    [string]$TestFilePattern,

    [Parameter(Mandatory = $true)] 
    [string]$TestBinaryPath,

    [Parameter(Mandatory = $true)] 
    [string]$OutputProjFile,

    [Parameter(Mandatory = $true)] 
    [ValidateSet("DevTestSuite", "ScenarioTestSuite")]
    [string]$JobTestSuiteName,

    [string]$CustomTaefQuery,

    [ValidateSet("UAP", "WPF", "Win32Explicit", "")]
    [string]$HostingMode,

    [ValidateSet("18362", "17763", "17134", "19042", "19045", "22000", "22621", "22631")]
    [string]$TargetOSVersion = "18362",

    [int]$TestExecutionMultiplier = 1,

    [bool]$RunIgnoredTests = $false,

    [bool]$IsValidateWindowsAppSDKRun = $false,

    [string]$TaefExePath
)


$testTimeout = "00:30:00"

$TestClassification = "Integration"
if($JobTestSuiteName -eq "ScenarioTestSuite")
{
    $TestClassification = "ScenarioTestSuite"
}

$taefPlatformQuery = "(@TestPass:IncludeOnlyOn='*Desktop*' OR (NOT(@TestPass:IncludeOnlyOn=*) AND NOT(@TestPass:ExcludeOn='*Desktop*')))"
if ($HostingMode)
{
    $taefHostModeQuery ="AND @Hosting:Mode='$HostingMode'"
}
$classificationQuery="AND @Classification='$TestClassification'"
$includeOnOsVerQuery="AND (@TestPass:MinOSVer<=$TargetOSVersion OR NOT @TestPass:MinOSVer=*) AND (@TestPass:MaxOSVer>=$TargetOSVersion OR NOT @TestPass:MaxOSVer=*)"

$taefBaseQuery = "$taefPlatformQuery $taefHostModeQuery $classificationQuery $includeOnOsVerQuery"
if($IsValidateWindowsAppSDKRun)
{
    $taefBaseQuery = "$taefBaseQuery AND NOT @IgnoreForValidateWindowsAppSDK='True'"
}


if($CustomTaefQuery)
{
    $taefBaseQuery = "$taefBaseQuery AND $CustomTaefQuery"
}

$WorkItemPrefix = ""
$testnameprefix = ""
if($HostingMode -eq "WPF")
{
    $WorkItemPrefix = "WPF"
    $testnameprefix = "WPF"
}
elseif($HostingMode -eq "Win32Explicit")
{
    $WorkItemPrefix = "Win32Explicit"
    $testnameprefix = "Win32Explicit"
}

if ($HostingMode) {
    $taefExtraParameters = "/p:HostingMode=$HostingMode"
}

$TestBinaryDirectoryPath = $TestBinaryPath

& $PSScriptRoot\common\pipeline\GenerateHelixWorkItems.ps1 -TestFilePattern $TestFilePattern `
    -TestBinaryDirectoryPath $TestBinaryDirectoryPath `
    -OutputProjFile $OutputProjFile `
    -TestExecutionMultiplier $TestExecutionMultiplier `
    -RunIgnoredTests $RunIgnoredTests `
    -WorkItemPrefix $WorkItemPrefix `
    -TaefBaseQuery $taefBaseQuery `
    -TestTimeout $testTimeout `
    -TaefExtraParameters $taefExtraParameters `
    -TestNamePrefix $testnameprefix `
    -TaefExePath $TaefExePath