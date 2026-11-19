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
| `ImmediateGdiPlus` | Damage-driven immediate path | GDI+ on the CPU; text through DirectWrite |
| `ImmediateD2DHardware` | Damage-driven immediate path | Direct2D on a hardware-backed D3D device |
| `ImmediateD2DSoftware` | Damage-driven immediate path | Direct2D on a software device, initially WARP |

These names are illustrative, not existing APIs or finalized public API names. The two Direct2D modes should share
one drawing implementation, with different device/resource creation. GDI+ needs a separate painter, sharing the
immediate traversal and damage infrastructure where semantics permit.

Plain GDI is not a candidate: it lacks antialiased geometry and per-pixel alpha, which Fluent styles rely on for
rounded corners and translucent fills. The GDI+ painter's contract is:

- Use `SmoothingModeAntiAlias` for geometry and premultiplied 32-bit ARGB (`PixelFormat32bppPARGB`) bitmaps.
- Render text through DirectWrite (for example `IDWriteBitmapRenderTarget`) rather than GDI+ text, so glyph
  shaping and layout match XAML's text stack.
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
| Presentation | All immediate modes present through one composition swap chain per root, hosted in a single Composition visual. GDI+ and D2D-SW copy only damaged pixels into the back buffer; D2D-HW draws into it directly | Same presentation path for every mode isolates painter cost, keeps islands, popups, and backdrops working, and makes copy bytes (W5) comparable |
| CPU attribution | The analyzer attributes ETW CPU samples by the modules on each sample's stack (for example d3d10warp → WARP raster, gdiplus → GDI+ raster, dwrite → text, Microsoft.ui.xaml → XAML), instead of relying on thread start modules alone | WARP uses thread-pool threads and GDI+ runs on the UI thread, so a thread-based split cannot separate raster cost |
| Fidelity references | No checked-in golden images. C1 compares each immediate mode against Composition rendered on the same machine and OS build, with per-region tolerances; exact equality is required only between two runs of the same mode (determinism) | Rasterizer DLL updates change antialiasing slightly; same-machine references need no maintenance |

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
