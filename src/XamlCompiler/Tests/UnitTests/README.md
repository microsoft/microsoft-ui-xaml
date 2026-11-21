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

## Static bindings with null template items

`FunctionBinding_Static*` covers static bindings in a `DataTemplate` without a data
item, including omitted `x:DataType`, static properties, mixed instance bindings,
value-type items, and null-to-item-to-null transitions. The C# cases compile and
execute the emitted binding class and setters against a minimal framework surface,
asserting the assigned text and that item-dependent functions do not reuse stale
data or evaluate BindBack-only receivers for missing items. The C++/WinRT
cases check the corresponding generated activation, root-validity guards, and
restoration of independent named-element sources.
`FunctionBinding_StaticBindBackDoesNotInitializeTemplateWithoutItem` also verifies
that a static BindBack-only root cannot activate an instance-dependent forward
binding before any item is supplied, while preserving reverse updates once an
item initializes the binding.
`FunctionBinding_StaticInitialization*` covers the combination of an independent
static forward binding and an item-dependent TwoWay function with a static
BindBack receiver. Initial null-item edits and the first item-to-target update
must not call BindBack; later edits, replacement/recycle ordering, and
reinitialization retain their existing behavior. Independent static TwoWay
bindings remain active without an item.

`ListAndTemplateTests.xaml` in both compiled-binding regression applications also
includes null-content static templates, so their generated C# and C++/WinRT are
compiled and compared with the codegen masters.

`Runtime_StaticTemplateBindings_CSharp`, `Runtime_StaticTemplateBindings_CppWinRT`,
and `Runtime_StaticTemplateBindings_CppWinRTIncremental` launch their built WinUI
applications with `--test-static-template-bindings` and require successful results
from every runtime assertion group. Run them with
`runtests.cmd /TestCaseFilter:FullyQualifiedName~Runtime_StaticTemplateBindings`.
They require an interactive Windows desktop and the runnable application payloads
produced by `XamlCompilerTests.sln`; missing payloads or timeouts fail rather than skip.
The TwoWay initialization group uses real dependency-property callbacks and a
counting static BindBack receiver, with and without the independent forward
binding. It checks null edits, first item arrival, later edits, replacement and
recycling, plus item-independent static and named-element reverse writes.
The C# function cases use the object-valued `Tag` target; the native function
cases use string-valued `Text` targets with the same item/independent binding
split and callback timing.
The native applications host the managed model through app-local `WinRT.Host.dll`
and require an installed .NET 8 runtime compatible with its runtimeconfig. The
launcher removes build-SDK `DOTNET_ROOT` overrides from the child process so they
cannot hide the installed target runtime.

The applications activate a real window and assert that initially null-content
`ContentControl` templates display their static function result, both without
`x:DataType` (the original #7810 reproduction) and with a declared data type.
They also check item-to-null-to-item transitions with an observable object that
is also a static binding source.
Untyped, entirely item-independent static templates additionally cover non-null
`Content`, null `Content` with inherited non-null `DataContext`, and null/item/null
transitions through both paths. Context-transition checks overwrite the previous
text so a skipped update cannot pass using a stale result. Direct untyped component
calls cover null and unrelated non-null items, preserving phase-zero/phase-one
scheduling. `FunctionBinding_StaticUntyped*` executes the same generated-entry-point
contract; required-conversion cases retain typed roots, whole-item bindings,
item-dependent function arguments, and reverse-only receivers.
It verifies that static notifications and static TwoWay bindings remain active
without updating or writing to the removed item, and that an already-connected
deferred named source keeps updating its target after item replacement or removal.
The C++/WinRT mode also verifies compatibility for ordinary and named-only templates:
ordinary bindings retain their previous null-context notifications/reverse writes,
and entering a live tree with no item does not newly initialize a named-only template.
Listener cleanup, missing-item root validity and static-only initialization are
restricted to DataTemplates with forward static bindings; page and ControlTemplate
bindings keep the shared legacy lifecycle.
Direct `IDataTemplateComponent.ProcessBindings` calls cover phase-zero/phase-one
scheduling, independence from subsequent DataContext inheritance, listener release
on `Recycle`, and reuse with a replacement item while independent static notifications
continue. Null-item reverse-assignment safety is covered through DataContext changes;
the separate existing typed `ProcessBindings(null, ...)` cast failure is not changed here.

`CodeGenerator_*NamedTemplateSourceConnection*` verifies that ordinary and deferred
named DataTemplate sources receive rewritten connection IDs matching generated
Connect cases in C# and C++/WinRT. The same cases preserve page and ControlTemplate
named-source connections. Assertions are scoped to the source's Connect case and
require assignment from the connected target rather than matching an unrelated
disconnect or connector-factory switch.

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
