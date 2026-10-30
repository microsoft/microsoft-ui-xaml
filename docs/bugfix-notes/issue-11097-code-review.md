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

## Round 2

Commit under review: 73948de78.

| Reviewer | Model | Findings | Disposition |
|---|---|---|---|
| r2-opus | claude-opus-5.5 | No must-fix or should-fix. Verified all citations. Nit: the ".cs is one long string literal, re-split" wording only fits the `XamlMember` hunk. The `XamlUserType` hunk is a plain line swap in a verbatim string. | Fixed the wording. |
| r2-sol | gpt-6.1-sol | No issues. | None. |
| r2-argon | gemini-4-argon | No issues. Verified every citation in the notes. | None. |
| r2-skeptic-a | gpt-5.6-terra | No issues. | None. |
| r2-skeptic-b | claude-sonnet-5.5 | Checked that the old `Simple` master lines 308 and 575 match the error lines in the issue, which backs up the red result. Should-fix: `docs\bugfix-notes` is a new directory. Nit: the red/green results are first-hand claims. Nit: the Tests section says the regression projects guard every template, but they only guard C++/WinRT. | Kept `docs\bugfix-notes`, since it's the flow's convention and is easy to drop before a PR. Kept the red/green claims. They come from my local runs; no log is attached. The old master lines back up the red side independently. Fixed the Tests wording. While fixing it, I found there's no C++/CX master either, so I said that in Tests and "Not covered". |

Result: notes-only changes. No code changes.

## Round 3

Commit under review: 08c89e996.

| Reviewer | Model | Findings | Disposition |
|---|---|---|---|
| r3-opus | claude-opus-5.5 | Should-fix: the Bug section said `/w15038` makes the build fail. That's only true with `/WX`, and it clashed with "from a warning or error". Nit: the "4 vs 5" skip comparison cited a run that isn't in the table. Nit: the "No runtime change" label repeated "Runtime behavior is identical". | Fixed all three. The Bug section now says warning, or failure under `/we5038` or `/WX`. The skip paragraph now explains the earlier 4-skip run. The label is now "No WinUI binary change". |
| r3-sol | gpt-6.1-sol | No issues. | None. |
| r3-argon | gemini-4-argon | No issues. | None. |
| r3-skeptic-a | gpt-5.6-terra | No issues. | None. |
| r3-skeptic-b | claude-sonnet-5.5 | Confirmed that no C++/CX master or project exists, and that the round-2 dispositions hold. Nits: "Nothing else changes" contradicts the preamble rows in the table. The skip cause is inferred, not observed. The round-2 disposition about red/green was circular. The template-audit credit was loose. The `docs\bugfix-notes` dir and model IDs shouldn't go into a public PR. | Fixed the first four. Kept `docs\bugfix-notes`, since the bug-fixer flow requires it. Drop the dir before any PR. |

Result: notes-only changes. Over three rounds, no code change was needed. All 5 reviewers in round 3 found the code correct, and none had a must-fix. Stopping here.
