[CmdLetBinding()]
Param(
    [string]$Platform,
    [string]$Configuration
)

$payloadDir = "HelixPayload\$Configuration\$Platform"

$repoDirectory = Join-Path (Split-Path -Parent $script:MyInvocation.MyCommand.Path) "..\..\"
$nugetPackagesDir = Join-Path (Split-Path -Parent $script:MyInvocation.MyCommand.Path) "packages"
 
# Create the payload directory. Remove it if it already exists.
If(test-path $payloadDir)
{
    Remove-Item $payloadDir -Recurse
}
New-Item -ItemType Directory -Force -Path $payloadDir

# Copy files from nuget packages
Copy-Item "$nugetPackagesDir\microsoft.windows.apps.test.1.0.181203002\lib\netcoreapp2.1\*.dll" $payloadDir
Copy-Item "$nugetPackagesDir\taef.redist.wlk.10.31.180822002\build\Binaries\$Platform\*" $payloadDir
Copy-Item "$nugetPackagesDir\taef.redist.wlk.10.31.180822002\build\Binaries\$Platform\CoreClr\*" $payloadDir
Copy-Item "$nugetPackagesDir\runtime.win-$Platform.microsoft.netcore.app.2.1.0\runtimes\win-$Platform\lib\netcoreapp2.1\*" $payloadDir
Copy-Item "$nugetPackagesDir\runtime.win-$Platform.microsoft.netcore.app.2.1.0\runtimes\win-$Platform\native\*" $payloadDir
New-Item -ItemType Directory -Force -Path "$payloadDir\.NETCoreApp2.1\"
Copy-Item "$nugetPackagesDir\runtime.win-$Platform.microsoft.netcore.app.2.1.0\runtimes\win-$Platform\lib\netcoreapp2.1\*" "$payloadDir\.NETCoreApp2.1\"
Copy-Item "$nugetPackagesDir\runtime.win-$Platform.microsoft.netcore.app.2.1.0\runtimes\win-$Platform\native\*" "$payloadDir\.NETCoreApp2.1\"
Copy-Item "$nugetPackagesDir\MUXCustomBuildTasks.1.0.38\tools\$platform\WttLog.dll" $payloadDir

function Copy-If-Exist
{
    Param($source, $destinationDir)

    if (Test-Path $source)
    {
        Copy-Item -Force $source $destinationDir
    }
}

# Copy files from the 'drop' artifact dir
Copy-If-Exist "$repoDirectory\Artifacts\drop\$Configuration\$Platform\Test\MUXControls.Test.dll" $payloadDir
Copy-If-Exist "$repoDirectory\Artifacts\drop\$Configuration\$Platform\AppxPackages\MUXControlsTestApp_Test\*" $payloadDir
Copy-If-Exist "$repoDirectory\Artifacts\drop\$Configuration\$Platform\AppxPackages\MUXControlsTestApp_Test\Dependencies\$Platform\*" $payloadDir
Copy-If-Exist "$repoDirectory\Artifacts\drop\$Configuration\$Platform\AppxPackages\IXMPTestApp_Test\*" $payloadDir
Copy-If-Exist "$repoDirectory\Artifacts\drop\$Configuration\$Platform\AppxPackages\IXMPTestApp_Test\Dependencies\$Platform\*" $payloadDir
# NuGet tests
Copy-If-Exist "$repoDirectory\Artifacts\drop\$Configuration\$Platform\AppxPackages\NugetPackageTestApp_Test\*" $payloadDir
Copy-If-Exist "$repoDirectory\Artifacts\drop\$Configuration\$Platform\AppxPackages\NugetPackageTestApp_Test\Dependencies\$Platform\*" $payloadDir
Copy-If-Exist "$repoDirectory\Artifacts\drop\$Configuration\$Platform\AppxPackages\NugetPackageTestAppCX_Test\*" $payloadDir
Copy-If-Exist "$repoDirectory\Artifacts\drop\$Configuration\$Platform\AppxPackages\NugetPackageTestAppCX_Test\Dependencies\$Platform\*" $payloadDir

# Copy files from the repo
Copy-Item "build\helix\runtests.cmd" $payloadDir
New-Item -ItemType Directory -Force -Path "$payloadDir\scripts"
Copy-Item "build\helix\ConvertWttLogToXUnit.ps1" "$payloadDir\scripts"
Copy-Item "build\helix\ConvertWttLogToXUnit.cs" "$payloadDir\scripts"
Copy-Item "build\helix\InstallTestAppDependencies.ps1" "$payloadDir\scripts"