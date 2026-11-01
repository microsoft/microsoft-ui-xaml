# Issue #7198: PathGeometry with TranslateTransform fails to render

Issue: https://github.com/microsoft/microsoft-ui-xaml/issues/7198

## Symptom

A Path whose geometry has `Transform = new TranslateTransform()` (X=0, Y=0) draws nothing.
Setting X to 0.01 makes it draw. Any transform that evaluates to identity has the same problem
(`ScaleTransform()`, identity `MatrixTransform`, `RotateTransform()`, and so on).

On some render paths it doesn't just vanish, it crashes (AV).

## Root cause

`CGeometry::GetPrintGeometry(cp, printParams, ppGeometry)` turns a core Geometry into a D2D
(PAL) geometry, with `Geometry.Transform` applied.

Before the fix (geometry.cpp, the block right after `IFCPTRRC(pGeometry, E_POINTER)`):

| Case | What happened |
|------|---------------|
| No `Transform` | `SetInterface(pTransformedGeometry, pGeometry)`. Good. |
| `Transform` set, non-identity | `CreateTransformedGeometry(...)`. Good. |
| `Transform` set, identity | Neither branch ran. `pTransformedGeometry` stayed NULL. The function returned **S_OK with a NULL geometry**. |

So "identity transform" was treated like "no geometry at all".

## Who gets the NULL, and what it did

| Caller | Before the fix |
|--------|---------------|
| `CShape` mask/print render, shape.cpp:375 and :387 | `if (pPrintGeometry != nullptr)` (shape.cpp:410) skips fill and stroke. The shape is invisible. This is the issue as reported. |
| `CGeometry::GetPrintGeometry` stretch overload, geometry.cpp:2553-2556 | `if (pTransformedGeometry && stretch != None)` is false, so it returns NULL too. Same invisible shape. |
| `CPathGeometry::GetCompositionGeometry` (WUC shapes mode), geometry.cpp:2153-2156 | Passes NULL to `UnwrapD2DGeometry`, which calls `pGeometry->GetType()` with no NULL check (D2DAcceleratedPrimitives.cpp:18). **AV.** |
| `CGeometryCollection::GetPrintGeometry` (GeometryGroup children), geometry.cpp:2228-2232 | Puts NULL in the child array and calls `CD2DFactory::CreateGeometryGroup`, which calls `UnwrapD2DGeometry` on each child (D2DAccelerated.cpp:362). **AV.** |
| `CUIElement::PrintPushUIEClip` / `PrintPushLayoutClip`, uielement.cpp:9425 and :9453 | `if (palClipGeometry != nullptr)` skips the clip. An element whose `Clip` has an identity transform is printed **unclipped**. |

## The fix

dxaml\xcp\core\core\elements\geometry.cpp, `CGeometry::GetPrintGeometry` (3-arg overload):

```cpp
CMILMatrix transform(true);
if (m_pTransform)
{
    m_pTransform->GetTransform(&transform);
}

if (!transform.IsIdentity())
{
    IFC(cp.GetFactory()->CreateTransformedGeometry(pGeometry, &transform, &pTransformedGeometry));
}
else
{
    SetInterface(pTransformedGeometry, pGeometry);
}
```

"No transform" and "identity transform" now take the same path: return the untransformed geometry.

## Why this fixes it

- Every caller in the table above now gets a non-NULL geometry for an identity transform.
- An identity transform is a no-op, so the untransformed geometry is exactly the right answer.
  This is what the "no transform" case already returned.
- The mask path then draws fill and stroke (shape.cpp:410 passes). The stretch overload applies
  the stretch. The WUC and GeometryGroup paths no longer AV.

## Why it's safe

- **Non-identity transforms:** same code as before (`CreateTransformedGeometry` with the same matrix).
- **No transform:** same as before. `transform` starts as identity, so we take the `SetInterface` branch.
- **Identity transform:** this is the only behavior change. The old result was NULL. Nobody relies
  on that:
  - The function already asserts the input is non-NULL (`IFCPTRRC(pGeometry, E_POINTER)`), so the
    identity case was the *only* way to get a NULL out. The callers' NULL checks are defensive, not
    a "skip this geometry" protocol.
  - Two callers crash on NULL, which shows NULL was never an intended output.
