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

. .\TaefHangWatchdog.ps1

# Time limit for each te.exe run, in minutes (0 means none). RunTestPassSliceOnBuildAgent.ps1 sets it for pipeline runs.
[double]$teHangTimeoutMinutes = 0
[void][double]::TryParse($env:WINUI_TE_HANG_TIMEOUT_MINUTES, [Globalization.NumberStyles]::Float, [Globalization.CultureInfo]::InvariantCulture, [ref]$teHangTimeoutMinutes)

# The hang watchdog's report, if it had to end a te.exe run.
$script:teHangReport = $null

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
Delete-IfExists $script:lifetimeTeConsoleLog

# te.exe's console output from the latest run, and a record of each run's exit code and te.wtl state (see Repair-TaefLog).
$script:teConsoleLog = Join-Path (Get-Location) "te_console.log"
$script:taefRuns = @()
Delete-IfExists $script:teConsoleLog

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
    param ([string] $taefAdditionalParams, [string] $runName = "original")

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
    Out-File -FilePath "run.cmd" -Encoding ascii -InputObject "@$teCommand"
    $preRunDumps = @(Get-CrashDumpFiles | ForEach-Object { $_.FullName })

    # Soak runs are long by design, so they don't get the hang time limit.
    $useHangWatchdog = ($teHangTimeoutMinutes -gt 0) -and ($soakMinutes -le 0) -and $env:HELIX_WORKITEM_UPLOAD_ROOT
    if ($useHangWatchdog)
    {
        Start-TaefHangWatchdog -TimeoutMinutes $teHangTimeoutMinutes -DumpFolder $env:HELIX_WORKITEM_UPLOAD_ROOT
    }

    try
    {
        if ($isLifetimeStress)
        {
            # Tee te.exe's output to a log (still streamed to the pipeline) so a crash can be attributed to the
            # in-flight scenario, and capture its exit code as a crash signal.
            & ./run.cmd 2>&1 | Tee-Object -FilePath $script:lifetimeTeConsoleLog -Append
            $teExitCode = $LASTEXITCODE
            $script:lifetimeTeExitCode = $teExitCode
        }
        else
        {
            # Keep te.exe's output (still streamed to the pipeline): if te.exe exits before it finishes writing
            # te.wtl, Repair-TaefLog takes the missing results from it.
            & ./run.cmd | Tee-Object -FilePath $script:teConsoleLog
            $teExitCode = $LASTEXITCODE
        }
    }
    finally
    {
        Remove-Item -LiteralPath ".\run.cmd" -Force
        if ($useHangWatchdog)
        {
            $hangReport = Stop-TaefHangWatchdog
            if ($hangReport)
            {
                $script:teHangReport = $hangReport
            }
        }
    }

    Write-Host ("te.exe exited with code {0} (0x{0:X8})." -f $teExitCode)

    if (-not $isLifetimeStress)
    {
        Repair-TaefLog -runName $runName -teExitCode $teExitCode -preRunDumps $preRunDumps
    }
}

function Get-CrashDumpFiles
{
    # Crash dumps currently in the shared per-slice dump folder ($env:HELIX_DUMP_FOLDER).
    if ($env:HELIX_DUMP_FOLDER -and (Test-Path $env:HELIX_DUMP_FOLDER))
    {
        return @(Get-ChildItem -Path $env:HELIX_DUMP_FOLDER -Filter *.dmp -ErrorAction SilentlyContinue)
    }
    return @()
}

function Move-TaefCrashDumps
{
    # Move te.exe crash dumps written during this run into the work item's upload folder, so they're uploaded with
    # its logs even if later work items fill the shared dump folder.
    param ([string[]] $preRunDumps)

    $movedDumps = @()
    $newDumps = @(Get-CrashDumpFiles | Where-Object { ($_.Name -like 'te.exe.*') -and ($preRunDumps -notcontains $_.FullName) })
    foreach ($dump in $newDumps)
    {
        try
        {
            Move-Item -LiteralPath $dump.FullName -Destination $env:HELIX_WORKITEM_UPLOAD_ROOT -Force -ErrorAction Stop
            $movedDumps += $dump.Name
        }
        catch
        {
            Write-Host "Could not move crash dump '$($dump.Name)' to the work item's upload folder: $($_.Exception.Message)"
        }
    }

    return $movedDumps
}

