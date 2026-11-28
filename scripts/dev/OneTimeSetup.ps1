<#
.SYNOPSIS
This is a simple Powershell script to set up your developer environment to build WinUI

.DESCRIPTION
This script can install all the tools and dependencies you might need to be able to build WinUI 3

.EXAMPLE
OneTimeSetup.ps1 -Install VSCode
Installs VSCode

.EXAMPLE
OneTimeSetup.ps1 -Install all
Installs all the necessary tools: VSCode (optional), Git, MSBuild, LogViewer (optional)

.EXAMPLE
OneTimeSetup.ps1 -Clone C:\winui
Clones the repo onto C:\winui

.LINK
https://microsoft.visualstudio.com/WinUI
#>
[cmdletbinding(DefaultParametersetName="menu")]
param(
    [Parameter(Position=0, ParameterSetName="install")][switch]$Install,
    [Parameter(Position=1, ParameterSetName="install", Mandatory=$true)][ValidateSet('MSBuild', 'VSCode', 'LogViewer', 'Git', 'all')][string]$Packages,
    [Parameter(Position=0, ParameterSetName="menu")][switch]$Interactive
)
$DownloadPath=$env:temp
$ProgressPreference = 'SilentlyContinue'

function Download([uri]$Uri, [string]$OutFile)
{
    Write-Host -NoNewline "Downloading $OutFile... "
    Invoke-WebRequest -Uri $Uri -OutFile $DownloadPath\$OutFile -ErrorAction Stop
    Write-Host -ForegroundColor Green Done.
}

function LaunchSetupAndWait([string]$exePath, [string[]]$ArgumentList, [string]$WorkingDirectory = (Get-Location).Path)
{
    Write-Host -NoNewline "Launching $exePath... "
    $setupPath = $exePath
    if (![System.IO.Path]::IsPathRooted($setupPath))
    {
        $setupPath = Join-Path $DownloadPath $setupPath
    }
    $c = Start-Process $setupPath -Wait -PassThru -ArgumentList $ArgumentList -WorkingDirectory $WorkingDirectory -ErrorAction Stop
    $exitcode = $c.ExitCode
    if ($exitCode -eq 0 -or $exitCode -eq 3010)
    {
        Write-Host -ForegroundColor Green Done.
        if ($exitCode -eq 3010)
        {
            Write-Host -ForegroundColor Yellow 'A restart is required to complete setup.'
        }
    }
    else
    {
        Write-Host -ForegroundColor Red "Error $exitcode"
    }
    return ($exitCode -eq 0 -or $exitCode -eq 3010)
}

function Install-LogViewer
{
    # Install MSBuild Log viewer
    Download https://github.com/KirillOsenkov/MSBuildStructuredLog/releases/latest/download/MSBuildStructuredLogSetup.exe -OutFile "MSBuildStructuredLogSetup.exe"
    $installed = LaunchSetupAndWait MSBuildStructuredLogSetup.exe -ArgumentList "-s" 
    if ($installed)
    {
        $logViewerVersion = Get-ChildItem "$env:LOCALAPPDATA\msbuildstructuredlogviewer\app-*"
        if ($logViewerVersion.Count -gt 0)
        {
            $logViewerVersion = "$($logViewerVersion[$logViewerVersion.Count-1].Name)"
        }
        $pathToLogViewerExe = "$env:LOCALAPPDATA\msbuildstructuredlogviewer\$logViewerVersion\structuredlogviewer.exe"
        if (!(Test-Path $pathToLogViewerExe))
        {
            Write-Error "Install of Logviewer failed!"
            return
        }
        $assoc = . cmd /c "assoc .binlog 2>NUL"
        if ($assoc -eq $null)
        {
            Write-Host Elevating to associate binlog files with the log viewer
            Start-Process cmd.exe -ArgumentList @('/c', "assoc .binlog=MSBuildStructuredLog&&ftype MSBuildStructuredLog=%LOCALAPPDATA%\msbuildstructuredlogviewer\$logViewerVersion\structuredlogviewer.exe %1 %*") -Verb RunAs
        }
    }
    else
    {
        Write-Host "See logfile at $env:localappdata\SquirrelTemp\SquirrelSetup.log for errors"
    }
}

