// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

// Safeguard: this profiler-only translation unit is gated on XamlProfilerEnabled (which defines
// XAMLPROFILER_ENABLED); the #error below turns an out-of-sync build into a clear diagnostic.
#ifndef XAMLPROFILER_ENABLED
#error "WucVisualTreeProfiler.cpp must only be compiled when XAMLPROFILER_ENABLED is defined (set XamlProfilerEnabled=true in dxaml\Xaml.Cpp.Props); do not remove the XamlProfilerEnabled ClCompile condition in Microsoft.UI.Xaml.CompTree.vcxproj."
#endif // XAMLPROFILER_ENABLED

#include "WucVisualTreeProfiler.h"
#include <Windows.UI.Composition.h>
#include "XamlProfilerTracing.h"
#include <unordered_set>

// This file ports the WUC-visual property extraction used by DCompTreeHelper's live
// <VisualTree> dump and reuses the identical attribute serialization, so the profiler
// stream and the live dump describe visuals the same way. All work here is gated behind
// XamlProfilerTracing::IsEnabled() by the callers below.

namespace
{
    // Maps the small WUC enums to readable names (instead of raw ints), matching DCompTreeHelper.
    const WCHAR* CompositeModeName(WUComp::CompositionCompositeMode value)
    {
        switch (value)
        {
            case WUComp::CompositionCompositeMode::CompositionCompositeMode_Inherit:           return L"Inherit";
            case WUComp::CompositionCompositeMode::CompositionCompositeMode_SourceOver:        return L"SourceOver";
            case WUComp::CompositionCompositeMode::CompositionCompositeMode_DestinationInvert: return L"DestinationInvert";
            case WUComp::CompositionCompositeMode::CompositionCompositeMode_MinBlend:          return L"MinBlend";
            default:                                                                            return L"Unknown";
        }
    }

    const WCHAR* BorderModeName(WUComp::CompositionBorderMode value)
    {
        switch (value)
        {
            case WUComp::CompositionBorderMode::CompositionBorderMode_Inherit: return L"Inherit";
            case WUComp::CompositionBorderMode::CompositionBorderMode_Soft:    return L"Soft";
            case WUComp::CompositionBorderMode::CompositionBorderMode_Hard:    return L"Hard";
            default:                                                            return L"Unknown";
        }
    }

    const WCHAR* BackfaceVisibilityName(WUComp::CompositionBackfaceVisibility value)
    {
        switch (value)
        {
            case WUComp::CompositionBackfaceVisibility::CompositionBackfaceVisibility_Inherit: return L"Inherit";
            case WUComp::CompositionBackfaceVisibility::CompositionBackfaceVisibility_Visible: return L"Visible";
            case WUComp::CompositionBackfaceVisibility::CompositionBackfaceVisibility_Hidden:  return L"Hidden";
            default:                                                                            return L"Unknown";
        }
    }

    // Asks a WUC object for its own runtime class name (e.g. "CompositionColorBrush"),
    // stripping the leading "Microsoft.UI.Composition." namespace. Writes "" on failure.
    void GetWucClassName(_In_opt_ IInspectable* pObject, _Out_writes_(bufferSize) WCHAR* buffer, _In_ size_t bufferSize)
    {
        buffer[0] = L'\0';
        if (pObject == nullptr)
        {
            return;
        }

        wrl_wrappers::HString className;
        if (SUCCEEDED(pObject->GetRuntimeClassName(className.GetAddressOf())))
        {
            UINT32 length = 0;
            const WCHAR* pFull = className.GetRawBuffer(&length);
            if (pFull != nullptr)
            {
                const WCHAR* pShort = wcsrchr(pFull, L'.');
                wcscpy_s(buffer, bufferSize, pShort != nullptr ? pShort + 1 : pFull);
            }
        }
    }

