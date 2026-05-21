# Patching Windows App Runtime with a Local Build

## Table of Contents

- [Rebase against matching commit](#rebase-against-matching-commit)
- [Build your changes](#build-your-changes)
  - [/muxfinal or not?](#muxfinal-or-not)
  - [x86 or x64?](#x86-or-x64)
  - [fre or chk?](#fre-or-chk)
- [Make a copy of the Windows App Runtime package](#make-a-copy-of-the-windows-app-runtime-package)
- [Patch and Repack](#patch-and-repack)

Normally when testing changes to WinUI, it is sufficient to test your changes using one of the Sample Apps in the repo
or by running `build.cmd /pack` to create the mock Microsoft.WindowsAppSdk nuget package.

However, sometimes it is necessary to test the true end-user scenario which is to use the WindowsAppSDK (which contains
WinUI). Getting a build of the Windows App Runtime that contains your changes (without merging your changes to main)
is not easy, since it requires running multiple build Pipelines across multiple different repos. So an alternative is
to take an existing build of the Windows App Runtime and patch it with your local build of WinUI.

This is possible, but a little tricky. Strap yourself in.

## Rebase against matching commit

Since you are going to just copy your local build of `Microsoft.UI.Xaml.dll` over the one from the WindowsAppSDK it is
important that what you build is compatible with the rest of the binaries in the Windows App Runtime package. The way
to do that is to choose the baseline commit for your change to be the commit that corresponds to the build that was
used for the WindowsAppSDK. For official releases of the WindowsAppSDK, this is likely something like `release/1.0.1`.

## Build your changes

### /muxfinal or not?

WinUI can be built with experimental features enabled or disabled. By default they are enabled, but for official
non-preview releases they are turned off. So you want the dll that you build to match the version of the WindowsAppSDK
that you are patching. To build WinUI with experimental features turned off, pass `/muxfinal` when you call `build.cmd`.

### x86 or x64?

Make sure that you are building for the architecture that matches the build of reunion that you are patching.

### fre or chk?

In theory there should be no problem with patching a reunion build with a 'chk' dll. But it might be more reliable to
build 'fre' to ensure things match.

## Make a copy of the Windows App Runtime package

You cannot directly patch the installed Windows App Runtime Framework Package that is installed under
`C:\Program Files\WindowsApps\` since that location is locked down by the OS. Instead you will make a copy of the
package contents, make your changes to the copy, create a new msix package and install it.

The best way to find the location to copy is to run a WindowsAppSDK app under the debugger and look at the paths in the
modules window. The path to the Windows App Runtime install is going to be something like:

```shell
C:\Program Files\WindowsApps\Microsoft.WindowsAppRuntime.1.1_1000.407.1501.0_x64__8wekyb3d8bbwe\
```

Open an elevated command prompt to access this directory, and copy the contents to a new directory, e.g.

```shell
C:\stuff\winappruntime
```

## Patch and Repack

Make whatever changes you need to your copy of the framework package. For example, overwrite `Microsoft.UI.Xaml.dll`
with your locally built copy.

Now create a new version of the Windows App Runtime msix that includes your change.

From a Developer Command Prompt:

Create a self-signing certificate, using "password" in the wizard (or use the one already generated: `.\build\WinUITest.pfx`):

```shell
MakeCert /n "CN=Microsoft Corporation, O=Microsoft Corporation, L=Redmond, S=Washington, C=US" /r /h 0 /eku "1.3.6.1.5.5.7.3.3,1.3.6.1.4.1.311.10.3.13" /sv MyTest.pvk MyTest.cer
```

Convert the cert and private key files to a pfx for signing:

```shell
Pvk2Pfx /pvk MyTest.pvk /pi password /spc MyTest.cer /pfx MyTest.pfx
```

Optionally trust the self-signing cert:

```shell
Certutil -addStore TrustedPeople MyTest.cer
```

Package up the Windows App Runtime content into an msix:

```shell
MakeAppx pack /p Microsoft.WindowsAppRuntime.9.9.msix /d C:\stuff\winappruntime /o
```

Now you need to sign this package before it can be installed:

```shell
SignTool sign /fd SHA256 /a /f <repo-root>\build\WinUITest.pfx Microsoft.WindowsAppRuntime.9.9.msix
```

Before you can install this package, you will need to uninstall the currently installed copy of the WindowsAppRuntime
and any apps that depend on it. You can uninstall each package manually with Remove-AppxPackage.

An alternative to uninstalling the existing copy is to bump the version of the package in `AppxManifest.xml` before
running `MakeAppx`. This will allow you to update the existing copy with your new copy without having to uninstall it.

Now you can install your freshly created copy of the Windows App Runtime:

```shell
powershell Add-AppxPackage -Path Microsoft.WindowsAppRuntime.9.9.msix
```
