# TableView Context Menu Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add column cell/header and row context flyouts with consistent input routing, native fallback, and an application override event.

**Architecture:** Row and header-host handlers resolve targets and forward them to `TableView::ProcessContextMenuRequest`. One pipeline in `TableView_ContextMenu.cpp` owns focus/current-cell coordination, precedence, override notification, revalidation, and display. Internal target/result values describe one synchronous request; there is no controller object, per-cell subscription, or flyout stamping.

**Tech Stack:** C++/WinRT, MIDL3, existing DP/event code generation, WIL scope guards, MSBuild through `initrun.ps1`, and the existing C# TableView sample.

**Spec:** `docs\superpowers\specs\2026-09-25-tableview-context-menu-class-design.md` (approved), plus `docs\superpowers\specs\2026-09-23-tableview-context-menu-design.md` for background.

**Revision:** 2026-09-25. This replaces the earlier distributed-handler implementation plan in place. The approved class design takes precedence where the older behavior spec differs: null override permits native fallback; stale custom targets are suppressed rather than retargeted.

**Execution status:** Approved for subagent-driven execution on 2026-09-25 in the existing `user/dipesh/context-menu` checkout. Preflight and baseline setup are in progress.

## Global Constraints

- Public additions retain `[MUX_PREVIEW]` and `[webhosthidden]`, matching `TableView.idl`.
- Never hand-edit `controls\dev\Generated\`; regenerate DP/event accessors from IDL.
- Register new native source/header files in `TableView.vcxitems`.
- `TableViewColumn` owns the cell/header flyout DPs; `TableView` owns `RowContextFlyout` and the override event.
- Only `ProcessContextMenuRequest` orders menu policy. Entry handlers do not select flyouts, raise the override event, or call `ShowAt`.
- No controller class, public cell container, public current-cell API, built-in commands, or new dependency.
- No per-cell realize/recycle work, menu cache, local cell `DataContext`, or assignment to an element's `ContextFlyout`.
- Use existing wrapper/header `Tag` values and `TableViewCellsPanel::CellForColumn`.
- Never change selection. Body requests move focus; headers do not.
- Body requests while editing fall through unhandled. Header requests are not blocked by body editing.
- Raise the override event once only when a column/row candidate exists.
- `Handled = true` suppresses native fallback. Otherwise, a null override permits native fallback.
- Do not manually show `TableView.ContextFlyout`; native bubbling owns that last rung.
- Suppress stale custom requests after application callbacks; do not restart resolution or raise another event. Explicit null override still permits native fallback.
- Weak event captures, auto-revocation on retemplating, request-local strong references, and a scoped per-TableView reentry guard.
- Propagate unexpected errors. Do not copy the broad catches in unrelated editing code.
- Commands below run from the repository root on Windows. Use the configured snip proxy.
- Preserve unrelated changes and existing untracked design documents. Do not stage the entire worktree.

## Review Focus

1. **Wrong cell from input/coordinates:** a pointer miss must not reuse the keyboard current column; host-coordinate hit-testing must survive offsets, scroll, and frozen columns. Task 2, cases B1-B4.
2. **Null versus suppression:** null falls to the native table menu, `Handled` blocks it, and no candidate raises no event. Tasks 2 and 3, cases B5-B8/H2.
3. **Mutation during callbacks:** removing/hiding a column, swapping items, replacing template parts, or recycling a row must not show a stale menu. Task 4, cases L1-L5.
4. **Native and nested ownership:** direct cell/row/header/editor flyouts keep platform priority; inner tables must not trigger outer custom policy. Tasks 2-4, cases B9/H3/L6.
5. **Duplicate processing/lifetime:** repeated template application, scrolling, and synchronous reentry must not accumulate handlers or double-show; guard restoration must survive exceptions. Task 4, cases L7-L9.

## Files and task boundaries

Paths in this table are relative to the repository root.

| File | Responsibility | Task |
|---|---|---|
| `controls\dev\TableView\TableView.idl` | DPs, event, event-args ABI, accurate API comments | 1 |
| `controls\dev\TableView\TableViewContextFlyoutRequestedEventArgs.h` | Public synchronous decision object | 1 |
| `controls\dev\TableView\TableViewContextMenu.h` | Internal target/result value types | 2 |
| `controls\dev\TableView\TableView.h` | Internal pipeline contracts, header revoker, guard | 2, 3 |
| `controls\dev\TableView\TableView_ContextMenu.cpp` | Shared policy; header resolution/validation | 2, 3 |
| `controls\dev\TableView\TableViewRow.h`, `TableViewRow.cpp` | Body target lookup/validation and thin handler | 2 |
| `controls\dev\TableView\TableView.cpp` | Header-host subscription lifecycle only | 3 |
| `controls\dev\TableView\TableView.vcxitems` | Native file registration | 1, 2 |
| `Samples\TableViewSampleApp\ContextMenuPage.xaml`, `.xaml.cs` | Executable scenarios and visible assertions | 1, 4 |
| `Samples\TableViewSampleApp\MainWindow.xaml`, `.xaml.cs` | Sample navigation | 1 |
| `Samples\TableViewSampleApp\README.md` | Document demonstrated contracts and reproduction steps | 4 |

Task order is 1 -> 2 -> 3 -> 4. Task 2 is one vertical slice because body resolution,
validation, and the shared pipeline require each other to produce testable behavior.
Task 3 adds the second input route without duplicating that policy.

## Validation and build commands

No TableView test project was found by filename search or by scanning `test` for
`TableView`/`Tabular` when this plan was revised. Do not invent a TAEF selector or
claim automated coverage. Use the existing sample with visible event/open counts
and explicit manual scenarios. If execution discovers an existing suitable runner,
port these same cases to it before implementing their corresponding behavior.

Read `Samples\TableViewSampleApp\AGENTS.md` before changing that sample. It consumes
a locally packed component; do not restore the old hand-wired projection, resource,
DLL-copying, or activation workarounds.

**N - targeted native build**, after each native task:

```powershell
C:\configs\tools\snip.exe -- powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\initrun.ps1 msb /q /restore controls\dev\dll-tabular\Microsoft.UI.Xaml.Controls.Tabular.vcxproj /p:Platform=x64
```

**P - product package prerequisites**, once when the full product outputs are
missing or required merged metadata/resources are stale:

```powershell
C:\configs\tools\snip.exe -- .\Build.cmd product /i amd64chk /q
```

**S - pack and build only this sample**, in one PowerShell session:

```powershell
C:\configs\tools\snip.exe -- powershell.exe -NoProfile -Command '$version = "3.0.0-ctxmenu" + (Get-Date -Format "yyyyMMddHHmmssfff"); & powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\initrun.ps1 .\pack.component.cmd /version $version; if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }; & powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\initrun.ps1 msb /q /restore Samples\TableViewSampleApp\TableViewSampleApp.csproj /p:Platform=x64 /p:WinUIVersion=$version; exit $LASTEXITCODE'
```

Use a fresh package version after every native change; never clear another user's
package cache. Command S intentionally restores because it changes the consumed
local package version. Command N uses the existing restore-enabled project build.

**Run:** launch `BuildOutput\obj\amd64chk\Samples\TableViewSampleApp\TableViewSampleApp.exe`
from Explorer or Visual Studio. Navigate to **Context menus**. Record each case as
pass/fail with observed request count, opened menu, and item. A build alone does not
prove routed input, binding, focus, or accessibility behavior.

Record execution evidence in the session workspace, not in additional repository
planning files. Mark unavailable physical-input/UIA scenarios as unrun, not passed.

## Preflight: settle platform assumptions before wiring handlers

**Read-only files:**
`dxaml\xcp\core\dll\eventmgr.cpp`,
`dxaml\xcp\core\core\elements\uielement.cpp`,
`dxaml\xcp\dxaml\lib\Control_Partial.cpp`,
`dxaml\xcp\components\ContentRoot\ContextMenuProcessor.cpp`,
`dxaml\xcp\dxaml\lib\ContextRequestedEventArgs_Partial.cpp`,
`dxaml\xcp\core\inc\ContextRequestedEventArgs.h`.

- [ ] Trace `CEventManager::Raise` through base dispatch, subscribed handlers, and the handled gate. Resolve the original spec's `CUIElement::OnContextRequested` versus `Control::OnContextRequestedImpl` double-show question using the actual call route. Record symbols/lines and the conclusion.
- [ ] Confirm keyboard/gamepad requests have no position. Confirm `TryGetPosition(nullptr, point)` uses host coordinates and `TryGetPosition(anchor, point)` returns anchor-relative coordinates. The projected implementation accepts a null relative element; follow its core implementation for the coordinate conversion.
- [ ] Read `TableViewRow::ResolvePressedColumn`, `TableView::SetCurrentCell`, `TableViewCellsPanel::CellForColumn`, and header construction in `RebuildHeaders`. Keep their existing selection/editing behavior unchanged.
- [ ] Run N to establish the native baseline. Resolve unrelated failures separately; do not conceal them as feature failures.

If native ordering contradicts the approved design, stop and report the specific
conflict before changing routing. Do not replace the design with per-cell stamping.

---

### Task 1: Public contract and executable sample fixture

**Files:** `controls\dev\TableView\TableView.idl`,
`TableViewContextFlyoutRequestedEventArgs.h`, `TableView.vcxitems`;
`Samples\TableViewSampleApp\ContextMenuPage.xaml`, `ContextMenuPage.xaml.cs`,
`MainWindow.xaml`, `MainWindow.xaml.cs`.

**Interfaces:**
- Consumes existing `SampleColumns.Text(string, string, GridLength)`,
  `SampleColumns.Pixels(double)`, `Data.Make(int n = 150)`, and `Item`.
- Produces `TableViewColumn.CellContextFlyout` / `HeaderContextFlyout`,
  `TableView.RowContextFlyout`, their DP accessors, and
  `TableView.ContextFlyoutRequested`.
- Produces `TableViewContextFlyoutRequestedEventArgs(item, column, isHeader, flyout)`
  as an internal C++ constructor; public properties match the approved design.
- Produces a sample fixture reused by all later task gates.

- [ ] **Step 1: Add the fixture below before changing native API**

Create the XAML page:

```xml
<Page
    x:Class="TableViewSampleApp.ContextMenuPage"
    xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
    xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
    xmlns:tv="using:Microsoft.UI.Xaml.Controls.Tabular">
    <Page.Resources>
        <DataTemplate x:Key="AppFlyoutCell">
            <TextBlock Text="{Binding Notes}" Margin="8,0">
                <TextBlock.ContextFlyout>
                    <MenuFlyout>
                        <MenuFlyoutItem Text="APP-OWNED"/>
                    </MenuFlyout>
                </TextBlock.ContextFlyout>
            </TextBlock>
        </DataTemplate>
    </Page.Resources>
    <Grid Padding="12" RowSpacing="8">
        <Grid.RowDefinitions>
            <RowDefinition Height="Auto"/>
            <RowDefinition Height="Auto"/>
            <RowDefinition Height="*"/>
            <RowDefinition Height="Auto"/>
        </Grid.RowDefinitions>
        <TextBlock Text="Context menus" FontSize="18"/>
        <StackPanel Grid.Row="1" Orientation="Horizontal" Spacing="8">
            <ComboBox x:Name="Mode" SelectedIndex="0">
                <ComboBoxItem Content="Keep"/>
                <ComboBoxItem Content="Replace"/>
                <ComboBoxItem Content="Null"/>
                <ComboBoxItem Content="Suppress"/>
                <ComboBoxItem Content="Remove column"/>
                <ComboBoxItem Content="Hide column"/>
                <ComboBoxItem Content="Swap items"/>
                <ComboBoxItem Content="Swap items + null"/>
            </ComboBox>
            <CheckBox x:Name="NoSelection" Content="SelectionMode.None"
                      Click="OnSelectionMode"/>
            <Button Content="Clear row menu" Click="OnClearRow"/>
            <Button Content="Reset" Click="OnReset"/>
        </StackPanel>
        <tv:TableView x:Name="Table" Grid.Row="2" IsReadOnly="False"/>
        <TextBlock x:Name="Status" Grid.Row="3" TextWrapping="Wrap"/>
    </Grid>
