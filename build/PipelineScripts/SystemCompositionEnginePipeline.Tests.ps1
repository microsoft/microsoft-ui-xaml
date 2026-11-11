# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

function New-FakeTaefCatalog
{
    param(
        [Parameter(Mandatory)]
        [string[]]$Tests
    )

    $lines = @('        Microsoft.UI.Xaml.Tests.External.Fake.dll')
    $testClasses = @(
        $Tests |
            Group-Object {
                $separatorIndex = $_.LastIndexOf('::')
                if ($separatorIndex -lt 0) {
                    throw "Test name '$_' does not contain a class separator."
                }

                $_.Substring(0, $separatorIndex)
            }
    )
    foreach ($testClass in $testClasses) {
        $lines += "            $($testClass.Name)"
        $lines += (
            '            Property[HelixWorkItemCreation] = ' +
            'CreateWorkItemPerTestClass')
        $lines += @(
            $testClass.Group |
                ForEach-Object { "                $_" }
        )
    }

    return $lines -join [Environment]::NewLine
}

function New-FakeTaefHarness
{
    param(
        [Parameter(Mandatory)]
        [string[]]$RegularTests,

        [string[]]$IgnoredTests = @()
    )

    $id = [guid]::NewGuid().ToString('N')
    $invocationLog = Join-Path $TestDrive "$id-invocations.txt"
    $harnessPath = Join-Path $TestDrive "$id.ps1"

    $regularCatalog = New-FakeTaefCatalog -Tests $RegularTests
    $ignoredCatalog = New-FakeTaefCatalog `
        -Tests @($RegularTests + $IgnoredTests)

    @(
        '$global:LASTEXITCODE = 0'
        "`$regularCatalog = @'"
        $regularCatalog
        "'@"
        "`$ignoredCatalog = @'"
        $ignoredCatalog
        "'@"
        "if (`$args -contains '/runIgnoredTests') {"
        "    Add-Content -LiteralPath '$invocationLog' -Value 'ignored'"
        '    Write-Output $ignoredCatalog'
        '}'
        'else {'
        "    Add-Content -LiteralPath '$invocationLog' -Value 'regular'"
        '    Write-Output $regularCatalog'
        '}'
    ) | Set-Content -LiteralPath $harnessPath -Encoding UTF8

    return [pscustomobject]@{
        Path = $harnessPath
        InvocationLog = $invocationLog
    }
}

