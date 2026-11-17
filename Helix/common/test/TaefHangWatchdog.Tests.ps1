# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

Describe 'TaefHangWatchdog' {
    . (Join-Path $PSScriptRoot 'TaefHangWatchdog.ps1')

    # Returns the processor architecture recorded in a minidump: 0 for x86, 9 for x64, 12 for ARM64.
    function Get-DumpArchitecture
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
            for ($i = 0; $i -lt $streamCount; $i++)
            {
                $reader.BaseStream.Position = $directoryRva + (12 * $i)
                $streamType = $reader.ReadUInt32()
                [void]$reader.ReadUInt32()
                $streamRva = $reader.ReadUInt32()
                if ($streamType -eq 7) # SystemInfoStream
                {
                    $reader.BaseStream.Position = $streamRva
                    return $reader.ReadUInt16()
                }
            }

            throw "$Path has no system info stream."
        }
        finally
        {
            $reader.Dispose()
        }
    }

    # ping.exe stands in for a hung te.exe: it runs for as long as it's asked to.
    function Start-StandIn
    {
        param ([string]$PingPath, [int]$Seconds)

        Start-Process -FilePath $PingPath -ArgumentList @('-n', $Seconds, '127.0.0.1') -WindowStyle Hidden -PassThru
    }

    function Invoke-HungStandIn
    {
        param ([string]$PingPath, [string]$DumpFolder)

        New-Item -ItemType Directory -Path $DumpFolder | Out-Null
        Start-TaefHangWatchdog -TimeoutMinutes 0.05 -DumpFolder $DumpFolder -ProcessNames @('PING')
        $standIn = Start-StandIn -PingPath $PingPath -Seconds 600
        $ended = $standIn.WaitForExit(120000)
        $report = (Stop-TaefHangWatchdog) -join "`n"
        if (-not $ended)
        {
            $standIn.Kill()
        }

        [pscustomobject]@{
            Ended = $ended
            Report = $report
            DumpPath = Join-Path $DumpFolder "PING.$($standIn.Id).hang.dmp"
            Id = $standIn.Id
        }
    }

    $nativeArchitecture = @{ 'x86' = 0; 'AMD64' = 9; 'ARM64' = 12 }[$env:PROCESSOR_ARCHITECTURE]
    $ping = Join-Path ([Environment]::GetFolderPath('System')) 'PING.EXE'
    $ping32 = Join-Path ([Environment]::GetFolderPath('SystemX86')) 'PING.EXE'

    It 'leaves a run that finishes in time alone' {
        $dumpFolder = Join-Path $TestDrive 'finishes'
        New-Item -ItemType Directory -Path $dumpFolder | Out-Null

        Start-TaefHangWatchdog -TimeoutMinutes 1 -DumpFolder $dumpFolder -ProcessNames @('PING')
        (Start-StandIn -PingPath $ping -Seconds 1).WaitForExit()

        Stop-TaefHangWatchdog | Should BeNullOrEmpty
        @(Get-ChildItem -Path $dumpFolder).Count | Should Be 0
    }

    It 'dumps and ends a run that is still going at the time limit' {
        $result = Invoke-HungStandIn -PingPath $ping -DumpFolder (Join-Path $TestDrive 'hangs')

        $result.Ended | Should Be $true
        $result.Report | Should Match 'still running after 0.05 minutes'
        $result.Report | Should Match "Ended PING \($($result.Id)\)"
        Test-Path -LiteralPath $result.DumpPath | Should Be $true
        Get-DumpArchitecture -Path $result.DumpPath | Should Be $nativeArchitecture
    }

    It 'writes 32-bit dumps of 32-bit processes' -Skip:(-not [Environment]::Is64BitProcess) {
        $result = Invoke-HungStandIn -PingPath $ping32 -DumpFolder (Join-Path $TestDrive 'hangs32')

        $result.Ended | Should Be $true
        Test-Path -LiteralPath $result.DumpPath | Should Be $true
        Get-DumpArchitecture -Path $result.DumpPath | Should Be 0
    }
}
