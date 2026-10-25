param(
    [ValidateSet('x64', 'ARM64')][string]$Architecture = 'x64',
    [switch]$ValidateAdapterGuardsOnly
)
$ErrorActionPreference = 'Stop'
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$vs) { throw 'Installed Visual Studio C++ toolchain not found.' }
$setup = if ($Architecture -eq 'x64') { 'vcvars64.bat' } else { 'vcvarsamd64_arm64.bat' }
$output = Join-Path $PSScriptRoot "bin\$Architecture"
New-Item -ItemType Directory -Force $output | Out-Null
$test = Join-Path $PSScriptRoot 'GpuHardwareWorkloadTests.cpp'
$foundation = Join-Path $PSScriptRoot '..\WinUIProfiler.Native\Metrics\GpuFoundation.cpp'
$array = Join-Path $PSScriptRoot '..\WinUIProfiler.Native\Metrics\GpuPdhArray.cpp'
$dll = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot "..\WinUIProfiler.Native\bin\$Architecture\Release\WinUIProfiler.Native.dll"))
$discovery = Split-Path $vswhere
$command = "set `"PATH=$discovery;%PATH%`" && call `"$vs\VC\Auxiliary\Build\$setup`" >nul && cd /d `"$output`" && cl /nologo /std:c++17 /W4 /WX /EHsc /O2 `"$test`" `"$foundation`" `"$array`" /Fe:GpuHardwareWorkloadTests.exe /link d3d11.lib dxgi.lib d3dcompiler.lib pdh.lib"
if ($Architecture -eq 'x64') {
    $argument = if ($ValidateAdapterGuardsOnly) { '--adapter-guards-only' } else { $dll }
    $command += " && .\GpuHardwareWorkloadTests.exe `"$argument`""
}
& $env:ComSpec /d /c $command
if ($LASTEXITCODE -ne 0) { throw "GPU hardware validation $Architecture failed (exit $LASTEXITCODE); unavailable is not a hardware pass." }
