# C++/WinRT XAML module regression projects

These projects test the generated-module contract and must accompany the compiler
feature. They are not development-only sample code.

| Project | Coverage |
| --- | --- |
| Basic/CppWinRT/SimpleModules | App/Page, compiled binding, root/direct imports, same-leaf pages in different namespaces, handwritten class/interface consumers |
| Features/StaticLibs/StaticControlsModuleLib | XAML control and binding interface production in a static library |
| Features/StaticLibs/StaticControlsModuleConsumer | Native ProjectReference propagation and consumption of the library's XAML module |
| ../PackageConsumers/NuGetModules | Real packaged WinUI/XamlC consumer, with no inherited repository project settings |

The 23 CppWinRTModuleTests unit methods exercise the compiler/generator contracts.
ValidatorTests contains the direct succinct-collection parser/diagnostic regressions.
The source counts above are inventories, not execution results.

## Run the tests

Use the initialized Windows build environment and build/stage the compiler and WinUI
payload first. The PowerShell runners in .github/scripts are retained without adding
fork Actions workflows:

- validate-xamlc-unit-tests.ps1 runs the full unfiltered suite and generated-code comparisons.
  Select -Configuration Debug after init.cmd x64chk, or -Configuration Release after
  init.cmd x64fre. It rejects a mismatched initialized compiler flavor.
- validate-xamlc-cppwinrt-modules.ps1 requires VS2026/v145 and SDK 26100. It builds module
  and header consumers, Debug/Release native code, libraries and no-change/change/remove/
  restore/rename/no-pages/code-generation-flag/mode transitions. It also rejects C7684 and
  duplicate native link inputs. Its documented Check artifact prerequisite remains explicit.
- validate-xamlc-package-consumer.ps1 requires the freshly produced package in PackageStore.
- validate-xamlc-module-sample.ps1 tests the pinned external sample against that package,
  including absolute/default-relative IntDir and relative no-change builds.
- export-xamlc-test-failures.ps1 retains failed test messages and generated comparison evidence.
- materialize-winui-validation-package.ps1 stages the native metadata from a supplied product package.

The ordinary XamlCompilerTests.sln remains on the product VS2022 baseline. Do not add
VS2026-only module projects to its default VS2022 build. Use the native runner explicitly.

## Scoped dependencies

Only these module/sample projects pin YexuanXiao.CppWinRTPlus. Product dependencies
are unchanged. CppWinRTProjectionValidation.targets restores metadata references and
uses the provider's existing incremental-input extension points for command-line mode
transitions. This is a test-scoped workaround, not a fix to the external provider cache.
Tests/UnitTests/Directory.Build.targets replaces a projection project reference only
when UseXamlCompilerValidationProjection=true for artifact-based validation; ordinary
source builds retain their original project references.

Keep these files in the submission so maintainers can reproduce the native coverage.
Exact candidate Debug/Release compiler output and T4 verification remain required.
