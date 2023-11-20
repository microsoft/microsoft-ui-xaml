// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "uielement.h"
#include "TransitionTarget.h"
#include "Matrix.h"
#include "Transform.h"
#include "CompositeTransform.h"
#include "XamlLocalTransformBuilder.h"
#include "CValueBoxer.h"
#include <MetadataAPI.h>
#include <RuntimeEnabledFeatures.h>
#include <DependencyLocator.h>
#include <MUX-ETWEvents.h>
#include <DependencyObjectDCompRegistry.h>
#include <Transform3D.h>
#include <UIElementCollection.h>
#include <LayoutTransitionElement.h>
#include <WinRTExpressionConversionContext.h>
#include <ExpressionHelper.h>
#include <DCompTreeHost.h>
#include <DOPointerCast.h>
#include <ImplicitAnimations.h>
#include <TranslateTransform.h>
#include <ScaleTransform.h>
#include <RotateTransform.h>
#include <MatrixTransform.h>
#include <TransformGroup.h>
#include <TransformCollection.h>
#include <Projection.h>
#include <RectangleGeometry.h>
#include <HWCompNode.h>
#include <Corep.h>
#include <UIElementStructs.h>
#include <XamlLightCollection.h>
#include <DependencyObject.h>
#include <XamlLight.g.h>
#include <XamlLight.h>
#include <Popup.h>
#include <RootVisual.h>
#include <VisualTreeHelper.h>
#include <WRLHelper.h>
#include <PropertySetListener.h>
#include <FacadeAnimationHelper.h>
#include <string>

#include "DependencyObject.h"
#include "BrushTransition.g.h"
#include "ScalarTransition.g.h"
#include "Vector3Transition.g.h"
#include "ManagedObjectReference.h"
#include "ExternalObjectReference.g.h"
#include "PropertyTransitions.h"
#include <FxCallbacks.h>

using namespace DirectUI;

const wchar_t* const CUIElement::s_translationString = L"Translation";

CUIElement::CUIElement(_In_ CCoreServices* core)
    : CDependencyObject(core)
    , m_pEventList(nullptr)
    , m_eOpacityPrivate(1.0f)
    , m_pTextFormatting(nullptr)
    , m_pLayoutTransitionStorage(nullptr)
    , m_enteredTreeCounter(XUINT16_MAX)
    , m_leftTreeCounter(XUINT16_MAX)
    , m_pAP(nullptr)
    , m_contentRenderData()
    , m_isRightToLeftGeneration(0)
    , m_isRightToLeft(FALSE)
    , m_fVisibility(static_cast<unsigned int>(DirectUI::Visibility::Visible))
    , m_fHitTestVisible(TRUE)
    , m_fAllowDrop(FALSE)
    , m_bTapEnabled(TRUE)
    , m_bDoubleTapEnabled(TRUE)
    , m_bRightTapEnabled(TRUE)
    , m_bHoldEnabled(TRUE)
    , m_bCanDrag(FALSE)
    , m_fIsEnabled(TRUE)
    , m_fCoercedIsEnabled(TRUE)
    , m_fUseLayoutRounding(EnterParams::UseLayoutRoundingDefault)
    , m_fNWVisibilityDirty(FALSE)
    , m_fNWTransformDirty(FALSE)
    , m_fNWProjectionDirty(FALSE)
    , m_isTransform3DDirty(FALSE)
    , m_fNWClipDirty(FALSE)
    , m_fNWOpacityDirty(FALSE)
    , m_fNWCompositeModeDirty(FALSE)
    , m_fNWContentDirty(FALSE)
    , m_fNWSubgraphDirty(FALSE)
    , m_fNWLayoutClipDirty(FALSE)
    , m_fNWHasOpacityAnimationDirty(FALSE)
    , m_fNWHasCompNodeDirty(FALSE)
    , m_fNWRedirectionDataDirty(FALSE)
    , m_fNWHadUserClip(FALSE)
    , m_fNWHadLayoutClip(FALSE)
    , m_hasTransformIA(FALSE)
    , m_hasProjectionIA(FALSE)
    , m_hasTransform3DIA(FALSE)
    , m_hasLocalOpacityIA(FALSE)
    , m_hasTransitionOpacityIA(FALSE)
    , m_hasLocalClipIA(FALSE)
    , m_hasTransitionClipIA(FALSE)
    , m_hasBrushColorIA(FALSE)
    , m_isManipulatable(FALSE)
    , m_hasManipulation(FALSE)
    , m_hasClipManipulation(FALSE)
    , m_isRootElement(FALSE)
    , m_isRedirectionElement(FALSE)
    , m_hasNonIdentityProjection(FALSE)
    , m_hasAxisUnalignedTransform(FALSE)
    , m_hasAxisUnalignedLocalClip(FALSE)
    , m_hasSwapChainContent(FALSE)
    , m_isUsingHandOffVisual(FALSE)
    , m_listeningForHandOffVisualPropertyChanges(false)
    , m_isUsingHandInVisual(FALSE)
    , m_usesCompositeMode(FALSE)
    , m_fIsDirectManipulationCrossSlideContainer(FALSE)
    , m_fIsDirectManipulationContainer(FALSE)
    , m_contentInnerboundsDirty(TRUE)
    , m_childBoundsDirty(TRUE)
    , m_combinedInnerBoundsDirty(TRUE)
    , m_outerBoundsDirty(TRUE)
    , m_skipFocusSubtree_OffScreenPosition()
    , m_skipFocusSubtree_Other()
    , m_contentInnerBounds()
    , m_childBounds()
    , m_combinedInnerBounds()
    , m_outerBounds()
    , m_propertyRenderData()
    , m_fireUIAPropertyChanges(FALSE)
    , m_pCompositionPeer(nullptr)
    , m_layoutFlags(0)
    , m_pChildren(nullptr)
    , m_pLayoutTransitionRenderers(nullptr)
    , m_isCanvasLeftAnimationDirty(false)
    , m_isCanvasTopAnimationDirty(false)
    , m_bBindingHidden(false)
    , m_requiresHitTestInvisibleCompNode(false)
    , m_isRedirectedChildOfSwapChainPanel(false)
    , m_hasTransform3D(false)
    , m_isNonClippingSubtree(false)
    , m_has3DDepthInSubtree(false)
    , m_fIsAutomationPeerFactorySet(false)
    , m_hasOffsetIA(false)
    , m_hasActiveConnectedAnimation(false)
    , m_hasEverStoredHandoffOrHandinVisual(false)
    , m_isScrollViewerHeader(false)
    , m_isItemContainer(false)
    , m_has3DDepth(false)
    , m_hasImplicitShowAnimation(false)
    , m_hasImplicitHideAnimation(false)
    , m_requestedPlayImplicitShow(false)
    , m_requestedPlayImplicitHide(false)
    , m_isLightTargetDirty(false)
    , m_isLightCollectionDirty(false)
    , m_isLightTargetOrHasLight(false)
    , m_isRenderTargetSource(false)
    , m_isTranslationEnabled(false)
    , m_isEntireSubtreeDirty(false)
    , m_eKeyboardNavigationMode(static_cast<unsigned int>(TAB_NAVIGATION_NOTSET))
    , m_forceNoCulling(false)
    , m_requiresCompNodeForRoundedCorners(false)
    , m_allowsDragAndDropPassThrough(false)
    , m_isHitTestingSuppressed(false)
    , m_hasFacadeAnimation(false)
    , m_hasTranslateZ(false)
    , m_hasNonZeroRotation(false)
    , m_hasScaleZ(false)
    , m_hasNonIdentityTransformMatrix(false)
    , m_hasNonZeroCenterPoint(false)
    , m_hasNonDefaultRotationAxis(false)
    , m_areFacadesInUse(false)
    , m_isScroller(false)
    , m_hasAttachedInteractions(false)
    , m_isShadowCaster(false)
    , m_isProjectedShadowDefaultReceiver(false)
    , m_isProjectedShadowCustomReceiver(false)
    , m_isKeepingAliveUntilHiddenEventIsRaised(false)
    , m_isTransitionRootWithChildren(false)
    , m_hasWUCOpacityAnimation(false)
    , m_hasLayoutStorage(false)
    , m_isTabStop(false)
    , m_isProtectedCursorSet(false)
    , m_shouldFadeInDropShadow(false)
{
    TraceElementCreatedInfo((UINT64)static_cast<CDependencyObject*>(this));

    ASSERT(IsEmptyRectF(m_contentInnerBounds) && IsEmptyRectF(m_childBounds)
        && IsEmptyRectF(m_combinedInnerBounds) && IsEmptyRectF(m_outerBounds));
}

FacadeTransformInfo CUIElement::GetFacadeTransformInfo(bool preferAnimatingValue) const
{
    FacadeTransformInfo facadeInfo;
    if (AreFacadesInUse())
    {
        facadeInfo.scale = GetScale(preferAnimatingValue);
        facadeInfo.rotationAngleInDegrees = -GetRotation(preferAnimatingValue); // Negated, DComp uses opposite handed-ness for rotation
        facadeInfo.rotationAxis = GetRotationAxis(preferAnimatingValue);
        facadeInfo.transformMatrix = GetTransformMatrix(preferAnimatingValue);
        facadeInfo.centerPoint = GetCenterPoint(preferAnimatingValue);
        facadeInfo.translationZ = GetTranslation(preferAnimatingValue).Z;
    }
    return facadeInfo;
}

// Gets the combined local transform. Returns TRUE if its identity, FALSE otherwise
/* static */ void CUIElement::GetLocalTransformHelper(
    _Inout_ LocalTransformBuilder* pBuilder,
    XFLOAT offsetX,
    XFLOAT offsetY,
    XFLOAT dmOffsetX,
    XFLOAT dmOffsetY,
    XFLOAT dmZoomFactorX,
    XFLOAT dmZoomFactorY,
    bool flipRTL,
    bool flipRTLInPlace,
    XFLOAT elementWidth,
    XFLOAT elementHeight,
    _In_opt_ CTransform *pRenderTransform,
    XPOINTF renderTransformOrigin,
    _In_opt_ CTransitionTarget* pTransitionTarget,
    bool applyDMZoomToOffset,
    _In_opt_ IUnknown* pDManipSharedTransform,
    _In_opt_ RedirectionTransformInfo* redirInfo,
    _In_opt_ FacadeTransformInfo* facadeInfo
    )
{
#ifdef DMUIEv_DBG
    if (dmOffsetX != 0.0f || dmOffsetY != 0.0f || dmZoomFactor != 1.0f)
    {
        IGNOREHR(gps->DebugOutputSzNoEndl(L"DMUIEv: GetLocalTransform ThreadID=%d, X=%4.6lf, Y=%4.6lf, dmX=%4.6lf, dmY=%4.6lf, dmZx=%4.8lf, dmZy=%4.8lf\r\n",
            GetCurrentThreadId(), offsetX, offsetY, dmOffsetX, dmOffsetY, dmZoomFactorX, dmZoomFactorY));
    }
#endif // DMUIEv_DBG

    if (facadeInfo != nullptr)
    {
        pBuilder->ApplyFacadeTransforms(facadeInfo);
    }

    if (pRenderTransform != NULL)
    {
        float originX = elementWidth * renderTransformOrigin.x;
        float originY = elementHeight * renderTransformOrigin.y;

        pBuilder->ApplyRenderTransform(pRenderTransform, originX, originY);
    }

    if (pTransitionTarget != NULL)
    {
        float originX = elementWidth * pTransitionTarget->m_ptRenderTransformOrigin.x;
        float originY = elementHeight * pTransitionTarget->m_ptRenderTransformOrigin.y;

        pBuilder->ApplyTransitionTargetRenderTransform(pTransitionTarget->m_pxf, originX, originY);
    }

    pBuilder->ApplyFlowDirection(
        flipRTL == TRUE,
        flipRTLInPlace == TRUE,
        elementWidth    // unscaledElementWidth
        );

    pBuilder->ApplyOffsetAndDM(
        offsetX,
        offsetY,
        dmOffsetX,
        dmOffsetY,
        dmZoomFactorX,
        dmZoomFactorY,
        applyDMZoomToOffset
        );

    if (pDManipSharedTransform != nullptr)
    {
        pBuilder->ApplyDManipSharedTransform(pDManipSharedTransform);
    }

    if (redirInfo != nullptr)
    {
        pBuilder->ApplyRedirectionTransform(redirInfo);
    }
}

DirectUI::ManipulationModes CUIElement::GetManipulationMode() const
{
    CValue result;
    VERIFYHR(GetValueByIndex(KnownPropertyIndex::UIElement_ManipulationMode, &result));
    return static_cast<DirectUI::ManipulationModes>(result.AsEnum());
}

DirectUI::ManipulationModes CUIElement::GetManipulationModeCore() const
{
    CValue result;
    VERIFYHR(const_cast<CUIElement*>(this)->GetEffectiveValueInSparseStorage(
        MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::UIElement_ManipulationMode),
        &result));
    return static_cast<DirectUI::ManipulationModes>(result.AsEnum());
}

_Check_return_ HRESULT CUIElement::SetManipulationModeCore(_In_ DirectUI::ManipulationModes value)
{
    CValue v;
    v.Set(value, KnownTypeIndex::ManipulationModes);
    IFC_RETURN(SetEffectiveValueInSparseStorage(
        MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::UIElement_ManipulationMode),
        v));
    return S_OK;
}

_Check_return_ HRESULT CUIElement::ManipulationMode(
    _In_ CDependencyObject* obj,
    _In_ UINT32 numberOfArgs,
    _Inout_updates_(numberOfArgs) CValue* args,
    _In_opt_ IInspectable* valueOuter,
    _Out_ CValue* result)
{
    if (numberOfArgs == 1)
    {
        IFC_RETURN(static_cast<CUIElement*>(obj)->SetManipulationModeCore(static_cast<DirectUI::ManipulationModes>(args->AsEnum())));
        return S_OK;
    }
    else
    {
        result->Set(static_cast<CUIElement*>(obj)->GetManipulationModeCore(), KnownTypeIndex::ManipulationModes);
        return S_OK;
    }
}

// Returns the global scale factor stored in sparse storage or 0.0f otherwise.
FLOAT CUIElement::GetGlobalScaleFactorCore() const
{
    CValue result;
    VERIFYHR(const_cast<CUIElement*>(this)->GetEffectiveValueInSparseStorage(
        MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::UIElement_GlobalScaleFactor),
        &result));
    return result.AsFloat();
}

// Stores the provided global scale factor in sparse storage.
_Check_return_ HRESULT CUIElement::SetGlobalScaleFactorCore(_In_ FLOAT value)
{
    CValue v;
    v.SetFloat(value);
    IFC_RETURN(SetEffectiveValueInSparseStorage(
        MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::UIElement_GlobalScaleFactor),
        v));
    return S_OK;
}

// Removes the global scale factor from sparse storage if it was previously set.
_Check_return_ HRESULT CUIElement::ResetGlobalScaleFactor()
{
    IFC_RETURN(ClearEffectiveValueInSparseStorage(
        MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::UIElement_GlobalScaleFactor)));
    return S_OK;
}

// Gets or sets the global scale factor for the provided element.
_Check_return_ HRESULT CUIElement::GlobalScaleFactor(
    _In_ CDependencyObject* obj,
    _In_ UINT32 numberOfArgs,
    _Inout_updates_(numberOfArgs) CValue* args,
    _In_opt_ IInspectable* valueOuter,
    _Out_ CValue* result)
{
    if (numberOfArgs == 1)
    {
        IFC_RETURN(static_cast<CUIElement*>(obj)->SetGlobalScaleFactorCore(args->AsFloat()));
        return S_OK;
    }
    else
    {
        result->SetFloat(static_cast<CUIElement*>(obj)->GetGlobalScaleFactorCore());
        return S_OK;
    }
}

xref_ptr<WUComp::IExpressionAnimation> CUIElement::GetWUCCanvasOffsetExpression() const
{
    xref_ptr<WUComp::IExpressionAnimation> expression;

    CValue result;
    IFCFAILFAST(GetValueByIndex(KnownPropertyIndex::UIElement_CanvasOffset, &result));

    IUnknown* unknown = result.AsIUnknown();
    if (unknown != nullptr)
    {
        IFCFAILFAST(unknown->QueryInterface(IID_PPV_ARGS(expression.ReleaseAndGetAddressOf())));
    }

    return expression;
}

xref_ptr<WUComp::IExpressionAnimation> CUIElement::GetWUCOpacityExpression() const
{
    xref_ptr<WUComp::IExpressionAnimation> expression;

    CValue result;
    IFCFAILFAST(GetValueByIndex(KnownPropertyIndex::UIElement_OpacityExpression, &result));

    IUnknown* unknown = result.AsIUnknown();
    if (unknown != nullptr)
    {
        IFCFAILFAST(unknown->QueryInterface(IID_PPV_ARGS(expression.ReleaseAndGetAddressOf())));
    }

    return expression;
}

xref_ptr<CTransitionTarget> CUIElement::GetTransitionTarget() const
{
    // Perf:  Avoid overhead of GetValueByIndex if the effective value is not actually set
    if (IsEffectiveValueInSparseStorage(KnownPropertyIndex::UIElement_TransitionTarget))
    {
        CValue result;
        VERIFYHR(GetValueByIndex(KnownPropertyIndex::UIElement_TransitionTarget, &result));
        return static_sp_cast<CTransitionTarget>(result.DetachObject());
    }
    return xref_ptr<CTransitionTarget>(nullptr);
}

FLOAT CUIElement::GetOffsetX() const
{
    CValue result;
    VERIFYHR(GetValueByIndex(KnownPropertyIndex::Canvas_Left, &result));
    return static_cast<FLOAT>(result.AsDouble());
}

FLOAT CUIElement::GetOffsetY() const
{
    CValue result;
    VERIFYHR(GetValueByIndex(KnownPropertyIndex::Canvas_Top, &result));
    return static_cast<FLOAT>(result.AsDouble());
}

INT32 CUIElement::GetZIndex() const
{
    CValue result;
    VERIFYHR(const_cast<CUIElement*>(this)->GetEffectiveValueInSparseStorage(
        MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::Canvas_ZIndex),
        &result));
    return result.AsSigned();
}

wrl::ComPtr<mui::IInputCursor> CUIElement::GetProtectedCursor()
{
    CValue value;
    VERIFYHR(const_cast<CUIElement*>(this)->GetEffectiveValueInSparseStorage(
        MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::UIElement_ProtectedCursor),
        &value));

    wrl::ComPtr<IInspectable> unwrappedValue;
    IFCFAILFAST(CValueBoxer::UnboxObjectValue(&value, nullptr, &unwrappedValue));

    wrl::ComPtr<mui::IInputCursor> result;
    IFCFAILFAST(unwrappedValue->QueryInterface(IID_PPV_ARGS(&result)));

    return result;
}

FLOAT ClampOpacity(_In_ FLOAT opacity)
{
    if (opacity < 0) return 0;
    if (opacity > 1) return 1;
    return opacity;
}

void CUIElement::HideElementForSuspendRendering(bool hidden)
{
    m_bBindingHidden = hidden;
    CUIElement::NWSetOpacityDirty(this, DirtyFlags::Render);
}

//  Combined opacity from the transition target and the UIElement.
//  This is the opacity that should be used in almost all cases (as opposed to GetOpacityLocal()).
FLOAT CUIElement::GetOpacityCombined()
{
    FLOAT fpOpacity = GetOpacityLocal() * (m_bBindingHidden ? 0.0f : 1.0f);

    auto transitionTarget = GetTransitionTarget();
    return transitionTarget ? fpOpacity * ClampOpacity(transitionTarget->m_opacity) : fpOpacity;
}

