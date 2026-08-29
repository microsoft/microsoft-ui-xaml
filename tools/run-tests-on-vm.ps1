# run-tests-on-vm.ps1 - Run WinUI tests on a local Hyper-V VM
#
# Uses PowerShell Direct (Hyper-V VMBus transport) -- no network config,
# no WinRM, no firewall rules needed.
#
# Usage:
#   .\run-tests-on-vm.ps1 -VMName "MyVM" "MyTestName"
#   .\run-tests-on-vm.ps1 -VMName "MyVM" "Button*" -HostingMode WPF
#   .\run-tests-on-vm.ps1 -VMName "MyVM" "MyTest" -SkipPayload
#   .\run-tests-on-vm.ps1 -VMName "MyVM" "MyTest" -FullCopy
#   .\run-tests-on-vm.ps1 -VMName "MyVM" -Stop          # kill any running test
#
# First run will prompt for VM credentials and cache them (encrypted, per-user).
# Subsequent runs reuse the cached credential automatically.

[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)]
    [string]$VMName,

    [Parameter(Position=0, Mandatory=$false)]
    [string]$TestName,

    [Parameter(ValueFromRemainingArguments=$true)]
    [string[]]$AdditionalArgs,

    [ValidateSet("x86", "x64", "arm64")]
    [string]$Platform = "",

    [ValidateSet("chk", "fre")]
    [string]$Configuration = "",

    # Pass a credential object directly (skips interactive prompt and caching)
    [PSCredential]$Credential,

    # Skip CreateTestPayload step (use existing payload)
    [switch]$SkipPayload,

    # CreateTestPayload mode. Most tests use DevTestSuite.
    [ValidateSet("Auto", "DevTestSuite", "All", "PGO")]
    [string]$Mode = "Auto",

    # Force a full copy instead of incremental
    [switch]$FullCopy,

    # Skip the testmachine-prerun step even if it hasn't run yet
    [switch]$SkipPrerun,

    # Force testmachine-prerun to re-run even if the marker file exists.
    # Useful after editing TestPass-OneTimeMachineSetupCore.ps1.
    [switch]$ForcePrerun,

    # Clear cached credentials for this VM
    [switch]$ResetCredential,

    # Stop any running test on the VM and clean up
    [switch]$Stop
)

$ErrorActionPreference = "Stop"

# Force UTF-8 so streamed VM log output uses a single, predictable encoding.
try {
    [Console]::OutputEncoding = [System.Text.Encoding]::UTF8
    $OutputEncoding = [System.Text.Encoding]::UTF8
} catch {}

# =========================================================================
#  Helper functions
# =========================================================================

function Get-VMCredential {
    <#
    .SYNOPSIS
    Get or prompt for VM credentials, with per-VM encrypted caching.
    #>
    param(
        [string]$VMName,
        [PSCredential]$Credential,
        [switch]$ResetCredential
    )

    $credDir = Join-Path $env:USERPROFILE ".winui-test"
    if (-not (Test-Path $credDir)) { New-Item -ItemType Directory $credDir -Force | Out-Null }
    $credFile = Join-Path $credDir "vmcred-$($VMName -replace '[^a-zA-Z0-9]','_').xml"

    if ($ResetCredential -and (Test-Path $credFile)) {
        Remove-Item $credFile -Force
        Write-Host "Cleared cached credential for $VMName." -ForegroundColor Yellow
    }

    if ($Credential) {
        Write-Host "Using provided credential." -ForegroundColor Gray
        return $Credential
    }

    if (Test-Path $credFile) {
        Write-Host "Using cached credential for $VMName ($credFile)." -ForegroundColor Gray
        return (Import-Clixml $credFile)
    }

    # Prompt in a separate window so the password never appears in the
    # calling session's history (important when run by an AI agent).
    Write-Host "No cached credential found for $VMName." -ForegroundColor Yellow
    Write-Host "A credential prompt will open in a new window..." -ForegroundColor Cyan

    $helperPath = Join-Path $credDir "prompt-cred.ps1"
    @'
param(
    [Parameter(Mandatory=$true)][string]$VMName,
    [Parameter(Mandatory=$true)][string]$CredFile
)
Write-Host '========================================' -ForegroundColor Cyan
Write-Host "  Credentials for VM: $VMName" -ForegroundColor Cyan
Write-Host '========================================' -ForegroundColor Cyan
Write-Host ''
$username = Read-Host 'Username'
$password = Read-Host 'Password' -AsSecureString
$cred = New-Object System.Management.Automation.PSCredential($username, $password)
$cred | Export-Clixml $CredFile
Write-Host ''
Write-Host 'Credential saved! This window will close.' -ForegroundColor Green
Start-Sleep -Seconds 2
'@ | Set-Content $helperPath -Encoding UTF8

    Start-Process powershell -ArgumentList "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", $helperPath, "-VMName", $VMName, "-CredFile", $credFile -Wait
    Remove-Item $helperPath -ErrorAction SilentlyContinue

    if (-not (Test-Path $credFile)) {
        Write-Host "Error: No credential was saved. Cannot continue." -ForegroundColor Red
        exit 1
    }

    Write-Host "Credential cached to $credFile (encrypted, your account only)." -ForegroundColor Gray
    return (Import-Clixml $credFile)
}

