# RecyclePool / ElementFactory sample app

A small WinUI 3 desktop app that exercises every member of `ElementFactory`, `RecyclePool`,
`RecyclingElementFactory` and `SelectTemplateEventArgs`, plus the three already-public types
they are built on (`IElementFactory`, `ElementFactoryGetArgs`, `ElementFactoryRecycleArgs`).
It backs the screenshots in
[`specs/RecyclePool/RecyclePool-spec.md`](../../specs/RecyclePool/RecyclePool-spec.md).

Each page shows the elements on the left and a live **Factory state** panel on the right
printing elements created, elements recycled in, pool hits, pool misses and `Core` override
hits. The counters are recorded at the call site rather than inferred, so what the panel prints
is what actually happened.

| Page | What it demonstrates |
| --- | --- |
| Recycle pool basics | `PutElement` (both overloads), `TryGetElement` (both overloads), key partitioning, owner affinity, live pool contents |
| RecyclingElementFactory | `Templates`, `RecyclePool`, `SelectTemplateKey`, `GetElementCore` / `RecycleElementCore` overrides, created-vs-recycled counters while scrolling |
| Custom ElementFactory | Deriving from `ElementFactory`: `GetElementCore`, `RecycleElementCore`, and a factory that owns its own pool |
| SelectTemplateEventArgs | `TemplateKey`, `DataContext` and `Owner` inspected live in the handler, and the shared-instance behaviour |
| Shared pool | `GetPoolInstance` / `SetPoolInstance` / `PoolInstanceProperty`, two repeaters over one pool |
| XAML markup | `RecyclingElementFactory` declared entirely in `Page.Resources` via `[contentproperty("Templates")]` |

Pages 1-4 subclass `RecyclePool`, `RecyclingElementFactory` or `ElementFactory` in order to
reach the `overridable` members, which is how the spec's two dispatch findings were measured:
`RecyclePool`'s `Core` overrides and `RecyclingElementFactory.OnSelectTemplateKeyCore` are
never reached, while `ElementFactory`'s are.

## Building and running

From an initialized repo (`init.cmd x64chk`):

```
msbuild Samples\RecyclePoolSampleApp\RecyclePoolSampleApp\RecyclePoolSampleApp.csproj ^
    /restore /t:Publish /p:PublishProfile=win-x64.pubxml
```

The app is also built by `buildsamples.cmd`, and can be built against a published Windows App SDK
package with `scripts\buildSample RecyclePoolSampleApp <version>`.

### From Visual Studio

Open `RecyclePoolSampleApp.sln`. Because this is a packaged single-project MSIX app, Visual
Studio requires `Properties\launchSettings.json`; it defines two profiles:

| Profile | `commandName` | Notes |
| --- | --- | --- |
| `RecyclePoolSampleApp (Package)` | `MsixPackage` | Deploys and debugs the MSIX. Use this one by default. |
| `RecyclePoolSampleApp (Unpackaged)` | `Project` | Launches the loose `.exe` directly. Works because the app runs self-contained. |

Pick a profile from the Start button dropdown, and make sure the solution platform is `x64`
(or `ARM64`) rather than `Any CPU`.

## Regenerating the spec screenshots

The app can put itself into a named state and render itself to a PNG, so the documentation
screenshots are reproducible and do not depend on an interactive desktop:

```
RecyclePoolSampleApp.exe Factory:warm out=C:\path\factory-warm.png
```

The first argument is `<PageTag>:<scenario>`; `out=` is optional and makes the app render the
page content with `RenderTargetBitmap`, save it, and exit. The scenarios used by the spec are:

| Screenshot | Argument |
| --- | --- |
| `pool-keys.png` | `Pool:keys` |
| `pool-owner-affinity.png` | `Pool:affinity` |
| `factory-cold.png` | `Factory:cold` |
| `factory-warm.png` | `Factory:warm` |
| `custom-factory.png` | `Custom:warm` |
| `selecttemplateargs.png` | `Args:reuse` |
| `shared-pool.png` | `Shared:shared` |
| `markup-factory.png` | `Markup:scrolled` |

Scenarios that need a scrolled list drive the `ScrollViewer` through
`Common/ScrollHelper.cs`, which retries until the requested offset is reached. A single
`ChangeView` is not enough: the scenario is applied before the page has been laid out, and an
`ItemsRepeater`'s extent only grows as items are realized.