</Page>
```

Create the code-behind. These captions are diagnostics, not built-in commands:

```csharp
using System;
using System.Collections.Generic;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Controls.Tabular;
using Microsoft.UI.Xaml.Data;

namespace TableViewSampleApp;

public sealed partial class ContextMenuPage : Page
{
    private readonly TableViewTextColumn _name =
        SampleColumns.Text("Name", nameof(Item.Name), SampleColumns.Pixels(140));
    private readonly TableViewTextColumn _city =
        SampleColumns.Text("City", nameof(Item.City), SampleColumns.Pixels(120));
    private readonly TableViewTextColumn _score =
        SampleColumns.Text("Score", nameof(Item.Score), SampleColumns.Pixels(70));
    private readonly TableViewTemplateColumn _notes = new();
    private readonly List<Item> _items = Data.Make(300);
    private int _requests;
    private int _opens;
    private object? _requestedItem;
    private string _lastRequest = "none";
    private string _lastOpen = "none";

    public ContextMenuPage()
    {
        InitializeComponent();
        _name.Binding = new Binding
        {
            Path = new PropertyPath(nameof(Item.Name)),
            Mode = BindingMode.TwoWay,
        };
        _notes.Header = "Notes";
        _notes.Width = SampleColumns.Pixels(220);
        _notes.CellTemplate = (DataTemplate)Resources["AppFlyoutCell"];
        Table.ContextFlyoutRequested += OnRequest;
        Table.SelectionChanged += (_, _) => Report();
        Reset();
    }

