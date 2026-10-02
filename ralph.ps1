<#
.SYNOPSIS
Runs one agent per queued task until the queue has no unchecked tasks.
.EXAMPLE
.\ralph.ps1 -Timeout '00:30:00' -Model 'auto'
#>
[CmdletBinding()]
param(
    [ValidateScript({ $_ -gt [TimeSpan]::Zero })]
    [TimeSpan]$Timeout = [TimeSpan]::FromHours(4),

    [ValidateNotNullOrEmpty()]
    [string]$Model = 'auto'
)

$ErrorActionPreference = 'Stop'
$workingDirectory = (Get-Location).Path
$instructionsPath = Join-Path $workingDirectory 'ralph-instructions.md'
$queuePath = Join-Path $workingDirectory 'inbox.md'

if (-not (Test-Path -LiteralPath $instructionsPath -PathType Leaf)) {
    throw "Instructions file not found: $instructionsPath"
}
if (-not (Test-Path -LiteralPath $queuePath -PathType Leaf)) {
    throw "Task queue not found: $queuePath"
}

$agency = (Get-Command agency -CommandType Application -ErrorAction Stop | Select-Object -First 1).Source
$agentArguments = @(
    'copilot', '--mcp', 'ado', '--yolo', '-s', '-p',
    '"Follow the instructions in .\ralph-instructions.md."'
)

while ($true) {
    $nextTask = Select-String -LiteralPath $queuePath -Pattern '^\s*-\s*\[\s\]\s+\[[^\]]+\]\s+\S' |
        Select-Object -First 1
    if (-not $nextTask) {
        Write-Host "No unchecked tasks remain in inbox.md. Stopping."
        break
    }

    $agent = $null
    try {
        Write-Host "Starting agent for: $($nextTask.Line.Trim())"
        Write-Host "Model: $Model; timeout: $Timeout. Press Ctrl+C to stop."
        $agent = Start-Process -FilePath $agency -ArgumentList ($agentArguments + @('--model', $Model)) `
            -WorkingDirectory $workingDirectory -NoNewWindow -PassThru
        $timer = [Diagnostics.Stopwatch]::StartNew()

        while (-not $agent.WaitForExit(1000)) {
            if ($timer.Elapsed -ge $Timeout) {
                Write-Host "Agent timed out after $Timeout. Restarting."
                break
            }
        }

        if ($agent.HasExited) {
            Write-Host "Agent exited with code $($agent.ExitCode). Restarting."
        }
    }
    finally {
        if ($null -ne $agent) {
            if (-not $agent.HasExited) {
                # Kill the entire tree, including any agent launched by agency.
                & taskkill.exe /PID $agent.Id /T /F
                if ($LASTEXITCODE -ne 0) {
                    throw "Failed to stop agent process $($agent.Id)."
                }
                $agent.WaitForExit()
            }
            $agent.Dispose()
        }
    }
}
