Inking (InkCanvas, InkPresenter, InkToolbar)
===

# Background

This API spec introduces `InkCanvas` to WinUI 3, closing a known feature gap with WinUI 2 / UWP. Ink
support has been a commonly requested capability, and today developers must either build substantial
functionality on top of the DirectInk APIs or adopt alternative solutions to provide handwriting,
annotation, markup, and signature capture experiences. Bringing `InkCanvas` to WinUI 3 enables these
scenarios with a first-party, XAML-native solution.

This change introduces `InkCanvas`, `InkToolbar`, and the supporting inking infrastructure to WinUI 3,
enabling developers to create rich inking experiences such as note-taking, annotation, and signature
capture. The API design closely mirrors the existing UWP surface area to ease migration and developer
adoption, with any required deviations clearly documented in this specification.

The types this spec covers, all in `Microsoft.UI.Xaml.Controls` (automation peers in
`Microsoft.UI.Xaml.Automation.Peers`):

- **`InkCanvas`** - a `FrameworkElement` that hosts an ink surface and renders wet and dry strokes.
- **`InkPresenter`** - the object (reached through `InkCanvas.InkPresenter`) that owns all ink
  configuration: input device types, drawing attributes, processing mode, the stroke container, and
  the stroke and erase input events.
- **`InkToolbar`** - a `Control` that auto-populates pen / pencil / highlighter / eraser / stencil
  buttons and drives an attached `InkCanvas`.

<img src="./inking-overview.png" alt="An InkCanvas with an InkToolbar, toolbar expanded to show the tool options" width="480"/>

# Conceptual pages (How To)

## How to add an inking surface

`InkCanvas` hosts the inking surface and handles the rendering of ink strokes. It can optionally be
used with `InkToolbar`, which provides a ready-to-use UI for selecting pens, highlighters, erasers,
and other inking tools.

### Standard usage

Put the **InkToolbar** above the **InkCanvas** and point it at the canvas with `TargetInkCanvas`.

```xaml
<Grid RowDefinitions="Auto,*"
      xmlns:controls="using:Microsoft.UI.Xaml.Controls">
    <controls:InkToolbar Grid.Row="0" TargetInkCanvas="{x:Bind InkSurface}" />
    <controls:InkCanvas x:Name="InkSurface" Grid.Row="1" />
</Grid>
```

By default the presenter accepts **pen input only**, matching UWP. `InputDeviceTypes` is a flags mask
that supports incremental updates, so add or remove a device type without restating the others:

```csharp
// Add mouse input, keeping pen.
InkSurface.InkPresenter.InputDeviceTypes |= Windows.UI.Core.CoreInputDeviceTypes.Mouse;

// Remove touch input, keeping everything else.
InkSurface.InkPresenter.InputDeviceTypes &= ~Windows.UI.Core.CoreInputDeviceTypes.Touch;
```

Drawing with the active pen renders a stroke; selecting the eraser and dragging over a stroke removes
it.

### Configuring the InkPresenter

All ink configuration flows through `InkCanvas.InkPresenter`, exactly as in UWP:

```csharp
var presenter = InkSurface.InkPresenter;

// Add mouse and touch to the default pen-only mask.
presenter.InputDeviceTypes |=
    Windows.UI.Core.CoreInputDeviceTypes.Mouse |
    Windows.UI.Core.CoreInputDeviceTypes.Touch;

// Set the default stroke color and size.
var attributes = presenter.CopyDefaultDrawingAttributes();
attributes.Color = Microsoft.UI.Colors.MediumPurple;
attributes.Size = new Windows.Foundation.Size(6, 6);
presenter.UpdateDefaultDrawingAttributes(attributes);
```

### Handling stroke input events

`InkPresenter` raises `StrokesCollected` when wet ink is committed to dry strokes, and `StrokesErased`
when strokes are removed. For lower-level input, `InkPresenter.StrokeInput` exposes the raw
start / continue / end / cancel of a stroke, and `InkPresenter.UnprocessedInput` exposes pointer
events the presenter did **not** turn into ink (for example when `InputProcessingConfiguration.Mode`
is `None`, used for lasso selection).

```csharp
presenter.StrokesCollected += (s, e) =>
{
    // e.Strokes is the set of strokes just committed.
    Log($"Collected {e.Strokes.Count} stroke(s).");
};

presenter.StrokesErased += (s, e) =>
{
    Log($"Erased {e.Strokes.Count} stroke(s).");
};

// Raw stroke lifecycle.
presenter.StrokeInput.StrokeStarted += (s, args) => { /* ... */ };
presenter.StrokeInput.StrokeEnded   += (s, args) => { /* ... */ };
```

### Using InkCanvas and InkToolbar in XAML, C#, and C++/WinRT

The same `InkCanvas` + `InkToolbar` pairing can be created in XAML or in code. All three snippets below
produce the identical result: a toolbar above a canvas, with the pencil selected as the active tool.

```xaml
<Grid RowDefinitions="Auto,*"
      xmlns:controls="using:Microsoft.UI.Xaml.Controls">
    <controls:InkToolbar x:Name="Toolbar" Grid.Row="0" TargetInkCanvas="{x:Bind InkSurface}" />
    <controls:InkCanvas x:Name="InkSurface" Grid.Row="1" />
</Grid>
```

