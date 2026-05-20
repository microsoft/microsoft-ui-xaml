// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <FeatureFlags.h>
#include <UIElement.h>

#include "UIElementCollection.h"
#include "theming\inc\Theme.h"
#include <TextElement.h>
#include "Popup.h"
#include "FlyoutBase.h"
#include "XamlIslandRoot.h"
#include "RootVisual.h"


CUIElement::~CUIElement()
{
    if (m_pChildren)
    {
        VERIFYHR(m_pChildren->Clear());
        ReleaseInterface(m_pChildren);
    }
}

void CUIElement::NWSetTransformDirty(_In_ CDependencyObject *pTargetf, DirtyFlags flags)
{
}

_Check_return_ HRESULT CUIElement::Measure(XSIZEF availableSize)
{
    return S_OK;
}

_Check_return_ HRESULT CUIElement::Arrange(XRECTF finalRect)
{
    return S_OK;
}

_Check_return_ HRESULT CUIElement::EnterImpl(_In_ CDependencyObject *pNamescopeOwner, _In_ EnterParams params)
{
    return CDependencyObject::EnterImpl(pNamescopeOwner, params);
}

_Check_return_ HRESULT CUIElement::LeaveImpl(_In_ CDependencyObject *pNamescopeOwner, _In_ LeaveParams params)
{
    return CDependencyObject::LeaveImpl(pNamescopeOwner, params);
}

_Check_return_ HRESULT CUIElement::MarkInheritedPropertyDirty(_In_ const CDependencyProperty* pdp, _In_ const CValue* pValue)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CUIElement::NotifyThemeChangedCore(_In_ Theming::Theme theme, _In_ bool fForceRefresh)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CUIElement::NotifyApplicationHighContrastAdjustmentChanged()
{
    return E_NOTIMPL;
}

bool CUIElement::ShouldRaiseEvent(_In_ EventHandle hEvent, _In_ bool fInputEvent, _In_opt_ CEventArgs *pArgs)
{
    return false;
}

void CUIElement::PropagateLayoutDirty(bool affectsParentMeasure, bool affectsParentArrange)
{
}

_Check_return_ HRESULT CUIElement::GetValue(_In_ const CDependencyProperty* dp, _Out_ CValue* value)
{
    return CDependencyObject::GetValue(dp, value);
}

_Check_return_ HRESULT CUIElement::SetValue(_In_ const SetValueParams& args)
{
    return CDependencyObject::SetValue(args);
}

_Check_return_ HRESULT CUIElement::OnPropertyChanged(_In_ const PropertyChangedParams& args)
{
    return S_OK;
}

_Check_return_ HRESULT CUIElement::PullInheritedTextFormatting()
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CUIElement::AddEventListener(
    _In_ EventHandle hEvent,
    _In_ CValue *pValue,
    _In_ XINT32 iListenerType,
    _Out_opt_ CValue *pResult,
    _In_ bool fHandledEventsToo)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CUIElement::RemoveEventListener(_In_ EventHandle hEvent, _In_ CValue *pValue)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CUIElement::AddChild(_In_ CUIElement *pChild)
{
    if (m_pChildren == nullptr)
    {
        CUIElementCollection *pUIElementCollection = NULL;
        #pragma warning(suppress: 6387) // test function, only case we allow nullptr
        pUIElementCollection =  new CUIElementCollection(nullptr);
        XCP_FAULT_ON_FAILURE(pUIElementCollection != nullptr);

        m_pChildren = pUIElementCollection;
    }

    IFCFAILFAST(m_pChildren->Append(pChild));
    IFCFAILFAST(pChild->AddParent(this));

    return S_OK;
}

void CUIElement::GetShouldFlipRTL(_Out_ bool *pShouldFlipRTL, _Out_ bool *pShouldFlipRTLInPlace)
{
}

CAutomationPeer* CUIElement::OnCreateAutomationPeer()
{
    return nullptr;
}

CAutomationPeer* CUIElement::GetAutomationPeer()
{
    return nullptr;
}

