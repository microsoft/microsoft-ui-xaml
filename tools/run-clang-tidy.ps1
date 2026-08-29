Param(
    [switch]$continue = $false
)

$doneFilename = ".\done.tmp"
$compileDb = ".\compile_commands.json"

$content = Get-Content $compileDb | ConvertFrom-JSON

if ( ( Test-Path $doneFilename -PathType Leaf ) -and !$continue )
{
    $gotResponse = $false

    while ( !$gotResponse )
    {
        write-host "Looks like your previous task wasn't finished."
        write-host "(C) - continue previous task"
        write-host "(R) - remove old results and restart"

        $choice = read-host "Choose and press Enter: "

        switch ( $choice )
        {
            C {
                $continue = $true
                $gotResponse = $true
            }

            R {
                $gotResponse = $true
            }
        }
    }
}

if ( $continue )
{
    $done = Get-Content $doneFilename
}
else
{
    $done = @()
    New-Item -ItemType File $doneFilename -Force | Out-Null
}

foreach ($obj in $content) {
    $path = [System.IO.Path]::GetFullPath( ( Join-Path $obj.directory -ChildPath $obj.file ) )

    if ( ! ( $path -in $done ) )
    {
        $args = @(
            "-p=.",
            "-checks=-*,modernize-use-override",
            "-header-filter=.*\\xcp\\.*",
            "--fix",
            $path)

        write-host $path

        & clang-tidy $args

        if ( $lastexitcode -ne 0 )
        {
            write-host "Error in $path."
            write-host "Fix errors and run with -continue switch to restart."
            exit 1
        }

        $done += $path
        Add-Content $doneFilename -Value $path
    }
}

Remove-Item $doneFilename