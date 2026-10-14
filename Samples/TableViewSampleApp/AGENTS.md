# TableView sample — build notes

The end-user quick start is in [README.md](README.md). This file records how the sample is wired
and why, for anyone maintaining it.

## There is nothing unusual here any more

The sample used to hand-wire around the product, because `TableView`'s type information was withheld
from the public winmd and its theme resources did not resolve in a consuming app. It linked raw build
output, regenerated its own CsWinRT projection, injected a XAML metadata provider, seeded an internal
template part into `XamlTypeInfo`, compiled the control's theme resources out of the control source
tree, merged activatable-class registrations into its own app manifest, and staged the control DLL
next to the EXE.

None of that is needed. The project file is short and the sample is an ordinary package
consumer, modelled on the [ChartApp](../ChartApp) samples — the existing pattern in this repo for a
separately-built control set.

If you find yourself reaching for one of those workarounds again, that is a signal the product
regressed. Check, in order:

1. Tabular types are in the public merged winmd (`MergedWinMD`), not gated out.
2. `<ActivatableClass>` registrations are emitted — they derive from the same merge inputs, so a
   withheld winmd silently removes them too.
3. The control's theme XBFs ship in the package and its default-style URI is authority-less
   `ms-appx:///`, matching the path in `AppxPriInitialPath`.

## How it builds

`TableView` is not in any published `Microsoft.WindowsAppSDK.WinUI` package yet, so the sample only
compiles against a locally packed component. `Build.cmd samples` handles that — it runs
`pack.component.cmd` and then builds every sample with the matching `/p:WinUIVersion`:

```
.\Build.cmd product
.\Build.cmd samples
```

Building the project on its own resolves `Microsoft.WindowsAppSDK.WinUI` from the feed instead, and
fails with `CS0234: The type or namespace name 'Tabular' does not exist` — the package is real, it
just predates Tabular in the public winmd. To iterate on this app alone, pack and override:

```
.\pack.component.cmd /version 3.0.0-mylocal
.\initrun.ps1 msb /q /restore Samples\TableViewSampleApp\TableViewSampleApp.csproj /p:Platform=x64 /p:WinUIVersion=3.0.0-mylocal
```

