# Retained XAML with selectable immediate-rendering backends

Status: exploratory design proposal. No backend, public API, or performance improvement described here is implemented
or guaranteed by this document. Success thresholds are proposed experiment gates, not measured results.

## Table of Contents

- [Summary](#summary)
- [Terminology and backend choices](#terminology-and-backend-choices)
- [Goals and non-goals](#goals-and-non-goals)
- [Current architecture and integration points](#current-architecture-and-integration-points)
- [Proposed architecture](#proposed-architecture)
- [Rendering and compatibility contracts](#rendering-and-compatibility-contracts)
- [DirectUI architectural precedent](#directui-architectural-precedent)
- [Memory and performance model](#memory-and-performance-model)
- [Evaluation plan](#evaluation-plan)
- [Success criteria and decision rules](#success-criteria-and-decision-rules)
- [Implementation milestones](#implementation-milestones)
- [Risks and open decisions](#risks-and-open-decisions)
- [Source references](#source-references)

## Summary

Preserve WinUI 3's retained XAML programming model while allowing developers to select either the existing
Composition rendering path or an immediate-rendering backend. Controls, dependency properties, bindings, layout,
input, and accessibility remain retained XAML responsibilities.

The hypothesis is that ordinary XAML content can be flattened into damage-updated raster regions, reducing
fine-grained retained Composition content and associated state. Selected Composition boundaries can still support
independent motion, presentation, and interop. The expected memory reduction must be demonstrated against the CPU,
GPU, latency, energy, compatibility, and surface-memory costs of this approach.

Keep all three immediate-rendering options in the experiment:

1. Immediate rendering through GDI+ (chosen over plain GDI because it supports antialiasing and per-pixel alpha).
2. Immediate rendering through Direct2D on a hardware device.
3. Immediate rendering through Direct2D on a software device, initially WARP.

Use the existing Composition path as the default and comparison baseline. Start with one startup selector applying
to all roots in a process, while placing internal policy and backend state at the content-root boundary. Independent
per-root selection is a design goal that requires additional shared-device and presentation validation.

## Terminology and backend choices

**Retained XAML, immediate rendering** is the working term. This is not an immediate-mode UI API: applications do not
rebuild their controls every frame or replace XAML with drawing code.

Immediate rendering means issuing drawing commands when pixels need updating rather than retaining corresponding
fine-grained Composition drawing objects. It does not imply continuous redraw, software rendering, synchronous GPU
execution, one submission per drawing call, or discarding useful resources after every frame.

| Proposed mode name | Rendering strategy | Content drawing implementation |
|---|---|---|
| `Composition` | Existing retained path | Existing behavior, unchanged |
| `ImmediateGdiPlus` | Damage-driven immediate path | GDI+ on the CPU, including text |
| `ImmediateD2DHardware` | Damage-driven immediate path | Direct2D on a hardware-backed D3D device |
| `ImmediateD2DSoftware` | Damage-driven immediate path | Direct2D on a software device, initially WARP |

These names are illustrative, not existing APIs or finalized public API names. The two Direct2D modes should share
one drawing implementation, with different device/resource creation. GDI+ needs a separate painter, sharing the
immediate traversal and damage infrastructure where semantics permit.

Plain GDI is not a candidate: it lacks antialiased geometry and per-pixel alpha, which Fluent styles rely on for
rounded corners and translucent fills. The GDI+ painter's contract is:

- Use `SmoothingModeAntiAlias` for geometry and premultiplied 32-bit ARGB (`PixelFormat32bppPARGB`) bitmaps.
- Render text with GDI+ (`Graphics::DrawString`, grayscale antialiasing on the PARGB bitmap) so the mode remains a
  pure GDI+ data point (see the prototype implementation decisions below). Glyph shaping and layout therefore differ
  from XAML's DirectWrite text stack; account for that in fidelity results.
- Paint into a premultiplied bitmap and copy damaged regions to the presentable surface, because drawing GDI+ directly
  to a surface HDC does not reliably preserve alpha. Count this copy in memory and upload metrics.
- Implement group opacity with an offscreen bitmap, as the other painters do.

GDI+ is maintained but no longer gains features; treat that as a product risk if the experiment favors this mode.

Rendering model and execution hardware are separate dimensions. Direct2D can render using hardware or software.
Treat GDI+ as a CPU-only rasterizer for this experiment.
CPU-painted content may subsequently be composited by the GPU. Similarly, a software Direct2D mode does not promise
that the windowing system, presentation, interop content, or other processes perform no GPU work.

## Goals and non-goals

### Goals

- Preserve existing XAML and application logic for the explicitly supported feature set.
- Offer four reproducibly selectable modes, including the existing default path.
- Reduce total attributable UI memory in identified application workloads, not merely visual-object counts.
- Reuse layout, input, accessibility, and appropriate text/image/resource infrastructure.
- Preserve selected independently composited interactions through explicit retained boundaries.
- Make the effective backend, fallback, resource costs, and unsupported features observable.
- Establish which workloads benefit from each mode and which should retain the existing backend.

### Initial non-goals

- Switching backends on a live root after rendering resources have been initialized.
- A public rendering-mode property on arbitrary controls or arbitrary mixed-mode subtrees.
- Full compatibility with every Composition API, effect, animation, or embedded surface in the first prototype.
- Eliminating all Composition objects, cached resources, or retained rendering metadata.
- Building a new custom Direct3D renderer when Direct2D can answer the initial experiment.
- Promising that every immediate backend outperforms Composition or that one mode wins every workload.

## Current architecture and integration points

The existing [XAML rendering architecture](rendering.md) provides background. Some historical sections identify
outdated behavior; the following integration observations were checked against this checkout.

| Boundary | Current observation | Proposed role |
|---|---|---|
| `CContentRoot` | Owns root-specific visual-tree, input, focus, and content-island relationships | Own immutable rendering policy and an associated root backend context |
| Root orchestration in `CCoreServices` | Iterates island roots, retargets a render target, and calls `HWWalk::RenderRoot` | Dispatch each root to its selected rendering strategy |
| `HWWalk::RenderRootImpl` | Directly constructs `VisualContentRenderer` and supplies it through `IContentRenderer` | Content-emission seam; not a complete backend boundary by itself |
| `HWWalk` and composition peers | Manage composition-relative transforms, retained render data, animation state, and dirty traversal | Extract reusable behavior; provide immediate-specific traversal/invalidation behavior |
| `CWindowRenderTarget` | Initializes walker/composition infrastructure and participates in cleanup/rebuild | Integrate backend resource lifetime with existing rendering lifecycle |
| Graphics device infrastructure | Includes shared-device and hardware/WARP behavior | Create appropriately scoped resources without accidentally changing unrelated roots |

WinUI already creates composition nodes selectively. The proposal is not simply to remove a composition node from
every element: the target includes fine-grained retained content visuals and their related state.

`CWindowRenderTarget` is not a one-to-one owner for every XAML root. The island-rendering loop reuses and retargets it.
A single mode field stored only on that object would therefore be insufficient for independently selectable roots.

Replacing `VisualContentRenderer` alone is also insufficient. A retained walker can skip an unchanged subtree because
its visuals already exist. An immediate damage repaint may need unchanged content from that subtree to reconstruct
the damaged pixels correctly. Composition-specific state, animation, and presentation assumptions must be audited
along the complete frame path, not just at primitive emission.

## Proposed architecture

```text
Application/window/island host chooses policy before root resource initialization
                                      |
                  ContentRoot policy + root backend context
                                      |
                         Root rendering dispatch
                         /                     \
             Existing retained path        Immediate path
                                           - damage tracking
                                           - affected-content traversal
                                           - ordering and state stacks
                                           - selected retained boundaries
                                           - bounded resource caches
                                                    |
                                            +-------+-------+
                                            |               |
                                       GDI+ painter     D2D painter
                                                            |
                                                     Hardware or WARP
                                      |
                   Presentation and selected Composition layers
```

This is a conceptual responsibility split, not a requirement to add a separate interface or class for every box.
Reuse existing helpers where their semantics fit. Do not copy the existing walker wholesale or introduce a second
per-element retained command tree without measuring its cost.

### Selection and lifetime

For the prototype, use an internal process-startup override with four values. Resolve it before backend resources
are initialized and apply the same policy to all roots. Keep root-scoped ownership internally so that the prototype
does not bake in a global-only product model.

The eventual developer-facing entry point should cover both windows and XAML island hosts. Final API shape and
the exact initialization boundary remain open; a window convenience API can delegate to the underlying root policy.
Policy should survive resize, retargeting, and device recovery, while device-dependent resources are recreated.
Reject late changes explicitly rather than partially switching resources.

Per-root hardware/software selection requires a device-sharing strategy. A global ForceWarp override is not an
implementation of an independently selectable software root. Distinguish shared immutable assets from resources
whose device affinity or lifetime belongs to a backend context.

#### D2D-HW adapter selection (prototype)

`D2DDeviceFactory.cpp` (policy in `inc\D2DDeviceFactory.h`) picks the first qualifying adapter from
`IDXGIFactory6::EnumAdapterByGpuPreference(HIGH_PERFORMANCE)`, falling back to `EnumAdapters1` order. It rejects
software adapters and every Microsoft (VendorId `0x1414`) adapter. On a Hyper-V VM, the display-only Hyper-V Video and
Remote Display adapters show up in DXGI as extra "Microsoft Basic Render Driver" adapters without the software flag,
and a D3D11 device created on them is still the Basic Render Driver. GPU-P guests report the host GPU vendor, so they
still qualify. `WINUI_RENDERING_ADAPTER_LUID` (hex) pins an adapter, and the pin is strict. With no qualifying adapter,
creation fails with `DXGI_ERROR_NOT_FOUND`; it never silently falls back to WARP.

### Painting and presentation

Share immediate-mode scheduling, damage, and drawing order across painters. Direct2D hardware and WARP should issue
equivalent drawing operations, subject to documented device capabilities. Retain appropriate image textures, glyph
resources, and bounded geometry caches rather than uploading or regenerating them unnecessarily.

Presentation is a separate concern. Both CPU and GPU content need an integration path into the WinUI host, including
alpha, clipping, DPI, resize, and synchronization. The GDI+ path must explicitly establish how its bitmap reaches
presentable surfaces and account for copies/uploads. Do not assume that drawing to an HWND alone preserves XAML
island or Composition interop semantics.

### Threading

Immediate drawing does not require UI-thread rendering. A render-thread snapshot is possible, but synchronization,
snapshot memory, and retained state are part of its cost.

Start the narrow prototype with UI-thread-driven command generation if that minimizes implementation scope. This is
not a production threading commitment. Selected compositor-managed layers can continue motion independently, but
new or changed content may still need the UI thread to paint. Measure that boundary explicitly.

## Rendering and compatibility contracts

### Damage and resource correctness

- Schedule rendering on invalidation or active animation, not an unconditional repaint timer.
- Repaint every contributor intersecting damaged pixels in painter's order, not just the changed element.
- Include old and new bounds when moving, removing, resizing, or changing the clip of ordinary content.
- Transform damage conservatively across coordinate spaces, DPI, antialiasing footprints, and supported effects.
- Preserve group-opacity semantics; overlapping children may require an intermediate surface.
- Separate content changes from transform/opacity changes at retained boundaries so eligible motion avoids repaint.
- Ensure partially updated buffers contain valid unchanged pixels; buffer preservation and presentation rules must
  be correct for the chosen surface/swap-chain implementation.
- Handle resize and device loss with valid full repaint/recreation; do not present partially reconstructed content.
- Bound caches and reclaim unused layers/surfaces without turning every frame into resource recreation.

Start with full redraw on invalidation to establish correctness, then implement damage-limited repainting. A full
redraw prototype does not establish the performance of the final damage-driven architecture.

### Retained boundaries and interop

Use retained boundaries deliberately for selected independent transforms, opacity animations, scrolling, and interop.
Promotion must preserve ordering, clipping, group semantics, and synchronization with raster content. An arbitrary
interop request cannot necessarily be satisfied by simply promoting one element.

An independently mutable Composition visual cannot be transparently substituted with flattened pixels. APIs exposing
or accepting Composition visuals need an explicit supported, promoted, or unsupported contract. Windowed popups and
other additional presentation roots also require an explicit policy/lifetime mapping.

Do not assume that currently clipped content never needs painting: independently animated retained layers may expose
it without another UI-thread update. Track valid content coverage and define how newly exposed regions are populated.

### Feature scope

| Stage | Candidate scope | Required evidence |
|---|---|---|
| First drawing slice | Solid fills, borders, images, text, rectangular clips, 2D transforms | Correct full redraw, lifecycle, actual selected device |
| Damage slice | Local updates, movement/removal, overlap, resize and DPI | Correct incremental pixels and actual repaint reduction |
| Interaction slice | Virtualized scrolling and a selected independently animated overlay | Independent motion, correct exposure, ordering, latency |
| Compatibility expansion | Editing/IME, gradients, rounded geometry, effects, popups, interop | Explicit feature matrix per backend and focused regression tests |

This is a candidate sequence, not a claim that GDI+ and Direct2D have identical capabilities. Before comparing any
workload, identify its must-preserve features and whether each backend supports them. Layout, hit testing, focus,
accessibility, selection, and text semantics must not be changed merely to obtain a better performance number.

### Full-backend decisions (S11-S23)

| Topic | Decision |
|---|---|
| Composition interop (`ElementCompositionPreview` hand-in/hand-off visuals with independent animations, implicit/connected animations), `SwapChainPanel`, `MediaPlayerElement` | **Per-window fallback to Composition.** When the walk finds such content in a window, that window (island) permanently switches to `HWWalk::RenderRoot` for the rest of its lifetime and the `WindowFallback` event is emitted. Other windows keep the immediate backend. |
| Hybrid (immediate content plus retained Composition layers in one window) | **Not now.** Backends stay independent: each implements materials, shadows, popups, Lottie, off-thread animation ticking, text, DPI/theme and device-lost recovery itself. Hybrid is reassessed later from measured functional or performance gaps. |
| WebView2 / hosted HWND (S17), InkCanvas (S18) | **Out of scope** for this phase. |
| Mica / desktop acrylic (host backdrop) | Approximated by the material's fallback color (same as Composition when transparency effects are disabled); in-app acrylic uses `BlurBackdrop` + tint/luminosity/noise. |
| Execution order | S11, S13, S14, S15, S12, S19, S16, S20, S21, S22, S23, each test-first. |

### Full XAML coverage: shapes

Covers families F1 (shapes and strokes) and F2 (brushes) from the test plan.

- **Painter contract.** `StrokeStyleDesc` holds the following:
  - dash array and offset in DIPs (the walk multiplies `StrokeDashArray` and `StrokeDashOffset` by the thickness, as XAML does)
  - dash, start and end caps, including Triangle
  - line join and miter limit (as a multiple of half the thickness)

  `BrushDesc` describes a solid, linear or radial brush:
  - stops and their spread (Pad, Reflect or Repeat)
  - sRGB or linear-light interpolation
  - for radial brushes: center, origin and radii
  - a brush-to-DIP transform

  New `*Brush` fill, stroke and glyph-run methods take both descriptions. The older solid and average-color methods are unchanged.
- **D2D.** Uses `ID2D1StrokeStyle` with custom dashes, `D2D1_CAP_STYLE_TRIANGLE`, and `D2D1_LINE_JOIN_MITER`, which clips at the limit as XAML does. Linear and radial gradient brushes use `D2D1_EXTEND_MODE` (clamp, mirror or wrap) for spread and `D2D1_GAMMA_1_0` for linear-light interpolation. The brush transform maps the unit space of relative brushes. Text gradients are drawn with the brush in glyph-run space. Dashed ellipses are stroked as a four-Bezier outline that starts at the rightmost point and runs clockwise, which matches Composition's dash phase (`DrawEllipse` places its dashes differently).
- **GDI+.** GDI+ has no exact equivalents, so it emulates them:
  - Dashes are split from the flattened outline by the painter (GDI+ dash patterns cannot express zero-length dashes, odd-length patterns, or per-dash caps that differ from the end caps). Each dash is stroked with the pen's line caps, including `LineCapTriangle`.
  - Rounded-rectangle outlines start at the top-left arc, matching D2D's dash phase.
  - GDI+ bevels a join once its miter limit is exceeded, while XAML clips the miter at the limit. GDI+ also measures the limit against the full stroke width, while XAML measures it against half. The painter passes half the XAML limit and fills the clipped miter quads itself.
  - Gradients are not mapped onto GDI+ gradient brushes, because `PathGradientBrush` cannot express focal or elliptic radial gradients and its interpolation and spread do not match. Instead, the painter builds a 1D ramp of premultiplied samples, interpolated with straight alpha in sRGB or linear light, as D2D and Composition do (a transparent stop keeps its color). It then computes t per device pixel (a linear projection, or the focal solution for radial) with spread applied, and shades a white coverage mask drawn into the bounded shadow scratch bitmap, then composites it 1:1. This makes radial origin/center/radii, Reflect/Repeat and linear-light interpolation exact up to ramp quantization.
- **Walk** (`ImmediateRenderer.Shapes.cpp`):
  - `ResolvePaint` maps the following to `BrushDesc`, with the drawing's bounds used for relative mapping:
    - `SolidColorBrush`
    - `LinearGradientBrush`, including Transform and RelativeTransform, spread and interpolation
    - the MUXC `RadialGradientBrush`. Its properties are read as custom dependency properties, and `GradientStops` through the peer's `IXamlMember`.
  - `ResolveStrokeStyle` maps the Shape stroke properties.
  - Gradient fills and strokes on Rectangle, Ellipse, Path, Line, Polyline, Polygon and Border, and gradient foregrounds on TextBlock and RichTextBlock glyph runs, render natively. The brush bounds match Composition (hwwalk `GetBrushBounds` per part): `CShape::GetFillBounds` for shape fills, the widened `CShape::GetStrokeBounds` for shape strokes (so a horizontal dashed Line still gets a gradient), the content bounds for Border, and the glyph-run line bounds for text. `Glyphs` runs have no line bounds, so, as Composition does with the run's realization bounds, the brush maps to the run's ink box (computed from the DirectWrite design metrics).
  - Linear gradients follow `CLinearGradientBrush`: relative start and end points are mapped into the bounds first, the gradient is perpendicular to start-end in DIP space (not in the unit square, as in WPF), and the combined Transform/RelativeTransform (`CBrushTypeUtils::GetBrushTransform`) is then applied as the brush transform, which may skew it.
  - The `Glyphs` element renders through `CGlyphs::VisitImmediate`, which reuses its text drawing context.
  - An ImageBrush whose transform includes rotation or skew draws the image under the brush transform, clipped to the element.
- **Still reported:**
  - `RadialGradientBrush HSL interpolation:approximated`: the RGB curve is used instead.
  - `RadialGradientBrush stops:fallback`: reported if the peer cannot be read; the brush then paints its fallback color, as Composition does.
  - TextBox and RichEditBox (RichEdit) gradient foregrounds keep the average color.
  - ImageBrush strokes and text foregrounds keep their current reporting.
- **Memory.** Each display-list entry holds its brush and style descriptions (stops plus dashes, a few hundred bytes at most). Painter stroke-style and gradient caches are LRU-bounded and are released with the painter. No surfaces are kept for individual elements. Measured on the `form` scene (one quick run): private MB median per phase is 43.0-43.6 for GDI+, 52.9-58.0 for D2D-SW and 45.3-46.1 for Composition. The scene draws no gradients or dashes, so it checks only that the new paths cost nothing when unused.
- **Tests:**
  - `ImmediatePainterShapeTests` holds the conformance cases for every mode. They check:
    - dash on and off positions, offset and start phase
    - cap extents, including Triangle
    - join shape and miter-limit clipping
    - linear and radial colors for each spread and interpolation mode
    - gradient glyph colors
    - cross-backend parity
  - Coverage pages are in `Coverage.Shapes.cs` (F1-shapes) and `Coverage.Brushes.cs` (F2-brushes).

### Full XAML coverage: text

Branch `immediate-rendering-text` (families F5 text and F6 text editing).

**Design.**
- **Static text.** `TextBlock`, `RichTextBlock`, `RichTextBlockOverflow` and `PasswordBox` keep the existing glyph-run replay: `VisitImmediate` walks XAML's recorded `D2DTextDrawingContext`.
  - TextHighlighters, decorations, selection, typography, spacing, line stacking, trimming, `MaxLines`, flow direction and reading order are all baked into the recorded runs, rectangles and lines, so they need no per-feature code.
  - `InlineUIContainer` children are ordinary visual children and are walked.
- **Editable text (RichEdit self-draw).** `CTextBoxView` renders through RichEdit's `ITextServices2::TxDrawD2D` into its recording `HWRenderTarget`, the same as in Composition mode, and then replays the recording through `ImmediateTextSink`.
  - The recording covers glyph runs, selection and highlight rectangles, underlines and strike lines, the wavy-line and inline-image bitmaps, and the placeholder.
  - This replaces the old plain `DrawText(TextBox.Text)` approximation and the `RichEditBox text` unsupported report. Scrolling, wrapping, alignment, selection (including `SelectionHighlightColor`, `SelectionHighlightColorWhenNotFocused` and read-only selection), IME and spell-check attributes, and RichEditBox formatting and images therefore match Composition by construction.
  - No new painter primitive is needed. A painter "external draw" callback was considered and rejected: the recording already exists, and replaying it keeps both painters and the display-list signature (damage) unchanged.
- **Caret.** The caret (`CompositeMode=DestInvert` rectangle) uses the new `IImmediatePainter::InvertRectangle(rect)` primitive, which applies `rgb = alpha - rgb` to the premultiplied destination and leaves alpha unchanged.
  - D2D: the rectangle recorded into a command list and drawn with `D2D1_COMPOSITE_MODE_MASK_INVERT`.
  - GDI+: in place on the PARGB target.
- **Color fonts.** `ImmediateTextSink::GlyphRunImageFormats` passes the recorded glyph image formats through, so runs recorded as monochrome only (`IsColorFontEnabled=False`) replay without color-glyph translation.

**Coverage.** Pages are in `Coverage.Text.cs` (F5-text) and `Coverage.TextEditing.cs` (F6-editing):
- F5-text (decorations, highlighters, typography, line-stacking, trimming-wrapping, flow-direction, rich-text, selection, fonts) and F6-editing (textbox, textbox-selection, textbox-scrolled, spell-check, passwordbox, richeditbox, suggest-number, caret) report nothing unsupported in GDI+ or D2D-SW.
- Mean absolute difference against Composition is ≤ 0.01 on every page in both modes, except GDI+ `fonts` at 0.014 (GDI+ antialiasing of large color and variable glyphs).

**Approximations and gaps.**
- *Caret shapes.* Rounded-rectangle and `Path` carets (RTL) are inverted over their bounding rectangle and reported `:approximated`.
- *Inverting a transparent destination.* Composition inverts the composed backdrop. The painters invert their own target, so a caret drawn over transparent pixels of a transparent island differs. It is identical over opaque content, which is the normal case.
- *Antialiasing.* D2D uses ClearType over opaque fills (S19) where Composition uses grayscale. GDI+ uses its own antialiasing.
- *Out of scope here.*
  - The `Glyphs` element is still reported `Element content` (F2, shapes).
  - The legacy dotted Hyperlink focus rectangles, used only when high-visibility focus visuals are disabled, belong to F14.
  - The elevation-border gradient on text controls is a brush issue (F3).
  - Spell-check squiggles take the same path as RichEdit's wavy underline, which is covered. The test VM has no spell checker, so they cannot be produced in any mode there.
- *Test page.* The RichEditBox page also inserts an inline image (`InsertImage` from an in-memory BMP). It renders in every mode. RichEdit reads the stream lazily, so the page keeps the stream alive.

**Memory.**
- No per-TextBox surfaces are added. The recorded RichEdit data already exists in Composition mode.
- Glyph runs are drawn directly with no glyph atlas.
- The wavy-line and inline-image bitmaps are the existing software `CWriteableBitmap` surfaces, cached by the renderer's bounded bitmap cache, which releases them after 120 idle frames.
- D2D `InvertRectangle` creates one short-lived command list per caret per frame.
- Measured on the `form` scene (quick run, peak private MB): Composition 46.3, GDI+ 45.5, D2D-SW 60.7. For D2D-SW the earlier full-backend baseline (`full-main`) was 60.6, so text self-draw adds nothing measurable.

### Failure, fallback, and observability

Benchmark runs must report requested mode, effective mode, actual adapter/device type, software fallback, and retained
layer promotions. Unsupported features must be visible. Fail initialization or mark a run unsupported instead of
silently switching to another painter or the existing backend. Production fallback policy remains a separate decision.

Expose promotion reason/count, surface bytes, cache bytes, requested damage, actual repainted area, and resource
creation/reuse counts. A successful launch in an immediate mode is not evidence that the scene avoided retained work.

## DirectUI architectural precedent

A local source investigation of `C:\os\src\windows\AdvCore\DUser` found retained markup/elements and display-node
gadgets combined with immediate paint callbacks, damage-aware traversal, optional caching, and selected retained
DirectComposition layers. This is architectural evidence, not a measured WinUI memory/performance result or a plan
to import DirectUI implementation code. The local source is not a dependency of this repository or its readers.

| Observation | Lesson for this proposal |
|---|---|
| Ordinary invalidation reaches a window paint region and a back-to-front drawing traversal | Retained UI can drive incremental immediate painting |
| Layered gadgets paint into retained surfaces and have a separate dirty-layer update path | Immediate content and retained presentation/motion can coexist |
| Selected scrolling paths layer viewer/content and integrate Direct Manipulation | Immediate painting need not put all motion on the UI thread |
| Layer position/opacity updates can avoid repainting content | Distinguish property changes from pixel-content changes |
| Dirty layers may need updating despite clipping because independent animation can expose them | Culling must consider animation and valid surface coverage |
| Gadgets and cached buffers remain retained | Immediate rendering is not zero retained state; measure all memory buckets |

Borrow these architectural lessons rather than assuming DirectUI's GDI/GDI+ drawing technology provides WinUI's
required fidelity, performance, or feature coverage. Local investigation entry points are recorded in the source
references; no DirectUI source excerpts are included here.

## Memory and performance model

| Memory bucket | Expected treatment and risk |
|---|---|
| Controls, properties, bindings, layout, input/accessibility state | Remain retained; not the primary savings target |
| Fine-grained Composition content and related state | Main reduction hypothesis; existing composition nodes are already selective |
| Traversal metadata, damage structures, snapshots | New or adapted state; must not recreate equivalent retained overhead |
| Images, glyph resources, geometry caches | Still useful; share or bound where appropriate |
| Raster targets, buffering, temporary effect surfaces | May grow enough to outweigh object savings, especially at high DPI |
| Selected retained layers and interop resources | Necessary for chosen contracts; promotion can erase savings |

At four bytes per pixel, a 3840 x 2160 buffer contains about 31.6 MiB of pixel data before padding, additional buffers,
intermediate surfaces, or resource metadata. Holding logical size constant while doubling DPI doubles each physical
dimension and approximately quadruples pixel storage. Real allocations and residency must be measured, not inferred
solely from this arithmetic.

Retained rendering can change visual properties without regenerating content. Immediate rendering trades some
retained object/state cost for traversal, command generation, repaint work, and potentially copies/uploads. Hardware
Direct2D accelerates drawing but does not remove CPU command-generation cost. Software paths can reduce dependence
on hardware content rendering but may increase CPU time, memory bandwidth, and energy.

Do not replace an objective of lower total UI memory with a proxy of fewer visuals, lower app working set, or lower
GPU allocation alone. Report benefits and regressions by workload and backend rather than producing one overall score.

## Evaluation plan

The detailed test cases, metric definitions, and measurement methods are in the
[comparative test plan](immediate-rendering-backends-test-plan.md).

### Common harness

Use one benchmark app with four launch modes, identical XAML/data, deterministic interactions, and explicit
phase markers. Collect correctness evidence and requested/effective backend identity before accepting timing results.
Do not implement the harness as four different applications with different trees or feature omissions.

### Controlled experiments

| Test | Controlled variation | Primary question |
|---|---|---|
| Element-count scaling | 100, 1,000, 10,000 realized elements at fixed viewport; approximately constant painted area | How do memory and traversal scale with structural complexity? |
| Physical pixel-area scaling | Same tree at different physical resolutions and DPI; distinguish fixed-viewport and fixed-logical-size runs | When do surface memory, fill, and bandwidth dominate? |
| Damage scaling | Approximately 1%, 10%, 100% viewport updates; contiguous and scattered changes | Does actual work track damage or degrade to broad repaint? |
| Primitive mix | Text, images, geometry, gradients, clipping tested separately | Which operations dominate each painter? |
| Overlap/transparency | Increase overdraw, nested clips, translucent overlap, group opacity | Are semantics correct and intermediate allocations bounded? |
| Retained-boundary scaling | Increase independently animated/interop boundaries at fixed ordinary content | When do promotion and extra surfaces eliminate savings? |

Measure requested damage separately from actual repainted area. Record realized elements and primitive/layer counts,
not just data-source size. Keep decoding, data generation, and layout costs comparable; use preloaded assets in an
isolation run and retain end-to-end runs for product relevance.

### Representative workloads

| Workload | Script | Key evidence |
|---|---|---|
| Static settings/form | Launch, idle, hover, change one setting | Startup, memory, small updates, idle power |
| Dense dashboard | Update selected indicator subsets at fixed rates | Memory versus update frequency, CPU/GPU work |
| Virtualized text-heavy list | Slow scroll, fling, reverse, append | Tail latency, missed frames, exposure correctness, cache churn |
| Editor/document | Type, select, scroll, caret, IME, mixed scripts | Editing semantics, text fidelity, input-to-present latency |
| Image gallery | Scroll preloaded and normally decoded images, zoom, recycle | Upload/copy bytes, reuse, peak memory |
| Animation scene | Contrast content-changing animation with transform/opacity-only motion | Repaint versus retained-boundary behavior |
| Multiwindow | Open 1, 5, 10 windows; minimize, restore, close | Per-root cost, sharing, reclamation |
| Interop scene | Combine XAML with selected Composition content/video/embedded surfaces | Support, ordering, synchronization, fallback |

During scrolling or a compositor-eligible animation, deliberately stall the UI thread for a bounded interval such as
200 ms. Record motion continuity, newly exposed pixels, and recovery. Do not require UI-thread-dependent operations
such as typing to continue during the stall. This is a diagnostic of independence, not permission to block production UI.

### Correctness and resilience

| Area | Cases |
|---|---|
| Damage | Move/remove overlapping elements, shrink text, change clips, expose covered content, scroll both directions |
| Fidelity | Fractional positions, RTL/mixed scripts, DPI, rounded corners, gradients, images, overlapping translucent children |
| Behavior | Input/hit testing, focus, keyboard navigation, selection, caret, accessibility tree and bounds |
| Lifecycle | Continuous resize, monitor/DPI changes, minimize/restore, occlusion, close/reopen |
| Recovery | Relevant device loss, allocation failure, missing hardware, explicitly selected software mode |
| Stability | Repeated navigation and window creation/destruction; 30-minute update/scroll soak |

Combine structural/behavioral assertions with screenshots. Define targeted text and antialiasing tolerances; neither
exact pixel equality for every backend nor a permissive global screenshot threshold is sufficient. Unsupported cases
must be classified before performance results are compared, not silently skipped or rendered with less content.

### Metrics and methodology

| Dimension | Metrics |
|---|---|
| Memory | App private commit and working set separately; graphics allocation/residency; attributable compositor cost; peak and steady state |
| Resources | Visual/layer count, allocated surface bytes, caches, temporary allocations, GDI handles |
| Responsiveness | Input-to-present p50/p95/p99, frame-deadline misses, longest visible stall |
| Work | CPU time by layout/traversal/command generation/rasterization, GPU time, repaint area, uploads/copies |
| Startup | First correct presented frame and first interactive frame separately |
| Idle/energy | Wakeups, unexpected redraws, CPU/GPU activity, energy per fixed interaction sequence |
| Reliability | Artifacts, failures, recovery duration, resource-growth trend |

Use optimized builds without a debugger. Fix physical viewport, DPI, refresh rate, content, and input schedule for
paired comparisons; vary one factor at a time in scaling tests. Warm up before steady-state sampling and distinguish
process-cold startup from warm-cache startup. Randomize mode order, repeat runs, and report variability.

Attribute memory without double-counting shared allocations. Report CPU memory and GPU allocation/residency separately
and explain any aggregate; private commit and working set are not additive. If compositor attribution is unavailable,
label the measurement incomplete rather than declaring total savings. Use matched system baselines and quiet-machine
runs for compositor/energy comparisons.

Do not use average FPS as the primary metric: idle windows should not redraw continuously, and averages hide stalls.
Record frame deadlines for the actual display refresh rate. Start on integrated- and discrete-GPU machines, then add
CPU-constrained hardware and remote-session testing after the basic hypothesis survives.

Initial deliverables are backend identity/phase events, per-frame work/resource counters, a repeatable harness script,
and a report separating steady state, interaction peaks, and lifecycle recovery. Exact trace providers/tools and
automation integration must be selected during baseline work; this proposal does not claim an existing runner.

## Success criteria and decision rules

Freeze target workloads, supported features, measurement windows, and acceptance budgets before comparing results.
The following are proposed gates, not predictions or a universal requirement that software modes match hardware.

| Dimension | Initial gate |
|---|---|
| Memory benefit | At least 20% lower attributable steady-state UI memory on two preselected target workloads; disclose peaks and high-DPI tradeoffs |
| Target interaction | Meet agreed display-frame budgets; p95 input-to-present no more than 10% worse than baseline on selected interactions; inspect tail stalls separately |
| Idle | No backend-induced periodic repainting or material idle-energy regression |
| Correctness | Pass the agreed supported-feature suite, with no stale pixels or semantic rendering differences |
| Stability | Return to a repeatable resource plateau after warmup and repeated lifecycle cycles; pass the soak/recovery cases |
| Selection integrity | Requested/effective mode and promotions are visible; no undisclosed fallback in measured runs |

Each mode needs a declared intended use case and its own acceptable CPU/energy/latency envelope. Hardware Direct2D
targets memory reduction with competitive interaction performance. GDI+ and software Direct2D remain candidates for
workloads or deployment constraints where their measured tradeoffs are useful; they are not automatically the low-memory
or low-power winners. Apply identical workloads and report all costs even when acceptance budgets differ.

Proceed only if savings survive correct damage handling, required retained boundaries, and essential interactions.
If savings disappear when those contracts are restored, or if costs exceed the target use case's budgets, stop or narrow
that backend. Consider improving the existing backend's retention/caching policy rather than forcing a second path.

## Implementation milestones

| Milestone | Work and deliverable | Exit evidence |
|---|---|---|
| M0: Baseline and contract | Choose target apps/features; instrument memory buckets; define backend identity and comparison protocol | Baseline report, agreed feature matrix and budgets |
| M1: Selection and lifecycle seam | Add startup selection/root policy; establish root strategy dispatch and resource ownership | Existing mode unchanged; unsupported modes fail explicitly; root/lifecycle tests |
| M2: Four-mode drawing slice | Implement narrow shared immediate traversal, GDI+ painter, D2D painter with hardware/WARP resources, and presentation | Correct full redraw for supported primitives in all modes; actual device verified |
| M3: Damage and resource reuse | Add conservative damage, correct buffer preservation, bounded caches, resize/DPI recovery | Sparse-update work reduction without stale pixels or unbounded growth |
| M4: Selected retained boundaries | Implement chosen scrolling/animation boundary and explicit interop policy | Text-heavy scroll plus independently animated overlay, including stall/exposure tests |
| M5: Evaluation and decision | Run common harness; expand only justified compatibility areas; report tradeoffs per mode | Reproducible scorecards, gate results, go/narrow/stop recommendation |

Introduce focused correctness tests with each behavior rather than deferring all regression coverage to M5.
Run element-count scaling, pixel-area scaling, sparse updates, scrolling, and idle across all modes as soon as the
required slices exist. Label full-redraw-only and unsupported-interaction results explicitly; do not treat them as
completed backend comparisons. No delivery dates are assumed before a scoped implementation investigation.

**Prototype readiness.** M1 and the M2/M3 slices needed by the RenderBench S1 (settings) and S2 (dashboard) scenes are
in place:

- `WINUI_RENDERING_BACKEND` selects a mode. It is reported as available only after a lazy probe creates its painter;
  without a hardware adapter, D2D-HW falls back with `DeviceUnavailable`.
- `ImmediateRenderer` (`dxaml\xcp\core\hw`) replaces `HWWalk::RenderRoot` for each island root in
  `CCoreServices::RenderWalk`.
- Each frame, the walk records a per-root display list and diffs element signatures and bounds against the previous
  frame. That produces up to 32 coalesced damage rects, and only commands that intersect the damage are replayed.
- Typical sparse damage per paint: S2 tile updates are about 5% of the window; S1 hover and toggle states are under
  0.1%.
- Composition behavior is unchanged when the variable is unset. The renderer is never created, and the probe is a
  single pointer store.

**Render-loop readiness for S3–S10.** All eight scenes render in GDI+ and D2D-SW (D2D-HW falls back on the GPU-less
lab VM). Captures were checked against Composition on the same machine. Anything skipped or approximated is reported
once per element type and detail as `UnsupportedContent`, and is also written as `# unsupported` lines to the
`WINUI_IMMEDIATE_STATS` file.

| Scene | Status | Render-loop work and remaining gaps |
|---|---|---|
| S3 Inbox | Works | Covers ListView, ItemsPresenter and ScrollViewer content, ellipses and trimmed text. When a scroller's content moved by whole device pixels and its viewport is unchanged, the scroll becomes `ScrollPixels` and only the exposed strip and changed items are damaged. Scrolled content origins are snapped to device pixels, as DManip does. Subtrees whose inner bounds miss the clip are culled. |
| S5 Gallery | Works | Images are drawn from the decoded software surface: premultiplied BGRA goes through `EnsureBitmap` (key is the surface identity) and `DrawBitmap`. Bitmaps that go unused for a few frames are released. Image elements and `ImageBrush` are supported, and so are full matrix transforms such as the hover `ScaleTransform`. |
| S6 Animated | Works | Animations that would be independent are forced to run as dependent animations on the UI thread, so each tick changes property values and damages only the animated bounds. `AnimatedIcon` (the CheckBox and Expander chevron glyphs) uses its `FallbackIconSource` in immediate modes. Expander's collapse clip is a Composition `InsetClip` on a hand-off visual (`GetElementVisual`); the walk applies its insets to the element's render size, because no comp node keeps the visual's Size in sync. Partial: ProgressRing's Lottie `AnimatedVisualPlayer` is hand-in Composition content, so it is skipped and reported as `Hand-in Composition visual`; use `--no-progressring`. |
| S4 Reader | Works | `TextBlock` and `RichTextBlock` text, including the S1 and S2 text, is drawn with `DrawGlyphRun` from XAML's own DirectWrite layout, which fixes the earlier measurement mismatch. Selection highlights and Hyperlink colors are included. `DrawText` remains the fallback. GDI+ glyph-run painting is slow: a full Reader frame takes about 420 ms. D2D-SW glyphs sit about 1 px lower than Composition. |
| S7 Layered | Works, with the effects policy below | Group opacity uses `PushOpacityLayer`; opacity is folded into a primitive only when the group draws exactly one primitive. CornerRadius-clipped content uses `PushRoundedRectangleClip`. Path, Line, Polyline and Polygon geometries are flattened to cubic Béziers; dash arrays are not drawn. |
| S8 Grid | Works; performance is partial | Covers ItemsRepeater rows, the frozen header, multiple scrollers and two-axis `ScrollPixels`. The incremental walk reuses the previous frame's commands for clean subtrees (described below). On sparse 10 Hz cell updates in GDI+, it visits about 450 elements per frame instead of about 1,900, and the walk takes 1.3 ms instead of 3.9 ms. Animated `ChangeView` scrolls arrive as one jump, because DManip/InteractionTracker drives the intermediate offsets in Composition and the UI thread only sees the final offset. |
| S9 Multiwindow | Works | Each island root gets its own painter and swap chain, created on its first paint. D2D modes share one device per process; GDI+ shares the GDI+ runtime and its presentation device. Closing a window prunes its island state (swap chain, painter, bitmaps, previous display list). Minimized windows keep their resources until they close. With `--windows 4`, peak private bytes were 82 MB in GDI+ and 87 MB in D2D-SW, falling to about 30 MB after closing all windows; Composition stayed at 81 MB. The TabView variant works. |
| S10 Form | Works, with approximations | The TextBox caret is painted as a filled rectangle; it blinks because its visibility changes. The caret uses `CompositeMode=DestInvert`, which is approximated with the TextBox foreground and reported as `CompositeMode DestInvert:approximated`. Focus visuals and hover states render. Open popups (ComboBox dropdown, Flyout, ToolTip) are painted from `PopupRoot`'s open-popup list on top of the island content, oldest first. Windowed popups are painted into the island's swap chain, so any part outside the window is clipped (reported as `Windowed popup:painted in island`). |

**Render-loop readiness for the full-backend scenes (S11–S22).** Measured with `-Quick -Repetitions 1` in GDI+ and
D2D-SW; all gates pass except where noted.

| Scene | Status | Render-loop work and remaining gaps |
|---|---|---|
| S11/S16 Fallback | Works | `ImmediateRenderer.Fallback.cpp` checks each walked element (hand-in visual, touched or implicitly animated hand-off visual, ConnectedAnimation, SwapChainPanel, MediaPlayerElement/Presenter, app-source AnimatedVisualPlayer). The island falls back permanently: its swap chain, painter and display lists are released, the subtree is dirtied and `HWWalk::RenderRoot` renders it with `forceRedraw`. Other islands stay immediate. Traced as `WindowFallback` and written as `# fallback` to the stats file. The frame that detects the fallback also reports the element as unsupported. In S16 the harness's `FramesObserved` gate fails after fallback because it counts only immediate frames. |
| S12 Lottie | Works, approximated | The built-in ProgressRing player is drawn natively (an arc). AnimatedIcon draws its `FallbackIconSource` and is reported as `AnimatedIcon:approximated` (now also seen in S1, S6 and S13). An AnimatedVisualPlayer with an app source causes a per-window fallback. |
| S13 Materials | Works, approximated | LinearGradientBrush uses the gradient primitives (relative or absolute mapping, brush transform, opacity); gradient strokes still use the average color. ThemeShadow is drawn as an ambient plus a directional rounded shadow whose blur scales with Z, before the element, with inflated damage. AcrylicBrush is drawn as `BlurBackdrop` (30 DIP) plus luminosity and tint, with backdrop damage expansion; noise is skipped. Shadows are drawn outside the element's opacity layer. Mica shows through the transparent swap chain. |
| S14 Windowed popups | Works | Each open windowed popup gets its own painter, swap chain and sprite visual on the popup's island visual, released on close. Time from open to first frame is written as `# popup-first-frame`. Measured open-to-frame: about 18–59 ms in GDI+ and 24–52 ms in D2D-SW, against 9–38 ms in Composition. RTL popups are mirrored. |
| S19 Text | Works | Glyph runs use color fonts. ClearType is enabled when an earlier axis-aligned opaque fill in the same layer fully covers the run. RichTextBlockOverflow and PasswordBox (mask or revealed text) take the `DrawGlyphRun` path. RichEditBox text is still unsupported (hosted ITextServices surface). |
| S22 RTB and focus | Works | `RenderTargetBitmap` asks the immediate renderer, through `RenderingBackend::SetCaptureElementCallback`, to paint the element subtree into an offscreen painter of the same mode and copies the pixels into the RTB texture. `RtbSuccessRate` is 1 in all modes; `CaptureMeanAbsDiff` is ≤ 0.002. Focus rects are real CBorders in a canvas that is a "secret" child of the focus target and is shown through an LTE in the host's TransitionRoot. The walk paints those LTE targets in place. Legacy dotted focus rects (`FocusVisualKind.DottedLine`) are not drawn. |

**Effects policy (immediate modes).**

- The swap chain is premultiplied and transparent wherever nothing is painted, so a system backdrop such as Mica still
  shows through behind the content.
- `AcrylicBrush` is drawn with a blurred backdrop. Other `XamlCompositionBrushBase` brushes paint their fallback color
  and are reported as `XamlCompositionBrush:fallback`.
- `ThemeShadow` is drawn as two blurred rounded-rectangle shadows.
- Projections and non-rectangular clips are skipped and reported. Hand-in Composition visuals cause a per-window
  fallback to Composition.

**Incremental walk.** The renderer clears dirty flags (`PropagateNWClean`) after every immediate frame, so a subtree
without render-dirty flags produces the same output as the previous frame if it is walked in the same parent context.
That context is the world transform, the clips, the scale, the scroller and the scroller content origin.

- For each subtree with children, the walk records the ranges it produced in the display list: commands, strings,
  glyph runs, paths, bitmaps, element records and nested spans.
- On the next frame, a clean subtree in the same context moves those ranges across instead of walking.
- Three cases are always walked again: content directly inside a scroller (it records the scroller offset), subtrees
  that contain popups, scrollers or hand-off visuals (their Composition clip changes don't set render-dirty flags), and
  commands where an ancestor's single-primitive opacity group was folded in.
- `WINUI_IMMEDIATE_FULL_WALK=1` turns reuse off for comparison. The `reused` stats column counts reused subtrees.
- Scrolling changes the world transform of everything inside the scroller, so scrolled content is still walked again.
  Reuse helps sparse updates, hover and animation the most.

**Building.** `AnimatedIcon`'s immediate-mode fallback lives in Microsoft.UI.Xaml.Controls.dll, which must be built
separately from Microsoft.ui.xaml.dll.

These are traced as `UnsupportedContent`:

- Composition-brush effects other than AcrylicBrush (only the fallback color is painted).
- Projections.
- Non-rectangular clips.
- RichEditBox text.
- Approximations, which are tagged `:approximated` (AnimatedIcon, `CompositeMode DestInvert`).

Content that needs Composition interop is traced as `WindowFallback` instead.

Retained boundaries (M4) remain outside the prototype.

## Risks and open decisions

| Decision or risk | Current direction / evidence still needed |
|---|---|
| Product contract | Start as opt-in with explicit feature scope; full transparent compatibility is not promised |
| Public selector placement | Window/island host before root resource initialization; exact API and late-call error behavior unresolved |
| Per-root mixed devices | Root ownership is the goal; validate shared-device, asset-sharing, and presentation constraints first |
| Layer granularity/promotion | Measure count, surface area, reason, and lifetime; establish explicit boundaries before automatic heuristics |
| GDI+ fidelity and presentation | Prove antialiasing, alpha, GDI+ text, DPI, copy/upload cost, and host integration rather than assuming parity; allow a slightly looser pixel tolerance than Direct2D |
| Text rendering | Reuse layout/shaping where possible; specify text quality and editing/IME contracts across painters |
| Threading | Initial UI-thread command generation is acceptable for experimentation; render-thread snapshots require a separate costed decision |
| Advanced effects and interop | Classify supported/promoted/unsupported; ordering and group semantics can require more than local promotion |
| Capture/printing | Audit RenderTargetBitmap and printing separately; live-render backend selection does not automatically implement those paths |
| Popups/additional roots | Define inheritance or explicit policy for extra presentation roots and their lifetime |
| Production fallback | Decide notification and fallback rules separately from strict benchmark selection |
| Maintenance cost | Require reusable traversal/resource infrastructure and workload benefits sufficient to justify multiple painters |

The next executable decision is to agree on the first two memory-target workloads and the must-preserve scrolling,
text, animation, and interop contracts. Then establish the M0 baseline before committing to broad compatibility work.

### Prototype implementation decisions

| Topic | Decision | Rationale |
|---|---|---|
| GDI+ text | The GDI+ painter draws text with GDI+ (grayscale antialiasing on the PARGB bitmap), not DirectWrite | Keeps the GDI+ backend a pure GDI+ data point |
| gdiplus.dll loading | Delay-load gdiplus.dll in Microsoft.ui.xaml.dll | Composition and D2D processes never load it |
| Presentation | All immediate modes present through one composition swap chain per root, hosted in a single Composition visual. Every mode paints into its own persistent target and copies only damaged pixels into the back buffer, then calls `Present1` with those dirty rects. For GDI+ and D2D-SW that copy is the W5 upload; for D2D-HW it is a GPU-local copy on the same adapter (reported separately, not as an upload) | Same presentation path for every mode isolates painter cost, keeps islands, popups, and backdrops working, and makes copy bytes (W5) comparable. A persistent target keeps partial damage correct without tracking flip-model buffer history; drawing D2D-HW straight into the back buffer is a later optimization to measure, not a prototype requirement |
| CPU attribution | The analyzer attributes ETW CPU samples by the modules on each sample's stack (for example d3d10warp → WARP raster, gdiplus → GDI+ raster, dwrite → text, Microsoft.ui.xaml → XAML), instead of relying on thread start modules alone | WARP uses thread-pool threads and GDI+ runs on the UI thread, so a thread-based split cannot separate raster cost |
| Fidelity references | No checked-in golden images. C1 compares each immediate mode against Composition rendered on the same machine and OS build, with per-region tolerances; exact equality is required only between two runs of the same mode (determinism) | Rasterizer DLL updates change antialiasing slightly; same-machine references need no maintenance |
| Render-loop dispatch | In `CCoreServices::RenderWalk`, each active island root goes to `ImmediateRenderer::RenderIsland` instead of `HWWalk::RenderRoot`, and its dirty flags are then cleared with `PropagateNWClean`. Paint failures are logged and retried on the next dirty frame; device-lost releases the renderer's resources. The root visual (outside islands) still uses HWWalk | Keeps frame scheduling, layout, and dirty propagation unchanged. A failure in the immediate path must not reach the render walk's fail-fast |
| Composition content suppression | HWWalk never visits an immediate island, so no comp nodes, comp peers, or visual content are created for its subtree. The island's root visual is replaced with one `SpriteVisual` whose brush wraps the swap chain through `DCompTreeHost::CreateCompositionSurfaceForSwapChain` (the compositor interop used by `SwapChainPanel`). That swap chain is `CreateSwapChainForComposition`, BGRA, premultiplied, flip-sequential, 2 buffers, physical size | The memory under test is what Composition would have retained per element. Hit testing and input use the XAML tree, so they keep working |
| Devices | D2D modes use `CreateD3DDeviceForMode`, and the painter's swap chain is created on that device. GDI+ creates its own BGRA-capable D3D11 device (hardware, else WARP) only for presentation | Each painter's presentation device stays independent of WinUI's device lifetime |
| Damage | Each element gets a signature (an FNV hash of its emitted commands) plus its clipped device bounds. When an element is added, removed, or changes, both its old and new bounds are damaged. More than 256 raw rects means full damage. Otherwise rects are clipped to the surface and merged into at most 32 disjoint rects by smallest-area growth. Full damage also applies on first frame, resize, DPI change, and after a failure | Sparse dashboard updates stay sparse without tracking per-property invalidation |
| Walk approximations | Opacity is folded into primitive alpha only when an opacity group draws exactly one primitive; otherwise it uses `PushOpacityLayer`. Clips are world-space axis-aligned bounding boxes, plus rounded clips for CornerRadius. Gradients are painted as their average stop color. Text is drawn from XAML's glyph runs. `DrawText` is the fallback, using the arranged rect: no-wrap text gets 2 em of slack and wrapping text gets ½ em | Each approximation is a known fidelity gap rather than a crash or a silent omission |
| Diagnostics | `WINUI_IMMEDIATE_CAPTURE=<path.bmp>` writes the most recently presented swap-chain buffer after each paint. `WINUI_IMMEDIATE_STATS=<path.txt>` writes one line per painted island frame, with element counts (visited, painted, culled), commands, blits, damage, walk and paint time in µs, and reused subtrees, followed by `# unsupported` lines. `WINUI_IMMEDIATE_PRESENT_UNION=1` also repaints the previous frame's damage. Mid-scroll captures showed it is not needed: flip-model `Present1` dirty rects carry the rest of the frame forward | Screen capture of a disconnected remote session returns black, so this is the C1 source on headless lab VMs |



Branch `scrolling` (F9). In immediate mode, scrolling and zoom run on the UI thread. Composition never drives content motion.

| Area | Decision | Rationale |
|---|---|---|
| DManip transforms | In WinUI 3, the shared DComp *manipulation compositor* ticks DManip viewports (pan, inertia, `ZoomToRect` animations); `CCompositorDirectManipulationViewport::UpdateTransform` has no callers. The immediate walk reads the scroller's XAML offsets and zoom (`ScrollContentPresenter` transform and zoom factor). It applies pure translations with a `ScrollPixels` blit plus exposed strips. Any zoom change repaints the whole viewport. The blit decision lives in `ImmediateRenderingPolicy` and has native tests | Offsets that XAML already knows render without the manipulation compositor. Content scaled by zoom cannot be blitted |
| Animated `ScrollViewer.ChangeView` | In immediate mode, a 400 ms cubic ease-out UI-thread tween drives `ChangeView(..., disableAnimation:false)` and `ChangeView(h, v, z)`. A 16 ms `DispatcherTimer` issues non-animated `ChangeViewInternal` steps inside one intermediate-ViewChanged section, and the last step applies the requested values with mandatory snap points. Zoom steps go through an unanimated DManip bring-into-viewport that completes asynchronously. The tween waits for each step's completion, with a watchdog of half the duration, and keeps the section open. A user DManip manipulation cancels the tween | DManip animations would otherwise jump in one frame. ViewChanging and intermediate/final ViewChanged events are preserved |
| ScrollPresenter / ScrollView / ItemsView | Content `Translation`/`Scale` facades are set on the UI thread from InteractionTracker `ValuesChanged` (`ApplyImmediateContentTransform`), replacing the translation and scale expression animations. Animated `ScrollTo`/`ScrollBy`/`ZoomTo`/`ZoomBy` are stepped on the UI thread with non-animated `TryUpdatePosition`/`TryUpdateScale`, one per `CompositionTarget.Rendering` tick. The animated operation is re-keyed to each step's request id and completes (`ScrollCompleted`/`ZoomCompleted`, state back to Idle) when the final step's `ValuesChanged` arrives, or after 8 ticks if the view does not move | Without a Composition frame loop, the compositor never finishes tracker custom animations (no `IdleStateEntered`), so completion events never fired. `ScrollAnimationStarting`/`ZoomAnimationStarting` still fire, but only the duration of their animation is honoured |
| SwipeControl, RefreshContainer/RefreshVisualizer, ParallaxView, AnnotatedScrollBar, Pivot headers | Their expression animations (swipe content offset, pull offset and indicator rotation, parallax source offset, scrollbar indicator, Pivot header offset) are evaluated on the UI thread from the tracker or ScrollViewer values, and applied through XAML transforms the walk paints | No retained Composition layers per the no-hybrid rule; the cost is a few UI-thread property sets per frame |
| Not animated or untested | Wheel (pure-inertia DManip input), touch and touchpad pan, pinch zoom, inertia, and overpan/bounce all run through DManip, so in immediate mode they apply final or per-contact transforms without inertia frames. These paths are untested on the lab VM: WinRT `InputInjector` needs a capability RenderBench lacks, and Win32 `SendInput`/`InjectTouchInput` fail in a disconnected session. The `ScrollViewer-MouseWheel` and `ScrollViewer-TouchPan` pages compare the unscrolled view when injection is unavailable. Keyboard and internal `BringIntoViewport(animate)` calls are not covered by the tween and jump | A UI-thread inertia model (or ticking the manipulation compositor from the render loop) is the follow-up |
| Memory | Per ScrollViewer: a tween state under 100 bytes and one lazily created `DispatcherTimer`. Per ScrollPresenter: a tween state under 100 bytes and one `Rendering` handler, held only while a tween runs. No surfaces or caches. Measured on S3 Inbox (continuous scrolling; `-Quick`, programmatic, VM without a GPU) with this branch: Composition peak 58.0 MB (phase medians 55.7–57.9), GDI+ peak 61.8 MB (56.1–60.3), D2D-SW peak 76.6 MB (69.2–73.7). No unsupported content and no fallback | Bounded; nothing is retained per frame. The D2D-SW overhead comes from WARP and the swap chain, not from scrolling state |

### Full XAML coverage: clips3d

This covers families F3 (clips and transforms) and F4 (3D). It replaces the "Walk approximations" row for clips: clips are no longer world-space axis-aligned bounding boxes.

**Painter contract.** Two pairs of methods were added in the `// ---- clips3d ----` block of `ImmediatePainter.h`:

- `PushGeometryClip` / `PopGeometryClip` take device-space path data (figures of points) and clip antialiased.
  - D2D: a layer with a geometric mask.
  - GDI+: region clips are aliased, so the content is drawn into a scratch layer, masked by an antialiased fill of the path, and then composited.
- `PushPerspectiveLayer` / `PopPerspectiveLayer` take a device-pixel region and a 4x4 matrix. The subtree is drawn into an offscreen target covering that region, which is then drawn with the matrix.
  - D2D: `DrawBitmap` with `D2D1_MATRIX_4X4_F`.
  - GDI+: GDI+ has no perspective, so on pop the layer is resampled per pixel through the inverse homography (bilinear, premultiplied) into the shadow scratch, then copied 1:1 under the outer clip. This is exact up to filtering, so no mesh approximation is needed.

D2D keeps up to 4 target-sized perspective bitmaps (one per nesting depth) only while consecutive frames use them; a frame that doesn't use a slot releases it. GDI+ uses its layer stack and the existing shadow scratch, which is released when it grows unusually large. There are 13 conformance cases in `ImmediatePainterClips3DTests`, run in every mode. They cover:

- pixels inside, outside and on the antialiased edge of a geometry clip;
- rotated, nested, empty and popped clips;
- perspective corner mapping for an identity, an affine and a depth matrix;
- perspective layers combined with push transforms, clips, damage, DPI scale and nested opacity.

**Walk** (`ImmediateRenderer.Clips3D.inl` plus the walk section of `ImmediateRenderer.cpp`):

- **Clips:**
  - A rectangle clip under a scale-and-translate world stays on the fast axis-aligned path.
  - Any other clip becomes a geometry clip group plus a conservative bounds rectangle for culling and damage:
    - rectangles under rotation or skew;
    - `RectangleGeometry` with a `Transform`;
    - rounded rectangle clips;
    - hand-off visual clips.
  - Element clips are pushed inside the opacity or independent-transform layer, so render-thread transforms move them with the content.
  - Ancestor layout clips are pushed in parent space.
- **3D:**
  - Each element's local 4x4 matrix is `handoff · Projection · Transform3D · facade 3D (Rotation, RotationAxis, Scale.Z, Translation.Z, TransformMatrix) · local 2D`. It is built with the same core helpers HWWalk uses (`LocalTransformBuilder`, `CProjection`, `CTransform3D`).
  - The world matrix is kept unflattened, as Composition does. That makes nested Transform3D render correctly.
  - When the combined homography's w is constant to float precision over the element bounds (no perspective divide), it is drawn as a plain 2D transform and needs no layer.
  - Otherwise a perspective layer is opened. Its region is the element bounds intersected with the inverse-mapped clip, snapped to device pixels.
  - Damage and culling use the projected quad.
  - Inside a layer, these are disabled:
    - scroller blits;
    - independent render-thread probes;
    - backdrop blur (GDI+);
    - ClearType (text is drawn grayscale).
- **Hand-off visual statics** (`ImmediateRenderer.HandOff.*`): the app-set static properties of `ElementCompositionPreview.GetElementVisual` are read and painted natively:
  - Offset, Size, Scale, RotationAngle, RotationAxis, CenterPoint, TransformMatrix, Opacity;
  - InsetClip and RectangleClip (including corner radii);
  - GeometricClip with a rectangle, rounded rectangle or ellipse geometry.

**Fallback policy.** `IsHandOffVisualTouched` now falls back only for these cases:

- an active app Composition animation on the hand-off visual or its clip, detected with `ICompositionObject4::TryGetAnimationController` per animatable property (animations on template parts of built-in `Microsoft.UI.Xaml.Controls` controls, such as NavigationView's selection indicator, paint their static values instead);
- a clip that can't be read back (a `CompositionPathGeometry` or a custom geometry);
- a facade animation (`StartAnimation` on Translation, Scale and other UIElement facades).

An app-created child visual (`SetElementChildVisual`) still falls back through its own hand-in rule.

Policy unit tests cover static properties (no fallback), an animated property (fallback) and a path clip (fallback).

**What still falls back, approximates or differs:**

- **Not detected:** ExpressionAnimations on the hand-off visual can't be detected, so they paint their current static value.
- **Approximated:**
  - A perspective layer larger than the window is clamped to the window size and reported as `Perspective layer larger than the window:approximated`.
  - Backdrop materials and ClearType inside a perspective layer are approximated.
- **Composition-mode differences that pages must avoid:**
  - XAML itself writes the hand-off visual's Offset, TransformMatrix and Opacity, so in Composition an app value set before layout can be overwritten. The coverage pages place such elements at layout offset 0.
  - In Composition, changing a static hand-off property after the first frame doesn't necessarily schedule a XAML frame.

**Fidelity.** These pages were run on the same machine and compared with Composition, as mean absolute difference per channel (0-1):

- F3-clips: rotated rect clip, rotated clip on a rotated parent, nested clips under skew, layout clip under rotation, and hand-off ellipse, rounded-rect and rectangle clips.
- F4-3d: PlaneProjection RotationY 30 and 60, and XYZ with offsets; Matrix3DProjection; nested PerspectiveTransform3D and CompositeTransform3D; 2D facades; RotationAxis Y; TransformMatrix with perspective; hand-off Offset, Scale and Rotation; hand-off Opacity with RotationAxis.

Every page measured at most 0.0005 in GDI+ and D2D-SW. The differences are antialiasing at edges, so the pages gate at 0.005. `handoff-path-clip` falls back as expected (`# fallback Grid Hand-off Composition visual`).

**Memory.** No per-element retained surfaces were added. The extra memory is the D2D perspective slots described above, which are bounded and released after any frame that doesn't use them, and transient GDI+ layer surfaces for antialiased geometry clips and perspective layers. A quick `form` run (programmatic input, 1 repetition) measured peak private bytes of 47.0 MB for Composition, 44.9 MB for GDI+ and 61.7 MB for D2D-SW. Earlier main-line runs of the same scene measured 45.5 MB (GDI+) and 60.6-66.3 MB (D2D-SW), so these numbers are within run-to-run noise.


### Full XAML coverage: transitions

Branch `transitions` (test-plan family F10). Rule: an animation XAML or MUXC *built itself* (known parameters) is
evaluated by XAML and rendered natively; an animation the *app* built on a Composition object (UIElement.StartAnimation
with an app CompositionAnimation, implicit show/hide animations, ElementCompositionPreview hand-off visuals) can't be read
back (no key-frame getters) and keeps the per-window Composition fallback. (Integrated with clips3d: static hand-off
visual opacity, transform and clip are painted; only running app animations on them fall back.)

- **Property and brush transitions.** `ScalarTransition`/`Vector3Transition` (Opacity, Rotation, Scale, Translation)
  and `BrushTransition` (Panel/Border/ContentPresenter Background) are XAML's own: in immediate modes the facade no
  longer creates Composition animations; `ImmediateAnimations::TransitionStore` (components\base, XAML-type free) records
  from/to, duration (clamped like SharedTransitionAnimations) and curve (cubic bezier 0.8,0,0.2,1 for property
  transitions, linear RGB for brushes). A retarget starts from the current animated value (Composition's StartingValue),
  `Vector3Transition.Components` masks snap unmasked components. The walk reads the animated value per element, forces a
  non-reused subtree walk, damages old and new bounds, and requests frames while the store is non-empty. Entries are a
  few dozen bytes and are pruned when finished or when the element leaves the tree. `HasFacadeAnimation()` now only
  triggers fallback for app `StartAnimation`.
- **ConnectedAnimation.** `ConnectedAnimationService` animations are emulated: `Prepare` captures the source element
  through the immediate capture callback (S22) into a bounded snapshot (<= 8 MP, <= 8192 px per side, i.e. <= 32 MB,
  released at Reset/completion), hides the source like the Composition path, and `TryStart` computes the same geometry
  as `StartSpriteAnimations` (offset/scale from source to destination, Gravity dip and shadow lift, Direct/Basic curves
  read from the configuration's cubic-bezier or linear easing; any other easing uses the default curve and reports
  `:approximated`). The renderer paints the snapshot at the `ConnectedAnimationRoot` with the animated host transform,
  inset clip and theme shadow, then walks the destination live on top with the overlay opacity; the destination is
  hidden from its normal position until completion. Coordinated elements move with the host. `Completed` fires at the
  configured duration. The cross-fade is a two-layer opacity blend of snapshot and live destination rather than
  Composition's surface cross-fade: reported as `ConnectedAnimation cross-fade:approximated`; custom component
  animations (`SetAnimationComponent`) are reported approximated and use the default curve.
- **Theme transitions/animations, VisualState transitions.** These are Storyboards on XAML properties (LTEs, transition
  roots, RenderTransform/Opacity/Projection); they already ran as dependent animations. Coverage pages hold each at
  mid-progress (Seek+Pause) and compare with Composition. Gap found and fixed: theme animations built by
  ThemeGenerator (DrillIn, PVL entrance/exit) animate `TransitionTarget.Opacity`, which Composition multiplies into the
  visual opacity; the walk now uses `GetOpacityCombined()` and the render-thread opacity track keeps that factor.
- **MUXC controls.** `controls\dev\inc\ImmediateRendering.h` (`IsRequested()` reads `WINUI_RENDERING_BACKEND`;
  `KeyFrames` builds a DoubleAnimationUsingKeyFrames with Linear/Spline(=cubic bezier)/Discrete(=single step) frames and
  Composition-style delays; `EnsureCompositeTransform` never replaces an app RenderTransform). Controls switch to that
  path only when immediate rendering is requested, so Composition behavior is unchanged:
  - NavigationView selection indicator: same key frames, durations and beziers on a CompositeTransform/Opacity
    Storyboard; the overflow button's implicit Offset animation is skipped (the button moves without animation).
    Segments without an explicit easing use linear (Composition's implicit default key-frame easing is not exposed;
    treated as an approximation).
  - TeachingTip open/close: same Scale key frames and beziers about the elements' CenterPoint; the content elevation
    (Translation.Z, which only sizes the ThemeShadow) is set at the start of expand and the end of contract instead of
    being animated.
  - ItemsView/LinedFlowLayout item transitions (add slide-in/scale-in, remove scale-out, move translate or
    scale-out/scale-in): same durations, delays and beziers as Storyboards; elements are left untransformed after the
    transition. ItemsRepeater with an app `ElementAnimator`/`ItemCollectionTransitionProvider` remains app code.
  - RatingControl: the per-star scale expression has constant inputs, so it is evaluated once and applied as a
    RenderTransform (the control no longer touches its hand-off visual, which made every RatingControl fall back).
  - Expander: the content clip (a Composition inset clip on a hand-off visual) becomes a RectangleGeometry clip kept in
    sync with the size.
  - Already immediate-aware or Storyboard-based: AnimatedIcon (FallbackIconSource), InfoBar, TabView, BreadcrumbBar,
    ProgressBar, PipsPager, CommandBarFlyout (VisualState storyboards; its SystemBackdrop link is materials scope).
    Still Composition-only (fallback): ScrollPresenter/ParallaxView/PullToRefresh/SwipeControl expression animations
    (scrolling scope), AnimatedVisualPlayer, Lights, SystemBackdropElement, AnnotatedScrollBar, InkCanvas/InkToolbar,
    WebView2.
- **Diagnostics.** `WINUI_IMMEDIATE_ANIMATION_HOLD=<0..1>` holds every XAML-evaluated animation (transitions,
  ConnectedAnimation) at that progress and never completes it, so mid-animation captures are deterministic.
- **Limitations.** The S15 render-thread ticker does not tick store transitions or ConnectedAnimation (they advance
  on UI-thread frames only); MUXC Storyboards are dependent animations (UI thread), like all Storyboards in immediate
  modes.- **Results.** F10 coverage (`Run-Coverage.ps1 -Pages 'F10*' -Modes composition,gdiplus,d2d-sw`): 32 pages; every page
  except a harness-only crash (programmatic `ToggleSwitch.IsOn`, 0xC000027B in Composition too; replaced by a generated
  VisualTransition page) passes in GDI+ and D2D-SW with no fallback. Expected unsupported details: the ConnectedAnimation
  cross-fade and AnimatedIcon's PathIcon (CheckBox glyph, Expander chevron, NavigationView pane button). ItemsView
  itself still falls back through its ScrollView (scrolling scope); its LinedFlowLayout item transitions are covered on
  a bare ItemsRepeater.
- **Memory.** No per-element retained state beyond the transitions store (a few floats per running transition, freed on
  completion). ConnectedAnimation holds one snapshot bitmap per running animation (bounded to 32 MB, released on
  completion or cancel). Idle private bytes per coverage page (in-process; Composition's surfaces live in DWM):
  ConnectedAnimation-Gravity 39.5 MB Composition / 53.7 GDI+ / 60.1 D2D-SW versus ~48-49 MB for plain transition pages
  in the immediate modes, i.e. the snapshot path costs ~5-11 MB of transient heap.

## Source references

These are investigation entry points, not permanent line-number contracts. Paths are relative to the repository unless
an external local root is explicitly stated. No code changes or benchmark runs are represented by these references.

### WinUI source and documentation

- `docs\design-notes\rendering.md`: rendering architecture background; heed its historical/outdated-section notes.
- `dxaml\xcp\components\ContentRoot\inc\ContentRoot.h`: root-specific ownership and host relationships.
- `dxaml\xcp\core\dll\xcpcore.cpp`: island root iteration/retargeting and `HWWalk::RenderRoot` calls, around line 7146.
- `dxaml\xcp\core\hw\hwwalk.cpp`: selective composition nodes around line 891; renderer construction around line 2132.
- `dxaml\xcp\core\hw\ContentRenderer.h`: existing `IContentRenderer` abstraction.
- `dxaml\xcp\core\hw\VisualContentRenderer.h`: content visuals and their recycling.
- `dxaml\xcp\core\compositor\windowrendertarget.h`: target ownership and rendering lifecycle interfaces.
- `dxaml\xcp\core\compositor\windowrendertarget.cpp`: initialization/cleanup/rebuild, around line 149 onward.
- `dxaml\xcp\plat\win\desktop\D3D11DeviceInstance.cpp`: shared-device and hardware/WARP behavior.

### DirectUI local investigation

The following references require access to the separately supplied local tree
`C:\os\src\windows\AdvCore\DUser`; they are not repository links or publicly available dependencies.

- `DirectUI\Engine\Parser\XMLParser.cpp:4120`: element instantiation, children, and property playback.
- `DirectUI\Engine\Core\Element.cpp:176`: display-node creation; `:3331`: paint callback dispatch.
- `DirectUI\Engine\Core\Render.cpp:1414`: element painting.
- `Engine\Lava\HWndContainer.cpp:260`: ordinary invalidation; `:560`: layered scheduling; `:1171`: paint handling.
- `Engine\Core\GPaint.cpp:714`: layered drawing; `:990`: damage intersection/traversal;
  `:1590`: layer/container invalidation; `:1778`: layered opacity handling.
- `Engine\Core\RootGadget.cpp:711`: dirty-layer traversal and independent-animation exposure considerations.
- `Engine\Core\GRect.cpp:305`: layered size changes versus position-only commits.
- `DirectUI\Engine\Control\TouchScrollViewer.cpp:2865`: selected layered scrolling/Direct Manipulation setup.
