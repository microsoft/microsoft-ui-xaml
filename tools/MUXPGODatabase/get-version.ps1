function get_version( $pgoBranch )
{
    Write-Host $(git branch)
    Write-Host $(git name-rev --name-only HEAD)

    $forkSHA = $( git merge-base --fork-point $pgoBranch )

    if ( $LastExitCode -ne 0 )
    {
        throw "FAILED: git merge-base"
    }

    $forkDate = ( Get-Date -Date $( git log -1 $forkSHA --date=iso --pretty=format:"%ad" ) ).ToUniversalTime().ToString("yyMMddHHmm")

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