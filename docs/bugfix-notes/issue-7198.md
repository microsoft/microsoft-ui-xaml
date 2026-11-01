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
| `CShape::PreChildrenPrintVirtual` (shape.cpp:330), calls at shape.cpp:375 and :387 | `if (pPrintGeometry != nullptr)` (shape.cpp:404) skips fill and stroke. The shape is invisible. This is the issue as reported. |
| `CGeometry::GetPrintGeometry` stretch overload, geometry.cpp:2553-2556 | `if (pTransformedGeometry && stretch != None)` is false, so the else branch (geometry.cpp:2593-2595) passes the NULL through. Same invisible shape. |
| `CPathGeometry::GetCompositionGeometry` (WUC shapes mode), geometry.cpp:2153-2156 | Passes NULL to `UnwrapD2DGeometry`, which calls `pGeometry->GetType()` with no NULL check (D2DAcceleratedPrimitives.cpp:18). **AV.** |
| `CGeometryCollection::GetPrintGeometry` (GeometryGroup children), geometry.cpp:2228-2232 | Puts NULL in the child array and calls `CD2DFactory::CreateGeometryGroup`, which calls `UnwrapD2DGeometry` on each child (D2DAccelerated.cpp:362). **AV.** |
| `CUIElement::PrintPushUIEClip`, uielement.cpp:9425-9426 | `if (palClipGeometry != nullptr)` skips the clip. An element whose `UIElement.Clip` has an identity transform is drawn **unclipped** on this path. |

`PrintPushLayoutClip` (uielement.cpp:9453) calls the same function, but layout clip geometries are
created internally with no `Transform`, so it was never affected.

The `CUIElement::Print` path (where the clip push happens, uielement.cpp:9339) is reached from:

- Printing: `PrintDocument.cpp:425`.
- Alpha masks: `AlphaMask.cpp:447` (`GetAlphaMask`, for example for `DropShadow.Mask`).
  The mask respects local properties like `Clip` (AlphaMask.cpp:198).

## The fix

dxaml\xcp\core\core\elements\geometry.cpp, `CGeometry::GetPrintGeometry` (3-arg overload), geometry.cpp:2499-2518:

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
- The shape mask/print path then draws fill and stroke (shape.cpp:404 passes). The stretch
  overload applies the stretch. The WUC PathGeometry and GeometryGroup paths no longer AV.

## Why it's safe

- **Non-identity transforms:** same code as before (`CreateTransformedGeometry` with the same matrix).
- **No transform:** same as before. `transform` starts as identity, so we take the `SetInterface` branch.
- **Identity transform:** this is the only behavior change. The old result was NULL. Nobody relies
  on that:
  - The function already rejects a NULL input (`IFCPTRRC(pGeometry, E_POINTER)`, geometry.cpp:2497).
    So the identity case was the only way to get **S_OK with a NULL** out of this overload.
    (Other failures, like a GeometryGroup with a NULL child, return an error HRESULT, not S_OK.)
    The callers' NULL checks are defensive, not a "skip this geometry" protocol.
  - Two callers crash on NULL, which shows NULL was never an intended output.
- **Refcounting:** the two branches are mutually exclusive, so `pTransformedGeometry` is written once.
  `SetInterface` AddRefs, and the existing Cleanup releases both `pGeometry` and `pTransformedGeometry`.
  This is the same pattern the old "no transform" branch used.
- **Uninitialized matrix:** the old code declared `CMILMatrix transform;` without init and relied on
  `GetTransform` filling it. The new code starts from identity, which is strictly safer.
