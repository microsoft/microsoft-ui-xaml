param (
    [string]$version,
    [ValidateSet('daily','signed','validated','preview','GA',"")]
    [string]$quality,
    [string]$channel
)

$dotnetInstallScript = "$env:TEMP\dotnet-install.ps1"

$repoInstallDir  = [System.IO.Path]::GetFullPath("$PSScriptRoot\..\.dotnet")
$versionPropsFileProject = ([xml](Get-Content -Raw "$PSScriptRoot\..\eng\versions.props")).Project
#dotNetSdkChannel and dotNetSdkVNextChannel refer to the current Long Term Support and Standard Term Support versions of .NET, respectively. 
$dotNetSdkChannel = $versionPropsFileProject.SelectSingleNode('//dotNetSdkChannel').InnerText
$dotNetSdkVNextChannel  = $versionPropsFileProject.SelectSingleNode('//dotNetSdkVNextChannel').InnerText

[System.Net.ServicePointManager]::SecurityProtocol = [System.Net.SecurityProtocolType]'Ssl3,Tls,Tls11,Tls12'

Invoke-WebRequest https://dot.net/v1/dotnet-install.ps1 -OutFile $dotnetInstallScript

$x64InstallDir  = $repoInstallDir
$x86InstallDir  = "$x64InstallDir\x86"



# This script will first check if an SDK version has been passed in
# If the version is present all other arguments are ignored and the SDK with the specified version will be installed.
# If the version argument is not present, it will check for the presence of the quality argument.
# If quality is present, it will check for the presence of the channel argument.
# If the channel argument is also present, it will use both the channel and quality arguments.
# If the quality argument is present but the channel argument is not, it will output an error message.
# If neither the version nor quality arguments are present, it will use the channel specified in the dotNet7SdkChannel/dotNet8SdkChannel variables in eng/versions.props

if (-not [string]::IsNullOrEmpty($version)) 
{
    # ignore quality and channel
    . $dotnetInstallScript -Version $version -InstallDir $x64InstallDir -Architecture x64
    Write-Host "Installed SDK (x64) with Version $version to $x64InstallDir." -ForegroundColor green

    . $dotnetInstallScript -Version $version -InstallDir $x86InstallDir -Architecture x86
    Write-Host "Installed SDK (x86) with Version $version to $x86InstallDir." -ForegroundColor green    
}
elseif (-not [string]::IsNullOrEmpty($quality)) 
{
    if ([string]::IsNullOrEmpty($channel)) 
    {
        # quality must be accompanied by a channel
        Write-Error "Error: Quality must be accompanied by a channel."
    }
    else 
    {
        # use channel and quality
        Write-Host "Using channel: $channel and quality: $quality"

        . $dotnetInstallScript -channel $channel -quality $quality -InstallDir $x64InstallDir -Architecture x64
        Write-Host "Installed SDK (x64) with channel $channel and quality $quality to $x64InstallDir." -ForegroundColor green
    
        . $dotnetInstallScript -channel $channel -quality $quality -InstallDir $x86InstallDir -Architecture x86
        Write-Host "Installed SDK (x86) with channel $channel and version $quality to $x86InstallDir." -ForegroundColor green 
    }
}
elseif (-not [string]::IsNullOrEmpty($channel)) 
{
    # use only channel
    Write-Host "Using channel: $channel"

    . $dotnetInstallScript -channel $channel -InstallDir $x64InstallDir -Architecture x64
    Write-Host "Installed SDK (x64) with channel $channel to $x64InstallDir." -ForegroundColor green

    . $dotnetInstallScript -channel $channel -InstallDir $x86InstallDir -Architecture x86
    Write-Host "Installed SDK (x86) with channel $channel to $x86InstallDir." -ForegroundColor green
}
else 
{
    #Picks up Channel Version specified in eng/Versions.props

    $backupChannel = $dotNetSdkChannel

    #Currently to install .NET8 a quality of "daily" must also be provided.
    if ($env:_DotNetMoniker -eq "net8") 
    {
        $backupChannel = $dotNetSdkVNextChannel
    }    

    . $dotnetInstallScript -channel $backupChannel -InstallDir $x64InstallDir -Architecture x64
    Write-Host "Installed SDK (x64) from Version.props with channel $backupChannel to $x64InstallDir." -ForegroundColor green

    . $dotnetInstallScript -channel $backupChannel -InstallDir $x86InstallDir -Architecture x86
    Write-Host "Installed SDK (x86) from Version.props with channel $backupChannel to $x86InstallDir." -ForegroundColor green
}


