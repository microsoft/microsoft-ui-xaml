# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
#
# Convert-CoverageToScc.ps1 - Convert the coverage pass's per-slice .coverage files into
# StackSampler sampled-coverage (.scc) files that can be uploaded with `StackSampler.exe upload`.
#
# Wraps an existing .coverage -> .scc converter (a Python package, common_coverage, whose location
# the caller passes in -ConverterRoot) rather than re-implementing it. Steps:
#   1. Group every slice's .coverage files by the BuildLabEx of the test host that produced them
#      (read from the _coverage-host-os.json the RunTests slice writes next to them). StackSampler
#      groups uploads by BuildLabEx, so each group becomes one .scc.
#   2. Merge each group's files (all slices, all modules) to Microsoft coverage XML.
#   3. Run coverage_to_scc.py against the ORIGINAL (uninstrumented) DLLs/PDBs saved by
#      Instrument-CoveragePayload.ps1. Instrumentation changes the PDB identity and RVA layout, so the
#      instrumented pair cannot be used; the DLLs must be in the working directory so the converter
#      records their checksum/timestamp/size. This produces a combined .scc (every module) and, with
#      more than one module, one .scc per module (-MUX / -MUXC for the WinUI DLLs, via --filter).
#      Upload either the combined file or the per-module files for a build, not both.
#   4. Validate each .scc (SQLite integrity, BuildLabEx present, every module resolved with hits and
#      matching the original DLL's PDB signature) and write scc-summary.json + UPLOAD.md.
#
# Tools are resolved, and fetched only when missing: dotnet-coverage (or the VS console tool), a
# 64-bit Python 3 (else the official `python` NuGet package), comtypes (pip, into a private folder),
# and msdia140.dll from the Visual Studio DIA SDK (not redistributed by this script).
#
# .scc is hit-only: covered/partially covered lines become RVA hits; uncovered lines and hit counts
# are not represented. Uploading needs an authorized account, so it is a manual step (see the
# generated UPLOAD.md).
#
# EXPERIMENTAL QUALITY.

[CmdletBinding()]
param(
    # Root searched recursively for the slices' *.coverage files (and _coverage-host-os.json).
    [Parameter(Mandatory = $true)]
    [string]$InputDir,

    # coverage-binaries.zip produced by the CreateTestPayload job, or an already-extracted folder
    # with <ModuleStem>\original\<Module> + <ModuleStem>.pdb (and optionally \instrumented\).
    [Parameter(Mandatory = $true)]
    [string]$CoverageBinaries,

    # The converter package: the folder that contains common_coverage\ (with cli\coverage_to_scc.py).
    [Parameter(Mandatory = $true)]
    [string]$ConverterRoot,

    [Parameter(Mandatory = $true)]
    [string]$OutputDir,

    # Instrumented DLLs to convert. Each must have an original DLL/PDB in -CoverageBinaries.
    [string[]]$Modules = @('Microsoft.ui.xaml.dll', 'Microsoft.UI.Xaml.Controls.dll'),

    # Base name of the .scc file(s), e.g. winui-<buildId>-Win10-RS5-x86chk.
    [string]$SccName = 'winui-coverage',

    # Test-pass name to suggest for `StackSampler.exe upload --name`.
    [string]$UploadName = '',

    # Scratch space (merged XML is several hundred MB). Defaults under the agent temp dir.
    [string]$WorkDir = '',

    [string]$CoverageToolPath = '',
    [string]$PythonPath = '',
    [string]$MsdiaPath = '',

    # Used only when no suitable Python is installed.
    [string]$PythonNuGetVersion = '3.12.10',
    # Used only when dotnet-coverage is missing and cannot be installed as a global tool.
    [string]$DotnetCoverageVersion = '18.12.0',
    # Feeds tried in order. Pipelines that must use internal package mirrors pass their own list.
    [string[]]$NuGetSources = @('https://api.nuget.org/v3/index.json'),

    # Used only when comtypes is not already importable.
    [string]$ComtypesVersion = '1.5.0',
    [string[]]$PipIndexUrls = @('https://pypi.org/simple/')
)

$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
[Net.ServicePointManager]::SecurityProtocol = [Net.ServicePointManager]::SecurityProtocol -bor [Net.SecurityProtocolType]::Tls12
$overallTimer = [Diagnostics.Stopwatch]::StartNew()

function Write-AdoWarning([string]$Message) { Write-Host "##vso[task.logissue type=warning]$Message" }
function Write-AdoError([string]$Message) { Write-Host "##vso[task.logissue type=error]$Message" }

# Runs a native command, streaming output to the console and a log file. Windows PowerShell turns
# native stderr into terminating errors under 'Stop', so relax that for the call only.
function Invoke-Native {
    param([string]$Exe, [string[]]$Arguments, [string]$LogFile = '', [string]$WorkingDirectory = '')
    $oldEap = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    if ($WorkingDirectory) { Push-Location $WorkingDirectory }
    try {
        $lines = & $Exe @Arguments 2>&1 | ForEach-Object { "$_" }
        $code = $LASTEXITCODE
    }
    finally {
        if ($WorkingDirectory) { Pop-Location }
        $ErrorActionPreference = $oldEap
    }
    $lines | ForEach-Object { Write-Host "    $_" }
    if ($LogFile) { $lines | Add-Content -Path $LogFile -Encoding UTF8 }
    return [pscustomobject]@{ ExitCode = $code; Output = @($lines) }
}

