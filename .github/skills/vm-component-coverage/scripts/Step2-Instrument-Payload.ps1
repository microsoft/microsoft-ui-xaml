<#
.SYNOPSIS
    Step 2 - Statically instrument the module and broadcast it into the local test payload.

.DESCRIPTION
    Static native instrumentation of one product DLL for a local payload that is deployed to a
    Hyper-V VM:

      * Instruments ONE copy (VS Microsoft.CodeCoverage.Console.exe, static native instrumentation,
        a fresh 16-hex session id) - this is slow for a 38 MB chk DLL (~10-20 min), so the result is
        CACHED in <WorkDir>\instrumented keyed by the SHA256 of the input DLL and reused next time
        (together with the session id that is baked into it). The cache is only reused while the
        coverage tool that instrumented it still exists at the same file version, because the
        collector (Step 3) must be that same tool.
      * Detects "fails while exiting 0" (no /PROFILE-style debug info) by checking the tool output
        and that the image grew, and requires static_covrun*.dll to be produced.
      * Backs up every payload copy of the module to <WorkDir>\payload-backup, then copies the
        instrumented DLL + static_covrun*.dll over every copy (a test app loads its own co-located
        copy, e.g. Test\UnpackagedApps\MUXControlsTestApp\). The backup is refreshed from the payload
        whenever that copy is not instrumented, so a rebuilt payload is never restored to an older
        build's DLL; an existing backup is kept only while the copy is still instrumented.
      * Writes <WorkDir>\instrument-state.json (session id, copies, backups, runtimes added) used by
        the run and restore steps.

    Never edits repo source. Only TestPayload\<flavor> (a generated folder) is modified, and
    Step4-Restore-Payload.ps1 restores it byte-for-byte.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$RepoRoot,
    [Parameter(Mandatory)][string]$WorkDir,
    # The instrumentable DLL (Step 1 output) and its PDB next to it.
    [Parameter(Mandatory)][string]$SourceDll,
    [string]$PayloadFlavor = 'x64chk',
    [string]$CoverageConsole = '',
    [switch]$ForceReinstrument
)
$ErrorActionPreference = 'Stop'
function Info($m) { Write-Host "[instrument] $m" }

if (-not $CoverageConsole) {
    # Highest file version wins (string-sorting install paths would rank "2022" above "18").
    $CoverageConsole = Get-ChildItem "${env:ProgramFiles}\Microsoft Visual Studio" -Recurse -Filter Microsoft.CodeCoverage.Console.exe -Depth 8 -EA SilentlyContinue |
        Sort-Object { [version]$_.VersionInfo.FileVersion } -Descending | Select-Object -First 1 -ExpandProperty FullName
}
if (-not $CoverageConsole -or -not (Test-Path $CoverageConsole)) { throw 'Microsoft.CodeCoverage.Console.exe not found (install VS Enterprise code coverage).' }
$toolVersion = (Get-Item $CoverageConsole).VersionInfo.FileVersion
Info "tool      : $CoverageConsole ($toolVersion)"

$moduleName = Split-Path $SourceDll -Leaf
$stem = [IO.Path]::GetFileNameWithoutExtension($moduleName)
$pdb = Join-Path (Split-Path $SourceDll) "$stem.pdb"
if (-not (Test-Path $pdb)) { throw "PDB must sit next to the DLL: $pdb" }

New-Item -ItemType Directory -Force $WorkDir | Out-Null
$WorkDir = (Resolve-Path $WorkDir).Path
$instrDir = Join-Path $WorkDir 'instrumented'
$cacheFile = Join-Path $instrDir 'instrumented-cache.json'
$srcHash = (Get-FileHash $SourceDll).Hash

# ---- 1. instrument (or reuse cache) ---------------------------------------------------------
$cache = if (Test-Path $cacheFile) { Get-Content $cacheFile -Raw | ConvertFrom-Json } else { $null }
$instrDll = Join-Path $instrDir $moduleName
$cacheHit = -not $ForceReinstrument -and $cache -and $cache.sourceHash -eq $srcHash -and (Test-Path $instrDll) -and
    (Get-FileHash $instrDll).Hash -eq $cache.instrumentedHash
# The collector must come from the same tool (and version) that instrumented the binary.
$cacheToolOk = $cache -and $cache.coverageTool -and (Test-Path $cache.coverageTool) -and
    "$((Get-Item $cache.coverageTool).VersionInfo.FileVersion)" -eq "$($cache.toolVersion)"
