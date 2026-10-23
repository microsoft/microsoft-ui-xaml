<#
.SYNOPSIS
    Step 3 - Run the selected tests on a Hyper-V VM with a code-coverage collector attached and
    download the resulting .coverage file.

.DESCRIPTION
    How the collector is wired up:
      1. Stage the VS code-coverage tools (CodeCoverage.Console + Dynamic Code Coverage Tools +
         InstrumentationEngine) on the VM once (C:\WinUICoverage\CoverageTool), no VS/dotnet needed.
      2. Start "Microsoft.CodeCoverage.Console.exe collect --session-id <sid> --server-mode --output <file>"
         as the logged-in VM user (interactive scheduled task - it must outlive the PS Direct session).
      3. Wait for \\.\pipe\CodeCoverage.pipe.<sid>, then give it a NULL DACL
         (lib\Set-CoveragePipeAcl.ps1) so low-integrity test processes can connect.
      4. Run the tests with the repo's own tools\run-tests-on-vm.ps1 -SkipPayload. -SkipPayload only
         skips regenerating TestPayload from BuildOutput (which would overwrite the instrumented
         DLLs); the runner still copies the changed payload files to the VM and runs runtests.cmd
         on the interactive desktop.
      5. "Microsoft.CodeCoverage.Console.exe shutdown <sid>" -> collector finalizes the .coverage.
      6. Copy the .coverage + collector logs back to <WorkDir>\raw (also when the run fails), and
         always close the PowerShell Direct session.

    Precondition: the VM user's desktop must be interactive. If the host is a Cloud PC / RDP session
    that can disconnect, use -MoveSessionToConsole so input does not depend on vmconnect's enhanced
    session (runs "tscon <id> /dest:console" as SYSTEM inside the VM).
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$RepoRoot,
    [Parameter(Mandatory)][string]$WorkDir,
    [Parameter(Mandatory)][string]$VMName,
    [Parameter(Mandatory)][string]$TestFilter,
    [string]$Flavor = 'amd64chk',
    [PSCredential]$Credential,
    [string]$RunLabel = '',
    [switch]$MoveSessionToConsole,
    [int]$PipeTimeoutSec = 90,
    [int]$FlushTimeoutSec = 600
)
$ErrorActionPreference = 'Stop'
function Info($m) { Write-Host "[vm-run] $m" }

$WorkDir = (Resolve-Path $WorkDir).Path
$state = Get-Content (Join-Path $WorkDir 'instrument-state.json') -Raw | ConvertFrom-Json
$sid = $state.sessionId
if (-not $RunLabel) { $RunLabel = Get-Date -Format 'yyyyMMdd-HHmmss' }
$rawDir = Join-Path $WorkDir "raw\$RunLabel"
New-Item -ItemType Directory -Force $rawDir | Out-Null

# Credential: same per-user DPAPI cache tools\run-tests-on-vm.ps1 uses.
if (-not $Credential) {
    $credFile = Join-Path $env:USERPROFILE ".winui-test\vmcred-$($VMName -replace '[^a-zA-Z0-9]','_').xml"
    if (-not (Test-Path $credFile)) { throw "No cached VM credential ($credFile). Run tools\run-tests-on-vm.ps1 once, or pass -Credential." }
    $Credential = Import-Clixml $credFile
}
$user = $Credential.UserName
$session = New-PSSession -VMName $VMName -Credential $Credential

$vmRoot = 'C:\WinUICoverage'
# Collector must be the same tool version that instrumented the binary (recorded by Step 2).
$hostTool = $state.coverageTool
if (-not $hostTool -or -not (Test-Path $hostTool)) {
    $hostTool = Get-ChildItem "${env:ProgramFiles}\Microsoft Visual Studio" -Recurse -Filter Microsoft.CodeCoverage.Console.exe -Depth 8 -EA SilentlyContinue |
        Sort-Object { [version]$_.VersionInfo.FileVersion } -Descending | Select-Object -First 1 -ExpandProperty FullName
}
$toolVer = (Get-Item $hostTool).VersionInfo.FileVersion
$vmToolDir = "$vmRoot\CoverageTool\$toolVer"
$vmTool = "$vmToolDir\Microsoft.CodeCoverage.Console.exe"
$vmOut = "$vmRoot\out\coverage-$RunLabel-$($state.moduleStem).coverage"
$vmLog = "$vmRoot\out\collector-$RunLabel.log"

