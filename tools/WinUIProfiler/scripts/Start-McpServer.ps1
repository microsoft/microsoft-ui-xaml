# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License. See LICENSE in the project root for license information.

<#
.SYNOPSIS
Starts the WinUI Profiler MCP server for the winui-profiler Copilot plugin.

.DESCRIPTION
plugin.json runs this script with Windows PowerShell. The first time, and again after the server's
sources change, it builds WinUIProfiler.Mcp from this folder into
%LOCALAPPDATA%\WinUIProfiler\Mcp\<hash of the sources>. Then it runs the server with the same dotnet.

Standard input and output carry the server's MCP messages, so this script writes only to standard
error. Building needs the .NET SDK version that global.json names.
#>
[CmdletBinding()]
param()

Set-StrictMode -Version 3.0
$ErrorActionPreference = 'Stop'

$toolRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$project = Join-Path $toolRoot 'WinUIProfiler.Mcp\WinUIProfiler.Mcp.csproj'
$serverFileName = 'WinUIProfiler.Mcp.dll'
$completeFileName = '.complete'
$lastUsedFileName = '.last-used'
$backslash = [char]92
$quote = [char]34
$utf8 = New-Object System.Text.UTF8Encoding($false)

function Write-Status([string] $Message) {
    [Console]::Error.WriteLine("[winui-profiler] $Message")
}

# Quotes an argument so that CommandLineToArgvW and the C runtime read it back unchanged.
function ConvertTo-Argument([string] $Value) {
    if ($Value.Length -gt 0 -and $Value.IndexOfAny([char[]] @([char]32, [char]9, $quote)) -lt 0) {
        return $Value
    }

    $builder = New-Object System.Text.StringBuilder
    [void] $builder.Append($quote)
    $backslashes = 0
    foreach ($character in $Value.ToCharArray()) {
        if ($character -ceq $backslash) {
            $backslashes++
            continue
        }

        if ($character -ceq $quote) {
            [void] $builder.Append($backslash, 2 * $backslashes + 1)
        } elseif ($backslashes -gt 0) {
            [void] $builder.Append($backslash, $backslashes)
        }

        [void] $builder.Append($character)
        $backslashes = 0
    }

    [void] $builder.Append($backslash, 2 * $backslashes)
    [void] $builder.Append($quote)
    return $builder.ToString()
}

# Runs dotnet in the tool folder and returns its exit code and output. Its standard input is closed,
# so it can't read the MCP messages meant for the server.
function Invoke-Dotnet([string[]] $Arguments, [int] $TimeoutMinutes) {
    $startInfo = New-Object System.Diagnostics.ProcessStartInfo
    $startInfo.FileName = $script:dotnet
    $startInfo.Arguments = @($Arguments | ForEach-Object { ConvertTo-Argument $_ }) -join ' '
    $startInfo.WorkingDirectory = $toolRoot
    $startInfo.UseShellExecute = $false
    $startInfo.CreateNoWindow = $true
    $startInfo.RedirectStandardInput = $true
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    $startInfo.StandardOutputEncoding = $utf8
    $startInfo.StandardErrorEncoding = $utf8
    $startInfo.EnvironmentVariables['DOTNET_NOLOGO'] = '1'
    $startInfo.EnvironmentVariables['MSBUILDDISABLENODEREUSE'] = '1'

    $process = [Diagnostics.Process]::Start($startInfo)
    try {
        $process.StandardInput.Close()
        $output = $process.StandardOutput.ReadToEndAsync()
        $errors = $process.StandardError.ReadToEndAsync()
        if (-not $process.WaitForExit($TimeoutMinutes * 60000)) {
            try { $process.Kill() } catch { }
            throw "dotnet $($Arguments[0]) didn't finish within $TimeoutMinutes minutes."
        }

        # A process that outlives dotnet could hold the pipes open, so don't wait on it for long.
        [void] $output.Wait(30000)
        [void] $errors.Wait(30000)
        return [pscustomobject] @{
            ExitCode = $process.ExitCode
            Output = $(if ($output.IsCompleted) { $output.Result } else { '' })
            Errors = $(if ($errors.IsCompleted) { $errors.Result } else { '' })
        }
    } finally {
        $process.Dispose()
    }
}

function Get-OutputLines($Result) {
    return @(($Result.Output + "`n" + $Result.Errors) -split "`r?`n" | Where-Object { $_.Trim() })
}

