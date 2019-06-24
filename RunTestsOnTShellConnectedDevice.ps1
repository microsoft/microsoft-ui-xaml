[CmdletBinding()]
param(
    [switch]$NoRun,
    [switch]$NoDeploy,
    [switch]$NoBuild,
    [switch]$ForceDeploy,
    [switch]$LogVerbose,
    [switch]$LogSuperVerbose,
    [switch]$Release,
    [Alias("L")][switch]$List,
    [Alias("CC")][switch]$CodeCoverage,
    [String[]]$Name,
    [String[]]$xName,
    [String]$Flavor = "Debug",
    [String]$Platform = "x86",
    [String]$MinVersion,
    [switch]$dllOnly,
    [switch]$appxOnly,
    [switch]$waitForDebugger,
    [switch]$waitForAppDebugger,
    [int]$Iterations = 1,
    [string]$LogFile = "TestResults.out",
    [String]$OutputPath = "$PSScriptRoot\CodeCoverageOutput",
    [String]$TestTimeout,
    [String]$MagellanInstallPath = "$env:USERPROFILE\.nuget\packages\microsoft.internal.magellan\5.4.170227001-pkges"
)

#
# NOTE: This file currently relies on TShell to execute commands on a remote machine, which is not available externally.
#       Use Open-Device to connect to a remote machine, then run this script to run tests on that machine.
#       TODO 19757865: Change this so that it no longer has a dependency on TShell.
#

if (!(test-path variable:\DeviceAddress))
{
    Write-Host "";
    Write-Host "DeviceAddress variable is not set -- are you connected to a device?" -foregroundcolor red
    Write-Host "";
    return;
}

$deviceDir = "c:\data\test\bin"
$ignoredOutput = cmdd if not exist $deviceDir mkdir $deviceDir


if(!$NoDeploy)
{
    $testSuite = "DevTest"
    if($Release)
    {
        $testSuite = "NugetPkgTests"
    }
    & .\CreateTestBinariesDirFromBuild.ps1 -NoBuild:$NoBuild -TestSuite:$testSuite -Flavor:$Flavor -Platform:$Platform
}


# If we're doing a code coverage run, let's instrument the binaries now.
if ($CodeCoverage)
{
    Invoke-Expression "$PSScriptRoot\tools\CodeCoverage\InstrumentBinaries.ps1 -BuildPlatform $Platform -BuildConfiguration $Flavor"
}

# Always copy over the test files.

$repoDirectory = Split-Path -Parent $script:MyInvocation.MyCommand.Path
if(!$NoDeploy)
{
    $payloadDir = Join-Path $repoDirectory "HelixPayload\$flavor\$platform\*"
    Write-Host "Copying files from '$payloadDir' to device";
    $putDOutput += putd -Recurse $payloadDir $deviceDir
}

# Make watson keep dumps offline locally. We will check after the tests run to see if there were any dumps.
$regdOutput += regd add "HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\Windows Error Reporting" /v DisableArchive /t REG_DWORD /d 0
$regdOutput += regd add "HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\Windows Error Reporting" /v Disabled /t REG_DWORD /d 0
$regdOutput += regd add "HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\Windows Error Reporting" /v BypassDataThrottling /t REG_DWORD /d 1
$regdOutput += regd add "HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\Windows Error Reporting" /v ConfigureArchive /t REG_DWORD /d 2
$regdOutput += regd add "HKEY_LOCAL_MACHINE\Software\Microsoft\Windows\Windows Error Reporting\LocalDumps" /v DumpType /t REG_DWORD /d 2
$regdOutput += regd add "HKEY_LOCAL_MACHINE\SOFTWARE\Policies\Microsoft\Windows\DataCollection" /v AllowTelemetry /t REG_DWORD /d 3

# Get a snapshot of the dump folder before we start running tests
$WERArchiveBeforeRun = ((cmdd dir /s /b "%programdata%\microsoft\windows\wer\reportarchive\*.zip" -HideOutput).Output -split "\r\n")

