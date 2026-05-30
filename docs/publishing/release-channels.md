## Windows App SDK Release Channels

## Table of Contents

- [How to build experimental and release](#how-to-build-experimental-and-release)
- [WinUI Experimental APIs](#winui-experimental-apis)

The Windows App SDK has three distinct [release
channels](https://docs.microsoft.com/en-us/windows/apps/windows-app-sdk/release-channels):
* Stable (supported)
* Preview (not supported)
* Experimental (not supported) 

Note that WinUI (this repo) is part of the Windows App SDK, but it's the only part that has
different runtime behavior between experimental and preview/release. This is primarily for disabling
UWP support, for which there's no API to block. Elsewhere it's metadata only and non-experimental is
implemented post-build by editing the winmd.

When we build this repo, we either build "experimental", or "release".  The below table describes
the nature of both flavors:

|| Experimental | Release |
|-|-------------|---------|
|`MUXFinalRelease`|is false|is true|
|`MUX_PRERELEASE` |is defined|is not defined|
|Experimental APIs|Yes|In Private WinMD only|
|Default for local build|Yes|No|
|Default for PR pipeline|Yes|No|
|Used for Stable releases|No|Yes|
|Used for Preview releases|No|Yes|
|Used for Experimental releases|Yes|No|

## How to build experimental and release

When you build locally, by default you'll build experimental.  If you want to build a release build
locally, build.cmd supports this"

`build /muxfinal`

Or if you're not building the whole repo:

`msb /p:MUXFinalRelease=true <myproject.vcxproj>`

Some WinUI pipelines allow the user to specify MUXFinalRelease when kicking off a manual run.  For
example, in the PR pipeline, the user may set the MUXFinalRelease variable to true in the variables
tab:

![Screenshot of the setting to enable MUXFinalRelease on a PR
pipeline](images/pr-muxfinalrelease.png)

This is handy if you're making changes that you're worried might affect the release build.

The **WinUI-Xaml-Release** pipeline does a release build of WinUI.

## WinUI Experimental APIs

In WinUI, some of our APIs are "experimental".  This means they aren't fully supported yet, but we
want developers using the experimental channel to be able to try them out and report bugs.  As you
can see in the above table, when we do a release build we exclude the experimental APIs, but put
them into a private WinMD so we can still run tests.  In experimental builds, the experimental APIs
are left in the shipping WinMD, and marked as "experimental".

In [featureflags.h](../../dxaml/xcp/inc/featureflags.h), we define feature flags that allow us to
specify which APIs should be present, absent, and marked as experimental.  Our IDL files describes
which APIs are controlled by which feature flags with the feature attribute, like this:
`[feature(Feature_MyExperimentalFeature)]`.  Please see the featureflags.h file for more
information.

