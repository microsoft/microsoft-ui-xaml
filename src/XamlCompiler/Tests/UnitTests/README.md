# XAML Compiler Unit Tests

`src\XamlCompiler\runtests.cmd` runs the XamlCompiler unit tests produced by the
current WinUI build environment.

## Quick start

Use the same command prompt for all four steps:

```cmd
init.cmd <flavor>
msbuild src\XamlCompiler\XamlCompiler.sln /restore
msbuild src\XamlCompiler\XamlCompilerTests.sln /restore
src\XamlCompiler\runtests.cmd
```

`XamlCompiler.sln` builds the compiler and product dependencies.
`XamlCompilerTests.sln` builds the regression projects, unit-test support
projects, and runnable test payload.

`runtests.cmd` assumes that `init.cmd` initialized the prompt and that the
solution build completed for that flavor. It does not initialize tools, build
projects, modify project files, or copy test binaries.

## Usage

```cmd
src\XamlCompiler\runtests.cmd [vstest arguments]
```

All arguments are forwarded unchanged to `vstest.console.exe`.

Examples:

```cmd
src\XamlCompiler\runtests.cmd
src\XamlCompiler\runtests.cmd /Tests:TestProxies
src\XamlCompiler\runtests.cmd /TestCaseFilter:"FullyQualifiedName~Basic01"
src\XamlCompiler\runtests.cmd "/Logger:trx;LogFileName=XamlCompilerTests.trx"
```

The flavor is always taken from the initialized environment:

```text
%_BuildArch%%_BuildType%
```

There are no script arguments for selecting a configuration, platform, flavor,
runtime identifier, or Visual Studio version.

## Test payload

The script runs `UnitTests.dll` directly from:

```text
%BuildOutputRoot%\%_BuildArch%%_BuildType%\src\XamlCompiler\Tests\UnitTests\XamlCompilerUnitTests
```

The project build places the test assembly, compiler assemblies, managed test
inputs, reference closures, codegen targets, and test masters in that directory.
Running from the build output avoids a second staging directory and stale copied
files.

The build creates `XamlCompilerUnitTests.payload.complete` only after all required
payload inputs have been validated and copied successfully. Before starting
VSTest, the script checks that marker and `UnitTests.dll`. If either is missing,
complete the compiler/test-solution sequence above and retry.

## VSTest discovery

The script resolves `vstest.console.exe` without assuming a Visual Studio
version or edition. It checks, in order:

1. `VSTEST_CONSOLE`, when explicitly set.
2. The initialized `PATH`.
3. `DevEnvDir` and `VSINSTALLDIR` from the Visual Studio developer environment.
4. A version-independent `vswhere` lookup.

If VSTest still cannot be found, rerun `init.cmd <flavor>` and ensure the Visual
Studio Test Platform is installed.

## Test settings

The script uses `test.runsettings` from this directory. Its
`DeploymentEnabled=false` setting is required because the build output already
contains the complete dependency set next to `UnitTests.dll`; MSTest deployment
would otherwise run from a partial copied payload.

The script returns VSTest's exit code unchanged.

## Array metadata regressions

`Schema_CppWinRTArray*` and `Schema_ArrayTypeNamesPreserveOtherLanguages` cover
array type projection. `Codegen_MetadataTestbedCppWinRT*` compares the generated
metadata for both ordinary and incremental C++/WinRT code generation.
Array element tests include the exact `guid` and `char16_t` projections, which
cannot use the compiler's legacy scalar `GUID` and `wchar_t` aliases.

`Codegen_EventHandlingCppWinRT` covers XAML-attached delegates with `UInt32`,
`Guid`, and `Char` pass, fill, and receive arrays, plus a scalar out string.
The native fixture verifies that generated parameter declarations agree with
the C++/WinRT projection as well as with the codegen masters.

`Metadata_ArrayPropertiesCppWinRT*` launches each metadata testbed with
`--test-array-metadata`. This mode runs without creating a XAML application or
window. It exercises the generated `IXamlMetadataProvider` and `IXamlMember`
getters/setters on a plain WinRT model, including empty arrays, invalid boxed
values, read-only properties, and attached properties. Both testbed projects
must be built for the test assembly's flavor; they are included in
`XamlCompilerTests.sln`.

The round trips use C++/WinRT's existing array boxing support. This projection
fix does not add generalized boxing for enum, custom-struct, or runtime-class
element types.
