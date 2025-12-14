$scriptsDir = $PSScriptRoot

Write-Host -NoNewline "Checking long path support...";
try
{
    $regKeyPath = "HKLM:/SYSTEM/CurrentControlSet/Control/FileSystem"
    $regKeyValue = "LongPathsEnabled"

    $isLongPathsEnabled = (Get-ItemProperty $regKeyPath -Name $regKeyValue -ErrorAction Stop).$regKeyValue -eq 1
}
catch
{
    $isLongPathsEnabled = $false
}

if ($isLongPathsEnabled)
{
    Write-Host -ForegroundColor Green "OK."
}
else
{
    Write-Host -ForegroundColor Red "Failed."
    Write-Host -NoNewline "Enabling long path support. This step will ask to run elevated..."
    Start-sleep 5
    $ArgumentList = "/c $scriptsDir\SetLongPathsEnabled.cmd"
    $c = Start-Process cmd.exe -Wait -PassThru -ArgumentList $ArgumentList -Verb RunAs
    $exitcode = $c.ExitCode

    $success = $exitCode -eq 0
    if ($success)
    {
        Write-Host -ForegroundColor Green "Done. A reboot may be required."
    }
    else
    {
        Write-Host -ForegroundColor Red "Error $exitcode."
        throw "Error enabling long path support."
    }
}