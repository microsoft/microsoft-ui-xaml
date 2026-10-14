param(
    [string]$ModuleVisualStudioVersion = '18.0',
    [string]$ModulePlatformToolset = 'v145',
    [string]$ModuleWindowsSdkVersion = '10.0.26100.0'
)

$ErrorActionPreference = 'Stop'

if (-not $env:VisualStudioVersion -or [version]$env:VisualStudioVersion -lt [version]'18.0') {
    throw "C++/WinRT 3.x named-module validation requires Visual Studio 2026 / MSVC v145 or later. Current VisualStudioVersion='$($env:VisualStudioVersion)'."
}

Write-Host "C++/WinRT named-module validation: VisualStudioVersion=$ModuleVisualStudioVersion PlatformToolset=$ModulePlatformToolset WindowsSdk=$ModuleWindowsSdkVersion VCToolsInstallDir=$env:VCToolsInstallDir"

$programFilesX86 = [Environment]::GetFolderPath([Environment+SpecialFolder]::ProgramFilesX86)
$installedSdkLib = Join-Path $programFilesX86 "Windows Kits\10\Lib\$ModuleWindowsSdkVersion"
if (-not (Test-Path $installedSdkLib)) {
    throw "Windows SDK $ModuleWindowsSdkVersion is required for named-module validation but was not found at '$installedSdkLib'."
}

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$binlogDir = Join-Path $repoRoot 'BuildOutput\binlogs'
New-Item -ItemType Directory -Force -Path $binlogDir | Out-Null

# The job builds XamlCompiler in Debug and consumes the x64 Check product artifact for native fixtures.
# Pin native metadata and XamlC props/targets to those artifacts, including during Release configuration.
$compilerPackageDirectory = Join-Path $repoRoot 'BuildOutput\packaging\Debug\build'
$validationArtifactsObjDirectory = Join-Path $repoRoot 'BuildOutput\obj\amd64chk'
$validationArtifactsBinDirectory = Join-Path $repoRoot 'BuildOutput\bin\amd64chk'
$compilerProps = Join-Path $compilerPackageDirectory 'Microsoft.UI.Xaml.Markup.Compiler.props'
if (-not (Test-Path $compilerProps)) {
    throw "The Debug XamlCompiler package props were not found at '$compilerProps'."
}

$cppWinRTPackagesConfig = Join-Path $repoRoot 'src\XamlCompiler\Tests\RegressionProjects\Basic\CppWinRT\SimpleModules\packages.config'
[xml]$packagesConfig = Get-Content $cppWinRTPackagesConfig
$cppWinRTPackage = $packagesConfig.packages.package | Where-Object { $_.id -eq 'YexuanXiao.CppWinRTPlus' }
if (-not $cppWinRTPackage) {
    throw 'YexuanXiao.CppWinRTPlus was not found in the SimpleModules packages.config.'
}

Write-Host "C++/WinRT projection package: $($cppWinRTPackage.id) $($cppWinRTPackage.version)"
$cppWinRTPackageDir = Join-Path $repoRoot ("packages\{0}.{1}" -f $cppWinRTPackage.id, $cppWinRTPackage.version)
if (-not (Test-Path $cppWinRTPackageDir)) {
    $nuget = Get-Command nuget.exe -ErrorAction Stop
    & $nuget.Source install $cppWinRTPackage.id -Version $cppWinRTPackage.version -OutputDirectory (Join-Path $repoRoot 'packages') -Source https://api.nuget.org/v3/index.json -NonInteractive
    if ($LASTEXITCODE -ne 0) {
        throw "NuGet restore for $($cppWinRTPackage.id) $($cppWinRTPackage.version) failed with exit code $LASTEXITCODE."
    }
}

