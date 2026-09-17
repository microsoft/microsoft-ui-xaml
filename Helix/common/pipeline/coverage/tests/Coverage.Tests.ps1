# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

# Tests coverage-script behavior and failure paths with fixtures, without running WinUI tests.

$script:coverageScripts = Split-Path $PSScriptRoot -Parent
$script:fixtureRoot = Join-Path $PSScriptRoot ('.fixtures-' + [guid]::NewGuid().ToString('N'))
$script:compiledTool = Join-Path $script:fixtureRoot 'CoverageToolFixture.exe'
$script:caseSequence = 0
$script:savedEnvironment = @{}
foreach ($name in @('WINUI_COVERAGE_TEST_ROOT', 'WINUI_COVERAGE_TEST_MODE', 'WINUI_COVERAGE_TEST_DISCOVERY', 'ProgramFiles(x86)'))
{
    $script:savedEnvironment[$name] = [Environment]::GetEnvironmentVariable($name)
}

function Write-FixtureFile([string]$Path, [string]$Content = 'fixture')
{
    New-Item -ItemType Directory -Path (Split-Path $Path) -Force | Out-Null
    [IO.File]::WriteAllText($Path, $Content)
}

function New-CoverageFixture
{
    $script:caseSequence++
    $script:caseRoot = Join-Path $script:fixtureRoot "$script:caseSequence case"
    $env:WINUI_COVERAGE_TEST_ROOT = $script:caseRoot
    $env:WINUI_COVERAGE_TEST_MODE = ''
    $env:WINUI_COVERAGE_TEST_DISCOVERY = ''
    $script:payload = Join-Path $script:caseRoot 'payload'
    $script:symbols = Join-Path $script:caseRoot 'symbols'
    $script:sessionId = 'coverage-regression-123'
    $script:vsRoot = Join-Path $script:caseRoot 'vs'
    $script:tool = Join-Path $script:vsRoot 'Common7\Microsoft.CodeCoverage.Console.exe'
    $script:modules = @('Microsoft.ui.xaml.dll', 'Microsoft.UI.Xaml.Controls.dll')
    $script:appDirectories = @('app one', 'nested\app two', 'nested\app three')

    Write-FixtureFile $script:tool
    Copy-Item -LiteralPath $script:compiledTool -Destination $script:tool -Force
    foreach ($directory in @(
        'Team Tools\Dynamic Code Coverage Tools',
        'Common7\IDE\CommonExtensions\Platform\InstrumentationEngine\x64',
        'Common7\IDE\CommonExtensions\Platform\InstrumentationEngine\x86'
    ))
    {
        Write-FixtureFile (Join-Path $script:vsRoot "$directory\dependency.dll")
    }
    foreach ($app in $script:appDirectories)
    {
        foreach ($module in $script:modules)
        {
            Write-FixtureFile (Join-Path $script:payload "$app\$module") "original:$module"
        }
        Write-FixtureFile (Join-Path $script:payload "$app\Unrelated.dll") 'leave dll alone'
        Write-FixtureFile (Join-Path $script:payload "$app\Unrelated.pdb") 'leave pdb alone'
    }
    foreach ($module in $script:modules)
    {
        Write-FixtureFile (Join-Path $script:symbols ([IO.Path]::ChangeExtension($module, '.pdb'))) "symbols:$module"
    }
    $global:LASTEXITCODE = 0
}

function Invoke-Instrumentation([string]$SessionId = $script:sessionId)
{
    & "$script:coverageScripts\Instrument-CoveragePayload.ps1" -PayloadDir $script:payload `
        -SymbolsSearchRoot $script:symbols -SessionId $SessionId -CoverageToolPath $script:tool -ErrorAction Stop
}

function Get-ToolCalls([string]$Command)
{
    $log = Join-Path $script:caseRoot 'calls.txt'
    if (Test-Path -LiteralPath $log)
    {
        foreach ($line in Get-Content -LiteralPath $log)
        {
            $arguments = $line -split "`t"
            if ($arguments[0] -eq $Command)
            {
                [pscustomobject]@{ Arguments = $arguments }
            }
        }
    }
}

function Invoke-CoverageMerge
{
    & "$script:coverageScripts\Merge-CodeCoverage.ps1" -InputDir $script:inputDir `
        -OutputDir $script:outputDir -CoverageToolPath $script:tool -ErrorAction Stop
}

function Invoke-Collector([scriptblock]$RunTests)
{
    & "$script:coverageScripts\Invoke-WithCodeCoverage.ps1" -PayloadDir $script:payload `
        -OutputFile $script:coverageOutput -RunTests $RunTests
}

function Assert-FixtureProcessExited([int]$ProcessId)
{
    $process = Get-Process -Id $ProcessId -ErrorAction SilentlyContinue
    if ($process)
    {
        try { $process.WaitForExit(5000) | Should Be $true }
        finally { $process.Dispose() }
    }
}

function Invoke-PayloadPreparation
{
    $template = Get-Content -LiteralPath "$PSScriptRoot\..\..\..\..\..\build\AzurePipelinesTemplates\WinUI-CreateTestPayload-Job.yml" -Raw
    $match = [regex]::Match($template, "(?ms)displayName: 'Prepare final runtime copies for code coverage'.*?script: \|\r?\n(?<code>(?: {10}[^\r\n]*\r?\n)+)")
    if (-not $match.Success) { throw 'Coverage preparation task was not found.' }
    $code = $match.Groups['code'].Value.Replace('$(artifactsDir)', "$script:caseRoot\artifacts")
    $code = $code.Replace('$(buildFlavor)', 'x86chk').Replace('$(testBinaryPath)', $script:payload)
    & ([scriptblock]::Create($code))
}