function Add-SourceFiles([string] $Directory, [System.Collections.Generic.List[string]] $Files) {
    foreach ($file in [IO.Directory]::GetFiles($Directory)) {
        $Files.Add($file)
    }

    foreach ($child in [IO.Directory]::GetDirectories($Directory)) {
        $name = [IO.Path]::GetFileName($child)
        $isLink = ([IO.File]::GetAttributes($child) -band [IO.FileAttributes]::ReparsePoint) -ne 0
        if (($name -eq 'bin') -or ($name -eq 'obj') -or $name.StartsWith('.') -or $isLink) {
            continue
        }

        Add-SourceFiles $child $Files
    }
}

# Hashes everything the build reads from this folder (the server and Core projects, the shared build
# files and this script) together with the dotnet and SDK that build it.
function Get-SourceHash([string[]] $Inputs) {
    $files = New-Object 'System.Collections.Generic.List[string]'
    foreach ($name in @('WinUIProfiler.Core', 'WinUIProfiler.Mcp')) {
        Add-SourceFiles (Join-Path $toolRoot $name) $files
    }

    foreach ($name in @('Directory.Build.props', 'Directory.Build.targets', 'Directory.Packages.props', 'global.json', 'NuGet.Config', 'scripts\Start-McpServer.ps1')) {
        $path = Join-Path $toolRoot $name
        if ([IO.File]::Exists($path)) {
            $files.Add($path)
        }
    }

    [string[]] $relative = @($files | ForEach-Object { $_.Substring($toolRoot.Length).TrimStart($backslash) })
    [Array]::Sort($relative, [StringComparer]::OrdinalIgnoreCase)

    $sha = [Security.Cryptography.SHA256]::Create()
    try {
        foreach ($text in $Inputs) {
            $bytes = $utf8.GetBytes("$text`n")
            [void] $sha.TransformBlock($bytes, 0, $bytes.Length, $null, 0)
        }

        foreach ($path in $relative) {
            $content = [IO.File]::ReadAllBytes((Join-Path $toolRoot $path))
            $header = $utf8.GetBytes("$($path.ToLowerInvariant())`n$($content.Length)`n")
            [void] $sha.TransformBlock($header, 0, $header.Length, $null, 0)
            [void] $sha.TransformBlock($content, 0, $content.Length, $null, 0)
        }

        [void] $sha.TransformFinalBlock((New-Object byte[] 0), 0, 0)
        return -join @($sha.Hash[0..7] | ForEach-Object { $_.ToString('x2') })
    } finally {
        $sha.Dispose()
    }
}

function Remove-Directory([string] $Path) {
    if ([IO.Directory]::Exists($Path)) {
        try {
            [IO.Directory]::Delete($Path, $true)
        } catch {
            Write-Status "Couldn't delete $($Path): $($_.Exception.Message)"
        }
    }
}

function Wait-Mutex([System.Threading.Mutex] $Mutex, [int] $Milliseconds) {
    try {
        return $Mutex.WaitOne($Milliseconds)
    } catch {
        # A session that ended while it was building leaves the mutex abandoned, and this one owns it now.
        for ($exception = $_.Exception; $null -ne $exception; $exception = $exception.InnerException) {
            if ($exception -is [System.Threading.AbandonedMutexException]) {
                return $true
            }
        }

        throw
    }
}

