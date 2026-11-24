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

-Scenes picks entries from the scene matrix below (default: S1-S10 with their variants, S1/S2 first), plan-id aliases
(s1..s32) or the groups full-backend (S11-S23), decision (S24-S32) and all. Variant entries (gallery-decoded, animated-noring,
multiwindow-tabs, interop-nointerop, lottie-noplayer, swapchain-nomedia) add their RenderBench flags; results go to
<Results>\<entry>\<mode>\run<N>.
Full runs use each scene's default phase lengths (README, "Scenes"); -Quick shortens every phase, per scene.

.EXAMPLE
.\Run-RenderBench.ps1 -Modes composition,d2d-sw -Scenes settings -Repetitions 1 -Quick

.EXAMPLE
.\Run-RenderBench.ps1 -Scenes inbox,gallery,gallery-decoded -InputMode programmatic -Repetitions 3
.EXAMPLE
.\Run-RenderBench.ps1 -Modes composition,gdiplus -Scenes s11,s11-nointerop,s15 -InputMode programmatic -Quick
#>
[CmdletBinding()]
param(
    [ValidateSet('composition', 'gdiplus', 'd2d-hw', 'd2d-sw')]
    [string[]]$Modes = @('composition', 'gdiplus', 'd2d-hw', 'd2d-sw'),
    # Entries of the scene matrix below, plan-id aliases (s1..s32, s11-nointerop, s12-noplayer, s16-nomedia), or the
    # groups 'full-backend' (S11-S16, S19-S23 and their variants), 'decision' (S24-S32) and 'all'. Validated after the matrix is defined.
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
    [string[]]$ExtraArgs = @(),
    # Composition-mode engine. System uses the internal Switcher\ProcessOverrides key (set for the run, then restored)
    # plus CompositionEngine.TrySetProcessEngine(System) in RenderBench's Main; a run that can't select it exits 4.
    [ValidateSet('Lifted', 'System')]
    [string]$CompositionEngine = 'Lifted',
    # XamlOptionalChanges opt-ins (all modes).
    [ValidateSet('None', 'All')]
    [string]$OptionalChanges = 'None'
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
    # Full-backend scenes (test plan S11-S23; S17 WebView2 and S18 InkCanvas are out of scope). S11/S12-player/S16
    # expect a per-window WindowFallback to Composition in immediate modes; the -no* variants must stay immediate.
    'interop'           = @{ Args = @('--scene', 'interop');                     Quick = 6 }
    'interop-nointerop' = @{ Args = @('--scene', 'interop', '--no-interop');     Quick = 6 }
    'materials'         = @{ Args = @('--scene', 'materials');                   Quick = 6 }
    'popups'            = @{ Args = @('--scene', 'popups');                      Quick = 12 }
    'independent'       = @{ Args = @('--scene', 'independent');                 Quick = 10 }
    'lottie'            = @{ Args = @('--scene', 'lottie');                      Quick = 6 }
    'lottie-noplayer'   = @{ Args = @('--scene', 'lottie', '--no-player');       Quick = 6 }
    'text'              = @{ Args = @('--scene', 'text');                        Quick = 6 }
    'swapchain'         = @{ Args = @('--scene', 'swapchain');                   Quick = 6 }
    'swapchain-nomedia' = @{ Args = @('--scene', 'swapchain', '--no-media');     Quick = 6 }
    # S20/S21 leak gates need 100 cycles; -Quick runs fewer and reports those gates as n/a.
    'dpitheme'          = @{ Args = @('--scene', 'dpitheme');                    Quick = 10; QuickArgs = @('--items', '20') }
    'devicelost'        = @{ Args = @('--scene', 'devicelost');                  Quick = 6 }
    'accessibility'     = @{ Args = @('--scene', 'accessibility');               Quick = 6 }
    'appshell'          = @{ Args = @('--scene', 'appshell');                    Quick = 10; QuickArgs = @('--items', '500') }
    # Backend decision scenes (test plan S24-S32): step sweeps that run to completion (--interaction only sets S29's
    # animation length). Steps hold --step-seconds (default 4 s), so each settled half has about four 500 ms samples.
    'winsize'           = @{ Args = @('--scene', 'winsize'); Quick = 1; QuickArgs = @('--step-seconds', '2') }
    'density'           = @{ Args = @('--scene', 'density'); Quick = 1; QuickArgs = @('--step-seconds', '2') }
    'windowcount'       = @{ Args = @('--scene', 'windowcount'); Quick = 1; QuickArgs = @('--step-seconds', '2') }
    'background'        = @{ Args = @('--scene', 'background'); Quick = 1; QuickArgs = @('--step-seconds', '2') }
    'navchurn'          = @{ Args = @('--scene', 'navchurn'); Quick = 1; QuickArgs = @('--step-seconds', '2', '--items', '60') }
    'sustained'         = @{ Args = @('--scene', 'sustained'); Quick = 10; QuickArgs = @('--step-seconds', '2') }
    'resize'            = @{ Args = @('--scene', 'resize'); Quick = 1; QuickArgs = @('--step-seconds', '2', '--items', '40') }
    'minimal'           = @{ Args = @('--scene', 'minimal'); Quick = 1; QuickArgs = @('--step-seconds', '2') }
    'largecanvas'       = @{ Args = @('--scene', 'largecanvas'); Quick = 1; QuickArgs = @('--step-seconds', '2') }
}
$sceneAliases = @{
    's1' = 'settings'; 's2' = 'dashboard'; 's3' = 'inbox'; 's4' = 'gallery'; 's5' = 'animated'; 's6' = 'reader'
    's7' = 'layered'; 's8' = 'datagrid'; 's9' = 'multiwindow'; 's10' = 'form'
    's11' = 'interop'; 's11-nointerop' = 'interop-nointerop'; 's12' = 'lottie'; 's12-noplayer' = 'lottie-noplayer'
    's13' = 'materials'; 's14' = 'popups'; 's15' = 'independent'; 's16' = 'swapchain'; 's16-nomedia' = 'swapchain-nomedia'
    's19' = 'text'; 's20' = 'dpitheme'; 's21' = 'devicelost'; 's22' = 'accessibility'; 's23' = 'appshell'
    's24' = 'winsize'; 's25' = 'density'; 's26' = 'windowcount'; 's27' = 'background'; 's28' = 'navchurn'
    's29' = 'sustained'; 's30' = 'resize'; 's31' = 'minimal'; 's32' = 'largecanvas'
}
$decisionScenes = @('winsize', 'density', 'windowcount', 'background', 'navchurn', 'sustained', 'resize', 'minimal', 'largecanvas')
$fullBackendScenes = @('interop', 'interop-nointerop', 'lottie', 'lottie-noplayer', 'materials', 'popups', 'independent',
    'swapchain', 'swapchain-nomedia', 'text', 'dpitheme', 'devicelost', 'accessibility', 'appshell')
