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
#include <HWCompNode.h>
#include "XamlProfilerTracing.h"

// Every public entry point returns immediately unless a session has the WucTree group enabled
// (IsEnabled()).

namespace
{
    using WucVisualTreeProfiler::VisualKind;
    using WucVisualTreeProfiler::VisualProperty;

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
                // Truncate rather than fail fast on an unexpectedly long name.
                wcsncpy_s(buffer, bufferSize, pShort != nullptr ? pShort + 1 : pFull, _TRUNCATE);
            }
        }
    }

    // Returns the concrete WUC visual kind via a QI ladder (most-derived first, since
    // Sprite/Layer/Shape/Redirect all derive from ContainerVisual). Mirrors DCompTreeHelper.
    VisualKind GetVisualKind(_In_opt_ WUComp::IVisual* pVisual)
    {
        if (pVisual == nullptr)
        {
            return VisualKind::Unknown;
        }

        wrl::ComPtr<WUComp::ISpriteVisual> spSprite;
        if (SUCCEEDED(pVisual->QueryInterface(IID_PPV_ARGS(&spSprite))))
        {
            return VisualKind::SpriteVisual;
        }
        wrl::ComPtr<WUComp::ILayerVisual> spLayer;
        if (SUCCEEDED(pVisual->QueryInterface(IID_PPV_ARGS(&spLayer))))
        {
            return VisualKind::LayerVisual;
        }
        wrl::ComPtr<WUComp::IShapeVisual> spShape;
        if (SUCCEEDED(pVisual->QueryInterface(IID_PPV_ARGS(&spShape))))
        {
            return VisualKind::ShapeVisual;
        }
        wrl::ComPtr<WUComp::IRedirectVisual> spRedirect;
        if (SUCCEEDED(pVisual->QueryInterface(IID_PPV_ARGS(&spRedirect))))
        {
            return VisualKind::RedirectVisual;
        }
        wrl::ComPtr<WUComp::IContainerVisual> spContainer;
        if (SUCCEEDED(pVisual->QueryInterface(IID_PPV_ARGS(&spContainer))))
        {
            return VisualKind::ContainerVisual;
        }
        return VisualKind::Visual;
    }

    // Normalizes any interface pointer of a WUC visual to its IVisual* identity, so the
    // emitted id matches DCompTreeHelper's "wucVisual" attribute regardless of which
    // interface the call site happened to hold (IVisual / IContainerVisual / sprite / ...).
    wrl::ComPtr<WUComp::IVisual> AsVisual(_In_opt_ IUnknown* pUnknown)
    {
        wrl::ComPtr<WUComp::IVisual> spVisual;
        if (pUnknown != nullptr && FAILED(pUnknown->QueryInterface(IID_PPV_ARGS(&spVisual))))
        {
            spVisual = nullptr;
        }
        return spVisual;
    }

    uint64_t VisualId(_In_opt_ IUnknown* pUnknown)
    {
        return reinterpret_cast<uint64_t>(AsVisual(pUnknown).Get());
    }

    // COM identity of a non-visual composition object (clip, brush, shadow, ...).
    uint64_t ObjectId(_In_opt_ IUnknown* pObject)
    {
        if (pObject == nullptr)
        {
            return 0;
        }
        wrl::ComPtr<IUnknown> spIdentity;
        if (FAILED(pObject->QueryInterface(IID_PPV_ARGS(&spIdentity))))
        {
            return 0;
        }
        return reinterpret_cast<uint64_t>(spIdentity.Get());
    }

    // Comment marker the tap stamps onto its OWN highlight adorner visual, so the producer
    // can skip emitting tree events for it (otherwise the adorner would surface as a phantom
    // node in the profiler's IVisual tree). The adorner is attached as a hand-in visual.
    constexpr const WCHAR* c_adornerCommentPrefix = L"__xp_adorner";

    // True if the visual belongs to a pick-overlay comp node (see the suppression notes in
    // WucVisualTreeProfiler.h), so no structure event may be emitted for it.
    bool IsSuppressedOwner(_In_opt_ const HWCompNode* ownerCompNode)
    {
        return ownerCompNode != nullptr && ownerCompNode->IsProfilerSuppressed();
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

    void EmitProperty(uint64_t visualId, VisualProperty property, float x, float y = 0.0f, float z = 0.0f, float w = 0.0f)
    {
        XamlProfilerTracing::WucVisualPropertyChanged(visualId, static_cast<uint32_t>(property), x, y, z, w);
    }

    void EmitObject(uint64_t visualId, VisualProperty property, _In_opt_ IUnknown* pObject)
    {
        uint64_t objectId = 0;
        WCHAR typeName[128] = L"";
        if (pObject != nullptr)
        {
            objectId = (property == VisualProperty::Source) ? VisualId(pObject) : ObjectId(pObject);

            wrl::ComPtr<IInspectable> spInspectable;
            if (SUCCEEDED(pObject->QueryInterface(IID_PPV_ARGS(&spInspectable))))
            {
                GetWucClassName(spInspectable.Get(), typeName, _countof(typeName));
            }
        }

        XamlProfilerTracing::WucVisualObjectChanged(visualId, static_cast<uint32_t>(property), objectId, typeName);
    }

    void EmitTransform(uint64_t visualId, const wfn::Matrix4x4& matrix)
    {
        const float values[16] = {
            matrix.M11, matrix.M12, matrix.M13, matrix.M14,
            matrix.M21, matrix.M22, matrix.M23, matrix.M24,
            matrix.M31, matrix.M32, matrix.M33, matrix.M34,
            matrix.M41, matrix.M42, matrix.M43, matrix.M44 };
        XamlProfilerTracing::WucVisualTransformChanged(visualId, values);
    }

    // Reads every reported property of a visual whose properties the app sets, and emits each as
    // a property event. Together these overwrite everything the consumer holds for visualId.
    // This is the only place visual properties are read back; it runs only for hand-in visuals
    // and adopted hand-off visuals, which are rare.
    void EmitAllProperties(uint64_t visualId, _In_ WUComp::IVisual* pVisual)
    {
        wfn::Vector3 offset = {};
        if (SUCCEEDED(pVisual->get_Offset(&offset)))
        {
            EmitProperty(visualId, VisualProperty::Offset, offset.X, offset.Y, offset.Z);
        }
        wfn::Vector2 size = {};
        if (SUCCEEDED(pVisual->get_Size(&size)))
        {
            EmitProperty(visualId, VisualProperty::Size, size.X, size.Y);
        }
        float opacity = 1.0f;
        if (SUCCEEDED(pVisual->get_Opacity(&opacity)))
        {
            EmitProperty(visualId, VisualProperty::Opacity, opacity);
        }
        boolean isVisible = TRUE;
        if (SUCCEEDED(pVisual->get_IsVisible(&isVisible)))
        {
            EmitProperty(visualId, VisualProperty::IsVisible, isVisible ? 1.0f : 0.0f);
        }
        wfn::Vector2 anchor = {};
        if (SUCCEEDED(pVisual->get_AnchorPoint(&anchor)))
        {
            EmitProperty(visualId, VisualProperty::AnchorPoint, anchor.X, anchor.Y);
        }
        wfn::Vector3 center = {};
        if (SUCCEEDED(pVisual->get_CenterPoint(&center)))
        {
            EmitProperty(visualId, VisualProperty::CenterPoint, center.X, center.Y, center.Z);
        }
        wfn::Vector3 scale = {};
        if (SUCCEEDED(pVisual->get_Scale(&scale)))
        {
            EmitProperty(visualId, VisualProperty::Scale, scale.X, scale.Y, scale.Z);
        }
        float rotationDegrees = 0.0f;
        if (SUCCEEDED(pVisual->get_RotationAngleInDegrees(&rotationDegrees)))
        {
            EmitProperty(visualId, VisualProperty::RotationAngleInDegrees, rotationDegrees);
        }
        wfn::Vector3 axis = {};
        if (SUCCEEDED(pVisual->get_RotationAxis(&axis)))
        {
            EmitProperty(visualId, VisualProperty::RotationAxis, axis.X, axis.Y, axis.Z);
        }
        wfn::Quaternion orientation = {};
        if (SUCCEEDED(pVisual->get_Orientation(&orientation)))
        {
            EmitProperty(visualId, VisualProperty::Orientation, orientation.X, orientation.Y, orientation.Z, orientation.W);
        }
        wfn::Matrix4x4 matrix = {};
        if (SUCCEEDED(pVisual->get_TransformMatrix(&matrix)))
        {
            EmitTransform(visualId, matrix);
        }
        WUComp::CompositionCompositeMode compositeMode = {};
        if (SUCCEEDED(pVisual->get_CompositeMode(&compositeMode)))
        {
            EmitProperty(visualId, VisualProperty::CompositeMode, static_cast<float>(compositeMode));
        }
        WUComp::CompositionBorderMode borderMode = {};
        if (SUCCEEDED(pVisual->get_BorderMode(&borderMode)))
        {
            EmitProperty(visualId, VisualProperty::BorderMode, static_cast<float>(borderMode));
        }
        WUComp::CompositionBackfaceVisibility backfaceVisibility = {};
        if (SUCCEEDED(pVisual->get_BackfaceVisibility(&backfaceVisibility)))
        {
            EmitProperty(visualId, VisualProperty::BackfaceVisibility, static_cast<float>(backfaceVisibility));
        }
        wrl::ComPtr<WUComp::ICompositionClip> spClip;
        if (SUCCEEDED(pVisual->get_Clip(&spClip)))
        {
            EmitObject(visualId, VisualProperty::Clip, spClip.Get());
        }

        wrl::ComPtr<WUComp::IVisual2> spVisual2;
        if (SUCCEEDED(pVisual->QueryInterface(IID_PPV_ARGS(&spVisual2))))
        {
            wfn::Vector3 relativeOffset = {};
            if (SUCCEEDED(spVisual2->get_RelativeOffsetAdjustment(&relativeOffset)))
            {
                EmitProperty(visualId, VisualProperty::RelativeOffsetAdjustment, relativeOffset.X, relativeOffset.Y, relativeOffset.Z);
            }
            wfn::Vector2 relativeSize = {};
            if (SUCCEEDED(spVisual2->get_RelativeSizeAdjustment(&relativeSize)))
            {
                EmitProperty(visualId, VisualProperty::RelativeSizeAdjustment, relativeSize.X, relativeSize.Y);
            }
        }
        wrl::ComPtr<WUComp::IVisual3> spVisual3;
        if (SUCCEEDED(pVisual->QueryInterface(IID_PPV_ARGS(&spVisual3))))
        {
            boolean isHitTestVisible = TRUE;
            if (SUCCEEDED(spVisual3->get_IsHitTestVisible(&isHitTestVisible)))
            {
                EmitProperty(visualId, VisualProperty::IsHitTestVisible, isHitTestVisible ? 1.0f : 0.0f);
            }
        }
        wrl::ComPtr<WUComp::IVisual4> spVisual4;
        if (SUCCEEDED(pVisual->QueryInterface(IID_PPV_ARGS(&spVisual4))))
        {
            boolean isPixelSnappingEnabled = FALSE;
            if (SUCCEEDED(spVisual4->get_IsPixelSnappingEnabled(&isPixelSnappingEnabled)))
            {
                EmitProperty(visualId, VisualProperty::IsPixelSnappingEnabled, isPixelSnappingEnabled ? 1.0f : 0.0f);
            }
        }

        wrl::ComPtr<WUComp::ISpriteVisual> spSprite;
        wrl::ComPtr<WUComp::ILayerVisual> spLayer;
        wrl::ComPtr<WUComp::IShapeVisual> spShape;
        wrl::ComPtr<WUComp::IRedirectVisual> spRedirect;
        if (SUCCEEDED(pVisual->QueryInterface(IID_PPV_ARGS(&spSprite))))
        {
            wrl::ComPtr<WUComp::ICompositionBrush> spBrush;
            if (SUCCEEDED(spSprite->get_Brush(&spBrush)))
            {
                EmitObject(visualId, VisualProperty::Brush, spBrush.Get());
            }
            wrl::ComPtr<WUComp::ISpriteVisual2> spSprite2;
            if (SUCCEEDED(spSprite.As(&spSprite2)))
            {
                wrl::ComPtr<WUComp::ICompositionShadow> spShadow;
                if (SUCCEEDED(spSprite2->get_Shadow(&spShadow)))
                {
                    EmitObject(visualId, VisualProperty::Shadow, spShadow.Get());
                }
            }
        }
        else if (SUCCEEDED(pVisual->QueryInterface(IID_PPV_ARGS(&spLayer))))
        {
            wrl::ComPtr<WUComp::ICompositionEffectBrush> spEffect;
            if (SUCCEEDED(spLayer->get_Effect(&spEffect)))
            {
                EmitObject(visualId, VisualProperty::Effect, spEffect.Get());
            }
            wrl::ComPtr<WUComp::ILayerVisual2> spLayer2;
            if (SUCCEEDED(spLayer.As(&spLayer2)))
            {
                wrl::ComPtr<WUComp::ICompositionShadow> spShadow;
                if (SUCCEEDED(spLayer2->get_Shadow(&spShadow)))
                {
                    EmitObject(visualId, VisualProperty::Shadow, spShadow.Get());
                }
            }
        }
        else if (SUCCEEDED(pVisual->QueryInterface(IID_PPV_ARGS(&spShape))))
        {
            wrl::ComPtr<wfc::IVector<WUComp::CompositionShape*>> spShapes;
            if (SUCCEEDED(spShape->get_Shapes(&spShapes)) && spShapes != nullptr)
            {
                unsigned int count = 0;
                if (SUCCEEDED(spShapes->get_Size(&count)))
                {
                    EmitProperty(visualId, VisualProperty::ShapeCount, static_cast<float>(count));
                }
            }
            wrl::ComPtr<WUComp::ICompositionViewBox> spViewBox;
            if (SUCCEEDED(spShape->get_ViewBox(&spViewBox)))
            {
                EmitObject(visualId, VisualProperty::ViewBox, spViewBox.Get());
            }
        }
        else if (SUCCEEDED(pVisual->QueryInterface(IID_PPV_ARGS(&spRedirect))))
        {
            wrl::ComPtr<WUComp::IVisual> spSource;
            if (SUCCEEDED(spRedirect->get_Source(&spSource)))
            {
                EmitObject(visualId, VisualProperty::Source, spSource.Get());
            }
        }
    }
}

