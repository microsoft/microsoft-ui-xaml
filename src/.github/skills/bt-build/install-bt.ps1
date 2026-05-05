# Install bt from the OS.Developer NuGet feed.
# Downloads the latest Bt package, extracts the architecture-appropriate
# bt.exe, and places it in ~/.bt/ (added to the current session's PATH).
# If bt.exe already exists on disk, just adds to PATH without downloading.
param(
    [string]$InstallDir = (Join-Path $HOME '.bt')
)

$ErrorActionPreference = 'Stop'
$btExe = Join-Path $InstallDir 'bt.exe'

# Already on disk -- add to PATH and refresh if stale
if (Test-Path $btExe) {
    if (-not ($env:PATH.Split(';') -contains $InstallDir)) {
        $env:PATH = "$InstallDir;$env:PATH"
    }

    # Refresh if older than 7 days. `bt update` is idempotent: it downloads
    # only when a newer release exists, otherwise it's a no-op.
    try {
        $ageDays = (New-TimeSpan -Start (Get-Item $btExe).LastWriteTime -End (Get-Date)).TotalDays
        if ($ageDays -gt 7) {
            Write-Host "bt is $([int]$ageDays) days old -- running 'bt update' ..." -ForegroundColor Cyan
            & $btExe update
            if ($LASTEXITCODE -ne 0) {
                Write-Host "bt update returned exit $LASTEXITCODE -- continuing with existing install." -ForegroundColor Yellow
            }
        }
    } catch {
        Write-Host "bt update skipped ($($_.Exception.Message)) -- continuing with existing install." -ForegroundColor Yellow
    }

    $version = & $btExe --version 2>&1
    Write-Host "bt $version (already installed at $InstallDir)" -ForegroundColor Green
    return
}

$feedUrl = 'https://pkgs.dev.azure.com/microsoft/OS.Developer/_packaging/tools/nuget/v3/index.json'
$packageId = 'Bt'

# Detect architecture (env var works on PS 5.1 and pwsh 7+)
$rid = switch ($env:PROCESSOR_ARCHITECTURE) {
    'AMD64' { 'win-x64' }
    'ARM64' { 'win-arm64' }
    default { throw "Unsupported architecture: $env:PROCESSOR_ARCHITECTURE" }
}

# Download package to a temp directory
$tempDir = Join-Path ([System.IO.Path]::GetTempPath()) 'bt-install'
if (Test-Path $tempDir) { Remove-Item $tempDir -Recurse -Force }

Write-Host "Installing bt ($rid) from NuGet feed ..." -ForegroundColor Cyan
dotnet nuget locals http-cache --clear | Out-Null
nuget install $packageId -Source $feedUrl -OutputDirectory $tempDir `
    -NonInteractive -Prerelease
if ($LASTEXITCODE -ne 0) { throw "nuget install $packageId failed" }

# Find the extracted package directory (Bt.<version>/)
$pkgDir = Get-ChildItem $tempDir -Directory -Filter "$packageId.*" |
    Sort-Object Name -Descending | Select-Object -First 1
if (-not $pkgDir) { throw "Package directory not found after install" }

$src = Join-Path $pkgDir.FullName "tools\$rid\bt.exe"
if (-not (Test-Path $src)) {
    throw "bt.exe not found at $src -- package may not include $rid"
}

# Copy to install directory
New-Item -ItemType Directory -Path $InstallDir -Force | Out-Null
Copy-Item $src $btExe -Force

# Clean up
Remove-Item $tempDir -Recurse -Force -ErrorAction SilentlyContinue

# Add to PATH for current session
if ($env:PATH -notlike "*$InstallDir*") {
    $env:PATH = "$InstallDir;$env:PATH"
}

$version = & $btExe --version 2>&1
Write-Host "Installed bt $version to $InstallDir" -ForegroundColor Green
Write-Host "Add $InstallDir to your PATH for future sessions." -ForegroundColor DarkGray
