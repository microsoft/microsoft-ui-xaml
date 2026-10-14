param([string]$SampleRoot)
$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
if (-not $SampleRoot) { $SampleRoot = Join-Path $repoRoot 'SampleValidation' }

$smoke = Get-Content (Join-Path $repoRoot 'Samples\XamlCppWinRTModules\XamlModuleSmoke.cpp') -Raw
if (-not $smoke.Contains('import XamlCppWinRTModulesSample.Application_Xaml;') -or $smoke -match 'XAML_(IMPL|USE)_MODULE') {
    throw 'Direct sample consumer is not using the plain public root import.'
}

$package = @(Get-ChildItem (Join-Path $repoRoot 'PackageStore') -Filter 'Microsoft.WindowsAppSDK.WinUI.*.nupkg' -File)
if ($package.Count -ne 1) { throw 'Expected one freshly built WinUI package.' }
$version = $package[0].BaseName.Substring('Microsoft.WindowsAppSDK.WinUI.'.Length)
$sampleCommit = 'a68afc06378f2d7e459bf81aa61b5aa7b499d5f5'
git clone https://github.com/hoshiizumiya/WinUICppwinrtModuleSample.git $SampleRoot
if ($LASTEXITCODE -ne 0) { throw 'Sample clone failed.' }
git -C $SampleRoot checkout --detach $sampleCommit
if ($LASTEXITCODE -ne 0) { throw 'Sample checkout failed.' }
# The external project must resolve its own SDK/package defaults, even when cloned below
# this repository. Stop MSBuild's upward search before it reaches WinUI's build settings.
foreach ($buildFile in @('Directory.Build.props', 'Directory.Build.targets')) {
    [IO.File]::WriteAllText((Join-Path $SampleRoot $buildFile), '<Project />')
}
Write-Host "Validating product sample $sampleCommit with WinUI $version"

$project = Join-Path $SampleRoot 'WinUICppwinrtModuleSample\WinUICppwinrtModuleSample.vcxproj'
$generatedRoot = Join-Path (Split-Path -Parent $project) 'Generated Files'
[xml]$projectXml = Get-Content $project
$manager = New-Object Xml.XmlNamespaceManager($projectXml.NameTable)
$manager.AddNamespace('m', 'http://schemas.microsoft.com/developer/msbuild/2003')
$reference = $projectXml.SelectSingleNode('//m:PackageReference[@Include="Microsoft.WindowsAppSDK.WinUI"]', $manager)
$reference.SetAttribute('Version', $version)
$projectXml.Save($project)

$feed = [Security.SecurityElement]::Escape((Join-Path $repoRoot 'PackageStore'))
$config = Join-Path $SampleRoot 'nuget.validation.config'
@"
<configuration>
  <packageSources><clear/><add key="Product" value="$feed"/><add key="nuget.org" value="https://api.nuget.org/v3/index.json"/></packageSources>
  <packageSourceMapping><clear/><packageSource key="Product"><package pattern="Microsoft.WindowsAppSDK.WinUI"/></packageSource><packageSource key="nuget.org"><package pattern="*"/></packageSource></packageSourceMapping>
</configuration>
"@ | Set-Content $config

$evidenceRoot = Join-Path $repoRoot 'BuildOutput\ProductSampleEvidence'
New-Item -ItemType Directory -Force $evidenceRoot | Out-Null
$compilerCommit = (& git -C $repoRoot rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0) { throw 'Compiler HEAD lookup failed.' }

