# Mini Windows SDK

During development of a new windows release we want to take advantage of under-development APIs. Doing so through an
insider SDK is the "right" thing but has drawbacks:

1) Requires that developers working on features that don't need the new SDK to install one.
2) The insider SDK changes frequently and getting all developers to have the same build number is hard (because VS requires
that you target *exactly* the right build.)
3) Adds latency if we want to start building on a feature as soon as it's checked in a dev branch but before it's
in an insider build (or even before an SDK for the dev branch becomes available).

Instead for development of these APIs we will build a nuget package for ourselves of just the pieces from the dev branch
that are necessary to build our component. That is, simply the IDL and WinMD files.

The downside of this approach is that it only works for the WinUI dll itself, not the test/sample apps (yet).

This approach is also optional -- if you don't have the nuget package installed to Microsoft.UI.Xaml then it won't
take effect and you'll just use whatever the targeted SDK is.

# Building and uploading the package

Making the package. Use the format 10.0.BuildNum.0 and the path to your public folder. Either checked in publics
with sdpublic or to %PUBLIC_ROOT%.

```
MakeMiniWindowsSDKNuPkg.cmd <version> <path to publics>
e.g. MakeMiniWindowsSDKNuPkg.cmd 10.0.14959.0 e:\RS0\sdpublic
```

Uploading the package:

```
nuget push *.nupkg -apikey VSTS -source MUXControls
```
