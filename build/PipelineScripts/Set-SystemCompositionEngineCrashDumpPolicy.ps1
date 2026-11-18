# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

[CmdletBinding(DefaultParameterSetName = 'Apply')]
param(
    [Parameter(ParameterSetName = 'Apply')]
    [ValidateNotNullOrEmpty()]
    [string]$ProcessName = 'te.processhost.exe',

    [Parameter(ParameterSetName = 'Apply')]
    [Microsoft.Win32.RegistryHive]$RegistryHive =
        [Microsoft.Win32.RegistryHive]::LocalMachine,

    [Parameter(ParameterSetName = 'Apply')]
    [Microsoft.Win32.RegistryView]$RegistryView =
        [Microsoft.Win32.RegistryView]::Registry64,

    [Parameter(ParameterSetName = 'Apply')]
    [ValidateNotNullOrEmpty()]
    [string]$LocalDumpsRegistryPath =
        'Software\Microsoft\Windows\Windows Error Reporting\LocalDumps',

    [Parameter(Mandatory, ParameterSetName = 'Restore')]
    [ValidateNotNull()]
    [psobject]$RestoreState
)

Set-StrictMode -Version 2.0
$ErrorActionPreference = 'Stop'

$dumpTypeValueName = 'DumpType'
$miniDumpType = 1

if ($PSCmdlet.ParameterSetName -eq 'Restore') {
    $requiredProperties = @(
        'PolicyVersion'
        'RegistryHive'
        'RegistryView'
        'LocalDumpsRegistryPath'
        'ProcessName'
        'ProcessKeyExisted'
        'DumpTypeExisted'
        'PreviousDumpType'
        'PreviousDumpTypeKind'
    )
    foreach ($requiredProperty in $requiredProperties) {
        if ($RestoreState.PSObject.Properties.Name -notcontains $requiredProperty) {
            throw "Crash dump policy restore state is missing '$requiredProperty'."
        }
    }
    if ([int]$RestoreState.PolicyVersion -ne 1) {
        throw (
            "Unsupported crash dump policy restore state version " +
            "'$($RestoreState.PolicyVersion)'.")
    }

    $restoreRegistryHive =
        [Microsoft.Win32.RegistryHive]([int]$RestoreState.RegistryHive)
    $restoreRegistryView =
        [Microsoft.Win32.RegistryView]([int]$RestoreState.RegistryView)
    $baseKey = [Microsoft.Win32.RegistryKey]::OpenBaseKey(
        $restoreRegistryHive,
        $restoreRegistryView)
    try {
        $localDumpsKey = $baseKey.CreateSubKey(
            [string]$RestoreState.LocalDumpsRegistryPath)
        try {
            $processKey = $localDumpsKey.OpenSubKey(
                [string]$RestoreState.ProcessName,
                $true)
            if ($null -eq $processKey -and
                [bool]$RestoreState.ProcessKeyExisted) {
                $processKey = $localDumpsKey.CreateSubKey(
                    [string]$RestoreState.ProcessName)
            }

            $deleteProcessKey = $false
            if ($null -ne $processKey) {
                try {
                    if ([bool]$RestoreState.DumpTypeExisted) {
                        $processKey.SetValue(
                            $dumpTypeValueName,
                            $RestoreState.PreviousDumpType,
                            [Microsoft.Win32.RegistryValueKind](
                                [int]$RestoreState.PreviousDumpTypeKind))
                    }
                    else {
                        $processKey.DeleteValue($dumpTypeValueName, $false)
                    }

                    $dumpTypeExists =
                        $processKey.GetValueNames() -contains $dumpTypeValueName
                    if ([bool]$RestoreState.DumpTypeExisted) {
                        if (-not $dumpTypeExists) {
                            throw (
                                "Failed to restore the previous crash dump policy " +
                                "for $($RestoreState.ProcessName).")
                        }
                        $restoredDumpType = $processKey.GetValue(
                            $dumpTypeValueName,
                            $null,
                            [Microsoft.Win32.RegistryValueOptions]::
                                DoNotExpandEnvironmentNames)
                        $restoredDumpTypeKind =
                            $processKey.GetValueKind($dumpTypeValueName)
                        if (-not [object]::Equals(
                                $restoredDumpType,
                                $RestoreState.PreviousDumpType) -or
                            $restoredDumpTypeKind -ne
                                [Microsoft.Win32.RegistryValueKind](
                                    [int]$RestoreState.PreviousDumpTypeKind)) {
                            throw (
                                "Failed to verify the restored crash dump policy " +
                                "for $($RestoreState.ProcessName).")
                        }
                    }
                    elseif ($dumpTypeExists) {
                        throw (
                            "Failed to remove the temporary crash dump policy " +
                            "for $($RestoreState.ProcessName).")
                    }

                    $deleteProcessKey =
                        -not [bool]$RestoreState.ProcessKeyExisted -and
                        $processKey.GetValueNames().Count -eq 0 -and
                        $processKey.GetSubKeyNames().Count -eq 0
                }
                finally {
                    $processKey.Close()
                }
            }

            if ($deleteProcessKey) {
                $localDumpsKey.DeleteSubKey(
                    [string]$RestoreState.ProcessName,
                    $false)
            }
        }
        finally {
            $localDumpsKey.Close()
        }
    }
    finally {
        $baseKey.Close()
    }

    Write-Host (
        "Restored the previous crash dump policy for " +
        "$($RestoreState.ProcessName).")
    return
}