# ---------------------------------------------------------------------------------------------
# Tool resolution
# ---------------------------------------------------------------------------------------------
function Resolve-CoverageTool {
    param([string]$Explicit, [string]$ToolsDir)
    $asTool = { param($exe) [pscustomobject]@{ Exe = $exe; Prefix = @(); Display = $exe } }
    if ($Explicit -and (Test-Path $Explicit)) { return (& $asTool $Explicit) }
    $dc = Get-Command 'dotnet-coverage' -ErrorAction SilentlyContinue
    if ($dc) { return (& $asTool $dc.Source) }
    $userTool = Join-Path $env:USERPROFILE '.dotnet\tools\dotnet-coverage.exe'
    if (Test-Path $userTool) { return (& $asTool $userTool) }
    $vs = Get-ChildItem -Path @("${env:ProgramFiles}\Microsoft Visual Studio", "${env:ProgramFiles(x86)}\Microsoft Visual Studio") `
        -Recurse -Filter 'Microsoft.CodeCoverage.Console.exe' -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($vs) { return (& $asTool $vs.FullName) }
    $dotnet = Get-Command 'dotnet' -ErrorAction SilentlyContinue
    if ($dotnet) {
        Write-Host 'dotnet-coverage not found; installing the global tool...'
        $null = Invoke-Native -Exe 'dotnet' -Arguments @('tool', 'install', '--global', 'dotnet-coverage', '--add-source', 'https://api.nuget.org/v3/index.json', '--ignore-failed-sources')
        if (Test-Path $userTool) { return (& $asTool $userTool) }
        # No .NET SDK (runtime only): run the tool package's framework-dependent dll directly.
        Write-Host "Fetching the dotnet-coverage $DotnetCoverageVersion package to run with the .NET runtime..."
        $pkg = Get-NuGetPackage -Id 'dotnet-coverage' -Version $DotnetCoverageVersion -Destination $ToolsDir
        if ($pkg) {
            $dll = Get-ChildItem -Path (Join-Path $pkg 'tools') -Recurse -Filter 'dotnet-coverage.dll' | Select-Object -First 1
            if ($dll) { return [pscustomobject]@{ Exe = $dotnet.Source; Prefix = @($dll.FullName); Display = "$($dotnet.Source) $($dll.FullName)" } }
        }
    }
    throw 'Could not find or install a coverage merge tool (dotnet-coverage or Microsoft.CodeCoverage.Console.exe).'
}

function Test-PythonCandidate([string]$Exe) {
    if (-not $Exe -or -not (Test-Path $Exe)) { return $false }
    # The Microsoft Store alias in WindowsApps is a stub that opens the Store instead of running.
    if ($Exe -match '\\WindowsApps\\') { return $false }
    $oldEap = $ErrorActionPreference; $ErrorActionPreference = 'Continue'
    $probe = & $Exe -c "import struct,sys,sqlite3; print('%d %d.%d' % (struct.calcsize('P')*8, sys.version_info[0], sys.version_info[1]))" 2>$null
    $ErrorActionPreference = $oldEap
    if ($LASTEXITCODE -ne 0 -or -not $probe) { return $false }
    $parts = "$probe".Trim().Split(' ')
    # DIA is loaded in-process, so Python must be 64-bit to load the amd64 msdia140.dll.
    return ($parts[0] -eq '64' -and [version]$parts[1] -ge [version]'3.9')
}

function Get-NuGetPackage([string]$Id, [string]$Version, [string]$Destination) {
    foreach ($source in $NuGetSources) {
        try {
            $index = Invoke-RestMethod -Uri $source -UseBasicParsing
            $base = ($index.resources | Where-Object { $_.'@type' -eq 'PackageBaseAddress/3.0.0' } | Select-Object -First 1).'@id'
            if (-not $base) { continue }
            $base = $base.TrimEnd('/')
            $lid = $Id.ToLowerInvariant(); $lver = $Version.ToLowerInvariant()
            $nupkg = Join-Path $Destination "$lid.$lver.nupkg"
            New-Item -ItemType Directory -Force -Path $Destination | Out-Null
            Invoke-WebRequest -Uri "$base/$lid/$lver/$lid.$lver.nupkg" -OutFile $nupkg -UseBasicParsing
            $extract = Join-Path $Destination "$lid.$lver"
            if (Test-Path $extract) { Remove-Item $extract -Recurse -Force }
            Add-Type -AssemblyName System.IO.Compression.FileSystem
            [System.IO.Compression.ZipFile]::ExtractToDirectory($nupkg, $extract)
            Write-Host "  Fetched $Id $Version from $source"
            return $extract
        }
        catch {
            Write-Host "  Could not fetch $Id $Version from ${source}: $($_.Exception.Message)"
        }
    }
    return $null
}

function Resolve-Python {
    param([string]$Explicit, [string]$ToolsDir)
    if ($Explicit) {
        if (Test-PythonCandidate $Explicit) { return $Explicit }
        throw "-PythonPath '$Explicit' is not a usable 64-bit Python 3.9+."
    }
    $candidates = @()
    $py = Get-Command 'py' -ErrorAction SilentlyContinue
    if ($py) {
        $oldEap = $ErrorActionPreference; $ErrorActionPreference = 'Continue'
        $fromLauncher = & $py.Source -3-64 -c "import sys; print(sys.executable)" 2>$null
        $ErrorActionPreference = $oldEap
        if ($LASTEXITCODE -eq 0 -and $fromLauncher) { $candidates += "$fromLauncher".Trim() }
    }
    $candidates += @(Get-Command 'python', 'python3' -All -ErrorAction SilentlyContinue | ForEach-Object { $_.Source })
    foreach ($c in $candidates) {
        if (Test-PythonCandidate $c) { return $c }
    }
    Write-Host "No usable 64-bit Python 3.9+ found; fetching the 'python' NuGet package $PythonNuGetVersion..."
    $pkg = Get-NuGetPackage -Id 'python' -Version $PythonNuGetVersion -Destination $ToolsDir
    if ($pkg) {
        $exe = Join-Path $pkg 'tools\python.exe'
        if (Test-PythonCandidate $exe) { return $exe }
    }
    throw 'Could not find or fetch a 64-bit Python 3.9+ (pass -PythonPath).'
}

# comtypes is the converter's only third-party Python dependency. Install it into a private
# folder (on PYTHONPATH) rather than into whatever Python we found.
function Initialize-Comtypes {
    param([string]$Python, [string]$SitePackages)
    New-Item -ItemType Directory -Force -Path $SitePackages | Out-Null
    $env:PYTHONPATH = $SitePackages
    $oldEap = $ErrorActionPreference; $ErrorActionPreference = 'Continue'
    $null = & $Python -c 'import comtypes' 2>$null
    $ErrorActionPreference = $oldEap
    if ($LASTEXITCODE -eq 0) { Write-Host '  comtypes already importable.'; return }
    foreach ($index in $PipIndexUrls) {
        $r = Invoke-Native -Exe $Python -Arguments @('-m', 'pip', 'install', '--disable-pip-version-check', '--no-input', '--quiet',
            '--target', $SitePackages, '--index-url', $index, "comtypes==$ComtypesVersion")
        if ($r.ExitCode -eq 0) { Write-Host "  Installed comtypes $ComtypesVersion from $index"; return }
    }
    throw "Could not install comtypes $ComtypesVersion from any of: $($PipIndexUrls -join ', ')."
}

function Resolve-Msdia {
    param([string]$Explicit)
    if ($Explicit) {
        if (Test-Path $Explicit) { return (Resolve-Path $Explicit).Path }
        throw "-MsdiaPath '$Explicit' does not exist."
    }
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswhere) {
        $found = & $vswhere -all -products '*' -find 'DIA SDK\bin\amd64\msdia140.dll' 2>$null | Select-Object -First 1
        if ($found -and (Test-Path $found)) { return $found }
    }
    $found = Get-ChildItem -Path @("${env:ProgramFiles}\Microsoft Visual Studio", "${env:ProgramFiles(x86)}\Microsoft Visual Studio") `
        -Recurse -Filter 'msdia140.dll' -ErrorAction SilentlyContinue |
        Where-Object { $_.FullName -match '\\DIA SDK\\bin\\amd64\\' } | Select-Object -First 1
    if ($found) { return $found.FullName }
    throw 'msdia140.dll (Visual Studio DIA SDK, amd64) not found. Install the VS C++ workload or pass -MsdiaPath.'
}

