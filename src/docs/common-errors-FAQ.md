# Common Errors in Building and running WinUI

## Table of Contents

- [Setting up dev environment](#setting-up-dev-environment)
- [Building WinUI](#building-winui)
- [Building WinUI Gallery](#building-winui-gallery)
- ["Modified" T4 (.tt) files which don't contain changes](#modified-t4-tt-files-which-dont-contain-changes)

## Setting up dev environment

If you face CredentialsProvider errors like: 

```
`CredentialProvider.Microsoft' due to an unrecoverable fault:  
NuGet.Protocol.Plugins.ProtocolException: A plugin protocol exception occurred. ---> 
NuGet.Protocol.Plugins.ProtocolException: The parameter is incorrect.`
```
or
```
C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\Common7\IDE\CommonExtensions\Microsoft\NuGet\NuGet.targets(128,5): 
error : Response status code does not indicate success: 401 (Unauthorized).
```

There are multiple solutions to this. 
One would be to force reauthentication by following command listed 
[here](https://docs.microsoft.com/en-us/nuget/reference/extensibility/nuget-cross-platform-authentication-plugin). 
Another alternative could be to delete the file `C:\Users\<user>\AppData\Local\MicrosoftCredentialProvider\SessionTokenCache.dat` 
which will force another reauth.

## Building WinUI

A successful build may have warnings but will have 0 errors.

As rule of thumb against build errors, running the following commands are the best way to get you to "clean state":
* `git clean -xdf`
* `nuget locals all -clear`
* `tools\clean.cmd`

If you get out-of-heap errors doing the build, the `/b` switch can be useful. It specifies a "background" build (only 
runs two instances of msbuild.exe.

If you get errors related to WindowsAppSdkMockCheck, like the following:
```
 C:\.tools\.nuget\packages\microsoft.windowsappsdk\999.0.0-mock-3.0.0-dev-x64-release\buildTransitive\Microsoft.WindowsAppSDK.Custom.targets(12,5): The Windows App SDK mock package should not be installed into a shared/global cache.
 If this was intended, set WindowsAppSdkMockCheck=false in your project to suppress this error.
 If this was not intended, delete the mock package from your shared/global cache,
 use a nuget.config with globalPackagesFolder and/or repositoryPath set to a custom location,
 and avoid using the NUGET_PACKAGES environment variable.
```
You can delete the NUGET_PACKAGES environment variable from your system environment variables, start a new command prompt, and try building again.

If you get an error related to unable to find package versions in package store, similar to below:
```
 D:\xaml\controls\test\MUXControls.Test\MUXControls.Test.csproj : error NU1102: Unable to find package Microsoft.NETCore.App.Crossgen2.win-x64 with version (= 8.0.21) [D:\xaml\controls\MUXControls.sln]
 D:\xaml\controls\test\MUXControls.Test\MUXControls.Test.csproj : error NU1102:   - Found 65 version(s) in WinUI.Dependencies [ Nearest version: 8.0.20 ] [D:\xaml\controls\MUXControls.sln]
 D:\xaml\controls\test\MUXControls.Test\MUXControls.Test.csproj : error NU1102:   - Found 0 version(s) in packagestore [D:\xaml\controls\MUXControls.sln]
 Restored D:\xaml\controls\test\testinfra\MUXTestInfra\MUXTestInfra.csproj (in 2.17 sec).
 Failed to restore D:\xaml\controls\test\MUXControls.Test\MUXControls.Test.csproj (in 3.18 sec).
```
In the above case, file an issue so the internal feed can be updated for the required version. 

If you get warning related to version 8.0.x not found, 9.0.x was resolved instead along with the above error, similar to below:
```
 D:\xaml\controls\test\MUXControlsTestApp\MUXControlsTestApp.csproj : warning NU1603: MUXControlsTestApp depends on Microsoft.NET.ILLink.Tasks (>= 8.0.21) but Microsoft.NET.ILLink.Tasks 8.0.21 was not found.
 Microsoft.NET.ILLink.Tasks 9.0.4 was resolved instead.
```
It can result to a version mismatch. Run a clean build again after the internal feed is updated for the required version.
`build.cmd /c`

After a successful build and launch of MuxControlsTestApp, if you get errors related to WindowsAppSdkMockCheck, like the following:
```
 C:\.tools\.nuget\packages\microsoft.windowsappsdk\999.0.0-mock-3.0.0-dev-x64-release\buildTransitive\Microsoft.WindowsAppSDK.Custom.targets(12,5): The Windows App SDK mock package should not be installed into a shared/global cache.
 If this was intended, set WindowsAppSdkMockCheck=false in your project to suppress this error.
 If this was not intended, delete the mock package from your shared/global cache,
 use a nuget.config with globalPackagesFolder and/or repositoryPath set to a custom location,
 and avoid using the NUGET_PACKAGES environment variable.
```
Close the solution and build the solution using the following command.
`buildsamples.cmd`
After the build is successful, launch MuxControlsTestApp again.

## Building WinUI Gallery

If the build fails with missing nuget depdendencies, do `nuget restore WinUIGallery.slnx` (actual solution's name) 
to restore all the missing nuget files.

For a crash with stowed exceptions (top of call stack contains `Microsoft_UI_Xaml!FailFastWithStowedExceptions`), try setting a 
breakpoint in CaptureErrorContext in dxaml\xcp\components\base\errorcontext.cpp; that will get you what the actual failure is.

## "Modified" T4 (.tt) files which don't contain changes

Sometimes a git checkout might convert tabs in T4 files (like group.tt) to spaces. But git still considers them
equivalent, so a "checkout --", restore, or hard reset doesn’t revert them. This makes it hard to get the files
switched back to an unmodified state.

One solution is to run the following commands.
**Note: Do not have any uncommitted changes before running these -- any uncommitted changes will be overwritten.**
```
git rm -r --cached .
git reset --hard HEAD
```
