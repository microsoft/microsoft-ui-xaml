function Run-Tool(
    [string] $toolPath,
    [string[]] $toolArgs )
{
    $toolName = [System.IO.Path]::GetFileNameWithoutExtension( $toolPath )

    Log-Command "$toolPath $toolArgs"
    Log-BeginGroup "$toolName output"

    & "$toolPath" $toolArgs

    if ( $LastExitCode -NE 0 )
    {
        Log-ErrorAndThrow "$toolName failed and returned $LastExitCode."
    }

    Log-EndGroup "$toolName output"
}