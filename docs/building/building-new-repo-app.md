# Creating a New Test App in the repo

## Table of Contents

- [For C# apps using the legacy project format](#for-c-apps-using-the-legacy-project-format)
- [For C# apps using the new SDK-style project format](#for-c-apps-using-the-new-sdk-style-project-format)
- [For C++ apps](#for-c-apps)

If you'd like to create a new app in the Xaml repo, there are a few simple steps to get the app up and running in
Visual Studio and using your local bits:

1. Create the Windows App SDK app in Visual Studio (C++ or C#) and place it in the `\Samples\` directory.
2. Modify the project files. In VS you have to unload the project first by right-clicking on the project in the
Solution Explorer and selecting "Unload Project".

## For C# apps using the legacy project format

Remove the `PackageReference` to `Microsoft.WindowsAppSDK` that looks like this:

*Note the version may be different*

```xml
<PackageReference Include="Microsoft.WindowsAppSDK">
  <Version>1.2-stable</Version>
</PackageReference>
```

 ## For C# apps using the new SDK-style project format
 
 Change the `PackageReference` to `Microsoft.WindowsAppSDK` to look like this:

```xml
<PackageReference Include="Microsoft.WindowsAppSDK"/>
```

Notice the lack of `Version` being specified. This is intentional, and so add this line at the top of the .csproj:

```xml
<Sdk Name="Microsoft.Build.CentralPackageVersions" Version="2.0.1" />
```

## For C++ apps

Ensure that the import of `Microsoft.WindowsAppSDK.props` is moved below the reading of version variables here:

```xml
<Import Project="$(VCTargetsPath)\Microsoft.Cpp.Default.props" />
```

And update the import to used project variables:

```xml
<Import Project="$(NugetPackageDirectory)\Microsoft.WindowsAppSDK.$(WindowsAppSdkPackageVersion)\build\native\Microsoft.WindowsAppSDK.props" 
        Condition="Exists('$(NugetPackageDirectory)\Microsoft.WindowsAppSDK.$(WindowsAppSdkPackageVersion)\build\native\Microsoft.WindowsAppSDK.props')" />
```

Do the same for the `Microsoft.WindowsAppSDK.targets` import at the bottom of the project file:

```xml
<Import Project="$(NugetPackageDirectory)\Microsoft.WindowsAppSDK.$(WindowsAppSdkPackageVersion)\build\native\Microsoft.WindowsAppSDK.targets" 
        Condition="Exists('$(NugetPackageDirectory)\Microsoft.WindowsAppSDK.$(WindowsAppSdkPackageVersion)\build\native\Microsoft.WindowsAppSDK.targets')" />
```

Finally, update the `Error` lines inside the `EnsureNuGetPackageBuildImports` target:

```xml
<Error Condition="!Exists('$(NugetPackageDirectory)\Microsoft.WindowsAppSDK.$(WindowsAppSdkPackageVersion)\build\native\Microsoft.WindowsAppSDK.props')" 
        Text="$([System.String]::Format('$(ErrorText)', '$(NugetPackageDirectory)\Microsoft.WindowsAppSDK.$(WindowsAppSdkPackageVersion)\build\native\Microsoft.WindowsAppSDK.props'))" />
<Error Condition="!Exists('$(NugetPackageDirectory)\Microsoft.WindowsAppSDK.$(WindowsAppSdkPackageVersion)\build\native\Microsoft.WindowsAppSDK.targets')" 
        Text="$([System.String]::Format('$(ErrorText)', '$(NugetPackageDirectory)\Microsoft.WindowsAppSDK.$(WindowsAppSdkPackageVersion)\build\native\Microsoft.WindowsAppSDK.targets'))" />
```

4. F5 and your app is now using local WinUI bits. Any time you rebuild one of the product binaries you just hit F5 and they will be used!