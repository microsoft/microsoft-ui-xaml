# Magellan toolkit is provided as a nuget package delivered
# to the target machine using the Package ES Build Initialize task.
# That task defines the XES_ToolPack_Magellan environment variable
# to discover the location of covermon.exe and other tools
# Covermon is run interactively. The data is stored and ready
# from a Magellan DB so that coverage results from multiple runs
# can be properly merged.

Add-Type -Language CSharp @"
using System;
using System.Collections.Generic;

namespace CodeCoverage {
    public class SourceLineCoverageData {
        public SourceLineCoverageData(int coveredBlocks, int totalBlocks) {
            CoveredBlocks = coveredBlocks;
            TotalBlocks = totalBlocks;
        }

        public int CoveredBlocks;
        public int TotalBlocks;
    }

    public class SourceFileCoverageData {
        public SourceFileCoverageData() {
            LineNumberToCoverageData = new System.Collections.Generic.Dictionary<int, SourceLineCoverageData>();
            CoveredMethodNames = new System.Collections.Generic.List<string>();
            TotalMethodNames = new System.Collections.Generic.List<string>();
            CoveredClassNames = new System.Collections.Generic.List<string>();
            TotalClassNames = new System.Collections.Generic.List<string>();
        }

        public System.Collections.Generic.List<string> CoveredMethodNames;
        public System.Collections.Generic.List<string> TotalMethodNames;
        public System.Collections.Generic.List<string> CoveredClassNames;
        public System.Collections.Generic.List<string> TotalClassNames;

        public System.Collections.Generic.Dictionary<int, SourceLineCoverageData> LineNumberToCoverageData;
    }

    public class BinaryCoverageData {
        public string BinaryName;
        public int CoveredInstructions;
        public int TotalInstructions;
        public int CoveredLines;
        public int TotalLines;
        public int CoveredMethods;
        public int TotalMethods;
        public int CoveredClasses;
        public int TotalClasses;
    }
}
"@

function Set-MagellanInstallPath
{
   param (
   $Path
   )

   New-Variable -Name magellanInstallPath -Value $Path -Scope Script
}

function Invoke-CodeCoverage_PreTest
{
    # add strongname verification skip for MagellanSDK DLL
   # & "${env:ProgramFiles(x86)}\Microsoft SDKs\Windows\v10.0A\bin\NETFX 4.6 Tools\sn.exe" -Vr *,31bf3856ad364e35

    Install-DLLs
    Start-Covermon
    Sleep -Seconds 10
    Invoke-ConfigureCoverMon
}

function Invoke-CodeCoverage_PostTest
{
    Save-CodeCoverageDataToDatabase
}

function Install-DLLs
{
  # if Magellan is not installed on the target machine, the x86 build of coverage.dll needs to be copied to the c:\Windows\SysWOW64 folder
  # and the x64 version of coverage.DLL needs to be copied to the c:\windows\system32 folder.
  # On an x86 machine, the x86 build needs to be copied to c:\windows\system32.
  $syswow64path = "$env:windir\SysWOW64"
  $system32path = "$env:windir\system32"

  if ($env:PROCESSOR_ARCHITECTURE -ilike "x86")
  {
      Write-Host "Copying coverage.dll to $system32path"
      $dllPath = Join-Path (Get-MagellanPath) "coverage.dll"
      Copy-Item -Path $dllPath -Destination $system32path -Force -Verbose

      Write-Host "Copying msvcp140.dll to $system32path"
      $dllPath = Join-Path (Get-MagellanPath) "msvcp140.dll"
      Copy-Item -Path $dllPath -Destination $system32path -Force -Verbose

      Write-Host "Copying vcruntime140.dll to $system32path"
      $dllPath = Join-Path (Get-MagellanPath) "vcruntime140.dll"
      Copy-Item -Path $dllPath -Destination $system32path -Force -Verbose
  }
  else
  {
      Write-Host "Copying coverage.dll to $syswow64path"
      $dllPath = Join-Path (Get-MagellanX86Path) "coverage.dll"
      Copy-Item -Path $dllPath -Destination $syswow64path -Force -Verbose

      Write-Host "Copying msvcp140.dll to $syswow64path"
      $dllPath = Join-Path (Get-MagellanX86Path) "msvcp140.dll"
      Copy-Item -Path $dllPath -Destination $syswow64path -Force -Verbose

      Write-Host "Copying vcruntime140.dll to $syswow64path"
      $dllPath = Join-Path (Get-MagellanX86Path) "vcruntime140.dll"
      Copy-Item -Path $dllPath -Destination $syswow64path -Force -Verbose

      Write-Host "Copying coverage.dll to $system32path"
      $dllPath = Join-Path (Get-MagellanPath) "coverage.dll"
      Copy-Item -Path $dllPath -Destination $system32path -Force -Verbose

      Write-Host "Copying msvcp140.dll to $system32path"
      $dllPath = Join-Path (Get-MagellanPath) "msvcp140.dll"
      Copy-Item -Path $dllPath -Destination $system32path -Force -Verbose

      Write-Host "Copying vcruntime140.dll to $system32path"
      $dllPath = Join-Path (Get-MagellanPath) "vcruntime140.dll"
      Copy-Item -Path $dllPath -Destination $system32path -Force -Verbose
  }
}

function Start-Covermon
{
    if ((Invoke-Expression "tasklist | findstr /i covermon.exe").Length -gt 0)
    {
        # CoverMon.exe is already running, so no need to start another instance of it.
        return
    }

    $covermonExePath = Join-Path (Get-MagellanPath) "covermon.exe"

    if (-not(Test-Path -Path $covermonExePath))
    {
        Write-Warning "Cannot find covermon.exe at $covermonExePath"
        Write-Error "Failed to start code coverage monitor"
    }

    # In RS1, the covermon console window will take foreground and the test window
    # will fail to claim it later. Running tests with an inactive window will
    # cause tests that rely on focus events (GotFocus and LostFocus) to fail.
    # To avoid this, we hide the covermon console.
    Start-Process -WindowStyle Hidden -FilePath $covermonExePath
}