```csharp
var canvas = new InkCanvas();
var toolbar = new InkToolbar { TargetInkCanvas = canvas };
toolbar.ActiveTool = toolbar.GetToolButton(InkToolbarTool.Pencil);

var grid = new Grid();
grid.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto });
grid.RowDefinitions.Add(new RowDefinition());
Grid.SetRow(toolbar, 0);
Grid.SetRow(canvas, 1);
grid.Children.Add(toolbar);
grid.Children.Add(canvas);
```

```cpp
using namespace winrt::Microsoft::UI::Xaml;
using namespace winrt::Microsoft::UI::Xaml::Controls;

InkCanvas canvas{};
InkToolbar toolbar{};
toolbar.TargetInkCanvas(canvas);
toolbar.ActiveTool(toolbar.GetToolButton(InkToolbarTool::Pencil));

Grid grid{};
RowDefinition autoRow{};
autoRow.Height(GridLengthHelper::Auto());
grid.RowDefinitions().Append(autoRow);
grid.RowDefinitions().Append(RowDefinition{});
Grid::SetRow(toolbar, 0);
Grid::SetRow(canvas, 1);
grid.Children().Append(toolbar);
grid.Children().Append(canvas);
```

<img src="./inking-csharp.png" alt="An InkToolbar above an InkCanvas with the pencil tool selected" width="360"/>

### Remarks

