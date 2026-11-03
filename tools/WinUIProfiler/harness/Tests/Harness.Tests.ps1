[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"
$harnessRoot = Split-Path -Parent $PSScriptRoot
$modulePath = Join-Path $harnessRoot "ProfilerHarness.psm1"
$configPath = Join-Path $harnessRoot "examples\store.local.json"
$ixpConfigPath = Join-Path $harnessRoot "examples\store.ixp.json"
$previewIxpConfigPath = Join-Path $harnessRoot "examples\store.ixp.preview.json"
$safeScenarioPath = Join-Path $harnessRoot "examples\store-search-netflix-safe.json"
$installScenarioPath = Join-Path $harnessRoot "examples\store-search-netflix.json"

Import-Module $modulePath -Force
$module = Get-Module ProfilerHarness
$script:passed = 0

function Assert-True {
    param(
        [Parameter(Mandatory)][bool]$Condition,
        [Parameter(Mandatory)][string]$Message
    )
    if (-not $Condition) {
        throw $Message
    }
}

function Assert-Equal {
    param(
        [AllowNull()]$Actual,
        [AllowNull()]$Expected,
        [Parameter(Mandatory)][string]$Message
    )
    if ($Actual -ne $Expected) {
        throw "$Message Expected '$Expected', got '$Actual'."
    }
}

function Assert-Throws {
    param(
        [Parameter(Mandatory)][scriptblock]$Action,
        [Parameter(Mandatory)][string]$MessagePattern
    )
    $caught = $null
    try {
        & $Action
    }
    catch {
        $caught = $_
    }
    if ($null -eq $caught) {
        throw "Expected an exception matching '$MessagePattern'."
    }
    if ($caught.Exception.Message -notlike $MessagePattern) {
        throw "Exception '$($caught.Exception.Message)' did not match '$MessagePattern'."
    }
}

function Invoke-Test {
    param(
        [Parameter(Mandatory)][string]$Name,
        [Parameter(Mandatory)][scriptblock]$Action
    )
    & $Action
    $script:passed++
    Write-Output "PASS $Name"
}

Invoke-Test "all PowerShell files parse" {
    foreach ($path in Get-ChildItem -LiteralPath $harnessRoot -Filter "*.ps1" -File -Recurse) {
        $tokens = $null
        $errors = $null
        [Management.Automation.Language.Parser]::ParseFile($path.FullName, [ref]$tokens, [ref]$errors) | Out-Null
        Assert-True ($errors.Count -eq 0) "PowerShell parse errors in '$($path.FullName)': $errors"
    }
}

Invoke-Test "example configuration validates" {
    $configuration = Read-ProfilerHarnessConfiguration $configPath
    Assert-Equal $configuration.name "store-local" "Unexpected example name."
    Assert-Equal $configuration.repositories.app.branch "user/akanpatel/profiler_harness" "Unexpected app branch."
    Assert-Equal $configuration.repositories.profiler.branch "user/akanpatel2206/profiler_harness" "Unexpected profiler branch."
}

Invoke-Test "Store example uses command-line build and signed deployment" {
    $configuration = Read-ProfilerHarnessConfiguration $configPath
    $sdkCommand = @($configuration.application.buildCommands) |
        Where-Object { (@($_.arguments) -join " ") -like "*Ensure-DotNetSdk.ps1*" } |
        Select-Object -First 1
    $buildCommand = @($configuration.application.buildCommands) |
        Where-Object { (@($_.arguments) -join " ") -like "*Build-NativeAot.ps1*" } |
        Select-Object -First 1
    Assert-True ($null -ne $sdkCommand) "Store SDK bootstrap is not configured."
    Assert-True ($null -ne $buildCommand) "Store build script is not configured."
    $buildArguments = @($buildCommand.arguments)
    $prepareCommand = @($configuration.application.deployCommands.local) |
        Where-Object { (@($_.arguments) -join " ") -like "*Prepare-StorePayload.ps1*" } |
        Select-Object -First 1
    $deployCommand = @($configuration.application.deployCommands.local) |
        Where-Object { (@($_.arguments) -join " ") -like "*StorePackage.ps1*" } |
        Select-Object -Last 1
    Assert-True ($buildArguments -contains "-BuildOnly") "Store build does not use the NativeAOT build-only workflow."
    Assert-True ($null -ne $prepareCommand) "Signed Store payload preparation is not configured."
    Assert-True ($null -ne $deployCommand) "Store deployment helper is not configured."
    Assert-True ((@($deployCommand.arguments) -join " ") -like "*payloads\store-local*") `
        "Local Store deployment does not use the signed staged payload."
    Assert-True ((($configuration.application | ConvertTo-Json -Depth 20) -notmatch "Invoke-VsBuild|devenv")) `
        "The Store example still contains the obsolete Visual Studio/DTE route."
}

Invoke-Test "Ixp Store example provisions a dedicated ge_current VM" {
    $configuration = Read-ProfilerHarnessConfiguration $ixpConfigPath
    Assert-Equal $configuration.target.kind "ixp" "Unexpected Ixp target kind."
    Assert-Equal $configuration.target.ixp.name "WinUIProfilerHarness-ge-current-latest" "Unexpected Ixp VM name."
    Assert-Equal $configuration.target.ixp.branch "ge_current" "The Ixp VM is not sourced from ge_current."
    Assert-Equal $configuration.target.ixp.connectionMode "psremote" "The tested Ixp connection mode is not selected."
    Assert-True $configuration.target.ixp.ensureCreated "The ge_current VM configuration cannot provision a missing VM."
    Assert-True $configuration.target.ixp.allowOffline "The ge_current VM configuration does not permit cached offline media."
    Assert-Equal $configuration.target.ixp.minimumOsBuild 27200 `
        "The ge_current VM configuration does not reject an older incompatible guest."
    Assert-True $configuration.target.ixp.enableCdKeys `
        "The ge_current VM configuration does not enable future/CD feature keys."
    Assert-True ([string]$configuration.application.payloadPath -like "*store-ixp*") "The Ixp Store payload is not configured."
    Assert-True ([string]$configuration.application.targetInstallScripts.ixp -like "*AllowMissingOriginalPackage*") `
        "The Store installer cannot handle a clean Ixp image."
}

Invoke-Test "preview Ixp Store example requires a compatible guest build" {
    $configuration = Read-ProfilerHarnessConfiguration $previewIxpConfigPath
    Assert-Equal $configuration.target.ixp.name "WinUIProfilerHarness-ge-current-directwex-latest" `
        "Unexpected preview Ixp VM name."
    Assert-True (-not $configuration.target.ixp.ensureCreated) `
        "The preview VM configuration can create a VM unexpectedly."
    Assert-Equal $configuration.target.ixp.minimumOsBuild 27200 `
        "The preview VM configuration does not reject the incompatible 26713 guest."
    Assert-True $configuration.target.ixp.enableCdKeys `
        "The preview VM configuration does not enable future/CD feature keys required by preview composition APIs."
}

Invoke-Test "Ixp creation forwards composition feature configuration" {
    $settings = [pscustomobject]@{
        name = "Harness-Test"
        branch = "ge_current_directwex"
        allowOffline = $true
        enableCdKeys = $true
        velocityKeysToEnable = @("Feature_A", "Feature_B")
        velocityKeysToDisable = @("Feature_C")
    }
    $parameters = & $module {
        param($Settings, $ConfigDirectory)
        Get-HarnessIxpCreationParameters $Settings $ConfigDirectory
    } $settings $harnessRoot
    Assert-True $parameters.AllowOffline "Ixp creation did not forward AllowOffline."
    Assert-True $parameters.EnableCDKeys "Ixp creation did not forward EnableCDKeys."
    Assert-Equal (@($parameters.VelocityKeysToEnable) -join ",") "Feature_A,Feature_B" `
        "Ixp creation did not forward enabled Velocity keys."
    Assert-Equal (@($parameters.VelocityKeysToDisable) -join ",") "Feature_C" `
        "Ixp creation did not forward disabled Velocity keys."
}

Invoke-Test "portable Agency source accepts an installation directory" {
    $testRoot = Join-Path $harnessRoot ".artifacts\tests\agency-source-$([Guid]::NewGuid().ToString('N'))"
    try {
        [IO.Directory]::CreateDirectory($testRoot) | Out-Null
        [IO.File]::WriteAllText(
            (Join-Path $testRoot "agency.exe"),
            "portable",
            [Text.UTF8Encoding]::new($false))
        $resolved = & $module {
            param($Path, $BaseDirectory)
            Resolve-HarnessAgencySource -ConfiguredPath $Path -BaseDirectory $BaseDirectory
        } $testRoot $harnessRoot
        Assert-Equal $resolved ([IO.Path]::GetFullPath($testRoot)) "The portable Agency directory resolved incorrectly."
    }
    finally {
        if (Test-Path -LiteralPath $testRoot) {
            Remove-Item -LiteralPath $testRoot -Recurse -Force
        }
    }
}

Invoke-Test "invalid Ixp connection mode is rejected" {
    $configuration = Get-Content -LiteralPath $ixpConfigPath -Raw -Encoding UTF8 | ConvertFrom-Json -Depth 100
    $configuration.target.ixp.connectionMode = "invalid"
    Assert-Throws {
        & $module {
            param($Configuration)
            Test-ProfilerHarnessConfiguration $Configuration
        } $configuration
    } "*connectionMode must be auto, background, or psremote*"
}

Invoke-Test "Store target payload preserves the expected package layout" {
    $testRoot = Join-Path $harnessRoot ".artifacts\tests\store-payload-$([Guid]::NewGuid().ToString('N'))"
    $repo = Join-Path $testRoot "repo"
    $output = Join-Path $testRoot "payload"
    $packageDirectory = Join-Path $repo "bin\Release\WinStore2\net10.0-windows10.0.28000.0\x64\AppPackages"
    $packageLayout = Join-Path $testRoot "package-layout"
    $packageScript = Join-Path $testRoot "StorePackage.ps1"
    try {
        [IO.Directory]::CreateDirectory($packageDirectory) | Out-Null
        [IO.Directory]::CreateDirectory($packageLayout) | Out-Null
        [IO.File]::WriteAllText((Join-Path $packageLayout "AppxManifest.xml"), @'
<?xml version="1.0" encoding="utf-8"?>
<Package xmlns="http://schemas.microsoft.com/appx/manifest/foundation/windows10">
  <Identity Name="Microsoft.WindowsStore"
            Publisher="CN=Microsoft Corporation, O=Microsoft Corporation, L=Redmond, S=Washington, C=US"
            Version="29999.0.1.1"
            ProcessorArchitecture="x64" />
  <Properties>
    <DisplayName>Harness Store Package</DisplayName>
    <PublisherDisplayName>Microsoft Corporation</PublisherDisplayName>
    <Logo>logo.png</Logo>
    <Framework>true</Framework>
  </Properties>
  <Resources>
    <Resource Language="en-us" />
  </Resources>
  <Dependencies>
    <TargetDeviceFamily Name="Windows.Desktop"
                        MinVersion="10.0.17763.0"
                        MaxVersionTested="10.0.26100.0" />
  </Dependencies>
</Package>
'@, [Text.UTF8Encoding]::new($false))
        [IO.File]::WriteAllBytes(
            (Join-Path $packageLayout "logo.png"),
            [Convert]::FromBase64String(
                "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mNk+A8AAQUBAScY42YAAAAASUVORK5CYII="))
        $makeAppx = Get-ChildItem (Join-Path ${env:ProgramFiles(x86)} "Windows Kits\10\bin") `
            -Filter "makeappx.exe" -File -Recurse |
            Where-Object { $_.Directory.Name -eq "x64" } |
            Sort-Object FullName -Descending |
            Select-Object -First 1
        Assert-True ($null -ne $makeAppx) "makeappx.exe was not found."
        $makeAppxOutput = @(
            & $makeAppx.FullName pack /d $packageLayout /p (Join-Path $packageDirectory "Store.msix") /o 2>&1
        )
        if ($LASTEXITCODE -ne 0) {
            throw "The test MSIX could not be created. $($makeAppxOutput -join [Environment]::NewLine)"
        }
        [IO.File]::WriteAllText($packageScript, "param()", [Text.UTF8Encoding]::new($false))
        $result = & (Join-Path $harnessRoot "scripts\Prepare-StorePayload.ps1") `
            -RepoRoot $repo `
            -OutputRoot $output `
            -StorePackageScript $packageScript
        Assert-Equal $result.packageCount 1 "The staged package count is wrong."
        Assert-True (Test-Path -LiteralPath (Join-Path $output "StorePackage.ps1") -PathType Leaf) `
            "The target Store deployment script was not staged."
        Assert-True (Test-Path -LiteralPath (
            Join-Path $output "bin\Release\WinStore2\net10.0-windows10.0.28000.0\x64\AppPackages\Store.msix"
        ) -PathType Leaf) "The Store package layout changed while staging."
        Assert-True (Test-Path -LiteralPath (Join-Path $output "StorePackageSigner.cer") -PathType Leaf) `
            "The Store signing certificate was not staged."
        $signature = Get-AuthenticodeSignature -LiteralPath $result.signedMsix
        Assert-Equal $signature.SignerCertificate.Thumbprint $result.signerThumbprint `
            "The staged Store package signer does not match the exported certificate."
    }
    finally {
        if (Test-Path -LiteralPath $testRoot) {
            Remove-Item -LiteralPath $testRoot -Recurse -Force
        }
    }
}

