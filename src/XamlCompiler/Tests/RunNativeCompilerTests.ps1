# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License. See LICENSE in the project root for license information.

<#
.SYNOPSIS
    Runs the XAML compiler regression tests against the Native AOT build of the compiler (net10.0 XamlCompiler.exe).

.DESCRIPTION
    The regression projects in XamlCompilerTests.sln are built with UseNativeXamlCompiler=true, so that the native
    executable compiles all their XAML instead of the in-process compiler task. Then:
      * the codegen tests (CodegenTests) compare the generated code against the checked-in TestMasters, and
      * with -Differential, everything the compiler generated (code, XBF, roots logs) is compared byte by byte with what the
        default compiler generates for the very same projects, which also covers the files that don't have masters (XBF).

    It needs an initialized prompt (init.cmd <flavor>), and XamlCompiler.sln and XamlCompilerTests.sln to have been built
    (see src\XamlCompiler\Tests\UnitTests\README.md). It overwrites the generated files of the regression projects (under
    BuildOutput\obj\<flavor>\CompilerTests), so build XamlCompilerTests.sln again afterwards to go back to the default compiler.

    With -Samples, it also validates the native compiler on real apps from the Samples folder, always differentially: each
    sample is built from scratch with the default compiler and then with the native one, and everything the XAML compiler
    generated (code, XBF, the rewritten XAML, type info) as well as the XAML compiler warnings and errors must be identical.
    The samples consume the local WinUI component package (PackageStore\Microsoft.WindowsAppSDK.WinUI.<version>.nupkg, see
    docs\ad-hoc-testing-of-local-build-with-fast-inner-loop.md), restored like for any other sample build, but both builds
    use the XAML compiler targets and tools that XamlCompiler.sln just built (XamlCompilerPropsAndTargetsDirectory, the same
    mechanism eng\usexamlcompiler.props uses for the regression projects), so that they test the current compiler even when
    the package is older. The default compiler (of msbuild.exe) is the in-process .NET Framework task. The sample builds go to
    BuildOutput\obj\<flavor>\Samples, which is deleted before each of the two builds.

.PARAMETER ExePath
    The native compiler to test. Defaults to the one XamlCompiler.sln packaged for the architecture of the current flavor.

.PARAMETER Differential
    Also build everything with the default compiler first, and require the output of the native compiler to be identical.

.PARAMETER TestFilter
    vstest test case filter for the tests that run after the build. Use '' to skip running tests.

.PARAMETER Samples
    Also build the sample apps in -SampleProjects with both compilers and require identical output (see above).

.PARAMETER SampleProjects
    The sample projects or solutions to build with -Samples, relative to the Samples folder. The default list has C# and
    C++/WinRT apps that build against the local WinUI component package; samples that need features newer than the package
    in PackageStore (e.g. ChartApp, which needs the Charts APIs) or that don't build reliably with /m
    (WinUICppDesktopSampleApp.sln: C1041 on its shared PDB) aren't in it.

.PARAMETER SkipRegressionProjects
    Don't build the regression projects (nor run the tests), e.g. to only validate the samples with -Samples.

.EXAMPLE
    RunNativeCompilerTests.ps1 -Differential -Samples

.EXAMPLE
    RunNativeCompilerTests.ps1 -SkipRegressionProjects -Samples -SampleProjects FolderTreeApp\FolderTreeApp\FolderTreeApp.csproj
#>
[CmdletBinding()]
param(
    [string]$ExePath,
    [switch]$Differential,
    [string]$TestFilter = 'FullyQualifiedName~UnitTests.CodegenTests',
    [switch]$Samples,
    [string[]]$SampleProjects = @(
        'TableViewSampleApp\TableViewSampleApp.csproj',
        'WinUICsDesktopSampleApp\WinUICsDesktopSampleApp.sln',
        'FolderTreeApp\FolderTreeApp\FolderTreeApp.csproj',
        'FolderTreeAppCpp\FolderTreeAppCpp.sln',
        'TableViewApp\TableViewAppCppUnpackaged\TableViewAppCppUnpackaged.vcxproj'),
    [switch]$SkipRegressionProjects
)

$ErrorActionPreference = 'Stop'

