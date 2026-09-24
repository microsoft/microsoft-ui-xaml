Param(
  [Parameter(Mandatory = $true, Position = 1)]
  [string] $version,
  [Parameter(Mandatory = $true, Position = 2)]
  [string] $get_git_stamp)

if ([System.Convert]::ToBoolean($get_git_stamp))
{
    $branch = ( & git rev-parse --abbrev-ref HEAD )
    $last_commit = ( & git log -1 --date=format:%y%m%d --pretty=format:"%cd_%h" $env:RepoRoot\dxaml\xcp )
    $clean = [string]::IsNullOrWhiteSpace( ( & git status -s $env:RepoRoot\dxaml\xcp ) )
    $dirty_str = If ($clean) { "" } Else { "[DIRTY]" }
    $version_str = "${version}_${branch}_${last_commit}${dirty_str}"
}
Else
{
    $version_str = "${version}"
}

$version_str = $version_str.replace("/", "_")

Write-Output $version_str