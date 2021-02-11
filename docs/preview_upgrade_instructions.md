## Upgrading steps
1. Make sure all Preview 4 prerequisites are installed. See [installation instructions here](https://aka.ms/winui3/preview4#install-winui-30-preview-4).
2. Open the NuGet package manager console (Tools > Nuget Package manager > Package Manager Console)
3. Run the following command in the NuGet package manager console:

    `install-package Microsoft.WinUI -Version 3.0.0-preview4.210202.1-CI`

4. If your project is a C# “Blank App (UWP)”, C# “Class Library (UWP)”, C# “Windows Runtime Component (UWP)”, C++ “Blank App (UWP)”, or C++ “Windows Runtime Component (UWP)” project, then **no further changes are necessary.**

5. If your project is a C# “Blank App, Packaged (WinUI in Desktop)” project:
    - Open the .csproj in the main app project
        - Locate the folder `“Properties\PublishProfiles\”` and in each `.pubxml` file replace the following line:
          ```xml
          <PublishTrimmed>True</PublishTrimmed>
          ```
          with:
          ```xml
          <!--
          See https://github.com/microsoft/CsWinRT/issues/373
          <PublishTrimmed>True</PublishTrimmed>
          -->
          ```

    - Open the .wapproj in the associated Windows Application Packaging Project and delete the following lines:
      ```xml
      <ItemGroup>
        <SDKReference Include="Microsoft.VCLibs.Desktop, Version=14.0" />
        <!-- Needed for ucrtbased.dll when running a debug build. -->
        <SDKReference Include="Microsoft.VCLibs, Version=14.0" Condition="'$(Configuration)' == 'Debug'" />
      </ItemGroup>
      ```

    - Create a new C# “Blank App, Packaged (WinUI in Desktop)” project, locate the folder containing the associated Windows Application Packaging Project, and copy the contents of the `“build\”` subdirectory (`Microsoft.WinUI.AppX.targets`) into the corresponding `“build\”` subdirectory of your app’s associated Windows Application Packaging Project, overwriting any existing files when prompted.

6. If your project is a C++ “Blank App, Packaged (WinUI in Desktop)” project:
    - Use the NuGet package manager to uninstall the NuGet package `Microsoft.VCRTForwarders.140` from the main app package.

    - Open the .vcxproj in the main app project
        - Locate the following XML element:
          ```xml
          <PropertyGroup Label="Configuration">
          ...
          </PropertyGroup>
          ```
          and add the following line of XML to its body (this sets the MSBuild property `DesktopCompatible` to `true`):
          ```xml
          <DesktopCompatible>true</DesktopCompatible>
          ```

    - Open the .wapproj in the associated Windows Application Packaging Project and delete the following lines:
      ```xml
      <ItemGroup>
        <SDKReference Include="Microsoft.VCLibs.Desktop, Version=14.0" />
        <!-- Needed for ucrtbased.dll when running a debug build. -->
        <SDKReference Include="Microsoft.VCLibs, Version=14.0" Condition="'$(Configuration)' == 'Debug'" />
      </ItemGroup>
      ```

    - Create a new C++ “Blank App, Packaged (WinUI in Desktop)” project, locate the folder containing the associated Windows Application Packaging Project, and copy the contents of the `“build\”` subdirectory (`Microsoft.WinUI.AppX.targets`) into the corresponding `“build\”` subdirectory of your app’s associated Windows Application Packaging Project, overwriting any existing files when prompted.