//  Walk up tree and calculate effective opacity for this element
FLOAT CUIElement::GetEffectiveOpacity()
{
    CUIElement *pElement = this;
    FLOAT effectiveOpacity = 1.0f;
    // Need to walk up and figure out the the parent chain.
    while (pElement != nullptr)
    {
        effectiveOpacity = ClampOpacity(effectiveOpacity * pElement->GetOpacityCombined());
        if (effectiveOpacity == 0.0f) // no need to go up tree anymore
        {
            break;
        }
        // Move to immediate parent
        pElement = pElement->GetUIElementParentInternal();
    }

    return effectiveOpacity;
}

FLOAT CUIElement::GetOpacityLocal()
{
    return ClampOpacity(m_eOpacityPrivate);
}

void CUIElement::SetOpacityLocal(FLOAT value)
{
    m_eOpacityPrivate = value;
}

void CUIElement::ReleaseDCompResources()
{
    __super::ReleaseDCompResources();

    VERIFYHR(ClearValue(MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::UIElement_CanvasOffset)));
    SetDCompAnimation(nullptr, KnownPropertyIndex::UIElement_OffsetXAnimation);
    SetDCompAnimation(nullptr, KnownPropertyIndex::UIElement_OffsetYAnimation);

    VERIFYHR(ClearValue(MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::UIElement_OpacityExpression)));
    SetDCompAnimation(nullptr, KnownPropertyIndex::UIElement_OpacityAnimation);
}

DirectUI::ElementCompositeMode CUIElement::GetCompositeMode() const
{
    CValue result;
    VERIFYHR(GetValueByIndex(KnownPropertyIndex::UIElement_CompositeMode, &result));
    return static_cast<DirectUI::ElementCompositeMode>(result.AsEnum());
}

// Returns true if this element has an explicit composite mode set (ie a composite mode other than Inherit).
bool CUIElement::IsUsingCompositeMode() const
{
    return m_usesCompositeMode;
}

bool CUIElement::HasActiveConnectedAnimation() const
{
    return m_hasActiveConnectedAnimation;
}

// Returns true if this element needs a composition node
// because a WinRT visual is necessary for the composition capture API.
bool CUIElement::IsRenderTargetSource() const
{
    return m_isRenderTargetSource;
}

CUIElementCollection* CUIElement::GetChildren()
{
    return m_pChildren;
}

CUIElementCollectionWrapper CUIElement::GetUnsortedChildren()
{
    return CUIElementCollectionWrapper(GetChildren());
}

void CUIElement::LockParent()
{
    CUIElement* pParent = GetUIElementParentInternal();
    if (pParent)
    {
        CCollection *pCollection = pParent->GetChildren();
        if (pCollection)
        {
            pCollection->Lock();
        }
    }
}

void CUIElement::UnlockParent()
{
    CUIElement* pParent = GetUIElementParentInternal();
    if (pParent)
    {
        CCollection *pCollection = pParent->GetChildren();
        if (pCollection)
        {
            pCollection->Unlock();
        }
    }
}

bool CUIElement::HasDepthLegacy() const
{
    if (HasHandOffVisualTransform())
    {
        // If there's a private stash then use that. Even if there's a Transform3D property set, it's possible that the app
        // has overwritten it with a WUC TransformMatrix that's 2D, in which case we should ignore the Transform3D. Check
        // whether the private stash is 3D and proceed from there. This only applies in container visuals mode.

        if (HasHandOffVisualTransformMatrix3D())
        {
            const auto& handOffMatrix3D = GetHandOffVisualTransformMatrix3D();
            return !handOffMatrix3D.Is2D();
        }
        else
        {
            // There's only a 2D transform in the private stash. That means there's no depth. Even if there's a Transform3D
            // property set, that transform isn't being rendered right now. The app has overwritten it with some 2D matrix
            // in the hand off visual.
            return false;
        }
    }
    else if (AreFacadesInUse())
    {
        FacadeTransformInfo facadeInfo = GetFacadeTransformInfo(true /* preferAnimatingValue */);
        return !facadeInfo.Is2D();
    }
    else
    {
        // With no private stash, we refer to the Xaml property.
        //
        // Bug 17258018
        // There's a subtle behavior here when cleaning up unreachable objects from managed code that leaves this element in an
        // inconsistent state, with m_hasTransform3D set yet with a null GetTransform3D().
        //
        // CDependencyObject::ResetReferencesFromSparsePropertyValues will iterate through sparse properties, find child DOs,
        // and call ResetReferenceFromChild on them _before_ clearing the property associated with the child. For Transform3D
        // that means the child CompositeTransform3D object gets unparented from this element while this element still has the
        // child in its Transform3D sparse storage. Unparenting the child then goes through the dirty flag propagation method,
        // CUIElement::NWSetTransform3DDirty. Normally this will unmark m_hasTransform3D, but in this case we still have that
        // CompositeTransform3D in sparse storage, so we leave the m_hasTransform3D flag set. ResetReferencesFromSparsePropertyValues
        // will then clear the sparse storage m_pValueTable, which removes the CompositeTransform3D in sparse storage while
        // leaving m_hasTransform3D set. This element is left in an inconsistent state.
        //
        // This is normally not a problem because this element is about to be deleted, but if we get here during subtree
        // cleanup (in the old 3D hit testing walk, via a sibling's CUIElement::PropagateDepthInSubtree, which calls the
        // parent's CUIElement::UpdateHas3DDepthInSubtree -> ComputeDepthInSubtree, which calls down here), then we'll AV
        // because m_hasTransform3D makes us look at the null Transform3D. As a workaround, make sure GetTransform3D() isn't
        // null first.
        xref_ptr<CTransform3D> transform3D = GetTransform3D();
        return HasTransform3DCompositionRequirement() && transform3D != nullptr && transform3D->HasDepth();
    }
}

// Returns either the Transform3D or nullptr
xref_ptr<CTransform3D> CUIElement::GetTransform3D() const
{
    // Perf:  Avoid overhead of GetValueByIndex if the effective value is not actually set
    if (IsEffectiveValueInSparseStorage(KnownPropertyIndex::UIElement_Transform3D))
    {
        CValue result;
        VERIFYHR(GetValueByIndex(KnownPropertyIndex::UIElement_Transform3D, &result));
        return static_sp_cast<CTransform3D>(result.DetachObject());
    }
    return xref_ptr<CTransform3D>(nullptr);
}

bool CUIElement::HasTransform3DForHitTestingLegacy() const
{
    return m_has3DDepth;
}

// Returns true if any child of this element has depth, or depth in their subtree.
bool CUIElement::ComputeDepthInSubtree()
{
    CUIElementCollection* children = GetChildren();
    if (children != nullptr)
    {
        for (auto& childDO : *children)
        {
            CUIElement* child = static_cast<CUIElement*>(childDO);

            // If there's an LTE targeting a child element, we need to look at whether that LTE has 3D depth as well.
            // See LTE comment in CUIElement::PropagateDepthInSubtree for the scenario.
            if (child->Has3DDepthOnSelfOrSubtreeOrLTETargetingSelf())
            {
                return true;
            }
        }
    }

    // TODO: HitTest: if a child in the transition root has 3D, does this need to check for it?

    return false;
}

bool CUIElement::Has3DDepthOnSelfOrSubtreeOrLTETargetingSelf() const
{
    if (Has3DDepthOnSelfOrSubtree())
    {
        return true;
    }
    else if (IsHiddenForLayoutTransition())
    {
        CUIElement* lteTargetingThis = GetFirstLTETargetingThis();
        if (lteTargetingThis->Has3DDepthOnSelfOrSubtree()) // We don't have LTEs pointing to LTEs.
        {
            return true;
        }
    }

    return false;
}

void CUIElement::PropagateDepthInSubtree()
{
    CUIElement* parent = GetParentOrLTEForWalkUp();
    if (parent != nullptr)
    {
        parent->UpdateHas3DDepthInSubtree();

        if (parent->OfTypeByIndex<KnownTypeIndex::PopupRoot>())
        {
            // The child of an open popup is rooted under the CPopupRoot directly, so we'll never reach the parent popup
            // by walking up the visual tree. If we reach the CPopupRoot, find the associated popup and propagate 3D
            // depth to it as well. This is needed for 3D hit testing, which follows the trail of m_has3DDepthInSubtree
            // flags down the tree and expects it to be set on the popups themselves too.

            CPopupRoot* popupRoot = static_cast<CPopupRoot*>(parent);
            CPopup* parentPopup = popupRoot->GetOpenPopupWithChild(this, false /* checkUnloadingChildToo */);
            if (parentPopup != nullptr)
            {
                parentPopup->UpdateHas3DDepthInSubtree();
            }
        }
    }

    if (OfTypeByIndex<KnownTypeIndex::Popup>())
    {
        // Also propagate the bit to the CPopupRoot. Since we walk to all elements with the m_has3DDepthInSubtree flag set,
        // this guarantees that we'll walk the popup contents (rooted under the CPopupRoot) if the popup itself has 3D depth.
        CPopupRoot* popupRoot = static_cast<CPopup*>(this)->GetAssociatedPopupRootNoRef();
        if (popupRoot != nullptr)
        {
            popupRoot->UpdateHas3DDepthInSubtree();
        }

        // Note: Popups still need to propagate 3D depth in the main tree. When we do redirected hit testing, we'll walk the
        // main tree to collect transforms, and the 3D depth markers will make us collect transforms above the popup. The
        // collected transforms will be used once we cross the 3D boundary underneath the popup.
    }
    else if (OfTypeByIndex<KnownTypeIndex::LayoutTransitionElement>())
    {
        // Here's the scenario we're trying to catch:
        //
        //  <Root>
        //      <A>
        //          <B Offset="100">
        //              <C>
        //                  <D Transform3D="...">
        //                      ...
        //                  </D>
        //              </C>
        //          </B>
        //          <LTE Target="C" />
        //      </A>
        //  </Root>
        //
        // Element D has a 3D transform and needs the transforms of all its ancestors to be collected. It's rendering inside
        // a redirected tree (LTE, which points to its parent C). The transforms that we're after are Root-A-B-LTE-D. The
        // tricky element here is B, because if we just follow the ancestor chain and LTEs up from D, we'll reach C, then LTE,
        // then skip directly to A (LTE's parent). We need to mark B with 3D depth as well, because we need its offset when
        // doing redirected hit testing.

        CUIElement* lteTarget = static_cast<CLayoutTransitionElement*>(this)->GetTargetElement();
        if (lteTarget != nullptr)
        {
            // Explicitly go to the target's parent element. If we call lteTarget->UpdateHas3DDepthInSubtree, that's just
            // going to propagate to the LTE again.
            CUIElement* lteTargetParent = lteTarget->GetUIElementParentInternal(false /* publicParentOnly */);
            if (lteTargetParent != nullptr)
            {
                lteTargetParent->UpdateHas3DDepthInSubtree();
            }
        }
    }
}

void CUIElement::UpdateHas3DDepth()
{
    bool hasDepth = false;

    // First check for 3D in facades.  Note that since Translation isn't a strict property, it can be present
    // in combination with the other scenarios below, hence we check for both.
    if (AreFacadesInUse())
    {
        FacadeTransformInfo facadeInfo = GetFacadeTransformInfo(true /* preferAnimatingValue */);
        hasDepth = !facadeInfo.Is2D();
    }
    if (!hasDepth)
    {
        // This should be checking m_hasTransform3D, but unit tests don't know how to set m_hasTransform3D, so we check
        // the Transform3D property. At this point the flag should be set iff there's a Transform3D property set.
        if (GetTransform3D() != nullptr)
        {
            hasDepth = GetTransform3D()->HasDepth();
        }
        else
        {
            // If we were in the 3D hit testing code path and just lost a Transform3D, then check whether we should drop
            // out of 3D hit testing. It's possible that there's no Xaml Transform3D property but the app has written a WUC
            // Visual.TransformMatrix that's 3D. If that's the case then we should stay in the 3D hit testing code path.
            // This is the case in both container visuals mode and legacy DComp visuals mode.
            bool hasHandOffVisualTransformMatrix3D = HasHandOffVisualTransformMatrix3D();
            hasDepth = hasHandOffVisualTransformMatrix3D && !GetHandOffVisualTransformMatrix3D().Is2D();
        }
    }
    SetHas3DDepth(hasDepth);
}

void CUIElement::SetHas3DDepth(bool value)
{
    if (m_has3DDepth != value)
    {
        m_has3DDepth = value;
        PropagateDepthInSubtree();
    }
}

void CUIElement::UpdateHas3DDepthInSubtree()
{
    bool oldHasDepthInSubtree = m_has3DDepthInSubtree;

    if (OfTypeByIndex<KnownTypeIndex::PopupRoot>())
    {
        CPopupRoot* thisPopupRoot = static_cast<CPopupRoot*>(this);
        m_has3DDepthInSubtree = thisPopupRoot->ComputeDepthInOpenPopups();
    }
    else if (OfTypeByIndex<KnownTypeIndex::Popup>())
    {
        CPopup* thisPopup = static_cast<CPopup*>(this);
        if (thisPopup->m_pChild != nullptr)
        {
            // The m_has3DDepthInSubtree flag does not account for properties on the element itself, so we can ignore
            // the popup and go straight to its child.
            // If there's an LTE targeting a child element, we need to look at whether that LTE has 3D depth as well.
            // See LTE comment in CUIElement::PropagateDepthInSubtree for the scenario.
            m_has3DDepthInSubtree = thisPopup->m_pChild->Has3DDepthOnSelfOrSubtreeOrLTETargetingSelf();
        }
    }
    else if (OfTypeByIndex<KnownTypeIndex::LayoutTransitionElement>())
    {
        CLayoutTransitionElement* thisLTE = static_cast<CLayoutTransitionElement*>(this);
        if (thisLTE->GetTargetElement() != nullptr)
        {
            // The m_has3DDepthInSubtree flag does not account for properties on the element itself, so we can ignore
            // the LTE and go straight to its target.
            m_has3DDepthInSubtree = thisLTE->GetTargetElement()->Has3DDepthOnSelfOrSubtree();
        }
    }
    else
    {
        m_has3DDepthInSubtree = ComputeDepthInSubtree();
    }

    if (m_has3DDepthInSubtree != oldHasDepthInSubtree)
    {
        PropagateDepthInSubtree();
    }
}

CUIElement* CUIElement::GetFirstLTETargetingThis() const
{
    if (IsHiddenForLayoutTransition())
    {
        const auto& layoutElements = GetLayoutTransitionElements();
        CLayoutTransitionElement* pLTE = nullptr;
        IFCFAILFAST(layoutElements->get_item(0, pLTE));
        return pLTE;
    }
    return nullptr;
}

CUIElement* CUIElement::GetParentOrLTEForWalkUp()
{
    // Note: if there are multiple LTEs, then only return the first one. The walk up will only take
    // the first branch, rather than all of them.
    CUIElement* firstLTETargetingThis = GetFirstLTETargetingThis();
    if (firstLTETargetingThis != nullptr)
    {
        return firstLTETargetingThis;
    }
    else
    {
        // We want private parents too. That allows us to walk from transition roots to their parent UIElements.
        return GetUIElementParentInternal(false /* publicParentOnly */);
    }
}

bool CUIElement::IsOffsetIndependentlyAnimating() const
{
    return m_hasOffsetIA;
}

bool CUIElement::IsTransformAffectingPropertyIndependentlyAnimating() const
{
    return IsTransformIndependentlyAnimating()
        || IsProjectionIndependentlyAnimating()
        || IsTransform3DIndependentlyAnimating()
        || IsManipulatedIndependently();
}

bool CUIElement::IsTransformIndependentlyAnimating() const
{
    return m_hasTransformIA;
}

bool CUIElement::IsProjectionIndependentlyAnimating() const
{
    return m_hasProjectionIA;
}

bool CUIElement::IsTransform3DIndependentlyAnimating() const
{
    return m_hasTransform3DIA;
}

bool CUIElement::IsManipulatedIndependently() const
{
    return m_hasManipulation;
}

bool CUIElement::IsLocalOpacityIndependentlyAnimating() const
{
    return m_hasLocalOpacityIA;
}

bool CUIElement::IsTransitionOpacityIndependentlyAnimating() const
{
    return m_hasTransitionOpacityIA;
}

bool CUIElement::NeedsWUCOffsetExpression()
{
    // Paused Xaml animations will unmark the "has independent animation" flag but leave the WUC animation set on this UIElement.
    // In those cases we still need the expression to exist for when the Xaml/WUC animation resumes. If we destroy the WUC expression
    // we'll lose the current progress of the WUC animation inside.

    return IsOffsetIndependentlyAnimating()
        || GetWUCDCompAnimation(KnownPropertyIndex::UIElement_OffsetXAnimation) != nullptr
        || GetWUCDCompAnimation(KnownPropertyIndex::UIElement_OffsetYAnimation) != nullptr;
}

void CUIElement::EnsureWUCOffsetExpression(_Inout_ WinRTExpressionConversionContext* context)
{
    const bool isXAnimationDirty = IsCanvasLeftAnimationDirty();
    const bool isYAnimationDirty = IsCanvasTopAnimationDirty();

    if (isXAnimationDirty || isYAnimationDirty)
    {
        const float offsetX = GetActualOffsetX();
        const float offsetY = GetActualOffsetY();

        auto offsetExpression = GetWUCCanvasOffsetExpression();
        if (offsetExpression == nullptr)
        {
            wrl::ComPtr<WUComp::ICompositionPropertySet> propertySet;
            context->CreateExpression(ExpressionHelper::sc_Expression_Offset, offsetExpression.ReleaseAndGetAddressOf(), propertySet.ReleaseAndGetAddressOf());
            context->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_OffsetX, offsetX);
            context->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_OffsetY, offsetY);

            CValue val;
            val.SetIUnknownAddRef(offsetExpression.get());
            IFCFAILFAST(SetValueByKnownIndex(KnownPropertyIndex::UIElement_CanvasOffset, val));

            if (GetDCompObjectRegistry() != nullptr)
            {
                GetDCompObjectRegistry()->EnsureObjectWithDCompResourceRegistered(this);
            }
        }

        const auto& offsetXAnimation = GetWUCDCompAnimation(KnownPropertyIndex::UIElement_OffsetXAnimation);
        const auto& offsetYAnimation = GetWUCDCompAnimation(KnownPropertyIndex::UIElement_OffsetYAnimation);
        // Note: Animations with explicit Duration="Forever" will not generate WUC animations, so there's no guarantee that we'll have one.

        const auto& timeManager = GetTimeManager();

        context->UpdateExpression(offsetExpression.get(), ExpressionHelper::sc_paramName_OffsetX, offsetX, m_isCanvasLeftAnimationDirty, offsetXAnimation, this, KnownPropertyIndex::Canvas_Left, timeManager);
        context->UpdateExpression(offsetExpression.get(), ExpressionHelper::sc_paramName_OffsetY, offsetY, m_isCanvasTopAnimationDirty, offsetYAnimation, this, KnownPropertyIndex::Canvas_Top, timeManager);

        m_isCanvasLeftAnimationDirty = false;
        m_isCanvasTopAnimationDirty = false;
    }
}

void CUIElement::ClearWUCOffsetExpression()
{
    CValue val;
    val.SetNull();
    IFCFAILFAST(SetValueByKnownIndex(KnownPropertyIndex::UIElement_CanvasOffset, val));
    m_isCanvasLeftAnimationDirty = false;
    m_isCanvasTopAnimationDirty = false;
}

bool CUIElement::NeedsWUCOpacityExpression()
{
    return IsLocalOpacityIndependentlyAnimating()
        || GetWUCDCompAnimation(KnownPropertyIndex::UIElement_OpacityAnimation) != nullptr;
}