function Test-SelectedTestsRan
{
    # te.exe didn't finish te.wtl, so the recovered log only has the tests te.exe got to: if te.exe exited between two
    # tests, nothing was in flight, and the tests after them never ran. Ask te.exe to list the work item's selection and
    # check that each selected test has a result. Fills in the selection fields of the run's record.
    param ([string] $wttPath, [System.Collections.IDictionary] $runRecord)

    $listCommand = "te.exe $testBinaries /list /unicodeOutput:false $taefParameters $taefQuery"
    Write-Host $listCommand
    Out-File -FilePath "list.cmd" -Encoding ascii -InputObject "@$listCommand"
    try
    {
        $listOutput = @(& ./list.cmd | ForEach-Object { "$_" })
        $listExitCode = $LASTEXITCODE
    }
    finally
    {
        Remove-Item -LiteralPath ".\list.cmd" -Force
    }

    # te.exe /list prints each test indented by 16 spaces, under its module and class.
    $selectedTests = @($listOutput | Where-Object { $_ -match '^ {16}\S' } | ForEach-Object { $_.Trim() })
    $runRecord.selectedTests = $selectedTests.Count
    if (($listExitCode -ne 0) -or ($selectedTests.Count -eq 0))
    {
        $listOutput | ForEach-Object { Write-Host $_ }
        $runRecord.selectionCheck = "Unverified"
        $runRecord.selectionCheckFailure = "te.exe exited before it finished te.wtl, and the selected tests couldn't be listed to check that they all ran (te.exe /list exited with code $listExitCode and listed $($selectedTests.Count) tests)."
        return
    }

    $check = [HelixTestHelpers.WttLogRecovery]::CheckSelectedTestsRan($wttPath, [string[]]$selectedTests, [string[]]@($runRecord.inFlightTests))
    $runRecord.testsWithoutResults = @($check.TestsWithoutResults)
    $runRecord.dataDrivenTestsCutShort = @($check.DataDrivenTestsCutShort)
    if ($check.AllSelectedTestsRan)
    {
        $runRecord.selectionCheck = "AllRan"
        return
    }

    if ($check.TestsWithoutResults.Count -gt 0)
    {
        $shownTests = @($check.TestsWithoutResults | Select-Object -First 20) -join ', '
        if ($check.TestsWithoutResults.Count -gt 20)
        {
            $shownTests += ", and $($check.TestsWithoutResults.Count - 20) more"
        }

        $failure = "te.exe exited before it ran every selected test: $($check.TestsWithoutResults.Count) of the $($selectedTests.Count) selected tests never ran: $shownTests."
    }
    else
    {
        $failure = "te.exe may have exited before it ran every selected test."
    }

    if ($check.DataDrivenTestsCutShort.Count -eq 1)
    {
        $failure += " It was running, or had just run, the data-driven test $($check.DataDrivenTestsCutShort[0]), so some of that test's variations may not have run."
    }
    elseif ($check.DataDrivenTestsCutShort.Count -gt 1)
    {
        $failure += " It was running, or had just run, the data-driven tests $($check.DataDrivenTestsCutShort -join ', '), so some of their variations may not have run."
    }

    $runRecord.selectionCheck = "NotAllRan"
    $runRecord.selectionCheckFailure = $failure
}

