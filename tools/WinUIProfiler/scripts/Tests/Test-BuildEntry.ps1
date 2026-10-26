[CmdletBinding()]
param(
    [ValidateSet('Release', 'Debug')][string]$Configuration = 'Release',
    [ValidateSet('x64', 'ARM64')][string]$Platform = 'x64'
)
$ErrorActionPreference = 'Stop'
$root = Split-Path (Split-Path $PSScriptRoot)
foreach ($shell in @('pwsh.exe', 'powershell.exe')) {
    $executable = (Get-Command $shell -CommandType Application -ErrorAction Stop).Source
    & $executable -NoProfile -NonInteractive -File (Join-Path $root 'scripts\Build-App.ps1') -Configuration $Configuration -Platform $Platform
    if ($LASTEXITCODE -ne 0) { throw "$shell build failed." }
    $output = (Get-Content (Join-Path $root "WinUIProfiler.App\obj\ProfilerAppOutput.$Platform.$Configuration.txt") -Raw).Trim()
    $directory = Join-Path $output 'ConnectionPayloads'
    $bytes = [IO.File]::ReadAllBytes((Join-Path $directory 'manifest.json'))
    if ($bytes.Length -lt 2 -or $bytes[0] -ne 0x7b) { throw "$shell emitted a BOM or invalid JSON prefix." }
    $manifest = ([Text.UTF8Encoding]::new($false, $true).GetString($bytes)) | ConvertFrom-Json
    if ($manifest.SchemaVersion -ne 1 -or $manifest.Payloads.Count -ne 2) { throw "$shell emitted an invalid manifest shape." }
    foreach ($entry in $manifest.Payloads) {
        foreach ($payload in @(@('WinUIProfiler.Injector.exe', 'HelperSHA256'), @('WinUIProfiler.Tap.dll', 'TapSHA256'))) {
            $path = Join-Path $directory "$($entry.Architecture)\$($payload[0])"
            if ((Get-FileHash $path).Hash -ne $entry.($payload[1])) { throw "$shell deployed an invalid payload hash." }
        }
    }
    Write-Output "BUILD_ENTRY_AND_BOMLESS_MANIFEST_PASS shell=$shell platform=$Platform"
}
