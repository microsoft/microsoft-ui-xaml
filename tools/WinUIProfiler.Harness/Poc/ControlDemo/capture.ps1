# Builds the control demo, records owned ETW sessions, and imports each trace through WinUIProfiler.
#Requires -Version 7
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$ProfilerCli,
    [Parameter(Mandatory)][string]$InitRun,
    [Parameter(Mandatory)][string]$DotNet,
    [Parameter(Mandatory)][string]$RunDirectory,
    [ValidateRange(1, 100)][int]$Iterations = 10
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$PSNativeCommandUseErrorActionPreference = $false
$ProfilerCli = (Resolve-Path -LiteralPath $ProfilerCli -ErrorAction Stop).Path
$InitRun = (Resolve-Path -LiteralPath $InitRun -ErrorAction Stop).Path
$DotNet = (Resolve-Path -LiteralPath $DotNet -ErrorAction Stop).Path
$RunDirectory = [IO.Path]::GetFullPath($RunDirectory)
$sourceRoot = [IO.Path]::GetFullPath($PSScriptRoot)
if ($RunDirectory.TrimEnd('\') -eq $sourceRoot.TrimEnd('\') -or
    $RunDirectory.StartsWith("$($sourceRoot.TrimEnd('\'))\", [StringComparison]::OrdinalIgnoreCase)) {
    throw 'RunDirectory must be outside the demo source.'
}
if (Test-Path -LiteralPath $RunDirectory) {
    throw 'RunDirectory must be new; existing measurements are never overwritten.'
}
Get-Command pwsh, logman -ErrorAction Stop | Out-Null
$mutex = [Threading.Mutex]::new($false, 'Local\WinUIProfilerHarness-ControlCapture')
if (-not $mutex.WaitOne(0)) {
    $mutex.Dispose()
    throw 'Another control-demo capture is running in this session.'
}

$sourceFiles = @(
    'App.xaml', 'App.xaml.cs', 'ScenarioWorkload.cs', 'ControlDemo.csproj',
    'Directory.Build.props', 'Directory.Build.targets', 'app.manifest',
    'global.json', 'NuGet.Config', 'markers.json', 'providers.txt', 'packages.lock.json',
    'build.ps1'
)
try {
    New-Item -ItemType Directory -Path $RunDirectory | Out-Null
    $hashes = @{}
    foreach ($file in $sourceFiles) {
        $hashes[$file] = (Get-FileHash -LiteralPath (Join-Path $sourceRoot $file)).Hash
    }
    # The XAML compiler's metadata loader cannot resolve intermediate assembly paths over MAX_PATH.
    $buildRoot = Join-Path $env:LOCALAPPDATA ('WinUIProfilerHarness\Builds\' + [guid]::NewGuid().ToString('N'))
    New-Item -ItemType Directory -Path $buildRoot | Out-Null
    Push-Location $sourceRoot
    try {
        & pwsh -NoProfile -File (Join-Path $sourceRoot 'build.ps1') `
            -InitRun $InitRun -DotNet $DotNet -OutputRoot $buildRoot
        if ($LASTEXITCODE -ne 0) {
            throw 'The demo build failed.'
        }
    } finally {
        Pop-Location
    }
    $executable = (Get-Content -LiteralPath (Join-Path $buildRoot 'obj\ControlDemoOutput.txt') -Raw).Trim()
    if (-not [IO.Path]::IsPathFullyQualified($executable) -or
        -not (Test-Path -LiteralPath $executable -PathType Leaf)) {
        throw 'The build did not produce an absolute executable output path.'
    }

    $runs = @()
    for ($iteration = 1; $iteration -le $Iterations; $iteration++) {
        $directory = Join-Path $RunDirectory ('iteration-{0:D3}' -f $iteration)
        New-Item -ItemType Directory -Path $directory | Out-Null
        $trace = Join-Path $directory 'trace.etl'
        $resultPath = Join-Path $directory 'app-result.json'
        $database = Join-Path $directory 'session.sqlite'
        $session = 'WinUIProfilerHarness-' + [guid]::NewGuid().ToString('N')
        & logman create trace $session -o $trace -pf (Join-Path $sourceRoot 'providers.txt') -bs 1024 -nb 16 128 -ets
        if ($LASTEXITCODE -ne 0) {
            throw 'ETW capture could not start. Elevation or Performance Log Users membership may be required.'
        }
        $process = $null
        try {
            $start = [Diagnostics.ProcessStartInfo]::new($executable)
            $start.UseShellExecute = $false
            $start.WorkingDirectory = Split-Path $executable -Parent
            $start.ArgumentList.Add('--result')
            $start.ArgumentList.Add($resultPath)
            $process = [Diagnostics.Process]::Start($start)
            $processId = $process.Id
            $createdUtc = $process.StartTime.ToUniversalTime()
            if (-not $process.WaitForExit(30000)) {
                throw "Iteration $iteration timed out."
            }
            if ($process.ExitCode -ne 0) {
                if (Test-Path -LiteralPath $resultPath) {
                    Write-Host (Get-Content -LiteralPath $resultPath -Raw)
                }
                throw "Iteration $iteration exited with code $($process.ExitCode)."
            }
        } finally {
            if ($process) {
                if (-not $process.HasExited) {
                    Stop-Process -Id $process.Id -Force
                }
                $process.Dispose()
            }
            & logman stop $session -ets
            if ($LASTEXITCODE -ne 0) {
                throw "Could not stop the owned ETW session $session."
            }
        }
        $app = Get-Content -LiteralPath $resultPath -Raw | ConvertFrom-Json
        if (-not $app.passed -or $app.processId -ne $processId -or
            ([DateTime]$app.processCreatedUtc).ToUniversalTime() -ne $createdUtc) {
            throw "Iteration $iteration returned incorrect process or scenario results."
        }
        & $ProfilerCli record $trace --pid $processId --out $database --markers (Join-Path $sourceRoot 'markers.json')
        if ($LASTEXITCODE -ne 0) {
            throw "WinUIProfiler failed to import iteration $iteration."
        }
        $runs += [ordered]@{
            iteration = $iteration
            trace = $trace
            database = $database
            appResult = $resultPath
            frameworkSha256 = (Get-FileHash -LiteralPath $app.frameworkPath).Hash
        }
        Write-Host "Captured and imported iteration $iteration of $Iterations."
    }

    foreach ($file in $sourceFiles) {
        if ((Get-FileHash -LiteralPath (Join-Path $sourceRoot $file)).Hash -ne $hashes[$file]) {
            throw "The input $file changed during measurement."
        }
    }
    $manifest = [ordered]@{
        schema = 'winui-profiler-control-poc/capture-v1'
        recordedUtc = [DateTime]::UtcNow.ToString('O')
        sourceRoot = $sourceRoot
        sourceHashes = $hashes
        profilerCli = $ProfilerCli
        profilerSha256 = (Get-FileHash -LiteralPath $ProfilerCli).Hash
        executable = $executable
        executableSha256 = (Get-FileHash -LiteralPath $executable).Hash
        operatingSystem = [Environment]::OSVersion.VersionString
        iterations = @($runs)
    }
    [IO.File]::WriteAllText(
        (Join-Path $RunDirectory 'capture-manifest.json'),
        ($manifest | ConvertTo-Json -Depth 10))
    Write-Output "CONTROL-CAPTURE: $RunDirectory"
} finally {
    $mutex.ReleaseMutex()
    $mutex.Dispose()
}
