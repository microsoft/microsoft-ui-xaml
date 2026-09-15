# WinUI/XAML clang-tidy checks

This directory contains a custom **clang-tidy module** that enforces
**Pillar B — "storage correctness by construction"** from the WinUI
Peer-Lifetime Synchronization design: storing a peer in a GC-invisible way
should be *uncompilable*, not merely discouraged.

## What the check does

`xaml-peer-comptr-field` (`XamlChecks/PeerComPtrFieldCheck.{h,cpp}`) walks every
non-static data member and reports an error when:

* the field is a raw `ComPtr<T>` (i.e. **not** `TrackerPtr<T>` /
  `TrackerPtrVector<T>`), **and**
* the pointee `T` is a reference-tracker target, i.e. `is_tracker_target<T>` is
  `true`.

Sanctioned, explicit opt-outs are **never** flagged:

| Storage            | Meaning                                        |
| ------------------ | ---------------------------------------------- |
| `TrackerPtr<T>`    | tracked (correct) storage — visible to GC walk |
| `ctl::WeakRefPtr`  | weak, non-owning reference                     |
| `NonTracked<T>`    | annotated deliberate opt-out                   |

The set of tracker-target types is **codegen-owned**: XAML codegen emits
`is_tracker_target<T>` specializations into `TrackerTargetTraits.g.h`
(see `dxaml/xcp/components/metadata/inc/TrackerTargetTraits.h` for the primary
template and `XamlGen/Templates/Metadata/TrackerTargetTraits.tt` for the
generator). The check reads that trait, so the lint and the storage default in
`PropertyDefinition.FrameworkFieldTypeName` share a single source of truth and
cannot drift.

## Building the module

The module is **not** part of the WinUI product build. It builds against an
LLVM/Clang source + build tree, exactly like the in-tree clang-tidy checks:

1. Clone `llvm/llvm-project` at the toolchain version used by CI.
2. Symlink or copy `XamlChecks/` to
   `clang-tools-extra/clang-tidy/xaml/`.
3. Add `add_subdirectory(xaml)` and the module to `ClangTidyForceLinking.h` /
   the `clang-tidy` `LINK_LIBS`, mirroring the existing modules.
4. Configure + build the `clang-tidy` target:
   ```
   cmake -G Ninja -S llvm -B build \
     -DLLVM_ENABLE_PROJECTS="clang;clang-tools-extra"
   ninja -C build clang-tidy
   ```

Alternatively, build it as an out-of-tree module and load it with
`clang-tidy --load=clangTidyXamlModule.so`.

## Enabling the gate

`.clang-tidy.example` is the configuration that turns the check on as
**warning-as-error**, scoped to the peer directories. Roll it out in phases
(per the design doc):

* **Phase 0 (warn-only):** run the check without `WarningsAsErrors` and count
  existing violations.
* **Phase 3 (error):** copy `.clang-tidy.example` to `.clang-tidy` under the
  gated peer directory (e.g. `dxaml/xcp/dxaml/lib`).

> The `.clang-tidy` file is intentionally shipped as `.example` so that the
> gate is not active until the `clangTidyXamlModule` is present in the running
> `clang-tidy` binary; otherwise every clang-tidy invocation in the repo would
> fail on the unknown check name.