Invoke-Test "SDK prerequisite helper accepts an existing exact SDK" {
    $testRoot = Join-Path $harnessRoot ".artifacts\tests\sdk-helper-$([Guid]::NewGuid().ToString('N'))"
    $repo = Join-Path $testRoot "repo"
    $installRoot = Join-Path $testRoot "dotnet"
    try {
        [IO.Directory]::CreateDirectory((Join-Path $installRoot "sdk\1.2.3")) | Out-Null
        [IO.Directory]::CreateDirectory($repo) | Out-Null
        [IO.File]::WriteAllText(
            (Join-Path $repo "global.json"),
            '{"sdk":{"version":"1.2.3"}}',
            [Text.UTF8Encoding]::new($false))
        $result = & (Join-Path $harnessRoot "scripts\Ensure-DotNetSdk.ps1") `
            -RepoRoot $repo `
            -InstallDirectory $installRoot
        Assert-Equal $result.version "1.2.3" "The SDK helper selected the wrong version."
        Assert-Equal $result.dotnetRoot ([IO.Path]::GetFullPath($installRoot)) "The SDK helper selected the wrong root."
        Assert-True (-not $result.installed) "The SDK helper reinstalled an existing exact SDK."
    }
    finally {
        if (Test-Path -LiteralPath $testRoot) {
            Remove-Item -LiteralPath $testRoot -Recurse -Force
        }
    }
}

