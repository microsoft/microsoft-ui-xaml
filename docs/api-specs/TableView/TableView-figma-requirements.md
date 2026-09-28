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

The header cell is the **same `cell` component** with `Type: Header cell` (*inspect panel*,
dark-theme table):

| Component property | Value shown |
| --- | --- |
| Compact | False |
| Type | Header cell |
| Filter options | true |

| Layout property | Value |
| --- | --- |
| Flow | Horizontal |
| Width | Fixed (240px) |
| Height | **Hug (32px)** |
| Padding | 2px |
| Gap | 8px |

Border: 1px, all sides, outer alignment, `Dark/Fill Color/Subtle/Transparent` = `#FFFFFF` 0%.

Two points worth carrying forward:

1. **The header is shorter than a body row**: 32px versus the body cell's 40px at the same
   `Compact: False` density. The measured Compact header is 26px. Header height is
   therefore its own value, not a copy of the row height.
2. The removable affordance is exposed as **`Filter options`**, while the annotation calls
   it "More options". The drawn glyph is a down chevron. The component name suggests
   filtering rather than a generic overflow menu; the two names should be reconciled with
   design before an implementation picks a behaviour.

## 02 Rows

**Table rows** variants: `Banded`, `None`, `Horizontal lined`, `Grid`.

> Banded is the default table type

The table frame holding the four row-style examples (*inspect panel*, identical in light and
dark):

| Property | Value |
| --- | --- |
| Name | `Frame 3465221` — a plain frame, **not a component instance** |
| Flow | Vertical |
| Width | Fixed (720px) |
| Height | Hug (160px) |

720px is three 240px columns with no column gap, and 160px is four 40px rows with **no row
gap** — rows butt directly against each other. This matches the `Section` component's fixed
720px width.

> **Row styling is not modelled as a component property.** The four row treatments are
> hand-assembled frames, not variants of a table component, so there is no authoritative
> per-variant token mapping to read. `Banded`, `None`, `Horizontal lined` and `Grid` are
> specified only by the annotation chips and the drawn examples. An implementation must
> derive each treatment's gridline and banding brushes from the measured paints rather than
> from a component definition.
>
> Separately, nine `Table / Table` component instances exist elsewhere on the canvas at
> different sizes (768 x 160, 768 x 120, 463 x 160). Whether that component supersedes or
> predates these frames is unresolved.

How the treatments differ at the **row** level, sampled from the `02 Rows` examples:

| Treatment | Row radius | Row border | Row fill |
| --- | --- | --- | --- |
| `Banded` | 4px | none | alternating band fill |
| `Horizontal lined` | none | Bottom 1px | none |
| `None` | *not sampled* | *not sampled* | *not sampled* |
| `Grid` | *not sampled* | *not sampled* | *not sampled* |

Both sampled rows are Fill 720px x Hug 40px. The treatments therefore differ by radius,
border and fill rather than by size. This matches the implementation's model, where
`GridLinesVisibility` drives row `BorderThickness` and `AlternatingRowBackground` drives the
band fill, with the row style already carrying `ControlCornerRadius` (4px).

### Banding parity: the first row is shaded

Measured from the `02 Rows` banded example. Its four rows start at y = 3056, 3096, 3136 and
3176 (a 40px pitch), and the band fills sit at y0 = 3058 and 3138 — inset 2px by the cell
padding. The bands therefore fall on the **first and third** rows, i.e. **even 0-based
indices**. The `Table / Table` component export shows the same pattern: first body row
shaded, then alternating.

`TableViewRow::RefreshRowBackground` now applies `AlternatingRowBackground` when
`(rowIndex % 2) == 0`, so the first row is shaded. This intentionally departs from WPF
`DataGrid`, which shades odd rows; apps that set both `RowBackground` and
`AlternatingRowBackground` will see the two brushes swap rows.

**Row selection** variants: `Single`, `Multi`.

`Multi` rows carry a leading checkbox. Selected rows show a row-wide fill plus a leading
accent marker.

The accent marker is a component named **`Selector`** (*inspect panel*):

| Property | Value |
| --- | --- |
| Width | 3px |
| Height | 16px |
| Radius | 999px (fully rounded) |
| Fill (light) | `Light/Fill Color/Accent/Default` = `#005FB8` |
| Fill (dark) | `Dark/Fill Color/Accent/Default` = `#60CDFF` |

