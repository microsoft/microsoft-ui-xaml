function Log-BeginGroup ( $msg )
{
    Write-Host ">>>>>> $msg >>>>>>" -ForegroundColor Green
}

function Log-EndGroup ( $msg )
{
    Write-Host "<<<<<< $msg <<<<<<" -ForegroundColor Green
}

function Log-Command ( $msg )
{
    Write-Host "$msg" -ForegroundColor Blue
}

function Log-Debug ( $msg )
{
    Write-Host "$msg" -ForegroundColor DarkGray
}

function Log-Info ( $msg )
{
    Write-Host "$msg"
}

function Log-Warning ( $msg )
{
    Write-Host "warning: $msg" -ForegroundColor Yellow
}

function Log-Error ( $msg )
{
    Write-Host "error: $msg" -ForegroundColor Red
}

function Log-ErrorAndThrow ( $msg )
{
    Log-Error $msg
    throw $msg
}