[CmdLetBinding()]
Param(
    [Parameter(Mandatory = $true)]
    [string]$ArgsFile,
    [switch]$noRerunFailed
)

$rerunFailed=$True
if ($noRerunFailed.IsPresent)  { $rerunFailed=$false }
# Uncomment to debug tests: start
# $rerunFailed=$false
# Uncomment to debug tests: end

Write-Host "WorkItemStartTime: $(Get-Date)"

$workingDir = Get-Location
Write-Host "workingDir = $workingDir"

cd $env:HELIX_CORRELATION_PAYLOAD

function Delete-IfExists
{
    param ([string] $path)

    if (Test-Path $path)
    {
        Remove-Item $path
    }
}

function Copy-IfExists
{
    param ([string] $srcPath, [string] $destPath)

    if (Test-Path $srcPath)
    {
        Copy-Item $srcPath $destPath -Force
    }
}

# Cleanup any files that may have been left-over from previous runs:
Delete-IfExists .\*_subresults.json
Delete-IfExists .\*.wtl
Delete-IfExists .\*.pgc
Delete-IfExists .\WexLogFileOutput\*
Delete-IfExists .\testResults.xml

$testnameprefix = $env:testnameprefix

Write-Host "Parsing Args:"
$args = (Get-Content (Join-Path $workingDir $ArgsFile))

$parts = $args -Split("/taefQuery ")
$testBinaries = $parts[0]

if ($parts.Count -gt 1)
{
    $parts = $parts[1] -Split("/taefParameters ")
    $taefQuery = $parts[0]

    if($parts.Count -gt 1)
    {
        $taefParameters = $parts[1]
    }
}

Write-Host "testBinaries = $testBinaries"
Write-Host "taefQuery = $taefQuery"
Write-Host "taefParameters = $taefParameters"
Write-Host "testnameprefix = $testnameprefix"

$picturesPath = [Environment]::GetFolderPath("mypictures")
$xamlTaefOutputPath = Join-Path $picturesPath "XamlTaefOutput"
Write-Host "xamlTaefOutputPath = $xamlTaefOutputPath"
Delete-IfExists "$xamlTaefOutputPath\*"

function Copy-MasterFiles
{
    Copy-IfExists $picturesPath\*.xml* $env:HELIX_WORKITEM_UPLOAD_ROOT
    Copy-IfExists $xamlTaefOutputPath\* $env:HELIX_WORKITEM_UPLOAD_ROOT
}

Add-Type -AssemblyName System.Windows.Forms

function Wiggle-Mouse
{
    [System.Windows.Forms.Cursor]::Position = New-Object System.Drawing.Point(1,1)
    Start-Sleep -Milliseconds 100
    [System.Windows.Forms.Cursor]::Position = New-Object System.Drawing.Point(5,5)
    Start-Sleep -Milliseconds 100
    [System.Windows.Forms.Cursor]::Position = New-Object System.Drawing.Point(1,1)
    Start-Sleep -Milliseconds 100
}


function Run-Taef
{
    param ([string] $taefAdditionalParams)

    .\TestPass-EnsureMachineStateCore.ps1
    Wiggle-Mouse

    $teCommand = "te.exe $testBinaries /enablewttlogging /enableEtwLogging /unicodeOutput:false /testtimeout:0:05 /p:DisableErrorHandling /screenCaptureOnError $taefParameters $taefAdditionalParams"
    Write-Host $teCommand

    # Ideally, we would just use '&' or 'Invoke-Expression' here to execute taef. However, powershell unhelpfully modifies the string to add 
    # extra quotes around parts of the arguments which gives the incorrect behavior since the argument string is already exactly as it needs 
    # to be. I was unable to find a way to disable this behavior, so as a workaround we create a .cmd file and invoke that.
    Out-File -FilePath "run.cmd" -Encoding ascii -InputObject $teCommand
    & ./run.cmd
}

function Copy-Screenshots
{
    if (Test-Path ".\WexLogFileOutput")
    {
        # Copy at most 10 screenshots to the upload path.
        # In the cases where a large number of tests failed, there is little value in uploading dozens of screenshots
        $files = Get-ChildItem -Path ".\WexLogFileOutput" -Filter *.jpg |Select-Object -First 10
        foreach($file in $files)
        {
            Copy-Item $file.FullName $env:HELIX_WORKITEM_UPLOAD_ROOT -Force
        }
        Delete-IfExists .\WexLogFileOutput\*
    }
}

