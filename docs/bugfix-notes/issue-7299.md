Fix crash when selecting a word in TextBlock/RichTextBlock text with a leading NUL or empty paragraph (#7299)

## Bug

Issue #7299: double-click (or double-tap) word selection crashes with an access violation.

The issue has two repros. They have two different root causes.

| Repro | Root cause |
|---|---|
| TextBlock whose `Text` starts with `\0` | TextBlock drops the text from its inline Run, but DWrite layout still uses the full string. Word selection walks past the end of the empty text container and AVs in `SelectionWordBreaker`. |
| RichTextBlock with an empty `Paragraph` (or empty `Span`) before the word | Paragraphs and Spans create their `InlineCollection` lazily, so an empty one is null. `TextBlockViewHelpers` dereferences it without a null check. |

### Why the leading NUL is dropped

`xstring_ptr::IsNullOrEmpty()` checks `GetBuffer()[0] == 0`. So a string like `L"\0word"` (count 5) looks empty.

- `CTextBlock::SetTextToInlineCollection` uses `IsNullOrEmpty`, so it adds an empty Run instead of the text.
- `ShouldRemoveEmptyRun` (text.cpp) also uses `IsNullOrEmpty`. The Run created by `AddText` isn't an explicit Run element, so this would remove it.
- The DWrite fast path lays out using `GetCount()`/`GetBuffer()`, so it still sees 5 characters.

The layout and the text container disagree. `TextSelectionManager::GetTextPosition` builds a position the container can't resolve, and `SelectionWordBreaker` dereferences a null.

## Fix

Use the character count instead of `IsNullOrEmpty` where the TextBlock decides whether its text is empty.

| File | Change |
|---|---|
| `TextBlock.cpp` `SetTextToInlineCollection` | `m_strText.GetCount() == 0` |
| `text.cpp` `ShouldRemoveEmptyRun` | `GetCount() == 0` |
| `TextBlockViewHelpers.cpp/.h` | Return `false` (keep walking) when the `InlineCollection` is null. Params changed to `_In_opt_`. |

Compat risk is low. See "Why it's safe" below.

## Why this fixes the issue

Leading NUL (TextBlock):

- The crash happens because the layout sees 5 characters but the text container sees 0. Selection then builds a position the container can't resolve.
- `SetTextToInlineCollection` now adds the full counted string as a Run (`TextBlock.cpp:488`). `ShouldRemoveEmptyRun` no longer removes that Run (`text.cpp:220`). So the container holds the same 5 characters that DWrite lays out, and the position resolves.
- This matches what TextBlock already does elsewhere. Its own empty-text fast paths already test `m_strText.GetCount() == 0` (`TextBlock.cpp:1658`, `:1790`, unchanged from main).
- The line services test covers the other layout path. It failed before the fix ("Element has no width") because that path lays out from the empty container.

Empty Paragraph or Span (RichTextBlock):

- The crash is a null dereference of `inlines` at the top of each helper. An empty Paragraph or Span has a null `InlineCollection`.
- All three helpers now return `false` for a null collection (`TextBlockViewHelpers.cpp:16`, `:100`, `:187`). `false` means "not found here, keep walking", which is the right answer for an empty collection. The callers then move on to the next paragraph (`RichTextBlockView.cpp:440`, `:502`, `PageNode.cpp:237`) or the next inline. That's the same result as a non-null collection with no items.

All 5 tests crashed or failed before the fix and pass after it.

## Why it's safe

Who can see a behavior change:

| Case | Before | After |
|---|---|---|
| `Text` with count 0 | Empty Run | Unchanged. `IsNullOrEmpty` was already true for these. The two checks only differ when count > 0 and the first char is NUL. |
| `Text` with no NUL, or a NUL that isn't first | Full Run | Unchanged. `IsNullOrEmpty` was already false. |
| `Text` that starts with NUL, count > 0 | Renders the text, container empty, selection crashes | Renders the text, container matches, selection works |
| Explicit `<Run>` elements | Never removed by `ShouldRemoveEmptyRun` (`text.cpp:210` checks `RunFlagsRunElement`) | Unchanged |
| RichTextBlock with no empty Paragraph or Span | Works | Unchanged. The null checks only run when `inlines` is null. |
| RichTextBlock with an empty Paragraph or Span | AV on selection, highlight, or IUC lookup | Skips the empty collection |

Other risks I checked:

- **Who reaches `ShouldRemoveEmptyRun`.** It's called from `CompressInlinesWhitespace`, used by TextBlock (`TextBlock.cpp:181`) and Paragraph (`paragraph.cpp:148`). It only looks at Runs created from content, not explicit Run elements. XAML markup can't contain a NUL character, so in practice only code that sets `Text` with a leading NUL sees the change. Those apps used to crash on word selection.
- **Reading `Text` back.** Still returns `""` for a leading-NUL string, same as before. That's a separate, pre-existing issue (see Follow-ups).
- **Chk-only `VERIFY` in `TextBlockView`.** A null collection now returns `false` instead of crashing. That can trip the chk assert in one unreachable-from-selection case (see Follow-ups). Release builds are unaffected.

## Tests

New tests in `Microsoft.UI.Xaml.Tests.External.Foundation.dll`:

| Test | What it does |
|---|---|
| `TextBlockTests::DoubleTapTextWithLeadingNull` | Touch double-tap on `L"\0word"`, DWrite layout. Expects `SelectedText == "word"`. |
| `TextBlockTests::DoubleClickTextWithLeadingNull` | Mouse double-click, same text. |
| `TextBlockTests::DoubleTapTextWithLeadingNullLineServices` | Touch double-tap with `CharacterSpacing = 100`, which forces line services layout. |
| `RichTextBlockTests::DoubleTapWordAfterEmptyParagraph` | RichTextBlock with `<Paragraph/>`, `<Paragraph><Span/>Hello</Paragraph>`, `<Paragraph/>`. Expects `SelectedText == "Hello"`. |
| `RichTextBlockTests::HighlightWordAfterEmptyParagraph` | RichTextBlock with `<Paragraph/>`, `<Paragraph><Span/>Hello</Paragraph>`, and a `TextHighlighter` on range {0, 5}. Rendering the highlight walks the same helpers. Expects no crash. |

The three TextBlock scenarios are separate test methods on purpose. Swapping window content after a touch word selection in the same test kills the test host, even with plain text like `"word"`. That's a separate, pre-existing issue and isn't related to this fix.

The mouse test computes the click point once, then clicks it twice. This matches the mouse double-click in `CommandingIntegrationTests`. Each click still does a short UI-thread call to convert coordinates, but the test infra has no single-call mouse double-click. The test passed on every VM run.

## Validation (local Hyper-V VM, amd64chk)

| Test | Before fix | After fix |
|---|---|---|
| `DoubleTapTextWithLeadingNull` | Failed (crash in MUX) | Passed |
| `DoubleClickTextWithLeadingNull` | Failed (host exit 0xC0000005) | Passed |
| `DoubleTapTextWithLeadingNullLineServices` | Failed ("Element has no width", SelectedText empty) | Passed |
| `DoubleTapWordAfterEmptyParagraph` | Failed (host exit 0xC0000005) | Passed |
| `HighlightWordAfterEmptyParagraph` | Failed (test host died on first render) | Passed |

Regression run of all `Graphics::TextBlockTests::*` and `Graphics::RichTextBlockTests::*` with the fix: 38 passed, 2 failed.

The 2 failures (`RenderFastPathTextBlocksWUCFull`, `TestTextBlockLineStackingStrategies`) are master-file render mismatches. They also fail on this VM without the fix.

## Follow-ups (not fixed here)

- `xruntime_string_ptr::DetachHSTRING` uses `IsNullOrEmpty`, so reading `TextBlock.Text` back returns `""` for a string that starts with NUL. This affects every string property, so it needs its own change.
- `CTextBlock::GetValue` (Text) uses `IsNullOrEmpty` to decide whether to mark `Text` local after rebuilding it from Inlines. For a Run that starts with NUL, `Text` is not marked local. Left unchanged here because it doesn't affect the crash and changing it would change `ReadLocalValue` results.
- `TextSelectionManager.cpp` (~line 2037, copy) also uses `IsNullOrEmpty` on selected text.
- `SelectionWordBreaker.cpp` (~lines 281 and 457) keeps `HStringReference(...).Get()` past the temporary's lifetime.
- Swapping window content after a touch word selection in one test kills the TAEF host (see Tests).
- `CHyperlink::GetContentText` (Hyperlink.cpp ~line 783) may dereference a null `m_pInlines` for an empty Hyperlink through UIA. Not confirmed.
- `TextBlockViewHelpers::AdjustPositionByCharacterCount` checks for the end of a collection by comparing the running position (which includes earlier paragraphs) to the collection's own length. For any paragraph after the first, a highlight that ends exactly at the paragraph end runs on to the next paragraph's start. This is pre-existing and affects all multi-paragraph RichTextBlocks.
- `TextBlockView::GetAdjustedPosition` and `GetCharacterIndex` wrap the helpers in `VERIFY`. A position inside a trailing empty Span now returns `false` instead of crashing, which would trip the assert in chk builds. Release builds are unaffected, and I couldn't reach this path from word selection.
