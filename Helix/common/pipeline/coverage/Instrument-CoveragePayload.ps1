# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
#
# Instrument-CoveragePayload.ps1 - Statically instrument the product DLL(s) in a test payload
# so a coverage collector can record which lines run.
#
# Runs on the build agent, inside the CreateTestPayload job (when -CollectCodeCoverage is on).
#
# The payload contains SEVERAL copies of Microsoft.ui.xaml.dll (root, Test\, and each
# unpackaged app's own dir).  A test app loads its OWN co-located copy, so we must instrument
# the copy it actually loads.  Instrumenting the huge product DLL is slow, so we instrument
# ONCE and copy the instrumented binary (plus the static_covrun*.dll runtime) over every other
# copy - they are the same build, so one instrumented binary works everywhere.
#
# Instrumentation needs the matching PDB next to the DLL.  Point -SymbolsSearchRoot at a tree
# that contains Microsoft.ui.xaml.pdb (e.g. the build drop's Symbols\Product).  We remove the
# PDBs again afterward so the published payload stays lean (line mapping is baked into the
# instrumented binary at instrument time, so the collector/merge do not need the PDB).
#
# Each module in -Modules gets its OWN independent session id (auto-generated unless -SessionIds
# supplies one, matched by position), and therefore its own named pipe / collector / .coverage
# file at collect time - there is no shared listener across modules. This lets Microsoft.ui.xaml.dll
# (MUX) and Microsoft.UI.Xaml.Controls.dll (MUXC) be instrumented and collected completely
# independently of one another. The session id actually baked into each module is recorded in a
# per-module discovery file, _coverage-session-id-<ModuleStem>.txt, which
# RunTestPassSliceOnBuildAgent.ps1 auto-discovers to know which collectors to start.
#
# EXPERIMENTAL QUALITY.

[CmdletBinding()]
param(
    # Payload root, e.g. $(Build.SourcesDirectory)\TestPayload\x86chk
    [Parameter(Mandatory = $true)]
    [string]$PayloadDir,

    # Session ID(s) to bake into the instrumented binaries, matched to -Modules by position
    # (index 0 of -SessionIds goes with index 0 of -Modules, etc.). Any entry left empty, or any
    # position beyond the end of this array, is auto-generated. Each module's session id is
    # independent - a distinct pipe/collector/.coverage file per module at collect time.
    [string[]]$SessionIds = @(),

    # Which modules to instrument.  Defaults to just Microsoft.ui.xaml.dll (the framework/core,
    # "MUX") for this focused coverage pass. Pass -Modules with additional entries (e.g.
    # Microsoft.UI.Xaml.Controls.dll, the controls / "MUXC") to broaden coverage later.
    [string[]]$Modules = @("Microsoft.ui.xaml.dll"),

    # A tree that contains the matching PDBs (searched recursively).  If empty we look for a
    # PDB next to one of the DLL copies.
    [string]$SymbolsSearchRoot = "",

    # Explicit path to Microsoft.CodeCoverage.Console.exe or dotnet-coverage(.exe).
    # If empty we auto-resolve (prefer the VS console tool, else the dotnet-coverage tool).
    [string]$CoverageToolPath = "",

    # Keep the PDBs in the payload (default: remove them to keep the artifact small).
    [switch]$KeepSymbols
)

$ErrorActionPreference = "Stop"

# Validate the session ids up front, before any slow work. A session id must be usable as a
# named-pipe suffix and as a filename fragment, and an entry that looks like a module name is a
# sure sign the caller passed -Modules space-separated (PowerShell then binds only the first
# token to -Modules and spills the rest onto -SessionIds, the next unbound positional
# parameter). Fail loudly instead of baking nonsense into the binary.
for ($sidIdx = 0; $sidIdx -lt $SessionIds.Count; $sidIdx++) {
    $sid = $SessionIds[$sidIdx]
    if (-not $sid) { continue }
    if ($sid -like "*.dll") {
        throw "SessionIds[$sidIdx] is '$sid', which looks like a module name. -Modules must be comma-separated (e.g. -Modules a.dll,b.dll); space-separated values spill onto -SessionIds."
    }
    if ($sid -match '[\\/:*?"<>|\s]') {
        throw "SessionIds[$sidIdx] ('$sid') contains characters that are not valid in a named pipe name or filename."
    }
}
if ($SessionIds.Count -gt $Modules.Count) {
    throw "Got $($SessionIds.Count) session id(s) for only $($Modules.Count) module(s). Session ids are matched to modules by position, so extra ids are always a caller bug."
}