    // Returns the concrete WUC visual type name via a QI ladder (most-derived first, since
    // Sprite/Layer/Shape/Redirect all derive from ContainerVisual). Mirrors DCompTreeHelper.
    const WCHAR* GetWucVisualTypeName(_In_opt_ WUComp::IVisual* pVisual)
    {
        if (pVisual == nullptr)
        {
            return L"null";
        }

        wrl::ComPtr<WUComp::ISpriteVisual> spSprite;
        if (SUCCEEDED(pVisual->QueryInterface(IID_PPV_ARGS(&spSprite))))
        {
            return L"SpriteVisual";
        }
        wrl::ComPtr<WUComp::ILayerVisual> spLayer;
        if (SUCCEEDED(pVisual->QueryInterface(IID_PPV_ARGS(&spLayer))))
        {
            return L"LayerVisual";
        }
        wrl::ComPtr<WUComp::IShapeVisual> spShape;
        if (SUCCEEDED(pVisual->QueryInterface(IID_PPV_ARGS(&spShape))))
        {
            return L"ShapeVisual";
        }
        wrl::ComPtr<WUComp::IRedirectVisual> spRedirect;
        if (SUCCEEDED(pVisual->QueryInterface(IID_PPV_ARGS(&spRedirect))))
        {
            return L"RedirectVisual";
        }
        wrl::ComPtr<WUComp::IContainerVisual> spContainer;
        if (SUCCEEDED(pVisual->QueryInterface(IID_PPV_ARGS(&spContainer))))
        {
            return L"ContainerVisual";
        }
        return L"Visual";
    }

    // Normalizes any interface pointer of a WUC visual to its IVisual* identity, so the
    // emitted id matches DCompTreeHelper's "wucVisual" attribute regardless of which
    // interface the call site happened to hold (IVisual / IContainerVisual / sprite / ...).
    uint64_t VisualId(_In_opt_ IUnknown* pUnknown)
    {
        if (pUnknown == nullptr)
        {
            return 0;
        }
        wrl::ComPtr<WUComp::IVisual> spVisual;
        if (SUCCEEDED(pUnknown->QueryInterface(IID_PPV_ARGS(&spVisual))))
        {
            return reinterpret_cast<uint64_t>(spVisual.Get());
        }
        return reinterpret_cast<uint64_t>(pUnknown);
    }

    // Prefix written into a tracked visual's Comment so an out-of-process tap can locate
    // this exact live visual by string match. There is no public "find visual by pointer"
    // API, and dereferencing the raw IVisual* cross-process is unsafe; matching a stamped
    // Comment on the live object is safe (the object only exists if it's still alive).
    constexpr const WCHAR* c_visualIdCommentPrefix = L"xpid:";

    // Comment marker the tap stamps onto its OWN highlight adorner visual, so the producer
    // can skip emitting tree events for it (otherwise the adorner would surface as a phantom
    // node in the profiler's IVisual tree).
    constexpr const WCHAR* c_adornerCommentPrefix = L"__xp_adorner";

    // Registry of comp-node ids that belong to the profiler's pick overlay and must be
    // suppressed from all emitted trees. Comp-tree mutation + the IsEnabled()-gated Notify*
    // paths run on the UI thread, but guard with an SRW lock so the set is never observed in a
    // torn state if a tracing callback ever runs off-thread.
    SRWLOCK g_suppressedCompNodesLock = SRWLOCK_INIT;

    std::unordered_set<uint64_t>& SuppressedCompNodes()
    {
        static std::unordered_set<uint64_t> set;
        return set;
    }

    // Stamps "xpid:<hex IVisual*>" onto the visual's Comment. The id written here is
    // identical to the VisualId() emitted over ETW, so a node the profiler clicked
    // (node.Id) round-trips back to this exact live visual. Idempotent and cheap; only
    // ever called from the already-IsEnabled()-gated Notify* paths below.
    void StampVisualId(_In_opt_ WUComp::IVisual* pVisual)
    {
        if (pVisual == nullptr)
        {
            return;
        }

        // Comment lives on ICompositionObject2 (CompositionObject), not IVisual directly.
        wrl::ComPtr<WUComp::ICompositionObject2> spObject2;
        if (FAILED(pVisual->QueryInterface(IID_PPV_ARGS(&spObject2))))
        {
            return;
        }

        WCHAR comment[32];
        swprintf_s(comment, _countof(comment), L"%s%llx", c_visualIdCommentPrefix, VisualId(pVisual));
        spObject2->put_Comment(wrl_wrappers::HStringReference(comment).Get());
    }

    // True if the visual is the tap's highlight adorner (its Comment starts with the adorner
    // marker). Such visuals are skipped so they never appear in the profiler's tree.
    bool IsProfilerAdorner(_In_opt_ WUComp::IVisual* pVisual)
    {
        if (pVisual == nullptr)
        {
            return false;
        }

        wrl::ComPtr<WUComp::ICompositionObject2> spObject2;
        if (FAILED(pVisual->QueryInterface(IID_PPV_ARGS(&spObject2))))
        {
            return false;
        }

        wrl_wrappers::HString comment;
        if (FAILED(spObject2->get_Comment(comment.GetAddressOf())))
        {
            return false;
        }

        UINT32 length = 0;
        const WCHAR* raw = comment.GetRawBuffer(&length);
        return raw != nullptr &&
               wcsncmp(raw, c_adornerCommentPrefix, wcslen(c_adornerCommentPrefix)) == 0;
    }