function Connect-TestVM {
    <#
    .SYNOPSIS
    Open a PS Direct session to the VM with a clear error message on failure.
    #>
    param(
        [string]$VMName,
        [PSCredential]$Credential
    )

    # PS Direct requires local admin or "Hyper-V Administrators" membership.
    $isAdmin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()
               ).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
    $inHyperVGroup = (whoami /groups 2>$null) -match 'Hyper-V Administrators'
    if (-not $isAdmin -and -not $inHyperVGroup) {
        Write-Host "Error: PS Direct requires local admin or 'Hyper-V Administrators' membership." -ForegroundColor Red
        Write-Host ""
        Write-Host "  One-time fix (run once from an admin prompt, then log out and back in):" -ForegroundColor Yellow
        Write-Host "    Add-LocalGroupMember -Group 'Hyper-V Administrators' -Member `$env:USERNAME" -ForegroundColor White
        exit 1
    }

    Write-Host ""
    Write-Host "Connecting to VM '$VMName'..." -ForegroundColor Cyan
    try {
        $s = New-PSSession -VMName $VMName -Credential $Credential
    } catch {
        Write-Host "Failed to connect to VM '$VMName'." -ForegroundColor Red
        Write-Host "  Make sure the VM is running and the credentials are correct." -ForegroundColor Yellow
        Write-Host "  Use -ResetCredential to clear cached credentials." -ForegroundColor Yellow
        Write-Host "  Error: $_" -ForegroundColor Red
        exit 1
    }
    Write-Host "Connected." -ForegroundColor Green
    return $s
}

function Stop-VMTests {
    <#
    .SYNOPSIS
    Kill any running tests on the VM and clean up.
    #>
    param($Session, [string]$RemoteTestDir)

    Write-Host ""
    Write-Host "Stopping any running tests on VM..." -ForegroundColor Cyan
    Invoke-Command -Session $Session -ScriptBlock {
        param($dir)

        $task = Get-ScheduledTask -TaskName "WinUI-RunTests" -ErrorAction SilentlyContinue
        if ($task) {
            if ($task.State -eq "Running") {
                Stop-ScheduledTask -TaskName "WinUI-RunTests"
                Write-Host "  Stopped scheduled task." -ForegroundColor Yellow
            }
            Unregister-ScheduledTask -TaskName "WinUI-RunTests" -Confirm:$false
            Write-Host "  Removed scheduled task." -ForegroundColor Yellow
        } else {
            Write-Host "  No scheduled task found." -ForegroundColor Gray
        }

        $teProcs = Get-Process -Name "te", "TE.ProcessHost" -ErrorAction SilentlyContinue
        if ($teProcs) {
            $teProcs | ForEach-Object {
                Write-Host "  Killing $($_.ProcessName) (PID $($_.Id))..." -ForegroundColor Yellow
                Stop-Process -Id $_.Id -Force
            }
        } else {
            Write-Host "  No te.exe or TE.ProcessHost.exe running." -ForegroundColor Gray
        }

        Remove-Item "$dir\_run-wrapper.cmd" -ErrorAction SilentlyContinue
    } -ArgumentList $RemoteTestDir

    Write-Host "Done." -ForegroundColor Green
}