- **Observable change for `UIElement.Clip`:** an element whose `Clip` geometry has an identity
  transform is now clipped when printed and in its alpha mask (for example a `DropShadow` mask).
  Before, the clip was silently dropped on those paths. On screen the clip was always applied
  (the comp tree path doesn't use `GetPrintGeometry`), so print and masks now match the screen.
- **Not covered by a test:** the `UIElement.Clip` change. RenderTargetBitmap renders comp visuals,
  not this print path (RenderTargetBitmapImplUsingSpriteVisuals.cpp:161-171), and there is no
  master-free way in the test tree to read alpha mask pixels. The code path is the same
  `GetPrintGeometry` call the shape tests cover.

## Tests

dxaml\test\native\external\foundation\graphics\rendering\ShapeTests.cpp / .h:

- `ShapeTests::GeometryWithIdentityTransform` (default rendering, shape mask path) - ShapeTests.cpp:1694
- `ShapeTests::GeometryWithIdentityTransformWUC` (WUC shapes mode) - ShapeTests.cpp:1699
- Shared body `GeometryWithIdentityTransformInternal` - ShapeTests.cpp:1738
- `ShapeTests::GeometryGroupChildWithIdentityTransform` (default rendering only) - ShapeTests.cpp:1837
- Helper `VerifyCenterPixelsAreGreen` - ShapeTests.cpp:1712. It puts the Paths side by side,
  renders each one with RenderTargetBitmap, and checks that its center pixel is opaque green.

GeometryGroup is in its own test. Before the fix it crashes the test host, and if it shared a test
with the other cases, the crash would hide whether the other cases failed.

| Case | Shape mask path | WUC shapes |
|------|-----------------|------------|
| No transform (control) | yes | yes |
| RectangleGeometry + `TranslateTransform()` | yes | runs, but doesn't exercise the fix: `CRectangleGeometry::GetCompositionGeometry` (rectangle.cpp:907-937) builds a WUC rectangle and ignores `Transform` |
| RectangleGeometry + `ScaleTransform()` | yes | same as above |
| PathGeometry + identity `MatrixTransform` | yes | yes (goes through `GetPrintGeometry`, geometry.cpp:2153) |
| Stroke-only PathGeometry with a PolyLineSegment + `TranslateTransform()` (the issue's repro shape) | yes | yes |
| 10x10 rect + `TranslateTransform()` in a 50x50 Path with `Stretch=Fill` | yes | skipped: WUC shapes don't support Stretch yet (`// WUCShapes_TODO: stretch attribute`, shape.cpp:1645) |
| GeometryGroup whose child has `TranslateTransform()` (separate test) | yes | skipped: `CGeometryGroup::GetCompositionGeometry` isn't implemented for WUC shapes |

The stroke case Path gets an explicit 50x50 size. A Path with no size in a Canvas has a 0x0 layout
size, and WUC shapes size the ShapeVisual to the layout size (VisualContentRenderer.cpp:641-642),
so it would draw nothing for reasons unrelated to this bug.

### Results (x64chk, VM ge_current-260723-Desktop)

| Run | Result |
|-----|--------|
| New tests, before the fix | All 3 fail. `GeometryWithIdentityTransform`: every identity-transform case reads pixel 0 (checked with Verify exceptions turned off, so all 6 cases ran); the no-transform control passes. `GeometryWithIdentityTransformWUC` and `GeometryGroupChildWithIdentityTransform`: the test host crashes with 0xC0000005 (the NULL `UnwrapD2DGeometry` AV). |
| New tests, after the fix | 3/3 pass |
| `*ShapeTests::*` after the fix | 35/35 pass |
| `*Geometry*` after the fix (round 0) | 3/3 pass |
| `*RenderTargetBitmap*` (`-HostingMode WPF`) after the fix (round 0) | 25/26. `RenderTargetBitmapTests::RasterizationScale` fails a master compare (alpha 0x1e vs 0x20). It fails **the same way on the unfixed build**, so it's pre-existing. It uses Ellipses with no Transform, which this change doesn't touch. |

## Brainstorm summary

Sonnet and Sol reviewed the plan before implementation:

- Both agreed on the fix and that it's low risk. No caller depends on NULL meaning "identity".
- Both said: don't `SetInterface` first and then `CreateTransformedGeometry` into the same pointer,
  because that leaks. Keep the branches exclusive.
- Sol pointed out that `CGeometryGroup::GetCompositionGeometry` isn't implemented for WUC shapes,
  so the GeometryGroup case only runs on the mask path.
- Both suggested adding a Stretch case, because the stretch overload also skipped on NULL.
- Both flagged the print-clip behavior change as correct, but untestable with RTB, so it's documented above.
