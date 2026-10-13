<#
.SYNOPSIS
    Step 1 - Produce an instrumentable copy of a WinUI chk product DLL (default:
    Microsoft.UI.Xaml.Controls.dll) WITHOUT touching the repo or BuildOutput.

.DESCRIPTION
    Static native instrumentation (Microsoft.CodeCoverage.Console instrument) needs the CV + FIXUP
    debug streams. MUXC's Debug (chk) link does not emit them, so instrumenting the normal chk DLL
    prints "Unable to obtain debug information. Link with the /PROFILE linker switch." and exits 0
    with the image untouched. Emitting them needs /DEBUGTYPE:CV,FIXUP with /INCREMENTAL:NO at link
    time.

    This skill must not edit repo files, so instead of changing the build this script re-runs the
    EXACT link command MSBuild last recorded (link.command.1.tlog) with:
      * /DEBUGTYPE:CV,FIXUP added (and /INCREMENTAL:NO enforced),
      * /OUT, /PDB, /IMPLIB, /MAP redirected into -OutputDir,
      * /WINMD disabled (the .winmd is not needed for coverage).
    The repo's BuildOutput DLL/PDB are never overwritten. Object files are reused as-is, so the code
    is byte-for-byte the same build - only the debug info differs.

    Microsoft.ui.xaml.dll (MUX) already links with /debugtype:cv,fixup,pdata, so for MUX you can skip
    this step and instrument BuildOutput\bin\<flavor>\Product\Microsoft.ui.xaml.dll directly.

.EXAMPLE
    .\Step1-New-InstrumentableDll.ps1 -RepoRoot C:\src\microsoft-ui-xaml -OutputDir C:\cov\work\relink
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$RepoRoot,
    [Parameter(Mandatory)][string]$OutputDir,
    [string]$Flavor = 'amd64chk',
    # Project-relative obj dir that holds the link .tlog for the module.
    [string]$ObjSubDir = 'controls\dev\dll',
    # Directory the original link ran in (relative paths such as /DEF are resolved from here).
    [string]$ProjectSubDir = 'controls\dev\dll',
    [string]$ModuleName = 'Microsoft.UI.Xaml.Controls.dll'
)
$ErrorActionPreference = 'Stop'
function Info($m) { Write-Host "[relink] $m" }

$objDir = Join-Path $RepoRoot "BuildOutput\obj\$Flavor\$ObjSubDir"
$projDir = Join-Path $RepoRoot $ProjectSubDir
if (-not (Test-Path $objDir)) { throw "obj dir not found: $objDir (build the repo first)" }

$stem = [IO.Path]::GetFileNameWithoutExtension($ModuleName)
$tlog = Get-ChildItem $objDir -Recurse -Filter 'link.command.1.tlog' |
    Where-Object { ([IO.File]::ReadAllText($_.FullName)) -match [regex]::Escape("\$stem.dll".ToUpperInvariant()) } |
    Select-Object -First 1
if (-not $tlog) { throw "No link.command.1.tlog under $objDir mentions $ModuleName" }
Info "link tlog : $($tlog.FullName)"

# tlog layout: line 0 = '^<sources>' (input list), remaining lines = the command line (options + objs).
$lines = [IO.File]::ReadAllText($tlog.FullName) -split "`r?`n"
$cmd = ($lines | Select-Object -Skip 1 | Where-Object { $_ -and -not $_.StartsWith('^') }) -join "`n"
if (-not $cmd) { throw "Could not read link command from $($tlog.FullName)" }

New-Item -ItemType Directory -Force $OutputDir | Out-Null
$OutputDir = (Resolve-Path $OutputDir).Path
$outDll = Join-Path $OutputDir $ModuleName
$outPdb = Join-Path $OutputDir "$stem.pdb"
$outLib = Join-Path $OutputDir "$stem.lib"
$outMap = Join-Path $OutputDir "$stem.map"