function New-TestPayload {
    <#
    .SYNOPSIS
    Create the test payload (or validate an existing one if -SkipPayload).
    Returns the payload directory path.
    #>
    param(
        [string]$RepoRoot,
        [string]$Platform,
        [string]$Configuration,
        [string]$Flavor,
        [string]$Mode,
        [switch]$SkipPayload
    )

    $resolvedMode = $Mode
    if ($Mode -eq "Auto") { $resolvedMode = "DevTestSuite" }

    $payloadDir = Join-Path $RepoRoot "TestPayload\$Flavor"

    if ($SkipPayload) {
        Write-Host ""
        Write-Host "[1/3] Skipping CreateTestPayload (-SkipPayload)." -ForegroundColor Yellow
        if (-not (Test-Path $payloadDir)) {
            Write-Host "Error: TestPayload not found at $payloadDir" -ForegroundColor Red
            Write-Host "Run without -SkipPayload first, or build and create the payload manually." -ForegroundColor Yellow
            exit 1
        }
        $requiredFiles = @("runtests.cmd", "testmachine-prerun.cmd", "te.exe")
        foreach ($f in $requiredFiles) {
            if (-not (Test-Path (Join-Path $payloadDir $f))) {
                Write-Host "Error: Payload is missing '$f' at $payloadDir" -ForegroundColor Red
                Write-Host "The payload may be incomplete. Try running without -SkipPayload." -ForegroundColor Yellow
                exit 1
            }
        }
    } else {
        Write-Host ""
        Write-Host "[1/3] Creating test payload (Mode: $resolvedMode)..." -ForegroundColor Cyan
        $createPayloadScript = Join-Path $RepoRoot "test\CreateTestPayload.ps1"
        & $createPayloadScript -Platform $Platform -Configuration $Configuration -Mode $resolvedMode -SkipSymbols -Quiet
        if ($LASTEXITCODE -ne 0) {
            Write-Host "CreateTestPayload failed (exit code $LASTEXITCODE)." -ForegroundColor Red
            $logFile = Join-Path $RepoRoot "TestPayload\$Flavor\CreateTestPayload.log"
            if (Test-Path $logFile) {
                $errorLines = Get-Content $logFile | Where-Object { $_ -match "ERROR" } | Select-Object -Last 3
                if ($errorLines) {
                    Write-Host "Last errors from log:" -ForegroundColor Yellow
                    $errorLines | ForEach-Object { Write-Host "  $_" -ForegroundColor Yellow }
                }
            }
            if (Test-Path $payloadDir) {
                Write-Host "A partial payload exists at $payloadDir." -ForegroundColor Yellow
                Write-Host "You can retry with -SkipPayload to use it as-is." -ForegroundColor Yellow
            }
            exit $LASTEXITCODE
        }
        if (-not (Test-Path $payloadDir)) {
            Write-Host "Error: TestPayload not found at $payloadDir after creation." -ForegroundColor Red
            exit 1
        }
        Write-Host "Payload created at $payloadDir" -ForegroundColor Green
    }

    return $payloadDir
}

