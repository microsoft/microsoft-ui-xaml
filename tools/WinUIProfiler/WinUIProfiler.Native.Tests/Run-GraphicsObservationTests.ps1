param([ValidateSet('x64', 'arm64')][string]$Architecture = 'x64')
$ErrorActionPreference = 'Stop'
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$vs) { throw 'Installed Visual Studio C++ toolchain not found.' }
$setup = if ($Architecture -eq 'x64') { 'vcvars64.bat' } else { 'vcvarsamd64_arm64.bat' }
$output = Join-Path $PSScriptRoot "bin\GraphicsObservation\$Architecture"
New-Item -ItemType Directory -Force $output | Out-Null
$test = Join-Path $PSScriptRoot 'GraphicsObservationTests.cpp'
$bridge = Join-Path $PSScriptRoot '..\..\..\dxaml\xcp\dxaml\lib\WinUIProfilerRuntimeBridge.cpp'
$include = Join-Path $PSScriptRoot '..\..\..\dxaml\xcp\components\base\inc'
$allocationInclude = Join-Path $PSScriptRoot '..\..\..\dxaml\xcp\components\allocation\inc'
$discovery = Split-Path $vswhere
$command = "set `"PATH=$discovery;%PATH%`" && call `"$vs\VC\Auxiliary\Build\$setup`" >nul && cd /d `"$output`" && cl /nologo /std:c++17 /W4 /WX /EHsc /O2 /DWINUI_PROFILER_ENABLED=1 /DWINUI_PROFILER_STANDALONE /DWIN32_LEAN_AND_MEAN /DNOMINMAX /I`"$include`" /I`"$allocationInclude`" `"$test`" `"$bridge`" /Fe:GraphicsObservationTests.exe /link advapi32.lib ole32.lib tdh.lib"
if ($Architecture -eq 'x64') { $command += ' && .\GraphicsObservationTests.exe && .\GraphicsObservationTests.exe --selection-gap && .\GraphicsObservationTests.exe --external-device && .\GraphicsObservationTests.exe --full-gap && .\GraphicsObservationTests.exe --runtime-restart && .\GraphicsObservationTests.exe --termination && .\GraphicsObservationTests.exe --surface-capacity && .\GraphicsObservationTests.exe --device-capacity && .\GraphicsObservationTests.exe --selection-race && .\GraphicsObservationTests.exe --epoch-race && .\GraphicsObservationTests.exe --texture-wrapper && .\GraphicsObservationTests.exe --brush-owner && .\GraphicsObservationTests.exe --brush-xaml-owner' }
if ($Architecture -eq 'x64') { $command += ' && .\GraphicsObservationTests.exe --brush-owner-gap && .\GraphicsObservationTests.exe --owner-selection-race && .\GraphicsObservationTests.exe --owner-epoch-race' }
& $env:ComSpec /d /c $command
if ($LASTEXITCODE -ne 0) { throw "Graphics observation $Architecture validation failed (exit $LASTEXITCODE)." }
