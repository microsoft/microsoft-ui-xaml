# Build System How-To

## Table of Contents

- [Overview](#overview)
- [Common Tasks](#common-tasks)
  - [Binplacing](#binplacing)
  - [Packaging](#packaging)
    - [Adding new content to the nuget package](#adding-new-content-to-the-nuget-package)
    - [Static packaging content](#static-packaging-content)
  - [References](#references)
    - [Private vs. Public Metadata](#private-vs-public-metadata)
  - [Xaml Compilation](#xaml-compilation)
  - [Creating project files](#creating-project-files)
    - [C++](#c)
    - [.NET](#net)
    - [No Targets](#no-targets)
  - [Using the product](#using-the-product)
- [C++ Projects](#c-projects)
  - [MIDL](#midl)
    - [Non-merged IDL projects](#non-merged-idl-projects)
    - [Merged IDL Projects](#merged-idl-projects)
  - [Precompiled Headers](#precompiled-headers)
  - [Using CPP/WinRT](#using-cppwinrt)
  - [Adding custom types to test projects](#adding-custom-types-to-test-projects)
  - [Consuming WinUIDetails](#consuming-winuidetails)
- [.NET Projects](#net-projects)
  - [Consuming native projects from .NET6](#consuming-native-projects-from-net6)
  - [Central Package Versions](#central-package-versions)
- [Building the VSIX](#building-the-vsix)
- [Building the C#/WinRT Interop Assembly](#building-the-cwinrt-interop-assembly)

## Overview

The purpose of this doc is help developers working on WinUI accomplish certain tasks within the repository. For a deeper
dive into some of the principles and design of the build, see the [build system design doc](build-system-design.md).
To see how to accomplish more day-to-day tasks, see the [developer guide](developer-guide.md).

## Common Tasks

### Binplacing

The build copies binaries from the output directory in the obj folder to specific locations in the bin folder for
packaging and running tests. To control where a project's output is located, set the `TargetDestination` property in the
MSBuild project file. The following values are provided for you for common locations:

```xml
<ProductBinplaceDestinationPath>$(BinplaceRootPath)\Product</ProductBinplaceDestinationPath>
<ProductSymbolsBinplacePath>$(BinplaceRootPath)\Symbols\Product</ProductSymbolsBinplacePath>
<TestBinplaceDestinationPath>$(BinplaceRootPath)\Test</TestBinplaceDestinationPath>
<TestSymbolsBinplacePath>$(BinplaceRootPath)\Symbols\Test</TestSymbolsBinplacePath>
<TestBinplaceResourcesDestinationPath>$(TestBinplaceDestinationPath)\resources</TestBinplaceResourcesDestinationPath>
<TestDependenciesResourcesDestinationPath>$(BinplaceRootPath)\TestDependencies</TestDependenciesResourcesDestinationPath>
```

These properties, alongside many others, are defined in [eng\folderpaths.props](../../eng/folderpaths.props)

You can set the destination for your project like this:

```xml
<PropertyGroup>
  <TargetDestination>$(TestBinplaceDestinationPath)</TargetDestination>
</PropertyGroup>
```

Items that are required for binplacing are calculated automatically. If you find that these targets don't do what you
need, you can manually binplace files by adding them to the `BinplaceItem` item like this:

```xml
<ItemGroup>
  <BinplaceItem Include="Foo.dll" />
</ItemGroup>
```

These files will go to the location of the `TargetDestination` property. If you need to control the output of files
independently of one another, you can set the `Destination` metadata on the item like this:

```xml
<ItemGroup>
  <BinplaceItem Include="Foo.dll" Destination="$(ProductBinplaceDestinationPath)" />
</ItemGroup>
```

You can opt-out of binplacing the output of your project by setting the `BinplaceOutputAssemblies` property to false,
like this:
```xml
<PropertyGroup>
  <BinplaceOutputAssemblies>false</BinplaceOutputAssemblies>
</PropertyGroup>
```

The implementation of the binplace target is in [eng\binplace.targets](../../eng/binplace.targets).

### Packaging

To create the mock `Microsoft.WindowsAppSDK.nupkg` package used for tests and samples, we first build a
'mock' WinUI transport package, based on the \BuildOutput\packaging folder content.
Both the WinUI transport package and the WindowsAppSDK package are mocks in the sense of being used only
build validation of test and samples. Unlike the production versions of both packages, these both contain
only a single architecture - that of the currently init-ed repo.

To ensure that the proper content is included in the package, projects need to manually specify the content that is to
be packaged. Anything added to the `PackageContent` MSBuild item will be picked up and copied to the appropriate
location and included in the package.

There are existing MSBuild properties defined in [eng\packaging.props](../../eng/packaging.props) that you can use to
specify the location:

```xml
<WinMDPackageLocation>$(PackageOutputLocation)\lib\uap10.0</WinMDPackageLocation>
<NativeAssemblyPackageLocation>$(PackageOutputLocation)\runtimes\$(WinUIRuntimeIdentifier)\native</NativeAssemblyPackageLocation>
<ToolsPackageLocation>$(PackageOutputLocation)\tools</ToolsPackageLocation>
<BuildTargetsPackageLocation>$(PackageOutputLocation)\build</BuildTargetsPackageLocation>
<CSWinRTInteropAssemblyPackageLocation>$(PackageOutputLocation)\lib\$(DotNetCoreTargetFrameworkMonikerForPackaging)</CSWinRTInteropAssemblyPackageLocation>
<IncludesPackageLocation>$(PackageOutputLocation)\include</IncludesPackageLocation>
```

For example, here is how to include the output assemblies from a project's build in the `NativeAssemblyPackageLocation`:

```xml
<ItemGroup>
  <PackageContent Include="$(OutDir)$(TargetName).dll" PackageLocation="$(NativeAssemblyPackageLocation)" />
</ItemGroup>
```

#### Adding new content to the nuget package

To add a new binary or winmd file to the nuget package, you need to specify it in the `PackageContent` MSBuild item of
some project that is built. If the project is being built in the WinUI repository, this is fairly straightforward – you
add the binary as in the example above. If the content comes from an external source and is being repackaged in our
nuget, then add the content to the `ExternalBinaries` or `ExternalWinMDs` item groups in
[productmetadata.props](../../eng/productmetadata.props). This same metadata is used throughout test projects to ensure
they are configured properly.

#### Static packaging content

Consider placing any static content in the [packaging\Microsoft.WinUI.msbuildproj](../../packaging/Microsoft.WinUI.msbuildproj)
project, as this is where all static content currently resides (other than the markup compiler targets).

### References

We use the standard MSBuild `ProjectReference` and `Reference` items to specify dependencies in our build. Targets in
the `eng` folder should be designed with this in mind, so that we don't burden devs with a deep understanding of the
entire build system.

There are some common `ItemGroups` defined in [productmetadata.props](../../eng/productmetadata.props) that can be used if
needing to reference common well-known assemblies:

- `ExternalWinMDs`: contains winmds that we consume as part of our build that make it into the final product. Currently
consists of the following sub-items:
  - `IXpWinmds`: contains the Composition and Input WinRT API
  - `MRTWinMDs`
  - `WinUIEditWinMD`: contains the WinUIEdit WinRT API
- `WinUIWinMDReferences`: contains the merged WinUI winmds. Primarily consumed by test applications.

Usage:
```xml
<ItemGroup>
  <Reference Include="@(WinUIWinMDReferences)" />
</ItemGroup>
```

#### Private vs. Public Metadata

If you want to include the private and experimental metadata and types, you can set the `IncludePrivateMetadata`
property to true, and this will include the proper references. This works for any project, including test applications
and sample apps.

```xml
<PropertyGroup>
  <IncludePrivateMetadata>true</IncludePrivateMetadata>
</PropertyGroup>
```

### Xaml Compilation

If your project needs to compile `.xaml` files into `.xbf`, we want to ensure that the
`Microsoft.UI.Xaml.Markup.Compiler.dll` that is produced from source in this repository is used,
rather than one that ships in the SDK. In order to ensure this happens, simply set the `UseXamlCompiler`
property to `true` in your project. This will ensure that the project is built and the proper targets are imported.

```xml
<PropertyGroup>
  <UseXamlCompiler>true</UseXamlCompiler>
</PropertyGroup>
```

### Creating project files

It's fairly common (and expected) that developers will copy/paste other project files to get started on a new one. Not
all project files have the same Imports right now, and most differ based on whether they are custom types projects in
the `\dxaml\test` or in the `\controls` directory. The two important things are to import `Xaml.Cpp.props` at the top of
the project and `Microsoft.UI.Xaml.Build.targets` at the bottom of the project. This will ensure that the rest of the
build system works as expected.

#### C++

```xml
<Project DefaultTargets="Build" ToolsVersion="15.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <Import Project="$([MSBuild]::GetPathOfFileAbove(Xaml.Cpp.props))"/>

  <!-- Interesting project stuff  -->

  <Import Project="$(XamlSourcePath)\Microsoft.UI.Xaml.Build.targets" />
</Project>
```

#### .NET

.NET projects should reference the `Microsoft.NET.Sdk` MSBuild Sdk and set the `TargetFramework` property. Depending on
the complexity of the project, this might be all that is needed. Compile items will be automatically included for you.

```xml
<Project Sdk="Microsoft.NET.Sdk">

  <PropertyGroup>
    <TargetFramework>netstandard2.0</TargetFramework>
  </PropertyGroup>

</Project>
```
#### No Targets

Some projects don't really require compilation of any sort and just run manual targets and/or Binplacing. To do this,
reference the `Microsoft.Build.NoTargets` MSBuild SDK and give your project the `.msbuildproj` extension. Note the
extension isn't required, it's just a formality. Below is an example of a project that just binplaces a xaml file to the
`TestBinplaceResourcesDestinationPath`.

```xml
<Project Sdk="Microsoft.Build.NoTargets">
  <PropertyGroup>
    <TargetFramework>netstandard2.0</TargetFramework>
    <TargetDestination>$(TestBinplaceResourcesDestinationPath)</TargetDestination>
  </PropertyGroup>
  <ItemGroup>
    <BinplaceItem Include="foo.xaml" />
  </ItemGroup>
</Project>
```

### Using the product

If you are just trying to use the product bits in an app from VS, it's recommended
to follow the steps in the [Developer Guide for Testing and Debugging](../developer-guide-testing-and-debugging.md).

If you're trying to test the Nuget, but your project doesn't belong in the `\Samples` directory,
then you can set the `AdHocApp` property to true:

```xml
<PropertyGroup>
  <AdHocApp>true</AdHocApp>
</PropertyGroup>
```

This is equivalent to specifying a `PackageReference` to the `Microsoft.WindowsAppSDK` nuget package, but instead
of using a Nuget cache, it uses the "psuedo" Nuget cache that we create as part of our build as described in the
[Packaging](#packaging) section.

**This is** the preferred approach to any test/sample project that needs to reference WinUI.

## C++ Projects

### MIDL

To properly build a project containing files with the `.idl` extension, you need to set the following properties:

```xml
<PropertyGroup>
  <IsWinmdProject>true</IsWinmdProject>
</PropertyGroup>
```
This will ensure that the proper targets are included and run. To include files in your MIDL compilation, include them
in the standard MSBuild item `Midl`:

```xml
<ItemGroup>
  <Midl Include="Foo.idl" />
</ItemGroup>
```

The default behavior will merge your `.idl` file with any explicit references to produce a merged winmd that is ready
for consumption.

#### Non-merged IDL projects

A select few projects produce winmds that are not intended for consumption, and will be merged into a larger WinMD.
Anything under the `dxaml\xcp\dxaml\idl\winrt` directory (except for the aptly named `merged` directory) falls into
this category, as well as the `maps` and `inkcontrols` folders. You can opt out of having your winmd merged by
specifying this property:

```xml
<PropertyGroup>
  <MergeWinMD>false</MergeWinMD>
</PropertyGroup>
```

#### Merged IDL Projects

**General usage**

The "standard" WinRT way of creating winmds and assemblies is to have one winmd per assembly. For projects that follow
this convention, nothing else is needed to be done. Just follow the instructions at the beginning of the [Midl](#midl)
section.

**Final Product metadata**

The WinUI metadata does not follow the traditional WinRT rules for naming assemblies. Due to this, we have a few
special projects that handle the merging of these assemblies.

If your project **does** merge the product winmd into one that is capable of consumption, you need to specify the
`OutputWinmds` that are a result of the merge, as well as the `WinMdNamespaceOptions`, which instructs mdmerge on how
to merge the winmds.

```xml
<PropertyGroup>
    <WinMdNamespaceOptions>$(WinMdNamespaceOptions) -n:Microsoft.UI.Private:3</WinMdNamespaceOptions>
    <OutputWinmds>
      $(OutputWinmds);
      $(OutDir)winmd\Microsoft.UI.Private.winmd;
    </OutputWinmds>
</PropertyGroup>
```

Note that these are tightly coupled, and the `OutputWinmds` property is solely used for incremental builds of the
`_ComposedMetadata` target.

The `<OutputWinmd>` property **only** needs to be specified for these special projects, and it's recommended to follow
the "golden path" of having a proper 1-1 mapping.

The final product metadata strips out experimental types, by setting the `StripVelocityFromMetadata` property to true.

```xml
<PropertyGroup>
  <StripVelocityFromMetadata>true</StripVelocityFromMetadata>
</PropertyGroup>
```

All of this build logic is in [eng\midl.targets](../../eng/midl.targets)

### Precompiled Headers

Because of the layout of many of our projects, we have a custom way of defining precompiled headers. To have your
project use one of our shared precompiled headers, set the following property:

```xml
<PropertyGroup>
  <XamlPrecompiledShared>..\pch\precomp.h</XamlPrecompiledShared>
</PropertyGroup>
```

See [dxaml\msbuild\buildsettings\PreCompHeader.targets](../../dxaml/msbuild/buildsettings/PreCompHeader.targets).

### Using CPP/WinRT

Cpp/WinRT is not enabled for code in the `\dxaml\` directory due to the binary size increase caused by exceptions.

### Adding custom types to test projects

Many test projects require use of custom types to be embedded in the TAEF test assembly. Back in the Windows OS repo,
the build system was not well equipped to build test projects that used MSBuild and invoked the xaml compiler to
generate the necessary type info that allowed the runtime to activate types in the TAEF application.  The infrastructure
to support this depends on the MSBuild project name being `customTypes.vcxproj`. There are a few examples in the repo
that follow this pattern. The following things are important to consider when working with these projects:
- The resulting `.dll` from this project is not used, and instead the generated code is compiled directly into the test
binary that wants to use those types.
- These projects should include the [CustomTypeBase.props](../../dxaml/test/native/external/CustomTypeBase.props) file in
the `\dxaml\test\native\external` directory. See existing inclusion of this file for projects that currently do this.
- All `*.xbf` and `*.xaml` files are binplaced to `$(TestBinplaceDestinationPath)`. This is because in order for APIs
like `Frame.Navigate` to work, the file containing the `Page` must be located next to the test binary.

### Consuming WinUIDetails

Consuming WinUIDetails is done automatically for projects, and the include path to the headers is included automatically.
To see where this is done, or change the default behavior, see [eng\winuidetails.targets](../../eng/winuidetails.targets).
The default right now is that only `.vcxproj` files binplace the assembly. If you need to opt-out of the automatic
inclusion, you can set the `UseWinUIDetails` property to `false`.

```xml
<PropertyGroup>
   <UseWinUIDetails>false</UseWinUIDetails>
</PropertyGroup>
```

## .NET Projects
While the majority of WinUI is native, there are some test projects and tool projects, including the markup compiler,
that use managed code.

### Consuming native projects from .NET6

C#/WinRT is the new language projection mechanism for .NET Core. The principles behind it are loosely similar to
Cpp/WinRT, in that they produce regular .NET assemblies. This removes the need for the .NET runtime to have special
knowledge of WinRT. See the [CS/WinRT GitHub repo](https://github.com/microsoft/cswinrt) if you are interested in
learning more.

The official C#/WinRT usage doc in https://github.com/microsoft/CsWinRT contains information on
how to build projections.

### Central Package Versions

There is an MSBuild SDK project that allows you to have a single place in your repo where you control all package
versions, and is supported by the Visual Studio UI. This support will soon be built into
[Nuget by default](https://github.com/NuGet/Home/wiki/Centrally-managing-NuGet-package-versions), but until then, we
use the existing
[CentralPackageVersions](https://github.com/microsoft/MSBuildSdks/tree/main/src/CentralPackageVersions)
MSBuild SDK that gives us the same experience.

For an example of how this works, see
[NetCoreDesktopSample.csproj](../../Samples/WinUIDesktop/NetCoreDesktopSample/NetCoreDesktopSample.csproj)

## Building the VSIX

The WinUI 3 Project Templates VSIX is now obsolete.

## Building the C#/WinRT Interop Assembly

The C#/WinRT interop assembly that goes into the Microsoft.WindowsAppSDK nuget is built from the
[src\projection\Microsoft.WinUI.csproj](../../src/projection/Microsoft.WinUI.csproj) project. This project uses the
built-in C#/WinRT support for generating the assembly, so please see the official
C#/WinRT usage doc in https://github.com/microsoft/CsWinRT on how to build projections if you
need to learn more.
