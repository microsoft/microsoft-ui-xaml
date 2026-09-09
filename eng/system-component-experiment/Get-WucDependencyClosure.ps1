[CmdletBinding()]
param(
    [string]$WinUIPath = "C:\microsoft-ui-xaml",
    [string]$LatestOsPath = "C:\os\src",
    [string]$Windows10OsPath = "C:\os1VB\src",
    [string]$OutputPath = (
        Join-Path $PSScriptRoot "..\..\artifacts\system-component-experiment\wuc-closure.json"
    )
)

$ErrorActionPreference = "Stop"

function Test-GitFile
{
    param(
        [Parameter(Mandatory = $true)]
        [string]$Repository,

        [Parameter(Mandatory = $true)]
        [string]$Path
    )

    & git -C $Repository cat-file -e "HEAD:$Path" 2>$null
    return $LASTEXITCODE -eq 0
}

function Get-GitFileText
{
    param(
        [Parameter(Mandatory = $true)]
        [string]$Repository,

        [Parameter(Mandatory = $true)]
        [string]$Path
    )

    $content = @(& git -C $Repository show "HEAD:$Path")
    if ($LASTEXITCODE -ne 0)
    {
        throw "git show failed in '$Repository' for '$Path'."
    }

    return $content
}

function Get-IdlDependencies
{
    param(
        [Parameter(Mandatory = $true)]
        [string]$Repository,

        [Parameter(Mandatory = $true)]
        [string]$Path
    )

    $dependencies = foreach ($line in (Get-GitFileText -Repository $Repository -Path $Path))
    {
        $dependencyMatch = [regex]::Match(
            $line,
            '^\s*(?:import\s+|#include\s+)[<"]([^>"]+)[>"]'
        )
        if ($dependencyMatch.Success)
        {
            $dependencyMatch.Groups[1].Value
        }
    }

    return @($dependencies | Sort-Object -Unique)
}

function Get-WinUIEvidence
{
    param(
        [Parameter(Mandatory = $true)]
        [string]$Repository,

        [Parameter(Mandatory = $true)]
        [string[]]$Patterns
    )

    $evidence = foreach ($pattern in $Patterns)
    {
        $lines = @(
            & git -C $Repository grep -n -I -F -e $pattern -- `
                "dxaml/*.idl" "dxaml/*.h" "dxaml/*.hpp" "dxaml/*.cpp" `
                "eng/*.props" "eng/*.targets" "eng/*.vcxproj" 2>$null
        )
        if ($LASTEXITCODE -notin @(0, 1))
        {
            throw "git grep failed in '$Repository' for '$pattern'."
        }

        [ordered]@{
            pattern = $pattern
            count = $lines.Count
            samples = @($lines | Select-Object -First 10)
        }
    }

    return @($evidence)
}

function Get-DeclaredIdlSymbols
{
    param(
        [Parameter(Mandatory = $true)]
        [string]$Repository,

        [Parameter(Mandatory = $true)]
        [string[]]$Paths
    )

    $symbols = foreach ($path in $Paths)
    {
        foreach ($line in (Get-GitFileText -Repository $Repository -Path $path))
        {
            $matches = [regex]::Matches(
                $line,
                '(?:(?:DUAL|LIFTED|COMPOSITION)_(?:UNSEALED_)?(?:RUNTIME_CLASS|INTERFACE(?:_NAME|_UUID)?|STATIC_NAME|CONSTRUCTOR_NAME|ENUM|FLAG_ENUM)|(?:runtimeclass|interface|enum))\s*\(\s*([A-Za-z_][A-Za-z0-9_]*)|(?:runtimeclass|interface|enum)\s+([A-Za-z_][A-Za-z0-9_]*)'
            )
            foreach ($symbolMatch in $matches)
            {
                $symbol = if ($symbolMatch.Groups[1].Success)
                {
                    $symbolMatch.Groups[1].Value
                }
                else
                {
                    $symbolMatch.Groups[2].Value
                }

                if ($symbol)
                {
                    [pscustomobject][ordered]@{
                        name = $symbol
                        path = $path
                    }
                }
            }
        }
    }

    return @($symbols | Sort-Object -Property name, path -Unique)
}