- **Refcounting:** the two branches are mutually exclusive, so `pTransformedGeometry` is written once.
  `SetInterface` AddRefs, and the existing Cleanup releases both `pGeometry` and `pTransformedGeometry`.
  This is the same pattern the old "no transform" branch used.
- **Uninitialized matrix:** the old code declared `CMILMatrix transform;` without init and relied on
  `GetTransform` filling it. The new code starts from identity, which is strictly safer.
- **Observable change in print output:** an element whose `Clip` (or layout clip) geometry has an
  identity transform will now be clipped when printed. Before, the clip was silently dropped. This
  is a correctness fix: on screen the clip is applied, so print now matches the screen.
  It can't be covered by an RTB test, because RTB uses comp visuals, not this print path
  (RenderTargetBitmapImplUsingSpriteVisuals.cpp:161-171).

## Tests

dxaml\test\native\external\foundation\graphics\rendering\ShapeTests.cpp / .h:

- `ShapeTests::GeometryWithIdentityTransform` (default rendering, mask path) - ShapeTests.cpp:1694
- `ShapeTests::GeometryWithIdentityTransformWUC` (WUC shapes mode) - ShapeTests.cpp:1699
- Shared body `GeometryWithIdentityTransformInternal` - ShapeTests.cpp:1707

Each test lays out a row of green 50x50 Paths. It renders each one with RenderTargetBitmap and checks
that the center pixel is opaque green.

| Case | Mask path | WUC shapes |
|------|-----------|------------|
| No transform (control) | yes | yes |
| RectangleGeometry + `TranslateTransform()` | yes | yes |
| RectangleGeometry + `ScaleTransform()` | yes | yes |
| PathGeometry + identity `MatrixTransform` | yes | yes |
| GeometryGroup whose child has `TranslateTransform()` | yes | skipped: `CGeometryGroup::GetCompositionGeometry` isn't implemented for WUC shapes |
| 10x10 rect + `TranslateTransform()` in a 50x50 Path with `Stretch=Fill` | yes | skipped: WUC shapes don't support Stretch yet (`// WUCShapes_TODO: stretch attribute`, shape.cpp:1645) |

The issue's repro uses Stroke, and the tests use Fill. Both are drawn inside the same
`if (pPrintGeometry != nullptr)` block (shape.cpp:410), so Fill covers it.

### Results (x64chk, VM ge_current-260723-Desktop)

| Run | Result |
|-----|--------|
| New tests, before the fix | **Both crash** the test host with 0xC0000005 (the NULL `UnwrapD2DGeometry` AV) |
| New tests, after the fix | 2/2 pass |
| `*ShapeTests::*` after the fix | 34/34 pass |
| `*Geometry*` after the fix | 3/3 pass |
| `*RenderTargetBitmap*` (`-HostingMode WPF`) after the fix | 25/26. `RenderTargetBitmapTests::RasterizationScale` fails a master compare (alpha 0x1e vs 0x20). It fails **the same way on the unfixed build**, so it's pre-existing. It uses Ellipses with no Transform, which this change doesn't touch. |

## Brainstorm summary

Sonnet and Sol reviewed the plan before implementation:

- Both agreed on the fix and that it's low risk. No caller depends on NULL meaning "identity".
- Both said: don't `SetInterface` first and then `CreateTransformedGeometry` into the same pointer,
  because that leaks. Keep the branches exclusive.
- Sol pointed out that `CGeometryGroup::GetCompositionGeometry` isn't implemented for WUC shapes,
  so the GeometryGroup case only runs on the mask path.
- Both suggested adding a Stretch case, because the stretch overload also skipped on NULL.
- Both flagged the print-clip behavior change as correct, but untestable with RTB, so it's documented above.
