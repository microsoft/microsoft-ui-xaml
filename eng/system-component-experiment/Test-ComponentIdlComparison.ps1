[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"

$testCases = @(
    @{ Component = "composition"; OsSource = "latest" },
    @{ Component = "composition"; OsSource = "windows10" },
    @{ Component = "dispatching"; OsSource = "latest" },
    @{ Component = "dispatching"; OsSource = "windows10" }
)

$temporaryFiles = @()

try
{
    foreach ($testCase in $testCases)
    {
        $temporaryFile = Join-Path $env:TEMP (
            "winui-idl-diff-{0}-{1}.json" -f $testCase.Component, $testCase.OsSource
        )
        $temporaryFiles += $temporaryFile

        & (Join-Path $PSScriptRoot "Compare-ComponentIdl.ps1") `
            -Component $testCase.Component `
            -OsSource $testCase.OsSource `
            -OutputPath $temporaryFile |
            Out-Null

        $comparison = Get-Content $temporaryFile -Raw | ConvertFrom-Json

        if ($comparison.counts.sharedLines -eq 0)
        {
            throw "No shared IDL was found for $($testCase.Component) $($testCase.OsSource)."
        }

        if ($comparison.counts.onlyLiftedLines -eq 0)
        {
            throw "No lifted-only IDL was found for $($testCase.Component) $($testCase.OsSource)."
        }

        if ($comparison.liftedFiles.Count -eq 0 -or $comparison.systemFiles.Count -eq 0)
        {
            throw "Comparison inputs are missing for $($testCase.Component) $($testCase.OsSource)."
        }
    }

    Write-Output "Component IDL comparison tests passed."
}
finally
{
    $temporaryFiles | ForEach-Object {
        Remove-Item $_ -ErrorAction SilentlyContinue
    }
}
