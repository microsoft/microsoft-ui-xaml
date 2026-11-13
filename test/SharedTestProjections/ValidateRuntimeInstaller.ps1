# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$fixture = Join-Path ([IO.Path]::GetTempPath()) ('WinUIRuntimeInstaller-' + [guid]::NewGuid().ToString('N'))
$oldArch = $env:_BuildArch
$oldMoniker = $env:_DotNetMoniker
$downloadState = @{ Count = 0 }

function Invoke-WebRequest {
    param([string]$Uri, [string]$OutFile, [string]$ErrorAction)
    $downloadState.Count++
    [IO.File]::WriteAllText($OutFile, $Uri)
}

try {
    New-Item -ItemType Directory -Path "$fixture\build", "$fixture\.tools\amd64" -Force | Out-Null
    Copy-Item -LiteralPath "$repoRoot\build\DownloadDotNetRuntimeInstaller.ps1" -Destination "$fixture\build"
    $env:_BuildArch = 'amd64'
    $installer = "$fixture\.tools\amd64\dotnet-windowsdesktop-runtime-installer.exe"
    [IO.File]::WriteAllText($installer, 'unidentified-old-installer')

    $env:_DotNetMoniker = 'net10.0'
    & "$fixture\build\DownloadDotNetRuntimeInstaller.ps1"
    $v2 = Get-Content -LiteralPath $installer -Raw
    if ($downloadState.Count -ne 1) { throw 'Existing installer prevented a download' }

    $env:_DotNetMoniker = 'net11.0'
    & "$fixture\build\DownloadDotNetRuntimeInstaller.ps1"
    $v3 = Get-Content -LiteralPath $installer -Raw
    if ($downloadState.Count -ne 2 -or $v3 -eq $v2 -or $v3 -notmatch 'win-x64\.exe$') { throw 'Wrong runtime installer downloaded' }
    Write-Output 'PASS: selected runtime is downloaded even when an installer already exists'

    & "$fixture\build\DownloadDotNetRuntimeInstaller.ps1"
    if ($downloadState.Count -ne 3) { throw 'Repeated request skipped its download' }
    if (Test-Path -LiteralPath "$installer.url") { throw 'Unexpected cache metadata' }
    Write-Output 'PASS: repeated requests download again without cache metadata'
} finally {
    $env:_BuildArch = $oldArch
    $env:_DotNetMoniker = $oldMoniker
    Remove-Item -LiteralPath $fixture -Recurse -Force
}