foreach ($name in 'EnvironmentInitialized', 'RepoRoot', 'BuildOutputRoot', '_BuildArch', '_BuildType') {
    if (-not (Get-Item "Env:$name" -ErrorAction SilentlyContinue)) {
        throw "The WinUI build environment is not initialized. Run init.cmd <flavor> in this prompt, then retry."
    }
}

$flavor = "$($env:_BuildArch)$($env:_BuildType)"
$configuration = if ($env:Configuration) { $env:Configuration } else { 'Debug' }
$rid = switch ($env:_BuildArch) { 'amd64' { 'win-x64' } 'arm64' { 'win-arm64' } default { throw "There is no native XAML compiler for '$($env:_BuildArch)'." } }

if (-not $ExePath) {
    $ExePath = Join-Path (Split-Path $env:BuildOutputRoot -Parent) "packaging\$configuration\tools\net10.0\$rid\XamlCompiler.exe"
}
if (-not (Test-Path $ExePath)) {
    throw "The native XAML compiler was not found at '$ExePath'. Build src\XamlCompiler\XamlCompiler.sln first, or pass -ExePath."
}
$ExePath = (Resolve-Path $ExePath).Path

$solution = Join-Path $env:RepoRoot 'src\XamlCompiler\XamlCompilerTests.sln'
$compilerTests = Join-Path $env:BuildOutputRoot "$flavor\CompilerTests"
$workRoot = Join-Path $env:BuildOutputRoot "$flavor\NativeCompilerTests"

# What the XAML compiler generates for the regression projects, which is what is compared in differential mode. The generated
# directory of a C++ project has more: the C++/WinRT tools write the <Type>.g.h/.g.cpp/winrt\ files next to the XAML compiler's own
# (<Page>.xaml.g.h, XamlTypeInfo*, XamlMetaDataProvider.*, ...), and the latter are the only ones to be regenerated here.
function Get-GeneratedFiles {
    Get-ChildItem $compilerTests -Recurse -Directory -Filter generated |
        ForEach-Object {
            $generatedDir = $_.FullName
            Get-ChildItem $generatedDir -Recurse -File |
                Where-Object {
                    $_.FullName -notmatch '\\winrt\\' -and (
                        $_.Name -match '\.xaml\.g\.|\.g\.i\.cs$|\.xbf$|\.xr\.xml$|^XamlTypeInfo|^XamlMetaDataProvider\.(idl|cpp)$|^XamlLibMetadataProvider|^XamlBindingInfo' -or
                        ($_.Extension -in '.cs', '.vb' -and $_.Name -match '\.g\.'))
                } |
                ForEach-Object { [pscustomobject]@{ Key = $_.FullName.Substring($compilerTests.Length + 1); Path = $_.FullName } }
        }
}

# Forces the next build to write all of the files of the XAML compiler again
function Reset-GeneratedFiles {
    Get-GeneratedFiles | ForEach-Object { Remove-Item $_.Path -Force }
    Get-ChildItem $compilerTests -Recurse -File -Filter 'XamlSaveStateFile*.xml' | Remove-Item -Force
}

function Invoke-TestsBuild([string[]]$extraArguments) {
    & msbuild $solution /nologo /clp:ErrorsOnly /m /nodeReuse:false /p:RunAnalyzers=false @extraArguments
    if ($LASTEXITCODE -ne 0) { throw "Building XamlCompilerTests.sln failed ($LASTEXITCODE)." }
}

# Copies the given generated files (Key = path relative to the compared root) to a new snapshot directory
function Save-Snapshot($files, [string]$directory) {
    if (Test-Path $directory) { Remove-Item $directory -Recurse -Force }
    foreach ($file in $files) {
        $destination = Join-Path $directory $file.Key
        New-Item -ItemType Directory -Force (Split-Path $destination) | Out-Null
        Copy-Item -LiteralPath $file.Path $destination
    }
}

