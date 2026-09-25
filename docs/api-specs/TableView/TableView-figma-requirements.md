# TableView Figma requirements (draft)

## Purpose and status

Request a TableView-specific Figma component and design handoff, aligned with the
Fluent Windows list and collection patterns. This document is a **proposal for
design review**, not an approved visual specification or an implementation change.
The [TableView API specification](./TableView-spec.md) remains the reference for
the control's API and feature scope.

The reference export does not contain a TableView component. TableView-specific
measurements, resource mappings, and interaction decisions therefore need explicit
design review; they must not be inferred as approved requirements from adjacent
controls.

## Reference material

The supplied `Fluent Windows Visual Library - IDC.zip` contains five single-page
PDF exports:

| Export | Component | Relevance to this proposal |
| --- | --- | --- |
| `Lists & Collections.pdf` | Grid View | Item interaction states and single/multiple selection references; not a tabular column-layout specification. |
| `Lists & Collections-1.pdf` | Expander | Expanded/collapsed affordance reference, not a TableView group-header specification. |
| `Lists & Collections-2.pdf` | Flip View | Collection-navigation reference; does not define TableView behavior. |
| `Lists & Collections-3.pdf` | Tree View | Hierarchy indentation, disclosure, checkbox, and item-state references for separately scoped hierarchical rows. |
| `Lists & Collections-4.pdf` | List View | Row interaction states, standard/multiple selection, dividers, headers, and custom multiline content references. |

The List View, Tree View, and Grid View sheets enumerate rest, hover, pressed,
disabled, and focus states. Some custom layouts are explicitly marked as design
patterns rather than built-in control variants. Reusing those patterns does not
automatically add the corresponding feature to TableView.

These exports are reference material, not an editable TableView Figma source or
a versioned design handoff. The archive and artwork are not redistributed in this
change. Any design links or assets added to this public repository must first be
approved for public sharing.

## Requested Figma deliverables

The following are proposed deliverables to review with design and engineering.
For every frame, identify whether it covers the current API or a separately
planned feature.

| ID | Deliverable | Required review coverage |
| --- | --- | --- |
| FIG-01 | Table anatomy and reusable components | Table surface, column header, row, text cell, template-cell slot, gridline/divider, leading-frozen-column boundary, and empty-state slot. Define component/variant names and composition rules. |
| FIG-02 | Row and cell state matrix | Rest, hover, pressed, disabled, selected/unselected, and keyboard focus; selected + hover/pressed/focus combinations; active versus inactive selection. Distinguish row selection from current-cell focus and show precedence when states overlap. |
| FIG-03 | Layout and density | Header and row heights, cell padding, spacing, corner radii, divider thickness, text baselines, and icon placement for each supported density. Annotate minimum sizes and content overflow/trimming behavior rather than estimating measurements from PDF scale. |
| FIG-04 | Typography, themes, and resources | Light, dark, and Windows contrast-theme treatments; typography styles; semantic foreground/background/border/focus resources; and a mapping from Figma variables to WinUI resources. Record any missing resource decisions explicitly. |
| FIG-05 | Headers, scrolling, and frozen columns | Header/body alignment while scrolling, leading-frozen-column boundary treatment, narrow/overflowing content, and horizontal/vertical scroll affordances. Do not introduce trailing-frozen columns or column virtualization through a visual example. |
| FIG-06 | Opt-in editing | Display versus edit mode, editor focus, commit/cancel transitions, validation-error presentation, and pending asynchronous validation. Distinguish built-in text editing from application-provided template editors. |
| FIG-07 | Accessibility and localization annotations | Keyboard-visible focus, non-color-only selection/error cues, contrast-theme behavior, text scaling, long/localized strings, and right-to-left layout. Annotate intended accessible names and state announcements for engineering review; Figma alone does not validate UI Automation or Narrator behavior. |
| FIG-08 | Separately planned interactions | Dedicated, clearly labeled frames for multiple/extended selection, checkbox selection if approved, sort indicators, filtering, grouping, hierarchical disclosure, and column resize/reorder. Resolve applicability and scope before treating these as implementation requirements. |
| FIG-09 | Representative compositions | A read-only table, a selected/focused row, a table with template content, an empty table, a horizontally scrolled table with leading-frozen columns, and an editable cell with an error. Include the supported theme/density variations. |

Do not copy ListView or TreeView dimensions, checkbox semantics, or hierarchy
depth into TableView without an explicit decision. Multiline examples in the
reference are not a commitment to automatic row sizing. Likewise, this proposal
does not expand the API specification's v1 scope to include marquee selection,
multi-column sort, column virtualization, or row headers.

## Handoff and acceptance checklist

- [ ] Identify the design owner and engineering reviewer.
- [ ] Provide the authoritative TableView Figma file and exact component/frame
      node links through an approved sharing location.
- [ ] Record the reviewed design version or dated snapshot so comparisons remain
      reproducible when the library changes.
- [ ] Resolve each FIG-01 through FIG-09 deliverable, or record an explicit
      deferral with its rationale and follow-up owner.
- [ ] Supply inspectable measurements, typography, semantic variables, icon
      choices, and state precedence; identify unresolved mappings rather than
      substituting guessed colors or pixel values.
- [ ] Separate current-control acceptance criteria from future-feature mockups
      and record any intentional deviations from the adjacent Fluent components.
- [ ] Agree on sample scenarios and screenshot conditions, including theme,
      density, scale, locale/direction, and focus/selection state.
- [ ] Review the implemented scenarios against the approved frames and record
      differences. Validate keyboard, Narrator/UI Automation, contrast themes,
      and text scaling in the running control separately from visual comparison.
- [ ] Obtain design and engineering sign-off before claiming Figma conformance.

## Open decisions

The reference export does not resolve the following TableView-specific questions:

- Which Figma component/frame set is authoritative, and which links can be
  published?
- Which state, density, and editing variants belong to the initial design handoff?
- What are the approved measurements and WinUI resource mappings?
- How should current-cell focus, row selection, editor focus, and inactive-window
  selection interact visually?
- Which planned interactions should receive exploratory frames now, and which
  should wait for their separate feature review?

Until these decisions and the handoff checklist are resolved, this document
records the Figma requirement without asserting that a final TableView design
exists or that the current control matches it.
