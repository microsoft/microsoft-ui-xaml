[CmdletBinding()]
param(
    [Parameter(Mandatory)][string[]] $InputCsv,
    [Parameter(Mandatory)][string] $Commit,
    [Parameter(Mandatory)][string] $BuildId,
    [Parameter(Mandatory)][string] $AgentName,
    [Parameter(Mandatory)][string] $OutputPath
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function ConvertTo-InvariantDouble {
    param([Parameter(Mandatory)][string] $Text, [Parameter(Mandatory)][string] $Context)

    $value = 0.0
    if (-not [double]::TryParse(
        $Text,
        [Globalization.NumberStyles]::Float,
        [Globalization.CultureInfo]::InvariantCulture,
        [ref]$value)) {
        throw "$Context value '$Text' is not numeric."
    }
    if ([double]::IsNaN($value) -or [double]::IsInfinity($value)) {
        throw "$Context value '$Text' must be finite."
    }
    if ($value -lt 0) {
        throw "$Context value '$Text' must not be negative."
    }
    return $value
}

function Get-GroupingRun {
    param([Parameter(Mandatory)][string] $Grouping, $NumericRun, [bool] $HasNumericRun)

    $matches = [regex]::Matches($Grouping, '(?:^|/)Run:(\d+)(?=/|$)')
    if ($matches.Count -ne 1) {
        throw "Grouping '$Grouping' must contain exactly one Run:<n> segment."
    }
    $groupingRun = [int]$matches[0].Groups[1].Value

    if ($HasNumericRun) {
        $parsedRun = 0
        if (-not [int]::TryParse(
            [string]$NumericRun,
            [Globalization.NumberStyles]::None,
            [Globalization.CultureInfo]::InvariantCulture,
            [ref]$parsedRun)) {
            throw "Numeric run value '$NumericRun' must be a nonnegative integer."
        }
        if ($parsedRun -ne $groupingRun) {
            throw "Numeric run '$parsedRun' does not agree with grouping run '$groupingRun'."
        }
    }

    return $groupingRun
}

if ($Commit -notmatch '^[0-9a-fA-F]{40}$') {
    throw 'Commit must be a 40-character hexadecimal string.'
}
if ([string]::IsNullOrWhiteSpace($BuildId)) {
    throw 'BuildId must not be blank.'
}
if ([string]::IsNullOrWhiteSpace($AgentName)) {
    throw 'AgentName must not be blank.'
}

$requiredHeaders = @('scenario', 'metric', 'value', 'interval', 'grouping')
$allowedHeaders = @($requiredHeaders + 'run')

$parsedRows = @(
    $resolvedInputFiles = [Collections.Generic.Dictionary[string, IO.FileInfo]]::new(
        [StringComparer]::OrdinalIgnoreCase
    )
    foreach ($inputPath in $InputCsv) {
        $matches = @()
        if (Test-Path -LiteralPath $inputPath -PathType Leaf) {
            $matches = @(Get-Item -LiteralPath $inputPath)
        } elseif (Test-Path -LiteralPath $inputPath -PathType Container) {
            $matches = @(Get-ChildItem -LiteralPath $inputPath -Filter '*.raw.csv' -File -Recurse)
        } else {
            $matches = @(Get-ChildItem -Path $inputPath -File -ErrorAction SilentlyContinue)
        }
        foreach ($file in $matches) {
            $resolvedInputFiles[$file.FullName] = $file
        }
    }
    $inputFiles = @($resolvedInputFiles.Values | Sort-Object FullName)
    if ($inputFiles.Count -eq 0) {
        throw "No raw CSV files matched: $($InputCsv -join ', ')"
    }

    foreach ($file in $inputFiles) {
        $firstLine = Get-Content -LiteralPath $file.FullName -TotalCount 1
        if ([string]::IsNullOrWhiteSpace($firstLine)) {
            continue
        }
        $headers = @($firstLine.TrimStart([char]0xFEFF).Split(',') | ForEach-Object { $_.Trim() })
        if ($headers -cnotcontains 'scenario' -or
            $headers -cnotcontains 'metric' -or
            $headers -cnotcontains 'interval') {
            continue
        }

        $rows = @(Import-Csv -LiteralPath $file.FullName)
        $selectedRows = @(
            $rows | Where-Object {
                $_.scenario -cmatch '^PRPerf-' -and
                $_.metric -ceq 'CPU/WallTime' -and
                -not [string]::IsNullOrWhiteSpace($_.interval) -and
                $_.interval -cne 'Total'
            }
        )
        if ($selectedRows.Count -eq 0) {
            continue
        }

        foreach ($requiredHeader in $requiredHeaders) {
            if ($headers -cnotcontains $requiredHeader) {
                throw "Missing required CSV column '$requiredHeader' in '$($file.FullName)'."
            }
        }
        foreach ($header in $headers) {
            if ($allowedHeaders -cnotcontains $header) {
                throw "Unsupported CSV column '$header' in '$($file.FullName)'. Raw inputs with matching rows may contain only scenario, metric, value, interval, grouping, and optional run."
            }
            if (@($headers | Where-Object { $_ -ceq $header }).Count -ne 1) {
                throw "CSV column '$header' appears more than once in '$($file.FullName)'."
            }
        }
        $hasNumericRun = $headers -ccontains 'run'

        foreach ($row in $selectedRows) {
            if ([string]::IsNullOrWhiteSpace($row.grouping)) {
                throw 'Selected row grouping must not be blank.'
            }

            $numericRun = if ($hasNumericRun) { $row.run } else { $null }
            $run = Get-GroupingRun -Grouping $row.grouping -NumericRun $numericRun -HasNumericRun $hasNumericRun
            if ($run -lt 0 -or $run -gt 7) {
                throw "Run '$run' is outside the supported launch range 0-7."
            }
            $normalizedGrouping = [regex]::Replace($row.grouping, '(?:^|/)Run:\d+(?=/|$)', '').Trim('/')
            $value = ConvertTo-InvariantDouble -Text ([string]$row.value) -Context "Run $run"

            [pscustomobject]@{
                Scenario = [string]$row.scenario
                Interval = [string]$row.interval
                Grouping = $normalizedGrouping
                Run = $run
                Value = $value
            }
        }
    }
)
if ($parsedRows.Count -eq 0) {
    throw 'Raw CSV inputs have no PRPerf CPU/WallTime rows with a non-total measured interval.'
}

$scenarioResults = @()
foreach ($scenarioGroup in @($parsedRows | Group-Object Scenario | Sort-Object Name)) {
    $measurementGroups = @(
        $scenarioGroup.Group |
            Group-Object { "$($_.Interval)$([char]31)$($_.Grouping)" }
    )
    if ($measurementGroups.Count -ne 1) {
        throw "Scenario '$($scenarioGroup.Name)' has ambiguous non-total CPU/WallTime measurement groups ($($measurementGroups.Count)); exactly one interval/grouping is required."
    }

    $measurements = @($measurementGroups[0].Group)
    foreach ($run in 0..7) {
        $matches = @($measurements | Where-Object Run -eq $run)
        if ($run -eq 0 -and $matches.Count -gt 1) {
            throw "Scenario '$($scenarioGroup.Name)' has duplicate Run 0 rows."
        }
        if ($run -gt 0 -and $matches.Count -ne 1) {
            if ($matches.Count -eq 0) {
                throw "Scenario '$($scenarioGroup.Name)' is missing required Run $run."
            }
            throw "Scenario '$($scenarioGroup.Name)' has duplicate Run $run rows."
        }
    }

    [double[]]$samples = @(
        $measurements |
            Where-Object { $_.Run -ge 1 -and $_.Run -le 7 } |
            Sort-Object Run |
            ForEach-Object Value
    )
    if ($samples.Count -ne 7) {
        throw "Scenario '$($scenarioGroup.Name)' must produce exactly seven measured samples after discarding Run 0."
    }

    $scenarioResults += [pscustomobject]@{
        name = $scenarioGroup.Name
        metrics = @(
            [pscustomobject]@{
                name = 'CpuTimeMs'
                unit = 'ms'
                samples = $samples
            }
        )
    }
}

$result = [pscustomobject]@{
    schemaVersion = 1
    benchmarkConfigVersion = 'pr-smoke-v1'
    commit = $Commit.ToLowerInvariant()
    buildId = $BuildId
    machine = [pscustomobject]@{
        agentName = $AgentName
        osBuild = [string][Environment]::OSVersion.Version.Build
        architecture = 'amd64'
        flavor = 'fre'
    }
    scenarios = @($scenarioResults)
}

$outputDirectory = Split-Path -Parent $OutputPath
if (-not [string]::IsNullOrWhiteSpace($outputDirectory)) {
    New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null
}
$result | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath $OutputPath -Encoding UTF8
