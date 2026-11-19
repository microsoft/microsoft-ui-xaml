[CmdletBinding()]
param(
    [Parameter(Mandatory)][string] $BinaryPath,
    [Parameter(Mandatory)][ValidatePattern('^[0-9a-fA-F]{40}$')][string] $Commit,
    [Parameter(Mandatory)][string] $BuildId,
    [Parameter(Mandatory)][string] $AgentName,
    [Parameter(Mandatory)][string] $OutputPath,
    [string] $ScenarioName = 'Startup.LoadWinUIBinary',
    [int] $SampleCount = 7,
    [int] $WarmupCount = 1
)

$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'PRPerfResults.psm1') -Force

if (-not (Test-Path -LiteralPath $BinaryPath -PathType Leaf)) {
    throw "Binary to measure was not found: $BinaryPath"
}
$resolvedBinary = (Resolve-Path -LiteralPath $BinaryPath).ProviderPath

# Each sample runs in a brand new process. Loading the same module twice in one process
# only increments a reference count, so repeated in-process loads would measure nothing.
$probe = @'
param([string] $Target)
$signature = @"
[DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
public static extern IntPtr LoadLibraryExW(string path, IntPtr reserved, uint flags);
"@
$native = Add-Type -MemberDefinition $signature -Name PRPerfLoader -Namespace PRPerf -PassThru
$stopwatch = [System.Diagnostics.Stopwatch]::StartNew()
$handle = $native::LoadLibraryExW($Target, [IntPtr]::Zero, 0x00000008)
$stopwatch.Stop()
if ($handle -eq [IntPtr]::Zero) {
    throw "LoadLibraryExW failed for '$Target' with error $([System.Runtime.InteropServices.Marshal]::GetLastWin32Error())."
}
$stopwatch.Elapsed.TotalMilliseconds.ToString([System.Globalization.CultureInfo]::InvariantCulture)
'@

$probePath = Join-Path ([System.IO.Path]::GetTempPath()) "prperf-probe-$([guid]::NewGuid()).ps1"
Set-Content -LiteralPath $probePath -Value $probe -Encoding UTF8

$samples = @()
try {
    $totalRuns = $WarmupCount + $SampleCount
    for ($run = 0; $run -lt $totalRuns; $run++) {
        $output = & powershell.exe -NoProfile -NonInteractive -ExecutionPolicy Bypass -File $probePath -Target $resolvedBinary 2>&1
        if ($LASTEXITCODE -ne 0) {
            throw "Perf probe failed for '$resolvedBinary': $output"
        }

        $reading = @($output | Where-Object { $_ -is [string] -and $_ -match '^\s*[0-9]+([.,][0-9]+)?\s*$' }) |
            Select-Object -Last 1
        if ([string]::IsNullOrWhiteSpace($reading)) {
            throw "Perf probe did not report a timing for '$resolvedBinary'. Output: $output"
        }

        # The first runs page the binary in from disk, which measures the file system
        # rather than the load itself, so they are discarded rather than averaged in.
        if ($run -lt $WarmupCount) { continue }

        $samples += [double]::Parse(
            $reading.Trim(),
            [System.Globalization.NumberStyles]::Float,
            [System.Globalization.CultureInfo]::InvariantCulture)
    }
} finally {
    Remove-Item -LiteralPath $probePath -Force -ErrorAction SilentlyContinue
}

$result = New-PRPerfLocalResult `
    -Commit $Commit `
    -BuildId $BuildId `
    -AgentName $AgentName `
    -ScenarioName $ScenarioName `
    -Samples $samples

$outputDirectory = Split-Path -Parent $OutputPath
if (-not [string]::IsNullOrWhiteSpace($outputDirectory)) {
    New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null
}
$result | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath $OutputPath -Encoding UTF8

$statistics = Get-PRPerfStatistics -Samples $samples
Write-Host ("Measured {0} on '{1}': median {2:F2} ms, CV {3:P1} over {4} samples." -f
    $ScenarioName, (Split-Path -Leaf $resolvedBinary), $statistics.Median, $statistics.CoefficientOfVariation, $samples.Count)