# ---------------------------------------------------------------------------
# Resolve the coverage tool.  Native static instrumentation is best supported by the VS
# Microsoft.CodeCoverage.Console.exe, so we prefer it; dotnet-coverage is the portable
# fallback.  Both share the instrument/collect/merge/shutdown command syntax we use.
# ---------------------------------------------------------------------------
function Resolve-CoverageTool {
    param([string]$Explicit)

    if ($Explicit -and (Test-Path $Explicit)) { return $Explicit }

    $vs = Get-ChildItem -Path @(
        "${env:ProgramFiles}\Microsoft Visual Studio",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio"
    ) -Recurse -Filter "Microsoft.CodeCoverage.Console.exe" -ErrorAction SilentlyContinue |
        Select-Object -First 1
    if ($vs) { return $vs.FullName }

    $dc = Get-Command "dotnet-coverage" -ErrorAction SilentlyContinue
    if ($dc) { return $dc.Source }

    Write-Host "dotnet-coverage not found; installing the global tool..."
    & dotnet tool install --global dotnet-coverage --add-source https://api.nuget.org/v3/index.json --ignore-failed-sources | Write-Host
    $env:PATH = "$env:PATH;$env:USERPROFILE\.dotnet\tools"
    $dc = Get-Command "dotnet-coverage" -ErrorAction SilentlyContinue
    if ($dc) { return $dc.Source }

    throw "Could not find or install a code coverage tool (Microsoft.CodeCoverage.Console.exe or dotnet-coverage)."
}

# Copy the VS code-coverage tools into the payload so the test agents can collect without any
# VS/dotnet install (root + amd64 + x64 + x86, self-contained). The lab test agents have neither
# Visual Studio nor the dotnet CLI, so the collector tool has to ride along in the
# payload. Best-effort: only works when the resolved tool is the VS Console exe (we can find the
# sibling engines relative to it); warns and skips otherwise.
function Add-CoverageToolsToPayload {
    param([string]$CoverageToolPath, [string]$PayloadDir)

    if ($CoverageToolPath -notmatch "Microsoft\.CodeCoverage\.Console\.exe$") {
        Write-Host "  Skipping tool staging: resolved tool is not the VS Console exe ($CoverageToolPath)." -ForegroundColor Yellow
        return
    }
    $marker = "\Common7\"
    $idx = $CoverageToolPath.IndexOf($marker)
    if ($idx -lt 0) {
        Write-Host "  Skipping tool staging: cannot derive VS base from $CoverageToolPath." -ForegroundColor Yellow
        return
    }
    $vsBase = $CoverageToolPath.Substring(0, $idx)

    $consoleSrc  = Join-Path $vsBase "Common7\IDE\Extensions\Microsoft\CodeCoverage.Console"
    $vanguardSrc = Join-Path $vsBase "Team Tools\Dynamic Code Coverage Tools"
    $ieSrc       = Join-Path $vsBase "Common7\IDE\CommonExtensions\Platform\InstrumentationEngine"

    if (-not (Test-Path (Join-Path $consoleSrc "Microsoft.CodeCoverage.Console.exe"))) {
        Write-Host "  Skipping tool staging: CodeCoverage.Console not found at $consoleSrc." -ForegroundColor Yellow
        return
    }

    $destRoot = Join-Path $PayloadDir "CoverageTool"
    Write-Host "`nStaging VS code-coverage tools into payload ($destRoot) for the test agents..."
    New-Item -ItemType Directory $destRoot -Force | Out-Null
    New-Item -ItemType Directory (Join-Path $destRoot "amd64") -Force | Out-Null
    New-Item -ItemType Directory (Join-Path $destRoot "x64") -Force | Out-Null
    New-Item -ItemType Directory (Join-Path $destRoot "x86") -Force | Out-Null

    $oldPP = $ProgressPreference
    $ProgressPreference = 'SilentlyContinue'
    try {
        Copy-Item -Path "$consoleSrc\*" -Destination $destRoot -Recurse -Force
        if (Test-Path $vanguardSrc) {
            Copy-Item -Path "$vanguardSrc\*" -Destination $destRoot -Recurse -Force
            if (Test-Path (Join-Path $vanguardSrc "amd64")) {
                Copy-Item -Path (Join-Path $vanguardSrc "amd64\*") -Destination (Join-Path $destRoot "amd64") -Recurse -Force
            }
        }
        if (Test-Path (Join-Path $ieSrc "x64")) {
            Copy-Item -Path (Join-Path $ieSrc "x64\*") -Destination (Join-Path $destRoot "x64") -Recurse -Force
        }
        if (Test-Path (Join-Path $ieSrc "x86")) {
            Copy-Item -Path (Join-Path $ieSrc "x86\*") -Destination (Join-Path $destRoot "x86") -Recurse -Force
        }
    } finally {
        $ProgressPreference = $oldPP
    }
    Write-Host "  Staged coverage tools into payload."
}

