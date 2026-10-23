// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstdint>
#include <unknwn.h>

// Producer-side helpers that fire XamlProfilerTracing WUC-visual-tree events so an
// out-of-process profiler can reconstruct the full Windows.UI.Composition IVisual tree
// (the dense tree DCompTreeHelper dumps as <VisualTree>).
//
// Structure and properties are reported separately so each event stays cheap:
//   - Notify* (insert / remove / clear / root) carry ids, the owner comp node and the visual's
//     kind only.
//   - The XAMLPROFILER_WUC_* macros below sit next to the setters through which XAML changes a
//     visual's properties and report the value XAML set, so the consumer's per-visual property
//     bag stays current.
// All work is gated behind the inline WucTree keyword check (IsEnabled() below), so these calls
// are nearly free when no one is listening.
//
// Visuals are passed as IUnknown* purely for identity; the helper QIs to WUComp::IVisual
// internally. Pass any interface pointer of the visual - the emitted id is normalized to the
// IVisual* identity to match DCompTreeHelper's "wucVisual" attribute.
//
// The Notify* API exists only when XAMLPROFILER_ENABLED is defined, which is every build
// configuration unless the build passes /p:XamlProfilerEnabled=false; without the define
// WucVisualTreeProfiler.cpp is excluded from the build. Unlike XamlProfilerTracing.h (the
// SwipeTestHooks convention), call sites include this header unconditionally: the
// XAMLPROFILER_WUC_* macros sit inline next to dozens of setters and compile to nothing without
// the define, which keeps those sites free of #ifdef blocks. Direct Notify* calls still go inside
// #ifdef XAMLPROFILER_ENABLED.
namespace WucVisualTreeProfiler
{
    // Concrete visual type carried by WucVisualChildInserted / WucVisualRootSet. Part of the ETW
    // contract with the profiler tools (e.g. tools/XamlProfiler/XamlProfilerApp): append only.
    enum class VisualKind : uint32_t
    {
        Unknown = 0,
        Visual = 1,
        ContainerVisual = 2,
        SpriteVisual = 3,
        LayerVisual = 4,
        ShapeVisual = 5,
        RedirectVisual = 6,
    };

    // Property ids carried by WucVisualPropertyChanged (components in X/Y/Z/W) and
    // WucVisualObjectChanged (object-valued). Part of the ETW contract: append only.
    enum class VisualProperty : uint32_t
    {
        Offset = 1,                     // X, Y, Z
        Size = 2,                       // X, Y
        Opacity = 3,                    // X
        IsVisible = 4,                  // X (0/1)
        IsHitTestVisible = 5,           // X (0/1)
        IsPixelSnappingEnabled = 6,     // X (0/1)
        CompositeMode = 7,              // X (CompositionCompositeMode value)
        BorderMode = 8,                 // X (CompositionBorderMode value)
        Scale = 9,                      // X, Y, Z
        CenterPoint = 10,               // X, Y, Z
        RotationAngleInDegrees = 11,    // X
        RotationAxis = 12,              // X, Y, Z
        AnchorPoint = 13,               // X, Y
        RelativeOffsetAdjustment = 14,  // X, Y, Z
        RelativeSizeAdjustment = 15,    // X, Y
        BackfaceVisibility = 16,        // X (CompositionBackfaceVisibility value)
        Orientation = 17,               // X, Y, Z, W (quaternion)
        ShapeCount = 18,                // X (number of shapes in a ShapeVisual)

        Clip = 20,                      // object (CompositionClip)
        Brush = 21,                     // object (CompositionBrush on a SpriteVisual)
        Shadow = 22,                    // object (CompositionShadow on a SpriteVisual / LayerVisual)
        Effect = 23,                    // object (CompositionEffectBrush on a LayerVisual)
        Source = 24,                    // object (source visual of a RedirectVisual; ObjectId is its VisualId)
        ViewBox = 25,                   // object (CompositionViewBox on a ShapeVisual)
    };
}

#ifdef XAMLPROFILER_ENABLED
#include "XamlProfilerTracing.h"

class HWCompNode;

namespace WucVisualTreeProfiler
{
    // True only when a session has enabled the provider's WucTree group at verbose level. Inline
    // load, compare and branch on the provider's enable state, which ETW keeps current through its
    // enable callback. Call sites use this to skip ALL emit work (including building the
    // arguments passed to the Notify* helpers below) when nobody listens.
    __forceinline bool IsEnabled()
    {
        return XamlProfilerTracing::IsEnabled(XamlProfilerKeywords::WucTree);
    }

