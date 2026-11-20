#requires -Version 7.4
[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$identity = [Security.Principal.WindowsIdentity]::GetCurrent()
try {
    if (-not [Security.Principal.WindowsPrincipal]::new($identity).IsInRole(
        [Security.Principal.WindowsBuiltInRole]::Administrator)) {
        throw 'Run this opt-in native/ETW integration test from an elevated PowerShell.'
    }
}
finally { $identity.Dispose() }
$architecture = switch ([Runtime.InteropServices.RuntimeInformation]::OSArchitecture.ToString()) {
    'X64' { 'x64' }
    'Arm64' { 'ARM64' }
    default { throw 'The startup handoff fixture requires x64 or ARM64 Windows.' }
}
& (Join-Path $PSScriptRoot 'Build-Launcher.ps1') -Architecture $architecture -BuildTests
$root = Split-Path -Parent $PSScriptRoot
$native = Join-Path $root "artifacts\launcher\win-$($architecture.ToLowerInvariant())"
& dotnet run --project (Join-Path $root 'WinUIProfiler.Tests\WinUIProfiler.Tests.csproj') -- `
    --startup-integration $native
if ($LASTEXITCODE -ne 0) { throw 'The native startup/ETW regression failed.' }
