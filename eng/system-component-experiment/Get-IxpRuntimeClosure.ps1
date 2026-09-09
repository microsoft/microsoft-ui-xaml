[CmdletBinding()]
param(
    [string]$WinUIPath = "C:\microsoft-ui-xaml",
    [string]$IxpPackagePath,
    [string]$DumpbinPath,
    [string]$OutputPath = (
        Join-Path $PSScriptRoot "..\..\artifacts\system-component-experiment\ixp-runtime-closure.json"
    )
)

$ErrorActionPreference = "Stop"

if (-not $IxpPackagePath)
{
    $IxpPackagePath = Get-ChildItem `
        -Path (Join-Path $WinUIPath "packages") `
        -Directory `
        -Filter "Microsoft.WindowsAppSDK.InteractiveExperiences.*" |
        Where-Object {
            Test-Path (Join-Path $_.FullName "runtimes-framework\win-x64\native")
        } |
        Sort-Object LastWriteTimeUtc -Descending |
        Select-Object -First 1 -ExpandProperty FullName
}
if (-not $IxpPackagePath)
{
    throw "No restored InteractiveExperiences package with an x64 framework runtime was found."
}

if (-not $DumpbinPath)
{
    $vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswhere)
    {
        $DumpbinPath = @(
            & $vswhere -latest -products * -find `
                "VC\Tools\MSVC\**\bin\Hostx64\x64\dumpbin.exe"
        ) | Select-Object -First 1
    }
}
if (-not $DumpbinPath -or -not (Test-Path $DumpbinPath))
{
    throw "dumpbin.exe was not found. Pass -DumpbinPath explicitly."
}

$configuration = Get-Content (Join-Path $PSScriptRoot "wuc-closure.json") -Raw |
    ConvertFrom-Json
$runtimePath = Join-Path $IxpPackagePath "runtimes-framework\win-x64\native"
$manifestPath = Join-Path $IxpPackagePath "runtimes-framework\package.appxfragment"
$runtimeDlls = @(Get-ChildItem $runtimePath -Filter "*.dll" | Sort-Object Name)
$runtimeDllNames = @($runtimeDlls.Name)

[xml]$appxFragment = Get-Content $manifestPath
$namespaceManager = [Xml.XmlNamespaceManager]::new($appxFragment.NameTable)
$namespaceManager.AddNamespace(
    "appx",
    "http://schemas.microsoft.com/appx/manifest/foundation/windows10"
)
$activationServers = @(
    foreach ($server in $appxFragment.SelectNodes("//appx:InProcessServer", $namespaceManager))
    {
        [pscustomobject][ordered]@{
            module = [string]$server.Path
            classes = @($server.ActivatableClass.ActivatableClassId)
        }
    }
)

$modules = foreach ($dll in $runtimeDlls)
{
    $dependencies = @(
        & $DumpbinPath /nologo /dependents $dll.FullName |
            Where-Object { $_ -match '^\s+[A-Za-z0-9_.-]+\.dll\s*$' } |
            ForEach-Object { $_.Trim() } |
            Where-Object { $_ -in $runtimeDllNames } |
            Sort-Object -Unique
    )
    $activation = $activationServers | Where-Object module -eq $dll.Name

    [pscustomobject][ordered]@{
        name = $dll.Name
        forbidden = $dll.Name -in $configuration.forbiddenRuntimeModules
        dependencies = $dependencies
        forbiddenDependencies = @(
            $dependencies |
                Where-Object { $_ -in $configuration.forbiddenRuntimeModules }
        )
        activatableClasses = @($activation.classes)
    }
}

$moduleByName = @{}
foreach ($module in $modules)
{
    $moduleByName[$module.name] = $module
}

foreach ($module in $modules)
{
    $visited = [Collections.Generic.HashSet[string]]::new(
        [StringComparer]::OrdinalIgnoreCase
    )
    $pending = [Collections.Generic.Queue[string]]::new()
    foreach ($dependency in $module.dependencies)
    {
        $pending.Enqueue($dependency)
    }

    while ($pending.Count -gt 0)
    {
        $dependency = $pending.Dequeue()
        if ($visited.Add($dependency) -and $moduleByName.ContainsKey($dependency))
        {
            foreach ($transitiveDependency in $moduleByName[$dependency].dependencies)
            {
                $pending.Enqueue($transitiveDependency)
            }
        }
    }

    $module | Add-Member -NotePropertyName transitiveDependencies -NotePropertyValue @(
        $visited | Sort-Object
    )
    $module | Add-Member -NotePropertyName forbiddenTransitiveDependencies -NotePropertyValue @(
        $visited |
            Where-Object { $_ -in $configuration.forbiddenRuntimeModules } |
            Sort-Object
    )
}

$result = [ordered]@{
    generatedAtUtc = (Get-Date).ToUniversalTime().ToString("o")
    packagePath = $IxpPackagePath
    packageVersion = (Split-Path $IxpPackagePath -Leaf).Replace(
        "Microsoft.WindowsAppSDK.InteractiveExperiences.",
        ""
    )
    architecture = "x64"
    forbiddenRuntimeModules = @($configuration.forbiddenRuntimeModules)
    modules = @($modules)
    retainedModulesImportingForbiddenModules = @(
        $modules |
            Where-Object {
                -not $_.forbidden -and
                $_.forbiddenDependencies.Count -gt 0
            } |
            Select-Object name, forbiddenDependencies
    )
    retainedModulesReachingForbiddenModules = @(
        $modules |
            Where-Object {
                -not $_.forbidden -and
                $_.forbiddenTransitiveDependencies.Count -gt 0
            } |
            Select-Object name, forbiddenTransitiveDependencies
    )
}

$resolvedOutputPath = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath(
    $OutputPath
)
$outputDirectory = Split-Path $resolvedOutputPath -Parent
New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null
$result | ConvertTo-Json -Depth 10 | Set-Content -Path $resolvedOutputPath -Encoding utf8

Write-Output $resolvedOutputPath