    // A WUC visual 'childVisual' was inserted into 'parentVisual's children collection.
    // ownerCompNode is the HWCompNode that owns childVisual (null if unknown); its address is
    // emitted as OwnerCompNodeId, and nothing is emitted when it is a suppressed pick-overlay
    // node. index is the child position when known, or -1 when the call site only knows
    // "top"/"bottom".
    void NotifyChildInserted(
        _In_opt_ IUnknown* parentVisual,
        _In_opt_ IUnknown* childVisual,
        _In_opt_ const HWCompNode* ownerCompNode,
        int32_t index);

    // An app-owned hand-in visual (ElementCompositionPreview::SetElementChildVisual) was inserted
    // under 'parentVisual'. Unlike XAML-owned visuals, its properties are set by the app, so they
    // are read in full here and reported as property events. The profiler tap's own highlight
    // adorner arrives through this path and is skipped. ownerCompNode as in NotifyChildInserted.
    void NotifyHandInVisualInserted(
        _In_opt_ IUnknown* parentVisual,
        _In_opt_ IUnknown* handInVisual,
        _In_opt_ const HWCompNode* ownerCompNode);

    // XAML adopted the hand-off visual (ElementCompositionPreview::GetElementVisual, requested
    // before the element had a comp node) as ownerCompNode's primary visual. The app may have
    // set its properties directly, so they are read in full here.
    void NotifyHandOffVisualAdopted(
        _In_opt_ IUnknown* handOffVisual,
        _In_opt_ const HWCompNode* ownerCompNode);

    // XAML created 'visual'. Lets the consumer reset any property bag a destroyed visual left at
    // the same address. Prefer XAMLPROFILER_WUC_CREATED.
    void NotifyVisualCreated(
        _In_opt_ IUnknown* visual,
        VisualKind kind);

    // 'childVisual' was removed from 'parentVisual's children collection.
    void NotifyChildRemoved(
        _In_opt_ IUnknown* parentVisual,
        _In_opt_ IUnknown* childVisual);

    // All children of 'parentVisual' were removed at once (IVisualCollection::RemoveAll).
    void NotifyChildrenCleared(
        _In_opt_ IUnknown* parentVisual);

    // 'visual' was set as the root of a composition target/island (ContentIsland::Root or
    // XamlIslandRoot::SetRootVisual). targetId identifies the target/island; ownerCompNode
    // is the owning HWCompNode (null if unknown), handled as in NotifyChildInserted.
    void NotifyRootSet(
        _In_opt_ IUnknown* visual,
        uint64_t targetId,
        _In_opt_ const HWCompNode* ownerCompNode);

    // The root of the composition target/island identified by targetId was cleared (null).
    void NotifyRootCleared(uint64_t targetId);

    // XAML set a scalar/vector property on 'visual'. Prefer the XAMLPROFILER_WUC_* macros.
    void NotifyPropertyChanged(
        _In_opt_ IUnknown* visual,
        VisualProperty property,
        float x,
        float y,
        float z,
        float w);

    // XAML set 'visual's TransformMatrix (row-major M11..M44). Prefer XAMLPROFILER_WUC_TRANSFORM.
    void NotifyTransformChanged(
        _In_opt_ IUnknown* visual,
        _In_reads_(16) const float* matrix);

    // NotifyTransformChanged for a 4x4 matrix struct with members M11..M44 (wfn::Matrix4x4),
    // taken by reference and out of line so the 16-value copy stays out of the setter call
    // sites. Used by XAMLPROFILER_WUC_TRANSFORM.
    template <typename TMatrix>
    __declspec(noinline) void NotifyTransformMatrixChanged(
        _In_opt_ IUnknown* visual,
        const TMatrix& matrix)
    {
        const float values[16] = {
            matrix.M11, matrix.M12, matrix.M13, matrix.M14,
            matrix.M21, matrix.M22, matrix.M23, matrix.M24,
            matrix.M31, matrix.M32, matrix.M33, matrix.M34,
            matrix.M41, matrix.M42, matrix.M43, matrix.M44 };
        NotifyTransformChanged(visual, values);
    }

    // XAML set (or cleared, when object is null) an object-valued property on 'visual'.
    // Prefer XAMLPROFILER_WUC_OBJECT.
    void NotifyObjectChanged(
        _In_opt_ IUnknown* visual,
        VisualProperty property,
        _In_opt_ IUnknown* object);

    // =====================================================================
    // Pick-overlay suppression
    //
    // The profiler's pick-mode tap injects a transparent capture overlay (a Popup hosting a
    // Border) into the target app. XAML renders that overlay into its own comp node + WUC
    // visual spine, which would otherwise surface as phantom nodes in the very trees the
    // profiler is inspecting. The tap stamps the overlay element's Name with c_overlayNamePrefix;
    // the core detects that Name when the element's comp peer is created and marks the comp node
    // (HWCompNode::MarkProfilerSuppressed) BEFORE any of its tree events are emitted. The mark
    // propagates to descendant comp nodes, and every comp-node / WUC-visual structure emission
    // gates on HWCompNode::IsProfilerSuppressed(). The mark is a bit on the node itself, so there
    // is no global registry, no lock, and no cleanup on comp-node teardown. Property events are
    // keyed by visual only and aren't gated; the consumer never shows a visual that wasn't
    // inserted.
    // =====================================================================

