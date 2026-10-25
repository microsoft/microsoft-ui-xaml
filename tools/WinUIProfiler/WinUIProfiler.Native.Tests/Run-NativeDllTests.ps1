param(
    [ValidateSet('x64', 'ARM64')][string]$Architecture = 'x64',
    [switch]$SkipBuild
)
$ErrorActionPreference = 'Stop'
$nativeRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\WinUIProfiler.Native'))
if (!$SkipBuild) { & (Join-Path $nativeRoot 'Build-NativeDll.ps1') -Architecture $Architecture }
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$vs) { throw 'Installed Visual Studio C++ toolchain not found.' }
$setup = if ($Architecture -eq 'x64') { 'vcvars64.bat' } else { 'vcvarsamd64_arm64.bat' }
$output = Join-Path $PSScriptRoot "bin\$Architecture"
New-Item -ItemType Directory -Force $output | Out-Null
$test = Join-Path $PSScriptRoot 'NativeDllSmokeTests.cpp'
$dll = Join-Path $nativeRoot "bin\$Architecture\Release\WinUIProfiler.Native.dll"
$discovery = Split-Path $vswhere
$command = "set `"PATH=$discovery;%PATH%`" && call `"$vs\VC\Auxiliary\Build\$setup`" >nul && cd /d `"$output`" && cl /nologo /std:c++17 /W4 /WX /EHsc /O2 `"$test`" /Fe:NativeDllSmokeTests.exe"
if ($Architecture -eq 'x64') { $command += " && .\NativeDllSmokeTests.exe `"$dll`"" }
& $env:ComSpec /d /c $command
if ($LASTEXITCODE -ne 0) { throw "Native DLL $Architecture smoke failed (exit $LASTEXITCODE)." }