void CUIElement::EnsureWUCOpacityExpression(_Inout_ WinRTExpressionConversionContext* context)
{
    const bool isOpacityAnimationDirty = IsOpacityAnimationDirty();

    if (isOpacityAnimationDirty)
    {
        const float opacity = GetOpacityLocal();

        auto opacityExpression = GetWUCOpacityExpression();
        if (opacityExpression == nullptr)
        {
            wrl::ComPtr<WUComp::ICompositionPropertySet> propertySet;
            context->CreateExpression(ExpressionHelper::sc_Expression_Opacity, opacityExpression.ReleaseAndGetAddressOf(), &propertySet);
            context->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_Opacity, opacity);

            CValue val;
            val.SetIUnknownAddRef(opacityExpression.get());
            IFCFAILFAST(SetValueByKnownIndex(KnownPropertyIndex::UIElement_OpacityExpression, val));

            if (GetDCompObjectRegistry() != nullptr)
            {
                GetDCompObjectRegistry()->EnsureObjectWithDCompResourceRegistered(this);
            }
        }

        const auto& opacityAnimation = GetWUCDCompAnimation(KnownPropertyIndex::UIElement_OpacityAnimation);
        // Note: The opacity animation could be null in cases of reentrancy.
        //
        // Normally when we get here there's an opacity animation, because that's the only reason why a WUC visual would need an
        // expression for its opacity. But there could be cases of reentrancy where an opacity animation isn't found here. For
        // example, VSM starts a Xaml opacity animation, which ticks, creates a WUC animation, and marks a UIE with an opacity
        // animation. We then pump media events or fire loaded events, and in those handlers VSM stops the Xaml animation that was
        // started. Stopping that animation will release the WUC animation, but we won't tick the Xaml animation again and we won't
        // unmark the UIE as having an opacity animation until next frame. We then get here for the UIE that was marked as having
        // an opacity animation, and we find nothing. In those cases we should continue on - the next UI thread frame will unmark
        // the UIE as having an opacity animation, and we'll go back to using a static opacity and delete the expression created
        // here.

        const auto& timeManager = GetTimeManager();

        context->UpdateExpression(opacityExpression.get(), ExpressionHelper::sc_paramName_Opacity, opacity, isOpacityAnimationDirty, opacityAnimation, this, KnownPropertyIndex::UIElement_Opacity, timeManager);

        m_isOpacityAnimationDirty = false;
    }
}

void CUIElement::ClearWUCOpacityExpression()
{
    CValue val;
    val.SetNull();
    IFCFAILFAST(SetValueByKnownIndex(KnownPropertyIndex::UIElement_OpacityExpression, val));
    m_isOpacityAnimationDirty = false;
}

bool CUIElement::NeedsWUCTransitionOpacityExpression()
{
    // Paused Xaml animations will unmark the "has independent animation" flag but leave the WUC animation set on this UIElement.
    // In those cases we still need the expression to exist for when the Xaml/WUC animation resumes. If we destroy the WUC expression
    // we'll lose the current progress of the WUC animation inside.

    return IsTransitionOpacityIndependentlyAnimating()
        || (GetTransitionTarget() != nullptr && GetTransitionTarget()->NeedsWUCOpacityExpression());
}

void CUIElement::EnsureWUCAnimationStarted(_Inout_ WinRTExpressionConversionContext* context)
{
    if (NeedsWUCOffsetExpression())
    {
        EnsureWUCOffsetExpression(context);
    }
    else
    {
        ClearWUCOffsetExpression();
    }

    if (NeedsWUCOpacityExpression())
    {
        EnsureWUCOpacityExpression(context);
    }
    else
    {
        ClearWUCOpacityExpression();
    }
}

bool CUIElement::MustAttachWUCExpression(const KnownPropertyIndex propertyIndex) const
{
    bool isMarkedForIndependentAnimation = false;
    bool hasWUCAnimation = false;

    switch (propertyIndex)
    {
        case KnownPropertyIndex::TransitionTarget_Opacity:
            isMarkedForIndependentAnimation = IsTransitionOpacityIndependentlyAnimating();
            hasWUCAnimation = GetTransitionTarget() != nullptr && GetTransitionTarget()->NeedsWUCOpacityExpression();
            break;

        // Other properties omitted for now.
        // TODO: Switch the other properties to these methods too
        case KnownPropertyIndex::UIElement_CanvasOffset:
        case KnownPropertyIndex::UIElement_Opacity:
        case KnownPropertyIndex::UIElement_RenderTransform:
        case KnownPropertyIndex::UIElement_Projection:
        case KnownPropertyIndex::UIElement_Transform3D:
            // Fall through to the assert

        default:
            ASSERT(false);
            break;
    }

    // This assert will detect times when we hit 8667408. It can be enabled once that bug is fixed.
    //ASSERT(!isMarkedForIndependentAnimation || hasWUCAnimation);

    return isMarkedForIndependentAnimation && hasWUCAnimation;
}

bool CUIElement::CanReleaseWUCExpression(const KnownPropertyIndex propertyIndex) const
{
    bool hasWUCAnimation = false;

    switch (propertyIndex)
    {
        case KnownPropertyIndex::TransitionTarget_Opacity:
            hasWUCAnimation = GetTransitionTarget() != nullptr && GetTransitionTarget()->NeedsWUCOpacityExpression();
            break;

        // Other properties omitted for now.
        // TODO: Switch the other properties to these methods too
        case KnownPropertyIndex::UIElement_CanvasOffset:
        case KnownPropertyIndex::UIElement_Opacity:
        case KnownPropertyIndex::UIElement_RenderTransform:
        case KnownPropertyIndex::UIElement_Projection:
        case KnownPropertyIndex::UIElement_Transform3D:
            // Fall through to the assert

        default:
            ASSERT(false);
            break;
    }

    return !hasWUCAnimation;
}

// Notes on flags and flow of Implicit Show/Hide animations
// Implicit Show/Hide animations have several distinct "states":
//
// 1) Animation is Set:  This is done through ElementCompositionPreview API.  This just tells XAML there is an animation
//    to be played at the appropriate time, but doesn't request the animation be played yet.  The triggering is done implicitly.
//    m_hasImplicitShowAnimation/m_hasImplicitHideAnimation flag will be set TRUE when an animation is set.
//
// 2) XAML tree changes are made.  For the most part this only results in dirtying the tree for render, with one notable exception:
//    When elements are removed from the tree or Collapsed, if that element or one of its children has a Hide animation, the element
//    gets special treatment as it must be kept visible in the rendering tree while these animations are playing.
//    For more details on "keep visible", see DCompTreeHost::SetTrackKeepVisible().
//
// 3) Just before the RenderWalk runs, we re-evaluate the effective visibility of all elements with Show/Hide animations, as well
//    as keep-visible elements.  Any animations that need to be started are requested, but this request does not trigger the animation immediately.
//    The triggering is done on the next walk of the CompNode tree.  This state exists for a special reason:
//    At the time of requesting an animation, we haven't yet updated the properties on the visual being animated.  This needs to
//    happen before we trigger the animation, otherwise we may run into "property stomping".
//    m_requestedPlayImplicitShow/m_requestedPlayImplicitHide flag will be set TRUE when an animation is requested.
//
// 3) Animation is Playing:  The animation is finally scheduled by the CompNode tree walk, when it sees an animation is requested.
//    Note that playing the animation will clear the m_requestedPlayImplicitShow/m_requestedPlayImplicitHide flag.

// Helper method for setting/clearing implicit show/hide animation
void CUIElement::SetImplicitShowHideAnimation(ImplicitAnimationType iaType, _In_opt_ WUComp::ICompositionAnimationBase* animation)
{
    DCompTreeHost* dcompTreeHost = GetDCompTreeHost();
    DCompTreeHost::ImplicitAnimationsMap& iaMap = dcompTreeHost->GetImplicitAnimationsMap(iaType);

    // First see if we have a currently set animation or not
    wrl::ComPtr<WUComp::ICompositionAnimationBase> currentAnimation;
    auto itFind = iaMap.find(this);
    if (itFind != iaMap.end())
    {
        currentAnimation = itFind->second.implicitAnimation;
    }

    if (currentAnimation.Get() != animation)
    {
        if (IsImplicitAnimationPlaying(iaType))
        {
            // The behavior we want here is to continue playing the current animation until it completes, or is canceled.
            // We cannot replace our animation information until the current animation is done, so we set aside this
            // information and set the "pendingChange" flag to indicate we need to make this change later in CleanupImplicitAnimationOnCompletion().
            itFind->second.pendingChange = true;
            itFind->second.pendingAnimation = animation;
        }
        else
        {
            UpdateImplicitShowHideAnimationInfo(iaType, animation);
        }
    }
}

// Helper method for updating our internal info about an implicit show/hide animation in response to app setting/clearing it
void CUIElement::UpdateImplicitShowHideAnimationInfo(ImplicitAnimationType iaType, _In_opt_ WUComp::ICompositionAnimationBase* animation)
{
    DCompTreeHost* dcompTreeHost = GetDCompTreeHost();
    DCompTreeHost::ImplicitAnimationsMap& iaMap = dcompTreeHost->GetImplicitAnimationsMap(iaType);

    auto itFind = iaMap.find(this);

    // Clear out whatever we have stored about this implicit animation.  We may re-populate later.
    if (itFind != iaMap.end())
    {
        iaMap.erase(itFind);
    }

    if (animation == nullptr)
    {
        // Case 1:  The animation is transitioning from non-null to null
        // The element no longer requires a CompNode for this type of animation
        UnsetRequiresComposition(ImplicitAnimationTypeToCompositionRequirement(iaType), IndependentAnimationType::None);
    }
    else
    {
        // Case 2: The animation is transitioning from null to non-null
        // Case 3: The animation is transitioning from non-null to a different non-null
        // Store an entry for this animation in the iaMap.
        ImplicitAnimationInfo info;
        info.implicitAnimation = animation;
        auto emplaceResult = iaMap.emplace(this, info);
        ASSERT(emplaceResult.second == true);

        if (!HasImplicitAnimation(iaType))
        {
            // The node now requires a CompNode for this type of animation
            IFCFAILFAST(SetRequiresComposition(ImplicitAnimationTypeToCompositionRequirement(iaType), IndependentAnimationType::None));
        }
    }

    // Track the effective visibility of this element if it has an implicit animation, otherwise stop tracking.
    dcompTreeHost->SetTrackEffectiveVisibility(this, HasImplicitAnimation(ImplicitAnimationType::Show) || HasImplicitAnimation(ImplicitAnimationType::Hide));
}

// Compute the effective visibility of this element for implicit Show/Hide animations and for Shown/Hidden events
EffectiveVisibilityInfo CUIElement::ComputeEffectiveVisibility()
{
    CUIElement* current = this;

    EffectiveVisibilityInfo result;

    // Do a walk up the tree evaluating rules that determine if this element is considered
    // Visible or Hidden from the perspective of "should this trigger an implicit Show/Hide animation".
    // Opacity and clipping are not part of this criteria.
    // For an element to have effective visibility of Visible, it must be in the live tree, and all ancestors
    // must be visible, and not in the process of being unloaded (the equivalent of in the live tree).
    // In addition we also compute a special case "exemption" from triggering implicit animations (see more below).

    // Rule #1:  Not in live tree == Hidden
    // Note carefully that exemption does not kick in for this case as we don't trigger implicit animations
    // for non-active elements, but we still compute effective visibility in case it changes later.
    if (!IsActive())
    {
        if (IsVisible() && GetParentInternal() == nullptr && OfTypeByIndex<KnownTypeIndex::Popup>() && static_cast<CPopup*>(this)->IsOpen())
        {
            // The exception is visible open parentless popups. They're counted as visible.
            // (The parentless popup itself is not counted as being "in the live tree".)
            result.vis = EffectiveVisibility::Visible;
            return result;
        }
        else
        {
            result.vis = EffectiveVisibility::Hidden;
            return result;
        }
    }

    // From here we do a walk all the way up to root so that we can properly detect item container children.
    // We'll store this as we also compute effective visibility, this minimizes the number of extra walks we do overall.
    while (current != nullptr)
    {
        // Rule #2:  Visibility::Collapsed == Hidden
        if (!current->IsVisible())
        {
            result.vis = EffectiveVisibility::Hidden;
        }
        else if (current->OfTypeByIndex<KnownTypeIndex::Popup>() && !static_cast<CPopup*>(current)->IsOpen())
        {
            // Rule #2b:  Closed popup == Hidden
            result.vis = EffectiveVisibility::Hidden;
        }

        CUIElement* parent = do_pointer_cast<CUIElement>(current->GetParentFollowPopups());
        if (parent)
        {
            // Children of item containers are exempt from implicit animations
            // See more details in DCompTreeHost::UpdateImplicitShowHideAnimations().
            if (parent->GetIsItemContainer())
            {
                result.isExempt = true;
            }

            // Rule #3:  In Unloading storage == Hidden
            CUIElementCollection* children = parent->GetChildren();
            // If we jumped to a popup, the child collection could be null.
            if (children != nullptr && children->IsUnloadingElement(current))
            {
                result.vis = EffectiveVisibility::Hidden;
            }
        }
        current = parent;
    }

    return result;
}

// Compute whether we should keep this element visible in the rendering tree while a Hide animation is playing.
// The basic idea is best explained with a picture.  Consider this example subtree:
//
// K
// |
// E1
// |
// E2
//
// We compute the keep-visible state for K as follows:
// -If K, E1, or E2 has an implicit Hide animation or has RequestKeepAlive() called on it, K must be kept-visible.
bool CUIElement::ComputeKeepVisible()
{
    bool result = false;

    if (GetContext()->NWGetWindowRenderTarget() != nullptr && GetDCompTreeHost() != nullptr)
    {
        DCompTreeHost* dcompTreeHost = GetDCompTreeHost();
        result = dcompTreeHost->ShouldKeepVisible(this);
    }

    return result;
}

// Set "keep visible" status for this element.  See notes above for more details.
void CUIElement::SetKeepVisible(bool keepVisible)
{
    DCompTreeHost* dcompTreeHost = GetDCompTreeHost();

    // Start/Stop keep-visible tracking, done each frame in DCompTreeHost::UpdateImplicitShowHideAnimations().
    dcompTreeHost->SetTrackKeepVisible(this, keepVisible);

    if (!keepVisible)
    {
        FlushPendingKeepVisibleOperations();
    }
}

// Returns true if this element is being kept visible for a Hide animation or because of RequestKeepAlive
bool CUIElement::IsKeepVisible()
{
    DCompTreeHost* dcompTreeHost = GetDCompTreeHost();
    return dcompTreeHost->IsKeepVisible(this);
}

void CUIElement::FlushPendingKeepVisibleOperations()
{
    // We have a bit of a chicken-and-egg problem here. If this element is the unloading child of a popup, then it
    // could also be part of the popup root's unloading storage. The popup root's unloading storage needs to walk
    // the logical parent pointer to clean up, and it will reset the logical parent pointer afterwards. But, popup
    // also needs to walk the logical parent pointer to get up to the popup, and it also needs to reset the logical
    // parent pointer afterwards in case popup root's unloading storage didn't do it. So we have two places that
    // both need to walk the pointer and both need to clear it afterwards.
    //
    // So we do this in three steps. First get the logical parent and store it away, then let unloading storage (if
    // it's in there) clean up, and finally use the stored logical parent to let the popup clean up.

    CPopup* popupWithUnloadingChild = CPopup::GetPopupOfUnloadingChild(this);

    CUIElement* parent = do_pointer_cast<CUIElement>(GetParentInternal());
    if (parent)
    {
        CUIElementCollection* children = parent->GetChildren();
        if (children->HasUnloadingStorage())
        {
            bool bWasUnloading = false;
            IFCFAILFAST(parent->GetChildren()->RemoveUnloadedElement(this, UC_REFERENCE_ImplicitAnimation, &bWasUnloading));
        }
    }

    if (popupWithUnloadingChild != nullptr)
    {
        ASSERT(this == popupWithUnloadingChild->m_unloadingChild);
        popupWithUnloadingChild->RemoveUnloadingChild();
    }

    SetDirtyToRoot();
}

// Returns true if this element has the given type of implicit animation set.  Does not indicate if the animation is requested or running.
bool CUIElement::HasImplicitAnimation(ImplicitAnimationType iaType) const
{
    bool result;

    switch (iaType)
    {
    case ImplicitAnimationType::Show:
        result = HasImplicitShowAnimation();
        break;
    case ImplicitAnimationType::Hide:
        result = HasImplicitHideAnimation();
        break;
    }

    if (result)
    {
        ASSERT(!HasShownHiddenHandlers());
    }

    return result;
}

// Helper method, maps ImplicitAnimationType to CompositionRequirement
/*static*/ CompositionRequirement CUIElement::ImplicitAnimationTypeToCompositionRequirement(ImplicitAnimationType iaType)
{
    CompositionRequirement result;

    switch (iaType)
    {
    case ImplicitAnimationType::Show:
        result = CompositionRequirement::HasImplicitShowAnimation;
        break;
    case ImplicitAnimationType::Hide:
        result = CompositionRequirement::HasImplicitHideAnimation;
        break;
    }

    return result;
}

// Retrieve data about this element's implicit Show/Hide animation, if any.
// Returns a struct with either nullptrs (if no animation is present) or valid pointers.
// Note that having this information means there's an animation, but it's not necessarily playing.
ImplicitAnimationInfo CUIElement::GetImplicitAnimationInfo(ImplicitAnimationType iaType)
{
    ImplicitAnimationInfo info;

    DCompTreeHost* dcompTreeHost = GetDCompTreeHost();
    DCompTreeHost::ImplicitAnimationsMap& iaMap = dcompTreeHost->GetImplicitAnimationsMap(iaType);
    auto itFind = iaMap.find(this);
    if (itFind != iaMap.end())
    {
        info = itFind->second;
    }

    return info;
}

// Returns true if the given implicit animation is actually running right now.
bool CUIElement::IsImplicitAnimationPlaying(ImplicitAnimationType iaType)
{
    ImplicitAnimationInfo info = GetImplicitAnimationInfo(iaType);
    return (info.scopedBatch != nullptr);
}

// Requests the triggering of implicit Show/Hide animation
void CUIElement::SetImplicitAnimationRequested(ImplicitAnimationType iaType, bool requested)
{
    switch (iaType)
    {
    case ImplicitAnimationType::Show:
        m_requestedPlayImplicitShow = requested;
        break;
    case ImplicitAnimationType::Hide:
        m_requestedPlayImplicitHide = requested;
        break;
    }

    if (requested)
    {
        // Mark the element dirty and force a RenderWalk if we're triggering the animation.
        // Don't mark the element dirty as we're clearing the flag as it may over-invalidate,
        // particularly when playing the animation (see PlayImplicitAnimation()).
        CUIElement::NWSetContentDirty(this, DirtyFlags::Render);
    }
}

// Returns true if an implicit Show/Hide animation is requested.
bool CUIElement::IsImplicitAnimationRequested(ImplicitAnimationType iaType)
{
    bool result = false;

    switch (iaType)
    {
    case ImplicitAnimationType::Show:
        result = m_requestedPlayImplicitShow;
        break;
    case ImplicitAnimationType::Hide:
        result = m_requestedPlayImplicitHide;
        break;
    }

    return result;
}

