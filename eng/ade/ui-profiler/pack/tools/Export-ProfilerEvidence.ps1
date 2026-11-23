# Read-only queries through the existing MUX profiler CLI and its canonical snapshot engine.
#Requires -Version 7
param(
    [Parameter(Mandatory)][string]$MeasurementPath,
    [Parameter(Mandatory)][string]$ProfilerCli,
    [Parameter(Mandatory)][string]$Directory
)
$ErrorActionPreference = 'Stop'
Import-Module "$PSScriptRoot\ProfilerHarness.psm1" -Force -DisableNameChecking
$measurement = Read-JsonFile $MeasurementPath
Assert-Measurement $measurement
if ((Get-ProfilerHash $ProfilerCli) -ne $measurement.profilerSha256) { throw 'Use the exact recorded profiler payload.' }
if (Test-Path -LiteralPath $Directory) { throw 'Engine exports require a new directory; existing evidence is never overwritten.' }
New-Item -ItemType Directory $Directory | Out-Null
$actions = Read-JsonFile $measurement.actions.path
$payload = Split-Path $ProfilerCli -Parent
foreach ($name in @('SQLitePCLRaw.core.dll', 'SQLitePCLRaw.provider.e_sqlite3.dll',
    'SQLitePCLRaw.batteries_v2.dll', 'Microsoft.Data.Sqlite.dll', 'WinUIProfiler.Core.dll')) {
    [void][Reflection.Assembly]::LoadFrom((Join-Path $payload $name))
}
[void][Runtime.InteropServices.NativeLibrary]::Load((Join-Path $payload 'e_sqlite3.dll'))
[SQLitePCL.Batteries_V2]::Init()
$token = [Threading.CancellationToken]::None
$jsonOptions = [Text.Json.JsonSerializerOptions]::new()
$jsonOptions.PropertyNamingPolicy = [Text.Json.JsonNamingPolicy]::CamelCase
$jsonOptions.WriteIndented = $true
$jsonOptions.Converters.Add([Text.Json.Serialization.JsonStringEnumConverter]::new())
function Write-NativeJson([string]$Path, $Value) {
    [IO.File]::WriteAllText($Path, [Text.Json.JsonSerializer]::Serialize[object]($Value, $jsonOptions),
        [Text.UTF8Encoding]::new($false))
}
$identity = [WinUIProfiler.Engine.Storage.SessionFile]::ReadProcessIdentity($measurement.database.path)
if ($null -eq $identity.Item2) { throw 'The recording cannot establish its process instance.' }
$paths = @()
if ($null -ne $measurement.PSObject.Properties['evidence']) {
    $paths = @($measurement.evidence | Where-Object kind -eq snapshot | ForEach-Object path)
} else {
    $live = Join-Path (Split-Path $MeasurementPath -Parent) 'live.ndjson'
    if (Test-Path -LiteralPath $live) {
        foreach ($line in [IO.File]::ReadLines($live)) {
            $item = $line | ConvertFrom-Json
            if ($item.type -eq 'summary' -and $null -ne $item.data.PSObject.Properties['snapshotPaths']) {
                $paths = @($item.data.snapshotPaths)
            }
        }
    }
}
$documents = [Collections.Generic.List[WinUIProfiler.Contracts.TreeSnapshotDocument]]::new()
$checkpoints = [Collections.Generic.List[object]]::new()
$windows = @(@{ id = 'scenario'; start = $actions.startedQpc; end = $actions.endedQpc }) +
    @($actions.steps | ForEach-Object { @{ id = "action:$($_.id)"; start = $_.startedQpc; end = $_.endedQpc } })
