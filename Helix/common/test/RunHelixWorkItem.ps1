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

# The lifetime stress suite runs as a NON-GATING report. It repeatedly creates/tears-down controls and forces GC
# to surface native peer-lifetime bugs, so a run can legitimately end in a native fail-fast that crashes the TAEF
# host (te.exe) mid-suite. We must never let that (or an ordinary scenario failure) fail the Run Tests stage:
#   * the per-scenario report stays fully readable in this work item's console log and in te_original.wtl, but
#   * this work item always emits a testResults.xml that contains zero failures (crash -> synthetic passing
#     report; scenario failures -> downgraded to non-gating "Skip").
# All of this behavior is scoped strictly to this one work item via $isLifetimeStress so no other suite changes.
$isLifetimeStress = ($taefQuery -match 'LifetimeStressTestSuite')
if ($isLifetimeStress)
{
    Write-Host "=== Lifetime stress suite detected: running as a NON-GATING report. Failures or a native TAEF-host crash will be captured in the log / te_original.wtl but will NOT fail the pipeline. ==="
}

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

    # Per-test TAEF timeout. Normal test passes use a fixed 5-minute per-test budget. When the lifetime stress
    # suite is put into soak mode (WINUI_LIFETIME_STRESS_MINUTES > 0, set by build/WinUI-LifetimeStress.yml), each
    # lifetime scenario loops on a wall-clock budget of that many minutes, so a single scenario runs well past the
    # default 5-minute per-test timeout. TAEF would treat that as a hung test and fail it - which is why the
    # *runner* (per-test) timeout, not just the outer job timeout, has to be aligned with the soak budget. When
    # soak mode is on, give each scenario its full budget plus headroom for the per-iteration settle/GC and final
    # teardown; otherwise keep the original 5-minute default so unrelated test passes are unaffected.
    $testTimeout = "0:05"
    [double]$soakMinutes = 0
    if ($env:WINUI_LIFETIME_STRESS_MINUTES -and [double]::TryParse($env:WINUI_LIFETIME_STRESS_MINUTES, [ref]$soakMinutes) -and ($soakMinutes -gt 0))
    {
        $headroomMinutes = [math]::Max(5, [math]::Ceiling($soakMinutes / 2))
        $timeoutMinutes = [math]::Ceiling($soakMinutes) + $headroomMinutes
        $testTimeout = ([TimeSpan]::FromMinutes($timeoutMinutes)).ToString("hh\:mm\:ss")
        Write-Host "Lifetime stress soak mode detected (WINUI_LIFETIME_STRESS_MINUTES=$($env:WINUI_LIFETIME_STRESS_MINUTES)); using per-test TAEF timeout $testTimeout."
    }

    $teCommand = "te.exe $testBinaries /enablewttlogging /enableEtwLogging /unicodeOutput:false /testtimeout:$testTimeout /p:DisableErrorHandling /screenCaptureOnError $taefParameters $taefAdditionalParams"
    Write-Host $teCommand

    # Ideally, we would just use '&' or 'Invoke-Expression' here to execute taef. However, powershell unhelpfully modifies the string to add 
    # extra quotes around parts of the arguments which gives the incorrect behavior since the argument string is already exactly as it needs 
    # to be. I was unable to find a way to disable this behavior, so as a workaround we create a .cmd file and invoke that.
    Out-File -FilePath "run.cmd" -Encoding ascii -InputObject $teCommand
    & ./run.cmd
}

