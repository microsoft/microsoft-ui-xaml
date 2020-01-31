function get_version( $pgoBranch )
{
    Write-Host $pgoBranch

    Write-Host $(git log)


    $forkSHA = $( git merge-base --fork-point $pgoBranch )

    Write-Host $LastExitCode
    Write-Host $forkSHA

    if ( $LastExitCode -ne 0 )
    {
        throw "FAILED: git merge-base"
    }

    $forkDate = ( Get-Date -Date $( git log -1 $forkSHA --date=iso --pretty=format:"%ad" ) ).ToUniversalTime().ToString("yyMMddHHmm")

    Write-Host $forkDate
    Write-Host $forkSHA

    if ( $LastExitCode -ne 0 )
    {
        throw "FAILED: Get forkDate"
    }

    if ( $pgoBranch -eq "origin/master" )
    {
        $version = $forkDate
    }
    else
    {
        $version = $forkDate + "-" + $pgoBranch.Replace("/", "_").Replace("-", "_").Replace(".", "_")
    }

    return $version
}