// Helper function to actually start playing any requested Show/Hide animations
void CUIElement::TriggerImplicitShowHideAnimations()
{
    if (IsImplicitAnimationRequested(ImplicitAnimationType::Show))
    {
        ImplicitAnimationInfo implicitShowAnimationInfo = GetImplicitAnimationInfo(ImplicitAnimationType::Show);
        PlayImplicitAnimation(implicitShowAnimationInfo, ImplicitAnimationType::Show);
    }
    else if(IsImplicitAnimationRequested(ImplicitAnimationType::Hide))
    {
        ImplicitAnimationInfo implicitHideAnimationInfo = GetImplicitAnimationInfo(ImplicitAnimationType::Hide);
        PlayImplicitAnimation(implicitHideAnimationInfo, ImplicitAnimationType::Hide);
    }
}

// Start playing an implicit Show/Hide animation, given the supplied info about the animation
void CUIElement::PlayImplicitAnimation(ImplicitAnimationInfo& info, ImplicitAnimationType iaType)
{
    ASSERT(info.implicitAnimation != nullptr);
    ASSERT(IsImplicitAnimationRequested(iaType));
    ASSERT(m_pCompositionPeer != nullptr);

    // Don't play if the animation is already playing, eg if the element is first Collapsed, then removed
    if (!IsImplicitAnimationPlaying(iaType))
    {
        auto core = GetContext();

        // Create a ScopedBatch which will let us know when the animation is complete.
        wrl::ComPtr<WUComp::ICompositionScopedBatch> scopedBatch;
        DCompTreeHost* dcompTreeHost = GetDCompTreeHost();
        IFCFAILFAST(dcompTreeHost->GetCompositor()->CreateScopedBatch(WUComp::CompositionBatchTypes_Animation, &scopedBatch));

        wrl::ComPtr<WUComp::IVisual> handOffVisual = m_pCompositionPeer->GetHandOffVisual();
        wrl::ComPtr<WUComp::ICompositionObject2> co;
        VERIFYHR(handOffVisual.As(&co));

        bool isAnimationEnabled = true;
        IGNOREHR(FxCallbacks::FrameworkCallbacks_IsAnimationEnabled(&isAnimationEnabled));

        // Play the animation!
        if (isAnimationEnabled)
        {
            HRESULT hr = co->StartAnimationGroup(info.implicitAnimation.Get());
            if (hr == RO_E_CLOSED)
            {
                // The app disposed the animation out from under us.  Don't fail-fast in this situation.
                hr = S_OK;
            }
            IFCFAILFAST(hr);
        }
        core->IncrementPendingImplicitShowHideCount();

        scopedBatch->End();

        // Listen for animation completion
        auto callback = wrl::Callback <
            wrl::Implements <
            wrl::RuntimeClassFlags<wrl::ClassicCom>,
            wf::ITypedEventHandler<IInspectable*, WUComp::CompositionBatchCompletedEventArgs*>,
            wrl::FtmBase >>
            (this, iaType == ImplicitAnimationType::Show ? &CUIElement::OnImplicitShowAnimationCompleted : &CUIElement::OnImplicitHideAnimationCompleted);

        EventRegistrationToken token;
        IFCFAILFAST(scopedBatch->add_Completed(callback.Get(), &token));

        DCompTreeHost::ImplicitAnimationsMap& iaMap = dcompTreeHost->GetImplicitAnimationsMap(iaType);
        auto itFind = iaMap.find(this);
        ASSERT(itFind != iaMap.end());
        itFind->second.scopedBatch = scopedBatch;
        itFind->second.animationTarget = co;
        itFind->second.token = token;
    }

    SetImplicitAnimationRequested(iaType, false);
}

// Cancel a requested or in-progress implicit Show/Hide animation, else do nothing
void CUIElement::CancelImplicitAnimation(ImplicitAnimationType iaType)
{
    if (HasImplicitAnimation(iaType))
    {
        SetImplicitAnimationRequested(iaType, false);

        if (IsImplicitAnimationPlaying(iaType))
        {
            // Note: There's no guarantee that this element still has a composition peer. An ancestor could have been culled
            // (e.g. by setting Opacity=0) after the implicit animation started.

            ImplicitAnimationInfo info = GetImplicitAnimationInfo(iaType);

            HRESULT hr = info.animationTarget->StopAnimationGroup(info.implicitAnimation.Get());
            if (hr == RO_E_CLOSED)
            {
                // The app disposed the animation out from under us.  Don't fail-fast in this situation.
                hr = S_OK;
            }
            IFCFAILFAST(hr);
            CleanupImplicitAnimationOnCompletion(iaType);
        }
    }
}

// Event handler for completion of implicit Hide animation
_Check_return_ HRESULT CUIElement::OnImplicitHideAnimationCompleted(
    _In_ IInspectable* sender,
    _In_ WUComp::ICompositionBatchCompletedEventArgs* args)
{
    CleanupImplicitAnimationOnCompletion(ImplicitAnimationType::Hide);
    SetDirtyToRoot();
    return S_OK;
}

// Event handler for completion of implicit Show animation
_Check_return_ HRESULT CUIElement::OnImplicitShowAnimationCompleted(
    _In_ IInspectable* sender,
    _In_ WUComp::ICompositionBatchCompletedEventArgs* args)
{
    CleanupImplicitAnimationOnCompletion(ImplicitAnimationType::Show);
    SetDirtyToRoot();
    return S_OK;
}

// Cleanup of implicit Show/Hide animation, run either after cancel, or after animation completion
void CUIElement::CleanupImplicitAnimationOnCompletion(ImplicitAnimationType iaType)
{
    // Release the book-keeping entry for this animation
    DCompTreeHost* dcompTreeHost = GetDCompTreeHost();
    DCompTreeHost::ImplicitAnimationsMap& iaMap = dcompTreeHost->GetImplicitAnimationsMap(iaType);
    auto itFind = iaMap.find(this);
    if (itFind != iaMap.end())
    {
        IFCFAILFAST(itFind->second.scopedBatch->remove_Completed(itFind->second.token));
        itFind->second.scopedBatch = nullptr;

        itFind->second.animationTarget = nullptr;

        if (itFind->second.pendingChange)
        {
            // The animation was changed by the app while an animation was playing.  Now that we're done playing
            // the animation, we can process the pending change by updating the info we have on this animation.
            wrl::ComPtr<WUComp::ICompositionAnimationBase> pendingAnimation = itFind->second.pendingAnimation.Get();
            UpdateImplicitShowHideAnimationInfo(iaType, pendingAnimation.Get());
        }
    }

    GetContext()->DecrementPendingImplicitShowHideCount();
}

// Remove the information we have stored about this implicit show/hide animation.
void CUIElement::CleanupImplicitAnimationInfo(ImplicitAnimationType iaType)
{
    DCompTreeHost* dcompTreeHost = GetDCompTreeHost();
    DCompTreeHost::ImplicitAnimationsMap& iaMap = dcompTreeHost->GetImplicitAnimationsMap(iaType);
    auto itFind = iaMap.find(this);
    if (itFind != iaMap.end())
    {
        iaMap.erase(itFind);
    }
}

void CUIElement::SetDirtyToRoot()
{
    CUIElement::NWSetContentDirty(this, DirtyFlags::Render | DirtyFlags::ForcePropagate);
}

bool CUIElement::IsVisible() const
{
    return (m_fVisibility == static_cast<unsigned int>(DirectUI::Visibility::Visible));
}

bool CUIElement::IsCollapsed() const
{
    return m_fVisibility == static_cast<unsigned int>(DirectUI::Visibility::Collapsed);
}

DirectUI::Visibility CUIElement::GetVisibility() const
{
    return static_cast<DirectUI::Visibility>(m_fVisibility);
}

void CUIElement::SetVisibility(DirectUI::Visibility value)
{
    unsigned int newValue = value == DirectUI::Visibility::Collapsed;

    if (m_fVisibility != newValue)
    {
        m_fVisibility = newValue;

        if (IsActive()
            // Parentless popups aren't active. Check them explicitly. We need them to be open though, otherwise they weren't visible to begin with.
            || (GetParentInternal() == nullptr && OfTypeByIndex<KnownTypeIndex::Popup>() && static_cast<CPopup*>(this)->IsOpen()))
        {
            if (!IsVisible())
            {
                // If this element or one of its children has an implicit Hide animation,
                // keep it visible for rendering while the animation(s) play.
                if (ComputeKeepVisible())
                {
                    SetKeepVisible(true);
                }
            }
        }
    }
}

xref_ptr<CTransform> CUIElement::GetHandOffVisualTransform() const
{
    return GetHandOffVisualTransformGroup();
}

// Gets the 2D TransformGroup used to store the hand off visual's transform
xref_ptr<CTransformGroup> CUIElement::GetHandOffVisualTransformGroup() const
{
    if (IsEffectiveValueInSparseStorage(KnownPropertyIndex::UIElement_HandOffVisualTransform))
    {
        CValue result;
        VERIFYHR(GetValueByIndex(KnownPropertyIndex::UIElement_HandOffVisualTransform, &result));
        return static_sp_cast<CTransformGroup>(result.DetachObject());
    }
    return xref_ptr<CTransformGroup>(nullptr);
}

// Get specified sub transform from HandOff visual's transform collection
xref_ptr<CTransform> CUIElement::GetHandOffVisualSubTransform(_In_ KnownTypeIndex subTransformType) const
{
    xref_ptr<CTransformCollection> spTransformCollection;
    xref_ptr<CTransform> spSubTransform;

    spTransformCollection = GetHandOffVisualTransformCollection();
    if (spTransformCollection)
    {
        for (int i = 0, count = spTransformCollection->GetCount(); i < count; i++)
        {
            xref_ptr<CDependencyObject> spSubTransformCurrent;

            spSubTransformCurrent.attach(spTransformCollection->GetItemDOWithAddRef(i));
            if (spSubTransformCurrent && spSubTransformCurrent->OfTypeByIndex(subTransformType))
            {
                spSubTransform = static_cast<CTransform*>(spSubTransformCurrent.get());
                break;
            }
        }
    }

    return spSubTransform;
}

// Ensure that the specified sub-transform for the HandOff visual has been created
_Check_return_ HRESULT CUIElement::EnsureHandOffVisualSubTransform(_In_ KnownTypeIndex subTransformType)
{
    xref_ptr<CTransform> spHandOffVisualTranslateTransform;

    IFC_RETURN(EnsureHandOffVisualTransformGroupAndCollection());

    spHandOffVisualTranslateTransform = GetHandOffVisualSubTransform(subTransformType);
    if (!spHandOffVisualTranslateTransform)
    {
        IFC_RETURN(CreateAndInsertHandOffVisualSubTransform(subTransformType));
    }

    return S_OK;
}

void CUIElement::ClearHandOffVisualSubTransform(_In_ KnownTypeIndex subTransformType)
{
    xref_ptr<CTransformCollection> spTransformCollection;

    spTransformCollection = GetHandOffVisualTransformCollection();
    if (spTransformCollection)
    {
        for (int i = 0, count = spTransformCollection->GetCount(); i < count; i++)
        {
            xref_ptr<CDependencyObject> spSubTransformCurrent;
            xref_ptr<CDependencyObject> spSubTransformRemoved;

            spSubTransformCurrent.attach(spTransformCollection->GetItemDOWithAddRef(i));
            if (spSubTransformCurrent && spSubTransformCurrent->OfTypeByIndex(subTransformType))
            {
                spSubTransformRemoved.attach(static_cast<CDependencyObject*>(spTransformCollection->RemoveAt(i)));   // We need to manually release the ref
                break;
            }
        }
    }
}

// Ensure that a TransformGroup with a TransformCollection for the HandOff visual has been created
_Check_return_ HRESULT CUIElement::EnsureHandOffVisualTransformGroupAndCollection()
{
    if (!GetHandOffVisualTransformGroup())
    {
        CREATEPARAMETERS cp(GetContext());
        xref_ptr<CTransformGroup> spTransformGroup;
        xref_ptr<CTransformCollection> spTransformCollection;
        CValue valueTransformGroup;
        CValue valueTransformCollection;

        IFC_RETURN(CTransformGroup::Create(reinterpret_cast<CDependencyObject **>(&spTransformGroup), &cp));
        IFC_RETURN(CTransformCollection::Create(reinterpret_cast<CDependencyObject **>(&spTransformCollection), &cp));
        valueTransformCollection.SetObjectAddRef(spTransformCollection.get());

        IFC_RETURN(spTransformGroup->SetValueByIndex(KnownPropertyIndex::TransformGroup_Children, valueTransformCollection));

        valueTransformGroup.SetObjectAddRef(spTransformGroup.get());
        IFC_RETURN(SetValueByIndex(KnownPropertyIndex::UIElement_HandOffVisualTransform, valueTransformGroup));
        SetIsListeningForHandOffVisualPropertyChanges(true);
    }

    return S_OK;
}

// Get collection of sub-transforms for HandOff visual
xref_ptr<CTransformCollection> CUIElement::GetHandOffVisualTransformCollection() const
{
    xref_ptr<CTransformCollection> spHandOffVisualTransformCollection;
    xref_ptr<CTransformGroup> spHandOffVisualTransformGroup;

    spHandOffVisualTransformGroup = GetHandOffVisualTransformGroup();
    if (spHandOffVisualTransformGroup)
    {
        spHandOffVisualTransformCollection = spHandOffVisualTransformGroup->m_pChild;
    }

    return spHandOffVisualTransformCollection;
}

// Create and insert sub transform of specified type into HandOff visual's transform collection.
// Order of sub transforms from leaf to root is Matrix, Rotate, Scale, Translate
_Check_return_ HRESULT CUIElement::CreateAndInsertHandOffVisualSubTransform(_In_ KnownTypeIndex subTransformType)
{
    xref_ptr<CTransformCollection> spTransformCollection;
    xref_ptr<CTransform> spSubTransform;
    unsigned int insertionIndex = 0;
    CREATEPARAMETERS cp(GetContext());

    spTransformCollection = GetHandOffVisualTransformCollection();
    IFCPTR_RETURN(spTransformCollection);

    // Get index to insert sub transform
    unsigned int subTransformOrder = GetHandOffVisualSubTransformOrder(subTransformType);
    insertionIndex = spTransformCollection->GetCount();
    for (unsigned int i = 0, count = spTransformCollection->GetCount(); i < count; i++)
    {
        xref_ptr<CDependencyObject> spSubTransformCurrent;
        spSubTransformCurrent.attach(spTransformCollection->GetItemDOWithAddRef(i));
        IFCPTR_RETURN(spSubTransformCurrent)

        if (subTransformOrder < GetHandOffVisualSubTransformOrder(spSubTransformCurrent->GetTypeIndex()))
        {
            insertionIndex = i;
            break;
        };
    }

    // Create sub transform
    switch (subTransformType)
    {
        case KnownTypeIndex::ScaleTransform:
           ASSERT(!GetHandOffVisualSubTransform(KnownTypeIndex::ScaleTransform));
           IFC_RETURN(CScaleTransform::Create(reinterpret_cast<CDependencyObject **>(&spSubTransform), &cp));
           break;

        case KnownTypeIndex::TranslateTransform:
          ASSERT(!GetHandOffVisualSubTransform(KnownTypeIndex::TranslateTransform));
          IFC_RETURN(CTranslateTransform::Create(reinterpret_cast<CDependencyObject **>(&spSubTransform), &cp));
          break;

        case KnownTypeIndex::RotateTransform:
           ASSERT(!GetHandOffVisualSubTransform(KnownTypeIndex::RotateTransform));
           IFC_RETURN(CRotateTransform::Create(reinterpret_cast<CDependencyObject **>(&spSubTransform), &cp));
           break;

        case KnownTypeIndex::MatrixTransform:
           ASSERT(!GetHandOffVisualSubTransform(KnownTypeIndex::MatrixTransform));
           IFC_RETURN(CMatrixTransform::Create(reinterpret_cast<CDependencyObject **>(&spSubTransform), &cp));
           break;

        default:
            // Unsupported
            ASSERT(0);
            IFC_RETURN(E_FAIL);
            break;
    }

    // Insert sub transform into collection
    IFC_RETURN(spTransformCollection->Insert(insertionIndex, spSubTransform));

    return S_OK;
}

// Get order of sub transform of HandOff Visual
unsigned int CUIElement::GetHandOffVisualSubTransformOrder(
    _In_ KnownTypeIndex subTransformType)
{
    static KnownTypeIndex c_transformOrder[] =
    {
        KnownTypeIndex::MatrixTransform,
        KnownTypeIndex::ScaleTransform,
        KnownTypeIndex::RotateTransform,
        KnownTypeIndex::TranslateTransform
    };

    for (unsigned int i = 0; i < ARRAY_SIZE(c_transformOrder); i++)
    {
       if (subTransformType == c_transformOrder[i])
       {
            return i;
       }
    }

    // Unknown type
    ASSERT(0);
    return 0;
}

/* static */ void CUIElement::NWSetHandOffVisualTransformDirty(_In_ CDependencyObject *pTarget, DirtyFlags flags)
{
    CUIElement *pUIE = static_cast<CUIElement*>(pTarget);
    pUIE->NWSetDirtyFlagsAndPropagate(flags, FALSE);
}

void CUIElement::SetHandOffVisualTransformDirty()
{
    xref_ptr<CTransformGroup> spHandOffVisualTransformGroup;

    // Mark TransformGroup as dirty because of transform change
    spHandOffVisualTransformGroup = GetHandOffVisualTransformGroup();
    if (spHandOffVisualTransformGroup)
    {
        CTransformGroup::NWSetTransformsDirty(spHandOffVisualTransformGroup, DirtyFlags::Render);
    }

    // Mark element bounds as dirty because of transform change. Treat this change as independent because we affect
    // only bounds, not rendering (the rendering flag was propagated via the TransformGroup above). This way, dirty
    // flag propagation won't add the render flag in CUIElement::NWSetSubgraphDirty while we're propagating it.
    CUIElement::NWSetHandOffVisualTransformDirty(this, DirtyFlags::Independent | DirtyFlags::Bounds);
}

// Handle change of HandOff visual's Offset by changing corresponding local transform for hit testing
_Check_return_ HRESULT CUIElement::SetHandOffVisualOffset(float x, float y)
{
    xref_ptr<CTransform> spTransform;
    CTranslateTransform* pTranslateTransform = nullptr;

    // Get or create translate transform
    IFC_RETURN(EnsureHandOffVisualSubTransform(KnownTypeIndex::TranslateTransform));
    spTransform = GetHandOffVisualSubTransform(KnownTypeIndex::TranslateTransform);
    IFCPTR_RETURN(spTransform);

    // Change translate transform using new offset
    pTranslateTransform = static_cast<CTranslateTransform*>(spTransform.get());
    pTranslateTransform->m_eX = x;
    pTranslateTransform->m_eY = y;

    // Mark TransformGroup and element bounds as dirty because of transform change
    SetHandOffVisualTransformDirty();

    return S_OK;
}

// Handle change of HandOff visual's Scale by changing corresponding local transform for hit testing
_Check_return_ HRESULT CUIElement::SetHandOffVisualScale(float scaleX, float scaleY)
{
    xref_ptr<CTransform> spTransform;
    CScaleTransform* pScaleTransform = nullptr;

    // Get or create scale transform
    IFC_RETURN(EnsureHandOffVisualSubTransform(KnownTypeIndex::ScaleTransform));
    spTransform = GetHandOffVisualSubTransform(KnownTypeIndex::ScaleTransform);
    IFCPTR_RETURN(spTransform);

    // Change scale transform using new offset
    pScaleTransform = static_cast<CScaleTransform*>(spTransform.get());
    pScaleTransform->m_eScaleX = scaleX;
    pScaleTransform->m_eScaleY = scaleY;

    // Mark TransformGroup and element bounds as dirty because of transform change
    SetHandOffVisualTransformDirty();

    return S_OK;
}

