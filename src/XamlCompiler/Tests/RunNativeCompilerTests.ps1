# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License. See LICENSE in the project root for license information.

<#
.SYNOPSIS
    Runs the XAML compiler regression tests end to end, with the Native AOT compiler (XamlCompiler.exe) and the CompileXaml task
    that XamlCompiler.sln built.

.DESCRIPTION
    The regression projects in XamlCompilerTests.sln are built (with -ExePath, by that compiler), and the codegen tests
    (CodegenTests) compare the generated code against the checked-in TestMasters. Then, optionally:
      * with -ReferencePackage (and -Differential), everything the compiler generated for the regression projects (code, XBF,
        roots logs) is compared byte by byte with what the XAML compiler of another package layout generates for the very same
        projects, which also covers the files that don't have masters (XBF);
      * with -Samples, real apps from the Samples folder are built from scratch (and, with -ReferencePackage, compared the same
        way, including the XAML compiler warnings and errors);
      * with -DesignTime, design-time (IntelliSense) builds of a C# and a C++/WinRT regression project are checked (see below).

    It needs an initialized prompt (init.cmd <flavor>), and XamlCompiler.sln and XamlCompilerTests.sln to have been built
    (see src\XamlCompiler\Tests\UnitTests\README.md). It overwrites the generated files of the regression projects (under
    BuildOutput\obj\<flavor>\CompilerTests), so build XamlCompilerTests.sln again afterwards if -ReferencePackage was used.

    -ReferencePackage is a package layout with the XAML compiler: BuildOutput\packaging\<configuration> of another build of
    this repository, or an extracted Microsoft.WindowsAppSDK.WinUI package. Both its layouts are supported: build\ and
    tools\Microsoft.UI.Xaml.Markup.Compiler.Tasks.dll + tools\win-<arch>\XamlCompiler.exe, and the older build\ and
    tools\net472\Microsoft.UI.Xaml.Markup.Compiler.dll (the in-process .NET Framework compiler), which is what the native
    compiler was validated against when it replaced it.

    The samples consume the local WinUI component package (PackageStore\Microsoft.WindowsAppSDK.WinUI.<version>.nupkg, see
    docs\ad-hoc-testing-of-local-build-with-fast-inner-loop.md), restored like for any other sample build, but they use the XAML
    compiler targets and tools that XamlCompiler.sln just built (XamlCompilerPropsAndTargetsDirectory, the same mechanism
    eng\usexamlcompiler.props uses for the regression projects), or the ones of the reference package, so that they test the
    current compiler even when the package is older. The sample builds go to BuildOutput\obj\<flavor>\Samples, which is deleted
    before each build.

    -DesignTime checks, for each project of -DesignTimeProjects, with the same command-line design-time builds as Visual Studio
    (DesignTimeBuild=true, BuildingInsideVisualStudio=true):
      * after a real build, a design-time build changes none of the files the XAML compiler generated (it must not truncate
        the code of the last real build nor replace its type info) and leaves no IDE overlays file behind;
      * on a clean project, the files the design-time build generates for IntelliSense (*.g.i.cs, *.xaml.g.h) are the ones the
        next real build generates;
      * a XAML error doesn't fail a design-time build: it is reported as a warning.
    The unsaved editor buffers of Visual Studio (the IDE host object) are covered by the CompileXaml task tests (TaskTests).

.PARAMETER ExePath
    The XAML compiler to test. Defaults to the one eng\usexamlcompiler.props uses (the win-x64 compiler XamlCompiler.sln
    packaged).

.PARAMETER ReferencePackage
    A package layout with another XAML compiler to compare with (see above).

.PARAMETER Differential
    Also build the regression projects with the XAML compiler of -ReferencePackage first, and require the outputs to be identical.

.PARAMETER TestFilter
    vstest test case filter for the tests that run after the build. Use '' to skip running tests.

.PARAMETER Samples
    Also build the sample apps in -SampleProjects (compared with -ReferencePackage, if any).

.PARAMETER SampleProjects
    The sample projects or solutions to build with -Samples, relative to the Samples folder. The default list has C# and
    C++/WinRT apps that build against the local WinUI component package; samples that need features newer than the package
    in PackageStore (e.g. ChartApp, which needs the Charts APIs) or that don't build reliably with /m
    (WinUICppDesktopSampleApp.sln: C1041 on its shared PDB) aren't in it.

.PARAMETER DesignTime
    Also check the design-time builds of -DesignTimeProjects (see above).

