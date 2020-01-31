function replace_many ( $string, $dictionary )
{
    foreach ( $key in $dictionary.Keys )
    {
        $field = '$' + $key.ToString()
        $string = $string.Replace( $field, $dictionary[$key].ToString() )
    }

    return $string
}

function fill_out_template ( $inputPath, $outputPath, $dictionary )
{
    $replaced = replace_many ( Get-Content $inputPath ) $dictionary
    Write-Output $replaced | Set-Content $outputPath -Force | Out-Null
}