Describe 'System composition engine pipeline review fixes' {
    BeforeEach {
        $script:repositoryRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
        $script:generator = Join-Path `
            $script:repositoryRoot `
            'Helix\common\pipeline\GenerateHelixWorkItems.ps1'
        $script:helixGenerator = Join-Path `
            $script:repositoryRoot `
            'Helix\GenerateHelixWorkItems.ps1'
        $script:broadGenerator = Join-Path `
            $PSScriptRoot `
            'New-SystemCompositionEngineBroadWorkItem.ps1'
        $script:crashDumpPolicy = Join-Path `
            $PSScriptRoot `
            'Set-SystemCompositionEngineCrashDumpPolicy.ps1'
        $script:sliceRunner = Join-Path `
            $script:repositoryRoot `
            'Helix\common\pipeline\RunTestPassSliceOnBuildAgent.ps1'
        $script:runSetupIfNeeded = Join-Path `
            $script:repositoryRoot `
            'Helix\common\test\RunSetupIfNeeded.ps1'
        $script:pipelineJobTemplate = Join-Path `
            $script:repositoryRoot `
            'build\AzurePipelinesTemplates\WinUI-RunTestPassOnPipeline-Job.yml'
        $script:excludedTestsManifest = Join-Path `
            $PSScriptRoot `
            'SystemCompositionEngineExcludedTests-WPF.csv'
        $script:focusedExcludedTestsManifest = Join-Path `
            $PSScriptRoot `
            'SystemCompositionEngineFocusedExcludedTests-WPF.csv'
        $script:testBinaryPath = Join-Path $TestDrive 'bin'
        New-Item `
            -ItemType Directory `
            -Path $script:testBinaryPath `
            -Force | Out-Null
    }

    Context 'excluded-test manifest validation' {
        BeforeEach {
            $normalTest = (
                'Microsoft::UI::Xaml::Tests::Foundation::Graphics::' +
                'ImplicitAnimationTests::NormalTest')
            $script:harness = New-FakeTaefHarness -RegularTests @($normalTest)
            $script:outputProjFile = Join-Path $TestDrive 'output.proj'
            $script:manifestPath = Join-Path $TestDrive 'excluded.csv'
            $script:broadArguments = @{
                TestFilePattern = 'Test\Microsoft.UI.Xaml.Tests.External.*.dll'
                TestBinaryPath = $script:testBinaryPath
                OutputProjFile = $script:outputProjFile
                TaefExePath = $script:harness.Path
                ExcludedTestsManifestPath = $script:manifestPath
                TargetOSVersion = '26704'
            }
        }

        It 'rejects a header-only exclusion manifest' {
            '"WorkItem","TestName"' |
                Set-Content -LiteralPath $script:manifestPath -Encoding ASCII

            { & $script:broadGenerator @script:broadArguments } |
                Should Throw 'contains no excluded tests'
        }

        It 'rejects an exclusion manifest with only blank data rows' {
            @(
                '"WorkItem","TestName"'
                ''
                '   '
            ) | Set-Content -LiteralPath $script:manifestPath -Encoding ASCII

            { & $script:broadGenerator @script:broadArguments } |
                Should Throw 'contains no excluded tests'
        }
    }

    Context 'focused exclusion manifest metadata' {
        It 'accepts public failure categories for focused exclusions' {
            $focusedTests = @(
                Import-Csv -LiteralPath $script:focusedExcludedTestsManifest |
                    ForEach-Object { [string]$_.TestName }
            )
            $focusedTests += (
                'Microsoft::UI::Xaml::Tests::Foundation::Graphics::' +
                'SystemCompositionEngineTests::UnexcludedControl')
            $harness = New-FakeTaefHarness -RegularTests $focusedTests
            $outputProjFile = Join-Path $TestDrive 'focused-output.proj'

            $result = & $script:helixGenerator `
                -TestFilePattern 'Test\Microsoft.UI.Xaml.Tests.External.*.dll' `
                -TestBinaryPath $script:testBinaryPath `
                -OutputProjFile $outputProjFile `
                -JobTestSuiteName DevTestSuite `
                -CompositionEngineType System `
                -HostingMode WPF `
                -TargetOSVersion 26200 `
                -TaefExePath $harness.Path `
                -ExcludedTestsManifestPath $script:focusedExcludedTestsManifest

            @($result.MatchedExcludedTests).Count | Should Be 14
            [xml]$plan = Get-Content -LiteralPath $outputProjFile -Raw
            $commands = @(
                $plan.SelectNodes('/Project/ItemGroup/HelixWorkItem') |
                    ForEach-Object { [string]$_.Command }
            )
            ($commands.Count -gt 0) | Should Be $true
            ($commands -join [Environment]::NewLine) |
                Should Not Match '/reportLoadingIssue'
        }

        It 'rejects an invalid focused failure category' {
            $manifestPath = Join-Path $TestDrive 'invalid-focused.csv'
            $manifestRows = @(
                Import-Csv -LiteralPath $script:focusedExcludedTestsManifest
            )
            $manifestRows[0].FailureCategory = 'Invalid category'
            $manifestRows |
                Export-Csv -LiteralPath $manifestPath -NoTypeInformation
            $focusedTests = @(
                $manifestRows |
                    ForEach-Object { [string]$_.TestName }
            )
            $harness = New-FakeTaefHarness -RegularTests $focusedTests
            $outputProjFile = Join-Path $TestDrive 'invalid-focused-output.proj'

            {
                & $script:helixGenerator `
                    -TestFilePattern 'Test\Microsoft.UI.Xaml.Tests.External.*.dll' `
                    -TestBinaryPath $script:testBinaryPath `
                    -OutputProjFile $outputProjFile `
                    -JobTestSuiteName DevTestSuite `
                    -CompositionEngineType System `
                    -HostingMode WPF `
                    -TargetOSVersion 26200 `
                    -TaefExePath $harness.Path `
                    -ExcludedTestsManifestPath $manifestPath
            } | Should Throw 'contains 1 invalid row'
        }
    }

    Context 'ignored-test catalog discovery' {
        It 'discovers ignored exclusions without enabling ignored test execution' {
            $workItemName = 'WPF-Foundation-Graphics-ImplicitAnimationTests'
            $normalTest = (
                'Microsoft::UI::Xaml::Tests::Foundation::Graphics::' +
                'ImplicitAnimationTests::NormalTest')
            $ignoredTest = (
                'Microsoft::UI::Xaml::Tests::Foundation::Graphics::' +
                'ImplicitAnimationTests::IgnoredKnownFailure')
            $harness = New-FakeTaefHarness `
                -RegularTests @($normalTest) `
                -IgnoredTests @($ignoredTest)
            $manifestPath = Join-Path $TestDrive 'ignored-excluded.csv'
            @(
                '"WorkItem","TestName"'
                "`"$workItemName`",`"$ignoredTest`""
            ) | Set-Content -LiteralPath $manifestPath -Encoding ASCII
            $outputProjFile = Join-Path $TestDrive 'ignored-output.proj'

            $result = & $script:broadGenerator `
                -TestFilePattern 'Test\Microsoft.UI.Xaml.Tests.External.*.dll' `
                -TestBinaryPath $script:testBinaryPath `
                -OutputProjFile $outputProjFile `
                -TaefExePath $harness.Path `
                -ExcludedTestsManifestPath $manifestPath `
                -TargetOSVersion '26704'

            ((Get-Content -LiteralPath $harness.InvocationLog) -join ',') |
                Should Be 'ignored,regular'
            @($result.ValidatedExcludedTests).Count | Should Be 1
            @($result.MatchedExcludedTests).Count | Should Be 0
            [xml]$plan = Get-Content -LiteralPath $outputProjFile -Raw
            $workItems = @($plan.SelectNodes('/Project/ItemGroup/HelixWorkItem'))
            $workItems.Count | Should Be 1
            ([string]$workItems[0].Command) | Should Not Match '/runIgnoredTests'
            ([string]$workItems[0].Command) |
                Should Not Match '/reportLoadingIssue'
        }
    }

    Context 'exact TAEF selection generation' {
        It 'uses the shorter positive selection for the 102-exclusion case' {
            $workItemName = 'WPF-Foundation-Graphics-ImplicitAnimationTests'
            $excludedTestNames = @(
                Import-Csv -LiteralPath $script:excludedTestsManifest |
                    Where-Object { $_.WorkItem -ceq $workItemName } |
                    ForEach-Object { [string]$_.TestName }
            )
            $excludedTestNames.Count | Should Be 102
            $implicitAnimationClass = (
                'Microsoft::UI::Xaml::Tests::Foundation::Graphics::' +
                'ImplicitAnimationTests')
            $remainingTestNames = @(
                'CollapsePopup'
                'CollapsePopupAncestor'
                'CollapseWhilePopupDescendantHasHideAnimation'
                'CollapseWhilePopupDescendantHasHideAnimation2'
                'GridView1WUCFull'
                'GridView2WUCFull'
                'GridView3WUCFull'
                'HideAnimation_CollapseOrRemovePopupChild'
                'HideAnimation6WUCFull'
                'Popup11'
                'Popup12'
                'Popup14'
                'Popup14b'
                'Popup20'
                'Popup4'
                'Popup6'
                'Popup7'
                'Popup8'
            ) | ForEach-Object { '{0}::{1}' -f $implicitAnimationClass, $_ }
            @($remainingTestNames | Sort-Object -Unique).Count | Should Be 18
            @(
                @($excludedTestNames + $remainingTestNames) |
                    Sort-Object -Unique
            ).Count | Should Be 120
            $harness = New-FakeTaefHarness `
                -RegularTests @($excludedTestNames + $remainingTestNames)
            $outputProjFile = Join-Path $TestDrive 'short-selection.proj'
            $excludedTestsByWorkItem = @{}
            $excludedTestsByWorkItem[$workItemName] = $excludedTestNames
            $baseQuery = "@HasAssociatedMasterFile='True'"

            $result = & $script:generator `
                -TestFilePattern 'Test\Microsoft.UI.Xaml.Tests.External.*.dll' `
                -TestBinaryDirectoryPath $script:testBinaryPath `
                -OutputProjFile $outputProjFile `
                -TaefBaseQuery $baseQuery `
                -TaefExtraParameters '/p:CompositionEngineType=System' `
                -TestNamePrefix 'WPF' `
                -WorkItemPrefix 'WPF' `
                -TaefExePath $harness.Path `
                -FailOnTaefDiscoveryError `
                -ExcludedTestsByWorkItem $excludedTestsByWorkItem

            $negativeQuery = (
                "$baseQuery AND NOT(" +
                (@(
                    $excludedTestNames |
                        ForEach-Object { "@Name='$_'" }
                ) -join ' OR ') +
                ')')
            ($negativeQuery.Length -ge 8000) | Should Be $true

            [xml]$plan = Get-Content -LiteralPath $outputProjFile -Raw
            $workItems = @($plan.SelectNodes('/Project/ItemGroup/HelixWorkItem'))
            $workItems.Count | Should Be 1
            $command = [string]$workItems[0].Command
            ($command.Length -lt 8000) | Should Be $true
            $command | Should Not Match 'NOT\(@Name='
            foreach ($remainingTestName in $remainingTestNames) {
                $command | Should Match (
                    [regex]::Escape("@Name='$remainingTestName'"))
            }
            foreach ($excludedTestName in $excludedTestNames) {
                $command | Should Not Match (
                    [regex]::Escape("@Name='$excludedTestName'"))
            }
            @($result.MatchedExcludedTests).Count | Should Be 102
        }

        It 'falls back to a negative selection when a remaining name is not representable' {
            $workItemName = 'WPF-Foundation-Graphics-ImplicitAnimationTests'
            $excludedTestName = (
                'Microsoft::UI::Xaml::Tests::Foundation::Graphics::' +
                'ImplicitAnimationTests::ExcludedTest')
            $remainingTestName = (
                'Microsoft::UI::Xaml::Tests::Foundation::Graphics::' +
                "ImplicitAnimationTests::Remaining'Test")
            $harness = New-FakeTaefHarness `
                -RegularTests @($excludedTestName, $remainingTestName)
            $outputProjFile = Join-Path $TestDrive 'negative-selection.proj'
            $excludedTestsByWorkItem = @{}
            $excludedTestsByWorkItem[$workItemName] = @($excludedTestName)

            & $script:generator `
                -TestFilePattern 'Test\Microsoft.UI.Xaml.Tests.External.*.dll' `
                -TestBinaryDirectoryPath $script:testBinaryPath `
                -OutputProjFile $outputProjFile `
                -TaefBaseQuery "@HasAssociatedMasterFile='True'" `
                -TaefExtraParameters '/p:CompositionEngineType=System' `
                -TestNamePrefix 'WPF' `
                -WorkItemPrefix 'WPF' `
                -TaefExePath $harness.Path `
                -FailOnTaefDiscoveryError `
                -ExcludedTestsByWorkItem $excludedTestsByWorkItem | Out-Null

            [xml]$plan = Get-Content -LiteralPath $outputProjFile -Raw
            $command = [string](
                $plan.SelectSingleNode('/Project/ItemGroup/HelixWorkItem').Command)
            $command | Should Match (
                [regex]::Escape("NOT(@Name='$excludedTestName')"))
        }
    }

    Context 'protected pipeline setup ordering' {
        It 'runs setup without the token before the secured test task' {
            $pipelineContent = Get-Content `
                -LiteralPath $script:pipelineJobTemplate `
                -Raw
            $preTestStepsIndex = $pipelineContent.IndexOf(
                '- ${{ each step in parameters.preTestSteps }}:')
            $setupTaskIndex = $pipelineContent.IndexOf(
                "displayName: 'Prepare machine for protected test launch'")
            $runTestsIndex = $pipelineContent.IndexOf(
                'displayName: Run Tests')
            $postTestIndex = $pipelineContent.IndexOf(
                "displayName: 'PostTestRun: Total lifetime native crashes/warnings'")

            ($preTestStepsIndex -ge 0) | Should Be $true
            ($setupTaskIndex -ge 0) | Should Be $true
            ($runTestsIndex -ge 0) | Should Be $true
            ($postTestIndex -ge 0) | Should Be $true
            ($preTestStepsIndex -lt $setupTaskIndex) | Should Be $true
            ($setupTaskIndex -lt $runTestsIndex) | Should Be $true
            ($runTestsIndex -lt $postTestIndex) | Should Be $true

            $setupTaskContent = $pipelineContent.Substring(
                $setupTaskIndex,
                $runTestsIndex - $setupTaskIndex)
            $runTestsContent = $pipelineContent.Substring(
                $runTestsIndex,
                $postTestIndex - $runTestsIndex)

            $setupTaskContent | Should Match (
                'filePath:\s+\$\(testPayloadDir\)' +
                '\\RunSetupIfNeeded\.ps1')
            $setupTaskContent | Should Match (
                'workingDirectory:\s+\$\(testPayloadDir\)')
            $setupTaskContent | Should Match (
                'HELIX_CORRELATION_PAYLOAD:\s+\$\(testPayloadDir\)')
            $setupTaskContent | Should Match (
                'HELIX_DUMP_FOLDER:\s+C:\\dumps')
            $setupTaskContent | Should Not Match (
                'COMPOSITION_ENGINE_LAF_TOKEN')
            $runTestsContent | Should Match 'pwsh:\s+false'
            $runTestsContent | Should Match 'COMPOSITION_ENGINE_LAF_TOKEN'
            [regex]::Matches(
                $pipelineContent,
                '(?m)^\s*COMPOSITION_ENGINE_LAF_TOKEN:').Count |
                Should Be 1
        }

        It 'requires completed setup before applying policy or creating the mapping' {
            $runnerContent = Get-Content -LiteralPath $script:sliceRunner -Raw
            $tokenBranchIndex = $runnerContent.IndexOf(
                'if (-not [string]::IsNullOrEmpty($env:COMPOSITION_ENGINE_LAF_TOKEN))')
            $markerPathIndex = $runnerContent.IndexOf(
                '$setupMarkerPath = Join-Path $TestPayloadDir "setupcomplete.txt"')
            $markerCheckIndex = $runnerContent.IndexOf(
                'Test-Path -LiteralPath $setupMarkerPath -PathType Leaf')
            $applyIndex = $runnerContent.IndexOf(
                '$systemCompositionEngineCrashDumpPolicyState = &')
            $launchRequestIndex = $runnerContent.IndexOf(
                '& $setLaunchRequestScript @launchRequestParameters')

            ($tokenBranchIndex -ge 0) | Should Be $true
            ($markerPathIndex -ge 0) | Should Be $true
            ($markerCheckIndex -ge 0) | Should Be $true
            ($applyIndex -ge 0) | Should Be $true
            ($launchRequestIndex -ge 0) | Should Be $true
            ($tokenBranchIndex -lt $markerPathIndex) | Should Be $true
            ($markerPathIndex -lt $markerCheckIndex) | Should Be $true
            ($markerCheckIndex -lt $applyIndex) | Should Be $true
            ($markerCheckIndex -lt $launchRequestIndex) | Should Be $true
        }

        It 'makes work-item setup a no-op after the preflight marker is written' {
            $payloadPath = Join-Path $TestDrive 'payload'
            $invocationLog = Join-Path $payloadPath 'core-invoked.txt'
            New-Item -ItemType Directory -Path $payloadPath | Out-Null
            @'
Add-Content -LiteralPath (Join-Path $PSScriptRoot 'core-invoked.txt') -Value 'invoked'
'@ | Set-Content `
                -LiteralPath (
                    Join-Path $payloadPath 'TestPass-OneTimeMachineSetupCore.ps1')

            $hadCorrelationPayload = Test-Path `
                -LiteralPath Env:\HELIX_CORRELATION_PAYLOAD
            $previousCorrelationPayload =
                $env:HELIX_CORRELATION_PAYLOAD
            try {
                $env:HELIX_CORRELATION_PAYLOAD = $payloadPath
                Push-Location $payloadPath
                try {
                    & $script:runSetupIfNeeded
                    & $script:runSetupIfNeeded
                }
                finally {
                    Pop-Location
                }
            }
            finally {
                if ($hadCorrelationPayload) {
                    $env:HELIX_CORRELATION_PAYLOAD =
                        $previousCorrelationPayload
                }
                else {
                    Remove-Item `
                        -LiteralPath Env:\HELIX_CORRELATION_PAYLOAD `
                        -ErrorAction SilentlyContinue
                }
            }

            @(Get-Content -LiteralPath $invocationLog).Count | Should Be 1
            (Get-Content `
                -LiteralPath (Join-Path $payloadPath 'setupcomplete.txt') `
                -Raw).Trim() |
                Should Be 'done'
        }
    }

    Context 'token-bearing host crash dump policy' {
        BeforeEach {
            $id = [guid]::NewGuid().ToString('N')
            $script:testRegistryCleanupPath = (
                "Software\Microsoft\Windows\Windows Error Reporting\" +
                "WinUITests\$id")
            $script:testLocalDumpsRegistryPath =
                "$script:testRegistryCleanupPath\LocalDumps"
            $script:testProcessName = 'te.processhost.exe'
        }

        AfterEach {
            $baseKey = [Microsoft.Win32.RegistryKey]::OpenBaseKey(
                [Microsoft.Win32.RegistryHive]::CurrentUser,
                [Microsoft.Win32.RegistryView]::Registry64)
            try {
                $baseKey.DeleteSubKeyTree(
                    $script:testRegistryCleanupPath,
                    $false)
            }
            finally {
                $baseKey.Close()
            }
        }

        It 'overrides an inherited full dump policy and removes its temporary key' {
            $baseKey = [Microsoft.Win32.RegistryKey]::OpenBaseKey(
                [Microsoft.Win32.RegistryHive]::CurrentUser,
                [Microsoft.Win32.RegistryView]::Registry64)
            try {
                $localDumpsKey = $baseKey.CreateSubKey(
                    $script:testLocalDumpsRegistryPath)
                try {
                    $localDumpsKey.SetValue(
                        'DumpType',
                        2,
                        [Microsoft.Win32.RegistryValueKind]::DWord)
                }
                finally {
                    $localDumpsKey.Close()
                }
            }
            finally {
                $baseKey.Close()
            }

            $state = & $script:crashDumpPolicy `
                -ProcessName $script:testProcessName `
                -RegistryHive CurrentUser `
                -RegistryView Registry64 `
                -LocalDumpsRegistryPath $script:testLocalDumpsRegistryPath

            $state.ProcessKeyExisted | Should Be $false
            $baseKey = [Microsoft.Win32.RegistryKey]::OpenBaseKey(
                [Microsoft.Win32.RegistryHive]::CurrentUser,
                [Microsoft.Win32.RegistryView]::Registry64)
            try {
                $processKey = $baseKey.OpenSubKey(
                    "$script:testLocalDumpsRegistryPath\$script:testProcessName")
                try {
                    [int]$processKey.GetValue('DumpType') | Should Be 1
                    $processKey.GetValueKind('DumpType') |
                        Should Be ([Microsoft.Win32.RegistryValueKind]::DWord)
                }
                finally {
                    $processKey.Close()
                }
            }
            finally {
                $baseKey.Close()
            }

            & $script:crashDumpPolicy -RestoreState $state

            $baseKey = [Microsoft.Win32.RegistryKey]::OpenBaseKey(
                [Microsoft.Win32.RegistryHive]::CurrentUser,
                [Microsoft.Win32.RegistryView]::Registry64)
            try {
                $localDumpsKey = $baseKey.OpenSubKey(
                    $script:testLocalDumpsRegistryPath)
                try {
                    [int]$localDumpsKey.GetValue('DumpType') | Should Be 2
                    $processKey = $localDumpsKey.OpenSubKey(
                        $script:testProcessName)
                    ($null -eq $processKey) | Should Be $true
                }
                finally {
                    $localDumpsKey.Close()
                }
            }
            finally {
                $baseKey.Close()
            }
        }

        It 'restores an existing per-process dump policy without changing sibling values' {
            $baseKey = [Microsoft.Win32.RegistryKey]::OpenBaseKey(
                [Microsoft.Win32.RegistryHive]::CurrentUser,
                [Microsoft.Win32.RegistryView]::Registry64)
            try {
                $processKey = $baseKey.CreateSubKey(
                    "$script:testLocalDumpsRegistryPath\$script:testProcessName")
                try {
                    $processKey.SetValue(
                        'DumpType',
                        2,
                        [Microsoft.Win32.RegistryValueKind]::DWord)
                    $processKey.SetValue(
                        'DumpCount',
                        7,
                        [Microsoft.Win32.RegistryValueKind]::DWord)
                    $processKey.SetValue(
                        'DumpFolder',
                        '%TEMP%\OriginalDumps',
                        [Microsoft.Win32.RegistryValueKind]::ExpandString)
                }
                finally {
                    $processKey.Close()
                }
            }
            finally {
                $baseKey.Close()
            }

            $state = & $script:crashDumpPolicy `
                -ProcessName $script:testProcessName `
                -RegistryHive CurrentUser `
                -RegistryView Registry64 `
                -LocalDumpsRegistryPath $script:testLocalDumpsRegistryPath

            $state.ProcessKeyExisted | Should Be $true
            $state.DumpTypeExisted | Should Be $true
            & $script:crashDumpPolicy -RestoreState $state

            $baseKey = [Microsoft.Win32.RegistryKey]::OpenBaseKey(
                [Microsoft.Win32.RegistryHive]::CurrentUser,
                [Microsoft.Win32.RegistryView]::Registry64)
            try {
                $processKey = $baseKey.OpenSubKey(
                    "$script:testLocalDumpsRegistryPath\$script:testProcessName")
                try {
                    [int]$processKey.GetValue('DumpType') | Should Be 2
                    [int]$processKey.GetValue('DumpCount') | Should Be 7
                    $processKey.GetValue(
                        'DumpFolder',
                        $null,
                        [Microsoft.Win32.RegistryValueOptions]::
                            DoNotExpandEnvironmentNames) |
                        Should Be '%TEMP%\OriginalDumps'
                }
                finally {
                    $processKey.Close()
                }
            }
            finally {
                $baseKey.Close()
            }
        }

        It 'applies before token creation and restores after mapping disposal' {
            $runnerContent = Get-Content -LiteralPath $script:sliceRunner -Raw
            $applyIndex = $runnerContent.IndexOf(
                '$systemCompositionEngineCrashDumpPolicyState = &')
            $launchRequestIndex = $runnerContent.IndexOf(
                '& $setLaunchRequestScript @launchRequestParameters')
            $disposeIndex = $runnerContent.IndexOf(
                '$systemCompositionEngineLaunchRequest.Dispose()')
            $restoreIndex = $runnerContent.IndexOf(
                '-RestoreState $systemCompositionEngineCrashDumpPolicyState')

            ($applyIndex -ge 0) | Should Be $true
            ($launchRequestIndex -ge 0) | Should Be $true
            ($disposeIndex -ge 0) | Should Be $true
            ($restoreIndex -ge 0) | Should Be $true
            ($applyIndex -lt $launchRequestIndex) | Should Be $true
            ($disposeIndex -lt $restoreIndex) | Should Be $true
        }
    }

    Context 'pipeline boolean parameters' {
        It 'compares MUXFinalRelease as a boolean in both stage conditions' {
            $pipelinePath = Join-Path `
                (Split-Path -Parent $PSScriptRoot) `
                'WinUI-PR.yml'
            $pipelineContent = Get-Content -LiteralPath $pipelinePath -Raw

            [regex]::Matches(
                $pipelineContent,
                "eq\(parameters\.MUXFinalRelease,\s*'false'\)").Count |
                Should Be 0
            [regex]::Matches(
                $pipelineContent,
                'eq\(parameters\.MUXFinalRelease,\s*false\)').Count |
                Should Be 2
        }

        It 'includes system composition validation in full non-final GitHub PR runs' {
            $pipelinePath = Join-Path `
                (Split-Path -Parent $PSScriptRoot) `
                'WinUI-GitHub-PR.yml'
            $pipelineContent = Get-Content -LiteralPath $pipelinePath -Raw
            $expectedStagePattern = (
                '\-\s+\$\{\{\s*if\s+and\(' +
                'eq\(parameters\.runFullValidation,\s*true\),\s*' +
                'eq\(parameters\.MUXFinalRelease,\s*false\)\)\s*\}\}:\s*' +
                '\r?\n\s*-\s+template:\s+' +
                'AzurePipelinesTemplates\\' +
                'WinUI-RunSystemCompositionEngineTests-Stage\.yml'
            )

            [regex]::Matches(
                $pipelineContent,
                'WinUI-RunSystemCompositionEngineTests-Stage\.yml').Count |
                Should Be 1
            $pipelineContent | Should Match $expectedStagePattern
        }
    }
}