function Add-TaefRunIncompleteResult
{
    # Fails a work item whose first te.exe run didn't run every selected test. The results it did get are still
    # reported, alongside a failed <work item>.TaefRunIncomplete test that says what's missing.
    param ([string] $resultsPath, [string] $testnameprefix, [string] $message)

    $prefix = ""
    if ($testnameprefix) { $prefix = "$testnameprefix." }
    $workItemName = Split-Path $env:HELIX_WORKITEM_UPLOAD_ROOT -Leaf
    $resultsPath = (Resolve-Path -LiteralPath $resultsPath).Path

    # XmlDocument.Load reads the converter's UTF-8 output correctly; Get-Content would use the ANSI code page.
    $doc = New-Object System.Xml.XmlDocument
    $doc.Load($resultsPath)
    $test = $doc.CreateElement("test")
    $test.SetAttribute("name", "$prefix$workItemName.TaefRunIncomplete")
    $test.SetAttribute("type", $workItemName)
    $test.SetAttribute("method", "TaefRunIncomplete")
    $test.SetAttribute("time", "0")
    $test.SetAttribute("result", "Fail")
    $failure = $doc.CreateElement("failure")
    $failureMessage = $doc.CreateElement("message")
    [void]$failureMessage.AppendChild($doc.CreateCDataSection($message))
    [void]$failure.AppendChild($failureMessage)
    [void]$test.AppendChild($failure)

    $collection = $doc.SelectSingleNode('//collection')
    [void]$collection.AppendChild($test)
    foreach ($scope in @($doc.SelectSingleNode('//assembly'), $collection))
    {
        $scope.SetAttribute("total", [string]([int]$scope.GetAttribute("total") + 1))
        $scope.SetAttribute("failed", [string]([int]$scope.GetAttribute("failed") + 1))
    }

    $doc.Save($resultsPath)
}