function Invoke-XamlModuleBuild {
    param(
        [Parameter(Mandatory)] [string]$RelativeProject,
        [Parameter(Mandatory)] [string]$LogName,
        [string[]]$ExtraProperties = @(),
        [string]$Target = 'Build',
        [string]$Configuration = 'Debug'
    )

    $project = Join-Path $repoRoot $RelativeProject
    $arguments = @(
        $project,
        "/t:$Target",
        "/p:Configuration=$Configuration",
        '/p:Platform=x64',
        "/p:VisualStudioVersion=$ModuleVisualStudioVersion",
        "/p:PlatformToolset=$ModulePlatformToolset",
        "/p:WindowsSdkTargetPlatformVersion=$ModuleWindowsSdkVersion",
        "/p:TargetPlatformVersion=$ModuleWindowsSdkVersion",
        "/p:WindowsTargetPlatformVersion=$ModuleWindowsSdkVersion",
        '/p:UseXamlCompiler=true',
        '/p:UseDebugWinUI=true',
        '/p:SkipXamlCompilerProjectReferences=true',
        "/p:ArtifactsObjDir=$validationArtifactsObjDirectory\",
        "/p:ArtifactsBinDir=$validationArtifactsBinDirectory\",
        "/p:XamlCompilerPropsAndTargetsDirectory=$compilerPackageDirectory\",
        '/p:IncludeXamlDlls=true',
        '/p:SpectreMitigation=false',
        '/m:2',
        '/ds:false',
        '/v:minimal',
        "/binaryLogger:$binlogDir\$LogName.binlog"
    ) + $ExtraProperties

    $textLog = Join-Path $binlogDir "$LogName.log"
    $previousPreference = $ErrorActionPreference
    try {
        # Capture warnings without losing the native process exit code to PowerShell's
        # stderr handling. Keep a readable log beside each binary build log.
        $ErrorActionPreference = 'Continue'
        & msbuild.exe @arguments 2>&1 | Tee-Object -FilePath $textLog | Out-Host
        $buildExit = $LASTEXITCODE
    }
    finally {
        $ErrorActionPreference = $previousPreference
    }
    if ($buildExit -ne 0) {
        throw "$LogName failed with exit code $buildExit."
    }
    if (Select-String -LiteralPath $textLog -Pattern '\bLNK4042\b' -Quiet) {
        throw "$LogName passed duplicate object inputs to the linker; see '$textLog'."
    }
}

$simpleModules = 'src\XamlCompiler\Tests\RegressionProjects\Basic\CppWinRT\SimpleModules\SimpleCppWinRTModules.vcxproj'
$staticProvider = 'src\XamlCompiler\Tests\RegressionProjects\Features\StaticLibs\StaticControlsModuleLib\StaticControlsModuleLib.vcxproj'
$staticConsumer = 'src\XamlCompiler\Tests\RegressionProjects\Features\StaticLibs\StaticControlsModuleConsumer\StaticControlsModuleConsumer.vcxproj'

function Get-OneGeneratedFile([string]$Directory, [string]$Name) {
    $files = @(Get-ChildItem -Path $Directory -Filter $Name -File -Recurse -ErrorAction SilentlyContinue)
    if ($files.Count -ne 1) { throw "Expected exactly one $Name under $Directory; found $($files.Count)." }
    return $files[0]
}

function Get-XamlClassModuleName([string]$Namespace, [string]$ClassName) {
    $normalized = $ClassName.Replace('::', '.')
    $prefix = "$Namespace."
    if ($normalized.StartsWith($prefix, [StringComparison]::Ordinal)) {
        $normalized = $normalized.Substring($prefix.Length)
    }
    return "$Namespace.Application_Xaml.$normalized"
}

function Assert-NoSupportModule([string]$Directory, [string]$Namespace) {
    foreach ($name in @('Application_Xaml.Support.g.ixx', 'XamlSupport.g.ixx', "$Namespace.Application_Xaml.Support.ifc")) {
        if (@(Get-ChildItem $Directory -Filter $name -File -Recurse -ErrorAction SilentlyContinue).Count -ne 0) {
            throw "Obsolete support-module artifact remains: $name"
        }
    }
}

