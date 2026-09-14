# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

[CmdletBinding()]
param([string]$CoverageToolPath)

$ErrorActionPreference = 'Stop'
$scripts = Split-Path $PSScriptRoot -Parent
$tool = & "$scripts\Get-CoverageTool.ps1" -CoverageToolPath $CoverageToolPath
$vsRoot = $tool.Substring(0, $tool.IndexOf('\Common7\', [StringComparison]::OrdinalIgnoreCase))
$vcvars = Join-Path $vsRoot 'VC\Auxiliary\Build\vcvars64.bat'
if (-not (Test-Path -LiteralPath $vcvars))
{
    throw 'The native smoke test requires the Visual Studio x64 C++ tools.'
}

$sessionId = 'native-smoke-' + [guid]::NewGuid().ToString('N')
$root = Join-Path $PSScriptRoot ".$sessionId"
$build = Join-Path $root 'build'
$payload = Join-Path $root 'payload'
$slices = Join-Path $root 'slices'
$merged = Join-Path $root 'merged'
$source = Join-Path $PSScriptRoot 'NativeSmokeFixture.cpp'
$modules = @('Microsoft.ui.xaml', 'Microsoft.UI.Xaml.Controls')
$apps = @('app one', 'app two')

function Invoke-NativeSlice([int]$Slice)
{
    $output = Join-Path $slices "coverage-$Slice.coverage"
    $collector = $null
    try
    {
        # This collector belongs only to this fixture. Its default ACL is never changed.
        $collector = Start-Process -FilePath $tool -ArgumentList @(
            'collect', '--session-id', $sessionId, '--server-mode',
            '--settings', "`"$scripts\coverage.config`"", '--output', "`"$output`""
        ) -PassThru -NoNewWindow -RedirectStandardOutput "$output.log" -RedirectStandardError "$output.err"
        $deadline = (Get-Date).AddSeconds(30)
        while (-not (Test-Path "\\.\pipe\CodeCoverage.pipe.$sessionId"))
        {
            if ($collector.HasExited -or (Get-Date) -ge $deadline)
            {
                throw "Native fixture collector did not become ready. $(Get-Content -LiteralPath "$output.err" -Raw)"
            }
            Start-Sleep -Milliseconds 100
        }
        & "$payload\$($apps[$Slice])\CoverageRunner.exe" $Slice
        if ($LASTEXITCODE -ne 0)
        {
            throw "Native fixture failed with exit code $LASTEXITCODE."
        }
        & $tool shutdown $sessionId | Out-Host
        if ($LASTEXITCODE -ne 0 -or -not $collector.WaitForExit(60000))
        {
            throw 'Native fixture collector failed to shut down.'
        }
        if (-not (Test-Path -LiteralPath $output) -or (Get-Item -LiteralPath $output).Length -eq 0)
        {
            throw "Native fixture produced no coverage in '$output'."
        }
    }
    finally
    {
        if ($collector)
        {
            if (-not $collector.HasExited)
            {
                & $tool shutdown $sessionId | Out-Host
                if (-not $collector.WaitForExit(10000))
                {
                    Stop-Process -Id $collector.Id -ErrorAction Continue
                    $collector.WaitForExit()
                }
            }
            $collector.Dispose()
        }
    }
}

try
{
    New-Item -ItemType Directory -Path $build, $slices | Out-Null
    $commands = @("call `"$vcvars`" >nul")
    foreach ($module in $modules)
    {
        # Native instrumentation needs the linker metadata emitted by /PROFILE.
        $commands += "cl /nologo /LD /Zi /Od /MT /DCOVERAGE_TEST_DLL /Fo`"$build\$module.obj`" /Fd`"$build\compiler-$module.pdb`" `"$source`" /link /DEBUG:FULL /PROFILE /INCREMENTAL:NO /OUT:`"$build\$module.dll`" /IMPLIB:`"$build\$module.lib`" /PDB:`"$build\$module.pdb`""
    }
    $commands += "cl /nologo /Od /MT /Fo`"$build\runner.obj`" /Fe`"$build\CoverageRunner.exe`" `"$source`""
    & $env:ComSpec /d /c ($commands -join ' && ')
    if ($LASTEXITCODE -ne 0)
    {
        throw 'The native coverage fixture did not compile.'
    }
    foreach ($app in $apps)
    {
        $directory = Join-Path $payload $app
        New-Item -ItemType Directory -Path $directory -Force | Out-Null
        Copy-Item "$build\*.dll", "$build\CoverageRunner.exe" -Destination $directory
    }

    $originalHashes = @{}
    foreach ($module in $modules)
    {
        $originalHashes[$module] = (Get-FileHash -LiteralPath "$build\$module.dll").Hash
    }
    & "$scripts\Instrument-CoveragePayload.ps1" -PayloadDir $payload -SymbolsSearchRoot $build `
        -SessionId $sessionId -CoverageToolPath $tool
    foreach ($module in $modules)
    {
        $hashes = @($apps | ForEach-Object { (Get-FileHash -LiteralPath "$payload\$_\$module.dll").Hash })
        if ($hashes[0] -eq $originalHashes[$module] -or $hashes[0] -ne $hashes[1])
        {
            throw "The native copies of $module.dll were not instrumented identically."
        }
    }
    foreach ($app in $apps)
    {
        if (@(Get-ChildItem -LiteralPath "$payload\$app" -Filter 'static_covrun*.dll').Count -eq 0)
        {
            throw "The native payload is missing its coverage runtime in '$app'."
        }
        foreach ($module in $modules)
        {
            if (Test-Path -LiteralPath "$payload\$app\$module.pdb")
            {
                throw "Staged symbols were not removed for $module in '$app'."
            }
        }
    }

    # Use the bundled collector, not the installed executable, to exercise its dependencies.
    $tool = Join-Path $payload 'CoverageTool\Microsoft.CodeCoverage.Console.exe'
    Invoke-NativeSlice 0
    Invoke-NativeSlice 1
    & "$scripts\Merge-CodeCoverage.ps1" -InputDir $slices -OutputDir $merged -CoverageToolPath $tool
    [xml]$report = Get-Content -LiteralPath "$merged\merged.cobertura.xml" -Raw
    foreach ($module in $modules)
    {
        $packages = @($report.coverage.packages.package | Where-Object { $_.name -in @($module, "$module.dll") })
        if ($packages.Count -eq 0 -or @($packages | Where-Object { [double]$_.'line-rate' -gt 0 }).Count -eq 0)
        {
            throw "Merged Cobertura report has no executed lines for $module."
        }
    }

    $validBytes = [IO.File]::ReadAllBytes((Join-Path $slices 'coverage-0.coverage'))
    foreach ($corruption in @('garbage', 'truncated'))
    {
        $invalidFile = Join-Path $slices 'coverage-invalid.coverage'
        if ($corruption -eq 'garbage')
        {
            [IO.File]::WriteAllText($invalidFile, 'not a coverage file')
        }
        else
        {
            [IO.File]::WriteAllBytes($invalidFile, $validBytes[0..([int]($validBytes.Length / 2))])
        }
        $rejected = $false
        try
        {
            & "$scripts\Merge-CodeCoverage.ps1" -InputDir $slices -OutputDir "$root\$corruption" -CoverageToolPath $tool
        }
        catch
        {
            $rejected = $true
            Write-Host "Rejected $corruption coverage alongside valid slices: $($_.Exception.Message)"
        }
        if (-not $rejected)
        {
            throw "The merge accepted $corruption coverage alongside valid slices."
        }
    }
    Write-Host 'Native smoke passed: two DLLs, two payload copies, two collected slices, Cobertura and binary coverage reports.'
}
finally
{
    if (Test-Path -LiteralPath $root)
    {
        Remove-Item -LiteralPath $root -Recurse -Force
    }
}