foreach ($window in $windows) {
    foreach ($boundary in @('start', 'end')) {
        $qpc = [long]$window[$boundary]
        # Only the process identity and query clock are used; this is not a tree capture.
        $document = [WinUIProfiler.Contracts.TreeSnapshotDocument]::new(4, [Guid]::Empty,
            [uint32]$identity.Item1, [long]$identity.Item2, [long]$actions.qpcFrequency, $qpc, $qpc,
            [WinUIProfiler.Contracts.TreeSnapshotRoot[]]@(), [WinUIProfiler.Contracts.XamlSnapshotNode[]]@(),
            [WinUIProfiler.Contracts.CompositionSnapshotNode[]]@(), [WinUIProfiler.Contracts.TreeSnapshotCorrelation[]]@(),
            [string[]]@(), $null)
        $checkpoints.Add(@{ scope = $window.id; boundary = $boundary; captureIndex = $documents.Count })
        $documents.Add($document)
    }
}
$snapshots = [Collections.Generic.List[object]]::new()
$phases = @{}
$previous = $null
foreach ($path in $paths) {
    $data = [WinUIProfiler.Engine.Snapshots.SnapshotViewerData]::LoadAsync($path, $token).GetAwaiter().GetResult()
    $document = $data.Document
    if ($document.ProcessId -ne $identity.Item1 -or $document.CreationTimeFileTime -ne $identity.Item2 -or
        $document.QpcFrequency -ne $actions.qpcFrequency) { throw 'Snapshot process/clock does not match the recording.' }
    $phase = if ($document.EndQpc -le $actions.startedQpc) { 'before-scenario' } elseif (
        $document.StartQpc -ge $actions.endedQpc) { 'after-scenario' } else { 'during-scenario' }
    if ($phases.ContainsKey($phase)) {
        $phases[$phase]++
        $phase += ":$($phases[$phase])"
    } else { $phases[$phase] = 1 }
    $index = $snapshots.Count
    $resourcePath = Join-Path $Directory "snapshot-$index-resources.json"
    $resources = [WinUIProfiler.Engine.Processing.SnapshotResourceComparison]::new($null, $document.ResourceSnapshot)
    Write-NativeJson $resourcePath $resources.Rows
    $histories = [Collections.Generic.List[object]]::new()
    foreach ($kind in @([WinUIProfiler.Engine.Snapshots.SnapshotTreeKind]::Xaml,
        [WinUIProfiler.Engine.Snapshots.SnapshotTreeKind]::Composition)) {
        $tree = if ($kind -eq [WinUIProfiler.Engine.Snapshots.SnapshotTreeKind]::Xaml) { $data.Xaml } else { $data.Composition }
        foreach ($node in $tree.Nodes.Values) {
            $object = [WinUIProfiler.Engine.Snapshots.SnapshotObjectIdentity]::Create($document, $node, $kind)
            $objectId = $object.FindObject($measurement.database.path, $token)
            $selection = $data.SelectNodes($kind, [ulong[]]@($node.Id))
            $histories.Add(@{
                tree = $kind.ToString(); nodeId = $node.Id; rootId = $node.RootId; rootEndQpc = $object.Qpc
                objectId = $objectId
                status = if ($null -ne $objectId) { 'matched' } else { 'unavailable' }
                matchingIds = if ($kind -eq [WinUIProfiler.Engine.Snapshots.SnapshotTreeKind]::Xaml) {
                    @($selection.VisualIds)
                } else { @($selection.XamlIds) }
            })
        }
    }
    $diffPaths = @()
    if ($null -ne $previous) {
        foreach ($tree in @('xaml', 'composition')) {
            $name = "snapshot-$index-diff-$tree"
            [void](Invoke-ProfilerQuery $ProfilerCli @('snapshot', 'diff', $previous, $path, '--tree', $tree) $Directory $name)
            $diffPaths += Join-Path $Directory "$name.json"
        }
    }
    $snapshots.Add(@{
        path = $path; sha256 = Get-Sha256 $path; phase = $phase
        resourcesPath = $resourcePath; diffPaths = $diffPaths; histories = @($histories)
    })
    $checkpoints.Add(@{ scope = "snapshot:$phase"; boundary = 'end'; captureIndex = $documents.Count })
    $documents.Add($document)
    $previous = $path
}
$captures = [WinUIProfiler.Engine.Storage.SessionFile]::ReadTextureSnapshots($measurement.database.path, $documents, $token)
$texturePath = Join-Path $Directory 'texture-checkpoints.json'
Write-NativeJson $texturePath $captures
$retainedLaunch = Join-Path (Split-Path $MeasurementPath -Parent) 'launch.json'
if ($measurement.schema -eq 'ui-profiler/measurement/v2' -and (Test-Path -LiteralPath $retainedLaunch)) {
    Copy-Item -LiteralPath $retainedLaunch -Destination (Join-Path $Directory 'launch.json')
} else {
    [void](Invoke-ProfilerQuery $ProfilerCli @('launch', $measurement.database.path) $Directory 'launch')
}
$tools = @(@{ path = $PSCommandPath; sha256 = Get-Sha256 $PSCommandPath })
$modulePath = Join-Path $PSScriptRoot 'ProfilerHarness.psm1'
$tools += @{ path = $modulePath; sha256 = Get-Sha256 $modulePath }
foreach ($file in [IO.Directory]::EnumerateFiles($payload, '*', [IO.SearchOption]::AllDirectories)) {
    $tools += @{ path = $file; sha256 = Get-Sha256 $file }
}
Write-JsonFile (Join-Path $Directory 'engine.json') @{
    schema = 'ui-profiler/engine-evidence/v1'
    measurementSha256 = Get-Sha256 $MeasurementPath
    tools = $tools
    snapshots = @($snapshots)
    textureCheckpoints = @($checkpoints)
    textureCapturesPath = $texturePath
    launchPath = Join-Path $Directory 'launch.json'
}
Write-Output 'Read-only MUX profiler evidence exported; zero captures.'
