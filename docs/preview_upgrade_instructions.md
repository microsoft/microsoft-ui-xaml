## Upgrading steps
1. Make sure all Preview 3 prerequisites are installed. See [installation instructions here](https://aka.ms/winui3/preview3#install-winui-30-preview-3).
2. Use the NuGet package manager (right-click on the project and select “Manage NuGet Packages…” from the context menu)
3. Run the following command in your NuGet package manager console:

    `install-package Microsoft.WinUI -Version 3.0.0-preview2.200713.0` 
    
    (TODO: update with final Preview 3 package version)
4. If your project is a C# “Blank App (UWP)”, C# “Class Library (UWP)”, C# “Windows Runtime Component (UWP)”, C++ “Blank App (UWP)”, or C++ “Windows Runtime Component (UWP)” project, then **no further changes are necessary.**
5.  If your project is a C# “Class Library (WinUI in Desktop)” project:
    - Open the .csproj
        - Change `<TargetFramework>net5.0</TargetFramework>` to `<TargetFramework>net5.0-windows10.0.18362.0</TargetFramework>`
        - Delete `<TargetPlatformVersion>10.0.18362.0</TargetPlatformVersion>`
        - Delete `<Platforms>AnyCPU;x86;x64</Platforms>`
        - Change `<RuntimeIdentifiers>win-x86;win-x64</RuntimeIdentifiers>` to `<RuntimeIdentifiers>win10-x86;win10-x64;win10-arm64</RuntimeIdentifiers>`
        - Delete `<PackageReference Include="Microsoft.VCRTForwarders.140" Version="1.0.6" />`

6. If your project is a C# “Blank App, Packaged (WinUI in Desktop)” project :
    - Open the .csproj in the main app project
        - Change `<TargetFramework>net5.0</TargetFramework>` to `<TargetFramework>net5.0-windows10.0.18362.0</TargetFramework>`
        - Delete `<TargetPlatformVersion>10.0.18362.0</TargetPlatformVersion>`
        - Change `<Platforms>AnyCPU;x86;x64</Platforms>` to `<Platforms>x86;x64;arm64</Platforms>`
        - Delete `<SelfContained>true</SelfContained>` and `<RuntimeIdentifier>win-$(Platform)</RuntimeIdentifier>`
        - Change `<RuntimeIdentifiers>win-x86;win-x64</RuntimeIdentifiers>` to `<RuntimeIdentifiers>win10-x86;win10-x64;win10-arm64</RuntimeIdentifiers>`
        - Open App.xaml.cs and delete the line `"using Microsoft.UI.Threading;"`
        - Create a new C# “Blank App, Packaged (WinUI in Desktop)” project, locate the folder containing the associated main app project, and copy the contents of the `“Properties\”` subdirectory (`"PublishProfiles\"`) into the corresponding `“Properties\”` subdirectory of your main app project, creating the `"Properties\"` subdirectory if necessary.

    - Open the .wapproj in the associated Windows Application Packaging Project and make the following edits:
        - Add the following XML to the `<ItemGroup Label="ProjectConfigurations">` section:
            ```xml
            <ProjectConfiguration Include="Debug|arm64">
                <Configuration>Debug</Configuration>
                <Platform>arm64</Platform>
            </ProjectConfiguration>
            <ProjectConfiguration Include="Release|arm64">
                <Configuration>Release</Configuration>
                <Platform>arm64</Platform>
            </ProjectConfiguration>
            ```
        - Locate the following code in your .wapproj file:

            ```xml
            <ItemGroup>
                <ProjectReference Include="..\<<APP_NAME>>\<<APP_NAME>>.csproj">
                    <SkipGetTargetFrameworkProperties>True</SkipGetTargetFrameworkProperties>
                </ProjectReference>`
            </ItemGroup>
            ```
            And add the following line right above the `</ProjectReference>` tag:
            ```xml
            <PublishProfile>Properties\PublishProfiles\win10-$(Platform).pubxml</PublishProfile>
            ```

        - Create a new C# “Blank App, Packaged (WinUI in Desktop)” project, locate the folder containing the associated Windows Application Packaging Project, and copy the contents of the `“build\”` subdirectory (`Microsoft.WinUI.AppX.targets`) into the corresponding `“build\”` subdirectory of your app’s associated Windows Application Packaging Project, overwriting any existing files when prompted. Delete the existing `LiftedWinRTClassRegistrations.xml` file in the `"build\"` subdirectory.
7. If your project is a C++ “Blank App, Packaged (WinUI in Desktop)” project:
    - Open the .wapproj in the associated Windows Application Packaging Project and make the following edits:
        - Add the following XML to the `<ItemGroup Label="ProjectConfigurations">` section:
            ```xml
            <ProjectConfiguration Include="Debug|arm64">
                <Configuration>Debug</Configuration>
                <Platform>arm64</Platform>
            </ProjectConfiguration>
            <ProjectConfiguration Include="Release|arm64">
                <Configuration>Release</Configuration>
                <Platform>arm64</Platform>
            </ProjectConfiguration>
            ```

        - Create a new C++ “Blank App, Packaged (WinUI in Desktop)” project, locate the folder containing the associated Windows Application Packaging Project, and copy the contents of the `“build\”` subdirectory (`Microsoft.WinUI.AppX.targets`) into the corresponding `“build\”` subdirectory of your app’s associated Windows Application Packaging Project, overwriting any existing files when prompted. Delete the existing `LiftedWinRTClassRegistrations.xml` file in the `"build\"` subdirectory.