# ---------------------------------------------------------------------------------------------
# Inputs
# ---------------------------------------------------------------------------------------------
if (-not $WorkDir) {
    $tempRoot = if ($env:AGENT_TEMPDIRECTORY) { $env:AGENT_TEMPDIRECTORY } else { $env:TEMP }
    $WorkDir = Join-Path $tempRoot 'CoverageToScc'
}
New-Item -ItemType Directory -Force -Path $OutputDir, $WorkDir | Out-Null
$OutputDir = (Resolve-Path $OutputDir).Path
$WorkDir = (Resolve-Path $WorkDir).Path
$logDir = Join-Path $OutputDir 'logs'
New-Item -ItemType Directory -Force -Path $logDir | Out-Null

$converterCli = Join-Path $ConverterRoot 'common_coverage\cli\coverage_to_scc.py'
if (-not (Test-Path $converterCli)) { throw "Converter not found: $converterCli (-ConverterRoot must contain common_coverage\)." }
$ConverterRoot = (Resolve-Path $ConverterRoot).Path
$converterCommit = ''
if (Get-Command git -ErrorAction SilentlyContinue) {
    $oldEap = $ErrorActionPreference; $ErrorActionPreference = 'Continue'
    $converterCommit = "$(& git -C $ConverterRoot rev-parse HEAD 2>$null)".Trim()
    $ErrorActionPreference = $oldEap
}
Write-Host "Converter: $converterCli $(if ($converterCommit) { "(commit $converterCommit)" })"

