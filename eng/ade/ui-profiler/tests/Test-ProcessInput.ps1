# Process transport checks; no agents, application launches or captures.
#Requires -Version 7
$ErrorActionPreference = 'Stop'
Import-Module "$PSScriptRoot\..\pack\tools\ProfilerHarness.psm1" -Force -DisableNameChecking
$temp = Join-Path ([IO.Path]::GetTempPath()) ('ui-profiler-input-' + [Guid]::NewGuid().ToString('N'))
[void](New-Item -ItemType Directory -Path $temp)
$python = (Get-Command python -ErrorAction Stop).Path
try {
    $inputText = ("Contracts`r`n" * 10000) + [char]0x03A9 + [char]0x4E2D + "`nEnd of prompt."
    $script = @'
import hashlib, json, sys
sys.stdout.write("o" * 131072 + "\n")
sys.stdout.flush()
sys.stderr.write("e" * 131072 + "\n")
sys.stderr.flush()
data = sys.stdin.buffer.read()
print(json.dumps({"bytes": len(data), "sha256": hashlib.sha256(data).hexdigest(),
                  "argumentCount": len(sys.argv)}))
'@
    $log = Join-Path $temp 'long-input.log'
    $text = Invoke-LoggedProcess $python @('-u', '-c', $script) $temp $log 10 -InputText $inputText
    $result = ($text.TrimEnd() -split "`n")[-1] | ConvertFrom-Json
    $bytes = [Text.Encoding]::UTF8.GetBytes($inputText)
    $hash = [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($bytes)).ToLowerInvariant()
    if ($result.bytes -ne $bytes.Length -or $result.sha256 -ne $hash -or $result.argumentCount -ne 1) {
        throw 'Long/multiline/Unicode prompt changed or was passed through command-line arguments.'
    }
    if (!(Get-Content -LiteralPath $log -Raw).Contains(('e' * 131072))) {
        throw 'Concurrent stderr was not drained and logged.'
    }
    $empty = Invoke-LoggedProcess $python @('-c', 'import sys; print(len(sys.stdin.buffer.read()))') `
        $temp (Join-Path $temp 'no-input.log') 10
    if ($empty.Trim() -ne '0') { throw 'Default process input must still close at EOF.' }

    $pidPath = Join-Path $temp 'blocked.pid'
    $script = 'import os,sys,time; open(sys.argv[1],"w").write(str(os.getpid())); time.sleep(30)'
    $failure = $null
    try {
        [void](Invoke-LoggedProcess $python @('-c', $script, $pidPath) $temp `
            (Join-Path $temp 'blocked.log') 3 -InputText ('x' * 1048576))
    } catch { $failure = $_.Exception.Message }
    if ($failure -notlike 'Timed out writing process input:*') {
        throw "An unread input pipe did not respect the process deadline: $failure"
    }
    $childId = [int](Get-Content -LiteralPath $pidPath -Raw)
    if (Get-Process -Id $childId -ErrorAction SilentlyContinue) { throw 'Timed-out input left its child running.' }

    $failure = $null
    try {
        [void](Invoke-LoggedProcess $python @('-c', 'import sys; sys.stdin.read(); print("rejected",file=sys.stderr); sys.exit(7)') `
            $temp (Join-Path $temp 'failure.log') 10 -InputText 'prompt')
    } catch { $failure = $_.Exception.Message }
    if ($failure -notlike 'Exit 7:*' -or !(Get-Content -LiteralPath (Join-Path $temp 'failure.log') -Raw).Contains('rejected')) {
        throw 'A nonzero process exit or its diagnostics were hidden.'
    }
    Write-Output '4 process-input checks passed: long UTF-8 prompt/concurrent output, legacy EOF, input timeout, failure diagnostics.'
} finally {
    Remove-Item -LiteralPath $temp -Recurse -Force
}
