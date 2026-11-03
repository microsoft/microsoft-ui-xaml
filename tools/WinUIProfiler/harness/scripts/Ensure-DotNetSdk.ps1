[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [string]$RepoRoot,

    [string]$InstallDirectory = (Join-Path $env:LOCALAPPDATA "Microsoft\dotnet"),

    [string]$InstallerUri = "https://dot.net/v1/dotnet-install.ps1"
)

$ErrorActionPreference = "Stop"

$repoPath = [IO.Path]::GetFullPath($RepoRoot)
$globalJsonPath = Join-Path $repoPath "global.json"
if (-not (Test-Path -LiteralPath $globalJsonPath -PathType Leaf)) {
    throw "Repository '$repoPath' does not contain global.json."
}

$globalJson = Get-Content -LiteralPath $globalJsonPath -Raw -Encoding UTF8 | ConvertFrom-Json
$requiredVersion = [string]$globalJson.sdk.version
if ([string]::IsNullOrWhiteSpace($requiredVersion)) {
    throw "Repository '$repoPath' does not declare sdk.version in global.json."
}
if ($requiredVersion -notmatch '^[0-9]+\.[0-9]+\.[0-9]+(?:[-+][A-Za-z0-9.-]+)?$') {
    throw "The SDK version '$requiredVersion' in '$globalJsonPath' is not valid."
}

$candidateRoots = [Collections.Generic.List[string]]::new()
$candidateRoots.Add([IO.Path]::GetFullPath($InstallDirectory))
foreach ($root in @(
    $env:DOTNET_ROOT,
    $(if ($env:LOCALAPPDATA) { Join-Path $env:LOCALAPPDATA "Microsoft\dotnet" }),
    $(if ($env:ProgramW6432) { Join-Path $env:ProgramW6432 "dotnet" }),
    $(if ($env:ProgramFiles) { Join-Path $env:ProgramFiles "dotnet" }),
    $(if (${env:ProgramFiles(x86)}) { Join-Path ${env:ProgramFiles(x86)} "dotnet" })
)) {
    if (-not [string]::IsNullOrWhiteSpace($root)) {
        $candidateRoots.Add($root)
    }
}
$dotnetCommand = Get-Command dotnet -ErrorAction SilentlyContinue
if ($null -ne $dotnetCommand -and -not [string]::IsNullOrWhiteSpace($dotnetCommand.Source)) {
    $candidateRoots.Add((Split-Path -Parent $dotnetCommand.Source))
}

$seen = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
foreach ($root in $candidateRoots) {
    $fullRoot = [IO.Path]::GetFullPath($root)
    if ($seen.Add($fullRoot) -and (Test-Path -LiteralPath (Join-Path $fullRoot "sdk\$requiredVersion") -PathType Container)) {
        [pscustomobject]@{
            version = $requiredVersion
            dotnetRoot = $fullRoot
            installed = $false
        }
        return
    }
}

$installRoot = [IO.Path]::GetFullPath($InstallDirectory)
[IO.Directory]::CreateDirectory($installRoot) | Out-Null
$installerPath = Join-Path ([IO.Path]::GetTempPath()) "dotnet-install-$([Guid]::NewGuid().ToString('N')).ps1"
try {
    Invoke-WebRequest -UseBasicParsing -Uri $InstallerUri -OutFile $installerPath
    $pwsh = Join-Path $PSHOME "pwsh.exe"
    if (-not (Test-Path -LiteralPath $pwsh -PathType Leaf)) {
        throw "PowerShell 7 was not found at '$pwsh'."
    }
    & $pwsh -NoLogo -NoProfile -File $installerPath `
        -Version $requiredVersion `
        -InstallDir $installRoot `
        -NoPath
    if ($LASTEXITCODE -ne 0) {
        throw "dotnet-install failed with exit code $LASTEXITCODE while installing SDK $requiredVersion."
    }
}
finally {
    Remove-Item -LiteralPath $installerPath -Force -ErrorAction SilentlyContinue
}

if (-not (Test-Path -LiteralPath (Join-Path $installRoot "sdk\$requiredVersion") -PathType Container)) {
    throw "dotnet-install completed without creating SDK $requiredVersion under '$installRoot'."
}

[pscustomobject]@{
    version = $requiredVersion
    dotnetRoot = $installRoot
    installed = $true
}
