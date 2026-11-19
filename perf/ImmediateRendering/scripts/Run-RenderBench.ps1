<#
.SYNOPSIS
Runs the RenderBench comparative matrix under WPR and analyzes each run.

.DESCRIPTION
For each repetition, runs every requested backend mode (in a seeded random order to spread thermal/background drift)
for each scene. Each run:
  1. sets WINUI_RENDERING_BACKEND (empty for composition, so the default path is measured unmodified),
  2. starts WPR with profiles\ImmediateRendering.wprp!<Profile>,
  3. launches RenderBench.exe and waits for it to exit,
  4. stops WPR into <Results>\<scene>\<mode>\run<N>\trace.etl,
  5. runs RenderBench.Analyzer analyze -> metrics.json next to the trace.
Afterwards it writes <Results>\report.md. Requires an elevated shell (WPR).

-Scenes picks entries from the scene matrix below (default: all, S1/S2 first). Variant entries (gallery-decoded,
animated-noring, multiwindow-tabs) add their RenderBench flags; results go to <Results>\<entry>\<mode>\run<N>.
Full runs use each scene's default phase lengths (README, "Scenes"); -Quick shortens every phase, per scene.

.EXAMPLE
.\Run-RenderBench.ps1 -Modes composition,d2d-sw -Scenes settings -Repetitions 1 -Quick

.EXAMPLE
.\Run-RenderBench.ps1 -Scenes inbox,gallery,gallery-decoded -InputMode programmatic -Repetitions 3
#>
[CmdletBinding()]
param(
    [ValidateSet('composition', 'gdiplus', 'd2d-hw', 'd2d-sw')]
    [string[]]$Modes = @('composition', 'gdiplus', 'd2d-hw', 'd2d-sw'),
    [ValidateSet('settings', 'dashboard', 'inbox', 'gallery', 'gallery-decoded', 'animated', 'animated-noring', 'reader',
        'layered', 'datagrid', 'multiwindow', 'multiwindow-tabs', 'form')]
    [string[]]$Scenes = @('settings', 'dashboard', 'inbox', 'gallery', 'gallery-decoded', 'animated', 'animated-noring',
        'reader', 'layered', 'datagrid', 'multiwindow', 'multiwindow-tabs', 'form'),
    # auto picks programmatic input in remote/disconnected sessions (README, "Input modes").
    [ValidateSet('auto', 'sendinput', 'programmatic')]
    [string]$InputMode = 'auto',
    [int]$Repetitions = 5,
    [ValidateSet('Timing', 'Memory', 'Gpu')]
    [string]$Profile = 'Timing',
    [string]$Results = (Join-Path $PSScriptRoot "..\results\$(Get-Date -Format yyyyMMdd-HHmmss)"),
    [string]$BenchExe,
    [int]$Seed = 1,
    # Short phases for smoke testing the pipeline; numbers are not comparable with full runs.
    [switch]$Quick,
    # Delete trace.etl after metrics.json is written (Timing ETLs are ~15 MB/s with CPU stacks).
    [switch]$DeleteTraces,
    [string[]]$ExtraArgs = @()
)

$ErrorActionPreference = 'Stop'
$root = Resolve-Path (Join-Path $PSScriptRoot '..')
$profilePath = Join-Path $root 'profiles\ImmediateRendering.wprp'
if (-not $BenchExe) {
    $BenchExe = Join-Path $root 'bin\RenderBench\x64\Release\net10.0-windows10.0.26100.0\win-x64\RenderBench.exe'
}
$analyzer = Join-Path $root 'bin\RenderBench.Analyzer\Release\net10.0-windows10.0.26100.0\RenderBench.Analyzer.exe'
foreach ($path in @($BenchExe, $analyzer, $profilePath)) {
    if (-not (Test-Path $path)) { throw "Missing $path. Build with: dotnet build -c Release (see README.md)." }
}
$principal = [Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    throw 'WPR needs an elevated shell.'
}

# Scene matrix: RenderBench flags per entry, plus the -Quick interaction length and extra flags. Full runs use the
# per-scene defaults built into RenderBench (SceneCatalog); S9's interaction is its lifecycle (steps x --step-seconds).
$sceneMatrix = [ordered]@{
    'settings'         = @{ Args = @('--scene', 'settings');                      Quick = 6 }
    'dashboard'        = @{ Args = @('--scene', 'dashboard');                     Quick = 6 }
    'inbox'            = @{ Args = @('--scene', 'inbox');                         Quick = 6 }
    'gallery'          = @{ Args = @('--scene', 'gallery');                       Quick = 6 }
    'gallery-decoded'  = @{ Args = @('--scene', 'gallery', '--decode-to-size');   Quick = 6 }
    'animated'         = @{ Args = @('--scene', 'animated');                      Quick = 6 }
    'animated-noring'  = @{ Args = @('--scene', 'animated', '--no-progressring'); Quick = 6 }
    'reader'           = @{ Args = @('--scene', 'reader');                        Quick = 6 }
    'layered'          = @{ Args = @('--scene', 'layered');                       Quick = 6 }
    'datagrid'         = @{ Args = @('--scene', 'datagrid');                      Quick = 6 }
    'multiwindow'      = @{ Args = @('--scene', 'multiwindow');                   Quick = 6; QuickArgs = @('--windows', '5', '--step-seconds', '1') }
    'multiwindow-tabs' = @{ Args = @('--scene', 'multiwindow', '--tabs');         Quick = 6; QuickArgs = @('--step-seconds', '0.5') }
    'form'             = @{ Args = @('--scene', 'form');                          Quick = 8 }
}
function Get-SceneArgs([string]$entry) {
    $spec = $sceneMatrix[$entry]
    $sceneArgs = @($spec.Args)
    if ($Quick) {
        $sceneArgs += @('--warmup', '2', '--settle', '4', '--interaction', "$($spec.Quick)", '--recovery', '2')
        if ($spec.QuickArgs) { $sceneArgs += $spec.QuickArgs }
    }
    if ($InputMode -ne 'auto') { $sceneArgs += @('--input-mode', $InputMode) }
    return $sceneArgs
}
New-Item -ItemType Directory -Force $Results | Out-Null
$Results = (Resolve-Path $Results).Path
& wpr -cancel 2>$null | Out-Null

