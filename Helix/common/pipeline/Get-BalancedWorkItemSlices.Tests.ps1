# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

Describe 'Get-BalancedWorkItemSlices' {
    BeforeAll {
        $script:subject = Join-Path $PSScriptRoot 'Get-BalancedWorkItemSlices.ps1'

        function New-WorkItems([string[]]$names) {
            $names | ForEach-Object { [pscustomobject]@{ Include = $_ } }
        }

        function Write-Durations([hashtable]$durations) {
            $path = Join-Path $TestDrive 'durations.json'
            @{ unit = 'seconds'; workItems = $durations } | ConvertTo-Json | Set-Content -Path $path
            $path
        }
    }

    It 'gives the longest work items their own slices and packs the rest by duration' {
        $file = Write-Durations @{ A = 100; B = 60; C = 50; D = 40; E = 10 }
        $plan = & $script:subject -WorkItems (New-WorkItems 'E', 'D', 'C', 'B', 'A') -TotalSlices 2 -DurationsFile $file

        $bySlice = @{}
        foreach ($p in $plan) { $bySlice[$p.Name] = $p.Slice }
        # LPT: A->1 (100), B->2 (60), C->2 (110), D->1 (140), E->2 (120)
        $bySlice['A'] | Should -Be 1
        $bySlice['B'] | Should -Be 2
        $bySlice['C'] | Should -Be 2
        $bySlice['D'] | Should -Be 1
        $bySlice['E'] | Should -Be 2
    }

    It 'assigns every work item exactly once and returns the original work item objects' {
        $file = Write-Durations @{ A = 5; B = 4; C = 3 }
        $items = New-WorkItems 'A', 'B', 'C', 'D'
        $plan = @(& $script:subject -WorkItems $items -TotalSlices 3 -DurationsFile $file)

        $plan.Count | Should -Be 4
        ($plan.Name | Sort-Object) -join ',' | Should -Be 'A,B,C,D'
        $plan | ForEach-Object { $_.Slice | Should -BeIn 1, 2, 3 }
        ($plan | Where-Object Name -eq 'A').WorkItem | Should -Be $items[0]
    }

    It 'uses the median known duration for work items missing from the file' {
        $file = Write-Durations @{ A = 10; B = 20; C = 90 }
        $plan = & $script:subject -WorkItems (New-WorkItems 'A', 'B', 'C', 'NewItem') -TotalSlices 2 -DurationsFile $file

        $new = $plan | Where-Object Name -eq 'NewItem'
        $new.Estimated | Should -BeTrue
        $new.Seconds | Should -Be 20
        ($plan | Where-Object Name -eq 'A').Estimated | Should -BeFalse
    }

    It 'produces the same plan regardless of input order' {
        $file = Write-Durations @{ A = 30; B = 30; C = 30; D = 30 }
        $first = & $script:subject -WorkItems (New-WorkItems 'A', 'B', 'C', 'D') -TotalSlices 2 -DurationsFile $file
        $second = & $script:subject -WorkItems (New-WorkItems 'D', 'C', 'B', 'A') -TotalSlices 2 -DurationsFile $file

        (($first | Sort-Object Name | ForEach-Object { "$($_.Name)=$($_.Slice)" }) -join ';') |
            Should -Be (($second | Sort-Object Name | ForEach-Object { "$($_.Name)=$($_.Slice)" }) -join ';')
    }
}