function Invoke-CodeCoverage_PostBuild
{
    param(
        [Parameter(Mandatory=$true, HelpMessage="VSO account name, default microsoft")]
        [ValidateNotNullOrEmpty()]
        [string] $vsoAccount,

        [Parameter(Mandatory=$true, HelpMessage="VSO project name or Id")]
        [ValidateNotNullOrEmpty()]
        [string] $vsoProject,

        [Parameter(Mandatory=$true, HelpMessage="Authorization token")]
        [ValidateNotNullOrEmpty()]
        [string] $vsoAuthorizationToken,

        [Parameter(Mandatory=$true, HelpMessage="VSO Build Number")]
        [ValidateNotNullOrEmpty()]
        [string] $buildNumber,

        [Parameter(Mandatory=$true, HelpMessage="Configuration the test is running: Release, Debug, or Cover")]
        [ValidateSet("Release","Debug","Cover")]
        [string] $buildConfiguration,
    
        [Parameter(Mandatory=$true, HelpMessage="Architecture of the build to test: x86, x64, or arm")]
        [ValidateSet("x86","x64","arm")]
        [string] $buildPlatform,

        [Parameter(Mandatory=$true, HelpMessage="Path to output the code coverage summary file to.  The default is the Build.ArtifactStagingDirectory variable.")]
        [ValidateNotNullOrEmpty()]
        [string] $coverageSummaryOutputPath,

        [Parameter(Mandatory=$true, HelpMessage="Filename of the Code Coverage Summary XML file  The default is the CodeCoverageSummary.xml.")]
        [ValidateNotNullOrEmpty()]
        [string] $coverageSummaryFileName = "CodeCoverageSummary.xml"
    )

    Publish-CodeCoverageResults `
        -vsoAccount $vsoAccount -vsoProject $vsoProject -vsoAuthorizationToken $vsoAuthorizationToken `
        -buildNumber $buildNumber -buildConfiguration $buildConfiguration -buildPlatform $buildPlatform `
        -coverageSummaryOutputPath $coverageSummaryOutputPath -coverageSummaryFileName $coverageSummaryFileName
}

function Get-MagellanPath
{
    $magellanPathName = "$magellanInstallPath\tools\$env:PROCESSOR_ARCHITECTURE"
    if (-not(Test-Path -Path $magellanPathName))
    {
        if ($env:PROCESSOR_ARCHITECTURE -like "amd64")
        {
            $magellanPathName = "$magellanInstallPath\tools\x64"
        }
        
        if (-not(Test-Path -Path $magellanPathName))
        {
            Write-Error "Cannot find path $magellanPathName"
        }
    }
    Write-Host "Get-MagellanPath: $magellanPathName"
    return $magellanPathName
}

function Get-MagellanX86Path
{
    $magellanPathName = "$magellanInstallPath\tools\x86"
    if (-not(Test-Path -Path $magellanPathName))
    {
       Write-Error "Cannot find path $magellanPathName"
    }
    Write-Host "Get-MagellanX86Path: $magellanPathName"
    return $magellanPathName
}

function Get-MagellanSdkPath
{
    $magellanSdkPath = Join-Path (Get-MagellanPath) "magellansdk.dll"
    if (-not(Test-Path -Path $magellanSdkPath))
    {
       Write-Error "Cannot find path $magellanSdkPath"
    }
    Write-Host ([string]::Format("Get-MagellanSdkPath: {0}", $magellanSdkPath))

    return $magellanSdkPath
}

function Get-CoverageDataPath
{
    return "$env:SystemDrive\ProgramData\Coverage"
}

function Get-BuildNumber
{
    return [string]::Format("{0}_{1}", $buildNumber, $buildPlatform)
}

function Set-ConnectionString
{
 param(
 [string]$DBConn
 )

    New-Variable -Name magellanDBConn -Value $DBConn -Scope Script
}

function Get-ConnectionString
{
   # return [string]::Format("Server=tcp:PKGESMAGE1.westus2.cloudapp.azure.com,1188;Database={0};User ID=magellan;Password=CC4TheWin!;Trusted_Connection=False;Connection Timeout=30;", (Get-BuildNumber))
   return $magellanDBConn
}

function Invoke-ConfigureCoverMon
{
  Write-Host "Invoke-ConfigureCoverMon"

  $coverageDataPath = Get-CoverageDataPath
  Write-Host "CodeCoverage data will be stored in $coverageDataPath"
  
  $coverCmdExePath = Join-Path (Get-MagellanPath) "CoverCmd.exe"

  # Verbose logging
  Write-Host "Executing command: & $coverCmdExePath /SetVerbose on"
  & $coverCmdExePath /SetVerbose on

  # Tell Magellan where to save the .covdata files
  Write-Host "Executing command: & $coverCmdExePath /SetPath $coverageDataPath"
  & $coverCmdExePath /SetPath "$coverageDataPath"

  Write-Host "Executing command: & $coverCmdExePath /GetPath"
  & $coverCmdExePath /GetPath

  # Close any .covdata files before cleaning up the directory
  Write-Host "Executing command: & $coverCmdExePath /Close"
  & $coverCmdExePath /Close

  # Reconstruct the .covdata directory
  Write-Host "Remove and recreate $coverageDataPath"
  if(Test-Path "$coverageDataPath")
  {
    Remove-Item $coverageDataPath -Recurse -Force
  }
  
  New-Item -Path $coverageDataPath -ItemType Directory -Force

  # Discard any existing coverage data for running binaries that has not been saved
  Write-Host "Executing command: & $coverCmdExePath /Reset"
  & $coverCmdExePath /Reset
}

function Save-CodeCoverageData
{
  param (
     [string]$testSuite
     )

  Write-Host "Save-CodeCoverageData"
  
  $coverageDataPath = Get-CoverageDataPath
  $coverCmdExePath = Join-Path (Get-MagellanPath) "CoverCmd.exe"

  Write-Host "Executing command: & $coverCmdExePath /Save /As '$testSuite'"
  & $coverCmdExePath /Save /As "$testSuite"

  Write-Host "Executing command: & $coverCmdExePath /Close"
  & $coverCmdExePath /Close

  Write-Host "Executing command: & $coverCmdExePath /List /ShowSessionID"
  & $coverCmdExePath /List /ShowSessionID

  # It can take the OS a little while to save the covdata file
  # Without the sleep, /List may show binaries that were instrumented,
  # but Get-ChildItem below won't show anything
  Sleep -s 5

  Write-Host "List .covdata files..."
  Get-ChildItem $coverageDataPath -Filter "*.covdata" -Recurse
}

function Import-CodeCoverageSymbolsToDatabase
{
  param(
    [Parameter(Mandatory=$true, HelpMessage="Path to where the *.covsym files are located.")]
    [ValidateNotNullOrEmpty()]
    [string] $coverageSymPath
  )

  Write-Host "Import-CodeCoverageSymbolsToDatabase"
  
  $coverageDataPath = Get-CoverageDataPath
  $covSymExePath = Join-Path (Get-MagellanPath) "CovSym.exe"

  Write-Host "importing .covsym files to database..."
  $covSymFiles = Get-ChildItem $coverageSymPath -Filter "*.covsym" -Recurse
  $covSymFiles
  $connectionString = Get-ConnectionString
  $covSymFiles | ForEach-Object -Process {
    $fullName = $_.FullName
    Write-Host "Executing command: & $covSymExePath /Read File '$fullName' /Write Database '$connectionString'"
    & $covSymExePath /Read File "$fullName" /Write Database "$connectionString" OpenOrCreate
  }
}

function Import-CodeCoverageDataToDatabase
{
  Write-Host "Import-CodeCoverageDataToDatabase"
  
  $coverageDataPath = Get-CoverageDataPath
  $covDataExePath = Join-Path (Get-MagellanPath) "CovData.exe"

  Write-Host "importing .covdata files to database..."
  $covDataFiles = Get-ChildItem $coverageDataPath -Filter "*.covdata" -Recurse
  $covDataFiles
  $connectionString = Get-ConnectionString
  $covDataFiles | ForEach-Object -Process {
    $fullName = $_.FullName
    Write-Host "Executing command: & $covDataExePath /Read File '$fullName' /Write Database '$connectionString'"
    & $covDataExePath /Read File "$fullName" /Write Database "$connectionString"
  }
}

function Create-CodeCoverageReportFromMagellanDB
{
    param(
        [Parameter(Mandatory=$true, HelpMessage="Path to output the code coverage summary file to.")]
        [ValidateNotNullOrEmpty()]
        [string] $sqlConnectionString,

        [Parameter(Mandatory=$true, HelpMessage="Path to output the code coverage summary file to.")]
        [ValidateNotNullOrEmpty()]
        [string] $coverageSummaryOutputPath,

        [Parameter(Mandatory=$true, HelpMessage="Filename of the Code Coverage Summary XML file.  The default is CodeCoverageSummary.xml.")]
        [ValidateNotNullOrEmpty()]
        [string] $coverageSummaryFileName = "CodeCoverageSummary.xml",

        [Parameter(Mandatory=$true, HelpMessage="Path to output the code coverage report to.")]
        [ValidateNotNullOrEmpty()]
        [string] $coverageReportOutputPath,
        
        [Parameter(Mandatory=$true, HelpMessage="A function that returns true if the directory passed into it should be considered for code coverage.")]
        [ValidateNotNull()]
        $directoryFilterFunction,
		
        [Parameter(Mandatory=$false, HelpMessage="A flag indicating whether or not this is a local run.")]
        [switch]$isLocalRun
    )

    Set-ConnectionString $sqlConnectionString

    Import-Module -Name (Get-MagellanSdkPath) -Verbose

    [MS.Magellan.Reporting.CoverageDatabase] $db = New-Object -TypeName "MS.Magellan.Reporting.CoverageDatabase" -ArgumentList (Get-ConnectionString)
    [MS.Magellan.Reporting.Project] $project = New-Object -TypeName "MS.Magellan.Reporting.Project"
    $project.CoverageDataSources.Add($db)

    Create-CodeCoverageReport `
        -coverageSummaryOutputPath $coverageSummaryOutputPath -coverageSummaryFileName $coverageSummaryFileName `
        -coverageReportOutputPath $coverageReportOutputPath `
        -project $project -directoryFilterFunction $directoryFilterFunction -isLocalRun:$isLocalRun
}