    // Builds the full property attribute string for a visual in the exact format
    // DCompTreeHelper emits for <WucVisual> nodes. Safe with pVisual == nullptr.
    void BuildPropertiesString(_In_opt_ WUComp::IVisual* pVisual, _Out_writes_(cap) WCHAR* out, _In_ int cap)
    {
        out[0] = L'\0';

        // Base render properties.
        float offsetX = 0, offsetY = 0, offsetZ = 0;
        float sizeW = 0, sizeH = 0;
        float opacity = 1.0f;
        bool isVisible = true;

        // Type-specific properties.
        int wucChildCount = -1;
        int shapeCount = -1;
        bool hasBrush = false, hasShadow = false, hasEffect = false, hasViewBox = false, hasClip = false;
        WCHAR brushType[128] = L"";
        WCHAR shadowType[128] = L"";
        WCHAR clipType[128] = L"";
        void* pSourceVisualRaw = nullptr;

        // Extended base Visual transform properties (IVisual / IVisual2 / IVisual3 / IVisual4).
        float anchorX = 0, anchorY = 0;
        float centerX = 0, centerY = 0, centerZ = 0;
        float scaleX = 1, scaleY = 1, scaleZ = 1;
        float rotationDegrees = 0;
        float rotationAxisX = 0, rotationAxisY = 0, rotationAxisZ = 1;
        float orientX = 0, orientY = 0, orientZ = 0, orientW = 1;
        wfn::Matrix4x4 matrix = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
        float relOffsetX = 0, relOffsetY = 0, relOffsetZ = 0;
        float relSizeX = 0, relSizeY = 0;
        const WCHAR* compositeMode = L"Inherit";
        const WCHAR* borderMode = L"Inherit";
        const WCHAR* backfaceVisibility = L"Inherit";
        bool isHitTestVisible = true;
        bool isPixelSnappingEnabled = false;

        if (pVisual != nullptr)
        {
            wfn::Vector3 offset = {};
            wfn::Vector2 size = {};
            boolean visible = TRUE;
            if (SUCCEEDED(pVisual->get_Offset(&offset))) { offsetX = offset.X; offsetY = offset.Y; offsetZ = offset.Z; }
            if (SUCCEEDED(pVisual->get_Size(&size)))     { sizeW = size.X; sizeH = size.Y; }
            pVisual->get_Opacity(&opacity);
            if (SUCCEEDED(pVisual->get_IsVisible(&visible))) { isVisible = !!visible; }

            wfn::Vector2 anchor = {};
            if (SUCCEEDED(pVisual->get_AnchorPoint(&anchor))) { anchorX = anchor.X; anchorY = anchor.Y; }
            wfn::Vector3 center = {};
            if (SUCCEEDED(pVisual->get_CenterPoint(&center))) { centerX = center.X; centerY = center.Y; centerZ = center.Z; }
            wfn::Vector3 scale = {};
            if (SUCCEEDED(pVisual->get_Scale(&scale))) { scaleX = scale.X; scaleY = scale.Y; scaleZ = scale.Z; }
            pVisual->get_RotationAngleInDegrees(&rotationDegrees);
            wfn::Vector3 axis = {};
            if (SUCCEEDED(pVisual->get_RotationAxis(&axis))) { rotationAxisX = axis.X; rotationAxisY = axis.Y; rotationAxisZ = axis.Z; }
            wfn::Quaternion orient = {};
            if (SUCCEEDED(pVisual->get_Orientation(&orient))) { orientX = orient.X; orientY = orient.Y; orientZ = orient.Z; orientW = orient.W; }
            pVisual->get_TransformMatrix(&matrix);

            WUComp::CompositionCompositeMode compMode = {};
            if (SUCCEEDED(pVisual->get_CompositeMode(&compMode))) { compositeMode = CompositeModeName(compMode); }
            WUComp::CompositionBorderMode bMode = {};
            if (SUCCEEDED(pVisual->get_BorderMode(&bMode))) { borderMode = BorderModeName(bMode); }
            WUComp::CompositionBackfaceVisibility bfVis = {};
            if (SUCCEEDED(pVisual->get_BackfaceVisibility(&bfVis))) { backfaceVisibility = BackfaceVisibilityName(bfVis); }

            wrl::ComPtr<WUComp::ICompositionClip> spClip;
            if (SUCCEEDED(pVisual->get_Clip(&spClip)) && spClip != nullptr)
            {
                hasClip = true;
                GetWucClassName(spClip.Get(), clipType, _countof(clipType));
            }

            wrl::ComPtr<WUComp::IVisual2> spVisual2;
            if (SUCCEEDED(pVisual->QueryInterface(IID_PPV_ARGS(&spVisual2))))
            {
                wfn::Vector3 relOffset = {};
                if (SUCCEEDED(spVisual2->get_RelativeOffsetAdjustment(&relOffset))) { relOffsetX = relOffset.X; relOffsetY = relOffset.Y; relOffsetZ = relOffset.Z; }
                wfn::Vector2 relSize = {};
                if (SUCCEEDED(spVisual2->get_RelativeSizeAdjustment(&relSize))) { relSizeX = relSize.X; relSizeY = relSize.Y; }
            }
            wrl::ComPtr<WUComp::IVisual3> spVisual3;
            if (SUCCEEDED(pVisual->QueryInterface(IID_PPV_ARGS(&spVisual3))))
            {
                boolean htv = TRUE;
                if (SUCCEEDED(spVisual3->get_IsHitTestVisible(&htv))) { isHitTestVisible = !!htv; }
            }
            wrl::ComPtr<WUComp::IVisual4> spVisual4;
            if (SUCCEEDED(pVisual->QueryInterface(IID_PPV_ARGS(&spVisual4))))
            {
                boolean pse = FALSE;
                if (SUCCEEDED(spVisual4->get_IsPixelSnappingEnabled(&pse))) { isPixelSnappingEnabled = !!pse; }
            }

            wrl::ComPtr<WUComp::ISpriteVisual> spSprite;
            wrl::ComPtr<WUComp::ILayerVisual> spLayer;
            wrl::ComPtr<WUComp::IShapeVisual> spShape;
            wrl::ComPtr<WUComp::IRedirectVisual> spRedirect;
            wrl::ComPtr<WUComp::IContainerVisual> spContainer;

            if (SUCCEEDED(pVisual->QueryInterface(IID_PPV_ARGS(&spSprite))))
            {
                wrl::ComPtr<WUComp::ICompositionBrush> spBrush;
                if (SUCCEEDED(spSprite->get_Brush(&spBrush)) && spBrush != nullptr)
                {
                    hasBrush = true;
                    GetWucClassName(spBrush.Get(), brushType, _countof(brushType));
                }
                wrl::ComPtr<WUComp::ISpriteVisual2> spSprite2;
                if (SUCCEEDED(spSprite->QueryInterface(IID_PPV_ARGS(&spSprite2))))
                {
                    wrl::ComPtr<WUComp::ICompositionShadow> spShadow;
                    if (SUCCEEDED(spSprite2->get_Shadow(&spShadow)) && spShadow != nullptr)
                    {
                        hasShadow = true;
                        GetWucClassName(spShadow.Get(), shadowType, _countof(shadowType));
                    }
                }
            }
            else if (SUCCEEDED(pVisual->QueryInterface(IID_PPV_ARGS(&spLayer))))
            {
                wrl::ComPtr<WUComp::ICompositionEffectBrush> spEffect;
                if (SUCCEEDED(spLayer->get_Effect(&spEffect)) && spEffect != nullptr)
                {
                    hasEffect = true;
                }
                wrl::ComPtr<WUComp::ILayerVisual2> spLayer2;
                if (SUCCEEDED(spLayer->QueryInterface(IID_PPV_ARGS(&spLayer2))))
                {
                    wrl::ComPtr<WUComp::ICompositionShadow> spShadow;
                    if (SUCCEEDED(spLayer2->get_Shadow(&spShadow)) && spShadow != nullptr)
                    {
                        hasShadow = true;
                        GetWucClassName(spShadow.Get(), shadowType, _countof(shadowType));
                    }
                }
            }
            else if (SUCCEEDED(pVisual->QueryInterface(IID_PPV_ARGS(&spShape))))
            {
                wrl::ComPtr<wfc::IVector<WUComp::CompositionShape*>> spShapes;
                if (SUCCEEDED(spShape->get_Shapes(&spShapes)) && spShapes != nullptr)
                {
                    unsigned int count = 0;
                    if (SUCCEEDED(spShapes->get_Size(&count))) { shapeCount = static_cast<int>(count); }
                }
                wrl::ComPtr<WUComp::ICompositionViewBox> spViewBox;
                if (SUCCEEDED(spShape->get_ViewBox(&spViewBox)) && spViewBox != nullptr)
                {
                    hasViewBox = true;
                }
            }
            else if (SUCCEEDED(pVisual->QueryInterface(IID_PPV_ARGS(&spRedirect))))
            {
                wrl::ComPtr<WUComp::IVisual> spSource;
                if (SUCCEEDED(spRedirect->get_Source(&spSource)))
                {
                    pSourceVisualRaw = spSource.Get();
                }
            }

            if (spContainer == nullptr)
            {
                pVisual->QueryInterface(IID_PPV_ARGS(&spContainer));
            }
            if (spContainer != nullptr)
            {
                wrl::ComPtr<WUComp::IVisualCollection> spChildren;
                if (SUCCEEDED(spContainer->get_Children(&spChildren)) && spChildren != nullptr)
                {
                    int count = 0;
                    if (SUCCEEDED(spChildren->get_Count(&count))) { wucChildCount = count; }
                }
            }
        }

        int n = 0;

        n += swprintf_s(out + n, cap - n,
            L"offsetX=\"%.1f\" offsetY=\"%.1f\" offsetZ=\"%.1f\" width=\"%.1f\" height=\"%.1f\" "
            L"anchorPointX=\"%.2f\" anchorPointY=\"%.2f\" "
            L"centerPointX=\"%.2f\" centerPointY=\"%.2f\" centerPointZ=\"%.2f\" "
            L"scaleX=\"%.3f\" scaleY=\"%.3f\" scaleZ=\"%.3f\" "
            L"rotationAngleInDegrees=\"%.2f\" rotationAxisX=\"%.2f\" rotationAxisY=\"%.2f\" rotationAxisZ=\"%.2f\" "
            L"orientationX=\"%.3f\" orientationY=\"%.3f\" orientationZ=\"%.3f\" orientationW=\"%.3f\" ",
            offsetX, offsetY, offsetZ, sizeW, sizeH,
            anchorX, anchorY,
            centerX, centerY, centerZ,
            scaleX, scaleY, scaleZ,
            rotationDegrees, rotationAxisX, rotationAxisY, rotationAxisZ,
            orientX, orientY, orientZ, orientW);

        n += swprintf_s(out + n, cap - n,
            L"transformMatrix=\"%.3f,%.3f,%.3f,%.3f, %.3f,%.3f,%.3f,%.3f, %.3f,%.3f,%.3f,%.3f, %.3f,%.3f,%.3f,%.3f\" "
            L"opacity=\"%.3f\" isVisible=\"%s\" isHitTestVisible=\"%s\" isPixelSnappingEnabled=\"%s\" "
            L"compositeMode=\"%s\" borderMode=\"%s\" backfaceVisibility=\"%s\" hasClip=\"%s\" clipType=\"%s\" "
            L"relativeOffsetAdjustmentX=\"%.2f\" relativeOffsetAdjustmentY=\"%.2f\" relativeOffsetAdjustmentZ=\"%.2f\" "
            L"relativeSizeAdjustmentX=\"%.2f\" relativeSizeAdjustmentY=\"%.2f\" ",
            matrix.M11, matrix.M12, matrix.M13, matrix.M14,
            matrix.M21, matrix.M22, matrix.M23, matrix.M24,
            matrix.M31, matrix.M32, matrix.M33, matrix.M34,
            matrix.M41, matrix.M42, matrix.M43, matrix.M44,
            opacity,
            isVisible ? L"true" : L"false",
            isHitTestVisible ? L"true" : L"false",
            isPixelSnappingEnabled ? L"true" : L"false",
            compositeMode, borderMode, backfaceVisibility,
            hasClip ? L"true" : L"false", clipType,
            relOffsetX, relOffsetY, relOffsetZ,
            relSizeX, relSizeY);

        n += swprintf_s(out + n, cap - n,
            L"wucChildCount=\"%d\" shapeCount=\"%d\" hasBrush=\"%s\" brushType=\"%s\" "
            L"hasShadow=\"%s\" shadowType=\"%s\" hasEffect=\"%s\" hasViewBox=\"%s\" sourceVisual=\"0x%p\"",
            wucChildCount, shapeCount,
            hasBrush ? L"true" : L"false", brushType,
            hasShadow ? L"true" : L"false", shadowType,
            hasEffect ? L"true" : L"false",
            hasViewBox ? L"true" : L"false",
            pSourceVisualRaw);
    }
}

