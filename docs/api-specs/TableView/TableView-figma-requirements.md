# TableView Figma requirements

Source: *Fluent Windows Visual Library - IDC*, file `Nv8iUI5i2SLf4vRxX16C17`, page
`72491:280393` (Lists & collections), TableView sheets **A** `182972:12908` and
**B** `183907:68060`. Reviewed 28 September 2026.

The two sheets carry **identical requirement text**: all 75 annotation blocks match.
They differ only in illustrations (see [Sheet differences](#sheet-differences)).

Quoted text is **verbatim** from the sheets. Values marked *Dev Mode* come from the Figma
inspect panel and take precedence over anything measured from the PDF export. Values marked
*measured* are read off the exported vectors and describe the drawn examples, not written
constraints.

This document states the design requirements. It does not assert that the current
implementation satisfies them; see [TableView-figma-coverage.md](./TableView-figma-coverage.md)
for review coverage and [TableView-spec.md](./TableView-spec.md) for the API.

## 00 General

> TableView displays data and controls in a tabular format.

**Table** size variants: `Regular`, `Compact`.

> A table is a minimum of 2 columns and 2 rows.

> If it includes "section", then each section in expanded state should have minimum 1 row.

**Types** — *Following types of tables are supported in TableView component*:

> Tables with and without header row
>
> Tables with and without sections

## 01 Header

**Size** variants: `Regular`, `Compact`.

> Header is an options element in a table
>
> More options () can be removed at column level

The "more options" affordance is drawn as a **down chevron** at the trailing edge of each
header cell.

## 02 Rows

**Table rows** variants: `Banded`, `None`, `Horizontal lined`, `Grid`.

> Banded is the default table type

**Row selection** variants: `Single`, `Multi`.

`Multi` rows carry a leading checkbox. Selected rows show a row-wide fill plus a leading
accent marker.

## 03 Sections

**State** variants: `Collapsed`, `Expanded`.

**Type** variants: `Regular`, `Strong`.

A section title spans the table width above its rows, with a separator rule. Collapsed uses
a right chevron, expanded an up chevron.

## 04 Columns

> All the column width of the table should remain customizable

Sizing modes:

> Ratio from viewport
>
> Fix width
>
> Auto width

## 05 Cell

**State** variants: `Rest`, `Hover`, `Focus`.

> minWidth: NA
>
> maxWidth: NA

**Size** variants and their written constraints:

| Size | Written constraint |
| --- | --- |
| `Regular` | `minHeight: 40px` / `maxHeight: NA` |
| `Compact` | `minHeight: 30px` / `maxHeight: NA` |

Cell `Content` frame layout (*Dev Mode*):

| Property | Value |
| --- | --- |
| Flow | Horizontal |
| Width | Fill (236px) |
| Height | Fill (36px) |
| Radius | 4px |
| Padding | Top 8px, Right 10px, Bottom 8px, Left 10px |
| Gap | 8px |

### Cell design patterns

> Cell can have multiple elements
>
> Multi-select is not permitted at cell level
>
> Single click to select cell.
>
> Double click in empty space to select row [Do we need row select?]
>
> Double click on content to select content
>
> Selecting a cell will highlight the respective row and column
>
> Spacing [Padding & margins] not to be controlled at cell level

The bracketed "[Do we need row select?]" is the designer's own open question, reproduced as
written. It is not a settled requirement.

### Truncation / Wrap logic

Variants: `Truncation`, `Wrap`, `Custom`.

> In case of wrap, change in height of 1 cell will extend height of entire row.
>
> The content of remaining cells will remain top aligned by default
>
> Need upper limit for lines in case of wrap
>
> If truncation is enabled, user should ALWAYS have option to resize columns & rows.

### Content alignment

Variants: `Left`, `Center`, `Right`, `Top`, `Middle`, `Bottom`. All nine combinations are
drawn; the example grid labels the first cell **Top Left - Default**.

The "Design patterns" note under Content alignment is an unfilled placeholder (`...`) and
carries no requirement.

## Colours

Source RGB with paint opacity, per theme. Opacity is part of the value; do not flatten it.

| Part | Light | Dark |
| --- | --- | --- |
| Selection accent / checked checkbox | `#005FB8` @ 100% | `#60CDFF` @ 100% |
| Alternating band fill | `#000000` @ 2.41% | `#FFFFFF` @ 4.19% |
| Hover fill | `#000000` @ 3.73% | `#FFFFFF` @ 6.05% |
| Section separator rule | `#000000` @ 16.22% | `#FFFFFF` @ 9.30% |
| Section chevron | `#000000` @ 89.56% | `#FFFFFF` @ 100% |

Sheet A's **00 General** dark example retains light-theme chevron and rule paints; its
dedicated **03 Sections** panel uses the correct dark paints, and Sheet B corrects the
General panel. Use the Sections panel values.

Contrast themes are not covered by either sheet.

## Measured example geometry

*Measured* from the exported vectors. Only the cell `Content` row above is authoritative
layout; these describe the drawn examples.

| Element | Value |
| --- | --- |
| Cell Content frame | 236 x 36 (Regular), 236 x 26 (Compact) |
| Cell focus outer ring | 240 x 40 (Regular), 240 x 32 (Compact) |
| Single-selection accent marker | 3 x 16 |
| Checked checkbox | 20 x 20 (exported as a filled rect, not a stroked outline) |
| Embedded editor (text field) | 238 x 30 |
| Sample column pitch, equal-width examples | 240 |
| Header example frame height | 32 (Regular), 26 (Compact) |

Header example heights are drawn sizes, not stated minimums. The sheets state minimum
heights for cells only. The 236 x 36 Content frame is painted with the hover fill in the
hover examples, which is why the same rectangle carries both the frame geometry and the
hover paint. Sheet B draws the embedded editor rectangle twice, so that geometry does not
identify a unique element there.

Body text renders at 14 units with a 20-unit wrapped-line step. The PDF export does not
carry a trustworthy font family name, so no family is asserted from it.

## Sheet differences

All requirement text is identical. The illustrations differ:

| # | Difference |
| --- | --- |
| 1 | Sheet B shifts General-panel content 8 units left and grows the block by 12 units. |
| 2 | Sheet B paints the General dark section chevrons and rules with dark-theme colours; Sheet A leaves them light-theme. |
| 3 | Sheet B adds a dark Regular embedded text-field example where Sheet A shows plain text. |
| 4 | Sheet B's light Table rows panel draws five examples; the extra one is a second horizontal-lined arrangement. Both sheets still name only the four row types. |

Neither sheet carries an approval marker or revision history, so neither is established as
superseding the other.

## Open questions carried by the design

Unresolved **in the source**:

1. `[Do we need row select?]` — whether double-click on empty space should select the row.
2. `Need upper limit for lines in case of wrap` — no line cap is chosen.
3. Row and column resize is required whenever truncation is enabled, but no affordance or
   bounds are designed.
4. How the row-and-column cross-highlight for a selected cell should look.
5. Whether the 2-column / 2-row minimum is a composition rule or a runtime constraint, and
   whether a header row counts toward it.

Not covered by these sheets at all: contrast themes, keyboard navigation, announcements,
sorting and filtering behaviour, column reorder, frozen columns, virtualization, editing
lifecycle, loading and error states. Absence here is not a decision against them.
