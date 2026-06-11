Param(
    [string]$ReportCsvFile = "UnreliableTestReport.csv"
)

$isHeader = $true
$isOutputtingUrls = $false

Get-Content $ReportCsvFile | ForEach-Object {
    if ($isHeader)
    {
        $isHeader = $false
    }
    else
    {
        $splitStrings = $_ -split ","

        if ($splitStrings[0].Length -gt 0)
        {
            if ($isOutputtingUrls)
            {
                Write-Host ""
            }

            Write-Host "$($splitStrings[0]) - Failure rate: $($splitStrings[1])"
            $isOutputtingUrls = $false
        }
        else
        {
            if (-not $isOutputtingUrls)
            {
                Write-Host ""
                Write-Host "URLs of builds with failures:"
                Write-Host ""
            }

            Write-Host "    $($splitStrings[2])"

            $isOutputtingUrls = $true
        }
    }
}