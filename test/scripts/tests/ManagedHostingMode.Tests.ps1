# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

<#
.SYNOPSIS
Checks that compiled managed UAP tests remain discoverable but ignored.
.DESCRIPTION
Uses TAEF /listProperties only; no test bodies or fixtures are executed. Supply
managed DLLs (wildcards are accepted) and TE.exe, or saved unfiltered listings
from /listProperties /runIgnoredTests and /listProperties respectively.
Include both class-level and method-level UAP tests in the supplied payload.
An optional pre-change /listProperties /runIgnoredTests listing also verifies
that the test inventory and non-UAP hosting/ignore metadata have not changed.
.EXAMPLE
powershell -NoProfile -File .\test\scripts\tests\ManagedHostingMode.Tests.ps1 -SelfTest
.EXAMPLE
.\test\scripts\tests\ManagedHostingMode.Tests.ps1 -TestDllPath '.\BuildOutput\bin\amd64chk\Test\Microsoft.UI.Xaml.Tests.Managed.*.dll' -TaefPath '.\packages\Microsoft.Taef.10.100.251104001\build\Binaries\x64\te.exe'
.EXAMPLE
.\test\scripts\tests\ManagedHostingMode.Tests.ps1 -AllTestsListingPath .\all-tests.log -DefaultListingPath .\default-tests.log -BaselineListingPath .\before-tests.log
#>
[CmdletBinding(DefaultParameterSetName = 'Binaries')]
param(
    [Parameter(Mandatory = $true, ParameterSetName = 'Binaries')]
    [string[]]$TestDllPath,

    [Parameter(Mandatory = $true, ParameterSetName = 'Binaries')]
    [string]$TaefPath,

    [Parameter(Mandatory = $true, ParameterSetName = 'Listings')]
    [string[]]$AllTestsListingPath,

    [Parameter(Mandatory = $true, ParameterSetName = 'Listings')]
    [string[]]$DefaultListingPath,

    [Parameter(ParameterSetName = 'Binaries')]
    [Parameter(ParameterSetName = 'Listings')]
    [string[]]$BaselineListingPath,

    [Parameter(Mandatory = $true, ParameterSetName = 'SelfTest')]
    [switch]$SelfTest
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

function Assert-True {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) { throw $Message }
}

function ConvertFrom-TaefListing {
    param([string[]]$Lines)

    $module = $null
    $class = $null
    $method = $null
    $tests = [Collections.Generic.List[object]]::new()
    foreach ($line in $Lines) {
        Assert-True ($line -notmatch '^\s*(Error|Failed)\b') "TAEF listing contains an error: $line"
        if ($line -notmatch '^(?<indent> *)(?<text>\S.*?)\s*$') { continue }
        $indent = $Matches.indent.Length
        $text = $Matches.text
        if ($text -match '^Property\[(?<key>.+)\]\s*=\s*(?<value>.*)$') {
            $owner = switch ($indent) {
                16 { $module }
                20 { $class }
                24 { $method }
            }
            Assert-True ($null -ne $owner) "Property without a recognized TAEF scope: $line"
            Assert-True (-not $owner.Properties.ContainsKey($Matches.key)) "Duplicate property in TAEF scope: $line"
            $owner.Properties[$Matches.key] = $Matches.value.Trim()
        } elseif ($indent -eq 8 -and $text -match '\.dll$') {
            $module = [pscustomobject]@{ Name = [IO.Path]::GetFileName($text); Properties = @{} }
            $class = $null
            $method = $null
        } elseif ($indent -eq 12 -and $null -ne $module) {
            $class = [pscustomobject]@{ Name = $text; Properties = @{} }
            $method = $null
        } elseif ($indent -eq 16 -and $null -ne $class -and
                  ($text.StartsWith($class.Name + '.') -or $text.StartsWith($class.Name + '::'))) {
            $method = [pscustomobject]@{
                Name = $text
                Module = $module
                Class = $class
                Properties = @{}
            }
            $tests.Add($method)
        }
    }

    foreach ($test in $tests) {
        if ($test.Module.Properties['TaefTestType'] -ne 'Managed') { continue }
        $effective = $test.Module.Properties.Clone()
        foreach ($properties in @($test.Class.Properties, $test.Properties)) {
            foreach ($key in $properties.Keys) { $effective[$key] = $properties[$key] }
        }
        $hostingScope = if ($test.Properties.ContainsKey('Hosting:Mode')) {
            'Method'
        } elseif ($test.Class.Properties.ContainsKey('Hosting:Mode')) {
            'Class'
        } else {
            'Module'
        }
        [pscustomobject]@{
            Id = $test.Module.Name + '!' + $test.Name
            HostingMode = $effective['Hosting:Mode']
            HostingScope = $hostingScope
            Ignored = $effective['Ignore'] -eq 'True'
        }
    }
}

