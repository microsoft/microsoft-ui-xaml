[CmdletBinding()]
param(
    [string]$WinUIPath = "C:\microsoft-ui-xaml",
    [string]$LatestOsPath = "C:\os\src",
    [string]$Windows10OsPath = "C:\os1VB\src",
    [string]$OutputPath = (
        Join-Path $PSScriptRoot "..\..\artifacts\system-component-experiment\inventory.json"
    )
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

function Get-GitMatches
{
    param(
        [Parameter(Mandatory = $true)]
        [string]$Repository,

        [Parameter(Mandatory = $true)]
        [string]$Pattern,

        [Parameter(Mandatory = $true)]
        [string[]]$Pathspecs
    )

    $arguments = @("-C", $Repository, "grep", "-n", "-I", "-F", "-e", $Pattern, "--")
    $arguments += $Pathspecs
    $lines = @(& git @arguments 2>$null)

    if ($LASTEXITCODE -notin @(0, 1))
    {
        throw "git grep failed in '$Repository' for '$Pattern'."
    }

    $parsedMatches = foreach ($line in $lines)
    {
        $lineMatch = [regex]::Match($line, "^(.*?):(\d+):(.*)$")
        if ($lineMatch.Success)
        {
            [pscustomobject][ordered]@{
                path = $lineMatch.Groups[1].Value
                line = [int]$lineMatch.Groups[2].Value
                text = $lineMatch.Groups[3].Value.Trim()
            }
        }
    }

    return @($parsedMatches)
}

function Get-ReferenceGroup
{
    param(
        [Parameter(Mandatory = $true)]
        [string]$Repository,

        [Parameter(Mandatory = $true)]
        [string[]]$Namespaces,

        [Parameter(Mandatory = $true)]
        [string[]]$Pathspecs
    )

    $referenceMatches = foreach ($namespace in $Namespaces)
    {
        Get-GitMatches -Repository $Repository -Pattern $namespace -Pathspecs $Pathspecs
    }

    $orderedMatches = @(
        $referenceMatches |
            Sort-Object -Property path, line, text -Unique
    )

    return [ordered]@{
        count = $orderedMatches.Count
        files = @($orderedMatches.path | Sort-Object -Unique)
        matches = $orderedMatches
    }
}

$config = Get-Content (Join-Path $PSScriptRoot "components.json") -Raw | ConvertFrom-Json
$manifest = Get-Content (Join-Path $PSScriptRoot "manifest.json") -Raw | ConvertFrom-Json

$inventory = [ordered]@{
    generatedAtUtc = (Get-Date).ToUniversalTime().ToString("o")
    subtrial = $manifest.activeSubtrial
    stage = $manifest.activeStage
    repositories = [ordered]@{
        winui = $WinUIPath
        latestOs = $LatestOsPath
        windows10Os = $Windows10OsPath
    }
    components = @(
        foreach ($component in $config.components)
        {
            $idlReferences = Get-ReferenceGroup `
                -Repository $WinUIPath `
                -Namespaces $component.liftedNamespaces `
                -Pathspecs @("*.idl")
            $nativeReferences = Get-ReferenceGroup `
                -Repository $WinUIPath `
                -Namespaces $component.liftedNamespaces `
                -Pathspecs @("*.h", "*.hpp", "*.cpp")
            $buildReferences = Get-ReferenceGroup `
                -Repository $WinUIPath `
                -Namespaces $component.liftedNamespaces `
                -Pathspecs @("*.props", "*.targets", "*.vcxproj", "*.csproj")
            $managedReferences = Get-ReferenceGroup `
                -Repository $WinUIPath `
                -Namespaces $component.liftedNamespaces `
                -Pathspecs @("*.cs", "*.xaml")
            $latestLiftedIdlFiles = Get-GitFiles `
                -Repository $LatestOsPath `
                -Patterns $component.liftedIdlPatterns
            $latestSystemIdlFiles = Get-GitFiles `
                -Repository $LatestOsPath `
                -Patterns $component.systemIdlPatterns
            $windows10LiftedIdlFiles = Get-GitFiles `
                -Repository $Windows10OsPath `
                -Patterns $component.liftedIdlPatterns
            $windows10SystemIdlFiles = Get-GitFiles `
                -Repository $Windows10OsPath `
                -Patterns $component.systemIdlPatterns

            [ordered]@{
                id = $component.id
                liftedNamespaces = @($component.liftedNamespaces)
                candidateSystemNamespaces = @($component.candidateSystemNamespaces)
                winui = [ordered]@{
                    idl = $idlReferences
                    native = $nativeReferences
                    build = $buildReferences
                    managed = $managedReferences
                }
                latestOs = [ordered]@{
                    liftedIdlFiles = $latestLiftedIdlFiles
                    systemIdlFiles = $latestSystemIdlFiles
                }
                windows10Os = [ordered]@{
                    liftedIdlFiles = $windows10LiftedIdlFiles
                    systemIdlFiles = $windows10SystemIdlFiles
                }
            }
        }
    )
}

$resolvedOutputPath = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath(
    $OutputPath
)
$outputDirectory = Split-Path $resolvedOutputPath -Parent
New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null
$inventory | ConvertTo-Json -Depth 12 | Set-Content -Path $resolvedOutputPath -Encoding utf8

Write-Output $resolvedOutputPath