$covTool = Resolve-CoverageTool -Explicit $CoverageToolPath
Write-Host "Coverage tool: $covTool"
if ($covTool -notmatch "Microsoft\.CodeCoverage\.Console\.exe$") {
    Write-Host "WARNING: Using dotnet-coverage. Native static instrumentation support is best-tested" -ForegroundColor Yellow
    Write-Host "         with the VS Microsoft.CodeCoverage.Console.exe. Verify on a real run." -ForegroundColor Yellow
}

Write-Host "Modules to instrument: $($Modules -join ', ')"

# Per-module failures are tracked rather than fatal: with more than one module we still want the
# modules that DID instrument to produce coverage. We only fail the whole script if nothing
# instrumented at all (which keeps the single-module behaviour identical to before).
$instrumentedModules = @()
$failedModules = @()

# Static native instrumentation settings.
$settingsPath = Join-Path $env:TEMP "winui-coverage-instr.config"
@'
<?xml version="1.0" encoding="utf-8"?>
<Configuration>
  <CodeCoverage>
    <EnableStaticNativeInstrumentation>True</EnableStaticNativeInstrumentation>
    <EnableDynamicNativeInstrumentation>False</EnableDynamicNativeInstrumentation>
    <ModulePaths>
      <Include>
        <ModulePath>.*</ModulePath>
      </Include>
    </ModulePaths>
  </CodeCoverage>
</Configuration>
'@ | Set-Content $settingsPath -Encoding UTF8