function Install-VSCode
{
# Install VSCode
# Use the current official stable installer URL.
$where = . cmd /c "where.exe code.cmd 2>NUL"
if ($where -ne $null) { Write-Host "VSCode already installed"; return; }
Download -Uri https://aka.ms/win32-x64-user-stable -OutFile "vscodesetup.exe"
$logfile = "$env:temp\vscodesetup.log"
$installed = LaunchSetupAndWait vscodesetup.exe -ArgumentList "/VERYSILENT /SUPPRESSMSGBOXES /NORESTART /LOG=$logfile /mergetasks=!runcode"
if ($installed)
{
    if (Test-Path $logfile)
    {
        $log = gc $logfile
        if (($log -like "Installation process succeeded.") -ne 1)
        {
            Write-Error "Something went wrong installing VSCode. See $logfile"
        }
    }
    else
    {
        Write-Host -ForegroundColor Yellow "$logfile not found"
    }
}
else
{
    Write-Host "See $logfile for errors"
}
}

function Install-MSBuild
{
    $repoRoot = (Resolve-Path "$PSScriptRoot\..\..").Path
    $installerDir = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer'
    $vswhere = Join-Path $installerDir 'vswhere.exe'
    $localBuildTools = Join-Path $repoRoot '.buildtools'
    $useLocalBuildTools = Test-Path -LiteralPath $localBuildTools -PathType Container
    $instance = $null
    if (Test-Path -LiteralPath $vswhere)
    {
        $productGroups = @(
            @('Microsoft.VisualStudio.Product.BuildTools'),
            @('Microsoft.VisualStudio.Product.Community', 'Microsoft.VisualStudio.Product.Professional', 'Microsoft.VisualStudio.Product.Enterprise')
        )
        if ($useLocalBuildTools)
        {
            $productGroups = ,@('*')
        }
        foreach ($products in $productGroups)
        {
            $vswhereArguments = @('-products') + $products + @('-version', '17.0', '-prerelease', '-format', 'json')
            if (!$useLocalBuildTools)
            {
                $vswhereArguments += '-latest'
            }
            $json = & $vswhere @vswhereArguments
            if ($LASTEXITCODE -ne 0)
            {
                throw "Visual Studio discovery failed with exit code $LASTEXITCODE."
            }
            if ([string]::IsNullOrWhiteSpace($json -join "`n"))
            {
                throw 'Visual Studio discovery returned no JSON.'
            }
            $instances = $json | ConvertFrom-Json -ErrorAction Stop
            if ($useLocalBuildTools)
            {
                $instance = $instances | Where-Object { $_.installationPath -and $_.installationPath.TrimEnd('\') -eq $localBuildTools } |
                    Select-Object -First 1
                if (!$instance)
                {
                    throw "The .buildtools directory exists, but no supported Visual Studio installation is registered at $localBuildTools."
                }
            }
            else
            {
                $instance = $instances | Select-Object -First 1
            }
            if ($instance)
            {
                break
            }
        }
    }

    elseif ($useLocalBuildTools -or (Test-Path -LiteralPath (Join-Path $installerDir 'setup.exe')))
    {
        throw "Visual Studio Installer or .buildtools is present, but discovery is unavailable at $vswhere. Repair the installer before running setup."
    }

    $configPath = Join-Path $repoRoot '.vsconfig_buildtools'
    if ($instance)
    {
        if (!$instance.installationPath -or !$instance.channelId -or !$instance.productId)
        {
            throw 'Visual Studio discovery returned an instance without its installation path, channel, or product ID.'
        }
        if ($instance.productId -ne 'Microsoft.VisualStudio.Product.BuildTools')
        {
            $configPath = Join-Path $repoRoot '.vsconfig'
        }
        $setupPath = Join-Path $installerDir 'setup.exe'
        if (!(Test-Path -LiteralPath $setupPath))
        {
            throw "Visual Studio Installer was not found at $setupPath."
        }
        Write-Host "Configuring $($instance.displayName) at $($instance.installationPath) using $configPath."
        # --wait is only supported by the bootstrapper, not the installed setup.exe.
        $installed = LaunchSetupAndWait $setupPath -WorkingDirectory $repoRoot -ArgumentList "modify --installPath `"$($instance.installationPath)`" --channelId `"$($instance.channelId)`" --config `"$configPath`""
    }
    else
    {
        Write-Host 'No supported Visual Studio installation found. Installing Visual Studio 2022 Build Tools.'
        Download -Uri https://aka.ms/vs/17/release/vs_BuildTools.exe -OutFile vs_buildtools.exe
        $installed = LaunchSetupAndWait vs_buildtools.exe -WorkingDirectory $repoRoot -ArgumentList "--add Microsoft.VisualStudio.Workload.MSBuildTools --config `"$configPath`" --wait"
    }

    if (!$installed)
    {
        $latestClientLog = Get-ChildItem $env:temp\dd_client_* -ErrorAction SilentlyContinue |
            Sort-Object -Property LastWriteTime -Descending | Select-Object -First 1
        if ($latestClientLog)
        {
            Get-Content $latestClientLog.FullName | Where-Object { $_ -like '*Error :*' } | Write-Host
        }
        throw 'Visual Studio setup failed. For more information see %temp%\dd_*.txt.'
    }
}

function Install-Git
{
$where = . cmd /c "where.exe git.exe 2>NUL"
if ($where -ne $null) { Write-Host "Git already installed"; return; }
Download -Uri https://github.com/git-for-windows/git/releases/download/v2.51.1.windows.1/Git-2.51.1-64-bit.exe -OutFile GitSetup.exe
$logfile = "$env:temp\gitsetup.log"
$installed = LaunchSetupAndWait GitSetup.exe -ArgumentList "/VERYSILENT /SP- /LOG=$logfile /NORESTART"
if ($installed)
{
    if (Test-Path $logfile)
    {
        $log = gc $logfile
        if (($log -like "Installation process succeeded.") -ne 1)
        {
            Write-Error "Something went wrong installing Git. See $logfile"
        }
    }
    else
    {
        Write-Host -ForegroundColor Yellow "$logfile not found"
    }
}
else
{
    Write-Host "See $logfile for errors"
}
}

function Clone-Repo([Parameter(Mandatory=$true)][string]$RepoRoot)
{
    . git clone https://github.com/microsoft/microsoft-ui-xaml.git $RepoRoot
}

function Process
{
    if ($PsCmdLet.ParameterSetName -eq "menu")
    {
            $opts = @{ 
            "Install tools" = @{ Parameters = @( @{ Name = 'Packages'; PossibleValues = @('MSBuild', 'Git', 'VSCode', 'LogViewer', 'all') } ); Expression = "Install-Packages"};
            "Clone repo" = @{ Parameters = @( @{ Name = 'RepoRoot'; AllowsEmpty = $false}); Expression = "Clone-Repo"}
        }
        RunMenu($opts)
    }
    else
    {
        if ($Install)
        {
            Install-Packages $Packages
        }

        if ($Clone)
        {
            Clone-Repo $RepoRoot
        }
    }

}

function Install-Packages([Parameter(Mandatory=$true)][string]$Packages)
{
    $availablePackages = @('MSBuild', 'Git', 'VSCode', 'LogViewer')
    if ($Packages -eq 'all')
    {
        $Packages = $availablePackages -join ';'
    }
    $installTasks  = ($Packages -split ';') | % { 'Install-' + $_ }
    foreach ($task in $installTasks)
    {
        iex $task
    }
}

function RunMenu($opts)
{
    Write-Host "WinUI 3.0 Setup script"
    Write-Host "---------------------------------------------------------------------"
    Write-Host "This will ensure you have the right tools and help you clone the repo"
    Write-Host ""

    $i = 1;
    $map = New-Object System.Collections.ArrayList
    foreach ($verb in $opts.Keys)
    {
        Write-Host "$i - $verb"
        $i++
        $map.Add($verb) | Out-Null
    }
    Write-Host "$i - Exit"
    $verb = Read-Host "Make a selection (1-$i)"
    $execOpts = @{}
    if ($verb -eq $i)
    {
        # Selected exit
        return;
    }
    elseif ($verb -ge 0 -and $verb -lt $i)
    {
        $verb = $map[$verb - 1]
        Write-Host You selected $verb
        $params = $opts[$verb].Parameters
        foreach ($param in $params)
        {
            if ($param.PossibleValues -ne $null)
            {
                Write-Host "Possible values: " ($param.PossibleValues -join ', ')
            }
            $paramValue = Read-Host -Prompt ($param.Name)
            if ($param.PossibleValues -ne $null)
            {
                $paramValue = $param.PossibleValues | where { $_ -ieq $paramValue }
            }
            
            if (($paramValue -eq '' -and $param.Contains('AllowsEmpty') -and !$param.AllowsEmpty) -or ($paramValue -eq $null))
            {
                Write-Error ("Unrecognized or empty parameter value for " + $param.Name)
                return;
            }
            else
            {
                $execOpts[$param.Name] = $paramValue;
            }
        }
        $execOpts = ($execOpts.Keys | % { '-' + $_ + ' ' + $execOpts[$_] }) -join ' '
        $expr = $opts[$verb].Expression + ' ' + $execOpts
        Write-Host Executing $expr
        iex $expr
    }
    else
    {
        Write-Error "Option $verb not recognized"
        return
    }
}


Process
Write-Host -NoNewline "Press any key to exit"
[Console]::ReadKey() | Out-Null
