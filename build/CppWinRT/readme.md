# C++/WinRT SDK

The C++/WinRT team currently doesn't manage a nuget package for their headers and tools. You can either build them yourself
or grab them from their daily share. Neither of these approaches is great, and instead of checking in a bunch of headers
and exes we instead package them up and publish them to our own nuget feed.

Once C++/WinRT produces an official package we will switch to that.

# Building and uploading the package

Making the package. Use the format 10.0.BuildNum.0 and the path to your public folder. Either checked in publics
with sdpublic or to %PUBLIC_ROOT%.

```
net use Z: \\redmond\osg\Threshold\Tools\CORE\DEP\DART\CppForWinRT
MakeCppWinRTNuPkg.cmd <version> <path to CppWinRTBuild>
e.g. MakeCppWinRTNuPkg.cmd 1.0.20160901.1 Z:\20160901.1
```

Uploading the package:

```
nuget push *.nupkg -apikey VSTS -source MUXControls
```
