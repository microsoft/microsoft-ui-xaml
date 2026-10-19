param(
    [ValidateSet('x64', 'ARM64', 'x86')][string]$Platform = 'x64',
    [ValidateSet('Debug', 'Release')][string]$Configuration = 'Debug',
    [switch]$Test
)
$ErrorActionPreference = 'Stop'
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$installation = & $vswhere -latest -products '*' -requires Microsoft.Component.MSBuild -property installationPath
if (!$installation) { throw 'Visual Studio with MSBuild and C++ tools is required.' }
$msbuild = Join-Path $installation 'MSBuild\Current\Bin\MSBuild.exe'
$nativePlatform = if ($Platform -eq 'x86') { 'Win32' } else { $Platform }
Push-Location $PSScriptRoot
try {
    & $msbuild 'OnlineXamlProfilerTap\OnlineXamlProfilerTap.vcxproj' /restore /m:2 /nologo /v:minimal "/p:Platform=$nativePlatform" "/p:Configuration=$Configuration"
    if ($LASTEXITCODE -ne 0) { throw "TAP build failed ($LASTEXITCODE)." }
    & $msbuild 'OnlineXamlProfiler.App\OnlineXamlProfiler.App.csproj' /restore /m:2 /nologo /v:minimal "/p:Platform=$Platform" "/p:Configuration=$Configuration"
    if ($LASTEXITCODE -ne 0) { throw "App build failed ($LASTEXITCODE)." }
    if ($Test) {
        & dotnet test 'OnlineXamlProfiler.Tests\OnlineXamlProfiler.Tests.csproj' --configuration $Configuration --filter 'TestCategory!=Live' --nologo
        if ($LASTEXITCODE -ne 0) { throw "Tests failed ($LASTEXITCODE)." }
    }
}
finally {
    Pop-Location
}
