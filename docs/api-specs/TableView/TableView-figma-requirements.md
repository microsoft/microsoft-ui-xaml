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

### Dimensional and styling guardrails

Cell width minima/maxima and maximum heights are marked unavailable, not zero or
unlimited. Header/section minimum heights, hit targets, spacing tokens, corner
radii, and stroke mappings are not established as requirements by these sheets.
Paint bounds must not replace layout constraints: for example, a drawn compact
focus outline can be taller than the explicit 30-px cell minimum.

A's MCP style inventory includes Segoe UI Variable Body (14/20, regular) and
Body Strong (14/20, semibold), plus light/dark semantic styles. This establishes
available styles, not an exact per-part WinUI resource mapping. Exported RGB
values alone omit paint opacity and cannot define final rendered colors.

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
