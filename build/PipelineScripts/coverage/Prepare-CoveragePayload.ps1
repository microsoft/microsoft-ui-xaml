# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

# Prepares loose runtime copies to match the final product PDBs used for coverage.

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$ProductDir,

    [Parameter(Mandatory = $true)]
    [string]$PayloadDir
)

$ErrorActionPreference = 'Stop'

# Test apps can carry component-package DLLs from an earlier link.
# Even with identical machine code, their PDB GUID/age can differ.
# Coverage downloads only the final product PDBs, so use their matching
# DLLs for every loose payload copy before instrumentation.
foreach ($module in @('Microsoft.ui.xaml.dll', 'Microsoft.UI.Xaml.Controls.dll'))
{
    $source = Get-Item -LiteralPath (Join-Path $ProductDir $module)
    $copies = @(Get-ChildItem -LiteralPath $PayloadDir -Recurse -File -Filter $module)
    if ($copies.Count -eq 0) { throw "No payload copies of $module found." }
    foreach ($copy in $copies)
    {
        Copy-Item -LiteralPath $source.FullName -Destination $copy.FullName -Force
    }
    Write-Host "Prepared $($copies.Count) copies of $module from $($source.FullName)."
}
