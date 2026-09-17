param(
    [Parameter(Mandatory=$true)] [string] $Stage,
    [Parameter(Mandatory=$true)] [string] $BuildArguments
)

$ErrorActionPreference = "Stop"
$startInfo = New-Object System.Diagnostics.ProcessStartInfo
$startInfo.FileName = (Get-Command msbuild.exe -CommandType Application -ErrorAction Stop | Select-Object -First 1).Source
$startInfo.Arguments = $BuildArguments
$startInfo.UseShellExecute = $false

$watch = [System.Diagnostics.Stopwatch]::StartNew()
$process = [System.Diagnostics.Process]::Start($startInfo)
try {
    while (-not $process.WaitForExit(60000)) {
        Write-Host ("Still building {0} ({1:hh\:mm\:ss} elapsed)..." -f $Stage, $watch.Elapsed)
    }
    $exitCode = $process.ExitCode
    Write-Host ("Finished {0} in {1:hh\:mm\:ss} (exit code {2})." -f $Stage, $watch.Elapsed, $exitCode)
}
finally {
    $process.Dispose()
}
exit $exitCode
