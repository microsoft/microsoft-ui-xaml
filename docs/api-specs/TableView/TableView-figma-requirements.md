# TableView Figma requirements (draft)

## Status and scope

This is a source-backed requirements draft for a later implementation-gap review,
not a statement of shipped behavior or design sign-off. It replaces the initial
generic handoff proposal: the supplied ZIP omitted TableView, but the live Figma
file contains TableView-specific designs.

**Whole-canvas review remains incomplete.** Both exported TableView sheets were
reviewed in detail, including their annotations and visual differences. Another
53 roots were inventoried, but still need detailed visual/component review.
See the [coverage ledger](./TableView-figma-coverage.md) for every root and the
access blockers. Do not interpret an item missing from this draft as absent from
the complete Figma file.

The [API specification](./TableView-spec.md) remains the API/scope reference.
This document does not change that scope, assess current implementation gaps,
or turn exploratory designs into release commitments.

## Sources

Reviewed September 28, 2026, in *Fluent Windows Visual Library - IDC*,
page `72491:280393` (Lists & collections):

- **A:** TableView sheet `182972:12908`.
- **B:** Adjacent TableView sheet `183907:68060`.
- **N1:** Variant/error/first-column-freeze note `183014:135964`.
- **N2:** Header/loading note `183027:14533`.

