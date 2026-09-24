Param(
  [Parameter(Mandatory = $true, Position = 1)]
  [string] $dest,
  [Parameter(Mandatory = $true, Position = 2)]
  [string] $language,
  [Parameter(Mandatory = $true, Position = 3)]
  [string] $version_str)

if ($language -eq "C++")
{
    $ext = "rc"
}
Elseif ($language -eq "C#")
{
    $ext = "cs"
}
Else
{
    $ext = "txt"
}

function make_resource_file($filename, $content)
{
    $replaced = $content.replace('$version_str', $version_str)
    Write-Output $replaced | Set-Content $filename -Force | Out-Null
}

New-Item -ItemType Directory -Force -Path $dest | Out-Null
make_resource_file $dest\VersionInfo.$ext ( Get-Content VersionInfo.$ext.template )