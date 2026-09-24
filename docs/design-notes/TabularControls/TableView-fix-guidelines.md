# TableView fix guidelines

Apply these guidelines when triaging reports and implementing fixes. Make changes only when
the diagnosis and intended behavior are supported by evidence.

## 1. Check parity with other controls

- Compare the same interaction and state with the closest existing WinUI controls.
- Inspect their implementation, templates, theme resources, accessibility behavior, and tests;
  do not infer a contract from a screenshot or a similarly named property.
- Reuse established primitives, helpers, resource keys, and state-management patterns.
- Record the reference control and any intentional differences.

## 2. Check parity with other UI platforms

- Compare equivalent behavior with an appropriate platform, such as WPF DataGrid or a native
  Windows list view. Use official documentation or implementation source.
- Match the configuration: selection unit, focus model, resizing, templates, direction, and theme.
- Use cross-platform behavior as supporting evidence, not a requirement to copy APIs or gestures
  that conflict with TableView's design.
- Separate a missing capability from a regression or an incorrect implementation.

## 3. Be consistent with existing design and implementations

- Read the TableView API specification and development specification before changing behavior.
- Preserve the published contract, template customization, resource overrides, virtualization,
  focus behavior, and ownership/lifetime conventions.
- Fix the responsible layer. Shared primitive defects belong in that primitive; application
  template issues should not be patched by rewriting arbitrary application content.
- Avoid unrelated cleanup, duplicate logic, speculative APIs, and workaround-only fixes.
- Update directly affected documentation and coordinate with existing work rather than duplicating it.

## Confidence gate

Before implementing a fix:

- Establish expected versus actual behavior with a reproducible case or a direct source-level
  proof. State explicitly when a diagnosis has not been reproduced at runtime.
- Classify the report as a confirmed defect, intended behavior, capability gap, application issue,
  or unresolved investigation. Intended behavior can merit a design change, but is not automatically
  a bug.
- Identify the root cause and the smallest complete correction.
- Add a regression check that fails before and passes after the fix. Source-contract checks do
  not substitute for compiled or runtime behavior.
- Run the relevant build and focused tests. For UI changes, cover applicable input methods,
  Light/Dark/HighContrast, live theme switching, RTL, scaling, and application overrides.
- Leave uncertain behavior unchanged. Keep a PR in draft while required validation remains blocked,
  and report the blocker instead of claiming completion.

## Example: contrast-theme fixes

| Comparison | Evidence | Consequence |
| --- | --- | --- |
| WinUI TreeViewItem | `controls/dev/TreeView/TreeViewItem.xaml`, `CommonStates`, pairs foreground and background in hover, pressed, selected, and disabled states. | Apply both sides of the color pair using TableView's existing state group and presenter. |
| WPF DataGrid | `dotnet/wpf`, `src/Microsoft.DotNet.Wpf/src/Themes/XAML/DataGrid.xaml`, uses dynamic system brushes for text and selection. | Respect the user's system palette; do not hard-code a replacement contrast palette. |
| Existing TableView design | `controls/dev/CommonStyles/TabularSurfaces_themeresources.xaml` already defines Default, Light, and HighContrast tokens; `controls/Tabular.ProjectImports.targets` includes them. | Consume that canonical dictionary instead of maintaining theme-invariant fallback brushes in the default styles. |

References:

- [TableView API specification](../../api-specs/TableView/TableView-spec.md)
- [TableView development specification](TableView-dev-spec.md)
- [Windows contrast-theme guidance](https://learn.microsoft.com/windows/apps/design/accessibility/high-contrast-themes)
- [WPF DataGrid theme source](https://github.com/dotnet/wpf/blob/main/src/Microsoft.DotNet.Wpf/src/Themes/XAML/DataGrid.xaml)