cdd $deviceDir
if (!$NoRun)
{
    if ($CodeCoverage)
    {
        Write-Host "Starting code coverage..."
        execd cmd -AsUser -Arguments "/c %SystemRoot%\system32\WindowsPowerShell\v1.0\powershell.exe -ExecutionPolicy Bypass -File $deviceDir\StartCodeCoverage.ps1 -MagellanInstallPath $deviceDir\Magellan"
    }

    $teCmd = "execd ""$deviceDir\TE.exe"" -DeviceLogPath $LogFile -Arguments """
    $selectQuery = @()

    if (!$appxOnly)
    {
        if(!$Release)
        {
            $teCmd += " MUXControls.Test.dll"
        }
        else
        {
            $teCmd += " MUXControls.ReleaseTest.dll"
        }
    }

    if (!$dllOnly)
    {
        if(!$Release)
        {
            $teCmd += " MUXControlsTestApp.appx /APPX:CertificateFileName=MUXControlsTestApp.cer";
            $teCmd += " IXMPTestApp.appx /APPX:CertificateFileName=IXMPTestApp.cer";
        }
    }

    if ($Name)
    {
        $nameCondition = ($Name | % { "@Name='*$_*'" }) -join " OR "
        $selectQuery += "$nameCondition"
    }

    if ($MinVersion)
    {
        $selectQuery += "@MinVersion='$MinVersion' OR not(@MinVersion=*)"
    }

    if($selectQuery)
    {
        $teCmd += " /select:""""$(($selectQuery | % { "($_)" }) -join " AND ")"""""
    }

    if ($LogVerbose -or $LogSuperVerbose)
    {
        $teCmd += " /logOutput:High /p:LogVerbose";
    }

    if ($LogSuperVerbose)
    {
        $teCmd += " /p:LogSuperVerbose";
    }

    if ($List)
    {
        $teCmd += " /list";
    }

    if ($waitForDebugger)
    {
        $teCmd += " /p:WaitForDebugger";
    }

    if ($waitForAppDebugger)
    {
        $teCmd += " /p:WaitForAppDebugger";
    }
    
    if ($TestTimeout)
    {
        $teCmd += " /testtimeout:$TestTimeout";
    }
    else
    {
        # Don't set a timeout if someone is attaching a debugger. We don't want the test to be killed while 
        # it is in the middle of being debugged.
        if(!$waitForDebugger -and !$waitForAppDebugger)
        {
            $teCmd += " /testtimeout:0:01";
        }
    }
    
    if ($Iterations -ne 1)
    {
        $teCmd += " /testmode:loop /looptest:$Iterations"
    }

    $teCmd += " /screenCaptureOnError""";

    Write-Host
    Write-Host "Running '$teCmd'";
    Write-Host
    Invoke-Expression $teCmd | Set-Variable TEOutput;
    if ($TEOutput.ExitCode -ne 0)
    {
        Write-Host ("ERROR: TE.exe returned error code {0}" -f $TEOutput.ExitCode);
    }
    
    if ($CodeCoverage)
    {
        Write-Host "Stopping code coverage..."
        execd cmd -AsUser -Arguments "/c %SystemRoot%\system32\WindowsPowerShell\v1.0\powershell.exe -ExecutionPolicy Bypass -File $deviceDir\StopCodeCoverage.ps1 -TestSuite Integration -MagellanInstallPath $deviceDir\Magellan" 1> $null
        Write-Host

        if ($TEOutput.ExitCode -eq 0)
        {
            Write-Host "Gathering code coverage data..."

            if (-not (Test-Path "$OutputPath"))
            {
                New-Item "$OutputPath" -ItemType Directory
            }

            Copy-Item "BuildOutput\$Flavor\$platform\Microsoft.UI.Xaml\instr\*.covsym" "$OutputPath\" 1> $null
            getd "C:\ProgramData\Coverage\microsoft.ui.xaml.dll\*.covdata" "$OutputPath\" 1> $null
        
            Write-Host "Creating code coverage report..."

            Invoke-Expression "$PSScriptRoot\tools\CodeCoverage\CreateCodeCoverageReport.ps1 -MagellanInstallPath `"$MagellanInstallPath`" -covSymPath `"$OutputPath`" -covDataPath `"$OutputPath`" -coverageSummaryOutputPath `"$OutputPath\Summary`" -coverageReportOutputPath `"$OutputPath\Report`" -isLocalRun" 1> $null

            if (Test-Path $OutputPath\Report\index.html)
            {
                Write-Host "Code coverage report written to $OutputPath\Report."

                $shouldOpenReport = ""
                while ($shouldOpenReport -inotlike "y" -and $shouldOpenReport -inotlike "n")
                {
                    $shouldOpenReport = Read-Host "Open report? (y/n)"
                }

                if ($shouldOpenReport -ilike "y")
                {
                    Start-Process -FilePath "$OutputPath\Report\index.html"
                }
            }
        }
        else
        {
            Write-Host "Test failures detected. Skipping code coverage data gathering."
        }
    }
}

# If we were doing a code coverage run, let's put back the old test appx now.
if ($CodeCoverage -and (Test-Path "$testAppOutputDir\AppPackages\MUXControlsTestApp_Test\MUXControlsTestApp.appx.orig"))
{
    Remove-Item "$testAppOutputDir\AppPackages\MUXControlsTestApp_Test\MUXControlsTestApp.appx"
    Rename-Item "$testAppOutputDir\AppPackages\MUXControlsTestApp_Test\MUXControlsTestApp.appx.orig" "$testAppOutputDir\AppPackages\MUXControlsTestApp_Test\MUXControlsTestApp.appx"
}

# Check for dumps
$WERArchiveAfterRun = ((cmdd dir /s /b "%programdata%\microsoft\windows\wer\reportarchive\*.zip" -HideOutput).Output -split "\r\n")
$NewCabs = $WERArchiveAfterRun | Where-Object { -not $WERArchiveBeforeRun.Contains($_) }
Write-Verbose "Before cabs: $WERArchiveBeforeRun"
Write-Verbose "After cabs: $WERArchiveAfterRun"
Write-Verbose "New cabs = $NewCabs"
if ($NewCabs)
{
    Write-Host "Error: Dumps found during test run" -foregroundcolor Red
    $LocalDumpPath = "$PSScriptRoot\DumpFiles"
    if (-not (Test-Path $LocalDumpPath)) { New-Item -Force $LocalDumpPath -ItemType Directory }
    foreach ($NewCab in $NewCabs)
    {
        # Make the filename the archive directory name (instead of "report.zip")
        $fileName = Split-Path (Split-Path $NewCab) -Leaf
        $localFilePath = "$LocalDumpPath\$fileName.zip"
        $ignored = Get-Device $NewCab -Destination $localFilePath
        Write-Host "CAB: $localFilePath" -foregroundcolor Red
    }
}