NuGet caches by version under `packages\microsoft.windowsappsdk.winui\`, so re-packing the same
version after a control change is silently ignored — use a fresh version string, or delete that
folder first.

The control DLL, its `.pri` and its theme XBFs arrive from the package. The consuming app's build
expands every referenced `.pri` and re-indexes it into the app's own `TableViewSampleApp.pri`, which
is why that file is several megabytes: it contains MUXC's and Tabular's resources as well as the
sample's.

## Two things worth knowing

**Tabular's theme resources must be merged.** The control resolves its own `generic.xaml` from the
package, but the theme resources that default style depends on live in `TabularControlsResources`,
so `App.xaml` merges that alongside `XamlControlsResources`. `SortIndicator` ships in the Tabular
DLL rather than in MUXC — `controls/Tabular.ProjectImports.targets` is the only importer of
`SortIndicator.vcxitems` — so `SortIndicatorForeground`, which `TableView`'s column-header style
resolves, is absent from MUXC's dictionary. Dropping the merge produces
`XamlParseException 0x802B000A` on the first layout pass, seen as a `0xC000027B` stowed exception
shortly after launch. If a table appears with no header row, that is a different problem: it is
almost always **no declared columns**, not a missing dictionary.

**`TableView` is `[MUX_PREVIEW]`.** C# usage raises `CS8305`, suppressed via `NoWarn` in the project
file. XAML usage raises `WMC1501` once per page; those are deliberately left visible.

## Entry point

`Program.cs` provides `Main` and the project defines `DISABLE_XAML_GENERATED_MAIN`, following the
[DisableXamlGeneratedMain](../DisableXamlGeneratedMain) sample. This is a normal WinUI pattern, not a
workaround.

`Main` forwards command-line arguments to `App`, and `App` forwards them to `MainWindow`. Supported
shell arguments are:

- `--page=<tag>` — open one of the page-map/navigation tags directly. This is intentionally supported
  for UIA automation and local verification.
- `--show-dev-info` — show the developer status bar that reports the loaded Tabular controls assembly.
- `--verify-groups` / `--verify-selection` — local automation log flags, compiled only when
  `TableViewSampleEnableVerificationLogs=true` defines `TABLEVIEW_SAMPLE_VERIFY`. Public sample
  builds must not write diagnostic log files next to the executable by default.

## Shell invariants

`App.xaml` owns the sample-specific row-brush resources used by grid-line/row-background demos.
Keep these resource key names stable because pages resolve them with `{ThemeResource}`:

- `SampleCustomRowBackgroundBrush`
- `SampleCustomAlternatingRowBackgroundBrush`

Theme dictionaries must use only the recognized keys `Light`, `Dark`, and `HighContrast`. The
high-contrast entries must use `SystemColor*` resources rather than accent colors.

`SamplePresenter` is a normal UserControl in this assembly. Bind `Header`, `Description`, `Example`,
and `Options` with `x:Bind`; do not reintroduce split-binary/CsWinRT manual content sync
workarounds. Its `SourceSnippet` and `AdditionalSnippet` values (or `Snippet="X"`, which names
`X.xaml.txt` and `X.cs.txt`) must correspond to files under `Snippets\*.txt`;
`tools\Update-Snippets.ps1 -Check` validates those declarations (see Snippets below).

`BuildInfo` is generated to `$(IntermediateOutputPath)\BuildInfo.g.cs` and included from there.
Do not check in generated `BuildInfo.cs`, and do not change the existing `BuildInfo` field names
without updating `Pages\AboutPage.xaml.cs`.

The pages deliberately source-free today are `AboutPage.xaml`, `HomePage.xaml` and
`SettingsPage.xaml`. Every other page declares `Snippet="X"`.

## Page structure

A page holds only the TableView behaviour it demonstrates; the generic parts are shared:

- `Pages\SamplePageBase.cs` — the page base (`<pages:SamplePageBase>` XAML root): Last action,
  readout refresh, Loaded/Unloaded tracking (`TrackItems`, `TrackTimer` for both timer types,
  `TrackLifetime`), `EnqueueIfLoaded`, `BeginBulkUpdate`, and the page hooks `OnShapingApplying`,
  `OnShapingApplied`, `OnShapingAction` (Expand all / Collapse all) and `OnLastActionSet`.
- `Controls\RailSection.cs`, `Controls\ShapingOptions.xaml`, `Controls\StatusPanel.cs` — the rail's
  section chrome, the Shaping section (Flat, Grouped, Hierarchy and Grouped hierarchy: GroupBy /
  ClearGroupBy and ParentBy / ClearParentBy on the page's one source, and gating; the control itself
  keeps the selection across the reshape. A page passes its `ParentRelation` to `Attach`, Person rows
  default to `ParentRelation.Manager`, and a page whose rows have no parent relation sets
  `HierarchyUnavailableReason`) and the Status grid with its fixed Rows → Shaping → Last action
  tail. Only Last action is a live region (Polite, raised once per new message); a readout that
  must be announced on its own sets `AutomationProperties.LiveSetting` itself. `ShapingOptions.TimeCall` lets a page
  time the GroupBy / ClearGroupBy and Expand / Collapse all calls. `SamplePresenter.TryIt` renders
  the Try it callout. The rail has two tabs (`RailTabs`: `RailOptionsTab`, default, and
  `RailInfoTab`) and scrolls on its own, bounded to the Example row (`Controls\RailHost.cs`).
  Options holds `SamplePresenter.Options`; Info holds `WhatToLookFor`, the page's `InfoExtras` and
  the shared `Controls\KeyboardReference.cs` (hierarchy keys with a parent relation or
  `ShowHierarchyKeys`).
- `Pages\<X>Page.Status.cs` — the page's own readouts (`RefreshReadouts`).
- `Converters\Converters.xaml` — every page converter key, merged in `App.xaml`. Chip and tint
  brushes come from `ChipBrushes.CreateTint` / `CreateDot` (`Converters\CellConverters.cs`): shared
  instances that `ChipBrushes.Refresh` recolours in place when a Contrast theme is switched.

The pages are code-behind on purpose: a page demonstrates TableView API calls, so the sample has no
view models. Do not read the code-behind structure as app architecture guidance.

Element AutomationIds are a contract with the UIA verification scripts. WinUI reports an unset
AutomationId as the element's `x:Name`, so do not add an `x:Name` to an element that has an
automation peer unless the AutomationId should change too.

## Snippets

The Source expander snippets (`Snippets\*.txt`) are generated from `<!-- snippet -->` /
`// <snippet>` regions by `tools\Update-Snippets.ps1` (`-Tags Sort,Groups` limits it to some pages)
and checked in. A named region (`// <snippet Groups Sort>`) in a shared or data file is appended to
those pages' snippets; a snippet whose first line is `// snippet:manual` opts out. Keep each region
readable on its own (declare, or take as parameters, the values it uses); narration
(`SetLastAction`, `RefreshReadouts`) is elided.

