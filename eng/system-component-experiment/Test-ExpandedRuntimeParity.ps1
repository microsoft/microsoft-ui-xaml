[CmdletBinding()]
param(
    [string]$LatestOsPath = "C:\os\src",
    [string]$Windows10OsPath = "C:\os1VB\src"
)

$ErrorActionPreference = "Stop"

function Test-GitText
{
    param(
        [Parameter(Mandatory = $true)]
        [string]$Repository,

        [Parameter(Mandatory = $true)]
        [string]$Path,

        [Parameter(Mandatory = $true)]
        [string]$Text
    )

    $content = @(& git -C $Repository show "HEAD:$Path" 2>$null)
    if ($LASTEXITCODE -ne 0)
    {
        throw "git show failed in '$Repository' for '$Path'."
    }
    if (-not ($content -join "`n").Contains($Text, [StringComparison]::Ordinal))
    {
        throw "Expected '$Text' in '$Repository\$Path'."
    }
}

$configuration = Get-Content (
    Join-Path $PSScriptRoot "expanded-runtime-closure.json"
) -Raw | ConvertFrom-Json

if ($configuration.families.Count -ne 6)
{
    throw "The expanded runtime family classification is incomplete."
}

$expectedModuleActions = [ordered]@{
    "Microsoft.DirectManipulation.dll" = "replace"
    "Microsoft.UI.Input.dll" = "rebuild"
    "Microsoft.InputStateManager.dll" = "rebuild"
    "Microsoft.UI.Windowing.Core.dll" = "rebuild"
    "Microsoft.UI.Windowing.dll" = "retain-after-dependency-rebuild"
    "Microsoft.Graphics.Display.dll" = "retain-after-dependency-rebuild"
    "Microsoft.UI.dll" = "retain-after-dependency-rebuild"
}

if ($configuration.runtimeModules.Count -ne $expectedModuleActions.Count)
{
    throw "The seven-module native runtime topology is incomplete."
}

foreach ($entry in $expectedModuleActions.GetEnumerator())
{
    $module = $configuration.runtimeModules | Where-Object name -eq $entry.Key
    if ($null -eq $module -or $module.action -ne $entry.Value)
    {
        throw "Expected runtime action '$($entry.Value)' for '$($entry.Key)'."
    }
}

$temporaryFiles = @()
try
{
    foreach ($component in @("input", "content", "windowing"))
    {
        foreach ($osSource in @("latest", "windows10"))
        {
            $temporaryFile = Join-Path $env:TEMP (
                "winui-expanded-parity-{0}-{1}.json" -f $component, $osSource
            )
            $temporaryFiles += $temporaryFile

            & (Join-Path $PSScriptRoot "Compare-ComponentIdl.ps1") `
                -Component $component `
                -OsSource $osSource `
                -LatestOsPath $LatestOsPath `
                -Windows10OsPath $Windows10OsPath `
                -OutputPath $temporaryFile |
                Out-Null

            $comparison = Get-Content $temporaryFile -Raw | ConvertFrom-Json
            if ($comparison.counts.onlyLiftedLines -lt 100)
            {
                throw "The non-equivalent '$component' result is unexpectedly small."
            }
        }
    }

    Test-GitText `
        -Repository $LatestOsPath `
        -Path "onecoreuap/windows/moderncore/Inputv2/Inc/SystemAndLiftedTypes.h" `
        -Text "using namespace Windows::UI::Composition"
    Test-GitText `
        -Repository $LatestOsPath `
        -Path "onecoreuap/windows/moderncore/Inputv2/Inc/SystemAndLiftedTypes.h" `
        -Text "using namespace Microsoft::UI::Composition"
    Test-GitText `
        -Repository $LatestOsPath `
        -Path "onecoreuap/windows/dwm/dmanip/dll/sources" `
        -Text "TARGETNAME = directmanipulation"
    Test-GitText `
        -Repository $LatestOsPath `
        -Path "onecoreuap/windows/dwm/dmanip/lifted/dll/sources" `
        -Text "TARGETNAME = Microsoft.DirectManipulation"

    Write-Output "Expanded runtime parity tests passed."
}
finally
{
    $temporaryFiles | ForEach-Object {
        Remove-Item $_ -ErrorAction SilentlyContinue
    }
}
