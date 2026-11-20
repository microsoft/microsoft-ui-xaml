# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

Describe "MSBuild compatibility with the .NET 10 SDK" {
    BeforeEach {
        Import-Module "$PSScriptRoot\..\MSBuildFunctions.psm1" -Force -DisableNameChecking
        $msbuildStub = Join-Path $TestDrive "MSBuild.ps1"
    }

    It "rejects MSBuild 17 even when Visual Studio is installed" {
        Set-Content $msbuildStub "'17.14.23.0'"
        Test-MSBuild $msbuildStub | Should Be $false
    }

    It "accepts MSBuild 18" {
        Set-Content $msbuildStub "'18.0.0.0'"
        Test-MSBuild $msbuildStub | Should Be $true
    }

    It "rejects a missing MSBuild executable" {
        Test-MSBuild (Join-Path $TestDrive "missing.exe") | Should Be $false
    }

    It "resolves a selected release channel to a download URL" {
        Import-Module "$PSScriptRoot\..\MSBuildFunctions.psm1" -ArgumentList "Release18x" -Force -DisableNameChecking
        Mock Invoke-WebRequest {} -ModuleName MSBuildFunctions

        Download-MSBuild (Join-Path $TestDrive "vs_installer.exe")

        Assert-MockCalled Invoke-WebRequest -ModuleName MSBuildFunctions -Times 1 -Exactly -ParameterFilter {
            $Uri -eq "https://aka.ms/vs/18/release/vs_community.exe"
        }
    }
}
