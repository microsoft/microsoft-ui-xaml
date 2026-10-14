# Complete XamlC module submission

This branch consolidates the compiler, common incremental fix, regression coverage,
generated-code expectations, documentation and developer sample formerly spread
across upstream PRs #12132, #12133 and #12134. Keep #12131's initialization changes
independent. Fork PR #40 remains the integrated development/evidence checkpoint.

## Submission scope

- Independent class declaration interfaces, pure Application_Xaml re-exports and
  separate BindingInfo/TypeInfo interfaces, with ordinary implementation .cpp files.
- Compiler mode/state propagation, semantic projection dependency closure, native
  registration, canonical IFC references and stale output/scanner/object cleanup.
- The common generation-flag/removal invalidation fix and the succinct-collection
  parser/diagnostic correction, including the three direct syntax regressions.
- The complete 23-method module unit suite, existing schema/function-binding tests,
  proxy support and the application layout fixture correction.
- SimpleModules multi-page fixtures, same-leaf namespace pages, handwritten/module
  consumers, the static library provider and consumer, and the NuGet package consumer.
- Manual native/unit/package/sample validation scripts and their metadata/payload
  support. These scripts are executable tests, rather than Actions orchestration.
- Reviewed generated-code expectations, FixMasters cleanup correction and its test,
  the minimal repository sample, and developer/architecture documentation.

No fork Actions workflow, fork build-pr orchestration delta or independent bootstrap
change is added by this branch. The existing upstream workflows remain unchanged.

## Regression entry points

The ordinary test solution retains its supported VS2022 baseline. The VS2026-only
module projects are driven explicitly by .github/scripts/validate-xamlc-cppwinrt-modules.ps1;
placing them into the ordinary solution's default build would force its toolchain
upgrade. The package consumer is driven by validate-xamlc-package-consumer.ps1.
The external sample script requires a freshly built WinUI package in PackageStore.

The unit runner now accepts -Configuration Debug or Release and requires a matching
initialized chk or fre environment. Release validation must use its own compiler,
projection and regression outputs. The existing native Release consumer checks that
use a Check compiler artifact do not establish Release compiler code-generation results.

## Generated expectations and pending checks

The development branch retained 232 updated Check expected outputs and 232 older Free
outputs, replacing their former common entries. The Free copies still used the legacy
generator form. This candidate restores the common layout with the observed Check
outputs as the expected new contract, while reconciling current upstream template-input
changes. In two Templates.xaml.g.hpp conflicts, upstream IsEnabled listener state is
retained and token declarations use the generator's std::int64_t mapping.

This is a reviewed expectation update, not a claim that the candidate generator has
already produced these bytes in both flavors. Exact Debug and Release generated-code
comparison must run. Any mismatch must be inspected against the current templates and
actual output; no test is ignored, filtered out of the full gate or automatically
accepted merely to make the candidate pass. Formal T4 regeneration is also required.

## Evidence boundary

The source was consolidated from #40 at a4d9a8b494f5dc41b1d4de44f5197c6fa58d9f27 onto
upstream f5f1a28357922127317727c2e01d46ebfedefc15 through three-way patch application.
This preserves later upstream compiler/test changes rather than overwriting them with
older fork files. Source auditing and project XML checks do not replace Windows builds.
Earlier integrated green runs are historical evidence only; the complete candidate
needs its own current-head tests, consumer builds and generated-output verification.
