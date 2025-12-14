Param(
    [Parameter(Mandatory = $true)] 
    [string]$PfxPath
)

$cerPath = [System.IO.Path]::Combine([System.IO.Path]::GetDirectoryName($PfxPath), "$([System.IO.Path]::GetFileNameWithoutExtension($PfxPath)).cer")

$shouldGenerate = $false
if (!(Test-Path $PfxPath) -or !(Test-Path $cerPath))
{
    # The file doesn't exist. We need to generate it.
    $shouldGenerate = $true
}
else
{
    $file = Get-Item $PfxPath
    if ($file.LastWriteTime -lt (Get-Date).AddMonths(-11)) 
    {
        # The file exists, but it is older than 11 months. We should regenerate it.
        # The default expiry is 12 months.
        $shouldGenerate = $true
    }
}

Write-Host "scripts\GenerateTestPfx.ps1: shouldGenerate=$($shouldGenerate)"

if($shouldGenerate)
{
    Write-Host "Generating $PfxPath..."

    $CertificateFriendlyName = "WinUITest"
    $Publisher = "CN=WinUITest"

    $cert = New-SelfSignedCertificate -Type Custom `
        -Subject $Publisher `
        -KeyUsage DigitalSignature `
        -FriendlyName $CertificateFriendlyName `
        -CertStoreLocation "Cert:\CurrentUser\My" `
        -TextExtension @("2.5.29.37={text}1.3.6.1.5.5.7.3.3", "2.5.29.19={text}")

    $certificateBytes = $cert.Export([System.Security.Cryptography.X509Certificates.X509ContentType]::Pkcs12)
    [System.IO.File]::WriteAllBytes($PfxPath, $certificateBytes)

    Get-PfxCertificate -FilePath $PfxPath | Export-Certificate -FilePath $cerPath -Type CERT | Out-Null
}