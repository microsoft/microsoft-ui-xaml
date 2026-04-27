# Ad-Hoc testing of local build with fast inner loop

When working on a change locally you will likely want to be able to run your changes in an ad-hoc app and be able to 
iterate quickly. This doc describes a workflow for how to enable that.

You will create an app in Visual Studio, target the local WinUI component package from your local build and iterate on that.

For information on building and running the sample apps in this repo see [Building Sample Apps](.\building\building-sample-apps.md). 
This document is about creating your own local ad-hoc test app, not about using the test apps in this repo.

# Prerequisites
You should be able to use Visual Studio to build and launch Windows App SDK WinUI Apps. 
Install the latest Visual Studio and include the Windows Application Development workload.

You should also be able to build this repo. See [developer guide](./building/developer-guide.md).

# Steps

## Initial Build of Lifted Xaml
Build the repo as normal. See the [developer guide](./building/developer-guide.md) for more information.
In short, run `init.cmd` and then run `build.cmd`. If you want you can skip building the test code by running `build.cmd product`.
The test code is not necessary for this scenario.

Assuming the build completes sucessfully it will create a local WinUI component package.

## Use Visual Studio to create an ad-hoc test app
Launch Visual Studio.  
Create a new project.  
Create a new "Blank App, Packaged (WinUI in Desktop)" app for either C# or C++.  
Create this project in a regular directory on your machine - i.e. not in the lifted xaml repo.
Visual Studio will create a new WinUI app that is targetting the latest public release of WinAppSDK.  
As a validation, verify that you can build and launch this app.

### Enable Native Debugging
In Visual Studio, make sure that you are debugging native code. For a C++ app this is always the case. For a C# app you need
to update the project settings. In Visual Studio, choose Debug -> [App Name] Debug Settings -> Enable Native Code Debugging.

This is important later on.

## Target the local WinUI component package

### Add nuget.config
Next, we will update this project to target the local WinUI component package from our local build.

Close the app and Visual Studio. 

In the root directory of this project (i.e. the location of the .sln file) create a new file `nuget.config`.

Add the following text to that file:
```xml
<configuration>
  <config>
    <clear />
    <add key="globalPackagesFolder" value="$\..\packages" />
    <add key="repositoryPath" value="$\..\packages" />
  </config>
  <packageSources>
    <add key="packagestore" value="<repo-root>\PackageStore" />
  </packageSources>
</configuration>
```
Update the path to PackageStore so that it points to the PackageStore directory in your local WinUI repo.

Adding this config does two things. First, it adds the location of the 'PackageStore' directory from your local lifted 
xaml git repo to the list of locations that nuget.exe will search for packages. This will allow the Visual Studio project
to target this local build. 

Second, it updates nuget to cache all its packages in a subdirectory of this directory instead of using the global package 
cache from the machine (e.g. `C:\Users\alias\.nuget\packages`). This is important since we want to avoid the local WinUI component package 
being cached globally since that can cause issues. It does mean that ALL the nuget packages that this project depends on 
will get cached here, which is a little redundant, but it is a worthwhile trade-off.

### Update the Project Target Platform (e.g. to x64)
Open the .sln in Visual Studio again. 

Update the build Configuration to match the Platform for your local build. The target platform that you build your app for 
must match the platform that you built winui for locally. By default `init.cmd` targets x64, so update your test app project to build 
for x64 by changing the drop down from x86 to x64.

This is important because the local WinUI component package only supports a single target platform unlike the real WinAppSDK package which supports
all (x86, x64, arm64).

### Update the Package References
In Visual Studio, use Tools -> Nuget Package Manager -> Manage Nuget Packages for Solution.
Make sure you have the `Include prerelease` checkbox checked to ensure that the local WinUI component package shows up.
Update your app to target the local WinUI component package instead of the public stable package.

If you prefer, instead of using the UI, you can also do this by modifying the .csproj and updating the `<PackageReference>` element.

Rebuild and launch your app with F5. 

You should now have your test app running against your local build. You can try out your scenario by updating the test
app.

You will probably want to iterate on your changes in the lifted xaml repo. Below describes a good inner loop for this.

## Inner Loop

