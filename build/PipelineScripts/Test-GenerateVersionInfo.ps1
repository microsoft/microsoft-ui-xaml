# Copyright (c) Microsoft Corporation and Contributors.
# Licensed under the MIT License.

# Run from a Visual Studio developer shell with the Windows SDK tools available.

[CmdletBinding()]
Param(
    [string]$RepoRoot
)

Set-StrictMode -Version 2.0
$ErrorActionPreference = 'Stop'

if ([string]::IsNullOrWhiteSpace($RepoRoot))
{
    $RepoRoot = Join-Path $PSScriptRoot '..\..'
}

$resourceCompiler = Get-Command rc.exe -ErrorAction Stop
$linker = Get-Command link.exe -ErrorAction Stop
$generator = Join-Path $RepoRoot 'build\PipelineScripts\GenerateVersionInfo.ps1'
$resourceTemplate = Join-Path $RepoRoot 'dxaml\xcp\inc\WinUIrc.ver'
$testRoot = Join-Path ([System.IO.Path]::GetTempPath()) ('WinUIVersionInfo-' + [guid]::NewGuid().ToString('N'))
$originalBinaryVersion = $env:WindowsAppSDKBinaryVersion
$defaultSourceRevision = '0123456789abcdef0123456789abcdef01234567'

function Test-VersionResource
{
    Param(
        [string]$Name,
        [string]$Version,
        [string[]]$ExpectedVersions,
        [string]$SourceRevision = $defaultSourceRevision,
        [string]$ExpectedComments = ''
    )

    $directory = Join-Path $testRoot $Name
    [System.IO.Directory]::CreateDirectory($directory) | Out-Null
    $env:WindowsAppSDKBinaryVersion = if ($Version) { $Version } else { $null }

    & $generator -ProductMajor 3 -ProductMinor 0 -VersionInfoPath (Join-Path $directory 'WindowsAppSDK-ProductInfo.h') 6>&1 | Out-Null
    Copy-Item -LiteralPath $resourceTemplate -Destination (Join-Path $directory 'WinUIrc.ver')
    [System.IO.File]::WriteAllLines((Join-Path $directory 'WinUISourceRevision.h'), @(
        '#define WINUI_BUILD_MACHINE fixture_machine',
        "#define WINUI_SOURCE_REVISION_ID $SourceRevision"
    ))
    [System.IO.File]::WriteAllLines((Join-Path $directory 'VersionInfo.rc'), @(
        '#define VER_INTERNALNAME_STR "VersionInfo.dll"',
        '#include "WinUIrc.ver"'
    ))

    $resource = Join-Path $directory 'VersionInfo.res'
    $binary = Join-Path $directory 'VersionInfo.dll'
    & $resourceCompiler.Source /nologo /fo $resource (Join-Path $directory 'VersionInfo.rc')
    if ($LASTEXITCODE -ne 0)
    {
        throw "Resource compilation failed for $Name."
    }
    & $linker.Source /nologo /dll /noentry /machine:x64 "/out:$binary" $resource
    if ($LASTEXITCODE -ne 0)
    {
        throw "Resource linking failed for $Name."
    }

    $info = [System.Diagnostics.FileVersionInfo]::GetVersionInfo($binary)
    $actualVersions = @(
        (@($info.FileMajorPart, $info.FileMinorPart, $info.FileBuildPart, $info.FilePrivatePart) -join '.'),
        $info.FileVersion,
        (@($info.ProductMajorPart, $info.ProductMinorPart, $info.ProductBuildPart, $info.ProductPrivatePart) -join '.'),
        $info.ProductVersion
    )
    $fields = @('Fixed FileVersion', 'String FileVersion', 'Fixed ProductVersion', 'String ProductVersion')
    for ($index = 0; $index -lt $fields.Count; $index++)
    {
        if ($actualVersions[$index] -cne $ExpectedVersions[$index])
        {
            throw "$Name $($fields[$index]): expected '$($ExpectedVersions[$index])', got '$($actualVersions[$index])'."
        }
    }
    if ($info.Comments -cne $ExpectedComments)
    {
        throw "$Name Comments: expected '$ExpectedComments', got '$($info.Comments)'."
    }
    Write-Output "PASS: $Name native fixed/string file/product versions and source revision."
}

