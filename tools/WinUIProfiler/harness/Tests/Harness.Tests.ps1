[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"
$harnessRoot = Split-Path -Parent $PSScriptRoot
. (Join-Path $harnessRoot "scripts\Assert-GuestExecution.ps1")
$modulePath = Join-Path $harnessRoot "ProfilerHarness.psm1"
$configPath = Join-Path $harnessRoot "examples\store.hyperv.json"
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
    foreach ($path in Get-ChildItem -LiteralPath $harnessRoot -File -Recurse |
        Where-Object Extension -In @(".ps1", ".psm1")) {
        $tokens = $null
        $errors = $null
        [Management.Automation.Language.Parser]::ParseFile($path.FullName, [ref]$tokens, [ref]$errors) | Out-Null
        Assert-True ($errors.Count -eq 0) "PowerShell parse errors in '$($path.FullName)': $errors"
    }
}

Invoke-Test "IXP prerequisite confirmations accept yes and y regardless of case" {
    $tokens = $null
    $errors = $null
    $path = Join-Path $harnessRoot "scripts\Initialize-IxpVm.ps1"
    $ast = [Management.Automation.Language.Parser]::ParseFile($path, [ref]$tokens, [ref]$errors)
    Assert-True ($errors.Count -eq 0) "PowerShell parse errors in '$path': $errors"
    $confirmation = $ast.Find({
        param($node)
        $node -is [Management.Automation.Language.FunctionDefinitionAst] -and $node.Name -eq "Confirm-Step"
    }, $true)
    Assert-True ($null -ne $confirmation) "IXP prerequisite confirmation function was not found."

    & {
        param($definition)
        . ([scriptblock]::Create($definition.Extent.Text))
        function Read-Host {
            param([string]$Prompt)
            return $response
        }
        foreach ($response in @("yes", "YES", "y", "Y")) {
            Confirm-Step "Approve this prerequisite?"
        }
        foreach ($response in @("", "no", "n", "yesterday", "yes please")) {
            Assert-Throws { Confirm-Step "Approve this prerequisite?" } "*Provisioning stopped by the user*"
        }
    } $confirmation
}

Invoke-Test "local execution and host processes are rejected before side effects" {
    $context = [pscustomobject]@{
        Config = [pscustomobject]@{ target = [pscustomobject]@{ kind = "local" } }
        DryRun = $false
    }
    Assert-Throws {
        & $module { param($Context) Assert-HarnessVmTarget $Context } $context
    } "*Devbox execution is disabled*"
    Assert-Throws {
        & $module { Invoke-HarnessProcess -File "pwsh" }
    } "*Host process execution is disabled*"
    Assert-Throws {
        & $module {
            param($Context)
            Invoke-HarnessAgent $context "agency" "test" "C:\test" "test" -Editing
        } $context
    } "*Agents may run only inside the selected VM*"
    Assert-Throws {
        & $module { Invoke-HarnessProcess -File "git" -Arguments @("apply", "change.patch") }
    } "*Host Git operation*is disabled*"
}

Invoke-Test "guest paths cannot escape the dedicated staging directory" {
    $context = [pscustomobject]@{
        Config = [pscustomobject]@{ target = [pscustomobject]@{ stageRoot = "C:\WinUIProfilerHarness" } }
    }
    & $module { param($Context) Assert-HarnessGuestPath $Context "C:\WinUIProfilerHarness\app" } $context
    foreach ($path in @("C:\", "C:\Windows", "C:\WinUIProfilerHarness", "C:\WinUIProfilerHarness-Other\app",
        "C:\WinUIProfilerHarness\..\Windows", "\\host\share\app", "C:\WinUIProfilerHarness\app'bad",
        "C:\WinUIProfilerHarness\file:stream")) {
        Assert-Throws {
            & $module { param($Context, $Path) Assert-HarnessGuestPath $Context $Path } $context $path
        } "*dedicated guest staging directory*"
    }
}

Invoke-Test "schema 1 and host command scopes cannot authorize execution" {
    $configuration = Get-Content -LiteralPath $ixpConfigPath -Raw | ConvertFrom-Json -Depth 100
    $configuration.schemaVersion = 1
    Assert-Throws { Test-ProfilerHarnessConfiguration $configuration } "*schemaVersion 2*"
    $configuration.schemaVersion = 2
    $configuration.application.buildCommands[0].scope = "host"
    Assert-Throws { Test-ProfilerHarnessConfiguration $configuration } "*scope must be guest*"
}