if ($cacheHit -and $cacheToolOk) {
    $sessionId = $cache.sessionId
    $CoverageConsole = $cache.coverageTool; $toolVersion = $cache.toolVersion
    Info "reusing cached instrumented DLL (session $sessionId, tool $toolVersion)"
} else {
    if ($cacheHit) { Info "cached DLL was instrumented by '$($cache.coverageTool)' $($cache.toolVersion), which is no longer available at that version; re-instrumenting" }
    Remove-Item $instrDir -Recurse -Force -EA SilentlyContinue
    New-Item -ItemType Directory -Force $instrDir | Out-Null
    Copy-Item $SourceDll, $pdb $instrDir
    $sessionId = [guid]::NewGuid().ToString('N').Substring(0, 16)
    $cfg = Join-Path $instrDir 'instrument.config'
    @'
<?xml version="1.0" encoding="utf-8"?>
<Configuration>
  <CodeCoverage>
    <EnableStaticNativeInstrumentation>True</EnableStaticNativeInstrumentation>
    <EnableDynamicNativeInstrumentation>False</EnableDynamicNativeInstrumentation>
    <ModulePaths><Include><ModulePath>.*</ModulePath></Include></ModulePaths>
  </CodeCoverage>
</Configuration>
'@ | Set-Content $cfg -Encoding UTF8
    $before = (Get-Item $instrDll).Length
    Info "instrumenting $moduleName with session $sessionId (slow for large chk DLLs)..."
    $sw = [Diagnostics.Stopwatch]::StartNew()
    $out = & $CoverageConsole instrument $instrDll --session-id $sessionId --settings $cfg --nologo 2>&1
    $exit = $LASTEXITCODE
    $text = ($out | ForEach-Object { "$_" }) -join "`n"
    $text | Set-Content (Join-Path $instrDir 'instrument.log')
    $after = (Get-Item $instrDll).Length
    Info ("instrument exit={0} in {1:mm\:ss}; size {2:N0} -> {3:N0}" -f $exit, $sw.Elapsed, $before, $after)
    if ($exit -ne 0 -or $text -match 'Unable to obtain debug information|/PROFILE linker switch' -or
        -not ($text -match 'successfully instrumented' -or $after -gt $before)) {
        throw "Instrumentation failed or silently skipped (see $instrDir\instrument.log). If it mentions /PROFILE, run Step1 (relink with /DEBUGTYPE:CV,FIXUP) first."
    }
    if (-not (Get-ChildItem $instrDir -Filter 'static_covrun*.dll')) { throw "static_covrun*.dll was not produced next to $instrDll" }
    [pscustomobject]@{ sessionId = $sessionId; sourceDll = $SourceDll; sourceHash = $srcHash
                       instrumentedHash = (Get-FileHash $instrDll).Hash; coverageTool = $CoverageConsole; toolVersion = $toolVersion
                       createdAt = (Get-Date).ToString('o') } |
        ConvertTo-Json | Set-Content $cacheFile -Encoding UTF8
}
$runtimes = @(Get-ChildItem $instrDir -Filter 'static_covrun*.dll')

# ---- 2. broadcast into the payload (back up originals) --------------------------------------
$payload = Join-Path $RepoRoot "TestPayload\$PayloadFlavor"
if (-not (Test-Path (Join-Path $payload 'runtests.cmd'))) { throw "Test payload not found at $payload. Run tools\run-tests-on-vm.ps1 (or test\CreateTestPayload.ps1) once first." }
$backupRoot = Join-Path $WorkDir 'payload-backup'
$statePath = Join-Path $WorkDir 'instrument-state.json'
$state = if (Test-Path $statePath) { Get-Content $statePath -Raw | ConvertFrom-Json } else { $null }
if ($state -and $state.payloadDir -ne $payload) { throw "instrument-state.json belongs to another payload ($($state.payloadDir)); restore it first." }

$copies = @(Get-ChildItem $payload -Recurse -Filter $moduleName -File)
if (-not $copies) { throw "No copies of $moduleName under $payload" }
$instrHash = (Get-FileHash $instrDll).Hash
$knownInstrumented = @($instrHash, $(if ($state) { $state.instrumentedHash })) | Where-Object { $_ }
$entries = @()
$runtimesAdded = @()
foreach ($c in $copies) {
    $rel = $c.FullName.Substring($payload.Length + 1)
    $bak = Join-Path $backupRoot $rel
    # A copy that is not instrumented is the payload's current original (possibly from a newer build
    # than an earlier backup), so always back it up. Keep an existing backup only while the copy is
    # still instrumented (e.g. an earlier run was not restored yet).
    $isInstrumented = ($knownInstrumented -contains (Get-FileHash $c.FullName).Hash) -or
        [bool](Get-ChildItem $c.DirectoryName -Filter 'static_covrun*.dll' -File -EA SilentlyContinue)
    if (-not $isInstrumented) {
        New-Item -ItemType Directory -Force (Split-Path $bak) | Out-Null
        Copy-Item $c.FullName $bak -Force
    } elseif (-not (Test-Path $bak)) {
        throw "$rel is already instrumented but has no backup; recreate the payload."
    }
    Copy-Item $instrDll $c.FullName -Force
    foreach ($rt in $runtimes) {
        $rtDest = Join-Path $c.DirectoryName $rt.Name
        if (-not (Test-Path $rtDest)) { $runtimesAdded += $rtDest.Substring($payload.Length + 1) }
        Copy-Item $rt.FullName $rtDest -Force
    }
    $entries += [pscustomobject]@{ relativePath = $rel; backup = $bak; originalHash = (Get-FileHash $bak).Hash }
    Info "  instrumented copy -> $rel"
}
if ($state -and $state.runtimesAdded) { $runtimesAdded = @($state.runtimesAdded) + $runtimesAdded | Select-Object -Unique }

$newState = [pscustomobject]@{
    module = $moduleName; moduleStem = $stem; sessionId = $sessionId; payloadDir = $payload
    instrumentedDll = $instrDll; instrumentedHash = $instrHash; coverageTool = $CoverageConsole; toolVersion = $toolVersion
    copies = $entries; runtimesAdded = @($runtimesAdded); instrumentedAt = (Get-Date).ToString('o')
}
$newState | ConvertTo-Json -Depth 5 | Set-Content $statePath -Encoding UTF8
Info "payload instrumented: $($entries.Count) copies, session $sessionId (state: $statePath)"
$newState