Write-Host "WorkItemSetupStartTime: $(Get-Date)"

Write-Host "Run Setup If Needed"
.\RunSetupIfNeeded.ps1

# Dump a list of installed AppxPackages to a text file. This can be useful for debugging purposes.
Write-Host "(Skipping Get-AppxPackge dump to save time)"
#Get-AppxPackage | Out-File -FilePath (Join-Path $env:HELIX_WORKITEM_UPLOAD_ROOT "appxpackages.txt" )

.\TestPass-PreRunCore.ps1

Write-Host "WorkItemTestStartTime: $(Get-Date)"

# Run the tests:
Run-Taef("$taefQuery")

Write-Host "WorkItemTestEndTime: $(Get-Date)"

Move-Item .\te.wtl te_original.wtl -Force
Copy-Item .\te_original.wtl $env:HELIX_WORKITEM_UPLOAD_ROOT -Force
Copy-Screenshots
Copy-IfExists .\*.pgc $env:HELIX_WORKITEM_UPLOAD_ROOT
Copy-MasterFiles

Add-Type -Language CSharp -ReferencedAssemblies System.Xml,System.Xml.Linq,System.Runtime.Serialization,System.Runtime.Serialization.Json (Get-Content .\HelixTestHelpers.cs -Raw)

$failedTestQuery = [HelixTestHelpers.FailedTestDetector]::GetFailedTestQuery((Join-Path (Get-Location) "te_original.wtl"))
Write-Host "failedTestQuery = $failedTestQuery"

# The first time, we'll just re-run failed tests once. In many cases, tests fail very rarely, such that
# a single re-run will be sufficient to detect many unreliable tests.
if ($failedTestQuery -and $rerunFailed)
{
    Write-Host "WorkItemTestRerunStartTime: $(Get-Date)"

    Run-Taef("/select:`"$failedTestQuery`"")

    Write-Host "WorkItemTestRerunEndTime: $(Get-Date)"

    Move-Item .\te.wtl te_rerun.wtl -Force
    Copy-Item .\te_rerun.wtl $env:HELIX_WORKITEM_UPLOAD_ROOT -Force
    Copy-Screenshots
    Copy-MasterFiles

    $failedTestQuery = [HelixTestHelpers.FailedTestDetector]::GetFailedTestQuery((Join-Path (Get-Location) "te_rerun.wtl"))
    Write-Host "failedTestQuery = $failedTestQuery"
}


# If there are still failing tests remaining, we'll run them nine more times, so they'll have been run a total of 11 times.
# We determine if a test is reported as 'failed' or 'unreliable' by comparing the number of passes to rerunPassesRequiredToAvoidFailure
# which is specified by the Pipeline.
if ($failedTestQuery -and $rerunFailed)
{
    Write-Host "WorkItemTestLoopStartTime: $(Get-Date)"

    Run-Taef("/testmode:Loop /Loop:9 /LoopTest:1 /isolationlevel:test /select:`"$failedTestQuery`"")
    Move-Item .\te.wtl te_rerun_multiple.wtl -Force
    Copy-Item .\te_rerun_multiple.wtl $env:HELIX_WORKITEM_UPLOAD_ROOT -Force

    Copy-Screenshots
    Copy-MasterFiles

    Write-Host "WorkItemTestLoopEndTime: $(Get-Date)"
}

.\ConvertWttLogToXUnit.ps1 te_original.wtl te_rerun.wtl te_rerun_multiple.wtl te_rerun_more.wtl testResults.xml $testnameprefix

Copy-Item .\*_subresults.json $env:HELIX_WORKITEM_UPLOAD_ROOT -Force

.\TestPass-PostRunCore.ps1

# Display testResults.xml
Get-Content .\testResults.xml

if ((Join-Path $workingDir "") -ne (Join-Path $env:HELIX_CORRELATION_PAYLOAD ""))
{
    Copy-Item .\testResults.xml $workingDir -Force
}

cd $workingDir

Write-Host "WorkItemEndTime: $(Get-Date)"