_Check_return_ HRESULT CUIElement::SetAutomationPeer(_In_ CAutomationPeer* pAP)
{
    return E_NOTIMPL;
}

CAutomationPeer* CUIElement::OnCreateAutomationPeerImpl()
{
    return nullptr;
}

_Check_return_ XUINT32 CUIElement::GetAPChildren(_Outptr_result_buffer_(return) CAutomationPeer ***pppReturnAP)
{
    return 0;
}

CAutomationPeer* CUIElement::GetPopupAssociatedAutomationPeer()
{
    return nullptr;
}

_Check_return_ HRESULT CUIElement::CoerceIsEnabled(_In_ bool bIsEnabled, _In_ bool bCoerceChildren)
{
    return E_NOTIMPL;
}

_Check_return_ XFLOAT CUIElement::GetActualWidth()
{
    return 0;
}

_Check_return_ XFLOAT CUIElement::GetActualHeight()
{
    return 0;
}

_Check_return_ HRESULT CUIElement::GetTightGlobalBounds(
        _Out_ XRECTF_RB* pBounds,
        _In_ bool ignoreClipping,
        _In_ bool useTargetInformation
        )
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CUIElement::GetTightInnerBounds(
    _Out_ XRECTF_RB* pBounds
    )
{
    return E_NOTIMPL;
}

void CUIElement::EvaluateIsRightToLeft()
{
}

bool CUIElement::IsRightToLeft()
{
    return false;
}

_Check_return_ HRESULT CUIElement::UpdateImplicitStyle(
    _In_opt_ CStyle *pOldStyle,
    _In_opt_ CStyle *pNewStyle,
    bool bForceUpdate,
    bool bUpdateChildren,
    bool isLeavingParentStyle
    )
{
    return E_NOTIMPL;
}

CUIElement* CUIElement::GetFirstChild()
{
    return nullptr;
}

void CUIElement::Shutdown()
{
}

void CUIElement::GetChildrenInRenderOrder(
    _Outptr_opt_result_buffer_(*puiChildCount) CUIElement ***pppUIElements,
    _Out_ XUINT32 *puiChildCount
    )
{
}

_Check_return_ HRESULT CUIElement::RecursiveInvalidateFontSize()
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CUIElement::InvalidateFontSize()
{
    return E_NOTIMPL;
}

void CUIElement::RecursiveInvalidateMeasure()
{
}

void CUIElement::OnChildDesiredSizeChanged(_In_ CUIElement* pElement)
{
}

_Check_return_ HRESULT CUIElement::UpdateLayoutClip(bool forceClipToRenderSize)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CUIElement::MeasureCore(XSIZEF availableSize, _Out_ XSIZEF& desiredSize)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CUIElement::ArrangeCore(XRECTF finalRect)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CUIElement::EffectiveViewportWalkCore(
    _In_ std::vector<TransformToPreviousViewport>& transformsToViewports,
    _In_ std::vector<UnidimensionalViewportInformation>& horizontalViewports,
    _In_ std::vector<UnidimensionalViewportInformation>& verticalViewports,
    _Out_ bool& addedViewports)
{
    addedViewports = false;
    return S_OK;
}

_Check_return_ HRESULT CUIElement::ComputeEffectiveViewportChangedEventArgsAndNotifyLayoutManager(
    _In_ std::vector<TransformToPreviousViewport>& transformsToViewports,
    _In_ const std::vector<UnidimensionalViewportInformation>& horizontalViewports,
    _In_ const std::vector<UnidimensionalViewportInformation>& verticalViewports)
{
    return S_OK;
}

void CUIElement::NotifyParentChange(
    _In_ CDependencyObject *pNewParent,
    _In_ RENDERCHANGEDPFN pfnNewParentRenderChangedHandler
    )
{
}

bool CUIElement::ReferenceTrackerWalkCore(
    _In_ DirectUI::EReferenceTrackerWalkType walkType,
    _In_ bool isRoot,
    _In_ bool shouldWalkPeer
    )
{
    return true;
}

