function Log-BeginGroup ( $msg )
{
    Write-Host "##[group]$msg"
}

function Log-EndGroup ( )
{
    Write-Host "##[endgroup]"
}

function Log-Command ( $msg )
{
    Write-Host "##[command]$msg"
}

function Log-Debug ( $msg )
{
    Write-Host "##[debug]$msg"
}

function Log-Info ( $msg )
{
    Write-Host "##[section]$msg"
}

function Log-Warning ( $msg )
{
    Write-Host "##[warning]$msg"
}

function Log-Error ( $msg )
{
    Write-Host "##[error]$msg"
}

function Log-ErrorAndThrow ( $msg )
{
    Log-Error $msg
    throw $msg
}