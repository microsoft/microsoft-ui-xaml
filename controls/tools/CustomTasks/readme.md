# Steps needed to update the custom tasks

_Assuming your repo is in D:\microsoft-ui-xaml-lift_

# Update files and build

Increment the version number in AssemblyInfo.cs and MUXCustomBuildTasks.nuspec. Edit these :

      1. controls/dev/dll/packages.config
      2. controls/tools/CustomTasks/NuSpecs/MUXCustomBuildTasks.nuspec
      3. controls/tools/CustomTasks/Properties/AssemblyInfo.cs (2 places)
      4. packages.config
      and increment their build version from "1.0.80" to "1.0.81" for instance.

- Build CustomTasks.sln for Release
  - Under `controls`, run `MSBuild.exe CustomTasks.sln /restore /p:Configuration=Release /p:Platform="Any CPU"`
  -  If you get build errors, open the CustomTasks.sln solution in Visual Studio and make sure the latest dependent nugets are installed.

- Under `controls\tools\CustomTasks\NuSpecs`, run `BuildNupkg.cmd` which does this:
      nuget pack MUXCustomBuildTasks.nuspec -OutputDirectory .

- >In most cases, you will want to test changes to MUXCustomBuildTasks locally before pushing.
   You can do this by copying the nupkg to D:\microsoft-ui-xaml-lift\PackageStore, which can be used as a local 'feed' that nuget will search for a restore.
   This means you only need to actually publish the package when you are satisfied that everything is correct
   For actual publishing, follow this :

   Under `controls\tools\CustomTasks\NuSpecs`, run `PublishNupkg.cmd`
      You should get this kind of result:

      Candidate nuget package: D:\microsoft-ui-xaml-lift\controls\tools\CustomTasks\NuSpecs\MUXCustomBuildTasks.1.0.81-winui3.nupkg
      nuget push D:\microsoft-ui-xaml-lift\controls\tools\CustomTasks\NuSpecs\MUXCustomBuildTasks.1.0.81-winui3.nupkg -Source WinUI.Dependencies -apikey AzureDevOps
      Pushing MUXCustomBuildTasks.1.0.81-winui3.nupkg to 'https://microsoft.pkgs.visualstudio.com/_packaging/1103ce32-f206-4cab-b967-dcc556dacd13/nuget/v2/'...
            PUT https://microsoft.pkgs.visualstudio.com/_packaging/1103ce32-f206-4cab-b967-dcc556dacd13/nuget/v2/
      MSBuild auto-detection: using msbuild version '16.4.0.56107' from 'C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\MSBuild\Current\Bin'.
      Accepted https://microsoft.pkgs.visualstudio.com/_packaging/1103ce32-f206-4cab-b967-dcc556dacd13/nuget/v2/ 2317ms

      If you get this error “Response status code does not indicate success: 409 (Conflict - The feed already contains 'MUXCustomBuildTasks 1.0.81-winui3'.
      (DevOps Activity ID: CD1C8317-38A9-4A55-AC3B-1C3A4EE0F86B))., start from the first step and increment the version one more time, because there is no way
      to forcefully overwrite an existing published version.

- Under `controls\tools\CustomTasks\NuSpecs`, run `UpdateReferences.cmd`
      If you get an error :
        ```
        Exception calling "ReadAllText" with "1" argument(s): "Could not find file
        'D:\microsoft-ui-xaml-lift\MergedWinMD\MergedWinMD.targets'."
        At D:\microsoft-ui-xaml-lift\controls\tools\CustomTasks\NuSpecs\UpdateReferences.ps1:61 char:5
        +     $fileContents = [System.IO.File]::ReadAllText($filePath)
        +     ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
          + CategoryInfo          : NotSpecified: (:) [], MethodInvocationException
          + FullyQualifiedErrorId : FileNotFoundException
        ```
      running UpdateReferences.cmd a second time did not cause the same error for me.

- Under `eng`, edit the file `versions.props` and change the `MuxCustomBuildTasksPackageVersion` tag's version from 1.0.80-winui3 to 1.0.81-winui3 for example.

- In the repo root, run `Init.cmd` again
      It can sometimes take a little while for the NuGet feed to update, so Init.cmd can sometimes say that the package wasn't found - in that case, you just need to wait a little while.
