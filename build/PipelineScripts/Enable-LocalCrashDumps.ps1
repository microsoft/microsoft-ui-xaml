# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

Param(
    [Parameter(Mandatory = $true)]
    [string] $crashDumpDirectory,
    [switch] $purgeDirectory
)

if ( Test-Path -LiteralPath $crashDumpDirectory -PathType Container )
{
    if ( $purgeDirectory )
    {
        Remove-Item $crashDumpDirectory\* -Recurse -Force | Out-Null
    }
}
else
{
    New-Item -Path $crashDumpDirectory -ItemType Directory | Out-Null
}

# Use .NET RegistryKey with explicit 64-bit view to avoid registry redirection: on ARM64 the agent
# runs as x86, so the PowerShell HKLM: provider would write to the WoW6432Node-redirected hive.
$regView = [Microsoft.Win32.RegistryView]::Registry64
$hklm = [Microsoft.Win32.RegistryKey]::OpenBaseKey([Microsoft.Win32.RegistryHive]::LocalMachine, $regView)
$werKey = $hklm.CreateSubKey("Software\Microsoft\Windows\Windows Error Reporting\LocalDumps")
$werKey.SetValue("DumpFolder", $crashDumpDirectory, [Microsoft.Win32.RegistryValueKind]::ExpandString)
$werKey.SetValue("DumpType", 2, [Microsoft.Win32.RegistryValueKind]::DWord)
$werKey.SetValue("DumpCount", 10, [Microsoft.Win32.RegistryValueKind]::DWord)
$werKey.Close()
$hklm.Close()
