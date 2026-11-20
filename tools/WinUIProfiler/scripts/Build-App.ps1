[CmdletBinding()]
param(
    [ValidateSet('Release', 'Debug')][string]$Configuration = 'Release',
    [Alias('Architecture')][ValidateSet('x64', 'ARM64')][string]$Platform = 'x64'
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot
if (!(Get-Command dotnet -CommandType Application -ErrorAction SilentlyContinue)) {
    throw 'The .NET SDK is required. Run .\scripts\Install-Prerequisites.ps1 to install it.'
}
$appProject = Join-Path $root 'WinUIProfiler.App\WinUIProfiler.App.csproj'
Push-Location -LiteralPath $root
try { & dotnet restore $appProject "-p:Configuration=$Configuration" "-p:Platform=$Platform" -nologo -v:quiet }
finally { Pop-Location }
if ($LASTEXITCODE -ne 0) { throw 'Restoring the SDK metadata for the app and tree snapshot TAP failed.' }
& (Join-Path $PSScriptRoot 'Build-NativePayloads.ps1') -Configuration $Configuration -Platform $Platform
Push-Location -LiteralPath $root
try {
    & dotnet build $appProject --no-restore "-p:Configuration=$Configuration" "-p:Platform=$Platform" -nologo -v:minimal
}
finally { Pop-Location }
if ($LASTEXITCODE -ne 0) { throw 'Building the self-contained profiler executable failed.' }
$outputFile = Join-Path $root "WinUIProfiler.App\obj\ProfilerAppOutput.$Platform.$Configuration.txt"
$output = (Get-Content -LiteralPath $outputFile -Raw).Trim()
if (![IO.Path]::IsPathRooted($output) -or $output -ne [IO.Path]::GetFullPath($output)) {
    throw 'The evaluated executable output is not an absolute canonical path.'
}
& (Join-Path $PSScriptRoot 'Build-NativePayloads.ps1') -Operation Stage -Configuration $Configuration `
    -Platform $Platform -OutputDirectory $output
foreach ($file in @('WinUIProfiler.App.exe', 'Microsoft.UI.Xaml.dll', 'WinUIProfiler.Native.dll')) {
    if (!(Test-Path -LiteralPath (Join-Path $output $file) -PathType Leaf)) { throw "Required executable payload missing: $file" }
}
Write-Output "Profiler executable: $(Join-Path $output 'WinUIProfiler.App.exe')"
