# Updating WebView2 SDK and Runtime Installers

When Edge releases a new WebView2 SDK, we may want to update the version that Xaml WebView2 uses. As part of the 
product, we ship a WebView2 SDK. For test code only, we include WebView2 Runtime installers, in case the pipeline VMs 
don't already have a runtime installed. This document will help you update these two components.

[[_TOC_]]

## Background

Edge WebView2 team's doc on SDK & browser versioning, compat scenarios, release process/schedule, etc: 
[Link to PowerPoint doc](https://microsoft.sharepoint.com/:p:/r/teams/Edge/Documents/Planning/Anaheim/Dev%20Experience/Application%20Platform/WebView2%20Versioning.pptx?d=w6d2662e8ad514a1a8ea8022d7d992e10&csf=1&web=1)  
Edge release schedule for the rest of 2021: 
[Edge_release_schedule.png](https://microsoft-my.sharepoint-df.com/:i:/p/krschau/Ea-PJOlkcaBLgur81pj-U7QBflthQQpvO-qo88xFR2qxcw)

### WebView2 SDK

The public Edge WebView2 nuget packages can be found here: https://www.nuget.org/packages/Microsoft.Web.WebView2/

In the past, Xaml had to consume prerelease packages or create their own modified SDK packages, because the release 
versions did not contain the WinRT bits. However, this limitation no longer exists and official release versions should 
be used when possible. When we must use a private SDK version, **TODO**

### WebView2 Runtime

When Xaml WebView2 tests run, they check to see if a WebView2 runtime is available on the machine to use. If not, the 
tests install one. These installers live in a nuget package we create and keep in the 
**Microsoft.UI.DCPP.Dependencies.Edge** feed. 

**This package is only used for tests.** The WinUI product does not specify or require a specific Runtime version 
(instead, apps may specify a minimum version depending on the APIs they use).

Versions are defined in `%<edgeroot>%\edge_embedded_browser\client\win\embedded_browser_version_info_values.h.version`:

```
  #ifndef EDGE_EMBEDDED_BROWSER_CLIENT_WIN_EMBEDDED_BROWSER_VERSION_INFO_VALUES_H_
  #define EDGE_EMBEDDED_BROWSER_CLIENT_WIN_EMBEDDED_BROWSER_VERSION_INFO_VALUES_H_
    
  #define PRODUCT_VERSION L"@MAJOR@.@MINOR@.@BUILD@.@PATCH@"
  #define MAJOR_NUMBER @MAJOR@
  #define MINOR_NUMBER @MINOR@
  #define BUILD_NUMBER @BUILD@
  #define PATCH_NUMBER @PATCH@
    
  #endif  // EDGE_EMBEDDED_BROWSER_CLIENT_WIN_EMBEDDED_BROWSER_VERSION_INFO_VALUES_H_
```

The version of Edge we install for tests should have a build number greater than or equal to the number of the SDK. For 
example, Edge version 82.0.**436**.0 can work with SDK version 0.9.**430**.

### How the SDK and Runtime work together

The SDK includes the WinRT API wrapper and the loader dll, and those get pushed into the consuming NuGet package 
(i.e. WinUI3 app). 

The implementation code (Win32/COM) is in `embeddedbrowserwebview.dll`, which ships with the browser. In order for 
implementations to be found (QIs to succeed), the 3-digit build # on the browser version must be >= 3-digit build # of 
SDK version.

However, they also have a policy of maintaining some older 'snapshots' of the implementation (the deprecated folders) 
for a limited amount of time. That way, older apps can still work with newer browsers in the general case. (For 
example, if an API was previously experimental before but is currently final, older app's QIs to 
`ICWV2Experimental->Foo()` won't fail while the deprecated path is maintained.)

## **Updating TL;DR**

1. Run `UpdateWebView2.cmd`
2. Drop in the installers
3. Pack and push the nuget package
4. Run `init` and make sure everything looks good

## Detailed Updating Instructions

0. First, decide what SDK and Runtime versions you're going to update to. See above for how these versions work 
   together. This may also necessitate working with the Edge WebView2 team to understand what SDK will have any new 
   APIs we need.

1. Run `UpdateWebView2.cmd`. This can be found in the `\scripts` folder. For help, run `UpdateWebView2.cmd /?`. 
   This script will:
   * Update the SDK version in `\eng\versions.props` and `\controls\dev\dll\packages.config`
     > Updating these numbers is actually the only thing that needs to happen to consume a new, public SDK. All other 
       instructions in this document (besides step 4 used for private SDKs) are related to the runtime we include for 
       tests.
   * Update the Runtime version in 
     * `\packages.config`
       * This is what actually gives the nuget package we're creating a version number. We use the SDK number, but it's 
         technically arbitrary
     * `\dxaml\test\infra\taefhostappmanaged\TaefHostAppManaged.csproj`
       * This is the binplace logic
       * Note in this path, any terminating .0 gets dropped (ie. Use 80.0.333 rather than 8.0.333.0)
     * `\dxaml\external\Microsoft.UI.DCPP.Dependencies.Edge.nuspec`
       * This points to the `mini_installer`s you will drop into the repo, to be packaged into a nuget package
   * Create directories where you will need to copy installers from the Edge Official Builds website
     * E.g.  
       `<repo_root>\dxaml\test\edge\88.0.676.0\x64\mini_installer.exe`  
       `<repo_root>\dxaml\test\edge\88.0.676.0\x86\mini_installer.exe`
   * Give you detailed instructions about the following steps.

2. Drop the Edge installers into the directories created in the previous step
   * These should be retrieved from the correct version at https://edgeteam.ms/es/official-builds
   * The links called Win64 and Win32 under "Installers" are mini_installer.exe files that must be dropped into the
     repo in the directories created by the script (Win64 goes under x64, Win32 goes under x86).
   * These files **should not** be checked in to the repo. Once the nuget package has been created and pushed per the 
     instructions below, these files can be deleted.

3. Pack and push the Runtime installer nuget package
   * **The update script will tell you exactly what commands to run** to do this. You will need to obtain the API key 
     from a team member.
   * Packing is done by
     * `nuget pack <.nuspec path> -OutputDirectory <repo_root>\packages`
     * Example command and output:
      ```
      >nuget pack <repo_root>\dxaml\external\Microsoft.UI.DCPP.Dependencies.Edge.nuspec -OutputDirectory <repo_root>\packages\Microsoft.UI.DCPP.Dependencies.Edge
      
      Attempting to build package from 'Microsoft.UI.DCPP.Dependencies.Edge.nuspec'.
      Successfully created package 'C:\Users\dkomin\source\repos\microsoft-ui-xaml-lift-full\packages\Microsoft.UI.DCPP.Dependencies.Edge\Microsoft.UI.DCPP.Dependencies.Edge.80.0.333.nupkg'.
      WARNING: NU5048: The 'PackageIconUrl'/'iconUrl' element is deprecated. Consider using the 'PackageIcon'/'icon' element instead. Learn more at https://aka.ms/deprecateIconUrl
      ```
   * Pushing is done by
     * `nuget push <.nupkg path> -Source WinUI.Dependencies -apikey <ask_a_teammate_for_key>`
     * Example command and output:
      ```
      > nuget push <repo_root>\packages\Microsoft.UI.DCPP.Dependencies.Edge\<created_nupkg> -Source WinUI.Dependencies -apikey <ask_a_teammate_for_key>
      
      MSBuild auto-detection: using msbuild version '16.3.2.50909' from 'C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\MSBuild\Current\Bin'.
          [CredentialProvider]VstsCredentialProvider - Acquired bearer token using 'ADAL Windows Integrated Authentication'
          [CredentialProvider]VstsCredentialProvider - Attempting to exchange the bearer token for an Azure DevOps session token.
      Pushing Microsoft.UI.DCPP.Dependencies.Edge.80.0.333.nupkg to 'https://microsoft.pkgs.visualstudio.com/_packaging/1103ce32-f206-4cab-b967-dcc556dacd13/nuget/v2/'...
        PUT https://microsoft.pkgs.visualstudio.com/_packaging/1103ce32-f206-4cab-b967-dcc556dacd13/nuget/v2/
        Accepted https://microsoft.pkgs.visualstudio.com/_packaging/1103ce32-f206-4cab-b967-dcc556dacd13/nuget/v2/ 16326ms
      Your package was pushed.
      ```
   * This pushes the package to here: 
     https://microsoft.visualstudio.com/DefaultCollection/WinUI/_packaging?_a=feed&feed=WinUI.Dependencies

4. **Follow this step ONLY if you are updating the SDK to a private version**  
   If you are updating to a private SDK version (one not on nuget.org) you must also push it to the WinUI.Dependencies 
   private feed. Download the signed nuget from the Edge WebView2 pipeline, and push it to the feed like you did above.
   * You can see which SDK versions were pushed manually by us, vs which came from the public Nuget Gallery, by looking 
     at the history 
     [here](https://microsoft.visualstudio.com/DefaultCollection/WinUI/_artifacts/feed/WinUI.Dependencies/NuGet/Microsoft.Web.WebView2/1.0.1178/versions)
 
5. Run `init` to ensure the package gets pulled down correctly.
   * Clear the nuget caches before doing this to be even more sure the right package is being pulled down
   * `init` should run without errors
   * Under the `packages` directory in your repo, you should now see a `packages/Microsoft.UI.DCPP.Dependencies.Edge` 
     directory containing the version you just created, and a `packages/Microsoft.UI.DCPP.Dependencies.Edge.<version>` 
     directory below it (e.g. `packages/Microsoft.UI.DCPP.Dependencies.Edge.80.0.333`)
   * Build the repo for additional assurance