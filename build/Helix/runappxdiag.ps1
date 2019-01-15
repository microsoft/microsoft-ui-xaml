[CmdLetBinding()]
Param(
    [Parameter(Mandatory = $true)] 
    [string]$Action
)

If($Action -eq "Start")
{
    Write-Host creating shell
    $wshell = New-Object -ComObject wscript.shell;
    Write-Host running appxdiag
    $code = $wshell.Run("AppxDiag.exe", 3)
    Write-Host exit code: $code
    Write-Host sleeping
    Start-Sleep 10
    Write-Host sending keys
    $wshell.SendKeys('Y')
}
ElseIF($Action -eq "Stop")
{
    $proc = Get-Process AppxDiag
    If($proc)
    {
        Write-Host "Found AppxDiag process"
        $wshell = New-Object -ComObject wscript.shell;
        Write-Host "Activate app"
        Start-Sleep 5
        $wshell.AppActivate($proc.id)
        Start-Sleep 5
        Write-Host "send keys"
        $wshell.SendKeys('{ENTER}')
        Write-Host waiting for appxdiag to exit
        Wait-Process AppxDiag -Timeout 240

        $proc = Get-Process AppxDiag
        If($proc)
        {
            Write-Host "AppxDiag did not exit!"
        }
        Else
        {
            Write-Host "AppxDiag exited"
        }
    }
    Else
    {
        Write-Host "Could not find AppxDiag process"
    }
}
Else
{
    Write-Host "Unknown action: " $Action
}

Write-Host done