# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

Describe "Binplacing the resolved CsWinRT runtime" {
    BeforeEach {
        $script:repoRoot = [System.IO.Path]::GetFullPath("$PSScriptRoot\..\..")
        $script:dotnet = "$script:repoRoot\.dotnet\dotnet.exe"
        [xml]$versions = Get-Content "$script:repoRoot\eng\Versions.props"
        $script:cswinrtVersion = $versions.SelectSingleNode("//MicrosoftCsWinRTPackageVersion").InnerText
        [xml]$sourceProject = Get-Content "$script:repoRoot\dxaml\test\infra\client\cswinrt\Private.Infrastructure.CsWinRT.csproj"
        $script:binplaceTarget = $sourceProject.SelectSingleNode("//Target[@Name='BinplaceCsWinRTRuntime']").OuterXml
        $script:projectPath = "$TestDrive\Runtime.csproj"
    }

    It "binplaces the real runtime asset without enabling library copy-local" {
        @"
<Project Sdk="Microsoft.NET.Sdk">
  <PropertyGroup>
    <TargetFramework>net10.0</TargetFramework>
    <CsWinRTEnabled>false</CsWinRTEnabled>
    <TestBinplaceDestinationPath>$TestDrive\binplace</TestBinplaceDestinationPath>
  </PropertyGroup>
  <ItemGroup>
    <PackageReference Include="Microsoft.Windows.CsWinRT" Version="$script:cswinrtVersion" />
  </ItemGroup>
  $script:binplaceTarget
</Project>
"@ | Set-Content $script:projectPath

        $restore = & $script:dotnet restore $script:projectPath --configfile "$script:repoRoot\NuGet.config" --packages "$script:repoRoot\packages" --verbosity quiet 2>&1
        if ($LASTEXITCODE -ne 0) { throw ($restore -join "`n") }
        $output = & $script:dotnet msbuild $script:projectPath "/t:ResolveReferences;BinplaceCsWinRTRuntime" /getProperty:CopyLocalLockFileAssemblies "/getItem:BinplaceItem,ReferenceCopyLocalPaths,RuntimeCopyLocalItems" /v:quiet 2>&1
        if ($LASTEXITCODE -ne 0) { throw ($output -join "`n") }
        $result = ($output -join "`n") | ConvertFrom-Json

        $result.Properties.CopyLocalLockFileAssemblies | Should Be "false"
        @($result.Items.ReferenceCopyLocalPaths | Where-Object Filename -eq "WinRT.Runtime").Count | Should Be 0
        $runtime = @($result.Items.RuntimeCopyLocalItems | Where-Object Filename -eq "WinRT.Runtime")
        $runtime.Count | Should Be 1
        $binplace = @($result.Items.BinplaceItem)
        $binplace.Count | Should Be 1
        $binplace[0].Identity | Should Be $runtime[0].Identity
        $binplace[0].Destination | Should Be "$TestDrive\binplace"
        Test-Path $binplace[0].Identity | Should Be $true
    }

    It "fails explicitly when no runtime package asset is resolved" {
        @"
<Project Sdk="Microsoft.NET.Sdk">
  <PropertyGroup>
    <TargetFramework>net10.0</TargetFramework>
  </PropertyGroup>
  $script:binplaceTarget
</Project>
"@ | Set-Content $script:projectPath

        $restore = & $script:dotnet restore $script:projectPath --configfile "$script:repoRoot\NuGet.config" --verbosity quiet 2>&1
        if ($LASTEXITCODE -ne 0) { throw ($restore -join "`n") }
        $output = & $script:dotnet msbuild $script:projectPath "/t:ResolveReferences;BinplaceCsWinRTRuntime" /v:quiet 2>&1
        $LASTEXITCODE | Should Be 1
        ($output -join "`n") | Should Match "The resolved CsWinRT runtime reference is missing."
    }
}
