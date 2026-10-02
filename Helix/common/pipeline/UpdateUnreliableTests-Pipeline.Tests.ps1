# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

Describe 'UpdateUnreliableTests-Pipeline test run selection' {
    BeforeEach {
        $script:subject = Join-Path $PSScriptRoot 'UpdateUnreliableTests-Pipeline.ps1'
        $script:arguments = @{
            AccessToken = 'test-token'
            CollectionUri = 'https://example.invalid/'
            TeamProject = 'WinUI'
            BuildUri = 'vstfs:///Build/Build/1'
            TestRunTitle = 'RegressionSuite'
            SubResultsDirPath = $TestDrive
            ErrorAction = 'Stop'
        }
        $script:runs = @(
            [pscustomobject]@{
                id = 9
                name = 'RegressionSuite'
                completedDate = '2026-09-23T00:01:52Z'
                url = 'https://example.invalid/WinUI/_apis/test/Runs/9'
                totalTests = 1
            },
            [pscustomobject]@{
                id = 10
                name = 'RegressionSuite'
                completedDate = '2026-09-22T05:18:41Z'
                url = 'https://example.invalid/WinUI/_apis/test/Runs/10'
                totalTests = 1
            },
            [pscustomobject]@{
                id = 11
                name = 'DifferentSuite'
                completedDate = '2026-09-24T00:00:00Z'
                url = 'https://example.invalid/WinUI/_apis/test/Runs/11'
                totalTests = 1
            }
        )
        $script:state = @{
            Runs = $script:runs
            QueriedRunIds = @()
            Updates = @()
            Outcome = 'Failed'
        }
        $script:currentSubresults = Join-Path $TestDrive 'current_subresults.json'
        @{
            errors = @('Expected test failure')
            results = @(1..11 | ForEach-Object {
                @{ duration = 100; outcome = 'Failed'; errorIndex = 1 }
            })
        } | ConvertTo-Json -Depth 4 -Compress | Set-Content -LiteralPath $script:currentSubresults

        Mock Write-Host {}
        $state = $script:state
        Mock Invoke-RestMethod ({
            param($Uri, $Method, $Body)

            if ($Method -eq 'Get' -and $Uri -match '/test/runs\?') {
                return @{ value = $state.Runs }
            }
            if ($Method -eq 'Get' -and $Uri -match '/test/Runs/(\d+)/results\?') {
                $runId = [int]$Matches[1]
                $state.QueriedRunIds += $runId
                $fileName = if ($runId -eq 10) { 'current_subresults.json' } else { 'previous_subresults.json' }
                return @{
                    count = 1
                    value = @(@{
                        id = 100000
                        testCaseTitle = 'Example.Test'
                        outcome = $state.Outcome
                        errorMessage = $fileName
                    })
                }
            }
            if ($Method -eq 'Patch') {
                $state.Updates += [pscustomobject]@{ Uri = $Uri; Body = (ConvertFrom-Json $Body) }
                return
            }
            throw "Unexpected REST request: $Method $Uri"
        }.GetNewClosure())
    }

    It 'uses the latest published run even when a previous attempt has a later completion date' {
        & $script:subject @script:arguments

        ($script:state.QueriedRunIds -join ',') | Should Be '10'
        $resultUpdate = @($script:state.Updates | Where-Object { $_.Uri -match '/results\?' })
        $resultUpdate.Count | Should Be 1
        $resultUpdate[0].Uri | Should Match '/Runs/10/results\?'
        $resultUpdate[0].Body.outcome | Should Be 'Failed'
        $resultUpdate[0].Body.subResults.Count | Should Be 11
        $script:state.Updates[-1].Body.state | Should Be 'Completed'
    }

    It 'does not use completion-date ties to choose an older attempt' {
        $script:state.Runs[0].completedDate = $script:state.Runs[1].completedDate

        & $script:subject @script:arguments -ReadOnlyTestMode

        ($script:state.QueriedRunIds -join ',') | Should Be '10'
        $script:state.Updates.Count | Should Be 0
    }

    It 'leaves a passing current run untouched even if the previous attempt failed' {
        $script:state.Outcome = 'Passed'

        & $script:subject @script:arguments

        ($script:state.QueriedRunIds -join ',') | Should Be '10'
        @($script:state.Updates | Where-Object { $_.Uri -match '/results\?' }).Count | Should Be 0
        $script:state.Updates[-1].Body.state | Should Be 'Completed'
    }

    It 'does not process a differently named run when no title matches' {
        $script:state.Runs = @($script:state.Runs[2])

        & $script:subject @script:arguments

        $script:state.QueriedRunIds.Count | Should Be 0
        $script:state.Updates.Count | Should Be 0
    }

    It 'still fails when the current attempt is missing its own subresults' {
        $script:state.Runs = @($script:state.Runs[1])
        Remove-Item -LiteralPath $script:currentSubresults

        { & $script:subject @script:arguments -ReadOnlyTestMode } | Should Throw 'current_subresults.json'
    }
}
