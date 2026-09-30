# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

# Instruments loose MUX/MUXC payload copies and bundles the collector for lab agents.

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$PayloadDir,

    [Parameter(Mandatory = $true)]
    [string]$SymbolsSearchRoot,

    [Parameter(Mandatory = $true)]
    [ValidatePattern('^[a-zA-Z0-9-]+$')]
    [string]$SessionId,

    [string]$CoverageToolPath
)

$ErrorActionPreference = 'Stop'
$tool = & "$PSScriptRoot\Get-CoverageTool.ps1" -CoverageToolPath $CoverageToolPath
$PayloadDir = (Get-Item -LiteralPath $PayloadDir).FullName

$packages = @(Get-ChildItem -LiteralPath $PayloadDir -Recurse -File |
    Where-Object { $_.Extension -in @('.appx', '.msix', '.appxbundle', '.msixbundle') })
if ($packages.Count -gt 0)
{
    Write-Host '##vso[task.logissue type=warning]Coverage instruments loose runtime DLLs only. Runtime copies inside APPX/MSIX packages (including IXMPTestApp) are not instrumented, so their execution is missing from the report.'
}

# Lab agents have no VS installation. Bundle the same collector that instruments the DLLs.
$consoleDir = Split-Path $tool
$common7 = $tool.IndexOf('\Common7\', [StringComparison]::OrdinalIgnoreCase)
if ($common7 -lt 0)
{
    throw "Cannot locate the Visual Studio coverage dependencies from '$tool'."
}
$vsDir = $tool.Substring(0, $common7)
$toolDir = Join-Path $PayloadDir 'CoverageTool'
$toolSources = @{
    Console = $consoleDir
    Native = "$vsDir\Team Tools\Dynamic Code Coverage Tools"
    x64 = "$vsDir\Common7\IDE\CommonExtensions\Platform\InstrumentationEngine\x64"
    x86 = "$vsDir\Common7\IDE\CommonExtensions\Platform\InstrumentationEngine\x86"
}
foreach ($source in $toolSources.Values)
{
    if (-not (Test-Path -LiteralPath $source -PathType Container))
    {
        throw "Required coverage tool directory is missing: $source"
    }
}
New-Item -ItemType Directory -Path $toolDir -Force | Out-Null
Copy-Item "$($toolSources.Console)\*" $toolDir -Recurse -Force
Copy-Item "$($toolSources.Native)\*" $toolDir -Recurse -Force
foreach ($arch in @('x64', 'x86'))
{
    $destination = Join-Path $toolDir $arch
    New-Item -ItemType Directory -Path $destination -Force | Out-Null
    Copy-Item "$($toolSources[$arch])\*" $destination -Recurse -Force
}
if (-not (Test-Path "$toolDir\Microsoft.CodeCoverage.Console.exe"))
{
    throw 'The coverage collector was not bundled into the payload.'
}

foreach ($module in @('Microsoft.ui.xaml.dll', 'Microsoft.UI.Xaml.Controls.dll'))
{
    $copies = @(Get-ChildItem -LiteralPath $PayloadDir -Recurse -File -Filter $module)
    if ($copies.Count -eq 0)
    {
        throw "No copies of $module found in '$PayloadDir'."
    }

    $primary = $copies[0]
    $markerFile = Join-Path $PayloadDir "_coverage-instrumented-$module.txt"
    $primaryHash = (Get-FileHash -LiteralPath $primary.FullName -Algorithm SHA256).Hash
    $originalHash = $primaryHash
    $alreadyInstrumented = $false
    if (Test-Path -LiteralPath $markerFile)
    {
        $marker = @(Get-Content -LiteralPath $markerFile)
        if ($marker.Count -ne 3 -or $marker[1] -notmatch '^[A-Fa-f0-9]{64}$' -or $marker[2] -notmatch '^[A-Fa-f0-9]{64}$')
        {
            throw "Invalid coverage marker for $module. Recreate the payload before retrying."
        }
        $alreadyInstrumented = $marker[1] -eq $primaryHash
        if ($alreadyInstrumented -and $marker[0] -ne $SessionId)
        {
            throw "$module is instrumented for another session. Recreate the payload before changing the session ID."
        }
        if ($alreadyInstrumented)
        {
            $originalHash = $marker[2]
        }
    }

    # Retries may repair an original copy, but must never replace a DLL from another build.
    foreach ($copy in $copies)
    {
        $copyHash = (Get-FileHash -LiteralPath $copy.FullName -Algorithm SHA256).Hash
        if ($copyHash -notin @($primaryHash, $originalHash))
        {
            throw "The copies of $module differ. Recreate the payload from a single build."
        }
    }

    if (-not $alreadyInstrumented)
    {
        $pdbName = [IO.Path]::ChangeExtension($module, '.pdb')
        $pdbs = @(Get-ChildItem -LiteralPath $SymbolsSearchRoot -Recurse -File -Filter $pdbName)
        if ($pdbs.Count -ne 1)
        {
            throw "Expected one matching $pdbName in '$SymbolsSearchRoot'; found $($pdbs.Count)."
        }

        $stagedPdb = Join-Path $primary.DirectoryName $pdbName
        if (Test-Path -LiteralPath $stagedPdb)
        {
            throw "Expected a symbol-free test payload. Recreate it with -SkipSymbols before instrumenting."
        }
        Copy-Item -LiteralPath $pdbs[0].FullName -Destination $stagedPdb
        try
        {
            Write-Host "Instrumenting $($primary.FullName)"
            & $tool instrument $primary.FullName --session-id $SessionId --settings "$PSScriptRoot\coverage.config" | Out-Host
            if ($LASTEXITCODE -ne 0)
            {
                throw "Instrumentation failed for $module (exit $LASTEXITCODE). Recreate the payload before retrying."
            }
        }
        finally
        {
            Remove-Item -LiteralPath $stagedPdb
        }

        $instrumentedHash = (Get-FileHash -LiteralPath $primary.FullName -Algorithm SHA256).Hash
        if ($instrumentedHash -eq $primaryHash)
        {
            throw "$module was not instrumented. Use a coverage-enabled build with matching symbols and linker fixup metadata."
        }
        $primaryHash = $instrumentedHash
        Set-Content -LiteralPath $markerFile -Value @($SessionId, $primaryHash, $originalHash) -Encoding ASCII
    }

    $runtimes = @(Get-ChildItem -LiteralPath $primary.DirectoryName -File -Filter 'static_covrun*.dll')
    if ($runtimes.Count -eq 0)
    {
        throw "No static_covrun*.dll runtime was produced for $module."
    }
    foreach ($copy in $copies)
    {
        if ($copy.FullName -ne $primary.FullName)
        {
            Copy-Item -LiteralPath $primary.FullName -Destination $copy.FullName -Force
        }
        foreach ($runtime in $runtimes)
        {
            $destination = Join-Path $copy.DirectoryName $runtime.Name
            if ($destination -ne $runtime.FullName)
            {
                Copy-Item -LiteralPath $runtime.FullName -Destination $destination -Force
            }
        }
    }
    Write-Host "Instrumented $module in $($copies.Count) payload locations."
}

Set-Content -LiteralPath (Join-Path $PayloadDir '_coverage-session-id.txt') -Value $SessionId -Encoding ASCII
Write-Host "Coverage payload ready. Session: $SessionId"
