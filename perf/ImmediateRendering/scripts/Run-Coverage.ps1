<#
.SYNOPSIS
Runs the XAML coverage corpus (RenderBench\Coverage\Coverage.<Family>.cs) in Composition and the immediate backends and
gates every page (test plan, "XAML coverage: harness and results").

.DESCRIPTION
1. RenderBench --list-coverage writes catalog.json (pages discovered by reflection from [CoveragePage] methods).
2. For each selected page and mode, one RenderBench process (so fallback/unsupported state never leaks between pages):
   WINUI_RENDERING_BACKEND=<mode> (unset for composition), WINUI_IMMEDIATE_STATS=<run>\stats.txt and
   WINUI_IMMEDIATE_CAPTURE=<run>\immediate.bmp. RenderBench --coverage <id> renders the page in a 1000x700 client area,
   awaits its Exercise step, waits for two identical consecutive captures, and writes capture.bmp (PrintWindow of the
   window and its popup windows) and app.json. The exit code (or "timeout") goes to exit.txt. No WPR: the stats file
   carries the `# unsupported` / `# fallback` / `# paint-failed` lines.
3. RenderBench.Analyzer coverage <Results> diffs every immediate capture against Composition, applies the gates and writes
   result.json per run plus coverage.json and coverage.html at the root.

Each RenderBench process holds the machine-wide bench lock (Global\mux-ir-bench, same as C:\mux-ir\with-bench-lock.ps1)
only while it runs, so other agents' runs interleave instead of waiting for the whole corpus.

