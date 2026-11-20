# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

Describe "Publishing the downloaded SDK to subsequent pipeline tasks" {
    BeforeEach {
        $script:savedTemp = $env:TEMP
        $script:savedPath = $env:PATH
        $script:savedTfBuild = $env:TF_BUILD
        $env:TEMP = $TestDrive
        $env:TF_BUILD = "True"
        $script:sdkScript = "$PSScriptRoot\..\..\build\DownloadDotNetCoreSdk.ps1"
        $script:sdkRoot = [System.IO.Path]::GetFullPath("$PSScriptRoot\..\..\.dotnet")

        Mock Invoke-WebRequest {
            Set-Content -Path $OutFile -Value @'
param ($Version, $Channel, $Quality, $InstallDir, $Architecture)
$env:PATH = "$InstallDir;$env:PATH"
'@
        }
    }

    AfterEach {
        $env:TEMP = $script:savedTemp
        $env:PATH = $script:savedPath
        $env:TF_BUILD = $script:savedTfBuild
    }

    It "publishes the x64 SDK after installing both architectures using <Selection>" -TestCases @(
        @{ Selection = "the default channel"; Arguments = @{} }
        @{ Selection = "an explicit version"; Arguments = @{ version = "10.0.404" } }
        @{ Selection = "an explicit channel"; Arguments = @{ channel = "10.0.4xx" } }
        @{ Selection = "a channel and quality"; Arguments = @{ channel = "10.0.4xx"; quality = "GA" } }
    ) {
        param ($Selection, $Arguments)

        $messages = @(& $script:sdkScript @Arguments 6>&1 | ForEach-Object { $_.ToString() })

        # The installer leaves x86 first in its own process; later tasks must prefer x64.
        ($env:PATH -split ";")[0] | Should Be "$script:sdkRoot\x86"
        $pathCommands = @($messages | Where-Object { $_.StartsWith("##vso[task.prependpath]") })
        $pathCommands.Count | Should Be 1
        $pathCommands[0] | Should Be "##vso[task.prependpath]$script:sdkRoot"
        ($messages -contains "##vso[task.setvariable variable=DOTNET_ROOT]$script:sdkRoot") | Should Be $true
        ($messages -contains "##vso[task.setvariable variable=DOTNET_ROOT_x86]$script:sdkRoot\x86") | Should Be $true
    }

    It "does not emit pipeline environment commands outside Azure Pipelines" {
        $env:TF_BUILD = $null

        $messages = @(& $script:sdkScript 6>&1 | ForEach-Object { $_.ToString() })

        @($messages | Where-Object { $_ -like "##vso*" }).Count | Should Be 0
    }
}