function Publish-Server([string] $Base, [string] $Hash, [string] $Target) {
    $work = Join-Path $Base ".work-$Hash-$PID"
    $publish = Join-Path $work 'publish'
    $logs = Join-Path $Base 'logs'
    $log = Join-Path $logs ('publish-{0}-{1}.log' -f $Hash, [DateTime]::Now.ToString('yyyyMMdd-HHmmss'))
    [void] [IO.Directory]::CreateDirectory($logs)
    Remove-Directory $work

    # --artifacts-path keeps the build's bin and obj folders out of the source folders.
    $arguments = @(
        'publish', $project,
        '--configuration', 'Release',
        '--artifacts-path', (Join-Path $work 'artifacts'),
        '--output', $publish,
        '-p:RestoreLockedMode=true',
        '-p:UseSharedCompilation=false',
        '-nodeReuse:false',
        '-nologo')
    Write-Status "Building the server from $toolRoot. This can take a minute after the sources change. Log: $log"
    $stopwatch = [Diagnostics.Stopwatch]::StartNew()
    $result = Invoke-Dotnet $arguments 8
    $commandLine = 'dotnet ' + (@($arguments | ForEach-Object { ConvertTo-Argument $_ }) -join ' ')
    $logText = "> $commandLine`r`nExit code $($result.ExitCode) after $([int] $stopwatch.Elapsed.TotalSeconds) s`r`n`r`n$($result.Output)`r`n$($result.Errors)"
    [IO.File]::WriteAllText($log, $logText, $utf8)

    if ($result.ExitCode -ne 0 -or -not [IO.File]::Exists((Join-Path $publish $serverFileName))) {
        $lines = @(Get-OutputLines $result | Select-Object -Last 25)
        Remove-Directory $work
        throw ("Building the server failed (dotnet exit code {0}). The log is {1}. Its last lines:`n{2}" -f $result.ExitCode, $log, ($lines -join "`n"))
    }

    # The marker goes in before the move, so a build folder that has it is complete.
    [IO.File]::WriteAllText((Join-Path $publish $completeFileName), "$script:sdkVersion`r`n", $utf8)
    if ([IO.Directory]::Exists($Target) -and -not [IO.File]::Exists((Join-Path $Target $completeFileName))) {
        Remove-Directory $Target
    }

    try {
        [IO.Directory]::Move($publish, $Target)
    } catch {
        # The same user's session in another Windows session can finish the same build first.
        if (-not [IO.File]::Exists((Join-Path $Target $completeFileName))) {
            throw
        }
    }

    Remove-Directory $work
    Write-Status ('Built the server in {0} s.' -f [int] $stopwatch.Elapsed.TotalSeconds)
}

# Deletes builds that no session has started for 30 days, and what unfinished builds left behind.
function Remove-OldBuilds([string] $Base, [string] $Current) {
    $now = [DateTime]::UtcNow
    foreach ($directory in [IO.Directory]::GetDirectories($Base)) {
        $name = [IO.Path]::GetFileName($directory)
        if ($name.StartsWith('.work-') -or $name.StartsWith('.delete-')) {
            if ([IO.Directory]::GetLastWriteTimeUtc($directory) -lt $now.AddDays(-1)) {
                Remove-Directory $directory
            }

            continue
        }

        if (($name -notmatch '^[0-9a-f]{16}$') -or ($name -eq $Current)) {
            continue
        }

        $marker = Join-Path $directory $lastUsedFileName
        $lastUsed = if ([IO.File]::Exists($marker)) { [IO.File]::GetLastWriteTimeUtc($marker) } else { [IO.Directory]::GetLastWriteTimeUtc($directory) }
        if ($lastUsed -lt $now.AddDays(-30)) {
            # A running server holds its .last-used file open, so its folder can't be renamed and is
            # never half deleted.
            $doomed = Join-Path $Base ".delete-$name"
            try {
                [IO.Directory]::Move($directory, $doomed)
            } catch {
                continue
            }

            Remove-Directory $doomed
        }
    }

    $logs = Join-Path $Base 'logs'
    if ([IO.Directory]::Exists($logs)) {
        foreach ($file in [IO.Directory]::GetFiles($logs, '*.log')) {
            if ([IO.File]::GetLastWriteTimeUtc($file) -lt $now.AddDays(-30)) {
                try { [IO.File]::Delete($file) } catch { }
            }
        }
    }
}

function Build-Server([string] $Base, [string] $Hash, [string] $Target) {
    [void] [IO.Directory]::CreateDirectory($Base)
    $mutex = New-Object System.Threading.Mutex($false, 'Local\WinUIProfilerMcpBuild')
    $acquired = $false
    try {
        $acquired = Wait-Mutex $mutex 0
        if (-not $acquired) {
            Write-Status 'Waiting for another session to finish building the server.'
            $acquired = Wait-Mutex $mutex 540000
            if (-not $acquired) {
                throw 'Another session has been building the server for 9 minutes. Start a new session to try again.'
            }
        }

        # Another session may have built the same sources while this one waited.
        if (-not [IO.File]::Exists((Join-Path $Target $completeFileName))) {
            Publish-Server $Base $Hash $Target
            Remove-OldBuilds $Base $Hash
        }
    } finally {
        if ($acquired) {
            $mutex.ReleaseMutex()
        }

        $mutex.Dispose()
    }
}