# Compares the files of a snapshot (the default compiler) with the given generated files (the native compiler)
function Compare-Snapshot([string]$directory, $files, [string]$what) {
    $native = @{}
    foreach ($file in $files) { $native[$file.Key] = $file.Path }

    $expected = @{}
    if (Test-Path $directory) {
        Get-ChildItem $directory -Recurse -File | ForEach-Object { $expected[$_.FullName.Substring($directory.Length + 1)] = $_.FullName }
    }

    $differences = @()
    foreach ($key in $expected.Keys) {
        if (-not $native.ContainsKey($key)) { $differences += "missing   $key" }
        elseif ((Get-FileHash -LiteralPath $native[$key]).Hash -ne (Get-FileHash -LiteralPath $expected[$key]).Hash) { $differences += "different $key" }
    }
    foreach ($key in $native.Keys) {
        if (-not $expected.ContainsKey($key)) { $differences += "extra     $key" }
    }

    Write-Host "[native-tests] Compared $($expected.Count) files ($what): $($differences.Count) difference(s)."
    $differences | Sort-Object | ForEach-Object { Write-Host "  $_" }
    return $differences.Count -eq 0
}

$failed = $false

if (-not $SkipRegressionProjects) {
    $baseline = $null
    if ($Differential) {
        Write-Host "[native-tests] Building the regression projects with the default compiler..."
        Reset-GeneratedFiles
        Invoke-TestsBuild @()
        $baseline = Join-Path $workRoot 'baseline'
        Save-Snapshot (Get-GeneratedFiles) $baseline
    }

    Write-Host "[native-tests] Building the regression projects with $ExePath ..."
    Reset-GeneratedFiles
    $nativeBuildStart = Get-Date
Invoke-TestsBuild @('/p:UseNativeXamlCompiler=true', "/p:XamlCompilerNativeExePath=$ExePath")

# Make sure that the native compiler is what built the projects (it leaves its output.json next to the generated files)
$nativeRuns = @(Get-ChildItem $compilerTests -Recurse -File -Filter output.json | Where-Object { $_.LastWriteTime -ge $nativeBuildStart })
if ($nativeRuns.Count -eq 0) { throw 'The native XAML compiler did not run for any project.' }
Write-Host "[native-tests] The native compiler ran for $($nativeRuns.Count) compilation passes."

    if ($Differential) {
        Write-Host "[native-tests] Comparing the generated files with the ones of the default compiler..."
        if (-not (Compare-Snapshot $baseline (Get-GeneratedFiles) 'regression projects')) { $failed = $true }
    }

    if ($TestFilter) {
        Write-Host "[native-tests] Running tests: $TestFilter"
        & (Join-Path $env:RepoRoot 'src\XamlCompiler\runtests.cmd') "/TestCaseFilter:$TestFilter"
        if ($LASTEXITCODE -ne 0) { $failed = $true }
    }
}