.PARAMETER DesignTimeProjects
    The regression projects -DesignTime checks, relative to Tests\RegressionProjects.

.PARAMETER SkipRegressionProjects
    Don't build the regression projects (nor run the tests), e.g. to only validate the samples with -Samples.

.EXAMPLE
    RunNativeCompilerTests.ps1 -DesignTime

.EXAMPLE
    RunNativeCompilerTests.ps1 -Differential -Samples -ReferencePackage \\share\WinUI\1.8\packaging\Release

.EXAMPLE
    RunNativeCompilerTests.ps1 -SkipRegressionProjects -Samples -SampleProjects FolderTreeApp\FolderTreeApp\FolderTreeApp.csproj
#>
[CmdletBinding()]
param(
    [string]$ExePath,
    [string]$ReferencePackage,
    [switch]$Differential,
    [string]$TestFilter = 'FullyQualifiedName~UnitTests.CodegenTests',
    [switch]$Samples,
    [string[]]$SampleProjects = @(
        'TableViewSampleApp\TableViewSampleApp.csproj',
        'WinUICsDesktopSampleApp\WinUICsDesktopSampleApp.sln',
        'FolderTreeApp\FolderTreeApp\FolderTreeApp.csproj',
        'FolderTreeAppCpp\FolderTreeAppCpp.sln',
        'TableViewApp\TableViewAppCppUnpackaged\TableViewAppCppUnpackaged.vcxproj'),
    [switch]$DesignTime,
    [string[]]$DesignTimeProjects = @(
        'Basic\CSharp\Simple\SimpleCS.csproj',
        'Basic\CppWinRT\Simple\SimpleCppWinRT.vcxproj'),
    [switch]$SkipRegressionProjects
)

$ErrorActionPreference = 'Stop'

foreach ($name in 'EnvironmentInitialized', 'RepoRoot', 'BuildOutputRoot', '_BuildArch', '_BuildType') {
    if (-not (Get-Item "Env:$name" -ErrorAction SilentlyContinue)) {
        throw "The WinUI build environment is not initialized. Run init.cmd <flavor> in this prompt, then retry."
    }
}
if ($Differential -and -not $ReferencePackage) {
    throw "-Differential needs a XAML compiler to compare with: pass -ReferencePackage."
}

$flavor = "$($env:_BuildArch)$($env:_BuildType)"
$configuration = if ($env:Configuration) { $env:Configuration } else { 'Debug' }
$packaging = Join-Path (Split-Path $env:BuildOutputRoot -Parent) "packaging\$configuration"
$platform = switch ($env:_BuildArch) { 'amd64' { 'x64' } 'arm64' { 'ARM64' } 'x86' { 'x86' } default { throw "Unsupported architecture '$($env:_BuildArch)'." } }

if (-not $ExePath) {
    $ExePath = Join-Path $packaging 'tools\win-x64\XamlCompiler.exe'
}
if (-not (Test-Path $ExePath)) {
    throw "The XAML compiler was not found at '$ExePath'. Build src\XamlCompiler\XamlCompiler.sln first, or pass -ExePath."
}
$ExePath = (Resolve-Path $ExePath).Path
$compilerArguments = @("/p:XamlCompilerNativeExePath=$ExePath")

