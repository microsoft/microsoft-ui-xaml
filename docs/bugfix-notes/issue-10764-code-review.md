# Code review: #10764 (caret browsing Hyperlink crash)

Branch: `user/copilot/issue-10764`

## Round 1 (reviewing cc063fdfe)

| Reviewer | Model | Kind | Result |
|---|---|---|---|
| r1-normal-a | gpt-6.1-terra | Normal | No significant issues |
| r1-normal-b | claude-opus-5.5 | Normal | 1 finding (medium) |
| r1-adv-a | gemini-4-argon | Adversarial | 6 findings |
| r1-adv-b | gpt-6-astra | Adversarial | No significant issues |

### Findings

| # | From | Severity | Finding | Disposition |
|---|---|---|---|---|
| 1 | r1-normal-b, r1-adv-a | Medium/High | TextBlock creates its selection manager lazily, and `GetSelectionStart/End` don't create it. A selectable TextBlock without a manager would hit the new early return and skip moving the caret. | **Fixed.** TextBlock branch now calls `GetSelectionManager()` first. Tried to prove the gap on the VM with two tests: Shift+Tab into the Hyperlink, and enabling selection then calling `Focus()` in one UI-thread call. Both passed without the change. A render (`RenderBackplateIfRequired`) creates the manager before the async GotFocus runs. Kept the one-line call to remove the ordering dependency. Dropped the tests because they don't fail without the change. |
| 2 | r1-adv-a | Medium | RTBO branch dereferences `m_pMaster` without a null check. | **Rejected.** Pre-existing. The Hyperlink's visual parent is only an RTBO when that overflow is linked to a master, so `m_pMaster` is non-null here. |
| 3 | r1-adv-a | Medium | RTBO focus/key/pointer handlers check only that the master's manager exists, not `IsSelectionEnabled()`. With high contrast active, they can run on non-selectable text. | **Out of scope.** Separate pre-existing issue, not reached by this bug. Noted as a possible follow-up in description.md. |
| 4 | r1-adv-a | Low | Turning off `IsTextSelectionEnabled` at runtime under high-contrast adjustment keeps the manager and may leave the caret visual. | **Out of scope.** Pre-existing, unrelated to this crash. |
| 5 | r1-adv-a | Low | Selectable test only covers the case where the TextBlock got focus first. Non-selectable tests could also check the caret visual count. | **Partly addressed.** Investigated the no-TextBlock-focus case (see #1). The null `SelectionStart` check plus "no crash" is the behavior under test, and the before-run proved the tests hit the crash path, so no visual-count checks. |
| 6 | r1-adv-a | Nit | description.md rationale about `CTextCore::IsTextSelectionEnabled` is inaccurate. | **Updated.** description.md has no such paragraph, but its "Selectable text behaves the same" line was inaccurate for the lazy-manager case. Replaced it with a section on the `GetSelectionManager()` call. |

### Validation after round 1

- Rebuilt MUX and Foundation tests.
- VM: `*HyperlinkInTextTests*` 29/29 passed.

## Round 2 (reviewing e57e98dd0)

| Reviewer | Model | Kind | Result |
|---|---|---|---|
| r2-normal-a | gpt-5.6-sol | Normal | No significant issues |
| r2-normal-b | claude-sonnet-5 | Normal | 1 finding (medium) |
| r2-adv-a | grok-4.7 | Adversarial | No defects; 1 description nit |
| r2-adv-b | gpt-6-sol | Adversarial | No significant issues |

### Findings

| # | From | Severity | Finding | Disposition |
|---|---|---|---|---|
| 1 | r2-normal-b | Medium | No test covers the `GetSelectionManager()` line. Suggests calling `hyperlink->Focus()` directly so the host never gets focus first. | **No change.** Already tried in round 1 (enable selection + `Focus(Keyboard)` in one UI-thread call). It passed without the line, because a render creates the manager before async GotFocus. description.md (the PR description) already says this line has no failing test. |
| 2 | r2-adv-a | Nit | description.md says RichTextBlock creates its manager "eagerly". It actually creates it in `EnsureBlockLayout` and `OnSelectionEnabledChanged`. | **Fixed** wording in description.md. |

Adversarial angles tried with no defect: null manager deref, one-sided null, early return skipping real work, selectable RTB/RTBO regression, RTBO `m_pMaster`, high-contrast paths, and test false passes.

No code changes in round 2, so no rebuild or VM rerun.

## Round 3 (reviewing e57e98dd0)

| Reviewer | Model | Kind | Result |
|---|---|---|---|
| r3-normal-a | gpt-5.6-terra | Normal | No significant issues |
| r3-normal-b | claude-haiku-5.5 | Normal | No significant issues |
| r3-adv-a | gemini-3.8-flash | Adversarial | No significant issues |
| r3-adv-b | gpt-6-luna | Adversarial | No significant issues |

Adversarial angles tried with no defect:
- `EnsureTextSelectionManager` assert on a null text view. Not reachable: `GetVisualParent` needs the text view, so the block is skipped without it.
- One-sided null leaking a reference. `xref_ptr` releases it.
- Early return skipping cleanup.
- Runtime toggling of `IsTextSelectionEnabled`.
- High contrast with selection off.
- Empty Hyperlink.
- Focus loss.
- Test isolation (`scope_exit` resets caret browsing) and false passes.

All description.md claims were checked and found accurate.

## Result

No sensible changes left after 3 rounds. Done.