`Fill Color/Accent/Default` is the Fluent token that WinUI exposes as
`AccentFillColorDefaultBrush` — `SystemAccentColorDark1` in Light and
`SystemAccentColorLight2` in Dark. It is **not** the raw `SystemAccentColor`.

## 03 Sections

**State** variants: `Collapsed`, `Expanded`.

**Type** variants: `Regular`, `Strong`.

A section title spans the table width above its rows, with a separator rule. Collapsed uses
a right chevron, expanded an up chevron.

The `Section` component's actual properties and layout (*inspect panel*):

| Component property | Value shown |
| --- | --- |
| Type | Default |
| Expanded | True |
| Expandable | true |

| Layout property | Value |
| --- | --- |
| Flow | Vertical |
| Width | Fixed (720px) |
| Height | Hug (29px) |
| Border | Top 1px |
| Padding | Bottom 4px |
| Gap | 4px |

The 720px fixed width is three 240px columns, and excludes the row's 8px left padding. The
separator is a **1px top border** on the section itself, which matches the 1-unit rules
measured in the export.

> **Two mismatches with the annotation chips.**
>
> 1. The annotations list `Type` as `Regular` / `Strong`, but the component reports
>    `Type: Default`. The full value set was not enumerated, so `Default` may be an
>    additional or renamed value.
> 2. The component exposes a separate boolean **`Expandable`** that the annotations never
>    mention, and models expansion as a boolean `Expanded` rather than a
>    `Collapsed`/`Expanded` state. A non-expandable section header therefore appears to be
>    supported, which is not stated anywhere in the sheet text.

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

### Cell component

The `cell` component's properties and layout (*inspect panel*, dark-theme table):

| Component property | Value shown |
| --- | --- |
| Compact | False |
| Type | Table cell |
| Hover | false |

| Layout property | Value |
| --- | --- |
| Flow | Horizontal |
| Width | Fixed (240px) |
| Height | Hug (40px) |
| Min height | **40px** |
| Padding | 2px |
| Gap | 8px |

Border: **1px, all sides, outer alignment**, bound to token
`Dark/Fill Color/Subtle/Transparent` = `#FFFFFF` at **0%** (and the `Light/...` equivalent
in light examples). The border is present but fully transparent by default, reserving the
1px for gridline treatments.

`Min height: 40px` is a real auto-layout constraint on the component, not merely annotation
text, so the written `minHeight: 40px` is enforced by the design itself. The cell's border
stays transparent in the **Focus** examples too, so the focus ring measured at 240 x 40 is a
separate element rather than the cell's own border.

Density is modelled as a **boolean `Compact`**, not a Regular/Compact enum: `Compact: False`
is the Regular cell. `Type` is `Table cell`, matching the annotated table-cell/header-cell
split. `Hover` is a boolean property on the component rather than an interaction state.

### Captured colour tokens

Token names as reported by the inspect panel. The cell itself is transparent in **both**
hover states, so hover must be painted on the `Content` child rather than the cell box.

| Element | State | Token | Value |
| --- | --- | --- | --- |
| `cell` fill (light) | `Hover: false` | *no fill layer* | — |
| `cell` fill (light) | `Hover: true` | `Light/Fill Color/Subtle/Transparent` | `#FFFFFF` 0% |
| `cell` fill (dark) | `Hover: false` | `Dark/Fill Color/Subtle/Transparent` | `#FFFFFF` 0% |
| `cell` fill (dark) | `Hover: true` | `Dark/Fill Color/Subtle/Transparent` | `#FFFFFF` 0% |
| `cell` border (light) | both | `Light/Fill Color/Subtle/Transparent` | `#FFFFFF` 0%, 1px all sides, outer alignment |
| `cell` border (dark) | both | `Dark/Fill Color/Subtle/Transparent` | `#FFFFFF` 0%, 1px all sides, outer alignment |
| `Header cell` border (dark) | — | `Dark/Fill Color/Subtle/Transparent` | `#FFFFFF` 0%, 1px all sides, outer alignment |

Setting `Hover: true` adds a fill layer to the cell, but that layer resolves to
`Subtle/Transparent` at 0%. The `Content` child of a hovered cell reports **no Colors block
at all**. So the visible hover wash — measured at 236 x 36, the Content frame size — is
painted by neither the cell box nor the `Content` frame itself. The node that carries it has
not been identified; it is likely a sibling or deeper child. Until it is, the hover fill is
known only by its measured value.

