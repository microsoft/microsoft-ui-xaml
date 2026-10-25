param([ValidateSet('x64', 'arm64')][string]$Architecture = 'x64')
$ErrorActionPreference = 'Stop'
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$vs) { throw 'Installed Visual Studio C++ toolchain not found.' }
$setup = if ($Architecture -eq 'x64') { 'vcvars64.bat' } else { 'vcvarsamd64_arm64.bat' }
$output = Join-Path $PSScriptRoot "bin\AllocatorWithoutRuntime\$Architecture"
New-Item -ItemType Directory -Force $output | Out-Null
$test = Join-Path $PSScriptRoot 'AllocatorWithoutRuntimeTests.cpp'
$include = Join-Path $PSScriptRoot '..\..\..\dxaml\xcp\components\base\inc'
$allocationInclude = Join-Path $PSScriptRoot '..\..\..\dxaml\xcp\components\allocation\inc'
$discovery = Split-Path $vswhere
$command = "set `"PATH=$discovery;%PATH%`" && call `"$vs\VC\Auxiliary\Build\$setup`" >nul && cd /d `"$output`" && cl /nologo /std:c++17 /W4 /WX /EHsc /O2 /DWINUI_PROFILER_ENABLED=1 /I`"$include`" /I`"$allocationInclude`" `"$test`" /Fe:AllocatorWithoutRuntimeTests.exe"
if ($Architecture -eq 'x64') { $command += ' && .\AllocatorWithoutRuntimeTests.exe' }
& $env:ComSpec /d /c $command
if ($LASTEXITCODE -ne 0) { throw "Allocator without runtime $Architecture validation failed (exit $LASTEXITCODE)." }