try
{
    [System.IO.Directory]::CreateDirectory($testRoot) | Out-Null
    foreach ($version in @('102.6.6.21', '102.6.6.0', '65535.65535.65535.65535'))
    {
        Test-VersionResource -Name $version -Version $version -ExpectedVersions @($version, $version, $version, $version) `
            -ExpectedComments "SourceRevision=$defaultSourceRevision"
    }
    $otherSourceRevision = '89abcdef0123456789abcdef0123456789abcdef'
    Test-VersionResource -Name 'DifferentSourceRevision' -Version '102.6.6.21' `
        -ExpectedVersions @('102.6.6.21', '102.6.6.21', '102.6.6.21', '102.6.6.21') `
        -SourceRevision $otherSourceRevision -ExpectedComments "SourceRevision=$otherSourceRevision"
    $standaloneVersions = @(
        '3.3.0.0',
        "3.3.0.0 (fixture_machine-$defaultSourceRevision)",
        '3.0.0.0',
        '3.0'
    )
    Test-VersionResource -Name 'Standalone' -Version '' -ExpectedVersions $standaloneVersions
    Test-VersionResource -Name 'LegacyMonobuild' -Version '$(WindowsAppSDKBinaryVersion)' -ExpectedVersions $standaloneVersions

    $localizedResource = Join-Path $testRoot 'Localized.rc'
    $localizedBinary = Join-Path $testRoot 'Microsoft.ui.xaml.dll.mui'
    [System.IO.File]::WriteAllLines($localizedResource, @(
        '#include <winver.h>',
        'LANGUAGE 7, 1',
        '1 VERSIONINFO',
        'FILEVERSION 3,2,0,0',
        'PRODUCTVERSION 2,0,0,0',
        'BEGIN',
        '  BLOCK "StringFileInfo"',
        '  BEGIN',
        '    BLOCK "040704b0"',
        '    BEGIN',
        ('      VALUE "Comments", "SourceRevision={0}\0"' -f $defaultSourceRevision),
        '      VALUE "FileVersion", "3.2.0.2610\0"',
        '      VALUE "ProductVersion", "2.0\0"',
        '      VALUE "OriginalFilename", "Microsoft.ui.xaml.dll.mui\0"',
        '      VALUE "LegalCopyright", L"\x00a9 Microsoft Corporation\0"',
        '    END',
        '  END',
        '  BLOCK "VarFileInfo"',
        '  BEGIN',
        '    VALUE "Translation", 0x407, 1200',
        '  END',
        'END',
        'STRINGTABLE',
        'BEGIN',
        '  100 L"Lokalisierte Zeichenfolge"',
        'END'
    ))
    & $resourceCompiler.Source /nologo /fo (Join-Path $testRoot 'Localized.res') $localizedResource
    if ($LASTEXITCODE -ne 0)
    {
        throw 'Localized resource compilation failed.'
    }
    & $linker.Source /nologo /dll /noentry /machine:x64 "/out:$localizedBinary" (Join-Path $testRoot 'Localized.res')
    if ($LASTEXITCODE -ne 0)
    {
        throw 'Localized resource linking failed.'
    }
    $localizedInfo = [System.Diagnostics.FileVersionInfo]::GetVersionInfo($localizedBinary)
    if (-not ('LocalizedResourceFixture' -as [type]))
    {
        Add-Type -TypeDefinition @'
using System;
using System.ComponentModel;
using System.Runtime.InteropServices;
using System.Text;

public static class LocalizedResourceFixture
{
    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    private static extern IntPtr LoadLibraryEx(string path, IntPtr file, uint flags);
    [DllImport("kernel32.dll")]
    private static extern bool FreeLibrary(IntPtr module);
    [DllImport("user32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    private static extern int LoadString(IntPtr module, uint id, StringBuilder value, int capacity);

    public static string ReadString(string path)
    {
        IntPtr module = LoadLibraryEx(path, IntPtr.Zero, 0x22);
        if (module == IntPtr.Zero)
        {
            throw new Win32Exception();
        }
        try
        {
            StringBuilder value = new StringBuilder(256);
            if (LoadString(module, 100, value, value.Capacity) == 0)
            {
                throw new Win32Exception();
            }
            return value.ToString();
        }
        finally
        {
            FreeLibrary(module);
        }
    }
}
'@
    }
    foreach ($version in @('102.7.3.41', '65535.65535.65535.65535', '102.7.3.0'))
    {
        & (Join-Path $PSScriptRoot 'Set-LocalizedResourceVersion.ps1') -Path $localizedBinary -Version $version | Out-Null
        $info = [System.Diagnostics.FileVersionInfo]::GetVersionInfo($localizedBinary)
        $actualVersions = @(
            (@($info.FileMajorPart, $info.FileMinorPart, $info.FileBuildPart, $info.FilePrivatePart) -join '.'),
            $info.FileVersion,
            (@($info.ProductMajorPart, $info.ProductMinorPart, $info.ProductBuildPart, $info.ProductPrivatePart) -join '.'),
            $info.ProductVersion
        )
        if (@($actualVersions | Where-Object { $_ -cne $version }).Count -gt 0)
        {
            throw "Localized fixed/string file/product versions did not converge to '$version'."
        }
        foreach ($field in @('Comments', 'OriginalFilename', 'LegalCopyright', 'Language'))
        {
            if ($info.$field -cne $localizedInfo.$field)
            {
                throw "Stamping a localized version changed $field."
            }
        }
        if ([LocalizedResourceFixture]::ReadString($localizedBinary) -cne 'Lokalisierte Zeichenfolge')
        {
            throw 'Stamping a localized version changed another resource.'
        }
        Write-Output "PASS: localized MUI version '$version' preserves translated metadata."
    }

    $invalidVersions = @(
        @{ Version = "10$([char]0x0662).6.6.21"; Message = 'is not a numeric four-part version' },
        @{ Version = '102.6.6'; Message = 'is not a numeric four-part version' },
        @{ Version = '102.6.6.65536'; Message = 'outside the VERSIONINFO range' }
    )
    $invalidHeader = Join-Path $testRoot 'InvalidVersion.h'
    foreach ($case in $invalidVersions)
    {
        $env:WindowsAppSDKBinaryVersion = $case.Version
        $failure = $null
        try
        {
            & $generator -ProductMajor 3 -ProductMinor 0 -VersionInfoPath $invalidHeader 6>&1 | Out-Null
        }
        catch
        {
            $failure = $_.Exception.Message
        }
        if ($failure -notlike "*$($case.Message)*" -or (Test-Path -LiteralPath $invalidHeader))
        {
            throw "Invalid version '$($case.Version)' did not fail the expected validation before generating a header: $failure"
        }
        Write-Output "PASS: rejected '$($case.Version)' before generating a header."

        $beforeContents = [Convert]::ToBase64String([System.IO.File]::ReadAllBytes($localizedBinary))
        $failure = $null
        try
        {
            & (Join-Path $PSScriptRoot 'Set-LocalizedResourceVersion.ps1') -Path $localizedBinary -Version $case.Version | Out-Null
        }
        catch
        {
            $failure = $_
        }
        if (-not $failure -or [Convert]::ToBase64String([System.IO.File]::ReadAllBytes($localizedBinary)) -cne $beforeContents)
        {
            throw "Invalid version '$($case.Version)' did not fail before modifying the localized binary."
        }
        Write-Output "PASS: rejected '$($case.Version)' before modifying localized resources."
    }
}
finally
{
    $env:WindowsAppSDKBinaryVersion = $originalBinaryVersion
    if (Test-Path -LiteralPath $testRoot)
    {
        Remove-Item -LiteralPath $testRoot -Recurse -Force
    }
}

Write-Output 'WinUI version-info tests passed.'
return 0