.EXAMPLE
.\Run-Coverage.ps1 -Pages 'F8*,F12*' -Results C:\mux-ir\results\coverage
.\Run-Coverage.ps1 -Pages 'F8-controls/Button*' -Modes composition,gdiplus
#>
param(
    # Comma-separated globs over page id ("F8-controls/Button"), family ("F8-controls") or family number ("F8");
    # "!glob" excludes.
    [string]$Pages = '*',
    [string[]]$Modes = @('composition', 'gdiplus', 'd2d-sw'),
    [string]$Results = (Join-Path $PSScriptRoot "..\results\coverage-$(Get-Date -Format yyyyMMdd-HHmmss)"),
    [int]$TimeoutSeconds = 60,
    [string]$BenchExe,
    # Re-gate an existing results tree without running anything.
    [switch]$AnalyzeOnly,
    # Keep only failing runs' BMPs (PNGs are always written).
    [switch]$DeleteBmps
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
if (-not $BenchExe) {
    $BenchExe = Join-Path $root 'bin\RenderBench\x64\Release\net10.0-windows10.0.26100.0\win-x64\RenderBench.exe'
}
$analyzer = Join-Path $root 'bin\RenderBench.Analyzer\Release\net10.0-windows10.0.26100.0\RenderBench.Analyzer.exe'
foreach ($path in @($BenchExe, $analyzer)) {
    if (-not (Test-Path $path)) { throw "Missing $path; build RenderBench.sln -c Release first." }
}
New-Item -ItemType Directory -Force $Results | Out-Null
$Results = (Resolve-Path $Results).Path

function Invoke-Bench([string[]]$Arguments, [hashtable]$Environment, [string]$StdErr, [int]$Timeout) {
    $info = [Diagnostics.ProcessStartInfo]::new($BenchExe)
    foreach ($a in $Arguments) { $info.ArgumentList.Add($a) }
    $info.UseShellExecute = $false
    $info.RedirectStandardError = $true
    $info.WorkingDirectory = Split-Path $BenchExe
    foreach ($name in @('WINUI_RENDERING_BACKEND', 'WINUI_IMMEDIATE_STATS', 'WINUI_IMMEDIATE_CAPTURE', 'WINUI_IMMEDIATE_NO_FALLBACK')) {
        $info.Environment.Remove($name) | Out-Null
    }
    foreach ($key in $Environment.Keys) { $info.Environment[$key] = $Environment[$key] }
    $mutex = [Threading.Mutex]::new($false, 'Global\mux-ir-bench')
    try { [void]$mutex.WaitOne() } catch [Threading.AbandonedMutexException] { }
    try {
        $process = [Diagnostics.Process]::Start($info)
        $errTask = $process.StandardError.ReadToEndAsync()
        if (-not $process.WaitForExit($Timeout * 1000)) {
            Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
            $process.WaitForExit(5000) | Out-Null
            $code = 'timeout'
        } else {
            $process.WaitForExit()
            $code = $process.ExitCode
        }
        if ($StdErr) { Set-Content -Path $StdErr -Value $errTask.Result }
        return $code
    } finally {
        $mutex.ReleaseMutex()
    }
}

if (-not $AnalyzeOnly) {
    $catalog = Join-Path $Results 'catalog.json'
    $code = Invoke-Bench @('--list-coverage', $catalog) @{} (Join-Path $Results 'catalog-stderr.txt') 60
    if (-not (Test-Path $catalog)) { throw "RenderBench --list-coverage failed ($code)" }
    $errors = Get-Content (Join-Path $Results 'catalog-errors.txt') -ErrorAction SilentlyContinue | Where-Object { $_ }
    foreach ($e in $errors) { Write-Warning "catalog: $e" }

    $all = Get-Content $catalog -Raw | ConvertFrom-Json
    $include = @($Pages.Split(',') | ForEach-Object Trim | Where-Object { $_ -and -not $_.StartsWith('!') })
    $exclude = @($Pages.Split(',') | ForEach-Object Trim | Where-Object { $_.StartsWith('!') } | ForEach-Object { $_.Substring(1) })
    function Test-Page($page, $pattern) {
        $page.Id -like $pattern -or $page.Family -like $pattern -or "F$($page.FamilyNumber)" -like $pattern
    }
    $selected = @($all | Where-Object {
        $p = $_
        ($include.Count -eq 0 -or ($include | Where-Object { Test-Page $p $_ })) -and -not ($exclude | Where-Object { Test-Page $p $_ })
    })
    Write-Host "$($selected.Count) of $($all.Count) pages x $($Modes.Count) modes -> $Results"

    $i = 0
    foreach ($page in $selected) {
        foreach ($mode in $Modes) {
            $i++
            $runDir = Join-Path $Results (Join-Path $page.Family (Join-Path $page.Name $mode))
            if (Test-Path $runDir) { Remove-Item -Recurse -Force $runDir }
            New-Item -ItemType Directory -Force $runDir | Out-Null
            $envs = @{}
            if ($mode -ne 'composition') {
                $envs['WINUI_RENDERING_BACKEND'] = $mode
                $envs['WINUI_IMMEDIATE_STATS'] = Join-Path $runDir 'stats.txt'
                $envs['WINUI_IMMEDIATE_CAPTURE'] = Join-Path $runDir 'immediate.bmp'
            }
            $started = Get-Date
            $code = Invoke-Bench @('--coverage', $page.Id, '--coverage-out', $runDir) $envs (Join-Path $runDir 'stderr.txt') $TimeoutSeconds
            Set-Content -Path (Join-Path $runDir 'exit.txt') -Value $code
            Write-Host ("[{0}/{1}] {2} {3}: exit {4} ({5:0.0} s)" -f $i, ($selected.Count * $Modes.Count), $page.Id, $mode, $code, ((Get-Date) - $started).TotalSeconds)
        }
    }
}

& $analyzer coverage $Results
$gateExit = $LASTEXITCODE
if ($DeleteBmps) {
    # Per page, so a failing immediate run keeps its Composition reference for -AnalyzeOnly re-runs.
    Get-ChildItem $Results -Recurse -Filter result.json | Group-Object { Split-Path (Split-Path $_.FullName) } | ForEach-Object {
        if (-not ($_.Group | Where-Object { -not (Get-Content $_.FullName -Raw | ConvertFrom-Json).Passed })) {
            $_.Group | ForEach-Object { Get-ChildItem $_.DirectoryName -Filter *.bmp | Remove-Item -Force }
        }
    }
}
exit $gateExit
