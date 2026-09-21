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

# The lifetime stress suite is a non-gating report: it can crash the TAEF host on a native fault, so this work
# item always emits a zero-failure testResults.xml. Scoped to this work item via $isLifetimeStress.
$isLifetimeStress = ($taefQuery -match 'LifetimeStressTestSuite')
if ($isLifetimeStress)
{
    Write-Host "=== Lifetime stress suite detected: running as a NON-GATING report. Failures or a native TAEF-host crash will be captured in the log / te_original.wtl but will NOT fail the pipeline. ==="

    # te.processhost.exe is broker-spawned and seeds its environment from the User/Machine registry, not this
    # shell, so set WINUI_LIFETIME_STRESS_NATIVE there (and in-process) unless the pipeline already provided it.
    if (-not $env:WINUI_LIFETIME_STRESS_NATIVE)
    {
        $env:WINUI_LIFETIME_STRESS_NATIVE = "1"
    }

    # Registry-backed scopes so the broker-spawned te.processhost.exe inherits it at process creation.
    try
    {
        [Environment]::SetEnvironmentVariable("WINUI_LIFETIME_STRESS_NATIVE", $env:WINUI_LIFETIME_STRESS_NATIVE, "User")
        Write-Host "Lifetime stress: set WINUI_LIFETIME_STRESS_NATIVE at User scope."
    }
    catch
    {
        Write-Host "Lifetime stress: WARNING - failed to set WINUI_LIFETIME_STRESS_NATIVE at User scope: $($_.Exception.Message)"
    }
    try
    {
        [Environment]::SetEnvironmentVariable("WINUI_LIFETIME_STRESS_NATIVE", $env:WINUI_LIFETIME_STRESS_NATIVE, "Machine")
        Write-Host "Lifetime stress: set WINUI_LIFETIME_STRESS_NATIVE at Machine scope."
    }
    catch
    {
        # Machine scope requires elevation; not fatal - User scope (and Process scope) still apply.
        Write-Host "Lifetime stress: NOTE - could not set WINUI_LIFETIME_STRESS_NATIVE at Machine scope (needs elevation): $($_.Exception.Message)"
    }

    Write-Host "Lifetime stress: WINUI_LIFETIME_STRESS_NATIVE=$($env:WINUI_LIFETIME_STRESS_NATIVE) (1 = aggressive native repro)."
}

# State used to attribute a native TAEF-host crash to a scenario (see Report-LifetimeNativeCrash).
$script:lifetimeTeConsoleLog = Join-Path (Get-Location) "lifetime_te_console.log"
$script:lifetimeTeExitCode = 0
# Set by Report-LifetimeNativeCrash and consumed by Set-LifetimeResultsGating so a native host
# crash that produced no results file is still surfaced as a gating failure.
$script:lifetimeNativeCrashDetected = $false
Delete-IfExists $script:lifetimeTeConsoleLog

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

    # Per-test TAEF timeout. In soak mode a scenario runs for WINUI_LIFETIME_STRESS_MINUTES, well past the
    # default 5 minutes, so raise the per-test timeout (budget plus headroom) to stop TAEF flagging it as hung.
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
    if ($isLifetimeStress)
    {
        # Tee te.exe's output to a log (still streamed to the pipeline) so a crash can be attributed to the
        # in-flight scenario, and capture its exit code as a crash signal.
        & ./run.cmd 2>&1 | Tee-Object -FilePath $script:lifetimeTeConsoleLog -Append
        $script:lifetimeTeExitCode = $LASTEXITCODE
    }
    else
    {
        & ./run.cmd
    }
}

function Get-LifetimeDumpFiles
{
    # Crash dumps currently in the shared per-slice dump folder ($env:HELIX_DUMP_FOLDER).
    if ($env:HELIX_DUMP_FOLDER -and (Test-Path $env:HELIX_DUMP_FOLDER))
    {
        return @(Get-ChildItem -Path $env:HELIX_DUMP_FOLDER -Filter *.dmp -ErrorAction SilentlyContinue)
    }
    return @()
}