function Create-CodeCoverageReportFromMagellanFiles
{
    param(
        [Parameter(Mandatory=$true, HelpMessage="Path to output the code coverage summary file to.")]
        [ValidateNotNullOrEmpty()]
        [string] $coverageSummaryOutputPath,

        [Parameter(Mandatory=$true, HelpMessage="Filename of the Code Coverage Summary XML file.  The default is CodeCoverageSummary.xml.")]
        [ValidateNotNullOrEmpty()]
        [string] $coverageSummaryFileName = "CodeCoverageSummary.xml",

        [Parameter(Mandatory=$true, HelpMessage="Path to output the code coverage report to.")]
        [ValidateNotNullOrEmpty()]
        [string] $coverageReportOutputPath,

        [Parameter(Mandatory=$true, HelpMessage="Path to where the *.covsym files are located.")]
        [ValidateNotNullOrEmpty()]
        [string] $coverageSymPath,

        [Parameter(Mandatory=$true, HelpMessage="Path to where the *.covdata files are located.")]
        [ValidateNotNullOrEmpty()]
        [string] $coverageDataPath,
        
        [Parameter(Mandatory=$true, HelpMessage="A function that returns true if the directory passed into it should be considered for code coverage.")]
        [ValidateNotNull()]
        $directoryFilterFunction,
		
        [Parameter(Mandatory=$false, HelpMessage="A flag indicating whether or not this is a local run.")]
		[switch]$isLocalRun
    )

    Import-Module -Name (Get-MagellanSdkPath) -Verbose

    [MS.Magellan.Reporting.Project] $project = New-Object -TypeName "MS.Magellan.Reporting.Project"
    $coverageSymPath = "$coverageSymPath\*.covsym"
    [MS.Magellan.Reporting.CoverageSymbolPath] $covSymbols = New-Object -TypeName "MS.Magellan.Reporting.CoverageSymbolPath" -ArgumentList ($coverageSymPath, 1)
    $coverageDataPath = "$coverageDataPath\*.covdata"
    [MS.Magellan.Reporting.CoverageDataPath] $covData = New-Object -TypeName "MS.Magellan.Reporting.CoverageDataPath" -ArgumentList ($coverageDataPath, 1)

    $project.CoverageDataSources.Add($covSymbols)
    $project.CoverageDataSources.Add($covData)

    Create-CodeCoverageReport `
        -coverageSummaryOutputPath $coverageSummaryOutputPath -coverageSummaryFileName $coverageSummaryFileName `
        -coverageReportOutputPath $coverageReportOutputPath `
        -project $project -directoryFilterFunction $directoryFilterFunction -isLocalRun:$isLocalRun
}

function Create-CodeCoverageReport
{
    param(
        [Parameter(Mandatory=$true, HelpMessage="Path to output the code coverage summary file to.")]
        [ValidateNotNullOrEmpty()]
        [string] $coverageSummaryOutputPath,

        [Parameter(Mandatory=$true, HelpMessage="Filename of the Code Coverage Summary XML file.  The default is the CodeCoverageSummary.xml.")]
        [ValidateNotNullOrEmpty()]
        [string] $coverageSummaryFileName = "CodeCoverageSummary.xml",
        
        [Parameter(Mandatory=$true, HelpMessage="The path to the report directory.")]
        [ValidateNotNullOrEmpty()]
        [string] $coverageReportOutputPath,

        [ValidateNotNullOrEmpty()]
        [MS.Magellan.Reporting.Project] $project,
        
        [Parameter(Mandatory=$true, HelpMessage="A function that returns true if the directory passed into it should be considered for code coverage")]
        [ValidateNotNull()]
        $directoryFilterFunction,
		
        [Parameter(Mandatory=$false, HelpMessage="A flag indicating whether or not this is a local run.")]
		[switch]$isLocalRun
    )

    Write-Host "Create-CodeCoverageReport"

    if(-not (Test-Path -Path $coverageSummaryOutputPath -PathType Container))
    {
        Write-Host "The directory $coverageSummaryOutputPath does not exist.  Creating directory..."
        New-Item -ItemType Directory -Path $coverageSummaryOutputPath
    }
    else
    {
        #  The directory exists, so let's clean it up.
        Write-Host "The directory $coverageSummaryOutputPath exists.  Removing all files and folders in that directory..."
        Remove-Item -Path "$coverageSummaryOutputPath\*.*" -Force -Recurse
    }
    
    # Add a filename to the path that was passed in so that we have a fully qualified path + filename.
    $coverageSummaryOutputPath = Join-Path -Path $coverageSummaryOutputPath -ChildPath $coverageSummaryFileName

    if (Test-Path -Path $coverageSummaryOutputPath -PathType Any)
    {
        Write-Host "Removing existing code coverage summary file $coverageSummaryOutputPath"
        Remove-Item -Path $coverageSummaryOutputPath
    }

    New-CoverageReportFromMagellanProject -project $project -coverageSummaryOutputPath $coverageSummaryOutputPath -coverageReportOutputPath $coverageReportOutputPath -directoryFilterFunction $directoryFilterFunction -isLocalRun:$isLocalRun

    Write-Host "Create-CodeCoverageReport complete!"

    #  Return the full path to the file we created.
    return $coverageSummaryOutputPath
}

function Write-CoverageSectionFooter
{
    param(
        [Parameter(Mandatory=$true, HelpMessage="HTML writer")]
        [ValidateNotNullOrEmpty()]
        [System.Xml.XmlWriter] $htmlWriter,

        [Parameter(Mandatory=$true, HelpMessage="Missed blocks")]
        [string] $missedBlocks,

        [Parameter(Mandatory=$true, HelpMessage="Covered blocks")]
        [string] $coveredBlocks,

        [Parameter(Mandatory=$true, HelpMessage="Missed lines")]
        [string] $missedLines,

        [Parameter(Mandatory=$true, HelpMessage="Covered lines")]
        [string] $coveredLines,

        [Parameter(Mandatory=$true, HelpMessage="Missed methods")]
        [string] $missedMethods,

        [Parameter(Mandatory=$true, HelpMessage="Covered methods")]
        [string] $coveredMethods,

        [Parameter(Mandatory=$true, HelpMessage="Missed classes")]
        [string] $missedClasses,

        [Parameter(Mandatory=$true, HelpMessage="Covered classes")]
        [string] $coveredClasses
    )

    $coverageSummaryWriter.WriteStartElement("counter")
    $coverageSummaryWriter.WriteAttributeString("type", "INSTRUCTION")
    $coverageSummaryWriter.WriteAttributeString("missed", $missedBlocks)
    $coverageSummaryWriter.WriteAttributeString("covered", $coveredBlocks)
    $coverageSummaryWriter.WriteEndElement() # counter

    $coverageSummaryWriter.WriteStartElement("counter")
    $coverageSummaryWriter.WriteAttributeString("type", "LINE")
    $coverageSummaryWriter.WriteAttributeString("missed", $missedLines)
    $coverageSummaryWriter.WriteAttributeString("covered", $coveredLines)
    $coverageSummaryWriter.WriteEndElement() # counter

    $coverageSummaryWriter.WriteStartElement("counter")
    $coverageSummaryWriter.WriteAttributeString("type", "METHOD")
    $coverageSummaryWriter.WriteAttributeString("missed", $missedMethods)
    $coverageSummaryWriter.WriteAttributeString("covered", $coveredMethods)
    $coverageSummaryWriter.WriteEndElement() # counter

    $coverageSummaryWriter.WriteStartElement("counter")
    $coverageSummaryWriter.WriteAttributeString("type", "CLASS")
    $coverageSummaryWriter.WriteAttributeString("missed", $missedClasses)
    $coverageSummaryWriter.WriteAttributeString("covered", $coveredClasses)
    $coverageSummaryWriter.WriteEndElement() # counter
}

$paddingPerIndent = 30;

# VSTS doesn't allow us to import CSS files, so we need to do everything in styles.
# We'll define the components of each style here to have the same effect.
$universalStyle = "box-sizing: border-box;-webkit-box-sizing: border-box;-moz-box-sizing: border-box;"

$htmlStyle = "height:100%;"
$bodyStyle = "height:100%;"

$bodyBuildDetailsStyle = "font-family: `"-apple-system`",BlinkMacSystemFont,`"Segoe UI`",Roboto,`"Helvetica Neue`",Helvetica,Ubuntu,Arial,sans-serif,`"Apple Color Emoji`",`"Segoe UI Emoji`",`"Segoe UI Symbol`";color: #212121;background-color: #fff;font-size: 12px;margin: 0;padding: 0;";

$buildDetailsSummarySectionHeaderStyle = "font-size: 18px;color: #333333;border-bottom: 1px solid #f4f4f4;padding: 10px 0 3px 0;"
$buildDetailsSummaryItemHeaderStyle = "color: #333333;font-size: 12px;margin: 5px 0 3px 0;"

$tableStyle = "border-collapse: collapse;border-spacing: 0px;margin: 0;padding: 0;border: 0;margin:20px 0 20px 0;"

$tableCoverageStyle = "empty-cells:show;border-collapse:collapse;"
$tableCoverageTheadStyle = "background-color:#e0e0e0;"
$tableCoverageTheadTdStyle = "white-space:nowrap;padding:2px 14px 0px 6px;border-bottom:#b0b0b0 1px solid;"
$tableCoverageTheadTdBarStyle = "border-left:#cccccc 1px solid;"
$tableCoverageTheadTdCtr1Style = "text-align:right;border-left:#cccccc 1px solid;"
$tableCoverageTheadTdCtr2Style = "text-align:right;padding-left:2px;"

$tableCoverageTbodyTdStyle = "white-space:nowrap;padding:2px 6px 2px 6px;border-bottom:#d6d3ce 1px solid;font-size:12px;"
$tableCoverageTbodyTdDirectoryStyle = "vertical-align:middle;"
$tableCoverageTbodyTdBarStyle = "border-left:#e8e8e8 1px solid;"
$tableCoverageTbodyTdCtr1Style = "text-align:right;padding-right:14px;border-left:#e8e8e8 1px solid;"
$tableCoverageTbodyTdCtr2Style = "text-align:right;padding-right:14px;padding-left:2px;"
$tableCoverageTbodyTdElementNameStyle = "padding-right:14px;padding-left:2px;"
$tableCoverageTbodyTdRectangleStyle = "vertical-align:bottom;padding-bottom:4px;"

$divCoveredRectangleStyle = "height:12px;display:table-cell;background: #83c000;background: -moz-linear-gradient(top, #83c000 0%, #649200 100%);background: -webkit-linear-gradient(top, #83c000 0%,#649200 100%);background: linear-gradient(to bottom, #83c000 0%,#649200 100%);filter: progid:DXImageTransform.Microsoft.gradient( startColorstr='#83c000', endColorstr='#649200',GradientType=0 );"
$divNotCoveredRectangleStyle = "height:12px;display:table-cell;background: #c00023;background: -moz-linear-gradient(top, #c00023 0%, #92001b 100%);background: -webkit-linear-gradient(top, #c00023 0%,#92001b 100%);background: linear-gradient(to bottom, #c00023 0%,#92001b 100%);filter: progid:DXImageTransform.Microsoft.gradient( startColorstr='#c00023', endColorstr='#92001b',GradientType=0 );"

$tableCoverageTfootTdStyle = "white-space:nowrap;padding:2px 6px 2px 6px;font-size:12px;"
$tableCoverageTfootTdBarStyle = "border-left:#e8e8e8 1px solid;"
$tableCoverageTfootTdCtr1Style = "text-align:right;padding-right:14px;border-left:#e8e8e8 1px solid;"
$tableCoverageTfootTdCtr2Style = "text-align:right;padding-right:14px;padding-left:2px;"

$tableLegendStyle = "empty-cells:show;"
$tableLegendTheadTdStyle = "padding:0 0 5px 0;"
$tableLegendTbodyStyle = "border:#e8e8e8 1px solid;font-size:12px;"
$tableLegendTdStyle = "border-bottom:#e8e8e8 1px solid;"
$tableLegendTdColorStyle = "width:25px;height:25px;"
$tableLegendTdFcStyle = "background-color:#ccffcc;"
$tableLegendTdPcStyle = "background-color:#ffffcc;"
$tableLegendTdNcStyle = "background-color:#ffaaaa;"
$tableLegendTdExplanationStyle = "padding:0 10px 0 10px;vertical-align:middle;"

$olSourceStyle = "border:#d6d3ce 1px solid;margin-bottom: 0px;margin-top:0px;font-family:monospace;"
$olSourceLiStyle = "border-left: 1px solid #D6D3CE;color: #A0A0A0;padding-left: 0px;list-style-type: decimal !important;"
$olSourceLiEvenNumberStyle = "background: #EEEEEE;"
$olSourceSpanStyle = "color: #212121;"
$olSourceSpanFcStyle = "background-color:#ccffcc;"
$olSourceSpanPcStyle = "background-color:#ffffcc;"
$olSourceSpanNcStyle = "background-color:#ffaaaa;"

function Write-HTMLHeader
{
    param(
        [Parameter(Mandatory=$true, HelpMessage="HTML writer")]
        [ValidateNotNullOrEmpty()]
        [System.Xml.XmlWriter] $htmlWriter,
        
        [Parameter(Mandatory=$true, HelpMessage="Path to resources")]
        [ValidateNotNull()]
        [AllowEmptyString()]
        [string] $resourcePath,
		
        [Parameter(Mandatory=$false, HelpMessage="A flag indicating whether or not this is a local run.")]
		[switch] $isLocalRun,
        
        [Parameter(Mandatory=$false, HelpMessage="The binary name we're writing about (null if none)")]
        [string] $binaryName = $null,
        
        [Parameter(Mandatory=$false, HelpMessage="The source file name we're writing about (null if none)")]
        [string] $sourceFileName = $null
    )

    $htmlWriter.WriteStartDocument()

    $htmlWriter.WriteStartElement("html")
    $htmlWriter.WriteAttributeString("lang", "en")
    $htmlWriter.WriteAttributeString("style", "$universalStyle$htmlStyle")
    
    $htmlWriter.WriteStartElement("head")

    $htmlWriter.WriteStartElement("meta")
    $htmlWriter.WriteAttributeString("http-equiv", "Content-Type")
    $htmlWriter.WriteAttributeString("content", "text/html;charset=UTF-8")
    $htmlWriter.WriteEndElement() # meta

    $title = "Code Coverage Report"

    if ($binaryName -ne $null -and $binaryName.Length -gt 0)
    {
        $title = "$title - $binaryName"
    }

    if ($sourceFileName -ne $null -and $sourceFileName.Length -gt 0)
    {
        $title = "$title - $sourceFileName"
    }

    $htmlWriter.WriteStartElement("title")
    $htmlWriter.WriteString($title)
    $htmlWriter.WriteEndElement() # title

    $htmlWriter.WriteEndElement() # head
    
    $htmlWriter.WriteStartElement("body")

	$styleString = "$universalStyle$bodyStyle$($bodyBuildDetailsStyle)overflow:scroll;"

	# We have no external padding applied to a local run, so let's add some in that circumstance.
	if ($isLocalRun)
	{
		$styleString = "$($styleString)margin-left: 20px;margin-right: 10px;"
	}

    $htmlWriter.WriteAttributeString("style", $styleString)

    $htmlWriter.WriteStartElement("div")
    $htmlWriter.WriteAttributeString("style", "$universalStyle$buildDetailsSummaryItemHeaderStyle")

    if (($binaryName -ne $null -and $binaryName.Length -gt 0) -or ($sourceFileName -ne $null -and $binaryName.Length -gt 0))
    {
        $htmlWriter.WriteStartElement("a")
        $htmlWriter.WriteAttributeString("style", "$universalStyle")
        $htmlWriter.WriteAttributeString("href", "../index.html")
        $htmlWriter.WriteString("Index")
        $htmlWriter.WriteEndElement() # a
    }
    
    if ($binaryName -ne $null -and $binaryName.Length -gt 0)
    {
        $htmlWriter.WriteString(" > ")

        if ($sourceFileName -ne $null -and $sourceFileName.Length -gt 0)
        {
            $htmlWriter.WriteStartElement("a")
            $htmlWriter.WriteAttributeString("style", "$universalStyle")
            $htmlWriter.WriteAttributeString("href", "index.html")
            $htmlWriter.WriteString($binaryName)
            $htmlWriter.WriteEndElement() # a
        }
        else
        {
            $htmlWriter.WriteString($binaryName)
        }
    
        if ($sourceFileName -ne $null -and $sourceFileName.Length -gt 0)
        {
            $htmlWriter.WriteString(" > $sourceFileName")
        }
    }

    $htmlWriter.WriteEndElement() # div

    [string]$headerText = $null
    
    if ($sourceFileName -ne $null -and $sourceFileName.Length -gt 0)
    {
        $headerText = $sourceFileName
    }
    elseif ($binaryName -ne $null -and $binaryName.Length -gt 0)
    {
        $headerText = $binaryName
    }
    else
    {
        $headerText = "Code Coverage"
    }
    
    $htmlWriter.WriteStartElement("div")
    $htmlWriter.WriteAttributeString("style", "$universalStyle$buildDetailsSummarySectionHeaderStyle")
    $htmlWriter.WriteString($headerText)
    $htmlWriter.WriteEndElement() # div
}

function Write-TableHeader
{
    param(
        [Parameter(Mandatory=$true, HelpMessage="HTML writer")]
        [ValidateNotNullOrEmpty()]
        [System.Xml.XmlWriter] $htmlWriter
    )

    $htmlWriter.WriteStartElement("table")
    $htmlWriter.WriteAttributeString("style", "$universalStyle$tableStyle$tableCoverageStyle")
    $htmlWriter.WriteAttributeString("cellspacing", "0")

    $htmlWriter.WriteStartElement("thead")
    $htmlWriter.WriteAttributeString("style", "$universalStyle$tableCoverageTheadStyle")
    $htmlWriter.WriteStartElement("tr")

    $htmlWriter.WriteStartElement("td")
    $htmlWriter.WriteAttributeString("style", "$universalStyle$tableCoverageTheadTdStyle$buildDetailsSummaryItemHeaderStyle")
    $htmlWriter.WriteString("Binaries")
    $htmlWriter.WriteEndElement() # td

    $htmlWriter.WriteStartElement("td")
    $htmlWriter.WriteAttributeString("style", "$universalStyle$tableCoverageTheadTdStyle$tableCoverageTheadTdBarStyle$buildDetailsSummaryItemHeaderStyle")
    $htmlWriter.WriteString("Instructions Covered")
    $htmlWriter.WriteEndElement() # td

    $htmlWriter.WriteStartElement("td")
    $htmlWriter.WriteAttributeString("style", "$universalStyle$tableCoverageTheadTdStyle")
    $htmlWriter.WriteString("")
    $htmlWriter.WriteEndElement() # td

    $htmlWriter.WriteStartElement("td")
    $htmlWriter.WriteAttributeString("style", "$universalStyle$tableCoverageTheadTdStyle$tableCoverageTheadTdBarStyle$buildDetailsSummaryItemHeaderStyle")
    $htmlWriter.WriteString("Lines Covered")
    $htmlWriter.WriteEndElement() # td

    $htmlWriter.WriteStartElement("td")
    $htmlWriter.WriteAttributeString("style", "$universalStyle$tableCoverageTheadTdStyle$tableCoverageTheadTdBarStyle$buildDetailsSummaryItemHeaderStyle")
    $htmlWriter.WriteString("Methods Covered")
    $htmlWriter.WriteEndElement() # td

    $htmlWriter.WriteStartElement("td")
    $htmlWriter.WriteAttributeString("style", "$universalStyle$tableCoverageTheadTdStyle$tableCoverageTheadTdBarStyle$buildDetailsSummaryItemHeaderStyle")
    $htmlWriter.WriteString("Classes Covered")
    $htmlWriter.WriteEndElement() # td

    $htmlWriter.WriteEndElement() # tr
    $htmlWriter.WriteEndElement() # thead

    $htmlWriter.WriteStartElement("tbody")
    $htmlWriter.WriteAttributeString("style", "$universalStyle")
}

function Write-DirectoryRow
{
    param(
        [Parameter(Mandatory=$true, HelpMessage="HTML writer")]
        [ValidateNotNullOrEmpty()]
        [System.Xml.XmlWriter] $htmlWriter,
        
        [Parameter(Mandatory=$true, HelpMessage="Path to resources")]
        [ValidateNotNull()]
        [AllowEmptyString()]
        [string] $resourcePath,

        [Parameter(Mandatory=$true, HelpMessage="Directory that we're indenting for")]
        [string] $directoryName,

        [Parameter(Mandatory=$true, HelpMessage="Number of indents to make")]
        [int] $indentLevel
    )

    $directoryId = $directoryName.Replace("\", "")

    $htmlWriter.WriteStartElement("tr")
    $htmlWriter.WriteAttributeString("style", "$universalStyle")

    $htmlWriter.WriteStartElement("td")
    $htmlWriter.WriteAttributeString("colspan", "6")
    $htmlWriter.WriteAttributeString("style", "$universalStyle$($tableCoverageTbodyTdStyle)$($tableCoverageTbodyTdDirectoryStyle)padding-left:$(2 + $paddingPerIndent * $indentLevel)px")
    $htmlWriter.WriteString($directoryName)
    $htmlWriter.WriteEndElement() # td

    $htmlWriter.WriteEndElement() # tr
}


function Write-CoverageRow
{
    param(
        [Parameter(Mandatory=$true, HelpMessage="HTML writer")]
        [ValidateNotNullOrEmpty()]
        [System.Xml.XmlWriter] $htmlWriter,

        [Parameter(Mandatory=$true, HelpMessage="Name of the element we're reporting coverage data of")]
        [string] $elementName,

        [Parameter(Mandatory=$true, HelpMessage="URL of the element we're reporting coverage data of")]
        [string] $elementUrl,

        [Parameter(Mandatory=$true, HelpMessage="Covered blocks")]
        [string] $coveredBlocks,

        [Parameter(Mandatory=$true, HelpMessage="Total blocks")]
        [string] $totalBlocks,

        [Parameter(Mandatory=$true, HelpMessage="Maximum total blocks across all files")]
        [string] $maxTotalBlocks,

        [Parameter(Mandatory=$true, HelpMessage="Covered lines")]
        [string] $coveredLines,

        [Parameter(Mandatory=$true, HelpMessage="Total lines")]
        [string] $totalLines,

        [Parameter(Mandatory=$true, HelpMessage="Covered methods")]
        [string] $coveredMethods,

        [Parameter(Mandatory=$true, HelpMessage="Total methods")]
        [string] $totalMethods,

        [Parameter(Mandatory=$true, HelpMessage="Covered classes")]
        [string] $coveredClasses,

        [Parameter(Mandatory=$true, HelpMessage="Total classes")]
        [string] $totalClasses,

        [Parameter(Mandatory=$true, HelpMessage="Number of indents to make")]
        [int] $indentLevel
    )
    
    $htmlWriter.WriteStartElement("tr")
    $htmlWriter.WriteAttributeString("style", "$universalStyle")

    $htmlWriter.WriteStartElement("td")
    $htmlWriter.WriteAttributeString("style", "$universalStyle$tableCoverageTbodyTdStyle$($tableCoverageTbodyTdElementNameStyle)padding-left:$(2 + $paddingPerIndent * $indentLevel)px")

    $htmlWriter.WriteStartElement("a")
    $htmlWriter.WriteAttributeString("style", "$universalStyle")
    $htmlWriter.WriteAttributeString("href", $elementUrl)
    $htmlWriter.WriteString($elementName)
    $htmlWriter.WriteEndElement() # a
    $htmlWriter.WriteEndElement() # td

    $coveredPercentage = $coveredBlocks / $totalBlocks
    $totalRectangleWidth = 120 * $totalBlocks / $maxTotalBlocks

    $htmlWriter.WriteStartElement("td")
    $htmlWriter.WriteAttributeString("style", "$universalStyle$tableCoverageTbodyTdStyle$tableCoverageTbodyTdBarStyle$tableCoverageTbodyTdRectangleStyle")

    $htmlWriter.WriteStartElement("div")
    $htmlWriter.WriteAttributeString("style", "$universalStyle$($divNotCoveredRectangleStyle)width:$((1 - $coveredPercentage) * $totalRectangleWidth)px")
    $htmlWriter.WriteString("")
    $htmlWriter.WriteEndElement() # div

    $htmlWriter.WriteStartElement("div")
    $htmlWriter.WriteAttributeString("style", "$universalStyle$($divCoveredRectangleStyle)width:$($coveredPercentage * $totalRectangleWidth)px")
    $htmlWriter.WriteString("")
    $htmlWriter.WriteEndElement() # div

    $htmlWriter.WriteEndElement() # td

    if ($totalBlocks -gt 0)
    {
        $percentageString = "$([math]::Round(100 * $coveredBlocks / $totalBlocks))%"
    }
    else
    {
        $percentageString = "N/A%"
    }

    $htmlWriter.WriteStartElement("td")
    $htmlWriter.WriteAttributeString("style", "$universalStyle$tableCoverageTbodyTdStyle")
    $htmlWriter.WriteString($percentageString)
    $htmlWriter.WriteEndElement() # td

    $htmlWriter.WriteStartElement("td")
    $htmlWriter.WriteAttributeString("style", "$universalStyle$tableCoverageTbodyTdStyle$tableCoverageTbodyTdCtr1Style")
    $htmlWriter.WriteString("$coveredLines of $totalLines")
    $htmlWriter.WriteEndElement() # td

    $htmlWriter.WriteStartElement("td")
    $htmlWriter.WriteAttributeString("style", "$universalStyle$tableCoverageTbodyTdStyle$tableCoverageTbodyTdCtr1Style")
    $htmlWriter.WriteString("$coveredMethods of $totalMethods")
    $htmlWriter.WriteEndElement() # td

    $htmlWriter.WriteStartElement("td")
    $htmlWriter.WriteAttributeString("style", "$universalStyle$tableCoverageTbodyTdStyle$tableCoverageTbodyTdCtr1Style")
    $htmlWriter.WriteString("$coveredClasses of $totalClasses")
    $htmlWriter.WriteEndElement() # td

    $htmlWriter.WriteEndElement() # tr
}

function Write-TableFooter
{
    param(
        [Parameter(Mandatory=$true, HelpMessage="HTML writer")]
        [ValidateNotNullOrEmpty()]
        [System.Xml.XmlWriter] $htmlWriter,

        [Parameter(Mandatory=$true, HelpMessage="Covered blocks")]
        [string] $coveredBlocks,

        [Parameter(Mandatory=$true, HelpMessage="Total blocks")]
        [string] $totalBlocks,

        [Parameter(Mandatory=$true, HelpMessage="Covered lines")]
        [string] $coveredLines,

        [Parameter(Mandatory=$true, HelpMessage="Total lines")]
        [string] $totalLines,

        [Parameter(Mandatory=$true, HelpMessage="Covered methods")]
        [string] $coveredMethods,

        [Parameter(Mandatory=$true, HelpMessage="Total methods")]
        [string] $totalMethods,

        [Parameter(Mandatory=$true, HelpMessage="Covered classes")]
        [string] $coveredClasses,

        [Parameter(Mandatory=$true, HelpMessage="Total classes")]
        [string] $totalClasses
    )

    $htmlWriter.WriteEndElement() # tbody

    $htmlWriter.WriteStartElement("tfoot")
    $htmlWriter.WriteAttributeString("style", "$universalStyle")
    $htmlWriter.WriteStartElement("tr")
    $htmlWriter.WriteAttributeString("style", "$universalStyle")

    $htmlWriter.WriteStartElement("td")
    $htmlWriter.WriteAttributeString("style", "$universalStyle$tableCoverageTfootTdStyle$tableCoverageTfootTdCtr2Style")
    $htmlWriter.WriteString("Total")
    $htmlWriter.WriteEndElement() # td

    $htmlWriter.WriteStartElement("td")
    $htmlWriter.WriteAttributeString("style", "$universalStyle$tableCoverageTfootTdStyle$tableCoverageTfootTdBarStyle")
    $htmlWriter.WriteString("$coveredBlocks of $totalBlocks")
    $htmlWriter.WriteEndElement() # td

    if ($totalBlocks -gt 0)
    {
        $percentageString = "$([math]::Round(100 * $coveredBlocks / $totalBlocks))%"
    }
    else
    {
        $percentageString = "N/A%"
    }

    $htmlWriter.WriteStartElement("td")
    $htmlWriter.WriteAttributeString("style", "$universalStyle$tableCoverageTfootTdStyle")
    $htmlWriter.WriteString($percentageString)
    $htmlWriter.WriteEndElement() # td

    $htmlWriter.WriteStartElement("td")
    $htmlWriter.WriteAttributeString("style", "$universalStyle$tableCoverageTfootTdStyle$tableCoverageTfootTdCtr1Style")
    $htmlWriter.WriteString("$coveredLines of $totalLines")
    $htmlWriter.WriteEndElement() # td

    $htmlWriter.WriteStartElement("td")
    $htmlWriter.WriteAttributeString("style", "$universalStyle$tableCoverageTfootTdStyle$tableCoverageTfootTdCtr1Style")
    $htmlWriter.WriteString("$coveredMethods of $totalMethods")
    $htmlWriter.WriteEndElement() # td

    $htmlWriter.WriteStartElement("td")
    $htmlWriter.WriteAttributeString("style", "$universalStyle$tableCoverageTfootTdStyle$tableCoverageTfootTdCtr1Style")
    $htmlWriter.WriteString("$coveredClasses of $totalClasses")
    $htmlWriter.WriteEndElement() # td

    $htmlWriter.WriteEndElement() # tr
    $htmlWriter.WriteEndElement() # tfoot
    $htmlWriter.WriteEndElement() # table
}

function Write-HTMLFooter
{
    param(
        [Parameter(Mandatory=$true, HelpMessage="HTML writer")]
        [ValidateNotNullOrEmpty()]
        [System.Xml.XmlWriter] $htmlWriter
    )
    
    $htmlWriter.WriteEndElement() # body

    $htmlWriter.Flush()
    $htmlWriter.Close()
}

function New-CoverageReportFromMagellanProject
{
    param(
        [Parameter(Mandatory=$true, HelpMessage="Magellan project with coverage data")]
        [ValidateNotNullOrEmpty()]
        [MS.Magellan.Reporting.Project] $project,
        
        [Parameter(Mandatory=$true, HelpMessage="The path to the summary file")]
        [ValidateNotNullOrEmpty()]
        [string] $coverageSummaryOutputPath,
        
        [Parameter(Mandatory=$true, HelpMessage="The path to the report directory")]
        [ValidateNotNullOrEmpty()]
        [string] $coverageReportOutputPath,
        
        [Parameter(Mandatory=$true, HelpMessage="A function that returns true if the directory passed into it should be considered for code coverage")]
        [ValidateNotNull()]
        $directoryFilterFunction,
		
        [Parameter(Mandatory=$false, HelpMessage="A flag indicating whether or not this is a local run.")]
		[switch]$isLocalRun
    )
            
    # Create the report output directory if it doesn't exist, or clean it up if it does.
    if(-not (Test-Path -Path $coverageReportOutputPath -PathType Container))
    {
        Write-Host "The directory $coverageReportOutputPath does not exist.  Creating directory..."
        New-Item -ItemType Directory -Path $coverageReportOutputPath
    }
    else
    {
        Write-Host "The directory $coverageReportOutputPath exists.  Removing all files and folders in that directory..."
        Remove-Item -Path "$coverageReportOutputPath\*.*" -Force -Recurse
    }

    [System.Xml.XmlWriterSettings] $xmlWriterSettings = New-Object System.Xml.XmlWriterSettings
    $xmlWriterSettings.Indent = $true

    [System.Xml.XmlWriter] $coverageSummaryWriter = [System.Xml.XmlWriter]::Create($coverageSummaryOutputPath, $xmlWriterSettings)
    [System.Xml.XmlWriter] $indexHtmlWriter = [System.Xml.XmlWriter]::Create("$coverageReportOutputPath\index.html", $xmlWriterSettings)

    Write-HTMLHeader -htmlWriter $indexHtmlWriter -resourcePath "" -isLocalRun:$isLocalRun
    Write-TableHeader -htmlWriter $indexHtmlWriter

    $totalInstructionsCovered = 0
    $totalInstructionsTotal = 0
    $totalLinesCovered = 0
    $totalLinesTotal = 0
    $totalMethodsCovered = 0
    $totalMethodsTotal = 0
    $totalClassesCovered = 0
    $totalClassesTotal = 0

    $coverageSummaryWriter.WriteStartDocument()
    $coverageSummaryWriter.WriteStartElement("report")
    $coverageSummaryWriter.WriteAttributeString("name", "JaCoCo")
            
    $coverageSummaryWriter.WriteStartElement("group")
    $coverageSummaryWriter.WriteAttributeString("name", "Microsoft")

    $binaryCoverageDataList = New-Object 'System.Collections.Generic.List[CodeCoverage.BinaryCoverageData]'

    [MS.Magellan.Reporting.BuildCollection] $builds = $project.GetBuilds()

    [MS.Magellan.Reporting.Build] $build = $null
    foreach ($build in $builds)
    {
        [MS.Magellan.Reporting.BinaryFileCollection] $binaries = $build.GetBinaries()

        [MS.Magellan.Reporting.BinaryFile] $binary = $null
        foreach ($binary in $binaries)
        {
            $binaryInstructionsCovered = 0
            $binaryInstructionsTotal = 0
            $binaryLinesCovered = 0
            $binaryLinesTotal = 0
            $binaryMethodsCovered = 0
            $binaryMethodsTotal = 0
            $binaryClassesCovered = 0
            $binaryClassesTotal = 0
            
            # We'll now prep for outputting the code coverage report for this binary.
            $binaryReportOutputPath = "$coverageReportOutputPath\$($binary.Name)"
            New-Item -ItemType Directory -Path $binaryReportOutputPath

            # We'll also start our HTML page for the index of this binary.
            [System.Xml.XmlWriter] $binaryIndexHtmlWriter = [System.Xml.XmlWriter]::Create("$binaryReportOutputPath\index.html", $xmlWriterSettings)

            Write-HTMLHeader -htmlWriter $binaryIndexHtmlWriter -resourcePath "../" -isLocalRun:$isLocalRun -binaryName $binary.Name
            Write-TableHeader -htmlWriter $binaryIndexHtmlWriter

            $coverageSummaryWriter.WriteStartElement("package")
            $coverageSummaryWriter.WriteAttributeString("name", $binary.Name)

            $sourceFileToCoverageData = New-Object 'System.Collections.Generic.Dictionary[MS.Magellan.Reporting.SourceFile, CodeCoverage.SourceFileCoverageData]'

            [MS.Magellan.Reporting.ClassCollection] $classes = $binary.GetClasses()

            [MS.Magellan.Reporting.Class]$cls = $null
            foreach ($cls in $classes)
            {
                $className = ""

                if ($cls.Namespace.Length -gt 0)
                {
                    $className = "$($cls.Namespace).$($cls.Name)"
                }
                else
                {
                    $className = $($cls.Name)
                }

                $coverageSummaryWriter.WriteStartElement("class")
                $coverageSummaryWriter.WriteAttributeString("name", $className)
                        
                $classLinesCovered = 0
                $classLinesTotal = 0

                $methodsCovered = 0

                [MS.Magellan.Reporting.FunctionCollection] $functions = $cls.GetFunctions()

                [MS.Magellan.Reporting.Function]$func = $null
                foreach ($func in $functions)
                {
                    if (-not $directoryFilterFunction.Invoke($func.SourceFile.Directory))
                    {
                        continue
                    }

                    [CodeCoverage.SourceFileCoverageData] $sourceFileCoverageData = $null
                    if (-not $sourceFileToCoverageData.TryGetValue($func.SourceFile, [ref]$sourceFileCoverageData))
                    {
                        $sourceFileCoverageData = New-Object 'CodeCoverage.SourceFileCoverageData'
                        $sourceFileToCoverageData.Add($func.SourceFile, $sourceFileCoverageData)
                    }

                    $coverageSummaryWriter.WriteStartElement("method")
                    $coverageSummaryWriter.WriteAttributeString("name", $func.Name)
                    $coverageSummaryWriter.WriteAttributeString("line", $func.StartSourceLineNumber)

                    $coverageSummaryWriter.WriteStartElement("counter")
                    $coverageSummaryWriter.WriteAttributeString("type", "INSTRUCTION")
                    $coverageSummaryWriter.WriteAttributeString("missed", $func.MissedBlockCount)
                    $coverageSummaryWriter.WriteAttributeString("covered", $func.HitBlockCount)
                    $coverageSummaryWriter.WriteEndElement() # counter

                    $linesCovered = 0
                    
                    [MS.Magellan.Reporting.SourceLineCollection] $lines = $func.GetSourceLines()

                    [MS.Magellan.Reporting.SourceLine]$line = $null
                    foreach ($line in $lines)
                    {
                        if ($line.LineCoverage -eq [MS.Magellan.Reporting.SourceLineCoverage]::Covered -or $line.LineCoverage -eq [MS.Magellan.Reporting.SourceLineCoverage]::PartiallyCovered)
                        {
                            $linesCovered++
                        }

                        $blocksCovered = 0
                        
                        [MS.Magellan.Reporting.Block]$block = $null
                        foreach ($block in $line.Blocks)
                        {
                            if ($block.IsCovered)
                            {
                                $blocksCovered++
                            }
                        }

                        [CodeCoverage.SourceLineCoverageData] $lineCoverageData = $null
                        if (-not $sourceFileCoverageData.LineNumberToCoverageData.TryGetValue($line.Number, [ref]$lineCoverageData))
                        {
                            $lineCoverageData = New-Object 'CodeCoverage.SourceLineCoverageData' -ArgumentList $blocksCovered, $line.Blocks.Count
                            $sourceFileCoverageData.LineNumberToCoverageData.Add($line.Number, $lineCoverageData)
                        }
                        else
                        {
                            $lineCoverageData.CoveredBlocks += $blocksCovered
                            $lineCoverageData.TotalBlocks += $line.Blocks.Count
                        }
                    }
                            
                    $coverageSummaryWriter.WriteStartElement("counter")
                    $coverageSummaryWriter.WriteAttributeString("type", "LINE")
                    $coverageSummaryWriter.WriteAttributeString("missed", $func.GetSourceLines().Count - $linesCovered)
                    $coverageSummaryWriter.WriteAttributeString("covered", $linesCovered)
                    $coverageSummaryWriter.WriteEndElement() # counter
                    
                    $coverageSummaryWriter.WriteStartElement("counter")
                    $coverageSummaryWriter.WriteAttributeString("type", "METHOD")

                    if ($func.HitBlockCount -gt 0)
                    {
                        $coverageSummaryWriter.WriteAttributeString("missed", 0)
                        $coverageSummaryWriter.WriteAttributeString("covered", 1)
                        $methodsCovered++

                        if (-not $sourceFileCoverageData.CoveredMethodNames.Contains($func.Name))
                        {
                            $sourceFileCoverageData.CoveredMethodNames.Add($func.Name)
                        }

                        if (-not $sourceFileCoverageData.CoveredClassNames.Contains($cls.Name))
                        {
                            $sourceFileCoverageData.CoveredClassNames.Add($cls.Name)
                        }
                    }
                    else
                    {
                        $coverageSummaryWriter.WriteAttributeString("missed", 1)
                        $coverageSummaryWriter.WriteAttributeString("covered", 0)
                    }

                    $coverageSummaryWriter.WriteEndElement() # counter

                    if (-not $sourceFileCoverageData.TotalMethodNames.Contains($func.Name))
                    {
                        $sourceFileCoverageData.TotalMethodNames.Add($func.Name)
                    }

                    if (-not $sourceFileCoverageData.TotalClassNames.Contains($cls.Name))
                    {
                        $sourceFileCoverageData.TotalClassNames.Add($cls.Name)
                    }

                    $classLinesCovered += $linesCovered
                    $classLinesTotal += $func.GetSourceLines().Count

                    $coverageSummaryWriter.WriteEndElement() # method
                }

                $classesMissed = 0
                $classesCovered = 0

                if ($cls.HitBlockCount -gt 0)
                {
                    $classesCovered = 1
                }
                else
                {
                    $classesMissed = 1
                }

                Write-CoverageSectionFooter -htmlWriter $coverageSummaryWriter `
                    -missedBlocks $cls.MissedBlockCount -coveredBlocks $cls.HitBlockCount `
                    -missedLines ($classLinesTotal - $classLinesCovered) -coveredLines $classLinesCovered `
                    -missedMethods ($cls.GetFunctions().Count - $methodsCovered) -coveredMethods $methodsCovered `
                    -missedClasses $classesMissed -coveredClasses $classesCovered

                $coverageSummaryWriter.WriteEndElement() # class
            }

            # First we need to do some processing on the files - we need to figure out what the maximum number of total blocks
            # are in the files so we know how large to draw the coverage rectangles, and we need to figure out what the root directory
            # of the source files is so we can organize the source files by directory.
            # We'll figure out the root directory first, because we want the max number of blocks to be per leaf directory.
            $gotInitialValue = $false
            $rootDirectory = ""

            [MS.Magellan.Reporting.SourceFile]$sourceFile
            foreach ($sourceFile in $sourceFileToCoverageData.Keys)
            {
                $sourceFilePath = "$($sourceFile.Directory)$($sourceFile.Name)"

                # If for whatever reason this source file doesn't exist, we can't do anything with it,
                # so we'll just skip it.
                if (-not (Test-Path $sourceFilePath))
                {
                    continue
                }

                if (-not $gotInitialValue)
                {
                    $rootDirectory = $sourceFile.Directory
                    $gotInitialValue = $true
                }

                $rootDirectoryInCommon = ""

                for ($i = 0; $i -lt $rootDirectory.Length -and $i -lt $sourceFile.Directory.Length; $i++)
                {
                    if ($rootDirectory[$i] -eq $sourceFile.Directory[$i])
                    {
                        $rootDirectoryInCommon += $rootDirectory[$i]
                    }
                    else
                    {
                        break
                    }
                }
                
                $rootDirectory = $rootDirectoryInCommon
            }
            
            $sourceFileToDirectoryDictionary = New-Object 'System.Collections.Generic.Dictionary[MS.Magellan.Reporting.SourceFile, string]'
            $sourceFilePathToFileDictionary = New-Object 'System.Collections.Generic.Dictionary[string, MS.Magellan.Reporting.SourceFile]'

            foreach ($sourceFile in $sourceFileToCoverageData.Keys)
            {
                if ($rootDirectory.Length -gt 0)
                {
                    $subDirectory = $sourceFile.Directory.Replace($rootDirectory, "")
                }
                else
                {
                    $subDirectory = $sourceFile.Directory
                }

                $sourceFileToDirectoryDictionary.Add($sourceFile, $subDirectory)
                $sourceFilePathToFileDictionary.Add("$($subDirectory)$($sourceFile.Name)", $sourceFile)
            }

            $sourceFileDirectoryToMaxTotalBlocksDictionary = New-Object 'System.Collections.Generic.Dictionary[string, int]'

            foreach ($sourceFile in $sourceFileToDirectoryDictionary.Keys)
            {
                $sourceFileDirectory = $sourceFileToDirectoryDictionary[$sourceFile]
                [CodeCoverage.SourceFileCoverageData]$coverageData = $sourceFileToCoverageData[$sourceFile]

                $maxTotalBlocks = 0
                $totalBlocks = 0

                foreach ($lineNumber in $coverageData.LineNumberToCoverageData.Keys)
                {
                    [CodeCoverage.SourceLineCoverageData]$lineCoverageData = $coverageData.LineNumberToCoverageData[$lineNumber]
                    $totalBlocks += $lineCoverageData.TotalBlocks
                }

                if (-not $sourceFileDirectoryToMaxTotalBlocksDictionary.TryGetValue($sourceFileDirectory, [ref]$maxTotalBlocks) -or $maxTotalBlocks -lt $totalBlocks)
                {
                    $sourceFileDirectoryToMaxTotalBlocksDictionary[$sourceFileDirectory] = $totalBlocks
                }
            }
            
            $sourceFilePathList = New-Object 'System.Collections.Generic.List[string]'

            foreach ($sourceFilePath in $sourceFilePathToFileDictionary.Keys)
            {
                $sourceFilePathList.Add($sourceFilePath)
            }

            $sourceFilePathList = $sourceFilePathList | Sort-Object

            $commonDirectoryList = New-Object 'System.Collections.Generic.List[string]'
            $indentLevel = 0

            foreach ($sourceFilePath in $sourceFilePathList)
            {
                $fullSourceFilePath = "$rootDirectory$sourceFilePath"

                "Checking for the existence of $fullSourceFilePath..."

                # If for whatever reason this source file doesn't exist, we can't do anything with it,
                # so we'll just skip it.
                if (-not (Test-Path $fullSourceFilePath))
                {
                    "$fullSourceFilePath does not exist. Skipping."
                    continue
                }

                "Processing $fullSourceFilePath..."

                $directoryList = New-Object 'System.Collections.Generic.List[string]'
                $sourceFilePath.Split("\") | ForEach-Object { $directoryList.Add($_) }
                $directoryList.Remove($directoryList[$directoryList.Count - 1]) 2>&1> $null

                $foldersInCommon = 0

                for ($i = 0; $i -lt $commonDirectoryList.Count -and $i -lt $directoryList.Count; $i++)
                {
                    if ($commonDirectoryList[$i] -eq $directoryList[$i])
                    {
                        $foldersInCommon++
                    }
                    else
                    {
                        break
                    }
                }

                while ($commonDirectoryList.Count -gt $foldersInCommon)
                {
                    $commonDirectoryList.RemoveAt($commonDirectoryList.Count - 1)
                    $indentLevel--
                }

                while ($commonDirectoryList.Count -lt $directoryList.Count)
                {
                    $newCommonDirectory = $directoryList[$commonDirectoryList.Count]
                    $commonDirectoryList.Add($newCommonDirectory)
                    Write-DirectoryRow -htmlWriter $binaryIndexHtmlWriter -resourcePath "../" -directoryName $newCommonDirectory -indentLevel $indentLevel
                    $indentLevel++
                }

                $sourceFile = $sourceFilePathToFileDictionary[$sourceFilePath]

                # We'll want to output an HTML page for every file as well.
                [System.Xml.XmlWriter] $sourceFileHtmlWriter = [System.Xml.XmlWriter]::Create("$binaryReportOutputPath\$($sourceFile.Name).html", $xmlWriterSettings)

                Write-HTMLHeader -htmlWriter $sourceFileHtmlWriter -resourcePath "../" -isLocalRun:$isLocalRun -binaryName $binary.Name -sourceFileName $sourceFile.Name

                [CodeCoverage.SourceFileCoverageData]$coverageData = $sourceFileToCoverageData[$sourceFile]

                $coverageSummaryWriter.WriteStartElement("sourcefile")
                $coverageSummaryWriter.WriteAttributeString("name", $sourceFile.Name)

                $coveredBlocks = 0
                $totalBlocks = 0

                $coveredLines = 0
            
                $lineNumberList = New-Object 'System.Collections.Generic.List[int]'

                foreach ($key in $coverageData.LineNumberToCoverageData.Keys)
                {
                    $lineNumberList.Add($key)
                }

                $lineNumberList = $lineNumberList | Sort-Object

                foreach ($lineNumber in $lineNumberList)
                {
                    [CodeCoverage.SourceLineCoverageData]$lineCoverageData = $coverageData.LineNumberToCoverageData[$lineNumber]

                    $coveredBlocks += $lineCoverageData.CoveredBlocks
                    $totalBlocks += $lineCoverageData.TotalBlocks

                    $coverageSummaryWriter.WriteStartElement("line")
                    $coverageSummaryWriter.WriteAttributeString("nr", $lineNumber)
                    $coverageSummaryWriter.WriteAttributeString("mi", $lineCoverageData.TotalBlocks - $lineCoverageData.CoveredBlocks)
                    $coverageSummaryWriter.WriteAttributeString("ci", $lineCoverageData.CoveredBlocks)
                    $coverageSummaryWriter.WriteAttributeString("mb", 0)
                    $coverageSummaryWriter.WriteAttributeString("cb", 0)
                    $coverageSummaryWriter.WriteEndElement() # line

                    if ($lineCoverageData.CoveredBlocks -gt 0)
                    {
                        $coveredLines++
                        $binaryLinesCovered++
                        $totalLinesCovered++
                    }

                    $totalLinesTotal++
                    $binaryLinesTotal++
                }

                $missedBlocks = $totalBlocks - $coveredBlocks
                $missedLines = $coverageData.LineNumberToCoverageData.Keys.Count - $coveredLines
                $missedMethods = $coverageData.TotalMethodNames.Count - $coverageData.CoveredMethodNames.Count
                $coveredMethods = $coverageData.CoveredMethodNames.Count
                $missedClasses = $coverageData.TotalClassNames.Count - $coverageData.CoveredClassNames.Count
                $coveredClasses = $coverageData.CoveredClassNames.Count

                Write-CoverageSectionFooter -htmlWriter $coverageSummaryWriter `
                    -missedBlocks $missedBlocks -coveredBlocks $coveredBlocks `
                    -missedLines $missedLines -coveredLines $coveredLines `
                    -missedMethods $missedMethods -coveredMethods $coveredMethods `
                    -missedClasses $classesMissed -coveredClasses $classesCovered

                $coverageSummaryWriter.WriteEndElement() # sourcefile
                        
                $binaryInstructionsCovered += $coveredBlocks
                $binaryInstructionsTotal += $totalBlocks
                $binaryMethodsCovered += $coveredMethods
                $binaryMethodsTotal += $coverageData.TotalMethodNames.Count
                $binaryClassesCovered += $coveredClasses
                $binaryClassesTotal += $coverageData.TotalClassNames.Count
                        
                $totalInstructionsCovered += $coveredBlocks
                $totalInstructionsTotal += $totalBlocks
                $totalMethodsCovered += $coveredMethods
                $totalMethodsTotal += $coverageData.TotalMethodNames.Count
                $totalClassesCovered += $coveredClasses
                $totalClassesTotal += $coverageData.TotalClassNames.Count

                # First we'll write the legend.
                $sourceFileHtmlWriter.WriteStartElement("table")
                $sourceFileHtmlWriter.WriteAttributeString("style", "$universalStyle$tableStyle$tableLegendStyle")

                $sourceFileHtmlWriter.WriteStartElement("thead")
                $sourceFileHtmlWriter.WriteAttributeString("style", "$universalStyle")
                $sourceFileHtmlWriter.WriteStartElement("tr")
                $sourceFileHtmlWriter.WriteAttributeString("style", "$universalStyle")
                $sourceFileHtmlWriter.WriteStartElement("td")
                $sourceFileHtmlWriter.WriteAttributeString("style", "$universalStyle$tableLegendTheadTdStyle")
                $sourceFileHtmlWriter.WriteAttributeString("colspan", "2")

                $sourceFileHtmlWriter.WriteStartElement("strong")
                $sourceFileHtmlWriter.WriteAttributeString("style", "$universalStyle")
                $sourceFileHtmlWriter.WriteString("Legend")
                $sourceFileHtmlWriter.WriteEndElement() # strong

                $sourceFileHtmlWriter.WriteEndElement() # td
                $sourceFileHtmlWriter.WriteEndElement() # tr
                $sourceFileHtmlWriter.WriteEndElement() # thead

                $sourceFileHtmlWriter.WriteStartElement("tbody")
                $sourceFileHtmlWriter.WriteAttributeString("style", "$universalStyle$tableLegendTbodyStyle")
                $sourceFileHtmlWriter.WriteStartElement("tr")
                $sourceFileHtmlWriter.WriteAttributeString("style", "$universalStyle")

                $sourceFileHtmlWriter.WriteStartElement("td")
                $sourceFileHtmlWriter.WriteAttributeString("style", "$universalStyle$tableLegendTdStyle$tableLegendTdColorStyle$tableLegendTdFcStyle")
                $sourceFileHtmlWriter.WriteString("")
                $sourceFileHtmlWriter.WriteEndElement() # td

                $sourceFileHtmlWriter.WriteStartElement("td")
                $sourceFileHtmlWriter.WriteAttributeString("style", "$universalStyle$tableLegendTdStyle$tableLegendTdExplanationStyle")
                $sourceFileHtmlWriter.WriteString("Fully covered")
                $sourceFileHtmlWriter.WriteEndElement() # td

                $sourceFileHtmlWriter.WriteEndElement() # tr
                $sourceFileHtmlWriter.WriteStartElement("tr")
                $sourceFileHtmlWriter.WriteAttributeString("style", "$universalStyle")

                $sourceFileHtmlWriter.WriteStartElement("td")
                $sourceFileHtmlWriter.WriteAttributeString("style", "$universalStyle$tableLegendTdStyle$tableLegendTdColorStyle$tableLegendTdPcStyle")
                $sourceFileHtmlWriter.WriteString("")
                $sourceFileHtmlWriter.WriteEndElement() # td

                $sourceFileHtmlWriter.WriteStartElement("td")
                $sourceFileHtmlWriter.WriteAttributeString("style", "$universalStyle$tableLegendTdStyle$tableLegendTdExplanationStyle")
                $sourceFileHtmlWriter.WriteString("Partially covered")
                $sourceFileHtmlWriter.WriteEndElement() # td

                $sourceFileHtmlWriter.WriteEndElement() # tr
                $sourceFileHtmlWriter.WriteStartElement("tr")
                $sourceFileHtmlWriter.WriteAttributeString("style", "$universalStyle")

                $sourceFileHtmlWriter.WriteStartElement("td")
                $sourceFileHtmlWriter.WriteAttributeString("style", "$universalStyle$tableLegendTdStyle$tableLegendTdColorStyle$tableLegendTdNcStyle")
                $sourceFileHtmlWriter.WriteString("")
                $sourceFileHtmlWriter.WriteEndElement() # td

                $sourceFileHtmlWriter.WriteStartElement("td")
                $sourceFileHtmlWriter.WriteAttributeString("style", "$universalStyle$tableLegendTdStyle$tableLegendTdExplanationStyle")
                $sourceFileHtmlWriter.WriteString("Not covered")
                $sourceFileHtmlWriter.WriteEndElement() # td

                $sourceFileHtmlWriter.WriteEndElement() # tr
                $sourceFileHtmlWriter.WriteEndElement() # tbody

                $sourceFileHtmlWriter.WriteEndElement() # table

                # Now we'll write the source itself.
                $sourceFileHtmlWriter.WriteStartElement("ol")
                $sourceFileHtmlWriter.WriteAttributeString("style", "$universalStyle$olSourceStyle")

                $sourceFileLines = New-Object 'System.Collections.Generic.List[string]'

                $lineNumber = 1
                $shouldSkip = $false

                (Get-Content $fullSourceFilePath -Raw).Split([Environment]::NewLine, [System.StringSplitOptions]::None) | ForEach-Object {
                    if (-not $shouldSkip)
                    {
                        $liStyle = $olSourceLiStyle

                        if ($lineNumber % 2 -eq 0)
                        {
                            $liStyle = "$liStyle$olSourceLiEvenNumberStyle"
                        }

                        $sourceFileHtmlWriter.WriteStartElement("li")
                        $sourceFileHtmlWriter.WriteAttributeString("style", "$universalStyle$liStyle")

                        $spanStyle = $olSourceSpanStyle

                        $sourceFileHtmlWriter.WriteStartElement("span")

                        [CodeCoverage.SourceLineCoverageData]$lineCoverageData = $null
                        if ($coverageData.LineNumberToCoverageData.TryGetValue($lineNumber, [ref]$lineCoverageData))
                        {
                            if ($lineCoverageData.CoveredBlocks -eq $lineCoverageData.TotalBlocks)
                            {
                                $spanStyle = "$spanStyle$olSourceSpanFcStyle"
                            }
                            elseif ($lineCoverageData.CoveredBlocks -eq 0)
                            {
                                $spanStyle = "$spanStyle$olSourceSpanNcStyle"
                            }
                            else
                            {
                                $spanStyle = "$spanStyle$olSourceSpanPcStyle"
                            }
                        }

                        $sourceFileHtmlWriter.WriteAttributeString("style", "$universalStyle$spanStyle")

                        # Sometimes nonprintable characters can creep in, which causes XmlWriter to freak out.
                        # Let's prune all of those characters to make sure.
                        $lineString = $_

                        for ($c = 0x00; $c -lt 0x20; $c++)
                        {
                            $lineString = $lineString.Replace("$([char]$c)", "")
                        }

                        # We also need to change all of the spaces to non-breaking spaces so they show up in layout.
                        $lineString = $lineString.Replace(' ', [char]0xA0)

                        $sourceFileHtmlWriter.WriteString($lineString)
                        
                        $sourceFileHtmlWriter.WriteEndElement() # span
                        $sourceFileHtmlWriter.WriteEndElement() # li

                        $lineNumber++

                        # Since the newline sequence is \r\n, every newline character becomes two new lines,
                        # so we'll skip every other line.
                        $shouldSkip = $true
                    }
                    else
                    {
                        $shouldSkip = $false
                    }
                }

                $sourceFileHtmlWriter.WriteEndElement() # ol

                Write-HTMLFooter -htmlWriter $sourceFileHtmlWriter

                Write-CoverageRow -htmlWriter $binaryIndexHtmlWriter -elementName $sourceFile.Name -elementUrl "$($sourceFile.Name).html" `
                    -coveredBlocks $coveredBlocks -totalBlocks $totalBlocks -maxTotalBlocks $sourceFileDirectoryToMaxTotalBlocksDictionary[$sourceFileToDirectoryDictionary[$sourceFile]] `
                    -coveredLines $coveredLines -totalLines $coverageData.LineNumberToCoverageData.Keys.Count `
                    -coveredMethods $coveredMethods -totalMethods $coverageData.TotalMethodNames.Count `
                    -coveredClasses $coveredClasses -totalClasses $coverageData.TotalClassNames.Count `
                    -indentLevel $indentLevel
            }

            Write-CoverageSectionFooter -htmlWriter $coverageSummaryWriter `
                -missedBlocks ($binaryInstructionsTotal - $binaryInstructionsCovered) -coveredBlocks $binaryInstructionsCovered `
                -missedLines ($binaryLinesTotal - $binaryLinesCovered) -coveredLines $binaryLinesCovered `
                -missedMethods ($binaryMethodsTotal - $binaryMethodsCovered) -coveredMethods $binaryMethodsCovered `
                -missedClasses ($binaryClassesTotal - $binaryClassesCovered) -coveredClasses $binaryClassesCovered

            $coverageSummaryWriter.WriteEndElement() # package

            Write-TableFooter -htmlWriter $binaryIndexHtmlWriter `
                -coveredBlocks $binaryInstructionsCovered -totalBlocks $binaryInstructionsTotal `
                -coveredLines $binaryLinesCovered -totalLines $binaryLinesTotal `
                -coveredMethods $binaryMethodsCovered -totalMethods $binaryMethodsTotal `
                -coveredClasses $binaryClassesCovered -totalClasses $binaryClassesTotal

            Write-HTMLFooter -htmlWriter $binaryIndexHtmlWriter

            $binaryCoverageData = New-Object 'CodeCoverage.BinaryCoverageData'
            $binaryCoverageData.BinaryName = $binary.Name
            $binaryCoverageData.CoveredInstructions = $binaryInstructionsCovered
            $binaryCoverageData.TotalInstructions = $binaryInstructionsTotal
            $binaryCoverageData.CoveredLines = $binaryLinesCovered
            $binaryCoverageData.TotalLines = $binaryLinesTotal
            $binaryCoverageData.CoveredMethods = $binaryMethodsCovered
            $binaryCoverageData.TotalMethods = $binaryMethodsTotal
            $binaryCoverageData.CoveredClasses = $binaryClassesCovered
            $binaryCoverageData.TotalClasses = $binaryClassesTotal

            $binaryCoverageDataList.Add($binaryCoverageData)
        }
    }

    Write-CoverageSectionFooter -htmlWriter $coverageSummaryWriter `
        -missedBlocks ($totalInstructionsTotal - $totalInstructionsCovered) -coveredBlocks $totalInstructionsCovered `
        -missedLines ($totalLinesTotal - $totalLinesCovered) -coveredLines $totalLinesCovered `
        -missedMethods ($totalMethodsTotal - $totalMethodsCovered) -coveredMethods $totalMethodsCovered `
        -missedClasses ($totalClassesTotal - $totalClassesCovered) -coveredClasses $totalClassesCovered

    $coverageSummaryWriter.WriteEndElement() # group

    Write-CoverageSectionFooter -htmlWriter $coverageSummaryWriter `
        -missedBlocks ($totalInstructionsTotal - $totalInstructionsCovered) -coveredBlocks $totalInstructionsCovered `
        -missedLines ($totalLinesTotal - $totalLinesCovered) -coveredLines $totalLinesCovered `
        -missedMethods ($totalMethodsTotal - $totalMethodsCovered) -coveredMethods $totalMethodsCovered `
        -missedClasses ($totalClassesTotal - $totalClassesCovered) -coveredClasses $totalClassesCovered

    $coverageSummaryWriter.WriteEndElement() # report
    $coverageSummaryWriter.WriteEndDocument()
    $coverageSummaryWriter.Flush()
    $coverageSummaryWriter.Close()

    $binaryCoverageDataList = $binaryCoverageDataList | Sort-Object BinaryName

    $maxBinaryTotalBlocks = 0

    foreach ($binaryCoverageData in $binaryCoverageDataList)
    {
        if ($maxBinaryTotalBlocks -lt $binaryCoverageData.TotalInstructions)
        {
            $maxBinaryTotalBlocks = $binaryCoverageData.TotalInstructions
        }
    }

    foreach ($binaryCoverageData in $binaryCoverageDataList)
    {
        Write-CoverageRow -htmlWriter $indexHtmlWriter -elementName $binaryCoverageData.BinaryName -elementUrl "$($binaryCoverageData.BinaryName)/index.html" `
            -coveredBlocks $binaryCoverageData.CoveredInstructions -totalBlocks $binaryCoverageData.TotalInstructions -maxTotalBlocks $maxBinaryTotalBlocks `
            -coveredLines $binaryCoverageData.CoveredLines -totalLines $binaryCoverageData.TotalLines `
            -coveredMethods $binaryCoverageData.CoveredMethods -totalMethods $binaryCoverageData.TotalMethods `
            -coveredClasses $binaryCoverageData.CoveredClasses -totalClasses $binaryCoverageData.TotalClasses `
            -indentLevel 0
    }

    Write-TableFooter -htmlWriter $indexHtmlWriter `
        -coveredBlocks $totalInstructionsCovered -totalBlocks $totalInstructionsTotal `
        -coveredLines $totalLinesCovered -totalLines $totalLinesTotal `
        -coveredMethods $totalMethodsCovered -totalMethods $totalMethodsTotal `
        -coveredClasses $totalClassesCovered -totalClasses $totalClassesTotal

    Write-HTMLFooter $indexHtmlWriter
}