try
{
    New-Item -ItemType Directory -Path $script:fixtureRoot | Out-Null
    $compiler = Join-Path $env:WINDIR 'Microsoft.NET\Framework64\v4.0.30319\csc.exe'
    & $compiler /nologo /target:exe "/out:$script:compiledTool" "$PSScriptRoot\CoverageToolFixture.cs"
    if ($LASTEXITCODE -ne 0)
    {
        throw 'Could not compile the coverage tool fixture with the Windows .NET Framework compiler.'
    }

    Describe 'Coverage tool discovery' {
        BeforeEach { New-CoverageFixture }

        It 'returns an explicit tool path containing spaces' {
            (& "$script:coverageScripts\Get-CoverageTool.ps1" -CoverageToolPath $script:tool) | Should Be $script:tool
            @(Get-ToolCalls '-latest').Count | Should Be 0
        }

        It 'rejects a missing explicit tool' {
            { & "$script:coverageScripts\Get-CoverageTool.ps1" -CoverageToolPath "$script:caseRoot\missing.exe" -ErrorAction Stop } | Should Throw 'Cannot find path'
        }

        It 'uses the first native coverage tool returned by vswhere' {
            $installer = Join-Path $script:caseRoot 'Microsoft Visual Studio\Installer\vswhere.exe'
            Write-FixtureFile $installer
            Copy-Item -LiteralPath $script:compiledTool -Destination $installer -Force
            [Environment]::SetEnvironmentVariable('ProgramFiles(x86)', $script:caseRoot)
            $env:WINUI_COVERAGE_TEST_DISCOVERY = "$script:tool`r`nunused-second-match.exe`r`n"
            (& "$script:coverageScripts\Get-CoverageTool.ps1") | Should Be $script:tool
            $call = @(Get-ToolCalls '-latest')[0]
            ($call.Arguments -join '|') | Should Be '-latest|-products|*|-find|Common7\IDE\Extensions\Microsoft\CodeCoverage.Console\Microsoft.CodeCoverage.Console.exe'
        }

        It 'fails when Visual Studio contains no coverage tool' {
            $installer = Join-Path $script:caseRoot 'Microsoft Visual Studio\Installer\vswhere.exe'
            Write-FixtureFile $installer
            Copy-Item -LiteralPath $script:compiledTool -Destination $installer -Force
            [Environment]::SetEnvironmentVariable('ProgramFiles(x86)', $script:caseRoot)
            { & "$script:coverageScripts\Get-CoverageTool.ps1" } | Should Throw 'is required'
        }

        It 'fails when the Visual Studio locator is missing' {
            [Environment]::SetEnvironmentVariable('ProgramFiles(x86)', $script:caseRoot)
            { & "$script:coverageScripts\Get-CoverageTool.ps1" -ErrorAction Stop } | Should Throw 'not recognized'
        }
    }

    Describe 'Coverage pipeline payload preparation' {
        BeforeEach {
            New-CoverageFixture
            $script:product = "$script:caseRoot\artifacts\drop\x86chk\Product"
            foreach ($module in $script:modules)
            {
                Write-FixtureFile "$script:product\$module" "final link:$module"
            }
        }

        It 'uses final product copies instead of the earlier component-package link' {
            Invoke-PayloadPreparation
            foreach ($module in $script:modules)
            {
                foreach ($app in $script:appDirectories)
                {
                    [IO.File]::ReadAllText("$script:payload\$app\$module") | Should Be "final link:$module"
                }
                [IO.File]::ReadAllText("$script:product\$module") | Should Be "final link:$module"
            }
            [IO.File]::ReadAllText("$script:payload\app one\Unrelated.dll") | Should Be 'leave dll alone'
            [IO.File]::ReadAllText("$script:payload\app one\Unrelated.pdb") | Should Be 'leave pdb alone'
        }

        It 'fails when the final product DLL is missing' {
            Remove-Item -LiteralPath "$script:product\Microsoft.ui.xaml.dll"
            { Invoke-PayloadPreparation } | Should Throw 'Cannot find path'
        }

        It 'fails when the payload has no target DLL copies' {
            Get-ChildItem -LiteralPath $script:payload -Recurse -Filter 'Microsoft.ui.xaml.dll' | Remove-Item
            { Invoke-PayloadPreparation } | Should Throw 'No payload copies'
        }
    }

    Describe 'Coverage payload instrumentation' {
        BeforeEach { New-CoverageFixture }

        It 'instruments only the two target modules and distributes every DLL and runtime copy' {
            $originalHashes = @{}
            foreach ($module in $script:modules)
            {
                $originalHashes[$module] = (Get-FileHash -LiteralPath "$script:payload\app one\$module").Hash
            }
            Invoke-Instrumentation
            $calls = @(Get-ToolCalls 'instrument')
            $calls.Count | Should Be 2
            foreach ($module in $script:modules)
            {
                @($calls | Where-Object { [IO.Path]::GetFileName($_.Arguments[1]) -eq $module }).Count | Should Be 1
                foreach ($app in $script:appDirectories)
                {
                    $directory = Join-Path $script:payload $app
                    [IO.File]::ReadAllText((Join-Path $directory $module)) | Should Be "original:$module|instrumented:$script:sessionId"
                    foreach ($architecture in @('32', '64'))
                    {
                        [IO.File]::ReadAllText((Join-Path $directory "static_covrun$architecture.dll")) | Should Be "runtime-$architecture"
                    }
                    Test-Path -LiteralPath (Join-Path $directory ([IO.Path]::ChangeExtension($module, '.pdb'))) | Should Be $false
                }
                $marker = @(Get-Content -LiteralPath "$script:payload\_coverage-instrumented-$module.txt")
                $marker.Count | Should Be 3
                $marker[0] | Should Be $script:sessionId
                $marker[1] | Should Be (Get-FileHash -LiteralPath "$script:payload\app one\$module").Hash
                $marker[2] | Should Be $originalHashes[$module]
                [IO.File]::ReadAllText((Join-Path $script:symbols ([IO.Path]::ChangeExtension($module, '.pdb')))) | Should Be "symbols:$module"
            }
            foreach ($app in $script:appDirectories)
            {
                [IO.File]::ReadAllText("$script:payload\$app\Unrelated.dll") | Should Be 'leave dll alone'
                [IO.File]::ReadAllText("$script:payload\$app\Unrelated.pdb") | Should Be 'leave pdb alone'
            }
            foreach ($call in $calls)
            {
                ($call.Arguments[2..5] -join '|') | Should Be "--session-id|$script:sessionId|--settings|$script:coverageScripts\coverage.config"
            }
            (Get-Content -LiteralPath "$script:payload\_coverage-session-id.txt").Trim() | Should Be $script:sessionId
            Test-Path -LiteralPath "$script:payload\CoverageTool\Microsoft.CodeCoverage.Console.exe" | Should Be $true
            foreach ($dependency in @('dependency.dll', 'x64\dependency.dll', 'x86\dependency.dll'))
            {
                Test-Path -LiteralPath "$script:payload\CoverageTool\$dependency" | Should Be $true
            }
        }

        It 'retries the same session without double instrumentation and repairs distributed copies' {
            Invoke-Instrumentation
            Write-FixtureFile "$script:payload\nested\app two\Microsoft.ui.xaml.dll" 'original:Microsoft.ui.xaml.dll'
            Remove-Item -LiteralPath "$script:payload\nested\app two\static_covrun64.dll"
            Invoke-Instrumentation
            @(Get-ToolCalls 'instrument').Count | Should Be 2
            [IO.File]::ReadAllText("$script:payload\nested\app two\Microsoft.ui.xaml.dll") | Should Be "original:Microsoft.ui.xaml.dll|instrumented:$script:sessionId"
            [IO.File]::ReadAllText("$script:payload\nested\app two\static_covrun64.dll") | Should Be 'runtime-64'
        }

        It 'warns that packaged runtime copies are outside coverage without changing the package' {
            $package = "$script:payload\Test\IXMPTestApp.appx"
            Write-FixtureFile $package 'signed package'
            Mock Write-Host {}
            Invoke-Instrumentation
            [IO.File]::ReadAllText($package) | Should Be 'signed package'
            Assert-MockCalled Write-Host -Times 1 -Exactly -Scope It -ParameterFilter {
                "$Object" -like '##vso*warning*Runtime copies inside APPX/MSIX packages*not instrumented*'
            }
        }

        foreach ($module in @('Microsoft.ui.xaml.dll', 'Microsoft.UI.Xaml.Controls.dll'))
        {
            It "rejects a different-build $module on retry without overwriting it" {
                Invoke-Instrumentation
                $otherBuild = "$script:payload\nested\app two\$module"
                Write-FixtureFile $otherBuild 'another build'
                { Invoke-Instrumentation } | Should Throw "copies of $module differ"
                [IO.File]::ReadAllText($otherBuild) | Should Be 'another build'
                @(Get-ToolCalls 'instrument').Count | Should Be 2
            }
        }

        It 'rejects legacy markers that cannot prove the original binary identity' {
            Invoke-Instrumentation
            $path = "$script:payload\_coverage-instrumented-Microsoft.ui.xaml.dll.txt"
            $marker = @(Get-Content -LiteralPath $path)
            Set-Content -LiteralPath $path -Value $marker[0..1]
            { Invoke-Instrumentation } | Should Throw 'Invalid coverage marker'
            @(Get-ToolCalls 'instrument').Count | Should Be 2
        }

        It 'resumes after the second module fails without instrumenting the first twice' {
            $env:WINUI_COVERAGE_TEST_MODE = 'fail-controls'
            { Invoke-Instrumentation } | Should Throw 'Instrumentation failed'
            Test-Path -LiteralPath "$script:payload\_coverage-session-id.txt" | Should Be $false
            $env:WINUI_COVERAGE_TEST_MODE = ''
            Invoke-Instrumentation
            @(Get-ToolCalls 'instrument').Count | Should Be 3
            [IO.File]::ReadAllText("$script:payload\app one\Microsoft.ui.xaml.dll") | Should Be "original:Microsoft.ui.xaml.dll|instrumented:$script:sessionId"
        }

        It 'rejects changing the session of instrumented binaries' {
            Invoke-Instrumentation
            { Invoke-Instrumentation -SessionId 'another-session' } | Should Throw 'another session'
            @(Get-ToolCalls 'instrument').Count | Should Be 2
            (Get-Content -LiteralPath "$script:payload\_coverage-session-id.txt").Trim() | Should Be $script:sessionId
        }

        It 'allows a new session after all binaries are rebuilt despite stale markers' {
            Invoke-Instrumentation
            foreach ($app in $script:appDirectories)
            {
                foreach ($module in $script:modules)
                {
                    Write-FixtureFile "$script:payload\$app\$module" "rebuilt:$module"
                }
            }
            Invoke-Instrumentation -SessionId 'rebuilt-session'
            @(Get-ToolCalls 'instrument').Count | Should Be 4
            (Get-Content -LiteralPath "$script:payload\_coverage-session-id.txt").Trim() | Should Be 'rebuilt-session'
        }

        It 'rejects differing copies before invoking the instrumenter' {
            Write-FixtureFile "$script:payload\nested\app two\Microsoft.ui.xaml.dll" 'another build'
            { Invoke-Instrumentation } | Should Throw 'copies of Microsoft.ui.xaml.dll differ'
            @(Get-ToolCalls 'instrument').Count | Should Be 0
        }

        It 'rejects a missing target module' {
            Get-ChildItem -LiteralPath $script:payload -Recurse -Filter 'Microsoft.ui.xaml.dll' | Remove-Item
            { Invoke-Instrumentation } | Should Throw 'No copies of Microsoft.ui.xaml.dll'
            @(Get-ToolCalls 'instrument').Count | Should Be 0
        }

        It 'rejects missing matching symbols' {
            Remove-Item -LiteralPath "$script:symbols\Microsoft.ui.xaml.pdb"
            { Invoke-Instrumentation } | Should Throw 'found 0'
            @(Get-ToolCalls 'instrument').Count | Should Be 0
        }

        It 'rejects ambiguous matching symbols' {
            Write-FixtureFile "$script:symbols\duplicate\Microsoft.ui.xaml.pdb"
            { Invoke-Instrumentation } | Should Throw 'found 2'
            @(Get-ToolCalls 'instrument').Count | Should Be 0
        }

        It 'does not overwrite or remove an existing matching payload PDB' {
            Write-FixtureFile "$script:payload\app one\Microsoft.ui.xaml.pdb" 'existing symbols'
            { Invoke-Instrumentation } | Should Throw 'symbol-free'
            [IO.File]::ReadAllText("$script:payload\app one\Microsoft.ui.xaml.pdb") | Should Be 'existing symbols'
            @(Get-ToolCalls 'instrument').Count | Should Be 0
        }

        It 'cleans up only staged symbols when the instrumenter fails' {
            $env:WINUI_COVERAGE_TEST_MODE = 'fail-instrument'
            { Invoke-Instrumentation } | Should Throw 'exit 23'
            Test-Path -LiteralPath "$script:payload\app one\Microsoft.ui.xaml.pdb" | Should Be $false
            Test-Path -LiteralPath "$script:payload\_coverage-instrumented-Microsoft.ui.xaml.dll.txt" | Should Be $false
            [IO.File]::ReadAllText("$script:payload\app one\Unrelated.pdb") | Should Be 'leave pdb alone'
            Test-Path -LiteralPath "$script:symbols\Microsoft.ui.xaml.pdb" | Should Be $true
        }

        It 'rejects a missing instrumentation runtime' {
            $env:WINUI_COVERAGE_TEST_MODE = 'no-runtime'
            { Invoke-Instrumentation } | Should Throw 'No static_covrun'
            Test-Path -LiteralPath "$script:payload\_coverage-session-id.txt" | Should Be $false
        }

        It 'rejects unchanged DLLs even when the tool succeeds and another module supplied a runtime' {
            $env:WINUI_COVERAGE_TEST_MODE = 'skip-controls'
            { Invoke-Instrumentation } | Should Throw 'Microsoft.UI.Xaml.Controls.dll was not instrumented'
            Test-Path -LiteralPath "$script:payload\app one\static_covrun32.dll" | Should Be $true
            Test-Path -LiteralPath "$script:payload\_coverage-instrumented-Microsoft.UI.Xaml.Controls.dll.txt" | Should Be $false
            Test-Path -LiteralPath "$script:payload\_coverage-session-id.txt" | Should Be $false
        }

        It 'rejects a missing coverage executable' {
            Remove-Item -LiteralPath $script:tool
            { Invoke-Instrumentation } | Should Throw 'Cannot find path'
            @(Get-ToolCalls 'instrument').Count | Should Be 0
        }

        It 'rejects missing Visual Studio dependencies' {
            Remove-Item -LiteralPath "$script:vsRoot\Common7\IDE\CommonExtensions\Platform\InstrumentationEngine\x86" -Recurse
            { Invoke-Instrumentation } | Should Throw 'Required coverage tool directory is missing'
            @(Get-ToolCalls 'instrument').Count | Should Be 0
        }

        It 'rejects a tool outside the Visual Studio layout' {
            $standalone = Join-Path $script:caseRoot 'standalone.exe'
            Copy-Item -LiteralPath $script:compiledTool -Destination $standalone
            $script:tool = $standalone
            { Invoke-Instrumentation } | Should Throw 'Cannot locate the Visual Studio coverage dependencies'
        }

        It 'rejects invalid session IDs before changing the payload' {
            { Invoke-Instrumentation -SessionId 'invalid/session' } | Should Throw 'does not match'
            Test-Path -LiteralPath "$script:payload\CoverageTool" | Should Be $false
            @(Get-ToolCalls 'instrument').Count | Should Be 0
        }
    }

    Describe 'Coverage slice merging' {
        BeforeEach {
            New-CoverageFixture
            $script:inputDir = Join-Path $script:caseRoot 'slices'
            $script:outputDir = Join-Path $script:caseRoot 'merged reports'
            Write-FixtureFile "$script:inputDir\x64\coverage-one.coverage" 'slice one'
            Write-FixtureFile "$script:inputDir\x86\nested\coverage-two.coverage" 'slice two'
            Write-FixtureFile "$script:inputDir\unrelated.coverage" 'ignore'
            Write-FixtureFile "$script:inputDir\coverage-one.coverage.log" 'ignore'
        }

        It 'passes every slice, without logs or unrelated files, to both requested output formats' {
            Invoke-CoverageMerge
            $calls = @(Get-ToolCalls 'merge')
            $calls.Count | Should Be 4
            $expectedInputs = @("$script:inputDir\x64\coverage-one.coverage", "$script:inputDir\x86\nested\coverage-two.coverage")
            foreach ($call in $calls[0..1])
            {
                $call.Arguments.Count | Should Be 6
                ($expectedInputs -contains $call.Arguments[1]) | Should Be $true
                $call.Arguments[5] | Should Be 'cobertura'
            }
            foreach ($call in $calls[2..3])
            {
                $call.Arguments.Count | Should Be 7
                ($call.Arguments[1..2] | Sort-Object) -join '|' | Should Be (($expectedInputs | Sort-Object) -join '|')
                $call.Arguments[3] | Should Be '--output'
                $call.Arguments[5] | Should Be '--output-format'
            }
            $calls[2].Arguments[4] | Should Be "$script:outputDir\merged.cobertura.xml"
            $calls[2].Arguments[6] | Should Be 'cobertura'
            $calls[3].Arguments[4] | Should Be "$script:outputDir\merged.coverage"
            $calls[3].Arguments[6] | Should Be 'coverage'
            [IO.File]::ReadAllText("$script:outputDir\merged.cobertura.xml") | Should Be '<coverage lines-valid="2" />'
            [IO.File]::ReadAllText("$script:outputDir\merged.coverage") | Should Be 'merged-coverage'
            @(Get-ChildItem -LiteralPath $script:outputDir -Directory).Count | Should Be 0
        }

        It 'fails for a missing input directory' {
            $script:inputDir = Join-Path $script:caseRoot 'missing'
            { Invoke-CoverageMerge } | Should Throw 'Cannot find path'
            @(Get-ToolCalls 'merge').Count | Should Be 0
        }

        It 'fails for a directory without matching coverage slices' {
            Get-ChildItem -LiteralPath $script:inputDir -Recurse -Filter 'coverage-*.coverage' | Remove-Item
            { Invoke-CoverageMerge } | Should Throw 'missing or empty'
            @(Get-ToolCalls 'merge').Count | Should Be 0
        }

        It 'fails if any slice is empty even when another is valid' {
            Write-FixtureFile "$script:inputDir\x64\coverage-one.coverage" ''
            { Invoke-CoverageMerge } | Should Throw 'missing or empty'
            @(Get-ToolCalls 'merge').Count | Should Be 0
        }

        It 'rejects corrupt input even when the real CLI behavior is to skip it and return success' {
            Write-FixtureFile "$script:inputDir\x86\nested\coverage-two.coverage" 'invalid'
            Write-FixtureFile "$script:outputDir\merged.cobertura.xml" 'stale'
            Write-FixtureFile "$script:outputDir\merged.coverage" 'stale'
            { Invoke-CoverageMerge } | Should Throw 'invalid or contains no source-line data'
            Test-Path -LiteralPath "$script:outputDir\merged.cobertura.xml" | Should Be $false
            Test-Path -LiteralPath "$script:outputDir\merged.coverage" | Should Be $false
            @(Get-ChildItem -LiteralPath $script:outputDir -Directory).Count | Should Be 0
        }

        It 'fails if the coverage tool is missing' {
            Remove-Item -LiteralPath $script:tool
            { Invoke-CoverageMerge } | Should Throw 'Cannot find path'
            @(Get-ToolCalls 'merge').Count | Should Be 0
        }

        foreach ($format in @('cobertura', 'coverage'))
        {
            It "fails on a nonzero $format merge exit code" {
                $env:WINUI_COVERAGE_TEST_MODE = "fail-$format"
                { Invoke-CoverageMerge } | Should Throw 'exit 24'
            }

            It "fails if a successful $format merge produces no output" {
                $env:WINUI_COVERAGE_TEST_MODE = "no-output-$format"
                { Invoke-CoverageMerge } | Should Throw 'did not produce'
            }

            It "fails if a successful $format merge produces empty output" {
                $env:WINUI_COVERAGE_TEST_MODE = "empty-$format"
                { Invoke-CoverageMerge } | Should Throw 'did not produce'
            }

            It "does not mistake a stale $format report for successful new output" {
                Write-FixtureFile "$script:outputDir\merged.cobertura.xml" 'stale'
                Write-FixtureFile "$script:outputDir\merged.coverage" 'stale'
                $env:WINUI_COVERAGE_TEST_MODE = "no-output-$format"
                { Invoke-CoverageMerge } | Should Throw 'did not produce'
            }
        }
    }

    Describe 'Coverage collection around test execution' {
        BeforeEach {
            New-CoverageFixture
            Write-FixtureFile "$script:payload\CoverageTool\Microsoft.CodeCoverage.Console.exe"
            Copy-Item -LiteralPath $script:compiledTool -Destination "$script:payload\CoverageTool\Microsoft.CodeCoverage.Console.exe" -Force
            Write-FixtureFile "$script:payload\_coverage-session-id.txt" $script:sessionId
            $script:coverageOutput = Join-Path $script:caseRoot 'results with spaces\coverage-slice.coverage'
            $script:collectorAccount = [Security.Principal.SecurityIdentifier]::new('S-1-5-18').Translate([Security.Principal.NTAccount]).Value
            $accountParts = $script:collectorAccount -split '\\', 2
            # Callbacks run inside other scripts, so share state explicitly across script scopes.
            $global:CoverageTestCollector = @{
                TestsRan = 0
                PipeChecks = 0
                ReadyAfter = 1
                DateCalls = 0
                DateStepSeconds = 1
                FailOutputDirectory = $false
                FailSessionRead = $false
                FailOutputCheck = $false
                FailOutputCleanup = $false
                FailConsoleDiscovery = $false
                FailSettingsRead = $false
                StartedArguments = $null
                CoverageOutput = $script:coverageOutput
                SessionId = $script:sessionId
                TestTool = $script:compiledTool
                OwnerDomain = $accountParts[0]
                OwnerUser = $accountParts[1]
                OwnerReturnValue = 0
                ConsoleUser = $script:collectorAccount
                CimProcess = New-CimInstance -ClassName Win32_Process -Property @{ ProcessId = [uint32]$PID } -ClientOnly
            }
            $script:collector = [pscustomobject]@{
                Id = 2147483001
                Handle = [IntPtr]1
                HasExited = $false
                ExitCode = 0
                FinishOnWait = $true
                WaitMilliseconds = 0
                Disposed = $false
            }
            $script:collector | Add-Member -MemberType ScriptMethod -Name WaitForExit -Value {
                param($milliseconds)
                $this.WaitMilliseconds = $milliseconds
                if ($this.FinishOnWait) { $this.HasExited = $true }
                return $this.FinishOnWait
            }
            $script:collector | Add-Member -MemberType ScriptMethod -Name Dispose -Value { $this.Disposed = $true }
            $global:CoverageTestCollector.Process = $script:collector

            Mock Get-CimInstance { $global:CoverageTestCollector.CimProcess } -ParameterFilter { $ClassName -eq 'Win32_Process' }
            Mock Get-CimInstance {
                if ($global:CoverageTestCollector.FailConsoleDiscovery) { throw 'fixture console discovery failure' }
                [pscustomobject]@{ UserName = $global:CoverageTestCollector.ConsoleUser }
            } -ParameterFilter { $ClassName -eq 'Win32_ComputerSystem' }
            Mock Invoke-CimMethod {
                [pscustomobject]@{
                    Domain = $global:CoverageTestCollector.OwnerDomain
                    User = $global:CoverageTestCollector.OwnerUser
                    ReturnValue = $global:CoverageTestCollector.OwnerReturnValue
                }
            } -ParameterFilter { $MethodName -eq 'GetOwner' }

            Mock Start-Process {
                $global:CoverageTestCollector.StartedArguments = $ArgumentList
                $global:CoverageTestCollector.Process
            }
            Mock Stop-Process {
                if ($global:CoverageTestCollector.Process.Id -eq $Id) { $global:CoverageTestCollector.Process.HasExited = $true }
            }
            Mock Start-Sleep {}
            Mock Write-Host {}
            Mock Write-Warning {}
            Mock Get-Date {
                $global:CoverageTestCollector.DateCalls++
                ([datetime]'2026-01-01').AddSeconds($global:CoverageTestCollector.DateCalls * $global:CoverageTestCollector.DateStepSeconds)
            }
            Mock Test-Path {
                $global:CoverageTestCollector.PipeChecks++
                $global:CoverageTestCollector.PipeChecks -ge $global:CoverageTestCollector.ReadyAfter
            } -ParameterFilter { $LiteralPath -like '\\.\pipe\CodeCoverage.pipe.*' }
            Mock New-Item { throw 'fixture output directory failure' } -ParameterFilter {
                $global:CoverageTestCollector.FailOutputDirectory -and
                $Path -eq (Split-Path $global:CoverageTestCollector.CoverageOutput)
            }
            Mock Get-Content { throw 'fixture session read failure' } -ParameterFilter {
                $global:CoverageTestCollector.FailSessionRead -and $LiteralPath -like '*\_coverage-session-id.txt'
            }
            Mock Get-Content { '<not-valid-xml' } -ParameterFilter {
                $global:CoverageTestCollector.FailSettingsRead -and $LiteralPath -like '*\coverage.config'
            }
            Mock Test-Path { throw 'fixture stale output check failure' } -ParameterFilter {
                $global:CoverageTestCollector.FailOutputCheck -and
                $LiteralPath -eq $global:CoverageTestCollector.CoverageOutput
            }
            Mock Remove-Item { throw 'fixture stale output cleanup failure' } -ParameterFilter {
                $global:CoverageTestCollector.FailOutputCleanup -and
                $LiteralPath -eq $global:CoverageTestCollector.CoverageOutput
            }
        }

        It 'starts the collector with quoted paths, waits for its pipe, then invokes tests and shuts down' {
            $global:CoverageTestCollector.ReadyAfter = 3
            Invoke-Collector {
                $global:CoverageTestCollector.TestsRan++
                [xml]$settings = Get-Content -LiteralPath "$($global:CoverageTestCollector.CoverageOutput).config" -Raw
                $settings.Configuration.CodeCoverage.AllowedUsers.User | Should Be "$($global:CoverageTestCollector.OwnerDomain)\$($global:CoverageTestCollector.OwnerUser)"
                Write-FixtureFile $global:CoverageTestCollector.CoverageOutput 'new coverage'
                & $global:CoverageTestCollector.TestTool test 0
            }
            $global:CoverageTestCollector.TestsRan | Should Be 1
            ($global:CoverageTestCollector.StartedArguments -join '|') | Should Be "collect|--session-id|$script:sessionId|--server-mode|--settings|`"$script:coverageOutput.config`"|--output|`"$script:coverageOutput`""
            Assert-MockCalled Get-CimInstance -Times 1 -Exactly -Scope It -ParameterFilter {
                $ClassName -eq 'Win32_Process' -and $Filter -eq "ProcessId = $PID" -and $OperationTimeoutSec -eq 30
            }
            Assert-MockCalled Invoke-CimMethod -Times 1 -Exactly -Scope It -ParameterFilter {
                $MethodName -eq 'GetOwner' -and $InputObject.ProcessId -eq $PID -and $OperationTimeoutSec -eq 30
            }
            Assert-MockCalled Write-Host -Times 1 -Exactly -Scope It -ParameterFilter { "$Object" -like 'Coverage allowed user:*S-1-5-18*' }
            Assert-MockCalled Start-Process -Times 1 -Exactly -Scope It -ParameterFilter {
                $FilePath -eq "$script:payload\CoverageTool\Microsoft.CodeCoverage.Console.exe" -and
                $RedirectStandardOutput -eq "$script:coverageOutput.log" -and
                $RedirectStandardError -eq "$script:coverageOutput.err" -and $PassThru -and $NoNewWindow
            }
            Assert-MockCalled Start-Sleep -Times 2 -Exactly -Scope It
            $calls = @(Get-ToolCalls 'shutdown')
            $calls.Count | Should Be 1
            @(Get-ToolCalls 'test').Count | Should Be 1
            ($calls[0].Arguments -join '|') | Should Be "shutdown|$script:sessionId"
            ($script:collector.WaitMilliseconds -gt 0 -and $script:collector.WaitMilliseconds -le 60000) | Should Be $true
            $script:collector.Disposed | Should Be $true
            [IO.File]::ReadAllText("$script:coverageOutput.shutdown.log") | Should Be 'shutdown completed'
            Assert-MockCalled Stop-Process -Times 0 -Exactly -Scope It
            $LASTEXITCODE | Should Be 0
        }

        It 'allows a distinct console account without changing the instrumentation settings or checked-in config' {
            $consoleAccount = [Security.Principal.SecurityIdentifier]::new('S-1-5-19').Translate([Security.Principal.NTAccount]).Value
            $global:CoverageTestCollector.ConsoleUser = $consoleAccount
            $originalHash = (Get-FileHash -LiteralPath "$script:coverageScripts\coverage.config").Hash
            Invoke-Collector { & $global:CoverageTestCollector.TestTool test 0 }
            [xml]$settings = Get-Content -LiteralPath "$script:coverageOutput.config" -Raw
            $users = @($settings.Configuration.CodeCoverage.AllowedUsers.User)
            $users.Count | Should Be 2
            ($users -contains $script:collectorAccount) | Should Be $true
            ($users -contains $consoleAccount) | Should Be $true
            [void]$settings.Configuration.CodeCoverage.RemoveChild($settings.Configuration.CodeCoverage.AllowedUsers)
            [xml]$original = Get-Content -LiteralPath "$script:coverageScripts\coverage.config" -Raw
            $settings.OuterXml | Should Be $original.OuterXml
            (Get-FileHash -LiteralPath "$script:coverageScripts\coverage.config").Hash | Should Be $originalHash
            Assert-MockCalled Write-Host -Times 1 -Exactly -Scope It -ParameterFilter { "$Object" -like 'Coverage allowed user:*S-1-5-19*' }
            Assert-MockCalled Get-CimInstance -Times 1 -Exactly -Scope It -ParameterFilter {
                $ClassName -eq 'Win32_ComputerSystem' -and $OperationTimeoutSec -eq 30
            }
        }

        It 'does not duplicate the same account with different casing' {
            $global:CoverageTestCollector.ConsoleUser = $script:collectorAccount.ToUpperInvariant()
            Invoke-Collector { & $global:CoverageTestCollector.TestTool test 0 }
            [xml]$settings = Get-Content -LiteralPath "$script:coverageOutput.config" -Raw
            @($settings.Configuration.CodeCoverage.AllowedUsers.User).Count | Should Be 1
            $settings.Configuration.CodeCoverage.AllowedUsers.User | Should Be $script:collectorAccount
        }

        It 'allows only the collector account when no console user is logged in' {
            $global:CoverageTestCollector.ConsoleUser = $null
            Invoke-Collector { & $global:CoverageTestCollector.TestTool test 0 }
            [xml]$settings = Get-Content -LiteralPath "$script:coverageOutput.config" -Raw
            @($settings.Configuration.CodeCoverage.AllowedUsers.User).Count | Should Be 1
            $settings.Configuration.CodeCoverage.AllowedUsers.User | Should Be $script:collectorAccount
            Assert-MockCalled Write-Host -Times 1 -Exactly -Scope It -ParameterFilter { "$Object" -like 'Coverage console account: none*' }
        }

        foreach ($failure in @('return-code', 'missing-user', 'missing-domain'))
        {
            It "does not start a collector if process-owner discovery fails ($failure)" {
                if ($failure -eq 'return-code') { $global:CoverageTestCollector.OwnerReturnValue = 2 }
                if ($failure -eq 'missing-user') { $global:CoverageTestCollector.OwnerUser = '' }
                if ($failure -eq 'missing-domain') { $global:CoverageTestCollector.OwnerDomain = '' }
                Invoke-Collector { $global:CoverageTestCollector.TestsRan++; & $global:CoverageTestCollector.TestTool test 37 }
                $global:CoverageTestCollector.TestsRan | Should Be 1
                $LASTEXITCODE | Should Be 37
                Assert-MockCalled Start-Process -Times 0 -Exactly -Scope It
                Assert-MockCalled Write-Host -Times 1 -Scope It -ParameterFilter { "$Object" -like '*Cannot determine the coverage collector account*' }
            }
        }

        It 'reports failed console discovery without starting a collector with default permissions' {
            $global:CoverageTestCollector.FailConsoleDiscovery = $true
            Invoke-Collector { & $global:CoverageTestCollector.TestTool test 37 }
            $LASTEXITCODE | Should Be 37
            Assert-MockCalled Start-Process -Times 0 -Exactly -Scope It
            Assert-MockCalled Write-Host -Times 1 -Scope It -ParameterFilter { "$Object" -like '*Coverage setup failed:*fixture console discovery failure*' }
        }

        It 'reports failed settings creation without starting a collector with default permissions' {
            $global:CoverageTestCollector.FailSettingsRead = $true
            Invoke-Collector { & $global:CoverageTestCollector.TestTool test 37 }
            $LASTEXITCODE | Should Be 37
            Assert-MockCalled Start-Process -Times 0 -Exactly -Scope It
            Assert-MockCalled Write-Host -Times 1 -Scope It -ParameterFilter { "$Object" -like '*Coverage setup failed:*' }
        }

        It 'fails before tests without opening, changing, or shutting down a real existing session pipe' {
            $script:sessionId = 'coverage-existing-' + [guid]::NewGuid().ToString('N')
            Write-FixtureFile "$script:payload\_coverage-session-id.txt" $script:sessionId
            Write-FixtureFile $script:coverageOutput 'stale'
            $existingPipe = [IO.Pipes.NamedPipeServerStream]::new("CodeCoverage.pipe.$script:sessionId")
            try
            {
                { Invoke-Collector { $global:CoverageTestCollector.TestsRan++ } } | Should Throw "Coverage session '$script:sessionId' already exists"
                $global:CoverageTestCollector.TestsRan | Should Be 0
                Test-Path -LiteralPath $script:coverageOutput | Should Be $false
                $existingPipe.SafePipeHandle.IsClosed | Should Be $false
                $existingPipe.IsConnected | Should Be $false
                Assert-MockCalled Start-Process -Times 0 -Exactly -Scope It
                Assert-MockCalled Stop-Process -Times 0 -Exactly -Scope It
                @(Get-ToolCalls 'shutdown').Count | Should Be 0
                Assert-MockCalled Get-CimInstance -Times 0 -Exactly -Scope It
                $script:collector.Disposed | Should Be $false
                Assert-MockCalled Write-Host -Times 0 -Exactly -Scope It -ParameterFilter { "$Object" -like '*Coverage setup failed*' }
                $client = [IO.Pipes.NamedPipeClientStream]::new('.', "CodeCoverage.pipe.$script:sessionId")
                try
                {
                    $client.Connect(1000)
                    $client.IsConnected | Should Be $true
                }
                finally { $client.Dispose() }
            }
            finally { $existingPipe.Dispose() }
        }

        It 'does not mistake another session with the same prefix for a collision' {
            $script:sessionId = 'coverage-existing-' + [guid]::NewGuid().ToString('N')
            Write-FixtureFile "$script:payload\_coverage-session-id.txt" $script:sessionId
            $existingPipe = [IO.Pipes.NamedPipeServerStream]::new("CodeCoverage.pipe.$script:sessionId-other")
            try
            {
                Invoke-Collector {
                    $global:CoverageTestCollector.TestsRan++
                    Write-FixtureFile $global:CoverageTestCollector.CoverageOutput 'coverage'
                    & $global:CoverageTestCollector.TestTool test 0
                }
                $global:CoverageTestCollector.TestsRan | Should Be 1
                Assert-MockCalled Start-Process -Times 1 -Exactly -Scope It
                [IO.Directory]::GetFiles('\\.\pipe\', "CodeCoverage.pipe.$script:sessionId-other").Length | Should Be 1
            }
            finally { $existingPipe.Dispose() }
        }

        It 'removes stale output before tests instead of accepting it as new coverage' {
            Write-FixtureFile $script:coverageOutput 'stale'
            Invoke-Collector {
                $global:CoverageTestCollector.TestsRan++
                Test-Path -LiteralPath $global:CoverageTestCollector.CoverageOutput | Should Be $false
                & $global:CoverageTestCollector.TestTool test 0
            }
            $global:CoverageTestCollector.TestsRan | Should Be 1
            Assert-MockCalled Write-Host -Times 1 -Scope It -ParameterFilter { "$Object" -like '*No coverage data was produced*' }
        }

        It 'still invokes tests and preserves their native failure when the session file is absent' {
            Write-FixtureFile $script:coverageOutput 'stale'
            Remove-Item -LiteralPath "$script:payload\_coverage-session-id.txt"
            Invoke-Collector {
                $global:CoverageTestCollector.TestsRan++
                Test-Path -LiteralPath $global:CoverageTestCollector.CoverageOutput | Should Be $false
                & $global:CoverageTestCollector.TestTool test 37
            }
            $global:CoverageTestCollector.TestsRan | Should Be 1
            $LASTEXITCODE | Should Be 37
            Assert-MockCalled Start-Process -Times 0 -Exactly -Scope It
            @(Get-ToolCalls 'shutdown').Count | Should Be 0
        }

        It 'still invokes tests when the session ID is invalid' {
            Write-FixtureFile $script:coverageOutput 'stale'
            Write-FixtureFile "$script:payload\_coverage-session-id.txt" 'invalid/session'
            Invoke-Collector { $global:CoverageTestCollector.TestsRan++; & $global:CoverageTestCollector.TestTool test 0 }
            $global:CoverageTestCollector.TestsRan | Should Be 1
            Test-Path -LiteralPath $script:coverageOutput | Should Be $false
            Assert-MockCalled Start-Process -Times 0 -Exactly -Scope It
            Assert-MockCalled Get-CimInstance -Times 0 -Exactly -Scope It
        }

        It 'clears stale output even when the session file cannot be read' {
            Write-FixtureFile $script:coverageOutput 'stale'
            Write-FixtureFile "$script:coverageOutput.config" 'stale settings'
            $global:CoverageTestCollector.FailSessionRead = $true
            Invoke-Collector { $global:CoverageTestCollector.TestsRan++; & $global:CoverageTestCollector.TestTool test 37 }
            $global:CoverageTestCollector.TestsRan | Should Be 1
            $LASTEXITCODE | Should Be 37
            Test-Path -LiteralPath $script:coverageOutput | Should Be $false
            Test-Path -LiteralPath "$script:coverageOutput.config" | Should Be $false
            Assert-MockCalled Start-Process -Times 0 -Exactly -Scope It
            Assert-MockCalled Write-Host -Times 1 -Scope It -ParameterFilter { "$Object" -like '*Coverage setup failed:*fixture session read failure*' }
        }

        foreach ($sessionPresent in @($true, $false))
        {
            It "stops before tests and suppresses the artifact when stale output cannot be removed (session present: $sessionPresent)" {
                Write-FixtureFile $script:coverageOutput 'stale'
                if (-not $sessionPresent) { Remove-Item -LiteralPath "$script:payload\_coverage-session-id.txt" }
                $global:CoverageTestCollector.FailOutputCleanup = $true
                { Invoke-Collector { $global:CoverageTestCollector.TestsRan++ } } | Should Throw 'Cannot clear previous coverage output'
                $global:CoverageTestCollector.TestsRan | Should Be 0
                [IO.File]::ReadAllText($script:coverageOutput) | Should Be 'stale'
                Assert-MockCalled Start-Process -Times 0 -Exactly -Scope It
                Assert-MockCalled Write-Host -Times 1 -Exactly -Scope It -ParameterFilter { "$Object" -eq '##vso[task.setvariable variable=skipPublish]true' }
                @(Get-ToolCalls 'shutdown').Count | Should Be 0
                Assert-MockCalled Get-CimInstance -Times 0 -Exactly -Scope It
            }
        }

        It 'stops before tests and suppresses the artifact when previous output cannot be checked' {
            Write-FixtureFile $script:coverageOutput 'stale'
            $global:CoverageTestCollector.FailOutputCheck = $true
            { Invoke-Collector { $global:CoverageTestCollector.TestsRan++ } } | Should Throw 'fixture stale output check failure'
            $global:CoverageTestCollector.TestsRan | Should Be 0
            Assert-MockCalled Start-Process -Times 0 -Exactly -Scope It
            Assert-MockCalled Write-Host -Times 1 -Exactly -Scope It -ParameterFilter { "$Object" -eq '##vso[task.setvariable variable=skipPublish]true' }
        }

        It 'suppresses publication when a real file lock prevents stale output cleanup' {
            Write-FixtureFile $script:coverageOutput 'stale'
            $lockedFile = [IO.File]::Open($script:coverageOutput, [IO.FileMode]::Open, [IO.FileAccess]::Read, [IO.FileShare]::None)
            try
            {
                { Invoke-Collector { $global:CoverageTestCollector.TestsRan++ } } | Should Throw 'Cannot clear previous coverage output'
                $global:CoverageTestCollector.TestsRan | Should Be 0
                Assert-MockCalled Start-Process -Times 0 -Exactly -Scope It
                Assert-MockCalled Write-Host -Times 1 -Exactly -Scope It -ParameterFilter { "$Object" -eq '##vso[task.setvariable variable=skipPublish]true' }
            }
            finally
            {
                $lockedFile.Dispose()
            }
            [IO.File]::ReadAllText($script:coverageOutput) | Should Be 'stale'
        }

        It 'suppresses publication when a real file lock prevents stale settings cleanup' {
            Write-FixtureFile "$script:coverageOutput.config" 'stale settings'
            $lockedFile = [IO.File]::Open("$script:coverageOutput.config", [IO.FileMode]::Open, [IO.FileAccess]::Read, [IO.FileShare]::None)
            try
            {
                { Invoke-Collector { $global:CoverageTestCollector.TestsRan++ } } | Should Throw 'Cannot clear previous coverage output'
                $global:CoverageTestCollector.TestsRan | Should Be 0
                Assert-MockCalled Start-Process -Times 0 -Exactly -Scope It
                Assert-MockCalled Write-Host -Times 1 -Exactly -Scope It -ParameterFilter { "$Object" -eq '##vso[task.setvariable variable=skipPublish]true' }
            }
            finally { $lockedFile.Dispose() }
        }

        It 'still invokes tests when the output directory cannot be created' {
            $global:CoverageTestCollector.FailOutputDirectory = $true
            Invoke-Collector { $global:CoverageTestCollector.TestsRan++; & $global:CoverageTestCollector.TestTool test 37 }
            $global:CoverageTestCollector.TestsRan | Should Be 1
            $LASTEXITCODE | Should Be 37
            Assert-MockCalled Start-Process -Times 0 -Exactly -Scope It
            Assert-MockCalled Write-Host -Times 1 -Scope It -ParameterFilter { "$Object" -like '*fixture output directory failure*' }
            @(Get-ToolCalls 'shutdown').Count | Should Be 0
        }

        It 'still invokes tests when the collector cannot start' {
            Mock Start-Process { throw 'fixture missing executable' }
            Invoke-Collector { $global:CoverageTestCollector.TestsRan++; & $global:CoverageTestCollector.TestTool test 37 }
            $global:CoverageTestCollector.TestsRan | Should Be 1
            $LASTEXITCODE | Should Be 37
            Assert-MockCalled Write-Host -Times 1 -Scope It -ParameterFilter { "$Object" -like '*Coverage setup failed:*fixture missing executable*' }
            @(Get-ToolCalls 'shutdown').Count | Should Be 0
        }

        It 'still invokes tests when the collector exits before its pipe appears' {
            $global:CoverageTestCollector.ReadyAfter = [int]::MaxValue
            $script:collector.ExitCode = 28
            $script:collector.HasExited = $true
            Invoke-Collector { $global:CoverageTestCollector.TestsRan++; & $global:CoverageTestCollector.TestTool test 37 }
            $global:CoverageTestCollector.TestsRan | Should Be 1
            $LASTEXITCODE | Should Be 37
            Assert-MockCalled Write-Host -Times 1 -Scope It -ParameterFilter { "$Object" -like '*collector exited with code 28*' }
            Assert-MockCalled Stop-Process -Times 0 -Exactly -Scope It
            @(Get-ToolCalls 'shutdown').Count | Should Be 0
        }

        It 'does not shut down a ready pipe after the launched collector has exited' {
            $script:collector.HasExited = $true
            Invoke-Collector { $global:CoverageTestCollector.TestsRan++; & $global:CoverageTestCollector.TestTool test 37 }
            $global:CoverageTestCollector.TestsRan | Should Be 1
            $LASTEXITCODE | Should Be 37
            @(Get-ToolCalls 'shutdown').Count | Should Be 0
            Assert-MockCalled Stop-Process -Times 0 -Exactly -Scope It
            $script:collector.Disposed | Should Be $true
        }

        It 'still invokes tests and cleans up after pipe readiness times out' {
            $global:CoverageTestCollector.ReadyAfter = [int]::MaxValue
            $global:CoverageTestCollector.DateStepSeconds = 31
            Invoke-Collector { $global:CoverageTestCollector.TestsRan++; & $global:CoverageTestCollector.TestTool test 37 }
            $global:CoverageTestCollector.TestsRan | Should Be 1
            $LASTEXITCODE | Should Be 37
            @(Get-ToolCalls 'shutdown').Count | Should Be 1
            Assert-MockCalled Write-Host -Times 1 -Scope It -ParameterFilter { "$Object" -like '*pipe did not appear within 30 seconds*' }
        }

        It 'does not send shutdown by session name if its collector exits during tests' {
            Invoke-Collector {
                $global:CoverageTestCollector.Process.HasExited = $true
                & $global:CoverageTestCollector.TestTool test 37
            }
            $LASTEXITCODE | Should Be 37
            @(Get-ToolCalls 'shutdown').Count | Should Be 0
            Assert-MockCalled Stop-Process -Times 0 -Exactly -Scope It
            $script:collector.Disposed | Should Be $true
        }

        It 'does not start a collector with an unresolvable account and preserves the test result' {
            $global:CoverageTestCollector.ConsoleUser = 'missing-coverage-user-' + [guid]::NewGuid().ToString('N')
            Invoke-Collector { $global:CoverageTestCollector.TestsRan++; & $global:CoverageTestCollector.TestTool test 37 }
            $global:CoverageTestCollector.TestsRan | Should Be 1
            $LASTEXITCODE | Should Be 37
            Assert-MockCalled Start-Process -Times 0 -Exactly -Scope It
            @(Get-ToolCalls 'shutdown').Count | Should Be 0
            Assert-MockCalled Write-Host -Times 1 -Scope It -ParameterFilter { "$Object" -like '*Coverage setup failed:*Cannot resolve coverage account*' }
        }

        It 'preserves a native failure through nested RunTests callbacks and successful shutdown' {
            Invoke-Collector {
                & {
                    Write-FixtureFile $global:CoverageTestCollector.CoverageOutput 'coverage'
                    & $global:CoverageTestCollector.TestTool test 37
                }
            }
            $LASTEXITCODE | Should Be 37
            ($script:collector.WaitMilliseconds -gt 0 -and $script:collector.WaitMilliseconds -le 60000) | Should Be $true
        }

        It 'preserves the original test exception and runs shutdown and forced cleanup' {
            $script:collector.FinishOnWait = $false
            { Invoke-Collector { throw 'original test exception' } } | Should Throw 'original test exception'
            @(Get-ToolCalls 'shutdown').Count | Should Be 1
            ($script:collector.WaitMilliseconds -gt 0 -and $script:collector.WaitMilliseconds -le 60000) | Should Be $true
            Assert-MockCalled Stop-Process -Times 1 -Exactly -Scope It -ParameterFilter { $Id -eq 2147483001 }
        }

        It 'retains a fast real shutdown exit code without a spurious failure warning' {
            Invoke-Collector {
                Write-FixtureFile $global:CoverageTestCollector.CoverageOutput 'coverage'
                & $global:CoverageTestCollector.TestTool test 0
            }
            $LASTEXITCODE | Should Be 0
            Assert-MockCalled Write-Host -Times 0 -Exactly -Scope It -ParameterFilter { "$Object" -like '*Coverage collection failed*' }
            Assert-MockCalled Stop-Process -Times 0 -Exactly -Scope It
            [IO.File]::ReadAllText("$script:coverageOutput.shutdown.log") | Should Be 'shutdown completed'
            [IO.File]::ReadAllText("$script:coverageOutput.shutdown.err") | Should Be ''
        }

        foreach ($finalExitCode in @(28, $null))
        {
            foreach ($nativeExitCode in @(0, 37))
            {
                It "warns for final collector exit '$finalExitCode' with nonempty output and preserves test exit $nativeExitCode" {
                    $script:collector.ExitCode = $finalExitCode
                    $global:CoverageTestCollector.NativeExitCode = $nativeExitCode
                    Invoke-Collector {
                        Write-FixtureFile $global:CoverageTestCollector.CoverageOutput 'coverage'
                        & $global:CoverageTestCollector.TestTool test $global:CoverageTestCollector.NativeExitCode
                    }
                    $LASTEXITCODE | Should Be $nativeExitCode
                    [IO.File]::ReadAllText($script:coverageOutput) | Should Be 'coverage'
                    @(Get-ToolCalls 'shutdown').Count | Should Be 1
                    [IO.File]::ReadAllText("$script:coverageOutput.shutdown.log") | Should Be 'shutdown completed'
                    if ($null -eq $finalExitCode)
                    {
                        Assert-MockCalled Write-Host -Times 1 -Exactly -Scope It -ParameterFilter {
                            "$Object" -like '*Coverage collection failed:*collector exit code is unavailable after shutdown*'
                        }
                    }
                    else
                    {
                        Assert-MockCalled Write-Host -Times 1 -Exactly -Scope It -ParameterFilter {
                            "$Object" -like '*Coverage collection failed:*collector failed (exit 28)*'
                        }
                    }
                    Assert-MockCalled Stop-Process -Times 0 -Exactly -Scope It
                    $script:collector.Disposed | Should Be $true
                }
            }
        }

        foreach ($collectorMode in @('native-smoke', 'fail-native-collector'))
        {
            It "reads the real collector exit after successful shutdown with nonempty output ($collectorMode)" {
                $env:WINUI_COVERAGE_TEST_MODE = $collectorMode
                Write-FixtureFile "$script:payload\_coverage-session-id.txt" ('coverage-final-exit-' + [guid]::NewGuid().ToString('N'))
                $global:CoverageTestCollector.StartCommand = Get-Command Start-Process -CommandType Cmdlet
                $global:CoverageTestCollector.StopCommand = Get-Command Stop-Process -CommandType Cmdlet
                $global:CoverageTestCollector.OwnedCollectorId = $null
                Mock Start-Process {
                    $process = & $global:CoverageTestCollector.StartCommand -FilePath $FilePath -ArgumentList $ArgumentList `
                        -PassThru -NoNewWindow -RedirectStandardOutput $RedirectStandardOutput -RedirectStandardError $RedirectStandardError
                    $global:CoverageTestCollector.OwnedCollectorId = $process.Id
                    return $process
                }
                Mock Get-Date { [datetime]::Now }
                Mock Start-Sleep { [Threading.Thread]::Sleep($Milliseconds) }
                Mock Test-Path {
                    [IO.Directory]::GetFiles('\\.\pipe\', [IO.Path]::GetFileName($LiteralPath)).Length -gt 0
                } -ParameterFilter { $LiteralPath -like '\\.\pipe\CodeCoverage.pipe.*' }
                try
                {
                    Invoke-Collector { & $global:CoverageTestCollector.TestTool test 0 }
                    $LASTEXITCODE | Should Be 0
                    [IO.File]::ReadAllText($script:coverageOutput) | Should Be 'coverage'
                    @(Get-ToolCalls 'shutdown').Count | Should Be 1
                    [IO.File]::ReadAllText("$script:coverageOutput.shutdown.log") | Should Be 'shutdown completed'
                    [IO.File]::ReadAllText("$script:coverageOutput.shutdown.err") | Should Be ''
                    Assert-MockCalled Write-Host -Times 0 -Exactly -Scope It -ParameterFilter {
                        "$Object" -like '*Coverage setup failed*'
                    }
                    if ($collectorMode -eq 'fail-native-collector')
                    {
                        Assert-MockCalled Write-Host -Times 1 -Exactly -Scope It -ParameterFilter {
                            "$Object" -like '*Coverage collection failed:*collector failed (exit 28)*'
                        }
                    }
                    else
                    {
                        Assert-MockCalled Write-Host -Times 0 -Exactly -Scope It -ParameterFilter {
                            "$Object" -like '*Coverage collection failed*'
                        }
                    }
                    Assert-MockCalled Stop-Process -Times 0 -Exactly -Scope It
                    foreach ($name in @('collector', 'shutdown'))
                    {
                        Assert-FixtureProcessExited ([int](Get-Content -LiteralPath "$script:caseRoot\$name.pid"))
                    }
                }
                finally
                {
                    $ownedIds = @($global:CoverageTestCollector.OwnedCollectorId)
                    $shutdownPidFile = Join-Path $script:caseRoot 'shutdown.pid'
                    if (Test-Path -LiteralPath $shutdownPidFile)
                    {
                        $ownedIds += [int](Get-Content -LiteralPath $shutdownPidFile)
                    }
                    foreach ($ownedId in $ownedIds)
                    {
                        if ($null -ne $ownedId)
                        {
                            $process = Get-Process -Id $ownedId -ErrorAction SilentlyContinue
                            if ($process)
                            {
                                try
                                {
                                    & $global:CoverageTestCollector.StopCommand -Id $ownedId -ErrorAction Continue
                                    [void]$process.WaitForExit(5000)
                                }
                                finally { $process.Dispose() }
                            }
                        }
                    }
                }
            }
        }

        It 'retains a fast failing shutdown exit code and captures its error log' {
            $env:WINUI_COVERAGE_TEST_MODE = 'fail-shutdown'
            Invoke-Collector { & $global:CoverageTestCollector.TestTool test 37 }
            $LASTEXITCODE | Should Be 37
            [IO.File]::ReadAllText("$script:coverageOutput.shutdown.err") | Should Be 'fixture shutdown failure'
            Assert-MockCalled Write-Host -Times 1 -Scope It -ParameterFilter { "$Object" -like '*shutdown failed (exit 26)*' }
            Assert-MockCalled Stop-Process -Times 1 -Exactly -Scope It -ParameterFilter { $Id -eq 2147483001 }
        }

        It 'drains both shutdown streams while waiting so full pipe buffers cannot deadlock it' {
            $env:WINUI_COVERAGE_TEST_MODE = 'verbose-shutdown'
            Invoke-Collector {
                Write-FixtureFile $global:CoverageTestCollector.CoverageOutput 'coverage'
                & $global:CoverageTestCollector.TestTool test 0
            }
            (Get-Item -LiteralPath "$script:coverageOutput.shutdown.log").Length | Should Be 262144
            (Get-Item -LiteralPath "$script:coverageOutput.shutdown.err").Length | Should Be 262144
            Assert-MockCalled Write-Host -Times 0 -Exactly -Scope It -ParameterFilter { "$Object" -like '*Coverage collection failed*' }
        }

        It 'cleans up and preserves the test exit code when the shutdown executable disappears' {
            $global:CoverageTestCollector.Payload = $script:payload
            Invoke-Collector {
                Remove-Item -LiteralPath "$($global:CoverageTestCollector.Payload)\CoverageTool\Microsoft.CodeCoverage.Console.exe"
                & $global:CoverageTestCollector.TestTool test 37
            }
            $LASTEXITCODE | Should Be 37
            Assert-MockCalled Write-Host -Times 1 -Scope It -ParameterFilter { "$Object" -like '*Coverage collection failed*' }
            Assert-MockCalled Stop-Process -Times 1 -Exactly -Scope It -ParameterFilter { $Id -eq 2147483001 }
            $script:collector.Disposed | Should Be $true
        }

        It 'reports log-write failures without replacing passing tests' {
            Invoke-Collector {
                Write-FixtureFile $global:CoverageTestCollector.CoverageOutput 'coverage'
                New-Item -ItemType Directory -Path "$($global:CoverageTestCollector.CoverageOutput).shutdown.log" | Out-Null
                & $global:CoverageTestCollector.TestTool test 0
            }
            $LASTEXITCODE | Should Be 0
            Assert-MockCalled Write-Host -Times 1 -Scope It -ParameterFilter { "$Object" -like '*Could not save coverage shutdown logs*' }
            $script:collector.Disposed | Should Be $true
        }

        foreach ($outcome in @('pass', 'fail', 'throw'))
        {
            It "terminates real owned processes after a stalled pipe reply and preserves $outcome" {
                $env:WINUI_COVERAGE_TEST_MODE = 'stall-shutdown'
                Write-FixtureFile "$script:payload\_coverage-session-id.txt" ('coverage-test-' + [guid]::NewGuid().ToString('N'))
                $global:CoverageTestCollector.OwnedProcesses = @()
                $global:CoverageTestCollector.Outcome = $outcome
                $global:CoverageTestCollector.StartCommand = Get-Command Start-Process -CommandType Cmdlet
                $global:CoverageTestCollector.StopCommand = Get-Command Stop-Process -CommandType Cmdlet
                Mock Start-Process {
                    $global:CoverageTestCollector.StartedArguments = $ArgumentList
                    $process = & $global:CoverageTestCollector.StartCommand -FilePath $FilePath -ArgumentList $ArgumentList `
                        -PassThru -NoNewWindow -RedirectStandardOutput $RedirectStandardOutput -RedirectStandardError $RedirectStandardError
                    $global:CoverageTestCollector.OwnedProcesses += $process.Id
                    return $process
                }
                Mock Stop-Process {
                    $shutdownPidPath = Join-Path $env:WINUI_COVERAGE_TEST_ROOT 'shutdown.pid'
                    if (Test-Path -LiteralPath $shutdownPidPath)
                    {
                        $global:CoverageTestCollector.OwnedProcesses += [int](Get-Content -LiteralPath $shutdownPidPath)
                        $global:CoverageTestCollector.OwnedProcesses = @($global:CoverageTestCollector.OwnedProcesses | Select-Object -Unique)
                    }
                    foreach ($processId in $Id)
                    {
                        ($global:CoverageTestCollector.OwnedProcesses -contains $processId) | Should Be $true
                    }
                    & $global:CoverageTestCollector.StopCommand -Id $Id -ErrorAction Stop
                }
                $run = {
                    if ($global:CoverageTestCollector.Outcome -eq 'throw') { throw 'original native test exception' }
                    $code = if ($global:CoverageTestCollector.Outcome -eq 'fail') { 37 } else { 0 }
                    & $global:CoverageTestCollector.TestTool test $code
                }
                $clock = [Diagnostics.Stopwatch]::StartNew()
                try
                {
                    if ($outcome -eq 'throw')
                    {
                        { & "$script:coverageScripts\Invoke-WithCodeCoverage.ps1" -PayloadDir $script:payload `
                            -OutputFile $script:coverageOutput -RunTests $run -ShutdownTimeoutSeconds 2 } | Should Throw 'original native test exception'
                    }
                    else
                    {
                        & "$script:coverageScripts\Invoke-WithCodeCoverage.ps1" -PayloadDir $script:payload `
                            -OutputFile $script:coverageOutput -RunTests $run -ShutdownTimeoutSeconds 2
                        $LASTEXITCODE | Should Be $(if ($outcome -eq 'fail') { 37 } else { 0 })
                    }
                    $clock.Elapsed.TotalSeconds | Should BeLessThan 15
                    Test-Path -LiteralPath "$script:caseRoot\shutdown-requested.txt" | Should Be $true
                    $global:CoverageTestCollector.OwnedProcesses.Count | Should Be 2
                    foreach ($processId in $global:CoverageTestCollector.OwnedProcesses)
                    {
                        Assert-FixtureProcessExited $processId
                    }
                }
                finally
                {
                    foreach ($processId in $global:CoverageTestCollector.OwnedProcesses)
                    {
                        $process = Get-Process -Id $processId -ErrorAction SilentlyContinue
                        if ($process) { & $global:CoverageTestCollector.StopCommand -Id $processId }
                    }
                }
            }
        }

        It 'preserves a native test exit code even when the test script then throws' {
            { Invoke-Collector { & $global:CoverageTestCollector.TestTool test 37; throw 'test failure after native failure' } } | Should Throw 'test failure after native failure'
            $LASTEXITCODE | Should Be 37
            @(Get-ToolCalls 'shutdown').Count | Should Be 1
        }

        It 'preserves the original test exception when shutdown also fails' {
            $env:WINUI_COVERAGE_TEST_MODE = 'fail-shutdown'
            { Invoke-Collector { throw 'original test exception' } } | Should Throw 'original test exception'
            Assert-MockCalled Stop-Process -Times 1 -Exactly -Scope It -ParameterFilter { $Id -eq 2147483001 }
        }

        It 'does not replace passing tests with a failed shutdown exit code' {
            $env:WINUI_COVERAGE_TEST_MODE = 'fail-shutdown'
            Invoke-Collector { & $global:CoverageTestCollector.TestTool test 0 }
            $LASTEXITCODE | Should Be 0
            Assert-MockCalled Write-Host -Times 1 -Scope It -ParameterFilter { "$Object" -like '*shutdown failed (exit 26)*' }
            Assert-MockCalled Stop-Process -Times 1 -Exactly -Scope It -ParameterFilter { $Id -eq 2147483001 }
        }

        It 'does not replace failing tests with a different failed shutdown exit code' {
            $env:WINUI_COVERAGE_TEST_MODE = 'fail-shutdown'
            Invoke-Collector { & $global:CoverageTestCollector.TestTool test 37 }
            $LASTEXITCODE | Should Be 37
            Assert-MockCalled Stop-Process -Times 1 -Exactly -Scope It -ParameterFilter { $Id -eq 2147483001 }
        }

        It 'reports empty coverage without changing the test result' {
            Invoke-Collector { Write-FixtureFile $global:CoverageTestCollector.CoverageOutput ''; & $global:CoverageTestCollector.TestTool test 0 }
            $LASTEXITCODE | Should Be 0
            Assert-MockCalled Write-Host -Times 1 -Scope It -ParameterFilter { "$Object" -like '*No coverage data was produced*' }
        }
    }

    Describe 'Native smoke collector lifecycle' {
        $ast = [Management.Automation.Language.Parser]::ParseFile("$PSScriptRoot\Run-NativeSmoke.ps1", [ref]$null, [ref]$null)
        $sliceFunction = $ast.Find({
            param($node)
            $node -is [Management.Automation.Language.FunctionDefinitionAst] -and $node.Name -eq 'Invoke-NativeSlice'
        }, $false)
        # Exercise the smoke helper without compiling native DLLs or running its entry point.
        . ([scriptblock]::Create($sliceFunction.Extent.Text))

        BeforeEach {
            New-CoverageFixture
            $script:scripts = $script:coverageScripts
            $script:slices = Join-Path $script:caseRoot 'slices'
            $script:apps = @('app one', 'app two')
            $script:sessionId = 'coverage-smoke-test-' + [guid]::NewGuid().ToString('N')
            $script:ShutdownTimeoutSeconds = 2
            New-Item -ItemType Directory -Path $script:slices | Out-Null
            Copy-Item -LiteralPath $script:compiledTool -Destination "$script:payload\app one\CoverageRunner.exe"
            $env:WINUI_COVERAGE_TEST_MODE = 'native-smoke'
        }

        AfterEach {
            foreach ($name in @('collector', 'shutdown'))
            {
                $pidFile = Join-Path $script:caseRoot "$name.pid"
                if (Test-Path -LiteralPath $pidFile)
                {
                    $process = Get-Process -Id ([int](Get-Content -LiteralPath $pidFile)) -ErrorAction SilentlyContinue
                    if ($process)
                    {
                        try
                        {
                            Stop-Process -Id $process.Id -ErrorAction Continue
                            $process.WaitForExit(5000) | Should Be $true
                        }
                        finally { $process.Dispose() }
                    }
                }
            }
        }

        It 'collects a fixture slice and retains the real shutdown exit code' {
            Invoke-NativeSlice 0
            [IO.File]::ReadAllText("$script:slices\coverage-0.coverage") | Should Be 'coverage'
            @(Get-ToolCalls 'shutdown').Count | Should Be 1
            foreach ($name in @('collector', 'shutdown'))
            {
                Assert-FixtureProcessExited ([int](Get-Content -LiteralPath "$script:caseRoot\$name.pid"))
            }
        }

        foreach ($mode in @('stall-shutdown', 'stall-collector'))
        {
            It "bounds $mode and terminates both owned processes without retrying shutdown" {
                $env:WINUI_COVERAGE_TEST_MODE = $mode
                $clock = [Diagnostics.Stopwatch]::StartNew()
                { Invoke-NativeSlice 0 } | Should Throw 'within 2 seconds'
                $clock.Elapsed.TotalSeconds | Should BeLessThan 15
                @(Get-ToolCalls 'collect').Count | Should Be 1
                @(Get-ToolCalls 'shutdown').Count | Should Be 1
                Test-Path -LiteralPath "$script:caseRoot\shutdown-requested.txt" | Should Be $true
                foreach ($name in @('collector', 'shutdown'))
                {
                    Assert-FixtureProcessExited ([int](Get-Content -LiteralPath "$script:caseRoot\$name.pid"))
                }
            }
        }

        It 'preserves runner failure and terminates its collector without starting shutdown' {
            $env:WINUI_COVERAGE_TEST_MODE = 'fail-native-runner'
            { Invoke-NativeSlice 0 } | Should Throw 'Native fixture failed with exit code 37'
            @(Get-ToolCalls 'shutdown').Count | Should Be 0
            Assert-FixtureProcessExited ([int](Get-Content -LiteralPath "$script:caseRoot\collector.pid"))
        }

        It 'reports the real failing shutdown exit code without retrying shutdown' {
            $env:WINUI_COVERAGE_TEST_MODE = 'fail-native-shutdown'
            { Invoke-NativeSlice 0 } | Should Throw 'Native fixture shutdown failed with exit code 26'
            @(Get-ToolCalls 'shutdown').Count | Should Be 1
            foreach ($name in @('collector', 'shutdown'))
            {
                Assert-FixtureProcessExited ([int](Get-Content -LiteralPath "$script:caseRoot\$name.pid"))
            }
        }
    }

    Describe 'Coverage merge retry gating' {
        BeforeEach {
            $script:mergeTemplate = Get-Content -LiteralPath "$PSScriptRoot\..\..\..\..\..\build\AzurePipelinesTemplates\WinUI-MergeCodeCoverage-Job.yml" -Raw
        }

        It 'requires successful test dependencies before merging' {
            $script:mergeTemplate | Should Match '(?m)^  dependsOn: \$\{\{ parameters\.dependsOn \}\}\r?$'
            $script:mergeTemplate | Should Match '(?m)^  condition: succeeded\(\)\r?$'
        }

        It 'downloads only successful slice reports from the current run' {
            $download = [regex]::Match($script:mergeTemplate, '(?ms)^  - task: DownloadPipelineArtifact@2\r?\n.*?(?=^  - task:|\z)')
            $download.Success | Should Be $true
            $download.Value | Should Match '(?m)^      buildType: current\r?$'
            $download.Value | Should Match '(?m)^      itemPattern: ''\*_Succeeded/\*\*/coverage-\*\.coverage''\r?$'
            $download.Value | Should Not Match '(?m)^      (artifactName|artifact):'
        }
    }

    Describe 'Coverage artifact publication' {
        It 'preserves cleanup-failure suppression in coverage jobs without changing the coverage-off probe condition' {
            $template = Get-Content -LiteralPath "$PSScriptRoot\..\..\..\..\..\build\AzurePipelinesTemplates\WinUI-RunTestPassOnPipeline-Job.yml" -Raw
            $probe = [regex]::Match($template, '(?ms)displayName: Check whether test-output artifact already exists.*?(?=  - task: PublishPipelineArtifact@1)')
            $probe.Success | Should Be $true
            $probe.Value | Should Match '(?m)^    \$\{\{ if eq\(parameters.collectCodeCoverage, true\) \}\}:\r?\n(?:      #[^\r\n]*\r?\n)*      condition: and\(always\(\), ne\(variables\[''skipPublish''\], ''true''\)\)\r?\n    \$\{\{ else \}\}:\r?\n      condition: always\(\)'
            $template | Should Match '(?m)^  - task: PublishPipelineArtifact@1\r?\n    condition: and\(always\(\), ne\(variables\[''skipPublish''\], ''true''\)\)'
        }
    }

}
finally
{
    Remove-Variable -Name CoverageTestCollector -Scope Global -ErrorAction SilentlyContinue
    foreach ($name in $script:savedEnvironment.Keys)
    {
        [Environment]::SetEnvironmentVariable($name, $script:savedEnvironment[$name])
    }
    if (Test-Path -LiteralPath $script:fixtureRoot)
    {
        Remove-Item -LiteralPath $script:fixtureRoot -Recurse -Force
    }
}