$Scenes = @(foreach ($entry in $Scenes) {
    $name = $entry.ToLowerInvariant()
    if ($name -eq 'all') { @($sceneMatrix.Keys) }
    elseif ($name -eq 'full-backend') { $fullBackendScenes }
    elseif ($name -eq 'decision') { $decisionScenes }
    elseif ($sceneAliases.ContainsKey($name)) { $sceneAliases[$name] }
    elseif ($sceneMatrix.Contains($name)) { $name }
    elseif ($name -in 's17', 's18') { throw "$entry (WebView2/InkCanvas) is out of scope; see the test plan." }
    else { throw "Unknown scene '$entry'. Valid: $((@($sceneMatrix.Keys) + @($sceneAliases.Keys) + 'full-backend', 'decision', 'all') -join ', ')" }
}) | Select-Object -Unique
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
# DWM keeps a closed window's surfaces (~30 MB after a 4K window) until another top-level window is shown and closed, so
# the previous run's leftovers would land in the next decision run's dwm.exe baseline. An in-process probe window does not
# trigger the release; showing and closing a WinForms form from this script does.
function Invoke-DwmSettle {
    Add-Type -AssemblyName System.Windows.Forms
    $form = [System.Windows.Forms.Form]::new()
    $form.Text = 'RenderBench DWM settle'
    $form.StartPosition = 'Manual'
    $form.SetBounds(0, 0, 200, 100)
    $form.Show()
    $clock = [Diagnostics.Stopwatch]::StartNew()
    while ($clock.ElapsedMilliseconds -lt 1000) { [System.Windows.Forms.Application]::DoEvents(); Start-Sleep -Milliseconds 20 }
    $form.Close()
    $form.Dispose()
    [System.Windows.Forms.Application]::DoEvents()
    Start-Sleep -Seconds 2
}
New-Item -ItemType Directory -Force $Results | Out-Null
$Results = (Resolve-Path $Results).Path
& wpr -cancel 2>$null | Out-Null

$random = [Random]::new($Seed)
$previous = $env:WINUI_RENDERING_BACKEND
$switcherKey = 'HKLM:\Software\Microsoft\WinUI\RenderingEngine\Switcher\ProcessOverrides'
$benchExeName = Split-Path -Leaf $BenchExe
$previousOverride = $null
if ($CompositionEngine -eq 'System' -and $Modes -contains 'composition') {
    if (-not (Test-Path $switcherKey)) { New-Item -Force $switcherKey | Out-Null }  # -Force on an existing key wipes its values
    $previousOverride = (Get-ItemProperty $switcherKey -Name $benchExeName -ErrorAction SilentlyContinue).$benchExeName
    New-ItemProperty $switcherKey -Name $benchExeName -PropertyType DWord -Value 1 -Force | Out-Null
}
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
                if ($decisionScenes -contains $scene) { Invoke-DwmSettle }
                & wpr -start "$profilePath!$Profile" -filemode
                if ($LASTEXITCODE -ne 0) { throw "wpr -start failed ($LASTEXITCODE)" }
                try {
                    $benchArgs = $sceneArgs + @('--seed', $Seed, '--screenshot', (Join-Path $runDir 'screenshot.bmp')) + $ExtraArgs
                    if ($mode -eq 'composition') { $benchArgs += @('--composition-engine', $CompositionEngine.ToLowerInvariant()) }
                    $benchArgs += @('--optional-changes', $OptionalChanges.ToLowerInvariant())
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
                # Decision scenes compare session dwm.exe memory against a pre-window baseline; give DWM time to release the
                # previous run's surfaces.
                Start-Sleep -Seconds $(if ($decisionScenes -contains $scene) { 6 } else { 2 })
            }
        }
    }
}
finally {
    $env:WINUI_RENDERING_BACKEND = $previous
    if ($CompositionEngine -eq 'System' -and $Modes -contains 'composition') {
        if ($null -ne $previousOverride) {
            New-ItemProperty $switcherKey -Name $benchExeName -PropertyType DWord -Value $previousOverride -Force | Out-Null
        }
        else {
            Remove-ItemProperty $switcherKey -Name $benchExeName -ErrorAction SilentlyContinue
        }
    }
    if ($heapTracing) {
        & wpr -heaptracingconfig (Split-Path -Leaf $BenchExe) disable | Out-Null
    }
}

& $analyzer report $Results
Write-Host "Report: $(Join-Path $Results 'report.md')"