function Deploy-TestPayload {
    <#
    .SYNOPSIS
    Copy the test payload to the VM using manifest-based incremental deploy.
    Packages changed files into a zip, copies once over PS Direct, expands on VM.
    #>
    param(
        $Session,
        [string]$PayloadDir,
        [string]$RemoteTestDir,
        [string]$VMName,
        [string]$RepoRoot,
        [string]$Flavor,
        [switch]$FullCopy
    )

    Write-Host ""
    $stopwatch = [System.Diagnostics.Stopwatch]::StartNew()
    Write-Host "[2/3] Deploying test payload to VM..." -ForegroundColor Cyan

    # If the remote dir doesn't exist yet, force a full copy (fresh/reimaged VM).
    $remoteExisted = Invoke-Command -Session $Session -ScriptBlock {
        param($dir)
        if (Test-Path $dir) { return $true }
        New-Item -ItemType Directory $dir -Force | Out-Null
        return $false
    } -ArgumentList $RemoteTestDir

    if (-not $remoteExisted -and -not $FullCopy) {
        Write-Host "Remote directory is new (fresh/reimaged VM?) -- forcing full copy." -ForegroundColor Yellow
        $FullCopy = $true
    }

    # --- Manifest-based change detection ---
    # We track which files (path -> size+mtime) we last copied to this VM.
    # Only files whose signature changed get re-copied.
    $safeVMName = $VMName -replace '[^a-zA-Z0-9]', '_'
    $manifestFile = Join-Path $RepoRoot "TestPayload" ".deploy-manifest-$safeVMName-$Flavor.json"

    $oldManifest = @{}
    if (-not $FullCopy -and (Test-Path $manifestFile)) {
        try {
            $json = Get-Content $manifestFile -Raw | ConvertFrom-Json
            foreach ($prop in $json.PSObject.Properties) { $oldManifest[$prop.Name] = $prop.Value }
        } catch {
            Write-Host "Could not read deploy manifest ($($_.Exception.Message)); forcing a full copy." -ForegroundColor Yellow
            $oldManifest = @{}
            $FullCopy = $true
        }
    }

    # Collect changed files
    $allFiles = Get-ChildItem -Path $PayloadDir -Recurse -File
    $changedFiles = @()
    foreach ($file in $allFiles) {
        $rel = $file.FullName.Substring($PayloadDir.Length + 1)
        $sig = "$($file.Length)|$($file.LastWriteTimeUtc.Ticks)"
        if ($FullCopy -or -not $oldManifest.ContainsKey($rel) -or $oldManifest[$rel] -ne $sig) {
            $changedFiles += @{ FullName = $file.FullName; RelativePath = $rel; Signature = $sig }
        }
    }

    if ($changedFiles.Count -eq 0 -and -not $FullCopy) {
        Write-Host "No files changed since last deploy. Skipping copy." -ForegroundColor Green
        Write-Host "  NOTE: This is an incremental deploy. Use -FullCopy to force a clean deploy." -ForegroundColor Yellow
    } else {
        # Suppress progress bars -- each update is a VMBus round-trip.
        $oldProgress = $ProgressPreference
        $ProgressPreference = 'SilentlyContinue'

        if ($FullCopy) {
            Write-Host "Full copy: packaging $($changedFiles.Count) files..." -ForegroundColor Cyan
            Invoke-Command -Session $Session -ScriptBlock {
                param($dir)
                if (Test-Path $dir) { Remove-Item "$dir\*" -Recurse -Force -ErrorAction SilentlyContinue }
            } -ArgumentList $RemoteTestDir
        } else {
            Write-Host "Incremental copy: packaging $($changedFiles.Count) changed file(s)..." -ForegroundColor Cyan
            Write-Host "  NOTE: This is an incremental deploy. Use -FullCopy to force a clean deploy." -ForegroundColor Yellow
        }

        # --- Package into zip ---
        Add-Type -AssemblyName System.IO.Compression.FileSystem
        $zipPath = Join-Path ([System.IO.Path]::GetTempPath()) "winui-deploy-$safeVMName-$Flavor-$(Get-Date -Format 'yyyyMMddHHmmssfff').zip"
        Remove-Item $zipPath -Force -ErrorAction SilentlyContinue

        Write-Host "  Packaging $($changedFiles.Count) files into zip..." -ForegroundColor Gray
        $timer = [System.Diagnostics.Stopwatch]::StartNew()
        $zip = [System.IO.Compression.ZipFile]::Open($zipPath, [System.IO.Compression.ZipArchiveMode]::Create)
        try {
            foreach ($file in $changedFiles) {
                $entryName = $file.RelativePath -replace '\\', '/'
                [System.IO.Compression.ZipFileExtensions]::CreateEntryFromFile($zip, $file.FullName, $entryName, [System.IO.Compression.CompressionLevel]::Fastest) | Out-Null
            }
        } finally { $zip.Dispose() }
        $timer.Stop()

        $zipSizeMB = [math]::Round((Get-Item $zipPath).Length / 1MB, 1)
        Write-Host "  Packaged: $zipSizeMB MB zip in $([math]::Round($timer.Elapsed.TotalSeconds, 1))s" -ForegroundColor Gray

        # --- Copy to VM ---
        $remoteZip = "$RemoteTestDir\__winui_deploy.zip"
        Write-Host "  Copying $zipSizeMB MB to VM..." -ForegroundColor Gray
        $timer = [System.Diagnostics.Stopwatch]::StartNew()
        Copy-Item -Path $zipPath -Destination $remoteZip -ToSession $Session -Force
        $timer.Stop()
        Write-Host "  Copied in $([math]::Round($timer.Elapsed.TotalSeconds, 1))s" -ForegroundColor Gray

        # --- Expand on VM ---
        Write-Host "  Expanding on VM..." -ForegroundColor Gray
        $timer = [System.Diagnostics.Stopwatch]::StartNew()
        $expandOk = Invoke-Command -Session $Session -ScriptBlock {
            param($zip, $dest)
            try {
                Add-Type -AssemblyName System.IO.Compression.FileSystem
                $archive = [System.IO.Compression.ZipFile]::OpenRead($zip)
                try {
                    foreach ($entry in $archive.Entries) {
                        if (-not $entry.Name) { continue }
                        $target = Join-Path $dest $entry.FullName
                        $targetDir = Split-Path -Parent $target
                        if (-not (Test-Path $targetDir)) { New-Item -ItemType Directory $targetDir -Force | Out-Null }
                        [System.IO.Compression.ZipFileExtensions]::ExtractToFile($entry, $target, $true)
                    }
                } finally { $archive.Dispose() }
                Remove-Item $zip -Force -ErrorAction SilentlyContinue
                return $true
            } catch {
                Write-Host "  Expand failed on VM: $_" -ForegroundColor Red
                return $false
            }
        } -ArgumentList $remoteZip, $RemoteTestDir
        $timer.Stop()
        Write-Host "  Expanded in $([math]::Round($timer.Elapsed.TotalSeconds, 1))s" -ForegroundColor Gray

        Remove-Item $zipPath -Force -ErrorAction SilentlyContinue

        if (-not $expandOk) {
            Write-Host "Failed to expand payload on VM." -ForegroundColor Red
            $ProgressPreference = $oldProgress
            exit 1
        }

        # Save manifest so next run only copies what changed.
        $newManifest = @{}
        foreach ($file in $allFiles) {
            $rel = $file.FullName.Substring($PayloadDir.Length + 1)
            $copiedEntry = $changedFiles | Where-Object { $_.RelativePath -eq $rel }
            if ($copiedEntry) { $newManifest[$rel] = $copiedEntry.Signature }
            elseif ($oldManifest.ContainsKey($rel)) { $newManifest[$rel] = $oldManifest[$rel] }
        }
        ($newManifest | ConvertTo-Json -Depth 1) | Set-Content $manifestFile -Encoding UTF8

        $label = if ($FullCopy) { "Full" } else { "Incremental" }
        Write-Host "$label copy complete ($($changedFiles.Count) files, $zipSizeMB MB)." -ForegroundColor Green
        $ProgressPreference = $oldProgress
    }

    $stopwatch.Stop()
    Write-Host ("Deploy phase took {0:mm\:ss} (m:s)." -f $stopwatch.Elapsed) -ForegroundColor Cyan
}

