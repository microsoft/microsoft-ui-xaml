# Getting started with building WinUI from GitHub

> [!IMPORTANT]  
> The WinUI OSS effort is still in progress. See the [WinUI OSS Update post](https://github.com/microsoft/microsoft-ui-xaml/discussions/10700) to check the latest status. Also see [Current limitations](#current-limitations).

## Preparing your environment

To build WinUI, you need to set up your development environment and clone the WinUI repository.
Follow these steps:

1. Install Git for Windows from [git-scm.com](https://git-scm.com/download/win), if you don't already
   have it.

2. Clone the WinUI repository. It is recommended to clone into a short path, such as `C:\winui3`:
   ```
   git clone https://github.com/microsoft/microsoft-ui-xaml.git C:\winui3
   cd C:\winui3
   ```

3. Switch to the `winui3/main` branch:
   ```
   git checkout winui3/main
   ```

4. Install the necessary MSBuild / Visual Studio 2022 dependencies. If you already have VS 2022
   ([see notes](#other-notes)) installed, this command will ensure you have the required components:
   ```
   cd C:\winui3\src
   .\OneTimeSetup.cmd -Install MSBuild
   ```

   Click the various "Yes", "Continue", and "Modify"/"Install" buttons as needed to complete
  the installation. When installation completes, close the Visual Studio Installer window.

## Building WinUI

Starting with a command prompt or PowerShell at the root of your WinUI repo:

1. Switch into the `src` directory:
   ```
   cd src
   ```

2. Initialize to build ([details](#initialize-cmd-or-powershell-with-support-for-vs-build-tools)):
   ```
   init.cmd
   ```
   Or, for PowerShell:
   ```
   .\init.ps1
   ```

3. Build the project:
   ```
   .\build.cmd
   ```

If the build was successful, the output should show this at the end:
   ```
   ---
   BUILD SUCCEEDED.
   ---
   ```

See [below](#build-failures) for more information on investigating build failures.

To do a clean build, you can run add a `/c` parameter:
   ```
   .\build.cmd /c
   ```

## Build output

After a successful build, you can find the product binaries in the output packaging
folder `%BuildArtifactsDir%\packaging\%Configuration%\runtimes\win10-%Platform%\native`.
For example:
```
C:\winui3\src>dir %BuildArtifactsDir%\packaging\%Configuration%\runtimes\win10-%Platform%\native
 Directory of C:\winui3\src\BuildOutput\packaging\Debug\runtimes\win10-x64\native

Microsoft.Internal.FrameworkUdk.dll
Microsoft.UI.Xaml
Microsoft.UI.Xaml.Controls.dll
Microsoft.UI.Xaml.Controls.pri
Microsoft.ui.xaml.dll
...
```

### Using the built WinUI binaries in your own project

To use the built WinUI binaries in your own project, you can either copy the built binaries into
your project's output folder after building your project, or you can build against the component
package built as part of the WinUI build.

#### Option 1: Copying built binaries into your project output folder

In this option, you would copy the built WinUI binaries from the output packaging folder to your
project's output folder after building your project. To do this, start with an app which is both
[unpackaged](https://learn.microsoft.com/windows/apps/windows-app-sdk/deploy-unpackaged-apps#use-the-windows-app-sdk-runtime)
and
[self-contained](https://learn.microsoft.com/windows/apps/package-and-deploy/self-contained-deploy/deploy-self-contained-apps).
This can be done by setting the following properties in your app's project file:

```xml
  <PropertyGroup>
    <WindowsPackageType>None</WindowsPackageType>
    <WindowsAppSDKSelfContained>true</WindowsAppSDKSelfContained>
  </PropertyGroup>
```

Build the project using [WindowsAppSDK 2.0-experimental3](https://www.nuget.org/packages/Microsoft.WindowsAppSDK/2.0.0-experimental3),
then replace the WinUI binaries in the output folder of your app with the ones you built from source.
You can then just run your app as normal to use the WinUI binaries you built.

You could also edit your app project to do this copy automatically, such as by adding this:

```xml
  <ItemGroup>
    <None Include="C:\winui3\src\BuildOutput\packaging\Debug\runtimes\win10-$(Platform)\native\**\*.*">
      <Visible>false</Visible>
      <CopyToOutputDirectory>PreserveNewest</CopyToOutputDirectory>
    </None>
  </ItemGroup>
 ```

Update the path in the `Include` attribute to point to your WinUI build output folder.

#### Option 2: Referencing the built `Microsoft.WindowsAppSDK.WinUI` package

In this option, you would reference the `Microsoft.WindowsAppSDK.WinUI` NuGet package built as part
of the WinUI build process. To do this, start with any app (either packaged or unpackaged), and add
this `nuget.config` file to the root of your app project (next to your `.csproj` or `.vcxproj` file):

```xml
<configuration>
  <config>
    <clear />
    <add key="globalPackagesFolder" value="$\..\packages" />
    <add key="repositoryPath" value="$\..\packages" />
  </config>
  <packageSources>
    <add key="packagestore" value="C:\winui3\src\PackageStore" />
  </packageSources>
</configuration>
```

Update the path in the `packagestore` value to point to the PackageStore folder in your WinUI repo.

Adding this config does two things. First, it adds the location of the 'PackageStore' directory from
your local WinUI repo to the list of locations that nuget.exe will search for packages. This will
allow the Visual Studio project to target this local build.

Second, it updates nuget to cache all its packages in the app folder instead of using the global
package cache from the machine (e.g. `C:\Users\alias\.nuget\packages`). This makes it easier to
update the package and avoids caching it globally where it may affect other projects.

##### Update the Package References

In Visual Studio, use Tools -> Nuget Package Manager -> Manage Nuget Packages for Solution.
Make sure you have the `Include prerelease` checkbox checked to ensure that the local package shows
up. Select and uninstall all `Microsoft.WindowsAppSDK.*` packages other than `Base`, `Foundation`,
`InteractiveExperiences`, and `WinUI`. Select `Microsoft.WindowsAppSDK.WinUI` and install
your local package, which should be version `3.0.0-dev`.

If you prefer, instead of using the UI, you can also do this by modifying the .csproj to remove
any `<PackageReference>` element for the full `Microsoft.WindowsAppSDK` package and then add or
update a `<PackageReference>` element for `Microsoft.WindowsAppSDK.WinUI`, like this:

```xml
  <ItemGroup>
    <PackageReference Include="Microsoft.WindowsAppSDK.WinUI" Version="3.0.0-dev" />
  </ItemGroup>
```

Rebuild and launch your app with F5. 

##### Picking up new WinUI builds

Each time you do a new build of WinUI from source, you will need to delete the
`packages\microsoft.windowsappsdk.winui` folder from your app project folder to force NuGet to
restore the updated package.

## Current limitations

The WinUI OSS effort is still in progress. Phase 2 of the plan outlined in the
[WinUI OSS Update post](https://github.com/microsoft/microsoft-ui-xaml/discussions/10700) is now
complete and Phase 3 is underway. Current limitations include:
* Only the product binaries[^1] can be built. No tests or samples can be built yet, including
  the test code already in the repo in the `controls\` subtree.
* Related to the previous, various sln files and project files in the repo cannot yet be built,
  including `Microsoft.UI.Xaml.sln` (`Microsoft.UI.Xaml.OSS.sln` is available instead) and
  `MUXControls.sln`.
* The product binaries built from source cannot yet be easily used during an app build without
  manually replacing the binaries in the app output folder as described above.
* The XAML Compiler is not yet fully shared and therefore isn't buildable. The build uses a
  prebuilt version instead.
* Most unittest and integration test code is not yet open sourced, so cannot be built or run.
* There is no support yet to run any builds/tests in a CI system.

[^1]: The product binaries built by this repo are the `Microsoft.UI.Xaml.*.dll` files,
excluding `Microsoft.UI.Xaml.Internal.dll` (which will not be OSS).
 
## Additional Info

### Initialize CMD or PowerShell with support for VS build tools

At the CMD prompt, in the `src\` folder off the root of the repo, run (for an x64chk build):
```
init.cmd
```

Or in PowerShell, run:
```
.\init.ps1
```

> Note:  If you get an error saying it can't find msbuild, this is likely due to a pending Visual Studio update. To see
if this is the case, try launching Visual Studio -- you may see a message saying it needs to reboot to finish updating.
If so, try that first, then retry the init command.

This script restores NuGet packages and initializes your build environment with the proper environment variables for
specifying which platform and configuration you are building.

> Any time you do a `git pull` you also need to re-run init.cmd

The default environment is `x64` `Debug`. Below is a list of other possible Platform/Configuration combinations and the
init command that sets them up.

| Platform  | Configuration | Init command       |
| :-------- | :----------   | :----------------- |
| x86       | Debug         | init.cmd  x86chk   |
| x86       | Release       | init.cmd  x86fre   |
| x64       | Debug         | init.cmd           |
| x64       | Debug         | init.cmd  x64chk   |
| x64       | Release       | init.cmd  x64fre   |
| arm64     | Debug         | init.cmd  arm64chk |
| arm64     | Release       | init.cmd  arm64fre |

### Build failures

Build output prints to screen, as well as to a `.binlog` file. To
see this output in a more human readable form, use the [MSBuild Log Viewer](#install-msbuild-log-viewer).

#### error C1076: compiler limit: internal heap limit reached / error C1060: compiler is out of heap space

These errors occasionally happen during the build of Microsoft.UI.Xaml.Controls.vcxproj:
```
error C1076: compiler limit: internal heap limit reached
error C1060: compiler is out of heap space
```

One option is to simply retry an incremental build by running `build.cmd` again. This simple
approach often works, but it may take multiple attempts.

Another option, which is more reliable, is to limit the number of parallel compiler processes
by setting the `CL` environment variable before building. For example, in CMD:
```
set CL=/MP2
```
And then run the build again with `build.cmd`.

### Install MSBuild Log Viewer

When investigating build failures, the [MSBuild Structure Log Viewer](http://msbuildlog.com/) can be
very helpful. This tool makes the build output (logs, errors, etc) much more readable, by presenting
the `.binlog` produced by the build in a human readable way. The link above provides a download link
as well as some basics of how to use the tool. Or you can install the tool by running the following
command at the root of the repo:
```
.\OneTimeSetup.cmd -Install LogViewer
```

### Other notes

* `OneTimeSetup.cmd` currently assumes Visual Studio 2022. However, this project can also be built
  with Visual Studio 2026 if all necessary components are installed.
