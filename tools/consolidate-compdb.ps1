$compileDb = ".\compile_commands.json"

$doc = @()

Get-ChildItem ".\compdb_*.json.tmp" -File -Recurse | foreach {
    $obj = ( Get-Content $_ ) -replace ",$" | ConvertFrom-JSON

    $obj.directory = [System.IO.Path]::GetFullPath( $obj.directory )
    $obj.PSObject.properties.remove('output')

    $nextIsPath = $false

    $newArgs = @()

    foreach ( $arg in $obj.arguments )
    {
        if ( $nextIsPath )
        {
            $arg = $arg -replace "\\+$"

            if ( [System.IO.Path]::IsPathRooted( $arg ) )
            {
                $arg = [System.IO.Path]::GetFullPath( $arg )
            }

            $nextIsPath = $false
        }
        elseif ( $arg -eq "-I" )
        {
            $nextIsPath = $true
        }
        elseif ( $arg.StartsWith( "-clang:-MJ" ) )
        {
            continue
        }

        $newArgs += $arg
    }

    $obj.arguments = $newArgs
    $doc += $obj
}

New-Item -ItemType File $compileDb -Force | Out-Null
$doc | ConvertTo-JSON -Compress | % { $_ -replace "\},", "},`n" } | Add-Content $compileDb