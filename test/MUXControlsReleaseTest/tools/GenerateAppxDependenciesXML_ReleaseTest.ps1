Param(
    [string]$sourceFile,
    [string]$outputFile,
    [string]$BUILDPLATFORM)

Write-Output @"
<Package xmlns="urn:Microsoft.WindowsPhone/PackageSchema.v8.00"
   Owner="Microsoft"
   OwnerType="Microsoft"
   ReleaseType="Test"
   Component="MUXControlsReleaseTest.FrameworkPackageDependencies">
   <Components>
      <OSComponent>
         <Files>
"@ > $outputFile

$appxRoot = Split-Path $sourceFile

foreach ($dependency in Get-Content $sourceFile)
{
    $relativeSourcePath = "Dependencies\$BUILDPLATFORM\$dependency"
    Write-Host "Checking '$appxRoot\$relativeSourcePath'"
    if (Test-Path "$appxRoot\$relativeSourcePath")
    {
        Write-Output ('           <File Source="$(_RELEASEDIR)\NugetPackageTestApp\AppPackages\NugetPackageTestApp_Test\'+$relativeSourcePath+'" DestinationDir="$(_BINPLACEDIR)" />') >> $outputFile
    }
}

Write-Output @"
         </Files>
      </OSComponent>
   </Components>
</Package>
"@ >> $outputFile