namespace WucVisualTreeProfiler
{
    void NotifyChildInserted(
        _In_opt_ IUnknown* parentVisual,
        _In_opt_ IUnknown* childVisual,
        _In_opt_ const HWCompNode* ownerCompNode,
        int32_t index)
    {
        if (!IsEnabled())
        {
            return;
        }

        // Skip visuals owned by a suppressed pick-overlay comp node.
        if (IsSuppressedOwner(ownerCompNode))
        {
            return;
        }

        const uint64_t parentVisualId = VisualId(parentVisual);
        const wrl::ComPtr<WUComp::IVisual> spChild = AsVisual(childVisual);
        if (parentVisualId == 0 || spChild == nullptr)
        {
            return;
        }

        XamlProfilerTracing::WucVisualChildInserted(
            parentVisualId,
            reinterpret_cast<uint64_t>(spChild.Get()),
            reinterpret_cast<uint64_t>(ownerCompNode),
            index,
            static_cast<uint32_t>(GetVisualKind(spChild.Get())));
    }

    void NotifyHandInVisualInserted(
        _In_opt_ IUnknown* parentVisual,
        _In_opt_ IUnknown* handInVisual,
        _In_opt_ const HWCompNode* ownerCompNode)
    {
        if (!IsEnabled())
        {
            return;
        }

        if (IsSuppressedOwner(ownerCompNode))
        {
            return;
        }

        const uint64_t parentVisualId = VisualId(parentVisual);
        const wrl::ComPtr<WUComp::IVisual> spChild = AsVisual(handInVisual);
        if (parentVisualId == 0 || spChild == nullptr)
        {
            return;
        }

        // Skip the tap's own highlight adorner so it never surfaces as a phantom node.
        if (IsProfilerAdorner(spChild.Get()))
        {
            return;
        }

        const uint64_t childVisualId = reinterpret_cast<uint64_t>(spChild.Get());
        XamlProfilerTracing::WucVisualChildInserted(
            parentVisualId,
            childVisualId,
            reinterpret_cast<uint64_t>(ownerCompNode),
            -1 /* index */,
            static_cast<uint32_t>(GetVisualKind(spChild.Get())));

        EmitAllProperties(childVisualId, spChild.Get());
    }

