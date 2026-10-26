<#
.SYNOPSIS
Builds local WinUI and publishes the unpackaged, self-contained non-rectangular window prototype.
.EXAMPLE
.\BuildNonRectangularWindow.ps1
#>
[CmdletBinding()]
param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Release"
)

$ErrorActionPreference = "Stop"
$flavor = if ($Configuration -eq "Release") { "amd64fre" } else { "amd64chk" }
$version = "3.0.0-nonrect.$([DateTime]::UtcNow.ToString('yyyyMMddHHmmss')).$([Guid]::NewGuid().ToString('N').Substring(0, 8))"

function Invoke-RepositoryCommand {
    param([string[]]$BuildArguments)
    & powershell.exe -NoProfile -ExecutionPolicy Bypass -File "$PSScriptRoot\initrun.ps1" -Flavor $flavor @BuildArguments
    if ($LASTEXITCODE -ne 0) {
        throw "Failed ($LASTEXITCODE): $($BuildArguments -join ' ')"
    }
}

Push-Location $PSScriptRoot
try {
    # Match build.cmd product's stages without requiring branch-specific PGO training data.
    foreach ($project in @(
        "XamlCompilerPrerequisites.sln",
        "Microsoft.UI.Xaml-Product.sln",
        "controls\dev\dll\Microsoft.UI.Xaml.Controls.vcxproj"
    )) {
        Invoke-RepositoryCommand -BuildArguments @("msb", "/q", $project, "/restore", "/p:PGOBuildMode=Off")
    }
    Invoke-RepositoryCommand -BuildArguments @("pack.component.cmd", "/version", $version)
    Invoke-RepositoryCommand -BuildArguments @(
        "msb", "/q", "Samples\NonRectangularWindow\NonRectangularWindow.csproj",
        "/restore", "/t:Publish", "/p:Platform=x64", "/p:Configuration=$Configuration",
        "/p:WinUIVersion=$version"
    )
    Write-Host "Published: $PSScriptRoot\BuildOutput\bin\$flavor\Samples\NonRectangularWindow"
    Write-Host "Copy the entire folder to another x64 machine and run NonRectangularWindow.exe."
} finally {
    Pop-Location
}