# The global properties that make a build use the XAML compiler of the reference package instead of the in-repo one
function Get-ReferenceArguments {
    $package = (Resolve-Path $ReferencePackage).Path
    $arguments = @("/p:XamlCompilerPropsAndTargetsDirectory=$package\build\")
    $tasks = Join-Path $package 'tools\Microsoft.UI.Xaml.Markup.Compiler.Tasks.dll'
    $legacyTask = Join-Path $package 'tools\net472\Microsoft.UI.Xaml.Markup.Compiler.dll'
    if (Test-Path $tasks) {
        $exe = Join-Path $package 'tools\win-x64\XamlCompiler.exe'
        $arguments += "/p:XamlCompilerTaskPath=$tasks", "/p:XamlCompilerNativeExePath=$exe"
    }
    elseif (Test-Path $legacyTask) {
        $arguments += "/p:XamlCompilerTaskPath=$legacyTask", "/p:XamlCompilerJsonTaskPath=$package\tools\net472\Microsoft.UI.Xaml.Markup.Compiler.IO.dll"
    }
    else {
        throw "'$package' is not a package layout with the XAML compiler (no tools\Microsoft.UI.Xaml.Markup.Compiler.Tasks.dll nor tools\net472\Microsoft.UI.Xaml.Markup.Compiler.dll)."
    }
    if (-not (Test-Path "$package\build\Microsoft.UI.Xaml.Markup.Compiler.interop.targets")) {
        throw "The XAML compiler targets were not found in '$package\build'."
    }
    return $arguments
}
$referenceArguments = if ($ReferencePackage) { @(Get-ReferenceArguments) } else { @() }

$solution = Join-Path $env:RepoRoot 'src\XamlCompiler\XamlCompilerTests.sln'
$compilerTests = Join-Path $env:BuildOutputRoot "$flavor\CompilerTests"
$workRoot = Join-Path $env:BuildOutputRoot "$flavor\NativeCompilerTests"

# The files the XAML compiler generates, in the given directory. A C++ project has more: the C++/WinRT tools write the
# <Type>.g.h/.g.cpp/winrt\ files next to the XAML compiler's own (<Page>.xaml.g.h, XamlTypeInfo*, XamlMetaDataProvider.*, ...).
function Test-XamlCompilerFile([IO.FileInfo]$file) {
    return $file.FullName -notmatch '\\winrt\\' -and (
        $file.Name -match '\.xaml\.g\.|\.g\.i\.cs$|\.xbf$|\.xr\.xml$|^XamlTypeInfo|^XamlMetaDataProvider\.(idl|cpp)$|^XamlLibMetadataProvider|^XamlBindingInfo' -or
        ($file.Extension -in '.cs', '.vb' -and $file.Name -match '\.g\.'))
}

# What the XAML compiler generated for the regression projects, which is what is compared in differential mode
function Get-GeneratedFiles {
    Get-ChildItem $compilerTests -Recurse -Directory -Filter generated |
        ForEach-Object { Get-ChildItem $_.FullName -Recurse -File } |
        Where-Object { Test-XamlCompilerFile $_ } |
        ForEach-Object { [pscustomobject]@{ Key = $_.FullName.Substring($compilerTests.Length + 1); Path = $_.FullName } }
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
    New-Item -ItemType Directory -Force $directory | Out-Null
    foreach ($file in $files) {
        $destination = Join-Path $directory $file.Key
        New-Item -ItemType Directory -Force (Split-Path $destination) | Out-Null
        Copy-Item -LiteralPath $file.Path $destination
    }
}

# Get-FileHash isn't available when Windows PowerShell is started from PowerShell 7 (PSModulePath)
function Get-Hash([string]$path) {
    $sha = [Security.Cryptography.SHA256]::Create()
    try { return [BitConverter]::ToString($sha.ComputeHash([IO.File]::ReadAllBytes($path))) } finally { $sha.Dispose() }
}

# Compares the files of a snapshot (expected) with the given files (actual)
function Compare-Snapshot([string]$directory, $files, [string]$what) {
    $actual = @{}
    foreach ($file in $files) { $actual[$file.Key] = $file.Path }

    $expected = @{}
    if (Test-Path $directory) {
        Get-ChildItem $directory -Recurse -File | ForEach-Object { $expected[$_.FullName.Substring($directory.Length + 1)] = $_.FullName }
    }

    $differences = @()
    foreach ($key in $expected.Keys) {
        if (-not $actual.ContainsKey($key)) { $differences += "missing   $key" }
        elseif ((Get-Hash $actual[$key]) -ne (Get-Hash $expected[$key])) { $differences += "different $key" }
    }
    foreach ($key in $actual.Keys) {
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
        Write-Host "[native-tests] Building the regression projects with the XAML compiler of $ReferencePackage ..."
        Reset-GeneratedFiles
        Invoke-TestsBuild $referenceArguments
        $baseline = Join-Path $workRoot 'baseline'
        Save-Snapshot (Get-GeneratedFiles) $baseline
    }

    Write-Host "[native-tests] Building the regression projects with $ExePath ..."
    Reset-GeneratedFiles
    $buildStart = Get-Date
    Invoke-TestsBuild $compilerArguments

    # Make sure that the compiler ran (the CompileXaml task leaves its XamlCompiler.<pass>.output.json next to the generated files)
    $runs = @(Get-ChildItem $compilerTests -Recurse -File -Filter 'XamlCompiler.*.output.json' | Where-Object { $_.LastWriteTime -ge $buildStart })
    if ($runs.Count -eq 0) { throw 'The XAML compiler did not run for any project.' }
    Write-Host "[native-tests] The XAML compiler ran for $($runs.Count) compilation passes."

    if ($Differential) {
        Write-Host "[native-tests] Comparing the generated files with the ones of the reference compiler..."
        if (-not (Compare-Snapshot $baseline (Get-GeneratedFiles) 'regression projects')) { $failed = $true }
    }

    if ($TestFilter) {
        Write-Host "[native-tests] Running tests: $TestFilter"
        & (Join-Path $env:RepoRoot 'src\XamlCompiler\runtests.cmd') "/TestCaseFilter:$TestFilter"
        if ($LASTEXITCODE -ne 0) { $failed = $true }
    }
}

if ($DesignTime) {
    $regressionProjects = Join-Path $env:RepoRoot 'src\XamlCompiler\Tests\RegressionProjects'
    $designTimeWork = Join-Path $workRoot 'designtime'
    if (Test-Path $designTimeWork) { Remove-Item $designTimeWork -Recurse -Force }
    New-Item -ItemType Directory -Force $designTimeWork | Out-Null

    # Builds a regression project like XamlCompilerTests.sln does, or (-DesignTimeBuild) like a design-time build of Visual
    # Studio: managed projects run CompileDesignTime, and C++ projects the generator target of their XAML files
    function Invoke-ProjectBuild([string]$project, [string]$log, [switch]$DesignTimeBuild, [string[]]$extraArguments = @()) {
        $arguments = @($project, '/nologo', '/v:m', '/nodeReuse:false', '/p:RunAnalyzers=false', "/p:Configuration=$configuration", "/p:Platform=$platform", "/flp:LogFile=$log;Verbosity=normal")
        if ($DesignTimeBuild) {
            $target = if ($project.EndsWith('.vcxproj')) { 'ResolveReferences;DesignTimeMarkupCompilation' } else { 'CompileDesignTime' }
            $arguments += "/t:$target", '/p:DesignTimeBuild=true', '/p:BuildingInsideVisualStudio=true', '/p:BuildingProject=false', '/p:SkipCompilerExecution=true', '/p:ProvideCommandLineArgs=true'
        }
        & msbuild @arguments @compilerArguments @extraArguments | Out-Null
        return $LASTEXITCODE
    }

    function Get-ProjectProperty([string]$project, [string]$property) {
        $output = @(& msbuild $project /nologo /noconlog "/p:Configuration=$configuration" "/p:Platform=$platform" "-getProperty:$property")
        if ($LASTEXITCODE -ne 0) { throw "Evaluating $property of '$project' failed." }
        # The value is the last line (evaluation warnings may come before it)
        return @($output | Where-Object { "$_".Trim() -ne '' })[-1].Trim()
    }

    # The files the XAML compiler wrote for a project, and its saved state
    function Get-ProjectOutputs([string]$generatedDirectory, [string]$savedState) {
        $files = @(Get-ChildItem $generatedDirectory -Recurse -File -ErrorAction SilentlyContinue | Where-Object { Test-XamlCompilerFile $_ })
        if (Test-Path $savedState) { $files += Get-Item $savedState }
        $files | ForEach-Object {
            $key = if ($_.FullName.StartsWith($generatedDirectory, [StringComparison]::OrdinalIgnoreCase)) { $_.FullName.Substring($generatedDirectory.TrimEnd('\').Length + 1) } else { $_.Name }
            [pscustomobject]@{ Key = $key; Path = $_.FullName }
        }
    }

    foreach ($relativeProject in $DesignTimeProjects) {
        $project = Join-Path $regressionProjects $relativeProject
        if (-not (Test-Path $project)) { throw "The design-time project '$relativeProject' was not found under '$regressionProjects'." }
        $name = [IO.Path]::GetFileNameWithoutExtension($project)
        $projectDirectory = Split-Path $project
        $generatedDirectory = Get-ProjectProperty $project 'XamlGeneratedOutputPath'
        if (-not [IO.Path]::IsPathRooted($generatedDirectory)) { $generatedDirectory = Join-Path $projectDirectory $generatedDirectory }
        $savedState = Get-ProjectProperty $project 'XamlSavedStateFilePath'
        if (-not [IO.Path]::IsPathRooted($savedState)) { $savedState = Join-Path $projectDirectory $savedState }
        $intelliSensePattern = if ($project.EndsWith('.vcxproj')) { '\.xaml\.g\.h$' } else { '\.g\.i\.cs$' }

        # 1. A design-time build after a real build changes none of the files of the real build
        Write-Host "[native-tests] Design-time build of $name after a real build..."
        if ((Invoke-ProjectBuild $project (Join-Path $designTimeWork "$name-real.log")) -ne 0) { throw "Building '$relativeProject' failed." }
        $snapshot = Join-Path $designTimeWork "$name-real"
        Save-Snapshot @(Get-ProjectOutputs $generatedDirectory $savedState) $snapshot
        $exitCode = Invoke-ProjectBuild $project (Join-Path $designTimeWork "$name-dt.log") -DesignTimeBuild
        if ($exitCode -ne 0) { Write-Host "  The design-time build failed ($exitCode)."; $failed = $true }
        if (-not (Compare-Snapshot $snapshot @(Get-ProjectOutputs $generatedDirectory $savedState) "$name, design-time build after a real build")) { $failed = $true }
        if (-not (Test-Path (Join-Path $generatedDirectory 'XamlCompiler.DesignTimeBuild.output.json'))) {
            Write-Host "  The design-time build didn't run the XAML compiler."; $failed = $true
        }
        if (Test-Path (Join-Path $generatedDirectory 'XamlCompiler.DesignTimeBuild.overlays.json')) {
            Write-Host "  The design-time build left its IDE overlays file behind."; $failed = $true
        }

        # 2. On a clean project, the design-time build generates the IntelliSense files of the next real build
        Write-Host "[native-tests] Design-time build of $name on a clean project, then a real build..."
        Get-ProjectOutputs $generatedDirectory $savedState | ForEach-Object { Remove-Item -LiteralPath $_.Path -Force }
        $exitCode = Invoke-ProjectBuild $project (Join-Path $designTimeWork "$name-clean-dt.log") -DesignTimeBuild
        if ($exitCode -ne 0) { Write-Host "  The design-time build failed ($exitCode)."; $failed = $true }
        $intelliSense = @(Get-ProjectOutputs $generatedDirectory $savedState | Where-Object { $_.Key -match $intelliSensePattern })
        if ($intelliSense.Count -eq 0) { Write-Host "  The design-time build generated no IntelliSense file."; $failed = $true }
        $snapshot = Join-Path $designTimeWork "$name-clean-dt"
        Save-Snapshot $intelliSense $snapshot
        if ((Invoke-ProjectBuild $project (Join-Path $designTimeWork "$name-clean-real.log")) -ne 0) { throw "Building '$relativeProject' failed." }
        if (-not (Compare-Snapshot $snapshot @(Get-ProjectOutputs $generatedDirectory $savedState | Where-Object { $_.Key -match $intelliSensePattern }) "$name, design-time vs real build IntelliSense files")) { $failed = $true }

        # 3. A XAML error doesn't fail a design-time build (it is a warning), but fails a real build
        Write-Host "[native-tests] Design-time build of $name with a XAML error..."
        $brokenDirectory = Join-Path $designTimeWork "$name-broken"
        New-Item -ItemType Directory -Force $brokenDirectory | Out-Null
        $brokenXaml = Join-Path $brokenDirectory 'BrokenPage.xaml'
        Set-Content -LiteralPath $brokenXaml -Encoding UTF8 -Value '<Page xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"><NotAKnownType /></Page>'
        $brokenTargets = Join-Path $brokenDirectory 'AddBrokenPage.targets'
        Set-Content -LiteralPath $brokenTargets -Encoding UTF8 -Value "<Project><ItemGroup><Page Include=`"$brokenXaml`" Link=`"BrokenPage.xaml`" /></ItemGroup></Project>"
        $brokenArguments = @("/p:CustomAfterMicrosoftCommonTargets=$brokenTargets")
        try {
            $log = Join-Path $designTimeWork "$name-broken-dt.log"
            $exitCode = Invoke-ProjectBuild $project $log -DesignTimeBuild -extraArguments $brokenArguments
            if ($exitCode -ne 0) { Write-Host "  The design-time build with a XAML error failed ($exitCode), see $log."; $failed = $true }
            if (-not (Select-String -LiteralPath $log -Pattern 'BrokenPage\.xaml.*warning WMC' -Quiet)) {
                Write-Host "  The design-time build didn't report the XAML error as a warning, see $log."; $failed = $true
            }
            $log = Join-Path $designTimeWork "$name-broken-real.log"
            if ((Invoke-ProjectBuild $project $log -extraArguments $brokenArguments) -eq 0 -or -not (Select-String -LiteralPath $log -Pattern 'BrokenPage\.xaml.*error WMC' -Quiet)) {
                Write-Host "  The real build with a XAML error didn't fail with the XAML error, see $log."; $failed = $true
            }
        }
        finally {
            # Back to the outputs of a real build of the project as it is
            Get-ProjectOutputs $generatedDirectory $savedState | ForEach-Object { Remove-Item -LiteralPath $_.Path -Force }
            if ((Invoke-ProjectBuild $project (Join-Path $designTimeWork "$name-restore.log")) -ne 0) { throw "Building '$relativeProject' failed." }
        }
    }
}

if ($Samples) {
    $samplesRoot = Join-Path $env:RepoRoot 'Samples'
    $samplesObj = Join-Path $env:BuildOutputRoot "$flavor\Samples"
    $samplesWork = Join-Path $workRoot 'samples'
    # The XAML compiler targets and tools XamlCompiler.sln built, instead of the ones in the WinUI component package
    $compilerTargets = Join-Path $packaging 'build\'
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
    # metadata built from them (XamlTypeInfo.g.obj, XamlMetaDataProvider.winmd, ...) aren't deterministic. The JSON files of the
    # CompileXaml task and XamlSaveStateFile.xml (the incremental build state) aren't compared.
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
    function Invoke-SamplesBuild([string]$pass, [string[]]$extraArguments, [string]$expectedCompiler) {
        if (Test-Path $samplesObj) { Remove-Item $samplesObj -Recurse -Force }
        $diagnostics = @()
        foreach ($sample in $SampleProjects) {
            $log = Join-Path $samplesWork "$pass-$(($sample -replace '[\\/]', '_')).log"
            Write-Host "[native-tests] Building $sample ($pass)..."
            & msbuild (Join-Path $samplesRoot $sample) /restore /nologo /clp:ErrorsOnly /m /nodeReuse:false "/p:Platform=$platform" "/p:Configuration=$configuration" `
                "/flp:LogFile=$log;Verbosity=normal" @extraArguments | Out-Host
            if ($LASTEXITCODE -ne 0) { throw "Building the sample '$sample' failed ($LASTEXITCODE), see $log." }
            if ($expectedCompiler -and -not (Select-String -LiteralPath $log -SimpleMatch $expectedCompiler -Quiet)) {
                throw "The build of the sample '$sample' ($pass) didn't use $expectedCompiler, see $log."
            }
            $diagnostics += Get-Content -LiteralPath $log | Where-Object { $_ -match 'XamlCompiler (warning|error) WMC' } | ForEach-Object { $_.Trim() }
        }
        return @($diagnostics | Sort-Object -Unique)
    }

    if (Test-Path $samplesWork) { Remove-Item $samplesWork -Recurse -Force }
    New-Item -ItemType Directory -Force $samplesWork | Out-Null

    if ($ReferencePackage) {
        $referenceDiagnostics = @(Invoke-SamplesBuild 'reference' $referenceArguments $null)
        $referenceFiles = @(Get-SampleGeneratedFiles)
        if ($referenceFiles.Count -eq 0) { throw "The reference compiler didn't generate anything for the samples under '$samplesObj'." }
        Save-Snapshot $referenceFiles (Join-Path $samplesWork 'baseline')
    }
    $diagnostics = @(Invoke-SamplesBuild 'current' (@("/p:XamlCompilerPropsAndTargetsDirectory=$compilerTargets") + $compilerArguments) $ExePath)
    if (@(Get-SampleGeneratedFiles).Count -eq 0) { throw "The XAML compiler didn't generate anything for the samples under '$samplesObj'." }

    if ($ReferencePackage) {
        Write-Host "[native-tests] Comparing the files generated for the samples with the ones of the reference compiler..."
        if (-not (Compare-Snapshot (Join-Path $samplesWork 'baseline') (Get-SampleGeneratedFiles) 'samples')) { $failed = $true }

        $diagnosticDifferences = @($referenceDiagnostics | Where-Object { $diagnostics -notcontains $_ } | ForEach-Object { "missing $_" }) +
            @($diagnostics | Where-Object { $referenceDiagnostics -notcontains $_ } | ForEach-Object { "extra   $_" })
        Write-Host "[native-tests] Compared $($referenceDiagnostics.Count) XAML compiler warning(s)/error(s) (samples): $($diagnosticDifferences.Count) difference(s)."
        $diagnosticDifferences | ForEach-Object { Write-Host "  $_" }
        if ($diagnosticDifferences.Count -gt 0) { $failed = $true }
    }
}

if ($failed) { Write-Host '[native-tests] FAILED'; exit 1 }
Write-Host '[native-tests] PASSED'
exit 0