if ($Samples) {
    $samplesRoot = Join-Path $env:RepoRoot 'Samples'
    $samplesObj = Join-Path $env:BuildOutputRoot "$flavor\Samples"
    $samplesWork = Join-Path $workRoot 'samples'
    # The XAML compiler targets and tools XamlCompiler.sln built, instead of the ones in the WinUI component package
    $compilerTargets = Join-Path (Split-Path $env:BuildOutputRoot -Parent) "packaging\$configuration\build\"
    $platform = switch ($env:_BuildArch) { 'amd64' { 'x64' } 'arm64' { 'ARM64' } }
    if (-not (Test-Path (Join-Path $compilerTargets 'Microsoft.UI.Xaml.Markup.Compiler.interop.targets'))) {
        throw "The XAML compiler targets were not found in '$compilerTargets'. Build src\XamlCompiler\XamlCompiler.sln first."
    }
    foreach ($sample in $SampleProjects) {
        if (-not (Test-Path (Join-Path $samplesRoot $sample))) { throw "The sample '$sample' was not found under '$samplesRoot'." }
    }

    # What the XAML compiler generates for a sample: the same files as for the regression projects (wherever the project puts
    # them: obj\ for C#, its 'generated' directory for C++), plus the XAML it rewrites (with the connection IDs) and the copies of
    # the XBF files. A <Name>.g.cs file is only the XAML compiler's if it also wrote <Name>.g.i.cs (e.g. TableViewSampleApp
    # generates a BuildInfo.g.cs with the build time). Only the source files of the type info: the C++ object files and
    # metadata built from them (XamlTypeInfo.g.obj, XamlMetaDataProvider.winmd, ...) aren't deterministic. input.json/output.json
    # are only written when the compiler is an executable, and XamlSaveStateFile.xml is the incremental build state.
    function Get-SampleGeneratedFiles {
        if (-not (Test-Path $samplesObj)) { return }
        Get-ChildItem $samplesObj -Recurse -File |
            Where-Object {
                $_.FullName -notmatch '\\winrt\\' -and (
                    $_.Name -match '\.xaml$|\.g\.i\.cs$|\.xbf$|\.xr\.xml$' -or
                    ($_.Name -match '\.xaml\.g\.|^(XamlTypeInfo|XamlMetaDataProvider\.|XamlLibMetadataProvider|XamlBindingInfo)' -and $_.Extension -in '.h', '.hpp', '.cpp', '.cs', '.vb') -or
                    ($_.Name -match '^(.+)\.g\.(cs|vb)$' -and (Test-Path -LiteralPath (Join-Path $_.DirectoryName "$($Matches[1]).g.i.$($Matches[2])"))))
            } |
            ForEach-Object { [pscustomobject]@{ Key = $_.FullName.Substring($samplesObj.Length + 1); Path = $_.FullName } }
    }

    # Builds all the samples from scratch, and returns the XAML compiler diagnostics (deduplicated)
    function Invoke-SamplesBuild([string]$pass, [string[]]$extraArguments) {
        if (Test-Path $samplesObj) { Remove-Item $samplesObj -Recurse -Force }
        $diagnostics = @()
        foreach ($sample in $SampleProjects) {
            $log = Join-Path $samplesWork "$pass-$(($sample -replace '[\\/]', '_')).log"
            Write-Host "[native-tests] Building $sample ($pass)..."
            & msbuild (Join-Path $samplesRoot $sample) /restore /nologo /clp:ErrorsOnly /m /nodeReuse:false "/p:Platform=$platform" "/p:Configuration=$configuration" `
                "/p:XamlCompilerPropsAndTargetsDirectory=$compilerTargets" "/flp:LogFile=$log;Verbosity=normal" @extraArguments | Out-Host
            if ($LASTEXITCODE -ne 0) { throw "Building the sample '$sample' failed ($LASTEXITCODE), see $log." }
            $usedNativeCompiler = Select-String -LiteralPath $log -SimpleMatch $ExePath -Quiet
            if ($usedNativeCompiler -ne ($pass -eq 'native')) {
                throw "The build of the sample '$sample' ($pass) $(if ($usedNativeCompiler) { 'used' } else { "didn't use" }) the native compiler, see $log."
            }
            $diagnostics += Get-Content -LiteralPath $log | Where-Object { $_ -match 'XamlCompiler (warning|error) WMC' } | ForEach-Object { $_.Trim() }
        }
        return @($diagnostics | Sort-Object -Unique)
    }

    if (Test-Path $samplesWork) { Remove-Item $samplesWork -Recurse -Force }
    New-Item -ItemType Directory -Force $samplesWork | Out-Null

    $defaultDiagnostics = @(Invoke-SamplesBuild 'default' @())
    $defaultFiles = @(Get-SampleGeneratedFiles)
    if ($defaultFiles.Count -eq 0) { throw "The default compiler didn't generate anything for the samples under '$samplesObj'." }
    Save-Snapshot $defaultFiles (Join-Path $samplesWork 'baseline')
    $nativeDiagnostics = @(Invoke-SamplesBuild 'native' @('/p:UseNativeXamlCompiler=true', "/p:XamlCompilerNativeExePath=$ExePath"))

    Write-Host "[native-tests] Comparing the files generated for the samples with the ones of the default compiler..."
    if (-not (Compare-Snapshot (Join-Path $samplesWork 'baseline') (Get-SampleGeneratedFiles) 'samples')) { $failed = $true }

    $diagnosticDifferences = @($defaultDiagnostics | Where-Object { $nativeDiagnostics -notcontains $_ } | ForEach-Object { "missing $_" }) +
        @($nativeDiagnostics | Where-Object { $defaultDiagnostics -notcontains $_ } | ForEach-Object { "extra   $_" })
    Write-Host "[native-tests] Compared $($defaultDiagnostics.Count) XAML compiler warning(s)/error(s) (samples): $($diagnosticDifferences.Count) difference(s)."
    $diagnosticDifferences | ForEach-Object { Write-Host "  $_" }
    if ($diagnosticDifferences.Count -gt 0) { $failed = $true }
}

if ($failed) { Write-Host '[native-tests] FAILED'; exit 1 }
Write-Host '[native-tests] PASSED'
exit 0