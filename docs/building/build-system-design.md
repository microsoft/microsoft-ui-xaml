# Build System Design

## Table of Contents

- [Overview](#overview)
- [Principles](#principles)
  - [Single build system location](#single-build-system-location)
  - [Single purpose files](#single-purpose-files)
  - [Managing import order](#managing-import-order)
  - [Minimize dependencies between build system and repo layout](#minimize-dependencies-between-build-system-and-repo-layout)
  - [Common MSBuild Patterns for Maintainers](#common-msbuild-patterns-for-maintainers)
    - [Reusing common targets/properties](#reusing-common-targetsproperties)
    - [Private and Public properties/items/targets](#private-and-public-propertiesitemstargets)
- [References](#references)

## Overview
The purpose of this doc is to give a high level overview of the design of the build system and the reasons behind the design. This will include some guidelines and best practices with the intent on giving an understanding as to why those decisions have been made.  This doc is for people interested in how this repository intends to use MSBuild and not necessarily how to accomplish certain tasks. For a simpler doc that describes how to accomplish what you are hoping to do see the [how-to doc](build-system-howto.md).

It's important to realize that these are just best practices and guidelines, and that they can always be broken if needed. However, those decisions should not be made without thought, and should be considered a last case scenario. Most importantly, they should be well documented with comments about why the decision was made.

Finally, it's important to realize that the code base in it's current form does not adhere to a lot of the principles and guidelines in this doc. The goal is that overtime we can make improvements and refer back to this document when making changes.

## Principles
This doc goes over some of the higher level principles that we have agreed on thus far. If you would like to propose a new principle, or challenge one of the existing ones, please file an issue so we can have a discussion.

### Single build system location
All build system related files are in the `\eng` directory located at the root of the repository.

### Single purpose files
Each MSBuild `.props` or `.targets` file should have a single use that is easily understand by the name of the file. All files in the `\eng` folder should reflect this.

For example: `eng\midl.props` and `eng\midl.targets` 

### Managing import order
Managing the import order of `.props` and `.targets` is one of the trickiest things to get right when working with MSBuild. MSBuild is highly sensitive to the order of imports, and trying to import a file twice will cause errors. Note that conditionally importing a file based on it already being imported **is not a solution**, this just hides the issue and adds complexity to the entire system.

To help manage this, *all build system files* are imported by the [root Directory.Build.props](../../Directory.Build.props) and [root Directory.Build.targets](../../Directory.Build.targets) files automatically by any project in the repository. The only caveat being that some project files may only be imported if certain conditions are met. This has two distinct advantages over requiring manual importing in the `.vcxproj` or `.csproj` files:

1. All projects have the same import order, and there is a single place where this has to be worried about.
2. There is no possibility of someone making a change to a single project, without making updates to others and upsetting the balance. Developers don't want to, and are not expected to update every file in the entire repo when they make a change.

**Disclaimer: There has been some hesitance on using Directory.Build.props due to the "magic" of the file being auto-included and potentially causing coupling between the repository structure and build system. This implementation is up for re-evalution if this proves to be too confusing.**

### Minimize dependencies between build system and repo layout
Since all build system files are in `\eng` folder, this will help minimize dependencies between the repository structure and the build system. However, in practice we tend to combine like projects into common directories, which is where using `Directory.Build.props` can be so powerful. It can greatly reduce the number of changes required when altering the build system, allowing for code reviews to be smaller and easier to digest. It also helps ensure that all projects are updated simultaneously, keeping our build system in a more homegenous state. Any `Directory.Build.props` outside of the repo root should really only set properties that affect how the build of the directory works. For example, the `Directory.Build.props` at the root of a directory called `test` can set the `TestCode` property.

### Common MSBuild Patterns for Maintainers
Writing MSBuild code is somewhat kinda like writing regular code. The only problem is understanding how MSBuild actually works takes a PhD. The below patterns focus solely on how we can help make it easier for us to maintain our build system over time.

#### Reusing common targets/properties
Whenever possible, properties and targets should be re-used. This allows us to make updates in a single place, and not worry about projects using different versions of tools or packages, or referencing different assemblies. This should be straightforward to do with every project including the same set of MSBuild `.props` and `.targets` files.

#### Private and Public properties/items/targets
While MSBuild doesn't have the notion of private and public properties, it can be helpful to denote your properties, items, or targets with an underscore (_) to denote that they aren't intended for general consumption. This can help people who are unfamiliar with MSBuild code to look through files and try to understand what is actually relevant.

## References
* [Customizing your build](https://docs.microsoft.com/en-us/visualstudio/msbuild/customize-your-build)
* [MSBuild Best Practices pt.1](https://docs.microsoft.com/en-us/archive/msdn-magazine/2009/february/msbuild-best-practices-for-creating-reliable-builds-part-1)
* [MSBuild Best Practices pt.2](https://docs.microsoft.com/en-us/archive/msdn-magazine/2009/march/msbuild-best-practices-for-creating-reliable-builds-part-2)