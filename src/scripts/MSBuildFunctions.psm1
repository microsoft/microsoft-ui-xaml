# Information about the MSBuild version we depend on and the install script for it.
# This script is designed so that only these two variables need to change if we have
# a new dependency on a specific version of MSBuild. If the cmd prompt used for init
# isn't the correct one, this will download MSBuild to the .buildtools directory. The
# version can be as precise as we need it to be (i.e. 16.8.0.45203), but we should
# be as flexible as we can
[version]$MSBuildVer = "16.9"
$repoRoot = Split-Path -Parent $PSScriptRoot
$vsconfigPath = "$repoRoot\.vsconfig_buildtools"

# This URI corresponds to MSBuild 16.8. If this ever changes, change the MSBuildVer
# property above.
# Index of Visual Studio Build Tools download links:
#   https://docs.microsoft.com/en-us/visualstudio/releases/2019/history#installing-an-earlier-release
# NOTE: As of 5/25/01, the VS BuildTools is broken and unnecessary (all agents now being on VS 16.9),
# and has been disabled. If/when we need to return to using it, a cached offline version in the repo
# should be used instead, to ensure version stability of the VS BuildTools install itself:
# https://developercommunity.visualstudio.com/t/vs-168-buildtools-install-is-now-laying-down-1610/1434962?from=email&viewtype=all

# 16.9 build tools
#$MSBuildInstallURI = "https://download.visualstudio.microsoft.com/download/pr/5c555b0d-fffd-45a2-9929-4a5bb59479a4/68c048e8c687ed045d1efd3fdc531e5ce95c05dc374b5aaaeec3614ca8ed2044/vs_BuildTools.exe"

# 16.x latest built tools
#$MSBuildInstallURI = "https://aka.ms/vs/16/release/vs_buildtools.exe"

# Note: we use vs_community instead of vs_buildtools for 17.0, as some controls projects use the Microsoft.Windows.Design.Metadata.ProvideMetadataAttribute type.
# This type comes from Microsoft.Windows.Design.Extensibility.dll, which only ships in VS flavors with an IDE.  vs_buildtools does not have a component
# with Microsoft.Windows.Design.Extensibility in it.

# 17.x Preview build tools
#$MSBuildInstallURI = "https://aka.ms/vs/17/pre/vs_community.exe"

# 17.x GA build 

$VSInstallURIs = @{}
$VSInstallURIs.BuildTools169 = 'https://download.visualstudio.microsoft.com/download/pr/5c555b0d-fffd-45a2-9929-4a5bb59479a4/68c048e8c687ed045d1efd3fdc531e5ce95c05dc374b5aaaeec3614ca8ed2044/vs_BuildTools.exe'
$VSInstallURIs.Latest16x = 'https://aka.ms/vs/16/release/vs_buildtools.exe'
$VSInstallURIs.Release17x = 'https://aka.ms/vs/17/release/vs_community.exe'
$VSInstallURIs.Preview17x = 'https://aka.ms/vs/17/pre/vs_community.exe'

if ( [string]::IsNullOrEmpty( $VsVersionToInstall ))
{
    $VsVersionToInstall = $VSInstallURIs.Release17x
}

$MSBuildInstallURI = $VsVersionToInstall

$MSBuildInstallParams = "--norestart --quiet --force --installPath ""{0}"" {1} --config $vsconfigPath"
$MSBuildUninstallParams = "uninstall --norestart --quiet --force --installPath ""{0}"" {1}"

function Download-MSBuild([string]$OutFile)
{
    Write-Host -NoNewline "Downloading $OutFile... "
    Invoke-WebRequest -Uri $MSBuildInstallURI -OutFile $OutFile
    Write-Host -ForegroundColor Green Done.
}

function LaunchSetupAndWait([string]$exePath, [string[]]$ArgumentList)
{
    Write-Host "$exePath $($ArgumentList -join ' ')"
    $c = Start-Process "$exePath" -Wait -PassThru -ArgumentList $ArgumentList
    $exitcode = $c.ExitCode

    # 3010 just means restart required, which isn't necessary
    # 5007 indicates installer could not make changes (triggered by ARM64 ATL) - requires elevation
    if($exitcode -eq 5007)
    {
        Write-Host -ForegroundColor Yellow "Warning: could not update build tools. If necessary, run init elevated.";
    }
    $success = $exitCode -eq 0 -or $exitCode -eq 3010 -or $exitcode -eq 5007
    if ($success)
    {
        Write-Host -ForegroundColor Green Done.
    }
    else
    {
        Write-Host -ForegroundColor Red "Error $exitcode"
    }
    return ($success)
}

function Install-MSBuild([string]$installDir, [string]$vs_installer, [string]$logsDir, [bool]$add_wait = $True)
{
    New-Item -Path $installDir -Force -ItemType 'Directory' | Out-Null
    if ($add_wait){ $wait = "--wait" }
    $installed = LaunchSetupAndWait $vs_installer -ArgumentList "$([string]::Format($MSBuildInstallParams, $installDir, $wait))"
    if (!$installed)
    {
        Write-InstallError $logsDir
        exit 1
    }

    Write-InstallError $logsDir
}

function Modify-MSBuild([string]$installDir, [string]$vs_installer, [string]$logsDir, [bool]$add_wait = $True)
{
    if ($add_wait){ $wait = "--wait" }
    $modified = LaunchSetupAndWait $vs_installer -ArgumentList "modify $([string]::Format($MSBuildInstallParams, $installDir, $wait))"
    if (!$modified)
    {
        Write-InstallError $logsDir
        exit 1
    }
}

function Uninstall-MSBuild([string]$installDir, [string]$vs_installer, [string]$logsDir)
{
    $installed = LaunchSetupAndWait $vs_installer -ArgumentList "$([string]::Format($MSBuildUninstallParams, $installDir))"
    if (!$installed)
    {
        Write-InstallError $logsDir
        exit 1
    }
}

function Write-InstallError([string]$logsDir)
{
    # try to figure out what went wrong
    $latestClientLog = (Get-ChildItem $env:temp\dd_* | Sort-Object -Property LastWriteTime -Descending)[0]
    $log = (Get-Content $latestClientLog)
    $errorLines = ($log -like '*Error :*')
    if ($errorLines)
    {
        Write-Host $errorLines
    }
    
    $logLocation = $env:Temp
    if (-not [string]::IsNullOrEmpty($logsDir))
    {
        New-Item -Path $logsDir -Force -ItemType 'Directory' | Out-Null
        $logLocation = $logsDir
        Copy-Item -Path "$env:Temp\*" -Filter "dd_*" -Destination $logsDir
    }

    Write-Host "For more information see $logLocation\dd_*"
}

function Test-MSBuild([string]$msbuildExe)
{
    $msbuildVerInstalled = "1.0"
    if (Test-Path $msbuildExe)
    {
        $msbuildVerInstalled = .$msbuildExe "-version" "-noLogo"
        Write-Host "Currently active MSBuild.exe at $msbuildExe is version $msbuildVerInstalled"
        Write-Host
    }
    return ([version]$msbuildVerInstalled) -ge $MSBuildVer
}