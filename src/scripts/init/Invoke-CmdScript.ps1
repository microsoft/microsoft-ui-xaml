##############################################################################  
##  
## Invoke-CmdScript.ps1  
##
## from http://www.leeholmes.com/blog/NothingSolvesEverythingPowerShellAndOtherTechnologies.aspx
##
## Invoke the specified batch file (and parameters), but also propigate any  
## environment variable changes back to the PowerShell environment that  
## called it.  
##  
## ie:  
##  
## PS > type foo-that-sets-the-FOO-env-variable.cmd  
## @set FOO=%*  
## echo FOO set to %FOO%.  
##   
## PS > $env:FOO  
##   
## PS > Invoke-CmdScript "foo-that-sets-the-FOO-env-variable.cmd" Test 
##   
## C:\Temp>echo FOO set to Test.
## FOO set to Test.  
##   
## PS > $env:FOO  
## Test  
##  
##############################################################################  

param([string] $script, [string] $parameters)  

# Make sure the temp directory exists before we call GetTempFileName
if (!(Test-Path -Path $env:TEMP))
{
    New-Item -Path $env:TEMP -Force -ItemType "Directory" | Out-Null
}

$environmentVariablesFile = [IO.Path]::GetTempFileName()
$workingDirectoryFile = [IO.Path]::GetTempFileName()

## Store the output of cmd.exe.  We also ask cmd.exe to output   
## the environment table after the batch file completes.  The same
## for the current directory.

cmd /c " `"$script`" $parameters && set > `"$environmentVariablesFile`" && cd > `"$workingDirectoryFile`" " 

# In case the command did a cd
Get-Content $workingDirectoryFile | Set-Location

## Go through the environment variables in the temp file.  
## For each of them, set the variable in our local environment.  
Get-Content $environmentVariablesFile | Foreach-Object {   
    if($_ -match "^([^=].*?)=(.*)$")  
    { 
        Set-Content "env:\$($matches[1])" $matches[2]  
    } 
}  

Remove-Item $environmentVariablesFile, $workingDirectoryFile