# Original/instrumented binaries.
if ((Test-Path $CoverageBinaries -PathType Container) -and (Test-Path (Join-Path $CoverageBinaries 'coverage-binaries.zip'))) {
    $CoverageBinaries = Join-Path $CoverageBinaries 'coverage-binaries.zip'
}
$binariesRoot = $CoverageBinaries
if ((Test-Path $CoverageBinaries -PathType Leaf) -and $CoverageBinaries -match '\.zip$') {
    $binariesRoot = Join-Path $WorkDir 'CoverageBinaries'
    if (Test-Path $binariesRoot) { Remove-Item $binariesRoot -Recurse -Force }
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    [System.IO.Compression.ZipFile]::ExtractToDirectory($CoverageBinaries, $binariesRoot)
    Write-Host "Extracted $CoverageBinaries"
}
if (-not (Test-Path $binariesRoot -PathType Container)) { throw "-CoverageBinaries '$CoverageBinaries' is neither a zip nor a folder." }

# The converter records each module's checksum/timestamp/size only when it can open the DLL at the
# path in the coverage XML, which is the bare file name - so the originals go in the working dir.
$cwd = Join-Path $WorkDir 'originals'
if (Test-Path $cwd) { Remove-Item $cwd -Recurse -Force }
New-Item -ItemType Directory -Force -Path $cwd | Out-Null
$moduleInfo = @()
foreach ($module in $Modules) {
    $stem = [IO.Path]::GetFileNameWithoutExtension($module)
    $origDir = Join-Path $binariesRoot "$stem\original"
    $dll = Join-Path $origDir $module
    $pdb = Join-Path $origDir "$stem.pdb"
    if (-not (Test-Path $dll) -or -not (Test-Path $pdb)) {
        throw "Original binaries for $module not found (expected $dll and $pdb)."
    }
    Copy-Item $dll $cwd -Force
    $moduleInfo += [pscustomobject]@{ Module = $module; Stem = $stem; OriginalDll = (Join-Path $cwd $module); Pdb = $pdb }
}

# Slice coverage, grouped by the BuildLabEx of the host that produced it.
$stems = @($moduleInfo | ForEach-Object { $_.Stem })
$coverageFiles = @(Get-ChildItem -Path $InputDir -Recurse -Filter '*.coverage' -ErrorAction SilentlyContinue |
    Where-Object { $f = $_; $f.Length -gt 0 -and @($stems | Where-Object { $f.Name -like "*-$_.coverage" }).Count -gt 0 })
if ($coverageFiles.Count -eq 0) { throw "No non-empty *-<ModuleStem>.coverage files under $InputDir for modules $($stems -join ', ')." }
Write-Host "Found $($coverageFiles.Count) coverage file(s) under $InputDir."

$hostFields = 'BuildLabEx', 'ProductName', 'DisplayVersion', 'ReleaseId', 'CurrentBuild', 'CurrentBuildNumber', 'CurrentMajor', 'CurrentMinor', 'Ubr', 'EditionId', 'InstallationType'
$groups = [ordered]@{}
$slices = @()
foreach ($dir in ($coverageFiles | Group-Object DirectoryName)) {
    $hostFile = Join-Path $dir.Name '_coverage-host-os.json'
    $hostOs = $null
    if (Test-Path $hostFile) { $hostOs = Get-Content $hostFile -Raw | ConvertFrom-Json }
    $buildLabEx = if ($hostOs -and $hostOs.BuildLabEx) { "$($hostOs.BuildLabEx)".Trim() } else { '' }
    if (-not $buildLabEx) { Write-AdoWarning "No test-host BuildLabEx for $($dir.Name) (missing/empty _coverage-host-os.json); its coverage goes into a .scc that cannot be uploaded." }
    if (-not $groups.Contains($buildLabEx)) { $groups[$buildLabEx] = @() }
    $groups[$buildLabEx] += [pscustomobject]@{ Dir = $dir.Name; Files = @($dir.Group | ForEach-Object { $_.FullName }); Host = $hostOs }
    $slices += [pscustomobject]@{
        Folder = $dir.Name; BuildLabEx = $buildLabEx
        ComputerName = if ($hostOs) { $hostOs.ComputerName } else { $null }
        CoverageFiles = @($dir.Group | ForEach-Object { $_.Name })
    }
}
Write-Host "Coverage from $($slices.Count) slice folder(s) in $($groups.Count) BuildLabEx group(s)."

