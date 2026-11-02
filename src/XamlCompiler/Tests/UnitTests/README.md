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

## What the tests run against

The tests run on .NET (CoreCLR) against `XamlCompiler.dll`, the IL build of the
XAML compiler executable (`Exe\Microsoft.UI.Xaml.Markup.Compiler.Executable.csproj`):
the code the Native AOT compiler (`XamlCompiler.exe`) is compiled from, which is
the only build of the compiler. The tests themselves don't run on Native AOT (the
test framework relies on reflection); the native binary is validated end to end
by `src\XamlCompiler\runtests-native.cmd` (see below).

- `XamlCompiler.sln` builds the compiler; its IL output folder,
  `BuildOutput\obj\<flavor>\src\XamlCompiler\Exe\net10.0\Microsoft.UI.Xaml.Markup.Compiler.Executable`,
  has `XamlCompiler.dll` and its dependency closure, which the test project
  stages next to `UnitTests.dll`.
- `XamlCompilerProxies\XamlCompilerProxies.csproj` wraps the internal compiler
  types through reflection (see `ProxyHelper`, which loads `XamlCompiler.dll`).
- The test process runs with invariant globalization, like the native compiler.
- `runtests-net10.cmd` is an obsolete alias of `runtests.cmd`.
## Native AOT compiler

`src\XamlCompiler\runtests-native.cmd` (`Tests\RunNativeCompilerTests.ps1`)
builds the regression projects with the Native AOT compiler
(`UseNativeXamlCompiler=true`) and runs `CodegenTests` against the masters. With
`-Differential`, it also builds them with the default compiler (the in-process
.NET Framework task) first and
requires the native compiler to generate byte-identical files. With `-Samples`,
it does the same differential comparison for real sample apps from `Samples\`
(see the script header for the list and the requirements).

## Primitive and array metadata regressions

`Schema_CppWinRTArray*` and `Schema_ArrayTypeNamesPreserveOtherLanguages` cover
scalar and array type projection. `Schema_CppCXPrimitiveTypeNames` verifies that
the C++/CX scalar and array projections remain unchanged.
`Codegen_MetadataTestbedCppWinRT*` compares the generated metadata for both
ordinary and incremental C++/WinRT code generation. Scalars and array elements
use the same primitive mapping, including `winrt::guid` and `char16_t`, rather
than the unsupported `GUID` and `wchar_t` types for C++/WinRT boxing.

`Codegen_EventHandlingCppWinRT` covers XAML-attached delegates with `UInt32`,
`Guid`, and `Char` pass, fill, and receive arrays, plus a scalar out string.
The native fixture verifies that generated parameter declarations agree with
the C++/WinRT projection as well as with the codegen masters.

`Metadata_ArrayPropertiesCppWinRT*` launches each metadata testbed with
`--test-array-metadata`. This mode runs without creating a XAML application or
window. It exercises the generated `IXamlMetadataProvider` and `IXamlMember`
getters/setters on a plain WinRT model, including scalar `Char` and `Guid`
properties, empty arrays, invalid boxed array values, read-only properties, and
attached properties. Scalar `Char` cases include NUL, Unicode, surrogate, and
maximum UTF-16 code units. Both testbed projects must be built for the test
assembly's flavor; they are included in `XamlCompilerTests.sln`.

The round trips use C++/WinRT's existing array boxing support. This projection
fix does not add generalized boxing for enum, custom-struct, or runtime-class
element types.
