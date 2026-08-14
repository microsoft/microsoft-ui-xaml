param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Debug",

    [ValidateSet("Win32", "x86", "x64", "ARM64", "ARM64EC")]
    [string]$Platform = "x64",

    [ValidateSet("Build", "Rebuild", "Clean")]
    [string]$Action = "Build"
)

$ErrorActionPreference = "Stop"

# Windows PowerShell can inherit PowerShell 7's module path when launched from
# another terminal. Prefer the modules that belong to the current host.
$hostModules = Join-Path $PSHOME "Modules"
$modulePaths = @($hostModules) + @($env:PSModulePath -split [IO.Path]::PathSeparator | Where-Object { $_ -and $_ -ne $hostModules })
$env:PSModulePath = $modulePaths -join [IO.Path]::PathSeparator

$repoRoot = Split-Path -Parent $PSScriptRoot

$architecture = switch ($Platform)
{
    "Win32" { "x86" }
    "x86" { "x86" }
    "x64" { "amd64" }
    "ARM64" { "arm64" }
    "ARM64EC" { "arm64ec" }
}

$flavor = if ($Configuration -eq "Debug") { "chk" } else { "fre" }
$buildFlavor = "$architecture$flavor"
$initScript = Join-Path $repoRoot "init.ps1"
$initializationMarker = Join-Path $repoRoot ".tools\VisualStudioBuild.initialized"

if ($Action -eq "Clean")
{
    if (-not (Test-Path $initializationMarker))
    {
        Write-Host "The WinUI repository has not been initialized; there is nothing to clean."
        exit 0
    }

    & $initScript $buildFlavor /envcheck /notitle
    if ($LASTEXITCODE -ne 0)
    {
        exit $LASTEXITCODE
    }
    & (Join-Path $repoRoot "tools\clean.cmd") /all
    exit $LASTEXITCODE
}

$requiresFullInit = -not (Test-Path (Join-Path $repoRoot ".tools")) -or
                    -not (Test-Path (Join-Path $repoRoot "packages")) -or
                    -not (Test-Path $initializationMarker)

if ($requiresFullInit)
{
    Write-Host "Preparing the WinUI repository for the first build..."
    & $initScript $buildFlavor /notitle
    if ($LASTEXITCODE -ne 0)
    {
        exit $LASTEXITCODE
    }
    New-Item -ItemType File -Path $initializationMarker -Force | Out-Null
}
else
{
    & $initScript $buildFlavor /envcheck /notitle
    if ($LASTEXITCODE -ne 0)
    {
        exit $LASTEXITCODE
    }
}

$buildArguments = @("product", "/restore")
if ($Action -eq "Rebuild")
{
    $buildArguments += "/c"
}

& (Join-Path $repoRoot "Build.cmd") @buildArguments
exit $LASTEXITCODE