for ($moduleIdx = 0; $moduleIdx -lt $Modules.Count; $moduleIdx++) {
    $module = $Modules[$moduleIdx]

    # Each module gets its own requested (or auto-generated) session id, independent of every
    # other module's. This is what gives Microsoft.ui.xaml.dll and Microsoft.UI.Xaml.Controls.dll
    # (when both are in -Modules) their own separate pipe/collector/.coverage file at collect
    # time - no shared listener between modules.
    $requestedSessionId = if ($moduleIdx -lt $SessionIds.Count -and $SessionIds[$moduleIdx]) { $SessionIds[$moduleIdx] } else { [guid]::NewGuid().ToString("N").Substring(0, 16) }

    Write-Host "`n=== Instrumenting $module (requested session id: $requestedSessionId) ==="
    $copies = @(Get-ChildItem -Path $PayloadDir -Recurse -Filter $module -ErrorAction SilentlyContinue |
                Where-Object { -not $_.PSIsContainer })
    if ($copies.Count -eq 0) {
        Write-Host "##vso[task.logissue type=warning]No copies of $module found under $PayloadDir; skipping."
        $failedModules += $module
        continue
    }
    Write-Host "  Found $($copies.Count) copies:"
    $copies | ForEach-Object { Write-Host "    $($_.FullName)" }

    $pdbName = [System.IO.Path]::GetFileNameWithoutExtension($module) + ".pdb"

    # Locate a matching PDB.
    $pdbSource = $null
    if ($SymbolsSearchRoot -and (Test-Path $SymbolsSearchRoot)) {
        $pdbSource = Get-ChildItem -Path $SymbolsSearchRoot -Recurse -Filter $pdbName -ErrorAction SilentlyContinue |
                     Select-Object -First 1
    }
    if (-not $pdbSource) {
        # Fall back to a PDB already sitting next to one of the copies.
        foreach ($c in $copies) {
            $sibling = Join-Path $c.DirectoryName $pdbName
            if (Test-Path $sibling) { $pdbSource = Get-Item $sibling; break }
        }
    }
    if (-not $pdbSource) {
        Write-Host "##vso[task.logissue type=warning]Could not find $pdbName (needed to instrument $module) under '$SymbolsSearchRoot' or next to any payload copy; skipping this module."
        $failedModules += $module
        continue
    }
    Write-Host "  PDB: $($pdbSource.FullName)"

    # Instrument the first copy in place (PDB must sit next to it).
    $primary = $copies[0]
    $primaryDir = $primary.DirectoryName

    # Idempotency: instrumenting an already-instrumented binary fails, so we must not do it twice
    # (e.g. a retried/resumed job in the same workspace). We drop a per-module marker after a
    # successful instrument and skip re-instrument if it's present. A marker is used instead of a
    # size threshold because each module inflates to a different size when instrumented.
    $moduleStem = [System.IO.Path]::GetFileNameWithoutExtension($module)
    $markerFile = Join-Path $PayloadDir "_coverage-instrumented-$moduleStem.txt"

    # The marker records the session id AND the SHA256 of the instrumented DLL. We
    # re-instrument unless the marker is present and the primary DLL still matches
    # the recorded hash. That way a refreshed payload (DLL overwritten without the
    # marker being cleared) is detected and re-instrumented, instead of skipping and
    # broadcasting an uninstrumented binary.
    $shouldInstrument = $true
    $recordedId = ""
    if (Test-Path $markerFile) {
        $markerLines  = @(Get-Content $markerFile -ErrorAction SilentlyContinue)
        $recordedId   = ($markerLines | Select-Object -First 1).ToString().Trim()
        $recordedHash = if ($markerLines.Count -ge 2) { $markerLines[1].ToString().Trim() } else { "" }
        $currentHash  = (Get-FileHash -Path $primary.FullName -Algorithm SHA256).Hash

        if (-not $recordedHash) {
            Write-Host "  Marker present but has no DLL hash (legacy marker); re-instrumenting." -ForegroundColor Yellow
            Remove-Item $markerFile -Force -ErrorAction SilentlyContinue
            $recordedId = ""
        }
        elseif ($recordedHash -ne $currentHash) {
            Write-Host "  Primary DLL changed since it was instrumented; re-instrumenting." -ForegroundColor Yellow
            Remove-Item $markerFile -Force -ErrorAction SilentlyContinue
            $recordedId = ""
        }
        else {
            $shouldInstrument = $false
            if ($recordedId -and $recordedId -ne $requestedSessionId) {
                Write-Host "  WARNING: $module is already instrumented with session '$recordedId', but '$requestedSessionId' was requested." -ForegroundColor Yellow
                Write-Host "           The baked-in id wins. Rebuild the payload from a fresh DLL to change it." -ForegroundColor Yellow
            }
            Write-Host "  Primary already instrumented (marker + DLL hash match); skipping re-instrument, re-broadcasting only."
        }
    }

    # The effective session id is whatever is ACTUALLY baked into the DLL: on the instrument
    # path that's the just-requested id; on the skip path it's the id recorded in the marker
    # (which may differ from what was requested - see the warning above). Writing anything
    # else to the discovery file below would tell the collector to listen on a pipe the
    # instrumented binary never talks to.
    $effectiveSessionId = if ($shouldInstrument) { $requestedSessionId } else { $recordedId }

    if ($shouldInstrument) {
        # The PDB must sit next to the DLL being instrumented. Skip the copy when it already
        # does (the -SymbolsSearchRoot fallback finds a sibling PDB), since Copy-Item throws on
        # a self-copy.
        $pdbDest = Join-Path $primaryDir $pdbName
        if ($pdbSource.FullName -ne $pdbDest) {
            Copy-Item $pdbSource.FullName $pdbDest -Force
        }

        Write-Host "  Instrumenting primary copy: $($primary.FullName)"
        $sizeBefore = (Get-Item $primary.FullName).Length
        $instrOutput = & $covTool instrument $primary.FullName --session-id $requestedSessionId --settings $settingsPath 2>&1
        $instrExit = $LASTEXITCODE
        $instrOutput | ForEach-Object { Write-Host "    $_" }
        $sizeAfter = (Get-Item $primary.FullName).Length

        # The VS instrumenter can FAIL WHILE STILL EXITING 0: e.g. a binary that was not linked
        # with /PROFILE makes it print "Unable to obtain debug information..." and leave the
        # image untouched. Trusting the exit code alone yields a green run that publishes an
        # empty report, so corroborate with the tool's own success/failure markers and with the
        # fact that instrumentation always grows the image.
        $instrText   = ($instrOutput | ForEach-Object { $_.ToString() }) -join "`n"
        $sawFailure  = $instrText -match 'Unable to obtain debug information|/PROFILE linker switch'
        $sawSuccess  = $instrText -match 'successfully instrumented'
        $imageGrew   = $sizeAfter -gt $sizeBefore

        if ($instrExit -ne 0 -or $sawFailure -or -not ($sawSuccess -or $imageGrew)) {
            $why = if ($instrExit -ne 0) { "exit $instrExit" }
                   elseif ($sawFailure)  { "the tool reported it could not obtain debug information (the binary likely needs to be re-linked with /PROFILE)" }
                   else                  { "no success marker and the image did not grow ($sizeBefore -> $sizeAfter bytes), so it was left uninstrumented" }
            Write-Host "##vso[task.logissue type=warning]Instrumentation failed for $($primary.FullName): $why. Skipping module $module - it will produce no coverage data."
            $failedModules += $module
            continue
        }

        $instrSize = [math]::Round((Get-Item $primary.FullName).Length / 1MB, 1)
        Write-Host "  Instrumented ($instrSize MB)."

        # Record the hash of the just-instrumented DLL so a later run can tell if the
        # payload was refreshed underneath us.
        $instrumentedHash = (Get-FileHash -Path $primary.FullName -Algorithm SHA256).Hash
        Set-Content $markerFile -Value @($requestedSessionId, $instrumentedHash) -Encoding ASCII
    }

    # The static coverage runtimes land next to the instrumented DLL.
    $runtimes = @(Get-ChildItem -Path $primaryDir -Filter "static_covrun*.dll" -ErrorAction SilentlyContinue)
    if ($runtimes.Count -eq 0) {
        Write-Host "##vso[task.logissue type=warning]Instrumentation did not produce static_covrun*.dll next to $($primary.FullName); skipping module $module."
        $failedModules += $module
        continue
    }

    # Broadcast: overwrite every other copy with the instrumented binary + runtimes.
    # The primary copy and its runtimes are already in place, so skip any self-copy
    # (e.g. a runtime whose destination dir is the primary's own dir).
    foreach ($c in $copies) {
        if ($c.FullName -ne $primary.FullName) {
            Copy-Item $primary.FullName $c.FullName -Force
        }
        foreach ($rt in $runtimes) {
            $rtDest = Join-Path $c.DirectoryName $rt.Name
            if ($rtDest -ne $rt.FullName) {
                Copy-Item $rt.FullName $rtDest -Force
            }
        }
    }
    Write-Host "  Broadcast instrumented $module + runtimes to all $($copies.Count) locations."

    # Record this module's effective session id so the collector step
    # (RunTestPassSliceOnBuildAgent.ps1) can discover it and start a dedicated collector for it.
    Set-Content (Join-Path $PayloadDir "_coverage-session-id-$moduleStem.txt") $effectiveSessionId -Encoding ASCII -NoNewline
    Write-Host "  Session ID for $module : $effectiveSessionId (recorded in _coverage-session-id-$moduleStem.txt)"
    $instrumentedModules += $module
}

