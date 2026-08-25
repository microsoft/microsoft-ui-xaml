# SelectionModel sample app

A small WinUI 3 desktop app that exercises every public member of `SelectionModel`,
`IndexPath` and the two `SelectionModel*EventArgs` types. It backs the screenshots in
[`specs/SelectionModel/SelectionModel-spec.md`](../../specs/SelectionModel/SelectionModel-spec.md).

Each page shows the data on the left and a live **Model state** panel on the right printing
`SelectedIndex`, `SelectedItem`, `AnchorIndex`, `SingleSelect`, `SelectedIndices` and
`SelectedItems`, so the effect of every call is visible.

| Page | What it demonstrates |
| --- | --- |
| Flat selection | `Select` / `Deselect` / `IsSelected`, `SingleSelect`, `SelectAll`, `SelectAllFlat`, `ClearSelection`, `SelectedIndex`, `Source` |
| Grouped selection | The group/item overloads, the tri-state `IsSelected`, `SelectAll` vs `SelectAllFlat` |
| Range and anchor | `AnchorIndex`, `SetAnchorIndex`, `SelectRangeFromAnchor(To)`, `DeselectRangeFromAnchor(To)`, `SelectRange`, `DeselectRange` |
| IndexPath | `CreateFrom`, `CreateFromIndices`, `GetSize`, `GetAt`, `CompareTo`, `ToString` |
| Events | `SelectionChanged` and `ChildrenRequested` over a lazily resolved tree |
| XAML binding | `SelectionModel` declared as a XAML resource and `{Binding SelectedItem}` |

## Building and running

From an initialized repo (`init.cmd x64chk`):

```
msbuild Samples\SelectionModelSampleApp\SelectionModelSampleApp\SelectionModelSampleApp.csproj ^
    /restore /t:Publish /p:PublishProfile=win-x64.pubxml
```

The app is also built by `buildsamples.cmd`, and can be built against a published Windows App SDK
package with `scripts\buildSample SelectionModelSampleApp <version>`.

## Regenerating the spec screenshots

The app can put itself into a named state and render itself to a PNG, so the documentation
screenshots are reproducible and do not depend on an interactive desktop:

```
SelectionModelSampleApp.exe Grouped:partial out=C:\path\grouped-partial-selection.png
```

The first argument is `<PageTag>:<scenario>`; `out=` is optional and makes the app render the
page content with `RenderTargetBitmap`, save it, and exit. The scenarios used by the spec are:

| Screenshot | Argument |
| --- | --- |
| `flat-multiple-selection.png` | `Flat:multi` |
| `flat-single-select.png` | `Flat:single` |
| `grouped-partial-selection.png` | `Grouped:partial` |
| `grouped-full-selection.png` | `Grouped:full` |
| `range-anchor-selection.png` | `Range:anchor` |
| `indexpath-api.png` | `IndexPath:default` |
| `events-childrenrequested.png` | `Events:lazy` |
| `binding-selecteditem.png` | `Binding:select` |
