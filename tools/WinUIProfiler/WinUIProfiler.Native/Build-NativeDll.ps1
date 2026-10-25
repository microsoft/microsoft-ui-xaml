param([ValidateSet('x64', 'ARM64')][string]$Architecture = 'x64')
$ErrorActionPreference = 'Stop'
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$vs) { throw 'Installed Visual Studio C++ toolchain not found.' }
$msbuild = Join-Path $vs 'MSBuild\Current\Bin\MSBuild.exe'
$project = Join-Path $PSScriptRoot 'WinUIProfiler.Native.vcxproj'
$toolset = Get-ChildItem (Join-Path $vs 'MSBuild\Microsoft\VC\v*\Platforms\x64\PlatformToolsets\v*') -Directory |
    Sort-Object Name -Descending | Select-Object -First 1 -ExpandProperty Name
if (!$toolset) { throw 'Installed C++ platform toolset not found.' }
& $msbuild $project /nologo /verbosity:quiet /t:Build /p:Configuration=Release "/p:Platform=$Architecture" "/p:PlatformToolset=$toolset" /p:BuildProjectReferences=false
if ($LASTEXITCODE -ne 0) { throw "Native DLL $Architecture build failed (exit $LASTEXITCODE)." }
$dll = Join-Path $PSScriptRoot "bin\$Architecture\Release\WinUIProfiler.Native.dll"
if (!(Test-Path $dll)) { throw "Native DLL build produced no artifact: $dll" }
Write-Output "Native DLL: $dll"
Get-FileHash -LiteralPath $dll -Algorithm SHA256 | Format-List Path,Hash
$dumpbin = Get-ChildItem (Join-Path $vs 'VC\Tools\MSVC\*\bin\Hostx64\x64\dumpbin.exe') |
    Sort-Object FullName -Descending | Select-Object -First 1 -ExpandProperty FullName
if (!$dumpbin) { throw 'Installed dumpbin not found; export/architecture validation incomplete.' }
& $dumpbin /nologo /exports $dll
if ($LASTEXITCODE -ne 0) { throw 'Native export scan failed.' }
& $dumpbin /nologo /headers $dll | Select-String 'machine \('
if ($LASTEXITCODE -ne 0) { throw 'Native architecture scan failed.' }
