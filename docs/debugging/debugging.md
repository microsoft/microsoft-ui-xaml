# Debugging

> See [Testing In WinUI FAQ](../testing/testing-FAQ.md) and [WinUI CI Test System Overview](../testing/test-system-overview.md)
> for instruction for general debugging instructions for testing your changes.

[Debugging Tips](debugging-tips.md) contains some useful tips for diagnosis and debugging. You are encouraged to update 
these docs if you have anything to add or amend.

## Table of Contents

- [Manual Testing](#manual-testing)
- [Testing and debugging private bits in different app configurations](#testing-and-debugging-private-bits-in-different-app-configurations)
  - [Testing against private bits within xaml repo](#testing-against-private-bits-within-xaml-repo)
  - [Overwriting WinUI binaries in a VS app](#overwriting-winui-binaries-in-a-vs-app)
  - [Overwriting WinUI binaries in a Store-deployed app](#overwriting-winui-binaries-in-a-store-deployed-app)
    - [.local and DLL Redirection](#local-and-dll-redirection)
    - [takeown + icalcs](#takeown--icalcs)
    - [PSExec](#psexec)
- [Testing changes to Lifted IXP (including the FrameworkUDK)](#testing-changes-to-lifted-ixp-including-the-frameworkudk)
- [Testing changes to WinUI Details](#testing-changes-to-winui-details)

## Manual Testing

You first need to perform a full build using the `build.cmd` script in the root of the repo.

To build and test any of the sample apps in the repo, including the WinUI Gallery, see 
[building sample apps](../building/building-sample-apps.md)

To create an app following all the same steps as someone externally using the preview builds, but using an 
internal build. See [Writing an app against an official internal build](../writing-an-app-with-internal-build.md)

To create a new app inside the repo using local bits, see [Creating a new test app in the repo](../building/building-new-repo-app.md)

## Testing and debugging private bits in different app configurations
One way to test updated product binaries with an app that's already installed is
to overwrite the product binaries in the app's install directory.

Depending on the app you are trying to test and debug with your hanges, different 
approaches can be used to replace dlls/binaries used by corresponding app with one
you provide to test your changes (and they can be debugged with corresponding PDBs)

### Testing against private bits within xaml repo
If you are working in `lifted-xaml` repo, sample apps `(\<repo\>\samples\\)` always pick up dlls from `BuildOutput`. All you need is to build `lifted-xaml` repo the usual way and rebuild any sample app. See [Testing In WinUI FAQ](../testing/testing-FAQ.md) for details.

### Overwriting WinUI binaries in a VS app

If you write your own app in VS (not in the repo's Samples directory), you can simply copy updated WinUI bits into the 
app's install location, which is in the app's project directory.

For exammple, if your app is `c:\repos\MyApp` and you're building Debug, the WinUI bits will be in
a directory such as `c:\repos\MyApp\MyApp (Package)\bin\x86\Debug\AppX\MyApp`
Usually, the output of build command in VS shows the location at some point.
You can also find an app's install location using the `scripts\find-appx` script.

For more information on creating a manual test app in VS and iterating on your changes see [Ad-Hoc testing of local build with fast innerloop](../ad-hoc-testing-of-local-build-with-fast-inner-loop.md).

> If your VS app is using WinUI binaries from [FrameworkPackage](https://learn.microsoft.com/en-us/windows/apps/windows-app-sdk/deployment-architecture#framework-package) and you want your app to use the you provided, follow section `local and DLL Redirection` below and use the combination of all steps 1-3a and 3b and add your binaries to location provided in 3b. This is useful in debugging in cases where one cannot use self contained packaging to replace binaries like when bug replicates only in framework package config and not self contained deployment config.
### Overwriting WinUI binaries in a Store-deployed app

There are various ways to overwrite these binaries, or have your app find a custom binary rather than the built-in one. 
In an app you build yourself, using `WindowsAppSDKSelfContained` is often easier and safer. However, with store or other 
third party apps, this may not always be possible.

```xml
<!-- .csproj of the app  -->
<PropertyGroup>
  ...
  <!-- Set this property to point to your local WinUI details repo -->
  <WindowsAppSDKSelfContained>true</WindowsAppSDKSelfContained>
</PropertyGroup>
```
#### .local and DLL Redirection

This method uses redirection rather than overwriting a built in binary. It can also be used for development scenarios. 
For example, F5 deploy of a sample app will also deploy a private copy of MUX.dll is you have it built locally.

Packaged apps (i.e. processes with package identity) have a "step 0" in their 
[DLL Search Order](https://github.com/microsoft/WindowsAppSDK/blob/main/specs/dynamicdependencies/DynamicDependencies.md#3152-packaged-processes)

0. DLL Redirection (aka `pkgdir\microsoft.system.package.metadata\Application.Local`) if 
[DevOverideEnable=1](https://docs.microsoft.com/en-us/windows/win32/dlls/dynamic-link-library-redirection)

This was added in Windows 8 specifically by request from shell devs to easily use patched DLLs to expedite testing.

The simple directions:

1\.  Set the `DevOverrideEnable` regkey
   * e.g.
   `REG ADD "HKLM\Software\Microsoft\Windows NT\CurrentVersion\Image File Execution Options" /v DevOverrideEnable /t REG_DWORD /d 1`
   * NOTE: You need to reboot for this to take effect 

2\. Create `microsoft.system.package.metadata\Application.Local` directories under the package's installed location
   * e.g.
   `MD "C:\Program Files\WindowsApps\Microsoft.WindowsCalculator_11.2206.0.0_x64__8wekyb3d8bbwe\microsoft.system.package.metadata\Application.Local"`    

3\. Copy your override DLL(s) to that location
    * e.g.
    `COPY C:\PrivateLocalBuild\shell32.dll "C:\Program Files\WindowsApps\Microsoft.WindowsCalculator_11.2206.0.0_x64__8wekyb3d8bbwe\microsoft.system.package.metadata\Application.Local\"`

> NOTE: This Application.Local trick applies to all packages no matter their source or install location. Long as the 
process has package identity and `DevOverrideEnable` is set, the Loader looks here before anywhere else (even before 
APIsets!)

> NOTE: If deploying a packaged app through Visual Studio, you will still need to follow step 4 below too, to create a `.exe` folder within `Appx` folder
but copy dlls to location mentioned in step 3 only. You can use `Modules` view within VS to check the location of loaded `Microsoft.ui.xaml.dll` to verify if the trick worked.


4\. For unpackaged apps, you can create a directory next to the exe and put your local binaries in it. For example, for `explorer.exe`  you can 
create `c:\windows\explorer.exe.local` and any DLLs in that folder will replace the system binaries.

#### takeown + icalcs

This method will overwrite an app's DLLs. For example, to replace the DLLs used by a WinUI Gallery that's been 
installed from the Store:

Open PowerShell as Administrator
```bat
cd $(get-appxpackage Microsoft.WinUIGallery).InstallLocation`
takeown /f *
icacls * /grant:r administrators:f
copy <product directory>*.dll
```
> Ex: `copy <repo-root>\BuildOutput\bin\x86chk\Product\*.dll`

Make sure you match architecture (x86/amd64)

You can find an app's install location using `scripts\find-appx`. For example, `scripts\find-appx WinUIGallery`  
and `cd $(scripts\find-appx WinUIGallery).InstallLocation`

> It will not work for replacing contents of Framework Package (...WindowsApps\Microsoft.WindowsAppRuntime*). Use .local and DLL Redirection approach
#### PSExec

Another way to overwrite binaries is to use [SysInternals PSExec](https://docs.microsoft.com/en-us/sysinternals/downloads/psexec). 
From an elevated command prompt, run `PSExec.exe -s cmd`. This will start a SYSTEM account command-line and therefore 
has permissions for file-system operations in the `WindowsApps` directory, like `xcopy`. However, this is unsupported 
and can cause problems. `Windows Defender` can start complaining on using this. 
> This option comes with large responsibility and improper usage can corrupt windows installation.