# ---------------------------------------------------------------------------------------------
# Tools
# ---------------------------------------------------------------------------------------------
$toolsDir = Join-Path $WorkDir 'tools'
$covTool = Resolve-CoverageTool -Explicit $CoverageToolPath -ToolsDir $toolsDir
Write-Host "Coverage merge tool: $($covTool.Display)"
$python = Resolve-Python -Explicit $PythonPath -ToolsDir $toolsDir
Write-Host "Python: $python"
Initialize-Comtypes -Python $python -SitePackages (Join-Path $toolsDir 'site-packages')
$msdia = Resolve-Msdia -Explicit $MsdiaPath
Write-Host "msdia140.dll: $msdia"

# Python helpers: original DLL <-> PDB signature check, and .scc validation. Both use the
# converter's own PE/DIA readers so they agree with what the converter wrote.
$helperPy = Join-Path $WorkDir 'scc_helpers.py'
@'
import json, os, sqlite3, sys, uuid
sys.path.insert(0, sys.argv[2])
from common_coverage import dia, peinfo

def norm(g):
    return str(uuid.UUID(str(g).strip('{}'))).lower() if g else None

def preflight(args):
    msdia, pairs = args[0], args[1:]
    out = []
    for dll, pdb in zip(pairs[0::2], pairs[1::2]):
        pe = peinfo.read_pe_identity(dll) or {}
        guid, age = dia.pdb_identity(pdb, msdia=msdia)
        out.append({'dll': os.path.basename(dll), 'pdb': pdb,
                    'dllPdbGuid': norm(pe.get('guid')), 'dllPdbAge': pe.get('age'),
                    'pdbGuid': norm(guid), 'pdbAge': age,
                    'hasLines': dia.pdb_has_lines(pdb, msdia=msdia),
                    'match': bool(pe) and norm(pe.get('guid')) == norm(guid) and pe.get('age') == age})
    return out

def validate(args):
    scc = args[0]
    con = sqlite3.connect('file:%s?mode=ro' % scc, uri=True)
    try:
        integrity = con.execute('PRAGMA integrity_check').fetchone()[0]
        tables = {r[0] for r in con.execute("SELECT name FROM sqlite_master WHERE type='table'")}
        row = con.execute('SELECT BuildLabEx FROM HostOsBuild WHERE Id = 1').fetchone() if 'HostOsBuild' in tables else None
        images = []
        for r in con.execute('SELECT i.Id, i.Path, i.Checksum, i.Size, i.Timestamp, p.Guid, p.Age, '
                             '(SELECT COUNT(*) FROM NativeImageHits h WHERE h.ImageId = i.Id) '
                             'FROM Images i LEFT JOIN Pdbs p ON p.ImageId = i.Id ORDER BY i.Id'):
            images.append({'path': r[1], 'checksum': r[2], 'size': r[3], 'timestamp': r[4],
                           'pdbGuid': str(uuid.UUID(bytes_le=r[5])) if r[5] else None, 'pdbAge': r[6], 'hits': r[7]})
        return {'integrity': integrity, 'tables': sorted(tables), 'buildLabEx': row[0] if row else None,
                'images': images, 'totalHits': sum(i['hits'] for i in images)}
    finally:
        con.close()

print(json.dumps({'preflight': preflight, 'validate': validate}[sys.argv[1]](sys.argv[3:])))
'@ | Set-Content -Path $helperPy -Encoding UTF8

