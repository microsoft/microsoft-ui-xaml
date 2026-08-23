# Developer Guide

This guide provides instructions on how to build the repo. If you encouter any errors in building WinUI or the WinUI Gallery, refer to [common errors FAQ](../common-errors-FAQ.md)
Documentation related to concepts and code architecture of the repo can be found here:
[Code architecture](../design-notes/readme.md). It contains high level concepts, important design decisions and
coding related information to help get started with the repository.

## Table of Contents

- [TL;DR for setup/build](#tldr-for-setupbuild)
- [Preparing the machine for building](#preparing-the-machine-for-building)
  - [Disk Space](#disk-space)
  - [Swap space: Preventing out-of-memory errors](#swap-space-preventing-out-of-memory-errors)
  - [Install Visual Studio](#install-visual-studio)
    - [Import the vsconfig file](#import-the-vsconfig-file)
  - [Install Git](#install-git)
  - [Install MSBuild Log Viewer](#install-msbuild-log-viewer)
  - [Clone the WinUI repo](#clone-the-winui-repo)
- [Building WinUI](#building-winui)
  - [Initialize CMD or PowerShell with support for VS build tools](#initialize-cmd-or-powershell-with-support-for-vs-build-tools)
    - [Configuring the .NET version](#configuring-the-net-version)
  - [C++ Language Standard](#c-language-standard)
  - [Run a build command](#run-a-build-command)
    - [Building specific directories](#building-specific-directories)
    - [Advanced build information](#advanced-build-information)
- [Next steps](#next-steps)

## TL;DR for setup/build

* Install the latest Visual Studio, then the .vsconfig file [(details)](#install-visual-studio)
* Clone the repo [(details)](#clone-the-winui-repo)
* In cmd, from the repo directory, run `init.cmd`. Or in PowerShell, run `init.ps1`. [(details)](#initialize-cmd-or-powershell-with-support-for-vs-build-tools)
* Run `build.cmd` [(details)](#building-winui)

## Preparing the machine for building

### Disk Space

A build of repo consumes about 80GB of disk space. This is for a single build flavor and includes the size of
the repo. If you are creating a VM to build, you should create it with a disk of at least 120 GB (to account for the OS
and the Visual Studio install). If you want to build more than one flavor, you will need further space to account for that.

### Swap space: Preventing out-of-memory errors

Recent releases of Visual Studio are producing ever larger PCH files, which can easily exhaust system resources,
especially when building on a laptop.  Setting your paging file to at least 64GB should resolve out of memory build errors,
however this can depend on machine configuration. The more cores your have, the higher the memory usage. If you have more
than 24 cores you may need to increase the page file to be even more than 64GB.
If you hit out of memory issues when building the pch files, try increasing the page file size.
[More info](https://devblogs.microsoft.com/cppblog/precompiled-header-pch-issues-and-recommendations/).

Some notes on this:
* It is the page file on the OS drive that matters for these purposes since that is what is used for fulfilling VirtualAlloc()
requests.
* The page file needs to be fixed size with both min and max set to the same value.
* Close any applications that consume large amounts of memory when trying to build.

### Install Visual Studio

You need to have Visual Studio both to build the Xaml product and to write a WinUI3 app.

Install using the Visual Studio Installer, which you can get from
[the archives here](https://docs.microsoft.com/en-us/visualstudio/releases/2022/release-history).
Select the latest version of Visual Studio **Enterprise** edition.
* There's a vsconfig file in the repo that you should import into the Visual Studio Installer. It will ensure your
Visual Studio installation has all the necessary components. See the
[Import the vsconfig file](#import-the-vsconfig-file) section below for instructions on vsconfig.
* It's fine to have multiple versions of Visual Studio installed, they can live side-by-side.
* It's not OK to have "Visual Studio Build Tools" installed. If you have this installed, it will show up in the
'Installed' tab in the Visual Studio Installer.

#### Import the vsconfig file

We have a .vcsconfig file in the root of the repository that will ensure your Visual Studio installation has all the
necessary components and workloads installed. You can install this through the Visual Studio Installer.

1) Download the [.vsconfig file](../../.vsconfig) from the root of the repo
2) When you download the file it will be named "vsconfig" (without the dot). Rename to ".vsconfig"
3) Open the Visual Studio Installer, select “More” on your product card and then "Import configuration"
4) Specify the .vsconfig file location and select “Review Details”

For more information, see the
[official documentation](https://docs.microsoft.com/en-us/visualstudio/install/import-export-installation-configurations#import-a-configuration).

### Install Git

If you don't have it already, [install Git](https://git-scm.com/downloads). Be sure to restart your CMD after
installing Git and cloning the repo.

### Install MSBuild Log Viewer

While technically optional, you should install the [MSBuild Structure Log Viewer](http://msbuildlog.com/). This tool
makes the build output (logs, errors, etc) much more readable, by presenting the `.binlog` produced by the build in a
human readable way. The link above provides a download link, as well as some basics of how to use the tool.

### Clone the WinUI repo

Clone the repo by running the following command:

```
git clone https://github.com/microsoft/microsoft-ui-xaml.git <destination_directory>
```

> Note: You should make sure to clone WinUI to a place where the path is relatively short (< 10 characters). The
default VS location of %USERDIR%\source\repos will lead to build problems due to file paths being too long, as Windows
has a path length limit of 260 characters. Therefore it is helpful to keep your repo path as short as possible
(ie. within "C:\\").

While running both “git clone” and “init” an authentication screen may pop up in which you will have to enter your
Microsoft credentials.


## Building WinUI

### Initialize CMD or PowerShell with support for VS build tools

At the CMD prompt, in the root of the repo, run (for an x64chk build):
```
init.cmd
```

Or in PowerShell, run:
```
init.ps1
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

Note that the terms `chk` and `fre` are  currently used throughout the build to refer to `Debug` and `Release`
configurations, respectively.

#### Configuring the .NET version
`init.cmd` is also responsible for controlling what .NET version the build targets. By default, it is .NET 6.
However, it can be controlled by passing in either `net6` or `net7` as an argument (e.g. `init.cmd net6` will
initialize the build to target .NET 6, `init.cmd x86fre net7` will initialize the build for x86fre targeting
.NET 7, etc.). In the pipeline, this behavior is controlled using the `dotNetFrameworkVersion` pipeline
variable with the same argument (e.g. setting `dotNetFrameworkVersion` to `net6` will target .NET 6).

To control the .NET version being targeted, `init.cmd` calls the
[SetupDotNetFiles.cmd](../../scripts/init/SetupDotNetFiles.cmd) script to deploy relevant files for targeting either .NET 6 or .NET 7.

Additionally, even when targeting .NET 6, the `Microsoft.WinUI` projection dll
produced by CSWinRT will always target .NET 6, since it ships with the Windows
App SDK and must support .NET 6.

### C++ Language Standard

Most of the projects in this repo should be compiling as C++20.  The most notable exception is microsoft.ui.xaml.dll and the projects
sharing props/targets files with it.  Any projects using C++/CX are and will remain on C++17 because CX and C++20 are mutually exclusive.

### Run a build command

From the root of the repo:

```
build.cmd
```

Notes:
+ This does an `msbuild /m` on `dxaml\Microsoft.UI.Xaml.sln` and `controls\MUXControls.sln`
+ This builds the product and test binaries, it does not build WinUI Gallery or other samples
+ The `/m` allows the build to spawn up to as many as many processes as there are logical CPUs
+ Did your build fail due to memory errors?  Please see the "Swap Space" section above

For more information on the various ways to build, run `build.cmd /?`. There are predefined "targets" that specify
which subsets of the code to build, such as product code, tests, or sample applications. There are also many
"options", some unique to WinUI and some applicable to all msbuild commands. These include many useful switches such as
`/c`, which will clear out anything in your repository created by a previous build.

For additional advanced build options, see [Advanced build topics: Advanced build options](./building-advanced.md#advanced-build-options)

Build output prints to screen, as well as to a `.binlog` file thanks to the `/bl` flag in the `msbuild.exe` command. To
see this output in a  more human readable form, use the [MSBuild Log Viewer](#install-msbuild-log-viewer).

#### Building specific directories

If you want to build a specific project or directory without using `build.exe`, you can use `msbuild.exe` directly.

Some msbuild aliases:
+ `msb` - builds the current directory's project and everything it depends on (multiproc and with .binlog output)
+ `bz` - msb the current directory's project without its dependencies, might fail
+ `msb /t:Rebuild` - re-builds

#### Advanced build information

For more information about advanced build scenarios, see [Advanced build topics](./building-advanced.md)

## Next steps


For more information about building and using Sample apps, see [WinUI Sample Apps](building-sample-apps.md)
For more information about manual testing and debugging, see [debugging](../debugging/debugging.md)
For more information on creating a manual test app in VS and iterating on your changes see [Ad-Hoc testing of local build with fast innerloop](../ad-hoc-testing-of-local-build-with-fast-inner-loop.md).
For more information about automated testing, see [Testing In WinUI FAQ](../testing/testing-FAQ.md) and
[WinUI CI Test System Overview](../testing/test-system-overview.md)


>Found a bug?  Please [file an issue](https://github.com/microsoft/microsoft-ui-xaml/issues) and we'll triage it.