# The link is not byte-deterministic, and Step 2 caches the (slow) instrumentation by the relinked
# DLL's hash, so reuse the previous relink when the recorded link command/inputs are unchanged.
$resultFile = Join-Path $OutputDir 'relink-result.json'
$inputsStamp = '{0}|{1}' -f (Get-FileHash $tlog.FullName).Hash,
    ((Get-ChildItem (Split-Path $tlog.FullName) -Filter 'link.read.*.tlog' | Get-FileHash | ForEach-Object Hash) -join ',')
$newestObj = Get-ChildItem $objDir -Filter *.obj | Sort-Object LastWriteTimeUtc -Descending | Select-Object -First 1
if ((Test-Path $resultFile) -and (Test-Path $outDll) -and (Test-Path $outPdb)) {
    $prev = Get-Content $resultFile -Raw | ConvertFrom-Json
    if ($prev.inputsStamp -eq $inputsStamp -and (-not $newestObj -or $newestObj.LastWriteTimeUtc -lt (Get-Item $outDll).LastWriteTimeUtc)) {
        Info "reusing previous relink (link inputs unchanged): $outDll"
        return [pscustomobject]@{ dll = $outDll; pdb = $outPdb }
    }
}

$q = '(?:"[^"]*"|\S+)'
$cmd = [regex]::Replace($cmd, "(?i)/OUT:$q",      "/OUT:`"$outDll`"")
$cmd = [regex]::Replace($cmd, "(?i)/PDB:$q",      "/PDB:`"$outPdb`"")
$cmd = [regex]::Replace($cmd, "(?i)/IMPLIB:$q",   "/IMPLIB:`"$outLib`"")
$cmd = [regex]::Replace($cmd, "(?i)/WINMDFILE:$q", '')
$cmd = [regex]::Replace($cmd, "(?i)/WINMD(?=\s)",  '/WINMD:NO')
$cmd = [regex]::Replace($cmd, "(?i)/MAP(?=\s)",    "/MAP:`"$outMap`"")
$cmd = [regex]::Replace($cmd, "(?i)/INCREMENTAL(:\w+)?", '/INCREMENTAL:NO')
$cmd = [regex]::Replace($cmd, "(?i)/DEBUGTYPE:$q", '')
$cmd += "`n/DEBUGTYPE:CV,FIXUP"

$rsp = Join-Path $OutputDir 'relink.rsp'
[IO.File]::WriteAllText($rsp, $cmd, [Text.Encoding]::Unicode)
Info "response  : $rsp"

# Resolve link.exe from the MSVC toolset recorded in the command's /LIBPATH (same toolset that built the objs).
$link = $null
$m = [regex]::Match($cmd, '(?i)"([^"]*\\VC\\TOOLS\\MSVC\\[\d\.]+)\\LIB\\X64"')
if ($m.Success) {
    $cand = Join-Path $m.Groups[1].Value 'bin\Hostx64\x64\link.exe'
    if (Test-Path $cand) { $link = $cand }
}
if (-not $link) {
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    $vs = & $vswhere -latest -products * -property installationPath
    $link = Get-ChildItem "$vs\VC\Tools\MSVC" -Recurse -Filter link.exe | Where-Object FullName -match 'Hostx64\\x64' | Select-Object -First 1 -ExpandProperty FullName
}
if (-not $link) { throw 'link.exe not found' }
Info "linker    : $link"

$sw = [Diagnostics.Stopwatch]::StartNew()
Push-Location $projDir
try {
    $out = & $link "@$rsp" 2>&1
    $exit = $LASTEXITCODE
} finally { Pop-Location }
$out | Where-Object { $_ -match 'error|warning LNK' } | Select-Object -First 20 | ForEach-Object { Info "  $_" }
if ($exit -ne 0 -or -not (Test-Path $outDll)) { throw "relink failed (exit $exit)" }
Info ("relinked in {0:mm\:ss}: {1} ({2:N0} bytes)" -f $sw.Elapsed, $outDll, (Get-Item $outDll).Length)

[pscustomobject]@{ dll = $outDll; pdb = $outPdb; linkTlog = $tlog.FullName; linker = $link; rsp = $rsp; inputsStamp = $inputsStamp } |
    ConvertTo-Json | Set-Content $resultFile -Encoding UTF8
[pscustomobject]@{ dll = $outDll; pdb = $outPdb }
