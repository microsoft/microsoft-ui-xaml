# How to add package to shine oss feed


1. Run nuget.exe sources list (to know whether shine oss feed is added as source or not)
If it's not present, run below command to add shine oss feed as source
2. `nuget.exe sources add -Name "WinUI-Dependencies" -Source "https://pkgs.dev.azure.com/shine-oss/microsoft-ui-xaml/_packaging/WinUI-Dependencies/nuget/v3/index.json%22 `
(Again you can verify it's added or not by running step 1 cmd)
3. Run below cmd to push the package
nuget.exe push -Source "WinUI-Dependencies" -ApiKey az \<package path\></br>
Example
`
nuget.exe push -Source "WinUI-Dependencies" -ApiKey az C:\repos\microsoft\IXPgithub\microsoft-ui-xaml\src\PackageStore\microsoft.windowsappsdk.interactiveexperiences.2.0.9-experimental.nupkg
`

## Remarks
- You should have permissions to push to this feed. Please check for push permission at: https://dev.azure.com/shine-oss/microsoft-ui-xaml/_artifacts/feed/WinUI-Dependencies
- Make sure that you get approvals from respective team before pushing them public.