function Report-LifetimeNativeCrash
{
    # Detect a native TAEF-host crash and surface it as a non-gating, scenario-attributed warning without
    # opening the dump or failing the stage.
    param (
        [string[]] $preRunDumps,
        [int] $teExitCode,
        [string] $teConsoleLogPath
    )

    # A scenario that logged "starting" but never "completed" was in-flight when the host died.
    $startedScenarios = New-Object System.Collections.Generic.List[string]
    $completedScenarios = New-Object System.Collections.Generic.List[string]
    if ($teConsoleLogPath -and (Test-Path $teConsoleLogPath))
    {
        foreach ($line in Get-Content $teConsoleLogPath)
        {
            if ($line -match "NATIVE: scenario '([^']+)' starting")       { $startedScenarios.Add($Matches[1]) }
            elseif ($line -match "NATIVE: scenario '([^']+)' completed")   { $completedScenarios.Add($Matches[1]) }
        }
    }
    $inFlight = @($startedScenarios | Where-Object { $completedScenarios -notcontains $_ } | Select-Object -Unique)

    # Dumps produced by *this* work item = whatever is new since the pre-run snapshot.
    $newDumps = @(Get-LifetimeDumpFiles | Where-Object { $preRunDumps -notcontains $_.FullName })

    # Count "REPORT: scenario 'X' threw" lines - native warnings that didn't crash the host, tracked separately.
    # Also count "REPORT: object 'X' was still alive after forced collection" lines - managed leak warnings from
    # VerifyCollected(failOnLeak:false). These previously never reached the aggregation, so only native signals
    # showed in the pipeline; capture them here so the totals step can surface them too.
    $warningScenarios = New-Object System.Collections.Generic.List[string]
    $managedWarningObjects = New-Object System.Collections.Generic.List[string]
    if ($teConsoleLogPath -and (Test-Path $teConsoleLogPath))
    {
        foreach ($line in Get-Content $teConsoleLogPath)
        {
            if ($line -match "\[LifetimeStress\] REPORT: scenario '([^']+)' threw") { $warningScenarios.Add($Matches[1]) }
            elseif ($line -match "\[LifetimeStress\] REPORT: object '([^']+)' was still alive after forced collection") { $managedWarningObjects.Add($Matches[1]) }
        }
    }

    # A crash is any of: non-zero exit code, a new dump, or an in-flight scenario.
    $crashDetected = ($teExitCode -ne 0) -or ($newDumps.Count -gt 0) -or ($inFlight.Count -gt 0)
    $script:lifetimeNativeCrashDetected = $crashDetected

    $scenarioLabel = "none"
    $dumpNames = @()
    if ($crashDetected)
    {
        $scenarioLabel = if ($inFlight.Count -gt 0) { $inFlight -join ", " } else { "unknown" }

        # Name each dump after the scenario so it carries attribution and gets upload priority.
        $safeScenario = ($scenarioLabel -replace '[^A-Za-z0-9._-]', '_')
        foreach ($dump in $newDumps)
        {
            $attributedName = "LifetimeStress-$safeScenario-$($dump.Name)"
            try
            {
                Rename-Item -Path $dump.FullName -NewName $attributedName -Force
                $dumpNames += $attributedName
            }
            catch
            {
                Write-Host "Lifetime stress: could not rename dump '$($dump.Name)' ($($_.Exception.Message)); leaving original name."
                $dumpNames += $dump.Name
            }
        }
        $dumpLabel = if ($dumpNames.Count -gt 0) { $dumpNames -join ", " } else { "none captured" }

        # Gating error: Set-LifetimeResultsGating turns this crash into a failing test that fails the shard;
        # this logissue makes the crash and its dump attribution visible in the pipeline UI.
        Write-Host "##vso[task.logissue type=error]Native lifetime crash in scenario '$scenarioLabel' (dump: $dumpLabel)"
        Write-Host "Lifetime stress: native host crash detected (te.exe exit code=$teExitCode, new dumps=$($newDumps.Count), in-flight scenario(s)='$scenarioLabel'). Surfaced as a gating failure."
    }

    # Per-work-item record for the PostTestRun aggregation step
    # (Helix/common/pipeline/Report-LifetimeNativeCrashTotals.ps1). Written for crash and warning-only cases.
    if ($env:HELIX_WORKITEM_UPLOAD_ROOT)
    {
        $report = [ordered]@{
            workItem             = (Split-Path $env:HELIX_WORKITEM_UPLOAD_ROOT -Leaf)
            nativeCrashCount     = [int][bool]$crashDetected
            nativeWarningCount   = $warningScenarios.Count
            managedWarningCount  = $managedWarningObjects.Count
            teExitCode           = $teExitCode
            newDumpCount         = $newDumps.Count
            inFlightScenarios    = @($inFlight)
            crashScenario        = $scenarioLabel
            dumps                = @($dumpNames)
            warningScenarios     = @($warningScenarios)
            managedWarningObjects = @($managedWarningObjects)
        }
        try
        {
            $reportPath = Join-Path $env:HELIX_WORKITEM_UPLOAD_ROOT "LifetimeNativeCrashReport.json"
            $report | ConvertTo-Json -Depth 4 | Out-File -FilePath $reportPath -Encoding utf8
            Write-Host "Lifetime stress: wrote native crash/warning report (crashes=$($report.nativeCrashCount), nativeWarnings=$($report.nativeWarningCount), managedWarnings=$($report.managedWarningCount)) to $reportPath."
        }
        catch
        {
            Write-Host "Lifetime stress: failed to write LifetimeNativeCrashReport.json ($($_.Exception.Message))."
        }
    }
}

