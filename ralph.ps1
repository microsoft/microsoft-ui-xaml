#Requires -Version 7.2
# Run: pwsh -NoProfile -File .\ralph.ps1
# Runs indefinitely by default. Ctrl+C stops the loop and its current agent.
# Edit ralph-instructions.md between turns; each turn reads a fresh copy.
# Run data and child-process temp files stay under artifacts\ralph in this checkout.
# Successful turns with new snapshots prune DLL/PDB files from older turns.
[CmdletBinding()]
param(
    [ValidateRange(0.01, 120)][double]$TimeoutMinutes = 120,
    [ValidateRange(0, 2147483647)][int]$MaxTurns = 0,
    [switch]$DryRun
)

$ErrorActionPreference = 'Stop'
$instructionsPath = Join-Path $PSScriptRoot 'ralph-instructions.md'

if ($DryRun) { Get-Content -LiteralPath $instructionsPath -Raw -Encoding utf8; return }
if (-not $IsWindows) { throw 'This loop requires Windows.' }

function Get-RalphSnapshotFiles {
    param([string]$Directory)

    # Do not follow junctions or symlinks outside a turn's evidence directory.
    foreach ($item in Get-ChildItem -LiteralPath $Directory -Force) {
        if ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) { continue }
        if ($item.PSIsContainer) {
            Get-RalphSnapshotFiles -Directory $item.FullName
        } elseif ($item.Extension -in '.dll', '.pdb') {
            $item
        }
    }
}

function Remove-PreviousRalphSnapshots {
    param([string[]]$HistoryRoots, [string]$CurrentTurnDirectory)

    $currentFiles = @(Get-RalphSnapshotFiles -Directory $CurrentTurnDirectory)
    $hasPair = @($currentFiles | Where-Object {
        $_.Extension -eq '.dll' -and
        [IO.Path]::ChangeExtension($_.FullName, '.pdb') -in $currentFiles.FullName
    }).Count -gt 0
    if (-not $hasPair) {
        Write-Host 'No replacement DLL/PDB pair; keeping previous snapshots.'
        return
    }

    $currentName = Split-Path -Leaf $CurrentTurnDirectory
    foreach ($root in $HistoryRoots) {
        if (-not (Test-Path -LiteralPath $root -PathType Container)) { continue }
        if ((Get-Item -LiteralPath $root -Force).Attributes -band [IO.FileAttributes]::ReparsePoint) {
            Write-Warning "Skipping linked history directory: $root"
            continue
        }
        foreach ($previous in Get-ChildItem -LiteralPath $root -Directory -Force) {
            if ($previous.Attributes -band [IO.FileAttributes]::ReparsePoint) { continue }
            if ($previous.Name -notmatch '^\d{8}-\d{6}-[0-9a-f]{8}$' -or
                $previous.FullName -eq $CurrentTurnDirectory -or
                [StringComparer]::OrdinalIgnoreCase.Compare($previous.Name, $currentName) -ge 0) { continue }
            if (-not (Test-Path -LiteralPath (Join-Path $previous.FullName 'prompt.txt') -PathType Leaf)) { continue }
            foreach ($snapshot in Get-RalphSnapshotFiles -Directory $previous.FullName) {
                Remove-Item -LiteralPath $snapshot.FullName -Force
                Write-Host "Removed old snapshot: $($snapshot.FullName)"
            }
        }
    }
}

$agency = (Get-Command agency -CommandType Application -ErrorAction Stop | Select-Object -First 1).Source
$repoKey = $PSScriptRoot.Replace(':', '').Replace('\', '_')
$legacyHistory = Join-Path $env:LOCALAPPDATA "WinUI\Ralph\$repoKey"
$history = Join-Path $PSScriptRoot 'artifacts\ralph'
$null = New-Item -ItemType Directory -Path $history -Force
Write-Host "History: $history. Ctrl+C stops the loop. Do not edit/build this checkout concurrently."

for ($turn = 1; $MaxTurns -eq 0 -or $turn -le $MaxTurns; $turn++) {
    $prompt = Get-Content -LiteralPath $instructionsPath -Raw -Encoding utf8
    if ([string]::IsNullOrWhiteSpace($prompt)) { throw "Instructions file is empty: $instructionsPath" }
    $id = (Get-Date -Format 'yyyyMMdd-HHmmss') + '-' + [guid]::NewGuid().ToString('N').Substring(0, 8)
    $directory = Join-Path $history $id
    $null = New-Item -ItemType Directory -Path $directory
    $tempDirectory = Join-Path $directory 'temp'
    $null = New-Item -ItemType Directory -Path $tempDirectory
    Set-Content -LiteralPath (Join-Path $directory 'prompt.txt') -Value $prompt -Encoding utf8
    $start = [Diagnostics.ProcessStartInfo]::new($agency)
    $start.UseShellExecute = $false
    $start.WorkingDirectory = $PSScriptRoot
    $start.Environment['RALPH_REPO_ROOT'] = $PSScriptRoot
    $start.Environment['RALPH_HISTORY'] = $history
    $start.Environment['RALPH_LEGACY_HISTORY'] = $legacyHistory
    $start.Environment['RALPH_TURN_DIR'] = $directory
    $start.Environment['TEMP'] = $tempDirectory
    $start.Environment['TMP'] = $tempDirectory
    foreach ($argument in @('copilot', '--hub', '--mcp', 'ado', '--yolo', '-s', '--no-ask-user', '-p', $prompt)) {
        $start.ArgumentList.Add($argument)
    }
    $agent = [Diagnostics.Process]::Start($start)
    $timer = [Diagnostics.Stopwatch]::StartNew()
    $timedOut = $false
    Write-Host "Turn $turn started (PID $($agent.Id)); limit $TimeoutMinutes minutes."
    try {
        while (-not $agent.WaitForExit(1000)) {
            if ($timer.Elapsed.TotalMinutes -ge $TimeoutMinutes) {
                $timedOut = $true
                Write-Warning "Turn $turn timed out; stopping PID $($agent.Id) and its descendants."
                break
            }
        }
    }
    finally {
        try {
            if (-not $agent.HasExited) {
                try { $agent.Kill($true) }
                catch [InvalidOperationException] { if (-not $agent.HasExited) { throw } }
                if (-not $agent.WaitForExit(30000)) { throw 'Agent did not exit; refusing to start another.' }
            }
            $exitCode = $agent.ExitCode
            @{
                exitCode = $exitCode
                timedOut = $timedOut
                elapsedSeconds = $timer.Elapsed.TotalSeconds
                finishedUtc = [DateTime]::UtcNow.ToString('o')
            } | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $directory 'exit.json') -Encoding utf8
        }
        finally { $agent.Dispose() }
    }
    Write-Host "Turn $turn finished. Evidence: $directory"
    if (-not $timedOut -and $exitCode -eq 0) {
        Remove-PreviousRalphSnapshots -HistoryRoots @($history, $legacyHistory) -CurrentTurnDirectory $directory
    }
    if ($MaxTurns -eq 0 -or $turn -lt $MaxTurns) { Start-Sleep -Seconds 5 }
}