// Handle change of HandOff visual's RotationAngle by changing corresponding local transform for hit testing
_Check_return_ HRESULT CUIElement::SetHandOffVisualRotationAngle(float angle)
{
    xref_ptr<CTransform> spTransform;
    CRotateTransform* pRotateTransform = nullptr;

    // Get or create rotate transform
    IFC_RETURN(EnsureHandOffVisualSubTransform(KnownTypeIndex::RotateTransform));
    spTransform = GetHandOffVisualSubTransform(KnownTypeIndex::RotateTransform);
    IFCPTR_RETURN(spTransform);

    // Change rotate transform using new angle
    pRotateTransform = static_cast<CRotateTransform*>(spTransform.get());
    pRotateTransform->m_eAngle = angle;

    // Mark TransformGroup and element bounds as dirty because of transform change
    SetHandOffVisualTransformDirty();

    return S_OK;
}

// Handle change of HandOff visual's CenterPoint by changing corresponding local transform for hit testing
// CenterPoint affects scale and rotation. It doesn't affect the matrix transform.
_Check_return_ HRESULT CUIElement::SetHandOffVisualCenterPoint(float centerX, float centerY)
{
    xref_ptr<CTransform> spScaleTransform;
    xref_ptr<CTransform> spRotateTransform;
    CScaleTransform* pScaleTransform = nullptr;
    CRotateTransform* pRotateTransform = nullptr;

    // Get or create transforms, and change them

    IFC_RETURN(EnsureHandOffVisualSubTransform(KnownTypeIndex::ScaleTransform));
    spScaleTransform = GetHandOffVisualSubTransform(KnownTypeIndex::ScaleTransform);
    IFCPTR_RETURN(spScaleTransform);

    pScaleTransform = static_cast<CScaleTransform*>(spScaleTransform.get());
    pScaleTransform->m_ptCenter.x = centerX;
    pScaleTransform->m_ptCenter.y = centerY;

    IFC_RETURN(EnsureHandOffVisualSubTransform(KnownTypeIndex::RotateTransform));
    spRotateTransform = GetHandOffVisualSubTransform(KnownTypeIndex::RotateTransform);
    IFCPTR_RETURN(spRotateTransform);

    pRotateTransform = static_cast<CRotateTransform*>(spRotateTransform.get());
    pRotateTransform->m_ptCenter.x = centerX;
    pRotateTransform->m_ptCenter.y = centerY;

    // Mark TransformGroup and element bounds as dirty because of transform change
    SetHandOffVisualTransformDirty();

    return S_OK;
}

// Handle change of HandOff visual's TransformMatrix by changing corresponding local transform for hit testing
void CUIElement::SetHandOffVisualTransformMatrix(_In_ const wfn::Matrix4x4& matrix4x4)
{
    if (IsEffectiveValueInSparseStorage(KnownPropertyIndex::UIElement_Projection))
    {
        // TODO: HitTest: We can handle this now, probably
        // RS3 Bug #12868519
        // When a Projection is in use, we cannot currently handle putting this matrix into our 3D stash.
        // Ignore the entire TransformMatrix in this scenario, and fall-back to using the Projection property.
        SetHandOffVisualTransformDirty();
        ClearHandOffVisualTransformMatrix3D();
    }
    else
    {
        CMILMatrix4x4 xamlMatrix4x4 = *reinterpret_cast<const CMILMatrix4x4*>(&matrix4x4);
        if (xamlMatrix4x4.Is2D())
        {
            // Get or create matrix transform
            xref_ptr<CTransform> spTransform;
            IFCFAILFAST(EnsureHandOffVisualSubTransform(KnownTypeIndex::MatrixTransform));
            spTransform = GetHandOffVisualSubTransform(KnownTypeIndex::MatrixTransform);

            // Change matrix
            CMILMatrix matrix3x3 = xamlMatrix4x4.Get2DRepresentation();
            CMatrixTransform* pMatrixTransform = static_cast<CMatrixTransform*>(spTransform.get());
            pMatrixTransform->SetMatrix(matrix3x3);

            // Mark TransformGroup and element bounds as dirty because of transform change
            SetHandOffVisualTransformDirty();

            // Clear the 3D matrix - fall back to 2D mode
            ClearHandOffVisualTransformMatrix3D();
        }
        else
        {
            EnsureHandOffVisualTransformMatrix3D(xamlMatrix4x4);

            // Mark TransformGroup and element bounds as dirty because of transform change
            SetHandOffVisualTransformDirty();

            // Clear the 2D matrix - switch to 3D mode
            ClearHandOffVisualSubTransform(KnownTypeIndex::MatrixTransform);
        }
    }
}

// Helper function to determine if we can use the HandOff visual's transform components in GetLocalTransform instead of XAML properties
bool CUIElement::CanUseHandOffVisualTransformGroupForLocalTransform() const
{
    return
        // Bug #13329990:  The RootVisual is excluded to workaround hit-test stash not being updated in certain situations.
        // The RootVisual's HandOff visual cannot be accessed anyway so we don't actually need its hit-test stash for correct hit-testing.
        !OfTypeByIndex<KnownTypeIndex::RootVisual>() &&

        IsListeningForHandOffVisualPropertyChanges() && // We must be listening for property changes
        IsEffectiveValueInSparseStorage(KnownPropertyIndex::UIElement_HandOffVisualTransform) &&  // There must be a 2D transform update from WUC
        GetTranslation(true).Z == 0;    // There must not be 3D in the Translation facade
}

void CUIElement::StopAllFacadeAnimations()
{
    FacadeAnimationHelper::CancelAnimation(this, KnownPropertyIndex::UIElement_Translation);
    FacadeAnimationHelper::CancelAnimation(this, KnownPropertyIndex::UIElement_Rotation);
    FacadeAnimationHelper::CancelAnimation(this, KnownPropertyIndex::UIElement_Scale);
    FacadeAnimationHelper::CancelAnimation(this, KnownPropertyIndex::UIElement_TransformMatrix);
    FacadeAnimationHelper::CancelAnimation(this, KnownPropertyIndex::UIElement_CenterPoint);
    FacadeAnimationHelper::CancelAnimation(this, KnownPropertyIndex::UIElement_RotationAxis);
    FacadeAnimationHelper::CancelAnimation(this, KnownPropertyIndex::UIElement_Opacity);
}

wfn::Vector3 CUIElement::GetTranslation(bool preferAnimatingValue) const
{
    if (preferAnimatingValue && GetContext()->GetFacadeStorage().HasAnimation(this, KnownPropertyIndex::UIElement_Translation))
    {
        return SimpleProperty::Property::id<KnownPropertyIndex::UIElement_AnimatedTranslation>::Get(this);
    }
    return SimpleProperty::Property::id<KnownPropertyIndex::UIElement_Translation>::Get(this);
}

void CUIElement::SetTranslation(const wfn::Vector3& translation)
{
    const bool hadExplicitAnimation = GetContext()->GetFacadeStorage().HasExplicitAnimation(this, KnownPropertyIndex::UIElement_Translation);
    const wfn::Vector3 oldTranslation = CUIElement::GetTranslation(false);

    // Note: Setting the same value is considered a no-op. Even if there's a transition in progress and the TranslationTransition
    // property is now null, we will not stop the ongoing transition. We will stop explicit animations, though, because they were
    // started after the last static property set, and have taken over from the static value.
    if (hadExplicitAnimation || translation != oldTranslation)
    {
        WUComp::ICompositionAnimationBase* implicitAnimation = nullptr;

        bool isAnimationEnabled = true;
        IGNOREHR(FxCallbacks::FrameworkCallbacks_IsAnimationEnabled(&isAnimationEnabled));
        DCompTreeHost* dcompTreeHost = GetDCompTreeHost();
        CVector3Transition* transition = GetVector3TransitionNoRef(*this, KnownPropertyIndex::UIElement_TranslationTransition);
        if (dcompTreeHost != nullptr && isAnimationEnabled && transition != nullptr)
        {
            implicitAnimation = transition->GetWUCAnimationNoRef(dcompTreeHost, wrl::Wrappers::HStringReference(FacadeProperty_Translation), translation);
        }

        if (implicitAnimation != nullptr)
        {
            // Note: Timing is very important here. We're making use of some different behaviors in order to get the outcome
            // we want. We need to start the implicit animation before stopping any existing animations and before setting the
            // static value. See comment in StartAnimation.
            IFCFAILFAST(StartAnimationImpl(implicitAnimation, true));
        }
        else
        {
            // No transition - stop any existing animations. If there was a transition, it will take over from the currently
            // running animation.
            FacadeAnimationHelper::CancelAnimation(this, KnownPropertyIndex::UIElement_Translation);
        }

        // Set the static value now that we've started any animations that depended on the old static value
        // Note: This will also call SetTranslationImpl via a simple property callback
        SimpleProperty::Property::id<KnownPropertyIndex::UIElement_Translation>::Set(this, translation);
    }
}

void CUIElement::SetTranslationImpl(const wfn::Vector3& oldTranslation)
{
    const bool hadExplicitAnimation = GetContext()->GetFacadeStorage().HasExplicitAnimation(this, KnownPropertyIndex::UIElement_Translation);
    const wfn::Vector3 translation = CUIElement::GetTranslation(false);

    if (hadExplicitAnimation || translation != oldTranslation)
    {
        // TODO: This will stomp over the implicit animation. Put it in an "if no animation" branch.
        FacadeAnimationHelper::PopulateBackingCompositionObjectWithFacadeIfReferenced(this, KnownPropertyIndex::UIElement_Translation);
        UpdateHasTranslateZ(translation);
        CUIElement::NWSetTransformDirty(this, DirtyFlags::Render | DirtyFlags::Bounds);
        m_areFacadesInUse = true;

        const bool newValueHas3DDepth = translation.Z != 0;
        if (!m_has3DDepth && newValueHas3DDepth)
        {
            SetHas3DDepth(true);
        }
        else if (m_has3DDepth && !newValueHas3DDepth)
        {
            // If the new value doesn't have 3D depth, we still have to check all the other properties.
            UpdateHas3DDepth();
        }
    }
}

void CUIElement::UpdateHasTranslateZ(const wfn::Vector3& translation)
{
    if (translation.Z != 0)
    {
        // If the Z component of Translation is non-zero, we need to create a CompNode as this element is essentially now in 3D space
        if (!HasTranslateZ())
        {
            SetRequiresComposition(CompositionRequirement::HasTranslateZ, IndependentAnimationType::None);
        }
    }
    else
    {
        if (HasTranslateZ())
        {
            UnsetRequiresComposition(CompositionRequirement::HasTranslateZ, IndependentAnimationType::None);
        }
    }
}

FLOAT CUIElement::GetRotation(bool preferAnimatingValue) const
{
    if (preferAnimatingValue && GetContext()->GetFacadeStorage().HasAnimation(this, KnownPropertyIndex::UIElement_Rotation))
    {
        return static_cast<FLOAT>(SimpleProperty::Property::id<KnownPropertyIndex::UIElement_AnimatedRotation>::Get(this));
    }
    return static_cast<FLOAT>(SimpleProperty::Property::id<KnownPropertyIndex::UIElement_Rotation>::Get(this));
}

void CUIElement::SetRotation(FLOAT rotation)
{
    const bool hadExplicitAnimation = GetContext()->GetFacadeStorage().HasExplicitAnimation(this, KnownPropertyIndex::UIElement_Rotation);
    const float oldRotation = CUIElement::GetRotation(false);

    // Note: Setting the same value is considered a no-op. Even if there's a transition in progress and the RotationTransition
    // property is now null, we will not stop the ongoing transition. We will stop explicit animations, though, because they were
    // started after the last static property set, and have taken over from the static value.
    if (hadExplicitAnimation || rotation != oldRotation)
    {
        bool isAnimationEnabled = true;
        IGNOREHR(FxCallbacks::FrameworkCallbacks_IsAnimationEnabled(&isAnimationEnabled));
        DCompTreeHost* dcompTreeHost = GetDCompTreeHost();
        CScalarTransition* transition = GetScalarTransitionNoRef(*this, KnownPropertyIndex::UIElement_RotationTransition);
        if (dcompTreeHost != nullptr && isAnimationEnabled && transition != nullptr)
        {
            WUComp::ICompositionAnimationBase* implicitAnimation = transition->GetWUCAnimationNoRef(dcompTreeHost, wrl::Wrappers::HStringReference(FacadeProperty_Rotation), rotation);

            // Note: Timing is very important here. We're making use of some different behaviors in order to get the outcome
            // we want. We need to start the implicit animation before stopping any existing animations and before setting the
            // static value. See comment in StartAnimation.
            IFCFAILFAST(StartAnimationImpl(implicitAnimation, true));
        }
        else
        {
            // No transition - stop any existing animations. If there was a transition, it will take over from the currently
            // running animation.
            FacadeAnimationHelper::CancelAnimation(this, KnownPropertyIndex::UIElement_Rotation);
        }

        // Set the static value now that we've started any animations that depended on the old static value
        // Note: This will also call SetRotationImpl via a simple property callback
        SimpleProperty::Property::id<KnownPropertyIndex::UIElement_Rotation>::Set(this, rotation);
    }
}

void CUIElement::SetRotationImpl(DOUBLE oldRotation)
{
    const bool hadExplicitAnimation = GetContext()->GetFacadeStorage().HasExplicitAnimation(this, KnownPropertyIndex::UIElement_Rotation);
    const float rotation = CUIElement::GetRotation(false);

    if (hadExplicitAnimation || rotation != static_cast<float>(oldRotation))
    {
        FacadeAnimationHelper::PopulateBackingCompositionObjectWithFacadeIfReferenced(this, KnownPropertyIndex::UIElement_Rotation);
        UpdateHasNonZeroRotation(rotation);
        CUIElement::NWSetTransformDirty(this, DirtyFlags::Render | DirtyFlags::Bounds);
        m_areFacadesInUse = true;
    }
}

void CUIElement::UpdateHasNonZeroRotation(float rotation)
{
    if (rotation != 0)
    {
        // If the rotation non-zero, we need to create a CompNode as this element no longer axis-aligned
        if (!HasNonZeroRotation())
        {
            SetRequiresComposition(CompositionRequirement::HasNonZeroRotation, IndependentAnimationType::None);
        }
    }
    else
    {
        if (HasNonZeroRotation())
        {
            UnsetRequiresComposition(CompositionRequirement::HasNonZeroRotation, IndependentAnimationType::None);
        }
    }
}

wfn::Vector3 CUIElement::GetScale(bool preferAnimatingValue) const
{
    if (preferAnimatingValue && GetContext()->GetFacadeStorage().HasAnimation(this, KnownPropertyIndex::UIElement_Scale))
    {
        return SimpleProperty::Property::id<KnownPropertyIndex::UIElement_AnimatedScale>::Get(this);
    }
    return SimpleProperty::Property::id<KnownPropertyIndex::UIElement_Scale>::Get(this);
}

void CUIElement::SetScale(const wfn::Vector3& scale)
{
    const bool hadExplicitAnimation = GetContext()->GetFacadeStorage().HasExplicitAnimation(this, KnownPropertyIndex::UIElement_Scale);
    const wfn::Vector3 oldScale = CUIElement::GetScale(false);

    // Note: Setting the same value is considered a no-op. Even if there's a transition in progress and the ScaleTransition
    // property is now null, we will not stop the ongoing transition. We will stop explicit animations, though, because they were
    // started after the last static property set, and have taken over from the static value.
    if (hadExplicitAnimation || scale != oldScale)
    {
        WUComp::ICompositionAnimationBase* implicitAnimation = nullptr;

        bool isAnimationEnabled = true;
        IGNOREHR(FxCallbacks::FrameworkCallbacks_IsAnimationEnabled(&isAnimationEnabled));
        DCompTreeHost* dcompTreeHost = GetDCompTreeHost();
        CVector3Transition* transition = GetVector3TransitionNoRef(*this, KnownPropertyIndex::UIElement_ScaleTransition);
        if (dcompTreeHost != nullptr && isAnimationEnabled && transition != nullptr)
        {
            implicitAnimation = transition->GetWUCAnimationNoRef(dcompTreeHost, wrl::Wrappers::HStringReference(FacadeProperty_Scale), scale);
        }

        if (implicitAnimation != nullptr)
        {
            // Note: Timing is very important here. We're making use of some different behaviors in order to get the outcome
            // we want. We need to start the implicit animation before stopping any existing animations and before setting the
            // static value. See comment in StartAnimation.
            IFCFAILFAST(StartAnimationImpl(implicitAnimation, true));
        }
        else
        {
            // No transition - stop any existing animations. If there was a transition, it will take over from the currently
            // running animation.
            FacadeAnimationHelper::CancelAnimation(this, KnownPropertyIndex::UIElement_Scale);
        }

        // Set the static value now that we've started any animations that depended on the old static value
        // Note: This will also call SetScaleImpl via a simple property callback
        SimpleProperty::Property::id<KnownPropertyIndex::UIElement_Scale>::Set(this, scale);
    }
}

void CUIElement::SetScaleImpl(const wfn::Vector3& oldScale)
{
    const bool hadExplicitAnimation = GetContext()->GetFacadeStorage().HasExplicitAnimation(this, KnownPropertyIndex::UIElement_Scale);
    const wfn::Vector3 scale = CUIElement::GetScale(false);

    if (hadExplicitAnimation || scale != oldScale)
    {
        FacadeAnimationHelper::PopulateBackingCompositionObjectWithFacadeIfReferenced(this, KnownPropertyIndex::UIElement_Scale);
        UpdateHasScaleZ(scale);
        CUIElement::NWSetTransformDirty(this, DirtyFlags::Render | DirtyFlags::Bounds);
        m_areFacadesInUse = true;

        const bool newValueHas3DDepth = scale.Z != 1;
        if (!m_has3DDepth && newValueHas3DDepth)
        {
            SetHas3DDepth(true);
        }
        else if (m_has3DDepth && !newValueHas3DDepth)
        {
            // If the new value doesn't have 3D depth, we still have to check all the other properties.
            UpdateHas3DDepth();
        }
    }
}

void CUIElement::UpdateHasScaleZ(const wfn::Vector3& scale)
{
    if (scale.Z != 1)
    {
        // If the Z component of Scale is something other than 1, we need to create a CompNode as this element is essentially now in 3D space.
        // Additional notes:
        // Technically Z == 0 is allowed but we don't bother checking for this as it would be rare and not very useful.
        // The presence of Z != 1 only has a visible effect if there is z positioning and perspective present but this would be complex to
        // keep updated so we keep it simple and always back the element with a CompNode in this case.
        if (!HasScaleZ())
        {
            SetRequiresComposition(CompositionRequirement::HasScaleZ, IndependentAnimationType::None);
        }
    }
    else
    {
        if (HasScaleZ())
        {
            UnsetRequiresComposition(CompositionRequirement::HasScaleZ, IndependentAnimationType::None);
        }
    }
}

wfn::Matrix4x4 CUIElement::GetTransformMatrix(bool preferAnimatingValue) const
{
    if (preferAnimatingValue && GetContext()->GetFacadeStorage().HasAnimation(this, KnownPropertyIndex::UIElement_TransformMatrix))
    {
        return SimpleProperty::Property::id<KnownPropertyIndex::UIElement_AnimatedTransformMatrix>::Get(this);
    }
    return SimpleProperty::Property::id<KnownPropertyIndex::UIElement_TransformMatrix>::Get(this);
}

void CUIElement::SetTransformMatrix(const wfn::Matrix4x4& transformMatrix)
{
    SimpleProperty::Property::id<KnownPropertyIndex::UIElement_TransformMatrix>::Set(this, transformMatrix);
}

