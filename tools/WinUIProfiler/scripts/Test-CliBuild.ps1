#requires -Version 7.4
[CmdletBinding()]
param([Parameter(Mandatory)][string]$ArtifactDirectory)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot
$fixture = Join-Path ($ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($ArtifactDirectory)) "cli-build-$([guid]::NewGuid().ToString('N'))"
$null = New-Item -ItemType Directory -Path $fixture
$failures = @()
try {
    $scripts = Join-Path $fixture 'scripts'
    $null = New-Item -ItemType Directory -Path $scripts
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'Build-NativePayloads.ps1') -Destination $scripts
    foreach ($architecture in @('x64', 'ARM64')) {
        foreach ($project in @('WinUIProfiler.Tap', 'WinUIProfiler.Injector')) {
            $directory = Join-Path $fixture "$project\bin\$architecture\Release"
            $null = New-Item -ItemType Directory -Path $directory -Force
            $extension = if ($project -eq 'WinUIProfiler.Tap') { 'dll' } else { 'exe' }
            [IO.File]::WriteAllBytes((Join-Path $directory "$project.$extension"), [byte[]]@(1, 2, 3))
        }
        $directory = Join-Path $fixture "artifacts\launcher\win-$($architecture.ToLowerInvariant())"
        $null = New-Item -ItemType Directory -Path $directory -Force
        foreach ($name in @('ProfilerLauncher.exe', 'ProfilerRedirect.dll', 'DETOURS-LICENSE.md')) {
            [IO.File]::WriteAllBytes((Join-Path $directory $name), [byte[]]@(4, 5, 6))
        }
    }
    $native = Join-Path $fixture 'WinUIProfiler.Native\bin\x64\Release'
    $null = New-Item -ItemType Directory -Path $native -Force
    [IO.File]::WriteAllBytes((Join-Path $native 'WinUIProfiler.Native.dll'), [byte[]]@(7, 8, 9))
    $stage = Join-Path $fixture 'stage'
    $null = New-Item -ItemType Directory -Path $stage
    $powershell51 = Join-Path $env:SystemRoot 'System32\WindowsPowerShell\v1.0\powershell.exe'
    & $powershell51 -NoLogo -NoProfile -NonInteractive -File (Join-Path $scripts 'Build-NativePayloads.ps1') `
        -Operation Stage -Configuration Release -Platform x64 -OutputDirectory $stage
    if ($LASTEXITCODE -ne 0) { $failures += 'Native payload staging must work in Windows PowerShell 5.1.' }
    else {
        $manifest = Get-Content -LiteralPath (Join-Path $stage 'ConnectionPayloads\manifest.json') -Raw | ConvertFrom-Json
        if ($manifest.Payloads.Count -ne 2 -or
            $manifest.Payloads[0].TapSHA256 -ne (Get-FileHash -LiteralPath (Join-Path $stage 'ConnectionPayloads\x64\WinUIProfiler.Tap.dll')).Hash) {
            throw 'PowerShell 5.1 staging must retain verified payloads for both architectures.'
        }
        Write-Output 'Passed: Windows PowerShell 5.1 stages and verifies native payloads.'
    }

    Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'Build-Cli.ps1') -Destination $scripts
    Copy-Item -LiteralPath (Join-Path $root 'NuGet.Config') -Destination $fixture
    [IO.File]::WriteAllText((Join-Path $fixture 'Directory.Build.props'), @"
<Project><PropertyGroup><TargetFramework>net10.0</TargetFramework><ImplicitUsings>enable</ImplicitUsings><RestorePackagesPath>$root\.packages</RestorePackagesPath></PropertyGroup></Project>
"@)
    foreach ($project in @('Core', 'App', 'Cli')) {
        $directory = Join-Path $fixture "WinUIProfiler.$project"
        $null = New-Item -ItemType Directory -Path $directory
        $reference = if ($project -eq 'Core') { '' } else {
            '<ItemGroup><ProjectReference Include="..\WinUIProfiler.Core\WinUIProfiler.Core.csproj" /></ItemGroup>'
        }
        $output = if ($project -eq 'Core') { '' } else { '<PropertyGroup><OutputType>Exe</OutputType></PropertyGroup>' }
        [IO.File]::WriteAllText((Join-Path $directory "WinUIProfiler.$project.csproj"), "<Project Sdk=`"Microsoft.NET.Sdk`">$output$reference</Project>")
        [IO.File]::WriteAllText((Join-Path $directory 'Program.cs'), $(if ($project -eq 'Core') {
            'public static class Fixture { public static string Value => "CLI build fixture"; }'
        } else { 'System.Console.WriteLine(Fixture.Value);' }))
    }
    # Native preparation's App restore is the side effect that used to invalidate the CLI's Core runtime assets.
    [IO.File]::WriteAllText((Join-Path $scripts 'Build-NativePayloads.ps1'), @'
param($Configuration, $Platform, $Operation = 'Build', $OutputDirectory)
if ($Operation -eq 'Build') {
    & dotnet restore (Join-Path (Split-Path $PSScriptRoot) 'WinUIProfiler.App\WinUIProfiler.App.csproj') -p:Platform=x64 -v:quiet
    if ($LASTEXITCODE -ne 0) { throw 'Fixture App restore failed.' }
    # Make the shared-graph invalidation deterministic across SDK restore optimizations.
    Remove-Item -LiteralPath (Join-Path (Split-Path $PSScriptRoot) 'WinUIProfiler.Core\obj\project.assets.json')
}
'@)
    foreach ($publish in @($false, $true)) {
        try {
            & (Join-Path $scripts 'Build-Cli.ps1') -Configuration Release -Platform x64 -Publish:$publish `
                -OutputDirectory (Join-Path $fixture "cli-$publish")
            if ($LASTEXITCODE -ne 0) { throw 'CLI fixture build failed.' }
            $text = & (Join-Path $fixture "cli-$publish\WinUIProfiler.Cli.exe")
            if ($LASTEXITCODE -ne 0 -or $text -ne 'CLI build fixture') { throw 'Built CLI fixture did not run.' }
            Write-Output "Passed: CLI runtime assets survive native App restore (Publish=$publish)."
        }
        catch { $failures += $_.Exception.Message }
    }
    if ($failures.Count) { throw ($failures -join [Environment]::NewLine) }
}
finally { Remove-Item -LiteralPath $fixture -Recurse -Force }
