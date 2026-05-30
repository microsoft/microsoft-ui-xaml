##############################################################################  
##  
## init.ps1  
##
## This is the PowerShell version of init.cmd.
## It mostly calls init.cmd, captures its environment variables, and adds them
## to the current PowerShell session.
##
##############################################################################  

function is-admin
{
    $currentPrincipal = New-Object Security.Principal.WindowsPrincipal(
        [Security.Principal.WindowsIdentity]::GetCurrent() );
    return $currentPrincipal.IsInRole( [Security.Principal.WindowsBuiltInRole]::Administrator )
}

$rootDir = $PSScriptRoot
$initScriptsDir = join-path $rootDir "scripts\init"

## Command line options that are specific to PowerShell init (init.ps1)
$arguments = New-Object Collections.ArrayList (,$args)

## Invoke-CmdScript will grab the env. vars. that init.cmd sets, and apply them to
## the PS environment.
. (join-path $initScriptsDir Invoke-CmdScript) (join-path $rootDir "init.cmd") $arguments.ToArray()

if ($LASTEXITCODE -ne 0)
{
    throw "init.cmd failed, error: $($LASTEXITCODE). See init.cmd output for more info."
}

if (!$env:NoTitle)
{
    ## Set the name of the console.
    ## It is required when running this script from an existing psh console
    ## The "Administrator:" string isn't forced by powershell, so do that, too.
    $admin = if (is-admin) {"Administrator: "} else  {""};
    $host.ui.RawUI.WindowTitle = $admin + "DCPP $env:RepoRoot - $env:_BuildArch$env:_BuildType"
}

# Add some common aliases
& (join-path $initScriptsDir Add-Aliases)