After editing a region, run `powershell -File tools\Update-Snippets.ps1` and commit the result. The
build's `_CheckTableViewSnippets` target runs it with `-Check` only when a source, snippet or the
tool changed, never in design-time builds; a stale or missing snippet is a warning (`TVSNIP001`)
locally and an error in CI (`TF_BUILD` / `ContinuousIntegrationBuild`, or
`/p:TableViewSampleStrictSnippets=true`).

## Accessibility

Rules distilled from the sample's accessibility test pass.

**Name every element a user can reach, exactly once.** A control that takes focus must report a
non-empty UIA Name. Prefer the name the user can see: `Content`, `Header`, or a child `TextBlock`.
Add `AutomationProperties.Name` only when UIA would otherwise be empty, or when the visible text is
a poor name out of context. Do not add it to a control that already derives a good name from its
content — the result is announced twice. Use `AutomationProperties.LabeledBy` when a separate
`TextBlock` is the visible label, and `AutomationProperties.HelpText` for the supporting sentence
under a control, never as a substitute for the name.

**Group related options into one control, not loose siblings.** A set of mutually exclusive choices
is a single selector — a `ComboBox` (as `ThemeModeCombo` on Settings) or `muxc:RadioButtons` — never
loose `RadioButton`s sharing a `GroupName`, which give each item its own tab stop and no set
position. Where a group of controls is not a selection, give the container an
`AutomationProperties.Name` so the group has context.

**Mark up structure.** A `TextBlock` that reads as a section heading carries
`AutomationProperties.HeadingLevel`; the rail's section headers are `Level2`. Headings are how a
screen reader user skims a page, so a visually-styled heading without one is a real defect.

**Every action reports its outcome.** A button, toggle or selector that changes the page calls
`SetLastAction` with what happened, in words a listener can act on: "Hid the Bio column", "showing
3 of 60 rows", not just the API name. One action, one announcement: don't announce the same
outcome from two handlers. `SetLastAction` re-announces a repeated message, so pressing the same
button twice is not silent.

**Announce only what changed.** In the status panel only Last action is a live region, raised once
per new message. A readout that must be announced on its own sets
`AutomationProperties.LiveSetting="Polite"` **and** raises `AutomationEvents.LiveRegionChanged` on
its peer after updating the text — a `TextBlock` never raises it by itself, so `LiveSetting` alone
is silent (see `StatusPanel.AnnounceLastAction`).
Do not make a whole status panel live: it will re-announce everything on every refresh. Do not use
an `InfoBar` for static guidance either: it announces its title and message whenever it opens, so
an always-open one is read aloud on every navigation. The Try it callout is a styled `Border`.

**Make values mean what they show.** Narrator reads a `Slider` value as a percentage of its range.
When the slider means something else (pixels, milliseconds), say so: put the unit in the `Header`
or `HelpText`, and set `AutomationProperties.ItemStatus` to the current value with its unit, as
`ColumnLayoutPage` does.