void CUIElement::SetTransformMatrixImpl(const wfn::Matrix4x4& oldTransformMatrix)
{
    const wfn::Matrix4x4& transformMatrix = GetTransformMatrix(false);
    FacadeAnimationHelper::CancelAnimation(this, KnownPropertyIndex::UIElement_TransformMatrix);
    FacadeAnimationHelper::PopulateBackingCompositionObjectWithFacadeIfReferenced(this, KnownPropertyIndex::UIElement_TransformMatrix);
    UpdateHasNonIdentityTransformMatrix(transformMatrix);
    CUIElement::NWSetTransformDirty(this, DirtyFlags::Render | DirtyFlags::Bounds);
    m_areFacadesInUse = true;

    const bool newValueHas3DDepth = !Matrix4x4Is2D(transformMatrix);
    if (!m_has3DDepth && newValueHas3DDepth)
    {
        SetHas3DDepth(true);
    }
    else if (m_has3DDepth && !newValueHas3DDepth)
    {
        // If the new value doesn't have 3D depth, we still have to check all the other properties.
        UpdateHas3DDepth();
    }
}

// Called just after the TransformMatrix has changed (either static or animating),
// this function responds to the TransformMatrix change by inspecting it for 3D and
// updating internal state related to hit-testing
void CUIElement::OnTransformMatrixForHitTestingChanged(const wfn::Matrix4x4& transformMatrix)
{
    bool has3D = !Matrix4x4Is2D(transformMatrix);

    SetHas3DDepth(has3D);

    // Mark element bounds as dirty because of transform change
    NWSetDirtyFlagsAndPropagate(DirtyFlags::Bounds, FALSE);
}

void CUIElement::UpdateHasNonIdentityTransformMatrix(const wfn::Matrix4x4& transformMatrix)
{
    if (memcmp(&transformMatrix, &SimpleProperty::Property::Default<KnownPropertyIndex::UIElement_TransformMatrix>(), sizeof(wfn::Matrix4x4)) != 0)
    {
        // If the transform matrix is not identity, we assume it has some component that requires a CompNode (eg depth)
        if (!HasNonIdentityTransformMatrix())
        {
            SetRequiresComposition(CompositionRequirement::HasNonIdentityTransformMatrix, IndependentAnimationType::None);
        }
    }
    else
    {
        if (HasNonIdentityTransformMatrix())
        {
            UnsetRequiresComposition(CompositionRequirement::HasNonIdentityTransformMatrix, IndependentAnimationType::None);
        }
    }
}

wfn::Vector3 CUIElement::GetCenterPoint(bool preferAnimatingValue) const
{
    if (preferAnimatingValue && GetContext()->GetFacadeStorage().HasAnimation(this, KnownPropertyIndex::UIElement_CenterPoint))
    {
        return SimpleProperty::Property::id<KnownPropertyIndex::UIElement_AnimatedCenterPoint>::Get(this);
    }
    return SimpleProperty::Property::id<KnownPropertyIndex::UIElement_CenterPoint>::Get(this);
}

void CUIElement::SetCenterPoint(const wfn::Vector3& centerPoint)
{
    SimpleProperty::Property::id<KnownPropertyIndex::UIElement_CenterPoint>::Set(this, centerPoint);
}

void CUIElement::SetCenterPointImpl(const wfn::Vector3& oldCenterPoint)
{
    const wfn::Vector3 centerPoint = GetCenterPoint(false);
    FacadeAnimationHelper::CancelAnimation(this, KnownPropertyIndex::UIElement_CenterPoint);
    FacadeAnimationHelper::PopulateBackingCompositionObjectWithFacadeIfReferenced(this, KnownPropertyIndex::UIElement_CenterPoint);
    UpdateHasNonZeroCenterPoint(centerPoint);
    CUIElement::NWSetTransformDirty(this, DirtyFlags::Render | DirtyFlags::Bounds);
    m_areFacadesInUse = true;
}

void CUIElement::UpdateHasNonZeroCenterPoint(const wfn::Vector3& centerPoint)
{
    if (memcmp(&centerPoint, &SimpleProperty::Property::Default<KnownPropertyIndex::UIElement_CenterPoint>(), sizeof(wfn::Vector3)) != 0)
    {
        // If the CenterPoint is not (0,0,0) we assume it interplays with other properties in a way that requires a CompNode
        if (!HasNonZeroCenterPoint())
        {
            SetRequiresComposition(CompositionRequirement::HasNonZeroCenterPoint, IndependentAnimationType::None);
        }
    }
    else
    {
        if (HasNonZeroCenterPoint())
        {
            UnsetRequiresComposition(CompositionRequirement::HasNonZeroCenterPoint, IndependentAnimationType::None);
        }
    }
}

wfn::Vector3 CUIElement::GetRotationAxis(bool preferAnimatingValue) const
{
    if (preferAnimatingValue && GetContext()->GetFacadeStorage().HasAnimation(this, KnownPropertyIndex::UIElement_RotationAxis))
    {
        return SimpleProperty::Property::id<KnownPropertyIndex::UIElement_AnimatedRotationAxis>::Get(this);
    }
    return SimpleProperty::Property::id<KnownPropertyIndex::UIElement_RotationAxis>::Get(this);
}

void CUIElement::SetRotationAxis(const wfn::Vector3& rotationAxis)
{
    SimpleProperty::Property::id<KnownPropertyIndex::UIElement_RotationAxis>::Set(this, rotationAxis);
}

void CUIElement::SetRotationAxisImpl(const wfn::Vector3& oldRotationAxis)
{
    const wfn::Vector3 rotationAxis = GetRotationAxis(false);
    FacadeAnimationHelper::CancelAnimation(this, KnownPropertyIndex::UIElement_RotationAxis);
    FacadeAnimationHelper::PopulateBackingCompositionObjectWithFacadeIfReferenced(this, KnownPropertyIndex::UIElement_RotationAxis);
    UpdateHasNonDefaultRotationAxis(rotationAxis);
    CUIElement::NWSetTransformDirty(this, DirtyFlags::Render | DirtyFlags::Bounds);
    m_areFacadesInUse = true;

    const bool newValueHas3DDepth = rotationAxis.X != 0 || rotationAxis.Y != 0;
    if (!m_has3DDepth && newValueHas3DDepth)
    {
        SetHas3DDepth(true);
    }
    else if (m_has3DDepth && !newValueHas3DDepth)
    {
        // If the new value doesn't have 3D depth, we still have to check all the other properties.
        UpdateHas3DDepth();
    }
}

double CUIElement::GetRasterizationScale() const
{
    return SimpleProperty::Property::id<KnownPropertyIndex::UIElement_RasterizationScale>::Get(this);
}

void CUIElement::SetRasterizationScale(double rasterizationScale)
{
    ASSERT(rasterizationScale > 0);
    SimpleProperty::Property::id<KnownPropertyIndex::UIElement_RasterizationScale>::Set(this, rasterizationScale);
}

void CUIElement::SetRasterizationScaleImpl(double oldRasterizationScale)
{
    CUIElement::NWSetTransformDirty(this, DirtyFlags::Render);
}

double CUIElement::GetRasterizationScaleIncludingAncestors()
{
    double rasterizationScale = 1;
    CUIElement* element = this;

    while (element != nullptr)
    {
        rasterizationScale *= element->GetRasterizationScale();

        // Note: This walks through windowed popups, as well. It's expected that windowed popups are treated the same way as
        // normal popups as far as RasterizationScale goes - i.e. the child of a popup will inherit all RasterizationScales
        // above it, regardless of whether the popup is windowed. This means whatever is hosting the windowed popup (hwnd or
        // an island) should be scaled by the system the same way as whatever is hosting the main tree (CoreWindow or an
        // island). Otherwise, we end up with inconsistent behavior where the popup's content can be sharp or blurry/grainy
        // depending on whether that popup is windowed.
        CDependencyObject* elementAncestor = element->GetParentFollowPopups();
        ASSERT(!elementAncestor || elementAncestor->OfTypeByIndex(KnownTypeIndex::UIElement));  // The parent of a UIElement must be a UIElement
        element = static_cast<CUIElement*>(elementAncestor);
    }

    return rasterizationScale;
}

bool CUIElement::BeforeSetLocalOpacity(float newOpacity)
{
    const bool hadExplicitAnimation = GetContext()->GetFacadeStorage().HasExplicitAnimation(this, KnownPropertyIndex::UIElement_Opacity);

    // Note: Setting the same value is considered a no-op. Even if there's a transition in progress and the OpacityTransition
    // property is now null, we will not stop the ongoing transition. We will stop explicit animations, though, because they were
    // started after the last static property set, and have taken over from the static value.
    if (hadExplicitAnimation || newOpacity != m_eOpacityPrivate)
    {
        bool isAnimationEnabled = true;
        IGNOREHR(FxCallbacks::FrameworkCallbacks_IsAnimationEnabled(&isAnimationEnabled));
        DCompTreeHost* dcompTreeHost = GetDCompTreeHost();
        CScalarTransition* transition = GetScalarTransitionNoRef(*this, KnownPropertyIndex::UIElement_OpacityTransition);
        if (dcompTreeHost != nullptr && isAnimationEnabled && transition != nullptr)
        {
            WUComp::ICompositionAnimationBase* implicitAnimation = transition->GetWUCAnimationNoRef(dcompTreeHost, wrl::Wrappers::HStringReference(FacadeProperty_Opacity), newOpacity);

            // Note: Timing is very important here. We're making use of some different behaviors in order to get the outcome
            // we want. We need to start the implicit animation before stopping any existing animations and before setting the
            // static value. See comment in StartAnimation.
            IFCFAILFAST(StartAnimationImpl(implicitAnimation, true));
            return true;
        }

        return false;
    }
    else
    {
        // Treat no-op setters as if we started an transition. Otherwise, setting opacity to the same value twice will cause
        // the second set to stop the transition started by the first set.
        //
        // This also means that the app can't stop a transition that's in progress by nulling out OpacityTransition and setting
        // the Opacity again. That's by design - setting to the same value is always a no-op.
        return true;
    }
}

void CUIElement::OnSetLocalOpacity(bool wasImplicitAnimationStarted)
{
    if (!wasImplicitAnimationStarted)
    {
        // No transition - stop any existing animations. If there was a transition, it will take over from the currently
        // running animation.
        FacadeAnimationHelper::CancelAnimation(this, KnownPropertyIndex::UIElement_Opacity);
    }
    FacadeAnimationHelper::PopulateBackingCompositionObjectWithFacadeIfReferenced(this, KnownPropertyIndex::UIElement_Opacity);
}

void CUIElement::OnActualOffsetChanged()
{
    FacadeAnimationHelper::PopulateBackingCompositionObjectWithFacadeIfReferenced(this, KnownPropertyIndex::UIElement_ActualOffset);
}

void CUIElement::OnActualSizeChanged()
{
    FacadeAnimationHelper::PopulateBackingCompositionObjectWithFacadeIfReferenced(this, KnownPropertyIndex::UIElement_ActualSize);
}

void CUIElement::UpdateHasNonDefaultRotationAxis(const wfn::Vector3& rotationAxis)
{
    if (memcmp(&rotationAxis, &SimpleProperty::Property::Default<KnownPropertyIndex::UIElement_RotationAxis>(), sizeof(wfn::Vector3)) != 0)
    {
        // If the RotationAxis is not (0,0,1) we assume it interplays with other properties in a way that requires a CompNode
        if (!HasNonDefaultRotationAxis())
        {
            SetRequiresComposition(CompositionRequirement::HasNonDefaultRotationAxis, IndependentAnimationType::None);
        }
    }
    else
    {
        if (HasNonDefaultRotationAxis())
        {
            UnsetRequiresComposition(CompositionRequirement::HasNonDefaultRotationAxis, IndependentAnimationType::None);
        }
    }
}

bool CUIElement::IsStrictOnly() const
{
    return GetObjectStrictness() == ObjectStrictness::StrictOnly;
}

// Helper function, allows CUIElement to participate in detecting non-strict-only properties in use
const WCHAR* CUIElement::FindFirstUIElementNonStrictOnlyPropertyInUse() const
{
    // Several ElementCompositionPreviews are non-strict-only but don't use the property system, detect these here.
    if (IsUsingHandOffVisual())
    {
        return L"ElementCompositionPreview.GetElementVisual";
    }
    if (GetIsTranslationEnabled())
    {
        return L"ElementCompositionPreview.SetIsTranslationEnabled";
    }

    return nullptr;
}

// Helper function, allows CUIElement to participate in detecting strict-only properties in use
const WCHAR* CUIElement::FindFirstUIElementStrictOnlyPropertyInUse() const
{
    // The property system doesn't keep track of facade animations which target strict-only properties.  Detect these here.
    KnownPropertyIndex animatingPropertyIndex = GetContext()->GetFacadeStorage().FindFirstStrictAnimatingFacade(this);
    if (animatingPropertyIndex != KnownPropertyIndex::UnknownType_UnknownProperty)
    {
        return MetadataAPI::GetPropertyBaseByIndex(animatingPropertyIndex)->GetName().GetBuffer();
    }

    return nullptr;
}

_Check_return_ HRESULT CUIElement::StartAnimation(_In_ WUComp::ICompositionAnimationBase* animation)
{
    // This is the public API method, which is used to start explicit animations.
    return StartAnimationImpl(animation, false /* isImplicitAnimation */);
}

_Check_return_ HRESULT CUIElement::StartAnimationImpl(_In_ WUComp::ICompositionAnimationBase* animation, bool isImplicitAnimation)
{
    //
    // Note: StartAnimation is also used for implicit property change animations, where timing is very important.
    // We're making use of some different behaviors in order to get the outcome we want.
    //
    // As an example, the old value is 0.2, and the new value is 0.8. We have an implicit animation applied, and the
    // outcome we want is for the animation to start from 0.2 (if animations are enabled on the system), but for the
    // public API value to immediately change to 0.8.
    //
    // We do that by kicking off an animation _before_ updating Xaml's static value of the property. At this point,
    // the value in Xaml is still 0.2. The implicit animation has key frames "this.StartingValue" and 0.8, and at
    // this point "this.StartingValue" will map to 0.2. The situation we want to avoid is updating the composition
    // object's value to 0.8 before starting the animation, because that means the implicit animation will run from
    // 0.8 to 0.8. If we start an animation before setting the new value, we'll update the composition object with
    // Xaml's old value of 0.2, which gives the behavior we're looking for.
    //
    // We also want to kick off the animation _before_ stopping any existing (including implicit) animations on the
    // property. That way we get the correct hand off behavior. StartAnimation identifies hand off cases because it
    // can find an animation already registered on the to-be-animated property. It handles them by not pushing the
    // Xaml static value to the composition object, but instead starting the new animation directly. In this case,
    // the new animation has "this.StartingValue" as its first key frame, which means it will continue from where
    // the previous animation leaves off. Since implicit property change animations go through StartAnimation
    // themselves, this also means we get correctly chained implicit property change animations for free.
    //
    // After starting the animation, we set Xaml's static value to the new value, so that the public API will return
    // the new value. We're also past the point where we update the composition object, so there's no danger of
    // messing up the animation.
    //

    IFC_RETURN(FacadeAnimationHelper::StartAnimation(this, animation, isImplicitAnimation));

    if (!HasFacadeAnimation())
    {
        SetRequiresComposition(CompositionRequirement::HasFacadeAnimation, IndependentAnimationType::None);
    }

    DOFacadeAnimationInfo* opacityFacadeAnimation = GetContext()->GetFacadeStorage().TryGetFacadeAnimationInfo(this, KnownPropertyIndex::UIElement_Opacity);
    if (opacityFacadeAnimation != nullptr && opacityFacadeAnimation->m_animationRunning && !HasWUCOpacityAnimation())
    {
        SetHasWUCOpacityAnimation(true);
    }

    // If the CompNode already exists, we can avoid dirtying the UIElement and instead just dirty the CompNode.
    // Note that we're guaranteed to tick again because we just started an animation, which requests another Commit.
    HWCompTreeNode* compNode = GetCompositionPeer();
    if (compNode != nullptr)
    {
        CDependencyObject::NWSetRenderDirty(compNode, DirtyFlags::Render);
    }

    m_areFacadesInUse = true;
    return S_OK;
}

_Check_return_ HRESULT CUIElement::StopAnimation(_In_ WUComp::ICompositionAnimationBase* animation)
{
    return FacadeAnimationHelper::StopAnimation(this, animation);
}

_Check_return_ HRESULT CUIElement::PullFacadePropertyValueFromCompositionObject(_In_ WUComp::ICompositionObject* backingCO, KnownPropertyIndex facadeID)
{
    wrl::ComPtr<WUComp::ICompositionPropertySet> backingPS;
    IFC_RETURN(backingCO->QueryInterface(IID_PPV_ARGS(&backingPS)));
    WUComp::CompositionGetValueStatus status;

    switch (facadeID)
    {
    case KnownPropertyIndex::UIElement_Translation:
        wfn::Vector3 translation;
        IFCFAILFAST(backingPS->TryGetVector3(wrl_wrappers::HStringReference(FacadeProperty_Translation).Get(), &translation, &status));
        SimpleProperty::Property::id<KnownPropertyIndex::UIElement_Translation>::Set(this, translation);
        UpdateHasTranslateZ(translation);
        CUIElement::NWSetTransformDirty(this, DirtyFlags::Render);
        break;
    case KnownPropertyIndex::UIElement_Rotation:
        FLOAT rotation;
        IFCFAILFAST(backingPS->TryGetScalar(wrl_wrappers::HStringReference(FacadeProperty_Rotation).Get(), &rotation, &status));
        SimpleProperty::Property::id<KnownPropertyIndex::UIElement_Rotation>::Set(this, rotation);
        UpdateHasNonZeroRotation(rotation);
        CUIElement::NWSetTransformDirty(this, DirtyFlags::Render);
        break;
    case KnownPropertyIndex::UIElement_Scale:
        wfn::Vector3 scale;
        IFCFAILFAST(backingPS->TryGetVector3(wrl_wrappers::HStringReference(FacadeProperty_Scale).Get(), &scale, &status));
        SimpleProperty::Property::id<KnownPropertyIndex::UIElement_Scale>::Set(this, scale);
        UpdateHasScaleZ(scale);
        CUIElement::NWSetTransformDirty(this, DirtyFlags::Render);
        break;
    case KnownPropertyIndex::UIElement_TransformMatrix:
        wfn::Matrix4x4 transformMatrix;
        IFCFAILFAST(backingPS->TryGetMatrix4x4(wrl_wrappers::HStringReference(FacadeProperty_TransformMatrix).Get(), &transformMatrix, &status));
        SimpleProperty::Property::id<KnownPropertyIndex::UIElement_TransformMatrix>::Set(this, transformMatrix);
        UpdateHasNonIdentityTransformMatrix(transformMatrix);
        CUIElement::NWSetTransformDirty(this, DirtyFlags::Render);
        OnTransformMatrixForHitTestingChanged(transformMatrix);
        break;
    case KnownPropertyIndex::UIElement_CenterPoint:
        wfn::Vector3 centerPoint;
        IFCFAILFAST(backingPS->TryGetVector3(wrl_wrappers::HStringReference(FacadeProperty_CenterPoint).Get(), &centerPoint, &status));
        SimpleProperty::Property::id<KnownPropertyIndex::UIElement_CenterPoint>::Set(this, centerPoint);
        UpdateHasNonZeroCenterPoint(centerPoint);
        CUIElement::NWSetTransformDirty(this, DirtyFlags::Render);
        break;
    case KnownPropertyIndex::UIElement_RotationAxis:
        wfn::Vector3 rotationAxis;
        IFCFAILFAST(backingPS->TryGetVector3(wrl_wrappers::HStringReference(FacadeProperty_RotationAxis).Get(), &rotationAxis, &status));
        SimpleProperty::Property::id<KnownPropertyIndex::UIElement_RotationAxis>::Set(this, rotationAxis);
        UpdateHasNonDefaultRotationAxis(rotationAxis);
        CUIElement::NWSetTransformDirty(this, DirtyFlags::Render);
        break;
    case KnownPropertyIndex::UIElement_Opacity:
        FLOAT opacity;
        IFCFAILFAST(backingPS->TryGetScalar(wrl_wrappers::HStringReference(FacadeProperty_Opacity).Get(), &opacity, &status));

        // Use standard SetValue to push Opacity back into the property system, this properly invokes all DP-related
        // setter side-effects.
        // Note that this will also go through the code path to start implicit property change animations, but it will
        // not actually start another animation because BeforeSetLocalOpacity checks that the new opacity is different
        // from the current one before starting an animation.
        IFCFAILFAST(SetValueByKnownIndex(KnownPropertyIndex::UIElement_Opacity, opacity));
        break;
    }

    UpdateHas3DDepth();
    return S_OK;
}

