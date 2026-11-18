# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
#Requires -PSEdition Desktop

[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [string]$LaunchRequestDirectory,

    [Parameter(Mandatory)]
    [string]$NativeManifestPath,

    [Parameter(Mandatory)]
    [string]$ManagedManifestPath
)

Set-StrictMode -Version 2.0
$ErrorActionPreference = 'Stop'

$compositionEngineLafToken = $env:COMPOSITION_ENGINE_LAF_TOKEN
if ([string]::IsNullOrEmpty($compositionEngineLafToken)) {
    throw 'COMPOSITION_ENGINE_LAF_TOKEN must be set before creating the launch request.'
}
Remove-Item `
    -LiteralPath Env:\COMPOSITION_ENGINE_LAF_TOKEN `
    -ErrorAction SilentlyContinue

$maximumCompositionEngineLafTokenSize = 64 * 1024
$sharedMemoryHeaderSize = 4
$sharedMemoryCapacity =
    $sharedMemoryHeaderSize + $maximumCompositionEngineLafTokenSize
$sharedMemoryNamePrefix = 'WinUI.SystemCompositionEngine.'

if (-not ([Management.Automation.PSTypeName]'AppContainerSecurity').Type) {
    Add-Type -TypeDefinition @'
using System;
using System.ComponentModel;
using System.Runtime.InteropServices;
using System.Text;

public static class AppContainerSecurity
{
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    private struct PackageId
    {
        public uint reserved;
        public uint processorArchitecture;
        public ulong version;
        [MarshalAs(UnmanagedType.LPWStr)]
        public string name;
        [MarshalAs(UnmanagedType.LPWStr)]
        public string publisher;
        [MarshalAs(UnmanagedType.LPWStr)]
        public string resourceId;
        [MarshalAs(UnmanagedType.LPWStr)]
        public string publisherId;
    }

    [DllImport("kernel32.dll", CharSet = CharSet.Unicode)]
    private static extern int PackageFamilyNameFromId(
        ref PackageId packageId,
        ref uint packageFamilyNameLength,
        StringBuilder packageFamilyName);

    [DllImport("userenv.dll", CharSet = CharSet.Unicode)]
    public static extern int DeriveAppContainerSidFromAppContainerName(
        string appContainerName,
        out IntPtr appContainerSid);

    [DllImport("advapi32.dll")]
    public static extern IntPtr FreeSid(IntPtr sid);

    public static string GetPackageFamilyName(string name, string publisher)
    {
        var packageId = new PackageId
        {
            name = name,
            publisher = publisher
        };
        uint length = 0;
        const int ErrorInsufficientBuffer = 122;
        int result = PackageFamilyNameFromId(
            ref packageId,
            ref length,
            null);
        if (result != ErrorInsufficientBuffer)
        {
            throw new Win32Exception(result);
        }

        var familyName = new StringBuilder((int)length);
        result = PackageFamilyNameFromId(
            ref packageId,
            ref length,
            familyName);
        if (result != 0)
        {
            throw new Win32Exception(result);
        }
        return familyName.ToString();
    }
}
'@
}

$appContainerSidsByValue = @{}
foreach ($manifestPath in @($NativeManifestPath, $ManagedManifestPath)) {
    if (-not (Test-Path -LiteralPath $manifestPath -PathType Leaf)) {
        throw "Package manifest was not found: $manifestPath"
    }

    [xml]$manifest = Get-Content -LiteralPath $manifestPath -Raw
    $identity = $manifest.Package.Identity
    $packageFamilyName = [AppContainerSecurity]::GetPackageFamilyName(
        [string]$identity.Name,
        [string]$identity.Publisher)
    $appContainerSidPointer = [IntPtr]::Zero
    $deriveResult =
        [AppContainerSecurity]::DeriveAppContainerSidFromAppContainerName(
            $packageFamilyName,
            [ref]$appContainerSidPointer)
    if ($deriveResult -ne 0) {
        throw "Failed to derive an AppContainer SID ($deriveResult)."
    }
    try {
        $sid = [Security.Principal.SecurityIdentifier]::new(
            $appContainerSidPointer)
        $appContainerSidsByValue[$sid.Value] = $sid
    }
    finally {
        [void][AppContainerSecurity]::FreeSid($appContainerSidPointer)
    }
}
Write-Host (
    'System composition engine launch request: resolved {0} unique AppContainer identities.' -f
    $appContainerSidsByValue.Count)

$launchRequestParent = Split-Path -Parent $LaunchRequestDirectory
if (-not (Test-Path -LiteralPath $launchRequestParent -PathType Container)) {
    throw "System composition engine launch request parent directory was not found: $launchRequestParent"
}
$currentUser = [Security.Principal.WindowsIdentity]::GetCurrent().User
$systemSid = [Security.Principal.SecurityIdentifier]::new('S-1-5-18')
$mappingSecurityDescriptor =
    'D:P' +
    "(A;;GA;;;$($currentUser.Value))" +
    "(A;;GA;;;$($systemSid.Value))"