**Group rail options with `RailSection`.** It reports a UIA Group named after its visible heading.
Build new option groups from it rather than a bare `StackPanel`, which has no peer and leaves the
group name unannounced. Don't copy the visible description into `FullDescription` or `HelpText`:
it is already readable, and repeating it makes every entry into the group long.

**Survive text scaling and reflow.** Do not put a fixed `Width` or `Height` on anything that renders
text. Use `MinWidth`/`MaxWidth` with `TextWrapping`, and let height be `Auto`; layout must hold at
200% and 400% text scaling and in a portrait window. `SamplePresenter` already drops the options
rail below the example under `WideLayoutMinWidth` (900px) — do not add a fixed-width container that
defeats it. Where text is trimmed, `TextTrimming` must be paired with a `ToolTipService.ToolTip`
and the full string exposed to UIA, so trimming never destroys information. `TableViewTextColumn`
always trims; where a cell value can be long (a path, a description), use a
`TableViewTemplateColumn` with a wrapping `TextBlock` and a `SortMemberPath`, as `TextWrapPage` and
`FilePropertiesPage` do. Keep a `TextBox` placeholder short and put the longer hint in `HelpText`
and a tooltip: placeholders do not wrap.

**Theme by token, not by accent.** Theme dictionaries use only `Light`, `Dark`, `HighContrast`, and
high-contrast entries use `SystemColor*` resources. For anything behind text — row banding,
chips, tints — use neutral layer/card fills such as `CardBackgroundFillColorSecondary`. Accent-derived
fills do not keep 4.5:1 against body text.

**Hide decoration, name meaning.** An image or glyph that carries information gets a name; one that
is purely decorative is removed from the tree with `AutomationProperties.AccessibilityView="Raw"`
so it is not a stop for a screen reader. An icon-only button needs both a `ToolTipService.ToolTip`
and an `AutomationProperties.Name`.

**Never strand focus.** Focus must not land on something that then disappears, and it must not fall
out of the window. After navigation, move focus to the destination page (`FocusFirstElement` in
`MainWindow.xaml.cs`). If you add a control that collapses or replaces content, decide where focus
goes before it is removed.

**Know what is not ours to fix.** Some findings are stock WinUI behaviour and reproduce without
TableView — the `TextBox` clear button is `IsTabStop="False"` and `AccessibilityView="Raw"` by
design, tooltips that Esc will not dismiss, and the "popup window" announcement for `ComboBox` and
`Flyout`, and `NavigationViewItem` labels that trim at 200% text. File those upstream against the
framework. Do not add a sample-local workaround that diverges from platform behaviour just to clear
a bug.

**Fix failures, not findings.** A test-pass bug is a claim, not a requirement. Before changing
anything, check that the behaviour fails the cited WCAG/MAS criterion and that the "expected"
behaviour is right for Windows. Reject the bug instead of changing the sample when it asks for
something no criterion requires (arrow keys between independent buttons, HelpText that repeats a
visible label, an announcement for a control that did nothing, a different glyph on hidden
decoration), when a stock control behaves that way on purpose, or when the fix would add cost for
everyone (an extra Tab stop on every page, a longer announcement). Keyboard access is met when every
control is reachable by Tab and scrolled into view; a `Slider` or selector owning the arrow keys
is correct.

**Do not "fix" the fixtures.** `Pages\KeyboardNavPage.xaml` deliberately contains an unnamed
template button next to an explicitly-named one, so the two can be compared in a UIA inspector, and
Before/After table buttons that exist only as focus sentinels. Leave them as they are. The same
applies to any element whose comment records an intentional asymmetry — read the comment before
changing the markup.

**Check it before you claim it.** Automated scans (Accessibility Insights / Fast Pass) catch a
minority of these; most need Narrator, Tab-only navigation, 400% scaling and a portrait window.
`AutomationProperties.AutomationId` is a contract with the UIA verification scripts: WinUI reports
an unset AutomationId as the element's `x:Name`, so do not add an `x:Name` to an element with an
automation peer unless the AutomationId should change too.
