# Converting To WinUI3 Using C# Conversion Analyzers

## Background

The purpose of this project is to create a porting solution to allow Developers to convert WinUI2 projects to the new WinUI3 format. WinUI3 will be lifted out of Windows Operating System and ships as an independant NuGet package.

- WinUI is a native user experience (UX) framework for both Windows Desktop and UWP applications. WinUI ships as part of the Windows OS.
- WinUI3 is the next version of WinUI. It runs on the native Windows 10 UI platform and supports both Windows Desktop and UWP apps. WinUI3 ships as a NuGet package.
[More on WinUI](https://docs.microsoft.com/en-us/windows/apps/winui/)

This porting assistance is provided in the form of Code Analyzers and Code Fixes. A Code Analyzer extends the Visual Studio experience and provides on the fly code inspections for C#. For most inspections these Code Analyzers provide light bulb quick fixes that can be applied to users code in the form of a Code Fix. A Code Fix consumes the diagnostics created by the Code Analyzer and modifies the C# file in-line. Users may choose to apply Code Fixes one at a time or resolve all instances of a Code Fix across a File, Project, or Solution.

## Description
This tool assists with the conversion process by providing Code Analyzers/Fixes to convert existing WinUI2 C# projects to WinUI3.

### What It Does

- Analyze/Convert C# files
- Updates Namespaces for Xaml Types from `Windows.UI.Xaml` to `Microsoft.UI.Xaml`
    - `Windows.UI.Xaml` is now `Microsoft.UI.Xaml`, so `using`s and explicit namespaces need to be updated
- Converts `App.OnLaunched` Method
    - 2 Updates need to be made to the `App.OnLaunched` method when converting to WinUI3
    1. Target `Microsoft.UI.Xaml.LaunchactivatedEvenArgs` as the method parameter type
    2. Instances of the parameter name in the `App.OnLaunched` method body must invoke `UWPLaunchActivatedEventArgs`
- Highlights Deprecated Types
    - Some types such as `Windows.UI.Input.Inking`, and `Windows.UI.Xaml.Media.AcrylicBackgroundSource` are not supported in WinUI3. These may be identified by the analyzer as deprecated but cannot be converted automatically.

### What It Does Not Do

 - Modify .csproj files or modify/resolve conflicting NuGet packages
 - Remove deprecated Code
 - Analyze/Convert .xaml files

## Examples
### Convert Namespaces:
Before converting:

![Visual Studio Lightbulb Suggestion](images\namespaceBefore.png#thumb)

After:

![Visual Studio Lightbulb Suggestion](images\namespaceAfter.png#thumb)

### Convert App.OnLaunched Method:
Before converting:

![Visual Studio Lightbulb Suggestion](images\onlaunchedBefore.png#thumb)

After:

![Visual Studio Lightbulb Suggestion](images\onlaunchedAfter.png#thumb)


## Conversion Process
Necessary steps for converting a WinUI C# app to WinUI3: 

1. Install Microsoft.WinUI and WinUIConvert NuGet packages in your app using the NuGet package manager: see the [Install WinUI 3 Preview](https://docs.microsoft.com/en-us/windows/apps/winui/winui3/#install-winui-3-preview-2) page for more information.
    
    ![Visual Studio Lightbulb Suggestion](\images\newPackage.png#thumb)

2. Uninstall Microsoft.UI.Xaml from your solution. Additional conflicting packages such as `Microsoft.Xaml.Behaviors.*` may also need to be removed. 
    
    ![Visual Studio Lightbulb Suggestion](\images\uninstallOldPackage.png#thumb)

3. Light Bulb suggestions should highlight issues that need to be updated for WinUI3 conversion. 

    ![Visual Studio Lightbulb Suggestion](\images\lightbulb.png#thumb)

4. Click the down arrow by the lightbulb, Convert to WinUI3 and select Fix all occurences in project. 

    ![Visual Studio Lightbulb Suggestion](\images\analyzer-1.png#thumb)

5. All namespace changes should be fixed in your project!

6. The Analyzer can only parse C# code and not Xaml. WebView is now WebView2 and should be updated: see the [Getting Started](https://docs.microsoft.com/en-us/windows/apps/winui/winui3/) page for more information.
---

Note: some WInUI2 resources are not compatible with WinUI3. These issues may be highlighted in code but cannot be fixed by the converter. 

![Visual Studio Lightbulb Suggestion](images\deprecatedWarning.png#thumb)