namespace WucVisualTreeProfiler
{
    bool IsEnabled()
    {
        return XamlProfilerTracing::IsEnabled();
    }

    void NotifyChildInserted(
        _In_opt_ IUnknown* parentVisual,
        _In_opt_ IUnknown* childVisual,
        uint64_t ownerCompNodeId,
        int32_t index)
    {
        if (!IsEnabled())
        {
            return;
        }

        // Skip visuals owned by a suppressed pick-overlay comp node.
        if (IsCompNodeSuppressed(ownerCompNodeId))
        {
            return;
        }

        wrl::ComPtr<WUComp::IVisual> spChild;
        if (childVisual != nullptr)
        {
            childVisual->QueryInterface(IID_PPV_ARGS(&spChild));
        }

        // Skip the tap's own highlight adorner so it never surfaces as a phantom node.
        if (IsProfilerAdorner(spChild.Get()))
        {
            return;
        }

        // Stamp the child's identity onto its Comment so a Ctrl+Click in the profiler can
        // resolve this exact live visual in the tap (match by Comment == "xpid:<node.Id>").
        StampVisualId(spChild.Get());

        WCHAR properties[4096];
        BuildPropertiesString(spChild.Get(), properties, _countof(properties));

        XamlProfilerTracing::WucVisualChildInserted(
            VisualId(parentVisual),
            VisualId(childVisual),
            ownerCompNodeId,
            index,
            GetWucVisualTypeName(spChild.Get()),
            properties);
    }