# Opens the build's .last-used file, stamps it with the time and returns it. The caller holds it open
# while the server runs, so cleanup in another session can't rename or delete the build. Cleanup can
# remove the build before the file is open, so this checks the build again once it is, and throws
# when the build isn't all there.
function Open-Lease([string] $Target) {
    $lease = New-Object System.IO.FileStream((Join-Path $Target $lastUsedFileName), [IO.FileMode]::OpenOrCreate, [IO.FileAccess]::ReadWrite, [IO.FileShare]::ReadWrite)
    try {
        $complete = Join-Path $Target $completeFileName
        if (-not [IO.File]::Exists($complete)) {
            throw "The build has no $completeFileName."
        }

        if (-not [IO.File]::Exists((Join-Path $Target $serverFileName))) {
            # Builds are moved into place whole, so the server was deleted since. Without its marker,
            # the build is replaced when it's built again.
            try { [IO.File]::Delete($complete) } catch { }
            throw "The build has no $serverFileName."
        }
    } catch {
        # Closed, so that building again can replace the build.
        $lease.Dispose()
        throw
    }

    # The open file is what keeps cleanup away. The time only tells cleanup when the build was last
    # used, once no server holds it.
    try {
        $stamp = $utf8.GetBytes([DateTime]::UtcNow.ToString('o'))
        $lease.SetLength(0)
        $lease.Write($stamp, 0, $stamp.Length)
        $lease.Flush()
    } catch {
        Write-Status "Couldn't record that this build is in use: $($_.Exception.GetBaseException().Message)"
    }

    return $lease
}

# Builds the server unless it's built, and returns the build's lease (see Open-Lease). When the build
# is gone or damaged by the time this holds it, as when cleanup in another session removed it, this
# builds it again.
function Lock-Build([string] $Base, [string] $Hash, [string] $Target) {
    $attempt = 0
    while ($true) {
        $attempt++
        if (-not [IO.File]::Exists((Join-Path $Target $completeFileName))) {
            Build-Server $Base $Hash $Target
        }

        try {
            return Open-Lease $Target
        } catch {
            $reason = $_.Exception.GetBaseException().Message
            if ($attempt -ge 3) {
                throw "Couldn't hold the server's build in $Target while the server runs: $reason"
            }

            Write-Status "Couldn't hold the server's build in $Target, so trying again: $reason"
            Start-Sleep -Milliseconds 500
        }
    }
}

$lease = $null
try {
    $requiredSdk = ''
    try {
        $requiredSdk = [string] (Get-Content -LiteralPath (Join-Path $toolRoot 'global.json') -Raw | ConvertFrom-Json).sdk.version
    } catch {
    }

    $sdkText = if ($requiredSdk) { ".NET SDK $requiredSdk, which global.json names" } else { 'the .NET SDK that global.json names' }
    $command = Get-Command -Name 'dotnet' -CommandType Application -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($null -eq $command) {
        throw "Building the server needs $sdkText, and dotnet isn't on PATH. Install the SDK from https://dotnet.microsoft.com/download/dotnet and start a new session."
    }

    $dotnet = $command.Path
    $version = Invoke-Dotnet @('--version') 2
    if ($version.ExitCode -ne 0) {
        throw ("Building the server needs $sdkText, and $dotnet doesn't have it. Install the SDK from https://dotnet.microsoft.com/download/dotnet and start a new session. dotnet --version said:`n" + ((Get-OutputLines $version) -join "`n"))
    }

    $sdkVersion = $version.Output.Trim()
    $hash = Get-SourceHash @("dotnet=$dotnet", "sdk=$sdkVersion")
    $base = Join-Path ([Environment]::GetFolderPath([Environment+SpecialFolder]::LocalApplicationData)) 'WinUIProfiler\Mcp'
    $target = Join-Path $base $hash
    $lease = Lock-Build $base $hash $target

    # Not redirected: the server reads and writes this script's standard input and output directly.
    $startInfo = New-Object System.Diagnostics.ProcessStartInfo
    $startInfo.FileName = $dotnet
    $startInfo.Arguments = ConvertTo-Argument (Join-Path $target $serverFileName)
    $startInfo.UseShellExecute = $false
    $server = [Diagnostics.Process]::Start($startInfo)
} catch {
    $lines = @("$($_.Exception.Message)" -split "`r?`n")
    Write-Status "Couldn't start the server: $($lines[0])"
    foreach ($line in @($lines | Select-Object -Skip 1)) {
        Write-Status "  $line"
    }

    if ($null -ne $lease) {
        $lease.Dispose()
    }

    exit 1
}

$server.WaitForExit()
$exitCode = $server.ExitCode
if ($null -ne $lease) {
    $lease.Dispose()
}

exit $exitCode
