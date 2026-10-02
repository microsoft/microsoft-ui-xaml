<#
.SYNOPSIS
    Check that the MSBuild installation on PATH is internally consistent.

.DESCRIPTION
    A Visual Studio update that cannot replace every file (usually because an MSBuild
    node from an earlier build still had them loaded) leaves the MSBuild bin folder
    holding a mix of file versions. The build then fails late and confusingly: tasks
    declared with TaskHostFactory report

        error MSB4216: Could not run the "<task>" task because MSBuild could not
        create or connect to a task host with runtime "CLR4" and architecture "x64".

    The parent build derives its node handshake from Microsoft.Build.dll, while the
    task host derives it from MSBuild.exe. When those two files come from different
    builds the handshake never matches and no task host can start.

    Only the root Bin folder is checked for the managed assemblies: the amd64 and
    arm64 MSBuild.exe.config files redirect Microsoft.Build* back to the root copies,
    so those are the ones that actually get loaded. Each per-architecture MSBuild.exe
    is checked too, because each one is its own task host.

.PARAMETER MSBuildPath
    Path to MSBuild.exe. Defaults to the first MSBuild.exe on PATH.

.OUTPUTS
    Exit code 0 when consistent (or when MSBuild cannot be found), 1 on a mismatch.
#>
param(
    [string]$MSBuildPath
)

$ErrorActionPreference = "Stop"

if (-not $MSBuildPath) {
    $MSBuildPath = (Get-Command MSBuild.exe -ErrorAction SilentlyContinue | Select-Object -First 1).Source
}

if (-not $MSBuildPath -or -not (Test-Path $MSBuildPath)) {
    # Nothing to check. Callers that need MSBuild report their own error.
    exit 0
}

$binDir = Split-Path -Parent $MSBuildPath
if ((Split-Path -Leaf $binDir) -match '^(amd64|arm64)$') {
    $binDir = Split-Path -Parent $binDir
}

$paths = @(
    (Join-Path $binDir "MSBuild.exe"),
    (Join-Path $binDir "Microsoft.Build.dll"),
    (Join-Path $binDir "Microsoft.Build.Framework.dll"),
    (Join-Path $binDir "Microsoft.Build.Tasks.Core.dll"),
    (Join-Path $binDir "Microsoft.Build.Utilities.Core.dll"),
    (Join-Path $binDir "amd64\MSBuild.exe"),
    (Join-Path $binDir "arm64\MSBuild.exe")
)

$found = foreach ($path in $paths) {
    if (Test-Path $path) {
        [pscustomobject]@{
            Path    = $path
            Version = (Get-Item $path).VersionInfo.FileVersion
        }
    }
}

if (($found | Select-Object -ExpandProperty Version -Unique).Count -le 1) {
    exit 0
}

Write-Host ""
Write-Host "WARNING: This MSBuild installation is inconsistent." -ForegroundColor Yellow
Write-Host "  A Visual Studio update did not replace every file, so these binaries do not" -ForegroundColor Yellow
Write-Host "  all come from the same build. Builds that use TaskHostFactory tasks fail with" -ForegroundColor Yellow
Write-Host "  'error MSB4216: ... could not create or connect to a task host'." -ForegroundColor Yellow
Write-Host ""
foreach ($file in $found) {
    Write-Host ("    {0,-18} {1}" -f $file.Version, $file.Path) -ForegroundColor Yellow
}
Write-Host ""
Write-Host "  To fix: close every MSBuild.exe, devenv.exe and VBCSCompiler.exe process, then" -ForegroundColor Yellow
Write-Host "  run Repair on this Visual Studio installation from the Visual Studio Installer." -ForegroundColor Yellow
Write-Host "  That needs administrator rights, so init.cmd cannot do it for you." -ForegroundColor Yellow
Write-Host ""

exit 1