foreach ($pathMode in @('Absolute', 'DefaultRelative')) {
    foreach ($configuration in @('Debug', 'Release')) {
        $buildArguments = @(
            $project, '/restore', '/t:Build', '/m:2', '/ds:false', "/p:Configuration=$configuration",
            '/p:Platform=x64', '/p:VisualStudioVersion=18.0', '/p:PlatformToolset=v145',
            '/p:WindowsTargetPlatformVersion=10.0.26100.0', '/p:WindowsPackageType=None', '/p:AppxPackage=false',
            "/p:RestoreConfigFile=$config", "/p:RestorePackagesPath=$SampleRoot\packages",
            "/p:OutDir=$SampleRoot\bin\$pathMode\$configuration\"
        )
        if ($pathMode -eq 'Absolute') {
            $intermediate = Join-Path $SampleRoot "obj\$configuration\"
            $evaluatedIntDir = $intermediate
            $buildArguments += "/p:IntDir=$intermediate"
        } else {
            # Do not override IntDir: this is the VS application default missed by the
            # previous absolute-path-only validation (relative x64\Debug\, etc.).
            $evaluationArguments = @($buildArguments | Where-Object { $_ -notin @('/restore', '/t:Build', '/m:2', '/ds:false') })
            $propertyOutput = & msbuild.exe @evaluationArguments /nologo /verbosity:quiet '-getProperty:IntDir,MSBuildProjectDirectory'
            if ($LASTEXITCODE -ne 0) { throw 'Sample default IntDir evaluation failed.' }
            $properties = ($propertyOutput -join [Environment]::NewLine | ConvertFrom-Json).Properties
            if (-not $properties.IntDir -or [IO.Path]::IsPathRooted($properties.IntDir)) {
                throw "Default-relative sample must actually use a relative IntDir, found '$($properties.IntDir)'."
            }
            $evaluatedIntDir = $properties.IntDir
            $intermediate = [IO.Path]::GetFullPath([IO.Path]::Combine($properties.MSBuildProjectDirectory, $properties.IntDir))
        }
        $buildLog = Join-Path $repoRoot "BuildOutput\binlogs\ProductSample.$pathMode.$configuration.log"
        & msbuild.exe @buildArguments "/binaryLogger:$repoRoot\BuildOutput\binlogs\ProductSample.$pathMode.$configuration.binlog" /fl "/flp:logfile=$buildLog;verbosity=normal"
        if ($LASTEXITCODE -ne 0) { throw "Sample $pathMode $configuration failed: $LASTEXITCODE" }
        if (Select-String -Path $buildLog -Pattern 'C7684|LNK4042' -Quiet) { throw "Sample $pathMode $configuration has conflicting module/object references." }

        $root = @(Get-ChildItem $generatedRoot -Filter 'Application_Xaml.g.ixx' -Recurse -File)
        if ($root.Count -ne 1) { throw 'Sample did not receive an automatic root aggregator.' }
        $rootText = Get-Content $root[0].FullName -Raw
        if (-not $rootText.Contains('export module WinUICppwinrtModuleSample.Application_Xaml;')) { throw 'Sample root module has the wrong identity.' }
        if ($rootText -match '#include|namespace |Application_Xaml\.Support') { throw 'Sample root is not a pure class-module aggregator.' }

        foreach ($obsolete in @('Application_Xaml.Support.g.ixx', 'XamlSupport.g.ixx', 'WinUICppwinrtModuleSample.Application_Xaml.Support.ifc')) {
            $obsoleteFiles = @(Get-ChildItem $generatedRoot, $intermediate -Filter $obsolete -Recurse -File -ErrorAction SilentlyContinue)
            if ($obsoleteFiles.Count -ne 0) {
                throw "Sample retained obsolete support artifact $obsolete."
            }
        }

        foreach ($class in @('App', 'MainWindow')) {
            $interface = @(Get-ChildItem $generatedRoot -Filter "$class.xaml.g.ixx" -Recurse -File)
            if ($interface.Count -ne 1) { throw "Sample lacks $class interface." }
            $identity = "WinUICppwinrtModuleSample.Application_Xaml.$class"
            $interfaceText = Get-Content $interface[0].FullName -Raw
            if (-not $interfaceText.Contains("export module $identity;")) { throw "$class interface has the wrong module identity." }
            if (-not $rootText.Contains("export import $identity;")) { throw "Root does not re-export $identity." }
            if ($class -eq 'App' -and $interfaceText.Contains('#include "App.g.h"')) { throw 'App interface incorrectly requires App.g.h.' }
            if (@(Get-ChildItem $intermediate -Filter "$identity.ifc" -Recurse -File).Count -ne 1) { throw "Sample lacks $class IFC." }
        }

        if (@(Get-ChildItem $intermediate -Filter 'WinUICppwinrtModuleSample.Application_Xaml.ifc' -Recurse -File).Count -ne 1) {
            throw 'Sample lacks the root XAML IFC.'
        }

        $generatedNativeFiles = @(Get-ChildItem $generatedRoot -Filter '*.xaml.g.cpp' -Recurse -File) +
            @(Get-ChildItem $generatedRoot -Filter 'XamlTypeInfo*.g.cpp' -Recurse -File) +
            @(Get-ChildItem $generatedRoot -Filter '*.g.ixx' -Recurse -File)
        foreach ($generated in $generatedNativeFiles) {
            $text = Get-Content $generated.FullName -Raw
            if ($text -match '(?<!\r)\n|\r(?!\n)') { throw "Sample generated file has inconsistent CRLF: $($generated.FullName)" }
            if (-not $text.StartsWith("//------------------------------------------------------------------------------`r`n// <auto-generated>")) {
                throw "Sample generated file lacks its leading banner: $($generated.FullName)"
            }
            if ([regex]::Matches($text, '// <auto-generated>').Count -ne 1) { throw "Sample generated file contains duplicate banners: $($generated.FullName)" }
            if ($generated.Extension -eq '.ixx') {
                if ($text.Contains('#pragma once') -or $text.Contains('#ifndef XAML_IMPL_MODULE')) {
                    throw "Sample declaration module embeds a legacy header preamble: $($generated.FullName)"
                }
            } elseif ($text -match '(?m)^module;\r?$' -or $text.Contains('import std;')) {
                throw "Sample generated ordinary TU has module/STL ownership regression: $($generated.FullName)"
            }
        }

        # Snapshot each configuration before the shared Generated Files directory is
        # overwritten. Keep text/source evidence downloadable independently of IFC/binlogs.
        $configurationEvidence = Join-Path $evidenceRoot "$pathMode\$configuration"
        New-Item -ItemType Directory -Force $configurationEvidence | Out-Null
        $snapshotFiles = @($generatedNativeFiles) +
            @(Get-ChildItem $generatedRoot -Filter '*.xaml.g.h' -Recurse -File) +
            @(Get-ChildItem $generatedRoot -Filter '*.xaml.g.hpp' -Recurse -File)
        foreach ($generated in @($snapshotFiles | Sort-Object FullName -Unique)) {
            $relativePath = $generated.FullName.Substring($generatedRoot.Length).TrimStart([char[]]'\/')
            $destination = Join-Path $configurationEvidence $relativePath
            New-Item -ItemType Directory -Force (Split-Path -Parent $destination) | Out-Null
            Copy-Item $generated.FullName $destination
        }
        $manifest = [ordered]@{
            CompilerCommit = $compilerCommit
            SampleCommit = $sampleCommit
            WinUIVersion = $version
            PathMode = $pathMode
            Configuration = $configuration
            Project = $project
            EvaluatedIntDir = $evaluatedIntDir
            ResolvedIntermediateDirectory = $intermediate
            BuildLog = [IO.Path]::GetFileName($buildLog)
            BuildPassed = $true
            GeneratedSourceChecksPassed = $true
            NoChangeBuildPassed = $null
        }
        $manifestPath = Join-Path $configurationEvidence 'manifest.json'
        $manifest | ConvertTo-Json | Set-Content $manifestPath -Encoding UTF8

        if ($pathMode -eq 'DefaultRelative') {
            # Keep the same inputs and intermediates to exercise saved scanner mappings.
            $rebuildLog = Join-Path $repoRoot "BuildOutput\binlogs\ProductSample.$pathMode.$configuration.NoChange.log"
            & msbuild.exe @buildArguments "/binaryLogger:$repoRoot\BuildOutput\binlogs\ProductSample.$pathMode.$configuration.NoChange.binlog" /fl "/flp:logfile=$rebuildLog;verbosity=normal"
            if ($LASTEXITCODE -ne 0) { throw "Sample $pathMode $configuration no-change build failed: $LASTEXITCODE" }
            if (Select-String -Path $rebuildLog -Pattern 'C7684|LNK4042' -Quiet) { throw "Sample no-change build has conflicting module/object references." }
            $manifest.NoChangeBuildPassed = $true
            $manifest['NoChangeBuildLog'] = [IO.Path]::GetFileName($rebuildLog)
            $manifest | ConvertTo-Json | Set-Content $manifestPath -Encoding UTF8
        }
    }
}



