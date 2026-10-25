param([ValidateSet('x64', 'arm64')][string]$Architecture = 'x64')
$ErrorActionPreference = 'Stop'
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$vs) { throw 'Installed Visual Studio C++ toolchain not found.' }
$setup = if ($Architecture -eq 'x64') { 'vcvars64.bat' } else { 'vcvarsamd64_arm64.bat' }
$output = Join-Path $PSScriptRoot "bin\GpuInventory\$Architecture"
New-Item -ItemType Directory -Force $output | Out-Null
$core = Join-Path $PSScriptRoot '..\WinUIProfiler.Native\Metrics\GpuAdapterInventory.cpp'
$test = Join-Path $PSScriptRoot 'GpuAdapterInventoryTests.cpp'
$sources = "`"$test`" `"$core`""
$discovery = Split-Path $vswhere
$command = "set `"PATH=$discovery;%PATH%`" && call `"$vs\VC\Auxiliary\Build\$setup`" >nul && cd /d `"$output`" && cl /nologo /std:c++17 /W4 /WX /EHsc /O2 $sources /Fe:GpuAdapterInventoryTests.exe"
if ($Architecture -eq 'x64') { $command += ' && .\GpuAdapterInventoryTests.exe' }
$source = Join-Path $PSScriptRoot '..\WinUIProfiler.Native\Metrics\GpuAdapterInventorySource.cpp'
$live = Join-Path $PSScriptRoot 'GpuAdapterInventorySourceTests.cpp'
$acquisition = "`"$live`" `"$core`" `"$source`""
$command += " && cl /nologo /std:c++17 /W4 /WX /EHsc /O2 $acquisition /Fe:GpuAdapterInventorySourceTests.exe"
if ($Architecture -eq 'x64') { $command += ' && .\GpuAdapterInventorySourceTests.exe' }
$fault = Join-Path $PSScriptRoot 'GpuAdapterInventoryFaultTests.cpp'
$command += " && cl /nologo /std:c++17 /W4 /WX /EHsc /O2 `"$fault`" `"$core`" `"$source`" /Fe:GpuAdapterInventoryFaultTests.exe"
if ($Architecture -eq 'x64') { $command += ' && .\GpuAdapterInventoryFaultTests.exe' }
& $env:ComSpec /d /c $command
if ($LASTEXITCODE -ne 0) { throw "GPU inventory $Architecture validation failed (exit $LASTEXITCODE)." }
