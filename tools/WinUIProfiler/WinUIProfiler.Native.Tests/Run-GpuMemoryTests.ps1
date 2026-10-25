param([ValidateSet('x64', 'arm64')][string]$Architecture = 'x64')
$ErrorActionPreference = 'Stop'
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$vs) { throw 'Installed Visual Studio C++ toolchain not found.' }
$setup = if ($Architecture -eq 'x64') { 'vcvars64.bat' } else { 'vcvarsamd64_arm64.bat' }
$output = Join-Path $PSScriptRoot "bin\GpuMemory\$Architecture"
New-Item -ItemType Directory -Force $output | Out-Null
$core = Join-Path $PSScriptRoot '..\WinUIProfiler.Native\Metrics\GpuMemoryCore.cpp'
$test = Join-Path $PSScriptRoot 'GpuMemoryTests.cpp'
$sources = "`"$test`" `"$core`""
$discovery = Split-Path $vswhere
$command = "set `"PATH=$discovery;%PATH%`" && call `"$vs\VC\Auxiliary\Build\$setup`" >nul && cd /d `"$output`" && cl /nologo /std:c++17 /W4 /WX /EHsc /O2 $sources /Fe:GpuMemoryTests.exe"
if ($Architecture -eq 'x64') { $command += ' && .\GpuMemoryTests.exe' }
$fault = Join-Path $PSScriptRoot 'GpuMemorySourceTests.cpp'
$command += " && cl /nologo /std:c++17 /W4 /WX /EHsc /O2 `"$fault`" `"$core`" /Fe:GpuMemorySourceTests.exe"
if ($Architecture -eq 'x64') { $command += ' && .\GpuMemorySourceTests.exe' }
$live = Join-Path $PSScriptRoot 'GpuMemoryLiveTests.cpp'
$command += " && cl /nologo /std:c++17 /W4 /WX /EHsc /O2 `"$live`" `"$core`" /Fe:GpuMemoryLiveTests.exe"
if ($Architecture -eq 'x64') { $command += ' && .\GpuMemoryLiveTests.exe' }
& $env:ComSpec /d /c $command
if ($LASTEXITCODE -ne 0) { throw "GPU memory $Architecture validation failed (exit $LASTEXITCODE)." }
