# Copies instrumented external binaries from transport packages into
# the build output, overwriting shipping versions for PGO training.
#
# Currently handles: WinUIEdit.dll from performance infrastructure packages.
Param(
    [Parameter(Mandatory = $true)]
    [string] $repoRoot,
    [Parameter(Mandatory = $true)]
    [string] $buildOutput,
    [Parameter(Mandatory = $true)]
    [string] $normalizedConfiguration,
    [Parameter(Mandatory = $true)]
    [string] $buildPlatform
)

$ErrorActionPreference = "Stop"

$arch = $buildPlatform.ToLower()
if ($arch -eq "win32") { $arch = "x86" }

$src = Get-ChildItem "$repoRoot\packages\Microsoft.Internal.WinUIDetails\*\Instrumentation\win10-$arch" -ErrorAction Stop | Select-Object -First 1
$productDir = "$repoRoot\BuildOutput\bin\$buildOutput$normalizedConfiguration\Product"
$packagingDir = "$repoRoot\BuildOutput\packaging\Release\runtimes-framework\win-$arch\native"

foreach ($dir in @($productDir, $packagingDir)) {
    Write-Host "Copying instrumented WinUIEdit to: $dir"
    Copy-Item "$($src.FullName)\WinUIEdit.dll" $dir -Force
}

Copy-Item "$($src.FullName)\WinUIEdit.pgd" $productDir -Force