Invoke-Test "artifact paths resolve relative to the config" {
    $configuration = Read-ProfilerHarnessConfiguration $configPath
    $context = & $module {
        param($Configuration)
        New-HarnessContext $Configuration $true ""
    } $configuration
    Assert-Equal $context.ArtifactsRoot (Join-Path $harnessRoot ".artifacts") "Artifacts root resolved incorrectly."
}

Invoke-Test "profiler SDK resolver escapes the repo-local SDK" {
    $repoRoot = [IO.Path]::GetFullPath((Join-Path $harnessRoot "..\..\.."))
    $repoDotNetRoot = Join-Path $repoRoot ".dotnet"
    $profilerGlobalJson = Get-Content -LiteralPath (Join-Path $harnessRoot "..\global.json") -Raw |
        ConvertFrom-Json
    $requiredSdkVersion = [string]$profilerGlobalJson.sdk.version
    $oldDotNetRoot = $env:DOTNET_ROOT
    $oldPath = $env:PATH
    try {
        if (Test-Path -LiteralPath (Join-Path $repoDotNetRoot "dotnet.exe") -PathType Leaf) {
            $env:DOTNET_ROOT = $repoDotNetRoot
            $env:PATH = "$repoDotNetRoot;$oldPath"
        }
        $resolved = & $module {
            param($RequiredSdkVersion)
            Resolve-HarnessDotNet -RequiredSdkVersion $RequiredSdkVersion
        } $requiredSdkVersion
        $sdks = & $resolved --list-sdks
        Assert-True (($sdks -join "`n") -match "(?m)^$([Regex]::Escape($requiredSdkVersion))\s+\[") `
            "The resolved dotnet does not contain SDK $requiredSdkVersion."
    }
    finally {
        $env:DOTNET_ROOT = $oldDotNetRoot
        $env:PATH = $oldPath
    }
}

Invoke-Test "invalid topic branch is rejected" {
    $configuration = Get-Content -LiteralPath $configPath -Raw -Encoding UTF8 | ConvertFrom-Json -Depth 100
    $configuration.repositories.app.branch = "main"
    Assert-Throws {
        & $module {
            param($Configuration)
            Test-ProfilerHarnessConfiguration $Configuration
        } $configuration
    } "*must match user/<alias>/profiler_harness*"
}

Invoke-Test "attempt count is capped at five" {
    $configuration = Get-Content -LiteralPath $configPath -Raw -Encoding UTF8 | ConvertFrom-Json -Depth 100
    $configuration.optimization.maxAttempts = 6
    Assert-Throws {
        & $module {
            param($Configuration)
            Test-ProfilerHarnessConfiguration $Configuration
        } $configuration
    } "*maxAttempts must be between 1 and 5*"
}

Invoke-Test "command tokens preserve Windows paths" {
    $tokens = @{ appRepo = "Q:\Repo\App"; runDirectory = "Q:\Runs\One" }
    $expanded = & $module {
        param($Value, $Tokens)
        Expand-HarnessTokens $Value $Tokens
    } "{appRepo}\build.ps1 --out {runDirectory}" $tokens
    Assert-Equal $expanded "Q:\Repo\App\build.ps1 --out Q:\Runs\One" "Token expansion failed."
}

Invoke-Test "timed out commands persist partial output" {
    $testRoot = Join-Path $harnessRoot ".artifacts\tests"
    [IO.Directory]::CreateDirectory($testRoot) | Out-Null
    $logBase = Join-Path $testRoot "timeout"
    $pwsh = (Get-Command "pwsh" -CommandType Application -ErrorAction Stop).Source
    try {
        Assert-Throws {
            & $module {
                param($Pwsh, $LogBase)
                Invoke-HarnessProcess -File $Pwsh -Arguments @(
                    "-NoLogo",
                    "-NoProfile",
                    "-Command",
                    "[Console]::Out.Write('partial-out'); [Console]::Error.Write('partial-err'); Start-Sleep -Seconds 5"
                ) -TimeoutSeconds 1 -LogBasePath $LogBase
            } $pwsh $logBase
        } "*exceeded its 1 second timeout*"
        Assert-Equal ([IO.File]::ReadAllText("$logBase.stdout.log")) "partial-out" "Timed-out stdout was not persisted."
        Assert-Equal ([IO.File]::ReadAllText("$logBase.stderr.log")) "partial-err" "Timed-out stderr was not persisted."
    }
    finally {
        Remove-Item -LiteralPath "$logBase.stdout.log" -Force -ErrorAction SilentlyContinue
        Remove-Item -LiteralPath "$logBase.stderr.log" -Force -ErrorAction SilentlyContinue
    }
}

Invoke-Test "failed capture status propagates and preserves evidence" {
    $testRoot = Join-Path $harnessRoot ".artifacts\tests\capture-result-$([Guid]::NewGuid().ToString('N'))"
    $resultPath = Join-Path $testRoot "capture-result.json"
    try {
        [IO.Directory]::CreateDirectory($testRoot) | Out-Null
        [IO.File]::WriteAllText(
            $resultPath,
            '{"status":"failed","error":"target exited"}',
            [Text.UTF8Encoding]::new($false))
        Assert-Throws {
            & $module {
                param($ResultPath)
                Resolve-HarnessCaptureResult $ResultPath $false ([InvalidOperationException]::new("driver exit 1"))
            } $resultPath
        } "*Evidence was preserved at '$resultPath'*target exited*driver exit 1*"
        Assert-True (Test-Path -LiteralPath $resultPath -PathType Leaf) "Failed capture evidence was removed."

        Remove-Item -LiteralPath $resultPath -Force
        Assert-Throws {
            & $module {
                param($ResultPath)
                Resolve-HarnessCaptureResult $ResultPath $false ([InvalidOperationException]::new("driver exit 1"))
            } $resultPath
        } "*driver exit 1*"
    }
    finally {
        if (Test-Path -LiteralPath $testRoot) {
            Remove-Item -LiteralPath $testRoot -Recurse -Force
        }
    }
}

Invoke-Test "both Store scenarios validate" {
    foreach ($path in @($safeScenarioPath, $installScenarioPath)) {
        $scenario = Get-Content -LiteralPath $path -Raw -Encoding UTF8 | ConvertFrom-Json -Depth 100
        Test-AutomationScenario $scenario | Out-Null
    }
}

Invoke-Test "improvement requires clean ETW and recording evidence" {
    $baseline = [pscustomobject]@{
        status = "succeeded"
        metrics = [pscustomobject]@{ drawnFrames = 20; p95DrawMilliseconds = 20.0 }
        captureValidity = [pscustomobject]@{
            etwAvailable = $true
            lostEvents = 0
            droppedEvents = 0
            etwError = $null
            recordingError = $null
        }
    }
    $candidate = [pscustomobject]@{
        status = "succeeded"
        metrics = [pscustomobject]@{ drawnFrames = 20; p95DrawMilliseconds = 18.0 }
        captureValidity = [pscustomobject]@{
            etwAvailable = $true
            lostEvents = 0
            droppedEvents = 0
            etwError = $null
            recordingError = $null
        }
    }
    $optimization = [pscustomobject]@{
        primaryMetric = "p95DrawMilliseconds"
        direction = "lower"
        minimumImprovementPercent = 5
    }
    $evaluation = Test-HarnessImprovement $baseline $candidate $optimization
    Assert-True $evaluation.improved "A valid ten-percent improvement was not accepted."

    $candidate.captureValidity.etwAvailable = $false
    $invalidEvaluation = Test-HarnessImprovement $baseline $candidate $optimization
    Assert-True (-not $invalidEvaluation.improved) "A capture without ETW was accepted."
}

Invoke-Test "duplicate patches are rejected" {
    $testRoot = Join-Path $harnessRoot ".artifacts\tests"
    [IO.Directory]::CreateDirectory($testRoot) | Out-Null
    $patch = Join-Path $testRoot "duplicate.patch"
    [IO.File]::WriteAllText($patch, "diff --git a/a b/a`n", [Text.UTF8Encoding]::new($false))
    try {
        $signatures = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
        $first = & $module {
            param($Signatures, $Patch)
            Add-HarnessPatchSignature $Signatures $Patch
        } $signatures $patch
        $second = & $module {
            param($Signatures, $Patch)
            Add-HarnessPatchSignature $Signatures $Patch
        } $signatures $patch
        Assert-True $first "The first patch signature was not accepted."
        Assert-True (-not $second) "A duplicate patch signature was accepted."
    }
    finally {
        Remove-Item -LiteralPath $patch -Force -ErrorAction SilentlyContinue
    }
}

Write-Output "PASS: $script:passed harness tests"
