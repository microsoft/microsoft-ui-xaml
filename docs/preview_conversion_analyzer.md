<!-- The purpose of this spec is to describe a new feature and
its APIs that make up a new feature in WinUI. -->

<!-- There are two audiences for the spec. The first are people
that want to evaluate and give feedback on the API, as part of
the submission process.  When it's complete
it will be incorporated into the public documentation at
docs.microsoft.com (http://docs.microsoft.com/uwp/toolkits/winui/).
Hopefully we'll be able to copy it mostly verbatim.
So the second audience is everyone that reads there to learn how
and why to use this API. -->
# Converting To WinUI3 Using CSharp Conversion Analyzers

## Background
<!-- Use this section to provide background context for the new API(s) 
in this spec. -->
The purpose of this project is to create a porting solution in the form of Code Analyzers/Fixes to allow Developers to convert WinUI2 projects to the new WinUI3 format. As Microsoft moves toward a purely public SDK, its Xaml UI components are being lifted out of the OS and deployed in WinUI3.

- WinUI2 is an official library with support for native Windows UI elements for Windows apps. 
- WinUI3 is the new offical UI framework for both Windows Desktop and UWP apps. 

<!-- This section and the appendix are the only sections that likely
do not get copied to docs.microsoft.com; they're just an aid to reading this spec. -->

<!-- If you're modifying an existing API, included a link here to the
existing page(s) -->

<!-- For example, this section is a place to explain why you're adding this API rather than
modifying an existing API. -->

<!-- For example, this is a place to provide a brief explanation of some dependent
area, just explanation enough to understand this new API, rather than telling
the reader "go read 100 pages of background information posted at ...". -->


## Description
<!-- Use this section to provide a brief description of the feature.
For an example, see the introduction to the PasswordBox control 
(http://docs.microsoft.com/windows/uwp/design/controls-and-patterns/password-box). -->
This tool assists with the conversion process by providing Code Analyzers/Fixes to convert existing WinUI2 C# projects to WinUI3.

### What It Does

- Analyze/Convert C# files
- Updates Namespaces for Xaml Types from Windows.UI.Xaml to Microsoft.UI.Xaml
    - Windows.UI.Xaml is now Microsoft.UI.Xaml, so usings and explicit namespaces need to be updated
- Converts App.onLaunched Method
    - 2 Updates need to be made to the App OnLaunched method when converting to WinUI3
    1. Target Microsoft.UI.Xaml.LaunchactivatedEvenArgs as the method parameter type
    2. Instances of the parameter name in the App OnLaunched method body must invoke .UWPLaunchActivatedEventArgs
- Highlights Deprecated Types
    - Some Types such as Windows.UI.Input.Inking, and Windows.UI.Xaml.Media.AcrylicBackgroundSource are not supported in WinUI3. These may be identified by the analyzer as deprecated but cannot be converted automatically.

### What It Does Not Do

 - Resolve conflicting packages
 - Remove Deprecated Code
 - Analyze/Convert .xaml files

## Examples
<!-- Use this section to explain the features of the API, showing
example code with each description. The general format is: 
  feature explanation,
  example code
  feature explanation,
  example code
  etc.-->
  
<!-- Code samples should be in C# and/or C++/WinRT -->
### Convert Namespaces:
Before Convert :

![Visual Studio Lightbulb Suggestion](images\namespaceBefore.png#thumb)

After:

![Visual Studio Lightbulb Suggestion](images\namespaceAfter.png#thumb)

### Convert App.OnLaunched Method:
Before Convert :

![Visual Studio Lightbulb Suggestion](images\onlaunchedBefore.png#thumb)

After:

![Visual Studio Lightbulb Suggestion](images\onlaunchedAfter.png#thumb)


## Conversion Process
<!-- Explanation and guidance on how to use the converter that doesn't fit into the Examples section. -->

1. Install Microsoft.WinUI and WinUIConvert NuGet packages in your app using the NuGet package manager: see the [Getting Started](https://docs.microsoft.com/uwp/toolkits/winui/getting-started) with the Windows UI Library page for more information.
    
    ![Visual Studio Lightbulb Suggestion](\images\newPackage.png#thumb)

2. Uninstall Microsoft.UI.Xaml from your solution. Additional conflicting packages such as Microsoft.Xaml.Behaviours may also need to be removed. 
    
    ![Visual Studio Lightbulb Suggestion](\images\uninstallOldPackage.png#thumb)

3. Light Bulb suggestions should highlight issues that need to be updated for WinUI3 conversion. 

    ![Visual Studio Lightbulb Suggestion](\images\lightbulb.png#thumb)

4. Click the down arrow by the lightbulb, Convert to WinUI3 and select Fix all occurences in project. 

    ![Visual Studio Lightbulb Suggestion](\images\analyzer-1.png#thumb)

5. All namespace changes should be fixed in your project!

6. The Analyzer can only parse CSharp code and not Xaml. WebView is now WebView2 and should be updated: see the [Getting Started](https://docs.microsoft.com/en-us/microsoft-edge/webview2/gettingstarted/winui) page for more information.
---

Note: some WInUI2 resources are not compatible with WinUI3. These issues may be highlighted in code but cannot be fixed by the converter. 

![Visual Studio Lightbulb Suggestion](images\deprecatedWarning.png#thumb)

<!--
## Appendix
 Anything else that you want to write down for posterity, but 
that isn't necessary to understand the purpose and usage of the API.
For example, implementation details. -->

<style>
img[src*="#thumb"] {
   margin-left:3%;
   width:50%;
   height:auto;
}
</style>