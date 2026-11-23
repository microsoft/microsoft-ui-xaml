# Copyright (c) Microsoft Corporation and Contributors.
# Licensed under the MIT License.

[CmdletBinding()]
param([string]$RepoRoot)

Set-StrictMode -Version 2.0
$ErrorActionPreference = 'Stop'

if (-not $RepoRoot)
{
    $RepoRoot = Join-Path $PSScriptRoot '..\..'
}
$RepoRoot = [System.IO.Path]::GetFullPath($RepoRoot)
$testRoot = Join-Path ([System.IO.Path]::GetTempPath()) ('WinUIManagedVersion-' + [guid]::NewGuid().ToString('N'))
$dotnet = (Get-Command dotnet -ErrorAction Stop).Source
$sdkPaths = @(& $dotnet --list-sdks | ForEach-Object {
    if ($_ -match '^(\S+) \[(.+)\]$')
    {
        Join-Path $Matches[2] $Matches[1]
    }
})
$compiler = Join-Path $sdkPaths[-1] 'Roslyn\bincore\csc.dll'
$frameworkReference = Join-Path $env:SystemRoot 'Microsoft.NET\Framework64\v4.0.30319\mscorlib.dll'
$keyFile = Join-Path $RepoRoot 'eng\key.snk'
$script:caseNumber = 0

