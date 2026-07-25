function Get-ExpandedFullPath( $path, $baseDirectory )
{
    $result = [System.Environment]::ExpandEnvironmentVariables( $path )

    if ( [System.IO.Path]::IsPathRooted( $result ) )
    {
        return [System.IO.Path]::GetFullPath( $result )
    }
    else
    {
        return [System.IO.Path]::GetFullPath( ( Join-path $baseDirectory $result ) )
    }
}

function Get-ResolvedConfiguration( $configFilePath )
{
    $configuration = ( Get-Content $configFilePath | ConvertFrom-JSON )
    $baseDirectory = [System.IO.Path]::GetDirectoryName( $configFilePath )

    $configuration.AppsFilePath = Get-ExpandedFullPath $configuration.AppsFilePath $baseDirectory
    $configuration.ScenariosFilePath = Get-ExpandedFullPath $configuration.ScenariosFilePath $baseDirectory
    $configuration.ProfilesFilePath = Get-ExpandedFullPath $configuration.ProfilesFilePath $baseDirectory
    $configuration.ShiftsDirectory = Get-ExpandedFullPath $configuration.ShiftsDirectory $baseDirectory
    $configuration.InstallersRootDirectory = Get-ExpandedFullPath $configuration.InstallersRootDirectory $baseDirectory
    $configuration.CsvCollator.ExperimentsDirectory = Get-ExpandedFullPath $configuration.CsvCollator.ExperimentsDirectory $baseDirectory
    $configuration.CsvCollator.ProcessedDataArchiveDirectory = [System.Environment]::ExpandEnvironmentVariables( $configuration.CsvCollator.ProcessedDataArchiveDirectory )

    return $configuration
}