    private MenuFlyout Menu(string label, bool bindItem = false)
    {
        var menu = new MenuFlyout();
        var presenterStyle = new Style { TargetType = typeof(MenuFlyoutPresenter) };
        presenterStyle.Setters.Add(new Setter(AutomationProperties.NameProperty, label));
        menu.MenuFlyoutPresenterStyle = presenterStyle;
        var entry = new MenuFlyoutItem { Text = label };
        AutomationProperties.SetName(entry, label);
        menu.Items.Add(entry);
        if (bindItem)
        {
            var bound = new MenuFlyoutItem();
            bound.SetBinding(MenuFlyoutItem.TextProperty,
                new Binding { Path = new PropertyPath(nameof(Item.Name)) });
            menu.Items.Add(bound);
            menu.Opened += (_, _) =>
            {
                bool matches = ReferenceEquals(bound.DataContext, _requestedItem);
                _lastOpen = $"{label}; binding={(matches ? "PASS" : "FAIL")}; text={bound.Text}";
            };
        }
        menu.Opened += (_, _) =>
        {
            ++_opens;
            if (!bindItem) _lastOpen = label;
            Report();
        };
        return menu;
    }

    private void Reset()
    {
        Table.CancelEdit();
        Table.Columns.Clear();
        foreach (var column in new TableViewColumn[] { _name, _city, _score, _notes })
        {
            column.Visibility = Visibility.Visible;
            Table.Columns.Add(column);
        }
        Table.RowContextFlyout = null;
        _name.CellContextFlyout = null;
        _name.HeaderContextFlyout = null;
        _city.HeaderContextFlyout = null;
        _requests = _opens = 0;
        _requestedItem = null;
        _lastRequest = _lastOpen = "none";
        _name.CellContextFlyout = Menu("CELL", true);
        _name.HeaderContextFlyout = Menu("HEADER");
        _city.HeaderContextFlyout = Menu("CITY HEADER");
        Table.RowContextFlyout = Menu("ROW", true);
        Table.ContextFlyout = Menu("NATIVE");
        Table.ItemsSource = _items;
        Mode.SelectedIndex = 0;
        Report();
    }

    private void OnRequest(TableView sender, TableViewContextFlyoutRequestedEventArgs args)
    {
        ++_requests;
        _requestedItem = args.Item;
        _lastRequest = $"{(args.IsHeader ? "header" : "body")}; " +
            $"column={args.Column?.Header ?? "(none)"}; item={(args.Item as Item)?.Name ?? "(none)"}";
        switch (Mode.SelectedIndex)
        {
            case 1: args.ContextFlyout = Menu("REPLACEMENT"); break;
            case 2: args.ContextFlyout = null; break;
            case 3: args.Handled = true; break;
            case 4:
                if (args.Column is not null) sender.Columns.Remove(args.Column);
                break;
            case 5:
                if (args.Column is not null) args.Column.Visibility = Visibility.Collapsed;
                break;
            case 6: sender.ItemsSource = Data.Make(300); break;
            case 7:
                sender.ItemsSource = Data.Make(300);
                args.ContextFlyout = null;
                break;
        }
        Report();
    }

    private void Report() =>
        Status.Text = $"requests={_requests}; opens={_opens}; selected={Table.SelectedIndex}; " +
            $"request=[{_lastRequest}]; open=[{_lastOpen}]";

    private void OnSelectionMode(object sender, RoutedEventArgs e) =>
        Table.SelectionMode = NoSelection.IsChecked == true
            ? TableViewSelectionMode.None : TableViewSelectionMode.Single;
    private void OnClearRow(object sender, RoutedEventArgs e)
    {
        Table.RowContextFlyout = null;
        Report();
    }
    private void OnReset(object sender, RoutedEventArgs e) => Reset();
}
```

`SelectionChanged` uses `Microsoft.UI.Xaml.Controls.SelectionChangedEventArgs`,
as declared in the existing TableView IDL; the lambda above infers that type.
The bound menu entry intentionally tests inherited context; do not repair a failed
binding by assigning its `DataContext` in the sample.

Add navigation:

```xml
<controls:NavigationViewItem Content="Context menus" Tag="contextmenus"/>
```

```csharp
"contextmenus" => typeof(ContextMenuPage),
```

The XAML/code-behind are included by the existing SDK-style project.

- [ ] **Step 2: Run S against the baseline native package**

Expected failure: the new flyout properties/event/args type do not exist. Save that
failure as the API red gate. Existing package/setup failures are not this red gate.

- [ ] **Step 3: Add IDL declarations**

Inside `TableViewColumn`, alongside existing properties/static DP declarations:

```idl
Microsoft.UI.Xaml.Controls.Primitives.FlyoutBase CellContextFlyout { get; set; };
Microsoft.UI.Xaml.Controls.Primitives.FlyoutBase HeaderContextFlyout { get; set; };
static Microsoft.UI.Xaml.DependencyProperty CellContextFlyoutProperty { get; };
static Microsoft.UI.Xaml.DependencyProperty HeaderContextFlyoutProperty { get; };
```

Inside `TableView`:

```idl
Microsoft.UI.Xaml.Controls.Primitives.FlyoutBase RowContextFlyout { get; set; };
static Microsoft.UI.Xaml.DependencyProperty RowContextFlyoutProperty { get; };
event Windows.Foundation.TypedEventHandler<MU_XC_NAMESPACE.TableView, MU_XC_NAMESPACE.TableViewContextFlyoutRequestedEventArgs> ContextFlyoutRequested;
```

Next to the existing editing event-args runtimeclasses:

```idl
[MUX_PREVIEW]
[webhosthidden]
runtimeclass TableViewContextFlyoutRequestedEventArgs
{
    Object Item { get; };
    MU_XC_NAMESPACE.TableViewColumn Column { get; };
    Boolean IsHeader { get; };
    Microsoft.UI.Xaml.Controls.Primitives.FlyoutBase ContextFlyout;
    Boolean Handled;
};
```

Do not add property-changed callbacks for the three flyout DPs. Document native
fallback for null and total suppression for `Handled`. Update the existing IDL
comment suggesting a future context-menu feature will require public current-cell
API: this feature intentionally keeps current-cell access internal.

- [ ] **Step 4: Add the event-args implementation**

Use the repository copyright/license header and:

```cpp
#pragma once
#include "TableViewContextFlyoutRequestedEventArgs.g.h"

