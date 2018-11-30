Param(
    [Parameter(Position=0)]
    [string[]]$args,
    [Parameter(Mandatory=$True)]
    [string]$vhdpath = $null,
    [string]$name = "Emulator",
    [Alias("video")]
    [string]$Resolution = "768x1280",
    [string]$DiagonalSize = $null,
    [switch]$Fresh,
    [switch]$NoGpu,
    [switch]$NoStart,
    [Alias("softButtons")]
    [switch]$NavBar,
    [int]$memsize = 512
    )

foreach ($i in $args)
{
    if (($i -eq "/480p") -or ($i -eq "-480p"))
    {
        $Resolution="480x800"
    }    
    elseif (($i -eq "/1080p") -or ($i -eq "-1080p"))
    {
        $Resolution="1080x1920"
    }
    elseif ($i -eq "/cityman")
    {
        $Resolution="1440x2560"
        $DiagonalSize="5.7"
    }
    else
    {
        Write-Host "Unknown argument(s): $i" -ForegroundColor Red
        Exit 1
    }
}

echo "VHD is $vhdpath";
Write-Host

# Code for the diff disk argument which we aren't using right now:
# /createDiffDisk $vhdSansExtension.$resString.diff.vhd /snapshot

$xdeBasePath = "c:\Program Files (x86)\Microsoft XDE"
$xdeDirs = gci $xdeBasePath | Sort -Property LastWriteTime -Descending | Where-Object { !$_.Name.Contains("8.1") }

if ($xdeDirs.Length -eq 0)
{
    Write-Host "No installed XDE found in '$xdeBasePath'" -ForegroundColor Red
}
else
{
     if ($xdeDirs.Length -gt 1) { Write-Host "Multiple dirs found under '$xdeBasePath', using latest" -ForegroundColor Yellow }
     $XDEPath = Join-Path $xdeDirs[0].FullName -ChildPath "xde.exe";
}

Write-Host "Using $XDEPath";

$diffDiskPath = $vhdpath -replace ".vhd$",".diff.$Resolution.vhd"

$arguments = "/com1 $name /name $name /showName /video $Resolution /memsize $memsize"
if (!(Test-Path $diffDiskPath) -or $Fresh)
{
    $arguments += " /vhd $vhdpath /createDiffDisk $diffDiskPath"
}
else
{
    $arguments += " /vhd $diffDiskPath"
}

if ($DiagonalSize)
{
   $arguments += " /diagonalSize $DiagonalSize";
}

if ($NoGpu -or [environment]::OSVersion.Version.Major -eq 10)
{
   $arguments += " /noGpu";
}

if ($NoStart)
{
   $arguments += " /noStart";
}

if ($NavBar)
{
   $arguments += " /softButtons";
}

Write-Host Powershell command: Start-Process -FilePath $XDEPath -ArgumentList "$arguments"

$doesVHDExist = Test-Path $vhdpath;
if (!$doesVHDExist)
{
    Write-Host "There is no file at $vhdpath!" -ForegroundColor Red;
    Exit;
}

Start-Process -FilePath $XDEPath -ArgumentList "$arguments"
