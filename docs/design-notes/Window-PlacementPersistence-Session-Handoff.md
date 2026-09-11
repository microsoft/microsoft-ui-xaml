# Window placement session handoff

Updated on 2026-09-11 for transfer to another session.

This is an engineering handoff, not part of the API contract or content for learn.microsoft.com.
It supersedes the earlier handoff about the September 10 squash. The current API proposal is
only [window-placement-persistence-spec.md](../../specs/window-placement-persistence-spec.md).

## Resume here

Jesse requested a fresh API spec after reviewing the previous draft. On September 10, he
authorized using the agent's best judgment about the API shape and using PlacementEx as the
default placement engine. He specifically requested conceptual explanations for missing
monitors, changed DPI, off-screen windows, snapping, and similar scenarios.

The previous spec was preserved byte-for-byte as
[window-placement-persistence-spec-old.md](../../specs/window-placement-persistence-spec-old.md).
The replacement was written from scratch using the repository's API-spec template.

This remains API design and documentation work, not implementation delivery. The replacement
contains agent-selected design choices made under Jesse's authorization. Do not describe those
choices as independently ratified by an API review board or already implemented.

On September 11, Jesse authorized committing the work, putting important session notes into
the repository, and pushing for transfer, with **no PR creation or reopening**. After being told
that both configured GitHub repositories are public, he explicitly selected:

> Push to public GitHub origin; branch visibility is OK, but do not open a PR.

That clarification permits a public branch push. It is not permission to announce the proposal,
start the public review period, post comments, or create a PR.

## Which files are authoritative