$random = [Random]::new($Seed)
$previous = $env:WINUI_RENDERING_BACKEND
# Memory profile: opt RenderBench.exe into heap tracing (HeapProcessId 0 in the wprp) so GDI+ heap use is attributable.
$heapTracing = $Profile -eq 'Memory'
if ($heapTracing) {
    & wpr -heaptracingconfig (Split-Path -Leaf $BenchExe) enable | Out-Null
    if ($LASTEXITCODE -ne 0) { throw "wpr -heaptracingconfig enable failed ($LASTEXITCODE)" }
}

# ---- Added by the D2D-HW branch ----
# D2D-HW needs a real GPU. On machines without one (e.g. a Hyper-V VM) the native factory returns DXGI_ERROR_NOT_FOUND, so
# instead of running and failing the gate, each D2D-HW run is recorded as SKIPPED ("no hardware adapter"); the report
# shows "n/a (no GPU)". The analyzer applies the same adapter policy as the native factory, including
# WINUI_RENDERING_ADAPTER_LUID.
$skipD2DHardware = $false
if ($Modes -contains 'd2d-hw') {
    & $analyzer adapters
    switch ($LASTEXITCODE) {
        0 { }
        1 { $skipD2DHardware = $true; Write-Warning 'd2d-hw: no hardware adapter; d2d-hw runs will be recorded as skipped.' }
        default { throw "RenderBench.Analyzer adapters failed ($LASTEXITCODE); check WINUI_RENDERING_ADAPTER_LUID." }
    }
}

try {
    for ($rep = 1; $rep -le $Repetitions; $rep++) {
        foreach ($scene in $Scenes) {
            $order = $Modes | Sort-Object { $random.Next() }
            foreach ($mode in $order) {
                $runDir = Join-Path $Results "$scene\$mode\run$rep"
                New-Item -ItemType Directory -Force $runDir | Out-Null
                $sceneArgs = Get-SceneArgs $scene
                if ($mode -eq 'd2d-hw' -and $skipD2DHardware) {
                    $variantArgs = (@($sceneMatrix[$scene].Args) | Select-Object -Skip 2) -join ' '
                    & $analyzer skip $sceneMatrix[$scene].Args[1] $mode 'no hardware adapter' --out (Join-Path $runDir 'metrics.json') `
                        --scene-args "$variantArgs"
                    continue
                }
                $etl = Join-Path $runDir 'trace.etl'
                $env:WINUI_RENDERING_BACKEND = if ($mode -eq 'composition') { '' } else { $mode }

                Write-Host "[$rep/$Repetitions] $scene $mode -> $runDir"
                & wpr -start "$profilePath!$Profile" -filemode
                if ($LASTEXITCODE -ne 0) { throw "wpr -start failed ($LASTEXITCODE)" }
                try {
                    $benchArgs = $sceneArgs + @('--seed', $Seed, '--screenshot', (Join-Path $runDir 'screenshot.bmp')) + $ExtraArgs
                    $process = Start-Process -FilePath $BenchExe -ArgumentList $benchArgs -PassThru -Wait `
                        -RedirectStandardError (Join-Path $runDir 'stderr.txt')
                    "exit $($process.ExitCode)" | Set-Content (Join-Path $runDir 'exitcode.txt')
                }
                finally {
                    & wpr -stop $etl | Out-Null
                }
                & $analyzer analyze $etl --out (Join-Path $runDir 'metrics.json')
                if ($DeleteTraces -and (Test-Path (Join-Path $runDir 'metrics.json'))) {
                    Remove-Item $etl -Force
                }
                Start-Sleep -Seconds 2
            }
        }
    }
}
finally {
    $env:WINUI_RENDERING_BACKEND = $previous
    if ($heapTracing) {
        & wpr -heaptracingconfig (Split-Path -Leaf $BenchExe) disable | Out-Null
    }
}

& $analyzer report $Results
Write-Host "Report: $(Join-Path $Results 'report.md')"
