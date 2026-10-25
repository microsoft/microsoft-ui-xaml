param([ValidateSet('x64', 'arm64')][string]$Architecture = 'x64')
$ErrorActionPreference = 'Stop'
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$vs) { throw 'Installed Visual Studio C++ toolchain not found.' }
$setup = if ($Architecture -eq 'x64') { 'vcvars64.bat' } else { 'vcvarsamd64_arm64.bat' }
$output = Join-Path $PSScriptRoot "bin\TapCom\$Architecture"
New-Item -ItemType Directory -Force $output | Out-Null
$test = Join-Path $PSScriptRoot 'TapComTests.cpp'
$tap = Join-Path $PSScriptRoot '..\WinUIProfiler.Tap'
$platform = if ($Architecture -eq 'x64') { 'x64' } else { 'ARM64' }
& "$vs\MSBuild\Current\Bin\MSBuild.exe" "$tap\WinUIProfiler.Tap.vcxproj" /nologo /v:minimal /t:GenerateTreeSnapshotProjections /p:Configuration=Release /p:Platform=$platform
if ($LASTEXITCODE -ne 0) { throw 'TAP snapshot SDK projection generation failed.' }
$projections = Join-Path $tap 'obj\SnapshotProjections'
$include = Join-Path $PSScriptRoot '..\..\..\dxaml\xcp\components\base\inc'
$sources = "`"$test`" `"$tap\Tap.cpp`" `"$tap\Protocol.cpp`" `"$tap\PipeServer.cpp`" `"$tap\TreeSnapshot.cpp`""
$discovery = Split-Path $vswhere
$command = "set `"PATH=$discovery;%PATH%`" && call `"$vs\VC\Auxiliary\Build\$setup`" >nul && cd /d `"$output`" && cl /nologo /std:c++17 /bigobj /W4 /WX /EHsc /O2 /DWIN32_LEAN_AND_MEAN /DNOMINMAX /I`"$include`" /I`"$projections`" $sources /Fe:TapComTests.exe /link advapi32.lib ole32.lib oleaut32.lib uuid.lib windowsapp.lib runtimeobject.lib"
if ($Architecture -eq 'x64') { $command += ' && .\TapComTests.exe' }
& $env:ComSpec /d /c $command
if ($LASTEXITCODE -ne 0) { throw "TAP COM $Architecture validation failed (exit $LASTEXITCODE)." }
$snapshotTest = Join-Path $PSScriptRoot 'TreeSnapshotTests.cpp'
$command = "set `"PATH=$discovery;%PATH%`" && call `"$vs\VC\Auxiliary\Build\$setup`" >nul && cd /d `"$output`" && cl /nologo /std:c++17 /bigobj /W4 /WX /EHsc /O2 /DWIN32_LEAN_AND_MEAN /DNOMINMAX /I`"$projections`" `"$snapshotTest`" `"$tap\TreeSnapshot.cpp`" /Fe:TreeSnapshotTests.exe /link ole32.lib oleaut32.lib uuid.lib windowsapp.lib runtimeobject.lib"
if ($Architecture -eq 'x64') { $command += ' && .\TreeSnapshotTests.exe' }
& $env:ComSpec /d /c $command
if ($LASTEXITCODE -ne 0) { throw "TAP tree snapshot $Architecture validation failed (exit $LASTEXITCODE)." }
