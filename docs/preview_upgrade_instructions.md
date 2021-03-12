## Upgrading your WinUI 3 app from Preview 4 to Project Reunion 0.5 Preview 

This is written for a developer who has an existing WinUI 3 Preview 4 app and wants to upgrade it to use WinUI 3 - Project Reunion 0.5 Preview. 

Before starting, make sure you have all the WinUI 3 - Project Reunion 0.5 Preview pre-requisites installed, including the Project Reunion VSIX and NuGet package. For installation instructions, see [this article](https://docs.microsoft.com/en-us/windows/apps/project-reunion/#set-up-your-development-environment). 

### If your app is a C# Blank App (UWP):

1. Go to Tools->Nuget Package Manager-> Package Manager Console in Visual Studio
2. Type in ```uninstall-package Microsoft.WinUI -ProjectName {yourProject}```
3. Type in ```install-package Microsoft.ProjectReunion -Version 0.5.0-prerelease -ProjectName {yourProject}```


### If your app is a C# Blank App Packaged (WinUI in Desktop):

Go to Tools->Nuget Package Manager-> Package Manager Console in Visual Studio

  1. Right Click your Application (Package) folder in the solution explorer and select "unload project"
  2. Type in ```uninstall-package Microsoft.WinUI -ProjectName {yourProject}```
  3. Type in ```install-package Microsoft.ProjectReunion -Version 0.5.0-prerelease -ProjectName {yourProjectName}```
  4. Type in ```install-package Microsoft.ProjectReunion.Foundation -Version 0.5.0-prerelease -ProjectName {yourProjectName}```
  5. Type in ```install-package Microsoft.ProjectReunion.WinUI -Version 0.5.0-prerelease -ProjectName {yourProjectName}```
  6. Reload the (package) folder

  7. Make the following changes in your Application (package).wapproj:
  
  Add this section:
  ```xml
  <PropertyGroup>
      <!--PackageReference.GeneratePathProperty does not support NUGET_PACKAGES env var...-->
      <NuGetPackageRoot Condition="'$(NuGetPackageRoot)'==''">$(NUGET_PACKAGES)</NuGetPackageRoot>
      <NuGetPackageRoot Condition="'$(NuGetPackageRoot)'==''">$(UserProfile)\.nuget\packages</NuGetPackageRoot>
      <PkgMicrosoft_ProjectReunion Condition="'$(PkgMicrosoft_ProjectReunion)'==''">$([MSBuild]::NormalizeDirectory('$(NuGetPackageRoot)', 'Microsoft.ProjectReunion', '0.5.0-prerelease'))</PkgMicrosoft_ProjectReunion>
      <PkgMicrosoft_ProjectReunion Condition="!Exists($(PkgMicrosoft_ProjectReunion))">$(SolutionDir)packages\Microsoft.ProjectReunion.0.5.0-prerelease\</PkgMicrosoft_ProjectReunion>
      <PkgMicrosoft_ProjectReunion_WinUI Condition="'$(PkgMicrosoft_ProjectReunion_WinUI)'==''">$([MSBuild]::NormalizeDirectory('$(NuGetPackageRoot)', 'Microsoft.ProjectReunion.WinUI', '0.5.0-prerelease'))</PkgMicrosoft_ProjectReunion_WinUI>
      <PkgMicrosoft_ProjectReunion_WinUI Condition="!Exists($(PkgMicrosoft_ProjectReunion_WinUI))">$(SolutionDir)packages\Microsoft.ProjectReunion.WinUI.0.5.0-prerelease\</PkgMicrosoft_ProjectReunion_WinUI>
      <Microsoft_ProjectReunion_AppXReference_props>$([MSBuild]::NormalizeDirectory('$(PkgMicrosoft_ProjectReunion)', 'build'))Microsoft.ProjectReunion.AppXReference.props</Microsoft_ProjectReunion_AppXReference_props>
      <Microsoft_WinUI_AppX_targets>$([MSBuild]::NormalizeDirectory('$(PkgMicrosoft_ProjectReunion_WinUI)', 'build'))Microsoft.WinUI.AppX.targets</Microsoft_WinUI_AppX_targets>
    </PropertyGroup>
    <ItemGroup>
      <PackageReference Include="Microsoft.ProjectReunion" Version="[0.5.0-prerelease]" GeneratePathProperty="true">
        <ExcludeAssets>all</ExcludeAssets>
      </PackageReference>
      <PackageReference Include="Microsoft.ProjectReunion.WinUI" Version="[0.5.0-prerelease]" GeneratePathProperty="true">
        <ExcludeAssets>all</ExcludeAssets>
      </PackageReference>
    </ItemGroup>
  ```
  Then replace the following:
  ```xml
    <Import Project="$(AppxTargetsLocation)Microsoft.WinUI.AppX.targets" />
  ```
  with:
  ```xml
  <Import Project="$(Microsoft_ProjectReunion_AppXReference_props)" />
  <Import Project="$(Microsoft_WinUI_AppX_targets)" />
  ```


### If your app is a C++ Blank App (UWP):

 Go to Tools->Nuget Package Manager-> Package Manager Console in Visual Studio
 1. Type in ```uninstall-package Microsoft.WinUI -ProjectName {yourProject}```
 2. Type in ```install-package  Microsoft.ProjectReunion -Version 0.5.0-prerelease -ProjectName {yourProject}``` 
 3. Type in ```install-package Microsoft.ProjectReunion.Foundation -Version 0.5.0-prerelease  -ProjectName {yourProject}```
 4. Type in ```install-package Microsoft.ProjectReunion.WinUI -Version 0.5.0-prerelease  -ProjectName {yourProject}```

### If your app is a C++ Blank App Packaged (WinUI in Desktop):

  Go to Tools->Nuget Package Manager-> Package Manager Console in Visual Studio
 1. Type in ```uninstall-package Microsoft.WinUI -ProjectName {yourProject}```
 2. Type in ```install-package  Microsoft.ProjectReunion -Version 0.5.0-prerelease -ProjectName {yourProject}```  
 3. Type in ```install-package Microsoft.ProjectReunion.Foundation -Version 0.5.0-prerelease  -ProjectName {yourProject}```
 4. Type in ```install-package Microsoft.ProjectReunion.WinUI -Version 0.5.0-prerelease  -ProjectName {yourProject}```
5. Make the following changes to your wapproj file:
 
 Add this section:
 
 ```xml
  <PropertyGroup>
    <!--PackageReference.GeneratePathProperty does not support NUGET_PACKAGES env var...-->
    <NuGetPackageRoot Condition="'$(NuGetPackageRoot)'==''">$(NUGET_PACKAGES)</NuGetPackageRoot>
    <NuGetPackageRoot Condition="'$(NuGetPackageRoot)'==''">$(UserProfile)\.nuget\packages</NuGetPackageRoot>
    <PkgMicrosoft_ProjectReunion Condition="'$(PkgMicrosoft_ProjectReunion)'==''">$([MSBuild]::NormalizeDirectory('$(NuGetPackageRoot)', 'Microsoft.ProjectReunion', '0.5.0-prerelease'))</PkgMicrosoft_ProjectReunion>
    <PkgMicrosoft_ProjectReunion Condition="!Exists($(PkgMicrosoft_ProjectReunion))">$(SolutionDir)packages\Microsoft.ProjectReunion.0.5.0-prerelease\</PkgMicrosoft_ProjectReunion>
    <PkgMicrosoft_ProjectReunion_WinUI Condition="'$(PkgMicrosoft_ProjectReunion_WinUI)'==''">$([MSBuild]::NormalizeDirectory('$(NuGetPackageRoot)', 'Microsoft.ProjectReunion.WinUI', '0.5.0-prerelease'))</PkgMicrosoft_ProjectReunion_WinUI>
    <PkgMicrosoft_ProjectReunion_WinUI Condition="!Exists($(PkgMicrosoft_ProjectReunion_WinUI))">$(SolutionDir)packages\Microsoft.ProjectReunion.WinUI.0.5.0-prerelease\</PkgMicrosoft_ProjectReunion_WinUI>
    <Microsoft_ProjectReunion_AppXReference_props>$([MSBuild]::NormalizeDirectory('$(PkgMicrosoft_ProjectReunion)', 'build'))Microsoft.ProjectReunion.AppXReference.props</Microsoft_ProjectReunion_AppXReference_props>
    <Microsoft_WinUI_AppX_targets>$([MSBuild]::NormalizeDirectory('$(PkgMicrosoft_ProjectReunion_WinUI)', 'build'))Microsoft.WinUI.AppX.targets</Microsoft_WinUI_AppX_targets>
    <EntryPointProjectUniqueName>..\ReunionCppBlankAppDesktop\ReunionCppBlankAppDesktop.vcxproj</EntryPointProjectUniqueName>
  </PropertyGroup>
  <ItemGroup>
    <PackageReference Include="Microsoft.ProjectReunion" Version="[0.5.0-prerelease]" GeneratePathProperty="true">
      <ExcludeAssets>all</ExcludeAssets>
    </PackageReference>
    <PackageReference Include="Microsoft.ProjectReunion.WinUI" Version="[0.5.0-prerelease]" GeneratePathProperty="true">
      <ExcludeAssets>all</ExcludeAssets>
    </PackageReference>
  </ItemGroup>
```

Then replace the following:
```xml
  <Import Project="$(AppxTargetsLocation)Microsoft.WinUI.AppX.targets" />
```

with:

```xml
 <Import Project="$(Microsoft_ProjectReunion_AppXReference_props)" />
 <Import Project="$(Microsoft_WinUI_AppX_targets)" />
  ```