Invoke-Test "example configuration validates" {
    $configuration = Read-ProfilerHarnessConfiguration $configPath
    Assert-Equal $configuration.name "store-hyperv" "Unexpected example name."
    Assert-Equal $configuration.repositories.app.branch "user/akanpatel/profiler_harness" "Unexpected app branch."
    Assert-Equal $configuration.repositories.profiler.branch "user/akanpatel2206/profiler_harness" "Unexpected profiler branch."
}

Invoke-Test "schema rejects provisioning options and host Agency sources" {
    foreach ($field in @("memoryInGb", "agencySourcePath")) {
        $configuration = Get-Content -LiteralPath $ixpConfigPath -Raw | ConvertFrom-Json -Depth 100
        if ($field -eq "memoryInGb") {
            $configuration.target.ixp | Add-Member -NotePropertyName $field -NotePropertyValue 16 -Force
        }
        else {
            $configuration.agents | Add-Member -NotePropertyName $field -NotePropertyValue "C:\HostAgency" -Force
        }
        $valid = Test-Json -Json ($configuration | ConvertTo-Json -Depth 100) `
            -SchemaFile (Join-Path $harnessRoot "config.schema.json") -ErrorAction SilentlyContinue
        Assert-True (-not $valid) "Obsolete '$field' was accepted as a harness execution setting."
    }
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
    $prepareCommand = @($configuration.application.deployCommands.hyperv) |
        Where-Object { (@($_.arguments) -join " ") -like "*Prepare-StorePayload.ps1*" } |
        Select-Object -First 1
    Assert-True ($buildArguments -contains "-BuildOnly") "Store build does not use the NativeAOT build-only workflow."
    Assert-True ($null -ne $prepareCommand) "Signed Store payload preparation is not configured."
    Assert-True ([string]$configuration.application.targetInstallScripts.hyperv -like "*StorePackage.ps1*") `
        "Store deployment must use a guest installation script."
    Assert-True ((($configuration.application | ConvertTo-Json -Depth 20) -notmatch "Invoke-VsBuild|devenv")) `
        "The Store example still contains the obsolete Visual Studio/DTE route."
}

Invoke-Test "Ixp Store example requires a manually provisioned ge_current VM" {
    $configuration = Read-ProfilerHarnessConfiguration $ixpConfigPath
    Assert-Equal $configuration.target.kind "ixp" "Unexpected Ixp target kind."
    Assert-Equal $configuration.target.ixp.name "WinUIProfilerHarness-ge-current-latest" "Unexpected Ixp VM name."
    Assert-Equal $configuration.target.ixp.branch "ge_current" "The Ixp VM is not sourced from ge_current."
    Assert-Equal $configuration.target.ixp.connectionMode "psremote" "The tested Ixp connection mode is not selected."
    Assert-True (-not $configuration.target.ixp.ensureCreated) "The harness must never create an Ixp VM."
    Assert-Equal $configuration.target.ixp.minimumOsBuild 27200 `
        "The ge_current VM configuration does not reject an older incompatible guest."
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
}

Invoke-Test "automatic Ixp creation is rejected" {
    $settings = [pscustomobject]@{
        name = "Harness-Test"
        branch = "ge_current_directwex"
        allowOffline = $true
        enableCdKeys = $true
        velocityKeysToEnable = @("Feature_A", "Feature_B")
        velocityKeysToDisable = @("Feature_C")
    }
    Assert-Throws {
        & $module {
            param($Settings, $ConfigDirectory)
            Get-HarnessIxpCreationParameters $Settings $ConfigDirectory
        } $settings $harnessRoot
    } "*manual provisioning script*"
}

Invoke-Test "invalid Ixp connection mode is rejected" {
    $configuration = Get-Content -LiteralPath $ixpConfigPath -Raw -Encoding UTF8 | ConvertFrom-Json -Depth 100
    $configuration.target.ixp.connectionMode = "invalid"
    Assert-Throws {
        & $module {
            param($Configuration)
            Test-ProfilerHarnessConfiguration $Configuration
        } $configuration
    } "*connectionMode must be psremote*"
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

Invoke-Test "SDK resolution never probes host dotnet installations" {
    Assert-Throws {
        & $module { Resolve-HarnessDotNet -RequiredSdkVersion "10.0.100" }
    } "*SDK resolution is guest-only*"
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

Invoke-Test "evidence transport refuses traversal and executable exports" {
    $destination = Join-Path $harnessRoot ".artifacts\tests\evidence"
    foreach ($name in @("capture-result.json", "snapshots\tree.json", "session\capture.sqlite")) {
        & $module { param($Destination, $Name) Assert-HarnessEvidenceEntry $Destination $Name } $destination $name
    }
    foreach ($name in @("..\escape.json", "Microsoft.UI.Xaml.dll", "run.ps1", "app.exe", "snapshot.json:stream.json")) {
        Assert-Throws {
            & $module { param($Destination, $Name) Assert-HarnessEvidenceEntry $Destination $Name } $destination $name
        } "*Refusing executable or out-of-directory guest evidence*"
    }
    Assert-Throws {
        & $module { param($Destination) Assert-HarnessEvidenceEntry $Destination "..\..\..\escape.json" } $destination
    } "*Host output must remain inside*"
    Assert-Throws {
        & $module { Assert-HarnessHostArtifactPath "C:\Windows\System32\test.dll" }
    } "*Host output must remain inside*"
}

Invoke-Test "identity pins and a clean checkpoint are mandatory" {
    foreach ($name in @("vmId", "guestMachineGuid", "checkpointName", "guestComputerName")) {
        $configuration = Get-Content -LiteralPath $configPath -Raw | ConvertFrom-Json -Depth 100
        $configuration.target.$name = ""
        Assert-Throws { Test-ProfilerHarnessConfiguration $configuration } "*is required*"
    }
    $configuration = Get-Content -LiteralPath $configPath -Raw | ConvertFrom-Json -Depth 100
    $configuration.target.vmId = "00000000-0000-0000-0000-000000000000"
    Assert-Throws { Test-ProfilerHarnessConfiguration $configuration } "*nonempty GUID*"
    $configuration = Get-Content -LiteralPath $ixpConfigPath -Raw | ConvertFrom-Json -Depth 100
    $configuration.target.ixp.ensureCreated = $true
    Assert-Throws { Test-ProfilerHarnessConfiguration $configuration } "*Automatic IXP creation is disabled*"
}

Invoke-Test "guest deployments reject overlapping paths before execution" {
    $context = [pscustomobject]@{
        Config = [pscustomobject]@{ target = [pscustomobject]@{ stageRoot = "C:\WinUIProfilerHarness" } }
    }
    Assert-Throws {
        & $module {
            param($Context)
            Copy-HarnessGuestDirectory $Context "C:\WinUIProfilerHarness\app" "C:\WinUIProfilerHarness\app\output"
        } $context
    } "*must not overlap*"
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

        [IO.File]::WriteAllText($resultPath, '{"status":"succeeded"}', [Text.UTF8Encoding]::new($false))
        Assert-Throws {
            & $module {
                param($ResultPath)
                Resolve-HarnessCaptureResult $ResultPath $false ([InvalidOperationException]::new("driver exit 1"))
            } $resultPath
        } "*driver exit 1*"

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

Invoke-Test "empty private snapshots cannot establish readiness" {
    $testRoot = Join-Path $harnessRoot ".artifacts\tests\empty-snapshot-$([Guid]::NewGuid().ToString('N'))"
    $resultPath = Join-Path $testRoot "capture-result.json"
    try {
        [IO.Directory]::CreateDirectory($testRoot) | Out-Null
        [IO.File]::WriteAllText($resultPath, @'
{"status":"succeeded","captureValidity":{"etwAvailable":true,"lostEvents":0,"droppedEvents":0},
"beforeSnapshot":{"path":"C:\\guest\\before.json","roots":0,"xamlNodes":0,"compositionNodes":0},
"afterSnapshot":{"path":"C:\\guest\\after.json","roots":0,"xamlNodes":0,"compositionNodes":0}}
'@, [Text.UTF8Encoding]::new($false))
        Assert-Throws {
            & $module { param($Path) Resolve-HarnessCaptureResult $Path $false } $resultPath
        } "*no captured roots or nodes*"
    }
    finally { Remove-Item -LiteralPath $testRoot -Recurse -Force }
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
    $candidate.captureValidity.etwAvailable = "false"
    $invalidEvaluation = Test-HarnessImprovement $baseline $candidate $optimization
    Assert-True (-not $invalidEvaluation.improved) "A nonboolean ETW availability value was accepted."
}

Invoke-Test "improvement must hold across at least three paired reset captures" {
    $optimization = [pscustomobject]@{ primaryMetric = "p95DrawMilliseconds"; direction = "lower"; minimumImprovementPercent = 5 }
    $baseline = @(
        1..3 | ForEach-Object {
            [pscustomobject]@{
                status = "succeeded"
                metrics = [pscustomobject]@{ drawnFrames = 20; p95DrawMilliseconds = 20.0 }
                captureValidity = [pscustomobject]@{ etwAvailable = $true; lostEvents = 0; droppedEvents = 0 }
            }
        }
    )
    $candidate = @(
        1..3 | ForEach-Object {
            [pscustomobject]@{
                status = "succeeded"
                metrics = [pscustomobject]@{ drawnFrames = 20; p95DrawMilliseconds = 18.0 }
                captureValidity = [pscustomobject]@{ etwAvailable = $true; lostEvents = 0; droppedEvents = 0 }
            }
        }
    )
    $evaluation = & $module {
        param($Baseline, $Candidate, $Optimization)
        Test-HarnessRepeatedImprovement $Baseline $Candidate $Optimization
    } $baseline $candidate $optimization
    Assert-True $evaluation.improved "Consistent improvement across three valid captures was rejected."
    $candidate[2].metrics.p95DrawMilliseconds = 21.0
    $evaluation = & $module {
        param($Baseline, $Candidate, $Optimization)
        Test-HarnessRepeatedImprovement $Baseline $Candidate $Optimization
    } $baseline $candidate $optimization
    Assert-True (-not $evaluation.improved) "A regression in one paired sample was accepted."
    Assert-Throws {
        & $module {
            param($Baseline, $Candidate, $Optimization)
            Test-HarnessRepeatedImprovement @($Baseline[0]) @($Candidate[0]) $Optimization
        } $baseline $candidate $optimization
    } "*at least three equally sized*"
}

Invoke-Test "invalid comparison values cannot produce an improvement decision" {
    $baseline = [pscustomobject]@{ metrics = [pscustomobject]@{ p95DrawMilliseconds = 20.0 } }
    $candidate = [pscustomobject]@{ metrics = [pscustomobject]@{ p95DrawMilliseconds = 18.0 } }
    $optimization = [pscustomobject]@{ direction = "invalid"; minimumImprovementPercent = 5 }
    Assert-Throws { Test-HarnessImprovement $baseline $candidate $optimization } "*direction must be lower or higher*"
    $optimization.direction = "lower"
    $optimization.minimumImprovementPercent = -1
    Assert-Throws { Test-HarnessImprovement $baseline $candidate $optimization } "*finite and nonnegative*"
    $optimization.minimumImprovementPercent = 5
    foreach ($value in @([double]::NaN, [double]::PositiveInfinity)) {
        $candidate.metrics.p95DrawMilliseconds = $value
        Assert-Throws { Test-HarnessImprovement $baseline $candidate $optimization } "*must be finite*"
    }
}

Invoke-Test "a zero threshold does not accept unchanged measurements" {
    $capture = [pscustomobject]@{
        status = "succeeded"
        metrics = [pscustomobject]@{ drawnFrames = 20; p95DrawMilliseconds = 20.0 }
        captureValidity = [pscustomobject]@{ etwAvailable = $true; lostEvents = 0; droppedEvents = 0 }
    }
    $optimization = [pscustomobject]@{ direction = "lower"; minimumImprovementPercent = 0 }
    $evaluation = Test-HarnessImprovement $capture $capture $optimization
    Assert-True (-not $evaluation.improved) "Unchanged measurements were reported as an improvement."
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