void CUIElement::NWPropagateDirtyFlag(
    DirtyFlags flags
    )
{
}

void CUIElement::EnsureContentRenderDataVirtual(
    RenderWalkType oldType,
    RenderWalkType newType
    )
{
}

_Check_return_ HRESULT CUIElement::PreChildrenPrintVirtual(
    _In_ const SharedRenderParams& sharedPrintParams,
    _In_ const D2DPrecomputeParams& cp,
    _In_ const D2DRenderParams& printParams
    )
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CUIElement::PrintChildren(
    _In_ const SharedRenderParams& sharedPrintParams,
    _In_ const D2DPrecomputeParams& cp,
    _In_ const D2DRenderParams& printParams
    )
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CUIElement::SetRequiresComposition(
    CompositionRequirement compositionReason,
    IndependentAnimationType detailedAnimationReason
    ) noexcept
{
    return E_NOTIMPL;
}

void CUIElement::UnsetRequiresComposition(
    CompositionRequirement compositionReason,
    IndependentAnimationType detailedAnimationReason
    ) noexcept
{
}

void CUIElement::ClearPCRenderData()
{
}

void CUIElement::GetIndependentlyAnimatedBrushes(
    _Outptr_result_maybenull_ CSolidColorBrush **ppFillBrush,
    _Outptr_result_maybenull_ CSolidColorBrush **ppStrokeBrush
    )
{
}

void CUIElement::CleanupDeviceRelatedResourcesRecursive(_In_ bool cleanupDComp)
{
}

void CUIElement::LeavePCSceneSubgraph()
{
}

_Check_return_ HRESULT CUIElement::BoundsTestInternal(
    _In_ const XPOINTF& target,
    _In_ CBoundedHitTestVisitor* pCallback,
    _In_opt_ const HitTestParams *hitTestParams,
    _In_ bool canHitDisabledElements,
    _In_ bool isXamlDiagHitTestMode,
    _Out_opt_ BoundsWalkHitResult* pResult
    )
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CUIElement::BoundsTestInternal(
    _In_ const HitTestPolygon& target,
    _In_ CBoundedHitTestVisitor* pCallback,
    _In_opt_ const HitTestParams *hitTestParams,
    _In_ bool canHitDisabledElements,
    _In_ bool isXamlDiagHitTestMode,
    _Out_opt_ BoundsWalkHitResult* pResult
    )
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CUIElement::GenerateContentBounds(
    _Out_ XRECTF_RB* pBounds
    )
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CUIElement::GenerateChildOuterBounds(
    _In_opt_ HitTestParams *hitTestParams,
    _Out_ XRECTF_RB* pBounds
    )
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CUIElement::BoundsTestChildren(
    _In_ const XPOINTF& target,
    _In_ CBoundedHitTestVisitor* pCallback,
    _In_opt_ const HitTestParams *hitTestParams,
    _In_ bool canHitDisabledElements,
    _In_ bool isXamlDiagHitTestMode,
    _Out_opt_ BoundsWalkHitResult* pResult
    )
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CUIElement::BoundsTestChildren(
    _In_ const HitTestPolygon& target,
    _In_ CBoundedHitTestVisitor* pCallback,
    _In_opt_ const HitTestParams *hitTestParams,
    _In_ bool canHitDisabledElements,
    _In_ bool isXamlDiagHitTestMode,
    _Out_opt_ BoundsWalkHitResult* pResult
    )
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CUIElement::HitTestLocalInternal(
    _In_ const XPOINTF& target,
    _Out_ bool* pHit
    )
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CUIElement::HitTestLocalInternal(
    _In_ const HitTestPolygon& target,
    _Out_ bool* pHit
    )
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CUIElement::GetBoundsForImageBrushVirtual(
    _In_ const CImageBrush *pImageBrush,
    _Out_ XRECTF *pBounds
    )
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CUIElement::OnAssociationFailure()
{
    return E_NOTIMPL;
}