### Find the path to Microsoft.UI.Xaml.dll that your app is using

Hit F5 to launch and debug your app.

Open the Modules window (Debug -> Windows -> Modules).

Find Microsoft.UI.Xaml in the Modules window and take note of the full path to the dll (Right click -> Copy Value -> Copy Path). 
It will look something like: `C:\Users\alias\source\repos\App17\App17\bin\x64\Debug\net8.0-windows10.0.19041.0\win-x64\AppX\Microsoft.ui.xaml.dll`.

Note that the local WinUI component package defaults to Self-Contained unlike the real package which defaults to Framework-Dependent. 
This is good since it makes debugging easier.

If you are iterating on Microsoft.UI.Xaml.Controls.dll take note of the path to that dll instead.

You should also verify that Visual Studio has loaded the symbols for this dll. The path to the pdb should look something 
like this:
`<repo-root>\BuildOutput\obj\amd64chk\dxaml\xcp\dxaml\dllsrv\winrt\native\Microsoft.ui.xaml.pdb`. If the 
symbols are not being loaded (or are being loaded from a different location) something is likely wrong.

### Build your changes in lifted-xaml repo

Make whatever code changes you need in the lifted xaml repo. Before building make sure to stop debugging your app in Visual Studio.

Build your changes. If you are making changes to Microsoft.UI.Xaml.dll the quickest way to rebuild that dll is

`msb dxaml\xcp\dxaml\dllsrv\winrt\native\Microsoft.ui.xaml.vcxproj`

If you are making changes to Microsoft.UI.Xaml.Controls.dll, use:

`msb controls\dev\dll\Microsoft.UI.Xaml.Controls.vcxproj`

These commands will only rebuild these respective dlls. Since it is only incrementally building the changed .cpp/.h files
it will normally be very quick (a couple of seconds). If you change a common header it might trigger a broader rebuild.

This works well if you are just making pure code changes. If you making other changes (such as updating API surface) you 
will need to do a more extensive build (such as rerunning build.cmd at the root of the repo). See below for such scenarios.

Take note of the path to the newly built dll. It should be something like:

`<repo-root>\BuildOutput\bin\amd64chk\Product\Microsoft.ui.xaml.dll`

### Copy the updated dll into your app

Replace the dll in your app's build output with the updated dll. E.g.

`copy /y <repo-root>\BuildOutput\bin\amd64chk\Product\Microsoft.ui.xaml.dll C:\Users\alias\source\repos\App17\App17\bin\x64\Debug\net8.0-windows10.0.19041.0\win-x64\AppX\Microsoft.ui.xaml.dll`

Note that we are NOT rebuilding the local WinUI component package. We are just replacing the dll in our app. So if you do a clean build of 
your app you will lose that updated dll and you will need to replace it again.

### Relaunch and debug your app

Hit F5 in Visual Studio to launch your app again.

You should verify that Visual Studio is still loading the symbols for the updated dll.

### Iterate

You should be able to continue making changes to your app and making changes to the product code in this manner.

## Broader Changes

The above steps work well when you are just making code changes to one of the product dlls. This should cover the 90% case. 
If you are making broader changes it might not be sufficient to just replace the dll. In that case you should fallback to
running build.cmd to build. This will build everything and re-create the local WinUI component package.

If you want to manually build the required pieces but still update the local WinUI component package you can use `pack.component.cmd` to recreate it.

### Updating the Local WinUI Component Package

If you create a new local WinUI component package by running build.cmd you might expect to be able to hit F5 in Visual Studio and have
your app pick up the changes. However this will NOT work. This is because nuget assumes that a given version of a nuget 
package is immutable. The package version does not update with each build (it is fixed). Nuget will cache the contents
of the package locally in the package cache and it is those files that are used in the build.

So, after you build a new version of the local WinUI component package, you want to delete it from the cache to force Visual Studio to pick
up the changes. Earlier in this document we described creating the nuget.config file which pointed to a local package 
cache. Go to that directory now in Explorer and delete 'microsoft.windowsappsdk'. 

Now when you build, Visual Studio will pick up the new local WinUI component package from your local repo and unpack it into the local
package cache again so you can start using the updated build.

