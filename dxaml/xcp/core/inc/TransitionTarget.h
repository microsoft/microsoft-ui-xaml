// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "MultiParentShareableDependencyObject.h"
#include "HitTestPolygon.h"
#include <NamespaceAliases.h>
#include <fwd/windows.ui.composition.h>

class TransformAndClipStack;
class CMILMatrix;
class WinRTExpressionConversionContext;

enum class TransitionTargetDirtyMode
{
    Dirty_None = 0,
    Dirty_Transform = 1,
    Dirty_Clip = 2,
    Dirty_Opacity = 4,
    Dirty_All = 0x7
};
DEFINE_ENUM_FLAG_OPERATORS(TransitionTargetDirtyMode);

class CTransitionTarget final : public CMultiParentShareableDependencyObject
{
protected:
    CTransitionTarget(_In_ CCoreServices *pCore)
        : CMultiParentShareableDependencyObject(pCore)
        , m_hasClipAnimation(false)
        , m_isOpacityAnimationDirty(false)
    {}

public:
#if defined(__XAML_UNITTESTS__)
    CTransitionTarget()  // !!! FOR UNIT TESTING ONLY !!!
        : CTransitionTarget(nullptr)
    {}
#endif

    ~CTransitionTarget() override;

    DECLARE_CREATE(CTransitionTarget);

    _Check_return_ HRESULT InitInstance() override;

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CTransitionTarget>::Index;
    }

    static void NWSetTransformDirty(_In_ CDependencyObject *pTarget, DirtyFlags flags);

    static void NWSetClipDirty(_In_ CDependencyObject *pTarget, DirtyFlags flags);

    static void NWSetOpacityDirty(_In_ CDependencyObject *pTarget, DirtyFlags flags);

    void NWSetPropertyDirtyOnTarget(_In_ CUIElement* pTarget, DirtyFlags flags);

    bool IsDirty() const;
    void Clean();

    void ApplyClip(
        _In_ XRECTF bounds,
        _Inout_ XRECTF *pClipRect
        );

    _Check_return_ HRESULT ApplyClip(
        _In_ XRECTF bounds,
        _Inout_ TransformAndClipStack *pTransformsAndClips
        );

    void GetClipTransform(_In_ const XRECTF &bounds, _Out_ CMILMatrix *pMatrix);

    bool HasClipAnimation() const { return m_hasClipAnimation; }
    void SetHasClipAnimation() { m_hasClipAnimation = TRUE; }

    _Check_return_ HRESULT ClipToTransitionTarget(
        _Inout_ XRECTF& bounds,
        _Inout_ XPOINTF& target,
        _Out_ bool *pHit
        );

    _Check_return_ HRESULT ClipToTransitionTarget(
        _Inout_ XRECTF& bounds,
        _Inout_ HitTestPolygon& target,
        _Out_ bool *pHit
        );

    bool NeedsWUCOpacityExpression();

    // Ensures an expression, but fills in only the transition target opacity animation.
    void EnsureWUCOpacityExpression(_Inout_ WinRTExpressionConversionContext* context);

    // Ensures an expression, but fills in the transition target opacity animation as well as a static prepend opacity.
    // If the opacity animation is already up-to-date then it will not be touched.
    void EnsureWUCOpacityExpression(
        _Inout_ WinRTExpressionConversionContext* context,
        float prependOpacity);

    void ClearWUCOpacityExpression();

    xref_ptr<WUComp::IExpressionAnimation> GetWUCOpacityExpression() const;

    void CleanupDeviceRelatedResourcesRecursive(bool cleanupDComp) override;
    void ReleaseDCompResources() final;

    void EnsureWUCAnimationStarted(_Inout_ WinRTExpressionConversionContext* context) override;

    void ReplaceTransform(_In_ CCompositeTransform* newTransform);

private:
    void TransformBounds(
        _Inout_ XRECTF& bounds
        );

    void ApplyClipTransformOrigin(
        _In_ const XRECTF& bounds,
        _Inout_ CMILMatrix *pMatrix
        );

public:
    CCompositeTransform *m_pxf              = nullptr;  // UIElement.RenderTransform
    CCompositeTransform *m_pClipTransform   = nullptr;  // Transform on the clip
    XFLOAT m_opacity                        = 1.0f;     // UIElement.Opacity
    XPOINTF m_ptRenderTransformOrigin       = {};       // UIElement.RenderTransformOrigin
    XPOINTF m_ptClipTransformOrigin         = {};       // TransitionTarget.ClipTransformOrigin - mimicks RT origin for clip

    xref_ptr<WUComp::IExpressionAnimation> m_opacityExpression;

    bool m_hasClipAnimation : 1;
    bool m_isOpacityAnimationDirty : 1;

private:
    TransitionTargetDirtyMode m_currentDirtyMode = TransitionTargetDirtyMode::Dirty_All;
};
