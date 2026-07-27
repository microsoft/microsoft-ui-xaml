# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License. See LICENSE in the project root for license information.

param(
    [string] $nugetContentFolder = $(Resolve-Path "$PSScriptRoot\..\packages")
)

Write-Host "Mock ARM64EC folders by copying x64"

function MockArm64EC-CopyFolder
{
    Param(
        [Parameter(Mandatory=$true)]
        [String] $src,
        [Parameter(Mandatory=$true)]
        [String] $dest
    )
    if (Test-Path -Path $dest -PathType Container) { 
        Write-Host "$dest exists"
    }
    if (Test-Path -Path $src -PathType Container) {
        robocopy /E /XC /XN /XO /NJH /NJS /NDL /NP /NFL $src $dest
        Write-Host "Robocopy operation exited with exit code: $LASTEXITCODE"
        $LASTEXITCODE = 0
    }
}

$packages = @("Microsoft.WindowsAppSDK.Foundation.TransportPackage")

foreach ($package in $packages) {
    $rootPath = (Get-ChildItem -Path $nugetContentFolder -Directory -Filter $package -Recurse).FullName
    Get-ChildItem -Path $rootPath -Directory | ForEach-Object {
        $nugetRoot = $_.FullName

        $libPath = $nugetRoot + "\lib\win10-x64"
        $libPathEC = $nugetRoot + "\lib\win10-arm64ec" 
        MockArm64EC-CopyFolder -src $libPath -dest $libPathEC

        $libNativePath = $nugetRoot + "\lib\native\win10-x64"
        $libNativePathEC = $nugetRoot + "\lib\native\win10-arm64ec" 
        MockArm64EC-CopyFolder -src $libNativePath -dest $libNativePathEC

        $runtimePath = $nugetRoot + "\runtimes\win10-x64"
        $runtimePathEC = $nugetRoot + "\runtimes\win10-arm64ec" 
        MockArm64EC-CopyFolder -src $runtimePath -dest $runtimePathEC
    }
}
exit 0