Write-Host "`nInstrumented $($instrumentedModules.Count) of $($Modules.Count) requested module(s): $($instrumentedModules -join ', ')"
if ($failedModules.Count -gt 0) {
    Write-Host "##vso[task.logissue type=warning]Skipped module(s): $($failedModules -join ', '). Those modules will produce no coverage data."
}
if ($instrumentedModules.Count -eq 0) {
    throw "No modules were instrumented (requested: $($Modules -join ', ')). Coverage would produce nothing, so failing here rather than publishing an empty report."
}

# Bundle the collector tool into the payload so the lab test agents (no VS, no dotnet) can use it.
Add-CoverageToolsToPayload -CoverageToolPath $covTool -PayloadDir $PayloadDir

# Only the VS console tool can be staged into the payload. If that is the resolved tool but the
# bundle is missing, fail loudly: the lab test agents have no VS/dotnet, so coverage would
# silently produce nothing (a green run with an empty report). If we fell back to dotnet-coverage
# (e.g. a non-lab/local run), just warn - the payload is not self-contained for lab agents, but
# the run can still proceed where a collector is already present.
$stagedTool = Join-Path $PayloadDir "CoverageTool\Microsoft.CodeCoverage.Console.exe"
$isVsConsoleTool = ($covTool -match "Microsoft\.CodeCoverage\.Console\.exe$")
if ($isVsConsoleTool) {
    if (-not (Test-Path $stagedTool)) {
        throw "Coverage collector was not bundled into the payload ($stagedTool missing). This usually means the VS Microsoft.CodeCoverage.Console.exe could not be found on the build agent. Coverage would produce no data on the lab test agents."
    }
    Write-Host "  Verified bundled collector: $stagedTool"
}
elseif (-not (Test-Path $stagedTool)) {
    Write-Host "WARNING: CoverageTool was not bundled because the resolved tool is not the VS Microsoft.CodeCoverage.Console.exe ($covTool). The payload is not self-contained; lab test agents will not be able to collect coverage." -ForegroundColor Yellow
}

# Trim PDBs so the published payload stays small (mapping is baked into the .coverage).
if (-not $KeepSymbols) {
    Get-ChildItem -Path $PayloadDir -Recurse -Filter "*.pdb" -ErrorAction SilentlyContinue |
        Remove-Item -Force -ErrorAction SilentlyContinue
    Write-Host "`nRemoved PDBs from payload (use -KeepSymbols to keep them)."
}

Write-Host "`nPayload instrumented."