class TableViewContextFlyoutRequestedEventArgs :
    public winrt::implementation::TableViewContextFlyoutRequestedEventArgsT<TableViewContextFlyoutRequestedEventArgs>
{
public:
    TableViewContextFlyoutRequestedEventArgs(
        winrt::IInspectable const& item,
        winrt::TableViewColumn const& column,
        bool isHeader,
        winrt::FlyoutBase const& flyout)
        : m_item(item), m_column(column), m_isHeader(isHeader), m_flyout(flyout) {}

    winrt::IInspectable Item() { return m_item; }
    winrt::TableViewColumn Column() { return m_column; }
    bool IsHeader() { return m_isHeader; }
    winrt::FlyoutBase ContextFlyout() { return m_flyout; }
    void ContextFlyout(winrt::FlyoutBase const& value) { m_flyout = value; }
    bool Handled() { return m_handled; }
    void Handled(bool value) { m_handled = value; }

private:
    winrt::IInspectable m_item{ nullptr };
    winrt::TableViewColumn m_column{ nullptr };
    bool m_isHeader{ false };
    winrt::FlyoutBase m_flyout{ nullptr };
    bool m_handled{ false };
};
```

Register it:

```xml
<ClInclude Include="$(MSBuildThisFileDirectory)TableViewContextFlyoutRequestedEventArgs.h" />
```

- [ ] **Step 5: Run N, then S; exercise API defaults**

Before configuring the sample's columns, temporarily assert defaults:

```csharp
System.Diagnostics.Debug.Assert(_name.CellContextFlyout is null);
System.Diagnostics.Debug.Assert(_name.HeaderContextFlyout is null);
System.Diagnostics.Debug.Assert(Table.RowContextFlyout is null);
System.Diagnostics.Debug.Assert(TableViewColumn.CellContextFlyoutProperty is not null);
System.Diagnostics.Debug.Assert(TableViewColumn.HeaderContextFlyoutProperty is not null);
System.Diagnostics.Debug.Assert(TableView.RowContextFlyoutProperty is not null);
```

Expected: sample compiles and assertions pass. Menu routing is still red: a Name
cell reaches `NATIVE`, never `CELL`, with zero custom requests. Save this behavior
baseline for Task 2. The direct Notes flyout already shows `APP-OWNED`.

- [ ] **Step 6: Commit only Task 1 implementation/sample files after its gate**

Suggested subject: `feat(TableView): add context flyout API`
Use the commit convention at the end of this plan. Do not stage design documents.

---

### Task 2: Body targets and the shared pipeline

**Files:** create `controls\dev\TableView\TableViewContextMenu.h`,
`TableView_ContextMenu.cpp`; modify `TableView.h`, `TableViewRow.h`,
`TableViewRow.cpp`, `TableView.vcxitems`.

**Interfaces:**
- Consumes Task 1 public properties/event args.
- Consumes existing `ResolvePressedColumn(originalSource, hostPoint)`,
  `CellForColumn(Panel const&, TableViewColumn const&) -> FrameworkElement`,
  `UnwrapEditingDataItem(IInspectable const&) const -> IInspectable`,
  `SetCurrentCell(item, column)`, `CurrentColumn()`, and `IsEditing()`.
- Produces all target/result types and pipeline methods below.
- Produces `TableViewRow::IsContextMenuTargetCurrent(target) const` for owner-driven
  body validation; cell-panel internals remain in the row.

- [ ] **Step 1: Run body cases B1, B5, B7 before implementation**

Use Task 1's fixture. Expected failures: no `CELL`/`ROW` candidate and no custom
override callback. Do not treat the baseline native menu as a pass.

- [ ] **Step 2: Create internal value types and declarations**

`TableViewContextMenu.h` contains the copyright/license header followed by:

```cpp
#pragma once
#include "pch.h"

namespace TableViewDetails
{
    enum class ContextMenuTargetKind { Body, Header };
    struct ContextMenuTarget
    {
        ContextMenuTargetKind Kind{ ContextMenuTargetKind::Body };
        winrt::TableViewRow Row{ nullptr };
        winrt::IInspectable Item{ nullptr };
        winrt::TableViewColumn Column{ nullptr };
        winrt::FrameworkElement Anchor{ nullptr };
        winrt::Panel ScopeRoot{ nullptr };
    };
    enum class ContextMenuResult { Unhandled, Suppressed, Shown };
}
```

Include it from both class headers and include `<optional>` where needed.
On `TableView`, add a public **C++-internal**, non-IDL method:

```cpp
TableViewDetails::ContextMenuResult ProcessContextMenuRequest(
    TableViewDetails::ContextMenuTarget target,
    winrt::ContextRequestedEventArgs const& args);
```

Private `TableView` declarations:

```cpp
bool IsContextMenuTargetCurrent(TableViewDetails::ContextMenuTarget const& target) const;
winrt::FlyoutBase ResolveContextFlyout(TableViewDetails::ContextMenuTarget const& target);
winrt::TableViewContextFlyoutRequestedEventArgs RaiseContextFlyoutRequested(
    TableViewDetails::ContextMenuTarget const& target,
    winrt::FlyoutBase const& resolvedFlyout);
void ShowContextFlyout(
    TableViewDetails::ContextMenuTarget const& target,
    winrt::FlyoutBase const& flyout,
    winrt::ContextRequestedEventArgs const& args);
bool m_isProcessingContextMenu{ false };
```

On `TableViewRow`, add public C++-internal validation and private routing:

```cpp
// Public C++-internal:
bool IsContextMenuTargetCurrent(TableViewDetails::ContextMenuTarget const& target) const;

// Private:
void OnContextRequested(winrt::ContextRequestedEventArgs const& args);
std::optional<TableViewDetails::ContextMenuTarget> ResolveContextMenuTarget(
    winrt::ContextRequestedEventArgs const& args);
