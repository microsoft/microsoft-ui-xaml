# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidatePattern('^[a-zA-Z0-9-]+$')]
    [string]$SessionId
)

$ErrorActionPreference = 'Stop'

# Preserve the experimental TAEF pipe-permission workaround. This changes only the
# DACL, not integrity labels, and belongs only on an isolated lab agent.
if (-not ('WinUI.Coverage.PipeAcl' -as [type]))
{
    Add-Type -Namespace 'WinUI.Coverage' -Name 'PipeAcl' -MemberDefinition @'
    [System.Runtime.InteropServices.DllImport("kernel32.dll", SetLastError = true, CharSet = System.Runtime.InteropServices.CharSet.Unicode)]
    public static extern System.IntPtr CreateFileW(
        string name, uint access, uint share, System.IntPtr security,
        uint disposition, uint flags, System.IntPtr template);

    [System.Runtime.InteropServices.DllImport("advapi32.dll")]
    public static extern uint SetSecurityInfo(
        System.IntPtr handle, int objectType, uint information,
        System.IntPtr owner, System.IntPtr group, System.IntPtr dacl, System.IntPtr sacl);

    [System.Runtime.InteropServices.DllImport("kernel32.dll")]
    public static extern bool CloseHandle(System.IntPtr handle);

    public static void SetNullDacl(string pipeName)
    {
        const uint WRITE_DAC = 0x00040000;
        const uint READ_CONTROL = 0x00020000;
        const uint OPEN_EXISTING = 3;
        const int SE_KERNEL_OBJECT = 6;
        const uint DACL_SECURITY_INFORMATION = 4;
        var handle = CreateFileW(pipeName, WRITE_DAC | READ_CONTROL, 0,
            System.IntPtr.Zero, OPEN_EXISTING, 0, System.IntPtr.Zero);
        if (handle == new System.IntPtr(-1))
        {
            throw new System.ComponentModel.Win32Exception(
                System.Runtime.InteropServices.Marshal.GetLastWin32Error());
        }
        try
        {
            uint error = SetSecurityInfo(handle, SE_KERNEL_OBJECT, DACL_SECURITY_INFORMATION,
                System.IntPtr.Zero, System.IntPtr.Zero, System.IntPtr.Zero, System.IntPtr.Zero);
            if (error != 0)
            {
                throw new System.ComponentModel.Win32Exception((int)error);
            }
        }
        finally
        {
            CloseHandle(handle);
        }
    }
'@
}

Write-Warning 'The coverage pipe will allow all local users until the collector exits. Use only on isolated test agents.'
[WinUI.Coverage.PipeAcl]::SetNullDacl("\\.\pipe\CodeCoverage.pipe.$SessionId")
