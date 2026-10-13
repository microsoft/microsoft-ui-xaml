# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
#
# Set-CoveragePipeAcl.ps1 - Put a NULL DACL on the code-coverage collector's named pipe.
#
# This sets the DACL in-process via P/Invoke, so the lab path never has to ship a native
# helper to the test agents.  TAEF's te.processhost runs at
# low integrity (Centennial); by default it cannot connect to the collector's pipe and
# coverage comes back empty with ACCESS_DENIED.  A NULL DACL opens the gate.
#
# Usage:
#   .\Set-CoveragePipeAcl.ps1 -SessionId aabbccdd11223344
#   .\Set-CoveragePipeAcl.ps1 -PipeName '\\.\pipe\CodeCoverage.pipe.aabbccdd11223344'

[CmdletBinding(DefaultParameterSetName = 'SessionId')]
param(
    [Parameter(Mandatory = $true, ParameterSetName = 'SessionId')]
    [string]$SessionId,

    [Parameter(Mandatory = $true, ParameterSetName = 'PipeName')]
    [string]$PipeName
)

$ErrorActionPreference = 'Stop'

if ($PSCmdlet.ParameterSetName -eq 'SessionId') {
    $PipeName = "\\.\pipe\CodeCoverage.pipe.$SessionId"
}

# P/Invoke: open the pipe with WRITE_DAC and hand it a NULL DACL via SetSecurityInfo.
# (Add-Type compiles this with the in-box .NET Framework C# compiler on any Windows agent.)
if (-not ('WinUI.Coverage.PipeAcl' -as [type])) {
    Add-Type -Namespace 'WinUI.Coverage' -Name 'PipeAcl' -MemberDefinition @'
    [System.Runtime.InteropServices.DllImport("kernel32.dll", SetLastError = true, CharSet = System.Runtime.InteropServices.CharSet.Unicode)]
    public static extern System.IntPtr CreateFileW(
        string lpFileName, uint dwDesiredAccess, uint dwShareMode, System.IntPtr lpSecurityAttributes,
        uint dwCreationDisposition, uint dwFlagsAndAttributes, System.IntPtr hTemplateFile);

    [System.Runtime.InteropServices.DllImport("advapi32.dll", SetLastError = false)]
    public static extern uint SetSecurityInfo(
        System.IntPtr handle, int ObjectType, uint SecurityInfo,
        System.IntPtr psidOwner, System.IntPtr psidGroup, System.IntPtr pDacl, System.IntPtr pSacl);

    [System.Runtime.InteropServices.DllImport("kernel32.dll", SetLastError = true)]
    public static extern bool CloseHandle(System.IntPtr hObject);

    // Open an existing pipe for WRITE_DAC | READ_CONTROL and set a NULL (allow-all) DACL.
    public static uint SetNullDacl(string pipeName)
    {
        const uint WRITE_DAC = 0x00040000;
        const uint READ_CONTROL = 0x00020000;
        const uint OPEN_EXISTING = 3;
        const int  SE_KERNEL_OBJECT = 6;
        const uint DACL_SECURITY_INFORMATION = 0x00000004;

        System.IntPtr h = CreateFileW(pipeName, WRITE_DAC | READ_CONTROL, 0, System.IntPtr.Zero,
                                      OPEN_EXISTING, 0, System.IntPtr.Zero);
        if (h == new System.IntPtr(-1))
        {
            throw new System.ComponentModel.Win32Exception(
                System.Runtime.InteropServices.Marshal.GetLastWin32Error(),
                "Failed to open pipe " + pipeName);
        }
        try
        {
            // pDacl = NULL with DACL_SECURITY_INFORMATION set => NULL DACL (everyone allowed).
            return SetSecurityInfo(h, SE_KERNEL_OBJECT, DACL_SECURITY_INFORMATION,
                                   System.IntPtr.Zero, System.IntPtr.Zero, System.IntPtr.Zero, System.IntPtr.Zero);
        }
        finally
        {
            CloseHandle(h);
        }
    }
'@
}

Write-Warning "Opening coverage pipe '$PipeName' with a NULL DACL - every local user can access it until the collector exits. Fine on an ephemeral CI agent; avoid running this on a shared machine."
Write-Host "Opening pipe: $PipeName"
$result = [WinUI.Coverage.PipeAcl]::SetNullDacl($PipeName)
if ($result -ne 0) {
    throw "SetSecurityInfo failed on '$PipeName' with error $result"
}
Write-Host "Successfully set NULL DACL on pipe"
