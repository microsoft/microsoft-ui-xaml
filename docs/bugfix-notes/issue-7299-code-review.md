# Code review log for #7299

Branch: `user/copilot/issue-7299`

## Round 1 (reviewing commit 60445c9d6)

| Reviewer | Model | Type |
|---|---|---|
| r1-review-a | gpt-6.1-terra | code-review |
| r1-review-b | gemini-4-argon | code-review |
| r1-adversary-a | gpt-6.1-sol | rubber-duck (adversarial) |
| r1-adversary-b | claude-sonnet-5.5 | rubber-duck (adversarial) |

No reviewer found a blocking defect in the product change.

| # | Finding | From | Severity | Disposition |
|---|---|---|---|---|
| 1 | Mouse double-click uses two `LeftMouseClick(element)` calls. Each one does a UI-thread round trip, so the two clicks could fall outside the double-click time. | review-a, adversary-a | Medium | **Fixed.** Compute the center point once, then click that point twice. Same pattern as CommandingIntegrationTests.cpp. (Round 3: each click still does a short UI-thread call; see round 3 item 2.) |
| 2 | The `AdjustPositionByCharacterCount` guard (TextHighlighter path) has no test. | adversary-a | Medium | **Fixed.** Added `RichTextBlockTests::HighlightWordAfterEmptyParagraph`. Fails before the fix (host dies on render), passes after. |
| 3 | The `FindIUCPositionInInlines` guard (UIA InlineUIContainer path) has no test. | adversary-a | Medium | Not taken. Reaching it needs a UIA text-range query on an RTB with an InlineUIContainer after an empty paragraph. The guard is the same one-line null check as the two tested helpers. |
| 4 | The `GetValue` change has no direct test. | adversary-a | Low | Not taken. `Text` read-back still returns `""` because of `DetachHSTRING` (listed follow-up), so a test can't see the difference. |
| 5 | Copy (`TextSelectionManager.cpp:2037`) skips text that starts with NUL. | adversary-b, review-b | Low | Not taken. Not a crash, and `CF_UNICODETEXT` is NUL-terminated anyway. Listed as a follow-up. |
| 6 | `CHyperlink::GetContentText` may dereference a null `m_pInlines` for an empty Hyperlink via UIA. | adversary-b | Speculative | Not taken. Pre-existing and out of scope. Added to follow-ups. |
| 7 | `AdjustPositionByCharacterCount` end-of-collection check compares the running position to the local collection length, so a highlight ending at the end of a non-first paragraph runs on. | review-b | Medium | Not taken. Pre-existing for every multi-paragraph RichTextBlock, not caused by this change. Changing highlight geometry is a behavior change. Added to follow-ups. |
| 8 | `TextBlockView` wraps the helpers in `VERIFY`. A position inside a trailing empty Span now returns `false`, which asserts in chk. | review-b | Low | Not taken. Tried a `Hello<Span/>` double-tap test on the VM: it passed with the `VERIFY` in place (and also passed before the fix, so it was dropped). Release builds evaluate and ignore the result. Added to follow-ups. |
| 9 | Speculative edge in `GetCharacterIndex` for positions inside an empty paragraph's placeholders. | review-b | Note | No change. Same behavior as an empty, non-null collection. |

Verified by reviewers (no change needed):

- `return false` without touching the in/out counters is the right contract for all callers. Callers add or subtract the 2 placeholder positions for the empty Paragraph or Span.
- `ShouldRemoveEmptyRun` only removes runs that aren't explicit Run elements. Its only caller is `CompressInlinesWhitespace`.
- Every other `m_strText` emptiness check on the TextBlock path already uses `GetCount()`.

Validation after round 1 (VM, amd64chk):

- `TextBlockTests::*LeadingNull*`: 3/3 passed.
- `RichTextBlockTests::*EmptyParagraph`: 2/2 passed.
- `HighlightWordAfterEmptyParagraph` with the helper guards reverted: failed (host died).

Round 1 changes committed as e800af8ee.

## Round 2 (reviewing 60445c9d6 + e800af8ee)

