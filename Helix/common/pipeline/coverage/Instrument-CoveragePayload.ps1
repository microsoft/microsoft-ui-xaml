# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

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
    $alreadyInstrumented = $false
    if (Test-Path -LiteralPath $markerFile)
    {
        $marker = @(Get-Content -LiteralPath $markerFile)
        $alreadyInstrumented = $marker.Count -eq 2 -and $marker[1] -eq $primaryHash
        if ($alreadyInstrumented -and $marker[0] -ne $SessionId)
        {
            throw "$module is instrumented for another session. Recreate the payload before changing the session ID."
        }
    }

    if (-not $alreadyInstrumented)
    {
        # Every app loads its own copy. Only broadcast over identical binaries from this build.
        foreach ($copy in $copies)
        {
            if ((Get-FileHash -LiteralPath $copy.FullName -Algorithm SHA256).Hash -ne $primaryHash)
            {
                throw "The copies of $module differ. Recreate the payload from a single build."
            }
        }

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
        Set-Content -LiteralPath $markerFile -Value @($SessionId, $primaryHash) -Encoding ASCII
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
