# Source code structure

## Table of Contents

- [/build, /tools](#build-tools)
- [/controls](#controls)
- [/docs](#docs)
- [/dxaml](#dxaml)
- [/eng](#eng)
- [/Helix](#helix)
- [/src](#src)

## /build, /tools
These folders contain scripts and other support machinery that you shouldn't 
need to edit for most changes.

In particular: /build/NuSpecs enables .nupkg generation

## /controls
This folder contains all the source code for the Microsoft.UI.Xaml.Controls.dll 
(formerly all WinUI2.x code).

See the [source code structure doc](../controls/docs/source_code_structure.md) for more 
information about the controls section of the repo. 

## /docs
This is where the repo documentation lives, including this document.

Note that developer usage documentation can be found separately on docs.microsoft.com.

## /dxaml
This is where the majority of WinUI source code is. This contains all the test and source code
for Microsoft.UI.Xaml.dll and Microsoft.UI.Xaml.Phone.dll.

## /eng
All build system and other engineering related files go in this directory.
For more information on the build system, see the [build system design](build-system-design.md)

## /Helix
This folder contains scripts as it pertains to running tests in Helix. You shouldn't
need to edit these for most changes, unless you are doing test infrastructure work.

## /src
This is where source code for the Xaml Compiler and VSIX are located.