winrt::UIElement::ContextRequested_revoker m_contextRequestedRevoker{};
```

- [ ] **Step 3: Implement bounded body target resolution**

In `TableViewRow.cpp`, reuse existing includes for TableView, column and cells panel.
The route walk rejects an inner TableView or another row before accepting this row.
It must not skip a foreign tag and continue to an outer cell.

```cpp
std::optional<TableViewDetails::ContextMenuTarget> TableViewRow::ResolveContextMenuTarget(
    winrt::ContextRequestedEventArgs const& args)
{
    auto const owner = GetOwningTableView();
    auto const host = m_cellsHost.get();
    if (!owner || !host) return std::nullopt;

    auto node = args.OriginalSource().try_as<winrt::DependencyObject>();
    bool reachedRow = false;
    while (node)
    {
        if (node == *this) { reachedRow = true; break; }
        if (node.try_as<winrt::TableView>() || node.try_as<winrt::TableViewRow>())
            return std::nullopt;
        if (auto fe = node.try_as<winrt::FrameworkElement>())
        {
            if (auto tagged = fe.Tag().try_as<winrt::TableViewColumn>())
            {
                if (winrt::get_self<TableViewColumn>(tagged)->GetOwningTableView() != owner)
                    return std::nullopt;
            }
        }
        node = winrt::VisualTreeHelper::GetParent(node);
    }
    if (!reachedRow) return std::nullopt;

    auto* ownerImpl = winrt::get_self<TableView>(owner);
    winrt::Point hostPoint{};
    bool const hasPosition = args.TryGetPosition(nullptr, hostPoint);
    auto column = hasPosition
        ? ResolvePressedColumn(args.OriginalSource(), hostPoint)
        : ownerImpl->CurrentColumn();
    uint32_t index{};
    if (column && (column.Visibility() != winrt::Visibility::Visible ||
        winrt::get_self<TableViewColumn>(column)->GetOwningTableView() != owner ||
        !owner.Columns().IndexOf(column, index)))
    {
        column = nullptr;
    }
    auto anchor = column ? TableViewCellsPanel::CellForColumn(host, column) : nullptr;
    if (!anchor) column = nullptr;

    TableViewDetails::ContextMenuTarget target;
    target.Row = *this;
    target.Item = ownerImpl->UnwrapEditingDataItem(DataContext());
    target.Column = column;
    target.Anchor = *this;
    if (anchor) target.Anchor = anchor;
    target.ScopeRoot = host;
    return target;
}
```

Do not use `CurrentColumn()` after a positioned miss. A null column in a valid row
request is the row rung; `std::nullopt` means this custom route is inapplicable.
No arbitrary non-null column may survive without its matching live wrapper.

- [ ] **Step 4: Implement row-owned liveness validation**

Validate item identity against both the container and the repeater's current
projection, not just the old container's `DataContext`.

```cpp
bool TableViewRow::IsContextMenuTargetCurrent(
    TableViewDetails::ContextMenuTarget const& target) const
{
    using TableViewDetails::ContextMenuTargetKind;
    if (target.Kind != ContextMenuTargetKind::Body || !target.Row ||
        winrt::get_self<TableViewRow>(target.Row) != this ||
        !target.Row.IsLoaded() || target.ScopeRoot != m_cellsHost.get() ||
        !target.ScopeRoot || !target.Anchor)
        return false;

    auto const owner = m_owningTableView.get();
    if (!owner) return false;
    auto* ownerImpl = winrt::get_self<TableView>(owner);
    if (ownerImpl->UnwrapEditingDataItem(target.Row.DataContext()) != target.Item)
        return false;
    auto repeater = ownerImpl->GetRowsRepeaterInternal();
    if (!repeater) return false;
    auto const index = repeater.GetElementIndex(target.Row);
    auto view = repeater.ItemsSourceView();
    if (!view || index < 0 || index >= view.Count() ||
        ownerImpl->UnwrapEditingDataItem(view.GetAt(index)) != target.Item)
        return false;

    auto node = target.ScopeRoot.as<winrt::DependencyObject>();
    while (node && node != target.Row)
        node = winrt::VisualTreeHelper::GetParent(node);
    if (!node) return false;
    if (!target.Column) return target.Anchor == target.Row;
    return target.Column.Visibility() == winrt::Visibility::Visible &&
        target.Anchor.Visibility() == winrt::Visibility::Visible &&
        TableViewCellsPanel::CellForColumn(target.ScopeRoot, target.Column) == target.Anchor &&
        winrt::VisualTreeHelper::GetParent(target.Anchor) == target.ScopeRoot;
}
```

- [ ] **Step 5: Implement the owner validation and flyout policy**

Start `TableView_ContextMenu.cpp` with the copyright/license header and:

```cpp
#include "pch.h"
#include "common.h"
#include "TableView.h"
#include "TableViewRow.h"
#include "TableViewColumn.h"
#include "TableViewCellsPanel.h"
#include "TableViewContextFlyoutRequestedEventArgs.h"
```

Owner validation checks shared column policy once and delegates body visual
validation to the row. Header validation is implemented now so Task 3 can reuse
the complete pipeline without changing its policy.

```cpp
bool TableView::IsContextMenuTargetCurrent(
    TableViewDetails::ContextMenuTarget const& target) const
{
    using TableViewDetails::ContextMenuTargetKind;
    if (!target.Anchor || !target.ScopeRoot || !target.Anchor.IsLoaded()) return false;
    auto node = target.ScopeRoot.as<winrt::DependencyObject>();
    winrt::TableView owner{ nullptr };
    while (node && !owner)
    {
        owner = node.try_as<winrt::TableView>();
        if (!owner) node = winrt::VisualTreeHelper::GetParent(node);
    }
    if (!owner || winrt::get_self<TableView>(owner) != this) return false;
    if (target.Column)
    {
        uint32_t index{};
        auto columns = owner.Columns();
        if (!columns || !columns.IndexOf(target.Column, index) ||
            target.Column.Visibility() != winrt::Visibility::Visible ||
            winrt::get_self<TableViewColumn>(target.Column)->GetOwningTableView() != owner)
            return false;
    }
    if (target.Kind == ContextMenuTargetKind::Body)
    {
        if (!target.Row) return false;
        auto* rowImpl = winrt::get_self<TableViewRow>(target.Row);
        return rowImpl->GetOwningTableView() == owner &&
            rowImpl->IsContextMenuTargetCurrent(target);
    }
    if (target.Row || target.Item || !target.Column ||
        target.ScopeRoot != m_headerHost.get() ||
        owner.HeadersVisibility() != winrt::TableViewHeadersVisibility::Column)
        return false;
    return target.Anchor.Visibility() == winrt::Visibility::Visible &&
        winrt::VisualTreeHelper::GetParent(target.Anchor) == target.ScopeRoot &&
        TableViewCellsPanel::CellForColumn(target.ScopeRoot, target.Column) == target.Anchor;
}

winrt::FlyoutBase TableView::ResolveContextFlyout(
    TableViewDetails::ContextMenuTarget const& target)
{
    if (target.Kind == TableViewDetails::ContextMenuTargetKind::Header)
        return target.Column.HeaderContextFlyout();
    if (target.Column)
    {
        if (auto flyout = target.Column.CellContextFlyout()) return flyout;
    }
    return RowContextFlyout();
}

winrt::TableViewContextFlyoutRequestedEventArgs TableView::RaiseContextFlyoutRequested(
    TableViewDetails::ContextMenuTarget const& target,
    winrt::FlyoutBase const& resolvedFlyout)
{
    auto result = winrt::make<TableViewContextFlyoutRequestedEventArgs>(
        target.Item, target.Column,
        target.Kind == TableViewDetails::ContextMenuTargetKind::Header, resolvedFlyout);
    if (m_contextFlyoutRequestedEventSource)
        m_contextFlyoutRequestedEventSource(*this, result);
    return result;
}

