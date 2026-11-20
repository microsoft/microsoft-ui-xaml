# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

Describe 'TaefHangWatchdog' {
    . (Join-Path $PSScriptRoot 'TaefHangWatchdog.ps1')

    $crashDumpPolicyScript = Join-Path $PSScriptRoot '..\..\..\build\PipelineScripts\Set-SystemCompositionEngineCrashDumpPolicy.ps1'

    # Returns a minidump's stream types and the processor architecture it records: 0 for x86, 9 for x64, 12 for ARM64.
    function Get-DumpInfo
    {
        param ([string]$Path)

        $reader = New-Object System.IO.BinaryReader ([System.IO.File]::OpenRead($Path))
        try
        {
            if ($reader.ReadUInt32() -ne 0x504D444D)
            {
                throw "$Path is not a minidump."
            }

            [void]$reader.ReadUInt32()
            $streamCount = $reader.ReadUInt32()
            $directoryRva = $reader.ReadUInt32()
            $streamTypes = @()
            $architecture = $null
            for ($i = 0; $i -lt $streamCount; $i++)
            {
                $reader.BaseStream.Position = $directoryRva + (12 * $i)
                $streamType = $reader.ReadUInt32()
                [void]$reader.ReadUInt32()
                $streamRva = $reader.ReadUInt32()
                $streamTypes += $streamType
                if ($streamType -eq 7) # SystemInfoStream
                {
                    $reader.BaseStream.Position = $streamRva
                    $architecture = $reader.ReadUInt16()
                }
            }

            [pscustomobject]@{ StreamTypes = $streamTypes; Architecture = $architecture }
        }
        finally
        {
            $reader.Dispose()
        }
    }

    # Whether the file contains the text as UTF-16, at any byte offset.
    function Test-FileContainsText
    {
        param ([string]$Path, [string]$Text)

        $latin1 = [System.Text.Encoding]::GetEncoding(28591)
        $content = $latin1.GetString([System.IO.File]::ReadAllBytes($Path))
        $pattern = $latin1.GetString([System.Text.Encoding]::Unicode.GetBytes($Text))
        $content.IndexOf($pattern, [StringComparison]::Ordinal) -ge 0
    }

    # Sets a Windows Error Reporting DumpType under the test's LocalDumps key, for one process or, without -ProcessName,
    # for all of them.
    function Set-TestDumpType
    {
        param ([int]$DumpType, [string]$ProcessName)

        $keyPath = $script:testLocalDumpsRegistryPath
        if ($ProcessName)
        {
            $keyPath = "$keyPath\$ProcessName"
        }

        $baseKey = [Microsoft.Win32.RegistryKey]::OpenBaseKey([Microsoft.Win32.RegistryHive]::CurrentUser, [Microsoft.Win32.RegistryView]::Registry64)
        try
        {
            $key = $baseKey.CreateSubKey($keyPath)
            try
            {
                $key.SetValue('DumpType', $DumpType, [Microsoft.Win32.RegistryValueKind]::DWord)
            }
            finally
            {
                $key.Close()
            }
        }
        finally
        {
            $baseKey.Close()
        }
    }

    # ping.exe stands in for a hung TAEF process: it runs for as long as it's asked to. The marker in its environment
    # stands in for a secret the process holds in memory.
    function Invoke-HungStandIn
    {
        param ([string]$PingPath, [string]$DumpFolder)

        New-Item -ItemType Directory -Path $DumpFolder | Out-Null
        Start-TaefHangWatchdog -TimeoutMinutes 0.05 -DumpFolder $DumpFolder -ProcessNames @('PING') `
            -RegistryHive CurrentUser -LocalDumpsRegistryPath $script:testLocalDumpsRegistryPath

        $startInfo = New-Object System.Diagnostics.ProcessStartInfo $PingPath, '-n 600 127.0.0.1'
        $startInfo.UseShellExecute = $false
        $startInfo.CreateNoWindow = $true
        $startInfo.EnvironmentVariables['TAEF_HANG_WATCHDOG_TEST_MARKER'] = $script:marker
        $standIn = [System.Diagnostics.Process]::Start($startInfo)

        $ended = $standIn.WaitForExit(120000)
        $report = (Stop-TaefHangWatchdog) -join "`n"
        if (-not $ended)
        {
            $standIn.Kill()
        }

        $dumpPath = Join-Path $DumpFolder "PING.$($standIn.Id).hang.dmp"
        $dumpExists = Test-Path -LiteralPath $dumpPath
        [pscustomobject]@{
            Ended = $ended
            Report = $report
            Id = $standIn.Id
            DumpExists = $dumpExists
            Dump = $(if ($dumpExists) { Get-DumpInfo -Path $dumpPath })
            DumpHasMarker = $dumpExists -and (Test-FileContainsText -Path $dumpPath -Text $script:marker)
        }
    }

    # Applies the crash dump policy that RunTestPassSliceOnBuildAgent.ps1 applies while the test host holds a token.
    function Invoke-HungStandInWithTokenPolicy
    {
        param ([string]$PingPath, [string]$DumpFolder)

        $policyState = & $crashDumpPolicyScript -ProcessName 'PING.EXE' -RegistryHive CurrentUser `
            -RegistryView Registry64 -LocalDumpsRegistryPath $script:testLocalDumpsRegistryPath
        try
        {
            Invoke-HungStandIn -PingPath $PingPath -DumpFolder $DumpFolder
        }
        finally
        {
            & $crashDumpPolicyScript -RestoreState $policyState
        }
    }

    $nativeArchitecture = @{ 'x86' = 0; 'AMD64' = 9; 'ARM64' = 12 }[$env:PROCESSOR_ARCHITECTURE]
    $ping = Join-Path ([Environment]::GetFolderPath('System')) 'PING.EXE'
    $ping32 = Join-Path ([Environment]::GetFolderPath('SystemX86')) 'PING.EXE'
    $threadListStream = 3
    # Written only for dumps that include the process's memory.
    $memory64ListStream = 9

    BeforeEach {
        $id = [guid]::NewGuid().ToString('N')
        $script:testRegistryCleanupPath = "Software\Microsoft\Windows\Windows Error Reporting\WinUITests\$id"
        $script:testLocalDumpsRegistryPath = "$script:testRegistryCleanupPath\LocalDumps"
        $script:marker = "TaefHangWatchdogTestMarker-$([guid]::NewGuid().ToString('N'))"
    }

    AfterEach {
        $baseKey = [Microsoft.Win32.RegistryKey]::OpenBaseKey([Microsoft.Win32.RegistryHive]::CurrentUser, [Microsoft.Win32.RegistryView]::Registry64)
        try
        {
            $baseKey.DeleteSubKeyTree($script:testRegistryCleanupPath, $false)
        }
        finally
        {
            $baseKey.Close()
        }
    }

    It 'leaves a run that finishes in time alone' {
        $dumpFolder = Join-Path $TestDrive 'finishes'
        New-Item -ItemType Directory -Path $dumpFolder | Out-Null

        Start-TaefHangWatchdog -TimeoutMinutes 1 -DumpFolder $dumpFolder -ProcessNames @('PING') `
            -RegistryHive CurrentUser -LocalDumpsRegistryPath $script:testLocalDumpsRegistryPath
        (Start-Process -FilePath $ping -ArgumentList @('-n', '1', '127.0.0.1') -WindowStyle Hidden -PassThru).WaitForExit()

        Stop-TaefHangWatchdog | Should BeNullOrEmpty
        @(Get-ChildItem -Path $dumpFolder).Count | Should Be 0
    }

    It 'writes a full dump of a process whose crash dumps are full dumps, then ends it' {
        Set-TestDumpType -DumpType 2 -ProcessName 'PING.EXE'

        $result = Invoke-HungStandIn -PingPath $ping -DumpFolder (Join-Path $TestDrive 'full')

        $result.Ended | Should Be $true
        $result.Report | Should Match 'still running after 0.05 minutes'
        $result.Report | Should Match 'Wrote full dump'
        $result.Report | Should Match "Ended PING \($($result.Id)\)"
        $result.Dump.Architecture | Should Be $nativeArchitecture
        $result.Dump.StreamTypes -contains $memory64ListStream | Should Be $true
        $result.DumpHasMarker | Should Be $true
    }

    It 'writes a mini dump of a process whose crash dumps are limited to mini dumps' {
        Set-TestDumpType -DumpType 2
        Set-TestDumpType -DumpType 2 -ProcessName 'PING.EXE'

        $result = Invoke-HungStandInWithTokenPolicy -PingPath $ping -DumpFolder (Join-Path $TestDrive 'mini')

        $result.Ended | Should Be $true
        $result.Report | Should Match 'Wrote mini dump .*, because crash dumps of PING\.exe'
        $result.Dump.StreamTypes -contains $threadListStream | Should Be $true
        $result.Dump.StreamTypes -contains $memory64ListStream | Should Be $false
        $result.DumpHasMarker | Should Be $false
    }

    It 'writes a mini dump when no crash dump setting is found' {
        $result = Invoke-HungStandIn -PingPath $ping -DumpFolder (Join-Path $TestDrive 'unset')

        $result.Ended | Should Be $true
        $result.Report | Should Match 'Wrote mini dump'
        $result.Dump.StreamTypes -contains $memory64ListStream | Should Be $false
        $result.DumpHasMarker | Should Be $false
    }

    It 'writes 32-bit full dumps of 32-bit processes whose crash dumps are full dumps' -Skip:(-not [Environment]::Is64BitProcess) {
        Set-TestDumpType -DumpType 2

        $result = Invoke-HungStandIn -PingPath $ping32 -DumpFolder (Join-Path $TestDrive 'full32')

        $result.Ended | Should Be $true
        $result.Report | Should Match 'Wrote full dump'
        $result.Dump.Architecture | Should Be 0
        $result.Dump.StreamTypes -contains $memory64ListStream | Should Be $true
        $result.DumpHasMarker | Should Be $true
    }

    It 'writes 32-bit mini dumps of 32-bit processes whose crash dumps are limited to mini dumps' -Skip:(-not [Environment]::Is64BitProcess) {
        Set-TestDumpType -DumpType 2 -ProcessName 'PING.EXE'

        $result = Invoke-HungStandInWithTokenPolicy -PingPath $ping32 -DumpFolder (Join-Path $TestDrive 'mini32')

        $result.Ended | Should Be $true
        $result.Report | Should Match 'Wrote mini dump'
        $result.Dump.Architecture | Should Be 0
        $result.Dump.StreamTypes -contains $threadListStream | Should Be $true
        $result.Dump.StreamTypes -contains $memory64ListStream | Should Be $false
        $result.DumpHasMarker | Should Be $false
    }
}
