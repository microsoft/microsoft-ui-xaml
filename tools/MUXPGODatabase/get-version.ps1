function get_version( $pgoBranch )
{
    $forkSHA = $( git merge-base --fork-point origin/$pgoBranch HEAD )

    if ( $LastExitCode -ne 0 )
    {
        throw "FAILED: git merge-base"
    }

    $forkDate = ( Get-Date -Date $( git log -1 $forkSHA --date=iso --pretty=format:"%ad" ) ).ToUniversalTime().ToString("yyMMddHHmm")

    if ( $LastExitCode -ne 0 )
    {
        throw "FAILED: Get forkDate"
    }

    if ( $pgoBranch -eq "master" )
    {
        $version = $forkDate
    }
    else
    {
        $version = $forkDate + "-" + $pgoBranch.Replace("/", "_").Replace("-", "_").Replace(".", "_")
    }

    return $version
}