void TableView::ShowContextFlyout(
    TableViewDetails::ContextMenuTarget const& target,
    winrt::FlyoutBase const& flyout,
    winrt::ContextRequestedEventArgs const& args)
{
    winrt::FlyoutShowOptions options;
    options.Placement(winrt::FlyoutPlacementMode::Auto);
    winrt::Point point{};
    if (args.TryGetPosition(target.Anchor, point)) options.Position(point);
    flyout.ShowAt(target.Anchor, options);
}
```

`ShowContextFlyout` requires a validated non-null target/flyout. Do not add silent
null-return branches that let its caller report `Shown` without actually showing.

- [ ] **Step 6: Implement the single pipeline**

The input-source snapshot catches an `ItemsSource` replacement even when row
recycling has not yet run. It is request-local, not another member or public field.
Capture body custom participation before current-cell/focus callbacks. If they
invalidate a native-only request, preserve `Unhandled`; configured custom requests
remain suppressed. Still-valid requests resolve the candidate afresh after focus.
Candidate presence is independent of whether the realized data item is null.

```cpp
TableViewDetails::ContextMenuResult TableView::ProcessContextMenuRequest(
    TableViewDetails::ContextMenuTarget target,
    winrt::ContextRequestedEventArgs const& args)
{
    using TableViewDetails::ContextMenuResult;
    using TableViewDetails::ContextMenuTargetKind;
    if (args.Handled()) return ContextMenuResult::Unhandled;
    if (m_isProcessingContextMenu) return ContextMenuResult::Suppressed;
    bool const body = target.Kind == ContextMenuTargetKind::Body;
    if (body && IsEditing()) return ContextMenuResult::Unhandled;
    if (!IsContextMenuTargetCurrent(target)) return ContextMenuResult::Suppressed;

    auto const lifetime = get_strong();
    m_isProcessingContextMenu = true;
    auto restore = wil::scope_exit([this]() noexcept { m_isProcessingContextMenu = false; });
    auto const sourceSnapshot = ItemsSource();
    auto const requestIsCurrent = [&]()
    {
        return IsContextMenuTargetCurrent(target) &&
            (!body || (ItemsSource() == sourceSnapshot && !IsEditing()));
    };
    if (body)
    {
        auto const invalidatedResult = ResolveContextFlyout(target)
            ? ContextMenuResult::Suppressed
            : ContextMenuResult::Unhandled;
        if (target.Column) SetCurrentCell(target.Item, target.Column);
        if (!requestIsCurrent()) return invalidatedResult;
        target.Row.Focus(winrt::FocusState::Programmatic);
        if (!requestIsCurrent()) return invalidatedResult;
    }
    auto flyout = ResolveContextFlyout(target);
    if (!flyout) return ContextMenuResult::Unhandled;
    auto decision = RaiseContextFlyoutRequested(target, flyout);
    if (decision.Handled()) return ContextMenuResult::Suppressed;
    flyout = decision.ContextFlyout();
    if (!flyout) return ContextMenuResult::Unhandled;
    if (!requestIsCurrent()) return ContextMenuResult::Suppressed;
    ShowContextFlyout(target, flyout, args);
    return ContextMenuResult::Shown;
}
```

No callback catch, no resolver restart, and no manual table fallback. The boolean
return from `Focus` is not a display outcome; state and attachment are revalidated
regardless. Keep failures visible during interaction validation.

- [ ] **Step 7: Wire the thin row handler**

```cpp
void TableViewRow::OnContextRequested(winrt::ContextRequestedEventArgs const& args)
{
    if (args.Handled()) return;
    auto owner = GetOwningTableView();
    if (!owner) return;
    auto target = ResolveContextMenuTarget(args);
    if (!target) return;
    auto result = winrt::get_self<TableView>(owner)->ProcessContextMenuRequest(*target, args);
    if (result != TableViewDetails::ContextMenuResult::Unhandled) args.Handled(true);
}
```

At the start of `TableViewRow::OnApplyTemplate`, revoke the prior registration;
after obtaining the new cells host, install one weak-capture subscription:

```cpp
m_contextRequestedRevoker.revoke();
```

```cpp
m_contextRequestedRevoker = ContextRequested(winrt::auto_revoke,
    [weakThis = get_weak()](winrt::IInspectable const&, winrt::ContextRequestedEventArgs const& args)
    {
        if (auto self = weakThis.get()) self->OnContextRequested(args);
    });
```

Do not alter `RebuildCells`, `ReleaseCellToolTips`, or the existing primary-button
guards for selection/editing. Repeated `OnApplyTemplate` calls must replace, not
append, the subscription.

- [ ] **Step 8: Register files and build**

```xml
<ClInclude Include="$(MSBuildThisFileDirectory)TableViewContextMenu.h" />
<ClCompile Include="$(MSBuildThisFileDirectory)TableView_ContextMenu.cpp" />
```

Run N, then S. Resolve compilation against actual projected types without adding
public API or weakening the target checks.

- [ ] **Step 9: Run the body regression matrix**

Reset before each case. Compare count deltas, not absolute counts.

| Case | Action | Expected |
|---|---|---|
| B1 | Right-click Name; dismiss; Shift+F10; Apps key | `CELL` each time; one request/open each; same item and column |
| B2 | Right-click a different row from the selected row; repeat with SelectionMode.None | Focus/current cell move; selection does not change; request names the clicked item |
| B3 | Set current Name, then right-click row space after the last cell | `ROW`, null column; never `CELL`; with row menu cleared, `NATIVE` and zero requests |
| B4 | Scroll horizontally/vertically with Name frozen and with the table offset by page padding; click a visible cell | Correct column and pointer position, not a host/row coordinate mix |
| B5 | Right-click City; clear row menu; repeat | First `ROW` with one request; then `NATIVE` with zero requests |
| B6 | Mode Replace on Name | `REPLACEMENT`; one request/open |
| B7 | Mode Null on Name | `NATIVE`; one custom request, one native open |
| B8 | Mode Suppress on Name | One request, zero opens, including zero native opens |
| B9 | Right-click Notes text; then begin Name editing and right-click TextBox | App/editor menu wins; custom request count unchanged |
| B10 | Dismiss menu, press F2 after right-clicking a different Name row | Editing begins on the context row/cell, not the previously selected row |
| B11 | Open CELL/ROW on distinct rows, including after virtualization | Bound entry reports correct row text and reference identity; no local DataContext workaround |

For B4, temporarily set `_name.FrozenEdge = TableViewFrozenEdge.Leading`;
reduce the window width until the body scrolls.
For B3, use a wide window so row trailing space is visible. At a breakpoint in
`ResolveContextMenuTarget`, verify that `OriginalSource` belongs to that row and
`hasPosition` is true. A click outside the row or a keyboard request is not a
substitute for this case; record a blocked setup if trailing space is unavailable.

- [ ] **Step 10: Commit the body/pipeline slice after its gates**

Suggested subject: `feat(TableView): centralize context flyout policy`
Stage only this task's native files; keep the reusable sample from Task 1.

---

### Task 3: Header route through the same pipeline

**Files:** `controls\dev\TableView\TableView.h`,
`TableView.cpp`, `TableView_ContextMenu.cpp`.

**Interfaces:**
- Consumes `ContextMenuTarget`, `ContextMenuResult`,
  `ProcessContextMenuRequest(target, args)` and owner validation from Task 2.
- Produces private `OnHeaderContextRequested(args)`,
  `ResolveHeaderContextMenuTarget(args) -> std::optional<ContextMenuTarget>`,
  and `m_headerContextRequestedRevoker`.
- Does not add another notification or display method.

- [ ] **Step 1: Run H1/H2 before wiring the header**

Expected red: header Name/City reaches `NATIVE`, not `HEADER`/`CITY HEADER`.
Body scenarios from Task 2 must remain green.

- [ ] **Step 2: Add declarations and bounded header resolution**

Private additions in `TableView.h`:

```cpp
void OnHeaderContextRequested(winrt::ContextRequestedEventArgs const& args);
std::optional<TableViewDetails::ContextMenuTarget> ResolveHeaderContextMenuTarget(
    winrt::ContextRequestedEventArgs const& args);
winrt::UIElement::ContextRequested_revoker m_headerContextRequestedRevoker{};
```

Implement in `TableView_ContextMenu.cpp`:

```cpp
std::optional<TableViewDetails::ContextMenuTarget> TableView::ResolveHeaderContextMenuTarget(
    winrt::ContextRequestedEventArgs const& args)
{
    auto const host = m_headerHost.get();
    if (!host) return std::nullopt;
    auto node = args.OriginalSource().try_as<winrt::DependencyObject>();
    while (node && node != host)
    {
        if (node.try_as<winrt::TableView>() || node.try_as<winrt::TableViewRow>())
            return std::nullopt;
        auto const parent = winrt::VisualTreeHelper::GetParent(node);
        if (parent == host)
        {
            auto cell = node.try_as<winrt::Grid>();
            if (!cell) return std::nullopt;
            auto column = cell.Tag().try_as<winrt::TableViewColumn>();
            if (!column) return std::nullopt;
            TableViewDetails::ContextMenuTarget target;
            target.Kind = TableViewDetails::ContextMenuTargetKind::Header;
            target.Column = column;
            target.Anchor = cell;
            target.ScopeRoot = host;
            if (!IsContextMenuTargetCurrent(target)) return std::nullopt;
            return target;
        }
        node = parent;
    }
    return std::nullopt;
}

