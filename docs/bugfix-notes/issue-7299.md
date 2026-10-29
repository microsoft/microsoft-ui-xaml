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

Compat risk is low:

- A string with count 0 behaves the same as before.
- A string that starts with NUL used to render the text (DWrite) but had an empty text container, which crashed on selection. Now the container matches the rendered text.
- The null-guard changes only affect code paths that used to AV.

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