Token names for the painted states — hover fill, band fill, selection accent, separator
rule, chevron — have **not** been read from the panel yet; the values below come from the
export. Selecting the `Content` child of a hovered cell would give the hover token name.

### Cell and Content geometry reconcile exactly

The cell's 2px padding accounts for the difference between the cell box and its `Content`
child, for both densities:

| Density | Cell | minus 2px padding | Content frame |
| --- | --- | --- | --- |
| Regular | 240 x 40 | -4 each axis | **236 x 36** |
| Compact | 240 x 30 | -4 each axis | **236 x 26** |

Both Content sizes match the frames measured in the export, and the cell heights match the
written 40px / 30px minimums. The effective left text inset is 2px (cell) + 10px (Content) =
**12px**, which matches the 12-unit inset measured from the cell origin in the export.

Cell `Content` frame layout (*inspect panel*):

| Property | Value |
| --- | --- |
| Flow | Horizontal |
| Width | Fill (236px) |
| Height | Fill (36px) |
| Radius | 4px |
| Padding | Top 8px, Right 10px, Bottom 8px, Left 10px |
| Gap | 8px |

Row layout (*inspect panel*). Header rows and body rows are different heights, and the
earlier apparent conflict was a header row being compared against body rows:

| Property | Header row (`00 General`) | Body row (`02 Rows`) | Body row (`00 General`) |
| --- | --- | --- | --- |
| Flow | Horizontal | Horizontal | Horizontal |
| Width | Hug (728px) | Fill (720px) | Fill (712px) |
| Height | **Hug (32px)** | **Hug (40px)** | **Hug (40px)** |
| Radius | 4px | 4px | 4px |
| Padding | Left 8px | none | none shown |

This is fully consistent with the cell components: a `Header cell` is 240 x 32 and a
`Table cell` is 240 x 40, so a header row hugs to 32px and a body row to 40px. The written
Regular cell minimum of 40px applies to body cells.

The `02 Rows` body row is the reference case: it fills the table's 720px width exactly
(three 240px columns, no padding) and hugs to 40px, matching the table frame's 160px ÷ 4
rows. The `00 General` header row's extra 8px left inset — hence 728px rather than 720px —
appears only in that mock and is not corroborated elsewhere.

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

## Typography

The file's text style ramp, read from the Figma styles panel. Cell and header text use
**Body** and **Body Strong** (both 14/20), which matches the 14-unit size and 20-unit
wrapped-line step measured in the export.

| Style | Size / line height |
| --- | --- |
| Caption | 12 / 16 |
| Body | 14 / 20 |
| Body Strong | 14 / 20 |
| Body Large | 18 / 24 |
| Body Large Strong | 18 / 24 |
| Subtitle | 20 / 28 |
| Title | 28 / 36 |
| Title Large | 40 / 52 |
| Display | 68 / 92 |

The PDF export carries Type3 subset fonts with `n/a` base names, so no font family is
asserted from the export itself.

## Colours