function Repair-TaefLog
{
    # If te.exe exits before it finishes writing te.wtl, the log can't be read. Rebuild it from the entries te.exe did
    # write plus the results it printed (see WttLogRecovery in HelixTestHelpers.cs), so the work item still reports
    # results. Records the run in TaefRuns.json, which RunTestPassSliceOnBuildAgent.ps1 reads to explain a work item
    # that has no results or didn't run every selected test.
    param ([string] $runName, [int] $teExitCode, [string[]] $preRunDumps)

    $workItemName = Split-Path $env:HELIX_WORKITEM_UPLOAD_ROOT -Leaf
    $exitCodeHex = "0x{0:X8}" -f $teExitCode
    $runRecord = [ordered]@{
        run                         = $runName
        teExitCode                  = $teExitCode
        teExitCodeHex               = $exitCodeHex
        logState                    = "NotChecked"
        logReadError                = $null
        resultsRecoveredFromLog     = 0
        resultsRecoveredFromConsole = 0
        inFlightTests               = @()
        dumps                       = @()
        selectionCheck              = "NotNeeded"
        selectedTests               = 0
        testsWithoutResults         = @()
        dataDrivenTestsCutShort     = @()
        selectionCheckFailure       = $null
        endedByHangWatchdog         = [bool]$script:teHangReport
    }

    $recovery = $null
    try
    {
        $recovery = [HelixTestHelpers.WttLogRecovery]::RecoverIncompleteLog(
            (Join-Path (Get-Location) "te.wtl"),
            $script:teConsoleLog,
            (Join-Path $env:HELIX_WORKITEM_UPLOAD_ROOT "te_$runName.incomplete.wtl"))
        $runRecord.logState = "$($recovery.State)"
        $runRecord.logReadError = $recovery.ReadError
        $runRecord.resultsRecoveredFromLog = $recovery.TestsFromLog
        $runRecord.resultsRecoveredFromConsole = $recovery.TestsFromConsole
        $runRecord.inFlightTests = @($recovery.InFlightTests)
    }
    catch
    {
        Write-Host "Could not check te.wtl from te.exe run '$runName': $($_.Exception.Message)"
    }

    # A recovered first run is only complete if it ran every selected test. Reruns don't need the check: a test that's
    # missing from a rerun stays failed. Nor does a run the hang watchdog ended: its failed TaefHang test says why.
    if (($runName -eq "original") -and -not $runRecord.endedByHangWatchdog -and ($null -ne $recovery) -and ($recovery.State -eq [HelixTestHelpers.WttLogState]::Recovered))
    {
        try
        {
            Test-SelectedTestsRan -wttPath (Join-Path (Get-Location) "te.wtl") -runRecord $runRecord
        }
        catch
        {
            $runRecord.selectionCheck = "Unverified"
            $runRecord.selectionCheckFailure = "te.exe exited before it finished te.wtl, and checking that it ran every selected test failed: $($_.Exception.Message)"
        }
    }

    # te.exe's exit code is the number of failed tests, so only a negative code (an NTSTATUS or HRESULT) means it crashed.
    $logIncomplete = ($null -ne $recovery) -and ($recovery.State -ne [HelixTestHelpers.WttLogState]::Complete)
    if ($logIncomplete -or ($teExitCode -lt 0))
    {
        $runRecord.dumps = @(Move-TaefCrashDumps -preRunDumps $preRunDumps)
        if ($runRecord.dumps.Count -gt 0)
        {
            $dumpText = "Uploaded te.exe crash dump(s) with the work item: $($runRecord.dumps -join ', ')."
        }
        elseif ($runRecord.endedByHangWatchdog)
        {
            $dumpText = "The hang watchdog's report, in TaefHang.txt, lists the dumps it wrote."
        }
        else
        {
            $dumpText = "No new te.exe crash dump was found in '$($env:HELIX_DUMP_FOLDER)'."
        }

        $exitText = "exited abnormally"
        if ($runRecord.endedByHangWatchdog)
        {
            $exitText = "was ended by the hang watchdog"
        }

        $where = "in work item '$workItemName' (te.exe run '$runName')"
        if (-not $logIncomplete)
        {
            Write-Host "##vso[task.logissue type=warning]te.exe $exitText (exit code $exitCodeHex) after it finished writing te.wtl, $where. $dumpText"
        }
        else
        {
            if ($recovery.LogFileExisted)
            {
                $logText = "te.wtl is incomplete ($($recovery.ReadError))"
            }
            else
            {
                $logText = "te.exe didn't create te.wtl"
            }

            if ($recovery.State -eq [HelixTestHelpers.WttLogState]::Recovered)
            {
                $recoveredText = "$($recovery.TestsFromLog) from te.wtl and $($recovery.TestsFromConsole) from te.exe's console output"
                if ($recovery.InFlightTests.Count -gt 0)
                {
                    $inFlightTests = $recovery.InFlightTests -join ', '
                    $when = "while running $inFlightTests"
                    if ($recovery.InFlightTests.Count -gt 1)
                    {
                        $recoveredText += ", and $inFlightTests are reported as failed"
                    }
                    else
                    {
                        $recoveredText += ", and $inFlightTests is reported as failed"
                    }
                }
                elseif ($runRecord.selectionCheck -eq "AllRan")
                {
                    $when = "after its last test finished"
                }
                else
                {
                    $when = "while no test was running"
                }

                $selectionText = ""
                if ($runRecord.selectionCheck -eq "AllRan")
                {
                    $selectionText = " All $($runRecord.selectedTests) selected tests have results."
                }
                elseif ($runRecord.selectionCheckFailure)
                {
                    $selectionText = " $($runRecord.selectionCheckFailure) The work item fails."
                }

                Write-Host "##vso[task.logissue type=warning]te.exe $exitText (exit code $exitCodeHex) $when, $where. $logText, so the results were recovered: $recoveredText.$selectionText $dumpText"
            }
            else
            {
                Write-Host "te.exe $exitText (exit code $exitCodeHex) $where. $logText, and te.exe reported no test results, so there are none to recover. $dumpText"
            }
        }
    }

    $script:taefRuns += [pscustomobject]$runRecord
    try
    {
        ConvertTo-Json -InputObject @($script:taefRuns) -Depth 3 | Out-File -FilePath (Join-Path $env:HELIX_WORKITEM_UPLOAD_ROOT "TaefRuns.json") -Encoding utf8
    }
    catch
    {
        Write-Host "Could not write TaefRuns.json: $($_.Exception.Message)"
    }
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
    $newDumps = @(Get-CrashDumpFiles | Where-Object { $preRunDumps -notcontains $_.FullName })

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

        # Non-gating warning: visible in the pipeline UI but doesn't fail the stage.
        Write-Host "##vso[task.logissue type=warning]Native lifetime crash in scenario '$scenarioLabel' (dump: $dumpLabel)"
        Write-Host "Lifetime stress: native host crash detected (te.exe exit code=$teExitCode, new dumps=$($newDumps.Count), in-flight scenario(s)='$scenarioLabel'). Emitted a non-gating warning; the stage stays green."
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

function Set-LifetimeResultsNonGating
{
    # Guarantees a testResults.xml with zero failures so the suite reports but never fails the pipeline.
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
                        # Downgrade to non-gating 'Skip' (PublishTestResults only fails on Failed tests).
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
        # No results file - te.exe likely crashed. Emit one passing entry so the suite isn't silently missing.
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

function Save-HungTaefLog
{
    # Keeps the incomplete log of a te.exe run that the hang watchdog ended, under a name the results conversion
    # doesn't read.
    param ([string] $name)

    if (Test-Path .\te.wtl)
    {
        Move-Item .\te.wtl $name -Force
        Copy-Item ".\$name" $env:HELIX_WORKITEM_UPLOAD_ROOT -Force
    }
}

function Add-TaefHangResult
{
    # Records the hang as a failed test in testResults.xml, alongside any results that could still be converted.
    param ([string] $resultsPath, [string] $testnameprefix, [string[]] $report)

    $prefix = ""
    if ($testnameprefix) { $prefix = "$testnameprefix." }
    $workItemName = Split-Path $env:HELIX_WORKITEM_UPLOAD_ROOT -Leaf

    $doc = $null
    if (Test-Path $resultsPath)
    {
        [xml]$doc = Get-Content $resultsPath -Raw
    }

    if (-not $doc -or -not $doc.assemblies)
    {
        $runDate = (Get-Date).ToString('yyyy-MM-dd')
        $runTime = (Get-Date).ToString('HH:mm:ss')
        [xml]$doc = @"
<assemblies>
  <assembly name="MUXControls.Test.dll" test-framework="TAEF" run-date="$runDate" run-time="$runTime" total="0" passed="0" failed="0" skipped="0" time="0" errors="0">
    <collection total="0" passed="0" failed="0" skipped="0" name="Test collection" time="0" />
  </assembly>
</assemblies>
"@
    }

    $test = $doc.CreateElement("test")
    $test.SetAttribute("name", "$prefix$workItemName.TaefHang")
    $test.SetAttribute("type", $workItemName)
    $test.SetAttribute("method", "TaefHang")
    $test.SetAttribute("time", "0")
    $test.SetAttribute("result", "Fail")
    $failure = $doc.CreateElement("failure")
    $message = $doc.CreateElement("message")
    [void]$message.AppendChild($doc.CreateCDataSection(($report -join "`n")))
    [void]$failure.AppendChild($message)
    [void]$test.AppendChild($failure)

    $collection = $doc.SelectSingleNode('//collection')
    [void]$collection.AppendChild($test)
    foreach ($scope in @($doc.SelectSingleNode('//assembly'), $collection))
    {
        $scope.SetAttribute("total", [string]([int]$scope.GetAttribute("total") + 1))
        $scope.SetAttribute("failed", [string]([int]$scope.GetAttribute("failed") + 1))
    }

    $doc.Save($resultsPath)
}

Write-Host "WorkItemSetupStartTime: $(Get-Date)"

Write-Host "Run Setup If Needed"
.\RunSetupIfNeeded.ps1

# Dump a list of installed AppxPackages to a text file. This can be useful for debugging purposes.
Write-Host "(Skipping Get-AppxPackge dump to save time)"
#Get-AppxPackage | Out-File -FilePath (Join-Path $env:HELIX_WORKITEM_UPLOAD_ROOT "appxpackages.txt" )

.\TestPass-PreRunCore.ps1

# Loaded before the first run because Run-Taef uses it to recover te.wtl if te.exe exits abnormally.
Add-Type -Language CSharp -ReferencedAssemblies System.Xml,System.Xml.Linq,System.Runtime.Serialization,System.Runtime.Serialization.Json (Get-Content .\HelixTestHelpers.cs -Raw)

Write-Host "WorkItemTestStartTime: $(Get-Date)"

# Snapshot dumps before the run so new ones can be attributed to a scenario afterwards.
$preRunLifetimeDumps = @()
if ($isLifetimeStress)
{
    $preRunLifetimeDumps = @(Get-CrashDumpFiles | ForEach-Object { $_.FullName })
}

# Run the tests:
Run-Taef("$taefQuery")

Write-Host "WorkItemTestEndTime: $(Get-Date)"

if ($isLifetimeStress -and -not (Test-Path .\te.wtl))
{
    # te.exe crashed without flushing its log; the non-gating handling below emits a synthetic report.
    Write-Host "Lifetime stress: te.wtl was not produced (TAEF host likely crashed on a native lifetime fault)."
}
elseif (Test-Path .\te.wtl)
{
    Move-Item .\te.wtl te_original.wtl -Force
    Copy-Item .\te_original.wtl $env:HELIX_WORKITEM_UPLOAD_ROOT -Force
}
elseif ($script:teHangReport)
{
    Write-Host "te.exe was ended by the hang watchdog, and it left no readable te.wtl."
}
Copy-Screenshots
Copy-IfExists .\*.pgc $env:HELIX_WORKITEM_UPLOAD_ROOT
Copy-MasterFiles

$failedTestQuery = $null
# A run the hang watchdog ended has an incomplete log, so it isn't searched for failed tests to rerun.
if (-not $script:teHangReport -and (Test-Path (Join-Path (Get-Location) "te_original.wtl")))
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

    Run-Taef -taefAdditionalParams "/select:`"$failedTestQuery`"" -runName "rerun"

    Write-Host "WorkItemTestRerunEndTime: $(Get-Date)"

    if ($script:teHangReport)
    {
        # The hang watchdog ended the rerun, so its log is incomplete: keep it for diagnosis, but don't parse it.
        Save-HungTaefLog te_rerun_hung.wtl
        Copy-Screenshots
        Copy-MasterFiles
        $failedTestQuery = $null
    }
    else
    {
        if (Test-Path .\te.wtl)
        {
            Move-Item .\te.wtl te_rerun.wtl -Force
            Copy-Item .\te_rerun.wtl $env:HELIX_WORKITEM_UPLOAD_ROOT -Force
        }
        Copy-Screenshots
        Copy-MasterFiles

        # Without a rerun log, the loop below reruns the tests that failed the first time.
        if (Test-Path (Join-Path (Get-Location) "te_rerun.wtl"))
        {
            $failedTestQuery = [HelixTestHelpers.FailedTestDetector]::GetFailedTestQuery((Join-Path (Get-Location) "te_rerun.wtl"))
        }
        Write-Host "failedTestQuery = $failedTestQuery"
    }
}


# If there are still failing tests remaining, we'll run them nine more times, so they'll have been run a total of 11 times.
# We determine if a test is reported as 'failed' or 'unreliable' by comparing the number of passes to rerunPassesRequiredToAvoidFailure
# which is specified by the Pipeline.
# (Skipped for the lifetime stress suite - see the note on the first rerun block above.)
if ($failedTestQuery -and $rerunFailed -and -not $isLifetimeStress)
{
    Write-Host "WorkItemTestLoopStartTime: $(Get-Date)"

    Run-Taef -taefAdditionalParams "/testmode:Loop /Loop:9 /LoopTest:1 /isolationlevel:test /select:`"$failedTestQuery`"" -runName "rerun_multiple"
    if ($script:teHangReport)
    {
        Save-HungTaefLog te_rerun_multiple_hung.wtl
    }
    elseif (Test-Path .\te.wtl)
    {
        Move-Item .\te.wtl te_rerun_multiple.wtl -Force
        Copy-Item .\te_rerun_multiple.wtl $env:HELIX_WORKITEM_UPLOAD_ROOT -Force
    }

    Copy-Screenshots
    Copy-MasterFiles

    Write-Host "WorkItemTestLoopEndTime: $(Get-Date)"
}

if ($isLifetimeStress)
{
    # Surface a native host crash as a non-gating warning before the results are normalized below.
    Report-LifetimeNativeCrash -preRunDumps $preRunLifetimeDumps -teExitCode $script:lifetimeTeExitCode -teConsoleLogPath $script:lifetimeTeConsoleLog

    # Convert real results when a log exists; Set-LifetimeResultsNonGating guarantees a valid zero-failure
    # testResults.xml even if conversion fails or the host crashed.
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
elseif ($script:teHangReport)
{
    # Keep whatever results can still be read, then add the hang as a failed test. TaefHang.txt tells
    # RunTestPassSliceOnBuildAgent.ps1 to fail the job once its other work items have run.
    if (Test-Path .\te_original.wtl)
    {
        try
        {
            .\ConvertWttLogToXUnit.ps1 te_original.wtl te_rerun.wtl te_rerun_multiple.wtl te_rerun_more.wtl testResults.xml $testnameprefix
        }
        catch
        {
            Write-Host "Couldn't convert te_original.wtl after the hang ($($_.Exception.Message)); reporting only the hang."
            Delete-IfExists .\testResults.xml
        }
    }

    Add-TaefHangResult -resultsPath (Join-Path (Get-Location) "testResults.xml") -testnameprefix $testnameprefix -report $script:teHangReport
    Set-Content -LiteralPath (Join-Path $env:HELIX_WORKITEM_UPLOAD_ROOT "TaefHang.txt") -Value $script:teHangReport
}
elseif (Test-Path .\te_original.wtl)
{
    .\ConvertWttLogToXUnit.ps1 te_original.wtl te_rerun.wtl te_rerun_multiple.wtl te_rerun_more.wtl testResults.xml $testnameprefix
}
else
{
    Write-Host "There's no readable te_original.wtl, so this work item has no test results."
}

# A first run that te.exe didn't finish can still be missing selected tests (see Test-SelectedTestsRan), even if a
# later rerun hung.
$originalRun = @($script:taefRuns | Where-Object { $_.run -eq "original" }) | Select-Object -First 1
if (-not $isLifetimeStress -and $originalRun -and $originalRun.selectionCheckFailure -and (Test-Path .\testResults.xml))
{
    Add-TaefRunIncompleteResult -resultsPath .\testResults.xml -testnameprefix $testnameprefix -message $originalRun.selectionCheckFailure
}

Copy-Item .\*_subresults.json $env:HELIX_WORKITEM_UPLOAD_ROOT -Force

.\TestPass-PostRunCore.ps1

# Display testResults.xml
if (Test-Path .\testResults.xml)
{
    Get-Content .\testResults.xml
}

if ((Join-Path $workingDir "") -ne (Join-Path $env:HELIX_CORRELATION_PAYLOAD ""))
{
    Copy-IfExists .\testResults.xml $workingDir
}

cd $workingDir

Write-Host "WorkItemEndTime: $(Get-Date)"
