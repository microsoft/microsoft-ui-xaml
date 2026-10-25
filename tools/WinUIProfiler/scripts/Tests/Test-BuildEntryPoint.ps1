param()
$ErrorActionPreference = 'Stop'
$entry = Join-Path $PSScriptRoot '..\Build.ps1'
if (!(Test-Path -LiteralPath $entry -PathType Leaf)) {
    throw 'The full mixed-solution Build.ps1 entry point is missing.'
}
$fixture = Join-Path ([IO.Path]::GetTempPath()) ("WinUIProfiler Build Entry " + [guid]::NewGuid().ToString('N'))
[IO.Directory]::CreateDirectory($fixture) | Out-Null
$copy = Join-Path $fixture 'Build.ps1'
$dependency = Join-Path $fixture 'Build-App.ps1'
$shell = (Get-Process -Id $PID).Path
$passed = 0
try {
    Copy-Item -LiteralPath $entry -Destination $copy
    @'
param([string]$Configuration, [string]$Architecture, [switch]$Restore, [switch]$Solution)
if (!$Solution) { throw 'The native mixed solution was not selected.' }
if ($env:WINUI_PROFILER_TEST_BUILD_FAILURE -eq '1') { throw 'Deliberate underlying build failure.' }
[ordered]@{ Configuration=$Configuration; Architecture=$Architecture; Restore=[bool]$Restore; Solution=[bool]$Solution } | ConvertTo-Json -Compress
'@ | Set-Content -LiteralPath $dependency -Encoding UTF8

    function Invoke-Entry([string[]]$Arguments, [bool]$Fail = $false) {
        $start = [Diagnostics.ProcessStartInfo]::new()
        $start.FileName = $shell
        $start.Arguments = '-NoProfile -NonInteractive -File "' + $copy + '" ' + ($Arguments -join ' ')
        $start.UseShellExecute = $false
        $start.RedirectStandardOutput = $true
        $start.RedirectStandardError = $true
        $start.WorkingDirectory = [IO.Path]::GetPathRoot($fixture)
        $start.EnvironmentVariables['WINUI_PROFILER_TEST_BUILD_FAILURE'] = if ($Fail) { '1' } else { '0' }
        $process = [Diagnostics.Process]::Start($start)
        try {
            $stdout = $process.StandardOutput.ReadToEndAsync()
            $stderr = $process.StandardError.ReadToEndAsync()
            if (!$process.WaitForExit(30000)) {
                $process.Kill()
                throw 'Build entry point did not return within the bounded fixture deadline.'
            }
            [pscustomobject]@{ ExitCode=$process.ExitCode; Output=$stdout.Result; Error=$stderr.Result }
        }
        finally { $process.Dispose() }
    }

    foreach ($case in @(
        @{ Arguments=@(); Configuration='Release'; Architecture='x64'; Restore=$false },
        @{ Arguments=@('-Configuration','Debug','-Architecture','x64'); Configuration='Debug'; Architecture='x64'; Restore=$false },
        @{ Arguments=@('-Configuration','Release','-Architecture','ARM64','-Restore'); Configuration='Release'; Architecture='ARM64'; Restore=$true }
    )) {
        $result = Invoke-Entry -Arguments $case.Arguments
        if ($result.ExitCode -ne 0) { throw "Build entry failed: $($result.Error)" }
        $actual = $result.Output | ConvertFrom-Json
        if ($actual.Configuration -cne $case.Configuration -or $actual.Architecture -cne $case.Architecture -or
            $actual.Restore -ne $case.Restore -or !$actual.Solution) {
            throw 'Build entry did not preserve configuration/architecture/explicit restore and mandatory native solution selection.'
        }
        $passed++
    }
    foreach ($arguments in @(@('-Architecture','x86'), @('-Configuration','Checked'))) {
        $result = Invoke-Entry -Arguments $arguments
        if ($result.ExitCode -eq 0 -or $result.Output) { throw 'An invalid build request reached the underlying build.' }
        $passed++
    }
    $result = Invoke-Entry -Arguments @() -Fail $true
    if ($result.ExitCode -eq 0 -or $result.Error -notmatch 'Deliberate underlying build failure') {
        throw 'An underlying mixed build failure was reported as success.'
    }
    $passed++
    Write-Output "Passed $passed build entry-point behavioral cases."
}
finally {
    foreach ($file in @($copy, $dependency)) {
        if (Test-Path -LiteralPath $file -PathType Leaf) { Remove-Item -LiteralPath $file }
    }
    [IO.Directory]::Delete($fixture)
}