    void NotifyChildRemoved(
        _In_opt_ IUnknown* parentVisual,
        _In_opt_ IUnknown* childVisual)
    {
        if (!IsEnabled())
        {
            return;
        }

        XamlProfilerTracing::WucVisualChildRemoved(
            VisualId(parentVisual),
            VisualId(childVisual));
    }

    void NotifyChildrenCleared(_In_opt_ IUnknown* parentVisual)
    {
        if (!IsEnabled())
        {
            return;
        }

        XamlProfilerTracing::WucVisualChildrenCleared(VisualId(parentVisual));
    }

    void NotifyRootSet(
        _In_opt_ IUnknown* visual,
        uint64_t targetId,
        uint64_t ownerCompNodeId)
    {
        if (!IsEnabled())
        {
            return;
        }

        // Skip visuals owned by a suppressed pick-overlay comp node.
        if (IsCompNodeSuppressed(ownerCompNodeId))
        {
            return;
        }

        wrl::ComPtr<WUComp::IVisual> spVisual;
        if (visual != nullptr)
        {
            visual->QueryInterface(IID_PPV_ARGS(&spVisual));
        }

        // Stamp the root visual's identity onto its Comment (see NotifyChildInserted).
        StampVisualId(spVisual.Get());

        WCHAR properties[4096];
        BuildPropertiesString(spVisual.Get(), properties, _countof(properties));

        XamlProfilerTracing::WucVisualRootSet(
            VisualId(visual),
            targetId,
            ownerCompNodeId,
            GetWucVisualTypeName(spVisual.Get()),
            properties);
    }

