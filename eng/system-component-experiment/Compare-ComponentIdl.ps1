[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$Component,

    [ValidateSet("latest", "windows10")]
    [string]$OsSource = "latest",

    [string]$LatestOsPath = "C:\os\src",
    [string]$Windows10OsPath = "C:\os1VB\src",
    [string]$OutputPath
)

$ErrorActionPreference = "Stop"

function Get-GitFiles
{
    param(
        [Parameter(Mandatory = $true)]
        [string]$Repository,

        [Parameter(Mandatory = $true)]
        [string[]]$Patterns
    )

    $files = foreach ($pattern in $Patterns)
    {
        @(& git -C $Repository ls-files $pattern)
        if ($LASTEXITCODE -ne 0)
        {
            throw "git ls-files failed in '$Repository' for '$pattern'."
        }
    }

    return @($files | Where-Object { $_ } | Sort-Object -Unique)
}

function Get-NormalizedIdl
{
    param(
        [Parameter(Mandatory = $true)]
        [string]$Repository,

        [Parameter(Mandatory = $true)]
        [string[]]$Files,

        [Parameter(Mandatory = $true)]
        [string]$SourceNamespace,

        [Parameter(Mandatory = $true)]
        [string]$TargetNamespace
    )

    $entries = foreach ($file in $Files)
    {
        $content = @(& git -C $Repository show "HEAD:$file")
        if ($LASTEXITCODE -ne 0)
        {
            throw "git show failed in '$Repository' for '$file'."
        }

        for ($lineIndex = 0; $lineIndex -lt $content.Count; $lineIndex++)
        {
            $line = $content[$lineIndex]
            $commentIndex = $line.IndexOf("//", [StringComparison]::Ordinal)
            if ($commentIndex -ge 0)
            {
                $line = $line.Substring(0, $commentIndex)
            }

            $line = $line.Replace(
                $SourceNamespace,
                $TargetNamespace,
                [StringComparison]::OrdinalIgnoreCase
            )
            $line = [regex]::Replace($line.Trim(), "\s+", " ")

            if ($line)
            {
                [pscustomobject][ordered]@{
                    text = $line
                    path = $file
                    line = $lineIndex + 1
                }
            }
        }
    }

    return @($entries)
}

$config = Get-Content (Join-Path $PSScriptRoot "components.json") -Raw | ConvertFrom-Json
$componentConfig = $config.components | Where-Object id -eq $Component

if ($null -eq $componentConfig)
{
    throw "Unknown component '$Component'."
}

if ($componentConfig.liftedNamespaces.Count -ne 1 -or
    $componentConfig.candidateSystemNamespaces.Count -ne 1)
{
    throw "Component '$Component' does not have a single namespace mapping."
}

$systemRepository = if ($OsSource -eq "latest") { $LatestOsPath } else { $Windows10OsPath }
$liftedFiles = Get-GitFiles $LatestOsPath $componentConfig.liftedIdlPatterns
$systemFiles = Get-GitFiles $systemRepository $componentConfig.systemIdlPatterns

if ($liftedFiles.Count -eq 0)
{
    throw "No lifted IDL files were found for '$Component' in '$LatestOsPath'."
}

if ($systemFiles.Count -eq 0)
{
    throw "No system IDL files were found for '$Component' in '$systemRepository'."
}

$liftedEntries = Get-NormalizedIdl `
    -Repository $LatestOsPath `
    -Files $liftedFiles `
    -SourceNamespace $componentConfig.liftedNamespaces[0] `
    -TargetNamespace $componentConfig.candidateSystemNamespaces[0]
$systemEntries = Get-NormalizedIdl `
    -Repository $systemRepository `
    -Files $systemFiles `
    -SourceNamespace $componentConfig.candidateSystemNamespaces[0] `
    -TargetNamespace $componentConfig.candidateSystemNamespaces[0]

$liftedByText = $liftedEntries | Group-Object text -AsHashTable -AsString
$systemByText = $systemEntries | Group-Object text -AsHashTable -AsString
$liftedLines = @($liftedByText.Keys | Sort-Object)
$systemLines = @($systemByText.Keys | Sort-Object)
$sharedLines = @($liftedLines | Where-Object { $systemByText.ContainsKey($_) })
$onlyLiftedLines = @($liftedLines | Where-Object { -not $systemByText.ContainsKey($_) })
$onlySystemLines = @($systemLines | Where-Object { -not $liftedByText.ContainsKey($_) })

$comparison = [ordered]@{
    generatedAtUtc = (Get-Date).ToUniversalTime().ToString("o")
    component = $Component
    osSource = $OsSource
    liftedRepository = $LatestOsPath
    systemRepository = $systemRepository
    namespaceMapping = [ordered]@{
        lifted = $componentConfig.liftedNamespaces[0]
        system = $componentConfig.candidateSystemNamespaces[0]
    }
    liftedFiles = $liftedFiles
    systemFiles = $systemFiles
    counts = [ordered]@{
        liftedNormalizedLines = $liftedLines.Count
        systemNormalizedLines = $systemLines.Count
        sharedLines = $sharedLines.Count
        onlyLiftedLines = $onlyLiftedLines.Count
        onlySystemLines = $onlySystemLines.Count
    }
    onlyLifted = @(
        foreach ($text in $onlyLiftedLines)
        {
            [ordered]@{
                text = $text
                sources = @($liftedByText[$text] | Select-Object path, line)
            }
        }
    )
    onlySystem = @(
        foreach ($text in $onlySystemLines)
        {
            [ordered]@{
                text = $text
                sources = @($systemByText[$text] | Select-Object path, line)
            }
        }
    )
}

if (-not $OutputPath)
{
    $fileName = "idl-diff-$Component-$OsSource.json"
    $OutputPath = Join-Path $PSScriptRoot "..\..\artifacts\system-component-experiment\$fileName"
}

$resolvedOutputPath = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath(
    $OutputPath
)
$outputDirectory = Split-Path $resolvedOutputPath -Parent
New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null
$comparison | ConvertTo-Json -Depth 12 | Set-Content -Path $resolvedOutputPath -Encoding utf8

Write-Output $resolvedOutputPath
