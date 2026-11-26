#Requires -Version 7
# Private, source-matched runtime trials only. Initialization is allowed only after
# initrun reports its known missing-prerequisite failure during approved execution.
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
Import-Module "$PSScriptRoot\ProfilerHarness.psm1" -DisableNameChecking

function Get-NativeBuildPlan {
    param([Parameter(Mandatory)][ValidateSet('control', 'framework')][string]$Level)
    $project = if ($Level -eq 'control') {
        'controls\dev\dll\Microsoft.UI.Xaml.Controls.vcxproj'
    } else {
        'dxaml\xcp\dxaml\dllsrv\winrt\native\Microsoft.ui.xaml.vcxproj'
    }
    # A project-only inner loop assumes a complete product build already exists.
    # /nomock avoids component-package installation in PackageStore or a NuGet cache.
    [pscustomobject]@{
        level = $Level; architecture = 'x64'; configuration = 'Release'; flavor = 'amd64fre'
        project = $project
        commands = @(
            @('-NoLogo', '-NoProfile', '-NonInteractive', '-File', '.\initrun.ps1',
                '-Flavor', 'amd64fre', '.\build.cmd', '/q', '/nomock', 'product'),
            @('-NoLogo', '-NoProfile', '-NonInteractive', '-File', '.\initrun.ps1',
                '-Flavor', 'amd64fre', 'msb', '/q', $project, '/nologo', '/verbosity:quiet',
                '/p:Platform=x64', '/p:Configuration=Release', '/p:DisableEmbeddedXbf=false')
        )
        initializationCommand = @('-NoLogo', '-NoProfile', '-NonInteractive',
            '-File', '.\init.ps1', 'amd64fre')
        command = ".\initrun.ps1 -Flavor amd64fre msb /q `"$project`" /nologo /verbosity:quiet /p:Platform=x64 /p:Configuration=Release /p:DisableEmbeddedXbf=false"
        payloadRelativePath = 'BuildOutput\packaging\Release\runtimes-framework\win-x64\native'
        expectedRuntimeFile = 'Microsoft.UI.Xaml.dll'
        xbfMode = 'embedded-in-pri'
        documentation = @('docs\ad-hoc-testing-of-local-build-with-fast-inner-loop.md',
            'eng\packaging.props', 'eng\packaging.targets',
            'controls\dev\dll\Microsoft.UI.Xaml.Common.targets',
            'build\nuspecs\Microsoft.WindowsAppSDK.WinUI.nuspec')
    }
}

function Get-NativeFullPath([string]$Path) {
    if ([string]::IsNullOrWhiteSpace($Path)) { throw 'An explicit path is required.' }
    [IO.Path]::GetFullPath($Path).TrimEnd('\')
}

function Test-NativePathWithin([string]$Path, [string]$Root) {
    $Path.Equals($Root, [StringComparison]::OrdinalIgnoreCase) -or
        $Path.StartsWith($Root.TrimEnd('\') + '\', [StringComparison]::OrdinalIgnoreCase)
}

function Assert-NativeNoLinks([string]$Path) {
    $item = Get-Item -LiteralPath $Path -Force
    while ($null -ne $item) {
        if ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) {
            throw "Private runtime paths must not traverse links: $Path"
        }
        $item = if ($item -is [IO.FileInfo]) { $item.Directory } else { $item.Parent }
    }
}

function Get-NativeChildPath([string]$Root, [string]$RelativePath) {
    if ([string]::IsNullOrWhiteSpace($RelativePath) -or [IO.Path]::IsPathRooted($RelativePath) -or
        $RelativePath -match '(^|[\\/])\.{1,2}([\\/]|$)|[:*?]|[\\/]{2}|[. ]($|[\\/])') {
        throw "Invalid runtime relative path: $RelativePath"
    }
    $path = Get-NativeFullPath (Join-Path $Root $RelativePath)
    if (!(Test-NativePathWithin $path $Root)) { throw 'Runtime path escapes its root.' }
    $path
}

function Get-NativeMachine([string]$Path) {
    $stream = [IO.File]::OpenRead($Path)
    $reader = [IO.BinaryReader]::new($stream)
    try {
        if ($stream.Length -lt 64 -or $reader.ReadUInt16() -ne 0x5a4d) { throw "Not a PE file: $Path" }
        $stream.Position = 0x3c
        $offset = $reader.ReadUInt32()
        if ($offset -lt 64 -or $offset + 24 -gt $stream.Length) { throw "Invalid PE header: $Path" }
        $stream.Position = $offset
        if ($reader.ReadUInt32() -ne 0x4550) { throw "Invalid PE signature: $Path" }
        $reader.ReadUInt16()
    } finally { $reader.Dispose(); $stream.Dispose() }
}

function Get-NativeFileVersion([string]$Path) {
    $info = [Diagnostics.FileVersionInfo]::GetVersionInfo($Path)
    if ([string]::IsNullOrWhiteSpace($info.FileVersion)) { throw "Missing native file version: $Path" }
    '{0}.{1}.{2}.{3}' -f $info.FileMajorPart, $info.FileMinorPart, $info.FileBuildPart, $info.FilePrivatePart
}

function Get-NativeClosure([string]$Root) {
    Assert-NativeNoLinks $Root
    # These are the runtime filters in Microsoft.WindowsAppSDK.WinUI.nuspec, not
    # a search for a convenient DLL in bin/obj. Everything under the XAML tree
    # is retained, including loose XBFs and non-overlaid asset prerequisites.
    foreach ($file in Get-ChildItem -LiteralPath $Root -File -Recurse -Force) {
        Assert-NativeNoLinks $file.FullName
        $relative = [IO.Path]::GetRelativePath($Root, $file.FullName)
        if ($relative -notmatch '^(Microsoft\.UI\.Xaml\.[^\\]+|WinUIEdit\.dll|Microsoft\.UI\.Xaml\\.+|[^\\]+\\Microsoft\.UI\.Xaml\.[^\\]+)$') {
            continue
        }
        if ($file.Extension -in @('.pdb', '.lib', '.exp', '.winmd')) { continue }
        if ($file.Length -eq 0) { throw "Empty runtime artifact: $relative" }
        $overlay = $file.Extension -in @('.dll', '.pri', '.xbf')
        $machine = $null
        $version = $null
        if ($file.Extension -eq '.dll') {
            $machine = Get-NativeMachine $file.FullName
            if ($machine -ne 0x8664) { throw "Runtime architecture is not x64: $relative" }
            $version = Get-NativeFileVersion $file.FullName
        }
        [pscustomobject]@{
            path = $relative
            source = $file.FullName
            sha256 = Get-Sha256 $file.FullName
            role = if ($overlay) { 'overlay' } else { 'prerequisite' }
            machine = $machine; fileVersion = $version
        }
    }
}

function Assert-NativeClosure($Files) {
    $names = @($Files | ForEach-Object path)
    if ($names.Count -eq 0 -or @($names | Sort-Object -Unique).Count -ne $names.Count) {
        throw 'Missing or ambiguous native runtime closure.'
    }
    foreach ($required in @('Microsoft.UI.Xaml.dll', 'Microsoft.UI.Xaml.Controls.dll',
        'Microsoft.UI.Xaml.Controls.pri', 'Microsoft.UI.Xaml.Phone.dll',
        'Microsoft.UI.Xaml.Internal.dll', 'WinUIEdit.dll',
        'Microsoft.UI.Xaml.resources.common.dll', 'Microsoft.UI.Xaml.resources.19h1.dll',
        'en-us\Microsoft.UI.Xaml.dll.mui', 'en-us\Microsoft.UI.Xaml.Phone.dll.mui',
        'Microsoft.UI.Xaml\Assets\NoiseAsset_256X256_PNG.png', 'Microsoft.UI.Xaml\Assets\Map.html')) {
        if ($required -notin $names) { throw "Incomplete packaged WinUI runtime closure: $required" }
    }
    if ('Microsoft.UI.Xaml.Controls.Tabular.dll' -in $names -and
        'Microsoft.UI.Xaml.Controls.Tabular.pri' -notin $names) {
        throw 'Incomplete packaged WinUI runtime closure: Microsoft.UI.Xaml.Controls.Tabular.pri'
    }
}

function Invoke-NativeBuild {
    param(
        [Parameter(Mandatory)][string]$RepositoryPath,
        [Parameter(Mandatory)][ValidateSet('control', 'framework')][string]$Level,
        [Parameter(Mandatory)][Alias('OutputDirectory')][string]$Directory
    )
    $root = Get-NativeFullPath $RepositoryPath
    $output = Get-NativeFullPath $Directory
    $hostRepository = Get-NativeFullPath (Join-Path $PSScriptRoot '..\..\..\..\..')
    if ($root -eq $hostRepository -or (Test-NativePathWithin $output $root) -or
        (Test-NativePathWithin $root $output)) {
        throw 'Native builds require an isolated source snapshot and a disjoint audit directory.'
    }
    # initrun internally constructs a command string. Do not pass shell metacharacters.
    if ($root -match "['`"`r`n;&|<>`$]") { throw 'Unsupported shell characters in isolated repository path.' }
    Assert-NativeNoLinks $root
    foreach ($required in @('initrun.ps1', 'init.ps1', 'init.cmd', 'build.cmd')) {
        if (!(Test-Path -LiteralPath (Join-Path $root $required) -PathType Leaf)) {
            throw "Native build prerequisite missing: $required. Supply a complete isolated source snapshot."
        }
    }
    if (Test-Path -LiteralPath (Join-Path $root 'BuildOutput')) {
        throw 'A fresh isolated BuildOutput is required; existing outputs cannot prove source parity.'
    }
    if (Test-Path -LiteralPath $output) { throw 'Native build audit directory already exists.' }
    Assert-NativeNoLinks (Split-Path $output -Parent)
    $plan = Get-NativeBuildPlan $Level
    if (!(Test-Path -LiteralPath (Join-Path $root $plan.project) -PathType Leaf)) {
        throw "Native source project missing: $($plan.project)"
    }
    $files = @(Get-RepositoryFiles $root)
    $sourceHash = Get-SourceHash $root $files
    $revision = (git -C $root rev-parse --verify HEAD).Trim()
    if ($LASTEXITCODE -ne 0 -or $revision -notmatch '^[0-9a-f]{40,64}$') { throw 'A Git source revision is required.' }
    $pwsh = (Get-Command pwsh -CommandType Application -ErrorAction Stop).Source
    New-Item -ItemType Directory $output | Out-Null
    Write-JsonFile (Join-Path $output 'plan.json') $plan
    $index = 0
    $initialized = $false
    $logs = @()
    foreach ($command in $plan.commands) {
        $index++
        $log = Join-Path $output "build-$index.log"
        $logs += $log
        try {
            $null = Invoke-LoggedProcess -Executable $pwsh -Arguments $command -Directory $root `
                -LogPath $log -TimeoutSeconds 1800
        } catch {
            $text = if (Test-Path -LiteralPath $log -PathType Leaf) { Get-Content -LiteralPath $log -Raw } else { '' }
            # Only initrun's pre-command environment failure is recoverable. A
            # compiler failure, timeout, initialization failure or retry failure
            # must stop before any payload is published or capture is reserved.
            if ($index -ne 1 -or $initialized -or
                $text -notmatch '(?m)^ERROR: init\.ps1 amd64fre /envcheck failed:' -or
                $text -notmatch '(?m)^Run a full init first: \.\\init\.ps1 amd64fre\s*$') { throw }
            $initialized = $true
            $initLog = Join-Path $output 'init-amd64fre.log'
            $logs += $initLog
            $null = Invoke-LoggedProcess -Executable $pwsh -Arguments $plan.initializationCommand -Directory $root `
                -LogPath $initLog -TimeoutSeconds 1800
            $retryLog = Join-Path $output "build-$index-after-init.log"
            $logs += $retryLog
            $null = Invoke-LoggedProcess -Executable $pwsh -Arguments $command -Directory $root `
                -LogPath $retryLog -TimeoutSeconds 1800
        }
    }
    if ((Get-SourceHash $root) -ne $sourceHash) {
        throw 'Native build modified source inputs; generated-source changes require a newly approved source snapshot.'
    }
    $payload = Join-Path $root $plan.payloadRelativePath
    if (!(Test-Path -LiteralPath $payload -PathType Container)) { throw 'Build did not produce the packaged private runtime.' }
    $closure = @(Get-NativeClosure $payload | Sort-Object path)
    Assert-NativeClosure $closure
    $copy = Join-Path $output 'payload'
    New-Item -ItemType Directory $copy | Out-Null
    foreach ($entry in $closure) {
        $target = Get-NativeChildPath $copy $entry.path
        New-Item -ItemType Directory -Force (Split-Path $target -Parent) | Out-Null
        Copy-Item -LiteralPath (Get-NativeChildPath $payload $entry.path) -Destination $target
        Assert-FileHash $target $entry.sha256
        $entry.source = $target
    }
    $manifest = [ordered]@{
        schema = 'ui-profiler/native-runtime/v1'; level = $Level
        architecture = 'x64'; configuration = 'Release'; flavor = 'amd64fre'
        source = @{ path = $root; revision = $revision; sha256 = $sourceHash }
        payloadDirectory = $copy; expectedRuntimeFile = $plan.expectedRuntimeFile
        xbfMode = $plan.xbfMode; files = $closure
        initialization = @{ performed = $initialized; policy = 'once-after-initrun-envcheck-failure' }
        logs = @($logs | ForEach-Object { @{ path = $_; sha256 = Get-Sha256 $_ } })
        plan = @{ path = (Join-Path $output 'plan.json'); sha256 = Get-Sha256 (Join-Path $output 'plan.json') }
    }
    $path = Join-Path $output 'native-runtime.json'
    Write-JsonFile $path $manifest
    @{ path = $path; sha256 = Get-Sha256 $path }
}

function Read-NativeRuntimeManifest([string]$ManifestPath) {
    Assert-NativeNoLinks $ManifestPath
    $manifest = Read-JsonFile $ManifestPath
    if ($manifest.schema -ne 'ui-profiler/native-runtime/v1' -or
        $manifest.architecture -ne 'x64' -or $manifest.configuration -ne 'Release' -or
        $manifest.flavor -ne 'amd64fre' -or $manifest.level -notin @('control', 'framework') -or
        $manifest.source.sha256 -notmatch '^[0-9a-f]{64}$' -or
        $manifest.source.revision -notmatch '^[0-9a-f]{40,64}$' -or
        $manifest.expectedRuntimeFile -ne 'Microsoft.UI.Xaml.dll' -or $manifest.xbfMode -ne 'embedded-in-pri') {
        throw 'Unsupported native runtime manifest or source provenance.'
    }
    $payload = Get-NativeFullPath $manifest.payloadDirectory
    if (!(Test-NativePathWithin $payload (Get-NativeFullPath (Split-Path $ManifestPath -Parent)))) {
        throw 'Runtime payload must be contained in its audited manifest directory.'
    }
    Assert-NativeClosure $manifest.files
    $actual = @(Get-NativeClosure $payload | Sort-Object path)
    if ($actual.Count -ne $manifest.files.Count) { throw 'Runtime payload closure changed.' }
    foreach ($entry in $manifest.files) {
        $path = Get-NativeChildPath $payload $entry.path
        if ($entry.source -ne $path) { throw 'Runtime artifact source does not match the immutable payload.' }
        $found = @($actual | Where-Object path -eq $entry.path)
        if ($found.Count -ne 1 -or $found[0].sha256 -ne $entry.sha256 -or
            $found[0].role -ne $entry.role -or $found[0].machine -ne $entry.machine -or
            $found[0].fileVersion -ne $entry.fileVersion) { throw "Runtime payload changed: $($entry.path)" }
        Assert-FileHash $path $entry.sha256
    }
    $manifest
}

function Get-NativePrivateDeployment([string]$DeploymentDirectory) {
    $root = Get-NativeFullPath $DeploymentDirectory
    Assert-NativeNoLinks $root
    # The caller creates this marker only after copying/building an isolated app.
    # originalDirectory is the immutable deployment used for replacement auditing.
    $marker = Join-Path $root '.ui-profiler-private-deployment.json'
    Assert-NativeNoLinks $marker
    $private = Read-JsonFile $marker
    if ($private.schema -ne 'ui-profiler/private-deployment/v1') { throw 'An explicit private deployment marker is required.' }
    $original = Get-NativeFullPath $private.originalDirectory
    if ((Test-NativePathWithin $root $original) -or (Test-NativePathWithin $original $root)) {
        throw 'The original application and private deployment must be disjoint.'
    }
    Assert-NativeNoLinks $original
    if (Test-Path -LiteralPath (Join-Path $root 'AppxManifest.xml')) { throw 'Packaged apps are not supported by the private runtime adapter.' }
    foreach ($item in Get-ChildItem -LiteralPath $root -Recurse -Force) { Assert-NativeNoLinks $item.FullName }
    $executable = Get-NativeChildPath $root $private.executable
    $originalExecutable = Get-NativeChildPath $original $private.executable
    Assert-NativeNoLinks $originalExecutable
    Assert-FileHash $executable (Get-Sha256 $originalExecutable)
    if ((Get-NativeMachine $executable) -ne 0x8664) { throw 'The private app must be an x64 executable.' }
    @{ directory = $root; originalDirectory = $original; executable = $executable; markerSha256 = Get-Sha256 $marker }
}

function Install-NativeRuntime {
    param(
        [Parameter(Mandatory)][string]$DeploymentDirectory,
        [Parameter(Mandatory)][string]$ManifestPath
    )
    $manifestHash = Get-Sha256 $ManifestPath
    $manifest = Read-NativeRuntimeManifest $ManifestPath
    $deployment = Get-NativePrivateDeployment $DeploymentDirectory
    $root = $deployment.directory
    $payload = Get-NativeFullPath $manifest.payloadDirectory
    if ((Test-NativePathWithin $root $payload) -or (Test-NativePathWithin $payload $root)) {
        throw 'Runtime payload and deployment must be disjoint.'
    }
    $receiptPath = Join-Path $root '.ui-profiler-native-runtime.json'
    if (Test-Path -LiteralPath $receiptPath) { throw 'A native runtime is already staged; use a fresh private deployment.' }
    $rows = @()
    # Validate the entire closure before overwriting even a single private file.
    foreach ($entry in $manifest.files) {
        $target = Get-NativeChildPath $root $entry.path
        $original = Get-NativeChildPath $deployment.originalDirectory $entry.path
        Assert-NativeNoLinks $original
        $oldHash = Get-Sha256 $original
        Assert-FileHash $target $oldHash
        if ($entry.role -eq 'prerequisite') {
            if ($oldHash -ne $entry.sha256) { throw "Packaged asset/MUI is incompatible: $($entry.path)" }
        } elseif ([IO.Path]::GetExtension($target) -eq '.dll') {
            if ((Get-NativeMachine $target) -ne $entry.machine -or
                (Get-NativeFileVersion $target) -ne $entry.fileVersion) {
                throw "Native runtime architecture/version is incompatible: $($entry.path)"
            }
        }
        $rows += [pscustomobject]@{
            path = $entry.path; role = $entry.role
            originalSha256 = $oldHash; replacementSha256 = $entry.sha256
        }
    }
    $existing = @(Get-NativeClosure $root)
    if ($existing.Count -ne $manifest.files.Count) { throw 'Application and private runtime closures differ; rebuilding against the local WinUI component is required.' }
    foreach ($entry in $manifest.files | Where-Object role -eq overlay) {
        Copy-Item -LiteralPath (Get-NativeChildPath $payload $entry.path) `
            -Destination (Get-NativeChildPath $root $entry.path) -Force
    }
    Assert-FileHash $ManifestPath $manifestHash
    $receipt = @{
        schema = 'ui-profiler/staged-native-runtime/v1'
        manifest = @{ path = Get-NativeFullPath $ManifestPath; sha256 = $manifestHash }
        deploymentDirectory = $root; originalDirectory = $deployment.originalDirectory
        markerSha256 = $deployment.markerSha256; files = $rows
        expectedRuntimePath = Join-Path $root $manifest.expectedRuntimeFile
    }
    Write-JsonFile $receiptPath $receipt
    Assert-NativeRuntime -DeploymentDirectory $root -ManifestPath $ManifestPath
}

function Assert-NativeRuntime {
    param(
        [Parameter(Mandatory)][string]$DeploymentDirectory,
        [Parameter(Mandatory)][string]$ManifestPath
    )
    $manifest = Read-NativeRuntimeManifest $ManifestPath
    $deployment = Get-NativePrivateDeployment $DeploymentDirectory
    $root = $deployment.directory
    $receiptPath = Join-Path $root '.ui-profiler-native-runtime.json'
    Assert-NativeNoLinks $receiptPath
    $receipt = Read-JsonFile $receiptPath
    if ($receipt.schema -ne 'ui-profiler/staged-native-runtime/v1' -or
        $receipt.manifest.path -ne (Get-NativeFullPath $ManifestPath) -or
        $receipt.deploymentDirectory -ne $root -or $receipt.originalDirectory -ne $deployment.originalDirectory -or
        $receipt.markerSha256 -ne $deployment.markerSha256 -or
        $receipt.expectedRuntimePath -ne (Join-Path $root $manifest.expectedRuntimeFile) -or
        $receipt.files.Count -ne $manifest.files.Count) { throw 'Native staging receipt does not match this private deployment.' }
    Assert-FileHash $ManifestPath $receipt.manifest.sha256
    $actual = @(Get-NativeClosure $root)
    if ($actual.Count -ne $manifest.files.Count) { throw 'Staged runtime closure changed.' }
    foreach ($entry in $manifest.files) {
        Assert-FileHash (Get-NativeChildPath $root $entry.path) $entry.sha256
        $row = @($receipt.files | Where-Object path -eq $entry.path)
        if ($row.Count -ne 1 -or $row[0].replacementSha256 -ne $entry.sha256 -or $row[0].role -ne $entry.role) {
            throw "Native staging receipt changed: $($entry.path)"
        }
        $original = Get-NativeChildPath $deployment.originalDirectory $entry.path
        Assert-NativeNoLinks $original
        Assert-FileHash $original $row[0].originalSha256
    }
    $receipt
}

Export-ModuleMember -Function Get-NativeBuildPlan, Invoke-NativeBuild, Install-NativeRuntime, Assert-NativeRuntime
