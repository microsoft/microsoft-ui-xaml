# Updating WebView2 SDK and Runtime Installers

When Edge releases a new WebView2 SDK, we may want to update the version that Xaml WebView2 uses. As part of the 
product, we ship a WebView2 SDK. For test code only, we include a WebView2 Runtime installer, in case the pipeline VMs
don't already have a runtime installed. This document will help you update these two components.

## Table of Contents

- [Background](#background)
  - [WebView2 SDK](#webview2-sdk)
  - [WebView2 Runtime](#webview2-runtime)
  - [How the SDK and Runtime work together](#how-the-sdk-and-runtime-work-together)
- [Updating TL;DR](#updating-tldr)
- [Detailed Updating Instructions](#detailed-updating-instructions)

## Background

Edge WebView2 team's doc on SDK & browser versioning, compat scenarios, release process/schedule, etc: 
[Link to PowerPoint doc](https://microsoft.sharepoint.com/:p:/r/teams/Edge/Documents/Planning/Anaheim/Dev%20Experience/Application%20Platform/WebView2%20Versioning.pptx?d=w6d2662e8ad514a1a8ea8022d7d992e10&csf=1&web=1)  

### WebView2 SDK

The public Edge WebView2 nuget packages can be found here: https://www.nuget.org/packages/Microsoft.Web.WebView2/

In the past, Xaml had to consume prerelease packages or create their own modified SDK packages, because the release 
versions did not contain the WinRT bits. However, this limitation no longer exists and official release versions should 
be used when possible. When we must use a private SDK version, **TODO**

### WebView2 Runtime

When Xaml WebView2 tests run, they check to see if a WebView2 runtime is available on the machine to use. If not, the
test infrastructure provisions one. The x64 installer is published as a new immutable version of
**Microsoft.UI.DCPP.Dependencies.Edge** on both the internal `WinUI.Dependencies` feed and the public shine-oss
`WinUI-Dependencies` feed.

The package is created from Microsoft's official public
[x64 Evergreen Standalone Installer](https://go.microsoft.com/fwlink/?linkid=2124701). The download link is moving, so
the installer must be downloaded, verified, and packaged manually. It must not be downloaded during package restore or
checked in to Git.

**This package is only used for tests.** The WinUI product does not specify or require a specific Runtime version 
(instead, apps may specify a minimum version depending on the APIs they use).

The package version must be the immutable four-part WebView2 Runtime version from the signed installer's embedded
`OfflineManifest.gup`, not the outer EXE's `FileVersion` or `ProductVersion`. Those outer values identify the Edge
Update engine.

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

1. Run `UpdateWebView2.cmd`.
2. Download and verify the official x64 Evergreen Standalone Installer.
3. Pack and, after approval, manually push the NuGet package.
4. Run `init` and make sure the SDK update looks good.

## Detailed Updating Instructions

0. First, decide what SDK and Runtime versions you're going to update to. See above for how these versions work 
   together. This may also necessitate working with the Edge WebView2 team to understand what SDK will have any new 
   APIs we need.

1. Run `UpdateWebView2.cmd`. This can be found in the `\scripts` folder. For help, run `UpdateWebView2.cmd /?`.
   This script will:
   * Update the SDK version in `\eng\versions.props`, `\controls\dev\dll\packages.config`, and
     `\controls\test\TestAppCX\packages.config`.
     > Updating these numbers is the only thing that needs to happen to consume a new public SDK.
   * Update the Runtime version and temporary x64 installer source path in
     `\dxaml\test\external\Microsoft.UI.DCPP.Dependencies.Edge.nuspec`.
   * Create the temporary directory where the x64 installer must be copied:
     `<repo_root>\dxaml\test\edge\<runtime-version>\x64\MicrosoftEdgeWebView2RuntimeInstallerX64.exe`.
   * Give you the pack and manual publication commands.

2. Download and verify the official installer.
   * Download `MicrosoftEdgeWebView2RuntimeInstallerX64.exe` from Microsoft's public
     [x64 Evergreen Standalone Installer link](https://go.microsoft.com/fwlink/?linkid=2124701) and place it in the
     temporary directory created by the script.
   * For Runtime version `152.0.4191.66`, the approved installer identity is:

     | Property | Value |
     | --- | --- |
     | Size | `258614480` bytes |
     | SHA-256 | `E7FA35755196AD9223596EF021A1CE6799509142EAA40BA35F634026BE50B831` |
     | Embedded WebView2 app ID | `{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}` |
     | Embedded manifest version | `152.0.4191.66` |

   * Verify the downloaded file before packing:

     ```powershell
     $installer = "<repo_root>\dxaml\test\edge\152.0.4191.66\x64\MicrosoftEdgeWebView2RuntimeInstallerX64.exe"
     Get-AuthenticodeSignature -LiteralPath $installer |
         Format-List Status, SignerCertificate, TimeStamperCertificate
     Get-FileHash -LiteralPath $installer -Algorithm SHA256
     (Get-Item -LiteralPath $installer).Length
     ```

     The Authenticode status must be `Valid`, the signer must be Microsoft Corporation, and the hash and size must
     exactly match the approved values above. Do not use the outer EXE's version `1.3.265.7`; that is the Edge Update
     engine version.
   * The EXE is temporary packaging input. It must not be checked in to Git and should be deleted after packaging.

3. Pack and push the Runtime installer NuGet package.
   * Packing is done with the command printed by the update script:

     ```console
     nuget pack <repo_root>\dxaml\test\external\Microsoft.UI.DCPP.Dependencies.Edge.nuspec -OutputDirectory <repo_root>\packages
     ```

   * Inspect the package before publication. It must contain exactly one installer at:

     ```text
     tools\x64\MicrosoftEdgeWebView2RuntimeInstallerX64.exe
     ```

   * NuGet normalizes a package version ending in `.0` by dropping that component from the generated filename. Set
     `$packagePath` to the exact `.nupkg` path reported by `nuget pack`; do not construct the filename from the
     four-part Runtime version.
   * Obtain required legal, redistribution, and feed-owner approval. Retrieve both feed URLs from Key Vault; do not
     place either URL in source code or documentation. First publish to the internal feed:

     ```powershell
     $packagePath = "<exact .nupkg path printed by nuget pack>"
     $internalFeed = "<internal feed URL from Key Vault>"
     nuget push $packagePath -Source $internalFeed -apikey AzureDevOps
     ```

   * Then publish the exact same `.nupkg` to the public shine-oss feed:

     ```powershell
     $shineOssFeed = "<shine-oss feed URL from Key Vault>"
     nuget push $packagePath -Source $shineOssFeed -apikey AzureDevOps
     ```

   * You need publish access to both feeds. The Azure Artifacts Credential Provider supplies authentication;
     `AzureDevOps` is the required non-secret NuGet API-key argument.
   * NuGet package versions are immutable. If installer bytes change while the embedded Runtime version remains the
     same, stop and investigate; do not overwrite or republish that package version.
   * Generated `.nupkg` files must not be checked in to Git.

4. **Follow this step ONLY if you are updating the SDK to a private version**  
   If you are updating to a private SDK version (one not on nuget.org) you must also push it to the WinUI.Dependencies 
   private feed. Download the signed nuget from the Edge WebView2 pipeline, and push it to the feed like you did above.
   * You can see which SDK versions were pushed manually by us, vs which came from the public Nuget Gallery, by looking 
     at the history in the feed.

5. Run `init` to ensure the SDK package gets pulled down correctly.
   * Clear the NuGet caches first to be more certain the right SDK package is restored.
   * `init` should run without errors.
   * Restoring and consuming `Microsoft.UI.DCPP.Dependencies.Edge` version `152.0.4191.66` is intentionally deferred
     to a separate change after this package has been approved and published.
   * Build the repo for additional assurance.