void CUIElement::FacadeAnimationComplete(KnownPropertyIndex animatedProperty)
{
    if (animatedProperty == KnownPropertyIndex::UIElement_Opacity && HasWUCOpacityAnimation())
    {
        SetHasWUCOpacityAnimation(false);
    }
}

void CUIElement::AllFacadeAnimationsComplete()
{
    UnsetRequiresComposition(CompositionRequirement::HasFacadeAnimation, IndependentAnimationType::None);
}

void CUIElement::GetFacadeEntries(_Out_ const FacadeMatcherEntry** entries, _Out_ size_t * count)
{
    static const FacadeMatcherEntry facadeEntries[] =
    {
        { FacadeProperty_Translation,     ARRAYSIZE(FacadeProperty_Translation),       KnownPropertyIndex::UIElement_Translation,       true,   false },
        { FacadeProperty_Scale,           ARRAYSIZE(FacadeProperty_Scale),             KnownPropertyIndex::UIElement_Scale,             true,   false },
        { FacadeProperty_TransformMatrix, ARRAYSIZE(FacadeProperty_TransformMatrix),   KnownPropertyIndex::UIElement_TransformMatrix,   true,   false },
        { FacadeProperty_CenterPoint,     ARRAYSIZE(FacadeProperty_CenterPoint),       KnownPropertyIndex::UIElement_CenterPoint,       true,   false },
        { FacadeProperty_RotationAxis,    ARRAYSIZE(FacadeProperty_RotationAxis),      KnownPropertyIndex::UIElement_RotationAxis,      true,   false },
        { FacadeProperty_Rotation,        ARRAYSIZE(FacadeProperty_Rotation),          KnownPropertyIndex::UIElement_Rotation,          false,  false },
        { FacadeProperty_Opacity,         ARRAYSIZE(FacadeProperty_Opacity),           KnownPropertyIndex::UIElement_Opacity,           false,  false },
        { FacadeProperty_ActualOffset,    ARRAYSIZE(FacadeProperty_ActualOffset),      KnownPropertyIndex::UIElement_ActualOffset,      true,   true },
        { FacadeProperty_ActualSize,      ARRAYSIZE(FacadeProperty_ActualSize),        KnownPropertyIndex::UIElement_ActualSize,        true,   true },
    };
    *entries = facadeEntries;
    *count = ARRAYSIZE(facadeEntries);
}

void CUIElement::CreateBackingCompositionObjectForFacade(_In_ WUComp::ICompositor* compositor, _Out_ WUComp::ICompositionObject** backingCO, _Outptr_result_maybenull_ IFacadePropertyListener** listener)
{
    wrl::ComPtr<WUComp::ICompositionPropertySet> backingPS;
    IFCFAILFAST(compositor->CreatePropertySet(&backingPS));
    VERIFYHR(backingPS.Get()->QueryInterface(IID_PPV_ARGS(backingCO)));

    // Create a PropertySetListener, which we'll use to listen for animating facade property values that affect hit-testing.
    wrl::ComPtr<IFacadePropertyListener> facadePropertyListener = wrl::Make<PropertySetListener>(this);
    *listener = facadePropertyListener.Detach();
}

// Propagate the property identified by facadeID from the simple property system into the backing CompositionObject
void CUIElement::PopulateBackingCompositionObjectWithFacade(WUComp::ICompositionObject* backingCO, KnownPropertyIndex facadeID)
{
    wrl::ComPtr<WUComp::ICompositionPropertySet> backingPS;
    VERIFYHR(backingCO->QueryInterface(IID_PPV_ARGS(&backingPS)));

    switch (facadeID)
    {
    case KnownPropertyIndex::UIElement_Translation:
        IFCFAILFAST(backingPS->InsertVector3(wrl_wrappers::HStringReference(FacadeProperty_Translation).Get(), GetTranslation()));
        break;
    case KnownPropertyIndex::UIElement_Rotation:
        IFCFAILFAST(backingPS->InsertScalar(wrl_wrappers::HStringReference(FacadeProperty_Rotation).Get(), GetRotation()));
        break;
    case KnownPropertyIndex::UIElement_Scale:
        IFCFAILFAST(backingPS->InsertVector3(wrl_wrappers::HStringReference(FacadeProperty_Scale).Get(), GetScale()));
        break;
    case KnownPropertyIndex::UIElement_TransformMatrix:
        IFCFAILFAST(backingPS->InsertMatrix4x4(wrl_wrappers::HStringReference(FacadeProperty_TransformMatrix).Get(), GetTransformMatrix()));
        break;
    case KnownPropertyIndex::UIElement_RotationAxis:
        IFCFAILFAST(backingPS->InsertVector3(wrl_wrappers::HStringReference(FacadeProperty_RotationAxis).Get(), GetRotationAxis()));
        break;
    case KnownPropertyIndex::UIElement_CenterPoint:
        IFCFAILFAST(backingPS->InsertVector3(wrl_wrappers::HStringReference(FacadeProperty_CenterPoint).Get(), GetCenterPoint()));
        break;
    case KnownPropertyIndex::UIElement_Opacity:
        IFCFAILFAST(backingPS->InsertScalar(wrl_wrappers::HStringReference(FacadeProperty_Opacity).Get(), m_eOpacityPrivate));
        break;
    case KnownPropertyIndex::UIElement_ActualOffset:
        IFCFAILFAST(backingPS->InsertVector3(wrl_wrappers::HStringReference(FacadeProperty_ActualOffset).Get(), GetActualOffset()));
        break;
    case KnownPropertyIndex::UIElement_ActualSize:
        IFCFAILFAST(backingPS->InsertVector2(wrl_wrappers::HStringReference(FacadeProperty_ActualSize).Get(), GetActualSize()));
        break;
    }
}

// References:  Given a candidate property name string, validate it and if valid return wrapper around backing CO.
_Check_return_ HRESULT CUIElement::PopulatePropertyInfo(
    _In_ HSTRING propertyName,
    _In_ WUComp::IAnimationPropertyInfo* animationPropertyInfo
    )
{
    return FacadeAnimationHelper::PopulatePropertyInfo(this, propertyName, animationPropertyInfo);
}

wfn::Vector3 CUIElement::GetActualOffset()
{
    return {GetActualOffsetX(), GetActualOffsetY(), 0};
}

wfn::Vector2 CUIElement::GetActualSize()
{
    return {GetActualWidth(), GetActualHeight()};
}

bool CUIElement::HasProjection() const
{
    auto projection = GetProjection();
    return (projection != nullptr);
}

xref_ptr<CProjection> CUIElement::GetProjection() const
{
    // Perf:  Avoid overhead of GetValueByIndex if the effective value is not actually set
    if (IsEffectiveValueInSparseStorage(KnownPropertyIndex::UIElement_Projection))
    {
        CValue result;
        VERIFYHR(GetValueByIndex(KnownPropertyIndex::UIElement_Projection, &result));
        return static_sp_cast<CProjection>(result.DetachObject());
    }
    return xref_ptr<CProjection>(nullptr);
}

bool CUIElement::AreAllAncestorsVisible()
{
    CUIElement* pElement = this;

    while (pElement != NULL)
    {
        CUIElement* pNext = pElement->GetUIElementAdjustedParentInternal(TRUE /*public parents only*/);

        if (pNext && pNext->IsCollapsed())
        {
            return false;
        }

        pElement = pNext;
    }

    return true;
}

// Gets the RectangleGeometry used to store the hand off visual's clip
xref_ptr<CRectangleGeometry> CUIElement::GetHandOffVisualClip() const
{
    if (IsEffectiveValueInSparseStorage(KnownPropertyIndex::UIElement_HandOffVisualClip))
    {
        CValue result;
        VERIFYHR(GetValueByIndex(KnownPropertyIndex::UIElement_HandOffVisualClip, &result));
        return static_sp_cast<CRectangleGeometry>(result.DetachObject());
    }
    return xref_ptr<CRectangleGeometry>(nullptr);
}

xref_ptr<CGeometry> CUIElement::GetClip() const
{
    CValue result;
    VERIFYHR(GetValueByIndex(KnownPropertyIndex::UIElement_Clip, &result));
    return static_sp_cast<CGeometry>(result.DetachObject());
}

xref_ptr<CGeometry> CUIElement::GetHandOffVisualClipOrElementClip() const
{
    const auto& handOffVisualClip = GetHandOffVisualClip();
    if (handOffVisualClip == nullptr)
    {
        return GetClip();
    }
    else
    {
        xref_ptr<CGeometry> clipAsGeometry(static_cast<CGeometry*>(handOffVisualClip.get()));
        return clipAsGeometry;
    }
}

// Ensure that a RectangleGeometry for the HandOff visual's clip has been created
xref_ptr<CRectangleGeometry> CUIElement::EnsureHandOffVisualClip()
{
    const auto& existingClip = GetHandOffVisualClip();

    if (!existingClip)
    {
        CREATEPARAMETERS cp(GetContext());

        xref_ptr<CRectangleGeometry> clip;
        IFCFAILFAST(CRectangleGeometry::Create(reinterpret_cast<CDependencyObject **>(&clip), &cp));

        CValue valueClip;
        valueClip.SetObjectAddRef(clip.get());
        IFCFAILFAST(SetValueByIndex(KnownPropertyIndex::UIElement_HandOffVisualClip, valueClip));

        SetIsListeningForHandOffVisualPropertyChanges(true);

        return clip;
    }
    else
    {
        return existingClip;
    }
}

// Handle change of HandOff visual's clip by changing corresponding local clip for hit testing
void CUIElement::SetHandOffVisualClip(_In_ const XRECTF& clip)
{
    const auto& clipGeometry = EnsureHandOffVisualClip();

    clipGeometry->m_rc = clip;

    NWSetDirtyFlagsAndPropagate(DirtyFlags::Bounds, FALSE);
}

void CUIElement::ClearHandOffVisualClip()
{
    if (IsEffectiveValueInSparseStorage(KnownPropertyIndex::UIElement_HandOffVisualClip))
    {
        IFCFAILFAST(ClearValueByIndex(KnownPropertyIndex::UIElement_HandOffVisualClip));
        NWSetDirtyFlagsAndPropagate(DirtyFlags::Bounds, FALSE);
    }
}

// Handle change of HandOff visual's clip by changing corresponding local clip for hit testing
void CUIElement::SetHandOffVisualClipTransform(_In_ const wfn::Matrix3x2& transform)
{
    const auto& clipGeometry = EnsureHandOffVisualClip();

    if (!clipGeometry->m_pTransform)
    {
        CREATEPARAMETERS cp(GetContext());
        IFCFAILFAST(CMatrixTransform::Create(reinterpret_cast<CDependencyObject**>(&(clipGeometry->m_pTransform)), &cp));
    }

    static_cast<CMatrixTransform*>(clipGeometry->m_pTransform)->SetMatrix(*reinterpret_cast<const CMILMatrix*>(&transform));

    NWSetDirtyFlagsAndPropagate(DirtyFlags::Bounds, FALSE);
}

void CUIElement::DetachWUCPropertyListenerFromWUCClip()
{
    const auto& handOffVisualDataMap = GetDCompTreeHost()->GetHandOffVisualDataMap();

    const auto& entry = handOffVisualDataMap.find(this);
    if (entry != handOffVisualDataMap.end())
    {
        entry->second.dcompPropertyChangedListener->DetachFromWUCClip();
    }
}

bool CUIElement::HasHandOffVisualTransform() const
{
    return IsEffectiveValueInSparseStorage(KnownPropertyIndex::UIElement_HandOffVisualTransformMatrix3D)
        || GetHandOffVisualSubTransform(KnownTypeIndex::MatrixTransform);
}

bool CUIElement::HasHandOffVisualTransformMatrix3D() const
{
    return IsEffectiveValueInSparseStorage(KnownPropertyIndex::UIElement_HandOffVisualTransformMatrix3D);
}

CMILMatrix4x4 CUIElement::GetHandOffVisualTransformMatrix3D() const
{
    // If we went down this code path then we should have a 3D matrix in the private stash.
    ASSERT(HasHandOffVisualTransformMatrix3D());

    CValue result;
    VERIFYHR(GetValueByIndex(KnownPropertyIndex::UIElement_HandOffVisualTransformMatrix3D, &result));
    return CMILMatrix4x4::FromFloatArray(result);
}

void CUIElement::EnsureHandOffVisualTransformMatrix3D(_In_ const CMILMatrix4x4& matrix)
{
    CREATEPARAMETERS cp(GetContext());

    xref_ptr<CMatrix4x4> cmatrix;
    IFCFAILFAST(CMatrix4x4::Create(reinterpret_cast<CDependencyObject **>(&cmatrix), &cp));
    cmatrix->m_matrix = matrix;

    CValue valueMatrix;
    valueMatrix.SetObjectAddRef(cmatrix.get());
    IFCFAILFAST(SetValueByIndex(KnownPropertyIndex::UIElement_HandOffVisualTransformMatrix3D, valueMatrix));

    SetIsListeningForHandOffVisualPropertyChanges(true);
    ASSERT(!matrix.Is2D());
    SetHas3DDepth(true);
}

void CUIElement::ClearHandOffVisualTransformMatrix3D()
{
    IFCFAILFAST(ClearValueByIndex(KnownPropertyIndex::UIElement_HandOffVisualTransformMatrix3D));

    // WUC has notified us that the net transform on this visual is 2D. Drop out of 3D hit testing mode.
    //
    // This only applies in container visuals mode. In legacy DComp visuals mode, the hand off visual is a separate visual
    // entirely, and transforms set on it will stack with Xaml properties, so it's possible for there to be no 3D matrix on
    // the hand off visual but still be in 3D mode.
    SetHas3DDepth(false);
}

CMILMatrix4x4 CUIElement::GetHitTestingTransform3DMatrix() const
{
    // TODO: HitTest: Legacy hit testing code path called this with false. This can be removed once we switch over to the new hit testing code path.
    return GetHitTestingTransform3DMatrix(false);
}

CMILMatrix4x4 CUIElement::GetHitTestingTransform3DMatrix(bool includePropertySetInComposition) const
{
    CMILMatrix4x4 result(true);
    bool has3DStash = HasHandOffVisualTransformMatrix3D();
    if (has3DStash)
    {
        result = GetHandOffVisualTransformMatrix3D();
    }
    else
    {
        const auto& transform3D = GetTransform3D();
        if (CanUseHandOffVisualTransformGroupForLocalTransform() && includePropertySetInComposition)
        {
            // Special case: if a hand off visual is involved then we're listening to its transform changes. In ContainerVisuals
            // mode, the hand off visual's transform includes all transforms on the UIElement, including its Transform3D. If the
            // final transform was 3D, then we should be in a different if branch above (under HasHandOffVisualTransformMatrix3D).
            // So the net transform must be 2D at this point, which means we've included it in the 2D transform and already returned
            // it from GetLocalTransform. So add nothing here.
            //
            // Note that we should add nothing even if this UIElement has a Transform3D that contains depth. If that's the case,
            // and we still made it to this if branch, it means the app stomped over the Transform3D with their own transform set
            // directly on the hand off visual. We should hit test against the transform set by the app on the hand off visual,
            // which was already included in GetLocalTransform.
        }
        else if (transform3D)
        {
            result = transform3D->GetTransformMatrix();
        }
    }
    if (AreFacadesInUse())
    {
        // Incorporate facades that are in use.  Note that since the Translation facade isn't a strict property, it can be present
        // in combination with the others.  Thus we append whatever facades we find to the end of the current result.
        FacadeTransformInfo facadeInfo = GetFacadeTransformInfo(true /* preferAnimatingValue */);
        if (!facadeInfo.Is2D())
        {
            if (!has3DStash)
            {
                // We still haven't incorporated any 2D hit-testing stash that might be present.  Incorporate that now.
                auto handoffVisualTransform = GetHandOffVisualTransform();
                if (handoffVisualTransform)
                {
                    CMILMatrix hitTestStash2D;
                    handoffVisualTransform->GetTransform(&hitTestStash2D);
                    CMILMatrix4x4 hitTestStash2D4x4;
                    hitTestStash2D4x4.SetTo2DTransform(&hitTestStash2D);
                    result = hitTestStash2D4x4;
                }
            }
            result.Append(facadeInfo.GetMatrix4x4());
        }
    }
    return result;
}

void CUIElement::SetIsTranslationEnabled(bool enabled)
{
    m_isTranslationEnabled = enabled;
    CUIElement::NWSetContentDirty(this, DirtyFlags::Render);
    EnsureTranslationPropertyInitialized();

    // Stop listening to the Prepend visual if Translation was disabled
    // Note:  PushProperties is responsible for attaching to the listener as we're not guaranteed to have a Prepend visual until then.
    if (!m_isTranslationEnabled)
    {
        DetachListenerFromPrependVisual();
    }
}

void CUIElement::EnsureTranslationPropertyInitialized()
{
    if (GetIsTranslationEnabled() && IsUsingHandOffVisual())
    {
        wrl::ComPtr<WUComp::IVisual> handOffVisual;
        GetStoredHandOffVisual(&handOffVisual);
        EnsureTranslationInitialized(handOffVisual.Get());
    }
}

/* static */
void CUIElement::EnsureTranslationInitialized(_In_ IUnknown* primaryVisual)
{
    Microsoft::WRL::ComPtr<WUComp::ICompositionObject> primaryAsCompositionObject;
    VERIFYHR(primaryVisual->QueryInterface(IID_PPV_ARGS(&primaryAsCompositionObject)));

    Microsoft::WRL::ComPtr<WUComp::ICompositionPropertySet> primaryPropertySet;
    IFCFAILFAST(primaryAsCompositionObject->get_Properties(primaryPropertySet.GetAddressOf()));
    WUComp::CompositionGetValueStatus translationResult;
    wfn::Vector3 translationValue;
    IFCFAILFAST(primaryPropertySet->TryGetVector3(wrl_wrappers::HStringReference(s_translationString).Get(), &translationValue, &translationResult));
    if (translationResult == WUComp::CompositionGetValueStatus_NotFound)
    {
        translationValue.X = 0;
        translationValue.Y = 0;
        translationValue.Z = 0;
        IFCFAILFAST(primaryPropertySet->InsertVector3(wrl_wrappers::HStringReference(s_translationString).Get(), translationValue));
    }
}

void CUIElement::AttachListenerToPrependVisual(_In_ WUComp::IVisual* prependVisual)
{
    const auto& handOffVisualDataMap = GetDCompTreeHost()->GetHandOffVisualDataMap();

    const auto& entry = handOffVisualDataMap.find(this);
    if (entry != handOffVisualDataMap.end())
    {
        wrl::ComPtr<IInspectable> inspectable;
        VERIFYHR(prependVisual->QueryInterface(IID_PPV_ARGS(&inspectable)));
        entry->second.dcompPropertyChangedListener->AttachToPrependVisual(inspectable.Get());
    }
}

