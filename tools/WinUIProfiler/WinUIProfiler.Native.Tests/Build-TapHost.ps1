param([ValidateSet('x64', 'arm64')][string]$Architecture = 'x64')
$ErrorActionPreference = 'Stop'
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$vs) { throw 'Installed Visual Studio C++ toolchain not found.' }
$setup = if ($Architecture -eq 'x64') { 'vcvars64.bat' } else { 'vcvarsamd64_arm64.bat' }
$platform = if ($Architecture -eq 'x64') { 'x64' } else { 'ARM64' }
$output = Join-Path $PSScriptRoot "bin\TapHost\$Architecture"
New-Item -ItemType Directory -Force $output | Out-Null
$tap = Join-Path $PSScriptRoot '..\WinUIProfiler.Tap'
& "$vs\MSBuild\Current\Bin\MSBuild.exe" "$tap\WinUIProfiler.Tap.vcxproj" /nologo /v:minimal /m /p:Configuration=Release /p:Platform=$platform
if ($LASTEXITCODE -ne 0) { throw "Native TAP $Architecture DLL build failed (exit $LASTEXITCODE)." }
Copy-Item "$tap\bin\$platform\Release\WinUIProfiler.Tap.dll" "$output\WinUIProfiler.Tap.dll" -Force
$test = Join-Path $PSScriptRoot 'TapHost.cpp'
$include = Join-Path $PSScriptRoot '..\..\..\dxaml\xcp\components\base\inc'
$discovery = Split-Path $vswhere
$command = "set `"PATH=$discovery;%PATH%`" && call `"$vs\VC\Auxiliary\Build\$setup`" >nul && cd /d `"$output`" && cl /nologo /std:c++17 /W4 /WX /EHsc /O2 /MT /DWIN32_LEAN_AND_MEAN /DNOMINMAX /I`"$include`" `"$test`" `"$tap\Protocol.cpp`" /Fe:TapHost.exe /link ole32.lib oleaut32.lib uuid.lib"
& $env:ComSpec /d /c $command
if ($LASTEXITCODE -ne 0) { throw "Native TAP $Architecture host build failed (exit $LASTEXITCODE)." }
