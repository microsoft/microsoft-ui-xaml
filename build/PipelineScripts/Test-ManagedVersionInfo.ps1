# Copyright (c) Microsoft Corporation and Contributors.
# Licensed under the MIT License.

[CmdletBinding()]
param([string]$RepoRoot, [string]$MSBuildPath)

Set-StrictMode -Version 2.0
$ErrorActionPreference = 'Stop'

if (-not $RepoRoot)
{
    $RepoRoot = Join-Path $PSScriptRoot '..\..'
}
$RepoRoot = [System.IO.Path]::GetFullPath($RepoRoot)
$testRoot = Join-Path ([System.IO.Path]::GetTempPath()) ('WinUIManagedVersion-' + [guid]::NewGuid().ToString('N'))
$dotnet = (Get-Command dotnet -ErrorAction Stop).Source
$msbuild = if ($MSBuildPath) { (Get-Command $MSBuildPath -ErrorAction Stop).Source } else { $dotnet }
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

Add-Type -TypeDefinition @'
using System;
using System.ComponentModel;
using System.Runtime.InteropServices;
public static class WinUIManagedVersionResourceTest
{
    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    private static extern IntPtr LoadLibraryEx(string path, IntPtr file, uint flags);
    [DllImport("kernel32.dll", CharSet = CharSet.Unicode)]
    private static extern IntPtr FindResource(IntPtr module, IntPtr name, IntPtr type);
    [DllImport("kernel32.dll")]
    private static extern uint SizeofResource(IntPtr module, IntPtr resource);
    [DllImport("kernel32.dll")]
    private static extern IntPtr LoadResource(IntPtr module, IntPtr resource);
    [DllImport("kernel32.dll")]
    private static extern IntPtr LockResource(IntPtr resource);
    [DllImport("kernel32.dll")]
    private static extern bool FreeLibrary(IntPtr module);
    public static byte[] Read(string path, int type, int name)
    {
        IntPtr module = LoadLibraryEx(path, IntPtr.Zero, 0x22);
        if (module == IntPtr.Zero) throw new Win32Exception();
        try
        {
            IntPtr resource = FindResource(module, new IntPtr(name), new IntPtr(type));
            if (resource == IntPtr.Zero) return new byte[0];
            byte[] data = new byte[SizeofResource(module, resource)];
            Marshal.Copy(LockResource(LoadResource(module, resource)), data, 0, data.Length);
            return data;
        }
        finally { FreeLibrary(module); }
    }
}
'@

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
        [ValidateSet('Default', 'Custom', 'None')][string]$Manifest = 'Default',
        [switch]$WithIcon,
        [switch]$VerifyIncremental,
        [string]$RetryBinaryVersion,
        [switch]$PartialResourceOutput,
        [string]$ExistingResource,
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
    $escapedSource = [System.Security.SecurityElement]::Escape((Join-Path $testRoot 'Program.cs'))
    $escapedKey = [System.Security.SecurityElement]::Escape($keyFile)
    $escapedReference = [System.Security.SecurityElement]::Escape($frameworkReference)
    $escapedMetadata = [System.Security.SecurityElement]::Escape(('Fixture "title" \ ' + [char]0xa9))
    $extension = if ($AssemblyName -eq 'XamlCompiler') { '.exe' } else { '.dll' }
    $outputType = if ($extension -eq '.exe') { 'exe' } else { 'library' }
    $manifestPath = if ($Manifest -eq 'Custom') { Join-Path $testRoot 'custom.manifest' } else { '' }
    $iconPath = if ($WithIcon) { Join-Path $testRoot 'fixture.ico' } else { '' }
    $noManifest = if ($Manifest -eq 'None') { 'true' } else { 'false' }
    $project = Join-Path $directory 'VersionFixture.csproj'
    [System.IO.File]::WriteAllText($project, @"
<Project Sdk="Microsoft.NET.Sdk">
  <PropertyGroup>
    <TargetFramework>$TargetFramework</TargetFramework>
    <AssemblyName>$AssemblyName</AssemblyName>
    <AssemblyTitle>$escapedMetadata</AssemblyTitle>
    <Company>$escapedMetadata</Company>
    <OutputType>$outputType</OutputType>
    <ProductBuildVersion>$ProductBuildVersion</ProductBuildVersion>
    <SourceControlInformationFeatureSupported>true</SourceControlInformationFeatureSupported>
    <EnableDefaultCompileItems>false</EnableDefaultCompileItems>
    <UseSharedCompilation>false</UseSharedCompilation>
    <SignAssembly>true</SignAssembly>
    <PublicSign>true</PublicSign>
    <AssemblyOriginatorKeyFile>$escapedKey</AssemblyOriginatorKeyFile>
    <ApplicationManifest>$manifestPath</ApplicationManifest>
    <NoWin32Manifest>$noManifest</NoWin32Manifest>
    <ApplicationIcon>$iconPath</ApplicationIcon>
    <Win32Resource>$ExistingResource</Win32Resource>
  </PropertyGroup>
  <ItemGroup>
    <Compile Include="$escapedSource" />
    <ReferencePathWithRefAssemblies Include="$escapedReference" />
  </ItemGroup>
  <Import Project="$escapedImport" />
</Project>
"@)
    $arguments = @()
    if (-not $MSBuildPath) { $arguments += 'msbuild' }
    $arguments += @(
        $project, '-nologo', '-verbosity:quiet', '-target:ResolveKeySource;GenerateAssemblyInfo;CoreCompile',
        '-getProperty:AssemblyVersion,FileVersion,InformationalVersion,GeneratedAssemblyInfoFile,TargetFileName,IntermediateOutputPath,_WindowsAppSDKManagedVersionResource,Company,AssemblyTitle',
        '-p:ImportDirectoryBuildProps=false', '-p:ImportDirectoryBuildTargets=false',
        "-p:IntermediateOutputPath=$directory\obj\", "-p:OutDir=$directory\bin\",
        '-p:BypassFrameworkInstallChecks=true', '-p:SourceRevisionId=fixture_revision',
        "-p:WindowsAppSDKBinaryVersion=$BinaryVersion",
        "-p:WindowsAppSDKInformationalVersion=$InformationalVersion"
    )
    try
    {
        $ErrorActionPreference = 'Continue'
        $output = @(& $msbuild @arguments 2>&1)
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
        throw "SDK fixture compilation failed for ${AssemblyName}: $text"
    }
    $properties = ($text | ConvertFrom-Json).Properties
    $binary = Join-Path $properties.IntermediateOutputPath $properties.TargetFileName
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
    $expectedInformation = if ($BinaryVersion) { $InformationalVersion } else { $ExpectedProductVersion }
    $attributes = [System.Reflection.Assembly]::Load([System.IO.File]::ReadAllBytes($binary)).GetCustomAttributesData()
    $information = @($attributes | Where-Object { $_.AttributeType.FullName -eq 'System.Reflection.AssemblyInformationalVersionAttribute' })
    if ($information.Count -ne 1 -or $information[0].ConstructorArguments[0].Value -cne $expectedInformation)
    {
        throw "$AssemblyName lost its AssemblyInformationalVersion '$expectedInformation'."
    }
    if ($versionInfo.CompanyName -cne $properties.Company -or $versionInfo.FileDescription -cne $properties.AssemblyTitle)
    {
        throw "$AssemblyName changed the company/title resource text, including escaped characters."
    }

    $baselineDirectory = Join-Path $directory 'baseline'
    [System.IO.Directory]::CreateDirectory($baselineDirectory) | Out-Null
    $baseline = Join-Path $baselineDirectory $properties.TargetFileName
    $compilerArguments = @(
        '/nologo', "/target:$outputType", '/nostdlib+', "/reference:$frameworkReference",
        '/publicsign+', "/keyfile:$keyFile", "/out:$baseline",
        $properties.GeneratedAssemblyInfoFile, (Join-Path $testRoot 'Program.cs')
    )
    if ($manifestPath) { $compilerArguments += "/win32manifest:$manifestPath" }
    if ($noManifest -eq 'true') { $compilerArguments += '/nowin32manifest' }
    if ($iconPath) { $compilerArguments += "/win32icon:$iconPath" }
    $compilerOutput = @(& $dotnet $compiler @compilerArguments 2>&1)
    if ($LASTEXITCODE -ne 0)
    {
        throw "Baseline compilation failed: $($compilerOutput -join [Environment]::NewLine)"
    }
    $baselineVersionInfo = [System.Diagnostics.FileVersionInfo]::GetVersionInfo($baseline)
    if ($RetryBinaryVersion)
    {
        $fakeSdk = Join-Path $directory 'sdk'
        foreach ($architecture in @('x86', 'x64', 'arm64'))
        {
            $compilerDirectory = Join-Path $fakeSdk "bin\10.0.0.0\$architecture"
            [System.IO.Directory]::CreateDirectory($compilerDirectory) | Out-Null
            Copy-Item -LiteralPath $failureCompiler -Destination (Join-Path $compilerDirectory 'rc.exe')
            if ($PartialResourceOutput)
            {
                [System.IO.File]::WriteAllText((Join-Path $compilerDirectory 'partial-output'), '')
            }
        }
        $retryArguments = @($arguments | Where-Object { $_ -notlike '-p:WindowsAppSDKBinaryVersion=*' })
        $retryArguments += "-p:WindowsAppSDKBinaryVersion=$RetryBinaryVersion"
        try
        {
            $ErrorActionPreference = 'Continue'
            $failedOutput = @(& $msbuild @retryArguments "-p:WindowsSdkDir=$fakeSdk" 2>&1)
            $failedResult = $LASTEXITCODE
        }
        finally
        {
            $ErrorActionPreference = 'Stop'
        }
        $receipt = $properties._WindowsAppSDKManagedVersionResource + '.failed'
        $failureMode = if ($PartialResourceOutput) { 'partial' } else { 'none' }
        if ($failedResult -eq 0 -or -not [System.IO.File]::Exists($receipt) -or
            [System.IO.File]::ReadAllText($receipt) -cne $failureMode)
        {
            throw "Resource compiler failure was not injected: $($failedOutput -join [Environment]::NewLine)"
        }
        $retryOutput = @(& $msbuild @retryArguments 2>&1)
        if ($LASTEXITCODE -ne 0)
        {
            throw "Failed-regeneration retry did not compile: $($retryOutput -join [Environment]::NewLine)"
        }
        $properties = (($retryOutput -join [Environment]::NewLine) | ConvertFrom-Json).Properties
        $retryIdentity = [System.Reflection.AssemblyName]::GetAssemblyName($binary)
        $versionInfo = [System.Diagnostics.FileVersionInfo]::GetVersionInfo($binary)
        $retryVersions = @(
            (@($versionInfo.FileMajorPart, $versionInfo.FileMinorPart, $versionInfo.FileBuildPart, $versionInfo.FilePrivatePart) -join '.'),
            $versionInfo.FileVersion,
            (@($versionInfo.ProductMajorPart, $versionInfo.ProductMinorPart, $versionInfo.ProductBuildPart, $versionInfo.ProductPrivatePart) -join '.'),
            $versionInfo.ProductVersion
        )
        $retryAttributes = [System.Reflection.Assembly]::Load([System.IO.File]::ReadAllBytes($binary)).GetCustomAttributesData()
        $fileAttribute = @($retryAttributes | Where-Object { $_.AttributeType.FullName -eq 'System.Reflection.AssemblyFileVersionAttribute' })
        $information = @($retryAttributes | Where-Object { $_.AttributeType.FullName -eq 'System.Reflection.AssemblyInformationalVersionAttribute' })
        if (@($retryVersions | Where-Object { $_ -cne $RetryBinaryVersion }).Count -gt 0 -or
            $fileAttribute.Count -ne 1 -or $fileAttribute[0].ConstructorArguments[0].Value -cne $RetryBinaryVersion -or
            $information.Count -ne 1 -or $information[0].ConstructorArguments[0].Value -cne $expectedInformation -or
            $retryIdentity.FullName -cne $identity.FullName)
        {
            throw "Failed-regeneration retry emitted '$($retryVersions -join '; ')'; expected '$RetryBinaryVersion' with preserved attributes and identity."
        }
        $arguments = $retryArguments
        $ExpectedFileVersion = $RetryBinaryVersion
        $ExpectedProductVersion = $RetryBinaryVersion
        Write-Output "PASS: failed-regeneration retry ($failureMode output) uses '$RetryBinaryVersion'."
    }
    foreach ($field in @('InternalName', 'OriginalFilename', 'CompanyName', 'FileDescription', 'ProductName',
        'LegalCopyright', 'LegalTrademarks', 'Comments', 'IsDebug', 'IsPatched', 'IsPrivateBuild', 'IsPreRelease', 'IsSpecialBuild'))
    {
        if ($versionInfo.$field -cne $baselineVersionInfo.$field)
        {
            throw "$AssemblyName changed native resource metadata ${field}: '$($versionInfo.$field)' instead of '$($baselineVersionInfo.$field)'."
        }
    }
    $versionResource = [System.Text.Encoding]::Unicode.GetString([WinUIManagedVersionResourceTest]::Read($binary, 16, 1))
    if ($versionResource -cnotmatch ('Assembly Version\0+' + [regex]::Escape($ExpectedAssemblyVersion) + '\0'))
    {
        throw "$AssemblyName lost the preserved 'Assembly Version' resource string."
    }

    if ($VerifyIncremental)
    {
        $resourceTime = [System.IO.File]::GetLastWriteTimeUtc($properties._WindowsAppSDKManagedVersionResource)
        $binaryTime = [System.IO.File]::GetLastWriteTimeUtc($binary)
        $repeatOutput = @(& $msbuild @arguments 2>&1)
        if ($LASTEXITCODE -ne 0)
        {
            throw "Incremental fixture compilation failed: $($repeatOutput -join [Environment]::NewLine)"
        }
        if ($resourceTime -ne [System.IO.File]::GetLastWriteTimeUtc($properties._WindowsAppSDKManagedVersionResource) -or
            $binaryTime -ne [System.IO.File]::GetLastWriteTimeUtc($binary))
        {
            throw 'An unchanged build rewrote the version resource or managed binary.'
        }
    }
    if ($outputType -eq 'exe')
    {
        foreach ($resource in @(@(24, 1), @(14, 32512), @(3, 1)))
        {
            $actual = [WinUIManagedVersionResourceTest]::Read($binary, $resource[0], $resource[1])
            $expected = [WinUIManagedVersionResourceTest]::Read($baseline, $resource[0], $resource[1])
            $actualValue = [Convert]::ToBase64String($actual)
            $expectedValue = [Convert]::ToBase64String($expected)
            if ($resource[0] -eq 24 -and $Manifest -eq 'Default')
            {
                # Framework and SDK compilers differ only in the default manifest's whitespace/BOM.
                $actualXml = [xml][System.Text.Encoding]::UTF8.GetString($actual).TrimStart([char]0xfeff)
                $expectedXml = [xml][System.Text.Encoding]::UTF8.GetString($expected).TrimStart([char]0xfeff)
                $actualValue = $actualXml.OuterXml
                $expectedValue = $expectedXml.OuterXml
            }
            if ($actualValue -cne $expectedValue)
            {
                throw "$AssemblyName changed resource type $($resource[0]) / name $($resource[1])."
            }
        }
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
    $failureSource = Join-Path $testRoot 'FailingResourceCompiler.cs'
    $failureCompiler = Join-Path $testRoot 'FailingResourceCompiler.exe'
    [System.IO.File]::WriteAllText($failureSource, @'
using System;
using System.IO;
internal static class FailingResourceCompiler
{
    public static int Main(string[] arguments)
    {
        int output = Array.IndexOf(arguments, "/fo") + 1;
        if (output == 0 || output >= arguments.Length) return 43;
        bool partial = File.Exists(Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "partial-output"));
        if (partial) File.WriteAllBytes(arguments[output], new byte[] { 0, 1, 2, 3 });
        File.WriteAllText(arguments[output] + ".failed", partial ? "partial" : "none");
        return 42;
    }
}
'@)
    $failureCompilerOutput = @(& $dotnet $compiler /nologo /target:exe /nostdlib+ "/reference:$frameworkReference" "/out:$failureCompiler" $failureSource 2>&1)
    if ($LASTEXITCODE -ne 0)
    {
        throw "Failure compiler fixture did not compile: $($failureCompilerOutput -join [Environment]::NewLine)"
    }
    [System.IO.File]::WriteAllText((Join-Path $testRoot 'custom.manifest'),
        '<assembly xmlns="urn:schemas-microsoft-com:asm.v1" manifestVersion="1.0"><assemblyIdentity version="7.6.5.4" name="VersionFixture.Custom" /><trustInfo xmlns="urn:schemas-microsoft-com:asm.v3"><security><requestedPrivileges><requestedExecutionLevel level="asInvoker" uiAccess="false" /></requestedPrivileges></security></trustInfo></assembly>')
    $icon = New-Object System.IO.MemoryStream
    $writer = New-Object System.IO.BinaryWriter $icon
    try
    {
        $writer.Write([byte[]]@(0, 0, 1, 0, 1, 0, 1, 1, 0, 0, 1, 0, 32, 0, 48, 0, 0, 0, 22, 0, 0, 0))
        foreach ($value in @(40, 1, 2)) { $writer.Write([int]$value) }
        $writer.Write([int16]1)
        $writer.Write([int16]32)
        foreach ($value in @(0, 4, 0, 0, 0, 0)) { $writer.Write([int]$value) }
        $writer.Write([byte[]]@(0, 0, 255, 255, 0, 0, 0, 0))
        [System.IO.File]::WriteAllBytes((Join-Path $testRoot 'fixture.ico'), $icon.ToArray())
    }
    finally
    {
        $writer.Dispose()
        $icon.Dispose()
    }

    Test-ManagedVersion -AssemblyName Microsoft.WinUI -Kind Projection -VerifyIncremental `
        -BinaryVersion '102.3.4.35' -InformationalVersion '2.3.4-dev.experimentalz' `
        -ExpectedFileVersion '102.3.4.35' -ExpectedProductVersion '102.3.4.35' -RetryBinaryVersion '102.3.4.36'
    Test-ManagedVersion -AssemblyName XamlCompiler -Kind Compiler -TargetFramework net472 -Manifest Custom -WithIcon `
        -VerifyIncremental -PartialResourceOutput -RetryBinaryVersion '102.3.4.36' `
        -BinaryVersion '102.3.4.35' -InformationalVersion '2.3.4-dev.experimentalz' `
        -ExpectedFileVersion '102.3.4.35' -ExpectedProductVersion '102.3.4.35'
    Test-ManagedVersion -AssemblyName Microsoft.WinUI -Kind Projection `
        -BinaryVersion '102.3.4.35' -InformationalVersion '2.3.4-dev.experimentalz' `
        -ExpectedFileVersion '102.3.4.35' -ExpectedProductVersion '102.3.4.35'
    foreach ($assembly in @('Microsoft.UI.Xaml.Markup.Compiler', 'Microsoft.UI.Xaml.Markup.Compiler.IO', 'Microsoft.UI.Xaml.Markup.Compiler.MSBuildInterop', 'XamlCompiler'))
    {
        $frameworks = if ($assembly -eq 'XamlCompiler') { @('net472') } else { @('net472', 'net8.0') }
        foreach ($framework in $frameworks)
        {
            Test-ManagedVersion -AssemblyName $assembly -Kind Compiler -TargetFramework $framework `
                -BinaryVersion '102.3.4.35' -InformationalVersion '2.3.4-dev.experimentalz' `
                -ExpectedFileVersion '102.3.4.35' -ExpectedProductVersion '102.3.4.35'
        }
    }
    foreach ($kind in @('Projection', 'Compiler'))
    {
        Test-ManagedVersion -AssemblyName Microsoft.WinUI -Kind $kind `
            -ExpectedFileVersion '3.0.0.0' -ExpectedProductVersion '3.0.0.0+fixture_revision'
    }
    Test-ManagedVersion -AssemblyName Microsoft.UI.Xaml.Markup.Compiler -Kind Compiler -ProductBuildVersion 7 `
        -BinaryVersion '102.3.4.0' -InformationalVersion '2.3.4' `
        -ExpectedFileVersion '102.3.4.0' -ExpectedProductVersion '102.3.4.0' -ExpectedAssemblyVersion '3.0.0.7'
    Test-ManagedVersion -AssemblyName Microsoft.WinUI -Kind Projection -VerifyIncremental `
        -BinaryVersion '12.4.0.1234' -InformationalVersion '2.4.0+commit-test' `
        -ExpectedFileVersion '12.4.0.1234' -ExpectedProductVersion '12.4.0.1234'
    Test-ManagedVersion -AssemblyName XamlCompiler -Kind Compiler -TargetFramework net472 -Manifest Custom -WithIcon `
        -BinaryVersion '12.4.0.1234' -InformationalVersion '2.4.0+commit-test' `
        -ExpectedFileVersion '12.4.0.1234' -ExpectedProductVersion '12.4.0.1234'
    Test-ManagedVersion -AssemblyName XamlCompiler -Kind Compiler -TargetFramework net472 -Manifest None `
        -BinaryVersion '102.3.4.0' -InformationalVersion '2.3.4' `
        -ExpectedFileVersion '102.3.4.0' -ExpectedProductVersion '102.3.4.0'
    Test-ManagedVersion -AssemblyName Microsoft.WinUI -Kind Projection `
        -BinaryVersion '102.3.4' -InformationalVersion '2.3.4' -ExpectedError 'numeric four-part version'
    Test-ManagedVersion -AssemblyName Microsoft.WinUI -Kind Projection `
        -BinaryVersion '102.3.4.65536' -InformationalVersion '2.3.4' -ExpectedError 'VERSIONINFO range'
    Test-ManagedVersion -AssemblyName Microsoft.WinUI -Kind Projection `
        -BinaryVersion '102.3.4.35' -ExpectedError 'WindowsAppSDKInformationalVersion is required'
    Test-ManagedVersion -AssemblyName Microsoft.WinUI -Kind Projection -ExistingResource existing.res `
        -BinaryVersion '102.3.4.35' -InformationalVersion '2.3.4' `
        -ExpectedError 'Cannot replace an existing Win32Resource'
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
