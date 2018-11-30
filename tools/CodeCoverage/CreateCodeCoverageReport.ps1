param(
[string]$MagellanInstallPath,
[string]$SqlConnectionString,
[string]$covSymPath,
[string]$covDataPath,
[string]$coverageSummaryOutputPath,
[string]$coverageReportOutputPath,
[switch]$isLocalRun
)

Import-Module $PSScriptRoot\CodeCoverage.psm1 -Force -DisableNameChecking
Set-MagellanInstallPath -Path $magellanInstallPath

$directoryFilterFunction =
{  
    param([string] $directory)

    return $directory.Contains("\dev\") `
        -and -not $directory.Contains("\dev\dll\") `
        -and -not $directory.Contains("\dev\inc\")
}

if ($SqlConnectionString -ne $null -and $SqlConnectionString.Length -gt 0)
{
    Create-CodeCoverageReportFromMagellanDB -sqlConnectionString $SqlConnectionString -coverageSummaryOutputPath "$coverageSummaryOutputPath" -coverageSummaryFileName 'coveragesummary.xml' -coverageReportOutputPath "$coverageReportOutputPath" -directoryFilterFunction $directoryFilterFunction -isLocalRun:$isLocalRun
}
else
{
    Create-CodeCoverageReportFromMagellanFiles -coverageSummaryOutputPath "$coverageSummaryOutputPath" -coverageSummaryFileName 'coveragesummary.xml' -coverageReportOutputPath "$coverageReportOutputPath" -coverageSymPath $covSymPath -coverageDataPath $covDataPath -directoryFilterFunction $directoryFilterFunction -isLocalRun:$isLocalRun
}