function Invoke-MachinePrerun {
    <#
    .SYNOPSIS
    Run TestPass-OneTimeMachineSetupCore.ps1 on the VM if it hasn't been done yet.
    Uses a marker file to avoid re-running. Re-runs if the setup script is newer
    than the marker (e.g. after a fresh deploy).
    #>
    param(
        $Session,
        [string]$RemoteTestDir,
        [string]$PrerunMarkerFile,
        [switch]$ForcePrerun
    )

    if ($ForcePrerun) {
        Write-Host "Forcing testmachine-prerun re-run (clearing marker file)..." -ForegroundColor Yellow
        Invoke-Command -Session $Session -ScriptBlock {
            param($marker)
            Remove-Item -Path $marker -Force -ErrorAction SilentlyContinue
        } -ArgumentList $PrerunMarkerFile
    }

    $prerunDone = Invoke-Command -Session $Session -ScriptBlock {
        param($marker, $scriptPath)
        if (-not (Test-Path $marker)) { return $false }
        # Re-run if the setup script is newer than the marker (script was updated).
        if (Test-Path $scriptPath) {
            if ((Get-Item $scriptPath).LastWriteTime -gt (Get-Item $marker).LastWriteTime) {
                return $false
            }
        }
        return $true
    } -ArgumentList $PrerunMarkerFile, "$RemoteTestDir\TestPass-OneTimeMachineSetupCore.ps1"

    if ($prerunDone) {
        Write-Host "testmachine-prerun already done (marker file found). Skipping." -ForegroundColor Gray
        Write-Host "  (Use -ForcePrerun to re-run.)" -ForegroundColor Gray
        return
    }

    Write-Host "Running testmachine-prerun on VM (one-time setup)..." -ForegroundColor Cyan
    $prerunExit = Invoke-Command -Session $Session -ScriptBlock {
        param($dir, $marker)
        Set-Location $dir
        # The setup script handles its own errors internally (e.g. appx already
        # installed, processes not running). We just check the process exit code.
        & powershell.exe -NonInteractive -ExecutionPolicy Bypass -File ".\TestPass-OneTimeMachineSetupCore.ps1" 2>&1 | ForEach-Object { Write-Host $_ }
        $exitCode = $LASTEXITCODE
        if ($exitCode -eq 0) { "done" | Set-Content $marker }
        return $exitCode
    } -ArgumentList $RemoteTestDir, $PrerunMarkerFile

    if ($prerunExit -eq 0) {
        Write-Host "testmachine-prerun completed." -ForegroundColor Green
    } else {
        Write-Host "ERROR: testmachine-prerun failed (exit code $prerunExit)." -ForegroundColor Red
        Write-Host "  This usually means the test payload is incomplete or the deploy failed." -ForegroundColor Red
        Write-Host "  Try: -FullCopy to re-deploy, or check that CreateTestPayload ran successfully." -ForegroundColor Yellow
        exit 1
    }
}

