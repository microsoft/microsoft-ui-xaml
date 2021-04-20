# Displaying progress is unnecessary and is just distracting.
$ProgressPreference = "SilentlyContinue"

$dependencyFiles = Get-ChildItem -Filter "*dependencies.txt"
$dependencies = [System.Collections.Generic.List[string]]@()

foreach ($file in $dependencyFiles)
{
    Write-Host "Collecting dependencies for $file..."

    foreach ($line in Get-Content $file)
    {
        if (-not $dependencies.Contains($line))
        {
            $dependencies.Add($line)
        }
    }
}

foreach ($dependency in $dependencies)
{
    Write-Host "Adding AppX dependency $dependency..."
    
    Write-Host "    Retrieving certificate..."
    $dependencyCertFile = "$env:TEMP\$($dependency.Replace(".appx", ".cer"))"
    [System.IO.File]::WriteAllBytes($dependencyCertFile, (Get-AuthenticodeSignature $dependency).SignerCertificate.Export([System.Security.Cryptography.X509Certificates.X509ContentType]::Cert))
    
    Write-Host "    Installing certificate..."
    Start-Process "certutil.exe" -ArgumentList "-addStore TrustedPeople $dependencyCertFile" -NoNewWindow -Wait | Out-Null
    
    Write-Host "    Installing AppX..."
    Add-AppxPackage $dependency -ErrorVariable appxerror -ErrorAction SilentlyContinue

    if ($appxerror)
    {
        foreach ($error in $appxerror)
        {
            # In the case where the package does not install becasuse a higher version is already installed
            # we don't want to print an error message, since that is just noise. Filter out such errors.
            if($error.Exception.Message -match "0x80073D06")
            {
                Write-Host "A higher version of this package is already installed."
            }
            else
            {
                Write-Error $error
            }
        }
    }
}