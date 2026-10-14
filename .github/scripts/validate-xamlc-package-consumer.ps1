param([switch]$Pass1Only)
$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$testRoot = Join-Path $repoRoot 'PackageConsumerValidation'
Copy-Item (Join-Path $repoRoot 'src\XamlCompiler\Tests\PackageConsumers\NuGetModules') $testRoot -Recurse -Force
$project = Join-Path $testRoot 'NuGetModules.vcxproj'
$package = @(Get-ChildItem (Join-Path $repoRoot 'PackageStore') -Filter 'Microsoft.WindowsAppSDK.WinUI.*.nupkg' -File)
if ($package.Count -ne 1) { throw 'Expected exactly one current development package.' }
$version = $package[0].BaseName.Substring('Microsoft.WindowsAppSDK.WinUI.'.Length)

[xml]$xml = Get-Content $project
$ns = New-Object Xml.XmlNamespaceManager($xml.NameTable)
$ns.AddNamespace('m', 'http://schemas.microsoft.com/developer/msbuild/2003')
$xml.SelectSingleNode('//m:PackageReference[@Include="Microsoft.WindowsAppSDK.WinUI"]', $ns).SetAttribute('Version', $version)
$xml.Save($project)

$feed = [Security.SecurityElement]::Escape((Join-Path $repoRoot 'PackageStore'))
$config = Join-Path $testRoot 'nuget.validation.config'
@"
<configuration><packageSources><clear/><add key="Product" value="$feed"/><add key="nuget.org" value="https://api.nuget.org/v3/index.json"/></packageSources><packageSourceMapping><clear/><packageSource key="Product"><package pattern="Microsoft.WindowsAppSDK.WinUI"/></packageSource><packageSource key="nuget.org"><package pattern="*"/></packageSource></packageSourceMapping></configuration>
"@ | Set-Content $config

$common = @('/restore', '/m:2', '/ds:false', '/p:Platform=x64', '/p:VisualStudioVersion=18.0', '/p:PlatformToolset=v145', '/p:WindowsTargetPlatformVersion=10.0.26100.0', '/p:WindowsPackageType=None', "/p:RestoreConfigFile=$config", "/p:RestorePackagesPath=$testRoot\packages", '/v:minimal')
$binlog = Join-Path $repoRoot 'BuildOutput\binlogs'

function Assert-AuthoredModuleConsumer([string]$RelativePath, [string[]]$RequiredImports, [string]$LastRequiredTextualHeader = $null) {
    $path = Join-Path $testRoot $RelativePath
    $text = Get-Content $path -Raw
    if ($text -match '(?m)^module;$' -or $text -match '(?m)^module\s+[^;]+;') {
        throw "Authored consumer became a module unit: $RelativePath"
    }
    if ($text.Contains('import std;')) { throw "Authored consumer imports std: $RelativePath" }
    if (-not $text.Contains('#define WINRT_IMPORT_MODULE')) { throw "Authored consumer does not enable C++/WinRT module consumption: $RelativePath" }
    $firstImport = $text.IndexOf('import ', [StringComparison]::Ordinal)
    if ($firstImport -lt 0) { throw "Authored consumer has no module imports: $RelativePath" }
    if ($LastRequiredTextualHeader) {
        $lastHeader = $text.IndexOf($LastRequiredTextualHeader, [StringComparison]::Ordinal)
        if ($lastHeader -lt 0 -or $lastHeader -gt $firstImport) {
            throw "Authored consumer does not establish its STL dependencies before imports: $RelativePath"
        }
    }
    foreach ($required in $RequiredImports) {
        if (-not $text.Contains("import $required;")) { throw "$RelativePath is missing import $required." }
    }
}

