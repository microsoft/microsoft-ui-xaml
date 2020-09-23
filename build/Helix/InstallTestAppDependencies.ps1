# Displaying progress is unnecessary and is just distracting.
$ProgressPreference = "SilentlyContinue"

$dependencyFiles = Get-ChildItem -Filter "*dependencies.txt"

foreach ($file in $dependencyFiles)
{
    Write-Host "Adding dependency $($file)..."

    foreach ($line in Get-Content $file)
    {
        Add-AppxPackage $line -ErrorVariable appxerror -ErrorAction SilentlyContinue

        if($appxerror)
        {
            foreach($error in $appxerror)
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
}