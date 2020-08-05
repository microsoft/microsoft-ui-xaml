# Converting To WinUI3 Using C# Conversion Analyzers

## Background

The purpose of the Microsoft.WinUI.Convert Nuget Package is to create a porting solution to allow Developers to convert WinUI2 projects to the new WinUI3 format.

- WinUI is a native user experience (UX) framework for both Windows Desktop and UWP applications. WinUI ships as part of the Windows OS. 
[More on WinUI](https://microsoft.github.io/microsoft-ui-xaml/), [Docs](https://docs.microsoft.com/en-us/windows/apps/winui/)
- WinUI3 is the next version of WinUI. It runs on the native Windows 10 UI platform and supports both Windows Desktop and UWP apps. WinUI3 ships as a NuGet package.
[More on WinUI3](https://docs.microsoft.com/en-us/windows/apps/winui/winui3/)

Converting an existing WinUI C# App to WinUI3 requires some changes to the C# source code. Most notably namespace changes from Windows.UI.* to Microsoft.UI.*

This porting assistance is provided in the form of Code Analyzers and Code Fixes. 
- A Code Analyzer extends the Visual Studio experience and provides on the fly code inspections for C#. For most inspections these Code Analyzers provide light bulb quick fixes that can be applied to users code in the form of a Code Fix. 
- A Code Fix consumes the diagnostics created by the Code Analyzer and modifies the C# file in-line. Users may choose to apply Code Fixes one at a time or resolve all instances of a Code Fix across a File, Project, or Solution.

### What The Conversion Analyzers Do:
Analyze/Convert C# files

- Updates Namespaces for Xaml Types from `Windows.*` to `Microsoft.*`
- Types moving from `Windows` to `Microsoft`:
    - `Windows.UI.Xaml`
    - `Windows.UI.Colors`
    - `Windows.UI.ColorHelper` 
    - `Windows.UI.Composition`
    - `Windows.UI.Text`
    - `Windows.System.DispatcherQueue` 
    - `Windows.System.DispatcherQueueController`
    - `Windows.System.DispatcherQueueHandler`
    - `Windows.System.DispatcherQueuePriority`
    - `Windows.System.DispatcherQueueShutdownStartingEventArgs`
    - `Windows.System.DispatcherQueueTimer`
- Converts `App.OnLaunched` Method
    - Two updates need to be made to the `App.OnLaunched` method when converting to WinUI3:
    1. Target `Microsoft.UI.Xaml.LaunchactivatedEvenArgs` as the method parameter type
    2. Instances of the parameter name in the `App.OnLaunched` method body must invoke `UWPLaunchActivatedEventArgs`
- Highlights Deprecated Types
    - Some types such as `Windows.UI.Input.Inking`, and `Windows.UI.Xaml.Media.AcrylicBackgroundSource` are not supported in WinUI3. These may be identified by the analyzer as deprecated but cannot be converted automatically.

### What The Conversion Analyzers Do Not Do:

 - Modify .csproj files or modify/resolve conflicting NuGet packages
 - Remove deprecated Code
 - Analyze/Convert .xaml files

## Examples
### Convert Namespaces:
Before converting:

![Visual Studio Lightbulb Suggestion](./images/namespaceBefore.png#thumb)

After:

![Visual Studio Lightbulb Suggestion](./images/namespaceAfter.png#thumb)

### Convert App.OnLaunched Method:
Before converting:

![Visual Studio Lightbulb Suggestion](./images/onLaunchedBefore.png#thumb)

After:

![Visual Studio Lightbulb Suggestion](./images/onLaunchedAfter.png#thumb)


## Conversion Process
Necessary steps for converting a WinUI C# App to WinUI3: 

1. Install `Microsoft.WinUI` and `Microsoft.WinUI.Convert` NuGet packages in your app using the NuGet package manager: see the [Install WinUI 3 Preview](https://docs.microsoft.com/en-us/windows/apps/winui/winui3/#install-winui-3-preview-2) page for more information.
    
    ![Visual Studio Lightbulb Suggestion](./images/newPackage.png#thumb)

2. Uninstall Microsoft.UI.Xaml from your solution. Additional conflicting packages such as `Microsoft.Xaml.Behaviors.*` may also need to be removed. 
    
    ![Visual Studio Lightbulb Suggestion](./images/uninstallOldPackage.png#thumb)

3. Light Bulb suggestions should highlight issues that need to be updated for WinUI3 conversion. 

    ![Visual Studio Lightbulb Suggestion](./images/lightbulb.png#thumb)

4. Click the down arrow by the lightbulb, Convert to WinUI3 and select Fix all occurences in project. 

    ![Visual Studio Lightbulb Suggestion](./images/analyzer-1.png#thumb)

5. All namespace changes should be fixed in your project!

6. The Analyzer can only parse C# code and not Xaml. WebView is now WebView2 and should be updated: see the [Getting Started](https://docs.microsoft.com/en-us/windows/apps/winui/winui3/) page for more information.

7. (Optional) These Analyzers serve little purpose outside of converting A WinUI C# App. After verifying the conversion is complete you might consider uninstalling the `Microsoft.WinUI.Convert` NuGet Package.
---

Note: some WInUI2 resources are not compatible with WinUI3. These issues may be highlighted in code but cannot be fixed by the converter. 

![Visual Studio Lightbulb Suggestion](./images/deprecatedWarning.png#thumb)
