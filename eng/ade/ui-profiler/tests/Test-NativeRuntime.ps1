#Requires -Version 7
# Offline synthetic PE/resource fixtures; no native build, app launch, or performance claim.
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
Import-Module "$PSScriptRoot\..\pack\tools\ProfilerHarness.psm1" -Force -DisableNameChecking
Import-Module "$PSScriptRoot\..\pack\tools\NativeRuntime.psm1" -Force -DisableNameChecking
$module = Get-Module NativeRuntime
$scratch = Join-Path $PSScriptRoot ('.native-runtime-fixture-' + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory $scratch | Out-Null
$passed = 0
function Expect([bool]$Condition, [string]$Message) {
    if (!$Condition) { throw $Message }
}
function Expect-Failure([scriptblock]$Action, [string]$Pattern) {
    try { & $Action | Out-Null } catch {
        if ($_.Exception.Message -notmatch $Pattern) { throw "Unexpected failure: $_ (expected $Pattern)" }
        return
    }
    throw "Expected failure: $Pattern"
}
function New-SyntheticPe([string]$Path, [int]$Machine = 0x8664) {
    $bytes = [byte[]]::new(256)
    $bytes[0] = 0x4d; $bytes[1] = 0x5a
    [BitConverter]::GetBytes([uint32]0x80).CopyTo($bytes, 0x3c)
    [BitConverter]::GetBytes([uint32]0x4550).CopyTo($bytes, 0x80)
    [BitConverter]::GetBytes([uint16]$Machine).CopyTo($bytes, 0x84)
    [IO.File]::WriteAllBytes($Path, $bytes)
}
function New-Payload([string]$Root) {
    foreach ($name in @('Microsoft.UI.Xaml.dll', 'Microsoft.UI.Xaml.Controls.dll',
        'Microsoft.UI.Xaml.Phone.dll', 'Microsoft.UI.Xaml.Internal.dll', 'WinUIEdit.dll',
        'Microsoft.UI.Xaml.resources.common.dll', 'Microsoft.UI.Xaml.resources.19h1.dll',
        'Microsoft.UI.Xaml.Controls.pri', 'en-us\Microsoft.UI.Xaml.dll.mui',
        'en-us\Microsoft.UI.Xaml.Phone.dll.mui', 'Microsoft.UI.Xaml\Themes\generic.xbf',
        'Microsoft.UI.Xaml\Assets\NoiseAsset_256X256_PNG.png', 'Microsoft.UI.Xaml\Assets\Map.html')) {
        $path = Join-Path $Root $name
        New-Item -ItemType Directory -Force (Split-Path $path -Parent) | Out-Null
        if ([IO.Path]::GetExtension($name) -eq '.dll') { New-SyntheticPe $path }
        else { [IO.File]::WriteAllText($path, "SYNTHETIC packaged resource: $name") }
    }
}
function Save-Manifest([string]$Root, [string]$Path) {
    $closure = & $module { param($r) @(Get-NativeClosure $r | Sort-Object path) } $Root
    Write-JsonFile $Path @{
        schema = 'ui-profiler/native-runtime/v1'; architecture = 'x64'; configuration = 'Release'
        flavor = 'amd64fre'; level = 'control'; payloadDirectory = $Root
        source = @{ path = $scratch; revision = ('a' * 40); sha256 = ('b' * 64) }
        expectedRuntimeFile = 'Microsoft.UI.Xaml.dll'; xbfMode = 'embedded-in-pri'; files = @($closure)
    }
}
function New-PrivateApp([string]$Name, [string]$Payload) {
    $original = Join-Path $scratch "$Name-original"
    $deployment = Join-Path $scratch "$Name-private"
    Copy-Deployment $Payload $original
    New-SyntheticPe (Join-Path $original 'app.exe')
    # Make the original runtime different without altering its synthetic PE header.
    [IO.File]::AppendAllText((Join-Path $original 'Microsoft.UI.Xaml.dll'), 'old app runtime')
    Copy-Deployment $original $deployment
    Write-JsonFile (Join-Path $deployment '.ui-profiler-private-deployment.json') @{
        schema = 'ui-profiler/private-deployment/v1'; originalDirectory = $original; executable = 'app.exe'
    }
    @{ original = $original; deployment = $deployment }
}
function New-UninitializedSource([string]$Source, [string]$Name) {
    $target = Join-Path $scratch $Name
    New-Item -ItemType Directory $target | Out-Null
    foreach ($item in Get-ChildItem -LiteralPath $Source -Force |
        Where-Object Name -NotIn @('BuildOutput', 'packages', '.tools')) {
        Copy-Item -LiteralPath $item.FullName -Destination $target -Recurse -Force
    }
    $target
}
try {
    foreach ($level in @('control', 'framework')) {
        $plan = Get-NativeBuildPlan $level
        Expect ($plan.flavor -eq 'amd64fre' -and $plan.configuration -eq 'Release') 'Native flavor drifted.'
        Expect ($plan.commands.Count -eq 2 -and $plan.commands[0] -contains '/nomock') 'Missing fresh product closure bootstrap.'
        Expect ($plan.commands[1] -contains '/q' -and $plan.commands[1] -contains '/verbosity:quiet') 'Build is not quiet.'
        Expect ($plan.commands[1] -contains $plan.project) 'Missing fixed project.'
        Expect (($plan.initializationCommand -join ' ') -eq
            '-NoLogo -NoProfile -NonInteractive -File .\init.ps1 amd64fre') 'Initialization command is not fixed.'
    }
    Expect-Failure { Get-NativeBuildPlan 'arbitrary-command' } 'ValidateSet|does not belong'
    $passed++

    $repo = Join-Path $scratch 'source'
    New-Item -ItemType Directory $repo | Out-Null
    Expect-Failure { Invoke-NativeBuild $repo control (Join-Path $scratch 'missing-prereqs') } 'prerequisite missing'
    $passed++

    # Only the version resource reader is mocked: synthetic fixtures are not real loadable DLLs.
    & $module { function script:Get-NativeFileVersion([string]$Path) { '1.2.3.4' } }
    foreach ($name in @('initrun.ps1', 'init.ps1', 'init.cmd', 'build.cmd',
        'controls\dev\dll\Microsoft.UI.Xaml.Controls.vcxproj')) {
        $path = Join-Path $repo $name
        New-Item -ItemType Directory -Force (Split-Path $path -Parent) | Out-Null
        [IO.File]::WriteAllText($path, '# synthetic build input, never executed')
    }
    foreach ($name in @('packages', '.tools')) { New-Item -ItemType Directory (Join-Path $repo $name) | Out-Null }
    [IO.File]::WriteAllText((Join-Path $repo '.gitignore'), "BuildOutput/`npackages/`n.tools/`n")
    git -C $repo init --quiet
    git -C $repo -c core.autocrlf=false add --all
    git -C $repo -c user.name='Offline Native Test' -c user.email='test@example.invalid' `
        -c commit.gpgsign=false commit --quiet -m "Synthetic native test fixture`n`nCo-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>`nCopilot-Session: fe7e0679-84a3-41a3-9fa9-3342fe4afcda"
    if ($LASTEXITCODE -ne 0) { throw 'Could not initialize synthetic source fixture.' }
    $seed = Join-Path $scratch 'build-seed'
    New-Payload $seed
    & $module {
        param($seedPath)
        $script:nativeTestSeed = $seedPath
        $script:nativeTestCalls = @()
        function script:Invoke-LoggedProcess($Executable, $Arguments, $Directory, $LogPath, $TimeoutSeconds) {
            $script:nativeTestCalls += ,@($Arguments)
            if ($TimeoutSeconds -lt 300) { throw 'Unbounded/short native build timeout.' }
            if ($script:nativeTestCalls.Count -eq 1) {
                Copy-Deployment $script:nativeTestSeed (Join-Path $Directory 'BuildOutput\packaging\Release\runtimes-framework\win-x64\native')
            }
            [IO.File]::WriteAllText($LogPath, 'Mock only: native compiler was not invoked.')
        }
    } $seed
    $built = Invoke-NativeBuild -RepositoryPath $repo -Level control -Directory (Join-Path $scratch 'build-audit')
    $builtManifest = Read-JsonFile $built.path
    Expect ((Get-Sha256 $built.path) -eq $built.sha256) 'Build manifest hash missing.'
    Expect ($builtManifest.source.sha256 -eq (Get-SourceHash $repo)) 'Build source provenance differs.'
    Expect ($builtManifest.files.Count -gt 3) 'Build published a DLL-only closure.'
    Expect (($builtManifest.files | Where-Object path -eq 'Microsoft.UI.Xaml.dll').source -eq
        (Join-Path $scratch 'build-audit\payload\Microsoft.UI.Xaml.dll')) 'Runtime source points to mutable BuildOutput.'
    Expect ((& $module { $script:nativeTestCalls.Count }) -eq 2) 'Fixed build stages did not run.'
    Expect-Failure { Invoke-NativeBuild $repo control (Join-Path $scratch 'stale-build') } 'fresh isolated BuildOutput'
    $passed++

    $failedRepo = Join-Path $scratch 'failed-source'
    New-Item -ItemType Directory $failedRepo | Out-Null
    foreach ($item in Get-ChildItem -LiteralPath $repo -Force | Where-Object Name -ne 'BuildOutput') {
        Copy-Item -LiteralPath $item.FullName -Destination $failedRepo -Recurse -Force
    }
    & $module {
        $script:nativeTestSuccessfulProcess = ${function:Invoke-LoggedProcess}
        function script:Invoke-LoggedProcess { throw 'Synthetic compiler failure; no build occurred.' }
    }
    $failedDirectory = Join-Path $scratch 'failed-build'
    Expect-Failure { Invoke-NativeBuild $failedRepo framework $failedDirectory } 'source project missing'
    New-Item -ItemType Directory -Force (Join-Path $failedRepo 'dxaml\xcp\dxaml\dllsrv\winrt\native') | Out-Null
    [IO.File]::WriteAllText((Join-Path $failedRepo 'dxaml\xcp\dxaml\dllsrv\winrt\native\Microsoft.ui.xaml.vcxproj'), '# synthetic')
    Expect-Failure { Invoke-NativeBuild $failedRepo framework $failedDirectory } 'Synthetic compiler failure'
    Expect (!(Test-Path -LiteralPath (Join-Path $failedDirectory 'native-runtime.json'))) 'Failed build published a success manifest.'
    & $module { Set-Item Function:script:Invoke-LoggedProcess $script:nativeTestSuccessfulProcess }
    $passed++

    & $module {
        $script:nativeTestCalls = @()
        function script:Invoke-LoggedProcess($Executable, $Arguments, $Directory, $LogPath, $TimeoutSeconds) {
            & $script:nativeTestSuccessfulProcess @PSBoundParameters
            [IO.File]::AppendAllText((Join-Path $Directory 'initrun.ps1'), '# synthetic build changed source')
        }
    }
    $changedSourceDirectory = Join-Path $scratch 'source-parity-build'
    Expect-Failure { Invoke-NativeBuild $failedRepo framework $changedSourceDirectory } 'modified source inputs'
    Expect (!(Test-Path -LiteralPath (Join-Path $changedSourceDirectory 'native-runtime.json'))) 'Source-mutating build published a success manifest.'
    $passed++

    & $module {
        function script:Invoke-LoggedProcess($Executable, $Arguments, $Directory, $LogPath, $TimeoutSeconds) {
            $script:nativeTestCalls += ,@($Arguments)
            if ($TimeoutSeconds -lt 300) { throw 'Initialization/build timeout is too short.' }
            $call = $script:nativeTestCalls.Count
            $missing = "ERROR: init.ps1 amd64fre /envcheck failed: synthetic missing prerequisites`nRun a full init first: .\init.ps1 amd64fre`n"
            if ($script:nativeInitMode -eq 'compiler-fails' -and $call -eq 1) {
                [IO.File]::WriteAllText($LogPath, "error C1234: synthetic compile error`nRun a full init first: .\init.ps1 amd64fre`n")
                throw 'Synthetic compiler failure'
            }
            if (($call -eq 1 -and $script:nativeInitMode -ne 'late-missing') -or
                ($call -eq 3 -and $script:nativeInitMode -eq 'retry-fails') -or
                ($call -eq 2 -and $script:nativeInitMode -eq 'late-missing')) {
                [IO.File]::WriteAllText($LogPath, $missing)
                throw 'Synthetic initrun missing initialization'
            }
            if ($Arguments[4] -eq '.\init.ps1') {
                [IO.File]::WriteAllText($LogPath, 'Mock only: init did not run or download anything.')
                if ($script:nativeInitMode -eq 'init-fails') { throw 'Synthetic initialization failure' }
                foreach ($name in @('packages', '.tools')) {
                    New-Item -ItemType Directory (Join-Path $Directory $name) | Out-Null
                }
                return
            }
            [IO.File]::WriteAllText($LogPath, 'Mock only: native compiler was not invoked.')
            if ($Arguments -contains '.\build.cmd') {
                Copy-Deployment $script:nativeTestSeed (Join-Path $Directory 'BuildOutput\packaging\Release\runtimes-framework\win-x64\native')
            }
        }
    }
    $recoverySource = New-UninitializedSource $repo 'uninitialized-source'
    & $module { $script:nativeInitMode = 'recover'; $script:nativeTestCalls = @() }
    $recovered = Invoke-NativeBuild $recoverySource control (Join-Path $scratch 'initialization-recovery')
    $recoveredManifest = Read-JsonFile $recovered.path
    Expect ((& $module { $script:nativeTestCalls.Count }) -eq 4) 'Expected build, one init, one retry, then scoped build.'
    Expect $recoveredManifest.initialization.performed 'Successful prerequisite initialization was not audited.'
    Expect ($recoveredManifest.logs.Count -eq 4) 'Initial failure, init, retry and scoped build logs were not preserved.'
    foreach ($log in $recoveredManifest.logs) { Assert-FileHash $log.path $log.sha256 }
    $recoveryCalls = & $module { ,$script:nativeTestCalls }
    Expect (($recoveryCalls[1] -join ' ') -eq
        '-NoLogo -NoProfile -NonInteractive -File .\init.ps1 amd64fre') 'Init used model-controlled arguments.'
    Expect (($recoveryCalls[0] -join ' ') -eq ($recoveryCalls[2] -join ' ')) 'Retry changed the approved build command.'
    Expect ($recoveredManifest.source.sha256 -eq (Get-SourceHash $recoverySource)) 'Initialization invalidated source provenance.'
    $passed++

    foreach ($case in @(
        @{ mode = 'init-fails'; expectedCalls = 2; failure = 'initialization failure' },
        @{ mode = 'retry-fails'; expectedCalls = 3; failure = 'missing initialization' },
        @{ mode = 'compiler-fails'; expectedCalls = 1; failure = 'compiler failure' },
        @{ mode = 'late-missing'; expectedCalls = 2; failure = 'missing initialization' }
    )) {
        $caseSource = New-UninitializedSource $repo "$($case.mode)-source"
        $caseDirectory = Join-Path $scratch "$($case.mode)-audit"
        & $module { param($mode) $script:nativeInitMode = $mode; $script:nativeTestCalls = @() } $case.mode
        Expect-Failure { Invoke-NativeBuild $caseSource control $caseDirectory } $case.failure
        Expect ((& $module { $script:nativeTestCalls.Count }) -eq $case.expectedCalls) 'An unexpected build/initialization retry occurred.'
        Expect (!(Test-Path -LiteralPath (Join-Path $caseDirectory 'native-runtime.json'))) 'Failed initialization/build published a runtime manifest.'
        $passed++
    }

    $audit = Join-Path $scratch 'audit'
    $payload = Join-Path $audit 'payload'
    New-Payload $payload
    [IO.File]::WriteAllText((Join-Path $payload 'unrelated.dll'), 'not a native runtime file')
    [IO.File]::WriteAllText((Join-Path $payload 'Microsoft.UI.Xaml.pdb'), 'not deployable')
    $manifestPath = Join-Path $audit 'native-runtime.json'
    Save-Manifest $payload $manifestPath
    $manifest = Read-JsonFile $manifestPath
    Expect ($manifest.files.path -contains 'Microsoft.UI.Xaml\Themes\generic.xbf') 'Loose compiled XAML lost.'
    Expect ($manifest.files.path -contains 'Microsoft.UI.Xaml.Controls.pri') 'Compiled PRI lost.'
    Expect ($manifest.files.path -notcontains 'unrelated.dll') 'Non-WinUI runtime was selected.'
    Expect ($manifest.files.path -notcontains 'Microsoft.UI.Xaml.pdb') 'Debug symbols were selected.'
    Expect (($manifest.files | Where-Object path -eq 'Microsoft.UI.Xaml\Assets\Map.html').role -eq 'prerequisite') 'Asset would be overwritten.'
    $embeddedOnly = @($manifest.files | Where-Object { [IO.Path]::GetExtension($_.path) -ne '.xbf' })
    & $module { param($files) Assert-NativeClosure $files } $embeddedOnly
    $missingTabularPri = @($manifest.files) + @([pscustomobject]@{ path = 'Microsoft.UI.Xaml.Controls.Tabular.dll' })
    Expect-Failure { & $module { param($files) Assert-NativeClosure $files } $missingTabularPri } 'Tabular.pri'
    $passed++

    $app = New-PrivateApp 'good' $payload
    $originalHash = Get-Sha256 (Join-Path $app.original 'Microsoft.UI.Xaml.dll')
    $receipt = Install-NativeRuntime -DeploymentDirectory $app.deployment -ManifestPath $manifestPath
    Expect ($receipt.expectedRuntimePath -eq (Join-Path $app.deployment 'Microsoft.UI.Xaml.dll')) 'Expected module path missing.'
    Expect ((Get-Sha256 (Join-Path $app.original 'Microsoft.UI.Xaml.dll')) -eq $originalHash) 'Original app was altered.'
    $row = $receipt.files | Where-Object path -eq 'Microsoft.UI.Xaml.dll'
    Expect ($row.originalSha256 -eq $originalHash -and $row.replacementSha256 -ne $originalHash) 'Replacement hashes not audited.'
    $null = Assert-NativeRuntime $app.deployment $manifestPath
    Expect-Failure { Install-NativeRuntime $app.deployment $manifestPath } 'already staged'
    $passed++

    $xbf = Join-Path $app.deployment 'Microsoft.UI.Xaml\Themes\generic.xbf'
    [IO.File]::AppendAllText($xbf, 'tampered')
    Expect-Failure { Assert-NativeRuntime $app.deployment $manifestPath } 'changed'
    Copy-Item -LiteralPath (Join-Path $payload 'Microsoft.UI.Xaml\Themes\generic.xbf') -Destination $xbf -Force
    New-SyntheticPe (Join-Path $app.deployment 'Microsoft.UI.Xaml.extra.dll')
    Expect-Failure { Assert-NativeRuntime $app.deployment $manifestPath } 'closure changed'
    $passed++

    $bad = New-PrivateApp 'architecture' $payload
    New-SyntheticPe (Join-Path $bad.original 'Microsoft.UI.Xaml.Controls.dll') 0x14c
    New-SyntheticPe (Join-Path $bad.deployment 'Microsoft.UI.Xaml.Controls.dll') 0x14c
    Expect-Failure { Install-NativeRuntime $bad.deployment $manifestPath } 'architecture/version'
    Expect ((Get-Sha256 (Join-Path $bad.deployment 'Microsoft.UI.Xaml.dll')) -eq
        (Get-Sha256 (Join-Path $bad.original 'Microsoft.UI.Xaml.dll'))) 'Preflight failure partially overlaid deployment.'
    $passed++

    $bad = New-PrivateApp 'assets' $payload
    $asset = 'Microsoft.UI.Xaml\Assets\Map.html'
    [IO.File]::WriteAllText((Join-Path $bad.original $asset), 'incompatible')
    [IO.File]::WriteAllText((Join-Path $bad.deployment $asset), 'incompatible')
    Expect-Failure { Install-NativeRuntime $bad.deployment $manifestPath } 'asset/MUI is incompatible'
    $passed++

    $bad = New-PrivateApp 'version' $payload
    & $module {
        function script:Get-NativeFileVersion([string]$Path) {
            if ($Path -match 'version-private') { '9.0.0.0' } else { '1.2.3.4' }
        }
    }
    Expect-Failure { Install-NativeRuntime $bad.deployment $manifestPath } 'architecture/version'
    & $module { function script:Get-NativeFileVersion([string]$Path) { '1.2.3.4' } }
    $passed++

    $bad = New-PrivateApp 'not-private' $payload
    Write-JsonFile (Join-Path $bad.deployment '.ui-profiler-private-deployment.json') @{
        schema = 'ui-profiler/private-deployment/v1'; originalDirectory = $bad.deployment; executable = 'app.exe'
    }
    Expect-Failure { Install-NativeRuntime $bad.deployment $manifestPath } 'must be disjoint'
    $passed++

    $changed = Read-JsonFile $manifestPath
    $changed.files[0].path = '..\outside.dll'
    Write-JsonFile $manifestPath $changed
    Expect-Failure { Install-NativeRuntime $app.original $manifestPath } 'Incomplete|Invalid runtime'
    Save-Manifest $payload $manifestPath
    $changed = Read-JsonFile $manifestPath
    $changed.files = @($changed.files | Where-Object path -ne 'Microsoft.UI.Xaml.Controls.pri')
    Write-JsonFile $manifestPath $changed
    Expect-Failure { Install-NativeRuntime $app.original $manifestPath } 'Incomplete'
    Save-Manifest $payload $manifestPath
    [IO.File]::AppendAllText((Join-Path $payload 'Microsoft.UI.Xaml.Controls.pri'), 'tampered')
    Expect-Failure { Install-NativeRuntime $app.original $manifestPath } 'payload changed'
    $passed++

    Write-Host "PASS: $passed native-runtime offline test groups (synthetic artifacts; no native execution)."
} finally {
    Remove-Item -LiteralPath $scratch -Recurse -Force
    Remove-Module NativeRuntime
}