function Get-WinUIIxpSymbols
{
    param(
        [Parameter(Mandatory = $true)]
        [string]$Repository
    )

    $lines = @(
        & git -C $Repository grep -n -I -E 'ixp::[A-Za-z_][A-Za-z0-9_]*' -- `
            "dxaml/*.h" "dxaml/*.hpp" "dxaml/*.cpp" 2>$null
    )
    if ($LASTEXITCODE -notin @(0, 1))
    {
        throw "git grep failed in '$Repository' for ixp symbols."
    }

    $uses = foreach ($line in $lines)
    {
        $lineMatch = [regex]::Match($line, '^(.*?):(\d+):(.*)$')
        if (-not $lineMatch.Success)
        {
            continue
        }

        foreach ($symbolMatch in [regex]::Matches(
            $lineMatch.Groups[3].Value,
            'ixp::([A-Za-z_][A-Za-z0-9_]*)'
        ))
        {
            [pscustomobject][ordered]@{
                name = $symbolMatch.Groups[1].Value
                path = $lineMatch.Groups[1].Value
                line = [int]$lineMatch.Groups[2].Value
            }
        }
    }

    return @($uses | Sort-Object -Property name, path, line -Unique)
}

$configuration = Get-Content (Join-Path $PSScriptRoot "wuc-closure.json") -Raw |
    ConvertFrom-Json
$manifest = Get-Content (Join-Path $PSScriptRoot "manifest.json") -Raw |
    ConvertFrom-Json

$nodes = foreach ($node in $configuration.nodes)
{
    $latestLiftedAvailable = if ($node.liftedIdl)
    {
        Test-GitFile -Repository $LatestOsPath -Path $node.liftedIdl
    }
    else
    {
        $null
    }
    $latestSystemAvailable = if ($node.systemIdl)
    {
        Test-GitFile -Repository $LatestOsPath -Path $node.systemIdl
    }
    else
    {
        $false
    }
    $windows10SystemAvailable = if ($node.systemIdl)
    {
        Test-GitFile -Repository $Windows10OsPath -Path $node.systemIdl
    }
    else
    {
        $false
    }

    if ($null -ne $node.latestSystemAvailable -and
        $latestSystemAvailable -ne $node.latestSystemAvailable)
    {
        throw "Latest system availability drifted for '$($node.id)'."
    }
    if ($null -ne $node.windows10SystemAvailable -and
        $windows10SystemAvailable -ne $node.windows10SystemAvailable)
    {
        throw "Windows 10 system availability drifted for '$($node.id)'."
    }
    if ($node.liftedIdl -and -not $latestLiftedAvailable)
    {
        throw "Lifted IDL is missing for '$($node.id)': $($node.liftedIdl)"
    }

    [ordered]@{
        id = $node.id
        kind = $node.kind
        liftedIdl = $node.liftedIdl
        systemIdl = $node.systemIdl
        latestLiftedAvailable = $latestLiftedAvailable
        latestSystemAvailable = $latestSystemAvailable
        windows10SystemAvailable = $windows10SystemAvailable
        liftedDependencies = if ($node.liftedIdl)
        {
            Get-IdlDependencies -Repository $LatestOsPath -Path $node.liftedIdl
        }
        else
        {
            @()
        }
        latestSystemDependencies = if ($node.systemIdl -and $latestSystemAvailable)
        {
            Get-IdlDependencies -Repository $LatestOsPath -Path $node.systemIdl
        }
        else
        {
            @()
        }
        windows10SystemDependencies = if ($node.systemIdl -and $windows10SystemAvailable)
        {
            Get-IdlDependencies -Repository $Windows10OsPath -Path $node.systemIdl
        }
        else
        {
            @()
        }
        winuiEvidence = if ($node.winuiEvidence.Count -gt 0)
        {
            Get-WinUIEvidence `
                -Repository $WinUIPath `
                -Patterns @($node.winuiEvidence)
        }
        else
        {
            @()
        }
        decision = $node.decision
    }
}

$compositionNodes = @(
    $configuration.nodes |
        Where-Object {
            $_.liftedIdl -and
            $_.liftedIdl -like "*/Microsoft.UI.Composition*.idl"
        }
)
$liftedCompositionFiles = @($compositionNodes.liftedIdl | Sort-Object -Unique)
$latestSystemCompositionFiles = @(
    $compositionNodes.systemIdl |
        Where-Object { $_ } |
        Sort-Object -Unique
)
$windows10SystemCompositionFiles = @(
    $latestSystemCompositionFiles |
        Where-Object { Test-GitFile -Repository $Windows10OsPath -Path $_ }
)

$liftedCompositionSymbols = Get-DeclaredIdlSymbols `
    -Repository $LatestOsPath `
    -Paths @($liftedCompositionFiles + $latestSystemCompositionFiles | Sort-Object -Unique)
$latestSystemCompositionSymbols = Get-DeclaredIdlSymbols `
    -Repository $LatestOsPath `
    -Paths $latestSystemCompositionFiles
$windows10SystemCompositionSymbols = Get-DeclaredIdlSymbols `
    -Repository $Windows10OsPath `
    -Paths $windows10SystemCompositionFiles
$winuiIxpSymbols = Get-WinUIIxpSymbols -Repository $WinUIPath

$liftedSymbolNames = @($liftedCompositionSymbols.name | Sort-Object -Unique)
$latestSystemSymbolNames = @($latestSystemCompositionSymbols.name | Sort-Object -Unique)
$windows10SystemSymbolNames = @($windows10SystemCompositionSymbols.name | Sort-Object -Unique)
$usedCompositionSymbolNames = @(
    $winuiIxpSymbols.name |
        Where-Object {
            $_ -in $liftedSymbolNames -and
            $_ -notin $configuration.externalSymbolsDeclaredByCompositionIdl
        } |
        Sort-Object -Unique
)

$usedCompositionSymbols = foreach ($symbolName in $usedCompositionSymbolNames)
{
    [ordered]@{
        name = $symbolName
        liftedSources = @(
            $liftedCompositionSymbols |
                Where-Object name -eq $symbolName |
                Select-Object -ExpandProperty path -Unique
        )
        latestSystemAvailable = $symbolName -in $latestSystemSymbolNames
        windows10SystemAvailable = $symbolName -in $windows10SystemSymbolNames
        winuiUseCount = @($winuiIxpSymbols | Where-Object name -eq $symbolName).Count
        winuiUseSamples = @(
            $winuiIxpSymbols |
                Where-Object name -eq $symbolName |
                Select-Object -First 10
        )
    }
}

$result = [ordered]@{
    generatedAtUtc = (Get-Date).ToUniversalTime().ToString("o")
    subtrial = $manifest.activeSubtrial
    stage = "A0"
    repositories = [ordered]@{
        winuiCommit = (& git -C $WinUIPath rev-parse HEAD)
        latestOsCommit = (& git -C $LatestOsPath rev-parse HEAD)
        windows10OsCommit = (& git -C $Windows10OsPath rev-parse HEAD)
    }
    roots = @($configuration.roots)
    nodes = @($nodes)
    edges = @($configuration.edges)
    compositionSymbolClosure = [ordered]@{
        usedSymbolCount = @($usedCompositionSymbols).Count
        latestSystemGapCount = @(
            $usedCompositionSymbols |
                Where-Object { -not $_.latestSystemAvailable }
        ).Count
        windows10SystemGapCount = @(
            $usedCompositionSymbols |
                Where-Object { -not $_.windows10SystemAvailable }
        ).Count
        symbols = @($usedCompositionSymbols)
    }
}

$resolvedOutputPath = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath(
    $OutputPath
)
$outputDirectory = Split-Path $resolvedOutputPath -Parent
New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null
$result | ConvertTo-Json -Depth 12 | Set-Content -Path $resolvedOutputPath -Encoding utf8

Write-Output $resolvedOutputPath