function Get-TestIndex {
    param([object[]]$Tests)
    $index = @{}
    foreach ($test in $Tests) {
        Assert-True (-not $index.ContainsKey($test.Id)) "Duplicate test in listings: $($test.Id)"
        $index[$test.Id] = $test
    }
    return $index
}

function Assert-ManagedHostingMetadata {
    param([object[]]$AllTests, [object[]]$DefaultTests, [object[]]$BaselineTests)

    $all = Get-TestIndex $AllTests
    $default = Get-TestIndex $DefaultTests
    Assert-True ($all.Count -gt 0) 'No managed tests found. Supply complete TAEF /listProperties output.'
    $uap = @($AllTests | Where-Object HostingMode -eq 'UAP')
    Assert-True ($uap.Count -gt 0) 'No managed UAP tests found in the /runIgnoredTests listing.'
    foreach ($scope in @('Class', 'Method')) {
        Assert-True (@($uap | Where-Object HostingScope -eq $scope).Count -gt 0) "No $scope-level UAP example in the /runIgnoredTests listing."
    }
    $enabledUap = @($uap | Where-Object { -not $_.Ignored })
    Assert-True ($enabledUap.Count -eq 0) ("Managed UAP tests must have effective Ignore=True ({0} enabled; showing up to 20):`n{1}" -f
        $enabledUap.Count, (($enabledUap | Select-Object -First 20 | ForEach-Object { $_.Id }) -join "`n"))

    foreach ($test in $AllTests) {
        Assert-True ($default.ContainsKey($test.Id) -eq (-not $test.Ignored)) "Default discovery disagrees with effective Ignore metadata: $($test.Id)"
        if ($default.ContainsKey($test.Id)) {
            Assert-True ($default[$test.Id].HostingMode -eq $test.HostingMode -and
                         $default[$test.Id].Ignored -eq $test.Ignored) "Metadata differs between listings: $($test.Id)"
        }
    }
    foreach ($test in $DefaultTests) {
        Assert-True ($all.ContainsKey($test.Id)) "Default test missing from /runIgnoredTests listing: $($test.Id)"
    }

    if ($BaselineTests.Count -gt 0) {
        $baseline = Get-TestIndex $BaselineTests
        Assert-True ($baseline.Count -eq $all.Count) 'Managed test count changed from the baseline.'
        foreach ($test in $BaselineTests) {
            Assert-True ($all.ContainsKey($test.Id)) "Managed test removed from the baseline: $($test.Id)"
            Assert-True ($all[$test.Id].HostingMode -eq $test.HostingMode) "Hosting mode changed from the baseline: $($test.Id)"
            if ($test.HostingMode -ne 'UAP') {
                Assert-True ($all[$test.Id].Ignored -eq $test.Ignored) "Non-UAP ignore metadata changed from the baseline: $($test.Id)"
            }
        }
    }
    [pscustomobject]@{
        ManagedTests = $all.Count
        IgnoredUapTests = $uap.Count
        ClassUapTests = @($uap | Where-Object HostingScope -eq 'Class').Count
        MethodUapTests = @($uap | Where-Object HostingScope -eq 'Method').Count
        DefaultTests = $default.Count
        BaselineCompared = $BaselineTests.Count -gt 0
    }
}