Source RGB with paint opacity, per theme, read from the exported vectors. These are the
rendered values; the **token names** behind them are still being collected (see
[Captured colour tokens](#captured-colour-tokens)).

| Part | Light | Dark |
| --- | --- | --- |
| Selection accent / checked checkbox | `#005FB8` @ 100% (`Fill Color/Accent/Default`) | `#60CDFF` @ 100% (`Fill Color/Accent/Default`) |
| Alternating band fill | `#000000` @ 2.41% | `#FFFFFF` @ 4.19% |
| Hover fill | `#000000` @ 3.73% | `#FFFFFF` @ 6.05% |
| Section separator rule | `#000000` @ 16.22% | `#FFFFFF` @ 9.30% |
| Section chevron | `#000000` @ 89.56% | `#FFFFFF` @ 100% |
| Cell fill and border (rest and hover) | — | `#FFFFFF` @ 0% (`Subtle/Transparent`) |

Opacity is part of the value; do not flatten it.

Sheet A's **00 General** dark example retains light-theme chevron and rule paints; its
dedicated **03 Sections** panel uses the correct dark paints, and Sheet B corrects the
General panel. Use the Sections panel values.

Neither sheet draws a contrast-theme table, so contrast appearance for TableView is not
specified here. The library itself does carry a **Contrast** colour-style group alongside
**Light** and **Dark** (each split into Fill Color, Elevation, Stroke Color, Background and
Shell), so contrast tokens exist to map against once the appearance is designed.

## Measured example geometry

*Measured* from the exported vectors, now superseded where an inspect panel exists.

| Element | Value |
| --- | --- |
| Cell focus outer ring | 240 x 40 (Regular), 240 x 32 (Compact) |
| Single-selection accent marker | 3 x 16 |
| Checked checkbox | 20 x 20 (exported as a filled rect, not a stroked outline) |
| Embedded editor (text field) | 238 x 30 |
| Compact header cell height | 26 |

The Regular header height of 32px is confirmed by the inspect panel; the Compact 26px
figure is still export-measured only. The focus ring is outer-aligned, so at Regular it
coincides with the 240 x 40 cell box. Sheet B draws the embedded editor rectangle twice, so
that geometry does not identify a unique element there.

Body text renders at 14 units with a 20-unit wrapped-line step, matching the Body and Body
Strong styles listed under [Typography](#typography).

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

## Implementation status

Addressed in this PR:

| Requirement | Change |
| --- | --- |
| Banded is the default table type | Default style supplies `AlternatingRowBackground` and sets `GridLinesVisibility="None"`. Local values still override. |
| Banding must stay distinguishable from hover | Band brushes moved to the weaker Fluent tertiary fill (Light `#06000000`, Dark `#0AFFFFFF`) instead of reusing the hover fill. Contrast-theme system colours unchanged. |
| Selection accent uses `Fill Color/Accent/Default` | `TabularSurfaceSelectionIndicatorBrush` now resolves `SystemAccentColorDark1` in Light and `SystemAccentColorLight2` in Dark, matching `AccentFillColorDefaultBrush`. It previously used raw `SystemAccentColor`, which is the wrong shade and was identical in both themes. HighContrast keeps `SystemColorHighlightColor`. |
| Header is shorter than a body row | New `TableViewHeaderMinHeight` resource (32 Standard / 26 Compact / 40 Comfortable), resolved by `GetDensityHeaderMinHeight()`. Headers previously reused the body row min-height. |
| Banding shades the first row | `RefreshRowBackground` now bands even 0-based indices. Departs from WPF `DataGrid`, which bands odd rows. |

The `Selector` geometry already matches: the template draws a 3 x 16 rectangle with
`RadiusX`/`RadiusY` of 1.5, which on a 3px width is the fully rounded shape Figma expresses
as radius 999px.

Cell minimum heights already match: `TableViewRowMinHeight` is 40 Standard / 30 Compact.

### Header height

The header cell is **32px** at Regular and 26px at Compact, independent of the body cell's
40px / 30px, and the header *row* hugs to 32px to match. Headers now resolve their own
`TableViewHeaderMinHeight` resource (32 Standard / 26 Compact / 40 Comfortable) through
`GetDensityHeaderMinHeight()`, instead of reusing `GetDensityRowMinHeight()`.

Comfortable is not covered by the design; it keeps Standard's -8 delta from the row height.

### Not attempted

Content alignment (`Top Left - Default`), the `Filter options` affordance, section
`Expandable`, cross-highlight on cell selection, and the wrap/resize requirements are
recorded but unimplemented. Several depend on the open questions below.

## Open questions carried by the design

Unresolved **in the source**:

1. `[Do we need row select?]` — whether double-click on empty space should select the row.
2. `Need upper limit for lines in case of wrap` — no line cap is chosen.
3. Row and column resize is required whenever truncation is enabled, but no affordance or
   bounds are designed.
4. How the row-and-column cross-highlight for a selected cell should look.
5. Whether the 2-column / 2-row minimum is a composition rule or a runtime constraint, and
   whether a header row counts toward it.
6. Whether the header row's extra 8px left inset in the `00 General` mock is intentional or
   an artefact; body rows elsewhere have no inset.
7. The `Section` component's full `Type` value set, and whether `Default` replaces or
   supplements the annotated `Regular` / `Strong`.
8. What a non-expandable section (`Expandable: false`) looks like and when it is used. The
   property exists on the component but appears nowhere in the sheet text.
9. Whether the header affordance is a filter (component property `Filter options`) or a
   general overflow menu (annotation wording "More options").

Not covered by these sheets at all: contrast themes, keyboard navigation, announcements,
sorting and filtering behaviour, column reorder, frozen columns, virtualization, editing
lifecycle, loading and error states. Absence here is not a decision against them.
