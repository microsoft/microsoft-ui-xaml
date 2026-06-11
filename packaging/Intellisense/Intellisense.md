# Intellisense

We get xml files from the docs team for our WinRT APIs.  But, there are some problems with them we must resolve:
* For C#, Visual Studio looks for an Intellisense xml file next to the managed assembly with the same name (e.g.,
for Microsoft.WinUI.dll, VS will look for Microsoft.WinUI.xml).  Since WinAppSDK clients use CsWinRT, the DLLs
that WinAppSDK sees are the projection assemblies for our APIs, so we must re-arrange the Intellisense strings
into xml files that match our projection assemblies.
* There's some markup in the Intellisense strings, meant for learn.microsoft.com, but don't render correctly
in Visual Studio. We'll clean the strings before including them.
* There are some duplicate entries, we need to make sure to only save out unique entries.

### Process for ingesting updates to Intellisense files:

1. Delete contents of **Intellisense\drop**
2. Copy in the new Intellisense XML files into **Intellisense\drop**
3. Run `powershell GenerateIntellisenseFiles.ps1`.  This will read the files in the **drop** subdir and update
the files in the **Intellisense\generated** dir.  The build will pick up these files and include them in the
nuget package, alongside the dll/winmd it describes.
    * Note: This step requires outputs from a build, so be sure to run `init.cmd` and `build.cmd` first.
4. Create a topic branch, add the files in **Intellisense\generated**, and create a PR. Have a look at the
changes in `Coverage.txt` to see how the Intellisense coverage of the API surface may have changed.