function Invoke-TaefListing {
    param([string]$Executable, [string[]]$Dlls, [switch]$RunIgnoredTests)
    $arguments = @($Dlls) + '/listProperties'
    if ($RunIgnoredTests) { $arguments += '/runIgnoredTests' }
    $lines = @(& $Executable @arguments 2>&1 | ForEach-Object { "$_" })
    Assert-True ($LASTEXITCODE -eq 0) "TAEF listing failed with exit code $LASTEXITCODE.`n$($lines -join "`n")"
    return $lines
}

function Assert-Fails {
    param([scriptblock]$Action, [string]$ExpectedMessage)
    try {
        & $Action | Out-Null
    } catch {
        Assert-True ($_.Exception.Message -like "*$ExpectedMessage*") "Unexpected failure: $_"
        return
    }
    throw "Expected validation failure: $ExpectedMessage"
}

function Test-ListingValidation {
    $allLines = @'
Test Authoring and Execution Framework v10.100k for x64
        C:\payload\Microsoft.UI.Xaml.Tests.Managed.Fixture.dll
                Property[TaefTestType] = Managed
                Property[Hosting:Mode] = WPF
            Fixture.UnsupportedClass
                    Property[Hosting:Mode] = UAP
                    Property[Ignore] = TRUE
                Fixture.UnsupportedClass.InheritsIgnore
                Fixture.UnsupportedClass.OverridesIgnore
                        Property[Ignore] = True
            Fixture.SupportedClass
                Fixture.SupportedClass.UnsupportedMethod
                        Property[Hosting:Mode] = UAP
                        Property[Ignore] = True
                Fixture.SupportedClass.SupportedMethod
            Fixture.IgnoredSupportedClass
                    Property[Ignore] = True
                Fixture.IgnoredSupportedClass.ReenabledMethod
                        Property[Ignore] = False
                Fixture.IgnoredSupportedClass.IgnoredMethod
        C:\payload\Native.dll
                Property[TaefTestType] = Native
            Native::Class
                Native::Class::NotManaged
'@ -split '\r?\n'
    $defaultLines = @'
        C:\payload\Microsoft.UI.Xaml.Tests.Managed.Fixture.dll
                Property[TaefTestType] = Managed
                Property[Hosting:Mode] = WPF
            Fixture.SupportedClass
                Fixture.SupportedClass.SupportedMethod
            Fixture.IgnoredSupportedClass
                    Property[Ignore] = True
                Fixture.IgnoredSupportedClass.ReenabledMethod
                        Property[Ignore] = False
'@ -split '\r?\n'
    $all = @(ConvertFrom-TaefListing $allLines)
    $default = @(ConvertFrom-TaefListing $defaultLines)
    $summary = Assert-ManagedHostingMetadata $all $default $all
    Assert-True ($summary.ManagedTests -eq 6 -and $summary.IgnoredUapTests -eq 3 -and
                 $summary.ClassUapTests -eq 2 -and $summary.MethodUapTests -eq 1 -and
                 $summary.DefaultTests -eq 2) 'Incorrect metadata inheritance or discovery counts.'

    $badClass = $allLines -replace 'Property\[Ignore\] = TRUE$', 'Property[Ignore] = False'
    Assert-Fails { Assert-ManagedHostingMetadata @(ConvertFrom-TaefListing $badClass) $default @() } 'must have effective Ignore=True'
    $badClassOverride = (($allLines -join "`n") -replace
        '(Fixture.UnsupportedClass.OverridesIgnore\n +Property\[Ignore\] = )True', '${1}False') -split '\r?\n'
    Assert-Fails { Assert-ManagedHostingMetadata @(ConvertFrom-TaefListing $badClassOverride) $default @() } 'must have effective Ignore=True'
    $badMethod = (($allLines -join "`n") -replace
        '(Fixture.SupportedClass.UnsupportedMethod\n +Property\[Hosting:Mode\] = UAP\n +Property\[Ignore\] = )True', '${1}False') -split '\r?\n'
    Assert-Fails { Assert-ManagedHostingMetadata @(ConvertFrom-TaefListing $badMethod) $default @() } 'must have effective Ignore=True'
    $duplicateProperty = $allLines -replace '^(                        Property\[Hosting:Mode\] = UAP)$', "`$1`n                        Property[Ignore] = False"
    Assert-Fails { ConvertFrom-TaefListing ($duplicateProperty -split '\r?\n') } 'Duplicate property'
    Assert-Fails { Assert-ManagedHostingMetadata $all $all @() } 'Default discovery disagrees'
    Assert-Fails { Assert-ManagedHostingMetadata $all @($default[0]) @() } 'Default discovery disagrees'
    Assert-Fails { Assert-ManagedHostingMetadata $default $default @() } 'No managed UAP tests'
    Assert-Fails { Assert-ManagedHostingMetadata @() @() @() } 'No managed tests'
    Assert-Fails { Assert-ManagedHostingMetadata ($all + $all[0]) $default @() } 'Duplicate test'
    Assert-Fails { ConvertFrom-TaefListing @('Error: Could not load test module') } 'listing contains an error'

    $baseline = @(ConvertFrom-TaefListing $allLines)
    $baseline[0].Ignored = $false
    $baseline[1].Ignored = $false
    $baseline[2].Ignored = $false
    Assert-ManagedHostingMetadata $all $default $baseline | Out-Null
    $baseline[3].Ignored = $true
    Assert-Fails { Assert-ManagedHostingMetadata $all $default $baseline } 'Non-UAP ignore metadata changed'
    Assert-Fails { Assert-ManagedHostingMetadata $all $default @($baseline[0]) } 'Managed test count changed'
    $baseline[3].Ignored = $false
    $baseline[3].HostingMode = 'Win32Explicit'
    Assert-Fails { Assert-ManagedHostingMetadata $all $default $baseline } 'Hosting mode changed'
    Write-Host 'Managed hosting metadata parser tests passed.'
}