    // Name (GetDebugLabel) prefix the tap stamps onto its overlay Popup/Border. Must match the
    // tap's StartPick stamping (Tap.cpp).
    constexpr const wchar_t* c_overlayNamePrefix = L"__xp_pick_overlay";
}

// Report a property XAML just set on a WUC visual. 'visual' is a raw interface pointer of the
// visual (any visual interface; use .Get() on smart pointers); 'property' is a VisualProperty
// enumerator name. Use these right after the setter call, passing the same value expression,
// which must be cheap and side-effect free (hoist it into a local otherwise). Wrap a braced
// value in parentheses: XAMLPROFILER_WUC_VECTOR2(v, Size, (wfn::Vector2{ w, h })).
#define XAMLPROFILER_WUC_SCALAR(visual, property, value) \
    do { if (WucVisualTreeProfiler::IsEnabled()) { WucVisualTreeProfiler::NotifyPropertyChanged((visual), WucVisualTreeProfiler::VisualProperty::property, static_cast<float>(value), 0.0f, 0.0f, 0.0f); } } while (0)

#define XAMLPROFILER_WUC_VECTOR2(visual, property, vector) \
    do { if (WucVisualTreeProfiler::IsEnabled()) { const auto& xpVector_ = (vector); WucVisualTreeProfiler::NotifyPropertyChanged((visual), WucVisualTreeProfiler::VisualProperty::property, xpVector_.X, xpVector_.Y, 0.0f, 0.0f); } } while (0)

#define XAMLPROFILER_WUC_VECTOR3(visual, property, vector) \
    do { if (WucVisualTreeProfiler::IsEnabled()) { const auto& xpVector_ = (vector); WucVisualTreeProfiler::NotifyPropertyChanged((visual), WucVisualTreeProfiler::VisualProperty::property, xpVector_.X, xpVector_.Y, xpVector_.Z, 0.0f); } } while (0)

#define XAMLPROFILER_WUC_VECTOR4(visual, property, vector) \
    do { if (WucVisualTreeProfiler::IsEnabled()) { const auto& xpVector_ = (vector); WucVisualTreeProfiler::NotifyPropertyChanged((visual), WucVisualTreeProfiler::VisualProperty::property, xpVector_.X, xpVector_.Y, xpVector_.Z, xpVector_.W); } } while (0)

#define XAMLPROFILER_WUC_TRANSFORM(visual, matrix) \
    do { if (WucVisualTreeProfiler::IsEnabled()) { WucVisualTreeProfiler::NotifyTransformMatrixChanged((visual), (matrix)); } } while (0)

#define XAMLPROFILER_WUC_OBJECT(visual, property, object) \
    do { if (WucVisualTreeProfiler::IsEnabled()) { WucVisualTreeProfiler::NotifyObjectChanged((visual), WucVisualTreeProfiler::VisualProperty::property, (object)); } } while (0)

// Report that XAML just created 'visual'; 'kind' is a VisualKind enumerator name.
#define XAMLPROFILER_WUC_CREATED(visual, kind) \
    do { if (WucVisualTreeProfiler::IsEnabled()) { WucVisualTreeProfiler::NotifyVisualCreated((visual), WucVisualTreeProfiler::VisualKind::kind); } } while (0)

#else

// The arguments are named in unevaluated sizeof operands so a local hoisted only for the
// profiler doesn't trigger an unreferenced-variable warning when the profiler is compiled out.
#define XAMLPROFILER_WUC_SCALAR(visual, property, value) do { (void)sizeof(visual); (void)sizeof(static_cast<float>(value)); } while (0)
#define XAMLPROFILER_WUC_VECTOR2(visual, property, vector) do { (void)sizeof(visual); (void)sizeof(vector); } while (0)
#define XAMLPROFILER_WUC_VECTOR3(visual, property, vector) do { (void)sizeof(visual); (void)sizeof(vector); } while (0)
#define XAMLPROFILER_WUC_VECTOR4(visual, property, vector) do { (void)sizeof(visual); (void)sizeof(vector); } while (0)
#define XAMLPROFILER_WUC_TRANSFORM(visual, matrix) do { (void)sizeof(visual); (void)sizeof(matrix); } while (0)
#define XAMLPROFILER_WUC_OBJECT(visual, property, object) do { (void)sizeof(visual); (void)sizeof(object); } while (0)
#define XAMLPROFILER_WUC_CREATED(visual, kind) do { (void)sizeof(visual); } while (0)

#endif // XAMLPROFILER_ENABLED