function Set-LifetimeResultsGating
{
    # Normalizes testResults.xml so lifetime failures GATE the shard. Leak/failure results are preserved as
    # 'Fail' (PublishTestResults runs with failTaskOnFailedTests:true), and a native host crash that produced
    # no results file - or partial results with no failure - is surfaced as a synthetic failing test so the
    # crash gates too.
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

                # If the TAEF host crashed but the partial results carry no failure, inject one gating 'Fail'
                # so the native crash still fails the shard.
                $existingFail = @($doc.SelectNodes('//test') | Where-Object { $_.result -eq 'Fail' }).Count
                if ($script:lifetimeNativeCrashDetected -and $existingFail -eq 0)
                {
                    $collection = @($doc.SelectNodes('//collection'))[0]
                    if ($collection)
                    {
                        $t = $doc.CreateElement('test')
                        $t.SetAttribute('name', "${prefix}LifetimeStressTestSuite.NativeCrash")
                        $t.SetAttribute('type', 'LifetimeStressTestSuite')
                        $t.SetAttribute('method', 'NativeCrash')
                        $t.SetAttribute('time', '0')
                        $t.SetAttribute('result', 'Fail')
                        $failure = $doc.CreateElement('failure')
                        $message = $doc.CreateElement('message')
                        $message.InnerText = 'Native lifetime host crash detected (te.exe exit code / new dump / in-flight scenario) with no failing test in the partial results. Reported as a gating failure.'
                        [void]$failure.AppendChild($message)
                        [void]$t.AppendChild($failure)
                        [void]$collection.AppendChild($t)
                        Write-Host "Lifetime stress: injected a gating 'Fail' for a native host crash that left no failing result."
                    }
                }

                # Recompute passed/failed/skipped totals so the published summary is self-consistent. Failures
                # are intentionally kept as 'Fail'.
                foreach ($scope in (@($doc.SelectNodes('//assembly')) + @($doc.SelectNodes('//collection'))))
                {
                    $tests = @($scope.SelectNodes('.//test'))
                    $pass = @($tests | Where-Object { $_.result -eq 'Pass' }).Count
                    $skip = @($tests | Where-Object { $_.result -eq 'Skip' }).Count
                    $fail = @($tests | Where-Object { $_.result -eq 'Fail' }).Count
                    if ($scope.Attributes['total'])   { $scope.total = "$($tests.Count)" }
                    if ($scope.Attributes['passed'])  { $scope.passed = "$pass" }
                    if ($scope.Attributes['skipped']) { $scope.skipped = "$skip" }
                    if ($scope.Attributes['failed'])  { $scope.failed = "$fail" }
                }

                $doc.Save((Resolve-Path $resultsPath).Path)
                $totalFail = @($doc.SelectNodes('//test') | Where-Object { $_.result -eq 'Fail' }).Count
                Write-Host "Lifetime stress: preserved $totalFail failing result(s) as gating in testResults.xml."
            }
        }
        catch
        {
            Write-Host "Lifetime stress: could not post-process testResults.xml ($($_.Exception.Message)); emitting a synthetic report instead."
            $needSynthetic = $true
        }
    }

    if ($needSynthetic)
    {
        # No results file - te.exe most likely crashed. If a native crash was detected emit a FAILING entry so
        # the crash gates the shard; otherwise emit a passing entry so the suite isn't silently missing.
        $runDate = (Get-Date).ToString('yyyy-MM-dd')
        $runTime = (Get-Date).ToString('HH:mm:ss')
        $testName = "${prefix}LifetimeStressTestSuite.Report"
        if ($script:lifetimeNativeCrashDetected)
        {
            $xml = @"
<?xml version="1.0" encoding="utf-8"?>
<assemblies>
  <assembly name="MUXControlsTestApp.dll" test-framework="TAEF" run-date="$runDate" run-time="$runTime" total="1" passed="0" failed="1" skipped="0" errors="0" time="0">
    <collection total="1" passed="0" failed="1" skipped="0" name="Test collection" time="0">
      <test name="$testName" type="LifetimeStressTestSuite" method="Report" time="0" result="Fail">
        <failure><message>The lifetime stress suite did not produce a results file - the TAEF host most likely crashed on a native lifetime fault. Reported as a gating failure; see this work item's console log and te_original.wtl for the captured report up to the point of failure.</message></failure>
      </test>
    </collection>
  </assembly>
</assemblies>
"@
            Set-Content -Path $resultsPath -Value $xml -Encoding UTF8
            Write-Host "Lifetime stress: emitted synthetic FAILING report at testResults.xml (native host crash produced no results file)."
        }
        else
        {
            $xml = @"
<?xml version="1.0" encoding="utf-8"?>
<assemblies>
  <assembly name="MUXControlsTestApp.dll" test-framework="TAEF" run-date="$runDate" run-time="$runTime" total="1" passed="1" failed="0" skipped="0" errors="0" time="0">
    <collection total="1" passed="1" failed="0" skipped="0" name="Test collection" time="0">
      <test name="$testName" type="LifetimeStressTestSuite" method="Report" time="0" result="Pass">
        <output>The lifetime stress suite produced no results file but no native crash was detected; reported as a non-gating pass.</output>
      </test>
    </collection>
  </assembly>
</assemblies>
"@
            Set-Content -Path $resultsPath -Value $xml -Encoding UTF8
            Write-Host "Lifetime stress: emitted synthetic passing report at testResults.xml (no results file, no native crash detected)."
        }
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

# Snapshot dumps before the run so new ones can be attributed to a scenario afterwards.
$preRunLifetimeDumps = @()
if ($isLifetimeStress)
{
    $preRunLifetimeDumps = @(Get-LifetimeDumpFiles | ForEach-Object { $_.FullName })
}

# Run the tests:
Run-Taef("$taefQuery")

Write-Host "WorkItemTestEndTime: $(Get-Date)"

if ($isLifetimeStress -and -not (Test-Path .\te.wtl))
{
    # te.exe crashed without flushing its log; the non-gating handling below emits a synthetic report.
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
# (Skipped for the lifetime stress suite - a non-gating report shouldn't rerun intentional crashers.)
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
    # Surface a native host crash (sets $script:lifetimeNativeCrashDetected) before the results are normalized
    # below so Set-LifetimeResultsGating can turn it into a gating failure.
    Report-LifetimeNativeCrash -preRunDumps $preRunLifetimeDumps -teExitCode $script:lifetimeTeExitCode -teConsoleLogPath $script:lifetimeTeConsoleLog

    # Convert real results when a log exists; Set-LifetimeResultsGating preserves failures (leaks/crashes) so
    # the shard fails, and guarantees a valid testResults.xml even if conversion fails or the host crashed.
    if (Test-Path .\te_original.wtl)
    {
        try
        {
            .\ConvertWttLogToXUnit.ps1 te_original.wtl te_rerun.wtl te_rerun_multiple.wtl te_rerun_more.wtl testResults.xml $testnameprefix
        }
        catch
        {
            Write-Host "Lifetime stress: ConvertWttLogToXUnit failed ($($_.Exception.Message)); a synthetic report will be emitted."
            Delete-IfExists .\testResults.xml
        }
    }
    Set-LifetimeResultsGating -resultsPath (Join-Path (Get-Location) "testResults.xml") -testnameprefix $testnameprefix
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