    void NotifyHandOffVisualAdopted(
        _In_opt_ IUnknown* handOffVisual,
        _In_opt_ const HWCompNode* ownerCompNode)
    {
        if (!IsEnabled())
        {
            return;
        }

        if (IsSuppressedOwner(ownerCompNode))
        {
            return;
        }

        const wrl::ComPtr<WUComp::IVisual> spVisual = AsVisual(handOffVisual);
        if (spVisual == nullptr)
        {
            return;
        }

        EmitAllProperties(reinterpret_cast<uint64_t>(spVisual.Get()), spVisual.Get());
    }

    void NotifyVisualCreated(
        _In_opt_ IUnknown* visual,
        VisualKind kind)
    {
        if (!IsEnabled())
        {
            return;
        }

        const uint64_t visualId = VisualId(visual);
        if (visualId == 0)
        {
            return;
        }

        XamlProfilerTracing::WucVisualCreated(visualId, static_cast<uint32_t>(kind));
    }

    void NotifyChildRemoved(
        _In_opt_ IUnknown* parentVisual,
        _In_opt_ IUnknown* childVisual)
    {
        if (!IsEnabled())
        {
            return;
        }

        const uint64_t parentVisualId = VisualId(parentVisual);
        const uint64_t childVisualId = VisualId(childVisual);
        if (parentVisualId == 0 || childVisualId == 0)
        {
            return;
        }

        XamlProfilerTracing::WucVisualChildRemoved(
            parentVisualId,
            childVisualId);
    }