if ($SelfTest) {
    Test-ListingValidation
    return
}

if ($PSCmdlet.ParameterSetName -eq 'Binaries') {
    $executable = (Resolve-Path -LiteralPath $TaefPath).Path
    $dlls = @($TestDllPath | ForEach-Object { Resolve-Path -Path $_ } | Select-Object -ExpandProperty Path -Unique)
    Assert-True ($dlls.Count -gt 0) 'No test DLLs resolved.'
    foreach ($dll in $dlls) {
        Assert-True ([IO.File]::Exists($dll) -and [IO.Path]::GetExtension($dll) -eq '.dll') "Not a test DLL: $dll"
    }
    $allLines = @(Invoke-TaefListing $executable $dlls -RunIgnoredTests)
    $defaultLines = @(Invoke-TaefListing $executable $dlls)
} else {
    $allLines = @(Get-Content -LiteralPath $AllTestsListingPath)
    $defaultLines = @(Get-Content -LiteralPath $DefaultListingPath)
}
$allTests = @(ConvertFrom-TaefListing $allLines)
$defaultTests = @(ConvertFrom-TaefListing $defaultLines)
$baselineTests = @()
if ($BaselineListingPath) {
    $baselineTests = @(ConvertFrom-TaefListing @(Get-Content -LiteralPath $BaselineListingPath))
    Assert-True ($baselineTests.Count -gt 0) 'No managed tests found in the baseline listing.'
}
Assert-ManagedHostingMetadata $allTests $defaultTests $baselineTests
Write-Host 'Managed UAP metadata and default discovery checks passed.'