void TableView::OnHeaderContextRequested(winrt::ContextRequestedEventArgs const& args)
{
    if (args.Handled()) return;
    auto target = ResolveHeaderContextMenuTarget(args);
    if (!target) return;
    auto result = ProcessContextMenuRequest(*target, args);
    if (result != TableViewDetails::ContextMenuResult::Unhandled) args.Handled(true);
}
```

A descendant's arbitrary `Tag` is not a header. Only the actual direct header Grid
identified by the current host and `CellForColumn` is accepted. Empty host space
returns no target and remains native fallback.

- [ ] **Step 3: Wire template lifecycle**

In `TableView::OnApplyTemplate`, revoke before replacing the old header host:

```cpp
m_headerContextRequestedRevoker.revoke();
```

Inside the new `if (auto headerHost = m_headerHost.get())` block:

```cpp
m_headerContextRequestedRevoker = headerHost.ContextRequested(winrt::auto_revoke,
    [weakThis](winrt::IInspectable const&, winrt::ContextRequestedEventArgs const& args)
    {
        if (auto self = weakThis.get()) self->OnHeaderContextRequested(args);
    });
```

Do not subscribe at TableView itself: its native `ContextFlyout` would already
have had the opportunity to show. Do not change header tab stops or focus policy.

- [ ] **Step 4: Run N and S, then the header matrix**

| Case | Action | Expected |
|---|---|---|
| H1 | Right-click Name/City headers, then focus each existing resizable header and use Shift+F10/Apps | Correct header menu; one request/open; Item null, IsHeader true |
| H2 | On Name header use Replace, Null, Suppress; invoke Score header with Keep | Replacement, native fallback, suppression; Score native fallback with zero requests |
| H3 | Give the actual header Grid a direct app flyout, then invoke it | Direct menu wins; no custom request |
| H4 | Invoke header while a body editor is open | Header path is not blocked by body editing; pipeline itself does not move focus/current cell |
| H5 | Right-click empty header-host space and group header | Native fallback; no column/row custom event |
| H6 | Hide headers; restore them; invoke again | No stale hidden-header show; one request after restore |
| H7 | Repeat B1, B5, B7, B8 | Body behavior unchanged |

Run H1/H2/H4/H6/H7 and the empty-host portion of H5 for this task's gate. H3 and
the grouped portion of H5 run with Task 4's diagnostic setup; they remain required
before feature completion, not prerequisites for building that later fixture.
Native focus behavior caused by opening/dismissing the flyout is separate from
the pipeline's deliberate no-header-focus rule.

- [ ] **Step 5: Commit the header route after its gates**

Suggested subject: `feat(TableView): route header context requests`
Stage only the three listed native files.

---

### Task 4: Mutation, ownership, lifetime, and accessibility gates

**Files:** modify `Samples\TableViewSampleApp\ContextMenuPage.xaml.cs`,
`ContextMenuPage.xaml`, and `README.md`. Native fixes, if these tests expose them,
belong in their existing owning classes and must preserve the approved interfaces.

**Interfaces:**
- Consumes the completed public feature and the Task 1 fixture/counts.
- Produces reproducible diagnostics and documented results, not a new test framework.
- Debug-only native probe below consumes `ProcessContextMenuRequest(target, args)`
  inside its implementation; it must not become shipping instrumentation.

- [ ] **Step 1: Add diagnostic buttons and visual enumeration**

Add these buttons to the fixture's controls:

```xml
<Button Content="Direct row menu" Click="OnDirectRow"/>
<Button Content="Direct header menu" Click="OnDirectHeader"/>
<Button Content="Retemplate" Click="OnRetemplate"/>
<Button Content="Mutate on next focus" Click="OnMutateOnFocus"/>
```

Add `using System.Linq;` and `using Microsoft.UI.Xaml.Media;`, then:

```csharp
private static IEnumerable<DependencyObject> Descendants(DependencyObject root)
{
    for (int i = 0; i < VisualTreeHelper.GetChildrenCount(root); ++i)
    {
        var child = VisualTreeHelper.GetChild(root, i);
        yield return child;
        foreach (var descendant in Descendants(child)) yield return descendant;
    }
}

private void OnDirectRow(object sender, RoutedEventArgs e)
{
    var row = Descendants(Table).OfType<TableViewRow>().FirstOrDefault();
    if (row is null) { Status.Text = "FAIL: no realized row"; return; }
    row.ContextFlyout = Menu("DIRECT ROW");
    Status.Text = "Invoke the first realized row.";
}

private void OnDirectHeader(object sender, RoutedEventArgs e)
{
    var header = Descendants(Table).OfType<Grid>()
        .FirstOrDefault(grid => ReferenceEquals(grid.Tag, _name));
    if (header is null) { Status.Text = "FAIL: no Name header"; return; }
    header.ContextFlyout = Menu("DIRECT HEADER");
    Status.Text = "Invoke the Name header.";
}

private void OnRetemplate(object sender, RoutedEventArgs e)
{
    var template = Table.Template;
    if (template is null) { Status.Text = "FAIL: template unavailable"; return; }
    Table.Template = null;
    Table.ApplyTemplate();
    Table.Template = template;
    Table.ApplyTemplate();
    Status.Text = "Template replaced; invoke Name again.";
}

private bool _mutateOnFocus;
private void OnMutateOnFocus(object sender, RoutedEventArgs e)
{
    _mutateOnFocus = true;
    Status.Text = "Right-click a different row without left-clicking first.";
}
```

Wire once in the constructor, after `InitializeComponent`:

```csharp
Table.GotFocus += (_, _) =>
{
    if (!_mutateOnFocus) return;
    _mutateOnFocus = false;
    Table.ItemsSource = Data.Make(300);
};
```

After a direct-menu diagnostic, navigate away/back to create clean containers.
Reset does not clear deliberately app-owned flyouts on existing elements.

- [ ] **Step 2: Add a nested-control diagnostic template**

Add this page resource and a button that assigns it to `_notes.CellTemplate`:

```xml
<DataTemplate x:Key="NestedTableCell">
    <tv:TableView Loaded="OnNestedLoaded" Height="100" Width="200"/>
</DataTemplate>
```

```csharp
private void OnNestedLoaded(object sender, RoutedEventArgs e)
{
    var inner = (TableView)sender;
    if (inner.Columns.Count != 0) return;
    var column = SampleColumns.Text("Inner", nameof(Item.Name), SampleColumns.Pixels(120));
    column.CellContextFlyout = Menu("INNER");
    inner.Columns.Add(column);
    inner.ItemsSource = Data.Make(2);
}

private void OnNested(object sender, RoutedEventArgs e) =>
    _notes.CellTemplate = (DataTemplate)Resources["NestedTableCell"];
