param([ValidateSet('x64', 'arm64')][string]$Architecture = 'x64')
$ErrorActionPreference = 'Stop'
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$vs) { throw 'Installed Visual Studio C++ toolchain not found.' }
$setup = if ($Architecture -eq 'x64') { 'vcvars64.bat' } else { 'vcvarsamd64_arm64.bat' }
$output = Join-Path $PSScriptRoot "bin\$Architecture"
New-Item -ItemType Directory -Force $output | Out-Null
$native = Join-Path $PSScriptRoot '..\WinUIProfiler.Native\Metrics\CpuNormalization.cpp'
$test = Join-Path $PSScriptRoot 'CpuNormalizationTests.cpp'
$abi = Join-Path $PSScriptRoot 'AbiContractTests.c'
$discovery = Split-Path $vswhere
$command = "set `"PATH=$discovery;%PATH%`" && call `"$vs\VC\Auxiliary\Build\$setup`" >nul && cd /d `"$output`" && cl /nologo /TC /W4 /WX /c `"$abi`" && cl /nologo /std:c++17 /W4 /WX /EHsc /O2 `"$native`" `"$test`" /Fe:CpuNormalizationTests.exe"
if ($Architecture -eq 'x64') { $command += ' && .\CpuNormalizationTests.exe' }
$source = Join-Path $PSScriptRoot '..\WinUIProfiler.Native\Metrics\CpuSource.cpp'
$sourceTest = Join-Path $PSScriptRoot 'CpuSourceTests.cpp'
$command += " && cl /nologo /std:c++17 /W4 /WX /EHsc /O2 `"$native`" `"$source`" `"$sourceTest`" /Fe:CpuSourceTests.exe"
if ($Architecture -eq 'x64') { $command += ' && .\CpuSourceTests.exe' }
$sourceFaultTest = Join-Path $PSScriptRoot 'CpuSourceFaultTests.cpp'
$command += " && cl /nologo /std:c++17 /W4 /WX /EHsc /O2 `"$source`" `"$sourceFaultTest`" /Fe:CpuSourceFaultTests.exe"
if ($Architecture -eq 'x64') { $command += ' && .\CpuSourceFaultTests.exe' }
$memory = Join-Path $PSScriptRoot '..\WinUIProfiler.Native\Metrics\ProcessMemorySampler.cpp'
$memoryTest = Join-Path $PSScriptRoot 'ProcessMemoryTests.cpp'
$command += " && cl /nologo /std:c++17 /W4 /WX /EHsc /O2 `"$memory`" `"$memoryTest`" /Fe:ProcessMemoryTests.exe"
if ($Architecture -eq 'x64') { $command += ' && .\ProcessMemoryTests.exe' }
$memoryFaultTest = Join-Path $PSScriptRoot 'ProcessMemoryFaultTests.cpp'
$command += " && cl /nologo /std:c++17 /W4 /WX /EHsc /O2 `"$memory`" `"$memoryFaultTest`" /Fe:ProcessMemoryFaultTests.exe"
if ($Architecture -eq 'x64') { $command += ' && .\ProcessMemoryFaultTests.exe' }
$gpu = Join-Path $PSScriptRoot '..\WinUIProfiler.Native\Metrics\GpuFoundation.cpp'
$snapshotTest = Join-Path $PSScriptRoot 'ProcessSnapshotTests.cpp'
$command += " && cl /nologo /std:c++17 /W4 /WX /EHsc /O2 `"$source`" `"$memory`" `"$snapshotTest`" /Fe:ProcessSnapshotTests.exe"
if ($Architecture -eq 'x64') { $command += ' && .\ProcessSnapshotTests.exe' }
$gpuTest = Join-Path $PSScriptRoot 'GpuFoundationTests.cpp'
$gpuProbe = Join-Path $PSScriptRoot '..\WinUIProfiler.Native\Metrics\GpuProbe.cpp'
$gpuArray = Join-Path $PSScriptRoot '..\WinUIProfiler.Native\Metrics\GpuPdhArray.cpp'
$command += " && cl /nologo /std:c++17 /W4 /WX /EHsc /O2 `"$gpu`" `"$gpuArray`" `"$gpuProbe`" `"$gpuTest`" /Fe:GpuFoundationTests.exe /link pdh.lib"
if ($Architecture -eq 'x64') { $command += ' && .\GpuFoundationTests.exe' }
$gpuSource = Join-Path $PSScriptRoot '..\WinUIProfiler.Native\Metrics\GpuSampler.cpp'
$gpuFaultTest = Join-Path $PSScriptRoot 'GpuSourceFaultTests.cpp'
$command += " && cl /nologo /std:c++17 /W4 /WX /EHsc /O2 `"$gpu`" `"$gpuArray`" `"$gpuSource`" `"$gpuFaultTest`" /Fe:GpuSourceFaultTests.exe /link pdh.lib"
if ($Architecture -eq 'x64') { $command += ' && .\GpuSourceFaultTests.exe' }
& $env:ComSpec /d /c $command
if ($LASTEXITCODE -ne 0) { throw "Native $Architecture validation failed (exit $LASTEXITCODE)." }
