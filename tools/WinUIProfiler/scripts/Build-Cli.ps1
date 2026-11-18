[CmdletBinding()]
param(
    [ValidateSet('Release', 'Debug')][string]$Configuration = 'Release',
    [Alias('Architecture')][ValidateSet('x64', 'ARM64')][string]$Platform = 'x64',
    [string]$OutputDirectory,
    [switch]$Publish
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot
$project = Join-Path $root 'WinUIProfiler.Cli\WinUIProfiler.Cli.csproj'
if (!$OutputDirectory) { $OutputDirectory = Join-Path $root "artifacts\cli\win-$($Platform.ToLowerInvariant())\$Configuration" }
$OutputDirectory = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($OutputDirectory)
$runtime = "win-$($Platform.ToLowerInvariant())"
Push-Location -LiteralPath $root
try {
    & (Join-Path $PSScriptRoot 'Build-NativePayloads.ps1') -Configuration $Configuration -Platform $Platform
    & dotnet restore $project "-p:Configuration=$Configuration" "-p:SelfContained=$($Publish.IsPresent.ToString().ToLowerInvariant())" `
        -r $runtime -nologo -v:quiet
    if ($LASTEXITCODE -ne 0) { throw 'Restoring CLI dependencies failed.' }
    if ($Publish) {
        & dotnet publish $project --no-restore "-p:Configuration=$Configuration" -r $runtime `
            --self-contained true -o $OutputDirectory -nologo -v:quiet
    }
    else {
        & dotnet build $project --no-restore "-p:Configuration=$Configuration" -r $runtime --self-contained false "-p:PlatformTarget=$Platform" `
            -o $OutputDirectory -nologo -v:quiet
    }
    if ($LASTEXITCODE -ne 0) { throw 'Building the CLI failed.' }
    & (Join-Path $PSScriptRoot 'Build-NativePayloads.ps1') -Operation Stage -Configuration $Configuration `
        -Platform $Platform -OutputDirectory $OutputDirectory
}
finally { Pop-Location }
Write-Output "Profiler CLI: $(Join-Path $OutputDirectory 'WinUIProfiler.Cli.exe')"