```

```xml
<Button Content="Nested table" Click="OnNested"/>
```

Keep outer `_requests` instrumentation only on the outer `Table`; an inner menu
open is recorded by the shared menu factory without pretending it is an outer
override notification. Test again after clearing the inner column's flyout:

```csharp
foreach (var inner in Descendants(Table).OfType<TableView>())
    foreach (var column in inner.Columns) column.CellContextFlyout = null;
```

Expose that operation with a diagnostic button or execute it at a debugger
breakpoint. Do not assign inner-table flyouts in production routing code.

- [ ] **Step 3: Run stale-target and ownership cases**

Run S for sample changes. Reset/recreate the page between destructive cases.

| Case | Action | Expected |
|---|---|---|
| L1 | Mode Remove column on Name cell, then Name header in a fresh page | One callback; zero opens; no row/native fallback |
| L2 | Mode Hide column on Name cell/header | One callback; zero opens; no stale anchor |
| L3 | Mode Swap items on body Name, then Swap items + null | First suppresses custom display; second permits `NATIVE` despite item mutation |
| L4 | Mutate on next focus; right-click another row | Zero custom events/opens after invalidation; no menu for the old item |
| L5 | Retemplate from inside the override callback, with Keep | One callback, no stale show; next fresh request works once |
| L6 | Direct row/header and nested template, with and without inner candidate; run H3 here | Native direct priority; inner request never raises outer custom event; native outer fallback remains allowed |
| L7 | Retemplate repeatedly; scroll beyond 150 rows and back; invoke Name | One request/open per action, correct current item and binding |
| L8 | Debug reentry probe below | Nested same-owner request is Suppressed without another event/show |
| L9 | Throw from override callback under debugger | Exception remains visible; no successful show result; guard resets during unwind |

For L5, temporarily add a distinct callback mode using this code, then keep it as
a named diagnostic choice if useful:

```csharp
var template = sender.Template;
if (template is null) throw new InvalidOperationException("Template unavailable");
sender.Template = null;
sender.ApplyTemplate();
sender.Template = template;
sender.ApplyTemplate();
```

For L8, place this temporary debug-only probe immediately after acquiring the
pipeline guard, where `target` and `args` are the actual live request:

```cpp
#if DBG
auto nestedResult = ProcessContextMenuRequest(target, args);
MUX_ASSERT(nestedResult == TableViewDetails::ContextMenuResult::Suppressed);
#endif
```

This must return before notification or display. Remove the probe before committing.
For L9, temporarily throw `new InvalidOperationException("context-menu probe")`
inside the sample's override handler. Break on the exception, inspect unwinding
through the scope guard, and verify the flag resets. An application-level
unhandled exception may terminate the sample; that is not a passing interaction
run. Remove the injection and relaunch for the remaining cases. Do not add a broad
catch to make this negative test appear successful.

- [ ] **Step 4: Complete input, focus, and accessibility coverage**

| Case | Setup/action | Expected |
|---|---|---|
| A1 | Touch-hold Name cell/header | Correct menu, one event/open; no selection change |
| A2 | Gamepad Menu from focused row/header | Same applicable menu as keyboard; no fabricated pointer point |
| A3 | RTL plus horizontal scroll/frozen Name | Correct column and platform-mirrored positioning |
| A4 | Inspect open menu with available UIA tooling/Narrator | Named menu items and presenter semantics; no duplicate popup; item binding is correct |
| A5 | Escape dismiss body menu; inspect focus, then use F2 | Focus returns to the row; current cell remains usable |
| A6 | Retain public args after callback; change flyout/Handled later | No second show or delayed decision |
| A7 | Supply a plain Flyout instead of MenuFlyout | Same pipeline, anchor, null, and suppression behavior |
| A8 | Group the fixture using TableViewSource; invoke group header (grouped H5) | Native table fallback; no row/column custom event |

For A3 set `Table.FlowDirection = FlowDirection.RightToLeft` and
`_name.FrozenEdge = TableViewFrozenEdge.Leading`.
For A7 configure:

```csharp
_name.CellContextFlyout = new Flyout
{
    Content = new TextBlock { Text = "PLAIN FLYOUT" },
};
```

For A8 reuse the sample's existing shaping API:

```csharp
var source = TableViewSource.From(_items);
source.GroupBy(new TableViewKeySelector(item => ((Item)item).City));
Table.ItemsSource = source;
```

This matches `ShapingPage.xaml.cs`: create with `From` and group with a
`TableViewKeySelector`, not a guessed `Source` property or string overload.

- [ ] **Step 5: Document the feature and coverage**

Add a **Context menus** section to the sample README with:

```markdown
The Context menus page demonstrates per-column cell/header flyouts and row
fallback. Column cell menus take precedence over RowContextFlyout; the native
TableView.ContextFlyout is the final fallback. ContextFlyoutRequested can replace
the resolved flyout. Setting it to null permits native fallback; setting Handled
suppresses all further display.

Context requests move body focus without changing selection. Direct element
flyouts keep native priority on their routed path. Menus are not stamped on
realized cells. A target invalidated during an app callback is not retargeted;
custom display is suppressed unless the app explicitly chooses null fallback.

Use Keep/Replace/Null/Suppress and mutation modes to compare the request/open
counts. Recreate the page after direct-element or nested-control diagnostics.
```

Keep exact reproduction steps for B1-B11, H1-H7, L1-L9, and A1-A8 accessible in this
plan. Record actual execution results separately, including unrun hardware cases.
Do not mark a matrix row passed merely because the source contains the expected
branch.

- [ ] **Step 6: Run final targeted gates and commit**

Run N only if native code changed after Task 3; run S for the final sample/package.
Repeat B1/B7/B8/H1/H2/L3/L7 after the last change. Confirm no debug probe or thrown
test exception remains. Suggested subject: `test(TableView): cover context menu lifecycle`.
Stage only the sample/documentation and any explicitly reviewed native fixes.

## Commit convention during execution

These are execution steps, not instructions to commit this plan update. Before
each implementation commit, inspect the diff and stage only that task's listed
files using explicit paths:

```powershell
C:\configs\tools\snip.exe -- git --no-pager diff --check
C:\configs\tools\snip.exe -- git --no-pager diff --stat
```

Use the suggested subject for the task and the session-required trailers:

```text
Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>
Copilot-Session: 75efebd4-6de1-4fe8-b075-fab226b722c6
```

The identifier above is required by the current session. If execution occurs in
another session, use its required session trailer instead.

## Completion criteria and execution handoff

- All four tasks deliver their defined interfaces without a separate policy path.
- Native build, local package consumption, and sample gates are recorded honestly.
- Behavior matches the approved class design, including no-candidate event gating,
  null fallback, stale-target suppression, and guarded synchronous reentry.
- Original selection/editing guards remain unchanged.
- Generated files are produced by the build; no temporary probes or unrelated
  changes are included in implementation commits.
- The platform double-show question has an evidence-backed answer.

Review this revised plan before implementing. Execution can be **native** in the
current session, or **subagent-driven** with per-task implementation/review gates.
Native execution is recommended for these four tightly coupled tasks: the row,
pipeline, and header contracts share context, and the sample gates are interactive.
It ends with one fresh whole-branch reviewer; subagent-driven execution adds fresh
implementation/review contexts per task and a final whole-branch review.
