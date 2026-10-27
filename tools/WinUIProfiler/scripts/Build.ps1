param(
    [ValidateSet('Debug', 'Release')][string]$Configuration = 'Release',
    [ValidateSet('x64', 'ARM64')][string]$Architecture = 'x64',
    [switch]$Clean
)

$ErrorActionPreference = 'Stop'
$tool = Split-Path $PSScriptRoot
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (!(Test-Path -LiteralPath $vswhere)) {
    throw 'Visual Studio MSBuild is required for the WinUI XAML compiler.'
}
$msbuild = & $vswhere -latest -products '*' -requires Microsoft.Component.MSBuild `
    -find 'MSBuild\**\Bin\amd64\MSBuild.exe' | Select-Object -First 1
if (!$msbuild) {
    throw 'The x64 Visual Studio MSBuild installation was not found.'
}

$previousSdkDirectory = $env:DOTNET_MSBUILD_SDK_RESOLVER_CLI_DIR
Push-Location $tool
try {
    # initrun selects the product's .NET 9 SDK; this standalone app uses the installed .NET 10 SDK.
    $env:DOTNET_MSBUILD_SDK_RESOLVER_CLI_DIR = Join-Path $env:ProgramFiles 'dotnet'
    $target = if ($Clean) { 'Rebuild' } else { 'Build' }
    & $msbuild .\WinUIProfiler.sln /restore "/t:$target" "/p:Configuration=$Configuration" `
        "/p:Platform=$Architecture" /verbosity:quiet /nologo
    if ($LASTEXITCODE -ne 0) {
        throw 'Profiler application build failed.'
    }
}
finally {
    $env:DOTNET_MSBUILD_SDK_RESOLVER_CLI_DIR = $previousSdkDirectory
    Pop-Location
}