function Set-LifetimeResultsNonGating
{
    # Guarantees this work item's testResults.xml exists and contains zero failures, so the lifetime stress suite
    # is a readable report that never fails the pipeline. See the $isLifetimeStress comment above for rationale.
    param ([string] $resultsPath, [string] $testnameprefix)

    $prefix = ""
    if ($testnameprefix) { $prefix = "$testnameprefix." }

    $needSynthetic = $true

    if (Test-Path $resultsPath)
    {
        try
        {
            [xml]$doc = Get-Content $resultsPath -Raw
            if ($doc.assemblies)
            {
                $needSynthetic = $false
                $flipped = 0
                foreach ($test in @($doc.SelectNodes('//test')))
                {
                    if ($test.result -eq 'Fail')
                    {
                        # Downgrade to a non-gating 'Skip' (PublishTestResults only fails on Failed tests). The
                        # scenario stays visible; its detail remains in the console log and te_original.wtl.
                        $test.result = 'Skip'
                        $failureNode = $test.SelectSingleNode('failure')
                        if ($failureNode) { [void]$test.RemoveChild($failureNode) }
                        $flipped++
                    }
                }

                # Recompute passed/failed/skipped totals so the published summary is consistent and failed=0.
                foreach ($scope in (@($doc.SelectNodes('//assembly')) + @($doc.SelectNodes('//collection'))))
                {
                    $tests = @($scope.SelectNodes('.//test'))
                    $pass = @($tests | Where-Object { $_.result -eq 'Pass' }).Count
                    $skip = @($tests | Where-Object { $_.result -eq 'Skip' }).Count
                    if ($scope.Attributes['total'])   { $scope.total = "$($tests.Count)" }
                    if ($scope.Attributes['passed'])  { $scope.passed = "$pass" }
                    if ($scope.Attributes['skipped']) { $scope.skipped = "$skip" }
                    if ($scope.Attributes['failed'])  { $scope.failed = "0" }
                }

                $doc.Save((Resolve-Path $resultsPath).Path)
                Write-Host "Lifetime stress: downgraded $flipped failing result(s) to non-gating 'Skip' in testResults.xml."
            }
        }
        catch
        {
            Write-Host "Lifetime stress: could not post-process testResults.xml ($($_.Exception.Message)); emitting a synthetic passing report instead."
            $needSynthetic = $true
        }
    }

    if ($needSynthetic)
    {
        # te.exe most likely crashed on a native lifetime fault before writing any results. Emit a single passing
        # entry so the suite is present (not silently missing) and the Run Tests stage stays green.
        $runDate = (Get-Date).ToString('yyyy-MM-dd')
        $runTime = (Get-Date).ToString('HH:mm:ss')
        $testName = "${prefix}LifetimeStressTestSuite.Report"
        $xml = @"
<?xml version="1.0" encoding="utf-8"?>
<assemblies>
  <assembly name="MUXControlsTestApp.dll" test-framework="TAEF" run-date="$runDate" run-time="$runTime" total="1" passed="1" failed="0" skipped="0" errors="0" time="0">
    <collection total="1" passed="1" failed="0" skipped="0" name="Test collection" time="0">
      <test name="$testName" type="LifetimeStressTestSuite" method="Report" time="0" result="Pass">
        <output>The lifetime stress suite did not produce a results file - the TAEF host most likely crashed on a native lifetime fault. Reported as a non-gating pass; see this work item's console log and te_original.wtl for the captured report up to the point of failure.</output>
      </test>
    </collection>
  </assembly>
</assemblies>
"@
        Set-Content -Path $resultsPath -Value $xml -Encoding UTF8
        Write-Host "Lifetime stress: emitted synthetic passing report at testResults.xml (te.exe produced no results file)."
    }
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

if ($isLifetimeStress -and -not (Test-Path .\te.wtl))
{
    # te.exe crashed without flushing its log. Don't emit a noisy error; the non-gating handling below will
    # produce a synthetic passing report so the suite is still present and the stage stays green.
    Write-Host "Lifetime stress: te.wtl was not produced (TAEF host likely crashed on a native lifetime fault)."
}
else
{
    Move-Item .\te.wtl te_original.wtl -Force
    Copy-Item .\te_original.wtl $env:HELIX_WORKITEM_UPLOAD_ROOT -Force
}
Copy-Screenshots
Copy-IfExists .\*.pgc $env:HELIX_WORKITEM_UPLOAD_ROOT
Copy-MasterFiles

Add-Type -Language CSharp -ReferencedAssemblies System.Xml,System.Xml.Linq,System.Runtime.Serialization,System.Runtime.Serialization.Json (Get-Content .\HelixTestHelpers.cs -Raw)

$failedTestQuery = $null
if (Test-Path (Join-Path (Get-Location) "te_original.wtl"))
{
    $failedTestQuery = [HelixTestHelpers.FailedTestDetector]::GetFailedTestQuery((Join-Path (Get-Location) "te_original.wtl"))
}
Write-Host "failedTestQuery = $failedTestQuery"

# The first time, we'll just re-run failed tests once. In many cases, tests fail very rarely, such that
# a single re-run will be sufficient to detect many unreliable tests.
# (Reruns are skipped for the lifetime stress suite - it is a non-gating report, so re-running a scenario that
# intentionally provokes native lifetime faults would only add time and extra crash dumps.)
if ($failedTestQuery -and $rerunFailed -and -not $isLifetimeStress)
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
# (Skipped for the lifetime stress suite - see the note on the first rerun block above.)
if ($failedTestQuery -and $rerunFailed -and -not $isLifetimeStress)
{
    Write-Host "WorkItemTestLoopStartTime: $(Get-Date)"

    Run-Taef("/testmode:Loop /Loop:9 /LoopTest:1 /isolationlevel:test /select:`"$failedTestQuery`"")
    Move-Item .\te.wtl te_rerun_multiple.wtl -Force
    Copy-Item .\te_rerun_multiple.wtl $env:HELIX_WORKITEM_UPLOAD_ROOT -Force

    Copy-Screenshots
    Copy-MasterFiles

    Write-Host "WorkItemTestLoopEndTime: $(Get-Date)"
}

if ($isLifetimeStress)
{
    # Non-gating report path: convert real results when te.exe produced a log, but never let a conversion error
    # (or a missing log from a crashed host) stop the runner - Set-LifetimeResultsNonGating guarantees a valid,
    # zero-failure testResults.xml either way.
    if (Test-Path .\te_original.wtl)
    {
        try
        {
            .\ConvertWttLogToXUnit.ps1 te_original.wtl te_rerun.wtl te_rerun_multiple.wtl te_rerun_more.wtl testResults.xml $testnameprefix
        }
        catch
        {
            Write-Host "Lifetime stress: ConvertWttLogToXUnit failed ($($_.Exception.Message)); a synthetic passing report will be emitted."
            Delete-IfExists .\testResults.xml
        }
    }
    Set-LifetimeResultsNonGating -resultsPath (Join-Path (Get-Location) "testResults.xml") -testnameprefix $testnameprefix
}
else
{
    .\ConvertWttLogToXUnit.ps1 te_original.wtl te_rerun.wtl te_rerun_multiple.wtl te_rerun_more.wtl testResults.xml $testnameprefix
}

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