    void NotifyChildrenCleared(_In_opt_ IUnknown* parentVisual)
    {
        if (!IsEnabled())
        {
            return;
        }

        const uint64_t parentVisualId = VisualId(parentVisual);
        if (parentVisualId == 0)
        {
            return;
        }

        XamlProfilerTracing::WucVisualChildrenCleared(parentVisualId);
    }

    void NotifyRootSet(
        _In_opt_ IUnknown* visual,
        uint64_t targetId,
        _In_opt_ const HWCompNode* ownerCompNode)
    {
        if (!IsEnabled())
        {
            return;
        }

        // Skip visuals owned by a suppressed pick-overlay comp node.
        if (IsSuppressedOwner(ownerCompNode))
        {
            return;
        }

        const wrl::ComPtr<WUComp::IVisual> spVisual = AsVisual(visual);
        if (spVisual == nullptr)
        {
            return;
        }

        XamlProfilerTracing::WucVisualRootSet(
            reinterpret_cast<uint64_t>(spVisual.Get()),
            targetId,
            reinterpret_cast<uint64_t>(ownerCompNode),
            static_cast<uint32_t>(GetVisualKind(spVisual.Get())));
    }

    void NotifyRootCleared(uint64_t targetId)
    {
        if (!IsEnabled())
        {
            return;
        }

        XamlProfilerTracing::WucVisualRootCleared(targetId);
    }

    void NotifyPropertyChanged(
        _In_opt_ IUnknown* visual,
        VisualProperty property,
        float x,
        float y,
        float z,
        float w)
    {
        if (!IsEnabled())
        {
            return;
        }

        const uint64_t visualId = VisualId(visual);
        if (visualId == 0)
        {
            return;
        }

        EmitProperty(visualId, property, x, y, z, w);
    }

    void NotifyTransformChanged(
        _In_opt_ IUnknown* visual,
        _In_reads_(16) const float* matrix)
    {
        if (!IsEnabled())
        {
            return;
        }

        const uint64_t visualId = VisualId(visual);
        if (visualId == 0)
        {
            return;
        }

        XamlProfilerTracing::WucVisualTransformChanged(visualId, matrix);
    }

    void NotifyObjectChanged(
        _In_opt_ IUnknown* visual,
        VisualProperty property,
        _In_opt_ IUnknown* object)
    {
        if (!IsEnabled())
        {
            return;
        }

        const uint64_t visualId = VisualId(visual);
        if (visualId == 0)
        {
            return;
        }

        EmitObject(visualId, property, object);
    }
}
