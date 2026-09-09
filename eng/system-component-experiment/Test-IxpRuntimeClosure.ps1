[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"

$outputPath = Join-Path $env:TEMP "winui-ixp-runtime-closure-test.json"

try
{
    & (Join-Path $PSScriptRoot "Get-IxpRuntimeClosure.ps1") `
        -OutputPath $outputPath |
        Out-Null

    $closure = Get-Content $outputPath -Raw | ConvertFrom-Json

    $requiredEdges = @(
        @{
            Module = "Microsoft.DirectManipulation.dll"
            Dependency = "dcompi.dll"
        },
        @{
            Module = "Microsoft.UI.Input.dll"
            Dependency = "CoreMessagingXP.dll"
        },
        @{
            Module = "Microsoft.UI.Input.dll"
            Dependency = "Microsoft.UI.Composition.OSSupport.dll"
        },
        @{
            Module = "Microsoft.UI.Windowing.Core.dll"
            Dependency = "CoreMessagingXP.dll"
        },
        @{
            Module = "Microsoft.UI.dll"
            Dependency = "Microsoft.UI.Windowing.Core.dll"
        }
    )

    foreach ($edge in $requiredEdges)
    {
        $module = $closure.modules | Where-Object name -eq $edge.Module
        if ($null -eq $module -or $edge.Dependency -notin $module.dependencies)
        {
            throw "Expected runtime edge '$($edge.Module)' -> '$($edge.Dependency)' is missing."
        }
    }

    if ($closure.retainedModulesImportingForbiddenModules.Count -lt 4)
    {
        throw "The strict system-component runtime closure is unexpectedly small."
    }

    $commonUi = $closure.retainedModulesReachingForbiddenModules |
        Where-Object name -eq "Microsoft.UI.dll"
    if ($null -eq $commonUi -or
        "CoreMessagingXP.dll" -notin $commonUi.forbiddenTransitiveDependencies)
    {
        throw "The transitive Microsoft.UI.dll to CoreMessagingXP.dll edge was not detected."
    }

    Write-Output "IXP runtime closure tests passed."
}
finally
{
    Remove-Item $outputPath -ErrorAction SilentlyContinue
}