function Test-ManagedVersion
{
    param(
        [string]$AssemblyName,
        [string]$Kind,
        [string]$BinaryVersion,
        [string]$InformationalVersion,
        [string]$ExpectedFileVersion,
        [string]$ExpectedProductVersion,
        [string]$ExpectedAssemblyVersion = '3.0.0.0',
        [string]$TargetFramework = 'net8.0',
        [int]$ProductBuildVersion = 0,
        [string]$ExpectedError
    )

    $script:caseNumber++
    $directory = Join-Path $testRoot ([string]$script:caseNumber)
    [System.IO.Directory]::CreateDirectory($directory) | Out-Null
    $import = Join-Path $RepoRoot 'eng\winui-version.props'
    if ($Kind -eq 'Compiler')
    {
        $import = Join-Path $testRoot 'compiler\Directory.Build.props'
    }
    $escapedImport = [System.Security.SecurityElement]::Escape($import)
    $project = Join-Path $directory 'VersionFixture.csproj'
    [System.IO.File]::WriteAllText($project, @"
<Project Sdk="Microsoft.NET.Sdk">
  <PropertyGroup>
    <TargetFramework>$TargetFramework</TargetFramework>
    <AssemblyName>$AssemblyName</AssemblyName>
    <ProductBuildVersion>$ProductBuildVersion</ProductBuildVersion>
    <SourceControlInformationFeatureSupported>true</SourceControlInformationFeatureSupported>
  </PropertyGroup>
  <Import Project="$escapedImport" />
</Project>
"@)
    $arguments = @(
        'msbuild', $project, '-nologo', '-verbosity:quiet', '-target:GenerateAssemblyInfo',
        '-getProperty:AssemblyVersion,FileVersion,InformationalVersion,GeneratedAssemblyInfoFile',
        '-p:ImportDirectoryBuildProps=false', '-p:ImportDirectoryBuildTargets=false',
        "-p:IntermediateOutputPath=$directory\obj\", "-p:OutDir=$directory\bin\",
        '-p:BypassFrameworkInstallChecks=true', '-p:SourceRevisionId=fixture_revision',
        "-p:WindowsAppSDKBinaryVersion=$BinaryVersion",
        "-p:WindowsAppSDKInformationalVersion=$InformationalVersion"
    )
    try
    {
        $ErrorActionPreference = 'Continue'
        $output = @(& $dotnet @arguments 2>&1)
        $result = $LASTEXITCODE
    }
    finally
    {
        $ErrorActionPreference = 'Stop'
    }
    $text = $output -join [Environment]::NewLine
    if ($ExpectedError)
    {
        if ($result -eq 0 -or $text -notlike "*$ExpectedError*")
        {
            throw "Expected '$ExpectedError' to reject '$BinaryVersion': $text"
        }
        Write-Output "PASS: rejected '$BinaryVersion' ($ExpectedError)."
        return
    }
    if ($result -ne 0)
    {
        throw "Assembly-info generation failed for ${AssemblyName}: $text"
    }
    $properties = ($text | ConvertFrom-Json).Properties
    $extension = if ($AssemblyName -eq 'XamlCompiler') { '.exe' } else { '.dll' }
    $binary = Join-Path $directory ($AssemblyName + $extension)
    $outputType = if ($extension -eq '.exe') { 'exe' } else { 'library' }
    $compilerArguments = @(
        '/nologo', "/target:$outputType", '/nostdlib+', "/reference:$frameworkReference",
        '/publicsign+', "/keyfile:$keyFile", "/out:$binary",
        $properties.GeneratedAssemblyInfoFile, (Join-Path $testRoot 'Program.cs')
    )
    $compilerOutput = @(& $dotnet $compiler @compilerArguments 2>&1)
    if ($LASTEXITCODE -ne 0)
    {
        throw "Fixture compilation failed: $($compilerOutput -join [Environment]::NewLine)"
    }
    $identity = [System.Reflection.AssemblyName]::GetAssemblyName($binary)
    $token = ([BitConverter]::ToString($identity.GetPublicKeyToken())).Replace('-', '').ToLowerInvariant()
    $versionInfo = [System.Diagnostics.FileVersionInfo]::GetVersionInfo($binary)
    $fixedFileVersion = @($versionInfo.FileMajorPart, $versionInfo.FileMinorPart, $versionInfo.FileBuildPart, $versionInfo.FilePrivatePart) -join '.'
    $fixedProductVersion = @($versionInfo.ProductMajorPart, $versionInfo.ProductMinorPart, $versionInfo.ProductBuildPart, $versionInfo.ProductPrivatePart) -join '.'
    $productCore = [version](($ExpectedProductVersion -split '[-+]', 2)[0])
    $expectedFixedProductVersion = '{0}.{1}.{2}.{3}' -f $productCore.Major, $productCore.Minor, [Math]::Max(0, $productCore.Build), [Math]::Max(0, $productCore.Revision)
    if ($identity.Name -cne $AssemblyName -or $identity.Version.ToString() -cne $ExpectedAssemblyVersion -or
        $token -cne 'de31ebe4ad15742b')
    {
        throw "Binding identity changed: $($identity.FullName)"
    }
    if ($fixedFileVersion -cne $ExpectedFileVersion -or $versionInfo.FileVersion -cne $ExpectedFileVersion -or
        $fixedProductVersion -cne $expectedFixedProductVersion -or $versionInfo.ProductVersion -cne $ExpectedProductVersion)
    {
        throw "$AssemblyName delivery versions: file '$($versionInfo.FileVersion)' / fixed '$fixedFileVersion', product '$($versionInfo.ProductVersion)' / fixed '$fixedProductVersion'; expected '$ExpectedFileVersion' / '$ExpectedProductVersion' / '$expectedFixedProductVersion'."
    }
    Write-Output "PASS: $AssemblyName ($TargetFramework): $ExpectedFileVersion / $ExpectedProductVersion; binding $ExpectedAssemblyVersion / $token."
}

try
{
    [System.IO.Directory]::CreateDirectory((Join-Path $testRoot 'compiler')) | Out-Null
    $escapedRoot = [System.Security.SecurityElement]::Escape($RepoRoot + '\')
    [System.IO.File]::WriteAllText((Join-Path $testRoot 'Directory.Build.props'),
        "<Project><PropertyGroup><ProjectRoot>$escapedRoot</ProjectRoot></PropertyGroup></Project>")
    Copy-Item -LiteralPath (Join-Path $RepoRoot 'src\XamlCompiler\Directory.Build.props') -Destination (Join-Path $testRoot 'compiler\Directory.Build.props')
    [System.IO.File]::WriteAllText((Join-Path $testRoot 'Program.cs'),
        'internal static class VersionFixture { public static void Main() {} }')

    Test-ManagedVersion -AssemblyName Microsoft.WinUI -Kind Projection `
        -BinaryVersion '102.3.4.35' -InformationalVersion '2.3.4-dev.experimentalz' `
        -ExpectedFileVersion '102.3.4.35' -ExpectedProductVersion '2.3.4-dev.experimentalz'
    foreach ($assembly in @('Microsoft.UI.Xaml.Markup.Compiler', 'Microsoft.UI.Xaml.Markup.Compiler.IO', 'Microsoft.UI.Xaml.Markup.Compiler.MSBuildInterop', 'XamlCompiler'))
    {
        $frameworks = if ($assembly -eq 'XamlCompiler') { @('net472') } else { @('net472', 'net8.0') }
        foreach ($framework in $frameworks)
        {
            Test-ManagedVersion -AssemblyName $assembly -Kind Compiler -TargetFramework $framework `
                -BinaryVersion '102.3.4.35' -InformationalVersion '2.3.4-dev.experimentalz' `
                -ExpectedFileVersion '102.3.4.35' -ExpectedProductVersion '2.3.4-dev.experimentalz'
        }
    }
    foreach ($kind in @('Projection', 'Compiler'))
    {
        Test-ManagedVersion -AssemblyName Microsoft.WinUI -Kind $kind `
            -ExpectedFileVersion '3.0.0.0' -ExpectedProductVersion '3.0.0.0+fixture_revision'
    }
    Test-ManagedVersion -AssemblyName Microsoft.UI.Xaml.Markup.Compiler -Kind Compiler -ProductBuildVersion 7 `
        -BinaryVersion '102.3.4.0' -InformationalVersion '2.3.4' `
        -ExpectedFileVersion '102.3.4.0' -ExpectedProductVersion '2.3.4' -ExpectedAssemblyVersion '3.0.0.7'
    Test-ManagedVersion -AssemblyName Microsoft.WinUI -Kind Projection `
        -BinaryVersion '102.3.4' -InformationalVersion '2.3.4' -ExpectedError 'numeric four-part version'
    Test-ManagedVersion -AssemblyName Microsoft.WinUI -Kind Projection `
        -BinaryVersion '102.3.4.65536' -InformationalVersion '2.3.4' -ExpectedError 'VERSIONINFO range'
    Test-ManagedVersion -AssemblyName Microsoft.WinUI -Kind Projection `
        -BinaryVersion '102.3.4.35' -ExpectedError 'WindowsAppSDKInformationalVersion is required'
}
finally
{
    if ([System.IO.Directory]::Exists($testRoot))
    {
        [System.IO.Directory]::Delete($testRoot, $true)
    }
}

Write-Output 'WinUI managed version-info tests passed.'
return 0
