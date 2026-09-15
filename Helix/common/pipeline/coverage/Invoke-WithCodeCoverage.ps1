# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

# Collects coverage around one test slice, with bounded cleanup that preserves the test result.

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$PayloadDir,

    [Parameter(Mandatory = $true)]
    [string]$OutputFile,

    [Parameter(Mandatory = $true)]
    [scriptblock]$RunTests,

    [ValidateRange(1, 600)]
    [int]$ShutdownTimeoutSeconds = 60
)

$ErrorActionPreference = 'Stop'
$collector = $null
$testExitCode = 0
$tool = Join-Path $PayloadDir 'CoverageTool\Microsoft.CodeCoverage.Console.exe'

try
{
    try
    {
        $sessionId = (Get-Content -LiteralPath (Join-Path $PayloadDir '_coverage-session-id.txt') -Raw).Trim()
        if ($sessionId -notmatch '^[a-zA-Z0-9-]+$')
        {
            throw 'The coverage payload has an invalid session ID.'
        }
        New-Item -ItemType Directory -Path (Split-Path $OutputFile) -Force | Out-Null
        if (Test-Path -LiteralPath $OutputFile)
        {
            Remove-Item -LiteralPath $OutputFile
        }

        $collector = Start-Process -FilePath $tool -ArgumentList @(
            'collect', '--session-id', $sessionId, '--server-mode',
            '--settings', "`"$PSScriptRoot\coverage.config`"", '--output', "`"$OutputFile`""
        ) -PassThru -NoNewWindow -RedirectStandardOutput "$OutputFile.log" -RedirectStandardError "$OutputFile.err"

        $pipe = "\\.\pipe\CodeCoverage.pipe.$sessionId"
        $deadline = (Get-Date).AddSeconds(30)
        while (-not (Test-Path $pipe))
        {
            if ($collector.HasExited)
            {
                throw "Coverage collector exited with code $($collector.ExitCode). See $OutputFile.err."
            }
            if ((Get-Date) -ge $deadline)
            {
                throw "Coverage collector pipe did not appear within 30 seconds. See $OutputFile.log."
            }
            Start-Sleep -Milliseconds 500
        }
        & "$PSScriptRoot\Set-CoveragePipeAcl.ps1" -SessionId $sessionId
    }
    catch
    {
        # Coverage is experimental. A collector problem must not prevent the tests from running.
        Write-Host "##vso[task.logissue type=warning]Coverage setup failed: $($_.Exception.Message)"
    }

    try
    {
        & $RunTests
    }
    finally
    {
        $testExitCode = $LASTEXITCODE
    }
}
finally
{
    if ($collector)
    {
        $shutdown = $null
        $shutdownOutput = $null
        $shutdownError = $null
        try
        {
            # Process.Start retains the handle; Windows PowerShell's Start-Process
            # can lose ExitCode when the child exits before WaitForExit.
            $shutdownClock = [Diagnostics.Stopwatch]::StartNew()
            $startInfo = [Diagnostics.ProcessStartInfo]::new($tool, "shutdown $sessionId")
            $startInfo.UseShellExecute = $false
            $startInfo.CreateNoWindow = $true
            $startInfo.RedirectStandardOutput = $true
            $startInfo.RedirectStandardError = $true
            $shutdown = [Diagnostics.Process]::Start($startInfo)
            $shutdownOutput = $shutdown.StandardOutput.ReadToEndAsync()
            $shutdownError = $shutdown.StandardError.ReadToEndAsync()
            if (-not $shutdown.WaitForExit($ShutdownTimeoutSeconds * 1000))
            {
                throw "Coverage shutdown did not finish within $ShutdownTimeoutSeconds seconds."
            }
            if ($shutdown.ExitCode -ne 0)
            {
                throw "Coverage shutdown failed (exit $($shutdown.ExitCode))."
            }
            $remainingMilliseconds = [int][Math]::Max(0, $ShutdownTimeoutSeconds * 1000 - $shutdownClock.ElapsedMilliseconds)
            if (-not $collector.WaitForExit($remainingMilliseconds))
            {
                throw "Coverage collector did not exit within $ShutdownTimeoutSeconds seconds."
            }
            if (-not (Test-Path -LiteralPath $OutputFile) -or (Get-Item -LiteralPath $OutputFile).Length -eq 0)
            {
                throw "No coverage data was produced in '$OutputFile'."
            }
        }
        catch
        {
            Write-Host "##vso[task.logissue type=warning]Coverage collection failed: $($_.Exception.Message)"
        }
        finally
        {
            foreach ($process in @($shutdown, $collector))
            {
                if ($process)
                {
                    if (-not $process.HasExited)
                    {
                        Stop-Process -Id $process.Id -ErrorAction Continue
                    }
                }
            }
            try
            {
                if ($shutdownOutput -and $shutdownError)
                {
                    if (-not [Threading.Tasks.Task]::WaitAll([Threading.Tasks.Task[]]@($shutdownOutput, $shutdownError), 1000))
                    {
                        throw 'Coverage shutdown log streams did not close.'
                    }
                    [IO.File]::WriteAllText("$OutputFile.shutdown.log", $shutdownOutput.Result)
                    [IO.File]::WriteAllText("$OutputFile.shutdown.err", $shutdownError.Result)
                }
            }
            catch
            {
                Write-Host "##vso[task.logissue type=warning]Could not save coverage shutdown logs: $($_.Exception.Message)"
            }
            foreach ($process in @($shutdown, $collector))
            {
                if ($process) { $process.Dispose() }
            }
        }
    }
    # Collector commands must not replace the test command's exit code.
    $global:LASTEXITCODE = $testExitCode
}
