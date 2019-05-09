[CmdLetBinding()]
Param(
    [string]$Platform,
    [string]$Configuration,
    [switch]$IsLocal
)

$payloadDir = "HelixPayload\$Configuration\$Platform"

$repoDirectory = Join-Path (Split-Path -Parent $script:MyInvocation.MyCommand.Path) "..\..\"

if ($IsLocal)
{
    $nugetPackagesDir = Join-Path (Join-Path $env:USERPROFILE ".nuget") "packages"
    $nugetPackagesDirConnector = [System.IO.Path]::DirectorySeparatorChar
}
else
{
    $nugetPackagesDir = Join-Path (Split-Path -Parent $script:MyInvocation.MyCommand.Path) "packages"
    $nugetPackagesDirConnector = "."
}
 
# Create the payload directory. Remove it if it already exists.
If(test-path $payloadDir)
{
    Remove-Item $payloadDir -Recurse
}
New-Item -ItemType Directory -Force -Path $payloadDir

# Copy files from nuget packages
Copy-Item "$nugetPackagesDir\microsoft.windows.apps.test$($nugetPackagesDirConnector)1.0.181203002\lib\netcoreapp2.1\*.dll" $payloadDir
Copy-Item "$nugetPackagesDir\taef.redist.wlk$($nugetPackagesDirConnector)10.31.180822002\build\Binaries\$Platform\*" $payloadDir
Copy-Item "$nugetPackagesDir\taef.redist.wlk$($nugetPackagesDirConnector)10.31.180822002\build\Binaries\$Platform\CoreClr\*" $payloadDir
Copy-Item "$nugetPackagesDir\runtime.win-$Platform.microsoft.netcore.app$($nugetPackagesDirConnector)2.1.0\runtimes\win-$Platform\lib\netcoreapp2.1\*" $payloadDir
Copy-Item "$nugetPackagesDir\runtime.win-$Platform.microsoft.netcore.app$($nugetPackagesDirConnector)2.1.0\runtimes\win-$Platform\native\*" $payloadDir
New-Item -ItemType Directory -Force -Path "$payloadDir\.NETCoreApp2.1\"
Copy-Item "$nugetPackagesDir\runtime.win-$Platform.microsoft.netcore.app$($nugetPackagesDirConnector)2.1.0\runtimes\win-$Platform\lib\netcoreapp2.1\*" "$payloadDir\.NETCoreApp2.1\"
Copy-Item "$nugetPackagesDir\runtime.win-$Platform.microsoft.netcore.app$($nugetPackagesDirConnector)2.1.0\runtimes\win-$Platform\native\*" "$payloadDir\.NETCoreApp2.1\"
Copy-Item "$nugetPackagesDir\MUXCustomBuildTasks$($nugetPackagesDirConnector)1.0.38\tools\$platform\WttLog.dll" $payloadDir

function Copy-If-Exists
{
    Param($source, $destinationDir)

    if (Test-Path $source)
    {
        Write-Host "Copy from '$source' to '$destinationDir'"
        Copy-Item -Force $source $destinationDir
    }
    else
    {
        Write-Host "'$source' does not exist."
    }
}

