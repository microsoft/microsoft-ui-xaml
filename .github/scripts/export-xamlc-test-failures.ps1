param(
    [Parameter(Mandatory)] [xml]$Trx,
    [Parameter(Mandatory)] [string]$ResultsDirectory
)

$ErrorActionPreference = 'Stop'
$failureRoot = Join-Path $ResultsDirectory 'failures'
New-Item -ItemType Directory -Force -Path $failureRoot | Out-Null
$manifest = @()

foreach ($result in $Trx.SelectNodes("//*[local-name()='UnitTestResult' and @outcome='Failed']")) {
    $testName = $result.GetAttribute('testName')
    $messageNode = $result.SelectSingleNode(".//*[local-name()='ErrorInfo']/*[local-name()='Message']")
    $message = if ($messageNode) { $messageNode.InnerText } else { '' }
    $failure = [ordered]@{ TestName = $testName; Message = $message }
    $comparison = [regex]::Match($message, 'bcomp "([^"]+)" "([^"]+)"')
    if ($comparison.Success) {
        # Preserve both complete generated-code directories for review. Never refresh masters
        # here: a text mismatch can be an intended change or an actual product regression.
        $testDirectory = Join-Path $failureRoot ($testName -replace '[^a-zA-Z0-9_.-]', '_')
        foreach ($side in @('actual', 'expected')) {
            $sourceFile = $comparison.Groups[$(if ($side -eq 'actual') { 1 } else { 2 })].Value
            $sourceDirectory = Split-Path $sourceFile
            if (-not (Test-Path $sourceDirectory -PathType Container)) {
                throw "Codegen failure directory is missing: $sourceDirectory"
            }
            $destinationDirectory = Join-Path $testDirectory $side
            New-Item -ItemType Directory -Force -Path $destinationDirectory | Out-Null
            foreach ($file in Get-ChildItem -LiteralPath $sourceDirectory -Filter '*.g.*' -File -Recurse) {
                $relativePath = $file.FullName.Substring($sourceDirectory.Length).TrimStart('\')
                $destination = Join-Path $destinationDirectory $relativePath
                New-Item -ItemType Directory -Force -Path (Split-Path $destination) | Out-Null
                Copy-Item -LiteralPath $file.FullName -Destination $destination -Force
            }
            $failure[$side] = $sourceDirectory
        }

        $previousPreference = $ErrorActionPreference
        try {
            $ErrorActionPreference = 'Continue'
            & git -C $testDirectory diff --no-index --no-color -- expected actual 2>&1 |
                Set-Content -LiteralPath (Join-Path $testDirectory 'codegen.diff')
            $diffExit = $LASTEXITCODE
        }
        finally {
            $ErrorActionPreference = $previousPreference
        }
        # git diff returns 1 for differences; this report does not change the VSTest outcome.
        if ($diffExit -gt 1) {
            throw "Exporting codegen differences for '$testName' failed with exit code $diffExit."
        }
        $failure.Diff = "$($testDirectory | Split-Path -Leaf)/codegen.diff"
    }
    $manifest += [pscustomobject]$failure
}

ConvertTo-Json -InputObject $manifest -Depth 5 | Set-Content -LiteralPath (Join-Path $failureRoot 'manifest.json') -Encoding UTF8
Write-Host "Saved $($manifest.Count) unit-test failure details under '$failureRoot'."
