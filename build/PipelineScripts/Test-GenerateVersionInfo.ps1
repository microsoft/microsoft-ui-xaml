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

function Test-VersionResource
{
    Param(
        [string]$Name,
        [string]$Version,
        [string[]]$ExpectedVersions
    )

    $directory = Join-Path $testRoot $Name
    [System.IO.Directory]::CreateDirectory($directory) | Out-Null
    $env:WindowsAppSDKBinaryVersion = if ($Version) { $Version } else { $null }

    & $generator -ProductMajor 3 -ProductMinor 0 -VersionInfoPath (Join-Path $directory 'WindowsAppSDK-ProductInfo.h') 6>&1 | Out-Null
    Copy-Item -LiteralPath $resourceTemplate -Destination (Join-Path $directory 'WinUIrc.ver')
    [System.IO.File]::WriteAllLines((Join-Path $directory 'WinUISourceRevision.h'), @(
        '#define WINUI_BUILD_MACHINE fixture_machine',
        '#define WINUI_SOURCE_REVISION_ID fixture_revision'
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
    Write-Output "PASS: $Name native fixed/string file/product versions."
}

try
{
    [System.IO.Directory]::CreateDirectory($testRoot) | Out-Null
    foreach ($version in @('102.6.6.21', '102.6.6.0', '65535.65535.65535.65535'))
    {
        Test-VersionResource -Name $version -Version $version -ExpectedVersions @($version, $version, $version, $version)
    }
    $standaloneVersions = @(
        '3.3.0.0',
        '3.3.0.0 (fixture_machine-fixture_revision)',
        '3.0.0.0',
        '3.0'
    )
    Test-VersionResource -Name 'Standalone' -Version '' -ExpectedVersions $standaloneVersions
    Test-VersionResource -Name 'LegacyMonobuild' -Version '$(WindowsAppSDKBinaryVersion)' -ExpectedVersions $standaloneVersions

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
