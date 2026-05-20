# Advanced build topics

For basic build information, see the [developer guide](../developer-guide.md)
For more information about the build system, see the [build system how-to guide](./build-system-howto.md)

## Table of Contents

- [Advanced build options](#advanced-build-options)
- [Updating solution (.sln) files](#updating-solution-sln-files)
- [WinUI components from other repos](#winui-components-from-other-repos)
- [Building custom build tasks](#building-custom-build-tasks)
- [Diagnosing GenXbf.dll build failures](#diagnosing-genxbfdll-build-failures)
  - [Debugging the XAML compiler when building locally](#debugging-the-xaml-compiler-when-building-locally)

## Advanced build options

These are available if you msbuild a solution or project.

| Option | Description |
| - | - |
| `/t:Preprocess /p:Source=myfile.cpp` | Produces myfile.pp by running the C preprocessor |
| `/p:BuildProjectReferences=false` | Builds the target project, but not the dependencies. Build may fail if dependencies have not already been built |
| `/p:BuildTestCode=false` | Don't build projects with TestCode='true' |
| `/t:SanityCheck` | Runs a validation check on some build variables |
| `/pp` | Pre-process the msbuild project (including imports) |
| `/bl` | Create a binary log (.binlog) file |

Note: all /p:k=v variables can be also passed as environment variables instead (set k=v before running msbuild)

## Updating solution (.sln) files

Solution files like Microsoft.UI.Xaml.sln require hand editing for adding/removing components. Adding or deleting
components within Visual Studio is very trivial but it cannot be used for this project. This is because opening and
saving them in Visual Studio introduces unwanted changes. A good workflow trick will be making a copy of solution file
and then making changes to the copied solution file by opening it in Visual Studio. Afterwards, using a diff between
original sln and copied sln, only necessary needed changes can be made to the original sln file. A final verification
would be starting a full build by issuing build.cmd. If all the changes are correct, the build will proceed just fine.

## WinUI components from other repos

The code in this repo contains only some of the WinUI product, there are other components outside this repo upon which
WinUI depends:

* The Windows App SDK Foundation package, which includes MRT/MRM support
* The Windows App SDK InteractiveExperiences package, which includes IXP binaries

## Building custom build tasks

WinUI uses a series of build tasks contained in a nuget package for doing a successful build. These are fetched during
the `init` step. If a modification needs to be made to the build tasks, it needs to be published to nuget store before
they can be consumed by WinUI compilation. Refer [here](../../controls/tools/CustomTasks/readme.md) for instructions to
build custom build tasks.

## Diagnosing GenXbf.dll build failures

When building this repo the first step is to compile GenXbf.dll.  Later stages of the build use that build product to
do any XAML file processing that is needed.  That means that regressions introduced to GenXbf can break the build in
confusing ways.

### Debugging the XAML compiler when building locally

An accidental regression that crashed the XAML compiler had the following unhelpful error output:

```build
<repo>\src\XamlCompiler\Targets\Microsoft.UI.Xaml.Markup.Compiler.interop.targets(629,9): XamlCompiler
error WMC0605: Failure Generating XAML Binary Format: Exception=External component has thrown an exception.
[<repo>\dxaml\test\native\external\tools\customTypes\customTypes.vcxproj]
```

Running the build under the debugger was far more helpful.  `tools/msb.cmd` was modified as follows:

```shell
:buildIt
MSBuild.exe  %args% %LoggingOpts% /m /nologo /bl /v:m /clp:Summary
exit /b %ERRORLEVEL%
```

to

```shell
:buildIt
windbg.exe MSBuild.exe  %args% %LoggingOpts% /m /nologo /bl /v:m /clp:Summary
exit /b %ERRORLEVEL%
```

The failing directory was then rebuilt using the msb.cmd wrapper to get it going under the debugger:
`msb <repo>\dxaml\test\native\external\tools\customTypes\customTypes.vcxproj`

WinDbgX then popped up and began debugging the build process which hosted GenXbf.dll.  As it ran the problem became
obvious because an `ASSERT` statement was causing the process to failfast.  Closer inspection led to the root cause
and _facepalms_ all around.