| Reviewer | Model | Type | Result |
|---|---|---|---|
| r2-review-a | claude-opus-5.5 | code-review | No issues |
| r2-review-b | gpt-5.6-sol | code-review | No issues |
| r2-adversary-a | grok-4.7 | rubber-duck (adversarial) | No proven issues |
| r2-adversary-b | gpt-6-sol | rubber-duck (adversarial) | No proven issues |

What they checked:

- No other caller on the TextBlock/Run text path relies on the old `IsNullOrEmpty` result. The remaining hits are language/font names, other controls, and the known Copy follow-up.
- For text without a leading NUL, `GetCount() == 0` gives the same answer as `IsNullOrEmpty`. XAML markup can't contain NUL, so only code-set text changes.
- A null and an empty `InlineCollection` now behave the same. `CSpan`/`CParagraph::GetPositionCount` return 2 in both cases, and the helpers leave the counters alone in both cases.
- Attack inputs traced with no new crash or regression: `"\0"`, `"\0\0"`, NUL in the middle, empty explicit Run, nested empty Spans, empty InlineUIContainer, empty last Paragraph, RichTextBlockOverflow, keyboard selection, SelectAll, TextTrimming/MaxLines, binding `"\0x"` then `""`.
- Tests: the line-services variant really leaves the DWrite fast path (`CharacterSpacing` != 0). The RTB tap lands on `Hello`. The tests fail for the right reason with the guards reverted.

Speculative notes (no action):

- A string that is only `"\0"` may have zero advance width, so a tap could miss it. That's glyph width, not the old mismatch.

No changes in round 2.

## Round 3 (reviewing 60445c9d6 + e800af8ee)

| Reviewer | Model | Type | Result |
|---|---|---|---|
| r3-review-a | gpt-6-astra | code-review | 1 finding |
| r3-review-b | claude-sonnet-5 | code-review | No issues |
| r3-adversary-a | gpt-5.6-terra | rubber-duck (adversarial) | 1 finding |
| r3-adversary-b | gemini-3.8-flash | rubber-duck (adversarial) | No issues |

| # | Finding | From | Severity | Disposition |
|---|---|---|---|---|
| 1 | The `GetValue` (Text) change isn't needed for the crash. It changes app-visible `ReadLocalValue`/`ClearValue` state for Inlines-built text that starts with NUL, and no test covers it. | adversary-a | Non-blocking | **Taken.** Reverted `GetValue` to `IsNullOrEmpty`. The fix is now smaller with no app-visible change outside the crash path. Added to follow-ups. |
| 2 | The round 1 double-click "fix" still does a UI-thread call per click: `LeftMouseClickOnPoint` runs `EnsureInputReady` and `ConvertToPhysicalDisplayLocationStatic` (WindowHelper.cpp:1173). The comment and docs claim nothing runs on the UI thread between clicks. | review-a | Medium | **Partly taken.** Fixed the comment, description.md, and round 1 item 1 note. Kept the code: it matches the only existing mouse double-click in the suite (CommandingIntegrationTests.cpp:847-863), and the test infra has no single-call mouse double-click. Adding one is out of scope. The test passed on every VM run. |

Also traced with no issues: keyboard selection across empty paragraphs, TextPointer/GetPositionAtOffset/GetCharacterRect, `Select(start, end)`, `SelectionChanged`, ContextFlyout Copy, `SelectedText`, UIA `GetText`/`Move`/`ExpandToEnclosingUnit(Word)`, SAL annotations, and all three helper call sites (TextBlockView, RichTextBlockView, PageNode). r3-adversary-a also confirmed the tests cover both `SetTextToInlineCollection` and `ShouldRemoveEmptyRun` on their own.

Validation after round 3 (VM, amd64chk, GetValue reverted):

- `TextBlockTests::*LeadingNull*`: 3/3 passed.
- `RichTextBlockTests::*EmptyParagraph`: 2/2 passed.

## Conclusion

Round 3 only took cleanup changes: a smaller product diff and corrected docs. No reviewer in rounds 2 or 3 found a product defect. No further changes make sense, so review stops here.
