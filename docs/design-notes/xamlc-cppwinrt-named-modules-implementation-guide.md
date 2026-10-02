# XamlC named-module implementation guide

The public contract and generated-file examples are in
[xamlc-cppwinrt-named-modules.md](xamlc-cppwinrt-named-modules.md).
This guide identifies the owning code and the validation required when changing it.

## Source ownership

| Concern | Owning source |
| --- | --- |
| Mode, current project inputs, incremental invalidation and generated output reporting | `BuildTasks/CompileXamlInternal.cs` |
| Persisted code-generation flags and per-file output ownership | `BuildTasks/SaveState.cs` |
| Project module identity and partition list | `BuildTasks/Microsoft/Xaml/XamlCompiler/XamlProjectInfo.cs` |
| Projection namespace discovery and header/import lowering | `CodeGenerators/CppWinRT_CodeGenerator.cs` |
| Page fields, base classes and x:Bind source/target closure | `CodeGenerators/PageDefinition.cs` |
| Metadata, generic argument and provider closure | `CodeGenerators/TypeInfoDefinition.cs` |
| App/Page/BindingInfo/TypeInfo declarations and ordinary Pass2 implementation | `CodeGenerators/CppWinRT/*.tt` |
| Native module registration, combined-item normalization, late compile references and IFC cleanup | `Targets/Microsoft.UI.Xaml.Markup.Compiler.interop.targets` |
| Focused unit methods | `Tests/UnitTests/CppWinRTModuleTests.cs` |
| Reflection proxies for internal compiler types | `Tests/UnitTests/XamlCompilerProxies/` |

Paths above are relative to `src/XamlCompiler`; `CodeGenerators` is under
`BuildTasks/Microsoft/Xaml/XamlCompiler`.

The `.tt` templates are authoritative. Regenerate their checked-in `.cs` outputs with
the repository T4 tooling. Do not hand-edit generated C# or introduce unrelated generator drift.

## Generation order and ownership

1. C++/WinRT resolves projection-module references before XAML Pass1.
2. XamlC Pass1 declares one `<RootNamespace>.Application_Xaml` primary interface
   in `XamlBindingInfo.xaml.g.h` and exports App/Page/optional TypeInfo partitions.
3. MSBuild registers generated `.xaml.g.h` files with `CompileAsCppModule`,
   `PrecompiledHeader=NotUsing`, and `WINRT_XAML_MODULE_INTERFACE`.
4. C++/WinRT adds its projection module interfaces. XamlC then normalizes the
   combined module input set by physical FullPath before VC dependency scanning.
5. Pass2 remains an ordinary translation unit. Late generated compile items receive
   the projection and XAML BMI search paths, without inheriting producer definitions.

BMI means binary module interface; MSVC stores it in an IFC file. XAML IFCs live in
`$(IntDir)XamlModules\`. Static-library references propagate them through VC/MSBuild's
native `ReferencedModuleBMIs` graph. `CppWinRTConsumeModule` has a separate meaning:
it selects a C++/WinRT projection producer and is not a XAML reference protocol.

Existing component `.g.h` files keep their `__has_include` XAML bridge. Ordinary
inclusion imports the umbrella; the producer branch emits module declarations.
`module;` must precede every preprocessed declaration, including `#pragma once`.
Exported XAML declarations use detached `extern "C++"` ownership so ordinary Pass2
translation units can supply their definitions.

App's concrete metadata-provider requirements move to Pass2, including construction
and destruction. Page partitions forward-declare local C++/WinRT bases rather than
pulling implementation `.g.h` files into named-module ownership.

## Dependency collection

Collect the semantic WinRT namespace required by each generated expression, then
lower it to an include or import at the C++/WinRT backend. Include nested generic
argument types. For x:Bind, trace both the source path and the concrete target:
intermediate/item types, function owners and parameters, connection IDs, target member
types, and dependency-property owners can each introduce another projection namespace.

Pass1 can lack local component metadata. Query observable-vector/map properties only
when the bind step's ValueType is available; recover an unresolved local field namespace
from its declared type path. Derive local composable-base names from BaseTypeName.
Dependencies needed solely by authored sources belong in those sources, not in the
XamlC partition. An import-everything preamble would hide missing generated dependencies.

## Incremental contract

| Trigger | Required behavior |
| --- | --- |
| No XAML changes | Re-report shared module interfaces; keep every current partition |
| One Page changes | Preserve unchanged project x:Class entries from saved state |
| Page removed | Delete orphaned Pass1/Pass2 output and backups; rebuild shared output |
| Stale ClInclude DependentUpon | Map only XAML files in the current project input set |
| Code-generation flags change | Persist the new flags and invalidate existing output |
| NoPageCodeGen | Keep App output and its partition; remove suppressed Page output |
| NoTypeInfoCodeGen | Omit the TypeInfo partition and its primary-interface export |
| Module to header mode, or final XAML removed | Remove the dedicated XamlModules directory |
| Header to module mode | Regenerate module interfaces and IFCs |

Validate task arguments before destructive cleanup. Multiple XAML inputs can share one
generated class prefix; retain output if a current input still owns that prefix.
Design-time builds preserve the established protection against deleting live Pass2 output.

## Failure classification

| First diagnostic | First owning layer to inspect |
| --- | --- |
| Missing projection import in generated code | Page/TypeInfo semantic closure and backend lowering |
| WMC9999 before local WinMD exists | Pass1 unresolved type handling; obtain the inner exception |
| Duplicate physical module item | Combined ClCompile set before VC SetModuleDependencies |
| Invalid global module fragment | Producer branch before module declaration |
| Pass2 looks like an interface producer | Generated item preprocessor metadata inheritance |
| Missing pch.h in XamlMetaDataProvider.cpp | C++/WinRT package wrapper target, not a XamlC T4 emitter |
| Old imports after module/header switch | C++/WinRT projection incremental inputs |
| Ambiguous winrt_numerics IFC | Consumer duplicated the provider's projection modules |
| APPX0703 after linking | Runtime payload/manifest agreement and IncludeXamlDlls |

A pure static-library consumer without its own IDL/XAML sets CppWinRTBuildModule=false;
it still imports provider BMIs through native ProjectReference propagation.
Namespace exclusions alone do not suppress the package's base/numerics modules.

## Validation evidence and limits

The pre-rebase [run 36811821254](https://github.com/hoshiizumiya/microsoft-ui-xaml/actions/runs/36811821254)
passed all six product configurations and the focused VS2026/v145/SDK26100 job.
Native compilation, linking, packaging, incremental transitions, static provider/consumer,
and all 10 module unit methods completed. This does not validate the subsequent rebase,
interactive sample execution, other architectures' modules, or the full unit suite.

The rebase includes upstream #11837 and its relocated proxy projects. Use upstream
`XamlCompilerUnitTests.csproj` payload staging and `runtests.cmd`; the latter only accepts
VSTest arguments and runs from the initialized flavor's output. It no longer accepts
legacy /config, /platform or /flavor switches or builds/copies projects itself.
KnownVersions is generated from the build's SDK property. Keep all SDK overrides coherent
on a hosted image with only SDK26100; do not fall back silently to unrelated metadata.

Fork-native fixtures and CI scripts are separate from product code. Their third-party
CppWinRTPlus pin and projection-mode cache are validation dependencies. Product package
policy remains upstream's. The broader test PR stays draft until its Windows results,
including all enabled generated-code comparisons, are understood.

For any new run, distinguish product builds, focused module results, and unfiltered suite
results. Record the exact head and job, and inspect the TRX (VSTest's XML results), rather
than treating source test counts as executed tests.