A/B references below identify named sections in both sheets; their exact panel
nodes are in the [source crosswalk](./TableView-figma-coverage.md#source-panel-crosswalk).
N1/N2 are supplemental note evidence, not completed component specifications.
Artwork and raw exports remain local.

**Explicit** means written in the source, not approved by a design owner.
**Example** means visibly illustrated; unspecified behavior must not be inferred.
Items referring to **D** decisions below are not ready for unconditional acceptance.

## Requirements

| ID | Requirement / design intent | Evidence |
| --- | --- | --- |
| TV-01 | Present data and embedded controls in rows and columns. Allow a header row and sections independently to be present or absent. | Explicit: A/B General, Header |
| TV-02 | Provide Regular and Compact density. Cell **minimum** heights are **40 px** and **30 px**, respectively; these are not fixed row heights or header minima. | Explicit: A/B Cell |
| TV-03 | Provide Regular/Compact column headers and allow the trailing options affordance to be omitted per column. A down-chevron is illustrated; menu contents and actions are unspecified (D6). | Explicit + example: A/B Header |
| TV-04 | Provide banded, unadorned, horizontal-line, and grid row treatments. **Banded is the stated default.** The grid examples include internal horizontal and vertical separators; they do not mandate an enclosing border. | Explicit + example: A/B Rows |
| TV-05 | Support single-row and multi-row selection. Do not conflate the unadorned row treatment with a no-selection mode. Selection gestures/modifiers remain unresolved (D2). | Explicit: A/B Rows |
| TV-06 | Single-selection examples use neutral row-wide emphasis and a short leading accent marker. Multi-selection examples show leading row checkboxes, checked/unchecked rows, and row-wide emphasis, across all four row treatments. | Example: A/B Row selection |
| TV-07 | Provide expanded/collapsed sections and Regular/Strong title treatments. Examples show a title spanning the table, a separator rule, a right chevron when collapsed, and an up chevron when expanded. | Explicit + example: A/B Sections |
| TV-08 | Keep column widths customizable with viewport-ratio, fixed, and automatic sizing choices. Preserve column alignment across groups. Sizing algorithms, bounds, defaults, and mixed-mode precedence need D5. | Explicit + example: A/B Columns |
| TV-09 | Provide cell Rest, Hover, and Focus presentations. Examples use quiet rest styling, neutral hover fill, and a visible rounded focus outline, at both densities. | Explicit + example: A/B Cell |
| TV-10 | Allow multiple elements inside a cell. Embedded controls are illustrated, but no editing lifecycle follows from their presence (D6). | Explicit + example: A/B General, Cell |
| TV-11 | Single-click selects a cell; cell-level multiselection is excluded. Double-clicking content is described as selecting that content, not necessarily entering edit mode. Interaction with controls and row selection needs D2. | Explicit: A/B Cell |
| TV-12 | Selecting a cell is intended to highlight its corresponding row **and column**. Exact paint, extent, and precedence relative to focus and true row selection remain unresolved. | Explicit, incomplete: A/B Cell; D2 |
| TV-13 | Do not expose independently controlled padding/margins at cell level. This is a configuration-ownership constraint, not a zero-padding requirement. Ownership and tokens need D5. | Explicit, incomplete: A/B Cell |
| TV-14 | Provide truncation, wrapping, and custom overflow handling. Examples cover Rest/Hover/Focus; ordinary truncation is illustrated as clipping, while the custom example preserves a suffix using an interior ellipsis. Default behavior and customization contract need D3. | Explicit + example: A/B Cell overflow |
| TV-15 | When wrapping makes a cell taller, increase the entire row's height; other cells remain top-aligned by default. No maximum wrapped-line count is settled (D3). | Explicit: A/B Cell overflow |
| TV-16 | The truncation notes require users to be able to resize **both rows and columns**. Affordances, bounds, and interaction with automatic row sizing remain undesigned (D3). | Explicit, incomplete: A/B Cell overflow |
| TV-17 | Support all nine horizontal/vertical alignment combinations: Left/Center/Right x Top/Middle/Bottom. **Top-left is the stated default.** | Explicit + example: A/B Cell alignment |
| TV-18 | Preserve light/dark treatments across table density, headers, row styles, selection, sections, and unequal column widths. Dedicated cell-state/overflow/alignment panels are light-only; they do not establish the complete dark or contrast-theme state matrix. | Example, partial: A/B General through Cell; D7 |
| TV-19 | A supplemental note says an optional header should remain visible when present. Treat this as sticky-header intent pending design confirmation, not a demonstrated scrolling implementation. | Note: N2; D6 |

## Measurements

**Explicit** values below are written design constraints. **Measured** values
describe the reviewed specimens, not approved defaults or hit targets. PDF
measurements use exported coordinate units; their conversion to runtime DIPs or
device pixels is not established. A/B source panels are listed in the crosswalk.

| Part | Regular | Compact | Evidence and interpretation |
| --- | --- | --- | --- |
| Cell minimum height | **40 px** | **30 px** | Explicit: A/B Cell. Content may make the row taller. |
| Header example frame height | 32 | 26 | Measured metadata: A `183101:72617` / `183101:72665`; B `183907:68229` / `183907:68234`. Not declared minimum heights. |
| Standalone cell hover background | 236 x 36 | 236 x 26 | Measured PDF paint bounds: A/B Cell. Inset background, not the cell's layout slot. |
| Standalone cell focus outer paint | 240 x 40 | 240 x 32 | Measured PDF outer bounds: A/B Cell. The compact focus paint does not redefine the 30-px minimum. |
| Single-selection accent marker | 3 x 16 | Density-specific contract unspecified | Measured PDF: A first single-selection example, x459-462 / y5488-5504; B corresponding y5763-5779. |
| Checked checkbox outline | 20 x 20 | Density-specific contract unspecified | Measured PDF: A checkbox example, x466-486 / y8298-8318; B corresponding y8573-8593. Not the interactive hit target. |
| Equal-width example column pitch | 240 | Default width unspecified | Measured PDF examples; TV-08 also requires customizable unequal widths. |
| Standalone cell text left inset | 12 from example cell origin; 10 from hover paint edge | Universal spacing token unspecified | Measured PDF: rest text x452 relative to cell x440; hover text x708 relative to paint x698. Different origins, not competing padding values. |

Cell width minima/maxima and maximum heights are marked unavailable, not zero or
unlimited. Section/header minimum heights, hit targets, universal padding,
corner radii, and stroke thicknesses still need D5. In particular, A/B General
examples differ in content alignment, so their offsets cannot establish a shared
padding token without a design decision.

## Colors and typography

### Observed light/dark paints

These are **source RGB plus paint opacity**, rounded from the exported vectors,
not flattened screen colors or prescribed WinUI resource keys. Preserve opacity
and the underlying surface when comparing the implementation. The table covers
the identified examples, not every state of every part.

| Part / example | Light RGB | Opacity | Dark RGB | Opacity | Source |
| --- | --- | --- | --- | --- | --- |
| Single-selection accent marker and checked checkbox accent | `#005FB8` | 100% | `#60CDFF` | 100% | A/B Row selection; approximate exported RGB, also present in A's MCP accent style inventory |
| Alternating band fill | `#000000` | 2.41% | `#FFFFFF` | 4.19% | A/B row examples |
| Hover fill | `#000000` | 3.73% | `#FFFFFF` | 6.05% | A/B table-cell hover examples; not a complete combined-state matrix |
| Section separator rule | `#000000` | 16.22% | `#FFFFFF` | 9.30% | A/B dedicated Sections examples |
| Section chevron | `#000000` | 89.56% | `#FFFFFF` | 100% | A/B dedicated Sections examples |

Do not copy A's General dark section-rule/chevron colors blindly: those examples
retain light-theme paints, unlike its dedicated Sections panel and B's corrected
General illustrations (D1). Selection background, focus stroke, text opacity,
disabled/error paints, and contrast-theme values are not fully mapped here.
Their absence is an evidence gap, not a requirement to reuse the nearest color.

### Available typography styles

A's MCP variable/style inventory supplies the following values. These establish
available styles, **not verified bindings for every table part**. Separately, both
PDFs show 14-unit control-label text and 20-unit wrapped-line baseline spacing.

| Figma style | Family | Face | Size / line height | Weight |
| --- | --- | --- | --- | --- |
| Body | Segoe UI Variable | Text Regular | 14 / 20 | 400 |
| Body Strong | Segoe UI Variable | Text Semibold | 14 / 20 | 600 |
| Caption | Segoe UI Variable | Small Regular | 12 / 16 | 400 |

All three reported styles have zero letter spacing. Strong section titles are
visibly heavier, but their exact style binding still needs inspection. Canonical
icon assets, numeric glyph IDs, and per-part resource mappings remain unresolved.

## Decisions required before conformance sign-off

| ID | Unresolved decision | Evidence / consequence |
| --- | --- | --- |
| D1 | Identify the authoritative sheet/version and reconcile differing illustrations. | A/B written notes agree, but General spacing differs; A's dark General section rules/chevrons conflict with its own dedicated dark Sections examples, while B changes those paints. B also adds an unnamed horizontal-line example. Do not infer a fifth row mode or authority from node numbering. |
| D2 | Define cell/content/row selection, focus, and embedded-control interaction. | Empty-space double-click row selection is explicitly questioned. Specify row/column cross-highlighting, modifiers, checkbox visibility, select-all/indeterminate behavior, and state precedence without assuming isolated specimens answer them. |
| D3 | Set overflow defaults, wrap limits, and row/column resize mechanics. | A three-line example is not an approved limit. Clarify how user resizing interacts with content-driven row growth and how custom truncation is configured. |
| D4 | Define empty, one-item, and undersized-table behavior. | General notes describe at least two columns/two rows and one row per expanded section, but do not say whether headers count or whether these are composition rules. Do not invent runtime rejection or hide valid data. |
| D5 | Specify layout/resource contracts. | Resolve ratio/auto/fixed sizing, minimum/maximum constraints, header/section dimensions, spacing ownership, typography/icon bindings, focus geometry, and theme resources. Example dimensions are not defaults. |
| D6 | Resolve supplemental and behavioral scope. | N1 explores first-column freeze and asks how errors should be shown; N2 requests loading-state design. Neither specifies a completed solution. Menu actions, sorting/filtering, reordering, editing lifecycle, section nesting, scrolling details, and virtualization are not established by the reviewed sheets. |
| D7 | Complete accessibility and state coverage. | Keyboard navigation, announcements, contrast themes, disabled/pressed/error states, complete dark cell states, text scaling, and RTL behavior are not established by the reviewed evidence. They may exist in unreviewed material and must not be fabricated. |
| D8 | Finish the whole-canvas review. | Inspect and classify all 53 supplemental roots, component variants, archive content, and application examples. Proximity to TableView or appearance inside File Explorer does not make every surrounding app feature a TableView requirement. |

## Later implementation-gap review

For each TV ID, record the source node, resolved design decision (if any), relevant
API/template/resource/code, and a reproducible sample or test. Classify the result
as **matches**, **behavior gap**, **visual gap**, **intentional deviation**, or
**design unresolved**. An unresolved D item is not automatically an implementation
bug. This PR records the requirements baseline; that comparison has not run.