$baseKey = [Microsoft.Win32.RegistryKey]::OpenBaseKey(
    $RegistryHive,
    $RegistryView)
try {
    $localDumpsKey = $baseKey.CreateSubKey($LocalDumpsRegistryPath)
    try {
        $processKey = $localDumpsKey.OpenSubKey($ProcessName, $true)
        $processKeyExisted = $null -ne $processKey
        if ($null -eq $processKey) {
            $processKey = $localDumpsKey.CreateSubKey($ProcessName)
        }
        try {
            $dumpTypeExisted =
                $processKey.GetValueNames() -contains $dumpTypeValueName
            $previousDumpType = $null
            $previousDumpTypeKind = $null
            if ($dumpTypeExisted) {
                $previousDumpType = $processKey.GetValue(
                    $dumpTypeValueName,
                    $null,
                    [Microsoft.Win32.RegistryValueOptions]::
                        DoNotExpandEnvironmentNames)
                $previousDumpTypeKind =
                    $processKey.GetValueKind($dumpTypeValueName)
            }

            $processKey.SetValue(
                $dumpTypeValueName,
                $miniDumpType,
                [Microsoft.Win32.RegistryValueKind]::DWord)

            $configuredDumpType = $processKey.GetValue($dumpTypeValueName)
            $configuredDumpTypeKind =
                $processKey.GetValueKind($dumpTypeValueName)
            if ([int]$configuredDumpType -ne $miniDumpType -or
                $configuredDumpTypeKind -ne
                    [Microsoft.Win32.RegistryValueKind]::DWord) {
                throw (
                    "Failed to enforce mini dumps for token-bearing process " +
                    "'$ProcessName'.")
            }
        }
        finally {
            $processKey.Close()
        }
    }
    finally {
        $localDumpsKey.Close()
    }
}
finally {
    $baseKey.Close()
}

Write-Host (
    "Temporarily enforcing mini dumps for token-bearing process '$ProcessName'.")
$previousDumpTypeKindValue = $null
if ($null -ne $previousDumpTypeKind) {
    $previousDumpTypeKindValue = [int]$previousDumpTypeKind
}
[pscustomobject]@{
    PolicyVersion = 1
    RegistryHive = [int]$RegistryHive
    RegistryView = [int]$RegistryView
    LocalDumpsRegistryPath = $LocalDumpsRegistryPath
    ProcessName = $ProcessName
    ProcessKeyExisted = $processKeyExisted
    DumpTypeExisted = $dumpTypeExisted
    PreviousDumpType = $previousDumpType
    PreviousDumpTypeKind = $previousDumpTypeKindValue
}
