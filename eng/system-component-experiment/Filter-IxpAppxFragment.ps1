[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$InputPath,

    [Parameter(Mandatory = $true)]
    [string]$OutputPath,

    [string]$ConfigurationPath
)

$ErrorActionPreference = "Stop"

if (-not $ConfigurationPath)
{
    $ConfigurationPath = Join-Path $PSScriptRoot "wuc-closure.json"
}

[xml]$manifest = Get-Content $InputPath
$configuration = Get-Content $ConfigurationPath -Raw | ConvertFrom-Json
$forbidden = @{}
foreach ($module in $configuration.forbiddenRuntimeModules)
{
    $forbidden[$module.ToLowerInvariant()] = $true
}

$namespaceManager = New-Object Xml.XmlNamespaceManager($manifest.NameTable)
$namespaceManager.AddNamespace(
    "appx",
    "http://schemas.microsoft.com/appx/manifest/foundation/windows10"
)

foreach ($server in @($manifest.SelectNodes("//appx:InProcessServer", $namespaceManager)))
{
    if ($forbidden.ContainsKey(([string]$server.Path).ToLowerInvariant()))
    {
        $extension = $server.ParentNode
        [void]$extension.RemoveChild($server)
        if (-not $extension.HasChildNodes)
        {
            [void]$extension.ParentNode.RemoveChild($extension)
        }
    }
}

$resolvedOutputPath = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath(
    $OutputPath
)
$outputDirectory = Split-Path $resolvedOutputPath -Parent
New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null
$manifest.Save($resolvedOutputPath)