try {
    # ---- interactive desktop check -----------------------------------------------------------
    $desk = Invoke-Command -Session $session -ScriptBlock {
        param($u)
        $rows = (qwinsta 2>$null) | Select-Object -Skip 1
        $mine = $rows | Where-Object { $_ -match "\s$u\s" } | Select-Object -First 1
        [pscustomobject]@{ line = "$mine"; onConsole = ("$mine" -match '^\s*>?console\s'); active = ("$mine" -match '\sActive') ;
                           id = if ("$mine" -match "\s$u\s+(\d+)\s") { [int]$Matches[1] } else { -1 } }
    } -ArgumentList $user
    Info "VM session: $($desk.line.Trim())"
    if (-not $desk.active) { throw "User $user has no Active session on $VMName. Log on to the VM desktop first." }
    if (-not $desk.onConsole) {
        if ($MoveSessionToConsole) {
            Info "moving session $($desk.id) to the VM console (tscon as SYSTEM)"
            Invoke-Command -Session $session -ScriptBlock {
                param($id)
                $a = New-ScheduledTaskAction -Execute "$env:windir\System32\tscon.exe" -Argument "$id /dest:console"
                $p = New-ScheduledTaskPrincipal -UserId 'SYSTEM' -LogonType ServiceAccount -RunLevel Highest
                Register-ScheduledTask -TaskName 'WinUICov-tscon' -Action $a -Principal $p -Force | Out-Null
                Start-ScheduledTask 'WinUICov-tscon'; Start-Sleep 5
                Unregister-ScheduledTask 'WinUICov-tscon' -Confirm:$false
            } -ArgumentList $desk.id
        } else {
            Write-Warning "User session is not on the VM console. If the host's vmconnect/RDP session disconnects, UI input fails ('Access is denied'). Re-run with -MoveSessionToConsole to avoid that."
        }
    }

    # ---- 1. stage collector tools (once) ------------------------------------------------------
    $hasTool = Invoke-Command -Session $session -ScriptBlock { param($t) Test-Path $t } -ArgumentList $vmTool
    if (-not $hasTool) {
        $cc = $hostTool
        $vsBase = $cc.Substring(0, $cc.IndexOf('\Common7\'))
        $stage = Join-Path $WorkDir "CoverageTool\$toolVer"
        Remove-Item $stage -Recurse -Force -EA SilentlyContinue
        New-Item -ItemType Directory -Force $stage, "$stage\amd64", "$stage\x64", "$stage\x86" | Out-Null
        Copy-Item "$vsBase\Common7\IDE\Extensions\Microsoft\CodeCoverage.Console\*" $stage -Recurse -Force
        $van = "$vsBase\Team Tools\Dynamic Code Coverage Tools"
        if (Test-Path $van) { Copy-Item "$van\*" $stage -Recurse -Force; if (Test-Path "$van\amd64") { Copy-Item "$van\amd64\*" "$stage\amd64" -Recurse -Force } }
        $ie = "$vsBase\Common7\IDE\CommonExtensions\Platform\InstrumentationEngine"
        foreach ($a in 'x64', 'x86') { if (Test-Path "$ie\$a") { Copy-Item "$ie\$a\*" "$stage\$a" -Recurse -Force } }
        $zip = Join-Path $WorkDir "CoverageTool-$toolVer.zip"
        Remove-Item $zip -EA SilentlyContinue
        Compress-Archive -Path "$stage\*" -DestinationPath $zip
        Invoke-Command -Session $session -ScriptBlock { param($r, $d) New-Item -ItemType Directory -Force $d, "$r\out" | Out-Null } -ArgumentList $vmRoot, $vmToolDir
        Copy-Item $zip "$vmRoot\CoverageTool.zip" -ToSession $session -Force
        Invoke-Command -Session $session -ScriptBlock { param($r, $d) Expand-Archive "$r\CoverageTool.zip" $d -Force; Remove-Item "$r\CoverageTool.zip" } -ArgumentList $vmRoot, $vmToolDir
        Info "staged collector tools ($toolVer) on VM: $vmTool"
    }
    Invoke-Command -Session $session -ScriptBlock { param($r) New-Item -ItemType Directory -Force "$r\out" | Out-Null } -ArgumentList $vmRoot
    Copy-Item (Join-Path $PSScriptRoot 'lib\Set-CoveragePipeAcl.ps1') "$vmRoot\Set-CoveragePipeAcl.ps1" -ToSession $session -Force

    # ---- 2. start the collector as the interactive user ---------------------------------------
    Invoke-Command -Session $session -ScriptBlock {
        param($tool, $sid, $out, $log, $u, $r)
        if (Test-Path "\\.\pipe\CodeCoverage.pipe.$sid") { throw "A collector for session $sid is already running on the VM. Run with a fresh session or shut it down: `"$tool`" shutdown $sid" }
        Remove-Item $out, $log -EA SilentlyContinue
        # Use a .cmd wrapper: "cmd /c" strips the outer quotes of a command line that starts and
        # ends with a quote, which silently breaks a quoted exe path + quoted redirections.
        $wrapper = "$r\collector-wrapper.cmd"
        "@echo off`r`n`"$tool`" collect --session-id $sid --server-mode --output `"$out`" > `"$log`" 2>&1" | Set-Content $wrapper -Encoding ASCII
        $a = New-ScheduledTaskAction -Execute 'cmd.exe' -Argument "/c `"$wrapper`""
        $p = New-ScheduledTaskPrincipal -UserId $u -LogonType Interactive -RunLevel Highest
        $s = New-ScheduledTaskSettingsSet -ExecutionTimeLimit ([TimeSpan]::Zero)
        Register-ScheduledTask -TaskName 'WinUICov-Collector' -Action $a -Principal $p -Settings $s -Force | Out-Null
        Start-ScheduledTask 'WinUICov-Collector'
    } -ArgumentList $vmTool, $sid, $vmOut, $vmLog, $user, $vmRoot
    Info "collector started (session $sid) -> $vmOut"

    # ---- 3. wait for the pipe, open its DACL ---------------------------------------------------
    $ready = Invoke-Command -Session $session -ScriptBlock {
        param($sid, $timeout, $log)
        $deadline = (Get-Date).AddSeconds($timeout)
        while ((Get-Date) -lt $deadline) {
            if (Test-Path "\\.\pipe\CodeCoverage.pipe.$sid") { return $true }
            if ((Get-ScheduledTask 'WinUICov-Collector').State -ne 'Running') { break }
            Start-Sleep -Milliseconds 500
        }
        Get-Content $log -EA SilentlyContinue | Select-Object -Last 20 | ForEach-Object { Write-Host "    collector: $_" }
        return $false
    } -ArgumentList $sid, $PipeTimeoutSec, $vmLog
    if (-not $ready) { throw "Collector pipe for session $sid did not appear on the VM." }
    Invoke-Command -Session $session -ScriptBlock {
        param($r, $sid) & powershell.exe -NoProfile -ExecutionPolicy Bypass -File "$r\Set-CoveragePipeAcl.ps1" -SessionId $sid
    } -ArgumentList $vmRoot, $sid | ForEach-Object { Info "  $_" }

    # ---- 4. run the tests with the repo's VM runner (payload already instrumented) -------------
    # run-tests-on-vm.ps1 deploys incrementally using a size+mtime manifest of what it copied last
    # time. Step 4 restores the VM without updating that manifest, and static_covrun*.dll keep the
    # tool's own timestamp, so a later run would skip them: the VM keeps the original DLL (empty
    # coverage) or gets the instrumented DLL without its runtime (the app fails to activate MUXC
    # types). Drop those entries so the instrumented files are always deployed.
    $instrumentedRel = @($state.copies.relativePath) + @($state.runtimesAdded)
    $manifestPattern = ".deploy-manifest-$($VMName -replace '[^a-zA-Z0-9]', '_')-*.json"
    foreach ($mf in Get-ChildItem (Join-Path $RepoRoot 'TestPayload') -Filter $manifestPattern -File -EA SilentlyContinue) {
        $entries = Get-Content $mf.FullName -Raw | ConvertFrom-Json
        $keep = [ordered]@{}
        foreach ($p in $entries.PSObject.Properties) {
            if ($instrumentedRel -notcontains $p.Name -and $p.Name -notmatch '(^|\\)static_covrun\w*\.dll$') { $keep[$p.Name] = $p.Value }
        }
        ($keep | ConvertTo-Json -Depth 1) | Set-Content $mf.FullName -Encoding UTF8
    }
    $testLog = Join-Path $rawDir 'vm-test.log'
    Info "running tests: run-tests-on-vm.ps1 -SkipPayload $TestFilter (log: $testLog)"
    Push-Location $RepoRoot
    try {
        & (Join-Path $RepoRoot 'initrun.ps1') -Flavor $Flavor .\tools\run-tests-on-vm.ps1 -VMName $VMName -SkipPayload $TestFilter *>&1 |
            Tee-Object -FilePath $testLog | Where-Object { "$_" -match 'Summary:|EndGroup:|Tests (PASSED|FAILED)|Error|\[\d/3\]' } | ForEach-Object { Write-Host "    $_" }
    } finally { Pop-Location }
    $summary = Select-String $testLog -Pattern 'Summary:\s+Total=' | Select-Object -Last 1
    if (-not $summary) {
        if (Select-String $testLog -Pattern 'selection criteria did not match any tests' -Quiet) {
            throw "No tests matched '$TestFilter' in the deployed payload. Check the filter, and that the build (and TestPayload) is current and contains the component's tests."
        }
        throw "The test run produced no TAEF summary; see $testLog."
    }
    Info "test summary: $($summary.Line.Trim())"
}
finally {
    # ---- 5. always flush: shut the collector down, wait for the file --------------------------
    try {
        $flush = Invoke-Command -Session $session -ScriptBlock {
            param($tool, $sid, $out, $timeout)
            $o = if (Test-Path "\\.\pipe\CodeCoverage.pipe.$sid") { & $tool shutdown $sid 2>&1 | Out-String } else { 'no collector pipe (collector not running)' }
            $deadline = (Get-Date).AddSeconds($timeout)
            do { Start-Sleep 3; $t = Get-ScheduledTask 'WinUICov-Collector' -EA SilentlyContinue } while ($t -and $t.State -eq 'Running' -and (Get-Date) -lt $deadline)
            if ($t -and $t.State -eq 'Running') { Stop-ScheduledTask 'WinUICov-Collector' }
            Unregister-ScheduledTask 'WinUICov-Collector' -Confirm:$false -EA SilentlyContinue
            [pscustomobject]@{ shutdown = $o.Trim(); size = if (Test-Path $out) { (Get-Item $out).Length } else { 0 } }
        } -ArgumentList $vmTool, $sid, $vmOut, $FlushTimeoutSec
        Info ("collector shutdown: {0}; coverage size {1:N0} bytes" -f $flush.shutdown, $flush.size)
    } catch { Write-Warning "collector shutdown failed: $_" }

    # ---- 6. download (also on failure, so the collector log is kept) and always close the session -
    # Copy errors are not raised here (they would hide the original failure); a missing or empty
    # coverage file is reported below.
    $local = Join-Path $rawDir (Split-Path $vmOut -Leaf)
    try {
        Copy-Item $vmOut $local -FromSession $session -Force -EA SilentlyContinue
        Copy-Item $vmLog (Join-Path $rawDir 'collector.log') -FromSession $session -Force -EA SilentlyContinue
    } finally { Remove-PSSession $session -EA SilentlyContinue }
}

if (-not (Test-Path $local) -or (Get-Item $local).Length -lt 1024) {
    throw "Coverage file missing or empty ($local). Check $rawDir\collector.log and that the test process loaded the instrumented DLL (session $sid)."
}
Info ("downloaded {0} ({1:N1} MB)" -f $local, ((Get-Item $local).Length / 1MB))
[pscustomobject]@{ coverageFile = $local; testLog = $testLog; testSummary = "$($summary.Line)".Trim(); sessionId = $sid; rawDir = $rawDir } |
    Tee-Object -Variable result | ConvertTo-Json | Set-Content (Join-Path $rawDir 'run-result.json') -Encoding UTF8
$result