void CUIElement::DetachListenerFromPrependVisual()
{
    if (GetContext()->NWGetWindowRenderTarget() != nullptr && GetDCompTreeHost() != nullptr)
    {
        const auto& handOffVisualDataMap = GetDCompTreeHost()->GetHandOffVisualDataMap();

        const auto& entry = handOffVisualDataMap.find(this);
        if (entry != handOffVisualDataMap.end())
        {
            entry->second.dcompPropertyChangedListener->DetachFromPrependVisual();
        }
    }
}

void CUIElement::AddLightTargetId(_In_ const xstring_ptr& lightId)
{
    auto& lightTargetIdMap = GetDCompTreeHost()->GetXamlLightTargetIdMap();
    bool hasMapChanged = lightTargetIdMap.AddTargetElementAndId(this, lightId);

    if (hasMapChanged)
    {
        CUIElement::NWSetLightTargetDirty(this, true /* isLightTarget */);
    }
}

void CUIElement::RemoveLightTargetId(_In_ const xstring_ptr& lightId)
{
    auto& lightTargetIdMap = GetDCompTreeHost()->GetXamlLightTargetIdMap();
    bool hasMapChanged = lightTargetIdMap.RemoveTargetElementAndId(this, lightId);

    if (hasMapChanged)
    {
        bool isLightTarget = lightTargetIdMap.ContainsTarget(this); // There can be multiple light IDs on this UIElement
        CUIElement::NWSetLightTargetDirty(this, isLightTarget);
    }
}

bool CUIElement::IsLightTarget()
{
    const auto& lightTargetIdMap = GetDCompTreeHost()->GetXamlLightTargetIdMap();
    return lightTargetIdMap.ContainsTarget(this);
}

bool CUIElement::IsTargetedByLight(_In_ CXamlLight* light)
{
    const auto& lightTargetIdMap = GetDCompTreeHost()->GetXamlLightTargetIdMap();
    const auto& lightId = light->GetLightId();
    return lightTargetIdMap.ContainsTarget(this, lightId);
}

xref_ptr<CXamlLightCollection> CUIElement::GetXamlLightCollection() const
{
    // Perf:  Avoid overhead of GetValueByIndex if the effective value is not actually set
    if (IsEffectiveValueInSparseStorage(KnownPropertyIndex::UIElement_Lights))
    {
        CValue result;
        VERIFYHR(GetValueByIndex(KnownPropertyIndex::UIElement_Lights, &result));
        return static_sp_cast<CXamlLightCollection>(result.DetachObject());
    }
    return xref_ptr<CXamlLightCollection>(nullptr);
}

bool CUIElement::HasXamlLights() const
{
    const auto& xamlLights = GetXamlLightCollection();
    return (xamlLights != nullptr) && (xamlLights->size() > 0);
}

void CUIElement::UntargetByLight(_In_ CXamlLight* light)
{
    auto& lightTargetMap = GetDCompTreeHost()->GetXamlLightTargetMap();
    lightTargetMap.RemoveTargetAndLight(this, light);
}

// Returns true if this element is included in the PC scene for rendering.
bool CUIElement::IsInPCScene() const
{
    return (m_propertyRenderData.IsRenderWalkTypeForComposition() && m_propertyRenderData.pc.isInScene);
}

// Returns true if this element is included in the PC scene for rendering.
// Note: We could be in a device lost state and still hold on to rendering data like comp nodes. This method counts that state
// as being in the PC scene as well. For RS4 we should remove the "device lost but preserve DComp" state altogether, since we can
// now track the values pushed into hand off visuals without keeping the comp node alive.
bool CUIElement::IsInPCScene_IncludingDeviceLost() const
{
    return IsInPCScene()
        || m_propertyRenderData.type == RWT_NonePreserveDComp;
}

_Check_return_ HRESULT CUIElement::GetPCPreChildrenRenderDataNoRef(_Outptr_ PCRenderDataList** renderData)
{
    HRESULT hr = S_OK;

    ASSERT(IsInPCScene());

    if (m_propertyRenderData.pc.pPreChildrenRenderData == NULL)
    {
        m_propertyRenderData.pc.pPreChildrenRenderData = new PCRenderDataList();
    }

    *renderData = m_propertyRenderData.pc.pPreChildrenRenderData;

    RRETURN(hr);//RRETURN_REMOVAL
}

_Check_return_ HRESULT CUIElement::TabFocusNavigation(
    _In_ CDependencyObject* object,
    _In_ XUINT32 cArgs,
    _Inout_updates_(cArgs) CValue *ppArgs,
    _In_opt_ IInspectable* pValueOuter,
    _Out_ CValue *pResult)
{
    ASSERT(cArgs < 2);

    CUIElement* uiElement = do_pointer_cast<CUIElement>(object);

    if (cArgs == 1 && ppArgs)
    {
        ASSERT(ppArgs->IsEnum());

        const DirectUI::KeyboardNavigationMode currentValue = uiElement->TabNavigation();
        DirectUI::KeyboardNavigationMode newValue = currentValue;

        if (ppArgs->IsEnum())
        {
            newValue = static_cast<DirectUI::KeyboardNavigationMode>(ppArgs->AsEnum());
        }

        if (currentValue != newValue)
        {
            uiElement->SetTabNavigation(newValue);
        }

        if (pResult && currentValue != newValue)
        {
            pResult->Set(newValue, KnownTypeIndex::KeyboardNavigationMode);
        }
    }
    else if (pResult)
    {
        // We are getting the value
        pResult->Set(uiElement->TabNavigation(), KnownTypeIndex::KeyboardNavigationMode);
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Adds a layout transition element targeting this element.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElement::AddLayoutTransitionRenderer(
    _In_ CLayoutTransitionElement *pLayoutTransitionElement
    )
{
    if (m_pLayoutTransitionRenderers == NULL)
    {
        m_pLayoutTransitionRenderers = new xvector<CLayoutTransitionElement*>();
    }

    // Notify the LTE whether it's the primary transition for this element or not.
    // TODO: HWPC: LTEs are bound to a single target element. All this initialization should happen on LTE creation instead of spread over a bunch of methods.
    pLayoutTransitionElement->SetIsPrimaryTransitionForTarget(m_pLayoutTransitionRenderers->empty());

    // These are back pointers, so they're not ref-counted as with other DO parent pointers.
    IFC_RETURN(m_pLayoutTransitionRenderers->push_back(pLayoutTransitionElement));

    // Since this element is being rendered in another part of the tree, remove it and any render
    // data in the subgraph from the scene. It'll be added back whenever the LTE renders it.
    // NOTE: The LTE will be rendered because it requires composition.
    LeavePCSceneRecursive();

    // Mark the real parent dirty since this child is hidden for layout transition.
    CDependencyObject *pParent = GetParentInternal(false);
    ASSERT(pParent->OfTypeByIndex<KnownTypeIndex::UIElement>());
    CUIElement::NWSetSubgraphDirty(pParent, DirtyFlags::Render | DirtyFlags::Bounds);

    // Mark the LTE dirty since this child is now being redirected.
    CUIElement::NWSetSubgraphDirty(pLayoutTransitionElement, DirtyFlags::Render | DirtyFlags::Bounds);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Removes a layout transition element targeting this element.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElement::RemoveLayoutTransitionRenderer(
    _In_ CLayoutTransitionElement *pLayoutTransitionElement
    )
{
    ASSERT(m_pLayoutTransitionRenderers != NULL && !m_pLayoutTransitionRenderers->empty());

    xvector<CLayoutTransitionElement*>::const_reverse_iterator rend = m_pLayoutTransitionRenderers->rend();

    // Since this element is being rendered in another part of the tree, remove it and any render
    // data in the subgraph from the scene. It'll be added back whenever its regular visual parent renders it.
    LeavePCSceneRecursive();

    // Mark the real parent dirty since it is now responsible for drawing and
    // bounding the child again. Note that in some cases the target might have
    // left the visual tree already (e.g. a page as it is replaced during navigation),
    // so we always check that the parent is not null first.
    CDependencyObject *pParent = GetParentInternal(false);
    if (pParent)
    {
        ASSERT(pParent->OfTypeByIndex<KnownTypeIndex::UIElement>());
        CUIElement::NWSetSubgraphDirty(pParent, DirtyFlags::Render | DirtyFlags::Bounds);
    }

    // Mark the LTE dirty since its target is no longer redirected.
    CUIElement::NWSetSubgraphDirty(pLayoutTransitionElement, DirtyFlags::Render | DirtyFlags::Bounds);

    for (xvector<CLayoutTransitionElement*>::const_reverse_iterator it = m_pLayoutTransitionRenderers->rbegin(); it != rend; ++it)
    {
        if (*it == pLayoutTransitionElement)
        {
            // Remove the renderer.
            IFC_RETURN(m_pLayoutTransitionRenderers->erase(it));
            return S_OK;
        }
    }

    // We should have removed an element and skipped to Cleanup.
    ASSERT(FALSE);

    return S_OK;
}

_Check_return_ CPopupRoot* CUIElement::GetPopupRoot()
{
    CPopupRoot* popupRoot;
    IFCFAILFAST(VisualTree::GetPopupRootForElementNoRef(this, &popupRoot));

    if (popupRoot == nullptr)
    {
        popupRoot = GetContext()->GetMainPopupRoot();
    }

    return popupRoot;
}

bool CUIElement::HasAncestorIncludingPopups(_In_ const CUIElement* const ancestor)
{
    const CUIElement* current = this;
    const CUIElement* next = nullptr;
    const CPopupRoot* popupRoot = GetPopupRoot();

    while (current != nullptr && current != ancestor)
    {
        next = static_cast<CUIElement*>(current->GetParentInternal(false /*fPublic*/));
        if (popupRoot != nullptr && next == popupRoot)
        {
            // We're interested in rendering as part of a subtree, and the unloading child of a popup still renders
            // in that child's subtree, so look at the unloading child too.
            current = popupRoot->GetOpenPopupWithChild(current, true /* checkUnloadingChildToo */);
        }
        else
        {
            current = next;
        }
    }

    return (current == ancestor);
}

void CUIElement::ShownHiddenHandlerAdded()
{
    ASSERT(!HasImplicitShowAnimation() && !HasImplicitHideAnimation());

    DCompTreeHost* dcompTreeHost = GetDCompTreeHost();
    dcompTreeHost->SetTrackEffectiveVisibility(this, true);
}

void CUIElement::AllShownHiddenHandlersRemoved()
{
    DCompTreeHost* dcompTreeHost = GetDCompTreeHost();
    dcompTreeHost->SetTrackEffectiveVisibility(this, false);
}

bool CUIElement::AllowsHandlerWhenNotLive(XINT32 iListenerType, KnownEventIndex eventIndex) const
{
    // The shown/hidden events are allowed to fire on inactive UIElements.
    return eventIndex == KnownEventIndex::UIElement_Shown ||eventIndex == KnownEventIndex::UIElement_Hidden;
}

bool CUIElement::HasKeepAliveCount() const
{
    return GetKeepAliveCount() > 0;
}

int CUIElement::GetKeepAliveCount() const
{
    return SimpleProperty::Property::id<KnownPropertyIndex::UIElement_KeepAliveCount>::Get(this);
}

void CUIElement::RequestKeepAlive()
{
    ASSERT(!HasImplicitShowAnimation() && !HasImplicitHideAnimation());

    int newKeepAliveCount = GetKeepAliveCount() + 1;
    ASSERT(newKeepAliveCount > 0);
    SimpleProperty::Property::id<KnownPropertyIndex::UIElement_KeepAliveCount>::Set(this, newKeepAliveCount);

    if (newKeepAliveCount == 1)
    {
        DCompTreeHost* dcompTreeHost = GetDCompTreeHost();
        dcompTreeHost->AddElementWithKeepAliveCount(this);
    }
}

void CUIElement::ReleaseKeepAlive()
{
    ASSERT(!HasImplicitShowAnimation() && !HasImplicitHideAnimation());

    int newKeepAliveCount = GetKeepAliveCount() - 1;
    ASSERT(newKeepAliveCount >= 0);
    SimpleProperty::Property::id<KnownPropertyIndex::UIElement_KeepAliveCount>::Set(this, newKeepAliveCount);

    if (newKeepAliveCount == 0)
    {
        DCompTreeHost* dcompTreeHost = GetDCompTreeHost();
        dcompTreeHost->RemoveElementWithKeepAliveCount(this);

        // Request a new frame, which will recompute elements that need to be kept visible.
        SetDirtyToRoot();
    }
}

bool CUIElement::IsKeepingAliveUntilHiddenEventIsRaised() const
{
    return m_isKeepingAliveUntilHiddenEventIsRaised;
}

void CUIElement::SetIsKeepingAliveUntilHiddenEventIsRaised(bool value)
{
    m_isKeepingAliveUntilHiddenEventIsRaised = value;
}

bool CUIElement::ShouldApplyLayoutClipAsAncestorClip() const
{
    return OfTypeByIndex<KnownTypeIndex::Panel>() &&          // Restrict to Panels, to limit app-compat risk
           !GetIsScrollViewerHeader();                      // Special-case:  ScrollViewer Headers, which can zoom, must scale the LayoutClip too
}

/* static */ bool CUIElement::RenderDataListHasHitTestVisibleVisuals(const PCRenderDataList* renderDataList)
{
    if (renderDataList != nullptr)
    {
        for (IUnknown* renderData : *renderDataList)
        {
            wrl::ComPtr<WUComp::IVisual> visual;
            if (SUCCEEDED(renderData->QueryInterface(IID_PPV_ARGS(&visual))))
            {
                wrl::ComPtr<ixp::IVisual3> visual3;
                IFCFAILFAST(visual.As(&visual3));

                boolean isHitTestVisible = true;
                IFCFAILFAST(visual3->get_IsHitTestVisible(&isHitTestVisible));

                // If we found a WUC::Visual, check for IsHitTestVisible. If it can't be hit tested, keep looking.
                if (isHitTestVisible)
                {
                    return true;
                }
            }
            else
            {
                // Note: This case should not be possible in the product, because we use SpriteVisuals when we're using
                // ViewportInteractions. Our tests can create this combination though.
                ASSERT(false);
                return true;
            }
        }
    }

    return false;
}

bool CUIElement::HasHitTestVisibleVisuals() const
{
    if (IsInPCScene())
    {
        if (RenderDataListHasHitTestVisibleVisuals(m_propertyRenderData.pc.pPreChildrenRenderData))
        {
            return true;
        }

        if (RenderDataListHasHitTestVisibleVisuals(m_propertyRenderData.pc.pPostChildrenRenderData))
        {
            return true;
        }
    }

    return false;
}

void CUIElement::AddedAsShadowReceiver()
{
    int usageCount = SimpleProperty::Property::id<KnownPropertyIndex::UIElement_ThemeShadowReceiverCount>::Get(this);
    ++usageCount;
    SimpleProperty::Property::id<KnownPropertyIndex::UIElement_ThemeShadowReceiverCount>::Set(this, usageCount);

    if (usageCount == 1)
    {
        SetRequiresComposition(CompositionRequirement::ProjectedShadowCustomReceiver, IndependentAnimationType::None);
    }
}

void CUIElement::RemovedAsShadowReceiver()
{
    int usageCount = SimpleProperty::Property::id<KnownPropertyIndex::UIElement_ThemeShadowReceiverCount>::Get(this);
    ASSERT(usageCount > 0);
    --usageCount;
    SimpleProperty::Property::id<KnownPropertyIndex::UIElement_ThemeShadowReceiverCount>::Set(this, usageCount);

    if (usageCount == 0)
    {
        UnsetRequiresComposition(CompositionRequirement::ProjectedShadowCustomReceiver, IndependentAnimationType::None);
    }
}

void CUIElement::SetShadowVisual(_In_opt_ WUComp::IVisual* shadowVisual)
{
    ASSERT(!CThemeShadow::IsDropShadowMode());
    GetDCompTreeHost()->GetProjectedShadowManager()->SetShadowVisual(this, shadowVisual);
}

WUComp::IVisual* CUIElement::GetShadowVisualNoRef()
{
    ASSERT(!CThemeShadow::IsDropShadowMode());
    return GetDCompTreeHost()->GetProjectedShadowManager()->GetShadowVisualNoRef(this);
}

bool CUIElement::HasActiveBrushTransitions() const
{
    return GetDCompTreeHost()->GetWUCBrushManager()->HasActiveBrushTransitions(this);
}

/* static */ template <typename InterfaceType, typename DXamlType, typename CoreType> CoreType* CUIElement::GetTransitionNoRef(_In_ CDependencyObject& object, KnownPropertyIndex propertyIndex)
{
    CValue result;
    VERIFYHR(object.GetValueByIndex(propertyIndex, &result));

    // The value can be stored in a couple of different ways...
    CDependencyObject* resultObject = result.AsObject();
    CoreType* coreTransition = do_pointer_cast<CoreType>(resultObject);
    if (coreTransition != nullptr)
    {
        // If it was set by the parser, then we have a core transition object directly in the property system. Return that.
        return coreTransition;
    }
    else
    {
        // If it was set from code-behind, then the core object is wrapped behind a few layers. This is because
        // the DXaml transitions block QIs to IDependencyObject, which means the property system does not recognize
        // them as DOs, even though they derive from DirectUI::DependencyObject internally. The property system
        // will wrap them inside an ExternalObjectReference, then wrap that inside a CManagedObjectReference.
        // We need to unwrap those two layers and then unwrap the DXaml transition object to get to the core
        // transition object.

        // TODO: If we can store the IInspectable DXaml transition instead of wrapping it, we can simplify some of this.

        CManagedObjectReference* resultMOR = do_pointer_cast<CManagedObjectReference>(resultObject);
        if (resultMOR != nullptr
            && resultMOR->GetDXamlPeer() != nullptr
            && resultMOR->GetDXamlPeer()->GetTypeIndex() == KnownTypeIndex::ExternalObjectReference)
        {
            auto resultEOR = static_cast<DirectUI::ExternalObjectReference*>(resultMOR->GetDXamlPeer());

            wrl::ComPtr<IInspectable> transitionII;
            resultEOR->get_Target(transitionII.ReleaseAndGetAddressOf());

            wrl::ComPtr<InterfaceType> dxamlTransitionInterface;
            IFCFAILFAST(transitionII.As(&dxamlTransitionInterface));

            // No need to return a ref counted pointer - the property system still holds a ref.
            DXamlType* dxamlTransition = ctl::impl_cast<DXamlType>(dxamlTransitionInterface.Get());
            return checked_cast<CoreType>(dxamlTransition->GetHandle());
        }
    }

    return nullptr;
}

/* static */ CScalarTransition* CUIElement::GetScalarTransitionNoRef(_In_ CDependencyObject& object, KnownPropertyIndex propertyIndex)
{
    return GetTransitionNoRef<xaml::IScalarTransition, DirectUI::ScalarTransitionGenerated, CScalarTransition>(object, propertyIndex);
}

/* static */ CVector3Transition* CUIElement::GetVector3TransitionNoRef(_In_ CDependencyObject& object, KnownPropertyIndex propertyIndex)
{
    return GetTransitionNoRef<xaml::IVector3Transition, DirectUI::Vector3TransitionGenerated, CVector3Transition>(object, propertyIndex);
}

/* static */ CBrushTransition* CUIElement::GetBrushTransitionNoRef(_In_ CDependencyObject& object, KnownPropertyIndex propertyIndex)
{
    return GetTransitionNoRef<xaml::IBrushTransition, DirectUI::BrushTransitionGenerated, CBrushTransition>(object, propertyIndex);
}