foreach ($mode in @('Module', 'Header')) {
    foreach ($configuration in @('Debug', 'Release')) {
        $module = if ($mode -eq 'Module') { 'true' } else { 'false' }
        $intermediate = Join-Path $testRoot "obj\$mode\$configuration\"
        $target = if ($Pass1Only) { 'MarkupCompilePass1' } else { 'Build' }
        & msbuild.exe $project @common "/t:$target" "/p:Configuration=$configuration" "/p:CppWinRTBuildModule=$module" "/p:IntDir=$intermediate" "/p:OutDir=$testRoot\bin\$mode\$configuration\" "/binaryLogger:$binlog\NuGetConsumer.$mode.$configuration.binlog"
        if ($LASTEXITCODE -ne 0) { throw "NuGet consumer $mode $configuration failed: $LASTEXITCODE" }

        if ($mode -eq 'Module') {
            Assert-AuthoredModuleConsumer 'App.xaml.cpp' @('winrt_base', 'NuGetModules.Application_Xaml.App', 'NuGetModules.Application_Xaml.MainPage')
            Assert-AuthoredModuleConsumer 'MainPage.xaml.cpp' @('winrt_base', 'NuGetModules.Application_Xaml.MainPage') '#include <cstdint>'
            Assert-AuthoredModuleConsumer 'EmptyPage.xaml.cpp' @('winrt_base', 'NuGetModules.Application_Xaml.EmptyPage') '#include <cstdint>'
            Assert-AuthoredModuleConsumer 'RootConsumer.cpp' @('winrt_base', 'NuGetModules.Application_Xaml')

            $root = @(Get-ChildItem $testRoot -Filter 'Application_Xaml.g.ixx' -Recurse -File)
            if ($root.Count -ne 1) { throw 'Automatic public root module missing.' }
            $rootText = Get-Content $root[0].FullName -Raw
            if (-not $rootText.Contains('export module NuGetModules.Application_Xaml;')) { throw 'Automatic public root module has the wrong identity.' }
            if ($rootText -match '#include|namespace |Application_Xaml\.Support') { throw 'Automatic public root is not a pure per-class aggregator.' }

            foreach ($obsolete in @('Application_Xaml.Support.g.ixx', 'XamlSupport.g.ixx', 'NuGetModules.Application_Xaml.Support.ifc')) {
                if (@(Get-ChildItem $testRoot -Filter $obsolete -Recurse -File -ErrorAction SilentlyContinue).Count -ne 0) {
                    throw "NuGet consumer retained obsolete support artifact $obsolete."
                }
            }

            foreach ($class in @('App', 'MainPage', 'EmptyPage')) {
                $interface = @(Get-ChildItem $testRoot -Filter "$class.xaml.g.ixx" -Recurse -File)
                if ($interface.Count -ne 1) { throw "No automatic interface for $class." }
                $identity = "NuGetModules.Application_Xaml.$class"
                $interfaceText = Get-Content $interface[0].FullName -Raw
                if (-not $interfaceText.Contains("export module $identity;")) { throw "$class interface has the wrong identity." }
                if ($class -eq 'App' -and -not $interfaceText.Contains('export import NuGetModules.Application_Xaml.TypeInfo;')) {
                    throw 'App module does not re-export the TypeInfo dependency used by its public template.'
                }
                if (-not $rootText.Contains("export import $identity;")) { throw "Root does not re-export $identity." }
                if (-not $Pass1Only -and @(Get-ChildItem $intermediate -Filter "$identity.ifc" -Recurse -File).Count -ne 1) {
                    throw "$identity IFC was not registered."
                }
            }

            if (-not $Pass1Only -and @(Get-ChildItem $intermediate -Filter 'NuGetModules.Application_Xaml.ifc' -Recurse -File).Count -ne 1) {
                throw 'Root IFC was not automatically registered.'
            }

            if (-not $Pass1Only) {
                foreach ($generated in @(Get-ChildItem $testRoot -Filter '*.xaml.g.cpp' -Recurse -File) + @(Get-ChildItem $testRoot -Filter 'XamlTypeInfo*.g.cpp' -Recurse -File)) {
                    $text = Get-Content $generated.FullName -Raw
                    if ($text -match '(?m)^\s*(?:export\s+)?module(?:\s+[^;]+)?\s*;' -or
                        $text.Contains('import std;')) {
                        throw "NuGet consumer generated ordinary TU has module/STL ownership regression: $($generated.FullName)"
                    }
                    if ($generated.Name -eq 'XamlTypeInfo.g.cpp' -and -not $text.Contains('import winrt.Microsoft.UI.Xaml.XamlTypeInfo;')) {
                        throw 'NuGet consumer TypeInfo TU is missing the projection import for its external metadata provider.'
                    }
                }
            }
        }

        if ($Pass1Only) { break }
    }
}