foreach ($sid in @($appContainerSidsByValue.Values)) {
    $mappingSecurityDescriptor += "(A;;GR;;;$($sid.Value))"
}
$mappingSecurity =
    [IO.MemoryMappedFiles.MemoryMappedFileSecurity]::new()
$mappingSecurity.SetSecurityDescriptorSddlForm(
    $mappingSecurityDescriptor)
$mappingName = $sharedMemoryNamePrefix + [Guid]::NewGuid().ToString('N')

$memoryMappedFile = $null
$tokenBytes = $null
try {
    if (Test-Path -LiteralPath $LaunchRequestDirectory) {
        Remove-Item -LiteralPath $LaunchRequestDirectory -Recurse -Force
    }
    New-Item -ItemType Directory -Path $LaunchRequestDirectory | Out-Null

    $acl = [Security.AccessControl.DirectorySecurity]::new()
    $acl.SetAccessRuleProtection($true, $false)
    $inheritance =
        [Security.AccessControl.InheritanceFlags]::ContainerInherit -bor
        [Security.AccessControl.InheritanceFlags]::ObjectInherit
    $propagation = [Security.AccessControl.PropagationFlags]::None
    $allow = [Security.AccessControl.AccessControlType]::Allow
    $fullControl = [Security.AccessControl.FileSystemRights]::FullControl
    $readAndExecute = [Security.AccessControl.FileSystemRights]::ReadAndExecute
    foreach ($sid in @($currentUser, $systemSid)) {
        $acl.AddAccessRule(
            [Security.AccessControl.FileSystemAccessRule]::new(
                $sid,
                $fullControl,
                $inheritance,
                $propagation,
                $allow))
    }
    foreach ($sid in @($appContainerSidsByValue.Values)) {
        $acl.AddAccessRule(
            [Security.AccessControl.FileSystemAccessRule]::new(
                $sid,
                $readAndExecute,
                $inheritance,
                $propagation,
                $allow))
    }
    Set-Acl -LiteralPath $LaunchRequestDirectory -AclObject $acl

    $tokenBytes = [Text.UTF8Encoding]::new(
        $false,
        $true).GetBytes($compositionEngineLafToken)
    $compositionEngineLafToken = $null
    if ($tokenBytes.Length -eq 0 -or
        $tokenBytes.Length -gt $maximumCompositionEngineLafTokenSize) {
        throw (
            'COMPOSITION_ENGINE_LAF_TOKEN must encode to between 1 and ' +
            "$maximumCompositionEngineLafTokenSize UTF-8 bytes.")
    }

    $memoryMappedFile =
        [IO.MemoryMappedFiles.MemoryMappedFile]::CreateNew(
            $mappingName,
            [long]$sharedMemoryCapacity,
            [IO.MemoryMappedFiles.MemoryMappedFileAccess]::ReadWrite,
            [IO.MemoryMappedFiles.MemoryMappedFileOptions]::None,
            $mappingSecurity,
            [IO.HandleInheritability]::None)
    $viewStream = $memoryMappedFile.CreateViewStream(
        0,
        $sharedMemoryCapacity,
        [IO.MemoryMappedFiles.MemoryMappedFileAccess]::ReadWrite)
    $writer = [IO.BinaryWriter]::new(
        $viewStream,
        [Text.UTF8Encoding]::new($false),
        $false)
    try {
        $writer.Write([int]$tokenBytes.Length)
        $writer.Write($tokenBytes)
        $writer.Flush()
    }
    finally {
        $writer.Dispose()
    }

    $launchRequestPath = Join-Path $LaunchRequestDirectory 'system'
    [IO.File]::WriteAllText(
        $launchRequestPath,
        $mappingName,
        [Text.UTF8Encoding]::new($false))
    if (-not (Test-Path -LiteralPath $launchRequestPath -PathType Leaf)) {
        throw 'System composition engine launch request was not created.'
    }

    Write-Host (
        'System composition engine launch request created in shared memory.')
    return $memoryMappedFile
}
catch {
    if ($null -ne $memoryMappedFile) {
        $memoryMappedFile.Dispose()
    }
    if (Test-Path -LiteralPath $LaunchRequestDirectory) {
        Remove-Item -LiteralPath $LaunchRequestDirectory -Recurse -Force
    }
    throw
}
finally {
    if ($null -ne $tokenBytes) {
        [Array]::Clear($tokenBytes, 0, $tokenBytes.Length)
    }
    $compositionEngineLafToken = $null
}