- **Rendering and the compositor**: using `InkCanvas` with the **system compositor** (via the
  compositor Switcher) is the recommended configuration. With the system compositor the canvas is free
  of the lifted compositor's rendering constraints. If an app does not opt in to the system compositor,
  `InkCanvas` presents its ink through lifted external content and is therefore subject to the Visual
  Layer [external content](https://learn.microsoft.com/windows/apps/develop/composition/visual-layer#external-content)
  limitations: XAML clipping, transforms, and opacity apply to the surface, but **z-order does not**.
  The ink surface is composed above the app's XAML content, so a XAML element placed over the canvas
  (an `InkToolbar` positioned on top of it, for example) is neither drawn above the ink nor able to
  receive pointer input where it overlaps the canvas. Lay overlapping UI out beside the canvas rather
  than on top of it, or opt in to the system compositor. Effects that need to read the ink pixels back
  (a XAML effect brush sampling the surface, or a `RenderTargetBitmap` capture of the ink) likewise do
  not compose over the ink. To snapshot ink in that configuration, render the strokes yourself from the
  `InkStrokeContainer` rather than capturing the surface.

## Custom drying (app-rendered dry ink)

By default the `InkPresenter` renders committed ("dry") strokes for you. **Custom drying** hands that
job to the app: you receive each stroke the moment the presenter commits it and draw it into your own
visual. This is what a note-taking or document app uses to apply a custom brush, run ink through its
own document model, or composite ink with other content.

Turn it on once, before the first stroke, by calling `InkPresenter.ActivateCustomDrying()`. It returns
an `InkSynchronizer`. From then on, handle `StrokesCollected` and bracket your rendering with
`BeginDry` / `EndDry`:

```csharp
// Once, at setup - before any stroke is drawn.
var presenter = InkSurface.InkPresenter;
InkSynchronizer synchronizer = presenter.ActivateCustomDrying();

presenter.StrokesCollected += (s, e) =>
{
    // BeginDry hands back the just-committed strokes and holds the wet layer up so there is no gap.
    var strokes = synchronizer.BeginDry();

    // Render the strokes into your own canvas / visual however you like (custom brush, effects, ...).
    foreach (var stroke in strokes)
    {
        RenderDryStroke(stroke);   // app-owned rendering
    }

    // Release the wet layer now that your dry ink is on screen.
    synchronizer.EndDry();
};
```

> [!NOTE]
> Call `ActivateCustomDrying()` before the first stroke is collected. `BeginDry` is only valid from
> inside the `StrokesCollected` handler (it runs in context on the presenter's commit), and every
> `BeginDry` must be paired with an `EndDry`. If you never activate custom drying, the presenter dries
> ink for you and none of this is needed.

![Custom drying: app-rendered dry strokes next to default drying](./inking-customdry.png)

# API Pages

## Differences from WinUI 2 (UWP)

This section is the complete list of differences between this API and the WinUI 2 / UWP inking surface.
**Anything not called out here is exactly the same** - same type name, same member names, same
signatures, and same behavior.

### Why there is a difference at all

In UWP the whole inking stack ran inside the app's view process, and apps talked to the OS ink objects
directly. In WinUI 3 the underlying OS ink objects (`Windows.UI.Input.Inking.InkPresenter` and
everything reached through it) are thread-affine to a dedicated **ink thread** that is not the XAML UI
thread. A XAML control cannot hand those objects to app code on the UI thread.

So `InkPresenter`, `InkStrokeContainer`, `InkStrokeInput`, `InkUnprocessedInput`, `InkSynchronizer`,
and the input-configuration types are re-declared in `Microsoft.UI.Xaml.Controls` as thin, UI-thread
accessible **mirrors**. Each mirror marshals calls to the ink thread and re-raises events back on the
UI thread. The member shapes are unchanged; only the declaring type and namespace differ.

Leaf data types that are already thread-agnostic are **reused from `Windows.UI.Input.Inking`
unchanged**: `InkStroke`, `InkDrawingAttributes`, `InkStrokeBuilder`, `InkPresenterRuler`,
`InkPresenterProtractor`, `InkPersistenceFormat`, `InkPresenterPredefinedConfiguration`,
`CoreInputDeviceTypes`, and `Windows.UI.Core.PointerEventArgs`. Because the stroke type itself is
reused, serialization (ISF / GIF) and `InkStrokeBuilder` interop behave identically to UWP.

### Type mapping

| WinUI 3 type | WinUI 2 / UWP type | Difference |
|---|---|---|
| `Microsoft.UI.Xaml.Controls.InkCanvas` | `Windows.UI.Xaml.Controls.InkCanvas` | Namespace only |
| `Microsoft.UI.Xaml.Controls.InkToolbar` and all toolbar button types | `Windows.UI.Xaml.Controls.*` | Namespace only |
| `Microsoft.UI.Xaml.Automation.Peers.*` peers | `Windows.UI.Xaml.Automation.Peers.*` | Namespace only |
| `Microsoft.UI.Xaml.Controls.InkPresenter` | `Windows.UI.Input.Inking.InkPresenter` | Re-declared as a UI-thread mirror; see member gaps below |
| `Microsoft.UI.Xaml.Controls.InkStrokeContainer` | `Windows.UI.Input.Inking.InkStrokeContainer` | Re-declared as a UI-thread mirror; see member gaps below |
| `Microsoft.UI.Xaml.Controls.InkStrokeInput` | `Windows.UI.Input.Inking.InkStrokeInput` | Re-declared as a UI-thread mirror; events re-raised on the UI thread |
| `Microsoft.UI.Xaml.Controls.InkUnprocessedInput` | `Windows.UI.Input.Inking.InkUnprocessedInput` | Re-declared as a UI-thread mirror; events re-raised on the UI thread |
| `Microsoft.UI.Xaml.Controls.InkSynchronizer` | `Windows.UI.Input.Inking.InkSynchronizer` | Re-declared as a UI-thread mirror |
| `Microsoft.UI.Xaml.Controls.InkInputProcessingConfiguration` | `Windows.UI.Input.Inking.InkInputProcessingConfiguration` | Re-declared as a UI-thread mirror |
| `Microsoft.UI.Xaml.Controls.InkInputConfiguration` | `Windows.UI.Input.Inking.InkInputConfiguration` | Re-declared as a UI-thread mirror; see member gaps below |
| `InkInputProcessingMode`, `InkInputRightDragAction`, `InkHighContrastAdjustment` | `Windows.UI.Input.Inking.*` | Re-declared enums, identical names and values |

### Functionality gaps

| Area | WinUI 2 / UWP | WinUI 3 | Why |
|---|---|---|---|
| `InkPresenter.StrokeContainer` | get / set | get only | The presenter owns its container; strokes are added and removed through the container's own methods. |
| `InkInputConfiguration.IsPenHapticFeedbackEnabled` | Available | Not available | No haptic feedback support. |
| `InkStrokeContainer.UpdateRecognitionResults()` / `GetRecognitionResults()` | Available | Not available | Handwriting recognition is out of scope for this surface. |
| `InkToolbar.IsRulerButtonCheckedChanged` | Available (deprecated) | Not available | Deprecated in UWP; superseded by `IsStencilButtonCheckedChanged`. |
| Surface Dial / `RadialController` ink integration | Available | Not available | Separate system feature, not part of this surface. |
| Pen-flyout live wet-stroke preview | Available | Not available | Toolbar visual not carried over. |
| `InkHighContrastAdjustment.UseSystemColorsWhenNecessary` default-palette filtering | Available | Not available | The other high-contrast modes behave as in UWP. |
| Rendering | Composed in-process with the rest of the app's content | Composed as external content unless the app opts in to the system compositor | See [Remarks](#remarks); the system compositor is the recommended configuration. |
| Threading of synchronous members | Ran on the app's view thread | Mirror members block the caller while marshaling to the ink thread | Most members are synchronous, so a call made while `SaveAsync` / `LoadAsync` is in flight waits for that I/O. `SaveAsync` / `LoadAsync` remain async; events are raised on the UI thread. |

## InkCanvas class

An unsealed `FrameworkElement` that hosts an ink surface. It exposes a single member, `InkPresenter`,
through which all ink configuration and events flow - mirroring UWP `InkCanvas`.

```csharp
public unsealed class InkCanvas : Microsoft.UI.Xaml.FrameworkElement
{
    public InkCanvas();
    public InkPresenter InkPresenter { get; }
}
```

### Example usage

```xaml
<controls:InkCanvas x:Name="InkSurface" />
```

```csharp
InkSurface.InkPresenter.InputDeviceTypes |=
    CoreInputDeviceTypes.Mouse | CoreInputDeviceTypes.Touch;
```

## InkCanvas.InkPresenter property

Gets the `InkPresenter` that owns this canvas's ink configuration, stroke container, and input events.
Read-only; the presenter is created with the canvas.

## InkPresenter class

The configuration and event hub for an `InkCanvas`, mirroring `Windows.UI.Input.Inking.InkPresenter`.

| Member | Kind | Description |
|---|---|---|
| `InputDeviceTypes` | property | Which pointer device types produce ink (`Pen`, `Mouse`, `Touch`, ...). Defaults to `Pen`, as in UWP. |
| `IsInputEnabled` | property | Enables / disables ink input. |
| `UpdateDefaultDrawingAttributes(InkDrawingAttributes)` | method | Sets the default drawing attributes (color, size, pen tip, ...). |
| `CopyDefaultDrawingAttributes()` | method | Returns an independent copy of the default drawing attributes; the caller mutates it and applies it back with `UpdateDefaultDrawingAttributes`. |
| `SetPredefinedConfiguration(InkPresenterPredefinedConfiguration)` | method | Applies a predefined input-processing configuration - `SimpleSinglePointer` or `SimpleMultiplePointer` (single- vs multi-pointer processing), not a tool. |
| `HighContrastAdjustment` | property | High-contrast rendering mode (`InkHighContrastAdjustment`). |
| `StrokeContainer` | property | The `InkStrokeContainer` holding this presenter's strokes. |
| `InputProcessingConfiguration` | property | `InkInputProcessingConfiguration` (`Mode`, `RightDragAction`). |
| `InputConfiguration` | property | `InkInputConfiguration` (barrel-button, eraser input toggles). |
| `StrokeInput` | property | `InkStrokeInput` - raw stroke lifecycle events. |
| `UnprocessedInput` | property | `InkUnprocessedInput` - pointer events not turned into ink. |
| `ActivateCustomDrying()` | method | Switches the presenter into custom drying and returns an `InkSynchronizer` so the app renders dry ink itself. Call before the first stroke is collected. See [Custom drying](#custom-drying-app-rendered-dry-ink). |
| `StrokesCollected` | event | Raised when wet ink is committed to dry strokes. Args: `InkStrokesCollectedEventArgs`. |
| `StrokesErased` | event | Raised when strokes are erased. Args: `InkStrokesErasedEventArgs`. |

## InkPresenter.ActivateCustomDrying method

Switches the presenter into **custom drying** and returns the `InkSynchronizer` the app uses to render
dry ink itself. In the default (system) drying mode the presenter renders committed strokes as dry
ink for you. With custom drying the app takes that over: it receives the strokes the presenter just
committed and draws them into its own visual, which is what lets an app apply custom brushes, effects,
or its own document model to dry ink.

Call `ActivateCustomDrying()` once, before the first stroke is collected. It returns an
`InkSynchronizer`; from then on `StrokesCollected` is your cue to run a `BeginDry` / render / `EndDry`
cycle (see [Custom drying](#custom-drying-app-rendered-dry-ink)).

## InkSynchronizer class

Returned by `InkPresenter.ActivateCustomDrying()`. Brackets the app's own rendering of dry ink so the
presenter does not also draw the same strokes.

| Member | Kind | Description |
|---|---|---|
| `BeginDry()` | method | Returns the strokes the presenter just committed and holds the wet layer up so there is no gap between the wet stroke disappearing and the app's dry ink appearing. |
| `EndDry()` | method | Releases the wet layer after the app has rendered the strokes as dry ink. |

The underlying OS synchronizer is thread-affine to the ink thread, so both calls marshal there through
the owning `InkPresenter`. `BeginDry` runs in context inside the presenter's commit, so call it from
the `StrokesCollected` handler and pair every `BeginDry` with an `EndDry`.

## InkStrokeContainer class

Holds and serializes the presenter's strokes and provides selection and clipboard operations.

Key members: `GetStrokes()`, `AddStroke` / `AddStrokes`, `Clear()`, `GetStrokeById(UInt32)`,
`SaveAsync` (with an `InkPersistenceFormat` overload) / `LoadAsync`, `BoundingRect`,
`SelectWithLine` / `SelectWithPolyLine`, `MoveSelected`, `DeleteSelected`,
`CopySelectedToClipboard` / `PasteFromClipboard` / `CanPasteFromClipboard`.

> [!NOTE]
> Geometry is in **DIPs**, the same as UWP and the same as the rest of the XAML tree. The canvas sizes
> the ink presenter from `ActualWidth` / `ActualHeight` directly, so `BoundingRect` returns, and
> `SelectWithLine` / `SelectWithPolyLine` expect, XAML layout coordinates - no rasterization-scale
> conversion is needed when feeding a lasso path back into the XAML tree.

## InkStrokeInput class

Raw stroke lifecycle, re-raised on the UI thread. Events: `StrokeStarted`, `StrokeContinued`,
`StrokeEnded`, `StrokeCanceled` (all
`TypedEventHandler<InkStrokeInput, Windows.UI.Core.PointerEventArgs>`). `InkPresenter` returns the
owning presenter.

> [!NOTE]
> These events carry `Windows.UI.Core.PointerEventArgs`, the same type UWP used, rather than
> `Microsoft.UI.Input.PointerEventArgs`. The argument is produced by the OS `InkPresenter` on the ink
> thread and passed straight through by the mirror; keeping the `Windows.UI.Core` type matches the UWP
> signature exactly and avoids a per-event marshaling conversion on the raw input path.

## InkUnprocessedInput class

Pointer events the presenter did not convert to ink (used with `InputProcessingMode.None`, for example
lasso selection). Events: `PointerEntered`, `PointerHovered`, `PointerExited`, `PointerPressed`,
`PointerMoved`, `PointerReleased`, `PointerLost`. `InkPresenter` returns the owning presenter.

## InkStrokesCollectedEventArgs / InkStrokesErasedEventArgs

Each exposes `Strokes` - an `IVectorView<Windows.UI.Input.Inking.InkStroke>` of the strokes that were
collected or erased.

## InkToolbar class

An unsealed `Control` that auto-populates and manages inking tool buttons and drives an attached
`InkCanvas`.

| Member | Kind | Description |
|---|---|---|
| `TargetInkCanvas` | property | The `InkCanvas` this toolbar drives. |
| `TargetInkPresenter` | property | Alternative target when driving an `InkPresenter` directly instead of an `InkCanvas`. |
| `InitialControls` | property | Which default buttons to auto-populate (`All`, `None`, `PensOnly`, `AllExceptPens`). |
| `ActiveTool` | property | The currently selected `InkToolbarToolButton`. |
| `InkDrawingAttributes` | property (get) | The active tool's drawing attributes. |
| `IsRulerButtonChecked` / `IsStencilButtonChecked` | properties | Stencil (ruler / protractor) toggles. |
| `ButtonFlyoutPlacement` | property | Placement of tool flyouts. |
| `Orientation` | property | `Horizontal` (default) or `Vertical`. |
| `Children` | property (get) | The toolbar's child buttons (content property). |
| `GetToolButton` / `GetToggleButton` / `GetMenuButton` | methods | Look up a button by `InkToolbarTool` / `InkToolbarToggle` / `InkToolbarMenuKind`. |
| `ActiveToolChanged`, `InkDrawingAttributesChanged`, `EraseAllClicked`, `IsStencilButtonCheckedChanged` | events | Toolbar interaction events. |

### InkToolbar button types

The toolbar's buttons form a small hierarchy:

- `InkToolbarToolButton` (`RadioButton`) -> `InkToolbarPenButton`, `InkToolbarEraserButton`,
  `InkToolbarCustomToolButton`
  - `InkToolbarPenButton` -> `InkToolbarBallpointPenButton`, `InkToolbarPencilButton`,
    `InkToolbarHighlighterButton`, `InkToolbarCustomPenButton`
- `InkToolbarToggleButton` (`CheckBox`) -> `InkToolbarRulerButton`, `InkToolbarCustomToggleButton`
- `InkToolbarMenuButton` (`ToggleButton`) -> `InkToolbarStencilButton`
- `InkToolbarCustomPen` (`DependencyObject`) - factory for custom drawing attributes.
- `InkToolbarPenConfigurationControl`, `InkToolbarFlyoutItem` - flyout / config building blocks.

`InkToolbarPenButton` exposes the color `Palette`, `MinStrokeWidth` / `MaxStrokeWidth`,
`SelectedBrush` / `SelectedBrushIndex`, and `SelectedStrokeWidth`. `InkToolbarEraserButton` exposes
`IsClearAllVisible` (default `true`). `InkToolbarStencilButton` exposes `Ruler`, `Protractor`,
`SelectedStencil`, `IsRulerItemVisible` / `IsProtractorItemVisible` (default `true`).

## InkCanvasAutomationPeer / InkToolbarAutomationPeer classes

`InkCanvasAutomationPeer` (from `FrameworkElementAutomationPeer`) provides the automation peer for
`InkCanvas`. `InkToolbarAutomationPeer` (from `FrameworkElementAutomationPeer`) is the peer for the
toolbar. The toolbar's buttons have dedicated peers: `InkToolbarToolButtonAutomationPeer` and
`InkToolbarMenuButtonAutomationPeer` implement `IExpandCollapseProvider` (expand / collapse the tool
flyout), and `InkToolbarFlyoutItemAutomationPeer` implements `IInvokeProvider`. Keyboard and
automation behavior match the WinUI 2 toolbar.

# API Details

```c# (but really MIDL3)
namespace Microsoft.UI.Xaml.Controls
{
    // ---- InkCanvas / InkPresenter surface ----

    [MUX_PREVIEW]
    runtimeclass InkStrokesCollectedEventArgs
    {
        Windows.Foundation.Collections.IVectorView<Windows.UI.Input.Inking.InkStroke> Strokes{ get; };
    }

    [MUX_PREVIEW]
    runtimeclass InkStrokesErasedEventArgs
    {
        Windows.Foundation.Collections.IVectorView<Windows.UI.Input.Inking.InkStroke> Strokes{ get; };
    }

    [MUX_PREVIEW]
    runtimeclass InkStrokeContainer
    {
        void Clear();
        Windows.Foundation.Collections.IVectorView<Windows.UI.Input.Inking.InkStroke> GetStrokes();
        void AddStroke(Windows.UI.Input.Inking.InkStroke stroke);
        void AddStrokes(Windows.Foundation.Collections.IIterable<Windows.UI.Input.Inking.InkStroke> strokes);
        [default_overload][overload("SaveAsync")]
        Windows.Foundation.IAsyncAction SaveAsync(Windows.Storage.Streams.IOutputStream outputStream);
        [overload("SaveAsync")]
        Windows.Foundation.IAsyncAction SaveWithFormatAsync(Windows.Storage.Streams.IOutputStream outputStream, Windows.UI.Input.Inking.InkPersistenceFormat inkPersistenceFormat);
        Windows.Foundation.IAsyncAction LoadAsync(Windows.Storage.Streams.IInputStream inputStream);
        Windows.UI.Input.Inking.InkStroke GetStrokeById(UInt32 id);
        Windows.Foundation.Rect DeleteSelected();
        Windows.Foundation.Rect MoveSelected(Windows.Foundation.Point translation);
        Windows.Foundation.Rect SelectWithLine(Windows.Foundation.Point from, Windows.Foundation.Point to);
        Windows.Foundation.Rect SelectWithPolyLine(Windows.Foundation.Collections.IIterable<Windows.Foundation.Point> points);
        Windows.Foundation.Rect BoundingRect{ get; };
        void CopySelectedToClipboard();
        Windows.Foundation.Rect PasteFromClipboard(Windows.Foundation.Point position);
        Boolean CanPasteFromClipboard();
    }

    [MUX_PREVIEW]
    enum InkInputProcessingMode { None = 0, Inking = 1, Erasing = 2 };

    [MUX_PREVIEW]
    enum InkInputRightDragAction { LeaveUnprocessed = 0, AllowProcessing = 1 };

    [MUX_PREVIEW]
    runtimeclass InkInputProcessingConfiguration
    {
        InkInputProcessingMode Mode;
        InkInputRightDragAction RightDragAction;
    }

    [MUX_PREVIEW]
    runtimeclass InkInputConfiguration
    {
        Boolean IsPrimaryBarrelButtonInputEnabled;
        Boolean IsEraserInputEnabled;
    }

    [MUX_PREVIEW]
    enum InkHighContrastAdjustment
    {
        UseSystemColorsWhenNecessary = 0,
        UseSystemColors = 1,
        UseOriginalColors = 2,
    };

    [MUX_PREVIEW]
    runtimeclass InkStrokeInput
    {
        event Windows.Foundation.TypedEventHandler<InkStrokeInput, Windows.UI.Core.PointerEventArgs> StrokeStarted;
        event Windows.Foundation.TypedEventHandler<InkStrokeInput, Windows.UI.Core.PointerEventArgs> StrokeContinued;
        event Windows.Foundation.TypedEventHandler<InkStrokeInput, Windows.UI.Core.PointerEventArgs> StrokeEnded;
        event Windows.Foundation.TypedEventHandler<InkStrokeInput, Windows.UI.Core.PointerEventArgs> StrokeCanceled;
        InkPresenter InkPresenter{ get; };
    }

    [MUX_PREVIEW]
    runtimeclass InkUnprocessedInput
    {
        event Windows.Foundation.TypedEventHandler<InkUnprocessedInput, Windows.UI.Core.PointerEventArgs> PointerEntered;
        event Windows.Foundation.TypedEventHandler<InkUnprocessedInput, Windows.UI.Core.PointerEventArgs> PointerHovered;
        event Windows.Foundation.TypedEventHandler<InkUnprocessedInput, Windows.UI.Core.PointerEventArgs> PointerExited;
        event Windows.Foundation.TypedEventHandler<InkUnprocessedInput, Windows.UI.Core.PointerEventArgs> PointerPressed;
        event Windows.Foundation.TypedEventHandler<InkUnprocessedInput, Windows.UI.Core.PointerEventArgs> PointerMoved;
        event Windows.Foundation.TypedEventHandler<InkUnprocessedInput, Windows.UI.Core.PointerEventArgs> PointerReleased;
        event Windows.Foundation.TypedEventHandler<InkUnprocessedInput, Windows.UI.Core.PointerEventArgs> PointerLost;
        InkPresenter InkPresenter{ get; };
    }

    [MUX_PREVIEW]
    runtimeclass InkSynchronizer
    {
        // Take over rendering of dry (committed) ink. BeginDry hands back the strokes the presenter
        // just committed and holds the wet layer; the app renders them as dry ink and calls EndDry to
        // release the wet layer. Both calls marshal to the ink thread through the owning InkPresenter.
        Windows.Foundation.Collections.IVectorView<Windows.UI.Input.Inking.InkStroke> BeginDry();
        void EndDry();
    }

    [MUX_PREVIEW]
    runtimeclass InkPresenter
    {
        Windows.UI.Core.CoreInputDeviceTypes InputDeviceTypes;
        Boolean IsInputEnabled;
        void UpdateDefaultDrawingAttributes(Windows.UI.Input.Inking.InkDrawingAttributes drawingAttributes);
        Windows.UI.Input.Inking.InkDrawingAttributes CopyDefaultDrawingAttributes();
        void SetPredefinedConfiguration(Windows.UI.Input.Inking.InkPresenterPredefinedConfiguration configuration);
        InkHighContrastAdjustment HighContrastAdjustment;
        InkStrokeContainer StrokeContainer{ get; };
        InkInputProcessingConfiguration InputProcessingConfiguration{ get; };
        InkInputConfiguration InputConfiguration{ get; };
        InkStrokeInput StrokeInput{ get; };
        InkUnprocessedInput UnprocessedInput{ get; };
        // Switch into custom-drying mode and return the synchronizer the app uses to render dry ink.
        // Must be called before the first stroke is collected.
        InkSynchronizer ActivateCustomDrying();
        event Windows.Foundation.TypedEventHandler<InkPresenter, InkStrokesCollectedEventArgs> StrokesCollected;
        event Windows.Foundation.TypedEventHandler<InkPresenter, InkStrokesErasedEventArgs> StrokesErased;
    }

    [MUX_PREVIEW]
    unsealed runtimeclass InkCanvas : Microsoft.UI.Xaml.FrameworkElement
    {
        InkCanvas();
        InkPresenter InkPresenter{ get; };
    };

    // ---- InkToolbar surface (enums) ----

    [MUX_PREVIEW] enum InkToolbarButtonFlyoutPlacement { Auto, Top, Bottom, Left, Right };
    [MUX_PREVIEW] enum InkToolbarFlyoutItemKind { Simple, Radio, Check, RadioCheck };
    [MUX_PREVIEW] enum InkToolbarInitialControls { All, None, PensOnly, AllExceptPens };
    [MUX_PREVIEW] enum InkToolbarMenuKind { Stencil };
    [MUX_PREVIEW] enum InkToolbarStencilKind { Ruler, Protractor };
    [MUX_PREVIEW] enum InkToolbarToggle { Ruler, Custom };
    [MUX_PREVIEW] enum InkToolbarTool { BallpointPen, Pencil, Highlighter, Eraser, CustomPen, CustomTool };

    // ---- InkToolbar surface (classes) ----

    [MUX_PREVIEW]
    runtimeclass InkToolbarIsStencilButtonCheckedChangedEventArgs
    {
        InkToolbarStencilButton StencilButton{ get; };
        InkToolbarStencilKind StencilKind{ get; };
    };

    [MUX_PREVIEW]
    [contentproperty("Children")]
    unsealed runtimeclass InkToolbar : Microsoft.UI.Xaml.Controls.Control
    {
        InkToolbar();
        InkToolbarInitialControls InitialControls;
        Microsoft.UI.Xaml.DependencyObjectCollection Children{ get; };
        InkToolbarToolButton ActiveTool;
        Windows.UI.Input.Inking.InkDrawingAttributes InkDrawingAttributes{ get; };
        Boolean IsRulerButtonChecked;
        InkCanvas TargetInkCanvas;
        Boolean IsStencilButtonChecked;
        InkToolbarButtonFlyoutPlacement ButtonFlyoutPlacement;
        [MUX_DEFAULT_VALUE("winrt::Orientation::Horizontal")]
        Microsoft.UI.Xaml.Controls.Orientation Orientation;
        InkPresenter TargetInkPresenter;
        event Windows.Foundation.TypedEventHandler<InkToolbar, Object> ActiveToolChanged;
        event Windows.Foundation.TypedEventHandler<InkToolbar, Object> InkDrawingAttributesChanged;
        event Windows.Foundation.TypedEventHandler<InkToolbar, Object> EraseAllClicked;
        event Windows.Foundation.TypedEventHandler<InkToolbar, InkToolbarIsStencilButtonCheckedChangedEventArgs> IsStencilButtonCheckedChanged;
        InkToolbarToolButton GetToolButton(InkToolbarTool tool);
        InkToolbarToggleButton GetToggleButton(InkToolbarToggle tool);
        InkToolbarMenuButton GetMenuButton(InkToolbarMenuKind menu);
        // + generated DependencyProperty statics for each property above.
    };

    [MUX_PREVIEW]
    [constructor_name("Microsoft.UI.Xaml.Controls.IInkToolbarCustomPenFactory")]
    unsealed runtimeclass InkToolbarCustomPen : Microsoft.UI.Xaml.DependencyObject
    {
        [method_name("CreateInstance")] protected InkToolbarCustomPen();
        Windows.UI.Input.Inking.InkDrawingAttributes CreateInkDrawingAttributes(Microsoft.UI.Xaml.Media.Brush brush, Double strokeWidth);
        overridable Windows.UI.Input.Inking.InkDrawingAttributes CreateInkDrawingAttributesCore(Microsoft.UI.Xaml.Media.Brush brush, Double strokeWidth);
    };

    [MUX_PREVIEW]
    unsealed runtimeclass InkToolbarPenConfigurationControl : Microsoft.UI.Xaml.Controls.Control
    {
        [method_name("CreateInstance")] InkToolbarPenConfigurationControl();
        InkToolbarPenButton PenButton{ get; };
    };

    [MUX_PREVIEW]
    unsealed runtimeclass InkToolbarFlyoutItem : Microsoft.UI.Xaml.Controls.Primitives.ButtonBase
    {
        [method_name("CreateInstance")] InkToolbarFlyoutItem();
        InkToolbarFlyoutItemKind Kind;
        Boolean IsChecked;
        event Windows.Foundation.TypedEventHandler<InkToolbarFlyoutItem, Object> Checked;
        event Windows.Foundation.TypedEventHandler<InkToolbarFlyoutItem, Object> Unchecked;
    };

    [MUX_PREVIEW]
    [constructor_name("Microsoft.UI.Xaml.Controls.IInkToolbarMenuButtonFactory")]
    unsealed runtimeclass InkToolbarMenuButton : Microsoft.UI.Xaml.Controls.Primitives.ToggleButton
    {
        InkToolbarMenuKind MenuKind{ get; };
        Boolean IsExtensionGlyphShown;
    };

    [MUX_PREVIEW]
    unsealed runtimeclass InkToolbarStencilButton : InkToolbarMenuButton
    {
        [method_name("CreateInstance")] InkToolbarStencilButton();
        Windows.UI.Input.Inking.InkPresenterRuler Ruler{ get; };
        Windows.UI.Input.Inking.InkPresenterProtractor Protractor{ get; };
        InkToolbarStencilKind SelectedStencil;
        [MUX_DEFAULT_VALUE("true")] Boolean IsRulerItemVisible;
        [MUX_DEFAULT_VALUE("true")] Boolean IsProtractorItemVisible;
    };

    [MUX_PREVIEW]
    [constructor_name("Microsoft.UI.Xaml.Controls.IInkToolbarToggleButtonFactory")]
    unsealed runtimeclass InkToolbarToggleButton : Microsoft.UI.Xaml.Controls.CheckBox
    {
        InkToolbarToggle ToggleKind{ get; };
    };

    [MUX_PREVIEW]
    [constructor_name("Microsoft.UI.Xaml.Controls.IInkToolbarToolButtonFactory")]
    unsealed runtimeclass InkToolbarToolButton : Microsoft.UI.Xaml.Controls.RadioButton
    {
        InkToolbarTool ToolKind{ get; };
        Boolean IsExtensionGlyphShown;
    };

    [MUX_PREVIEW] unsealed runtimeclass InkToolbarCustomToggleButton : InkToolbarToggleButton { [method_name("CreateInstance")] InkToolbarCustomToggleButton(); };
    [MUX_PREVIEW] unsealed runtimeclass InkToolbarRulerButton : InkToolbarToggleButton { [method_name("CreateInstance")] InkToolbarRulerButton(); };

    [MUX_PREVIEW]
    unsealed runtimeclass InkToolbarCustomToolButton : InkToolbarToolButton
    {
        [method_name("CreateInstance")] InkToolbarCustomToolButton();
        Microsoft.UI.Xaml.UIElement ConfigurationContent;
    };

    [MUX_PREVIEW]
    unsealed runtimeclass InkToolbarEraserButton : InkToolbarToolButton
    {
        [method_name("CreateInstance")] InkToolbarEraserButton();
        [MUX_DEFAULT_VALUE("true")] Boolean IsClearAllVisible;
    };

    [MUX_PREVIEW]
    [constructor_name("Microsoft.UI.Xaml.Controls.IInkToolbarPenButtonFactory")]
    unsealed runtimeclass InkToolbarPenButton : InkToolbarToolButton
    {
        Windows.Foundation.Collections.IVector<Microsoft.UI.Xaml.Media.Brush> Palette;
        Double MinStrokeWidth;
        Double MaxStrokeWidth;
        Microsoft.UI.Xaml.Media.Brush SelectedBrush{ get; };
        Int32 SelectedBrushIndex;
        Double SelectedStrokeWidth;
    };

    [MUX_PREVIEW] unsealed runtimeclass InkToolbarBallpointPenButton : InkToolbarPenButton { [method_name("CreateInstance")] InkToolbarBallpointPenButton(); };

    [MUX_PREVIEW]
    unsealed runtimeclass InkToolbarCustomPenButton : InkToolbarPenButton
    {
        [method_name("CreateInstance")] InkToolbarCustomPenButton();
        InkToolbarCustomPen CustomPen;
        Microsoft.UI.Xaml.UIElement ConfigurationContent;
    };

    [MUX_PREVIEW] unsealed runtimeclass InkToolbarHighlighterButton : InkToolbarPenButton { [method_name("CreateInstance")] InkToolbarHighlighterButton(); };
    [MUX_PREVIEW] unsealed runtimeclass InkToolbarPencilButton : InkToolbarPenButton { [method_name("CreateInstance")] InkToolbarPencilButton(); };
}

namespace Microsoft.UI.Xaml.Automation.Peers
{
    [MUX_PREVIEW]
    unsealed runtimeclass InkCanvasAutomationPeer : Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer
    {
        InkCanvasAutomationPeer(Microsoft.UI.Xaml.Controls.InkCanvas owner);
    }

    [MUX_PREVIEW]
    unsealed runtimeclass InkToolbarAutomationPeer : Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer
    {
        InkToolbarAutomationPeer(Microsoft.UI.Xaml.Controls.InkToolbar owner);
    }

    // Tool and menu buttons expose ExpandCollapse (open / close the tool flyout); flyout items expose Invoke.
    [MUX_PREVIEW]
    unsealed runtimeclass InkToolbarToolButtonAutomationPeer : Microsoft.UI.Xaml.Automation.Peers.RadioButtonAutomationPeer,
        Microsoft.UI.Xaml.Automation.Provider.IExpandCollapseProvider
    {
        InkToolbarToolButtonAutomationPeer(Microsoft.UI.Xaml.Controls.InkToolbarToolButton owner);
    }

    [MUX_PREVIEW]
    unsealed runtimeclass InkToolbarMenuButtonAutomationPeer : Microsoft.UI.Xaml.Automation.Peers.ToggleButtonAutomationPeer,
        Microsoft.UI.Xaml.Automation.Provider.IExpandCollapseProvider
    {
        InkToolbarMenuButtonAutomationPeer(Microsoft.UI.Xaml.Controls.InkToolbarMenuButton owner);
    }

    [MUX_PREVIEW]
    unsealed runtimeclass InkToolbarFlyoutItemAutomationPeer : Microsoft.UI.Xaml.Automation.Peers.ButtonBaseAutomationPeer,
        Microsoft.UI.Xaml.Automation.Provider.IInvokeProvider
    {
        InkToolbarFlyoutItemAutomationPeer(Microsoft.UI.Xaml.Controls.InkToolbarFlyoutItem owner);
    }
}
```


