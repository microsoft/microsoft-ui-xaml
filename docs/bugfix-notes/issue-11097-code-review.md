# Code review log: issue #11097

Commit under review: d26ac30a8 (round 1).

## Round 1

| Reviewer | Model | Findings | Disposition |
|---|---|---|---|
| r1-opus | claude-opus-5.5 | No must-fix or should-fix. Nit: compat bullet says "built against the old runtime"; it's really "built with the old Xaml compiler". Nit: C++/CX `XamlMember` citation should be `:701-706`, not `:704-706`. Info: open PR #11823 touches the same files. | Fixed both nits. Added an "Overlapping PRs" section. |
| r1-sol | gpt-6.1-sol | No issues. Info: open PR #11846 re-splits the same `XamlMember` string literal in `CppWinRT_TypeInfoPass1Impl.cs`. | Added to "Overlapping PRs". Checked #11846's diff: it keeps the old order, so whichever PR lands second must keep the new order. |
| r1-argon | gemini-4-argon | Checked every ctor list, `.tt`/`.cs` parity, and the 23 masters. Audited all other C++ templates and found no other out-of-order lists. Claimed (should-fix) that the C++/CX bools have no `= false` default. Claimed the citations for `_fullName` and the preamble lines were wrong. | Rejected the bool claim: `MoComCppTypeInfoPass1.tt:372-377` has `bool _isArray = false;` etc. Rejected the preamble line claims: `/we5038` is at `AppPreamble.cpp.props:148` and `RuntimeComponentPreamble.cpp.props:92`. Rejected the `_fullName` claim: it's at `CppWinRT_TypeInfoPass1.tt:175`. Argon's line numbers were off by one. Added its template audit to "Not covered", with `XamlConsolidatedType` as an example. |
| r1-skeptic-a | gpt-5.6-terra | No issues. | None. |
| r1-skeptic-b | claude-sonnet-5.5 | No must-fix or should-fix. Nit: `dxaml\test\native\external\Automation\xamluiabridge\XamlTypeInfo.Impl.g.cpp` is a stale checked-in C++/CX copy with the old order. Nit: `docs\bugfix-notes` is a new directory. | Mentioned the xamluiabridge copy in "Not covered" and left it unchanged, since dxaml turns C5038 off (`dxaml\common.props:84`). Kept `docs\bugfix-notes`, since it's the bug-fixer flow's convention. |

Result: notes-only changes. No code changes.