function Invoke-TestsOnVM {
    <#
    .SYNOPSIS
    Run tests on the VM via a scheduled task (for interactive desktop access)
    and tail the log output back to the host.
    Returns the test exit code.
    #>
    param(
        $Session,
        [string]$RemoteTestDir,
        [string[]]$TestArgs,
        [string]$TaskUser
    )

    $testArgsDisplay = $TestArgs -join ' '
    Write-Host ""
    Write-Host "[3/3] Running tests on VM: runtests.cmd $testArgsDisplay" -ForegroundColor Cyan
    Write-Host "--------------------------------------------" -ForegroundColor DarkGray

    # TAEF UI tests need an interactive desktop session. PS Direct runs in
    # session 0 (non-interactive), so we launch tests via a scheduled task
    # on the VM's active desktop. We tail the log file to stream output
    # back to the host in near-real-time.
    $exitCode = Invoke-Command -Session $Session -ScriptBlock {
        param($dir, [string[]]$testArgs, $taskUser)

        # Escape an argument for safe embedding in a .cmd file.
        function Escape-CmdArg {
            param([string]$arg)
            if ([string]::IsNullOrEmpty($arg)) { return '""' }
            $escaped = $arg.Replace('^', '^^').Replace('%', '%%').Replace('"', '^"')
            return '"' + $escaped + '"'
        }

        $argsString = ($testArgs | ForEach-Object { Escape-CmdArg $_ }) -join ' '

        $taskName = "WinUI-RunTests"
        $logFile  = "$dir\testrun-output.log"
        $exitFile = "$dir\testrun-exitcode.txt"

        # Clean up from previous run
        Remove-Item $logFile -ErrorAction SilentlyContinue
        Remove-Item $exitFile -ErrorAction SilentlyContinue
        Unregister-ScheduledTask -TaskName $taskName -Confirm:$false -ErrorAction SilentlyContinue

        # Build a wrapper script that captures the exit code.
        $wrapperScript = @"
cd /d "$dir"
CALL .\runtests.cmd $argsString > "$logFile" 2>&1
echo %ERRORLEVEL% > "$exitFile"
"@
        $wrapperPath = "$dir\_run-wrapper.cmd"
        $wrapperScript | Set-Content $wrapperPath -Encoding ASCII

        # Run as scheduled task on the interactive desktop (needed for UI tests).
        $action    = New-ScheduledTaskAction -Execute "cmd.exe" -Argument "/c `"$wrapperPath`"" -WorkingDirectory $dir
        $principal = New-ScheduledTaskPrincipal -UserId $taskUser -LogonType Interactive -RunLevel Highest
        $task      = New-ScheduledTask -Action $action -Principal $principal
        Register-ScheduledTask -TaskName $taskName -InputObject $task -Force | Out-Null
        Start-ScheduledTask -TaskName $taskName

        Write-Host "Scheduled task started. Tailing output..."

        # --- Log tailing ---
        # The log is mixed encoding: runtests.ps1 writes single-byte (UTF-8),
        # but te.exe writes UTF-16LE when stdout is redirected. We normalize by
        # stripping NUL bytes so both portions become clean ASCII/UTF-8.
        $linesSeen = 0
        $pollInterval = 3

        function Read-NormalizedLog {
            param($path)
            if (-not (Test-Path $path)) { return @() }
            $fs = [System.IO.FileStream]::new($path, [System.IO.FileMode]::Open, [System.IO.FileAccess]::Read, [System.IO.FileShare]::ReadWrite)
            $ms = [System.IO.MemoryStream]::new()
            $fs.CopyTo($ms)
            $fs.Close()
            $bytes = $ms.ToArray()
            $ms.Close()

            $bytes = $bytes | Where-Object { $_ -ne 0 }
            if ($bytes.Count -ge 3 -and $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF) {
                $bytes = $bytes[3..($bytes.Count - 1)]
            } elseif ($bytes.Count -ge 1 -and ($bytes[0] -eq 0xFF -or $bytes[0] -eq 0xFE)) {
                $bytes = $bytes[1..($bytes.Count - 1)]
            }
            return ([System.Text.Encoding]::UTF8.GetString([byte[]]$bytes) -split "`r?`n")
        }

        function Read-LogTail {
            param($path, [ref]$seen)
            try {
                $lines = Read-NormalizedLog $path
                if ($lines.Count -gt $seen.Value) {
                    for ($i = $seen.Value; $i -lt $lines.Count; $i++) {
                        if ($lines[$i].Trim()) { Write-Host $lines[$i] }
                    }
                    $seen.Value = $lines.Count
                }
            } catch {}
        }

        do {
            Start-Sleep -Seconds $pollInterval
            Read-LogTail $logFile ([ref]$linesSeen)
            $task = Get-ScheduledTask -TaskName $taskName -ErrorAction SilentlyContinue
        } while ($task -and $task.State -eq "Running")

        # Final flush
        Start-Sleep -Seconds 1
        Read-LogTail $logFile ([ref]$linesSeen)

        # Read exit code
        $exitCode = 1
        if (Test-Path $exitFile) {
            try {
                $raw = [System.IO.File]::ReadAllText($exitFile).Trim()
                if ($raw -match '^\d+$') { $exitCode = [int]$raw }
            } catch {
                $raw = (Get-Content $exitFile -ErrorAction SilentlyContinue).Trim()
                if ($raw -match '^\d+$') { $exitCode = [int]$raw }
            }
        }

        # te.exe returns 0 even when tests fail. Parse the TAEF summary line.
        if ($exitCode -eq 0 -and (Test-Path $logFile)) {
            $logLines = Read-NormalizedLog $logFile
            $summaryLine = $logLines | Where-Object { $_ -match 'Summary:\s+Total=\d+' } | Select-Object -Last 1
            if ($summaryLine -match 'Failed=(\d+)') {
                if ([int]$Matches[1] -gt 0) { $exitCode = 1 }
            }
        }

        # Clean up
        Unregister-ScheduledTask -TaskName $taskName -Confirm:$false -ErrorAction SilentlyContinue
        Remove-Item $wrapperPath -ErrorAction SilentlyContinue

        $exitCode
    } -ArgumentList $RemoteTestDir, (, $TestArgs), $TaskUser

    Write-Host "--------------------------------------------" -ForegroundColor DarkGray
    if ($exitCode -eq 0) {
        Write-Host "Tests PASSED." -ForegroundColor Green
    } else {
        Write-Host "Tests FAILED (exit code $exitCode)." -ForegroundColor Red
    }

    return $exitCode
}

# =========================================================================
#  Main script
# =========================================================================

# -- Resolve platform/config --------------------------------------------
if (-not $Platform) {
    $Platform = if ($env:BUILDPLATFORM) { $env:BUILDPLATFORM } else { "x86" }
}
if (-not $Configuration) {
    $Configuration = if ($env:_BuildType) { $env:_BuildType } else { "chk" }
}

$Flavor = "$Platform$Configuration"
$repoRoot = $env:reporoot
if (-not $repoRoot -or -not (Test-Path $repoRoot)) {
    Write-Host "Error: reporoot env var is not set. Run via initrun.ps1 or from a WinUI dev prompt." -ForegroundColor Red
    exit 1
}

# Unique remote directory per host machine + repo so two enlistments
# don't stomp each other's payload on the same VM.
$RemoteTestDir = "C:\TestPayload\$env:COMPUTERNAME-$(Split-Path $repoRoot -Leaf)"
$PrerunMarkerFile = "$RemoteTestDir\.prerun-complete"

# -- Banner --------------------------------------------------------------
Write-Host "============================================" -ForegroundColor Cyan
Write-Host "  run-tests-on-vm.ps1" -ForegroundColor Cyan
Write-Host "  VM:            $VMName" -ForegroundColor Cyan
Write-Host "  Flavor:        $Flavor" -ForegroundColor Cyan
Write-Host "  Test:          $TestName" -ForegroundColor Cyan
Write-Host "  Remote dir:    $RemoteTestDir" -ForegroundColor Cyan
Write-Host "============================================" -ForegroundColor Cyan

# -- Credential & connect ------------------------------------------------
$cred = Get-VMCredential -VMName $VMName -Credential $Credential -ResetCredential:$ResetCredential
$session = Connect-TestVM -VMName $VMName -Credential $cred

# -- Handle -Stop --------------------------------------------------------
if ($Stop) {
    Stop-VMTests -Session $session -RemoteTestDir $RemoteTestDir
    Remove-PSSession $session
    exit 0
}

if (-not $TestName) {
    Write-Host "Error: TestName is required (unless using -Stop)." -ForegroundColor Red
    Write-Host "Usage: run-tests-on-vm.ps1 -VMName <vm> <testname>" -ForegroundColor Yellow
    Remove-PSSession $session
    exit 1
}

# -- Run -----------------------------------------------------------------
try {
    $payloadDir = New-TestPayload -RepoRoot $repoRoot -Platform $Platform -Configuration $Configuration `
                                  -Flavor $Flavor -Mode $Mode -SkipPayload:$SkipPayload

    Deploy-TestPayload -Session $session -PayloadDir $payloadDir -RemoteTestDir $RemoteTestDir `
                       -VMName $VMName -RepoRoot $repoRoot -Flavor $Flavor -FullCopy:$FullCopy

    if (-not $SkipPrerun) {
        Invoke-MachinePrerun -Session $session -RemoteTestDir $RemoteTestDir `
                             -PrerunMarkerFile $PrerunMarkerFile -ForcePrerun:$ForcePrerun
    }

    $testArgs = @($TestName)
    if ($AdditionalArgs) { $testArgs += $AdditionalArgs }

    $testExitCode = Invoke-TestsOnVM -Session $session -RemoteTestDir $RemoteTestDir `
                                     -TestArgs $testArgs -TaskUser $cred.UserName
} finally {
    if ($session) { Remove-PSSession $session -ErrorAction SilentlyContinue }
}

exit $testExitCode
