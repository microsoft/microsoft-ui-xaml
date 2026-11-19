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

.EXAMPLE
.\Run-RenderBench.ps1 -Modes composition,d2d-sw -Scenes settings -Repetitions 1 -Quick
#>
[CmdletBinding()]
param(
    [ValidateSet('composition', 'gdiplus', 'd2d-hw', 'd2d-sw')]
    [string[]]$Modes = @('composition', 'gdiplus', 'd2d-hw', 'd2d-sw'),
    [ValidateSet('settings', 'dashboard')]
    [string[]]$Scenes = @('settings', 'dashboard'),
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

$phaseArgs = if ($Quick) { @('--warmup', '2', '--settle', '4', '--interaction', '6', '--recovery', '2') } else { @() }
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
try {
    for ($rep = 1; $rep -le $Repetitions; $rep++) {
        foreach ($scene in $Scenes) {
            $order = $Modes | Sort-Object { $random.Next() }
            foreach ($mode in $order) {
                $runDir = Join-Path $Results "$scene\$mode\run$rep"
                New-Item -ItemType Directory -Force $runDir | Out-Null
                $etl = Join-Path $runDir 'trace.etl'
                $env:WINUI_RENDERING_BACKEND = if ($mode -eq 'composition') { '' } else { $mode }

                Write-Host "[$rep/$Repetitions] $scene $mode -> $runDir"
                & wpr -start "$profilePath!$Profile" -filemode
                if ($LASTEXITCODE -ne 0) { throw "wpr -start failed ($LASTEXITCODE)" }
                try {
                    $benchArgs = @('--scene', $scene, '--seed', $Seed, '--screenshot', (Join-Path $runDir 'screenshot.bmp')) + $phaseArgs + $ExtraArgs
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