| File | Role |
|---|---|
| `specs\window-placement-persistence-spec.md` | The only current API proposal |
| `specs\window-placement-persistence-spec-old.md` | Unmodified archive of the previous spec |
| `docs\design-notes\Window-PlacementPersistence.md` | Historical engineering design, not current policy |
| `docs\design-notes\Window-PlacementPersistence-PlacementEx-WinRT.md` | Historical advanced API design, not current policy |
| `dxaml\xcp\components\windowplacement\inc\PlacementEx\` | Native engine source used to establish default mechanics |
| This handoff | Current transfer context, rationale, and limitations |

Do not reconcile the new spec back to the historical documents. In particular, the old public
flags, copy constructor, hidden-placement wording, and broader unpackaged-storage plans are not
the current proposal. The native component README also predates build integration; its statement
that the component is not wired into a build target is stale.

The rewrite touched only the new spec and its archive. This transfer additionally updates this
handoff. The engine source, historical designs, prototype, generated API files, and tests were
not changed by the rewrite or transfer.

The archived spec's SHA-256 is:

```text
B4F9E92CA79E9AF1656646E0D8912505F8A7DF803C2E3AF80AE895F343E4E83A
```

Its Git blob id is `c785923f016a6752976b9d9c379f5f073f9d2614`. Preserve the archive rather than
editing it alongside the new proposal.

## Current proposed shape

- Keep the packaged-app easy path: set `Window.PersistPlacementId`, then retain the app's
  initial `Activate()` call.
- Keep `Window.InitialShowOptions`, `Window.Show()`, and `Window.Hide()`.
- Keep direct `Window.TryGetPlacement` and `Window.TrySetInitialPlacement` methods. Do not
  reintroduce a public manager or builder merely because archived discussions used one.
- Use mutable, detached `WindowPlacement` with `NormalRect`, `WorkArea`, `Dpi`, `State`,
  nullable `SnapRect`, `DisplayDeviceName`, and nullable `VirtualDesktopId`.
- Replace the old `WindowPlacementFlags` and `WindowPlacementShowState` combination with
  `WindowPlacementState`: `Normal`, `Maximized`, `Minimized`, `Snapped`,
  `MinimizedFromMaximized`, and `MinimizedFromSnapped`.
- Use `Clone()` for an independent copy. There is no same-type constructor.
- Do not expose `AllowPartiallyOffScreen` or saved source-window resizability. Use ordinary
  keep-on-screen behavior and current target capability and constraints.
- `TrySetInitialPlacement(null)` clears staged explicit placement while the opportunity is
  open. A non-null value is copied and validated; later mutation cannot alter staging.
- `true` means accepted, not applied or durably saved. Explicit selection prevents automatic
  loading even if applying that value later fails.
- Make the two data/options classes agile with thread-safe individual accesses and coherent
  snapshot operations. Multiple property assignments are not a transaction. Window operations
  retain their UI-thread affinity.
- Automatic persistence is packaged-only in this version. Unpackaged apps can capture, stage
  explicit placement, and use display options. No placeholder storage setup API is proposed.

The six-state enum, null-to-clear behavior, target-derived sizing, and agile mutable values are
deliberate new design choices. Review their usability, but do not silently revert them to the
older flags-based design.

## Lifecycle and persistence decisions

Read the conceptual lifecycle and API pages for the full contract. The important distinctions are:

- One opportunity belongs to the window, not one to each display method.
- `Show()` uses the new pipeline. Initial `Activate()` uses it when persistence, explicit
  placement, or non-null initial options configure the feature. An unconfigured initial
  `Activate()` preserves the legacy path and consumes the opportunity without restore.
- A non-null, default-valued options object therefore opts `Activate()` into the new pipeline.
  Null supplies default values when that pipeline is used for another reason.
- Both initial entry methods honor reason, activation behavior, and `KeepHidden`.
- Invalid initial option enums fail before consumption. Later invalid options are ignored
  behaviorally because the options are no longer consulted.
- Direct native display consumes the opportunity without this restore. Hiding does not reopen it.
- Initial reentrant `Show`, `Activate`, and `Hide` are ignored. Close is not ignored, and the
  outer operation must not continue using a closed window.
- Later `Show()` is a no-op for an already shown window, including minimized. It reveals a
  hidden window without unminimizing it and requests activation only if not minimized.
- Later `Activate()` reveals, restores minimization, and requests activation. No later call
  reapplies initial placement.
- Only `ApplicationRestart + DoNotActivate` preserves saved minimization and attempts desktop
  restoration. `KeepHidden` suppresses display without changing the selected policy row.
- `Launch` uses a valid shell monitor hint, including for fallback geometry without saved data.
  WinUI does not replay a native startup show command for each window.
- Hidden overlapped capture uses current geometry and the last meaningful state. Hidden app
  geometry adjustments are retained. This intentionally differs from saving a whole old
  non-hidden snapshot.
- Other presenters use the last complete valid overlapped placement. An unsupported target
  presenter is not changed by initial restore; the opportunity is consumed without applying.
- Saving eligibility requires actual display or successful application of explicit/loaded
  placement while hidden. Mere construction, capture, staging, failed hidden apply, or moving
  fallback geometry for a launch hint does not qualify.
- Ids are case-sensitive, ordinal, and not Unicode-normalized. Last successful save wins.
  Late changes affect saving, not restore. There is no topology history or duplicate cascade.
- Saving and restore failures are best effort. Migration examples do not claim that staging
  proves durable storage or that old data can safely be deleted.

## Engine findings that must not be lost

The replacement's appendix links the exact engine ranges. These conclusions were checked in
the actual source after an exploratory agent returned some inaccurate summaries:

| Finding | Source or consequence |
|---|---|
| Monitor fallback uses the saved normal rectangle, not the saved work area | `PlacementEx::FindClosestMonitor` matches device name, then calls `MonitorData::FromRect(normalRect)` |
| The native action API's own fallback is not the helper's policy | PlacementEx explicitly supplies its selected target monitor |
| Position and size migrate differently | `AdjustNormalRect` scales work-area-relative offsets and DPI-relative dimensions |
| Validity requires overlap with the saved work area | `PlacementEx::IsValid` requires positive rectangles, DPI at least 96, and intersection |
| Old-screen validity is not current-screen overlap | A valid saved rectangle can be wholly outside today's screens after monitor removal |
| A live wholly off-work-area capture may be invalid | Saving then skips rather than overwriting the previous stored value |
| Snap bounds are visible frame bounds | They exclude invisible resize borders and migrate relative to work-area edges |
| Snap groups are not represented | Do not claim Snap Layout or snap-group reconstruction |
| Minimized-from-snapped history is not recovered by a single capture | `GetPlacement` captures current arrangement and restore-to-maximized; WinUI must track restore-to-arranged history |
| Ordinary normalization needs a snap-specific adapter step | Convert minimized-from-snapped to arranged before ordinary adjustment |
| Native and downlevel fitting differ | Native uses fit-to-monitor; downlevel uses geometry helpers and `AllowSizing` |
| Downlevel no-activate is not implemented by simply setting the native flag | Cloaking does not prove that focus and activation were preserved |
| Native capture can be DPI-virtualized | The public physical-pixel contract requires a controlled coordinate boundary |
| The native in/out value is not a resulting-window snapshot | The native path can leave old metadata; downlevel mutates normal bounds to workspace coordinates |
| Failure can occur after partial native changes | No transactional rollback guarantee; fallback uses valid current bounds |
| Public input needs more than native `IsValid` | Use checked edge conversion, scaling, and adjustments before invoking native arithmetic |

Do not repeat the exploratory claims that `GetPlacement` independently reconstructs
minimized-from-snapped history, that downlevel `NoActivate` is sufficient, or that monitor
fallback uses the saved work area. The corrected spec and direct source reads supersede them.

## What remains unproven

The spec's acceptance criteria describe work an implementation must still demonstrate:

- Visibility, focus, event ordering, and native launcher commands on both backends, especially
  non-activating maximized/snapped restore and hidden restart on another virtual desktop.
- Correct lifecycle eligibility and compatibility for unconfigured `Activate()`, direct
  `AppWindow` display, reentrancy, close, and presenter changes.
- Coherent hidden and pre-presenter placement tracking, including when native geometry/state
  is unavailable or only partly observed.
- Translation of the six states and loss of unavailable snap state without inventing history.
- Checked handling of the full public numeric domain. The archived serializer's coordinate
  and DPI caps are not automatically the new public contract.
- Exact MIDL/codegen integration, C#/C++/WinRT projection, null contracts, and atomic snapshots
  for agile mutable runtime classes.
- Reliable no-store and write-failure behavior without treating `TrySetInitialPlacement`
  acceptance as a save acknowledgment.

These are implementation and focused review risks, not evidence that runtime behavior already
matches the proposal. Do not expand this transfer into fixing the prototype without a new task.

The earlier copy-constructor concern was independently supported by Microsoft's
[C++/WinRT guidance](https://learn.microsoft.com/windows/apps/develop/cpp-winrt/consume-apis#dont-copy-construct-by-mistake).
The new `Clone` avoids it. The exact proposed IDL was not compiled.

The ABI-name distinction was checked against
[MIDL documentation](https://learn.microsoft.com/uwp/midl-3/advanced#overloads):
`method_name` changes the ABI name, not the projected `Show()` or `Hide()` name.

Published `windows-app-sdk-2.0-experimental` AppWindow documentation did not substantiate the
old complete-field-equivalence claim. The latest 2.4.1 experimental WinMD was not inspected.
Do not turn that documentation limitation into a claim about the latest package. The new spec
does not depend on those experimental APIs or claim parity.

## Validation and prototype status

The rewrite's document checks covered template sections, internal and relative links, code-fence
balance, enum/table agreement, removal of obsolete API names, ASCII, line width, and diff
whitespace. The old spec's exact hash was retained.

No app/product build, runtime test, or end-to-end MIDL generation was performed for the rewrite.
The prior commit preserves a much older implementation; documentation-only checks do not
validate it. The latest transfer commit is documentation-only, but the complete branch is not.

Known historical prototype gaps include initial Activate forcing activation and clearing
KeepHidden, different later Show behavior, absent public placement value/methods and Hide,
and incomplete direct-display, reentrancy, visibility, and lifetime handling.

Historical results before the September rebases were a successful DLL/prodtest build,
35 passing serialization tests, and a VM run with 35 passed and 2 blocked on ApplicationData
activation. None validates the new spec. If implementation resumes, use the repository's
current build/test workflow; do not assume the old VM environment or credentials exist.

## Git state and transfer destination

- Repository: `microsoft/microsoft-ui-xaml`.
- Local worktree branch: `jecollin-microsoft-window-placement`; it was not renamed.
- Approved remote: `origin`, the public `microsoft/microsoft-ui-xaml` GitHub repository.
- Transfer branch: `jessecol-window-placement-spec-rewrite`. This new remote name follows
  Jesse's GitHub naming preference and avoids reusing a historical PR head.
- The transfer commit follows `4a46c37f60` ("Add window placement design and prototype").
  That earlier squashed commit has parent `914147a04d5439e88d551f23869cfaeae6eb474b`.
- Use Git to obtain the exact current SHA and confirm remote state. This file records the
  chosen destination, not an assertion that an in-progress push has already succeeded.
- Only the transfer branch is to be pushed. Do not push backup refs, other branches, or tags.
- Local recovery ref `refs/backup/jessecol-window-placement-pre-squash-20260910-1539` points
  to `b4688991cc380aef14811605d65ee5494fae5a18`, the older pre-squash history.

Closed historical PRs #11708 and #11699 are not transfer targets. Do not restore their heads,
reopen them, or create replacement PRs. A public branch remains visible without a PR.

## Continuing in another session

Work in the new session's assigned worktree, not the main checkout. Start with the current spec
and its acceptance criteria. The historical designs are background only; this handoff does not
make them current again.

The authoring guide is `specs\api_spec_template.md`, which points to the Windows App SDK
spec template. Use its six major sections. Keep observable behavior in the publishable
conceptual/API sections and engine details in the appendix. Keep prose declarative, ASCII,
and approximately 100 columns. Contract version 12 is illustrative, not a shipping commitment.

Do not automatically announce or publish for review because the branch is now pushed.
Any future PR description or comment must start with:

```text
*This content was largely generated by AI.  AI makes mistakes.*
```

The current session's scratch-files folder was empty at transfer. Important decisions and
review caveats previously held in the session database are summarized here and in the spec;
the next session does not need access to that private session state.
