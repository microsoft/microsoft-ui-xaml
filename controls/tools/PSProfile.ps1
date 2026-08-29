Write-Host "Loading tools\PSProfile.ps1"

#
# ImportCmdAlias helper
#

function ImportCmdAlias($line)
{
    $aliasParts = $line.Split("=",2);

    if ($aliasParts[0] -and $aliasParts[1])
    {
        $aliasName = $aliasParts[0].Trim()
        $aliasValue = $aliasParts[1].Trim()

        $aliasValue = [regex]::Replace($aliasValue, '\&\& cd \$\*', ''); # Weird thing on some .. aliases

        $aliasValue = [regex]::Replace($aliasValue, 'cd /d ', 'pushd '); # cd /d doesn't work in powershell. use pushd
        $aliasValue = [regex]::Replace($aliasValue, "%(.+?)%", '${env:$1}');
        $aliasValue = [regex]::Replace($aliasValue, '\$\*', '$args');
        $aliasValue = [regex]::Replace($aliasValue, '\$1', '$args');
        $aliasValue = [regex]::Replace($aliasValue, '\$\*', '$args');
        $aliasValue = [regex]::Replace($aliasValue, 'pushd (.*)', 'pushd "$1"');
        if ($aliasValue.Substring(0,1) -eq '"') { $aliasValue = "start $aliasValue" }


        #
        # Now we have the alias name and value.
        # The name is just going to be the function name, the value will be
        # 

        if (test-path function:\$aliasName)
        {
            Write-Host "'$aliasName' is already defined as a function, skipping"
        }
        else
        {
            try
            {
                Invoke-Expression "function global:CmdAlias-$aliasName {$aliasValue}"
                set-alias $aliasName CmdAlias-$aliasName -Scope global -option "AllScope" -force
            } catch {
                #Write-Host "Unable to import alias '$line':" $_.Exception.Message
            }
        }
    }
}

#
# Import aliases
#

cmd.exe /c doskey /macros -p cmd.exe | ForEach-Object {
    ImportCmdAlias $_
}

$localRepoName = $script:MyInvocation.MyCommand.Path | Split-Path -Parent | Split-Path -Parent | Split-Path -Leaf

$Host.UI.RawUI.WindowTitle = "$localRepoName : $(git rev-parse --abbrev-ref HEAD)"