function Invoke-Helper([string]$Mode, [string[]]$HelperArgs) {
    $oldEap = $ErrorActionPreference; $ErrorActionPreference = 'Continue'
    $out = & $python $helperPy $Mode $ConverterRoot @HelperArgs 2>&1 | ForEach-Object { "$_" }
    $code = $LASTEXITCODE
    $ErrorActionPreference = $oldEap
    if ($code -ne 0) { throw "scc_helpers.py $Mode failed (exit $code): $($out -join "`n")" }
    return (($out | Where-Object { $_ -match '^[\[\{]' } | Select-Object -Last 1) | ConvertFrom-Json)
}

Write-Host "`nChecking that each original DLL matches its PDB..."
$pairArgs = @($msdia) + @($moduleInfo | ForEach-Object { $_.OriginalDll; $_.Pdb })
$preflight = @(Invoke-Helper 'preflight' $pairArgs)
foreach ($p in $preflight) {
    Write-Host ("  {0}: DLL expects {1}/{2}, PDB is {3}/{4}, line info: {5}" -f $p.dll, $p.dllPdbGuid, $p.dllPdbAge, $p.pdbGuid, $p.pdbAge, $p.hasLines)
    if (-not $p.match) { throw "$($p.dll) does not match $($p.pdb); RVAs would be wrong. Use the original (uninstrumented) DLL and its own PDB." }
    if (-not $p.hasLines) { throw "$($p.pdb) has no line information (stripped PDB); the converter needs private symbols." }
}

# ---------------------------------------------------------------------------------------------
# Convert each BuildLabEx group: one combined .scc (every module) plus, when there is more than one
# module, one .scc per module (named MUX/MUXC for the WinUI DLLs, else the module stem).
# ---------------------------------------------------------------------------------------------
$moduleLabels = @{ 'Microsoft.ui.xaml' = 'MUX'; 'Microsoft.UI.Xaml.Controls' = 'MUXC' }
foreach ($m in $moduleInfo) {
    $m | Add-Member -NotePropertyName Label -NotePropertyValue $(if ($moduleLabels.ContainsKey($m.Stem)) { $moduleLabels[$m.Stem] } else { $m.Stem })
}
$hostOptionMap = [ordered]@{
    BuildLabEx = '--build-lab-ex'; ProductName = '--product-name'; DisplayVersion = '--display-version'; ReleaseId = '--release-id'
    CurrentBuild = '--current-build'; CurrentBuildNumber = '--current-build-number'; CurrentMajor = '--current-major'
    CurrentMinor = '--current-minor'; Ubr = '--ubr'; EditionId = '--edition-id'; InstallationType = '--installation-type'
}

# Converts the group's merged XML into one .scc holding exactly $Modules, then validates it.
# -Filter restricts the converter to those modules (per-DLL files); the combined file is unfiltered.
# Returns the converter stats and validation; throws on any problem.
function Convert-ToScc {
    param([string]$Xml, [string]$SccPath, [object[]]$Modules, [string[]]$HostArgs, [string]$Log, [string]$BuildLabEx, [switch]$Filter)
    $timer = [Diagnostics.Stopwatch]::StartNew()
    $convertArgs = @($converterCli, '--coverage', $Xml, '--out', $SccPath, '--msdia', $msdia, '--offline') + $HostArgs
    foreach ($m in $Modules) {
        $convertArgs += @('--pdb', $m.Pdb)
        if ($Filter) { $convertArgs += @('--filter', $m.Module) }
    }
    Write-Host "Converting $([IO.Path]::GetFileName($SccPath)) ($(($Modules | ForEach-Object { $_.Module }) -join ', '))..."
    $convert = Invoke-Native -Exe $python -Arguments $convertArgs -LogFile $Log -WorkingDirectory $cwd
    if ($convert.ExitCode -ne 0) { throw "coverage_to_scc.py failed (exit $($convert.ExitCode)); see $Log." }
    $stats = $null
    $statLine = $convert.Output | Where-Object { $_ -match '^coverage-to-scc: modules=' } | Select-Object -Last 1
    if ($statLine -match 'modules=(\d+) resolved=(\d+) blocked\(stripped\)=(\d+) images=(\d+) hits=(\d+)') {
        $stats = [ordered]@{ Modules = [int]$Matches[1]; Resolved = [int]$Matches[2]; BlockedStripped = [int]$Matches[3]; Images = [int]$Matches[4]; Hits = [long]$Matches[5] }
    }

    $v = Invoke-Helper 'validate' @($SccPath)
    $problems = @()
    if ($v.integrity -ne 'ok') { $problems += "SQLite integrity_check: $($v.integrity)" }
    if (@('HostOsBuild', 'RunStatistics') | Where-Object { $v.tables -notcontains $_ }) { $problems += 'missing HostOsBuild/RunStatistics tables' }
    if ($BuildLabEx -and "$($v.buildLabEx)".Trim() -ne $BuildLabEx) { $problems += "HostOsBuild.BuildLabEx is '$($v.buildLabEx)', expected '$BuildLabEx'" }
    $expected = @($Modules | ForEach-Object { $_.Module })
    $extra = @($v.images | Where-Object { $expected -notcontains [IO.Path]::GetFileName($_.path) } | ForEach-Object { $_.path })
    if ($extra.Count -gt 0) { $problems += "unexpected image(s) in .scc: $($extra -join ', ')" }
    foreach ($m in $Modules) {
        $img = $v.images | Where-Object { [IO.Path]::GetFileName($_.path) -eq $m.Module } | Select-Object -First 1
        $pf = $preflight | Where-Object { $_.dll -eq $m.Module } | Select-Object -First 1
        if (-not $img) { $problems += "$($m.Module): no image in .scc"; continue }
        if ([long]$img.hits -le 0) { $problems += "$($m.Module): 0 hits" }
        if ($img.pdbGuid -ne $pf.pdbGuid -or [int]$img.pdbAge -ne [int]$pf.pdbAge) { $problems += "$($m.Module): .scc PDB signature $($img.pdbGuid)/$($img.pdbAge) does not match the original PDB" }
        if ([long]$img.size -le 0 -or [long]$img.timestamp -le 0) { $problems += "$($m.Module): image size/timestamp not recorded (original DLL not found by the converter)" }
        Write-Host ("  {0}: {1:N0} hits, PDB {2}/{3}, size {4}, timestamp {5}" -f $m.Module, [long]$img.hits, $img.pdbGuid, $img.pdbAge, $img.size, $img.timestamp)
    }
    if ($problems.Count -gt 0) { throw ($problems -join '; ') }
    Write-Host ("  OK: {0:N0} unique RVA hits in {1:N0}s." -f [long]$v.totalHits, $timer.Elapsed.TotalSeconds)
    return [pscustomobject]@{ Converter = $stats; Validation = $v }
}

$results = @()
$failed = $false
$groupIndex = 0
foreach ($buildLabEx in $groups.Keys) {
    $groupIndex++
    $members = $groups[$buildLabEx]
    $files = @($members | ForEach-Object { $_.Files })
    $suffix = if (-not $buildLabEx) { '-no-host-os' } elseif ($groups.Count -gt 1) { "-g$groupIndex" } else { '' }
    $log = Join-Path $logDir "$SccName$suffix.log"
    $label = if ($buildLabEx) { $buildLabEx } else { '<unknown BuildLabEx>' }
    $sliceIds = @($members | ForEach-Object { if ($_.Host) { $_.Host.JobPositionInPhase } else { Split-Path $_.Dir -Leaf } })
    Write-Host "`n=== $label : $($members.Count) slice(s), $($files.Count) file(s) ==="

    # Combined first, then one file per module (skipped when there is only one module, since it
    # would be identical to the combined file).
    $targets = @([pscustomobject]@{ Kind = 'Combined'; FileSuffix = ''; Modules = $moduleInfo; Filter = $false })
    if ($moduleInfo.Count -gt 1) {
        $targets += @($moduleInfo | ForEach-Object { [pscustomobject]@{ Kind = $_.Label; FileSuffix = "-$($_.Label)"; Modules = @($_); Filter = $true } })
    }
    $groupResults = @($targets | ForEach-Object {
        [pscustomobject][ordered]@{
            Scc = "$SccName$suffix$($_.FileSuffix).scc"; Kind = $_.Kind; Modules = @($_.Modules | ForEach-Object { $_.Module })
            BuildLabEx = $buildLabEx; Uploadable = $false; Slices = $sliceIds
            HostOsBuild = $null; Converter = $null; Validation = $null; Errors = @()
        }
    })

    $xml = Join-Path $WorkDir "$SccName$suffix.coverage.xml"
    try {
        Write-Host "Merging to coverage XML: $xml"
        $merge = Invoke-Native -Exe $covTool.Exe -Arguments ($covTool.Prefix + @('merge') + $files + @('-o', $xml, '-f', 'xml')) -LogFile $log
        if ($merge.ExitCode -ne 0 -or -not (Test-Path $xml)) { throw "Coverage XML merge failed (exit $($merge.ExitCode))." }

        # Only claim a host field when every slice in the group agrees on it.
        $hostOs = [ordered]@{}
        foreach ($field in $hostFields) {
            $values = @($members | ForEach-Object { if ($_.Host) { $_.Host.$field } else { $null } } | Select-Object -Unique)
            if ($values.Count -eq 1 -and $null -ne $values[0] -and "$($values[0])" -ne '') { $hostOs[$field] = $values[0] }
            elseif ($values.Count -gt 1) { Write-AdoWarning "Slices in BuildLabEx group '$label' disagree on $field ($($values -join ', ')); leaving it unset." }
        }
        $hostArgs = @()
        foreach ($field in $hostOptionMap.Keys) { if ($hostOs.Contains($field)) { $hostArgs += @($hostOptionMap[$field], "$($hostOs[$field])") } }

        for ($i = 0; $i -lt $targets.Count; $i++) {
            $r = $groupResults[$i]
            $r.HostOsBuild = $hostOs
            try {
                $out = Convert-ToScc -Xml $xml -SccPath (Join-Path $OutputDir $r.Scc) -Modules $targets[$i].Modules -HostArgs $hostArgs `
                    -Log $log -BuildLabEx $buildLabEx -Filter:$targets[$i].Filter
                $r.Converter = $out.Converter
                $r.Validation = $out.Validation
                $r.Uploadable = [bool]$buildLabEx
            }
            catch {
                $r.Errors = @("$_")
                if ($buildLabEx) { $failed = $true; Write-AdoError "SCC conversion of $($r.Scc) (BuildLabEx '$label') failed: $_" }
                else { Write-AdoWarning "SCC conversion of $($r.Scc) (slices without host OS metadata) failed: $_" }
            }
        }
    }
    catch {
        foreach ($r in $groupResults) { $r.Errors = @("$_") }
        if ($buildLabEx) { $failed = $true; Write-AdoError "SCC conversion for BuildLabEx '$label' failed: $_" }
        else { Write-AdoWarning "SCC conversion for slices without host OS metadata failed: $_" }
    }
    finally {
        Remove-Item $xml -Force -ErrorAction SilentlyContinue
    }
    $results += $groupResults
}

# ---------------------------------------------------------------------------------------------
# Outputs
# ---------------------------------------------------------------------------------------------
if ((Test-Path $CoverageBinaries -PathType Leaf) -and $CoverageBinaries -match '\.zip$') {
    $binOut = Join-Path $OutputDir 'CoverageBinaries'
    New-Item -ItemType Directory -Force -Path $binOut | Out-Null
    Copy-Item $CoverageBinaries $binOut -Force
    $manifest = Join-Path (Split-Path $CoverageBinaries -Parent) 'coverage-binaries.json'
    if (-not (Test-Path $manifest)) { $manifest = Join-Path $binariesRoot 'coverage-binaries.json' }
    if (Test-Path $manifest) { Copy-Item $manifest $binOut -Force }
}

$summary = [ordered]@{
    GeneratedAt = (Get-Date).ToString('o')
    Build = [ordered]@{ Id = $env:BUILD_BUILDID; Number = $env:BUILD_BUILDNUMBER; SourceBranch = $env:BUILD_SOURCEBRANCH; SourceVersion = $env:BUILD_SOURCEVERSION }
    UploadName = $UploadName
    Converter = [ordered]@{ Root = $ConverterRoot; Commit = $converterCommit }
    Tools = [ordered]@{ CoverageTool = $covTool.Display; Python = $python; Msdia = $msdia }
    Modules = $preflight
    Slices = $slices
    Results = $results
}
$summary | ConvertTo-Json -Depth 8 | Set-Content (Join-Path $OutputDir 'scc-summary.json') -Encoding UTF8

$uploadable = @($results | Where-Object { $_.Uploadable })
$nameArg = if ($UploadName) { $UploadName } else { $SccName }
$uploadLabExes = @($uploadable | ForEach-Object { $_.BuildLabEx } | Select-Object -Unique)
function Get-UploadCommand($r) {
    $n = $nameArg
    if ($r.Kind -ne 'Combined') { $n = "$n - $($r.Kind)" }
    if ($uploadLabExes.Count -gt 1) { $n = "$n ($($r.BuildLabEx))" }
    return "   .\StackSampler\StackSampler.exe upload --file-pattern `"<artifact folder>\$($r.Scc)`" --name `"$n`""
}
$upload = @(
    '# Upload the .scc files to StackSampler (sampled code coverage)'
    ''
    'Uploading needs an authorized account, so the pipeline does not do it.'
    ''
    '1. Download this artifact and get StackSampler from its usual distribution point. Extract it to'
    '   .\StackSampler and keep its files together.'
    ''
)
$combinedUploads = @($uploadable | Where-Object { $_.Kind -eq 'Combined' })
$perModuleUploads = @($uploadable | Where-Object { $_.Kind -ne 'Combined' })
if ($perModuleUploads.Count -gt 0) {
    $upload += @(
        '2. Upload ONE of the two sets below, not both. The combined file already contains every'
        '   per-DLL hit and all files share the same test-host BuildLabEx, so uploading both sets'
        '   would record the same coverage twice.'
        ''
        '   Option A - combined (one test pass covering every DLL):'
        ''
        '   ```powershell'
    )
    $upload += @($combinedUploads | ForEach-Object { Get-UploadCommand $_ })
    $upload += @('   ```', '', '   Option B - one test pass per DLL:', '', '   ```powershell')
    $upload += @($perModuleUploads | ForEach-Object { Get-UploadCommand $_ })
    $upload += '   ```'
}
else {
    $upload += @('2. Upload each file below (one StackSampler upload per BuildLabEx):', '', '   ```powershell')
    $upload += @($combinedUploads | ForEach-Object { Get-UploadCommand $_ })
    $upload += '   ```'
}
$upload += @('', '| File | Contents | Test host BuildLabEx | Slices | Unique RVA hits | Uploadable |', '| --- | --- | --- | --- | --- | --- |')
foreach ($r in $results) {
    $hits = if ($r.Validation) { '{0:N0}' -f [long]$r.Validation.totalHits } else { '-' }
    $upload += "| $($r.Scc) | $($r.Kind): $($r.Modules -join ', ') | $(if ($r.BuildLabEx) { $r.BuildLabEx } else { '(unknown)' }) | $(@($r.Slices).Count) | $hits | $(if ($r.Uploadable) { 'yes' } else { 'no' }) |"
}
$upload += @('', '.scc is hit-only: covered/partially covered lines become RVA hits; uncovered lines and hit counts are not represented.',
    'CoverageBinaries\coverage-binaries.zip holds the original DLL/PDB the RVAs refer to, plus the instrumented DLL/PDB the tests ran.')
$upload | Set-Content (Join-Path $OutputDir 'UPLOAD.md') -Encoding UTF8

Write-Host "`n=== SCC conversion summary ($([int]$overallTimer.Elapsed.TotalSeconds)s) ==="
foreach ($r in $results) {
    $hits = if ($r.Validation) { '{0:N0}' -f [long]$r.Validation.totalHits } else { '-' }
    Write-Host ("  {0}  [{1}]  BuildLabEx={2}  slices={3}  hits={4}  uploadable={5}{6}" -f $r.Scc, $r.Kind, $(if ($r.BuildLabEx) { $r.BuildLabEx } else { '(unknown)' }), @($r.Slices).Count, $hits, $r.Uploadable, $(if ($r.Errors) { "  ERROR: $($r.Errors -join '; ')" }))
}
Write-Host "Output: $OutputDir (scc-summary.json, UPLOAD.md)"

if ($failed -or $uploadable.Count -eq 0) {
    Write-AdoError 'No uploadable .scc was produced for at least one test-host BuildLabEx; see scc-summary.json.'
    exit 1
}
exit 0