void CUIElement::SetHWRealizationCache(_In_opt_ HWRealization *pNewRenderingCache)
{
}

void CUIElement::GetStoredHandOffVisual(_Outptr_result_maybenull_ WUComp::IVisual** ppHandOffVisual)
{
}

DCompTreeHost* CUIElement::GetDCompTreeHost() const
{
    return nullptr;
}

bool CUIElement::IsUsingHandOffVisual() const
{
    return false;
}

_Check_return_ HRESULT CUIElement::GetHandOffVisual(
    _Outptr_result_nullonfailure_ IUnknown** ppUnkHandOffVisual)
{
    *ppUnkHandOffVisual = nullptr;
    return E_NOTIMPL;
}

void CUIElement::LeavePCSceneRecursive()
{
}

_Ret_maybenull_ CUIElement* CUIElement::GetUIElementAdjustedParentInternal(
    bool publicParentsOnly, /* = TRUE */
    bool useRealParentForClosedParentedPopups /* = FALSE */
    )
{
    return nullptr;
}

_Check_return_ HRESULT CUIElement::OnKeyDown(_In_ CEventArgs* pEventArgs)
{
    return S_OK;
}

bool CUIElement::HasShownHiddenHandlers() const
{
    return false;
}

bool CUIElement::HasHiddenHandlers() const
{
    return false;
}

void CUIElement::FireShownHiddenEvent(KnownEventIndex eventIndex)
{
}

CTransform* CUIElement::GetRenderTransformLocal() const
{
    return nullptr;
}

XPOINTF CUIElement::GetRenderTransformOrigin(void) const
{
    return {0, 0};
}

bool CUIElement::HasTransitionTarget(void) const
{
    return false;
}

_Check_return_ HRESULT CUIElement::GetHandInVisual(_Outptr_result_maybenull_ WUComp::IVisual * *)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CUIElement::GetElementSizeForProjection(_Out_ struct XSIZEF *)
{
    return E_NOTIMPL;
}

bool CUIElement::IsLocalClipIndependentlyAnimating(void)const
{
    return false;
}

bool CUIElement::IsTransitionClipIndependentlyAnimating(void)const
{
    return false;
}

bool CUIElement::IsManipulatable(void)const
{
    return false;
}

bool CUIElement::IsUsingHandInVisual(void)const
{
    return false;
}

bool CUIElement::RequiresHitTestInvisibleCompNode(void) const
{
    return false;
}

void CUIElement::GetViewportInteraction(_In_ DCompTreeHost*, _Out_ IInspectable** interaction)
{
    *interaction = nullptr;
}

void CUIElement::StoreLayoutCycleWarningContext(size_t framesToSkip)
{
}

void CUIElement::StoreLayoutCycleWarningContext(_In_opt_ CLayoutManager* layoutManager, size_t framesToSkip)
{
}

xref_ptr<CFlyoutBase> CUIElement::GetContextFlyout() const
{
    return nullptr;
}

CFrameworkElement* CTextElement::GetContainingFrameworkElement() { return nullptr; }

CPopupRoot* CPopup::GetAssociatedPopupRootNoRef()
{
    return nullptr;
}

_Check_return_ HRESULT CPopup::SetRootVisualForWindowedPopupWindow(_In_ struct WUComp::IVisual *)
{
    return E_NOTIMPL;
}

CUIElement * CRootVisual::GetRootScrollViewerOrCanvas(void)
{
    return nullptr;
}

bool ShouldOverrideRenderOpacity(float,class CUIElement *)
{
    return false;
}

bool CPopupRoot::ComputeDepthInOpenPopups()
{
    return false;
}

CDependencyObject* CUIElement::GetRootOfPopupSubTree()
{
    return nullptr;
}

_Check_return_ HRESULT
CUIElement::UpdateFocusState(_In_ DirectUI::FocusState focusState)
{
    return E_NOTIMPL;
}

bool CUIElement::IsFocusable()
{
  return false;
}