    void NotifyRootCleared(uint64_t targetId)
    {
        if (!IsEnabled())
        {
            return;
        }

        XamlProfilerTracing::WucVisualRootCleared(targetId);
    }

    void SuppressCompNode(uint64_t compNodeId)
    {
        if (compNodeId == 0)
        {
            return;
        }
        AcquireSRWLockExclusive(&g_suppressedCompNodesLock);
        SuppressedCompNodes().insert(compNodeId);
        ReleaseSRWLockExclusive(&g_suppressedCompNodesLock);
    }

    void UnsuppressCompNode(uint64_t compNodeId)
    {
        if (compNodeId == 0)
        {
            return;
        }
        AcquireSRWLockExclusive(&g_suppressedCompNodesLock);
        SuppressedCompNodes().erase(compNodeId);
        ReleaseSRWLockExclusive(&g_suppressedCompNodesLock);
    }

    bool IsCompNodeSuppressed(uint64_t compNodeId)
    {
        if (compNodeId == 0)
        {
            return false;
        }
        AcquireSRWLockShared(&g_suppressedCompNodesLock);
        const bool suppressed = SuppressedCompNodes().count(compNodeId) != 0;
        ReleaseSRWLockShared(&g_suppressedCompNodesLock);
        return suppressed;
    }
}
