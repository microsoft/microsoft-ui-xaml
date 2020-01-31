function get_version( $pgoBranch )
{
    Write-Output $pgoBranch

    $forkSHA = ( & git merge-base --fork-point $pgoBranch )

    Write-Output $forkSHA

    if ( $LastExitCode -ne 0 )
    {
        Write-Output $LastExitCode
        throw "FAILED: git merge-base"
    }

    $forkDate = ( Get-Date -Date ( & git log -1 $forkSHA --date=iso --pretty=format:"%ad" ) ).ToUniversalTime().ToString("yyMMddHHmm")

    Write-Output $forkDate

    if ( $LastExitCode -ne 0 )
    {
        Write-Output $LastExitCode
        throw "FAILED: Get forkDate"
    }

    if ($pgoBranch -eq "master")
    {
        $version = $forkDate
    }
    else
    {
        $version = $forkDate + "-" + $pgoBranch.Replace("/", "_").Replace("-", "_").Replace(".", "_")
    }

    return $version
}