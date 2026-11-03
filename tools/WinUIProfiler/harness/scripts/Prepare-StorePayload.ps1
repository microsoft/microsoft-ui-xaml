[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [string]$RepoRoot,

    [Parameter(Mandatory)]
    [string]$OutputRoot,

    [Parameter(Mandatory)]
    [string]$StorePackageScript,

    [string]$Configuration = "Release",

    [ValidateSet("x64", "ARM64")]
    [string]$Platform = "x64",

    [string]$TargetFramework = "net10.0-windows10.0.28000.0"
)

$ErrorActionPreference = "Stop"
. (Join-Path $PSScriptRoot "Assert-GuestExecution.ps1")
$publisher = "CN=Microsoft Corporation, O=Microsoft Corporation, L=Redmond, S=Washington, C=US"
$certificateFriendlyName = "WinUI Profiler Harness Store Sideload"

function Get-StoreSigningCertificate {
    $certificate = Get-ChildItem "Cert:\CurrentUser\My" -CodeSigningCert |
        Where-Object {
            $_.Subject -eq $publisher -and
            $_.FriendlyName -eq $certificateFriendlyName -and
            $_.HasPrivateKey -and
            $_.NotAfter -gt (Get-Date).AddDays(7)
        } |
        Sort-Object NotAfter -Descending |
        Select-Object -First 1

    if ($null -ne $certificate) {
        return $certificate
    }

    return New-SelfSignedCertificate `
        -Type Custom `
        -Subject $publisher `
        -KeyUsage DigitalSignature `
        -FriendlyName $certificateFriendlyName `
        -CertStoreLocation "Cert:\CurrentUser\My" `
        -TextExtension @(
            "2.5.29.37={text}1.3.6.1.5.5.7.3.3",
            "2.5.29.19={text}"
        ) `
        -NotAfter (Get-Date).AddYears(1)
}

function Get-SignToolPath {
    $kitsRoot = Join-Path ${env:ProgramFiles(x86)} "Windows Kits\10\bin"
    if (-not (Test-Path -LiteralPath $kitsRoot -PathType Container)) {
        throw "Windows SDK tools were not found at '$kitsRoot'."
    }

    $architecture = if ($Platform -eq "ARM64") { "arm64" } else { "x64" }
    $signTool = Get-ChildItem -LiteralPath $kitsRoot -Filter "signtool.exe" -File -Recurse |
        Where-Object { $_.Directory.Name -eq $architecture } |
        Sort-Object FullName -Descending |
        Select-Object -First 1
    if ($null -eq $signTool) {
        throw "signtool.exe for $architecture was not found below '$kitsRoot'."
    }
    return $signTool.FullName
}

$sourcePackages = Join-Path $RepoRoot "bin\$Configuration\WinStore2\$TargetFramework\$Platform\AppPackages"
if (-not (Test-Path -LiteralPath $sourcePackages -PathType Container)) {
    throw "Store AppPackages directory was not found: '$sourcePackages'. Run the app build first."
}
if (-not (Get-ChildItem -LiteralPath $sourcePackages -Filter "*.msix" -File -Recurse | Select-Object -First 1)) {
    throw "No Store MSIX was found below '$sourcePackages'."
}
if (-not (Test-Path -LiteralPath $StorePackageScript -PathType Leaf)) {
    throw "Store package deployment script was not found: '$StorePackageScript'."
}

$fullOutputRoot = [IO.Path]::GetFullPath($OutputRoot)
$volumeRoot = [IO.Path]::GetPathRoot($fullOutputRoot)
if ($fullOutputRoot.TrimEnd('\').Equals($volumeRoot.TrimEnd('\'), [StringComparison]::OrdinalIgnoreCase)) {
    throw "Refusing to use a volume root as the Store payload output directory: '$fullOutputRoot'."
}

if (Test-Path -LiteralPath $fullOutputRoot) {
    Remove-Item -LiteralPath $fullOutputRoot -Recurse -Force
}
$relativePackages = "bin\$Configuration\WinStore2\$TargetFramework\$Platform\AppPackages"
$destinationPackages = Join-Path $fullOutputRoot $relativePackages
[IO.Directory]::CreateDirectory((Split-Path -Parent $destinationPackages)) | Out-Null
Copy-Item -LiteralPath $sourcePackages -Destination $destinationPackages -Recurse -Force
Copy-Item -LiteralPath $StorePackageScript -Destination (Join-Path $fullOutputRoot "StorePackage.ps1") -Force
Copy-Item -LiteralPath (Join-Path $PSScriptRoot "Assert-GuestExecution.ps1") `
    -Destination (Join-Path $fullOutputRoot "Assert-GuestExecution.ps1") -Force

$destinationMsix = Get-ChildItem -LiteralPath $destinationPackages -Filter "*.msix" -File -Recurse |
    Sort-Object LastWriteTimeUtc -Descending |
    Select-Object -First 1
if ($null -eq $destinationMsix) {
    throw "No copied Store MSIX was found below '$destinationPackages'."
}

$certificate = Get-StoreSigningCertificate
$signTool = Get-SignToolPath
& $signTool sign /fd SHA256 /sha1 $certificate.Thumbprint /td SHA256 $destinationMsix.FullName
if ($LASTEXITCODE -ne 0) {
    throw "signtool failed with exit code $LASTEXITCODE while signing '$($destinationMsix.FullName)'."
}

$signature = Get-AuthenticodeSignature -LiteralPath $destinationMsix.FullName
if ($null -eq $signature.SignerCertificate -or
    $signature.SignerCertificate.Thumbprint -ne $certificate.Thumbprint) {
    throw "The copied Store MSIX was not signed by the expected harness certificate."
}

$certificatePath = Join-Path $fullOutputRoot "StorePackageSigner.cer"
Export-Certificate -Cert $certificate -FilePath $certificatePath -Force | Out-Null

[pscustomobject]@{
    outputRoot = $fullOutputRoot
    packages = $destinationPackages
    packageCount = @(Get-ChildItem -LiteralPath $destinationPackages -Filter "*.msix" -File -Recurse).Count
    signedMsix = $destinationMsix.FullName
    signerThumbprint = $certificate.Thumbprint
    signingCertificate = $certificatePath
}
