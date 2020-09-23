# Migrating a UWP App To WinUI3

**It is recommended to use the  [try-convert](https://github.com/dotnet/try-convert/blob/feature/winui) tool for the full conversion process.**
**[try-convert](https://github.com/dotnet/try-convert/blob/feature/winui) will automate most of the conversion steps.**

## Background

The purpose of the [try-convert WinUI3 feature](https://github.com/dotnet/try-convert/tree/feature/winui) is to create a porting solution to allow developers to convert WinUI2 projects to the new WinUI3 format.

- WinUI is a native user experience (UX) framework for both Windows Desktop and UWP applications. WinUI ships as part of the Windows OS. 
[More on WinUI](https://microsoft.github.io/microsoft-ui-xaml), [Docs](https://docs.microsoft.com/en-us/windows/apps/winui)
- WinUI3 is the next version of WinUI. It runs on the native Windows 10 UI platform and supports both Windows Desktop and UWP apps. WinUI3 ships as a NuGet package.
[More on WinUI3](https://docs.microsoft.com/en-us/windows/apps/winui/winui3)

Converting an existing WinUI C# App to WinUI3 requires some changes to the C# source code. Most notably namespace changes from Windows.UI.* to Microsoft.UI.*

This porting assistance is provided in the form of Roslyn Analyzers and Code Fixes. 
- A Code Analyzer provides on the fly code inspections for C# and creates diagnostics. 
- A Code Fix consumes the diagnostics created by the Code Analyzer and modifies the C# file in-line. 

See the [try-convert documentation](https://github.com/dotnet/try-convert/blob/feature/winui) for more information on these code analyzers and how to automoate the process with [try-convert](https://github.com/dotnet/try-convert/blob/feature/winui/WinUIConvert.md).