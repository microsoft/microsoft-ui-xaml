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
$sessionExists = $false
$testExitCode = 0
$tool = Join-Path $PayloadDir 'CoverageTool\Microsoft.CodeCoverage.Console.exe'

try
{
    try
    {
        foreach ($file in @($OutputFile, "$OutputFile.config"))
        {
            if (Test-Path -LiteralPath $file)
            {
                Remove-Item -LiteralPath $file
            }
        }
    }
    catch
    {
        # Artifact upload runs even after a failed test task.
        Write-Host '##vso[task.setvariable variable=skipPublish]true'
        throw "Cannot clear previous coverage output '$OutputFile' or its settings. The slice artifact will not be published. $($_.Exception.Message)"
    }

    try
    {
        $sessionId = (Get-Content -LiteralPath (Join-Path $PayloadDir '_coverage-session-id.txt') -Raw).Trim()
        if ($sessionId -notmatch '^[a-zA-Z0-9-]+$')
        {
            throw 'The coverage payload has an invalid session ID.'
        }
        $pipe = "\\.\pipe\CodeCoverage.pipe.$sessionId"
        # Enumerate without opening a pipe that may belong to another collector.
        $sessionExists = [IO.Directory]::GetFiles('\\.\pipe\', "CodeCoverage.pipe.$sessionId").Length -gt 0
        if ($sessionExists)
        {
            throw "Coverage session '$sessionId' already exists. Tests will not run; the existing collector will not be changed."
        }
        New-Item -ItemType Directory -Path (Split-Path $OutputFile) -Force | Out-Null

        # Query the process owner, not a possibly impersonated PowerShell thread.
        $process = Get-CimInstance -ClassName Win32_Process -Filter "ProcessId = $PID" -OperationTimeoutSec 30
        $owner = Invoke-CimMethod -InputObject $process -MethodName GetOwner -OperationTimeoutSec 30
        if ($owner.ReturnValue -ne 0 -or [string]::IsNullOrWhiteSpace($owner.Domain) -or
            [string]::IsNullOrWhiteSpace($owner.User))
        {
            throw "Cannot determine the coverage collector account (GetOwner returned '$($owner.ReturnValue)')."
        }
        $users = @("$($owner.Domain)\$($owner.User)")
        Write-Host "Coverage collector account: $($users[0])"
        $consoleUser = (Get-CimInstance -ClassName Win32_ComputerSystem -OperationTimeoutSec 30).UserName
        if (-not [string]::IsNullOrWhiteSpace($consoleUser))
        {
            $users += $consoleUser
            Write-Host "Coverage console account: $consoleUser"
        }
        else
        {
            Write-Host 'Coverage console account: none; allowing only the collector account.'
        }

        [xml]$settings = Get-Content -LiteralPath "$PSScriptRoot\coverage.config" -Raw
        $allowedUsers = $settings.CreateElement('AllowedUsers')
        foreach ($user in $users | Sort-Object -Unique)
        {
            # An unresolvable name can make the collector fall back to its defaults.
            try
            {
                $sid = [Security.Principal.NTAccount]::new($user).Translate([Security.Principal.SecurityIdentifier])
            }
            catch
            {
                throw "Cannot resolve coverage account '$user' to a SID. $($_.Exception.Message)"
            }
            Write-Host "Coverage allowed user: $user ($($sid.Value))"
            $entry = $settings.CreateElement('User')
            $entry.InnerText = $user
            [void]$allowedUsers.AppendChild($entry)
        }
        [void]$settings.Configuration.CodeCoverage.AppendChild($allowedUsers)
        $settingsFile = "$OutputFile.config"
        $settings.Save($settingsFile)
        Write-Host "Coverage collection settings: $settingsFile"

        $collector = Start-Process -FilePath $tool -ArgumentList @(
            'collect', '--session-id', $sessionId, '--server-mode',
            '--settings', "`"$settingsFile`"", '--output', "`"$OutputFile`""
        ) -PassThru -NoNewWindow -RedirectStandardOutput "$OutputFile.log" -RedirectStandardError "$OutputFile.err"

        # Retain the handle so Windows PowerShell can read ExitCode after the
        # collector exits, even if it exits before WaitForExit is called.
        $collectorHandle = $collector.Handle
        if ($null -eq $collectorHandle -or $collectorHandle -eq [IntPtr]::Zero)
        {
            throw 'Cannot retain the coverage collector process handle.'
        }

        $deadline = (Get-Date).AddSeconds(30)
        while ($true)
        {
            if ($collector.HasExited)
            {
                throw "Coverage collector exited with code $($collector.ExitCode). See $OutputFile.err."
            }
            if (Test-Path -LiteralPath $pipe) { break }
            if ((Get-Date) -ge $deadline)
            {
                throw "Coverage collector pipe did not appear within 30 seconds. See $OutputFile.log."
            }
            Start-Sleep -Milliseconds 500
        }
    }
    catch
    {
        if ($sessionExists) { throw }
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
            if ($collector.HasExited)
            {
                throw "Coverage collector exited with code $($collector.ExitCode) before shutdown."
            }
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
            $collectorExitCode = $collector.ExitCode
            if ($null -eq $collectorExitCode)
            {
                throw 'Coverage collector exit code is unavailable after shutdown.'
            }
            if ($collectorExitCode -ne 0)
            {
                throw "Coverage collector failed (exit $collectorExitCode). See $OutputFile.err."
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
