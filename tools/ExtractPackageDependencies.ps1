Param(
    [string]$sourceFile,
    [string]$platform,
    [string]$outputFile)

# This is for going against AppxManifest.xml but that seems inconsistently generated.
# $dependencies = select-string -path $sourceFile -Pattern 'PackageDependency Name="(.*?)"' | % { $_.Matches.Groups[1].Value }

$dependencies = select-string -path $sourceFile -Pattern '<AppxLocation>(.*?)([^\\]+?)</AppxLocation>' | % { $_.Matches.Groups[2].Value } | Where { !($_ -imatch "(ARM|x86|x64)") -or ($_ -imatch $platform) } | Sort-Object | Get-Unique

$dependencies > $outputFile
