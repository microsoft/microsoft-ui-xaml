# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

# This script is used with BuildAll in the WindowsAppSDK aggregate repo.

Param(
    [string]$Platform = "x86,x64,arm64",
    [string]$Configuration = "release,debug",
    [string]$LocalPackagesPath = $null,
    [string]$UpdateVersionDetailsPath = $null
)

$packageVersion = "3.0.0-dev"

foreach ($plat in $Platform.Split(","))
{
    foreach ($config in $Configuration.Split(","))
    {
        if ($config -eq "release") {
            $config = "fre"
        } elseif ($config -eq "debug") {
            $config = "chk"
        }
        .\BuildTransportPackageCmdWrapper.cmd ($plat+$config) $packageVersion
    }
}

$packageName = "Microsoft.ProjectReunion.WinUI.TransportPackage"
Copy-Item -Path "PackageStore\$packageName.$packageVersion.nupkg" -Destination $LocalPackagesPath
&"$UpdateVersionDetailsPath" -dependencyName $packageName -dependencyVersion $packageVersion