function Assert-XamlGraph([string]$Directory, [string]$Namespace, [string[]]$Classes, [bool]$ExpectTypeInfo = $true) {
    $root = Get-OneGeneratedFile $Directory 'Application_Xaml.g.ixx'
    $text = Get-Content $root.FullName -Raw
    if ($text -match '#include|namespace |export import :') { throw 'The root must only aggregate independent named modules.' }
    if (-not $text.Contains("export module $Namespace.Application_Xaml;")) { throw 'Incorrect root module identity.' }
    Assert-NoSupportModule $Directory $Namespace

    foreach ($class in $Classes) {
        $identity = Get-XamlClassModuleName $Namespace $class
        if (-not $text.Contains("export import $identity;")) { throw "Root is missing $identity." }

        $source = @(Get-ChildItem $Directory -Filter '*.xaml.g.ixx' -File -Recurse | Where-Object {
            (Get-Content $_.FullName -Raw).Contains("export module $identity;")
        })
        if ($source.Count -ne 1) { throw "Expected one independent interface for $identity, found $($source.Count)." }
        $interface = Get-Content $source[0].FullName -Raw
        if ($class -eq 'Simple.MainPage' -and -not $interface.Contains("export import $Namespace.Application_Xaml.BindingInfo;")) {
            throw 'MainPage must re-export BindingInfo because its public template uses XamlBindings.'
        }
        if ($class -eq 'Simple.App' -and $ExpectTypeInfo -and -not $interface.Contains("export import $Namespace.Application_Xaml.TypeInfo;")) {
            throw 'App must re-export TypeInfo because its public template uses XamlMetaDataProvider.'
        }
        foreach ($required in @('#define XAML_IMPL_MODULE', '#undef XAML_IMPL_MODULE')) {
            if (-not $interface.Contains($required)) { throw "$identity lacks '$required'." }
        }
        if (-not $interface.Contains('import std;') -or -not $interface.Contains('import winrt_base;')) {
            throw "$identity is not using the C++/WinRT module-interface preamble."
        }

        $shortName = ($class -split '\.')[-1]
        if ($shortName -eq 'App') {
            if ($interface.Contains('#include "App.g.h"')) {
                throw 'App module incorrectly depends on a C++/WinRT App.g.h producer scaffold.'
            }
        } else {
            if (-not $interface.Contains('#define WINRT_IMPORT_MODULE')) {
                throw "$identity does not enable WINRT_IMPORT_MODULE for its producer scaffold."
            }
            if (-not $interface.Contains("#include `"$shortName.g.h`"")) {
                throw "$identity does not absorb its C++/WinRT producer scaffold."
            }
        }

        $sentinelPath = $source[0].FullName -replace '\.ixx$', '.h'
        $sentinel = Get-Content $sentinelPath -Raw
        if (-not $sentinel.Contains('named-module sentinel')) { throw "Missing module sentinel: $sentinelPath" }
        if ($sentinel -match 'struct\s+\w+T|#include\s+<winrt/|export\s+module|export extern') {
            throw "Module-mode sentinel still owns declarations: $sentinelPath"
        }
        Get-OneGeneratedFile $Directory "$identity.ifc" | Out-Null
    }

    $actualExports = @([regex]::Matches($text, 'export\s+import\s+[^;\r\n]+;'))
    if ($actualExports.Count -ne $Classes.Count) { throw "Aggregator exports $($actualExports.Count) modules; expected $($Classes.Count)." }
    Get-OneGeneratedFile $Directory "$Namespace.Application_Xaml.ifc" | Out-Null

    Get-OneGeneratedFile $Directory 'XamlBindingInfo.xaml.g.ixx' | Out-Null
    Get-OneGeneratedFile $Directory "$Namespace.Application_Xaml.BindingInfo.ifc" | Out-Null
    if ($ExpectTypeInfo) {
        Get-OneGeneratedFile $Directory 'XamlTypeInfo.xaml.g.ixx' | Out-Null
        Get-OneGeneratedFile $Directory "$Namespace.Application_Xaml.TypeInfo.ifc" | Out-Null
    } else {
        foreach ($name in @('XamlTypeInfo.xaml.g.ixx', "$Namespace.Application_Xaml.TypeInfo.ifc")) {
            if (@(Get-ChildItem $Directory -Filter $name -File -Recurse -ErrorAction SilentlyContinue).Count -ne 0) {
                throw "NoTypeInfoCodeGen retained $name."
            }
        }
    }
}

function Assert-OrdinaryGeneratedConsumer([string]$Path, [string[]]$RequiredImports, [string[]]$ExpectedTextualHeaders) {
    $text = Get-Content $Path -Raw
    if ($text -match '(?m)^module;$' -or $text -match '(?m)^module\s+[^;]+;') {
        throw "Ordinary generated TU became a module unit: $Path"
    }
    if ($text.Contains('import std;')) { throw "Ordinary generated TU imports std instead of owning STL textually: $Path" }
    if ($text -match '\.xaml\.g\.hpp') { throw "Ordinary generated TU refers to the obsolete XAML header contract: $Path" }
    if (-not $text.Contains('#define WINRT_IMPORT_MODULE')) { throw "Ordinary generated TU lacks WINRT_IMPORT_MODULE: $Path" }
    if (-not $text.Contains('import winrt_base;')) { throw "Ordinary generated TU lacks import winrt_base: $Path" }

    $actualTextualHeaders = @([regex]::Matches($text, '(?m)^#include <([^>]+)>\r?$') | ForEach-Object {
        $_.Groups[1].Value
    } | Where-Object {
        $_ -notin @('windows.h', 'unknwn.h', 'winrt/base_macros.h')
    })
    if (-not [string]::Equals(($actualTextualHeaders -join "`n"), ($ExpectedTextualHeaders -join "`n"), [StringComparison]::Ordinal)) {
        throw "Unexpected textual STL headers in $Path. Expected [$($ExpectedTextualHeaders -join ', ')], found [$($actualTextualHeaders -join ', ')]."
    }

    $firstImport = $text.IndexOf('import ', [StringComparison]::Ordinal)
    if ($firstImport -lt 0) { throw "Ordinary generated TU has no named-module imports: $Path" }
    foreach ($header in $ExpectedTextualHeaders) {
        $include = $text.IndexOf("#include <$header>", [StringComparison]::Ordinal)
        if ($include -lt 0 -or $include -gt $firstImport) {
            throw "Textual STL header <$header> must precede module imports: $Path"
        }
    }
    foreach ($required in $RequiredImports) {
        if (-not $text.Contains("import $required;")) { throw "$Path is missing import $required." }
    }
}

function Assert-RemovedClass([string]$Directory, [string]$ShortName, [string]$Identity) {
    foreach ($name in @("$ShortName.xaml.g.h", "$ShortName.xaml.g.cpp", "$ShortName.xaml.g.hpp", "$ShortName.xaml.g.hpp.backup", "$ShortName.xaml.g.ixx", "$Identity.ifc")) {
        if (@(Get-ChildItem $Directory -Filter $name -File -Recurse -ErrorAction SilentlyContinue).Count -ne 0) { throw "Removed class left $name." }
    }
    $orphanedModuleArtifacts = @(Get-ChildItem $Directory -File -Recurse | Where-Object {
        $_.DirectoryName -like '*\XamlModules*' -and $_.Name -like "$ShortName.xaml.g.ixx.*"
    })
    if ($orphanedModuleArtifacts.Count -ne 0) {
        $paths = ($orphanedModuleArtifacts | ForEach-Object { $_.FullName }) -join '; '
        throw "Removed class left scanner or object outputs: $paths"
    }
    if ((Get-Content (Get-OneGeneratedFile $Directory 'Application_Xaml.g.ixx').FullName -Raw).Contains("export import $Identity;")) {
        throw 'Removed class remains in the root.'
    }
}

$simpleGeneratedRoot = Join-Path $repoRoot 'BuildOutput\obj\amd64chk\CompilerTests\Basic\CppWinRT\SimpleModules'
$fixtureRoot = Split-Path (Join-Path $repoRoot $simpleModules)
$classes = @('Simple.App', 'Simple.MainPage', 'Simple.EmptyPage', 'Simple.Views.PanelPage', 'Simple.Controls.PanelPage')
Invoke-XamlModuleBuild $simpleModules 'SimpleModules.CleanModule'
Assert-XamlGraph $simpleGeneratedRoot 'Simple' $classes

$panelScans = @(Get-ChildItem $simpleGeneratedRoot -Filter 'PanelPage.xaml.g.ixx.module.json' -Recurse -File)
if ($panelScans.Count -ne 2 -or $panelScans[0].DirectoryName -eq $panelScans[1].DirectoryName) {
    throw 'Same-leaf class interfaces did not receive independent scanner outputs.'
}

$mainInterface = Get-OneGeneratedFile $simpleGeneratedRoot 'MainPage.xaml.g.ixx'
$mainInterfaceText = Get-Content $mainInterface.FullName -Raw
if (-not $mainInterfaceText.Contains('export import winrt.Simple;')) { throw 'Unresolved local field dependency was not exported.' }
$appImplementation = Get-Content (Join-Path $fixtureRoot 'App.cpp') -Raw
if ($appImplementation -notmatch '(?s)#ifdef WINRT_IMPORT_MODULE\s*import Simple\.Application_Xaml\.App;\s*import Simple\.Application_Xaml\.MainPage;\s*#endif') {
    throw 'Handwritten App.cpp must explicitly import both its App and MainPage XamlC modules.'
}

Assert-OrdinaryGeneratedConsumer (Get-OneGeneratedFile $simpleGeneratedRoot 'App.xaml.g.cpp').FullName @(
    'winrt_base',
    'Simple.Application_Xaml.App',
    'Simple.Application_Xaml.TypeInfo'
) @('type_traits')
Assert-OrdinaryGeneratedConsumer (Get-OneGeneratedFile $simpleGeneratedRoot 'MainPage.xaml.g.cpp').FullName @(
    'winrt_base',
    'Simple.Application_Xaml.MainPage',
    'Simple.Application_Xaml.BindingInfo',
    'winrt.Simple.Models',
    'winrt.Simple.Targets'
) @('cstdint', 'memory', 'type_traits', 'utility')
Assert-OrdinaryGeneratedConsumer (Get-OneGeneratedFile $simpleGeneratedRoot 'EmptyPage.xaml.g.cpp').FullName @(
    'winrt_base',
    'Simple.Application_Xaml.EmptyPage'
) @('cstdint', 'memory', 'type_traits', 'utility')
Assert-OrdinaryGeneratedConsumer (Get-OneGeneratedFile $simpleGeneratedRoot 'XamlBindingInfo.xaml.g.cpp').FullName @(
    'winrt_base',
    'Simple.Application_Xaml.BindingInfo'
) @('cstdint', 'memory', 'utility')
# C++/WinRT module-first builds emit TypeInfo through the dedicated
# XamlTypeInfo.Impl.g.cpp and XamlTypeInfo.g.cpp consumers asserted below.
Assert-OrdinaryGeneratedConsumer (Get-OneGeneratedFile $simpleGeneratedRoot 'XamlTypeInfo.Impl.g.cpp').FullName @(
    'winrt_base',
    'Simple.Application_Xaml.TypeInfo'
) @('cstdint', 'memory', 'mutex', 'regex', 'stdexcept', 'string')
Assert-OrdinaryGeneratedConsumer (Get-OneGeneratedFile $simpleGeneratedRoot 'XamlTypeInfo.g.cpp').FullName @(
    'winrt_base',
    'Simple.Application_Xaml',
    'Simple.Application_Xaml.BindingInfo',
    'Simple.Application_Xaml.TypeInfo'
) @('algorithm', 'cstddef', 'cstdint', 'functional', 'map', 'memory', 'mutex', 'regex', 'string', 'type_traits', 'utility', 'vector')

$emptyIdentity = 'Simple.Application_Xaml.EmptyPage'
$emptySource = Get-OneGeneratedFile $simpleGeneratedRoot 'EmptyPage.xaml.g.ixx'
$emptyIfc = Get-OneGeneratedFile $simpleGeneratedRoot "$emptyIdentity.ifc"
$emptySourceTime = $emptySource.LastWriteTimeUtc
$emptyIfcTime = $emptyIfc.LastWriteTimeUtc
Invoke-XamlModuleBuild $simpleModules 'SimpleModules.NoChange'
if ((Get-Item $emptyIfc.FullName).LastWriteTimeUtc -ne $emptyIfcTime) { throw 'No-change build rebuilt a class IFC.' }

$mainXaml = Join-Path $fixtureRoot 'MainPage.xaml'
$mainOriginal = [IO.File]::ReadAllBytes($mainXaml)
$mainIdentity = 'Simple.Application_Xaml.MainPage'
$mainIfc = Get-OneGeneratedFile $simpleGeneratedRoot "$mainIdentity.ifc"
$mainIfcTime = $mainIfc.LastWriteTimeUtc
$rootSource = Get-OneGeneratedFile $simpleGeneratedRoot 'Application_Xaml.g.ixx'
$rootSourceTime = $rootSource.LastWriteTimeUtc
$rootSourceContent = [IO.File]::ReadAllText($rootSource.FullName)
try {
    $changedXaml = [Text.Encoding]::UTF8.GetString($mainOriginal).Replace('<targets:BindTarget', '<TextBlock x:Name="IncrementalField" Text="Changed declaration" /><targets:BindTarget')
    [IO.File]::WriteAllText($mainXaml, $changedXaml)
    Invoke-XamlModuleBuild $simpleModules 'SimpleModules.OnePageChanged'
    if (-not (Get-Content (Get-OneGeneratedFile $simpleGeneratedRoot 'MainPage.xaml.g.ixx').FullName -Raw).Contains('_IncrementalField')) {
        throw 'Changing XAML did not regenerate its module declaration.'
    }
    if ((Get-Item $mainIfc.FullName).LastWriteTimeUtc -eq $mainIfcTime) { throw 'Changing the declaration did not rebuild its class IFC.' }
    if ((Get-Item $rootSource.FullName).LastWriteTimeUtc -ne $rootSourceTime -or [IO.File]::ReadAllText($rootSource.FullName) -ne $rootSourceContent) {
        throw 'Editing one class rewrote the unchanged root source.'
    }
    if ((Get-Item $emptySource.FullName).LastWriteTimeUtc -ne $emptySourceTime -or (Get-Item $emptyIfc.FullName).LastWriteTimeUtc -ne $emptyIfcTime) {
        throw 'Changing MainPage rebuilt the sibling EmptyPage interface.'
    }
} finally {
    [IO.File]::WriteAllBytes($mainXaml, $mainOriginal)
}

Invoke-XamlModuleBuild $simpleModules 'SimpleModules.PageRemoved' @('/p:IncludeIncrementalEmptyPage=false')
Assert-RemovedClass $simpleGeneratedRoot 'EmptyPage' $emptyIdentity
Assert-XamlGraph $simpleGeneratedRoot 'Simple' @($classes | Where-Object { $_ -ne 'Simple.EmptyPage' })
Invoke-XamlModuleBuild $simpleModules 'SimpleModules.PageRestored' @('/p:IncludeIncrementalEmptyPage=true')
Assert-XamlGraph $simpleGeneratedRoot 'Simple' $classes

$original = @{}
$renamed = @{}
foreach ($file in Get-ChildItem $fixtureRoot -File | Where-Object { $_.Extension -in @('.h', '.cpp', '.ixx', '.xaml', '.idl', '.vcxproj') }) {
    $original[$file.FullName] = [IO.File]::ReadAllBytes($file.FullName)
    $target = $file.FullName -replace 'EmptyPage\.(xaml|idl|h|cpp|ixx)$', 'RenamedPage.$1'
    $renamed[$file.FullName] = $target
}
try {
    foreach ($path in $original.Keys) {
        $content = [Text.Encoding]::UTF8.GetString($original[$path]).Replace('EmptyPage', 'RenamedPage')
        [IO.File]::WriteAllText($renamed[$path], $content)
        if ($renamed[$path] -ne $path) { Remove-Item $path }
    }
    Invoke-XamlModuleBuild $simpleModules 'SimpleModules.Rename'
    Assert-RemovedClass $simpleGeneratedRoot 'EmptyPage' $emptyIdentity
    Assert-XamlGraph $simpleGeneratedRoot 'Simple' @($classes | ForEach-Object { $_.Replace('EmptyPage', 'RenamedPage') })
} finally {
    foreach ($path in $original.Keys) {
        if ($renamed[$path] -ne $path -and (Test-Path $renamed[$path])) { Remove-Item $renamed[$path] }
        [IO.File]::WriteAllBytes($path, $original[$path])
    }
}
Invoke-XamlModuleBuild $simpleModules 'SimpleModules.RenameRestored'
Assert-RemovedClass $simpleGeneratedRoot 'RenamedPage' 'Simple.Application_Xaml.RenamedPage'
Assert-XamlGraph $simpleGeneratedRoot 'Simple' $classes

Invoke-XamlModuleBuild -RelativeProject $simpleModules -LogName 'SimpleModules.NoPageInterfaces' -Target 'ClCompile' -ExtraProperties @('/p:XamlCodeGenerationControlFlags=NoPageCodeGen', '/p:ValidateXamlInterfacesOnly=true')
Assert-XamlGraph $simpleGeneratedRoot 'Simple' @('Simple.App')
foreach ($page in @('MainPage', 'EmptyPage', 'PanelPage')) {
    if (@(Get-ChildItem $simpleGeneratedRoot -Filter "$page.xaml.g.h" -File -Recurse).Count -ne 0) { throw 'NoPageCodeGen retained a non-App sentinel.' }
}
Invoke-XamlModuleBuild $simpleModules 'SimpleModules.PageCodeGenRestored'
Assert-XamlGraph $simpleGeneratedRoot 'Simple' $classes

Invoke-XamlModuleBuild $simpleModules 'SimpleModules.HeaderSwitch' @('/p:CppWinRTBuildModule=false')
if (@(Get-ChildItem $simpleGeneratedRoot -File -Recurse | Where-Object { $_.Name -like '*.g.ixx' -or ($_.Extension -eq '.ifc' -and $_.DirectoryName -like '*\XamlModules*') }).Count -ne 0) {
    throw 'Header mode retained XAML module interfaces or IFCs.'
}
Invoke-XamlModuleBuild $simpleModules 'SimpleModules.ModuleSwitchBack' @('/p:CppWinRTBuildModule=true')
Assert-XamlGraph $simpleGeneratedRoot 'Simple' $classes

$providerRoot = Join-Path $repoRoot 'BuildOutput\obj\amd64chk\CompilerTests\Features\StaticLibs\StaticControlsModuleLib'
Invoke-XamlModuleBuild $staticProvider 'StaticProvider.Module'
Assert-XamlGraph $providerRoot 'StaticControlsModuleLib' @('StaticControlsModuleLib.BlankUserControl')
Invoke-XamlModuleBuild -RelativeProject $staticProvider -LogName 'StaticProvider.NoTypeInfoInterfaces' -Target 'ClCompile' -ExtraProperties @('/p:XamlCodeGenerationControlFlags=NoTypeInfoCodeGen', '/p:ValidateXamlInterfacesOnly=true')
Assert-XamlGraph $providerRoot 'StaticControlsModuleLib' @('StaticControlsModuleLib.BlankUserControl') $false
Invoke-XamlModuleBuild $staticProvider 'StaticProvider.TypeInfoRestored'
Assert-XamlGraph $providerRoot 'StaticControlsModuleLib' @('StaticControlsModuleLib.BlankUserControl')

$consumerRoot = Join-Path $repoRoot 'BuildOutput\obj\amd64chk\CompilerTests\Features\StaticLibs\StaticControlsModuleConsumer'
$sentinel = Join-Path $consumerRoot 'XamlModules'
New-Item -ItemType Directory -Force $sentinel | Out-Null
Set-Content (Join-Path $sentinel 'stale.ifc') 'stale'
Invoke-XamlModuleBuild -RelativeProject $staticConsumer -LogName 'StaticConsumer.NoXamlCleanup' -Target 'XamlCppWinRTRemoveInactiveModuleOutputs' -ExtraProperties @('/p:CppWinRTBuildModule=true')
if (Test-Path $sentinel) { throw 'No-XAML project left stale module outputs.' }
Invoke-XamlModuleBuild $staticConsumer 'StaticConsumer.CrossProject'
if (@(Get-ChildItem $consumerRoot -Filter '*.ixx' -File -Recurse).Count -ne 0) { throw 'Pure consumer generated duplicate interfaces.' }
Invoke-XamlModuleBuild $staticProvider 'StaticProvider.Header' @('/p:CppWinRTBuildModule=false')
Invoke-XamlModuleBuild $staticConsumer 'StaticConsumer.Header' @('/p:CppWinRTBuildModule=false', '/p:ValidateHeaderMode=true')
Invoke-XamlModuleBuild $staticConsumer 'StaticConsumer.ModuleRestored'

Invoke-XamlModuleBuild -RelativeProject $simpleModules -LogName 'SimpleModules.Release' -Configuration Release -Target 'Rebuild'
Invoke-XamlModuleBuild -RelativeProject $simpleModules -LogName 'SimpleModules.ReleaseHeader' -Configuration Release -ExtraProperties @('/p:CppWinRTBuildModule=false') -Target 'Rebuild'
Invoke-XamlModuleBuild -RelativeProject $staticProvider -LogName 'StaticProvider.ReleaseHeader' -Configuration Release -ExtraProperties @('/p:CppWinRTBuildModule=false') -Target 'Rebuild'
Invoke-XamlModuleBuild -RelativeProject $staticConsumer -LogName 'StaticConsumer.ReleaseHeader' -Configuration Release -ExtraProperties @('/p:CppWinRTBuildModule=false', '/p:ValidateHeaderMode=true') -Target 'Rebuild'
Invoke-XamlModuleBuild -RelativeProject $staticProvider -LogName 'StaticProvider.ReleaseModule' -Configuration Release -Target 'Rebuild'
Invoke-XamlModuleBuild -RelativeProject $staticConsumer -LogName 'StaticConsumer.ReleaseModule' -Configuration Release -Target 'Rebuild'

$graphEvidence = Join-Path $binlogDir 'XamlModuleGraph'
foreach ($file in Get-ChildItem (Join-Path $repoRoot 'BuildOutput\obj') -Recurse -File | Where-Object {
    ($_.Extension -eq '.ifc' -and $_.DirectoryName -like '*\XamlModules*') -or
    $_.Name -like '*.g.ixx' -or
    $_.Name -like '*.module.json' -or
    $_.Name -like '*.module.json.command'
}) {
    $relative = $file.FullName.Substring($repoRoot.Length).TrimStart('\')
    $destination = Join-Path $graphEvidence $relative
    New-Item -ItemType Directory -Force (Split-Path $destination) | Out-Null
    Copy-Item $file.FullName $destination
}

& (Join-Path $PSScriptRoot 'validate-xamlc-unit-tests.ps1') -VisualStudioVersion $ModuleVisualStudioVersion -WindowsSdkVersion $ModuleWindowsSdkVersion -TestCaseFilter 'FullyQualifiedName~UnitTests.CppWinRTModuleTests' -SkipRegressionBuild

