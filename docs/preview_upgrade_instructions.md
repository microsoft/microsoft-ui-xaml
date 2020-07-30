# Instructions for upgrading a WinUI 3 Preview 1 app to use WinUI 3 Preview 2

While it's possible to upgrade your Preview 1 application to use the new Preview 2 bits, please be careful and follow all steps in order below when doing so. The steps below outline the process that the team had to take while updating their own Preview 1 apps. 

If these steps don't work for you, feel free to [file an issue](https://github.com/microsoft/microsoft-ui-xaml/issues/new/choose). The easiest way to work around this is to just create a new Preview 2 project and copy your content over. 

## Upgrading steps
1. Make sure all Preview 2 prerequisites are installed. See [installation instructions here](https://aka.ms/winui3/preview2#install-winui-30-preview-2).
2. Use the NuGet package manager (right-click on the project and select “Manage NuGet Packages…” from the context menu) 
3. Select the “Microsoft.WinUI” package, ensure that “Include prerelease” is checked, select the latest version of the package, and then click “Upgrade”, accepting prompts that appear  
4. If your project is a C# “Blank App (UWP)”, C# “Class Library (UWP)”, C# “Windows Runtime Component (UWP)”, C++ “Blank App (UWP)”, or C++ “Windows Runtime Component (UWP)” project, then **no further changes are necessary.**
5.  If your project is a C# “Class Library (WinUI in Desktop)” project:
    - Open the .csproj and change <TargetFramework>netcoreapp5.0</TargetFramework> to <TargetFramework>net5.0</TargetFramework> 
6. If your project is a C# “Blank App, Packaged (WinUI in Desktop)” project :
    - Open the .csproj in the main app project and change <TargetFramework>netcoreapp5.0</TargetFramework> to <TargetFramework>net5.0</TargetFramework>  
    - Open the .wapproj in the associated Windows Application Packaging Project and make the following edits: 
        - Add the following line to the first `<PropertyGroup>` element after `<Import Project="$(WapProjPath)\Microsoft.DesktopBridge.props" />`:  

        ```xml
         <AppxTargetsLocation Condition="'$(AppxTargetsLocation)'==''">$(MSBuildThisFileDirectory)build\</AppxTargetsLocation> 
         ```

        - Change `<Import Project="build\Microsoft.WinUI.AppX.targets" />` to `<Import Project="$(AppxTargetsLocation)Microsoft.WinUI.AppX.targets" /> `

        - Create a new C# “Blank App, Packaged (WinUI in Desktop)” project, locate the folder containing the associated Windows Application Packaging Project, and copy the contents of the `“build\”` subdirectory (`LiftedWinRTClassRegistrations.xml` and `Microsoft.WinUI.AppX.targets`) into the corresponding `“build\”` subdirectory of your app’s associated Windows Application Packaging Project, overwriting any existing files when prompted.
7. If your project is a C++ “Blank App, Packaged (WinUI in Desktop)” project:
    - Open the .wapproj in the associated Windows Application Packaging Project and make the following edits: 

        - Add the following line to the first `<PropertyGroup>` element after `<Import Project="$(WapProjPath)\Microsoft.DesktopBridge.props" />`:  

        ```xml
        <AppxTargetsLocation Condition="'$(AppxTargetsLocation)'==''">$(MSBuildThisFileDirectory)build\</AppxTargetsLocation> 
        ```
        - Change `<Import Project="build\Microsoft.WinUI.AppX.targets" />` to `<Import Project="$(AppxTargetsLocation)Microsoft.WinUI.AppX.targets" />`

        - Create a new C++ “Blank App, Packaged (WinUI in Desktop)” project, locate the folder containing the associated Windows Application Packaging Project, and copy the contents of the `“build\”` subdirectory (`LiftedWinRTClassRegistrations.xml` and `Microsoft.WinUI.AppX.targets`) into the corresponding `“build\”` subdirectory of your actual project’s associated Windows Application Packaging Project, overwriting any existing files when prompted 


