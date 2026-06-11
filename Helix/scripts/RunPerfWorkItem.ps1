[CmdLetBinding()]
Param(
    [Parameter(Mandatory = $true)]
    [string]$PackageState,
    [string]$ArgsFile
)
function Run-OneTimeSetup ()
{
    $platform = $env:testbuildplatform

    if(!$platform)
    {
        $platform = "x86"
        if(Test-Path ./buildconfig.json)
        {
            $buildConfig = Get-Content ./buildconfig.json | Out-String | ConvertFrom-Json
            $platform = $buildConfig.Platform
        }
    }

    if ( $platform -eq "amd64" )
    {
        $platform = "x64"
    }

    Write-Host "Run-OneTimeSetup, platform=$platform"

    Write-Host "Setting registry keys"
    reg add HKLM\Software\Policies\Microsoft\Windows\Appx /v AllowDevelopmentWithoutDevLicense /t REG_DWORD /d 1 /f
    reg add HKLM\SOFTWARE\Microsoft\WinUI\XAML /v ErrorLogDirectory /t REG_SZ /d C:\ProductErrorLogs /f

    Write-Host "Adding test certs"
    certutil -addstore TrustedPeople WinUITest.cer

    Write-Host "Installing scenario apps"
    Get-AppxPackage -name XamlPGO* | foreach { Remove-AppxPackage $_.PackageFullName }
    Get-ChildItem Test\perf\apps\ | foreach { & Test\perf\apps\$_\Add-AppDevPackage.ps1 -force }

    Write-Host "Installing VC Redist"
    Start-Process -Wait -FilePath .\vc_redist.$platform.exe -ArgumentList "/install /quiet /norestart"
}

$saveWorkingDir = Get-Location
Write-Host "saveWorkingDir = $saveWorkingDir"
cd $env:HELIX_CORRELATION_PAYLOAD

# Setup

if (Test-Path .\pgc)
{
    Remove-Item .\pgc -Recurse -ErrorAction Stop
}

New-Item .\pgc -ItemType Directory -ErrorAction Stop

$markerfile = ".\setupcomplete.txt"

if ($PackageState -eq "/p:packaged")
{
    if ( !( Test-Path( $markerfile ) ) )
    {
        Write-Host "Running setup"
        Run-OneTimeSetup
        Set-Content $markerfile "done"
    }
}
else
{
    Write-Host "Running unpackaged. Setup not needed. \n"    
}

# Ideally, we would just use '&' or 'Invoke-Expression' here to execute taef. However, powershell unhelpfully modifies the string to add
# extra quotes around parts of the arguments which gives the incorrect behavior since the argument string is already exactly as it needs
# to be. I was unable to find a way to disable this behavior, so as a workaround we create a .cmd file and invoke that.

$taefArgs = ( Get-Content ( Join-Path $saveWorkingDir $ArgsFile ) )
$teCommand = "te.exe $taefArgs"
Write-Host $teCommand
Out-File -FilePath ".\run.cmd" -Encoding ascii -InputObject $teCommand
& .\run.cmd

if (!(Test-Path $env:HELIX_WORKITEM_UPLOAD_ROOT))
{
    New-Item $env:HELIX_WORKITEM_UPLOAD_ROOT -ItemType Directory -ErrorAction Stop
}

Copy-Item .\pgc $env:HELIX_WORKITEM_UPLOAD_ROOT -Force -Recurse

cd $saveWorkingDir