if ($IsLocal)
{
    # Copy files from the 'drop' artifact dir
    Copy-Item "$repoDirectory\BuildOutput\$Configuration\$Platform\MUXControls.Test.TAEF\MUXControls.Test.dll" $payloadDir
    Copy-If-Exists "$repoDirectory\BuildOutput\$Configuration\$Platform\MUXControlsTestApp.TAEF\AppPackages\MUXControlsTestApp_Test\MUXControlsTestApp_Test\*" $payloadDir
    Copy-If-Exists "$repoDirectory\BuildOutput\$Configuration\$Platform\MUXControlsTestApp.TAEF\AppPackages\MUXControlsTestApp_Test\MUXControlsTestApp_Test\Dependencies\$Platform\*" $payloadDir
    Copy-If-Exists "$repoDirectory\BuildOutput\$Configuration\$Platform\IXMPTestApp.TAEF\AppPackages\IXMPTestApp_Test\*" $payloadDir
    Copy-If-Exists "$repoDirectory\BuildOutput\$Configuration\$Platform\IXMPTestApp.TAEF\AppPackages\IXMPTestApp_Test\$Platform\*" $payloadDir
    # NuGet test artifacts
    Copy-If-Exists "$repoDirectory\BuildOutput\$Configuration\$Platform\MUXControls.ReleaseTest.TAEF\MUXControls.ReleaseTest.dll" $payloadDir
    Copy-If-Exists "$repoDirectory\BuildOutput\$Configuration\$Platform\NugetPackageTestApp\AppPackages\NugetPackageTestApp_Test\*" $payloadDir
    Copy-If-Exists "$repoDirectory\BuildOutput\$Configuration\$Platform\NugetPackageTestApp\AppPackages\NugetPackageTestApp_Test\Dependencies\$Platform\*" $payloadDir
    Copy-If-Exists "$repoDirectory\BuildOutput\$Configuration\$Platform\NugetPackageTestAppCX\AppPackages\NugetPackageTestAppCX_Test\*" $payloadDir
    Copy-If-Exists "$repoDirectory\BuildOutput\$Configuration\$Platform\NugetPackageTestAppCX\AppPackages\NugetPackageTestAppCX_Test\Dependencies\$Platform\*" $payloadDir
}
else
{
    # Copy files from the 'drop' artifact dir
    Copy-Item "$repoDirectory\Artifacts\drop\$Configuration\$Platform\Test\MUXControls.Test.dll" $payloadDir
    Copy-If-Exists "$repoDirectory\Artifacts\drop\$Configuration\$Platform\AppxPackages\MUXControlsTestApp_Test\*" $payloadDir
    Copy-If-Exists "$repoDirectory\Artifacts\drop\$Configuration\$Platform\AppxPackages\MUXControlsTestApp_Test\Dependencies\$Platform\*" $payloadDir
    Copy-If-Exists "$repoDirectory\Artifacts\drop\$Configuration\$Platform\AppxPackages\IXMPTestApp_Test\*" $payloadDir
    Copy-If-Exists "$repoDirectory\Artifacts\drop\$Configuration\$Platform\AppxPackages\IXMPTestApp_Test\Dependencies\$Platform\*" $payloadDir
    # NuGet test artifacts
    Copy-If-Exists "$repoDirectory\Artifacts\drop\$Configuration\$Platform\Test\MUXControls.ReleaseTest.dll" $payloadDir
    Copy-If-Exists "$repoDirectory\Artifacts\drop\$Configuration\$Platform\AppxPackages\NugetPackageTestApp_Test\*" $payloadDir
    Copy-If-Exists "$repoDirectory\Artifacts\drop\$Configuration\$Platform\AppxPackages\NugetPackageTestApp_Test\Dependencies\$Platform\*" $payloadDir
    Copy-If-Exists "$repoDirectory\Artifacts\drop\$Configuration\$Platform\AppxPackages\NugetPackageTestAppCX_Test\*" $payloadDir
    Copy-If-Exists "$repoDirectory\Artifacts\drop\$Configuration\$Platform\AppxPackages\NugetPackageTestAppCX_Test\Dependencies\$Platform\*" $payloadDir
}

# Copy files from the repo
New-Item -ItemType Directory -Force -Path "$payloadDir\scripts"
Copy-Item "build\helix\ConvertWttLogToXUnit.ps1" "$payloadDir\scripts"
Copy-Item "build\helix\ConvertWttLogToXUnit.cs" "$payloadDir\scripts"
Copy-Item "build\helix\runtests.cmd" $payloadDir
Copy-Item "build\helix\InstallTestAppDependencies.ps1" "$payloadDir\scripts"