// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "MetadataAPI.h"
#include "Triggers.h"
#include "XamlLocalTransformBuilder.h"
#include "XamlTraceLogging.h"
#include <RuntimeEnabledFeatures.h>
#include <DependencyLocator.h>
#include <FrameworkTheming.h>
#include "Transform3D.h"
#include "HitTestParams.h"
#include <GeneralTransformHelper.h>
#include <DependencyObjectDCompRegistry.h>
#include "ICollectionChangeCallback.h"
#include "OcclusivityTester.h"
#include "HighContrastTracingHelper.h"
#include <GraphicsUtility.h>
#include "AutomationAnnotationCollection.h"
#include <HWCompNodeWinRT.h>
#include "HWRedirectedCompTreeNodeWinRT.h"
#include <DCompPropertyChangedListener.h>
#include "UIElement.h"
#include <ImplicitAnimations.h>
#include <MenuFlyout.h>
#include "ContextRequestedEventArgs.h"
#include <UIThreadScheduler.h>
#include <WindowsGraphicsDeviceManager.h>
#include <PixelFormat.h>
#include "DXamlCore.h"
#include <RootScale.h>

#include <AccessKeyInvokedEventArgs.h>
#include <AccessKeyShownEventArgs.h>
#include <AccessKeyHiddenEventArgs.h>
#include <BringIntoViewRequestedEventArgs.h>

#include "AKExport.h"

#include "ListViewBaseItemChrome.h"
#include "ThemeShadow.h"

#include "FocusProperties.h"
#include "ElementGestureTracker.h"
#include "theming\inc\Theme.h"
#include "KeyboardAcceleratorUtility.h"
#include <XamlOneCoreTransforms.h>
#include <SimpleProperties.h>
#include "UIElement_Partial.h"
#include "RectUtil.h"
#include "StableXbfIndexes.g.h"
#include "RuntimeProfiler.h"
#include "RuntimeProfiler_DynamicProfiler.h"
#include "XbfMetadataApi.h"
#include "LayoutCycleDebugSettings.h"
#include "XStringUtils.h"

#include <CValueBoxer.h>

using namespace DirectUI;
using namespace DCompHelpers;
using namespace Focus;
using namespace Theming;

// Uncomment to get generic UIElement debug traces
// #define UIE_DBG

// Uncomment to get DirectManipulation debug traces
// #define DMUIEv_DBG

// Uncomment for handoff visual debug traces
// #define HOVCTN_DBG

// Uncomment for handoff visual debug traces
// #define HOVUIE_DBG

// Uncomment for hand-in visual debug traces
// #define HIVUIE_DBG

void
PrimitiveCompositionPropertyData::Clear()
{
    ReleaseInterface(pHWRealizationCache);

    // Render data should already have been cleared. See ClearRenderData().
    ASSERT(!isInScene);

    ASSERT(pPreChildrenRenderData == NULL || pPreChildrenRenderData->empty());
    SAFE_DELETE(pPreChildrenRenderData);

    ASSERT(pPostChildrenRenderData == NULL || pPostChildrenRenderData->empty());
    SAFE_DELETE(pPostChildrenRenderData);
}

void
PrimitiveCompositionPropertyData::ClearInSceneData()
{
    wasRedirectedTransformAnimating = FALSE;
    pLastCompNodeInSubgraphNoRef = NULL;
    lastSpriteVisualInSubgraphNoRef = nullptr;
}

void PrimitiveCompositionPropertyData::ClearRenderData(_In_ CCoreServices* coreServices)
{
    ClearRenderDataList(pPreChildrenRenderData, coreServices);
    ClearRenderDataList(pPostChildrenRenderData, coreServices);

    isInScene = FALSE;
    ClearInSceneData();
}

void PrimitiveCompositionPropertyData::ClearRenderDataList(_In_ PCRenderDataList* pRenderData, _In_ CCoreServices* coreServices)
{
    if (pRenderData != nullptr)
    {
        PCRenderDataList::iterator end = pRenderData->end();
        for (PCRenderDataList::iterator it = pRenderData->begin(); it != end; ++it)
        {
            WUComp::IVisual* visual = *it;

            TraceDCompRemovePrimitiveInfo(reinterpret_cast<XUINT64>(visual));

            xref_ptr<WUComp::IContainerVisual> spParent;
            HRESULT hr = visual->get_Parent(spParent.ReleaseAndGetAddressOf());

            // For CoreWindow scenarios, the CompositionContent is also listening for the CoreWindow's closed event.
            // CompositionContent will get the notification first and close the entire visual tree, then Xaml will
            // exit its message loop and tear down the tree. Since CompositionContent already closed everything,
            // Xaml will get lots of RO_E_CLOSED errors. These are all safe to ignore. So tolerate RO_E_CLOSED if
            // we're also in the middle of tearing down the tree.
            if (FAILED(hr)
                && (hr != RO_E_CLOSED || !coreServices->IsTearingDownTree()))
            {
                IFCFAILFAST(hr);
            }

            // If we're doing synchronous tree operations and we remove a comp node from the tree, then we'll
            // unparent all direct SpriteVisual children of that comp node from the comp node tree (see
            // HWCompTreeNodeWinRT::RemoveSynchronous). Those SpriteVisuals will remain unparented until the
            // next render walk, when they get reparented to some ancestor comp node. If their UIElements are
            // removed from the tree in the meantime, then the SpriteVisuals will not have a parent when we
            // go to clean them up.
            if (spParent)
            {
                xref_ptr<WUComp::IVisualCollection> spChildren;
                IFCFAILFAST(spParent->get_Children(spChildren.ReleaseAndGetAddressOf()));
                IFCFAILFAST(spChildren->Remove(visual));
            }

            ReleaseInterface(visual);
        }

        pRenderData->clear();
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      ctor
//
//------------------------------------------------------------------------
D2DPrecomputeParams::D2DPrecomputeParams(
    _In_ IPALAcceleratedGraphicsFactory *pFactory,
    _In_ IDirtyRegionAccumulator *pDirtyRegion
    )
    : m_pFactoryNoRef(pFactory)
    , m_fShouldAddDirtyRegion(TRUE)
{
    XCP_WEAK(&m_pFactoryNoRef);

    EmptyRectF(&m_cacheSize);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Resets the aggregated size of the cached surface.
//
//------------------------------------------------------------------------
void
D2DPrecomputeParams::Reset()
{
    EmptyRectF(&m_cacheSize);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Unions a rect with the size of the current cache. The rect passed
//      in should already be in the coordinate space of the cached surface
//      and does not need to be transformed further.
//
//------------------------------------------------------------------------
void
D2DPrecomputeParams::AddSurfaceSpaceRect(const XRECTF_RB& rect)
{
    UnionRectF(&m_cacheSize, &rect);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the size of the cached surface.
//
//------------------------------------------------------------------------
void
D2DPrecomputeParams::GetCacheSize(
    _Out_ XINT32& width,
    _Out_ XINT32& height
) const
{
    //
    // UnionRectF checks for empty rects, so we are guaranteed that m_cacheSize is
    // well formed.
    //
    ASSERT(m_cacheSize.right >= m_cacheSize.left && m_cacheSize.bottom >= m_cacheSize.top);

    //
    // No need to account for renderAtScale here because NWAddCachedContents included it in
    // the transform when calling PreRender.
    //
    // Since the surface offset is always an integer, add a pixel to the top/left to account
    // for content drawn at a decimal offset. Since the surface's width & height are always
    // integers, add another pixel to round up.
    //
    XFLOAT rWidth = m_cacheSize.right - m_cacheSize.left + 2;
    XFLOAT rHeight = m_cacheSize.bottom - m_cacheSize.top + 2;

    if (rWidth < XINT32_MAX)
    {
        width = static_cast<XINT32>(floorf(rWidth));
    }
    else
    {
        width = XINT32_MAX;
    }

    if (rHeight < XINT32_MAX)
    {
        height = static_cast<XINT32>(floorf(rHeight));
    }
    else
    {
        height = XINT32_MAX;
    }

    //
    // The bounds might 2x2 if nothing draws into this cache. Ideally we would skip rendering
    // in this case, but the render walk still needs to happen to pick up the cache path for
    // rendering the next cache, so we still need to set up a render target. No actual surface
    // will be created, because later CUIElement::D2DRasterizeCacheCreateNode will detect that
    // the render target is empty.
    // TODO: D2D: When PC is integrated and CC is taken out, skip rendering for 2x2 bounds
    //
    // The bounds might be 2x2 (as opposed to will be 2x2) because the element doesn't track
    // which contents it draws into which cache. It just uses a single set of bounds for all
    // its contents. So if it draws nothing into this cache and something into a different
    // implicit cache, this cache will still have bounds bigger than 2x2 even though it's
    // empty.
    // TODO: D2D: No longer applicable when PC is integrated and implicit caches are taken out
    //
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the offset of the cached surface.
//
//------------------------------------------------------------------------
void
D2DPrecomputeParams::GetCacheOffsets(
    _Out_ XINT32& offsetX,
    _Out_ XINT32& offsetY
) const
{
    //
    // No need to account for renderAtScale here because NWAddCachedContents included it in
    // the transform when calling PreRender.
    //
    offsetX = static_cast<XINT32>(floorf(m_cacheSize.left));
    offsetY = static_cast<XINT32>(floorf(m_cacheSize.top));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      ctor
//
//------------------------------------------------------------------------
NWRenderParams::NWRenderParams()
    : fTransformDirty(FALSE)
    , fForceOpaque(FALSE)
    , fCachedCompositionEnabled(FALSE)
    , fRenderChildren(TRUE)
    , fCacheEdgeVectors(TRUE)
{
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      ctor
//
//------------------------------------------------------------------------
PrintParams::PrintParams()
    : m_fForceVector(FALSE)
    , m_fIsPrintTarget(NULL)
{
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      ctor
//
//------------------------------------------------------------------------
D2DRenderParams::D2DRenderParams(
    _In_ IPALAcceleratedRenderTarget *pRenderTarget,
    _In_ IDirtyRegionQuery *pDirtyRegion,
    bool fIsPrintTarget
    )
    : m_forceOpaque(TRUE)
    , m_pRenderTargetNoRef(pRenderTarget)
    , m_renderFill(true)
    , m_renderStroke(true)
{
    XCP_WEAK(&m_pRenderTargetNoRef);

    m_fIsPrintTarget = fIsPrintTarget;
}

// Destructor for UI element object.
CUIElement::~CUIElement()
{
    auto core = GetContext();

    if (GetDCompObjectRegistry() != nullptr)
    {
        GetDCompObjectRegistry()->UnregisterObject(this);
    }

    IGNOREHR(RemoveAllEventListeners(false /* leaveUIEHiddenEventListenersAttached */));

    if (m_pEventList)
    {
        // Remove this UIElement's entry from the EventManager's EventRequestMap.  Otherwise,
        // we will leak that entry until the app shuts down.
        if (core)
        {
            CEventManager* pEventManager = core->GetEventManager();

            if (pEventManager)
            {
                IGNOREHR(pEventManager->RemoveObject(this));
            }
        }

        m_pEventList->Clean();
        delete m_pEventList;
    }

    if (m_pChildren)
    {
        IGNOREHR(m_pChildren->RemoveAllElements(FALSE));
        IGNOREHR(m_pChildren->SetOwner(NULL));
    }
    ReleaseInterface(m_pChildren);

    if (m_pAP)
    {
        m_pAP->InvalidateOwner();
        ReleaseInterface(m_pAP);
    }

    // there are situations where we are registered in the LayoutManager to do a transition
    // and that transition has not started yet: this occurs when our parent is being removed
    // immediately after this element has registered a transition
    // That situation does not exist often, only when there is no storyboard yet but there is a transition
    IGNOREHR(DeleteLayoutTransitionStorage());

    // All LayoutTransitionElements should be unhooked before the UIElement is released.
    ASSERT(m_pLayoutTransitionRenderers == NULL || m_pLayoutTransitionRenderers->empty());
    SAFE_DELETE(m_pLayoutTransitionRenderers);

    m_propertyRenderData.Reset(RWT_None);
    m_contentRenderData.Reset(RWT_None);

    // NOTE: We don't remove the composition peer from the tree here, that should already have happened.
    //           The composition peer should almost always be NULL in the destructor, this just prevents leaks in error conditions.
    ReleaseInterface(m_pCompositionPeer);

    ReleaseInterface(m_pTextFormatting);

    // Cleanup any Composition objects attached to this element
    if (core->NWGetWindowRenderTarget() != nullptr && GetDCompTreeHost() != nullptr)
    {
        if (HasImplicitShowAnimation())
        {
            ASSERT(!IsImplicitAnimationPlaying(ImplicitAnimationType::Show));
            CleanupImplicitAnimationInfo(ImplicitAnimationType::Show);
        }
        if (HasImplicitHideAnimation())
        {
            ASSERT(!IsImplicitAnimationPlaying(ImplicitAnimationType::Hide));
            CleanupImplicitAnimationInfo(ImplicitAnimationType::Hide);
        }

        DCompTreeHost* dcompTreeHost = GetDCompTreeHost();
        dcompTreeHost->SetTrackEffectiveVisibility(this, false);
        dcompTreeHost->SetTrackKeepVisible(this, false);
        dcompTreeHost->CleanupEffectiveVisibilityTracker(this);

        auto& lightTargetIdMap = dcompTreeHost->GetXamlLightTargetIdMap();
        lightTargetIdMap.RemoveTarget(this);

        dcompTreeHost->ReleasePointerSourceForElement(this);

        dcompTreeHost->GetWUCBrushManager()->CleanUpBrushTransitions(*this);
    }

    // Any outstanding facade animation will have a NoRef pointer back to this element in its completed handler.
    // Those pointers are about to become dangling, so disconnect all completed handlers.
    StopAllFacadeAnimations();

    ForceCloseHandInAndHandOffVisuals();

    core->GetFacadeStorage().OnDODestroyed(this);

    SimpleProperty::Property::NotifyDestroyed<CUIElement>(this);

    TraceElementDestroyedInfo((XUINT64)this);
}

// Force close on any hand-in or handoff visuals. This causes them to be disconnected from any other visuals and
// ensures we are not holding on to graphics resources, even if app is still holding its reference to the visual.
void CUIElement::ForceCloseHandInAndHandOffVisuals()
{
    if (m_hasEverStoredHandoffOrHandinVisual)
    {
        // In multi-view apps, it's possible that we get here via CCoreServices destructor, at a point when
        // DCompTreeHost has already been cleaned up. For example, CTimeManager might be holding on to
        // animation targets outside the visual tree, and it is cleaned up later than DCompTreeHost in core shutdown.
        // In this case, the compositor is also about to be destroyed, which will clean up any hand-ins/handoffs,
        // so we don't need to take any action here.
        if ((GetContext()->NWGetWindowRenderTarget() == nullptr) || (GetDCompTreeHost() == nullptr)) { return; }

        auto& handOffVisualDataMap = GetDCompTreeHost()->GetHandOffVisualDataMap();
        auto& handInVisualsMap = GetDCompTreeHost()->GetHandInVisualsMap();

        auto handOffPair = handOffVisualDataMap.find(this);
        if (handOffPair != handOffVisualDataMap.end())
        {
            Microsoft::WRL::ComPtr<WUComp::IVisual> handOffVisual = handOffPair->second.handOffVisual;

            if (handOffVisual != nullptr)
            {
                // DComp takes a reference on the listener via registration which we need to explicitly release via unregistration.
                Microsoft::WRL::ComPtr<DCompPropertyChangedListener> listener = handOffPair->second.dcompPropertyChangedListener;
                if (listener)
                {
                    listener->DetachFromHandOffVisual();
                    listener->DetachFromPrependVisual();
                    listener->DetachFromWUCClip();
                }
            }

            handOffVisualDataMap.erase(handOffPair);
        }

        // Hand-in visual - use IClosable::Close to disconnect / release resources
        auto handInPair = handInVisualsMap.find(this);
        if (handInPair != handInVisualsMap.end())
        {
            Microsoft::WRL::ComPtr<wf::IClosable> handInVisualAsClosable;

            if (handInPair->second != nullptr)
            {
                // Unlike the case above for handoffs, the QI here should always succeed since IXamlDCompInteropInteropPrivate does not allow creation of hand-in visuals.
                IFCFAILFAST(handInPair->second.As(&handInVisualAsClosable));
                IGNOREHR(handInVisualAsClosable->Close());
                handInPair->second.Reset();
            }
            handInVisualsMap.erase(handInPair);
        }
    }
}

//  Performs any lazy computation required for TransitionTarget. Only allowed during DynamicTimeline OnBegin.
_Check_return_ HRESULT CUIElement::GetValue(_In_ const CDependencyProperty* dp, _Out_ CValue* value)
{
    switch (dp->GetIndex())
    {
        case KnownPropertyIndex::UIElement_TransitionTarget:
            {
                auto core = GetContext();

                if (core->IsAllowingTransitionTargetCreations() && IsPropertyDefaultByIndex(KnownPropertyIndex::UIElement_TransitionTarget))
                {
                    IFC_RETURN(dp->CreateDefaultValueObject(core, value));
                    IFC_RETURN(SetValue(SetValueParams(dp, *value)));
                }
            }
            break;
    }

   IFC_RETURN(CDependencyObject::GetValue(dp, value));

   return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Override to base SetValue to allow adding a child visual
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElement::SetValue(_In_ const SetValueParams& args)
{
    HRESULT hr = S_OK;
    auto core = GetContext();

// If the incoming property has no index than it is an object.  See if it is of
// the correct type to add it to our list of children.

    if ((args.m_pDP == nullptr)
        || (args.m_pDP->GetIndex() == KnownPropertyIndex::Panel_Children))
    {
        ASSERT(args.m_pDP, L"The XAML Parser no longer adds children via SetValue(null, child).");

        if ((args.m_value.GetType() != valueObject) ||
            !args.m_value.AsObject()->OfTypeByIndex<KnownTypeIndex::UIElement>())
        {
            IFC(E_INVALIDARG);
        }

        IFC(AddChild((CUIElement *)args.m_value.AsObject()));
    }
    else
    {
        if (TraceLoggingProviderEnabled(g_hTraceProvider, WINEVENT_LEVEL_LOG_ALWAYS, MICROSOFT_KEYWORD_MEASURES))
        {
            auto theme = core->GetFrameworkTheming()->GetHighContrastTheme();
            if (theme == Theme::HighContrastWhite || theme == Theme::HighContrastBlack)
            {
                TraceLoggingHCColor(this, theme, args);
            }
        }

        // If layout cycle tracking is active (checked this way for optimal perf), and this
        // element live in the tree (setting values on non-live elements won't trigger layout),
        // and this property affects layout, then log this SetValue.
        if (core->IsLayoutCycleTrackingActive() && IsActive() && (args.m_pDP->AffectsMeasure() || args.m_pDP->AffectsArrange()))
        {
            // IsLayoutCycleTrackingActive only checks if layout cycle tracking is active somewhere,
            // but we need to check if it is active for the LayoutManager for this element.
            auto layoutManager = VisualTree::GetLayoutManagerForElement(this);
            if (layoutManager && layoutManager->StoreLayoutCycleWarningContexts())
            {
                std::wstring setValueInfo(L"SetValue(");
                xstring_ptr propName = args.m_pDP->GetName();
                setValueInfo.append(propName.IsNullOrEmpty() ? std::to_wstring((int)args.m_pDP->GetIndex()) : propName.GetBuffer());

                std::wstring newPropertyValue;
                switch (args.m_value.GetType())
                {
                case valueDouble:
                    newPropertyValue = std::to_wstring(args.m_value.AsDouble());
                    break;
                case valueThickness:
                    {
                        auto thicknessValue = args.m_value.AsThickness();
                        auto strValue = StringCchPrintfWWrapper(L"%g,%g,%g,%g",
                            thicknessValue->left,
                            thicknessValue->top,
                            thicknessValue->right,
                            thicknessValue->bottom);
                        newPropertyValue = strValue.GetBuffer();
                        break;
                    }
                case valueEnum:
                    newPropertyValue = std::to_wstring(args.m_value.AsEnum());
                    break;
                }
                if (!newPropertyValue.empty())
                {
                    setValueInfo.append(L"=");
                    setValueInfo.append(newPropertyValue);
                }
                setValueInfo.append(L")");

                std::vector<std::wstring> warningInfo;
                warningInfo.push_back(std::move(setValueInfo));
                StoreLayoutCycleWarningContext(warningInfo, layoutManager);

                if (LayoutCycleDebugSettings::ShouldDebugBreak(DirectUI::LayoutCycleDebugBreakLevel::Level1))
                {
                    __debugbreak();
                }
            }
        }

        bool wasOpacityImplicitAnimationStarted = false;
        switch (args.m_pDP->GetIndex())
        {
            case KnownPropertyIndex::UIElement_ManipulationMode:
            {
                DirectUI::ManipulationModes manipulationMode;
                if (args.m_value.IsEnum())
                {
                    manipulationMode = static_cast<DirectUI::ManipulationModes>(args.m_value.AsEnum());
                }
                else if (valueObject == args.m_value.GetType())
                {
                    if (args.m_value.AsObject()->GetClassInformation()->IsEnum())
                    {
                        const auto pEnumerated = static_cast<const CEnumerated*>(args.m_value.AsObject());
                        manipulationMode = static_cast<DirectUI::ManipulationModes>(pEnumerated->m_nValue);
                    }
                    else
                    {
                        IFC(E_INVALIDARG);
                    }
                }
                else
                {
                    IFC(E_INVALIDARG);
                }

                if (GetIsManipulationModeInvalid(manipulationMode))
                {
                    // In Win8, ManipulationMode can't contain flags from both the custom and system sections.
                    // Starting with WinBlue, System can be combined with TranslateX, TranslateY, TranslateRailsX, TranslateRailsY and DirectUI::ManipulationModes::TranslateInertia
                    // DirectUI::ManipulationModes::TranslateInertia cannot be the only flag combined with System though.
                    // Starting in TH2, System can also be combined with Scale and ScaleInertia. ScaleInertia cannot be the only flag combined with System though.
                    // System cannot be combined with both translation and scaling flags either.
                    IErrorService *pErrorService = NULL;
                    IFC(core->getErrorService(&pErrorService));
                    HRESULT hrToOriginate = E_INVALIDARG;
                    if (pErrorService)
                    {
                        IFC(pErrorService->ReportGenericError(
                            hrToOriginate,
                            InitializeError,
                            AG_E_INVALID_MIXED_MANIPULATION_MODE,
                            TRUE, /* bRecoverable */
                            0,    /* uLineNumber */
                            0,    /* uCharPosition */
                            NULL, /* ppParam */
                            0,    /* cParams */
                            NULL, /* pErrorEventArgs */
                            NULL  /* pSender */));
                    }

                    IFC(hrToOriginate);
                }
                break;
            }
            case KnownPropertyIndex::RelativePanel_LeftOf:
            case KnownPropertyIndex::RelativePanel_Above:
            case KnownPropertyIndex::RelativePanel_RightOf:
            case KnownPropertyIndex::RelativePanel_Below:
            case KnownPropertyIndex::RelativePanel_AlignLeftWith:
            case KnownPropertyIndex::RelativePanel_AlignTopWith:
            case KnownPropertyIndex::RelativePanel_AlignRightWith:
            case KnownPropertyIndex::RelativePanel_AlignBottomWith:
            case KnownPropertyIndex::RelativePanel_AlignHorizontalCenterWith:
            case KnownPropertyIndex::RelativePanel_AlignVerticalCenterWith:
            {
                // These properties can be set to null.
                bool valueIsNull = (args.m_value.GetType() == valueNull);

                // These properties can be set to valueStrings, too.
                bool valueIsValidString = (args.m_value.GetType() == valueString);

                // These properties can be set to valueObjects, but only if they
                // are of type UIElement or String.
                bool valueIsValidObject =
                    args.m_value.AsObject() &&
                    (args.m_value.AsObject()->OfTypeByIndex<KnownTypeIndex::UIElement>() || args.m_value.AsObject()->OfTypeByIndex<KnownTypeIndex::String>());

                if (!(valueIsNull || valueIsValidString || valueIsValidObject))
                {
                    IFC(SetAndOriginateError(E_NER_INVALID_OPERATION, RuntimeError, AG_E_RELATIVEPANEL_INVALID_TYPE));
                }

                break;
            }

            case KnownPropertyIndex::UIElement_Shadow:
            {
                bool isOpenPopup = OfTypeByIndex<KnownTypeIndex::Popup>() && static_cast<CPopup*>(this)->IsOpen();
                bool shadowChanged = false;

                if (!CThemeShadow::IsDropShadowMode())
                {
                    // Explicitly check the shadow for ancestor elements set as Receivers. We can't rely on Enter here, because
                    // ThemeShadows can be shared, and the new shadow could already be in the tree elsewhere.
                    CThemeShadow* themeShadow = checked_cast<CThemeShadow>(args.m_value.AsObject());
                    if (themeShadow != nullptr)
                    {
                        IFC(themeShadow->CheckForAncestorReceivers(this));
                    }

                    // Notes on creating a CompNode for projected shadows:
                    // We simplify Caster compnode requirement management needed to support Global vs Custom shadows as follows:
                    // (1) For global scene, Popup.Child is the caster - a compnode is already guaranteed by CompositionRequirement::ProjectedShadowDefaultReceiver (see CPopup::SetChild/RemoveChild)
                    // (2) For custom scene, element itself is the caster - gets comp node below in OnSetShadowValue().
                    //
                    // An element can transition between (1) and (2) by changing its Receivers property.
                    // To reduce code complexity around this, we ensure a compnode for both scenarios.
                    shadowChanged = OnSetShadowValue(args);
                    //If the root is canvas, we force it to have a compNode
                    //This code is specific for the case that ThemeShadow is
                    //set after elements entering the tree
                    if (IsActive() || isOpenPopup)
                    {
                        EnsureRootCanvasCompNode();
                    }
                }
                else
                {
                    // Notes in creating a CompNode for drop shadows:
                    // We largely borrow the same logic as for projected shadows:
                    // (1) Popup.Child implicity gets a CompNode for shadows on Popup via CompositionRequirement::ProjectedShadowDefaultReceiver
                    // (2) All other elements explicitly get a CompNode below in OnSetShadowValue().
                    shadowChanged = OnSetShadowValue(args);
                    // Some elements will be given a special name in order to set their
                    // fade-in flag to true.
                    if (!ShouldFadeInDropShadow())
                    {
                        auto name = m_strName.GetBuffer();
                        if (name)
                        {
                            if (0 == wcscmp(name, L"DropShadowFadeInTarget"))
                            {
                                SetShouldFadeInDropShadow(true);
                            }
                        }
                    }
                }

                if (shadowChanged)
                {
                    // If this is a popup, dirty the child since it is the actual caster element
                    if (isOpenPopup)
                    {
                        CUIElement* child = static_cast<CPopup*>(this)->m_pChild;
                        if (child)
                        {
                            NWSetContentDirty(child, DirtyFlags::Render);
                        }
                    }
                }

                break;
            }

            case KnownPropertyIndex::UIElement_Opacity:
            {
                wasOpacityImplicitAnimationStarted = BeforeSetLocalOpacity(static_cast<float>(args.m_value.As<valueDouble>()));
                break;
            }
        }

        DirectUI::Visibility oldVisibility = GetVisibility();

        // The call to SetValue should come after the following switch. Reorder after SL3!
        hr = CDependencyObject::SetValue(args);

        switch (args.m_pDP->GetIndex())
        {
        case KnownPropertyIndex::UIElement_Transitions:
            {
                if (args.m_value.AsObject() || args.m_value.AsIInspectable())
                {
                    IFC(CUIElement::EnsureLayoutTransitionStorage(this, NULL, TRUE));
                }
                // lifetime of TransitionCollection is manually handled since it has no parent
                IFC(SetPeerReferenceToProperty(args.m_pDP, args.m_value));
                break;
            }
        case KnownPropertyIndex::UIElement_CanDrag:
            {
                IFC(FxCallbacks::UIElement_NotifyCanDragChanged(this, args.m_value.AsBool()));
                break;
            }
        case KnownPropertyIndex::UIElement_Visibility:
            {
                if (oldVisibility != GetVisibility() && IsActive())
                {
                    // Set a flag on the core indicating that Visibility property has changed
                    // somewhere in the visual tree.
                    core->SetVisibilityToggled(TRUE);

                    const auto contentRoot = VisualTree::GetContentRootForElement(this);
                    const auto& akExport = contentRoot->GetAKExport();

                    if (akExport.IsActive())
                    {
                        IFC(akExport.OnVisibilityChanged(this, GetVisibility()));
                    }

                    if (oldVisibility == DirectUI::Visibility::Visible)
                    {
                        // Discard the potential rejection viewports within this collapsed element's subtree
                        IFC(DiscardRejectionViewportsInSubTree());
                    }

                    if (oldVisibility == DirectUI::Visibility::Visible)
                    {
                        // Process the PointerExited event if the pointer entered element isn't visible.
                        IFC(ProcessPointerExitedEventByPointerEnteredElementStateChange());
                    }
                }
                break;
            }
        case KnownPropertyIndex::Canvas_Top:
        case KnownPropertyIndex::Canvas_Left:
            {
                OnActualOffsetChanged();
                break;
            }
        case KnownPropertyIndex::FrameworkElement_Width:
        case KnownPropertyIndex::FrameworkElement_Height:
        case KnownPropertyIndex::FrameworkElement_MinWidth:
        case KnownPropertyIndex::FrameworkElement_MinHeight:
        case KnownPropertyIndex::FrameworkElement_MaxWidth:
        case KnownPropertyIndex::FrameworkElement_MaxHeight:
            {
                if (OfTypeByIndex<KnownTypeIndex::Canvas>())
                {
                    OnActualSizeChanged();
                }
                break;
            }
        case KnownPropertyIndex::UIElement_ManipulationMode:
        case KnownPropertyIndex::UIElement_IsTapEnabled:
        case KnownPropertyIndex::UIElement_IsDoubleTapEnabled:
        case KnownPropertyIndex::UIElement_IsRightTapEnabled:
        case KnownPropertyIndex::UIElement_IsHoldingEnabled:
            {
                if (!IsInteractionEngineRequired(true /*ignoreActiveState*/))
                {
                    // Destroy the specified touch interaction engine on this element
                    xref_ptr<CInputServices> inputServices(GetContext()->GetInputServices());
                    if (inputServices)
                    {
                        inputServices->DestroyInteractionEngine(this);
                    }
                }
                break;
            }
        case KnownPropertyIndex::UIElement_Opacity:
            {
                OnSetLocalOpacity(wasOpacityImplicitAnimationStarted);
                break;
            }
        case KnownPropertyIndex::UIElement_CanBeScrollAnchor:
            {
                bool isAnchorCandidate = args.m_value.AsBool();
                if (isAnchorCandidate)
                {
                    IFC(UpdateAnchorCandidateOnParentScrollProvider(true /* add */));
                }
                else
                {
                    IFC(UpdateAnchorCandidateOnParentScrollProvider(false /* add */));
                }

                break;
            }
        case KnownPropertyIndex::UIElement_ProtectedCursor:
            {
                CValue newValue = args.m_value;
                bool hasNewCursor = !!(newValue.AsObject());

                SetIsProtectedCursorSet(hasNewCursor);

                // update the cursor
                xref_ptr<CInputServices> inputServices(GetContext()->GetInputServices());
                if (inputServices && IsActive())
                {
                    inputServices->UpdateCursor(this);
                }

                break;
            }
        }
        IFC(hr);
    }

Cleanup:
    return hr;
}

bool CUIElement::OnSetShadowValue(_In_ const SetValueParams& args)
{
    CValue oldValue;
    IFCFAILFAST(GetValueByIndex(KnownPropertyIndex::UIElement_Shadow, &oldValue));
    bool hasShadowOld = !!(oldValue.AsObject());
    bool hasShadowNew = !!(args.m_value.AsObject());

    // An element can transition between (1) and (2) by changing its Receivers property.
    // To reduce code complexity around this, we ensure a compnode for both scenarios.
    bool shadowChanged = hasShadowOld != hasShadowNew;
    if (shadowChanged)
    {
        if (hasShadowNew)
        {
            SetRequiresComposition(CompositionRequirement::ShadowCaster, IndependentAnimationType::None);
        }
        else
        {
            UnsetRequiresComposition(CompositionRequirement::ShadowCaster, IndependentAnimationType::None);
        }
    }

    return shadowChanged;
}

bool CUIElement::IsScrollAnchorProvider(_In_ CUIElement* element)
{
    // We want to know if some element is an IScrollAnchorProvider without having to do a QI which
    // can be expensive when walking the tree. We can use ScrollPort registration here but unfortunately
    // ScrollContentPresenter is the scroll port and ScrollViewer is the anchor provider. This check ensures
    // that we avoid QI'ing unnecessarily on ScrollContentPresenter.
    return element->GetTypeIndex() == KnownTypeIndex::ScrollViewer ||
           (element->IsScroller() && element->GetTypeIndex() != KnownTypeIndex::ScrollContentPresenter);
}

_Check_return_ HRESULT CUIElement::UpdateAnchorCandidateOnParentScrollProvider(bool add)
{
    // Walk up the parent chain to find first IScrollAnchorProvider and register on it.
    CUIElement *parent = GetUIElementParentInternal();
    ctl::ComPtr<xaml_controls::IScrollAnchorProvider> anchorProvider;

    while (parent)
    {
        // Avoid doing a QI on every single parent. We can restrict it to just elements that are registered
        // as scroll port. Turns out ScrollViewer is not a scrollport but ScrollContentPresenter is. This
        // check is to avoid unnecessary QIs. For someone implementing IScrollAnchorProvider this adds the
        // requirement that they need to be a scrollport.
        if (IsScrollAnchorProvider(parent))
        {
            auto parentPeer = parent->GetDXamlPeer();
            if (parentPeer != nullptr && SUCCEEDED(reinterpret_cast<IUnknown*>(parentPeer)->QueryInterface<xaml_controls::IScrollAnchorProvider>(anchorProvider.GetAddressOf())))
            {
                auto peer = GetDXamlPeer();
                if (peer != nullptr)
                {
                    auto thisAsIUI = static_cast<xaml::IUIElement*>(static_cast<DirectUI::UIElement*>(peer));
                    if (add)
                    {
                        IFC_RETURN(anchorProvider->RegisterAnchorCandidate(thisAsIUI));
                    }
                    else
                    {
                        IFC_RETURN(anchorProvider->UnregisterAnchorCandidate(thisAsIUI));
                    }

                    break;
                }
            }
        }

        parent = parent->GetUIElementParentInternal();
    }

    return S_OK;
}

_Check_return_ HRESULT CUIElement::OnPropertyChanged(_In_ const PropertyChangedParams& args)
{
    IFC_RETURN(CDependencyObject::OnPropertyChanged(args));

    switch (args.m_pDP->GetIndex())
    {
        case KnownPropertyIndex::RelativePanel_LeftOf:
        case KnownPropertyIndex::RelativePanel_Above:
        case KnownPropertyIndex::RelativePanel_RightOf:
        case KnownPropertyIndex::RelativePanel_Below:
        case KnownPropertyIndex::RelativePanel_AlignLeftWith:
        case KnownPropertyIndex::RelativePanel_AlignTopWith:
        case KnownPropertyIndex::RelativePanel_AlignRightWith:
        case KnownPropertyIndex::RelativePanel_AlignBottomWith:
        case KnownPropertyIndex::RelativePanel_AlignHorizontalCenterWith:
        case KnownPropertyIndex::RelativePanel_AlignVerticalCenterWith:
        case KnownPropertyIndex::RelativePanel_AlignLeftWithPanel:
        case KnownPropertyIndex::RelativePanel_AlignTopWithPanel:
        case KnownPropertyIndex::RelativePanel_AlignRightWithPanel:
        case KnownPropertyIndex::RelativePanel_AlignBottomWithPanel:
        case KnownPropertyIndex::RelativePanel_AlignHorizontalCenterWithPanel:
        case KnownPropertyIndex::RelativePanel_AlignVerticalCenterWithPanel:
        {
            // Invalidate measure on the RelativePanel when the values of the
            // associated attached properties change so it can re-arrange
            // its children.
            CUIElement* pParent = GetUIElementParentInternal();

            if (pParent && pParent->OfTypeByIndex<KnownTypeIndex::RelativePanel>())
            {
                pParent->InvalidateMeasure();
            }
            break;
        }
        case KnownPropertyIndex::UIElement_AccessKey:
        {
            if (IsActive())
            {
                const auto contentRoot = VisualTree::GetContentRootForElement(this);
                const auto& akExport = contentRoot->GetAKExport();
                //Only add the element to the Scope if we are in AK mode
                if (akExport.IsActive())
                {
                    IFC_RETURN(akExport.AddElementToAKMode(this));
                }
            }
            break;
        }
    }

    return S_OK;
}

xref_ptr<CPointerCollection> CUIElement::GetPointerCaptures() const
{
    CValue result;
    VERIFYHR(GetValueByIndex(KnownPropertyIndex::UIElement_PointerCaptures, &result));
    return static_sp_cast<CPointerCollection>(result.DetachObject());
}

xref_ptr<CTransform> CUIElement::GetRenderTransform() const
{
    // Perf:  Avoid overhead of GetValueByIndex if the effective value is not actually set
    if (IsEffectiveValueInSparseStorage(KnownPropertyIndex::UIElement_RenderTransform))
    {
        CValue result;
        VERIFYHR(GetValueByIndex(KnownPropertyIndex::UIElement_RenderTransform, &result));
        return static_sp_cast<CTransform>(result.DetachObject());
    }
    return xref_ptr<CTransform>(nullptr);
}

CTransform* CUIElement::GetRenderTransformLocal() const
{
    return GetRenderTransform().get();
}

XPOINTF CUIElement::GetRenderTransformOrigin() const
{
    // Perf:  Avoid overhead of GetValueByIndex if the effective value is not actually set
    if (IsEffectiveValueInSparseStorage(KnownPropertyIndex::UIElement_RenderTransformOrigin))
    {
        CValue result;
        VERIFYHR(GetValueByIndex(KnownPropertyIndex::UIElement_RenderTransformOrigin, &result));
        return *(result.AsPoint());
    }
    XPOINTF originDefault = {0.0f, 0.0f};
    return originDefault;
}

xref_ptr<CTransitionCollection> CUIElement::GetTransitions() const
{
    CValue result = CheckOnDemandProperty(KnownPropertyIndex::UIElement_Transitions);
    return static_sp_cast<CTransitionCollection>(result.DetachObject());
}

_Check_return_ HRESULT CUIElement::SetClip(_In_ xref_ptr<CGeometry> value)
{
    IFC_RETURN(SetValueByKnownIndex(KnownPropertyIndex::UIElement_Clip, value.get()));
    return S_OK;
}

xref_ptr<CFlyoutBase> CUIElement::GetContextFlyout() const
{
    CValue result;
    VERIFYHR(GetValueByIndex(KnownPropertyIndex::UIElement_ContextFlyout, &result));
    return static_sp_cast<CFlyoutBase>(result.DetachObject());
}

_Check_return_ HRESULT CUIElement::SetContextFlyout(_In_ xref_ptr<CFlyoutBase> value)
{
    IFC_RETURN(SetValueByKnownIndex(KnownPropertyIndex::UIElement_ContextFlyout, value.get()));
    return S_OK;
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Pulls all non-locally set inherited properties from the parent.
//
//  Overrides for FrameworkElement, Control and TextBlock handle locally
//  set values on those elements.
//  UIElement has no locally settable text properties, swe we just copy our
//  parents values.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT CUIElement::PullInheritedTextFormatting()
{
    IFCEXPECT_ASSERT_RETURN(m_pTextFormatting != nullptr);

    if (m_pTextFormatting->IsOld())
    {
        xref_ptr<TextFormatting> parentTextFormatting;

        // Get the text core properties that we will be inheriting from.
        IFC_RETURN(GetParentTextFormatting(parentTextFormatting.ReleaseAndGetAddressOf()));

        // Process each TextElement text core property one by one.
        IFC_RETURN(m_pTextFormatting->SetFontFamily(this, parentTextFormatting->m_pFontFamily));
        if (!m_pTextFormatting->m_freezeForeground)
        {
            IFC_RETURN(m_pTextFormatting->SetForeground(this, parentTextFormatting->m_pForeground));
        }

        m_pTextFormatting->SetLanguageString(parentTextFormatting->m_strLanguageString);
        m_pTextFormatting->SetResolvedLanguageString(parentTextFormatting->GetResolvedLanguageStringNoRef());
        m_pTextFormatting->SetResolvedLanguageListString(parentTextFormatting->GetResolvedLanguageListStringNoRef());
        m_pTextFormatting->m_eFontSize = parentTextFormatting->m_eFontSize;
        m_pTextFormatting->m_nFontWeight = parentTextFormatting->m_nFontWeight;
        m_pTextFormatting->m_nFontStyle = parentTextFormatting->m_nFontStyle;
        m_pTextFormatting->m_nFontStretch = parentTextFormatting->m_nFontStretch;
        m_pTextFormatting->m_nCharacterSpacing = parentTextFormatting->m_nCharacterSpacing;
        m_pTextFormatting->m_nTextDecorations = parentTextFormatting->m_nTextDecorations;
        m_pTextFormatting->m_nFlowDirection = parentTextFormatting->m_nFlowDirection;
        m_pTextFormatting->m_isTextScaleFactorEnabled = parentTextFormatting->m_isTextScaleFactorEnabled;

        m_pTextFormatting->SetIsUpToDate();
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Fast get of RightToLeft flag
//
//  Needed for e.g. animation scenarios where rendering needs the RTL flag
//  without any other text properties.
//
//  UIElement does not have a locally settable FlowDirection property
//  (although it's descendent FrameWorkElement does).
//
//  For UIElement we cache the vale of IsRightToLeft.
//
//------------------------------------------------------------------------

void CUIElement::EvaluateIsRightToLeft()
{
    bool isRightToLeft = false;

    // Fast result if inherited properties are up to date.

    if (m_pTextFormatting != NULL  &&  !m_pTextFormatting->IsOld())
    {
        isRightToLeft = m_pTextFormatting->m_nFlowDirection == DirectUI::FlowDirection::RightToLeft;
    }
    else
    {
        // We'll need to get direction from our parent.

        CDependencyObject *pParent = GetInheritanceParentInternal();

        if (pParent != NULL)
        {
            isRightToLeft = pParent->IsRightToLeft();
        }
    }

    m_isRightToLeft = isRightToLeft;
    m_isRightToLeftGeneration = GetContext()->m_cIsRightToLeftGenerationCounter;
}

//------------------------------------------------------------------------
//
//  Method:   CUIElement::ShouldRaiseEvent
//
//  Synopsis:
//      Returns whether an event needs to be fired or not. The event manager should call this
//      before queueing up an event.
//------------------------------------------------------------------------

bool CUIElement::ShouldRaiseEvent(_In_ EventHandle hEvent, _In_ bool fInputEvent, _In_opt_ CEventArgs *pArgs)
{
    if (fInputEvent && !AllowsDrop())
    {
        switch (static_cast<KnownEventIndex>(hEvent.index))
        {
        case KnownEventIndex::UIElement_DragEnter:
        case KnownEventIndex::UIElement_DragLeave:
        case KnownEventIndex::UIElement_DragOver:
        case KnownEventIndex::UIElement_Drop:
            // Do not raise drag events for non allowed element.
            return false;
        }
    }

    // If this is a control delegate (PointerEnter, etc.), then always raise the event for now.
    if (CUIElement::IsValidDelegate(static_cast<KnownEventIndex>(hEvent.index)))
    {
        // TODO: Let the peer know if they're interested in this event.
        return true;
    }

    // If our list is NULL, there are no listeners yet.
    if (m_pEventList == NULL)
    {
        return false;
    }

    // Do not raise input events for disabled element.
    if (fInputEvent && !IsEnabled())
    {
        return false;
    }

    return true;
}

// Invalidates measure or arrange as per input.
void CUIElement::PropagateLayoutDirty(bool affectsParentMeasure, bool affectsParentArrange)
{
    if (affectsParentMeasure)
    {
        InvalidateMeasure();
    }
    if (affectsParentArrange)
    {
        InvalidateArrange();
    }
}

//------------------------------------------------------------------------
//
//  Method: EnterImpl
//
//  Synopsis:
//      Causes the object and its "children" to enter scope. If bLive,
//      then the object can now respond to OM requests and perform actions
//      like downloads and animation.
//
//      Derived classes are expected to first call <base>::EnterImpl, and
//      then call Enter on any "children".
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CUIElement::EnterImpl(_In_ CDependencyObject *pNamescopeOwner, EnterParams params)
{
    CCoreServices * const core = GetContext();
    const bool isParentEnabled = params.fCoercedIsEnabled;
    static auto runtimeEnabledFeatureDetector = RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector();

    // When Xaml Diagnostics is enabled we need to peg the peer to make sure that it enters the tree so that
    // Xaml Diagnostics can take a reference to it.
    bool peerWasPegged = false;
    auto unpegOnExit = wil::scope_exit([&]()
    {
        if (peerWasPegged)
        {
            UnpegManagedPeer();
        }
    });

    // Explicitly check the shadow for ancestor elements set as Receivers. We can't rely on CThemeShadow::EnterImpl
    // here, because ThemeShadows can be shared, and the shadow could already be in the tree elsewhere.
    if (params.fIsLive && !CThemeShadow::IsDropShadowMode())
    {
        CValue shadowValue;
        IFC_RETURN(GetValueByIndex(KnownPropertyIndex::UIElement_Shadow, &shadowValue));
        ASSERT(shadowValue.IsNull() || shadowValue.GetType() == valueObject);
        CThemeShadow* themeShadow = checked_cast<CThemeShadow>(shadowValue.AsObject());

        if (themeShadow != nullptr)
        {
            IFC_RETURN(themeShadow->CheckForAncestorReceivers(this /* newParent */));
            //Task: 15141734
            //If ThemeShadow is set, we force the canvas root to have compNode
            EnsureRootCanvasCompNode();
        }
    }

    // If parent is enabled, but local value of IsEnabled is FALSE, need to disable children.
    if (isParentEnabled && !GetIsEnabled())
    {
        params.fCoercedIsEnabled = FALSE;
    }

    if (params.fIsLive)
    {
        // If parent is disabled, but local value is enabled, then coerce to FALSE.
        if (!isParentEnabled && GetIsEnabled())
        {
            // Coerce value and raise changed event.
            IFC_RETURN(CoerceIsEnabled(FALSE, /*bCoerceChildren*/FALSE));
        }

        // Inherit the UseLayoutRounding property
        if (!IsPropertyDefaultByIndex(KnownPropertyIndex::UIElement_UseLayoutRounding))
        {
            // Pass on the non-default value
            params.fUseLayoutRounding = GetUseLayoutRounding();
        }
        else
        {
            // Inherit the new value
            SetUseLayoutRounding(params.fUseLayoutRounding);
        }

        // layout storage has been created by the parent collection (OnAddToCollection) or by the setting
        // of one of the transitions.
        // when transitioning into a live tree, the absolute offset can be transformed into a relative value
        // that is used by layout transitions.
        if (!IsActive())
        {
            if (HasLayoutTransitionStorage())
            {
                LayoutTransitionStorage* pStorage = GetLayoutTransitionStorage();

                XRECTF offset = { 0, 0, 0, 0 };
                XRECTF topLeft = { 0, 0, 0, 0 };
                if (pStorage->m_nextGenerationCounter > 0)  // used to verify storage should have meaningful values
                {
                    CLayoutManager* pLayoutManager = VisualTree::GetLayoutManagerForElement(this);
                    // after the base implementation of enter, the element will be active (and thus no longer relative)

                    ASSERT(GetParentInternal(false));

                    // since we have just been reparented, the offset is currently to the plugin
                    xref_ptr<ITransformer> pTransformer;
                    IFC_RETURN(((CUIElement*)GetParentInternal(false))->TransformToRoot(pTransformer.ReleaseAndGetAddressOf()));
                    IFC_RETURN(CTransformer::TransformBounds(pTransformer, &topLeft, &offset));

                    // current offset will be either the last actual layout pass or the bcb information
                    pStorage->m_currentOffset.x -= offset.X;
                    pStorage->m_currentOffset.y -= offset.Y;

                    // copy these offsets to next generation as well
                    pStorage->m_nextGenerationOffset = pStorage->m_currentOffset;
                    if (pLayoutManager)
                    {
                        pStorage->m_nextGenerationCounter = pLayoutManager->GetNextLayoutCounter();
                    }
                }
            }

            m_enteredTreeCounter = EnteredInThisTick;   // the enter counter will be set on the next layout cycle
        }
    }

    // Pass updated params to children.
    IFC_RETURN(CDependencyObject::EnterImpl(pNamescopeOwner, params));

    // Extends EnterImpl to the ContextFlyout
    CFlyoutBase* pFlyoutBase = GetContextFlyout();
    if (pFlyoutBase)
    {
        // This FlyoutBase can be shared between ContentRoots -- remove the VisualTree
        // pointer here for this enter.  TODO: figure out why this happens
        // Bug 19548424: Investigate places where an element entering the tree doesn't have a unique VisualTree ptr
        EnterParams newParams(params);
        newParams.visualTree = nullptr;
        IFC_RETURN(pFlyoutBase->Enter(pNamescopeOwner, newParams/*EnterParams*/));
    }

    // Work on the children
    if (m_pChildren)
    {
        IFC_RETURN(m_pChildren->Enter(pNamescopeOwner, params));
    }

    {
        xref_ptr<CDependencyObject>* annotations = GetAutomationAnnotationsStorage();

        if (annotations && annotations->get())
        {
            IFC_RETURN(annotations->get()->Enter(pNamescopeOwner, params));
        }
    }

    // If a diagnostic session is active, and the element is in the tree, then we need to call EnsurePeerAndTryPeg so that the peer
    // gets pegged. This will keep it alive until it gets in the tree, once it's in the tree XamlDiagnostics will hold
    // a strong reference to it. After we enter the managed peer, we can remove our peg. We can check if the peer is going
    // to actually enter the tree to ensure we don't create one unnecessarily.
    if (params.fIsLive && runtimeEnabledFeatureDetector->IsFeatureEnabled(RuntimeFeatureBehavior::RuntimeEnabledFeature::XamlDiagnostics))
    {
        IFC_RETURN(EnsurePeerAndTryPeg(&peerWasPegged));
    }

    //If this object has a managed peer, it needs to process Enter as well.
    if(HasManagedPeer())
    {
        IFC_RETURN(FxCallbacks::DependencyObject_EnterImpl(
            this,
            pNamescopeOwner,
            params.fIsLive,
            params.fSkipNameRegistration,
            params.fCoercedIsEnabled,
            params.fUseLayoutRounding
        ));
    }

    const auto contentRoot = VisualTree::GetContentRootForElement(this);

    if (params.fIsLive)
    {
        // If there are events registered on this element, ask the
        // EventManager to extract them and a request for every event.
        if (m_pEventList)
        {
            // Get the event manager.
            IFCPTR_RETURN(core);
            CEventManager* pEventManager = core->GetEventManager();
            IFCPTR_RETURN(pEventManager);
            IFC_RETURN(pEventManager->AddRequestsInOrder(this, m_pEventList));
        }

        // Make sure that we propagate OnDirtyPath bits to the new parent.
        if ((GetIsLayoutElement() || GetIsParentLayoutElement()))
        {
            SetIsMeasureDirty(TRUE);
            SetIsArrangeDirty(TRUE);
        }
        else
        {
            if (GetIsMeasureDirty() == FALSE)
            {
                SetLayoutFlags(LF_MEASURE_DIRTY_PENDING, TRUE);
            }
            if (GetIsArrangeDirty() == FALSE)
            {
                SetLayoutFlags(LF_ARRANGE_DIRTY_PENDING, TRUE);
            }
        }

        const bool propMeasure = GetRequiresMeasure();
        const bool propArrange = GetRequiresArrange();

        const bool propViewport = GetIsViewportDirtyOrOnViewportDirtyPath();
        const bool propContributesToViewport = GetWantsViewportOrContributesToViewport();

        CValue isAnchorCandidateValue;
        IFC_RETURN(GetValueByIndex(KnownPropertyIndex::UIElement_CanBeScrollAnchor, &isAnchorCandidateValue));
        if (isAnchorCandidateValue.AsBool())
        {
            IFC_RETURN(UpdateAnchorCandidateOnParentScrollProvider(true /* add */));
        }

        if (EventEnabledElementAddedInfo())
        {
            auto pParent = this->GetUIElementParentInternal();
            TraceElementAddedInfo(reinterpret_cast<XUINT64>(this), reinterpret_cast<XUINT64>(pParent));
        }

        if (propMeasure)
        {
            PropagateOnMeasureDirtyPath();

            // We need to invalidate the parent's measure so that it will re-measure its children
            CUIElement* pParent = GetUIElementParentInternal();
            if (pParent != nullptr)
            {
                pParent->InvalidateMeasure();
            }
        }

        if (propArrange)
        {
            PropagateOnArrangeDirtyPath();
        }

        if (propViewport)
        {
            PropagateOnViewportDirtyPath();
        }

        if (propContributesToViewport)
        {
            PropagateOnContributesToViewport();
        }

        InvalidateAutomationPeerDataInternal();

        const auto& akExport = contentRoot->GetAKExport();

        if (akExport.IsActive())
        {
            IFC_RETURN(akExport.AddElementToAKMode(this));
        }

        ResetLayoutInformation();

        // Let this DirectManipulation container know that it is now live in the tree.
        if (m_fIsDirectManipulationContainer)
        {
            ctl::ComPtr<CUIDMContainer> dmContainer;
            IFC_RETURN(GetDirectManipulationContainer(&dmContainer));
            if (dmContainer != nullptr)
            {
                CInputServices* inputServicesNoRef = GetContext()->GetInputServices();
                if (inputServicesNoRef)
                {
                    inputServicesNoRef->EnsureHwndForDManipService(this, GetElementInputWindow());
                }

                IFC_RETURN(dmContainer->NotifyManipulatabilityAffectingPropertyChanged(TRUE /*fIsInLiveTree*/));
            }
        }
    }

    //  Adding instrumentation for type.
    if (contentRoot)
    {
        const auto profiler = RuntimeProfiler::GetXamlIslandTypeProfiler(contentRoot->GetIslandType());

        if (profiler)
        {
            Parser::StableXbfTypeIndex myIndex = Parser::GetStableXbfTypeIndex(GetTypeIndex());
            LONG * pTypeCounts = profiler->GetCounterBuffer();

            if (((UINT16)myIndex) < Parser::StableXbfTypeCount)
            {
                volatile LONG *pCounter = &(pTypeCounts[(UINT16)myIndex]);

                if (0 == ::InterlockedIncrement(pCounter))
                {
                    profiler->RegisterType(myIndex, pCounter);
                }
            }
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns True when the provided manipulationMode is an invalid combination.
//      System can be combined with TranslateX, TranslateY, TranslateRailsX,
//      TranslateRailsY and DirectUI::ManipulationModes::TranslateInertia.
//      DirectUI::ManipulationModes::TranslateInertia cannot be the only flag combined with System though.
//      System can also be combined with Scale and Scale + ScaleInertia.
//      System cannot be combined with both translation and scaling flags though.
//
//------------------------------------------------------------------------
/* static */
bool CUIElement::GetIsManipulationModeInvalid(_In_ DirectUI::ManipulationModes manipulationMode)
{
    DirectUI::ManipulationModes translationFlagsAllowedWithSystem =
        DirectUI::ManipulationModes::TranslateX |
        DirectUI::ManipulationModes::TranslateY |
        DirectUI::ManipulationModes::TranslateRailsX |
        DirectUI::ManipulationModes::TranslateRailsY |
        DirectUI::ManipulationModes::TranslateInertia;

    DirectUI::ManipulationModes scalingFlagsAllowedWithSystem =
        DirectUI::ManipulationModes::Scale |
        DirectUI::ManipulationModes::ScaleInertia;

    DirectUI::ManipulationModes flagsAllowedWithSystem =
        translationFlagsAllowedWithSystem |
        scalingFlagsAllowedWithSystem;

    return
        ManipulationModes::None != SystemManipulationModes(manipulationMode)
        &&
        (
        (DirectUI::ManipulationModes::None != (CustomManipulationModes(manipulationMode) & ~flagsAllowedWithSystem)) // Do not allow rotation flags.
        ||
        (DirectUI::ManipulationModes::None != (CustomManipulationModes(manipulationMode) & translationFlagsAllowedWithSystem) &&
         DirectUI::ManipulationModes::None != (CustomManipulationModes(manipulationMode) & scalingFlagsAllowedWithSystem)) // Do not allow both translation and scaling flags.
        ||
        ((DirectUI::ManipulationModes::System | DirectUI::ManipulationModes::TranslateInertia) == manipulationMode)
        ||
        ((DirectUI::ManipulationModes::System | DirectUI::ManipulationModes::ScaleInertia) == manipulationMode)
        );
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Discards the potential rejection viewports associated with this
//      element or its subtree.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElement::DiscardRejectionViewportsInSubTree()
{
    HRESULT hr = S_OK;
    CInputServices *inputServices = NULL;

    // Discard the potential rejection viewport associated with this element
    if (GetContext())
    {
        inputServices = GetContext()->GetInputServices();
        if (inputServices)
        {
            inputServices->AddRef();
            IFC(inputServices->DiscardRejectionViewportsInSubTree(this));
        }
    }

Cleanup:
    ReleaseInterface(inputServices);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Causes the object and its properties to leave scope. If bLive,
//      then the object is leaving the "Live" tree, and the object can no
//      longer respond to OM requests related to being Live.   Actions
//      like downloads and animation will be halted.
//
//      Derived classes are expected to first call <base>::LeaveImpl, and
//      then call Leave on any "children".
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CUIElement::LeaveImpl(_In_ CDependencyObject *pNamescopeOwner, LeaveParams params)
{
    CCoreServices * const core = GetContext();
    const bool isParentEnabled = params.fCoercedIsEnabled;
    bool alreadyCanceledTransitions = false;

    // If parent is enabled, but local value of IsEnabled is FALSE, need to disable children.
    if (isParentEnabled && !GetIsEnabled())
    {
        params.fCoercedIsEnabled = FALSE;
    }

    // Clear the skip focus subtree flags.
    m_skipFocusSubtree_OffScreenPosition = false;
    m_skipFocusSubtree_Other = false;

    // When visual tree is being reset, (CCoreServices::ResetVisualTree) no need to coerce values/raise events.
    if (params.fIsLive && !params.fVisualTreeBeingReset)
    {
        // If parent is enabled and local value is enabled and coerced value is disabled, then coerce to TRUE.
        if (isParentEnabled && GetIsEnabled() && !IsEnabled())
        {
            // Coerce value and raise changed event.
            IFC_RETURN(CoerceIsEnabled(TRUE, /*bCoerceChildren*/FALSE));
        }

        // Revert the UseLayoutRounding property
        if (!IsPropertyDefaultByIndex(KnownPropertyIndex::UIElement_UseLayoutRounding))
        {
            // Pass on the non-default value
            params.fUseLayoutRounding = GetUseLayoutRounding();
        }
        else
        {
            // Inherit the new value
            SetUseLayoutRounding(params.fUseLayoutRounding);
        }
    }

    // If we have absolutely positioned renderers, we always need
    // to cancel transitions regardless of fIsLive. This covers scenarios
    // like removing the dragged item from a list while dragging, which
    // only results in a non-live leave walk (because of how DOCollection
    // handles removes).
    if (HasAbsolutelyPositionedLayoutTransitionRenderers())
    {
        if (HasLayoutTransitionStorage())
        {
            GetLayoutTransitionStorage()->UnregisterElementForTransitions(this);     // kills unrealized transitions in the layout manager, very unlikely
        }

        IFC_RETURN(CTransition::CancelTransitions(this));  // kills currently running animations, quite likely.
        alreadyCanceledTransitions = true;
    }

    // calculate offsets when transitioning from a live element in the visual tree to a non live element
    if (params.fIsLive)
    {
        CLayoutManager* pLayoutManager = VisualTree::GetLayoutManagerForElement(this);

        // transform the offset to be absolute against the plugin
        if (HasLayoutTransitionStorage())
        {
            XRECTF offset = {0, 0, 0, 0};
            XRECTF topLeft = {0, 0, 0, 0};
            LayoutTransitionStorage* pStorage = GetLayoutTransitionStorage();
            CUIElement* pParent = do_pointer_cast<CUIElement>(GetParentInternal(false));

            ASSERT(pParent);    // difficult assert, based on the logic inside Collection::Remove
                                // but it can be trusted to never change.

            // no matter what, if we are leaving the live tree and we have a transition active, we should stop it
            // * this is an important step in how unloading nesting works. When a sub-graph is leaving the visual tree,
            //   individual nodes that are unloading are not stopped by the cancellation of unloads happening
            //   higher in the tree (there are no dependencies).
            // * another important reason to call cancel here is that the cancel will update the current offset with data
            //   from the LTE.

            if (!alreadyCanceledTransitions)
            {
                pStorage->UnregisterElementForTransitions(this);     // kills unrealized transitions in the layout manager, very unlikely
                IFC_RETURN(CTransition::CancelTransitions(this));  // kills currently running animations, quite likely.
            }

            xref_ptr<ITransformer> pTransformer;
            IFC_RETURN(pParent->TransformToRoot(pTransformer.ReleaseAndGetAddressOf()));
            IFC_RETURN(CTransformer::TransformBounds(pTransformer, &topLeft, &offset));

            // preparation:
            // if we are in a new layout cycle, next generation information is actually correct
            if (pStorage && pLayoutManager && pLayoutManager->GetLayoutCounter() >= pStorage->m_nextGenerationCounter)
            {
                pStorage->m_currentOffset = pStorage->m_nextGenerationOffset;
                pStorage->m_currentSize = pStorage->m_nextGenerationSize;
            }

            // current offset was relative to parent
            pStorage->m_currentOffset.x += offset.X;
            pStorage->m_currentOffset.y += offset.Y;

            // update opacity
            {
                // if there was an active transition, it will always have better information
                // by way of the opacity on the LayoutTransitionElement.
                // Unfortunately, the cancelling of a transition and the call to leave occurs
                // at different points in time, so we use a value to indicate whether the
                // leaveImpl should bother with caching the opacity.
                // It will do so, if there was not active transition during unload.

                // The flag will normally be true, except if an element is being removed that had
                // an active transition.
                if (pStorage->m_opacityCache == LeaveShouldDetermineOpacity)
                {
                    pStorage->m_opacityCache = GetOpacityToRoot();
                }
            }

        }

        if (pLayoutManager) // can be null when plugin is tearing down.
        {
            // indicates when this leave occurred (is used as a signal to know if a reparent is occurring)

            // we cannot use the LeftInThisTick constant, since that would be sure to trigger a reparent
            // when the element is being entered again. Instead, we need to set the counter such that,
            // --- if an enter occurs in the same tick it will be counted as a reparent ---

            m_leftTreeCounter = pLayoutManager->GetLayoutCounter();
        }
    }

    if (params.fIsLive)
    {
        // Ensure the element's render data is removed from the scene by clearing it.
        // We don't need to do this recursively since the Leave() walk is already recursive.
        // TODO: INCWALK: Consider just calling the recursive version here anyway? It would be simpler and less error-prone, but perhaps worse for perf.

        // Cleanup all the unshared per element device resources. This include primitives,
        // composition nodes, visuals, shape realizations and cache realizations.
        // The recursive leave call for children would do similar cleanup on the elements of subtree.
        if (m_propertyRenderData.IsRenderWalkTypeForComposition())
        {
            EnsurePropertyRenderData(RWT_None);
        }
        else if (m_propertyRenderData.type == RWT_NonePreserveDComp)
        {
            // An item was removed from the tree after a Device Lost, but before the
            // the Property Render Data was completely restored.  Clean up the
            // composition data.
            RemoveCompositionPeer();
        }

        // If there is a UIA client listening, register this element to have the StructureChanged
        // automation event fired for it. We do this here because CUIElement::LeavePCSceneRecursive
        // (i.e. the place where we register removed elements) will not be called on this subtree,
        // given that it is trying to be efficient and ride on the Leave walk instead of doing
        // another one.
        if (core->UIAClientsAreListening(UIAXcp::AEStructureChanged) == S_OK)
        {
            RegisterForStructureChangedEvent(
                AutomationEventsHelper::StructureChangedType::Removed);
        }

        // Special case for LTE embedded in the tree in the HWWalk. The TransitionRoot does not
        // enter/leave the tree like regular UIElements do, so we need to ensure we keep it up-to-date here.
        CTransitionRoot* localTransitionRootNoRef = GetLocalTransitionRoot(false);
        if (localTransitionRootNoRef)
        {
            localTransitionRootNoRef->LeavePCSceneRecursive();
        }
    }

    IFC_RETURN(CDependencyObject::LeaveImpl(pNamescopeOwner, params));

    // Extends LeaveImpl to the ContextFlyout.
    CFlyoutBase* pFlyoutBase = GetContextFlyout();
    if (pFlyoutBase)
    {
        IFC_RETURN(pFlyoutBase->Leave(pNamescopeOwner, params /*LeaveParams*/));
    }

    if (EventEnabledElementRemovedInfo() && params.fIsLive)
    {
        auto pParent = this->GetUIElementParentInternal();
        TraceElementRemovedInfo(reinterpret_cast<XUINT64>(this), reinterpret_cast<XUINT64>(pParent));
    }

    // Work on the children
    if (m_pChildren)
    {
        IFC_RETURN(m_pChildren->Leave(pNamescopeOwner, params));
    }

    // If this object has a managed peer, it needs to process Leave as well.
    if (HasManagedPeer())
    {
        IFC_RETURN(FxCallbacks::DependencyObject_LeaveImpl(
            this,
            pNamescopeOwner,
            params.fIsLive,
            params.fSkipNameRegistration,
            params.fCoercedIsEnabled,
            params.fVisualTreeBeingReset
        ));
    }

    if (params.fIsLive)
    {
        // If we are leaving the Live tree and there are events.
        // Popup can live outside the live tree. Do not remove event handlers for Popup when it leaves the tree.
        // Popup's event handlers will be removed when it gets deleted.
        if (!OfTypeByIndex<KnownTypeIndex::Popup>())
        {
            IFC_RETURN(RemoveAllEventListeners(true /* leaveUIEShownHiddenEventListenersAttached */));
        }

        // Let this DirectManipulation container know that it no longer lives in the tree
        if (m_fIsDirectManipulationContainer)
        {
            ctl::ComPtr<CUIDMContainer> dmContainer;
            IFC_RETURN(GetDirectManipulationContainer(&dmContainer));
            if (dmContainer != nullptr)
            {
                // This call marks the associated viewport's m_fNeedsUnregistration flag to true so that the viewport
                // will be removed from the InputManager's internal viewports xvector, and the viewport will no longer be
                // submitted to the compositor.
                IFC_RETURN(dmContainer->NotifyManipulatabilityAffectingPropertyChanged(FALSE /*fIsInLiveTree*/));
            }
        }

        // Discard the potential rejection viewports within this leaving element's subtree
        IFC_RETURN(DiscardRejectionViewportsInSubTree());

        CValue isAnchorCandidateValue;
        IFC_RETURN(GetValueByIndex(KnownPropertyIndex::UIElement_CanBeScrollAnchor, &isAnchorCandidateValue));
        if (isAnchorCandidateValue.AsBool())
        {
            IFC_RETURN(UpdateAnchorCandidateOnParentScrollProvider(false /* add */));
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method: AddEventListener
//
//  Synopsis:
//      Override to base AddEventListener
//
//------------------------------------------------------------------------

_Check_return_
HRESULT
CUIElement::AddEventListener(
    _In_ EventHandle hEvent,
    _In_ CValue *pValue,
    _In_ XINT32 iListenerType,
    _Out_opt_ CValue *pResult,
    _In_ bool fHandledEventsToo)
{
    bool isThemeChangedEvent = (hEvent.index == KnownEventIndex::FrameworkElement_ActualThemeChanged);

    if (hEvent.index == KnownEventIndex::FrameworkElement_EffectiveViewportChanged)
    {
        SetWantsViewport(TRUE);
        PropagateOnContributesToViewport();
    }

    return CEventManager::AddEventListener(this, &m_pEventList, hEvent, pValue, iListenerType, pResult, fHandledEventsToo, isThemeChangedEvent);
}

//------------------------------------------------------------------------
//
//  Method: RemoveEventListener
//
//  Synopsis:
//      Override to base RemoveEventListener
//
//------------------------------------------------------------------------

_Check_return_
HRESULT
CUIElement::RemoveEventListener(
    _In_ EventHandle hEvent,
    _In_ CValue *pValue)
{
    auto scopeGuard = wil::scope_exit([&]
    {
        // If we just removed the last registration of the EffectiveViewportChanged
        // event, then clear the flag on this element.
        if (hEvent.index == KnownEventIndex::FrameworkElement_EffectiveViewportChanged
            && !GetContext()->GetEventManager()->IsRegisteredForEvent(this, hEvent))
        {
            SetWantsViewport(FALSE);
        }
    });

    IFC_RETURN(CEventManager::RemoveEventListener(this, m_pEventList, hEvent, pValue));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   RemoveAllEventListeners
//
//  Synopsis:  Removes all the event listeners attached for events on this object.
//      EventManager does an AddRef on the object for each request. If we dont
//      clean up, we will leak.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CUIElement::RemoveAllEventListeners(bool leaveUIEShownHiddenEventListenersAttached)
{
    CEventManager *pEventManager = GetContext()->GetEventManager();

    if (pEventManager && m_pEventList)
    {
        // Add the events in...
        CXcpList<REQUEST>::XCPListNode *pTemp = m_pEventList->GetHead();
        while (pTemp)
        {
            REQUEST * pRequest = (REQUEST *)pTemp->m_pData;
            if (!leaveUIEShownHiddenEventListenersAttached
                || (pRequest->m_hEvent.index != KnownEventIndex::UIElement_Hidden && pRequest->m_hEvent.index != KnownEventIndex::UIElement_Shown))
            {
                IFC_RETURN(pEventManager->RemoveRequest(this, pRequest));
            }
            pTemp = pTemp->m_pNext;
        }
    }
    return S_OK;
}

bool CUIElement::IsLoadedEventPending()
{
    CEventManager *pEventManager = GetContext()->GetEventManager();

    if (pEventManager)
    {
        return pEventManager->IsLoadedEventPending(this);
    }

    return false;
}

// Gets the combined local transform; returns whether it's identity
_Check_return_ bool CUIElement::GetLocalTransform(
    TransformRetrievalOptions transformRetrievalOptions,
    _Out_ CMILMatrix *pLocalTransform)
{
    pLocalTransform->SetToIdentity();
    XamlLocalTransformBuilder builder(pLocalTransform);

    bool includePropertySetInComposition = (transformRetrievalOptions & TransformRetrievalOptions::IncludePropertiesSetInComposition) == TransformRetrievalOptions::IncludePropertiesSetInComposition;

    // There is an important difference in what's required to gather up the local transform between ContainerVisuals mode and DComp mode when the
    // HandOff visual is being used by the app.
    //
    // In DComp mode, since the HandOff visual is a separate visual, we incorporate the transforms on the HandOff visual in addition
    // to all the XAML transforms.  However in ContainerVisuals mode, since the HandOff visual is also used to store the XAML transforms,
    // and we've been listening for updates to this visual's transform components via DCompPropertyChangedListener, we already have all the
    // transforms we're interested in, cached in sparse storage, so we can just use it directly. The exception is if we're intentionally
    // ignoring the transforms set on the WUC visual, in which case we go through the Xaml calculations again.
    //
    // In ContainerVisuals mode when HandOff visuals are involved, we have either a HandOff visual transform object if the transform is
    // entirely 2D, or a HandOff visual matrix 3D that combines the 2D and 3D transforms together. Here we consider only 2D transforms,
    // so we look at only the transform object. If the matrix is involved, it will be handled via GetHitTestingTransform3DMatrix when we
    // look for 3D transforms.
    if (CanUseHandOffVisualTransformGroupForLocalTransform() && includePropertySetInComposition)
    {
        auto handoffVisualTransform = GetHandOffVisualTransform();
        ASSERT(handoffVisualTransform != nullptr);
        builder.ApplyHandOffVisualTransform(handoffVisualTransform, GetTranslation(true));
    }
    else
    {
        bool fTransformSet = false;
        XFLOAT dmOffsetX = 0.0f;
        XFLOAT dmOffsetY = 0.0f;
        XFLOAT dmUncompressedZoomFactor = 1.0f;
        XFLOAT dmZoomFactorX = 1.0f;
        XFLOAT dmZoomFactorY = 1.0f;

        // For facades, we'll prefer the animating value for hit-testing related queries only
        bool preferAnimatingValue = HasFacadeAnimation() && includePropertySetInComposition;

        // Facades_TODO:  Need to handle translation.Z in combination with perspective
        wfn::Vector3 translation = GetTranslation(preferAnimatingValue);
        XFLOAT offsetX = GetActualOffsetX() + translation.X;
        XFLOAT offsetY = GetActualOffsetY() + translation.Y;

        bool shouldFlipRTL, shouldFlipRTLInPlace;
        GetShouldFlipRTL(&shouldFlipRTL, &shouldFlipRTLInPlace);

        XFLOAT elementWidth = GetActualWidth();
        XFLOAT elementHeight = GetActualHeight();

        if (IsManipulatable())
        {
            BOOL transformSet;
            IFCFAILFAST(GetDirectManipulationCompositorTransform(
                transformRetrievalOptions,
                transformSet,
                dmOffsetX,
                dmOffsetY,
                dmUncompressedZoomFactor,
                dmZoomFactorX,
                dmZoomFactorY));

            // TextBoxView requires special handling if its FlowDirection is RightToLeft.
            // TextBoxView internally uses an LTR coordinate space even when in RTL mode.
            // This code performs the required coordinate conversion to make the flip if necessary.
            if (OfTypeByIndex<KnownTypeIndex::TextBoxView>())
            {
                dmOffsetX += (static_cast<CTextBoxView*>(this))->GetRightToLeftOffset();
            }
            fTransformSet = !!transformSet;
        }

        FacadeTransformInfo facadeInfo = GetFacadeTransformInfo(preferAnimatingValue);

        GetLocalTransformHelper(
            &builder,
            offsetX,
            offsetY,
            fTransformSet ? dmOffsetX : 0.0f,
            fTransformSet ? dmOffsetY : 0.0f,
            fTransformSet ? dmZoomFactorX : 1.0f,
            fTransformSet ? dmZoomFactorY : 1.0f,
            shouldFlipRTL,
            shouldFlipRTLInPlace,
            elementWidth,
            elementHeight,
            GetRenderTransformLocal(),
            GetRenderTransformOrigin(),
            GetTransitionTarget().get(),
            true,    // applyDMZoomToOffset
            nullptr, // pDManipSharedTransform is only used by HWCompTreeNode
            nullptr, // rto is only used by HWCompTreeNodeWinRT
            AreFacadesInUse() ? &facadeInfo : nullptr
            );
    }

    return pLocalTransform->IsIdentity();
}

// Helper function to extract the combination of the 2D local transform and 3D transforms into a single 4x4 matrix.
CMILMatrix4x4 CUIElement::GetLocalTransform4x4()
{
    CMILMatrix4x4 result(true);
    CMILMatrix transformMatrix;
    const bool localTransformIsIdentity = GetLocalTransform(TransformRetrievalOptions::IncludePropertiesSetInComposition, &transformMatrix);
    const CMILMatrix4x4 transform3DMatrix = GetHitTestingTransform3DMatrix(true /* includePropertiesSetInComposition */);
    CMILMatrix4x4 projectionMatrix(true);
    if (HasActiveProjection())
    {
        XSIZEF elementSize;
        IFCFAILFAST(GetElementSizeForProjection(&elementSize));
        projectionMatrix = GetProjection()->GetOverallProjectionMatrix(elementSize);
    }

    if (localTransformIsIdentity && transform3DMatrix.IsIdentity() && projectionMatrix.IsIdentity())
    {
        // Everything is identity. No need to do anything.
        return result;
    }

    //
    // Update cumulative transform
    //
    // If this element is on a 3D branch, or if the element itself has 3D, then prepend the element's combined local
    // transform (both 2D and 3D) to the existing cumulative transform.
    //
    if (!localTransformIsIdentity)
    {
        CMILMatrix4x4 temp(&transformMatrix); // Convert the incoming matrix to a (flat) 4x4
        temp._33 = 1.0f; // Don't flatten.
        result = temp;
    }

    // Populate with 3D transform.
    if (!transform3DMatrix.IsIdentity())
    {
        // Populate the builder with any 3D transforms.
        result.Prepend(transform3DMatrix);
    }

    // Populate with projection
    if (!projectionMatrix.IsIdentity())
    {
        result.Prepend(projectionMatrix);
    }

    return result;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns whether this element should flip the flow direction and
//      whether the flip should be done in-place.
//
//------------------------------------------------------------------------
void CUIElement::GetShouldFlipRTL(
    _Out_ bool *pShouldFlipRTL,
    _Out_ bool *pShouldFlipRTLInPlace
    )
{
    CUIElement *pParent = GetUIElementAdjustedParentInternal(FALSE /*fPublicParentsOnly*/);
    CPopup *pParentAsPopup = do_pointer_cast<CPopup>(pParent);

    bool isParentlessPopup = !IsActive() && OfTypeByIndex<KnownTypeIndex::Popup>();
    bool isParentAParentlessPopup = pParentAsPopup != NULL && !pParentAsPopup->IsActive();
    bool myRTLMatchesParentRTL = pParent == NULL || pParent->IsRightToLeft() == IsRightToLeft();
    bool IsParentAnRTLParentlessPopup = isParentAParentlessPopup && pParentAsPopup->IsRightToLeft();

    //
    // If the flow direction changed between the parent and this element, push a transform to horizontally
    // flip this element.  The transform adjusts for the width of the element, with one exception...
    //
    // Parentless popups are treated specially here.  The flow direction is handled by the parentless Popup's
    // child.  Parentless Popups aren't layed out so they just flip their content, but since the flip is
    // handled by their child adjusting for the child's width now would be incorrect since the flow
    // direction logically changed above the child.
    //
    // TODO: Merge: Why don't parentless popups handle the flip? Why does the child have to do it?
    //
    bool flipRTL =
        // If this is a parentless popup, do nothing about RTL. The child will handle RTL.
        !isParentlessPopup
        && (
            // This isn't a parentless popup and the RTL doesn't match the parent's RTL. Flip.
            !myRTLMatchesParentRTL
            // This isn't a parentless popup but this is the child of a parentless popup. Handle the parent's RTL.
            || IsParentAnRTLParentlessPopup
            );

    *pShouldFlipRTL = flipRTL;

    // Parentless popups don't flip their content in place.
    *pShouldFlipRTLInPlace = !isParentAParentlessPopup;
}

//------------------------------------------------------------------------
//
//  Method:   GetActualWidth
//
//  Synopsis:
//      RenderWidth if layout is present, else max of width and minwidth
//
//------------------------------------------------------------------------
_Check_return_ XFLOAT CUIElement::GetActualWidth()
{
    return 0.0f;
}

//------------------------------------------------------------------------
//
//  Method:   GetActualHeight
//
//  Synopsis:
//      RenderHeight if layout is present, else max of height and minheight
//
//------------------------------------------------------------------------
_Check_return_ XFLOAT CUIElement::GetActualHeight()
{
    return 0.0f;
}

// Update the hit-test-visibility, and possibly update corresponding CompNode state
void CUIElement::SetIsHitTestVisible(bool fIsHitTestVisible)
{
    if (m_fHitTestVisible != fIsHitTestVisible)
    {
        m_fHitTestVisible = fIsHitTestVisible;
        UpdateHitTestVisibilityForComposition();
    }
}

// Update enabled state, and possibly update corresponding CompNode state
void CUIElement::SetIsEnabled(bool fIsEnabled)
{
    if (m_fIsEnabled != fIsEnabled)
    {
        m_fIsEnabled = fIsEnabled;
        UpdateHitTestVisibilityForComposition();
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Should be called before doing programmatic hit testing.
//      To proceed with programmatic hit testing, Two conditions must
//      be met:
//      (1) The element is HitTestVisible.
//      (2) Should hittest disabled element OR The element is enabled.
//
//------------------------------------------------------------------------
bool CUIElement::IsEnabledAndVisibleForHitTest(bool canHitDisabledElements, bool canHitInvisibleElements) const
{
    return IsHitTestVisible(canHitInvisibleElements) && (canHitDisabledElements || IsEnabled());
}

// Used to determine if this element is a candidate for creating a dedicated
// CompNode for hit-test-invisibility purposes.
bool CUIElement::IsEnabledAndVisibleForDCompHitTest() const
{
    // This function is similar to IsEnabledAndVisibleForHitTest(), except it
    // queries m_fIsEnabled instead of m_fCoercedIsEnabled as a performance optimization.
    // Since m_fCoercedIsEnabled is propagated to the entire child hierarchy of the
    // element with m_fIsEnabled, querying m_fCoercedIsEnabled would lead to the creation
    // of a CompNode for each node in the hierarchy, which is unnecessary.
    return IsHitTestVisible() && GetIsEnabled();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns true if the current element is part of the subtree of the
//  other element, adjusting for popup boundaries.
//
//------------------------------------------------------------------------
bool CUIElement::IsInUIElementsAdjustedVisualSubTree( _In_ const CUIElement *pPotentialParentOrSelf )
{
    CUIElement* pElement = this;

    // Need to walk up and figure out the the parent chain.
    while (pElement != nullptr)
    {
        if (pElement == pPotentialParentOrSelf)
        {
            return true;
        }

        // Move to adjusted parent
        pElement = pElement->GetUIElementAdjustedParentInternal(FALSE);
    }

    return false;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      dtor
//
//------------------------------------------------------------------------
CHitTestResults::~CHitTestResults()
{
    // Release all reference inside
    if ( m_cResults )
    {
        CXcpList<CUIElement>::XCPListNode *pNode = NULL;

        pNode = m_oResults.GetHead();
        while ( pNode != NULL )
        {
            // Release current item
            CDependencyObject* pResult = reinterpret_cast<CDependencyObject*>(pNode->m_pData);
            ReleaseInterface(pResult);

            // Move to the next element
            pNode = pNode->m_pNext;
        }
    }

    m_oResults.Clean(FALSE);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Gets the first returned UIElement, addref'd.
//
//------------------------------------------------------------------------
_Ret_maybenull_ CUIElement *
CHitTestResults::First()
{
    if (m_cResults)
    {
        CUIElement *pFirst = m_oResults.GetHead()->m_pData;
        AddRefInterface(pFirst);
        return pFirst;
    }
    return NULL;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Adds a new UIElement to the current result set.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CHitTestResults::Add( _In_ CUIElement* pResult )
{

    IFCPTR_RETURN(pResult);

    IFC_RETURN( m_oResults.Add(pResult) );
    AddRefInterface(pResult);
    m_cResults++;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns an ALLOCATED array of CDependencyObject* with the list
//      of things hit.
//      Caller is responsible for freeing this one.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CHitTestResults::GetAnswer(
    _Out_ XUINT32 *pCount,
    _Outptr_result_buffer_(*pCount) CUIElement ***pppResults )
{
    HRESULT hr = S_OK;

    XUINT32 nResults = m_cResults;
    CXcpList<CUIElement>::XCPListNode *pNode = NULL;
    CUIElement **ppResults = NULL;

    if ( m_cResults )
    {
        ppResults = new CUIElement *[ m_cResults ];

        pNode = m_oResults.GetHead();
        while ( pNode != NULL )
        {
            // Store the result in the results array - the list
            // has them in reverse order
            nResults--;
            ppResults[nResults] = pNode->m_pData;
            AddRefInterface(ppResults[nResults]);

            // Move to the next element
            pNode = pNode->m_pNext;
        }
    }

    ASSERT( nResults == 0 );

    *pCount = m_cResults;
    *pppResults = ppResults;
    RRETURN(hr);//RRETURN_REMOVAL
}

//------------------------------------------------------------------------
//
//  Synopsis:
//            Global entry point for point hit testing. Returns only
//            the first CUIElement hit.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElement::HitTestEntry(
    const HitTestParams& hitTestParams,
    _In_ XPOINTF hitPoint,
    _In_ bool canHitDisabledElements,
    _In_ bool canHitInvisibleElements,
    _Out_ CUIElement **ppHitElement
    )
{
    CHitTestResults results;

    IFC_RETURN(HitTestEntry(hitTestParams, hitPoint, false /*canHitMultipleElements*/, canHitDisabledElements, canHitInvisibleElements, &results));

    *ppHitElement = results.First();

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//            Global entry point for point hit testing. Multiple results
//            can be returned.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElement::HitTestEntry(
    const HitTestParams& hitTestParams,
    _In_ XPOINTF hitPoint,
    _In_ bool canHitMultipleElements,
    _In_ bool canHitDisabledElements,
    _In_ bool canHitInvisibleElements,
    _Inout_ CHitTestResults *pHitElements
    )
{
    CBoundedHitTestVisitor Visitor(pHitElements, canHitMultipleElements);

    IFC_RETURN(BoundsTestEntry(hitTestParams, hitPoint, &Visitor, canHitDisabledElements, canHitInvisibleElements));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//            Global entry point for rect hit testing.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElement::HitTestEntry(
    const HitTestParams& hitTestParams,
    const HitTestPolygon& hitPolygon,
    _In_ bool canHitMultipleElements,
    _In_ bool canHitDisabledElements,
    _In_ bool canHitInvisibleElements,
    _Inout_ CHitTestResults *pHitElements
    )
{
    CBoundedHitTestVisitor Visitor(pHitElements, canHitMultipleElements);

    IFC_RETURN(BoundsTestEntry(hitTestParams, hitPolygon, &Visitor, canHitDisabledElements, canHitInvisibleElements));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//     Gets the first element in the ancestor chain that renders to an
// intermediate surface or NULL if there are no intermediates present
//
//------------------------------------------------------------------------
_Ret_maybenull_ CUIElement*
CUIElement::GetFirstAncestorWithIntermediate()
{
    CUIElement *pElement = this;

    // Need to walk up and figure if there's a parent with intermediate
    while (pElement != NULL)
    {
        if (pElement->NeedsIntermediateRendering())
        {
            // We found the root of the intermediate tree,
            // bail out...
            break;
        }

        // Move to immediate parent
        pElement = pElement->GetUIElementAdjustedParentInternal(TRUE /*fPublicParentsOnly*/);
    }

    return pElement;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Gets the parent of element and adjust it to popup if parent is PopupRoot
//
//------------------------------------------------------------------------
_Ret_maybenull_ CUIElement*
CUIElement::GetUIElementAdjustedParentInternal(
    bool publicParentsOnly, /* = TRUE */
    bool useRealParentForClosedParentedPopups /* = FALSE */
    )
{
    if (GetTypeIndex() == KnownTypeIndex::XamlIslandRoot)
    {
        // When an element is in a XamlIslandRoot, we treat the XamlIslandRoot as the root of the tree.
        // Content in a XamlIslandRoot shouldn't interact with elements outside of that XamlIslandRoot.
        // Example: TransformToRoot on an element within a XamlIslandRoot shouldn't include the transform
        // of the RootVisual, which is the DPI scale.
        return nullptr;
    }

    CUIElement *pParent = GetUIElementParentInternal(publicParentsOnly);

    // If the immediate parent is the popup root it means this element is a
    // Popup's child, so we want to jump to the logical parent (the Popup).
    if (pParent)
    {
        bool parentIsPopupRoot = false;

        if (SUCCEEDED(GetContext()->IsObjectAnActivePopupRoot(pParent, &parentIsPopupRoot)))
        {
            // The only elements visually parented to the PopupRoot are Popup.Child and TransitionRoots.
            // In the former case, the adjusted parent we'll return is the Popup itself.
            // In the latter case, the PopupRoot will be returned as the regular visual parent for the TransitionRoot.
            if (parentIsPopupRoot && !OfTypeByIndex<KnownTypeIndex::TransitionRoot>())
            {
                pParent = static_cast<CUIElement*>(GetLogicalParentNoRef());
                ASSERT(pParent->OfTypeByIndex<KnownTypeIndex::Popup>());
            }
        }
    }

    //
    // If this element is a parent-less Popup or an open nested Popup inside a closed ancestor,
    // fallback to treating the PopupRoot as its visual parent since the Popup isn't in a live tree.
    //
    // The exception is if we're walking up the tree to gather the rendering context for a redirected
    // walk and that this is the first step of the walk (see GetRedirectionTransformsAndParentCompNode).
    // In that case, if the first step comes to a closed parented popup, we allow the walk to continue
    // from the closed popup.
    //
    //  - Open parented popups are active and have a parent. The walk continues from the popup's parent.
    //
    //  - Closed parented popups aren't active but still have a parent. The walk continues from the
    //    popup's parent if closed popups are allowed. Otherwise the walk continues from the popup root.
    //
    //  - Parentless popups aren't active and have no parent, regardless of whether or not they are open.
    //    The walk continues from the popup root, because the popup by definition does not have a parent.
    //
    if (!IsActive()
        && OfTypeByIndex<KnownTypeIndex::Popup>()
        && (pParent == nullptr || !useRealParentForClosedParentedPopups))
    {
        CPopupRoot *pPopupRoot = nullptr;
        IGNOREHR(GetContext()->GetAdjustedPopupRootForElement(this, &pPopupRoot));
        pParent = pPopupRoot;
    }

    return pParent;
}

//------------------------------------------------------------------------
//
//  Method:   AdjustBoundingRectForIntermediates
//
//  Synopsis:
//      This API adjust the raw bounding boxes that come from rendering to
// include the extra transforms done by intermediate surface rendering.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElement::AdjustBoundingRectToRoot(_Inout_ XRECTF *pRect)
{
    HRESULT hr = S_OK;
    ITransformer* pTransformer = NULL;

    // Need to walk up and figure if there's a parent with intermediate
    CUIElement *pIntermediateParent = GetFirstAncestorWithIntermediate();

    IFCPTR(pRect);

    if (pIntermediateParent)
    {
        // Get the transformation chain to the root
        IFC(pIntermediateParent->TransformToRoot(&pTransformer));

        // Now transform the bounds
        IFC(CTransformer::TransformBounds(
            pTransformer,
            pRect,
            pRect,
            FALSE /* bReverse */
            ));
    }

Cleanup:
    ReleaseInterface(pTransformer);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   GetClickablePointRasterizedClient
//
//  Synopsis:
//      This API for getting a point that represents the RasterizedClient coordinates
//      of the element that is associated with the automation peer.
//
//------------------------------------------------------------------------

_Check_return_
HRESULT
CUIElement::GetClickablePointRasterizedClient(_Out_ XPOINTF *pPoint)
{
    XRECTF rectBounded;
    XRECTF_RB bounds = { };

    IFCPTR_RETURN(pPoint);
    *pPoint = {};

    // Set the empty rectangle
    EmptyRectF(&rectBounded);

    // Get the bounding rect
    IFC_RETURN(GetGlobalBounds(&bounds));
    rectBounded = ToXRectF(bounds);

    // Adjust for intermediates along the way
    IFC_RETURN(AdjustBoundingRectToRoot(&rectBounded));

    // Calculate the clickable point from the bounding rect
    if (rectBounded.Width != 0 || rectBounded.Height != 0)
    {
        pPoint->x = rectBounded.X + (rectBounded.Width / 2);
        pPoint->y = rectBounded.Y + (rectBounded.Height / 2);
    }

    if (XamlOneCoreTransforms::IsEnabled())
    {
        // In OneCoreTransforms mode, GetGlobalBounds returns logical pixels so we must convert to RasterizedClient
        const auto logicalPoint = *pPoint;
        const auto physicalPoint = logicalPoint * (RootScale::GetRasterizationScaleForElement(this));
        *pPoint = physicalPoint;
    }

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates an abstract transformation object capable of transforming
//      XPOINTF up and down the object hierarchy.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElement::GetTransformer(
    _Outptr_result_maybenull_ ITransformer** ppTransformer
    )
{
    HRESULT hr = S_OK;

    ITransformer* pTransformer = NULL;
    CPerspectiveTransformer *pTransformer3D = NULL;
    CAggregateTransformer *pAggregateTransformer = NULL;
    XUINT32 uiTransformerCount = 0;

    CMILMatrix matLocal;
    if (!GetLocalTransform(TransformRetrievalOptions::IncludePropertiesSetInComposition, &matLocal))
    {
        pTransformer = new CMatrixTransformer(matLocal);
        ++uiTransformerCount;
    }

    IFC(NWGetTransformerHelper(&pTransformer3D));

    if (pTransformer3D)
    {
        ++uiTransformerCount;
    }

    if (uiTransformerCount > 1)
    {
        pAggregateTransformer = new CAggregateTransformer();

        if (pTransformer3D)
        {
            IFC(pAggregateTransformer->Add(pTransformer3D));
        }
        if (pTransformer)
        {
            IFC(pAggregateTransformer->Add(pTransformer));
        }

        *ppTransformer = pAggregateTransformer; // steal ref
        pAggregateTransformer = NULL;
    }
    else
    {
        *ppTransformer =
            pTransformer ? pTransformer :
            pTransformer3D ? pTransformer3D :
            NULL;   // steal ref

        pTransformer = NULL;
        pTransformer3D = NULL;
    }

Cleanup:
    ReleaseInterface(pTransformer);
    ReleaseInterface(pTransformer3D);
    ReleaseInterface(pAggregateTransformer);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates a transformer to handle any other transform-affecting
//      properties on this element, if needed.  New walk only.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElement::NWGetTransformerHelper(
    _Outptr_result_maybenull_ CPerspectiveTransformer **ppTransformer3D
    )
{
    //
    // Check for a no-op projection for compat. CUIElement::TransformToVisual
    // tries to return a CMatrixTransform whenever possible, and that
    // requires all transformers to be IsPure2D(). A perspective transformer,
    // even a no-op one, reports that it isn't pure 2D, which would mean
    // TransformToVisual no longer returns a CMatrixTransform.
    //
    if (HasActiveProjection())
    {
        IFC_RETURN(NWGetProjectionTransformer(ppTransformer3D));
    }
    else
    {
        *ppTransformer3D = NULL;
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates a transformer to account for the projection on this
//      element.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElement::NWGetProjectionTransformer(
    _Outptr_result_maybenull_ CPerspectiveTransformer **ppTransformer
    )
{
    XSIZEF elementSize;

    ASSERT(GetProjection());

    IFC_RETURN(GetElementSizeForProjection(&elementSize));

    if (elementSize.width > 0 && elementSize.height > 0)
    {
        GetProjection()->GetTransformerCommon(elementSize, ppTransformer);
    }
    else
    {
        //
        // TODO: Merge: When we use rendered bounds for projection rather than a mix of
        // layout bounds and surface bounds, this case will no longer be necessary. For now this
        // case exists to match old walk behavior: CProjection::OWGetTransformer returns null if
        // either dimension is 0.
        //
        *ppTransformer = nullptr;
    }

    return S_OK;
}

//  Set the z-Index of this element and notify the parent that the z-index has changed
//  so it can reorder the children collection.
_Check_return_ HRESULT CUIElement::SetZIndex(_In_ INT32 value)
{
    // TODO: Merge: not needed after root visual stops fiddling with z order, holds on to its children, and renders them in a predetermined order
    // TODO: Merge: XcpTypes triggers NWSetTransformDirty when ZIndex changes just to generate a dirty region, which is overkill

    CValue cValue;

    cValue.Set<valueSigned>(value);

    // Set the basic index on the UIElement.
    IFC_RETURN(SetEffectiveValueInSparseStorage(
        MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::Canvas_ZIndex),
        cValue));

    IFC_RETURN(OnZOrderChanged());

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Updates persistent rendering data structures in response to a z-order change.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElement::OnZOrderChanged()
{
    // When z-order changes, the back-to-front rendering order of the child collection the element resides in needs to
    // be updated. The parent owns this collection.
    CUIElement *pParent = GetUIElementParentInternal();
    if (pParent)
    {
        pParent->SetChildRenderOrderDirty();
    }

    // The primitive composition walk uses a persistent data structure for its render data.
    // A z-index change needs to be treated as if the element was removed and re-added to the scene in order for the
    // data structure to remain valid - there's no efficient 'move' operation currently supported for the data structure.
    if (IsInPCScene())
    {
        LeavePCSceneRecursive();

        // Make sure the element is marked dirty so that it'll be rendered again and put back into the scene.
        // ZIndex changes from the property system will call this change handler anyway, but changes that just call
        // this method will not. Since successive calls will no-op, better to be safe than sorry.
        // TODO: HWPC: Choosing 'transform dirty' is somewhat arbitrary, since the subgraph should be entirely regenerated because it will be entering the scene again anyway.
        CUIElement::NWSetTransformDirty(this, DirtyFlags::Render);
    }

    return S_OK; // RRETURN_REMOVAL
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Public ZIndex property change callback.
//
//------------------------------------------------------------------------
_Check_return_
HRESULT
CUIElement::ZIndex(
    _In_ CDependencyObject *pObject,
    _In_ XUINT32 cArgs,
    _Inout_updates_(cArgs) CValue *ppArgs,
    _In_opt_ IInspectable* pValueOuter,
    _Out_ CValue *pResult
)
{
    if (cArgs == 1 && ppArgs)
    {
        CUIElement* pElm = NULL;
        XINT32 nNewZIndex = 0;
        XINT32 nOldZIndex = 0;
        bool bChanged = false;

        IFC_RETURN(DoPointerCast(pElm, pObject));

        nOldZIndex = pElm->GetZIndex();

        // We are setting the value...
        if (ppArgs->GetType() == valueSigned)
        {
            nNewZIndex = ppArgs->AsSigned();
        }
        else if (ppArgs->GetType() == valueObject)
        {
            if ((ppArgs->AsObject())->OfTypeByIndex<KnownTypeIndex::Int32>())
            {
                const auto pDO = static_cast<const CInt32*>(ppArgs->AsObject());
                nNewZIndex = pDO->m_iValue;
            }
            else
            {
                IFC_RETURN(E_INVALIDARG);
            }
        }
        else
        {
            IFC_RETURN(E_INVALIDARG);
        }

        // Validate against max allowed value
        if ( nNewZIndex > XCP_MAX_ZINDEX_VALUE )
        {
            IFC_RETURN(E_INVALIDARG);
        }

        if (nOldZIndex != nNewZIndex)
        {
            IFC_RETURN(pElm->SetZIndex(nNewZIndex));
            bChanged = TRUE;
        }

        if (pResult)
        {
            pResult->SetBool(!!bChanged);
        }
    }
    else if (pResult)
    {
       // We are getting the value
       pResult->SetSigned(do_pointer_cast<CUIElement>(pObject)->GetZIndex());
    }
    else
    {
        IFC_RETURN(E_INVALIDARG);
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CapturePointer
//
//  Synopsis:
//      Set pointer capture on this element if it is hittestable
//
//------------------------------------------------------------------------
_Check_return_
HRESULT
CUIElement::CapturePointer(
    _In_ CPointer* pPointer,
    _Out_ bool *pbCapture)
{
    if (!IsHitTestVisible())
    {
        IFC_RETURN(E_INVALIDARG);
    }

    CContentRoot* contentRoot = VisualTree::GetContentRootForElement(this);
    IFC_RETURN(contentRoot->GetInputManager().GetPointerInputProcessor().SetPointerCapture(this, pPointer, pbCapture));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   ReleasePointerCapture
//
//  Synopsis:
//      Release capture on this element if it is hittestable
//
//------------------------------------------------------------------------
_Check_return_
HRESULT
CUIElement::ReleasePointerCapture(
    _In_ CPointer* pPointer)
{
    CInputManager& inputManager = VisualTree::GetContentRootForElement(this)->GetInputManager();
    IFC_RETURN(inputManager.GetPointerInputProcessor().ReleasePointerCapture(this, pPointer));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   ReleasePointerCaptures
//
//  Synopsis:
//      Release all captures on this element if it is hittestable
//
//------------------------------------------------------------------------
_Check_return_
HRESULT
CUIElement::ReleasePointerCaptures()
{
    CContentRoot* contentRoot = VisualTree::GetContentRootForElement(this);
    IFC_RETURN(contentRoot->GetInputManager().GetPointerInputProcessor().ReleaseAllPointerCaptures(this));

    return S_OK;
}

_Check_return_ HRESULT
CUIElement::TransformToVisualHelperGetTransformer(
    _In_ CUIElement* pVisual,
    _Out_ bool* isReverse,
    _Outptr_ ITransformer** returnValue)
{
    CUIElement *pRoot;
    CUIElement *pOther;
    xref_ptr<ITransformer> transformer;

    *isReverse = FALSE;
    *returnValue = nullptr;

    // 19H1 Bug #19652900:  When the element is in a XamlIslandRoot, we cannot rely on GetTreeRoot() logic as this
    // function is unaware of islands, and this ends up causing us to find the wrong root when pVisual == NULL.
    // The fix is to detect when we're in a XamlIslandRoot scenario and retrieve the appropriate root from ContentRoot.
    CContentRoot* contentRoot = VisualTree::GetContentRootForElement(this);
    if (contentRoot != nullptr && contentRoot->GetType() == CContentRoot::Type::XamlIslandRoot)
    {
        pRoot = contentRoot->GetVisualTreeNoRef()->GetRootElementNoRef();
        if (pVisual != nullptr)
        {
            pOther = pVisual;
        }
        else
        {
            pOther = pRoot;
        }
    }
    else
    {
        // For app-compat reasons, use this code path when not in an island scenario.
        // TODO:  Can we delete this code path altogether?
        IFC_RETURN(DoPointerCast(pRoot, GetTreeRoot()));

        IFC_RETURN(pRoot ? S_OK : E_FAIL);

        if (!pVisual)
        {
            if (1.0f == RootScale::GetRasterizationScaleForElement(this))
            {
                pOther = pRoot;
            }
            else
            {
                IFC_RETURN(DoPointerCast(pOther, GetTreeRoot(TRUE /* fPublic */)));
            }
        }
        else
        {
            pOther = pVisual;
        }
    }

    // Compute transformer
    if (pOther == this)
    {
        // NO-OP
    }
    else if (pOther == pRoot)
    {
        IFC_RETURN(TransformToRoot(transformer.ReleaseAndGetAddressOf()));
    }
    else
    {
        // Determine parent and child
        CUIElement *pTemp = this;
        while (pTemp != nullptr && pTemp != pOther)
        {
            // move to parent
            pTemp = pTemp->GetUIElementAdjustedParentInternal();
        }

        if (pTemp == pOther)
        {
            // OTHER is in our parent chain
            IFC_RETURN(TransformToAncestor(pOther, transformer.ReleaseAndGetAddressOf()));
        }
        else
        {
            // OTHER is not in our parent chain, so look for it in our child chain
            pTemp = pOther;
            while (pTemp != nullptr && pTemp != this)
            {
                // move to parent
                pTemp = pTemp->GetUIElementAdjustedParentInternal();
            }

            if (pTemp == this)
            {
                // OTHER is in our child chain
                IFC_RETURN(pOther->TransformToAncestor(this, transformer.ReleaseAndGetAddressOf()));
                *isReverse = TRUE;
            }
            else
            {
                xref_ptr<ITransformer> upTransformer;
                xref_ptr<ITransformer> downTransformer;

                // Other is on a sibling chain, do an up+down transformation...
                IFC_RETURN(TransformToRoot(upTransformer.ReleaseAndGetAddressOf()));
                IFC_RETURN(pOther->TransformToRoot(downTransformer.ReleaseAndGetAddressOf()));
                transformer.attach(new CUpDownTransformer(upTransformer.get(), downTransformer.get()));
            }
        }
    }

    *returnValue = transformer.detach();

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   TransformToVisual
//
//  Synopsis: Returns a Transform object that represents the mapping form
//  the current UIElement to the given UIElement (or visual).  If the given
//  UIElement is null then the transform to the root visual is computed.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElement::TransformToVisual(
    _In_ CUIElement* pVisual,
    _Out_ xref_ptr<CGeneralTransform>* transform)
{
    return TransformToVisual(pVisual, false /* ignore3D */, transform);
}

//------------------------------------------------------------------------
//
//  Method:   TransformToVisual
//
//  Synopsis: Returns a Transform object that represents the mapping form
//  the current UIElement to the given UIElement (or visual).  If the given
//  UIElement is null then the transform to the root visual is computed.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElement::TransformToVisual(
    _In_ CUIElement* pVisual,
    _In_ bool ignore3D,
    _Out_ xref_ptr<CGeneralTransform>* transform)
{
    bool bIsReverse = false;
    bool bIs2D = false;
    xref_ptr<ITransformer> transformer;

    CREATEPARAMETERS cp(GetContext());

    IFC_RETURN(TransformToVisualHelperGetTransformer(pVisual, &bIsReverse, transformer.ReleaseAndGetAddressOf()));

    // If we can convert this CInternalTransform to a CMatrixTransform, do so for compatibility.
    bIs2D = !transformer || transformer->IsPure2D() || ignore3D;
    if (bIs2D)
    {
        CMILMatrix matTransform(FALSE);
        xref_ptr<CMatrix> matrix;
        xref_ptr<CMatrixTransform> matrixTransform;

        IFC_RETURN(CMatrixTransform::Create(reinterpret_cast<CDependencyObject**>(matrixTransform.ReleaseAndGetAddressOf()), &cp));
        IFC_RETURN(CMatrix::Create(reinterpret_cast<CDependencyObject**>(matrix.ReleaseAndGetAddressOf()), &cp));

        if (transformer)
        {
            matTransform = transformer->Get2DMatrixIgnore3D();
            if (bIsReverse)
            {
                if (!matTransform.Invert())
                {
                    matTransform.SetToIdentity();
                }
            }
        }
        else
        {
            matTransform.SetToIdentity();
        }

        matrix->m_matrix = matTransform;
        IFC_RETURN(matrixTransform->SetValueByKnownIndex(KnownPropertyIndex::MatrixTransform_Matrix, matrix.get()));

        *transform = std::move(matrixTransform);
    }
    else
    {
        xref_ptr<CInternalTransform> internalTransform;

        IFC_RETURN(CInternalTransform::Create((CDependencyObject**)(internalTransform.ReleaseAndGetAddressOf()), &cp));
        internalTransform->SetTransformer(transformer.get(), bIsReverse);
        *transform = std::move(internalTransform);
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   AggregateElementTransform
//
//  Synopsis: Gets the transformer for an element and adds it to an
//  aggregate transform.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CUIElement::AggregateElementTransform(_In_ CUIElement* pElement, _In_ CAggregateTransformer* pAggregate)
{
    HRESULT hr = S_OK;
    ITransformer* pTransformer = NULL;

    // Get transformer to transform from immediate parent to current
    IFC(pElement->GetTransformer(&pTransformer));

    // Add it to the compound
    IFC(pAggregate->Add(pTransformer));

Cleanup:
    ReleaseInterface(pTransformer);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   TransformToAncestor
//
//  Synopsis: Gets the Transformer to transform a point in the current visual's
//  coordinate space to that of an ancestor.  Returns an error if the visual
//  is not in the chain.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElement::TransformToAncestor(
    _In_ const CUIElement *pAncestor,
    _Outptr_ ITransformer** ppTransformer
    )
{
    RRETURN(GetTransformToAncestorCommon(
        pAncestor,
        TRUE,   // fPublicParentsOnly
        FALSE,  // fStopAtParentWithIntermediate
        ppTransformer
        ));
    // TODO: Merge: This one doesn't allow private parents. Is this intentional?
}

//------------------------------------------------------------------------
//
//  Method:   TransformToRoot
//
//  Synopsis: Gets the Transformer to transform a point in the current visual's
//  coordinate space to that of its root.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElement::TransformToRoot(
    _Outptr_ ITransformer** ppTransformer
    )
{
    RRETURN(GetTransformToAncestorCommon(
        NULL,   // pAncestor
        FALSE,  // fPublicParentsOnly
        FALSE,  // fStopAtParentWithIntermediate
        ppTransformer
        ));
}

//------------------------------------------------------------------------
//
//  Method:   TransformToParentWithIntermediate
//
//  Synopsis:
//      Gets the Transformer to transform a point in the current visual's
//      coordinate space to that of the nearest parent with an intermediate.
//      If no such parent exists, return the Transformer that transforms it
//      to the root.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElement::TransformToParentWithIntermediate(
    _Outptr_ ITransformer **ppTransformer
    )
{
    RRETURN(GetTransformToAncestorCommon(
        NULL,   // pAncestor
        FALSE,  // fPublicParentsOnly
        TRUE,   // fStopAtParentWithIntermediate
        ppTransformer
        ));
}

//------------------------------------------------------------------------
//
//  CUIElement::GetTransformToAncestorCommon
//
//  Synopsis:
//      Returns a ITransformer from this element up to some ancestor
//      element. The ancestor can be specified, or it can be null if it
//      should be the root. There is also a flag for stopping when the
//      upward walk encounters the first element (not counting this one)
//      that requires an intermediate to render.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElement::GetTransformToAncestorCommon(
    _In_opt_ const CUIElement *pAncestor,
    bool fPublicParentsOnly,
    bool fStopAtParentWithIntermediate,
    _Outptr_ ITransformer** ppTransformer
    )
{
    HRESULT hr = S_OK;

    CUIElement *pElement = this;
    CAggregateTransformer *pAggregate = new CAggregateTransformer();

    do
    {
        if (pElement == pAncestor)
        {
            //
            // Found the target ancestor. It should not be null, otherwise we shouldn't
            // have looped. pElement won't be null in the first iteration because it
            // is initialized as 'this'.
            //
            ASSERT(pElement);
            break;
        }

        IFC(AggregateElementTransform(pElement, pAggregate));

        pElement = pElement->GetUIElementAdjustedParentInternal(!!fPublicParentsOnly);

    } while (   pElement != NULL
             && (!fStopAtParentWithIntermediate || !pElement->NeedsIntermediateRendering()));

    // Either we weren't looking for a particular ancestor or we found a match
    IFCEXPECT(!pAncestor || pElement == pAncestor);

    *ppTransformer = pAggregate;
    pAggregate = NULL;

Cleanup:
    ReleaseInterface(pAggregate);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  CUIElement::ComputeRealizationTransform
//
//  Synopsis:
//      Computes a transform to root in the same way that the render walk
//      computes the realization transform.  This transform is different
//      than the transform produced by TransformToRoot as it handles
//      projections in a way that optimizes the HWRealization transform.
//
//  Note:
//      Any changes to the way the realization transform is computed
//      need to be kept in sync with this method.
//      (see HWWalk::RenderProperties())
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElement::ComputeRealizationTransform(_Out_ CTransformToRoot *pRealizationTransform)
{
    CTransformToRoot combinedTransformToRoot;
    CDependencyObject *pCurrent = this;
    while (pCurrent != NULL)
    {
        if (pCurrent->OfTypeByIndex<KnownTypeIndex::UIElement>())
        {
            CUIElement *pCurrentUIElement = static_cast<CUIElement*>(pCurrent);

            if (pCurrentUIElement->HasActiveProjection())
            {
                XSIZEF elementSize;
                IFC_RETURN(pCurrentUIElement->GetElementSizeForProjection(&elementSize));

                CMILMatrix4x4 projectionTransform = pCurrentUIElement->GetProjection()->GetOverallProjectionMatrix(elementSize);

                combinedTransformToRoot.Append(projectionTransform);
            }

            CMILMatrix localTransform;
            // When rendering, we don't care about properties set by composition on the handoff visual. That will prevent problems like
            // creating a tiny mask because there's a WUC scale animation going on.
            if (!pCurrentUIElement->GetLocalTransform(TransformRetrievalOptions::None, &localTransform))
            {
                combinedTransformToRoot.Append(localTransform);
            }
        }

        pCurrent = pCurrent->GetParentInternal(/*fPublic*/ FALSE);
    }

     // Apply the root scale:
    const auto scale = RootScale::GetRasterizationScaleForElementWithFallback(this);
    if (scale != 1.0)
    {
        combinedTransformToRoot.MultiplyRasterizationScale(scale);
    }

    *pRealizationTransform = combinedTransformToRoot;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates the children collection.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElement::CreateChildrenCollection(
    _Outptr_ CUIElementCollection** ppChildrenCollection
    )
{
    HRESULT hr = S_OK;
    CUIElementCollection *pUIElementCollection = NULL;
    EnterParams enterParams(
        /*isLive*/                IsActive(),
        /*skipNameRegistration*/  FALSE,
        /*coercedIsEnabled*/      GetCoercedIsEnabled(),
        /*useLayoutRounding*/     GetUseLayoutRounding(),
        /*visualTree*/            VisualTree::GetForElementNoRef(this, LookupOptions::NoFallback)
    );

    enterParams.fCheckForResourceOverrides = ShouldCheckForResourceOverrides();

    if (!ppChildrenCollection)
    {
        IFC(E_INVALIDARG);
    }

    // We need to create our Collection since we are being asked to
    pUIElementCollection =  new CUIElementCollection(GetContext());

    // Note: this association should be kept in sync with the UIElement.Children declaration in xcptypes.h.
    IFC(pUIElementCollection->SetOwner(this, RENDERCHANGEDPFN(CUIElement::NWSetSubgraphDirty)));

    IFC(pUIElementCollection->Enter(GetStandardNameScopeOwner(), enterParams));

    *ppChildrenCollection = pUIElementCollection;
    pUIElementCollection = NULL;

Cleanup:
    ReleaseInterface(pUIElementCollection);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Notification that children collection has changed.
//      This method checks for z-order changes and invalidates sorting as needed.
//
//------------------------------------------------------------------------
void
CUIElementCollection::OnChildrenChanged(_In_opt_ CDependencyObject *pChildSender)
{
    CUIElement* child = do_pointer_cast<CUIElement>(pChildSender);
    CUIElement* owner = static_cast<CUIElement*>(GetOwner());

    if (child != nullptr)
    {
        //
        // When a child is added its cached edge stores might have been created with a different
        // transform, so mark the child's transform as dirty to regenerate edges as needed.
        //
        // There's no concern about flags having already been set on the child before it was attached
        // here that might prevent propagation to this element, since the act of attaching it guarantees
        // this parent element receives a change notification from the children DOCollection.
        //
        CUIElement::NWSetTransformDirty(child, DirtyFlags::Render | DirtyFlags::Bounds);

        // If we added a child that already has 3D to a parent that doesn't have 3D in subtree marked, we need
        // to propagate that flag up the tree.
        if (!owner->Has3DDepthInSubtree())
        {
            // If there's an LTE targeting a child element, we need to look at whether that LTE has 3D depth as well.
            // See LTE comment in CUIElement::PropagateDepthInSubtree for the scenario.
            if (child->Has3DDepthOnSelfOrSubtreeOrLTETargetingSelf())
            {
                owner->UpdateHas3DDepthInSubtree();
            }
        }
    }
    else
    {
        // A child was removed. If that child was the only source of 3D, then we need to propagate the flag
        // up the tree. We don't have a pointer to the removed child, so we can't optimize the check, so check
        // always.
        if (owner->Has3DDepthInSubtree())
        {
            owner->UpdateHas3DDepthInSubtree();
        }
    }

    //
    // Two cases when we need to mark the collection as dirty for checking and re-sorting:
    //   1. The child with a ZIndex is added
    //   2. The child collection is already sorted
    //      - A new element needs to be sorted into the existing sorted collection.
    //      - A removed element needs to be removed from the existing sorted collection.
    //
    // Note: The sort dirtiness is ignored if there are no children or there is one child.
    // It won't cause checking/sorting and won't be cleared until we have added a second
    // child.
    //
    if (   child && child->GetZIndex() != 0
        || HasSortedChildren())
    {
        SetSortedCollectionDirty(TRUE);
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Recursively Invalidate font size for all children CUIElement.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElement::RecursiveInvalidateFontSize()
{
    IFC_RETURN(InvalidateFontSize());

    // There are some hidden children of (CFullWindowMediaRoot and CSwapChainPanel)
    // cannot be returned using GetUnsortedChildren().
    // In practice the effects are minor, we ignore these children.
    auto children = GetUnsortedChildren();
    UINT32 childrenCount = children.GetCount();

    for (XUINT32 childIndex = 0; childIndex < childrenCount; childIndex++)
    {
        CUIElement* pCurrentChild = children[childIndex];
        ASSERT(pCurrentChild);
        IFC_RETURN(pCurrentChild->RecursiveInvalidateFontSize());
    }

    return S_OK;
}

_Check_return_ HRESULT
CUIElement::InvalidateFontSize()
{
    RRETURN(S_OK);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Recursively Invalidate measure for all children CUIElement.
//
//------------------------------------------------------------------------
void
CUIElement::RecursiveInvalidateMeasure()
{
    InvalidateMeasure();

    // There are some hidden children of (CFullWindowMediaRoot and CSwapChainPanel )
    // cannot be returned using GetUnsortedChildren().
    // In practice the effects are minor, we ignore these children.
    auto children = GetUnsortedChildren();
    UINT32 childrenCount = children.GetCount();

    for (XUINT32 childIndex = 0; childIndex < childrenCount; childIndex++)
    {
        CUIElement* pCurrentChild = children[childIndex];
        ASSERT(pCurrentChild);
        pCurrentChild->RecursiveInvalidateMeasure();
    }
}


void
CUIElement::InvalidateMeasure()
{
    if (!GetIsMeasureDirty() && !GetIsMeasuringSelf())
    {
        InvalidateAutomationPeerDataInternal();

        CUIElement* pParent = GetUIElementParentInternal();

        if (GetIsLayoutElement() || GetIsOnMeasureDirtyPath() || (pParent && pParent->GetIsLayoutElement()))
        {
            CLayoutManager* pLayoutManager = VisualTree::GetLayoutManagerForElement(this);

            InvalidateMeasureInternal(pLayoutManager);

            if (GetIsOnMeasureStack() && pLayoutManager)
            {
                pLayoutManager->PropagateAncestorDirtyFromCurrentLayoutElement(this);
            }
        }
        else if (HasLayoutStorage())
        {
            CLayoutManager* layoutManager = VisualTree::GetLayoutManagerForElement(this);
            if (layoutManager && layoutManager->StoreLayoutCycleWarningContexts())
            {
                StoreLayoutCycleWarningContext(layoutManager);

                if (LayoutCycleDebugSettings::ShouldDebugBreak(DirectUI::LayoutCycleDebugBreakLevel::Level1))
                {
                    __debugbreak();
                }
            }

            SetIsMeasureDirtyPending(TRUE);
        }
    }
}

void
CUIElement::InvalidateMeasureInternal(_In_opt_ CLayoutManager* layoutManager)
{
    // Note: callers are expected to pass in a non-null layoutManager if one exists
    if (layoutManager && layoutManager->StoreLayoutCycleWarningContexts())
    {
        StoreLayoutCycleWarningContext(layoutManager);
        if (LayoutCycleDebugSettings::ShouldDebugBreak(DirectUI::LayoutCycleDebugBreakLevel::Level1))
        {
            __debugbreak();
        }
    }

    SetIsMeasureDirty(TRUE);
    PropagateOnMeasureDirtyPath();
    if (EventEnabledInvalidateMeasureInfo())
    {
        auto pParent = GetUIElementParentInternal();
        TraceInvalidateMeasureInfo(reinterpret_cast<XUINT64>(this), reinterpret_cast<XUINT64>(pParent));
    }
}

void
CUIElement::InvalidateArrange()
{
    if (!GetIsArrangeDirty())
    {
        InvalidateAutomationPeerDataInternal();

        CUIElement* pParent = GetUIElementParentInternal();

        if (GetIsLayoutElement() || GetIsOnArrangeDirtyPath() || (pParent && pParent->GetIsLayoutElement()))
        {
            CLayoutManager* pLayoutManager = VisualTree::GetLayoutManagerForElement(this);

            InvalidateArrangeInternal(pLayoutManager);

            if (GetIsOnArrangeStack() && pLayoutManager)
            {
                pLayoutManager->PropagateAncestorDirtyFromCurrentLayoutElement(this);
            }
        }
        else if (HasLayoutStorage())
        {
            CLayoutManager* layoutManager = VisualTree::GetLayoutManagerForElement(this);
            if (layoutManager && layoutManager->StoreLayoutCycleWarningContexts())
            {
                StoreLayoutCycleWarningContext(layoutManager);

                if (LayoutCycleDebugSettings::ShouldDebugBreak(DirectUI::LayoutCycleDebugBreakLevel::Level1))
                {
                    __debugbreak();
                }
            }

            SetIsArrangeDirtyPending(TRUE);
        }
    }
}

void
CUIElement::InvalidateArrangeInternal(_In_opt_ CLayoutManager* layoutManager)
{
    // Note: callers are expected to pass in a non-null layoutManager if one exists
    if (layoutManager && layoutManager->StoreLayoutCycleWarningContexts())
    {
        StoreLayoutCycleWarningContext(layoutManager);
        if (LayoutCycleDebugSettings::ShouldDebugBreak(DirectUI::LayoutCycleDebugBreakLevel::Level1))
        {
            __debugbreak();
        }
    }

    SetIsArrangeDirty(TRUE);
    PropagateOnArrangeDirtyPath();
    if (EventEnabledInvalidateArrangeInfo())
    {
        auto pParent = GetUIElementParentInternal();
        TraceInvalidateArrangeInfo(reinterpret_cast<XUINT64>(this), reinterpret_cast<XUINT64>(pParent));
    }
}

// These methods store a new WarningContext of type WarningContextType::LayoutCycle when CLayoutManager::StoreLayoutCycleWarningContexts()==True
// indicates that a layout cycle crash may be imminent.
bool
CUIElement::StoreLayoutCycleWarningContexts()
{
    CLayoutManager* layoutManager = VisualTree::GetLayoutManagerForElement(this);

    return layoutManager && layoutManager->StoreLayoutCycleWarningContexts();
}

void
CUIElement::StoreLayoutCycleWarningContext(size_t framesToSkip)
{
    StoreLayoutCycleWarningContext(VisualTree::GetLayoutManagerForElement(this), framesToSkip);
}

void
CUIElement::StoreLayoutCycleWarningContext(_In_opt_ CLayoutManager* layoutManager, size_t framesToSkip)
{
    if (layoutManager && layoutManager->StoreLayoutCycleWarningContexts())
    {
        std::vector<std::wstring> warningInfo;

        StoreLayoutCycleWarningContext(warningInfo, layoutManager, framesToSkip);
    }
}

void
CUIElement::StoreLayoutCycleWarningContext(_In_ std::vector<std::wstring>& warningInfo, _In_opt_ CLayoutManager* layoutManager, size_t framesToSkip)
{
    if (!layoutManager)
    {
        layoutManager = VisualTree::GetLayoutManagerForElement(this);

        if (!layoutManager || !layoutManager->StoreLayoutCycleWarningContexts())
        {
            return;
        }
    }

    std::wstring layoutCycleCountdown(L"LayoutCycleCountdown: ");
    layoutCycleCountdown.append(std::to_wstring(layoutManager->LayoutCycleWarningContextsCountdown()));
    warningInfo.push_back(std::move(layoutCycleCountdown));

    StoreWarningContext(WarningContextLog::WarningContextType::LayoutCycle, warningInfo, framesToSkip);
}

void
CUIElement::InvalidateAutomationPeerDataInternal()
{
    SetIsAutomationPeerDirty(TRUE);
    PropagateOnAutomationPeerDirtyPath();
}

_Check_return_ HRESULT CUIElement::UpdateLayout()
{
    CLayoutManager* pLayoutManager = VisualTree::GetLayoutManagerForElement(this);

    if (pLayoutManager)
    {
        if (EventEnabledUpdateLayoutInfo())
        {
            auto pParent = GetUIElementParentInternal();
            TraceUpdateLayoutInfo(reinterpret_cast<XUINT64>(this), reinterpret_cast<XUINT64>(pParent));
        }

        IFC_RETURN(pLayoutManager->UpdateLayout());
    }
    else
    {
        IFC_RETURN(E_UNEXPECTED);
    }

    return S_OK;
}

void
CUIElement::OnChildDesiredSizeChanged(_In_ CUIElement* child)
{
    InvalidateMeasure();
}

XRECTF
CUIElement::GetProperArrangeRect(XRECTF parentFinalRect)
{
    if (HasLayoutStorage())
    {
        bool isRootVisual = false;
        XRECTF arrangeRect = FinalRect;

        //make sure the visual root gets arranged at plugin size.
        HRESULT hr = GetContext()->IsObjectAnActivePublicRootVisual(this, &isRootVisual);
        ASSERTSUCCEEDED(hr);
        if (FAILED(hr))
        {
            isRootVisual = FALSE;
        }

        if (isRootVisual)
        {
            arrangeRect = parentFinalRect;
        }
        else if (GetUIElementParentInternal() == NULL)
        {
            // Elements without a parent (top level) get Arrange at DesiredSize if they were measured "to content" (as infinity indicates).
            // If we arrange the element that is temporarily disconnected so it is not a top-level one, the assumption is that it will be
            // layout-invalidated and/or recomputed by the parent when reconnected.

            arrangeRect.X = arrangeRect.Y = 0;

            if (IsInfiniteF(PreviousConstraint.width))
                arrangeRect.Width = DesiredSize.width;

            if (IsInfiniteF(PreviousConstraint.height))
                arrangeRect.Height = DesiredSize.height;
        }
        else if (!GetIsParentLayoutElement())
        {
            // If the parent is not a layout element, we need to change the size to the desired size
            // but keep the offsets. We have to do this because although WPF assumes that the root
            // of a dirty island has not actually changed size (it was marked as measure-dirty because
            // a child changed size, but the reason that it is the root is because it has not changed
            // size, otherwise it would have marked its parent dirty, etc.) but in our case something may
            // have changed size but not marked its parent as dirty because its parent, not being a
            // layout element, didn't care.

            arrangeRect.Size() = DesiredSize;
        }

        return arrangeRect;
    }
    else
    {
        // This situation should only arise if an element is being arranged without first being measured.
        // Since the width and height of this rect will eventually be used for the ActualWidth/ActualHeight
        // properties, the values should be legal.
        XRECTF arrangeRect = {0, 0, 0, 0};
        return arrangeRect;
    }
}

//------------------------------------------------------------------------
//
//  Method:   GetRenderSize (static)
//
//  Synopsis:
//      Static wrapper to allow the framework to call InvalidateMeasure
//
//------------------------------------------------------------------------

_Check_return_
 HRESULT
 CUIElement::GetRenderSize(
    _In_ CDependencyObject *pObject,
    _In_ XUINT32 cArgs,
    _Inout_updates_(cArgs) CValue *ppArgs,
    _In_opt_ IInspectable* pValueOuter,
    _Out_ CValue *pResult
)
{
    // This is a read-only property, so it's OK to use a static for this
    static XSIZEF size = { 0, 0 };

    ASSERT(pObject->OfTypeByIndex<KnownTypeIndex::UIElement>());
    IFC_RETURN(pObject->OfTypeByIndex<KnownTypeIndex::UIElement>() ? S_OK : E_INVALIDARG);
    ASSERT(cArgs == 0);
    IFC_RETURN(cArgs == 0 ? S_OK : E_INVALIDARG);
    ASSERT(pResult);
    IFCPTRRC_RETURN(pResult, E_INVALIDARG);
    {
        pResult->SetNull();

        const CUIElement* puie = static_cast<CUIElement*>(pObject);
        if (puie->HasLayoutStorage())
        {
            size = puie->m_layoutStorage.m_size;
        }
        else
        {
            size.width = size.height = 0;
        }

        pResult->WrapSize(&size);
    }
    return S_OK;
}


//------------------------------------------------------------------------
//
//  Synopsis:
//    Hold the logic to determine if a layout action (measure or arrange)
//    should be performed.
//
// 1. always perform if we are not in LT phase
// 2. perform if this element is the one being transitioned
// 3. perform if this element has no transition that would have applied in last layout cycle.
//    If there was a transition that could have been applied, it was. If it has already ended or was
//    cancelled, layout during a layouttransition phase of a parent should not influence this element.
//
//------------------------------------------------------------------------
_Check_return_ bool ShouldPerformLayout(_In_ CLayoutManager* pLayoutManager,_In_ CUIElement* pThis)
{
    return (!pLayoutManager->GetIsInLayoutTransitionPhase() ||
        pLayoutManager->GetTransitioningElement() == pThis ||
        !CTransition::HasActiveTransition(pThis));
}

bool CUIElement::RestoreDirtyPathFlagIfNeeded(const bool wasOnDirtyPath, const bool isMeasureNotArrange)
{
    // We already called ArrangeInternal, which should have called Arrange on all the children.
    // We only terminate early in the case of layout being suspended, where we don't call Arrange
    // on any of our children at all.
    // The IsAncestorDirty flag can only cause early termination in the case that this UIElement
    // had the IsOnArrangeDirtyPath flag but not IsArrangeDirty (see below).
    const bool isLayoutSuspended = GetIsLayoutSuspended();

    if (isLayoutSuspended || GetIsAncestorDirty())
    {
        if (isLayoutSuspended && wasOnDirtyPath)
        {
            isMeasureNotArrange ? SetIsOnMeasureDirtyPath(TRUE) : SetIsOnArrangeDirtyPath(TRUE);
        }
        return true;
    }

    return false;
}

//------------------------------------------------------------------------
//
//  Method:   Measure
//
//  Synopsis:
//      Figure out how much space this needs.
//
//------------------------------------------------------------------------
_Check_return_
HRESULT
CUIElement::Measure(XSIZEF availableSize)
{
    HRESULT hr = S_OK;
    XUINT32 fMeasureOnControlResize = FALSE;    // Are we measuring due to control resize.
    XUINT32 count = CLayoutManager::MaxLayoutIterations;

    CLayoutManager* pLayoutManager = NULL;      // Pointer to LayoutManager.

    bool thisIsRootVisual = false;

    bool wasInNonClippingTree = false;

    // All paths exiting this method MUST go through Cleanup.
    // This line should ALWAYS execute.
    LockParent();

    pLayoutManager = VisualTree::GetLayoutManagerForElement(this);
    IFCPTR(pLayoutManager);

    pLayoutManager->PushCurrentLayoutElement(this);

    // remember whether we need to set this value back
    wasInNonClippingTree = pLayoutManager->GetIsInNonClippingTree();
    // once entering a tree that is non-clipping, we don't get out of it
    // remember this for down-stream
    pLayoutManager->SetIsInNonClippingTree(GetIsNonClippingSubtree() || wasInNonClippingTree);

    SetIsOnMeasureStack(TRUE);
    SetIsAncestorDirty(FALSE);

    if (GetIsLayoutElement() || GetIsParentLayoutElement())
        IFC(EnsureLayoutStorage()); // Required for constraint comparison

    //check if this is the dummy root and measure is called because of browser resize.
    IFC(GetContext()->IsObjectAnActiveRootVisual(this, &thisIsRootVisual));
    fMeasureOnControlResize = ( thisIsRootVisual
                               && (availableSize.width != pLayoutManager->GetPreviousPluginWidth()
                               || availableSize.height != pLayoutManager->GetPreviousPluginHeight()));

    // As long as measure is not complete and we didn't
    // consume the maximum number of layout iterations.
    while (count-- > 0)
    {
        if (GetIsMeasureDirty() || ((GetIsLayoutElement() || GetIsParentLayoutElement()) && HasLayoutStorage() && !IsSameSize(availableSize, PreviousConstraint)) || fMeasureOnControlResize)
        {
            fMeasureOnControlResize = FALSE;
            bool wasOnMeasureDirtyPath = GetIsOnMeasureDirtyPath();
            SetIsOnMeasureDirtyPath(FALSE);

            IFC(MeasureInternal(availableSize));

            if (RestoreDirtyPathFlagIfNeeded(wasOnMeasureDirtyPath, true /*isMeasureNotArrange*/))
                break;
        }
        // If on a Measure Dirty path, and it is not layout suspended,
        // this means that one of its children - or more - needs to be
        // measured.
        else if (GetIsOnMeasureDirtyPath() && !GetIsLayoutSuspended())
        {
            // Clear the LF_ON_MEASURE_DIRTY_PATH flag.
            SetIsOnMeasureDirtyPath(FALSE);

            XUINT32 childIndex    = 0;              // Index of current Child.
            CUIElement*  pCurrentChild = NULL;      // Pointer to the current Child.
            CUIElementCollection *pCollectionForUnloading = NULL;
            UIElementCollectionUnloadingStorage::UnloadingMap::const_iterator unloadIterator;
            bool bShouldProcessUnloadingElements = false;

            auto children = GetUnsortedChildren();      // regular children
            UINT32 childrenCount = children.GetCount();

            pCollectionForUnloading = GetChildren();

            if (pCollectionForUnloading && pCollectionForUnloading->HasUnloadingStorage() && pCollectionForUnloading->m_pUnloadingStorage->Count() > 0)
            {
                unloadIterator = pCollectionForUnloading->m_pUnloadingStorage->m_unloadingElements.begin();
                bShouldProcessUnloadingElements = TRUE;
            }

            do
            {
                pCurrentChild = NULL;

                if (childIndex < childrenCount) // regular case: do layout on normal children
                {
                    pCurrentChild = children[childIndex];
                    childIndex++; // for next iteration
                    ASSERT(pCurrentChild);
                }
                else if (bShouldProcessUnloadingElements && unloadIterator != pCollectionForUnloading->m_pUnloadingStorage->m_unloadingElements.end()) // unloading children
                {
                    pCurrentChild = unloadIterator->first;
                    ++unloadIterator;   // for next iteration
                    ASSERT(pCurrentChild);
                }

                if (pCurrentChild != NULL && pCurrentChild->GetRequiresMeasure())
                {
                    IFC(pCurrentChild->EnsureLayoutStorage());
                    IFC(pCurrentChild->Measure(pCurrentChild->PreviousConstraint));
                }

                if (GetIsAncestorDirty())
                {
                    SetIsOnMeasureDirtyPath(TRUE);
                    break;
                }

            } while (pCurrentChild && !GetIsMeasureDirty());
        }
        else
            break;
    }

    // If this ASSERT was hit it means that there is a layout cycle and we hit the limit
    ASSERT(count > 0);

Cleanup:

    UnlockParent();

    // We're not on Measure Stack anymore.
    SetIsOnMeasureStack(FALSE);

    if (pLayoutManager)
    {
        // restore the clipping tree value now that we are bubbling up again
        pLayoutManager->SetIsInNonClippingTree(wasInNonClippingTree);
        VERIFY_COND(pLayoutManager->PopCurrentLayoutElement(), == this);
    }
    RRETURN(hr);
}

_Check_return_
HRESULT
CUIElement::MeasureInternal(XSIZEF availableSize)
{
    HRESULT hr = S_OK;
    XSIZEF prevSize = {0,0}, desiredSize = {0,0};
    XUINT32 fEnteredMeasure = FALSE;

    SetIsMeasuringSelf(TRUE);

    CLayoutManager* pLayoutManager = VisualTree::GetLayoutManagerForElement(this);
    bool bInLayoutTransition = pLayoutManager->GetTransitioningElement() == this;
    XRECTF layoutSlot = XRECTF();
    XSIZEF transitioningConstraint = availableSize;
    if (bInLayoutTransition)
    {
        // being in a layout transition does not necessarily mean we have an active transition
        IFC(CTransition::GetLayoutSlotDuringTransition(this, &layoutSlot));
        transitioningConstraint = layoutSlot.Size();
    }

    IFC(EnsureLayoutStorage());

    // If Layout is suspended i.e, element is collapsed.
    if (GetIsLayoutSuspended())
    {
        if (!IsSameSize(availableSize, PreviousConstraint))
        {
            // Don't store a layout cycle context (if even active) for invalidating this collapsed element
            auto suspender = pLayoutManager->SuspendLayoutCycleLogging();

            InvalidateMeasureInternal(pLayoutManager);
            PreviousConstraint = availableSize;
        }

        CDependencyObject::PropagateLayoutDirty(TRUE, FALSE);
        goto Cleanup;
    }

    if (!GetIsMeasureDirty() && IsSameSize(availableSize, PreviousConstraint) && !bInLayoutTransition)
    {
        goto Cleanup;
    }

    SetHasBeenMeasured(TRUE);

    prevSize = DesiredSize;

    {
        // Don't store a layout cycle context (if even active) for this expected InvalidateArrange call.
        auto suspender = pLayoutManager->SuspendLayoutCycleLogging();
        InvalidateArrange();
    }

    if (ShouldPerformLayout(pLayoutManager, this))
    {
        IFC(pLayoutManager->EnterMeasure(this));
        fEnteredMeasure = TRUE;

        TraceMeasureElementBegin((XUINT64)this, availableSize.width, availableSize.height);

        if (pLayoutManager->StoreLayoutCycleWarningContexts())
        {
            if (LayoutCycleDebugSettings::ShouldTrace(DirectUI::LayoutCycleTracingLevel::Level2))
            {
                std::wstring measureInfo(L"Measure(");
                measureInfo.append(std::to_wstring(availableSize.width));
                measureInfo.append(L"x");
                measureInfo.append(std::to_wstring(availableSize.height));
                measureInfo.append(L")");

                std::vector<std::wstring> warningInfo;
                warningInfo.push_back(std::move(measureInfo));
                StoreLayoutCycleWarningContext(warningInfo, pLayoutManager);
            }
            if (LayoutCycleDebugSettings::ShouldDebugBreak(DirectUI::LayoutCycleDebugBreakLevel::Level2))
            {
                __debugbreak();
            }
        }

        IFC(MeasureCore(transitioningConstraint, desiredSize));

        TraceMeasureElementEnd((XUINT64)this, desiredSize.width, desiredSize.height);
    }
    else
    {
        // skipped measure so desiredSize is not valid.
        desiredSize = DesiredSize;
    }

Cleanup:
    if (HasLayoutStorage())
    {
        PreviousConstraint = availableSize;
    }
    // ignore failure for ExitMeasure() so that we still go through the failure code path below in case there was a failure above
    if (fEnteredMeasure)
    {
        IGNOREHR(pLayoutManager->ExitMeasure(this));
    }

    if (SUCCEEDED(hr))
    {
        if(IsInfiniteF(desiredSize.width) || IsInfiniteF(desiredSize.height))
        {
            hr = E_UNEXPECTED;
        }

        SetIsMeasureDirty(FALSE);

        DesiredSize = desiredSize;

        // do _not_ raise size changes if caused by a layout transition
        // When a LT is in effect, there is already an animation getting this element to the ultimate
        // size. If we were to communicate the current change to a parent, a cycle would be created.
        if (!GetIsMeasureDuringArrange() && ! IsSameSize(prevSize, desiredSize) && !bInLayoutTransition)
        {
            CUIElement* pParent =  GetUIElementParentInternal();
            if (pParent)
            {
                if (pLayoutManager->StoreLayoutCycleWarningContexts() &&
                    pLayoutManager->ShouldReportDesiredSizeChanged(this))
                {
                    std::wstring desiredSizeInfo(L"DesiredSize changed, old: ");
                    desiredSizeInfo.append(std::to_wstring(prevSize.width));
                    desiredSizeInfo.append(L"x");
                    desiredSizeInfo.append(std::to_wstring(prevSize.height));
                    desiredSizeInfo.append(L" new: ");
                    desiredSizeInfo.append(std::to_wstring(desiredSize.width));
                    desiredSizeInfo.append(L"x");
                    desiredSizeInfo.append(std::to_wstring(desiredSize.height));

                    std::vector<std::wstring> warningInfo;
                    warningInfo.push_back(std::move(desiredSizeInfo));
                    StoreLayoutCycleWarningContext(warningInfo, pLayoutManager);

                    if (LayoutCycleDebugSettings::ShouldDebugBreak(DirectUI::LayoutCycleDebugBreakLevel::Level1))
                    {
                        __debugbreak();
                    }
                }
                auto suspender = pLayoutManager->SuspendLayoutCycleLogging();
                pParent->OnChildDesiredSizeChanged(this);
            }
        }
    }
    // Differs from WPF behavior in that we don't replace the last exception element
    // if one has already been defined.  This ensures that the element that was being
    // laid out when an exception was raised will be the last exception element.
    else if (pLayoutManager->GetLastExceptionElement() == NULL)
    {
        pLayoutManager->SetLastExceptionElement(this);
    }

    SetIsMeasuringSelf(FALSE);

    RRETURN(hr);
}

_Check_return_ HRESULT
CUIElement::MeasureCore(XSIZEF availableSize, _Out_ XSIZEF& desiredSize)
{
    HRESULT hr = 0;

    desiredSize.width = desiredSize.height = 0;

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   Arrange
//
//  Synopsis:
//      Position this element in the specified rectangle.
//
//------------------------------------------------------------------------
_Check_return_
HRESULT
CUIElement::Arrange(XRECTF finalRect)
{
    HRESULT hr = S_OK;
    bool bSavedAllowTransitionsToRun = false;
    int count = CLayoutManager::MaxLayoutIterations;

    LockParent(); // all paths exiting this method MUST go through Cleanup, and this line should ALWAYS execute

    CLayoutManager* pLayoutManager = VisualTree::GetLayoutManagerForElement(this);
    IFCPTR(pLayoutManager);

    pLayoutManager->PushCurrentLayoutElement(this);

    bSavedAllowTransitionsToRun = pLayoutManager->GetAllowTransitionsToRunUnderCurrentSubtree();
    pLayoutManager->SetAllowTransitionsToRunUnderCurrentSubtree(bSavedAllowTransitionsToRun && CTransition::GetAllowsTransitionsToRun(this));

    SetIsOnArrangeStack(TRUE);
    SetIsAncestorDirty(FALSE);

    if (GetIsLayoutElement() || GetIsParentLayoutElement())
    {
        IFC(EnsureLayoutStorage()); // Required for constraint comparison
    }

    if (GetUseLayoutRounding())
    {
        finalRect.X = LayoutRound(finalRect.X);
        finalRect.Y = LayoutRound(finalRect.Y);
        finalRect.Width = LayoutRound(finalRect.Width);
        finalRect.Height = LayoutRound(finalRect.Height);
    }

    while (count-- > 0)
    {
        if (pLayoutManager->GetRequiresMeasure())
            break;

        if (GetIsArrangeDirty() || ((GetIsLayoutElement() || GetIsParentLayoutElement()) && HasLayoutStorage() && !IsSameRect(finalRect, FinalRect)))
        {
            bool wasOnArrangeDirtyPath = GetIsOnArrangeDirtyPath();
            SetIsOnArrangeDirtyPath(FALSE);

            IFC(ArrangeInternal(finalRect));

            if (RestoreDirtyPathFlagIfNeeded(wasOnArrangeDirtyPath, false /*isMeasureNotArrange*/))
                break;
        }
        else if (GetIsOnArrangeDirtyPath() && !GetIsLayoutSuspended())
        {
            SetIsOnArrangeDirtyPath(FALSE);

            XUINT32 childIndex = 0;
            CUIElement* pCurrentChild = nullptr;
            CUIElementCollection* pCollectionForUnloading = nullptr;
            UIElementCollectionUnloadingStorage::UnloadingMap::const_iterator unloadIterator;
            bool bShouldProcessUnloadingElements = false;

            auto children = GetUnsortedChildren();
            UINT32 childrenCount = children.GetCount();

            pCollectionForUnloading = GetChildren();

            if (pCollectionForUnloading && pCollectionForUnloading->HasUnloadingStorage() && pCollectionForUnloading->m_pUnloadingStorage->Count() > 0)
            {
                unloadIterator = pCollectionForUnloading->m_pUnloadingStorage->m_unloadingElements.begin();
                bShouldProcessUnloadingElements = TRUE;
            }

            do
            {
                pCurrentChild = nullptr;

                if (childIndex < childrenCount) // regular case: do layout on normal children
                {
                    pCurrentChild = children[childIndex];
                    childIndex++; // for next iteration
                    ASSERT(pCurrentChild);
                }
                else if (bShouldProcessUnloadingElements && unloadIterator != pCollectionForUnloading->m_pUnloadingStorage->m_unloadingElements.end()) // unloading children
                {
                    pCurrentChild = unloadIterator->first;
                    ++unloadIterator;   // for next iteration
                    ASSERT(pCurrentChild);
                }

                if (pCurrentChild != nullptr && pCurrentChild->GetRequiresArrange())
                {
                    IFC(pCurrentChild->Arrange(pCurrentChild->GetProperArrangeRect(finalRect)));
                }

                if (GetIsAncestorDirty())
                {
                    SetIsOnArrangeDirtyPath(TRUE);
                    break;
                }

            } while (pCurrentChild && !pLayoutManager->GetRequiresMeasure());
        }
        else
            break;
    }

Cleanup:
    UnlockParent();

    if (pLayoutManager->GetRequiresMeasure())
    {
        // If the tree is MeasureDirty, we need to set this node's ArrangeDirtyPath flag back to true
        // since some of the children may still be ArrangeDirty.  Note that since Arrange can be called
        // from an application (and not just from layout), we need to ensure that the tree remains in
        // a cosistent state, so we make sure to propagate the flag up the tree if it isn't already
        // there.
        SetIsOnArrangeDirtyPath(TRUE);
        PropagateOnArrangeDirtyPath(true /*stopAtLayoutSuspended*/);
    }

    SetIsOnArrangeStack(FALSE);

    if (pLayoutManager)
    {
        VERIFY_COND(pLayoutManager->PopCurrentLayoutElement(), == this);
        pLayoutManager->SetAllowTransitionsToRunUnderCurrentSubtree(bSavedAllowTransitionsToRun);
    }

    RRETURN(hr);
}

_Check_return_
HRESULT
CUIElement::ArrangeInternal(XRECTF finalRect)
{
    HRESULT hr = S_OK;
    HRESULT hr2 = S_OK;
    XUINT32 fEnteredArrange = FALSE;

    CLayoutManager* pLayoutManager = VisualTree::GetLayoutManagerForElement(this);

    // communicate this layout change
    IFC(CTransition::OnLayoutChanging(this, finalRect));

    IFC(EnsureLayoutStorage());

    if (GetIsLayoutSuspended())
    {
        if (! IsSameRect(finalRect, FinalRect))
        {
            FinalRect = finalRect;
        }
        goto Cleanup;
    }

    // In case this element's parent did not call Measure on this element yet, we call it now. This can occur if a parent calls
    // Arrange on a child directly, and we haven't yet gotten around to measuring the child. The parent can skip calling
    // Measure on a child if it does not care about child's size.
    // Passing finalSize practically means "set size" because that's what Measure(sz)/Arrange(same_sz) means.
    // Note that in case of IsLayoutSuspended (temporarily out of the tree) the MeasureDirty can be true
    // while it does not indicate that we should re-measure - we just performed a Measure that did nothing
    // because of suspension.

    if (GetIsMeasureDirty())
    {
        SetIsMeasureDuringArrange(TRUE);

        if (GetHasBeenMeasured())
        {
            hr = Measure(PreviousConstraint);
        }
        else
        {
            hr = Measure(finalRect.Size());
        }

        SetIsMeasureDuringArrange(FALSE);
        IFC(hr);
    }

    // Bypass - if not dirty and the rect is the same, no need to re-arrange.
    if (!GetIsArrangeDirty() && IsSameRect(finalRect, FinalRect))
    {
        goto Cleanup;
    }

    InvalidateViewport();

    if (ShouldPerformLayout(pLayoutManager, this))
    {
        // we are currently in the LT phase, so we should skip measuring elements that have LT's defined
        // itself

        // this is not the element that is actually triggering this LayoutTransition walk
        // and we do have an LT defined, so we are expected to take care of position ourselves
        // parent should just use the DesiredSize that is defined on this element

        IFC(pLayoutManager->EnterArrange(this));
        fEnteredArrange = TRUE;


        TraceArrangeElementBegin((XUINT64)this, finalRect.X, finalRect.Y, finalRect.Width, finalRect.Height);

        if (pLayoutManager->StoreLayoutCycleWarningContexts())
        {
            if (LayoutCycleDebugSettings::ShouldTrace(DirectUI::LayoutCycleTracingLevel::Level2))
            {
                std::wstring arrangeInfo(L"Arrange(");
                arrangeInfo.append(std::to_wstring(finalRect.X));
                arrangeInfo.append(L",");
                arrangeInfo.append(std::to_wstring(finalRect.Y));
                arrangeInfo.append(L" ");
                arrangeInfo.append(std::to_wstring(finalRect.Width));
                arrangeInfo.append(L"x");
                arrangeInfo.append(std::to_wstring(finalRect.Height));
                arrangeInfo.append(L")");

                std::vector<std::wstring> warningInfo;
                warningInfo.push_back(std::move(arrangeInfo));
                StoreLayoutCycleWarningContext(warningInfo, pLayoutManager);
            }
            if (LayoutCycleDebugSettings::ShouldDebugBreak(DirectUI::LayoutCycleDebugBreakLevel::Level2))
            {
                __debugbreak();
            }
        }

        IFC(ArrangeCore(finalRect));

        TraceArrangeElementEnd((XUINT64)this, VisualOffset.x, VisualOffset.y, RenderSize.width, RenderSize.height);
    }

    if (IsTransformDirty() && CanUseHandOffVisualTransformGroupForLocalTransform())
    {
        // If layout computed a new VisualOffset, and the HandOff visual is in use,
        // forcefully update the HandOff visual transform information to reflect the new VisualOffset.
        // Not doing this will cause GetLocalTransform to return a stale value until after the RenderWalk
        // has pushed this value into the HandOff visual, and a callback is received from the DWM,
        // which can affect apps that use TransformToVisual just after layout.
        auto& handOffVisualDataMap = GetDCompTreeHost()->GetHandOffVisualDataMap();
        auto handOffPair = handOffVisualDataMap.find(this);
        if (handOffPair != handOffVisualDataMap.end())
        {
            Microsoft::WRL::ComPtr<DCompPropertyChangedListener> listener = handOffPair->second.dcompPropertyChangedListener;
            if (listener)
            {
                listener->ForceUpdateFromLayoutOffset();
            }
        }
    }

Cleanup:
    if (fEnteredArrange)
    {
        hr2 = pLayoutManager->ExitArrange(this);
        hr = FAILED(hr) ? hr : hr2;
    }

    if (SUCCEEDED(hr))
    {
        if (FinalRect.Width != finalRect.Width || FinalRect.Height != finalRect.Height)
        {
            // Mark this element's rendering content as dirty.
            CUIElement::NWSetContentDirty(this, DirtyFlags::Render | DirtyFlags::Bounds);
        }

        FinalRect = finalRect;

        // If the element should be clipped do that here.
        // We cannot clip elements in Render, because LayoutUpdated
        // event gets fired BEFORE we do our first render, hence,
        // ClipGeometry info would be incorrect if accessed in the
        // LayoutUpdated event.
        // If the element doesn't require clipping, this call would also
        // clear previous clip geometry (if any).
        hr2 = UpdateLayoutClip(FALSE /*forceClipToRenderSize*/);
        hr = FAILED(hr2) ? hr2 : hr;

        //if some element downstream made the tree measure dirty, then the subsequent
        //elements have not been arranged yet. Keep the arrange dirty flag here so that
        //we arrange those children next time around.
        if (!pLayoutManager->GetRequiresMeasure())
        {
            SetIsArrangeDirty(FALSE);
        }
    }
    // Differs from WPF behavior in that we don't replace the last exception element
    // if one has already been defined.  This ensures that the element that was being
    // laid out when an exception was raised will be the last exception element.
    else if (pLayoutManager->GetLastExceptionElement() == NULL)
    {
        pLayoutManager->SetLastExceptionElement(this);
    }

    // communicate this arrange
    hr2 = CTransition::OnLayoutChanged(this);
    hr = FAILED(hr) ? hr : hr2;

    RRETURN(hr);
}

_Check_return_
HRESULT
CUIElement::ArrangeCore(XRECTF finalRect)
{
    // The offset values are used to make a translation transform in GetLocalTransform.

    IFC_RETURN(EnsureLayoutStorage());

    RenderSize = finalRect.Size();

    if (VisualOffset.x != finalRect.X || VisualOffset.y != finalRect.Y)
    {
        VisualOffset = finalRect.Point();

        // Mark this element's transform as dirty.
        CUIElement::NWSetTransformDirty(this, DirtyFlags::Render | DirtyFlags::Bounds);

    }

    return S_OK;
}

_Check_return_ HRESULT
CUIElement::TransformToGlobalCoordinateSpaceThroughViewports(
    _In_ const bool treatAsViewport,
    _In_ std::vector<TransformToPreviousViewport>& transformsToViewports,
    _Out_ XRECTF& rect)
{
    ASSERT(HasLayoutStorage());
    rect = { 0.0f, 0.0f, RenderSize.width, RenderSize.height };

    // If we're treating this element as a viewport, it means that we care
    // about its clip, if it has one. We intersect the clip with the element
    // itself, and the resulting rect is what we want to transform.
    if (treatAsViewport)
    {
        xref_ptr<CGeometry> clip = GetClip();

        if (clip != nullptr)
        {
            XRECTF_RB bounds;
            IFC_RETURN(clip->GetBounds(&bounds));
            XRECTF clipRect = ToXRectF(bounds);
            IntersectRect(&rect, &clipRect);
        }
    }

    // We want to obtain the coordinates of every viewport in the same
    // frame of reference (i.e. global coordinates) with the intention of
    // simplifying the math needed to calculate the effective viewport.
    // TransformToVisual walks up the visual tree in order to collect the
    // desired transform, but we want to avoid walking up through the same
    // elements again and again for performance reasons. Thus, we solely
    // request the transform of a viewport up to the previous viewport.
    // The transform of the very first viewport does go all the way up to
    // the root, so in the end we have the full chain of transforms, which
    // we can use to take the coordinates of every viewport up to global.
    xref_ptr<CGeneralTransform> transform;
    if (transformsToViewports.empty())
    {
        IFC_RETURN(TransformToVisual(nullptr, &transform));
    }
    else
    {
        IFC_RETURN(TransformToVisual(transformsToViewports.back().GetElement(), &transform));
    }

    XRECTF transformedRect = {};
    IFC_RETURN(transform->TransformRect(rect, &transformedRect));
    rect = transformedRect;

    for (auto it = transformsToViewports.rbegin(); it != transformsToViewports.rend(); ++it)
    {
        const auto& transformToViewport = *it;
        transformedRect = {};
        IFC_RETURN(transformToViewport.GetTransform()->TransformRect(rect, &transformedRect));
        rect = transformedRect;
    }

    // Finally, if we're treating this element as a viewport, we want to
    // collect its transform to the previous viewport.
    if (treatAsViewport)
    {
        transformsToViewports.emplace_back(this, transform);
    }

    return S_OK;
}

_Check_return_ HRESULT
CUIElement::TransformToElementCoordinateSpaceThroughViewports(
    _In_ const std::vector<TransformToPreviousViewport>& transformsToViewports,
    _Inout_ XRECTF& rect)
{
    // We transform a given rect from global coordinates to this element's
    // local coordinates. In order to do so, we take the inverse of the
    // transforms of each viewport in the chain (including the transform from
    // the element in question to the immediately previous viewport) and apply
    // them, starting from the root and walking down.
    xref_ptr<CGeneralTransform> transform;
    if (transformsToViewports.empty())
    {
        IFC_RETURN(TransformToVisual(nullptr, &transform));
    }
    else
    {
        IFC_RETURN(TransformToVisual(transformsToViewports.back().GetElement(), &transform));
    }

    XRECTF transformedRect = {};
    for (const auto& transformToViewport : transformsToViewports)
    {
        xref_ptr<CGeneralTransform> inverse(GetInverseTransform(transformToViewport.GetTransform()));
        IFC_RETURN(inverse->TransformRect(rect, &transformedRect));
        rect = transformedRect;
        transformedRect = {};
    }
    xref_ptr<CGeneralTransform> inverse(GetInverseTransform(transform));
    IFC_RETURN(inverse->TransformRect(rect, &transformedRect));
    rect = transformedRect;

    return S_OK;
}

_Check_return_ HRESULT
CUIElement::EffectiveViewportWalkCore(
    _In_ std::vector<TransformToPreviousViewport>& transformsToViewports,
    _In_ std::vector<UnidimensionalViewportInformation>& horizontalViewports,
    _In_ std::vector<UnidimensionalViewportInformation>& verticalViewports,
    _Out_ bool& addedViewports)
{
    addedViewports = false;

    // We store the transform information of every element that identifies
    // itself as a viewport while we are walking down the visual tree. When we
    // reach an element that is listening to changes in its effective viewport,
    // we will use this information to compute the new values.
    if (IsScroller())
    {
        XRECTF rect = { 0.0f, 0.0f, 0.0f, 0.0f };
        IFC_RETURN(TransformToGlobalCoordinateSpaceThroughViewports(
            true /* treatAsViewport */,
            transformsToViewports,
            rect));
        horizontalViewports.emplace_back(rect.X, rect.Width);
        verticalViewports.emplace_back(rect.Y, rect.Height);
        addedViewports = true;
    }

    return S_OK;
}

_Check_return_ HRESULT
CUIElement::ComputeEffectiveViewportChangedEventArgsAndNotifyLayoutManager(
    _In_ std::vector<TransformToPreviousViewport>& transformsToViewports,
    _In_ const std::vector<UnidimensionalViewportInformation>& horizontalViewports,
    _In_ const std::vector<UnidimensionalViewportInformation>& verticalViewports)
{
    CLayoutManager* layoutManager = VisualTree::GetLayoutManagerForElement(this);
    ASSERT(layoutManager);

    XRECTF rect = { 0.0f, 0.0f, 0.0f, 0.0f };
    IFC_RETURN(TransformToGlobalCoordinateSpaceThroughViewports(
        false /* treatAsViewport */,
        transformsToViewports,
        rect));

    XRECTF ev = { 0.0f, 0.0f, 0.0f, 0.0f };
    ComputeUnidimensionalEffectiveViewport(horizontalViewports, ev.X, ev.Width);
    ComputeUnidimensionalEffectiveViewport(verticalViewports, ev.Y, ev.Height);

    if (ev.X != std::numeric_limits<float>::infinity() && ev.Y != std::numeric_limits<float>::infinity())
    {
        // If we have a valid effective viewport, we now need to transform
        // from global coordinates to the element coordinate space.
        IFC_RETURN(TransformToElementCoordinateSpaceThroughViewports(transformsToViewports, ev));
    }
    else
    {
        // If the effective viewport is invalid in at least one direction,
        // it results in an empty rect.
        ev.X = ev.Y = std::numeric_limits<float>::infinity();
        ev.Width = ev.Height = -std::numeric_limits<float>::infinity();
    }

    XRECTF mv = { 0.0f, 0.0f, 0.0f, 0.0f };
    ComputeUnidimensionalMaxViewport(horizontalViewports, mv.X, mv.Width);
    ComputeUnidimensionalMaxViewport(verticalViewports, mv.Y, mv.Height);

    if (mv.X != std::numeric_limits<float>::infinity() && mv.Y != std::numeric_limits<float>::infinity())
    {
        // If we have a valid max viewport, we now need to transform from
        // global coordinates to the element coordinate space.
        IFC_RETURN(TransformToElementCoordinateSpaceThroughViewports(transformsToViewports, mv));
    }
    else
    {
        // By design, the max viewport should never be an invalid rect.
        ASSERT(false);
        mv.X = mv.Y = std::numeric_limits<float>::infinity();
        mv.Width = mv.Height = -std::numeric_limits<float>::infinity();
    }

    float dx = 0.0f;
    float dy = 0.0f;
    ComputeUnidimensionalBringIntoViewDistance(rect.X, rect.Width, horizontalViewports, dx);
    ComputeUnidimensionalBringIntoViewDistance(rect.Y, rect.Height, verticalViewports, dy);

    CFrameworkElement* frameworkElement = do_pointer_cast<CFrameworkElement>(this);
    ASSERT(frameworkElement);

    // We will not fire the EffectiveViewportChanged event unless the values
    // to report via the event args have, in fact, changed.
    if (!IsSameRect(frameworkElement->GetEffectiveViewport(), ev)
        || !IsSameRect(frameworkElement->GetMaxViewport(), mv)
        || (frameworkElement->GetBringIntoViewDistanceX() != dx)
        || (frameworkElement->GetBringIntoViewDistanceY() != dy))
    {
        IFC_RETURN(layoutManager->EnqueueForEffectiveViewportChanged(frameworkElement, ev, mv, dx, dy));

        // Update the properties with the new values.
        frameworkElement->SetEffectiveViewport(ev);
        frameworkElement->SetMaxViewport(mv);
        frameworkElement->SetBringIntoViewDistanceX(dx);
        frameworkElement->SetBringIntoViewDistanceY(dy);
    }

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Plateau-aware layout rounding utility.
//      Also takes into account ScrollViewer zoom factor when it uses at
//      least one header.
//
//------------------------------------------------------------------------
float CUIElement::LayoutRoundHelper(const float value, _In_ RoundCeilOrFloorFn operationFn)
{
    double returnValue = value;

    // GetScaleFactorForLayoutRounding() returns the plateau scale in most cases. For ScrollContentPresenter children though,
    // the plateau scale gets combined with the owning ScrollViewer's ZoomFactor if headers are present.
    double scaleFactor = GetScaleFactorForLayoutRounding();

    // Plateau scale is applied as a scale transform on the root element. All values computed by layout
    // will be multiplied by this scale. Layout assumes a plateau of 1, and values rounded to
    // integers at layout plateau of 1 will not be integer values when scaled by plateau transform, causing
    // sub-pixel rendering at plateau != 1. To correctly put element edges at device pixel boundaries, layout rounding
    // needs to take plateau into account and produce values that will be rounded after plateau scaling is applied,
    // i.e. multiples of 1/Plateau.
    if (scaleFactor != 1.0)
    {
        returnValue = (operationFn(returnValue * scaleFactor) / scaleFactor);
    }
    else
    {
        // Avoid unnecessary multiply/divide at scale factor 1.
        returnValue = operationFn(returnValue);
    }
    return static_cast<float>(returnValue);
}

XFLOAT CUIElement::LayoutRound(_In_ XFLOAT value)
{
    return LayoutRoundHelper(value, XcpRound);
}

// Does a layout round, except only rounds upwards.
// Used to round the ninegrid insets - we only round up so we don't drop anything from the corner
float CUIElement::LayoutRoundCeiling(const float value)
{
    return LayoutRoundHelper(value, XcpCeiling);
}

// Does a layout round, except only rounds down.
float CUIElement::LayoutRoundFloor(const float value)
{
    return LayoutRoundHelper(value, XcpFloor);
}

// Returns the plateau scale if no global scale factor was stored in sparse storage, or the global scale factor otherwise.
// For now it just combines the plateau scale and the zoom factor of the owning ScrollViewer when headers are present.
FLOAT CUIElement::GetScaleFactorForLayoutRounding()
{
    if (UseGlobalScaleFactorForLayoutRounding() && IsEffectiveValueInSparseStorage(KnownPropertyIndex::UIElement_GlobalScaleFactor))
    {
        return GetGlobalScaleFactorCore();
    }
    else
    {
        return RootScale::GetRasterizationScaleForElement(this);
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Base implementation does not support layout clipping.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElement::UpdateLayoutClip(bool forceClipToRenderSize)
{
    // Forced clipping is handled by the FrameworkElement override.
    ASSERT(!forceClipToRenderSize);

    if (LayoutClipGeometry != NULL)
    {
        ReleaseInterface(LayoutClipGeometry);

        // We went from having a layout clip to having no layout clip.
        SetLayoutClipDirty();
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   OnCreateAutomationPeer
//
//  Synopsis:  Creates an CAutomationPeer by checking each override
//
//------------------------------------------------------------------------
CAutomationPeer*
CUIElement::OnCreateAutomationPeer()
{
    CAutomationPeer* pTempAP = NULL;

    pTempAP = OnCreateAutomationPeerInternal();
    if (m_pAP)
    {
        if (m_pAP != pTempAP)
        {
            m_pAP->InvalidateOwner();
        }
        ReleaseInterface(m_pAP);
    }
    m_pAP = pTempAP;
    return m_pAP;
}

//------------------------------------------------------------------------
//
//  Method:   GetAutomationPeer
//
//  Synopsis:  A Win8 version of OnCreateAutomationPeer. For Win8 scenarios
//  there are many call sites where we don't want to forcibly create the
//  Automation Peer, for performance reasons.
//
//------------------------------------------------------------------------
CAutomationPeer*
CUIElement::GetAutomationPeer()
{
    return m_pAP;
}

//------------------------------------------------------------------------
//
//  Method:   OnCreateAutomationPeerImpl
//
//  Synopsis:  Creates and returns CAutomationPeer associated with this UIE
//
//------------------------------------------------------------------------
CAutomationPeer*
CUIElement::OnCreateAutomationPeerImpl()
{
    return NULL;
}

//------------------------------------------------------------------------
//
//  Method:   GetPopupAssociatedAutomationPeer
//
//  Synopsis: Return the automation peer that associated with the popup.
//
//------------------------------------------------------------------------
CAutomationPeer*
CUIElement::GetPopupAssociatedAutomationPeer()
{
    CAutomationPeer *pPopupAssociatedAP = NULL;
    CDependencyObject *pPopup = NULL;
    CDependencyObject *pVisualParent = this->GetParentInternal(false);

    // The visual parent must be a popup root to get the logical parent of the popup
    if (NULL != do_pointer_cast<CPopupRoot>(pVisualParent))
    {
        CFrameworkElement* pElement = do_pointer_cast<CFrameworkElement>(this);
        if (pElement)
        {
            pPopup = pElement->GetLogicalParentNoRef();
            if (pPopup && pPopup->OfTypeByIndex<KnownTypeIndex::Popup>())
            {
                pPopupAssociatedAP = pPopup->OnCreateAutomationPeer();
           }
        }
    }

    return pPopupAssociatedAP;
}

// Returns count of UIElement CAutomationPeer children of this UIElement.
// This does not include Hyperlink children, which
// are not UIElements and can't be included.
_Check_return_ XUINT32 CUIElement::GetAPChildrenCount()
{
    uint32_t uAPCount = 0;

    // if we are Application.RootVisual then we want to add popups APs to AP count.
    if (this == GetPublicRootVisual())
    {
        uAPCount = GetAPPopupChildrenCount();
    }

    if (m_pChildren)
    {
        uint32_t cKids = m_pChildren->GetCount();

        for (uint32_t i = 0; i < cKids; i++)
        {
            xref_ptr<CUIElement> pChild;
            pChild.attach(do_pointer_cast<CUIElement>(m_pChildren->GetItemDOWithAddRef(i)));

            if ((pChild && DirectUI::Visibility::Visible == pChild->GetVisibility())
                && !ShouldSkipForAPChildren(pChild))
            {
                const CAutomationPeer* pChildAP = pChild->OnCreateAutomationPeer();

                if (pChildAP)
                {
                    uAPCount++;
                }
                else
                {
                    uAPCount += pChild->GetAPChildrenCount();
                }
            }
        }
    }

    return uAPCount;
}

// Returns all CAutomationPeer children associated with this UIE
_Check_return_ XUINT32 CUIElement::GetAPChildren(_Outptr_result_buffer_(return) CAutomationPeer ***pppReturnAP)
{
    uint32_t uAPIndex = 0;
    uint32_t uAPPopupCount = 0;

    const uint32_t uieAPCount = GetAPChildrenCount();
    const uint32_t linkAPCount = GetLinkAPChildrenCount();
    const uint32_t totalAPCount = uieAPCount + linkAPCount;

    if (totalAPCount == 0)
    {
        return totalAPCount;
    }

    CAutomationPeer **ppChildrenAP = nullptr;
    ppChildrenAP = new CAutomationPeer*[totalAPCount];

    // if we are Application.RootVisual then we want to append popups APs to AP array.

    // Note: This places the popups _before_ the real visual roots. They should be after (on top).
    if (this == GetPublicRootVisual())
    {
        uAPPopupCount = GetAPPopupChildrenCount();
        if (uAPPopupCount > uieAPCount)
        {
            // this is a FALSE condition. We want to ASSERT it always.
            ASSERT(FALSE, L"uAPPopupCount > uieAPCount");
            uAPPopupCount = uieAPCount;
        }
        uAPIndex = AppendAPPopupChildren(uAPPopupCount, ppChildrenAP);
        ASSERT(uAPIndex == uAPPopupCount);
    }

    // if popup count and total count is equal then we don't have to iterate
    // for the visual tree children all elements we have is popups only
    if (m_pChildren && uAPPopupCount < uieAPCount)
    {
        uint32_t cKids = m_pChildren->GetCount();

        for (uint32_t i = 0; i < cKids && uAPIndex < uieAPCount; i++)
        {
            xref_ptr<CUIElement> pChild;
            pChild.attach(do_pointer_cast<CUIElement>(m_pChildren->GetItemDOWithAddRef(i)));
            if ((pChild && uAPIndex < uieAPCount && DirectUI::Visibility::Visible == pChild->GetVisibility())
                && !ShouldSkipForAPChildren(pChild))
            {
                CAutomationPeer* pChildAP = pChild->OnCreateAutomationPeer();
                if (pChildAP)
                {
                    ppChildrenAP[uAPIndex++] = pChildAP;
                }
                else
                {
                    uint32_t uAPChildrenCount = 0;

                    uAPChildrenCount = pChild->GetAPChildrenCount();
                    if (uAPChildrenCount > 0)
                    {
                        CAutomationPeer **ppLocalChildrenAP = nullptr;

                        uAPChildrenCount = pChild->GetAPChildren(&ppLocalChildrenAP);

                        if (uAPChildrenCount > 0 && ppLocalChildrenAP)
                        {
                            ASSERT(uAPIndex + uAPChildrenCount <= uieAPCount);
                            for (uint32_t j = 0; j < uAPChildrenCount && uAPIndex < uieAPCount; j++)
                            {
                                ppChildrenAP[uAPIndex++] = ppLocalChildrenAP[j];
                            }
                            delete[] ppLocalChildrenAP;
                        }
                    }
                }
            }
        }
    }

    if (linkAPCount > 0)
    {
        CDOCollection *focusableChildren = nullptr;
        if (this->OfTypeByIndex<KnownTypeIndex::TextBlock>())
        {
            static_cast<CTextBlock*>(this)->GetFocusableChildren(&focusableChildren);
        }
        else if (this->OfTypeByIndex<KnownTypeIndex::RichTextBlock>())
        {
            static_cast<CRichTextBlock*>(this)->GetFocusableChildren(&focusableChildren);
        }
        else if (this->OfTypeByIndex<KnownTypeIndex::RichTextBlockOverflow>())
        {
            static_cast<CRichTextBlockOverflow*>(this)->GetMaster()->GetFocusableChildren(&focusableChildren);
        }
        for (uint32_t i = 0; i < focusableChildren->GetCount() && uAPIndex < totalAPCount; i++)
        {
            xref_ptr<CTextElement> pChild;
            pChild.attach(static_cast<CTextElement*>(focusableChildren->GetItemDOWithAddRef(i)));
            if (pChild && (pChild->OfTypeByIndex<KnownTypeIndex::Hyperlink>()))
            {
                CAutomationPeer* pChildAP = pChild->OnCreateAutomationPeer();
                if (pChildAP)
                {
                    ppChildrenAP[uAPIndex++] = pChildAP;
                }

            }
        }
    }
    *pppReturnAP = ppChildrenAP;

    return totalAPCount;
}

// Should this element be skipped while finding AutomationPeer children?
bool CUIElement::ShouldSkipForAPChildren(_In_ CUIElement *pChild)
{
    // Skip windowed popup, because its window will provide accessibility
    // using WM_GETOBJECT.
    const CPopup *pPopup = do_pointer_cast<CPopup>(pChild);
    if (pPopup && pPopup->IsWindowed())
    {
        return true;
    }

    // We also want to skip elements that satisfy the conditions to be AP popup children
    return !ShouldSkipForAPPopupChildren(pChild, GetPopupRoot());
}

bool CUIElement::ShouldSkipForAPPopupChildren(_In_ CUIElement *pChild, _In_opt_ const CPopupRoot* pPopupRoot) const
{
    CPopup *pPopup = do_pointer_cast<CPopup>(pChild);

    if (!pPopup)
    {
        return true;
    }

    // Skip windowed popup, because its window will provide accessibility
    // using WM_GETOBJECT.
    if (pPopup->IsWindowed())
    {
        return true;
    }

    if (!pPopupRoot)
    {
        // There is no PopupRoot.  This can happening during startup if we get a UIA message
        // before the PopupRoot is set during Application startup.
        return true;
    }

    const CDependencyObject * pLogicalParent = pPopup->GetLogicalParentNoRef();

    // Skip popups that have an active logical parent or whose logical parent is popup.
    if (pLogicalParent == NULL || (!pLogicalParent->IsActive() && !pLogicalParent->OfTypeByIndex<KnownTypeIndex::Popup>()))
    {
        return false;
    }

    return true;
}

//------------------------------------------------------------------------
//
//  Method:   GetAPPopupChildrenCount
//
//  Synopsis:  Returns CAutomationPeer count for Child popup windows
//
//------------------------------------------------------------------------
_Check_return_ XUINT32 CUIElement::GetAPPopupChildrenCount()
{
    return GetAPPopupChildren(0, NULL);
}

//------------------------------------------------------------------------
//
//  Method:   AppendAPPopupChildren
//
//  Synopsis:  Appends CAutomationPeers to the Child collection of AutomationPeers.
//
//------------------------------------------------------------------------
_Check_return_ XUINT32 CUIElement::AppendAPPopupChildren(_In_ XUINT32 nCount, _Inout_updates_(nCount) CAutomationPeer ** ppChildrenAP)
{
    return GetAPPopupChildren(nCount, ppChildrenAP);
}

//------------------------------------------------------------------------
//
//  Synopsis:   Appends CAutomationPeers to the Child collection of AutomationPeers if
//              valid pointer is provided, otherwise returns count of Child popup windows
//
//  Remarks:    If ppChildrenAP array pointer is null we ignore nCount parameter and always return
//              count of popup children. If ppChildrenAP array pointer is not null but nCount is 0
//              (means we want to add 0 elements to the array) we ignore iteration on the visual (PopupRoot)
//              tree and return 0.
//
//------------------------------------------------------------------------
_Check_return_ XUINT32 CUIElement::GetAPPopupChildren(_In_ XUINT32 nCount, _Inout_updates_opt_(nCount) CAutomationPeer ** ppChildrenAP)
{
    XUINT32 uAPCount = 0;

    CPopupRoot* pPopupRoot = GetPopupRoot();
    if (!pPopupRoot)
    {
        // There is no PopupRoot.  This can happening during startup if we get a UIA message
        // before the PopupRoot is set during Application startup.  In this case, we just return
        // 0 for the number of Popup AP children.
        return 0;
    }

    CDOCollection *pCollection = static_cast<CDOCollection*>(pPopupRoot->GetChildren());

    if (pCollection && (ppChildrenAP == NULL || nCount > 0))
    {
        XUINT32 cKids = pCollection->GetCount();

        // When we are actually creating APs not just counting (ppChildrenAP != nullptr) then
        // we can have a case where uAPCount == nCount and we still have popups to process.
        // It was not the case earlier when we didn't skip any popups while counting but as you see below
        // we now skip popups that are either windowed or have logical parent. Hence the assert was wrong
        // and we need to accommodate this case in our logic.
        for (XUINT32 i = 0; (i < cKids) && (ppChildrenAP == nullptr || uAPCount < nCount); i++)
        {
            CFrameworkElement *pChild = do_pointer_cast<CFrameworkElement>(pCollection->GetItemDOWithAddRef(i));
            if (pChild)
            {
                CPopup * pPopup = do_pointer_cast<CPopup>(pChild->GetLogicalParentNoRef());
                // All logical parents of PopupRoot collection must be Popups.
                ASSERT(pPopup != NULL);

                // More than one visual child of the popup root may have the same logical parent popup.
                // That is, when a popup opens, it not only adds its child to the popup root, but it also adds its
                // light dismiss overlay element, if that was set. Since we shouldn't add a popup to the AP collection
                // here more than once, we ignore elements that aren't the child of their corresponding popup parent.

                if (!ShouldSkipForAPPopupChildren(pPopup, pPopupRoot) && static_cast<CUIElement*>(pChild) == pPopup->m_pChild)
                {
                    CAutomationPeer* pPopupAP = pPopup->OnCreateAutomationPeer();
                    if (pPopupAP)
                    {
                        if (ppChildrenAP != NULL && uAPCount < nCount)
                        {
                            ppChildrenAP[uAPCount] = pPopupAP;
                        }
                        uAPCount++;
                    }
                }
            }
            ReleaseInterface(pChild);
        }

        // Popup is active and dismiss type is light dismiss
        // PopupRoot is listening to pointer down in this case
        // PopupRoot automation peer is active now
        // The light-dismiss layer is a visual child of XAML island roots,
        // so we won't special-case adding it to the AP children here
        // in that circumstance.
        if (uAPCount > 0 && pPopupRoot->IsTopmostPopupInLightDismissChain() && !OfTypeByIndex<KnownTypeIndex::XamlIslandRoot>())
        {
            CAutomationPeer* pPopupRootAP = pPopupRoot->OnCreateAutomationPeer();
            if (pPopupRootAP)
            {
                if (ppChildrenAP != NULL && uAPCount < nCount)
                {
                    ppChildrenAP[uAPCount] = pPopupRootAP;
                }
                uAPCount++;
            }
        }

    }
    return uAPCount;
}

// Checks the automatic UIAutomation properties for changes, and fires changed
// events for them.
_Check_return_ HRESULT CUIElement::CheckAutomaticAutomationChanges()
{
    // If you are debugging this tree walk, chances are you
    // are dealing with https://task.ms/12334058, which refers to this function
    // executing a tree walk that calls into app code without protecting from
    // live visual tree changes. If so, keep reading.
    //
    // CUIElement::CheckAutomaticAutomationChanges, which is called by
    // CLayoutManager::UpdateLayout, has multiple design flaws which result in
    // incorrect and unpredictable behavior as well as non-trivial perf costs.
    // When making this fix, keep the following things in mind:
    //
    // 1. The tree walk relies on following dirty flags similar to Measure,
    // Arrange, and EffectiveViewport. In this case, there is the
    // LF_AUTOMATION_PEER_DIRTY and the LF_ON_AUTOMATION_PEER_DIRTY_PATH flags.
    // Unfortunately, there's a bug in CUIElement::PropagateOnAutomationPeerDirtyPath
    // which causes both, the dirty path flag and the dirty flag itself to get
    // propagated to all the ancestors of a dirty UIElement.
    // This has several negative implications: i) All elements in the chain
    // will be processed instead of only processing those that are legitimately
    // dirty, and ii) the dirty path flag has essentially no purpose.
    //
    // 2. Contrary to the walks done by Measure, Arrange and EffectiveViewport,
    // which protect the visual tree from changing by calling
    // CUIElement::LockParent, this tree walk allows app code to modify the
    // visual tree in the middle of the traversal. This means that, depending
    // on how the tree is changed, we might not be able to process all the
    // necessary elements. In fact, because there are no guarantees at all, it
    // is possible to change the tree in a way that leaves us in a bad state
    // which results in completely erroneous behavior.
    //
    // 3. If app code invalidates the AutomationPeer of a UIElement in a branch
    // that we already walked, this UIElement will not be processed until the
    // next Tick.
    //
    // I haven't been able to corroborate if this approach is appropriate, but
    // we should consider fixing this issue by removing the tree walk entirely.
    // My hypothesis is that, instead of using flags, it might be possible to
    // simply queue the UIElements when we call
    // CUIElement::InvalidateAutomationPeerDataInternal, and then process this
    // queue as many times as needed until it is empty at the end of
    // CLayoutManager::UpdateLayout instead of walking the tree. Not only does
    // this fix all the design flaws listed above, but it also saves an *entire
    // tree walk during layout*. The main counterargument to this fix is that
    // the order in which elements are processed changes (i.e. resolved in
    // order of invalidation instead of tree order), and this might be
    // unacceptable; verify with the appropriate owners before making this change.
    //
    // Update: I was able to extract the StructureChanged event from
    // this function by using the solution I described above. It should be
    // possible to do the same for the properties involved in
    // CAutomationPeer::RaiseAutomaticPropertyChanges. Consider moving these to
    // the AutomationEventsHelper class in a similar way.

    CLayoutManager *pLayoutManager = VisualTree::GetLayoutManagerForElement(this);

    IFCEXPECT_ASSERT_RETURN(pLayoutManager);

    if (GetIsAutomationPeerDirty())
    {
        if (pLayoutManager->GetUIAClientsListeningToProperty())
        {
            CAutomationPeer *pAP = GetAutomationPeer();
            if (pAP)
            {
                pAP->RaiseAutomaticPropertyChanges(m_fireUIAPropertyChanges);
                m_fireUIAPropertyChanges = TRUE;
            }
        }
    }

    SetIsAutomationPeerDirty(FALSE);
    SetIsOnAutomationPeerDirtyPath(FALSE);

    // This tree walk does not prevent visual tree changes, so children
    // could be added or removed during this walk. Guard against problems
    // due to changes by always checking the current children count.
    CUIElementCollection *children = GetChildren();
    if (children)
    {
        const XUINT32 initialChildrenCount = children->GetCount();
        for (XUINT32 childIndex = 0; childIndex < std::min(initialChildrenCount, children->GetCount()); childIndex++)
        {
            CUIElement* currentChild = static_cast<CUIElement*>(children->GetCollection()[childIndex]);
            if (currentChild != nullptr)
            {
                if (currentChild->GetIsAutomationPeerDirty() || currentChild->GetIsOnAutomationPeerDirtyPath())
                {
                    IFC_RETURN(currentChild->CheckAutomaticAutomationChanges());
                }
            }
        }
    }

    return S_OK;
}

// Consider calling this event only if there is a UIA client listening to the StructureChanged
// automation event.
void CUIElement::RegisterForStructureChangedEvent(
    AutomationEventsHelper::StructureChangedType type)
{
    // Historically, XAML has never fired StructureChanged events in response to reorder
    // operations.
    ASSERT(type != AutomationEventsHelper::StructureChangedType::Reordered);

    // Don't register for structure changed for LayoutTransitionElements. They are not exposed via the automation
    // tree. Making this call and creating an automation peer also has a side effect of creating a managed peer
    // for the LTE. Nothing will reference the managed peer, and when it gets deleted, it will also clear out the
    // sparse storage on the native CLayoutTransitionElement, which can contain WUC expressions currently in use.
    if (!OfTypeByIndex<KnownTypeIndex::LayoutTransitionElement>())
    {
        CAutomationPeer * automationPeer = nullptr;

        if (type == AutomationEventsHelper::StructureChangedType::Added)
        {
            automationPeer = OnCreateAutomationPeer();
        }
        else if (type == AutomationEventsHelper::StructureChangedType::Removed)
        {
            // Get the AutomationPeer for this element if it exists already. We intentionally do not
            // create one in order to avoid a teardown problem where the owner is already partially
            // destructed.
            automationPeer = GetAutomationPeer();
        }

        // If the AutomationPeer does exist, register it to have the the StructureChanged automation
        // event fired for it.
        if (automationPeer)
        {
            xref_ptr<CAutomationPeer> automationPeerParent;
            xref_ptr<IUnknown> parentNativeNode;

            HRESULT hr = automationPeer->Navigate(
                UIAXcp::AutomationNavigationDirection_Parent,
                automationPeerParent.ReleaseAndGetAddressOf(),
                parentNativeNode.ReleaseAndGetAddressOf());

            if (hr == S_OK && automationPeerParent)
            {
                GetContext()->RegisterForStructureChangedEvent(
                    automationPeer,
                    automationPeerParent,
                    type);
            }
        }
    }
}

_Check_return_ HRESULT
CUIElement::SetAutomationPeer(_In_ CAutomationPeer* pAP)
{
    if(m_pAP == NULL)
    {
        IFCEXPECT_ASSERT_RETURN(pAP);
        m_pAP = pAP;
        AddRefInterface(static_cast<CAutomationPeer*>(m_pAP));
    }

    return S_OK;
}

_Check_return_ HRESULT GetProgrammaticHitTestingParams(
    _In_ CCoreServices* coreServices,
    _In_ CUIElement* uiElement,
    _Outptr_ ITransformer** transformer,
    _Outptr_ CUIElement** ppThisNoRef,
    _Outptr_ CPopupRoot** popupRootNoRef,
    _Outptr_ CUIElement** popupHitTestSubtreeRootNoRef,
    _Out_ bool* isRootNull)
{
    if (coreServices->GetInitializationType() == InitializationType::IslandsOnly)
    {
        // When an app is running in an islands-only context, it must specify a UIElement to scope down the subtree
        // that we're hit testing against. Otherwise there is no way to tell which island it's talking about. Each
        // island has its own coordinate space, so we can't just pass the same coordinates to each island and expect
        // something that makes sense.
        //
        // Note that we do not make an exception for the special case where there's only one island. The app must
        // still specify that island. This way the VisualTreeHelper::FindElementsInHostCoordinates API has more
        // consistent behavior and will not be suddenly broken if the app adds another Xaml island.
        if (uiElement == nullptr)
        {
            IFC_RETURN(DirectUI::ErrorHelper::OriginateErrorUsingResourceID(E_INVALIDARG, ERROR_HIT_TEST_IS_NOT_ASSOCIATED_WITH_CONTENT_TREE));
        }

        // Do not return a transformer. There's no need to transform the incoming coordinates into the island's
        // coordinate space. The island itself does not apply a plateau scale.
        *transformer = nullptr;

        *ppThisNoRef = uiElement;

        VisualTree* visualTree = VisualTree::GetForElementNoRef(uiElement);
        if (visualTree != nullptr)
        {
            *popupRootNoRef = visualTree->GetPopupRoot();
        }
        else
        {
            *popupRootNoRef = nullptr;
        }

        // An app can't access the true root of the Xaml island to pass in. So if they pass in the Content property
        // (the public root), treat it as if they specified the XamlIslandRoot for popup hit testing. This allows
        // programmatic hit testing to hit parentless popups, which are under the XamlIslandRoot but not under the
        // Content element.
        CDependencyObject* uiElementAncestor = uiElement->GetParentInternal(false /* public parent only */);
        if (uiElementAncestor != nullptr && uiElementAncestor->GetTypeIndex() == KnownTypeIndex::XamlIslandRoot)
        {
            *popupHitTestSubtreeRootNoRef = static_cast<CUIElement*>(uiElementAncestor);
        }
        else
        {
            *popupHitTestSubtreeRootNoRef = uiElement;
        }

        *isRootNull = false;
    }
    else
    {
        CUIElement* pThis = nullptr;

        if (uiElement == nullptr)
        {
            // If nothing was passed in, we will assume the visual root.

            {
                pThis = do_pointer_cast<CUIElement>(coreServices->getVisualRoot());
            }

            if (pThis == NULL)
            {
                *isRootNull = true;
                IFC_RETURN(E_FAIL);
            }
        }
        else
        {
            pThis = uiElement;
        }

        ASSERT(coreServices == pThis->GetContext());

        // Use the transform on the internal root visual to account for plateau scale.
        CRootVisual* rootVisual = VisualTree::GetRootForElement(pThis);
        if (rootVisual != nullptr)
        {
            IFC_RETURN(rootVisual->TransformToRoot(transformer));
            *popupRootNoRef = rootVisual->GetAssociatedPopupRootNoRef();
        }

        *ppThisNoRef = pThis;

        if (*popupRootNoRef == nullptr)
        {
            *popupRootNoRef = coreServices->GetMainPopupRoot();
        }

        // For popups, if nothing is passed in we will not filter
        *popupHitTestSubtreeRootNoRef = uiElement ? pThis : nullptr;

        if (rootVisual == nullptr)
        {
            *isRootNull = true;
            IFC_RETURN(E_FAIL);
        }
        else
        {
            *isRootNull = false;
        }
    }

    return S_OK;
}

namespace CoreImports
{
    //------------------------------------------------------------------------
    //
    //  Synopsis:
    //      Public export for the Point case
    //
    //  Notes:
    //      The coordinates for ptHit are absolute coordinates
    //      (relative to RootVisual) and not relative to pvUIElement.
    //
    //      When passing null as pvUIElement, will assume the visual
    //      root and not filter popups (so unrooted popups will
    //      be included).
    //
    //------------------------------------------------------------------------
    _Check_return_ HRESULT UIElement_HitTestPoint(
        _In_opt_ CUIElement* uiElement,
        _In_ CCoreServices* coreServices,
        _In_ XPOINTF ptHit,
        _In_ bool canHitDisabledElements,
        _In_ bool canHitInvisibleElements,
        _Out_ XINT32* pnCount,
        _Outptr_result_buffer_(*pnCount) CUIElement*** ppElements)
    {
        CUIElement *pThisNoRef = nullptr;
        CHitTestResults oResults;
        XUINT32 nResults = 0;
        bool hitTestConstrainedToPopup = false;
        xref_ptr<ITransformer> transformer;
        CPopupRoot* popupRootNoRef = nullptr;
        CUIElement* popupHitTestSubtreeRootNoRef = nullptr;
        bool isRootNull_dontCare;

        IFCPTR_RETURN(pnCount);
        IFCPTR_RETURN(ppElements);

        IFC_RETURN(GetProgrammaticHitTestingParams(
            coreServices,
            uiElement,
            transformer.ReleaseAndGetAddressOf(),
            &pThisNoRef,
            &popupRootNoRef,
            &popupHitTestSubtreeRootNoRef,
            &isRootNull_dontCare));

        if (transformer != nullptr)
        {
            // Use the transform on the internal root visual to account for plateau scale.
            IFC_RETURN(transformer->Transform(&ptHit, &ptHit, 1));
        }

        if (popupRootNoRef)
        {
            HitTestPerfData hitTestPerfData;
            HitTestParams hitTestParams(&hitTestPerfData);

            hitTestParams.SaveWorldSpaceHitTarget(ptHit);

            IFC_RETURN(popupRootNoRef->HitTestPopups(
                ptHit,
                &hitTestParams,
                true, /*canHitMultipleElements*/
                popupHitTestSubtreeRootNoRef,
                canHitDisabledElements,
                canHitInvisibleElements,
                &oResults));
            hitTestConstrainedToPopup = (CPopup::GetClosestPopupAncestor(popupHitTestSubtreeRootNoRef) != nullptr);
        }

        // If we filtered to a subtree within a Popup, we skip the main hit test path
        // to avoid getting duplicate entries. This is because while a normal hit test
        // walk will stop at Popup boundaries, it will merrily proceed along if the
        // walk starts from within a Popup's subtree, and we already did this hit test
        // walk above.
        if (!hitTestConstrainedToPopup)
        {
            bool transformedPoint = true;

            HitTestPerfData hitTestPerfData;
            HitTestParams hitTestParams(&hitTestPerfData);
            hitTestParams.SaveWorldSpaceHitTarget(ptHit);

            // Since the coordinates at this point are relative to the internal root visual,
            // all transforms between the internal root visual and the subtree (pThis)
            // must be pushed onto the transform stack before we start calling
            // the public hit test API on pThis element.
            CUIElement* pThisParent = do_pointer_cast<CUIElement>(pThisNoRef->GetParentInternal());
            if (pThisParent)
            {
                // TODO: HWPC: Consider leaving bounds stale and only updating layout+bounds during Tick
                // Bounds are dependent on layout, so layout must be updated first.
                IFC_RETURN(pThisParent->UpdateLayout());

                pThisNoRef->PrepareHitTestParamsStartingHere(hitTestParams, ptHit);
            }

            if (transformedPoint)
            {
                IFC_RETURN(pThisNoRef->HitTestEntry(hitTestParams, ptHit, true /*canHitMultipleElements*/, canHitDisabledElements, canHitInvisibleElements, &oResults));
            }
        }

        // Get the results into a list
        CUIElement **ppResults = nullptr;
        auto guard = wil::scope_exit([&ppResults]()
        {
            delete [] ppResults;
        });

        IFC_RETURN(oResults.GetAnswer(&nResults, &ppResults));

        *ppElements = ppResults;
        *pnCount = nResults;
        guard.release();

        return S_OK;
    }

    //------------------------------------------------------------------------
    //
    //  Synopsis:
    //      Public export for the Rect case
    //
    //  Notes:
    //      The coordinates for ptHit are absolute coordinates
    //      (relative to RootVisual) and not relative to pvUIElement.
    //
    //      When passing null as pvUIElement, will assume the visual
    //      root and not filter popups (so unrooted popups will
    //      be included).
    //
    //------------------------------------------------------------------------
    _Check_return_ HRESULT UIElement_HitTestRect(
        _In_opt_ CUIElement* uiElement,
        _In_ CCoreServices* coreServices,
        _In_ XRECTF rcHit,
        _In_ bool canHitDisabledElements,
        _In_ bool canHitInvisibleElements,
        _Out_ XINT32 *pnCount,
        _Outptr_result_buffer_(*pnCount) CUIElement*** ppElements,
        _Out_ bool *isRootNull)
    {
        CUIElement *pThisNoRef = nullptr;
        CHitTestResults oResults;
        XUINT32 nResults = 0;
        XPOINTF ptsHitRect[4];
        bool hitTestConstrainedToPopup = false;
        xref_ptr<ITransformer> transformer;
        CPopupRoot* popupRootNoRef;
        CUIElement* popupHitTestSubtreeRootNoRef = nullptr;

        IFCPTR_RETURN(pnCount);
        IFCPTR_RETURN(ppElements);

        IFC_RETURN(GetProgrammaticHitTestingParams(
            coreServices,
            uiElement,
            transformer.ReleaseAndGetAddressOf(),
            &pThisNoRef,
            &popupRootNoRef,
            &popupHitTestSubtreeRootNoRef,
            isRootNull));

        if (transformer != nullptr)
        {
            // Use the transform on the internal root visual to account for plateau scale.
            FillPointsFromRectCCW(&ptsHitRect[0], rcHit);
            IFC_RETURN(transformer->Transform(&ptsHitRect[0], &ptsHitRect[0], 4));
            if (!FillRectFromPointsCCW(ptsHitRect, rcHit))
            {
                // We don't expect the transform to make us non-rectangular.
                ASSERT(FALSE);
                IFC_RETURN(E_FAIL);
            }
        }

        if (popupRootNoRef)
        {
            HitTestPolygon polygonHit;
            polygonHit.SetRect(rcHit);

            HitTestPerfData hitTestPerfData;
            HitTestParams hitTestParams(&hitTestPerfData);
            hitTestParams.SaveWorldSpaceHitTarget(polygonHit);

            IFC_RETURN(popupRootNoRef->HitTestPopups(
                polygonHit,
                &hitTestParams,
                true, /*canHitMultipleElements*/
                popupHitTestSubtreeRootNoRef,
                canHitDisabledElements,
                canHitInvisibleElements,
                &oResults));
            hitTestConstrainedToPopup = (CPopup::GetClosestPopupAncestor(popupHitTestSubtreeRootNoRef) != nullptr);
        }

        // If we filtered to a subtree within a Popup, we skip the main hit test path
        // to avoid getting duplicate entries. This is because while a normal hit test
        // walk will stop at Popup boundaries, it will merrily proceed along if the
        // walk starts from within a Popup's subtree, and we already did this hit test
        // walk above.
        if (!hitTestConstrainedToPopup)
        {
            bool transformedRect = true;

            HitTestPolygon hitPolygon;
            hitPolygon.SetRect(rcHit);

            HitTestPerfData hitTestPerfData;
            HitTestParams hitTestParams(&hitTestPerfData);
            hitTestParams.SaveWorldSpaceHitTarget(hitPolygon);

            // Since the coordinates at this point are relative to the internal root visual,
            // all transforms between the internal root visual and the subtree (pThis)
            // must be pushed onto the transform stack before we start calling
            // the public hit test API on pThis element.
            CUIElement* pThisParent = do_pointer_cast<CUIElement>(pThisNoRef->GetParentInternal());
            if (pThisParent)
            {
                // TODO: HWPC: Consider leaving bounds stale and only updating layout+bounds during Tick
                // Bounds are dependent on layout, so layout must be updated first.
                IFC_RETURN(pThisParent->UpdateLayout());

                pThisNoRef->PrepareHitTestParamsStartingHere(hitTestParams, hitPolygon);
            }

            if (transformedRect)
            {
                IFC_RETURN(pThisNoRef->HitTestEntry(
                    hitTestParams,
                    hitPolygon,
                    true, /*canHitMultipleElements*/
                    canHitDisabledElements,
                    canHitInvisibleElements,
                    &oResults
                    ));
            }
        }

        // Get the results into a list
        CUIElement **ppResults = nullptr;

        auto guard = wil::scope_exit([&ppResults]()
        {
            delete [] ppResults;
        });

        IFC_RETURN(oResults.GetAnswer(&nResults, &ppResults));

        *ppElements = ppResults;
        *pnCount = nResults;
        guard.release();

        return S_OK;
    }

    //------------------------------------------------------------------------
    //
    //  Method:   CoreImports::UIElement_DeleteList
    //
    //  Synopsis:
    //      Public export for deleting an array of UIElements
    //
    //------------------------------------------------------------------------
    _Check_return_
    HRESULT
    UIElement_DeleteList(
        _In_ void *pvUIElement,
        _In_ XINT32 nResults )
    {
        CDependencyObject **ppResults = static_cast<CDependencyObject **>(pvUIElement);

        if( ppResults )
        {
            // Release individual results
            for ( XINT32 i=0; i<nResults; i++ )
            {
                CDependencyObject *pDO = ppResults[i];
                ReleaseInterface(pDO);
            }
        }

        // delete the array
        delete [] ppResults;

        RRETURN(S_OK);
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Property method, get/set the bitfield.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElement::HitTestVisible(
    _In_ CDependencyObject *pObject,
    _In_ XUINT32 cArgs,
    _Inout_updates_(cArgs) CValue *ppArgs,
    _In_opt_ IInspectable* pValueOuter,
    _Out_ CValue *pResult
    )
{
    BitfieldGetFn getFn = &CUIElement::GetIsHitTestVisible;
    BitfieldSetFn setFn = &CUIElement::SetIsHitTestVisible;
    RRETURN(GetOrSetBitfield(
        pObject,
        cArgs,
        ppArgs,
        getFn,
        setFn,
        pResult
        ));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Property method, get/set the bitfield.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElement::AllowDrop(
    _In_ CDependencyObject *pObject,
    _In_ XUINT32 cArgs,
    _Inout_updates_(cArgs) CValue *ppArgs,
    _In_opt_ IInspectable* pValueOuter,
    _Out_ CValue *pResult
    )
{
    BitfieldGetFn getFn = &CUIElement::GetAllowDrop;
    BitfieldSetFn setFn = &CUIElement::SetAllowDrop;
    RRETURN(GetOrSetBitfield(
        pObject,
        cArgs,
        ppArgs,
        getFn,
        setFn,
        pResult
        ));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Property method, get/set the bitfield.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElement::IsTapEnabled(
    _In_ CDependencyObject *pObject,
    _In_ XUINT32 cArgs,
    _Inout_updates_(cArgs) CValue *ppArgs,
    _In_opt_ IInspectable* pValueOuter,
    _Out_ CValue *pResult
    )
{
    BitfieldGetFn getFn = &CUIElement::GetIsTapEnabled;
    BitfieldSetFn setFn = &CUIElement::SetIsTapEnabled;
    RRETURN(GetOrSetBitfield(
        pObject,
        cArgs,
        ppArgs,
        getFn,
        setFn,
        pResult
        ));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Property method, get/set the bitfield.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElement::IsDoubleTapEnabled(
    _In_ CDependencyObject *pObject,
    _In_ XUINT32 cArgs,
    _Inout_updates_(cArgs) CValue *ppArgs,
    _In_opt_ IInspectable* pValueOuter,
    _Out_ CValue *pResult
    )
{
    BitfieldGetFn getFn = &CUIElement::GetIsDoubleTapEnabled;
    BitfieldSetFn setFn = &CUIElement::SetIsDoubleTapEnabled;
    RRETURN(GetOrSetBitfield(
        pObject,
        cArgs,
        ppArgs,
        getFn,
        setFn,
        pResult
        ));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Property method, get/set the bitfield.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElement::IsRightTapEnabled(
    _In_ CDependencyObject *pObject,
    _In_ XUINT32 cArgs,
    _Inout_updates_(cArgs) CValue *ppArgs,
    _In_opt_ IInspectable* pValueOuter,
    _Out_ CValue *pResult
    )
{
    BitfieldGetFn getFn = &CUIElement::GetIsRightTapEnabled;
    BitfieldSetFn setFn = &CUIElement::SetIsRightTapEnabled;
    RRETURN(GetOrSetBitfield(
        pObject,
        cArgs,
        ppArgs,
        getFn,
        setFn,
        pResult
        ));
}

_Check_return_ HRESULT CUIElement::IsTabStopPropertyGetterSetter(
    _In_ CDependencyObject *pObject,
    _In_ XUINT32 cArgs,
    _Inout_updates_(cArgs) CValue *ppArgs,
    _In_opt_ IInspectable* pValueOuter,
    _Out_ CValue *pResult)
{
    BitfieldGetFn getFn = &CUIElement::IsTabStop;
    BitfieldSetFn setFn = &CUIElement::SetIsTabStop;
    RRETURN(GetOrSetBitfield(
        pObject,
        cArgs,
        ppArgs,
        getFn,
        setFn,
        pResult
        ));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Property method, get/set the bitfield.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElement::IsHoldEnabled(
    _In_ CDependencyObject *pObject,
    _In_ XUINT32 cArgs,
    _Inout_updates_(cArgs) CValue *ppArgs,
    _In_opt_ IInspectable* pValueOuter,
    _Out_ CValue *pResult
    )
{
    BitfieldGetFn getFn = &CUIElement::GetIsHoldEnabled;
    BitfieldSetFn setFn = &CUIElement::SetIsHoldEnabled;
    RRETURN(GetOrSetBitfield(
        pObject,
        cArgs,
        ppArgs,
        getFn,
        setFn,
        pResult
        ));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Property method, get/set the bitfield.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElement::CanDrag(
    _In_ CDependencyObject *pObject,
    _In_ XUINT32 cArgs,
    _Inout_updates_(cArgs) CValue *ppArgs,
    _In_opt_ IInspectable* pValueOuter,
    _Out_ CValue *pResult
    )
{
    BitfieldGetFn getFn = &CUIElement::GetCanDrag;
    BitfieldSetFn setFn = &CUIElement::SetCanDrag;
    RRETURN(GetOrSetBitfield(
        pObject,
        cArgs,
        ppArgs,
        getFn,
        setFn,
        pResult
        ));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Helper for property methods that get/set a bitfield.
//      Caller needs to provide a simple Get/Set member function for the bit field's storage.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElement::GetOrSetBitfield(
    _In_ CDependencyObject *pObject,
    _In_ XUINT32 cArgs,
    _Inout_updates_(cArgs) CValue *ppArgs,
    _In_ BitfieldGetFn pGetFn,
    _In_ BitfieldSetFn pSetFn,
    _Out_ CValue *pResult
    )
{
    CUIElement *pUIElement = NULL;
    bool fNewValue = false;

    IFC_RETURN(DoPointerCast(pUIElement, pObject));

    if (cArgs == 1 && ppArgs)
    {
        bool fChanged = false;
        bool fOldValue = (pUIElement->*pGetFn)();

        ASSERT(ppArgs->GetType() == valueBool || ppArgs->GetType() == valueObject);

        // We are setting the value...
        if (ppArgs->GetType() == valueBool)
        {
            fNewValue = (ppArgs->AsBool() != 0);
        }
        else if (ppArgs->GetType() == valueObject)
        {
            if ((ppArgs->AsObject())->GetClassInformation()->IsEnum())
            {
                const auto pDO = static_cast<const CEnumerated*>(ppArgs->AsObject());
                fNewValue = (pDO->m_nValue != 0) ? TRUE : FALSE;
            }
            else
            {
                IFC_RETURN(E_INVALIDARG);
            }
        }
        else
        {
            IFC_RETURN(E_INVALIDARG);
        }

        if (fNewValue != fOldValue)
        {
            (pUIElement->*pSetFn)(fNewValue);
            fChanged = TRUE;
        }

        if (pResult)
        {
            pResult->SetBool(!!fChanged);
        }
    }
    else if (pResult)
    {
       // We are getting the value
       pResult->SetBool(!!(pUIElement->*pGetFn)());
    }
    else
    {
        IFC_RETURN(E_INVALIDARG);
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method: UseLayoutRounding
//
//  Synopsis:
//      Property method, get/set the bitfield.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CUIElement::UseLayoutRounding(
    _In_ CDependencyObject *pObject,
    _In_ XUINT32 cArgs,
    _Inout_updates_(cArgs) CValue *ppArgs,
    _In_opt_ IInspectable* pValueOuter,
    _Out_ CValue *pResult
)
{
    CUIElement *pUIElement = NULL;
    bool fValue = false;

    IFC_RETURN(DoPointerCast(pUIElement, pObject));

    if (cArgs == 1 && ppArgs)
    {
        ASSERT(ppArgs->GetType() == valueBool || ppArgs->GetType() == valueObject);

        if (ppArgs->GetType() == valueBool)
        {
            fValue = ppArgs->AsBool();
        }
        else if (ppArgs->GetType() == valueObject)
        {
            if ((ppArgs->AsObject())->GetClassInformation()->IsEnum())
            {
                const auto pDO = static_cast<const CEnumerated*>(ppArgs->AsObject());
                fValue = !!pDO->m_nValue;
            }
            else
            {
                IFC_RETURN(E_INVALIDARG);
            }
        }
        else
        {
            IFC_RETURN(E_INVALIDARG);
        }

        pUIElement->SetUseLayoutRounding(fValue);
        if (!pUIElement->ParserOwnsParent())
        {
            IFC_RETURN(pUIElement->PushUseLayoutRounding(fValue));
        }
    }
    else if (cArgs == 0 && pResult)
    {
       // We are getting the value
       pResult->SetBool(pUIElement->GetUseLayoutRounding());
    }
    else
    {
        IFC_RETURN(E_INVALIDARG);
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method: CoerceIsEnabled
//
//------------------------------------------------------------------------

_Check_return_
HRESULT
CUIElement::CoerceIsEnabled(_In_ bool bIsEnabled, _In_ bool bCoerceChildren)
{
    bool bCoerce = false;

    // Does element need to be coerced?
    if (bIsEnabled == false)
    {
        // Need to coerce to false, only if currently enabled.
        if (IsEnabled())
        {
            bCoerce = true;
        }
    }
    else
    {
        // Can coerce to true, only if local value is true.
        if (GetIsEnabled())
        {
            // Need to coerce to true, only if currently disabled.
            if (!IsEnabled())
            {
                bCoerce = true;
            }
        }
    }

    // Coerce to enabled or disabled
    if (bCoerce)
    {
        const CDependencyProperty *pdp = NULL;
        CValue value;

        pdp = MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::Control_IsEnabled);

        SetCoercedIsEnabled(bIsEnabled);

        // Raise public changed event, currently only exposed on Control.
        value.SetBool(bIsEnabled);
        IFC_RETURN(RaiseIsEnabledChangedEvent(&value));

        // Notify property changed
        IFC_RETURN(NotifyPropertyChanged(PropertyChangedParams(pdp, CValue::Empty(), value)));

        // Discard the potential rejection viewports within this disabled element's subtree
        IFC_RETURN(DiscardRejectionViewportsInSubTree());

        if (!bIsEnabled)
        {
            // Process the PointerExited event if the pointer entered element is disabled.
            IFC_RETURN(ProcessPointerExitedEventByPointerEnteredElementStateChange());
        }

        if (bCoerceChildren)
        {
           // Enable/Disable our visual tree.
           IFC_RETURN(CoerceIsEnabledOnVisualChildren(bIsEnabled));
        }

        if (IsActive())
        {
            const auto contentRoot = VisualTree::GetContentRootForElement(this);
            const auto& akExport = contentRoot->GetAKExport();

            if (akExport.IsActive())
            {
                IFC_RETURN(akExport.OnIsEnabledChanged(this, bIsEnabled));
            }
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method: CoerceForceInheritPropertiesOnVisualChildren
//
//  Synopsis:
//     Coerce force inherit properties (IsEnabled) on visual children.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElement::CoerceIsEnabledOnVisualChildren(bool fIsEnabled)
{
    HRESULT hr = S_OK;
    CUIElement *pChild = NULL;
    CUIElementCollection *pChildren = NULL;

    pChildren = GetChildren();

    if (pChildren && pChildren->GetCount() > 0)
    {
        for (XUINT32 i = 0; i < pChildren->GetCount(); i++)
        {
            pChild = static_cast<CUIElement*>(pChildren->GetItemWithAddRef(i));
            if (pChild)
            {
                IFC(pChild->CoerceIsEnabled(fIsEnabled, /*fCoerceChildren*/ true));
            }
            ReleaseInterface(pChild);
        }
    }

Cleanup:
    ReleaseInterface(pChild);
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CUIElement::Focus()
//
//  Synopsis:
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CUIElement::Focus(DirectUI::FocusState focusState, bool animateIfBringIntoView, _Out_ bool* focusChanged, DirectUI::FocusNavigationDirection focusNavigationDirection, InputActivationBehavior inputActivationBehavior)
{
    CFocusManager *pFocusManager = NULL;

    *focusChanged = false;

    // Get FocusManager
    pFocusManager = VisualTree::GetFocusManagerForElement(this, LookupOptions::NoFallback);
    IFCPTR_RETURN(pFocusManager);

    // FocusMovement is OK with NULL parameter, sets focusChanged to false
    FocusMovement movement(this, focusNavigationDirection, focusState);
    movement.animateIfBringIntoView = animateIfBringIntoView;
    movement.requestInputActivation = (inputActivationBehavior == InputActivationBehavior::RequestActivation);
    const FocusMovementResult result = pFocusManager->SetFocusedElement(movement);
    IFC_RETURN(result.GetHResult());
    *focusChanged = result.WasMoved();

    return S_OK;
}

bool CUIElement::IsFocusable()
{
    return IsActive() &&
        IsVisible() &&
        (IsEnabled() || static_cast<CFrameworkElement*>(this)->AllowFocusWhenDisabled()) &&
        (IsTabStop() || IsFocusableForFocusEngagement()) &&
        AreAllAncestorsVisible();
}

_Check_return_ HRESULT
CUIElement::UpdateFocusState(_In_ DirectUI::FocusState focusState)
{
    if (focusState != GetFocusState())
    {
        CValue value;
        value.Set(focusState);
        const CDependencyProperty *pFocusStateDP = GetPropertyByIndexInline(KnownPropertyIndex::UIElement_FocusState);
        IFC_RETURN(CDependencyObject::SetValue(pFocusStateDP, value));

        //If the keyboard is used to navigate to the element, then the focus rectangle should be displayed.
        //Conversely, the user shouldn't need to use the keyboard to remove the focus, so any act that would remove focus is acceptable.
        if (focusState == DirectUI::FocusState::Keyboard || focusState == DirectUI::FocusState::Unfocused)
        {
            const CDependencyProperty *pFocusProperty = this->GetPropertyByIndexInline(KnownPropertyIndex::UIElement_UseSystemFocusVisuals);
            CValue useSystemFocusVisualsValue;
            IFC_RETURN(this->GetValue(pFocusProperty, &useSystemFocusVisualsValue));

            //Check if the SystemFocusVisuals are enabled
            if (useSystemFocusVisualsValue.AsBool())
            {
                CUIElement* focusTargetDescendant = nullptr;

                if (OfTypeByIndex<KnownTypeIndex::Control>())
                {
                    CValue focusTargetDescendantValue;
                    const CDependencyProperty *pdp = this->GetPropertyByIndexInline(KnownPropertyIndex::Control_FocusTargetDescendant);

                    //Retrieve the FocusTargetDescendant from Sparse Storage
                    IFC_RETURN(this->GetValue(pdp, &focusTargetDescendantValue));
                    CDependencyObject* pDO = nullptr;
                    IFC_RETURN(DirectUI::CValueBoxer::UnwrapWeakRef(&focusTargetDescendantValue, pdp, &pDO));
                    focusTargetDescendant = do_pointer_cast<CUIElement>(pDO);
                }

                if (focusTargetDescendant != nullptr)
                {
                    CFocusManager* pFocusManager = VisualTree::GetFocusManagerForElement(this);
                    CUIElement::NWSetContentDirty(focusTargetDescendant, DirtyFlags::Render);
                    if (focusState == DirectUI::FocusState::Unfocused)
                    {
                        pFocusManager->SetFocusRectangleUIElement(nullptr);
                    }
                    else
                    {
                        pFocusManager->SetFocusRectangleUIElement(focusTargetDescendant);
                    }
                }
                else
                {
                    CUIElement::NWSetContentDirty(this, DirtyFlags::Render);
                }
            }
        }
    }
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method: Visibility
//
//  Synopsis:
//      Property method, get/set the visibility.
//
//------------------------------------------------------------------------

_Check_return_
HRESULT
CUIElement::VisibilityState(
    _In_ CDependencyObject *pObject,
    _In_ XUINT32 cArgs,
    _Inout_updates_(cArgs) CValue *ppArgs,
    _In_opt_ IInspectable* pValueOuter,
    _Out_ CValue *pResult
)
{
    CUIElement *pUIElement = NULL;
    DirectUI::Visibility eVisibility = DirectUI::Visibility::Visible;

    IFC_RETURN(DoPointerCast(pUIElement, pObject));

    if (cArgs == 1 && ppArgs)
    {
        bool bChanged = false;

        // We are setting the value...
        if (ppArgs->GetType() == valueSigned)
        {
            eVisibility = (Visibility)ppArgs->AsSigned();
        }
        else if (ppArgs->GetType() == valueBool)
        {
            eVisibility = (Visibility)ppArgs->AsBool();
        }
        else if (ppArgs->IsEnum())
        {
            eVisibility = (Visibility)ppArgs->AsEnum();
        }
        else if (ppArgs->GetType() == valueObject)
        {
            if ((ppArgs->AsObject())->OfTypeByIndex<KnownTypeIndex::Int32>())
            {
                const auto pDO = static_cast<const CInt32*>(ppArgs->AsObject());
                eVisibility = (Visibility)pDO->m_iValue;
            }
            else if ((ppArgs->AsObject())->GetClassInformation()->IsEnum())
            {
                const auto pDO = static_cast<const CEnumerated*>(ppArgs->AsObject());
                eVisibility = (Visibility)pDO->m_nValue;
            }
            else
            {
                IFC_RETURN(E_INVALIDARG);
            }
        }
        else if (ppArgs->GetType() == valueString)
        {
            XUINT32 cString = 0;
            const WCHAR* pString = ppArgs->AsEncodedString().GetBufferAndCount(&cString);

            if ((7 == cString) && !xstrncmpi(L"visible", pString, 7))
            {
                eVisibility = DirectUI::Visibility::Visible;
            }
            else if ((9 == cString) && !xstrncmpi(L"collapsed", pString, 9))
            {
                eVisibility = DirectUI::Visibility::Collapsed;
            }
            else
            {
                IFC_RETURN(E_INVALIDARG);
            }
        }
        else
        {
            IFC_RETURN(E_INVALIDARG);
        }

        if (eVisibility != pUIElement->GetVisibility())
        {
            pUIElement->SetVisibility(eVisibility);

            pUIElement->InvalidateMeasure();

            // RS4 Bug #14881450:
            // When un-collapsing the ancestor of an open Popup, the Popup doesn't render.  Although
            // the Popup itself is marked as not being in the PC Scene and thus will get rendered if
            // we manage to walk to it, we must first walk through the PopupRoot.  If the PopupRoot isn't
            // explicitly marked dirty for render, the RenderWalk will stop and not visit its children.
            // The simple fix here is to assume there might be an open Popup under this node and mark
            // the PopupRoot dirty.
            if (eVisibility == DirectUI::Visibility::Visible)
            {
                CPopupRoot *popupRoot = nullptr;
                pUIElement->GetContext()->GetAdjustedPopupRootForElement(pUIElement, &popupRoot);
                if (popupRoot != nullptr)
                {
                    CUIElement::NWSetContentDirty(popupRoot, DirtyFlags::Render);
                }
            }

            bChanged = true;
        }

        if (bChanged && eVisibility != DirectUI::Visibility::Visible)
        {
            const bool hasFocus = FocusProperties::HasFocusedElement<CDependencyObject>(pUIElement);
            if (hasFocus)
            {
                CFocusManager *pFocusManager = VisualTree::GetFocusManagerForElement(pUIElement);
                IFCPTR_RETURN(pFocusManager);

                // Set the focus on the next focusable control.
                // If we are trying to set focus in a changing focus event handler, we will end up leaving focus on the disabled control.
                // As a result, we fail fast here. This is being tracked by Bug 9840123
                // Use InputActivationBehavior::NoActivate because hiding this element shouldn't steal activation from another window/island.
                IFCFAILFAST(pFocusManager->SetFocusOnNextFocusableElement(pFocusManager->GetRealFocusStateForFocusedElement(), true, InputActivationBehavior::NoActivate));
            }
        }

        if (pResult)
        {
            pResult->SetBool(bChanged);
        }
    }
    else if (pResult)
    {
       // We are getting the value
       pResult->Set(pUIElement->GetVisibility());
    }
    else
    {
        IFC_RETURN(E_INVALIDARG);
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method: PushUseLayoutRounding
//
//  Synopsis:
//      Push the UseLayoutRounding flag to children, until a local
//      set is found.
//
//------------------------------------------------------------------------

_Check_return_
HRESULT
CUIElement::PushUseLayoutRounding(bool fUseLayoutRounding)
{
    // Push the value down the tree until a non-set value is found.
    // The correct value has already been set on this element.

    CUIElementCollection *pCollection = GetChildren();
    if (pCollection)
    {
        XUINT32 cKids =  pCollection->GetCount();

        for (XUINT32 i = 0; i < cKids; i++)
        {
            HRESULT recordHr = S_OK;
            CUIElement *pChild = static_cast<CUIElement*>(pCollection->GetItemWithAddRef(i));
            IFCPTR_RETURN(pChild);
            if (pChild->GetUseLayoutRounding() != fUseLayoutRounding && pChild->IsPropertyDefaultByIndex(KnownPropertyIndex::UIElement_UseLayoutRounding))
            {
                pChild->SetUseLayoutRounding(fUseLayoutRounding);
                pChild->InvalidateMeasure();
                pChild->InvalidateArrange();
                RECORDFAILURE(pChild->PushUseLayoutRounding(fUseLayoutRounding));
            }
            pChild->Release();
            IFC_RETURN(recordHr);
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Compares ZIndex for draw order sorting.
//
//------------------------------------------------------------------------
bool CUIElement::IsDrawOrderSmallerThan(_In_ const CUIElement *pOther) const
{
    ASSERT(pOther);
    return GetZIndex() < pOther->GetZIndex();
}

//------------------------------------------------------------------------
//
//  Method: GetRootOfPopupSubTree
//
//  Synopsis:
//      If the element is under a popup, this method returns the immediate child of that
//      that popup. If the element is not under a popup, this method returns NULL.
//
//------------------------------------------------------------------------
CDependencyObject* CUIElement::GetRootOfPopupSubTree()
{
    CDependencyObject *pParent = GetParentInternal(false);
    CDependencyObject* pChild = this;

    while (pParent)
    {
        if (pParent->OfTypeByIndex<KnownTypeIndex::PopupRoot>())
        {
            return pChild;
        }
        pChild = pParent;
        pParent = pParent->GetParentInternal(false);
    }
    return NULL;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      If the element is rendering to an intermediate returns TRUE
//
//------------------------------------------------------------------------
bool CUIElement::NeedsIntermediateRendering() const
{
    // TODO: MERGE: Callers of this method are almost certainly broken in PC.
    return HasActiveProjection();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Invokes UpdateImplicitStyle method for each children from CUIElementCollection
//      and continue going on children.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElement::UpdateImplicitStyle(
    _In_opt_ CStyle *pOldStyle,
    _In_opt_ CStyle *pNewStyle,
    bool bForceUpdate,
    bool bUpdateChildren,
    bool isLeavingParentStyle
    )
{
    HRESULT hr = S_OK;

    // Push the value down the tree until a non-set value is found.
    CUIElementCollection *pCollection = GetChildren();
    if (bUpdateChildren && pCollection)
    {
        XUINT32 cKids =  pCollection->GetCount();

        for (XUINT32 i = 0; i < cKids; i++)
        {
            CUIElement *pChild = static_cast<CUIElement*>(pCollection->GetItemWithAddRef(i));
            IFCPTR(pChild);
            hr = pChild->UpdateImplicitStyle(pOldStyle, pNewStyle, bForceUpdate);

            pChild->Release();
            IFC(hr);
        }
    }
Cleanup:
    RRETURN(hr);
}


//------------------------------------------------------------------------
//
//  Method:   BringIntoView
//
//  Synopsis:
//      Attempts to raise an event to bring the element into view.
//------------------------------------------------------------------------
void CUIElement::BringIntoView()
{
    XRECTF emptyRect = {0.0f, 0.0f, 0.0f, 0.0f};
    BringIntoView(
        emptyRect,
        false /*forceIntoView */,
        false /*animateIfBringIntoView*/,
        true /*skipDuringManipulation*/);
}

//------------------------------------------------------------------------
//
//  Method:   BringIntoView
//
//  Synopsis:
//      Attempts to raise an event to bring the given rectangle of
//      the element into view.
//------------------------------------------------------------------------
void CUIElement::BringIntoView(
    XRECTF rectangle,
    bool forceIntoView,
    bool useAnimation,
    bool skipDuringManipulation,
    double horizontalAlignmentRatio,
    double verticalAlignmentRatio,
    double horizontalOffset,
    double verticalOffset)
{
#ifdef UIE_DBG
    IGNOREHR(gps->DebugOutputSzNoEndl(
        L"UIE[0x%p]:   BringIntoView - rectangle=(%f, %f, %f, %f), forceIntoView=%d, useAnimation=%d, skipDuringManipulation=%d.\r\n",
        this,
        rectangle.X, rectangle.Y, rectangle.Width, rectangle.Height,
        forceIntoView,
        useAnimation,
        skipDuringManipulation
    ));
    IGNOREHR(gps->DebugOutputSzNoEndl(
        L"                   horizontalAlignmentRatio=%lf, verticalAlignmentRatio=%lf, horizontalOffset=%lf, verticalOffset=%lf.\r\n",
        horizontalAlignmentRatio,
        verticalAlignmentRatio,
        horizontalOffset,
        verticalOffset
    ));
#endif // UIE_DBG

    // bringing into view requires TransformToVisual, which needs the
    // the element to be in the visual tree.
    if (IsActive())
    {
        RaiseRequestBringIntoViewEvent(
            rectangle,
            forceIntoView,
            useAnimation,
            skipDuringManipulation,
            horizontalAlignmentRatio,
            verticalAlignmentRatio,
            horizontalOffset,
            verticalOffset);
    }
}

//------------------------------------------------------------------------
//
//  Method:   RaiseRequestBringIntoViewEvent
//
//  Synopsis:
//      Generates a RequestBringIntoViewEvent that must be handled
//      to bring the given element and rectangle into view.
//------------------------------------------------------------------------

void CUIElement::RaiseRequestBringIntoViewEvent(
    XRECTF rectangle,
    bool forceIntoView,
    bool useAnimation,
    bool skipDuringManipulation,
    double horizontalAlignmentRatio,
    double verticalAlignmentRatio,
    double horizontalOffset,
    double verticalOffset)
{
    // Get event manager.
    CEventManager* const pEventManager = GetContext()->GetEventManager();
    CBringIntoViewRequestedEventArgs* pArgs = nullptr;
    EventHandle hEvent(KnownEventIndex::UIElement_BringIntoViewRequested);

    if (pEventManager)
    {
        // Create the DO that represents the event args.
        pArgs = new CBringIntoViewRequestedEventArgs();

        pArgs->m_targetRect = rectangle;
        IFCFAILFAST(pArgs->put_Source(this));
        IFCFAILFAST(pArgs->put_TargetElement(this));
        pArgs->m_forceIntoView = forceIntoView;
        pArgs->m_animationDesired = useAnimation;
        pArgs->m_interruptDuringManipulation = skipDuringManipulation;
        pArgs->m_horizontalAlignmentRatio = horizontalAlignmentRatio;
        pArgs->m_verticalAlignmentRatio = verticalAlignmentRatio;
        pArgs->m_horizontalOffset = horizontalOffset;
        pArgs->m_verticalOffset = verticalOffset;

        // Raise event
        pEventManager->RaiseRoutedEvent(hEvent, this, pArgs);

        // Since the event is routed asynchronously, we want to tell the connected animation service that
        // animations should be delayed a frame to give it a chance to process.
        CConnectedAnimationService* connectedAnimationService = GetContext()->GetConnectedAnimationServiceNoRef();
        if (connectedAnimationService)
        {
            connectedAnimationService->DelayStartOfAnimations();
        }
    }

    ReleaseInterface(pArgs);
}

extern "C" int g_SupportSSE2(); // For Effects

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the local transition root which can be used to hook LTE's
//      If one doesn't exist, it will create one.
//
// Note: TransitionRoot actually lives on the UIElementCollection, but to
// mirror AddChild, this method is also defined on UIElement.
//
// Note: we never pro-actively remove the transition root (not even when
// all children are removed) because once a UIElement has had transitions
// it is likely that it will have them in the future, making it more
// expensive to cleanup and create transition roots all the time.
//
//------------------------------------------------------------------------
CTransitionRoot* CUIElement::GetLocalTransitionRoot(bool ensureTransitionRoot)
{
    CUIElementCollection* pCollection = GetChildren();

    if (pCollection)
    {
        return pCollection->GetLocalTransitionRoot(ensureTransitionRoot);
    }
    else
    {
        return nullptr;
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Append a child to the visual
//
//      The forceCanHaveChildren flag is necessary because of Control. We
//      don't want to allow a custom Control to have Children in the usual
//      and externally manipulatable sense, but internally it does have
//      children, controlled by the ImplementationRoot.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElement::AddChild(_In_ CUIElement *pChild)
{
    if (pChild == NULL)
    {
        IFC_RETURN(E_INVALIDARG);
    }
    if (pChild == this)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    if (CanHaveChildren())
    {
        IFC_RETURN(EnsureChildrenCollection());

        IFC_RETURN(m_pChildren->FailIfLocked());
        IFC_RETURN(m_pChildren->Append(pChild));
        IFC_RETURN(m_pChildren->OnAddToCollection(pChild));
    }
    else
    {
        // Children are not allowed on this type of Object...
        IFC_RETURN(E_FAIL);
    }

    return S_OK;
}

bool CUIElement::CanModifyChildrenCollection() const
{
    if (CanHaveChildren())
    {
        return (m_pChildren != nullptr) ? !m_pChildren->IsLocked() : true;
    }
    else
    {
        return false;
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Add a child to the visual at the given index.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElement::InsertChild(_In_ XUINT32 nIndex, _In_ CUIElement *pChild)
{
    if (pChild == NULL)
    {
        IFC_RETURN(E_INVALIDARG);
    }
    if (pChild == this)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    if (CanHaveChildren())
    {
        IFC_RETURN(EnsureChildrenCollection());

        IFC_RETURN(m_pChildren->FailIfLocked());
        IFC_RETURN(m_pChildren->Insert(nIndex, pChild));
        IFC_RETURN(m_pChildren->OnAddToCollection(pChild));
    }
    else
    {
        // Children are not allowed on this type of Object...
        IFC_RETURN(E_FAIL);
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Removes a child from the visual
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElement::RemoveChild(_In_ CUIElement *pChild)
{
    HRESULT hr = S_OK;
    CDOCollection* pCollection = NULL;
    bool peggedPeer = false;
    bool peerIsPendingDelete = false;

    if (pChild == NULL)
    {
        IFC(E_INVALIDARG);
    }

    pCollection = static_cast<CDOCollection*>(GetChildren());
    if (pCollection)
    {
        CUIElement *ptrItem = NULL;
        XINT32 iIndex = -1;

        IFC(pCollection->FailIfLocked());
        IFC(pCollection->IndexOf(pChild, &iIndex));
        if (iIndex == -1)
        {
            IFC(E_INVALIDARG);  // not found
        }

        // Peg the child to make sure its peer doesn't get destroyed until at least
        // after the OnRemoveFromCollection call. Inside OnRemoveFromCollection, we attempt
        // to notify the automation peer for this child that we're being removed. We need
        // to avoid re-creating the framework peer by pegging it here. Don't
        // use PegManagedPeer, because it will mark the peer as stateful, and prevent
        // peer resurrection.
        pChild->TryPegPeer(&peggedPeer, &peerIsPendingDelete);

        ptrItem = (CUIElement *)pCollection->RemoveAt(iIndex);

        if (ptrItem)
        {
            IFC(pCollection->OnRemoveFromCollection(ptrItem, iIndex));
        }
        ReleaseInterface(ptrItem);
    }

Cleanup:
    if (peggedPeer)
    {
        pChild->UnpegManagedPeer();
    }
    RRETURN(hr);
}


//------------------------------------------------------------------------
//
//  Method:   GetFirstChildNoAddRef
//
//  Synopsis:
//      Returns the pointer to the first child
//        - This is NOT ADDRef'd
//
//------------------------------------------------------------------------
CUIElement*
CUIElement::GetFirstChildNoAddRef()
{
    CUIElement *pItem = GetFirstChild();
    ReleaseInterfaceNoNULL(pItem);
    return pItem;
}

//------------------------------------------------------------------------
//
//  Method:   GetFirstChild
//
//  Synopsis:
//      Returns the pointer to the first child
//
//------------------------------------------------------------------------
CUIElement*
CUIElement::GetFirstChild()
{
    if (m_pChildren && (m_pChildren->GetCount() > 0))
    {
        CUIElement *pItem = static_cast<CUIElement *>(m_pChildren->GetItemWithAddRef(0));
        return pItem;
    }
    return NULL;
}

// TRUE iff this element has a Projection that is not 2D aligned.
bool CUIElement::HasActiveProjection() const
{
    auto projection = GetProjection();
    return (projection != nullptr) && !projection->Is2DAligned();
}

bool CUIElement::HasTransitionTarget() const
{
    return GetTransitionTarget() != nullptr;
}

// Returns true if this node requires a CompNode for hit-test-invisibility purposes
bool CUIElement::RequiresHitTestInvisibleCompNode() const
{
    return m_requiresHitTestInvisibleCompNode;
}

// Setter to determine if this node requires a CompNode for hit-test-invisibility purposes
void CUIElement::SetRequiresHitTestInvisibleCompNode(bool value)
{
    m_requiresHitTestInvisibleCompNode = value;
}

void CUIElement::UpdateHitTestVisibilityForComposition()
{
    // In SpriteVisuals mode we transfer hit-test information directly through the SpriteVisuals,
    // all that's needed here is to dirty the element and its subtree, the RenderWalk will take care of the rest.
    SetEntireSubtreeDirty();
}

bool CUIElement::IsRedirectedChildOfSwapChainPanel() const
{
    return m_isRedirectedChildOfSwapChainPanel;
}

void CUIElement::SetIsRedirectedChildOfSwapChainPanel(bool value)
{
    m_isRedirectedChildOfSwapChainPanel = value;
}

//------------------------------------------------------------------------
//
//  Method:   HasCycle
//
//  Synopsis:
//      Walks the parent tree of this element, to make sure it that it
//      doesn't have a cycle.
//      Please note, this is not a complete cycle detection algorithm,
//      this was added to serve WriteableBitmap purposes (as part of 88739)
//      and could be expanded later to be Complete.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElement::HasCycle(_Out_ bool *pfHasCycle)
{
    CUIElement* pParent =  this;
    bool fResult = false;

    do
    {
        pParent = do_pointer_cast<CUIElement>(pParent->GetLogicalParentNoRef());
        if (pParent == this)
        {
            fResult = TRUE;
        }
    }
    while (pParent && !fResult);

    *pfHasCycle = fResult;

    RRETURN(S_OK);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns whether this element has a Foreground brush and, if so, its current value.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElement::GetForegroundBrush(
    _Out_ bool *pHasForegroundProperty,
    _Outptr_result_maybenull_ CBrush **ppForegroundBrush
    )
{
    *pHasForegroundProperty = OfTypeByIndex<KnownTypeIndex::TextBlock>()
        || OfTypeByIndex<KnownTypeIndex::RichTextBlock>()
        || OfTypeByIndex<KnownTypeIndex::Control>()
        || OfTypeByIndex<KnownTypeIndex::ContentPresenter>();

    if (*pHasForegroundProperty)
    {
        IFC_RETURN(EnsureTextFormattingForRead());
        SetInterface(*ppForegroundBrush, m_pTextFormatting->m_pForeground);
    }
    else
    {
        *ppForegroundBrush = nullptr;
    }

    return S_OK;
}

// Returns the DComp handoff visual for this CUIElement - it is being requested via the
// IXamlDCompInteropPrivate implementation of UIElement. The value returned is an IVisual (interop visual)
void
CUIElement::GetStoredHandOffVisual(_Outptr_result_maybenull_ WUComp::IVisual** ppHandOffVisual)
{
    Microsoft::WRL::ComPtr<WUComp::IVisual> handOffVisual;
    auto handOffVisualDataMap = GetDCompTreeHost()->GetHandOffVisualDataMap();

    auto entry = handOffVisualDataMap.find(this);
    if (entry != handOffVisualDataMap.end())
    {
        handOffVisual = entry->second.handOffVisual;
    }

    *ppHandOffVisual = handOffVisual.Detach();
}

// Start the process of caching the given HandOff visual and other supporting data structures
void CUIElement::StartHandOffVisualCaching(_In_ WUComp::IVisual* pHandOffVisual)
{
#ifdef HOVUIE_DBG
    IGNOREHR(gps->DebugOutputSzNoEndl(
        L"HOVUIE[0x%p]:StartHandOffVisualCaching - pHandOffVisual=0x%p\r\n", this, pHandOffVisual));
#endif // HOVUIE_DBG

    auto& handOffVisualDataMap = GetDCompTreeHost()->GetHandOffVisualDataMap();

    wrl::ComPtr<DCompPropertyChangedListener> listener = wrl::Make<DCompPropertyChangedListener>(this);

    // The listener expects an IInspectable pointer, which isn't guaranteed to be the same pointer as WUComp::IVisual.
    // Do a QI explicitly.
    wrl::ComPtr<IInspectable> wucVisualAsIInspectable;
    IFCFAILFAST(pHandOffVisual->QueryInterface(IID_PPV_ARGS(&wucVisualAsIInspectable)));
    listener->AttachToHandOffVisual(wucVisualAsIInspectable.Get());

    wrl::ComPtr<WUComp::IVisual> wucVisual;
    IFCFAILFAST(pHandOffVisual->QueryInterface(IID_PPV_ARGS(&wucVisual)));
    // The WUC visual will send notifications whenever the clip changes, and we'll automatically attach the property change
    // listener to the new clip, but in this case if there's a clip already set on the visual we'll have to attach the
    // listener explicitly.
    if (wucVisual != nullptr)
    {
        wrl::ComPtr<WUComp::ICompositionClip> wucClip;
        IFCFAILFAST(wucVisual->get_Clip(&wucClip));

        if (wucClip != nullptr)
        {
            wrl::ComPtr<IInspectable> wucClipAsIInspectable;
            IFCFAILFAST(wucClip.As(&wucClipAsIInspectable));
            listener->AttachToWUCClip(wucClipAsIInspectable.Get());
        }
    }

    HandOffVisualData data;
    data.handOffVisual = pHandOffVisual;
    data.dcompPropertyChangedListener = listener;
    auto emplaceResult = handOffVisualDataMap.emplace(this, data);
    ASSERT(emplaceResult.second == true);

    m_hasEverStoredHandoffOrHandinVisual = true;
}

// Stop the process of caching this CUIElement's HandOff visual and other supporting data structures
void CUIElement::StopHandOffVisualCaching()
{
#ifdef HOVUIE_DBG
        IGNOREHR(gps->DebugOutputSzNoEndl(
            L"HOVUIE[0x%p]:StopHandOffVisualCaching\r\n", this));
#endif // HOVUIE_DBG

    auto& handOffVisualDataMap = GetDCompTreeHost()->GetHandOffVisualDataMap();
    auto iter = handOffVisualDataMap.find(this);
    ASSERT(iter != handOffVisualDataMap.end());

    // We must unregister the listener before releasing our reference to it as registration takes a reference on the listener (as well as the visual)
    Microsoft::WRL::ComPtr<DCompPropertyChangedListener> listener = iter->second.dcompPropertyChangedListener;
    if (listener)
    {
        listener->DetachFromHandOffVisual();
        listener->DetachFromPrependVisual();
        listener->DetachFromWUCClip();
    }
    handOffVisualDataMap.erase(iter);
}

// Returns the WUComp::IVisual hand-in visual for this CUIElement.
void CUIElement::GetStoredHandInVisual(_Outptr_result_maybenull_ WUComp::IVisual** ppChildVisual)
{
    Microsoft::WRL::ComPtr<WUComp::IVisual> handInVisual;
    auto handInVisualsMap = GetDCompTreeHost()->GetHandInVisualsMap();

    auto entry = handInVisualsMap.find(this);
    if (entry != handInVisualsMap.end())
    {
        handInVisual = entry->second;
    }

    *ppChildVisual = handInVisual.Detach();
}

// Stores the provided WUComp::IVisual instance in DCompTreeHost::m_handInVisuals map.
void CUIElement::SetStoredHandInVisual(_In_opt_ WUComp::IVisual* pChildVisual)
{
#ifdef HOVUIE_DBG
    IGNOREHR(gps->DebugOutputSzNoEndl(
        L"HOVUIE[0x%p]:SetStoredHandInVisual - pChildVisual=0x%p\r\n", this, pChildVisual));
#endif // HOVUIE_DBG

    Microsoft::WRL::ComPtr<WUComp::IVisual> handInVisual = pChildVisual;
    auto& handInVisualsMap = GetDCompTreeHost()->GetHandInVisualsMap();

    auto emplaceResult = handInVisualsMap.emplace(this, handInVisual);

    // If insertion failed and this UIE already has a Hand-in visual, replace it.
    // Note that hand-in visuals are provided by the app and hence can be arbitrarily replaced.
    if (emplaceResult.second == false)
    {
        // Replacing the hand-in visual. Note we don't force Close() here as we do in the destructor.
        // If the app is sophisticated enough to replace the visual, we'll trust it to promptly release it.
        // This also keeps our internal Close policy simple.
        handInVisualsMap.erase(this);
        auto retryEmplaceResult = handInVisualsMap.emplace(this, handInVisual);
        ASSERT(retryEmplaceResult.second == true);
    }

    m_hasEverStoredHandoffOrHandinVisual = true;
}

// Returns the existing DComp handoff visual for this CUIElement, or creates a new one.
_Check_return_ HRESULT CUIElement::EnsureHandOffVisual(
    _Outptr_result_nullonfailure_ WUComp::IVisual** ppHandOffVisual,
    bool createLayerVisual)
{
    ctl::ComPtr<WUComp::IVisual> spHandOffVisual;

#ifdef HOVUIE_DBG
    IGNOREHR(gps->DebugOutputSzNoEndl(
        L"HOVUIE[0x%p]:EnsureHandOffVisual - entry\r\n", this));
#endif // HOVUIE_DBG

    *ppHandOffVisual = nullptr;

    // In ContainerVisuals mode, the HandOff visual is the Primary visual which is always required.
    // Thus, in this mode, we may already have a HandOff visual before the app has ever asked for one.
    // This is significantly different from non-ContainerVisuals mode, where the HandOff visual is created on demand.
    // For the sake of simplicity, only query for the HandOff visual from the CompNode in ContainerVisuals mode.
    if (m_pCompositionPeer != nullptr)
    {
        spHandOffVisual = m_pCompositionPeer->GetHandOffVisual();
        if (!IsUsingHandOffVisual())
        {
            // If we are not currently using a handoff visual, then we need to cache it and supporting data now.
            StartHandOffVisualCaching(spHandOffVisual.Get());
        }
    }

    if (spHandOffVisual == nullptr)
    {
        GetStoredHandOffVisual(spHandOffVisual.ReleaseAndGetAddressOf());
    }

    if (spHandOffVisual == nullptr)
    {
        // This UI element has no associated handoff visual, a new one must be created.
        // Ensure the DComp device was created before attempting to create a new handoff visual.
        IFC_RETURN(EnsureDCompDevice());

        if (createLayerVisual)
        {
            ctl::ComPtr<WUComp::ILayerVisual> layerHandOffVisual;
            IFC_RETURN(GetDCompTreeHost()->GetCompositor2()->CreateLayerVisual(&layerHandOffVisual));
            IFCFAILFAST(layerHandOffVisual.As(&spHandOffVisual));
        }
        else
        {
            ctl::ComPtr<ixp::IContainerVisual> handOffVisual;
            IFC_RETURN(GetDCompTreeHost()->GetCompositor()->CreateContainerVisual(handOffVisual.ReleaseAndGetAddressOf()));
            IFCFAILFAST(handOffVisual.As(&spHandOffVisual));

        }

#ifdef HOVCTN_DBG
        IGNOREHR(gps->DebugOutputSzNoEndl(
            L"HOVCTN:            EnsureHandOffVisual - created spHandOffVisual=0x%p\r\n", spHandOffVisual.Get()));
#endif // HOVCTN_DBG

        // Cache the HandOff visual and other per HandOff visual supporting data structures
        StartHandOffVisualCaching(spHandOffVisual.Get());

        OnHandOffOrHandInVisualDirty();

        ASSERT(!IsUsingHandOffVisual());
    }

    if (!IsUsingHandOffVisual())
    {
        // Mark this UI element as needing a composition node so
        // that a new HWCompTreeNode is associated to it on the next render walk.
        IFC_RETURN(SetRequiresComposition(
            CompositionRequirement::HandOffVisualNeeded,
            IndependentAnimationType::None /*detailedAnimationReason*/));

        EnsureTranslationPropertyInitialized();

        if (m_pCompositionPeer != nullptr)
        {
            // PushProperties depends on the hand-off visual state, dirtying guarantees we update it.
            CDependencyObject::NWSetRenderDirty(m_pCompositionPeer, DirtyFlags::Render);
        }
    }

    *ppHandOffVisual = spHandOffVisual.Detach();

#ifdef HOVUIE_DBG
    IGNOREHR(gps->DebugOutputSzNoEndl(
        L"HOVUIE[0x%p]:EnsureHandOffVisual - returns pHandOffVisual=0x%p\r\n", this, *ppHandOffVisual));
#endif // HOVUIE_DBG

    return S_OK;
}

// Returns the DComp handoff visual for this CUIElement - it is being requested via the
// IXamlDCompInteropPrivate implementation of UIElement, or ElementCompositionPreviewFactory::GetElementVisual.
// The value returned is an IUnknown which is in fact a WUComp::IVisual.
_Check_return_ HRESULT CUIElement::GetHandOffVisual(_Outptr_result_nullonfailure_ IUnknown** ppUnkHandOffVisual)
{
    xref_ptr<WUComp::IVisual> spHandOffVisual;

    *ppUnkHandOffVisual = nullptr;

    IFC_RETURN(EnsureHandOffVisual(spHandOffVisual.ReleaseAndGetAddressOf(), false /* createLayerVisual */));

    ASSERT(spHandOffVisual != nullptr);
    ASSERT(IsUsingHandOffVisual());

    // A handoff visual was successfully retrieved. Return it as an IUnknown.
    IFC_RETURN(spHandOffVisual->QueryInterface(IID_PPV_ARGS(ppUnkHandOffVisual)));

    return S_OK;
}

_Check_return_ HRESULT CUIElement::GetHandOffLayerVisual(_Outptr_result_nullonfailure_ WUComp::ILayerVisual** layerVisual)
{
    *layerVisual = nullptr;

    xref_ptr<WUComp::IVisual> handOffVisual;
    IFC_RETURN(EnsureHandOffVisual(handOffVisual.ReleaseAndGetAddressOf(), true /* createLayerVisual */));

    ASSERT(handOffVisual != nullptr);
    ASSERT(IsUsingHandOffVisual());

    // If we are too late and an interop visual was already created, this QI will fail. Ignore the error and return
    // a null visual instead.
    IGNOREHR(handOffVisual->QueryInterface(IID_PPV_ARGS(layerVisual)));

    return S_OK;
}

// Invoked by ElementCompositionPreviewFactory::GetElementChildVisual to retrieve the previously set child WinRT Visual for this UIElement.
// Returns nullptr if none was set.
_Check_return_ HRESULT
CUIElement::GetHandInVisual(
_Outptr_result_maybenull_ WUComp::IVisual** ppChildVisual)
{
    *ppChildVisual = nullptr;

    GetStoredHandInVisual(ppChildVisual);

#ifdef HIVUIE_DBG
    IGNOREHR(gps->DebugOutputSzNoEndl(
        L"HIVUIE[0x%p]:GetHandInVisual - returns pChildVisual=0x%p\r\n", this, *ppChildVisual));
#endif // HIVUIE_DBG

    return S_OK;
}

// Invoked by ElementCompositionPreviewFactory::SetElementChildVisual to hook up a WinRT Visual as a child of this UIElement.
_Check_return_ HRESULT CUIElement::SetHandInVisual(_In_opt_ WUComp::IVisual* pChildVisual)
{
#ifdef HIVUIE_DBG
    IGNOREHR(gps->DebugOutputSzNoEndl(
        L"HIVUIE[0x%p]:SetHandInVisual - pChildVisual=0x%p\r\n", this, pChildVisual));
#endif // HIVUIE_DBG

    ctl::ComPtr<WUComp::IVisual> spCurrentChildVisual;

    GetStoredHandInVisual(spCurrentChildVisual.ReleaseAndGetAddressOf());

    // Nothing to do if the provided visual is already set as the hand-in visual.
    if (spCurrentChildVisual.Get() == pChildVisual)
    {
        return S_OK;
    }

    if (spCurrentChildVisual != nullptr)
    {
        // Discard the current hand-in visual from the composition peer first.
        // If the hand-in visual is not replaced with another one, also clean-up the sparse storage
        // and mark this element as no longer needing a composition node for a hand-in visual.
        DiscardHandInVisual(pChildVisual != nullptr /*isHandInVisualReplaced*/);
    }

    if (pChildVisual != nullptr)
    {
#if DBG
        if (spCurrentChildVisual == nullptr)
        {
            // This UI element has no associated handoff visual, a new one must be created.
            const CWindowRenderTarget* pRenderTargetNoRef = GetContext()->NWGetWindowRenderTarget();
            ASSERT(pRenderTargetNoRef != nullptr);
            ASSERT(pRenderTargetNoRef->GetGraphicsDeviceManager() != nullptr);
        }
#endif

        // Set the sparse storage for the new hand-in visual.
        SetStoredHandInVisual(pChildVisual);

        OnHandOffOrHandInVisualDirty();

        if (!IsUsingHandInVisual())
        {
            // Mark this UI element as needing a composition node so
            // that a new HWCompTreeNode is associated to it on the next render walk.
            IFC_RETURN(SetRequiresComposition(
                CompositionRequirement::HandInVisualNeeded,
                IndependentAnimationType::None /*detailedAnimationReason*/));
        }
    }

    return S_OK;
}

// Discards the DComp handoff visual which was retrieved earlier with the GetHandOffVisual method.
// Does nothing if GetHandOffVisual was not called.
void CUIElement::DiscardHandOffVisual()
{
#ifdef HOVUIE_DBG
    IGNOREHR(gps->DebugOutputSzNoEndl(
        L"HOVUIE[0x%p]:DiscardHandOffVisual - isUsingHandOffVisual=%d\r\n", this, IsUsingHandOffVisual()));
#endif // HOVUIE_DBG

    if (IsUsingHandOffVisual())
    {
#ifdef DBG
        // The HandOffCompositionVisual property is expected to be non-NULL
        xref_ptr<WUComp::IVisual> spHandOffVisualDbg;
        GetStoredHandOffVisual(spHandOffVisualDbg.ReleaseAndGetAddressOf());
        ASSERT(spHandOffVisualDbg != nullptr);
#endif // DBG

        StopHandOffVisualCaching();

        // Mark this UI element as no longer needing a composition node.
        UnsetRequiresComposition(
            CompositionRequirement::HandOffVisualNeeded,
            IndependentAnimationType::None /*detailedAnimationReason*/);
    }
}

void CUIElement::OnHandOffOrHandInVisualDirty()
{
    if (GetContext()->InRenderWalk())
    {
        // With synchronous comp tree updates, we shouldn't be calling this method if the comp node is about to be removed.
        ASSERT(RequiresComposition());

        //
        // If we're doing synchronous comp tree updates, then it's possible we dirty the UIElement tree while rendering.
        // One example is WebView - as part of ensuring a CWebView's comp node, it synchronously calls CWebView::SetVisual,
        // which goes through DirectUI::CoreWebViewHost::UpdateRootVisual and puts a hand in visual on the CWebView. This
        // change in a hand-in visual will propagate a dirty flag up the tree to make sure the CUIElement is rendered again
        // and the HWCompTreeNode calls into it to attach its new hand-in visual, except we're already in the middle of the
        // render walk so propagating dirty flags is not allowed. Before synchronous comp tree updates, we would have waited
        // until the end of the frame (i.e. outside the render walk) to call CWebView::SetVisual, and we would have gone
        // back next frame to attach the hand in visual.
        //
        // The fix is to just not propagate a dirty flag if we're doing synchronous comp tree updates. The purpose of the
        // dirty flag is for HWCompTreeNode to call back to the CUIElement to attach the hand-in visual. We'll dirty the
        // comp node itself directly so that it gets walked during the PushProperties walk.
        //
        if (m_pCompositionPeer != nullptr)
        {
            CDependencyObject::NWSetRenderDirty(m_pCompositionPeer, DirtyFlags::Render);
        }
        else
        {
            //
            // If we have no comp node, then we haven't created one yet. There are two possibilities:
            //
            //   1. We haven't reached this element during the render walk yet. In that case, this element should still be
            //      marked dirty for comp node. We don't have to do anything, because the render walk will automatically
            //      reach this element later.
            ASSERT(PCHasCompositionNodeDirty());
            //
            //   2. The render walk has already visited this element, and determined that it didn't need a comp node at that
            //      time. In that case, this element now needs a comp node, and needs to be visited by the render walk again.
            //      We do not support this case with synchronous comp tree updates, and Xaml currently does not have any
            //      scenarios where the render walk sets a hand-off or a hand-in visual on a CUIElement that doesn't already
            //      have a comp node. This will cause an assert during SetRequiresComposition.
            //
        }
    }
    else
    {
        // Mark this element dirty to guarantee that HWCompTreeNode calls this UIElement to attach the new hand off/hand in
        // visual. We're reusing the DirtyFlags::Render flag even though the transform may not be dirty so we can save a bit
        // for an extra dirty flag.
        CUIElement::NWSetTransformDirty(this, DirtyFlags::Render);
    }
}

// Discards the hand-in visual which was set earlier with the SetHandInVisual method.
// Does nothing if SetHandInVisual was not called.
void CUIElement::DiscardHandInVisual(_In_ bool isHandInVisualReplaced)
{
#ifdef HIVUIE_DBG
    IGNOREHR(gps->DebugOutputSzNoEndl(L"HIVUIE[0x%p]:DiscardHandInVisual - entry\r\n", this));
#endif // HIVUIE_DBG

#ifdef DBG
    // The HandInCompositionVisual property is expected to be non-NULL
    xref_ptr<WUComp::IVisual> spHandInVisualDbg;
    GetStoredHandInVisual(spHandInVisualDbg.ReleaseAndGetAddressOf());
    ASSERT(spHandInVisualDbg != nullptr);
#endif // DBG

    if (m_pCompositionPeer != nullptr && m_pCompositionPeer->HasHandInVisual())
    {
        m_pCompositionPeer->DiscardHandInVisual();

        // If we're doing synchronous comp tree updates, there's no need to mark this CUIElement as dirty. We just
        // detached the hand-in visual from the comp node, and the UnsetRequiresComposition call below will remove
        // the comp node synchronously if it's no longer needed. All the changes that need to be made to the comp
        // tree have already been made without needing another render walk.
        if (isHandInVisualReplaced) // If we got a new hand-in visual then it still needs to be attached.
                                    // The comp node will not be removed in this case.
        {
            OnHandOffOrHandInVisualDirty();
        }
    }

    if (!isHandInVisualReplaced)
    {
        // Reset the sparse storage for the hand-in visual.
        SetStoredHandInVisual(nullptr);

        // Mark this UI element as no longer needing a composition node because of a hand-in visual.
        UnsetRequiresComposition(
            CompositionRequirement::HandInVisualNeeded,
            IndependentAnimationType::None /*detailedAnimationReason*/);
    }
}

_Check_return_ HRESULT
CUIElement::EnsureDCompDevice()
{
    CWindowRenderTarget* pRenderTargetNoRef = GetContext()->NWGetWindowRenderTarget();
    ASSERT(pRenderTargetNoRef != nullptr);
    ASSERT(pRenderTargetNoRef->GetGraphicsDeviceManager() != nullptr);

    IFC_RETURN(pRenderTargetNoRef->GetGraphicsDeviceManager()->EnsureDCompDevice());

    return S_OK;
}

DCompTreeHost* CUIElement::GetDCompTreeHost() const
{
    auto windowRenderTarget = GetContext()->NWGetWindowRenderTarget();
    if (windowRenderTarget)
    {
        return windowRenderTarget->GetDCompTreeHost();
    }
    return nullptr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Shutdown all children
//
//------------------------------------------------------------------------
void
CUIElement::Shutdown()
{
    CUIElement *pChild = NULL;

    // Cancel all active transitions.
    if (CTransition::HasActiveTransition(this) && GetContext())
    {
        IGNOREHR(CTransition::CancelTransitions(this));
    }

    if (m_pChildren)
    {
        // Cancel and clean-up unloading transitions as well.
        IGNOREHR(m_pChildren->RemoveAllUnloadingChildren(false /* removeFromKeepAliveList */, nullptr /* dcompTreeHost */));

        XUINT32 cKids = m_pChildren->GetCount();
        for (XUINT32 i = 0; i < cKids; i++)
        {
            pChild = (CUIElement *)m_pChildren->GetItemWithAddRef(i);
            if (pChild)
            {
                pChild->Shutdown();
                ReleaseInterface(pChild);
            }
        }
    }
    ReleaseInterface(pChild);

    m_layoutStorage.ResetLayoutInformation();
    m_hasLayoutStorage = false;

    //Clearing all layout flags except the expecting events.
    m_layoutFlags &= LF_EXPECTED_EVENTS;

    // note we do not change the life cycle of an element here. it will get
    // the left tree value from the LeaveImpl call
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates the collection if it does not yet exist.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElement::EnsureChildrenCollection()
{
    if (m_pChildren == NULL)
    {
        ASSERT(CanHaveChildren());

        IFC_RETURN(CreateChildrenCollection(&m_pChildren));

        // Mark children collection as associated with this DO,
        // so it will not be set as property value of other DO's.
        m_pChildren->SetAssociated(true, this);
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns an array of children in the order they should be processed
//      for rendering.
//
//------------------------------------------------------------------------
void CUIElement::GetChildrenInRenderOrder(
    _Outptr_result_buffer_maybenull_(*puiChildCount) CUIElement ***pppUIElements,
    _Out_ XUINT32 *puiChildCount
    )
{
    if (m_pChildren != nullptr)
    {
        m_pChildren->GetChildrenInRenderOrderInternal(
            pppUIElements,
            puiChildCount
            );
    }
    else
    {
        *pppUIElements = nullptr;
        *puiChildCount = 0;
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Property getter for UIElement.Children property.  This is not
//      exposed directly on UIElement, but on derived classes, e.g.
//      as Panel.Children.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElement::GetChildren(
    _In_ CDependencyObject *pObject,
    _In_ XUINT32 cArgs,
    _Inout_updates_(cArgs) CValue *ppArgs,
    _In_opt_ IInspectable* pValueOuter,
    _Out_ CValue *pResult
    )
{
    ASSERT(static_cast<CDependencyObject*>(pObject)->OfTypeByIndex<KnownTypeIndex::UIElement>());
    CUIElement *pUIE = static_cast<CUIElement*>(pObject);

    if (pUIE->CanHaveChildren())
    {
        IFC_RETURN(GetChildrenInternal(pObject, cArgs, ppArgs, pValueOuter, pResult));
    }
    else
    {
        // Children are not allowed on this type of Object...
        IFC_RETURN(E_FAIL);
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Property getter for UIElement.ChildrenInternal property.
//
//------------------------------------------------------------------------
// TODO: This property is only exposed because TickBar isn't implemented as a panel in the dxaml layer but needs to access its children
_Check_return_ HRESULT
CUIElement::GetChildrenInternal(
    _In_ CDependencyObject *pObject,
    _In_ XUINT32 cArgs,
    _Inout_updates_(cArgs) CValue *ppArgs,
    _In_opt_ IInspectable* pValueOuter,
    _Out_ CValue *pResult
    )
{
    CCollection *pChildren = NULL;

    ASSERT(static_cast<CDependencyObject*>(pObject)->OfTypeByIndex<KnownTypeIndex::UIElement>());
    CUIElement *pUIE = static_cast<CUIElement*>(pObject);

    IFC_RETURN(pUIE->EnsureChildrenCollection());
    pChildren = pUIE->GetChildren();

    pResult->SetObjectAddRef(pChildren);

    return S_OK;
}


//------------------------------------------------------------------------
//
// GC's ReferenceTrackerWalk. Walk children
// Called on GC's thread
//
//------------------------------------------------------------------------

bool CUIElement::ReferenceTrackerWalkCore(
    _In_ DirectUI::EReferenceTrackerWalkType walkType,
    _In_ bool isRoot,
    _In_ bool shouldWalkPeer)
{
    bool walked = CDependencyObject::ReferenceTrackerWalkCore(walkType, isRoot, shouldWalkPeer);

    if (walked)
    {
        if (walkType == DirectUI::EReferenceTrackerWalkType::RTW_GetElementCount)
        {
            auto pCore = DirectUI::DXamlServices::GetDXamlCore();
            ASSERT(pCore);

            auto oldCount = pCore->GetRTWElementCount();
            pCore->SetRTWElementCount(oldCount + 1);
        }

        CUIElementCollection *pChildren = static_cast<CUIElementCollection*>(GetChildren());

        if (pChildren)
        {
            // Iterate using the actual CDOCollection::m_items instead of CDOCollection::GetCount/GetItem
            // because CDOCollection::GetCount will return 0 if it is processing Neat().
            const std::vector<CDependencyObject*>& children = pChildren->GetCollection();
            for (unsigned int i = 0; i < children.size(); i++)
            {
                CDependencyObject *pChildNoRef = children[i];

                if (pChildNoRef)
                {
                    pChildNoRef->ReferenceTrackerWalk(
                        walkType,
                        false,  //isRoot
                        true);  //shouldWalkPeer
                }
            }

            auto transitionRoot = pChildren->GetLocalTransitionRoot(false /*ensureTransitionRoot*/);
            if (transitionRoot)
            {
                transitionRoot->ReferenceTrackerWalk(walkType, false /*isRoot*/, true /*shouldWalkPeer*/);
            }
        }
    }

    return walked;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Walk up to the root element to calculate the actual opacity of the element.
//
//------------------------------------------------------------------------
float CUIElement::GetOpacityToRoot()
{
    float collectedOpacity = 1.0f;

    // Collect opacities and walk up as long as the ancestor is visible.
    CUIElement* element = this;
    while (element != nullptr && element->IsVisible())
    {
        collectedOpacity *= element->GetOpacityCombined();

        element = element->GetUIElementAdjustedParentInternal(TRUE /*public parents only*/);
    }

    return collectedOpacity;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Recursive walk up to the parent HWCompTreeNode to collect the
//      transforms [and projections] to this element.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElement::GetRedirectionTransformsAndParentCompNode(
    _In_ CWindowRenderTarget *pRenderTarget,
    _Inout_ CMILMatrix *pTransformsToParentCompNode,
    bool allowClosedPopup,
    _Out_ CTransformToRoot *pTransformToRoot,
    _Out_ bool *pIsTransformToRootAnimating,
    _Out_ bool *pAreAllAncestorsVisible,
    _Outptr_result_maybenull_ HWCompTreeNode **ppNearestCompNode
    )
{
    HRESULT hr = S_OK;

    //
    // Popups and LTEs use redirected comp nodes for drawing, which let them
    // pick up transforms from elsewhere in the tree.
    //
    // For a tree that looks like:
    //   <a HWCompTreeNode>
    //     <b>
    //       <c>
    //         <d HWCompTreeNode>
    //           <e>
    //             <t/>
    //           </e>
    //         <d>
    //       </c>
    //       <r HWCompTreeNode RedirectionTarget="t" />
    //     </b>
    //   </a>
    //
    // Element r will draw with element t's transform. Since r's comp node is
    // parented to a's comp node, it will need the transform chain of t-e-d-c-b.
    // To get that, it will walk upwards from t until it hits the element that
    // has its own parent comp node, a. This method does the walking.
    //
    // Note that the walk up will not work if there are other comp nodes
    // between r and a that are not ancestors of t. This situation never comes
    // up, because XAML itself positions r in the tree, and we always place
    // it as an immediate sibling of some part of t's parent chain.
    //
    // Element r renders with t's transform, so it must get updated transforms
    // as t animates around. We accomplish this by finding all animated transforms
    // in the t-e-d-c-b chain and setting the same DComp objects on r. This way,
    // as the animated values are updated, r will get up-to-date values and stay
    // in sync with the rest of the tree.
    //
    // If element c is entirely clipped out, then we will not walk to d, so it
    // will not have a comp node. When we walk upwards from t, we have to recognize
    // that d should have a comp node and create one for it. This comp node will
    // be there entirely for updating transforms and will not render any content,
    // so its z-order doesn't matter and it can be added anywhere in the parent comp
    // node's child collection.
    //

    // If this element isn't visible then the redirected element shouldn't render, so there's no need to
    // keep walking to collect the transforms.
    // If this element needs to be kept visible for a hide animation, we count it as visible regardless.
    bool isVisible = IsVisible() || IsKeepVisible();
    if (OfTypeByIndex<KnownTypeIndex::Popup>() && !allowClosedPopup)
    {
        //
        // Normally nothing renders inside a closed popup. We make a single exception: the child transition of a
        // popup will play even if the popup is closed. This allows things like the add/delete and popup theme
        // transitions to render for closing popups, rather than just having the content disappear.
        //
        // The tree in that case will look like this:
        //  <root>
        //    ...
        //      <a HWCompTreeNode>
        //        <b>
        //          <p Popup RedirectionTarget="p">
        //            <d />
        //          </p>
        //        </b>
        //      </a>
        //    ...
        //    <PopupRoot>
        //      <Popup p /> <!-- Not a direct child; rendered as part of CPopupRoot::m_pOpenPopups -->
        //      <r HWCompTreeNode RedirectionTarget="d" />
        //    </PopupRoot>
        //  </root>
        //
        // We'll make 3 passes to render everything:
        //
        //   1. The regular tree walk starts at the root and arrives at p. It stops because p requires redirected
        //      rendering.
        //
        //      This step isn't affected by whether or not p is closed.
        //
        //   2. The popup's redirected walk first picks up the rendering context starting from p's parent, then walks
        //      down the tree. Here, the context will be comp node A and transform b. We then stop at d because d
        //      requires redirected rendering.
        //
        //      Picking up the rendering context (i.e. calling this method) isn't affected by whether p is closed,
        //      because it starts at p's parent b. When doing the render walk, we immediately find that p is closed
        //      and stop rendering. Even if p was open, we would stop at d shortly after.
        //
        //   3. The LTE's redirected walk picks up the rendering context starting from d's parent, then walks down the
        //      tree. Here, it does matter whether p is open or closed.
        //
        //      If p is open, then it has a comp node. The rendering context will be comp node P and transform identity.
        //
        //      If p is closed, then it has no comp node. Rather than terminating the walk up and not rendering, we
        //      allow the walk to continue. We'll create a temporary comp node for the closed popup and use it in the
        //      rendering context. Note that a's subtree could itself be inside a closed popup. In that case we'll walk
        //      up until we encounter the popup, then stop the walk and not render anything. It's only the immediate
        //      children of a popup that are allowed to render when the popup is closed.
        //
        // This can also happen if p is a parentless popup. In that case its child will be attached directly to the
        // popup root:
        //  <root>
        //    <PopupRoot>
        //      <d /> <!-- direct child -->
        //      <Popup p /> <!-- Not a direct child; rendered as part of CPopupRoot::m_pOpenPopups -->
        //      <r HWCompTreeNode RedirectionTarget="d" />
        //    </PopupRoot>
        //  </root>
        //
        // The three cases are the same as above, except the rendering context will have use root comp node rather than
        // a's comp node.
        //

        // Closed popups aren't considered visible. Note that the first step of the walk will have allowClosedPopup TRUE
        // and will not go through this code path.
        isVisible = (isVisible && static_cast<CPopup*>(this)->IsOpen())
            // If this element needs to be kept visible for a hide animation, we count it as visible regardless.
            || IsKeepVisible();
    }
    bool areAncestorsVisible = true;

    CTransformToRoot transformToRoot;

    bool isTransformAnimating = false;
    bool isAncestorTransformAnimating = false;

    HWCompTreeNode *pThisCompNode = NULL;
    HWCompTreeNode *pNearestAncestorCompNode = NULL;

    if (isVisible)
    {
        // Stop walking at the root element.
        if (!IsRenderWalkRoot())
        {
            // The first step of the walk could go through a closed parented popup.
            CUIElement *pParent = GetUIElementAdjustedParentInternal(true /*public parents only*/, !!allowClosedPopup);

            //
            // In the case of a closed popup nested inside a closed parentless popup, we could walk past the end and not
            // have a parent anymore. Don't render in that case. This is not normally a problem because we would not walk
            // past the closed inner popup, but we made an exception for it if it's the first step of the walk.
            //
            if (pParent != NULL)
            {
                //
                // On the first call, we allow a walk through a closed popup. This is because we want transitions on the
                // child of the popup to keep playing even if the popup is closed, which requires knowing where the popup
                // would have rendered if it was still open. On recursive calls we disallow it.
                //
                IFC(pParent->GetRedirectionTransformsAndParentCompNode(
                    pRenderTarget,
                    pTransformsToParentCompNode,
                    FALSE, /* allowClosedPopup */
                    &transformToRoot,
                    &isAncestorTransformAnimating,
                    &areAncestorsVisible,
                    &pNearestAncestorCompNode
                    ));
            }
            else
            {
                areAncestorsVisible = FALSE;
            }
        }

        // If we've determined we're still visible after the recursive call, look for comp nodes
        // and update transforms.
        // If there was no parent, we still want to include transforms and comp node for the root element.
        if (areAncestorsVisible)
        {
            //
            // Each walk up originates from a Popup or LayoutTransitionElement.
            // Most UIElements do not have clones that are sent over to the compositor thread.
            // When an element does have a clone there will be a 1:1 mapping between UIE and clones in nearly
            // all situations. See further comments below for the one exception.
            //

            // Check whether there's a comp node analogous to this UIE.
            if (   RequiresCompositionNode()
                || IsHiddenForLayoutTransition()
                || OfTypeByIndex<KnownTypeIndex::Popup>()
                || OfTypeByIndex<KnownTypeIndex::RootVisual>())
            {
                // If the element is the target of a layout transition, then the analogous comp node
                // was cloned from the LayoutTransitionElement itself instead of the UIE.
                if (IsHiddenForLayoutTransition())
                {
                    CLayoutTransitionElement *pLTE = NULL;
                    IFC(m_pLayoutTransitionRenderers->get_item(0, pLTE));
                    ASSERT(pLTE != NULL);

                    SetInterface(pThisCompNode, pLTE->m_pCompositionPeer);
                }
                else
                {
                    SetInterface(pThisCompNode, m_pCompositionPeer);
                }

                //
                // A portaling animation scenario may look like this:
                //      <a HWCompTreeNode>
                //        <b>
                //          <c HWCompTreeNode>
                //            <d/>
                //          </c>
                //        </b>
                //        <r1 HWRedirectedCompTreeNode RedirectionTarget="c"/>
                //        <r2 HWRedirectedCompTreeNode RedirectionTarget="c"/>
                //      </a>
                //
                // It would have a matching comp node tree like this:
                //      <A>
                //        <C/>
                //        <R1/>
                //        <R2/>
                //      </A>
                //
                // Because r1 and r2 both target c, c gets walked twice. Its contents are added to both R1 and R2.
                //
                // Suppose that d also needed a HWCompTreeNode. The matching comp node tree would need to look like
                // this:
                //      <A>
                //        <C/>
                //        <R1>
                //          <D1/>
                //        </R1>
                //        <R2>
                //          <D2/>
                //        </R2>
                //      </A>
                //
                // The element d now has two corresponding comp nodes, D1 and D2. They need to be parented separately
                // to R1 and R2. We don't currently support having multiple comp nodes for a single element.
                //
                // TODO: JCOMP:  This worked before because the comp tree wasn't incremental and we always created
                // new comp nodes whenever we need one instead of reusing a cached one. Now we will parent D to R1,
                // then reparent it to R2 on the second walk. Given that comp nodes are now more commonly used
                // (e.g. projections, unaligned transforms - see RequiresCompositionNode & HasComplexTransformation),
                // this should be fixed.
                //
                // The scenario becomes much more complicated if there are more portaling transitions inside c itself.
                // But this scenario is very rare and does not need to be supported.
                //

                if (pThisCompNode == NULL)
                {
                    // Corner case - if the UIE required a comp node, but we didn't find one, it's because the render
                    // walk culled out the subgraph including this UIE since it was determined there was nothing to render.
                    // Those checks don't take into account whether there are Popups/LTEs in that subgraph that _should_
                    // be drawn, though.  We account for this by creating the comp nodes now, as needed.

                    // NOTE: This walk is finding a _target_ comp node for the Popup/LTE we started from to use in
                    //       order to position itself correctly on the render thread.  In the case where we culled
                    //       the subgraph the target UIE was in, no content will be rendered underneath the comp
                    //       node created here - it's only used as a reference point for transforms.

                    IFC(CreateTemporaryCompositionPeer(
                        pRenderTarget,
                        pNearestAncestorCompNode,
                        pTransformsToParentCompNode,
                        &pThisCompNode
                        ));

                    // The root comp node can never be culled out, so we should always have found a parent to attach to.
                    ASSERT(pNearestAncestorCompNode != NULL);
                }
            }
            else
            {
#if DBG
                // If the UIE doesn't qualify to have a comp node, it shouldn't have one.
                ASSERT(m_pCompositionPeer == NULL);
#endif
            }

            // Calculate the local transform.
            CMILMatrix matLocal(TRUE);
            if (IsHiddenForLayoutTransition())
            {
                // get information from destination LayoutTransitionElement
                // todo: should ultimately get high-def information from render thread

                // TODO: HWPC: This only looks for the first LTE.  Corner case.
                // TODO: HWPC: If we have transitions or Popups nested inside a 'portaling' transition where the ancestor has multiple LTEs, this won't work.  They'll all render relative to the first LTE.
                CLayoutTransitionElement *pLTE = NULL;
                IFC(m_pLayoutTransitionRenderers->get_item(0, pLTE));
                ASSERT(pLTE != NULL);
                XPOINTF relativelocation = pLTE->GetPositionRelativeToParent();
                matLocal.SetDx(relativelocation.x);
                matLocal.SetDy(relativelocation.y);

                isTransformAnimating = pLTE->IsTransformOrOffsetAffectingPropertyIndependentlyAnimating();
            }
            else
            {
                IGNORERESULT(GetLocalTransform(TransformRetrievalOptions::IncludePropertiesSetInComposition, &matLocal));
                isTransformAnimating = IsTransformOrOffsetAffectingPropertyIndependentlyAnimating();
            }

            const bool isRootAndOneCoreStrict = this->GetTypeIndex() == KnownTypeIndex::RootVisual && XamlOneCoreTransforms::IsEnabled();
            const bool isXamlIslandRoot = this->GetTypeIndex() == KnownTypeIndex::XamlIslandRoot;
            if (isRootAndOneCoreStrict || isXamlIslandRoot)
            {
                // In OneCore strict mode, the root visual transform is identity
                // We need to apply the scale from the composition island.
                // In XamlIslandRoots mode, the XamlIslandRoot alone knows the correct scale.
                const float plateauScale = RootScale::GetRasterizationScaleForElement(this);
                if (plateauScale != 1.0f)
                {
                    CMILMatrix rootScale(TRUE);
                    rootScale.Scale(plateauScale, plateauScale);
                    transformToRoot.Prepend(rootScale);
                }
            }

            const bool hasTransform = !matLocal.IsIdentity();

            // Always include the transform as part of the transform-to-root.
            if (hasTransform)
            {
                transformToRoot.Prepend(matLocal);
            }

            if (HasActiveProjection())
            {
                XSIZEF elementSize;
                IFC(GetElementSizeForProjection(&elementSize));

                CMILMatrix4x4 projectionMatrix = GetProjection()->GetOverallProjectionMatrix(elementSize);

                transformToRoot.Prepend(projectionMatrix);
            }

            const double rasterizationScale = GetRasterizationScale();
            if (rasterizationScale != 1.0)
            {
                transformToRoot.MultiplyRasterizationScale(rasterizationScale);
            }

            // Note carefully that we don't pixel snap transformToRoot.  This is a limitation of the redirection walk.
            // For example with a tree like this:
            //
            // ScrollContentPresenter             PopupRoot
            //      |                                |
            //    Grid                           Popup.Child
            //      |                               ^
            //    Popup    <.........................
            //
            // Grid is manipulatable and so we typically apply pixel snapping to its TransformVisual.  The Popup
            // child scrolls with the Grid, but since we use the redirection walk it doesn't inherit the pixel
            // snapping from Grid's visual.  To make Popup pixel snap we would need to apply pixel snapping
            // to the appropriate visual in Popup.Child's HWRedirectedCompTreeNode.  But we currently don't do this...
            // As we gather the transforms via HWRedirectedCompTreeNode::GatherRedirectionTransforms(), we don't keep track
            // of which of these transforms were set on visual(s) that got pixel snapped, doing so would require
            // us to create extra visuals in the HWRedirectedCompTreeNode's visual chain.
            //
            // For now we simply leave this part of the transform chain un-pixel-snapped.
            // Thus this part of the world transform (in transformToRoot) must also stay un-pixel-snapped,
            // to stay in sync with the overall transform that the Compositor will compute.
            // This would be complicated to do correctly, and is an uncommon scenario so for now we're keeping this simple.

            // Apply or reset the transform-to-comp-node as needed.
            if (pThisCompNode != NULL)
            {
                // Reset the transform-to-comp-node since this element had a comp node itself.
                // The transform for this element is cloned directly over to the composition tree.
                pTransformsToParentCompNode->SetToIdentity();
            }
            else
            {
                if (hasTransform)
                {
                    // Update the transform to the nearest comp node.
                    pTransformsToParentCompNode->Prepend(matLocal);
                }

                // Projections are implemented through comp nodes now. An element without a comp node
                // cannot have a projection.
                ASSERT(!HasActiveProjection());
            }
        }
    }

    *pTransformToRoot = transformToRoot;
    *pIsTransformToRootAnimating = isTransformAnimating || isAncestorTransformAnimating;
    *pAreAllAncestorsVisible = isVisible && areAncestorsVisible;

    // Return the nearest comp node found.
    SetInterface(*ppNearestCompNode, (pThisCompNode != NULL) ? pThisCompNode : pNearestAncestorCompNode);

Cleanup:
    ReleaseInterfaceNoNULL(pThisCompNode);
    ReleaseInterfaceNoNULL(pNearestAncestorCompNode);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Clean up walk-specific render data when the type of render walk
//      changes, and ensure the appropriate storage is available for the
//      new walk.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElement::EnsureContentRenderData(RenderWalkType newType)
{
    RenderWalkType oldType = m_contentRenderData.type;
    if (oldType != newType)
    {
        m_contentRenderData.Reset(newType);

        // Fire virtual to clean-up element-specific state for the walk change.
        EnsureContentRenderDataVirtual(oldType, newType);
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Virtual to clean up walk-specific render data when the type of
//      render walk changes.
//
//------------------------------------------------------------------------
void
CUIElement::EnsureContentRenderDataVirtual(
    RenderWalkType oldType,
    RenderWalkType newType
    )
{
    ASSERT(oldType != newType);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Clean up walk-specific render data when the type of render walk
//      changes, and ensure the appropriate storage is available for the
//      new walk.
//
//------------------------------------------------------------------------
void
CUIElement::EnsurePropertyRenderData(RenderWalkType walkType)
{
    if (m_propertyRenderData.type != walkType)
    {
        // If we switched from PC, clear PC render data.
        // This call is here, and not handled in PrimitiveCompositionPropertyData.Clear(), because it's virtual
        // so derived classes can clear class-specific render data as well.
        if (IsInPCScene())
        {
            ClearPCRenderData();

            // The composition peer needs to be removed as well. If the element is drawn with PC again and
            // still requires composition, the peer will be added again during the render walk.
            if (walkType != RWT_NonePreserveDComp) RemoveCompositionPeer();
        }

        // Release the existing property render data, and initialize it for the new walk type.
        m_propertyRenderData.Reset(walkType);
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Notify the parent object that this object is being reparented.
//
//------------------------------------------------------------------------
void CUIElement::NotifyParentChange(
    _In_ CDependencyObject *pNewParent,
    _In_ RENDERCHANGEDPFN pfnNewParentRenderChangedHandler
    )
{
    pfnNewParentRenderChangedHandler(pNewParent, DirtyFlags::Bounds);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Overridden to handle dirty flag propagation logic special to UIElement trees.
//
//------------------------------------------------------------------------
void
CUIElement::NWPropagateDirtyFlag(DirtyFlags flags)
{
#if DBG
    DbgCheckElementDirtyFlags(flags);
#endif

    // If the element is hidden because it's being rendered via redirection, dirty flag
    // propagation only needs to go to the LTE(s), not to the parent in the tree.
    if (IsHiddenForLayoutTransition())
    {
        NWPropagateDirtyFlagForLayoutTransitions(flags);
    }
    else
    {
        // Call into base implementation to propagate to parent.
        CDependencyObject::NWPropagateDirtyFlag(flags);
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Helper to validate dirty flag propagation invariants.
//
//------------------------------------------------------------------------
#if DBG
void
CUIElement::DbgCheckElementDirtyFlags(DirtyFlags flags)
{
    // A dirty flag propagating up through this element implies that some rendering state has changed.
    // This isn't a safe operation during the render walk, since that's where the flags are being consumed.
    // If a flag propagates up during the walk, it can corrupt state and cause subsequent rendering errors.
    //
    // One exception is if we're doing synchronous comp tree updates. There, a comp node can get removed as
    // part of rendering when an element is culled from the scene, which can remove a DComp property listener
    // on a visual and dirty the bounds. Bounds dirty flags are not consumed during the render walk, so allow
    // them to be dirtied.
    ASSERT(!GetContext()->InRenderWalk()
        || flags == (DirtyFlags::Independent | DirtyFlags::Bounds));


    // Independent property changes should only ever dirty UIElement bounds, not rendering. We should
    // never need to perform a render walk in response. Dirty flag changes in general may, however,
    // propagate up through the sub-property DOs in case changes have side effects (e.g. invalidated
    // cached transforms for a TransformGroup).
    ASSERT(!flags_enum::is_set(flags, DirtyFlags::Independent) || flags == (DirtyFlags::Independent | DirtyFlags::Bounds));
}
#endif

//------------------------------------------------------------------------
//
//  Synopsis:
//      If there's a local transform, append this to the ancestor and set
//      it as the new transform.
//
//------------------------------------------------------------------------
void
CUIElement::GetCumulativeTransform(
    _Inout_ CMILMatrix *pLocalTransform,
    _Inout_ const CMILMatrix **ppCumulativeTransform
    )
{
    if (!GetLocalTransform(TransformRetrievalOptions::IncludePropertiesSetInComposition, pLocalTransform))
    {
        // Append this elements transform to the current transform on the stack.
        pLocalTransform->Append(**ppCumulativeTransform);

        *ppCumulativeTransform = pLocalTransform;
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Calculate the element's size for projections. The element size is
//      used for positioning the origin of the 3D transforms. It comes
//      from the layout size of the element. In the case of a Canvas (which
//      doesn't do layout) without an explicit width or height, we use the
//      clipped render bounds of the Canvas.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElement::GetElementSizeForProjection(_Out_ XSIZEF *pElementSize)
{
    XSIZEF elementSize = NWComputeElementSize();

    if ((elementSize.width == 0.0f || elementSize.height == 0.0f) && OfTypeByIndex<KnownTypeIndex::Canvas>())
    {
        XRECTF_RB clippedInnerBounds;

        IFC_RETURN(GetInnerBounds(&clippedInnerBounds));

        IFC_RETURN(ApplyUIEClipToBounds(&clippedInnerBounds));

        if (!ShouldApplyLayoutClipAsAncestorClip())
        {
            // TODO_WinRT: We need to decide what the behavior should be in ContainerVisuals mode.
            // This decision will affect the center of projection. The current behavior is to not apply the LayoutClip.
            IFC_RETURN(ApplyLayoutClipToBounds(&clippedInnerBounds));
        }

        elementSize.width = clippedInnerBounds.right - clippedInnerBounds.left;
        elementSize.height = clippedInnerBounds.bottom - clippedInnerBounds.top;
    }

    elementSize.width = static_cast<XFLOAT>(XcpCeiling(elementSize.width));
    elementSize.height = static_cast<XFLOAT>(XcpCeiling(elementSize.height));

    //
    // TODO: PC: If popups are involved, this size won't be accurate because the popup's
    // ancestor won't have the up-to-date size until the redirected render walk for the popup
    // happens.
    //
    *pElementSize = elementSize;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Calculate the element's size for projections. The element size
//      can come from specifying a Width/Height on the element or from
//      layout. If the element size isn't available, use the dimensions
//      of the intermediate surface as the element size.
//
//------------------------------------------------------------------------
XSIZEF CUIElement::NWComputeElementSize()
{
    // TODO: Merge: This should use the element's render bounds. Not the layout bounds and not the
    // bounds from the intermediate (the intermediate doesn't include any transparent edges).
    XSIZEF elementSize;
    elementSize.width = GetActualWidth();
    if (elementSize.width <= 0.0f && HasLayoutStorage())
    {
        // Use the width from layout
        elementSize.width = GetLayoutStorage()->m_size.width;
    }

    elementSize.height = GetActualHeight();
    if (elementSize.height <= 0.0f && HasLayoutStorage())
    {
        // Use the height from layout
        elementSize.height = GetLayoutStorage()->m_size.height;
    }

    return elementSize;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Calculate the difference between the element's size and the
//      projection quad's size, taking into account where the quad will
//      be positioned relative to the element. Specifically, generate four
//      offsets that are added to/subtracted from the element's bounds to
//      get the quad's bounds.
//
//      The offsets for each direction is positive if the quad bound is
//      outside the element bound and negative if the quad bound is inside
//      the element bound.
//
//------------------------------------------------------------------------
/* static */ void
CUIElement::NWComputeProjectionQuadPadding(
    XINT32 quadOffsetX,
    XINT32 quadOffsetY,
    XINT32 quadWidth,
    XINT32 quadHeight,
    _In_ const XSIZEF *pElementSize,
    _Out_ XRECTF_RB *pQuadPadding
    )
{
    //
    // The element's left/top bounds are at 0, so the quad is inside the element's bounds
    // by (fVBOffsetX, fVBOffsetY).
    //
    pQuadPadding->left = static_cast<XFLOAT>(-quadOffsetX);
    pQuadPadding->top = static_cast<XFLOAT>(-quadOffsetY);

    //
    // The content quad is outside the element's bounds if its right edge (rQuadOffsetX + quad's width)
    // is more to the right than the element's right bound (element's width).
    //
    pQuadPadding->right = static_cast<XFLOAT>(quadOffsetX + quadWidth) - pElementSize->width;

    //
    // The content quad is outside the element's bounds if its bottom edge (rQuadOffsetY + quad's height)
    // is below the element's bottom bound (element's height).
    //
    pQuadPadding->bottom = static_cast<XFLOAT>(quadOffsetY + quadHeight) - pElementSize->height;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Marks the child collection as dirty for sorting.
//
//------------------------------------------------------------------------
void
CUIElement::SetChildRenderOrderDirty()
{
    CUIElementCollection *pChildren = GetChildren();
    if (pChildren)
    {
        pChildren->SetSortedCollectionDirty(TRUE);
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      This RENDERCHANGEDPFN marks that this UIElement has changed to or from
//      having an opacity animation.
//
//------------------------------------------------------------------------
/* static */ void
CUIElement::NWSetHasOpacityAnimationDirty(_In_ CDependencyObject *pTarget, DirtyFlags flags)
{
    ASSERT(pTarget->OfTypeByIndex<KnownTypeIndex::UIElement>());

    ASSERT(!flags_enum::is_set(flags, DirtyFlags::Independent));

    CUIElement *pUIE = static_cast<CUIElement*>(pTarget);

    //
    // OpacityAnimation - Dirties: (Render)
    //
    pUIE->NWSetDirtyFlagsAndPropagate(flags | DirtyFlags::Render, pUIE->m_fNWHasOpacityAnimationDirty);
    pUIE->m_fNWHasOpacityAnimationDirty = TRUE;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      This RENDERCHANGEDPFN marks that this UIElement has changed to or from
//      having a comp node (or a brush animation).
//
//------------------------------------------------------------------------
/* static */ void
CUIElement::NWSetHasCompositionNodeDirty(_In_ CDependencyObject *pTarget, DirtyFlags flags)
{
    ASSERT(pTarget->OfTypeByIndex<KnownTypeIndex::UIElement>());

    ASSERT(!flags_enum::is_set(flags, DirtyFlags::Independent));

    CUIElement *pUIE = static_cast<CUIElement*>(pTarget);

    //
    // CompositionNode - Dirties: (Render)
    //
    pUIE->NWSetDirtyFlagsAndPropagate(flags | DirtyFlags::Render, pUIE->m_fNWHasCompNodeDirty);
    pUIE->m_fNWHasCompNodeDirty = TRUE;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      This RENDERCHANGEDPFN marks that this UIElement's redirection target or the
//      data it collects from it may have changed.
//
//------------------------------------------------------------------------
/* static */ void
CUIElement::NWSetRedirectionDataDirty(_In_ CDependencyObject *pTarget, DirtyFlags flags)
{
    ASSERT(pTarget->OfTypeByIndex<KnownTypeIndex::UIElement>());

    ASSERT(!flags_enum::is_set(flags, DirtyFlags::Independent));

    CUIElement *pUIE = static_cast<CUIElement*>(pTarget);

    //
    // CompositionNode - Dirties: (Render)
    //
    pUIE->NWSetDirtyFlagsAndPropagate(flags | DirtyFlags::Render, pUIE->m_fNWRedirectionDataDirty);
    pUIE->m_fNWRedirectionDataDirty = TRUE;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      This RENDERCHANGEDPFN marks this UIElement's visibility as dirty for rendering.
//
//------------------------------------------------------------------------
// TODO: MERGE: Future optimization - add IsVisible() checks to all UIE RENDERCHANGEDPFNs
// TODO: MERGE: to prevent unnecessary change propagations from collapsed subgraphs.
/* static */ void
CUIElement::NWSetVisibilityDirty(_In_ CDependencyObject *pTarget, DirtyFlags flags)
{
    ASSERT(pTarget->OfTypeByIndex<KnownTypeIndex::UIElement>());

    // Visibility cannot change independently.
    ASSERT(!flags_enum::is_set(flags, DirtyFlags::Independent));

    CUIElement *pUIE = static_cast<CUIElement*>(pTarget);

    // Propagate up if visibility flag wasn't set yet, or if inner bounds weren't marked dirty already.
    //
    // Visibility is special-cased from the way other changes invalidate bounds. Visibility doesn't
    // affect the content inner bounds or child inner bounds, but it instead affects the combined
    // inner bounds directly.  The combined inner bounds are zeroed out while Collapsed, but calculated
    // from the other cached inner bounds when Visible.  This special case allows changes to the child or
    // content inner bounds to remembered, but to not invalidate the tree, while Visibility=Collapsed until
    // the visibility itself changes.
    if (!pUIE->m_fNWVisibilityDirty || !pUIE->AreInnerBoundsDirty())
    {
        pUIE->m_fNWVisibilityDirty = TRUE;

        pUIE->m_combinedInnerBoundsDirty = TRUE;
        pUIE->m_outerBoundsDirty = TRUE;

        //
        // Visibility - Dirties: (Render | Bounds)
        //
        pUIE->NWPropagateDirtyFlag(flags | DirtyFlags::Render | DirtyFlags::Bounds);
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      This RENDERCHANGEDPFN marks this UIElement's transform as dirty for rendering.
//
//------------------------------------------------------------------------
/* static */ void
CUIElement::NWSetTransformDirty(_In_ CDependencyObject *pTarget, DirtyFlags flags)
{
    ASSERT(pTarget->OfTypeByIndex<KnownTypeIndex::UIElement>());
    CUIElement *pUIE = static_cast<CUIElement*>(pTarget);

    if (!flags_enum::is_set(flags, DirtyFlags::Independent))
    {
        //
        // Transform - Dirties: (Render | Bounds)
        //
        pUIE->NWSetDirtyFlagsAndPropagate(flags | DirtyFlags::Render | DirtyFlags::Bounds, pUIE->m_fNWTransformDirty);
        pUIE->m_fNWTransformDirty = TRUE;
    }
    else
    {
        // Independent changes only dirty bounds.
        pUIE->NWSetDirtyFlagsAndPropagate(DirtyFlags::Independent | DirtyFlags::Bounds, FALSE);
    }

    // TODO: JCOMP: Consider giving RenderTransform its own dirty-handler to prevent extra checks for callers that change offset, DM values, etc

    // Update composition requirements for render transform, if necessary.
    // The composition engine only supports 2D axis-alignment-preserving transformations on primitives.
    // If any other transform is encountered, a composition node must be created to handle it.
    bool hasAxisUnalignedRenderTransform;
    auto renderTransform = pUIE->GetRenderTransform();
    if (renderTransform != nullptr)
    {
        CMILMatrix renderTransformMatrix;
        renderTransform->GetTransform(&renderTransformMatrix);
        hasAxisUnalignedRenderTransform = !renderTransformMatrix.IsScaleOrTranslationOnly();
    }
    else
    {
        hasAxisUnalignedRenderTransform = FALSE;
    }

    if (!pUIE->m_hasAxisUnalignedTransform && hasAxisUnalignedRenderTransform)
    {
        HRESULT hr = pUIE->SetRequiresComposition(
            CompositionRequirement::RenderTransformAxisUnaligned,
            IndependentAnimationType::None
            );
        XCP_FAULT_ON_FAILURE(SUCCEEDED(hr));
    }
    else if (pUIE->m_hasAxisUnalignedTransform && !hasAxisUnalignedRenderTransform)
    {
        ASSERT(pUIE->HasComplexTransformation());
        pUIE->UnsetRequiresComposition(
            CompositionRequirement::RenderTransformAxisUnaligned,
            IndependentAnimationType::None
            );
    }

    // NOTE:  This doesn't make the same check for the TransitionTarget transform, because complex transforms
    //            on the TransitionTarget are not expected or supported. Those transforms are expected to remain
    //            axis-aligned, even when animated.
#if DBG
    if (pUIE->GetTransitionTarget() != NULL && pUIE->GetTransitionTarget()->m_pxf != NULL)
    {
        CMILMatrix transitionClipTransform;
        pUIE->GetTransitionTarget()->m_pxf->GetTransform(&transitionClipTransform);
        ASSERT(transitionClipTransform.IsScaleOrTranslationOnly());
    }
#endif
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      This RENDERCHANGEDPFN marks this UIElement's projection as dirty for rendering.
//
//------------------------------------------------------------------------
/* static */ void
CUIElement::NWSetProjectionDirty(_In_ CDependencyObject *pTarget, DirtyFlags flags)
{
    ASSERT(pTarget->OfTypeByIndex<KnownTypeIndex::UIElement>());
    CUIElement *pUIE = static_cast<CUIElement*>(pTarget);

    if (!flags_enum::is_set(flags, DirtyFlags::Independent))
    {
        //
        // Projection - Dirties: (Render | Bounds)
        //
        pUIE->NWSetDirtyFlagsAndPropagate(flags | DirtyFlags::Render | DirtyFlags::Bounds, pUIE->m_fNWProjectionDirty);

        if (!pUIE->m_fNWProjectionDirty)
        {
            pUIE->m_fNWProjectionDirty = TRUE;
        }
    }
    else
    {
        // Independent changes only dirty bounds.
        pUIE->NWSetDirtyFlagsAndPropagate(DirtyFlags::Independent | DirtyFlags::Bounds, FALSE);
    }

    // Update composition requirements for projection, if necessary.
    // The composition engine only supports 2D axis-alignment-preserving transformations on primitives.
    // If any projection is encountered, a composition node must be created to handle it.
    bool hasProjection = pUIE->HasActiveProjection();
    if (!pUIE->m_hasNonIdentityProjection && hasProjection)
    {
        HRESULT hr = pUIE->SetRequiresComposition(
            CompositionRequirement::Projection,
            IndependentAnimationType::None
            );
        XCP_FAULT_ON_FAILURE(SUCCEEDED(hr));
    }
    else if (pUIE->m_hasNonIdentityProjection && !hasProjection)
    {
        ASSERT(pUIE->HasComplexTransformation());
        pUIE->UnsetRequiresComposition(
            CompositionRequirement::Projection,
            IndependentAnimationType::None
            );
    }
}

/* static */ void
CUIElement::NWSetTransform3DDirty(_In_ CDependencyObject *pTarget, DirtyFlags flags)
{
    ASSERT(pTarget->OfTypeByIndex<KnownTypeIndex::UIElement>());
    CUIElement *pUIE = static_cast<CUIElement*>(pTarget);

    if (!flags_enum::is_set(flags, DirtyFlags::Independent))
    {
        //
        // Transform3D - Dirties: (Render | Bounds)
        //
        pUIE->NWSetDirtyFlagsAndPropagate(flags | DirtyFlags::Render | DirtyFlags::Bounds, pUIE->m_isTransform3DDirty);

        if (!pUIE->m_isTransform3DDirty)
        {
            pUIE->m_isTransform3DDirty = TRUE;
        }
    }
    else
    {
        // Independent changes only dirty bounds.
        pUIE->NWSetDirtyFlagsAndPropagate(DirtyFlags::Independent | DirtyFlags::Bounds, FALSE);
    }

    bool hasTransform3D = (pUIE->GetTransform3D() != nullptr);

    if (!pUIE->m_hasTransform3D && hasTransform3D)
    {
        IFCFAILFAST(pUIE->SetRequiresComposition(
            CompositionRequirement::Transform3D,
            IndependentAnimationType::None
            ));
    }
    else if (pUIE->m_hasTransform3D && !hasTransform3D)
    {
        pUIE->UnsetRequiresComposition(
            CompositionRequirement::Transform3D,
            IndependentAnimationType::None
            );
    }

    ASSERT(pUIE->m_hasTransform3D == hasTransform3D);
    pUIE->UpdateHas3DDepth();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      This RENDERCHANGEDPFN marks this UIElement's clip as dirty for rendering.
//
//------------------------------------------------------------------------
/* static */ void
CUIElement::NWSetClipDirty(_In_ CDependencyObject *pTarget, DirtyFlags flags)
{
    ASSERT(pTarget->OfTypeByIndex<KnownTypeIndex::UIElement>());
    CUIElement *pUIE = static_cast<CUIElement*>(pTarget);

    if (!flags_enum::is_set(flags, DirtyFlags::Independent))
    {
        //
        // Clip - Dirties: (Render | Bounds)
        //
        pUIE->NWSetDirtyFlagsAndPropagate(flags | DirtyFlags::Render | DirtyFlags::Bounds, pUIE->m_fNWClipDirty);
        pUIE->m_fNWClipDirty = TRUE;
    }
    else
    {
        // Independent changes only dirty bounds.
        pUIE->NWSetDirtyFlagsAndPropagate(DirtyFlags::Independent | DirtyFlags::Bounds, FALSE);
    }

    // Update composition requirements for the clip, if necessary.
    // The composition engine only supports axis-aligned clips on primitives.
    // If any other clip is encountered, a composition node must be created to handle it.
    bool hasAxisUnalignedLocalClip;
    auto clip = pUIE->GetClip();
    if (clip != nullptr && clip->m_pTransform != NULL)
    {
        CMILMatrix clipTransform;
        clip->m_pTransform->GetTransform(&clipTransform);
        hasAxisUnalignedLocalClip = !clipTransform.IsScaleOrTranslationOnly();
    }
    else
    {
        hasAxisUnalignedLocalClip = FALSE;
    }

    if (!pUIE->m_hasAxisUnalignedLocalClip && hasAxisUnalignedLocalClip)
    {
        HRESULT hr = pUIE->SetRequiresComposition(
            CompositionRequirement::LocalClipAxisUnaligned,
            IndependentAnimationType::None
            );
        XCP_FAULT_ON_FAILURE(SUCCEEDED(hr));
    }
    else if (pUIE->m_hasAxisUnalignedLocalClip && !hasAxisUnalignedLocalClip)
    {
        ASSERT(pUIE->HasComplexTransformation());
        pUIE->UnsetRequiresComposition(
            CompositionRequirement::LocalClipAxisUnaligned,
            IndependentAnimationType::None
            );
    }

    // NOTE:  This doesn't make the same check for the TransitionTarget clip, because complex clip transforms
    //            on the TransitionTarget are not expected or supported. Those clips are expected to remain
    //            axis-aligned, even when animated.
#if DBG
    if (pUIE->GetTransitionTarget() != NULL && pUIE->GetTransitionTarget()->m_pClipTransform != NULL)
    {
        CMILMatrix transitionClipTransform;
        pUIE->GetTransitionTarget()->m_pClipTransform->GetTransform(&transitionClipTransform);
        ASSERT(transitionClipTransform.IsScaleOrTranslationOnly());
    }
#endif
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Marks this UIElement's layout clip as dirty for rendering.
//      Note: not a RENDERCHANGEDPFN. The layout clip gets generated and
//      set internally.
//
//------------------------------------------------------------------------
void
CUIElement::SetLayoutClipDirty()
{
    //
    // Layout clip - Dirties: (Render | Bounds)
    //
    NWSetDirtyFlagsAndPropagate(DirtyFlags::Render | DirtyFlags::Bounds, m_fNWLayoutClipDirty);
    m_fNWLayoutClipDirty = TRUE;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      This RENDERCHANGEDPFN marks this UIElement's transitiontarget as dirty.
//      Since a TransitionTarget is a layer of indirection for number of rendering properties,
//      it manages invalidating the UIElement itself.
//
//------------------------------------------------------------------------
/* static */ void
CUIElement::NWSetTransitionTargetDirty(_In_ CDependencyObject *pTarget, DirtyFlags flags)
{
    ASSERT(pTarget->OfTypeByIndex<KnownTypeIndex::UIElement>());
    CUIElement *pUIE = static_cast<CUIElement*>(pTarget);

    auto transitionTarget = pUIE->GetTransitionTarget();
    if (transitionTarget != nullptr)
    {
        if (!flags_enum::is_set(flags, DirtyFlags::Independent))
        {
            transitionTarget->NWSetPropertyDirtyOnTarget(pUIE, flags);
        }
        else if (flags_enum::is_set(flags, DirtyFlags::Bounds))
        {
            // Independent changes only dirty bounds.
            transitionTarget->NWSetPropertyDirtyOnTarget(pUIE, DirtyFlags::Independent | DirtyFlags::Bounds);
        }
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Pushes a clip for rendering. Uses PushAxisAlignedClip when
//      possible.
//
//------------------------------------------------------------------------
/* static */ _Check_return_ HRESULT
CUIElement::D2DSetUpClipHelper(
    _In_ IPALAcceleratedRenderTarget *pD2DRenderTarget,
    bool pushAxisAlignedClip,
    _In_opt_ IPALAcceleratedGeometry *pAcceleratedClipGeometry,
    _In_opt_ const XRECTF_RB *pContentBounds,
    _Inout_ bool *pPushedClipLayer,
    _Inout_ bool *pPushedAxisAlignedClip,
    _Out_ bool *pIsClipEmpty
    )
{
    //
    // Three possibilities:
    // 1. No clip. Continue rendering the element.
    // 2. Has an empty clip. Don't render the element.
    // 3. Has a non-empty clip. Push the clip, render the element, then pop the clip.
    //

    if (pAcceleratedClipGeometry != NULL)
    {
        XRECTF_RB clipBounds;
        IFC_RETURN(pAcceleratedClipGeometry->GetBounds(&clipBounds));

        if (clipBounds.right > clipBounds.left && clipBounds.bottom > clipBounds.top)
        {
            // Case 3. Non-empty clip.

            if (pContentBounds &&
                pAcceleratedClipGeometry->IsAxisAlignedRectangle() &&
                DoesRectContainRect(&clipBounds, pContentBounds))
            {
                // Content is contained by the clip rect
                // so there is no need to do clipping
                *pPushedClipLayer = FALSE;
                *pPushedAxisAlignedClip = FALSE;
                *pIsClipEmpty = FALSE;
            }
            else if (pushAxisAlignedClip)
            {
                pD2DRenderTarget->PushAxisAlignedClip(&clipBounds);

                *pPushedClipLayer = FALSE;
                *pPushedAxisAlignedClip = TRUE;
                *pIsClipEmpty = FALSE;
            }
            else
            {
                IFC_RETURN(pD2DRenderTarget->PushClipLayer(
                    pAcceleratedClipGeometry,
                    pContentBounds
                    ));

                *pPushedClipLayer = TRUE;
                *pPushedAxisAlignedClip = FALSE;
                *pIsClipEmpty = FALSE;
            }
        }
        else
        {
            // Case 2. Empty clip.
            *pPushedClipLayer = FALSE;
            *pPushedAxisAlignedClip = FALSE;
            *pIsClipEmpty = TRUE;
        }
    }
    else
    {
        // Case 1. No clip.
        *pPushedClipLayer = FALSE;
        *pPushedAxisAlignedClip = FALSE;
        *pIsClipEmpty = FALSE;
    }

    // We shouldn't have pushed both a clip layer and an axis aligned clip.
    ASSERT(!*pPushedClipLayer || !*pPushedAxisAlignedClip);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Pops a clip layer or an axis aligned clip.
//
//------------------------------------------------------------------------
/* static */ _Check_return_ HRESULT
CUIElement::D2DPopClipHelper(
    _In_ IPALAcceleratedRenderTarget *pD2DRenderTarget,
    bool pushedClipLayer,
    bool pushedAxisAlignedClip
    )
{
    // We shouldn't have pushed both a clip layer and an axis aligned clip.
    ASSERT(!pushedClipLayer || !pushedAxisAlignedClip);

    if (pushedClipLayer)
    {
        IFC_RETURN(pD2DRenderTarget->PopLayer());
    }
    else if (pushedAxisAlignedClip)
    {
        pD2DRenderTarget->PopAxisAlignedClip();
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      The start of the recursive print walk. Pushes the element transform and calls
//      PrintTransformed.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CUIElement::Print(
    _In_ const SharedRenderParams& sharedPrintParams,
    _In_ const D2DPrecomputeParams& cp,
    _In_ const D2DRenderParams& printParams
    )
{
    CMILMatrix localTransform;
    bool pushedLayoutClip = false;

    // Copy render params since they'll be modified to handle transform, layers, etc.
    D2DPrecomputeParams myPrecomputeParams(cp);
    SharedRenderParams mySharedPrintParams(sharedPrintParams);

    // If an element isn't visible no work needs to be done for the entire subgraph.
    if (!IsVisible() || GetOpacityCombined() == 0.0f)
    {
        return S_OK;
    }

    // Vector printing does not handle projections.
    if (!printParams.m_fForceVector && HasActiveProjection())
    {
        IFC_RETURN(AgError(AG_E_NO_VECTOR_PRINT));
    }

    if (ShouldApplyLayoutClipAsAncestorClip())
    {
        // In ContainerVisuals mode, we apply the LayoutClip as an ancestor clip.
        IFC_RETURN(PrintPushLayoutClip(myPrecomputeParams, printParams, &pushedLayoutClip));
    }

    // AlphaMask generation which uses overrideTransform pre-calculates the render transform so there
    // is no need to append the local transform here.
    if (!sharedPrintParams.overrideTransform)
    {
        // When rendering, we don't care about properties set by composition on the handoff visual. That will prevent problems like
        // creating a tiny mask because there's a WUC scale animation going on.
        if(!GetLocalTransform(TransformRetrievalOptions::None, &localTransform))
        {
            // Append this elements transform to the current transform on the stack.
            if (sharedPrintParams.pCurrentTransform)
            {
                localTransform.Append(*sharedPrintParams.pCurrentTransform);
            }
            mySharedPrintParams.pCurrentTransform = &localTransform;
        }
        else if (!mySharedPrintParams.pCurrentTransform)
        {
            // Set the current transform to identity.
            mySharedPrintParams.pCurrentTransform = &localTransform;
        }
    }

    IFC_RETURN(printParams.GetD2DRenderTarget()->SetTransform(mySharedPrintParams.pCurrentTransform));

    // Print this element with its transform.
    IFC_RETURN(PrintTransformed(mySharedPrintParams, myPrecomputeParams, printParams));

    // Restore the parent element's transform.
    if (sharedPrintParams.pCurrentTransform)
    {
        IFC_RETURN(printParams.GetD2DRenderTarget()->SetTransform(sharedPrintParams.pCurrentTransform));
    }

    if (pushedLayoutClip)
    {
        IFC_RETURN(PrintPopClip(printParams));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Pushes layout and render clips, prints pre-children content,
//      recursively prints children and pops the clips.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CUIElement::PrintTransformed(
    _In_ const SharedRenderParams& sharedPrintParams,
    _In_ const D2DPrecomputeParams& cp,
    _In_ const D2DRenderParams& printParams
    )
{
    bool pushedUIEClip = false;
    bool pushedLayoutClip = false;

    IFC_RETURN(PrintPushUIEClip(cp, printParams, &pushedUIEClip));

    if (!ShouldApplyLayoutClipAsAncestorClip())
    {
        IFC_RETURN(PrintPushLayoutClip(cp, printParams, &pushedLayoutClip));
    }

    IFC_RETURN(PrintPushOpacity(printParams));

    IFC_RETURN(PreChildrenPrintVirtual(sharedPrintParams, cp, printParams));

    IFC_RETURN(PrintChildren(sharedPrintParams, cp, printParams));

    IFC_RETURN(PrintPopOpacity(printParams));

    if (pushedUIEClip)
    {
        IFC_RETURN(PrintPopClip(printParams));
    }

    if (pushedLayoutClip)
    {
        IFC_RETURN(PrintPopClip(printParams));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Overridden by various elements to render pre-children content.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CUIElement::PreChildrenPrintVirtual(
    _In_ const SharedRenderParams& sharedPrintParams,
    _In_ const D2DPrecomputeParams& cp,
    _In_ const D2DRenderParams& printParams
    )
{
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Calls Print on the children.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CUIElement::PrintChildren(
    _In_ const SharedRenderParams& sharedPrintParams,
    _In_ const D2DPrecomputeParams& cp,
    _In_ const D2DRenderParams& printParams
    )
{
    const CUIElementCollection *pCollection = GetChildren();

    if (pCollection)
    {
        XUINT32 uiChildCount;
        CUIElement **ppUIElements;

        GetChildrenInRenderOrder(&ppUIElements, &uiChildCount);

        for (XUINT32 i = 0; i < uiChildCount; i++)
        {
            IFC_RETURN(ppUIElements[i]->Print(sharedPrintParams, cp, printParams));
        }
    }

    return S_OK;
}

// Push the UIElement.Clip onto the current print rendering context
_Check_return_ HRESULT CUIElement::PrintPushUIEClip(
    _In_ const D2DPrecomputeParams& cp,
    _In_ const D2DRenderParams& printParams,
    _Out_ bool* pushedUIEClip
    )
{
    xref_ptr<IPALAcceleratedGeometry> palClipGeometry;
    *pushedUIEClip = false;

    auto clip = GetClip();
    if (clip != nullptr)
    {
        IFC_RETURN(clip->GetPrintGeometry(cp, printParams, palClipGeometry.ReleaseAndGetAddressOf()));
        if (palClipGeometry != nullptr)
        {
            IFC_RETURN(printParams.GetD2DRenderTarget()->PushClipLayer(
                palClipGeometry,
                nullptr /* pContentBounds */
                ));

            *pushedUIEClip = true;
        }
    }

    return S_OK;
}

// Push the LayoutClip onto the current print rendering context
_Check_return_ HRESULT
CUIElement::PrintPushLayoutClip(
    _In_ const D2DPrecomputeParams& cp,
    _In_ const D2DRenderParams& printParams,
    _Out_ bool* pushedLayoutClip
    )
{
    xref_ptr<IPALAcceleratedGeometry> palLayoutClipGeometry;
    *pushedLayoutClip = false;

    if (HasLayoutClip())
    {
        IFC_RETURN(LayoutClipGeometry->GetPrintGeometry(cp, printParams, palLayoutClipGeometry.ReleaseAndGetAddressOf()));
        if (palLayoutClipGeometry != nullptr)
        {
            IFC_RETURN(printParams.GetD2DRenderTarget()->PushClipLayer(
                palLayoutClipGeometry,
                nullptr /* pContentBounds */
                ));

            *pushedLayoutClip = true;
        }
    }

    return S_OK;
}

// Pop a single clip layer off the current print rendering context
/* static */
_Check_return_ HRESULT CUIElement::PrintPopClip(_In_ const D2DRenderParams& printParams)
{
    IFC_RETURN(printParams.GetD2DRenderTarget()->PopLayer());

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Sets up a layer to handle the opacity.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CUIElement::PrintPushOpacity(_In_ const D2DRenderParams &printParams)
{
    // There's no need to add the geometry to a layer with 0 opacity.
    if (GetOpacityCombined() < 1.0f)
    {
        IFC_RETURN(printParams.GetD2DRenderTarget()->PushOpacityLayer(
            GetOpacityCombined(),
            nullptr /* pContentBounds */
            ));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Pops the opacity layer.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CUIElement::PrintPopOpacity(_In_ const D2DRenderParams &printParams)
{
    if (GetOpacityCombined() < 1.0f)
    {
        IFC_RETURN(printParams.GetD2DRenderTarget()->PopLayer());
    }

    return S_OK;
}



//------------------------------------------------------------------------
//
//  Synopsis: Called to make sure there is storage for LT values
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CUIElement::EnsureLayoutTransitionStorage(
    _In_     CDependencyObject   *pObject,
    _In_opt_ const CDependencyProperty *pDp,
    _In_     bool forGetValue // If this is a GetValue call
)
{
    HRESULT hr = S_OK;

    CUIElement* pThis = do_pointer_cast<CUIElement>(pObject);

    if (pThis && !pThis->m_pLayoutTransitionStorage)
    {
        LayoutTransitionStorage *layoutTransitionData = new LayoutTransitionStorage();
        CLayoutManager* pLayoutManager = VisualTree::GetLayoutManagerForElement(pObject);
        pThis->m_pLayoutTransitionStorage = layoutTransitionData;


        if (pThis->GetLayoutStorage())
        {
            // need to fill out as much as possible
            layoutTransitionData->m_currentSize.width = layoutTransitionData->m_nextGenerationSize.width = pThis->GetActualWidth();
            layoutTransitionData->m_currentSize.height = layoutTransitionData->m_nextGenerationSize.height = pThis->GetActualHeight();
            layoutTransitionData->m_currentOffset.x = layoutTransitionData->m_nextGenerationOffset.x = pThis->GetActualOffsetX();
            layoutTransitionData->m_currentOffset.y = layoutTransitionData->m_nextGenerationOffset.y = pThis->GetActualOffsetY();
            if (pLayoutManager)
            {
                layoutTransitionData->m_nextGenerationCounter = pLayoutManager->GetNextLayoutCounter();
            }

            layoutTransitionData->m_opacityCache = pThis->GetOpacityToRoot();
        }
    }

    RRETURN(hr);    // RRETURN_REMOVAL
}

//------------------------------------------------------------------------
//
//  Synopsis: Walks the tree (analogous to measure and arranges) to perform
//            new measure and arranges for items that have changed size.
//
//  Note: although we also take care of location (visualoffset), it is not
//        strictly necessary to invalidate the LayoutTransition when only
//        location has changed.
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElement::TransitionLayout(_In_ CUIElement *pRoot,_In_  XRECTF rootRect)
{
    CLayoutManager* pLayoutManager = VisualTree::GetLayoutManagerForElement(this);
    IFCPTR_RETURN(pLayoutManager);

    pLayoutManager->PushCurrentLayoutElement(this);
    auto layoutElementStackCleanupGuard = wil::scope_exit([&]
    {
        if (pLayoutManager)
        {
            VERIFY_COND(pLayoutManager->PopCurrentLayoutElement(), == this);
        }
    });

    SetIsOnLayoutTransitionStack(TRUE);
    SetIsAncestorDirty(FALSE);
    pLayoutManager->SetTransitioningElement(this);

    auto cleanupGuard = wil::scope_exit([&]() {
        SetIsOnLayoutTransitionStack(FALSE);
        if (pLayoutManager)
        {
            pLayoutManager->SetTransitioningElement(nullptr);
        }
    });

    if (GetIsLayoutElement() || GetIsParentLayoutElement())
    {
        IFC_RETURN(EnsureLayoutStorage()); // Required for constraint comparison
    }

    if (GetIsLayoutTransitionDirty())
    {
        // after a layoutdirty pass has occurred, all information is reset, except the
        // information used during the render pass.
        XSIZEF originalDesiredSize;
        XSIZEF originalPreviousConstraint;
        XSIZEF originalUnclippedDesiredSize;
        XRECTF originalFinalRect;

        // setup caches
        originalDesiredSize = DesiredSize;
        originalPreviousConstraint = PreviousConstraint;    // todo: can we use previous constraint to improve perf?
        originalUnclippedDesiredSize = UnclippedDesiredSize;
        originalFinalRect = FinalRect;

        IFC_RETURN(CTransition::GetLayoutSlotDuringTransition(this, &FinalRect));

        InvalidateMeasure();

        // emulate a full new walk
        // a full walk is needed because the sizes of parent and child must
        // be independent of each other.
        // From a perf pov this might look bad, however, keep in mind that the walk is _extremely_ direct
        // and no upward walks are ever performed while in LT phase.
        IFC_RETURN(pRoot->Measure(rootRect.Size()));
        IFC_RETURN(pRoot->Arrange(rootRect));

        // restore information needed by following measures and arranges
        DesiredSize = originalDesiredSize;
        PreviousConstraint = originalPreviousConstraint;
        UnclippedDesiredSize = originalUnclippedDesiredSize;
        FinalRect = originalFinalRect;

        SetIsLayoutTransitionDirty(FALSE);
    }

    if (GetIsOnLayoutTransitionDirtyPath())
    {
        SetIsOnLayoutTransitionDirtyPath(FALSE);

        auto children = GetUnsortedChildren();
        UINT32 count = children.GetCount();

        for (XUINT32 childIndex = 0; childIndex < count; ++childIndex)
        {
            CUIElement* pChild = children[childIndex];

            if (pChild != NULL && pChild->GetRequiresLayoutTransition())
            {
                IFC_RETURN(pChild->TransitionLayout(pRoot, rootRect));
            }
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis: When an element is put back into the tree, it might still be
//  associated. Normally that is disallowed, but in the case of that
//  element actually being in the process of unloading, we can recover
//  by stopping that unload.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CUIElement::OnAssociationFailure()
{
    CUIElement* pParent = this;

    while (pParent)
    {
        const LayoutTransitionStorage* pStorage = pParent->GetLayoutTransitionStorage();
        if (pStorage)
        {
            if (CTransition::HasActiveTransition(pParent, DirectUI::TransitionTrigger::Unload))
            {
                IFC_RETURN(CTransition::CancelTransitions(pParent));
                break;
            }
        }
        pParent = do_pointer_cast<CUIElement>(pParent->GetParentInternal());
    }

    // If this element is running its Hide animation, cancel it. Same goes for its child and unloading child.
    CancelImplicitAnimation(ImplicitAnimationType::Hide);
    if (OfTypeByIndex<KnownTypeIndex::Popup>())
    {
        CPopup* popup = static_cast<CPopup*>(this);
        popup->CancelHideAnimationToPrepareForShow();
    }

    // Finish unloading and stop tracking keep-visible status as we're no longer in the same place in the tree
    // Note that if this takes the element out of the tree, it will automatically cancel any implicit hide animations
    // in LeavePCSceneRecursive.
    FlushPendingKeepVisibleOperations();

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns TRUE if this element has any absolutely positioned layout transition
//      renderers.
//
//------------------------------------------------------------------------
bool CUIElement::HasAbsolutelyPositionedLayoutTransitionRenderers() const
{
    if (m_pLayoutTransitionRenderers != nullptr && !m_pLayoutTransitionRenderers->empty())
    {
        xvector<CLayoutTransitionElement*>::const_reverse_iterator rend = m_pLayoutTransitionRenderers->rend();

        for (xvector<CLayoutTransitionElement*>::const_reverse_iterator it = m_pLayoutTransitionRenderers->rbegin(); it != rend; ++it)
        {
            if ((*it)->IsAbsolutelyPositioned())
            {
                return true;
            }
        }
    }

    return false;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Removes any absolutely positioned layout transition renderers.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CUIElement::RemoveAbsolutelyPositionedLayoutTransitionRenderers()
{
    if (m_pLayoutTransitionRenderers != nullptr && !m_pLayoutTransitionRenderers->empty())
    {
        xvector<CLayoutTransitionElement*>::const_reverse_iterator rend = m_pLayoutTransitionRenderers->rend();

        bool hasLeftScene = false;
        for (xvector<CLayoutTransitionElement*>::const_reverse_iterator it = m_pLayoutTransitionRenderers->rbegin(); it != rend; ++it)
        {
            if ((*it)->IsAbsolutelyPositioned())
            {
                if (!hasLeftScene)
                {
                    hasLeftScene = true;

                    // Since this element is being rendered in another part of the tree, remove it and any render
                    // data in the subgraph from the scene. It'll be added back whenever its regular visual parent renders it.
                    LeavePCSceneRecursive();

                    // The element needs to be marked dirty to be sure the path down through the 'regular' visual parent is
                    // walked and the element's subgraph is regenerated.
                    // TODO: HWPC: Choosing 'transform dirty' is somewhat arbitrary, since the subgraph should be entirely regenerated because it will be entering the scene again anyway.
                    CUIElement::NWSetTransformDirty(this, DirtyFlags::Render);
                }

                CLayoutTransitionElement *pLTE = *it;

                CDependencyObject* lteParent = pLTE->GetParentInternal(false);
                ASSERT(lteParent->OfTypeByIndex<KnownTypeIndex::TransitionRoot>());
                CTransitionRoot* pLayer = static_cast<CTransitionRoot*>(lteParent);
                // if we don't have this, we are seriously in trouble: we won't be able to ever remove the LTE, leaving the lte visible on the screen forever
                ASSERT(pLayer);

                // Pass in NULL to prevent Detach from erasing the current entry from m_pLayoutTransitionRenders.
                // We'll do it here, in the loop, instead.
                IFC_RETURN(pLTE->DetachTransition(nullptr /*pTarget*/, pLayer));

                // Since we passed a null target, we skipped the part of CUIElement::RemoveLayoutTransitionRenderer
                // that propagates a dirty flag up the tree. Do that manually.
                // Note: The NWSetTransformDirty call above tries to propagate dirty flags up the tree, but it's
                // insufficient. At that point, there are still LTE entries in m_pLayoutTransitionRenderers,
                // so we'll propagate dirty flags up to the LTE instead of up the real tree. It's also possible
                // that the element is already marked with transform dirty (which is a problem no matter which
                // dirty flag we use), in which case the element wouldn't propagate a dirty flag up the tree at
                // all.
                CDependencyObject* thisParent = GetParentInternal(false);
                if (thisParent != nullptr)
                {
                    ASSERT(thisParent->OfTypeByIndex<KnownTypeIndex::UIElement>());
                    CUIElement::NWSetSubgraphDirty(thisParent, DirtyFlags::Render | DirtyFlags::Bounds);
                }

                IFC_RETURN(m_pLayoutTransitionRenderers->erase(it));
            }
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Static factory method
//
//------------------------------------------------------------------------
/*static*/ _Check_return_ HRESULT CLayoutTransitionElement::Create(
    _In_ CUIElement *pTargetUIE,
    bool isAbsolutelyPositioned,
    _Outptr_ CLayoutTransitionElement **ppLTE
    )
{
    xref_ptr<CLayoutTransitionElement> LTE;
    LTE.attach(new CLayoutTransitionElement(pTargetUIE, isAbsolutelyPositioned));

    IFC_RETURN(LTE->Initialize());

    *ppLTE = LTE.detach();

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Override to allow a default render transform to be set which
//      the transition will target.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CLayoutTransitionElement::Initialize()
{
    CREATEPARAMETERS cp(GetContext());

    xref_ptr<CCompositeTransform> destinationTransform;
    IFC_RETURN(CreateDO(destinationTransform.ReleaseAndGetAddressOf(), &cp));

    // this element will rely on this configuration.
    IFC_RETURN(SetValueByKnownIndex(KnownPropertyIndex::UIElement_RenderTransform, destinationTransform.get()));

    xref_ptr<CDependencyObject> transitionTarget;
    IFC_RETURN(CTransitionTarget::Create(transitionTarget.ReleaseAndGetAddressOf(), &cp));
    IFC_RETURN(SetValueByKnownIndex(KnownPropertyIndex::UIElement_TransitionTarget, transitionTarget.get()));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Set-up method to add the LTE to the visual tree and get it ready for rendering.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CLayoutTransitionElement::AttachTransition(
    _In_ CUIElement *pTarget,
    _In_ CTransitionRoot *pParent
    )
{
    IFC_RETURN(pParent->AddChild(this));

    IFC_RETURN(SetRequiresComposition(
        CompositionRequirement::RedirectionElement,
        IndependentAnimationType::None /*detailedAnimationReason*/
        ));

    IFC_RETURN(pTarget->AddLayoutTransitionRenderer(this));

    ASSERT(!m_isRegisteredOnCore);
    IFC_RETURN(GetContext()->RegisterRedirectionElement(this));
    m_isRegisteredOnCore = TRUE;

    pParent->ReEvaluateRequiresCompNode();

    return S_OK;
}

void CLayoutTransitionElement::AttachTransition(_In_ CTransitionRoot *parent)
{
    CUIElement *targetNoRef = GetTargetElement();
    IFCFAILFAST(AttachTransition(targetNoRef, parent));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Clean-up method to remove the LTE from the visual tree, and clean-up its render
//      state.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CLayoutTransitionElement::DetachTransition(
    _In_opt_ CUIElement *pTarget,
    _In_ CTransitionRoot *pParent
    )
{
    // This assert is triggered on WPF/Win32 mode when the OnUnloaded event is called
    // Bug 18096379: XamlObjects events can be fired after its parent Island has been Disposed
    // This because the XamlIslandRoot that owns the Xaml tree has been Disposed.
    LOG_ASSERT((this->GetParentInternal(false) == pParent));
    ASSERT(m_isRegisteredOnCore);
    IFC_RETURN(GetContext()->UnregisterRedirectionElement(this));
    m_isRegisteredOnCore = FALSE;

    if (pTarget)
    {
        IFC_RETURN(pTarget->RemoveLayoutTransitionRenderer(this));
    }

    UnsetRequiresComposition(
        CompositionRequirement::RedirectionElement,
        IndependentAnimationType::None /*detailedAnimationReason*/
        );

    // The transition root is not considered to be in the tree. When we remove the LTE from it, it will not trigger Leave on the
    // LTE. We normally rely on the Leave() walk to call LeavePCScene to clean up PC render data and the composition peer. If it
    // isn't called, then the composition peer leaks and can mess up ordering of our comp tree nodes. Call LeavePCScene explicitly
    // here to clean up the composition peer.
    LeavePCSceneRecursive();

    IFC_RETURN(pParent->RemoveChild(this));

    pParent->ReEvaluateRequiresCompNode();

    return S_OK;
}

void CLayoutTransitionElement::DetachTransition()
{
    CUIElement *targetNoRef = GetTargetElement();

    CDependencyObject *parentDONoRef = GetParent();
    ASSERT(parentDONoRef->OfTypeByIndex<KnownTypeIndex::TransitionRoot>());
    CTransitionRoot *parentNoRef = static_cast<CTransitionRoot*>(parentDONoRef);

    IFCFAILFAST(DetachTransition(targetNoRef, parentNoRef));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Clears secondary render data storage for this LTE.
//
//------------------------------------------------------------------------
void
CLayoutTransitionElement::ClearSecondaryRenderData()
{
    PrimitiveCompositionPropertyData::ClearRenderDataList(m_pSecondaryPathRenderData, GetContext());
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      If no animation needed to be setup to target the translates,
//      the offset should mirror the Targets transition. If a translate is
//      targeted that value includes all considerations of layout (margin, etc).
//
//      Needed for rendering, the rendertransform applied will be on top of this.
//
//      Although m_destinationOffset is a copy of m_pTarget's offset made at the
//      moment of setting up the transition, it needs to be independent of
//      m_pTarget's offset since we need accurate information at a next layout cycle
//      about where this LTE is on the screen (for instance, a new layout comes in,
//      changes offset of the target, and we figure we need to setup a new transition.
//      The start position of this interruption transition would be based on the new
//      offset, where we need to old offset).
//
//------------------------------------------------------------------------
_Check_return_ XFLOAT CLayoutTransitionElement::GetActualOffsetX()
{
   return m_destinationOffset.x;
}

_Check_return_ XFLOAT CLayoutTransitionElement::GetActualOffsetY()
{
   return m_destinationOffset.y;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns whether this element should flip the flow direction and
//      whether the flip should be done in-place.
//      Overridden so that LTE's preserve RTL flipping for their target.
//
//------------------------------------------------------------------------
void CLayoutTransitionElement::GetShouldFlipRTL(
    _Out_ bool *pShouldFlipRTL,
    _Out_ bool *pShouldFlipRTLInPlace
    )
{
    m_pTarget->GetShouldFlipRTL(pShouldFlipRTL, pShouldFlipRTLInPlace);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Gets location of this element relative to its parent.
//
//------------------------------------------------------------------------
_Check_return_ XPOINTF CLayoutTransitionElement::GetPositionRelativeToParent()
{
    XPOINTF location = {0.0f, 0.0f};

    CMILMatrix mtrx = CMILMatrix(TRUE);
    if (!GetLocalTransform(TransformRetrievalOptions::IncludePropertiesSetInComposition, &mtrx))
    {
        mtrx.Transform(&location, &location, 1);
    }
    return location;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Removes the render data for this element from the scene.
//      Overridden to remove secondary storage for multi-path transitions.
//
//------------------------------------------------------------------------
void CLayoutTransitionElement::ClearPCRenderData()
{
    ASSERT(IsInPCScene_IncludingDeviceLost());

    CUIElement::ClearPCRenderData();

    ClearSecondaryRenderData();
    SAFE_DELETE(m_pSecondaryPathRenderData);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns secondary storage for a secondary LTE render walk. The primary walk
//      uses the storage in the target element subgraph.
//
//------------------------------------------------------------------------
PCRenderDataList* CLayoutTransitionElement::GetSecondaryTransitionRenderDataNoRef()
{
    ASSERT(!IsPrimaryTransitionForTarget());

    if (!m_pSecondaryPathRenderData)
    {
        m_pSecondaryPathRenderData = new PCRenderDataList();
    }
    else
    {
        // Ensure that any render data from the previous frame has been cleared, since it
        // will all be regenerated on this frame.
        ClearSecondaryRenderData();
    }

    return m_pSecondaryPathRenderData;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Removes the render data for this element's subgraph from the scene.
//
//------------------------------------------------------------------------
void CLayoutTransitionElement::LeavePCSceneSubgraph()
{
    // The LTE walks to its target directly, instead of the target being walked by its actual visual parent.
    ASSERT(m_pTarget != nullptr);

    // The primary transition is responsible for managing the cached render data in the target's tree.
    // Secondary transitions store the render data storage for the target's subgraph themselves.
    if (IsPrimaryTransitionForTarget())
    {
        m_pTarget->LeavePCSceneRecursive();
    }
}

//----------------------------------------------------------------------------
//
//  Synopsis:
//      The virtual method which does the tree walk to clean up all
//      the device related resources like brushes, textures,
//      primitive composition data etc. in this subgraph.
//
//-----------------------------------------------------------------------------
void CLayoutTransitionElement::CleanupDeviceRelatedResourcesRecursive(_In_ bool cleanupDComp)
{
    // The LTE walks to its target directly, instead of the target being walked by its actual visual parent.
    ASSERT(m_pTarget != nullptr);

    // The primary transition is responsible for managing the cached render data in the target's tree.
    // Secondary transitions store the render data storage for the target's subgraph themselves.
    if (IsPrimaryTransitionForTarget())
    {
        m_pTarget->CleanupDeviceRelatedResourcesRecursive(cleanupDComp);
    }

    // It's important to do this after calling cleanup on the target. In SpriteVisuals mode, a UIElement does not take a
    // reference to its render data. Calling the target to do cleanup will walk its render data list, which assumes the
    // SpriteVisuals are still alive. Those SpriteVisuals are kept alive by the child collection of some parent
    // ContainerVisual. This call to clean up the LTE will clear the content of that ContainerVisual, which will release
    // all those SpriteVisuals, so make sure we're done with them first.
    CUIElement::CleanupDeviceRelatedResourcesRecursive(cleanupDComp);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Get the combined outer bounds of all children.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CLayoutTransitionElement::GenerateChildOuterBounds(
    _In_opt_ HitTestParams *hitTestParams,
    _Out_ XRECTF_RB* pBounds
    )
{
    // Ensure all the bounds are valid. The target isn't walked from the main tree - it's only walked
    // here - so its outer bounds won't have been calculated (and flags cleaned) unless we ask for them.
    XRECTF_RB unused;
    IFC_RETURN(m_pTarget->GetOuterBounds(hitTestParams, &unused));

    // LTEs ignore render properties on their target, so we actually want to return the inner bounds only.
    IFC_RETURN(m_pTarget->GetInnerBounds(hitTestParams, pBounds));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Walk the children of the element finding elements that intersect
//      with the given point.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CLayoutTransitionElement::BoundsTestChildren(
    _In_ const XPOINTF& target,
    _In_ CBoundedHitTestVisitor* pCallback,
    _In_opt_ const HitTestParams *hitTestParams,
    _In_ bool canHitDisabledElements,
    _In_ bool canHitInvisibleElements,
    _Out_opt_ BoundsWalkHitResult* pResult
    )
{
    BoundsWalkHitResult childHitResult = BoundsWalkHitResult::Continue;

    // LTEs ignore render properties on their target - skip directly to the content and children.
    if (m_pTarget->IsEnabledAndVisibleForHitTest(canHitDisabledElements, canHitInvisibleElements))
    {
        IFC_RETURN(m_pTarget->BoundsTestContentAndChildren(target, pCallback, hitTestParams, canHitDisabledElements, canHitInvisibleElements, &childHitResult));
    }

    if (pResult)
    {
        *pResult = childHitResult;
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Walk the children of the element finding elements that intersect
//      with the given polygon.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CLayoutTransitionElement::BoundsTestChildren(
    _In_ const HitTestPolygon& target,
    _In_ CBoundedHitTestVisitor* pCallback,
    _In_opt_ const HitTestParams *hitTestParams,
    _In_ bool canHitDisabledElements,
    _In_ bool canHitInvisibleElements,
    _Out_opt_ BoundsWalkHitResult* pResult
    )
{
    BoundsWalkHitResult childHitResult = BoundsWalkHitResult::Continue;

    // LTEs ignore render properties on their target - skip directly to the content and children.
    if (m_pTarget->IsEnabledAndVisibleForHitTest(canHitDisabledElements, canHitInvisibleElements))
    {
        IFC_RETURN(m_pTarget->BoundsTestContentAndChildren(target, pCallback, hitTestParams, canHitDisabledElements, canHitInvisibleElements, &childHitResult));
    }

    if (pResult)
    {
        *pResult = childHitResult;
    }

    return S_OK;
}

// Returns True when this element has a gesture or manipulation setting that requires gesture processing.
// When ignoreActiveState is False, this element also needs to be in the live tree for this method to return True.
bool CUIElement::IsInteractionEngineRequired(_In_ bool ignoreActiveState) const
{
    if (!ignoreActiveState && !IsActive())
    {
        return false;
    }

    return m_bTapEnabled
        || m_bDoubleTapEnabled
        || m_bRightTapEnabled
        || m_bHoldEnabled
        || ManipulationModes::None != CustomManipulationModes(GetManipulationMode());
}

// Allocates a new CUIDMContainer instance if it was not already created.
_Check_return_ HRESULT CUIElement::CreateDirectManipulationContainer()
{
    ctl::ComPtr<CUIDMContainer> dmContainer;

    const auto dp = MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::UIElement_DirectManipulationContainer);
    if (IsPropertyDefault(dp))
    {
        xref_ptr<IDirectManipulationContainerHandler> dmContainerHandler;
        CValue value;

        IFC_RETURN(CUIDMContainer::Create(&dmContainer, GetContext(), this));
        ASSERT(dmContainer);

        value.WrapIUnknownNoRef(dmContainer.Get());
        IFC_RETURN(SetValue(dp, value));

        IFC_RETURN(CreateDirectManipulationContainerHandler(dmContainerHandler.ReleaseAndGetAddressOf()));
        IFC_RETURN(dmContainer->SetManipulationHandler(dmContainerHandler));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CreateDirectManipulationContainerHandler
//
//  Synopsis:
//    Allocates a new CUIDMContainerHandler instance, which is the
//    IDirectManipulationContainerHandler interface implementation for this
//    CUIElement.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElement::CreateDirectManipulationContainerHandler(
    _Outptr_ IDirectManipulationContainerHandler** ppDirectManipulationContainerHandler)
{
    CUIDMContainerHandler* pUIDMContainerHandler = NULL;

    ASSERT(ppDirectManipulationContainerHandler);
    *ppDirectManipulationContainerHandler = NULL;

    IFC_RETURN(CUIDMContainerHandler::Create(&pUIDMContainerHandler, GetContext()->GetInputServices(), this));
    *ppDirectManipulationContainerHandler = pUIDMContainerHandler;

    return S_OK;
}

// Returns a previously created CUIDMContainer or NULL if none was created.
_Check_return_ HRESULT CUIElement::GetDirectManipulationContainer(_Outptr_ CUIDMContainer** directManipulationContainer)
{
    CValue value;
    ctl::ComPtr<CUIDMContainer> dmContainer;

    *directManipulationContainer = nullptr;

    IFC_RETURN(GetValueByIndex(KnownPropertyIndex::UIElement_DirectManipulationContainer, &value));
    dmContainer = static_cast<CUIDMContainer*>(value.AsIUnknown());
    *directManipulationContainer = dmContainer.Detach();
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Called when this framework element's horizontal or vertical
//      alignment has changed.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElement::OnAlignmentChanged(
    _In_ bool fIsForHorizontalAlignment,
    _In_ bool fIsForStretchAlignment,
    _In_ bool fIsStretchAlignmentTreatedAsNear)
{
    HRESULT hr = S_OK;
    CInputServices* inputServices = GetContext()->GetInputServices();

    if (inputServices)
    {
        inputServices->AddRef();
        IFC(inputServices->NotifyManipulatedElementAlignmentChanged(
            this,
            fIsForHorizontalAlignment,
            fIsForStretchAlignment,
            fIsStretchAlignmentTreatedAsNear));
    }

Cleanup:
    ReleaseInterface(inputServices);
    RRETURN(hr);
}

// Retrieve the manipulation transform directly from DManip
_Check_return_ HRESULT
CUIElement::GetDirectManipulationCompositorTransform(
    _In_ TransformRetrievalOptions transformRetrievalOptions,
    _Out_ BOOL& fTransformSet,
    _Out_ FLOAT& translationX,
    _Out_ FLOAT& translationY,
    _Out_ FLOAT& uncompressedZoomFactor,
    _Out_ FLOAT& zoomFactorX,
    _Out_ FLOAT& zoomFactorY)
{
    CInputServices* inputServices = GetContext()->GetInputServices();
    if (inputServices != nullptr)
    {
        IFC_RETURN(inputServices->GetDirectManipulationCompositorTransform(this, transformRetrievalOptions, fTransformSet, translationX, translationY, uncompressedZoomFactor, zoomFactorX, zoomFactorY));
    }
    else
    {
        fTransformSet = FALSE;
        translationX = 0.0f;
        translationX = 0.0f;
        uncompressedZoomFactor = 1.0f;
        zoomFactorX = 1.0f;
        zoomFactorY = 1.0f;
    }

    return S_OK;
}

// Retrieve the manipulation transform of the clip content directly from DManip
_Check_return_ HRESULT
CUIElement::GetDirectManipulationClipContentTransform(
    _In_ IPALDirectManipulationService* pDirectManipulationService,
    _In_ IObject* pCompositorClipContent,
    _Out_ FLOAT& translationX,
    _Out_ FLOAT& translationY,
    _Out_ FLOAT& zoomFactorX,
    _Out_ FLOAT& zoomFactorY)
{
    CInputServices* inputServices = GetContext()->GetInputServices();
    if (inputServices != nullptr)
    {
        IFC_RETURN(inputServices->GetDirectManipulationClipContentTransform(this /*pClipContentElement*/, pDirectManipulationService, pCompositorClipContent, translationX, translationY, zoomFactorX, zoomFactorY));
    }
    else
    {
        translationX = 0.0f;
        translationX = 0.0f;
        zoomFactorX = 1.0f;
        zoomFactorY = 1.0f;
    }

    return S_OK;
}

// Returns True when this element is:
// - declared manipulatable for DirectManipulation
// - is the primary content of a viewport
_Check_return_ HRESULT
CUIElement::IsManipulatablePrimaryContent(
    _Out_ bool* pIsManipulatablePrimaryContent)
{
    CInputServices* inputServices = GetContext()->GetInputServices();
    if (inputServices != nullptr)
    {
        IFC_RETURN(inputServices->IsManipulatablePrimaryContent(this, pIsManipulatablePrimaryContent));
    }
    else
    {
        *pIsManipulatablePrimaryContent = false;
    }

    return S_OK;
}

// Returns True if this element is manipulatable by DirectManipulation and is a primary content.
// It is then aligned by DirectManipulation as opposed to our layout engine.
_Check_return_ HRESULT
CUIElement::IsAlignedByDirectManipulation(
    _Out_ bool* pIsAlignedByDirectManipulation)
{
    bool isManipulatablePrimaryContent = false;

    *pIsAlignedByDirectManipulation = false;

    IFC_RETURN(IsManipulatablePrimaryContent(&isManipulatablePrimaryContent));
    *pIsAlignedByDirectManipulation = isManipulatablePrimaryContent;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   GetDirectManipulationServiceAndContent
//
//  Synopsis:
//      For this CUIElement, if it is manipulatable, returns the
//      DManip service and content for this element, otherwise nullptrs.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElement::GetDirectManipulationServiceAndContent(
    _Outptr_result_maybenull_ IPALDirectManipulationService** ppDMService,
    _Outptr_result_maybenull_ IObject** ppCompositorContent,
    _Outptr_result_maybenull_ IObject** ppCompositorClipContent,
    _Out_ XDMContentType* pDMContentType,
    _Out_ float* pContentOffsetX,
    _Out_ float* pContentOffsetY)
{
    CInputServices* inputServices = GetContext()->GetInputServices();
    ASSERT(inputServices != nullptr);
    return inputServices->GetDirectManipulationServiceAndContent(this, ppDMService, ppCompositorContent, ppCompositorClipContent, pDMContentType, pContentOffsetX, pContentOffsetY);
}

// Helper function to clear out any DManip shared transforms set and dirty the element
// See comments in InputManager.cpp where we call PrepareCompositionNodesForBringIntoView()
void CUIElement::ResetCompositionNodeManipulationData()
{
    if (m_pCompositionPeer != nullptr)
    {
        m_pCompositionPeer->ResetManipulationData();
        CUIElement::NWSetContentDirty(this, DirtyFlags::Render);
    }
}

// Returns true if this element has a CompNode and that CompNode is using DManip-on-DComp to drive the specified type of transform
// If targetsClip = true, checks for independent Clip Transform, otherwise the Manipulation Transform.
bool CUIElement::HasSharedManipulationTransform(bool targetsClip)
{
    return (m_pCompositionPeer != nullptr && m_pCompositionPeer->HasSharedManipulationTransform(targetsClip));
}

// Helper function to transfer the DManip content to DMDeferredRelease object and clear out
// its content and shared transform for the specified type of transform, in preparation for creating new ones.
// See more details on deferred DM Release in CInputServices::PrepareSecondaryContentRelationshipForCurveUpdate().
void CUIElement::PrepareForSecondaryCurveUpdate(bool targetsClip)
{
    ASSERT(HasSharedManipulationTransform(targetsClip));

    m_pCompositionPeer->PrepareForSecondaryCurveUpdate(targetsClip);
    CUIElement::NWSetContentDirty(this, DirtyFlags::Render);
}

IUnknown* CUIElement::GetSharedPrimaryContentTransform() const
{
    return m_pCompositionPeer ? m_pCompositionPeer->GetSharedPrimaryContentTransform() : nullptr;
}

float CUIElement::GetDirectManipulationContentOffsetX() const
{
    return m_pCompositionPeer ? m_pCompositionPeer->GetDirectManipulationContentOffsetX() : 0;
}

float CUIElement::GetDirectManipulationContentOffsetY() const
{
    return m_pCompositionPeer ? m_pCompositionPeer->GetDirectManipulationContentOffsetY() : 0;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Sets that this element needs off-thread composition work, for the given reason.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CUIElement::SetRequiresComposition(
    CompositionRequirement compositionReason,
    IndependentAnimationType detailedAnimationReason
    ) noexcept
{
#ifdef DMUIEv_DBG
    IGNOREHR(gps->DebugOutputSzNoEndl(
        L"DMUIEv[0x%p]:SetRequiresComposition ThreadID=%d, isForAnimation=%d, isForManipulation=%d, isForClipManipulation=%d\r\n",
        this,
        GetCurrentThreadId(),
        compositionReason == CompositionRequirement::IndependentAnimation,
        compositionReason == CompositionRequirement::IndependentManipulation,
        compositionReason == CompositionRequirement::IndependentClipManipulation
        ));
#endif // DMUIEV_DBG

    bool didRequireCompNode = RequiresCompositionNode();
    bool hadBrushAnimation = IsBrushIndependentlyAnimating();
    bool wasIndependentTarget = IsIndependentTarget();

    IFC_RETURN(CDependencyObject::SetRequiresComposition(compositionReason, detailedAnimationReason));

    // If this element became an independent target, mark it as dirty. This is overridden here so that UIElement's
    // specific dirty flags can be set and consumed in the render walk. The element needs to be marked dirty
    // so that the render walk can pick up its new animation-target-mapped clone.
    if (!wasIndependentTarget && IsIndependentTarget())
    {
        CUIElement::NWSetContentDirty(this, DirtyFlags::Render);
    }

    switch (compositionReason)
    {
        case CompositionRequirement::IndependentAnimation:
        {
            switch (detailedAnimationReason)
            {
                case IndependentAnimationType::Transform:
                    ASSERT(!m_hasTransformIA);
                    m_hasTransformIA = TRUE;
                    break;

                case IndependentAnimationType::ElementProjection:
                    ASSERT(!m_hasProjectionIA);
                    m_hasProjectionIA = TRUE;
                    break;

                case IndependentAnimationType::ElementTransform3D:
                    ASSERT(!m_hasTransform3DIA);
                    m_hasTransform3DIA = TRUE;

                    break;

                case IndependentAnimationType::ElementOpacity:
                    ASSERT(!m_hasLocalOpacityIA);
                    m_hasLocalOpacityIA = TRUE;

                    // Opacity animations affect batching - specifically when IndexBuffer_InteriorGeometry can be used
                    CUIElement::NWSetHasOpacityAnimationDirty(this, DirtyFlags::Render);
                    break;

                case IndependentAnimationType::ElementClip:
                    ASSERT(!m_hasLocalClipIA);
                    m_hasLocalClipIA = TRUE;
                    break;

                case IndependentAnimationType::TransitionOpacity:
                    ASSERT(!m_hasTransitionOpacityIA);
                    m_hasTransitionOpacityIA = TRUE;

                    // Opacity animations affect batching - specifically when IndexBuffer_InteriorGeometry can be used
                    CUIElement::NWSetHasOpacityAnimationDirty(this, DirtyFlags::Render);
                    break;

                case IndependentAnimationType::TransitionClip:
                    ASSERT(!m_hasTransitionClipIA);
                    m_hasTransitionClipIA = TRUE;
                    break;

                case IndependentAnimationType::BrushColor:
                    ASSERT(!m_hasBrushColorIA);
                    m_hasBrushColorIA = TRUE;
                    break;

                case IndependentAnimationType::Offset:
                    ASSERT(!m_hasOffsetIA);
                    m_hasOffsetIA = true;
                    break;

                case IndependentAnimationType::None:
                default:
                    // All detailed animation reasons should be valid and accounted for.
                    ASSERT(FALSE);
            }
            break;
        }

        case CompositionRequirement::Manipulatable:
        {
            ASSERT(!m_isManipulatable);
            m_isManipulatable = TRUE;

            // An element is not expected to be actively manipulated when it becomes manipulatable.
            ASSERT(!m_hasManipulation);

            // The element's layout responsibility is transitioning from DManip to our layout engine.
            // Ensure the element gets re-arranged if it's smaller than its viewport and placed elsewhere than the top/left corner.
            InvalidateArrange();
            break;
        }

        case CompositionRequirement::IndependentManipulation:
            ASSERT(!m_hasManipulation);
            m_hasManipulation = TRUE;

            // An element should always be marked manipulatable before it is actively manipulated.
            ASSERT(m_isManipulatable);
            break;

        case CompositionRequirement::IndependentClipManipulation:
            ASSERT(!m_hasClipManipulation);
            m_hasClipManipulation = TRUE;

            // An element should always be marked manipulatable before its clip is actively manipulated.
            ASSERT(m_isManipulatable);
            break;

        case CompositionRequirement::RootElement:
            ASSERT(!m_isRootElement);
            m_isRootElement = TRUE;
            break;

        case CompositionRequirement::RedirectionElement:
            ASSERT(!m_isRedirectionElement);
            m_isRedirectionElement = TRUE;
            break;

        case CompositionRequirement::Projection:
            ASSERT(!m_hasNonIdentityProjection);
            m_hasNonIdentityProjection = TRUE;
            break;

        case CompositionRequirement::Transform3D:
            ASSERT(!m_hasTransform3D);
            m_hasTransform3D = true;
            break;

        case CompositionRequirement::RenderTransformAxisUnaligned:
            ASSERT(!m_hasAxisUnalignedTransform);
            m_hasAxisUnalignedTransform = TRUE;
            break;

        case CompositionRequirement::LocalClipAxisUnaligned:
            ASSERT(!m_hasAxisUnalignedLocalClip);
            m_hasAxisUnalignedLocalClip = TRUE;
            break;

        case CompositionRequirement::SwapChainContent:
            ASSERT(!m_hasSwapChainContent);
            m_hasSwapChainContent = TRUE;
            break;

        case CompositionRequirement::HandOffVisualNeeded:
            ASSERT(!m_isUsingHandOffVisual);
            m_isUsingHandOffVisual = TRUE;
            break;

        case CompositionRequirement::HandInVisualNeeded:
            ASSERT(!m_isUsingHandInVisual);
            m_isUsingHandInVisual = TRUE;
            break;

        case CompositionRequirement::UsesCompositeMode:
            ASSERT(!m_usesCompositeMode);
            m_usesCompositeMode = TRUE;
            break;

        case CompositionRequirement::IsNonHitTestableChildOfSwapChainOrMap:
            // This reason is special and the underlying flag should only be turned on during the render walk,
            // in which case we don't require marking the element as dirty for render.
            ASSERT(FALSE);
            break;

        case CompositionRequirement::HasConnectedAnimation:
            ASSERT(!m_hasActiveConnectedAnimation);
            m_hasActiveConnectedAnimation = true;
            break;

        case CompositionRequirement::HasImplicitShowAnimation:
            ASSERT(!m_hasImplicitShowAnimation);
            m_hasImplicitShowAnimation = true;
            break;

        case CompositionRequirement::HasImplicitHideAnimation:
            ASSERT(!m_hasImplicitHideAnimation);
            m_hasImplicitHideAnimation = true;
            break;

        case CompositionRequirement::XamlLight:
            ASSERT(!m_isLightTargetOrHasLight);
            m_isLightTargetOrHasLight = true;
            break;

        case CompositionRequirement::RenderTargetBitmap:
            ASSERT(!m_isRenderTargetSource);
            m_isRenderTargetSource = true;
            SetAndPropagateForceNoCulling(true);
            SetDirtyToRoot();
            break;

        case CompositionRequirement::HasRoundedCorners:
            ASSERT(!m_requiresCompNodeForRoundedCorners);
            m_requiresCompNodeForRoundedCorners = true;
            break;

        case CompositionRequirement::HasFacadeAnimation:
            ASSERT(!m_hasFacadeAnimation);
            m_hasFacadeAnimation = true;
            break;

        case CompositionRequirement::HasTranslateZ:
            ASSERT(!m_hasTranslateZ);
            m_hasTranslateZ = true;
            break;

        case CompositionRequirement::HasNonZeroRotation:
            ASSERT(!m_hasNonZeroRotation);
            m_hasNonZeroRotation = true;
            break;

        case CompositionRequirement::HasScaleZ:
            ASSERT(!m_hasScaleZ);
            m_hasScaleZ = true;
            break;

        case CompositionRequirement::HasNonIdentityTransformMatrix:
            ASSERT(!m_hasNonIdentityTransformMatrix);
            m_hasNonIdentityTransformMatrix = true;
            break;

        case CompositionRequirement::HasNonZeroCenterPoint:
            ASSERT(!m_hasNonZeroCenterPoint);
            m_hasNonZeroCenterPoint = true;
            break;

        case CompositionRequirement::HasNonDefaultRotationAxis:
            ASSERT(!m_hasNonDefaultRotationAxis);
            m_hasNonDefaultRotationAxis = true;
            break;

        case CompositionRequirement::ShadowCaster:
            ASSERT(!m_isShadowCaster);
            m_isShadowCaster = true;
            break;

        case CompositionRequirement::ProjectedShadowDefaultReceiver:
            ASSERT(!m_isProjectedShadowDefaultReceiver);
            m_isProjectedShadowDefaultReceiver = true;
            break;

        case CompositionRequirement::ProjectedShadowCustomReceiver:
            ASSERT(!m_isProjectedShadowCustomReceiver);
            m_isProjectedShadowCustomReceiver = true;
            break;

        case CompositionRequirement::TransitionRootWithChildren:
            ASSERT(!m_isTransitionRootWithChildren);
            m_isTransitionRootWithChildren = true;
            break;

        default:
            // All reasons should be accounted for.
            ASSERT(FALSE);
    }

    // If composition requirements changed, propagate the change through the tree.
    if (!didRequireCompNode && RequiresCompositionNode())
    {
        // Mark the element as dirty for render because it now has a comp node.
        //
        // Note: When doing synchronous comp tree updates, if the render walk calls code that adds a hand-off visual
        // (HandOffVisualNeeded) or a hand-in visual (HandInVisualNeeded) on a CUIElement that doesn't otherwise need
        // a comp node, then that CUIElement will now need a comp node, and it will propagate this dirty flag. Since
        // we're in the middle of a render walk guided by dirty flags, dirty flag propagation is not allowed, which
        // means this call will hit an assert.
        //
        // This scenario (adding comp nodes to the tree during the render walk) is not supported by synchronous comp
        // tree updates. The render walk can add hand off or hand in visuals to UIElements that _already_ need a comp
        // node, and this case comes up when attaching content visuals to a CWebView.
        CUIElement::NWSetHasCompositionNodeDirty(this, DirtyFlags::Render);
    }

    if (!hadBrushAnimation && IsBrushIndependentlyAnimating())
    {
        // Mark the element as dirty for render because it now has a brush animation, which will require a DComp color rather than a static value.
        CUIElement::NWSetContentDirty(this, DirtyFlags::Render);
    }

    ASSERT(RequiresComposition());

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Unsets that this element needs to be composed on the render thread.
//
//------------------------------------------------------------------------
void CUIElement::UnsetRequiresComposition(
    CompositionRequirement compositionReason,
    IndependentAnimationType detailedAnimationReason
    ) noexcept
{
#ifdef DMUIEv_DBG
    IGNOREHR(gps->DebugOutputSzNoEndl(
        L"DMUIEv[0x%p]:SetRequiresComposition ThreadID=%d, isForAnimation=%d, isForManipulation=%d, isForClipManipulation=%d\r\n",
        this,
        GetCurrentThreadId(),
        compositionReason == CompositionRequirement::IndependentAnimation,
        compositionReason == CompositionRequirement::IndependentManipulation,
        compositionReason == CompositionRequirement::IndependentClipManipulation
        ));
#endif // DMUIEV_DBG

    bool didRequireCompNode = RequiresCompositionNode();
    bool hadBrushAnimation = IsBrushIndependentlyAnimating();

    ASSERT(RequiresComposition());

    switch (compositionReason)
    {
        case CompositionRequirement::IndependentAnimation:
        {
            switch (detailedAnimationReason)
            {
                case IndependentAnimationType::Transform:
                    ASSERT(m_hasTransformIA);
                    m_hasTransformIA = FALSE;
                    break;

                case IndependentAnimationType::ElementProjection:
                    ASSERT(m_hasProjectionIA);
                    m_hasProjectionIA = FALSE;
                    break;

                case IndependentAnimationType::ElementTransform3D:
                    ASSERT(m_hasTransform3DIA);
                    m_hasTransform3DIA = FALSE;
                    break;

                case IndependentAnimationType::ElementOpacity:
                    ASSERT(m_hasLocalOpacityIA);
                    m_hasLocalOpacityIA = FALSE;

                    // Opacity animations affect batching - specifically when IndexBuffer_InteriorGeometry can be used
                    CUIElement::NWSetHasOpacityAnimationDirty(this, DirtyFlags::Render);
                    break;

                case IndependentAnimationType::ElementClip:
                    ASSERT(m_hasLocalClipIA);
                    m_hasLocalClipIA = FALSE;
                    break;

                case IndependentAnimationType::TransitionOpacity:
                    ASSERT(m_hasTransitionOpacityIA);
                    m_hasTransitionOpacityIA = FALSE;

                    // Opacity animations affect batching - specifically when IndexBuffer_InteriorGeometry can be used
                    CUIElement::NWSetHasOpacityAnimationDirty(this, DirtyFlags::Render);
                    break;

                case IndependentAnimationType::TransitionClip:
                    ASSERT(m_hasTransitionClipIA);
                    m_hasTransitionClipIA = FALSE;
                    break;

                case IndependentAnimationType::BrushColor:
                    ASSERT(m_hasBrushColorIA);
                    m_hasBrushColorIA = FALSE;
                    break;

                case IndependentAnimationType::Offset:
                    ASSERT(m_hasOffsetIA);
                    m_hasOffsetIA = false;
                    break;

                case IndependentAnimationType::None:
                default:
                    // All detailed animation reasons should be valid and accounted for.
                    ASSERT(FALSE);
            }
            break;
        }

        case CompositionRequirement::Manipulatable:
        {
            ASSERT(m_isManipulatable);
            m_isManipulatable = FALSE;

            // An element shouldn't stop being manipulatable while it has an active manipulation.
            ASSERT(!m_hasManipulation);
            ASSERT(!m_hasClipManipulation);

            // The element's layout responsibility is transitioning from DManip to our layout engine.
            // Ensure the element gets re-arranged if it's smaller than its viewport and placed elsewhere than the top/left corner.
            InvalidateArrange();
            break;
        }

        case CompositionRequirement::IndependentManipulation:
            ASSERT(m_hasManipulation);
            m_hasManipulation = FALSE;
            break;

        case CompositionRequirement::IndependentClipManipulation:
            ASSERT(m_hasClipManipulation);
            m_hasClipManipulation = FALSE;
            break;

        case CompositionRequirement::RootElement:
            ASSERT(m_isRootElement);
            m_isRootElement = FALSE;
            break;

        case CompositionRequirement::RedirectionElement:
            ASSERT(m_isRedirectionElement);
            m_isRedirectionElement = FALSE;
            break;

        case CompositionRequirement::Projection:
            ASSERT(m_hasNonIdentityProjection);
            m_hasNonIdentityProjection = FALSE;
            break;

        case CompositionRequirement::Transform3D:
            ASSERT(m_hasTransform3D);
            m_hasTransform3D = false;
            break;

        case CompositionRequirement::RenderTransformAxisUnaligned:
            ASSERT(m_hasAxisUnalignedTransform);
            m_hasAxisUnalignedTransform = FALSE;
            break;

        case CompositionRequirement::LocalClipAxisUnaligned:
            ASSERT(m_hasAxisUnalignedLocalClip);
            m_hasAxisUnalignedLocalClip = FALSE;
            break;

        case CompositionRequirement::SwapChainContent:
            ASSERT(m_hasSwapChainContent);
            m_hasSwapChainContent = FALSE;
            break;

        case CompositionRequirement::HandOffVisualNeeded:
            ASSERT(m_isUsingHandOffVisual);
            m_isUsingHandOffVisual = FALSE;
            break;

        case CompositionRequirement::HandInVisualNeeded:
            ASSERT(m_isUsingHandInVisual);
            m_isUsingHandInVisual = FALSE;
            break;

        case CompositionRequirement::UsesCompositeMode:
            ASSERT(m_usesCompositeMode);
            m_usesCompositeMode = FALSE;
            break;

        case CompositionRequirement::IsNonHitTestableChildOfSwapChainOrMap:
            ASSERT(m_requiresHitTestInvisibleCompNode);
            m_requiresHitTestInvisibleCompNode = false;
            break;

        case CompositionRequirement::HasConnectedAnimation:
            ASSERT(m_hasActiveConnectedAnimation);
            m_hasActiveConnectedAnimation = false;
            break;

        case CompositionRequirement::HasImplicitShowAnimation:
            ASSERT(m_hasImplicitShowAnimation);
            m_hasImplicitShowAnimation = false;
            break;

        case CompositionRequirement::HasImplicitHideAnimation:
            ASSERT(m_hasImplicitHideAnimation);
            m_hasImplicitHideAnimation = false;
            break;

        case CompositionRequirement::XamlLight:
            ASSERT(m_isLightTargetOrHasLight);
            m_isLightTargetOrHasLight = false;
            break;

        case CompositionRequirement::RenderTargetBitmap:
            ASSERT(m_isRenderTargetSource);
            m_isRenderTargetSource = false;
            SetAndPropagateForceNoCulling(false);
            SetDirtyToRoot();
            break;

        case CompositionRequirement::HasRoundedCorners:
            ASSERT(m_requiresCompNodeForRoundedCorners);
            m_requiresCompNodeForRoundedCorners = false;
            break;

        case CompositionRequirement::HasFacadeAnimation:
            ASSERT(m_hasFacadeAnimation);
            m_hasFacadeAnimation = false;
            break;

        case CompositionRequirement::HasTranslateZ:
            ASSERT(m_hasTranslateZ);
            m_hasTranslateZ = false;
            break;

        case CompositionRequirement::HasNonZeroRotation:
            ASSERT(m_hasNonZeroRotation);
            m_hasNonZeroRotation = false;
            break;

        case CompositionRequirement::HasScaleZ:
            ASSERT(m_hasScaleZ);
            m_hasScaleZ = false;
            break;

        case CompositionRequirement::HasNonIdentityTransformMatrix:
            ASSERT(m_hasNonIdentityTransformMatrix);
            m_hasNonIdentityTransformMatrix = false;
            break;

        case CompositionRequirement::HasNonZeroCenterPoint:
            ASSERT(m_hasNonZeroCenterPoint);
            m_hasNonZeroCenterPoint = false;
            break;

        case CompositionRequirement::HasNonDefaultRotationAxis:
            ASSERT(m_hasNonDefaultRotationAxis);
            m_hasNonDefaultRotationAxis = false;
            break;

        case CompositionRequirement::ShadowCaster:
            ASSERT(m_isShadowCaster);
            m_isShadowCaster = false;
            break;

        case CompositionRequirement::ProjectedShadowDefaultReceiver:
            ASSERT(m_isProjectedShadowDefaultReceiver);
            m_isProjectedShadowDefaultReceiver = false;
            break;

        case CompositionRequirement::ProjectedShadowCustomReceiver:
            ASSERT(m_isProjectedShadowCustomReceiver);
            m_isProjectedShadowCustomReceiver = false;
            break;

        case CompositionRequirement::TransitionRootWithChildren:
            ASSERT(m_isTransitionRootWithChildren);
            m_isTransitionRootWithChildren = false;
            break;

        default:
            // All reasons should be accounted for.
            ASSERT(FALSE);
    }

    // If composition requirements changed, propagate the change through the tree.
    if (didRequireCompNode && !RequiresCompositionNode())
    {
        // Mark the element as dirty for render because it no longer has a comp node.
        CUIElement::NWSetHasCompositionNodeDirty(this, DirtyFlags::Render);

        RemoveCompositionPeer();
    }

    if (hadBrushAnimation && !IsBrushIndependentlyAnimating())
    {
        // Mark the element as dirty for render because it no longer has a color animation, and can switch from a DComp color
        // back to a static value.
        CUIElement::NWSetContentDirty(this, DirtyFlags::Render);
    }

    bool wasIndependentTarget = !!IsIndependentTarget();

    CDependencyObject::UnsetRequiresComposition(compositionReason, detailedAnimationReason);

    // If this element is no longer an independent target, mark it as dirty. This is overridden here so that UIElement's
    // specific dirty flags can be set and consumed in the render walk. The element needs to be marked dirty
    // so that the render walk will release its animation-target-mapped clone.
    if (wasIndependentTarget && !IsIndependentTarget())
    {
        CUIElement::NWSetContentDirty(this, DirtyFlags::Render);
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Adds the peer to this element in the composition tree.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CUIElement::EnsureCompositionPeer(
    _In_ HWCompTreeNode *pCompositionPeer,
    _In_opt_ HWCompTreeNode *pParentNode,
    _In_ HWCompNode *pPreviousSibling,
    _In_opt_ WUComp::IVisual* previousSiblingVisual)
{
    RRETURN(CUIElement::EnsureCompositionPeer(
        GetDCompTreeHost(),
        pCompositionPeer,
        pParentNode,
        pPreviousSibling,
        previousSiblingVisual,
        this,
        &m_pCompositionPeer));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Adds the peer to this element in the composition tree.
//
//------------------------------------------------------------------------
/*static*/ _Check_return_ HRESULT
CUIElement::EnsureCompositionPeer(
    _In_ DCompTreeHost* dcompTreeHost,
    _In_ HWCompTreeNode *pCompositionPeer,
    _In_opt_ HWCompTreeNode *pParentNode,
    _In_ HWCompNode *pPreviousSibling,
    _In_opt_ WUComp::IVisual* previousSiblingVisual,
    _In_opt_ CUIElement *pElement,
    _Inout_ HWCompTreeNode **ppTargetCompositionPeer
    )
{
    HRESULT hr = S_OK;

    HWCompTreeNode *pTargetCompositionPeer  = *ppTargetCompositionPeer;

    // Each UIElement that requires composition splits the composition tree into 2 additional layers.
    // The existing layer the element resides in remains, but is filled only with content behind the element.
    // This existing layer continues to be owned by an ancestor UIElement (and its corresponding composition peer).
    //
    // The element introduces a new layer for all its content, and content in its subgraph. The composition peer
    // owns this directly - it is its 'content' node.
    //
    // An additional layer is also added for content in front of that element. This is also owned by the composition
    // peer, and is called its 'postsubgraph' node.
    //
    // Before:
    //
    //...    ancestor composition peer ...
    //       |
    //...    existing node ...
    //       (UIElement content is here)
    //
    // After:
    // UIElement now has its own composition peer
    //
    //...                    ancestor composition peer
    //                       |                  |                |
    //...    existing node       new comp peer      postsubgraph node (post-UIE content)
    //  (pre-UIE content)   (UIE and subgraph content)
    //

    // It's possible that the composition peer already exists, but its parent has changed. This can happen
    // when a new composition peer was added to an element in this element's parent chain.
    // There's no easy way to determine which children of an existing node need to be made children of
    // a new node, and which should remain siblings, without relying on a tree walk of the UIElements
    // to determine the nesting. The render walk performs this task.
    if (pTargetCompositionPeer != nullptr)
    {
        // It's not expected that the composition peer itself should change.
        ASSERT(pTargetCompositionPeer == pCompositionPeer);

        // If the parent has changed, remove the existing composition peer from its old location in the tree.
        // It will be added back in the correct location below.
        CDependencyObject *pParent = pTargetCompositionPeer->GetParentInternal(false /*isPublic*/);
        ASSERT(pParent == nullptr || pParent->OfTypeByIndex<KnownTypeIndex::HWCompTreeNode>());
        HWCompTreeNode *pCurrentParentNode = static_cast<HWCompTreeNode*>(pParent);

        if (pCurrentParentNode != nullptr && pCurrentParentNode != pParentNode)
        {
            HWCompTreeNodeWinRT* compositionPeerWinRT = static_cast<HWCompTreeNodeWinRT*>(pTargetCompositionPeer);
            compositionPeerWinRT->RemoveForReparenting();

            pCurrentParentNode = nullptr;
        }
        if (pCurrentParentNode == nullptr)
        {
            ReleaseInterface(pTargetCompositionPeer);
        }
    }

    // Insert the composition peer into the tree.
    if (pTargetCompositionPeer == nullptr)
    {
        // If this is anything but the root, insert it into the tree.
        if (pParentNode != nullptr)
        {
            //
            // If this is an inline popup nested inside a windowed popup, we need to move some visuals around. Normally
            // the visuals under an inline popup still live in the main Xaml tree under the XamlIslandRoot's IXP island,
            // but visuals in the main tree are covered up by visuals inside windowed popups. If this inline popup is
            // nested inside a windowed popup, it needs to not be covered up by the outer windowed popup, which means its
            // visuals cannot live in the main Xaml tree and must live in the windowed popup's IXP island along with the
            // windowed popup visuals.
            //
            // To accomplish this, we can use the same mechanism that windowed popups have. They'll leave a placeholder
            // visual in the main tree to serve as a reference point for the incremental render walk, and move all the real
            // visuals over to another island. Find inline popups nested inside windowed popups here and mark them as needing
            // a placeholder.
            //
            // Note that we won't ever need to unmark them. The outer windowed popup can't be marked as unwindowed once it's
            // been opened. The app also can't move the inner inline popup out of the outer windowed popup without closing
            // it first (it implicitly closes as soon as it's detached), which cause CUIElement::RemoveCompositionPeer to
            // throw away the entire comp node. When the inline popup opens again, it'll get a new comp node that won't
            // be marked as needing a placeholder.
            //
            if (pElement->OfTypeByIndex(KnownTypeIndex::Popup))
            {
                CPopup* thisPopup = static_cast<CPopup*>(pElement);
                CPopup* ancestorWindowedPopup = nullptr;
                if (!thisPopup->IsWindowed())
                {
                    ancestorWindowedPopup = pElement->GetFirstAncestorPopup(true /* windowedOnly */);
                }

                if (ancestorWindowedPopup)
                {
                    FAIL_FAST_ASSERT(pCompositionPeer->OfTypeByIndex(KnownTypeIndex::HWRedirectedCompTreeNodeWinRT));
                    static_cast<HWRedirectedCompTreeNodeWinRT*>(pCompositionPeer)->SetUsesPlaceholderVisual(true);
                }
            }

            // In sprite visuals mode, we don't queue operations in the CompositorTreeHost. We do them synchronously.
            // There's no post subgraph node either.
            HWCompTreeNodeWinRT* parentCompNodeWinRT = static_cast<HWCompTreeNodeWinRT*>(pParentNode);
            parentCompNodeWinRT->InsertChildSynchronous(
                dcompTreeHost,
                pCompositionPeer,
                pPreviousSibling,
                previousSiblingVisual,
                pElement,
                false /* ignoreInsertVisualErrors */);
        }
        else
        {
            // The root comp node needs to create its visuals. Normally this is done when inserting the content render
            // data node into the root comp node, but when we're synchronously updating the comp tree, there are no render
            // data nodes.
            pCompositionPeer->EnsureVisual(dcompTreeHost);
        }

        TraceCompTreeSetCompositionPeerInfo(
            reinterpret_cast<XUINT64>(pElement),
            reinterpret_cast<XUINT64>(pCompositionPeer)
            );

        SetInterface(*ppTargetCompositionPeer, pCompositionPeer);
    }

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates and adds a temporary peer to this element in the
//      composition tree.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElement::CreateTemporaryCompositionPeer(
    _In_ CWindowRenderTarget *pRenderTarget,
    _In_ HWCompTreeNode *pParentNode,
    _In_ const CMILMatrix *pPrependMatrix,
    _Outptr_ HWCompTreeNode **ppCompNode
    )
{
    HRESULT hr = S_OK;

    CompositorTreeHost *pCompTreeHostNoRef = pParentNode->GetCompositorTreeHost();
    HWCompTreeNode *pTemporaryCompNode = NULL;

    // If there's already a real composition peer, we should have found it and used it for the LTE.
    ASSERT(m_pCompositionPeer == NULL);

    // Insert the composition peer into the tree.
    IFC(HWCompTreeNodeWinRT::Create(
        GetContext(),
        pCompTreeHostNoRef,
        pRenderTarget->GetDCompTreeHost(),
        TRUE /* isPlaceholderCompNode */,
        &pTemporaryCompNode));

    // Don't allow temporary comp nodes to use the hand off visual. When building the WUC visual tree from the comp node
    // tree, Xaml will ignore errors from DComp about visuals that already have parents. We do this to handle the portaling
    // scenario with hand off visuals, where it needs to be in multiple places at once but can only have a single parent.
    //
    // This policy means comp nodes are racing to take ownership of the hand off visual. The first one that gets to it gets
    // to parent it in the tree. Here, the temporary comp node is going to win the race and take ownership of the hand off
    // visual. If this element gets rendered later in the same render walk, it'll create a real comp node that also wants
    // to take ownership of the hand off visual. That's a problem, since the temporary comp node got to it first, so the
    // real comp node will hit a DComp error when trying to add its WUC visual to the tree, then silently ignore the error.
    // At the end of the frame, all temporary comp nodes are deleted, which means this temporary comp node will clear out
    // the parent and children of the hand off visual. That leaves the tree in an inconsistent state, where the real comp
    // node for this element is still in the comp node tree, but the hand off visual for this element is not.
    //
    // To avoid this problem we don't allow temporary comp nodes to get a hold of the hand off visual. If the app set WUC
    // properties on the hand off visual, then the LTE that uses the temporary comp node can render in the wrong place.
    // Since temporary comp nodes are transient, and the combination of hand off visuals and temporary comp nodes is a rare
    // scenario, we don't expect this to be a big problem.
    pTemporaryCompNode->SetAllowReuseHandOffVisual(false);

    XRECTF prependClip;
    SetInfiniteClip(&prependClip);

    DCompTreeHost* dcompTreeHost = GetDCompTreeHost();

    IFC(pTemporaryCompNode->SetPrependProperties(
        pPrependMatrix,
        &prependClip, /* Only the transforms from the target are used, so this clip value will be ignored */
        1.0f /* Only the transforms from the target are used, so this opacity value will be ignored */));

    IFC(pTemporaryCompNode->SetElementData(
        pRenderTarget,
        this,
        false /* isHitTestVisibleSubtree */));

    IFC(pParentNode->InsertChildAtBeginning(
        dcompTreeHost,
        pTemporaryCompNode));

    pCompTreeHostNoRef->TrackTemporaryNode(*pTemporaryCompNode);

    *ppCompNode = pTemporaryCompNode;
    pTemporaryCompNode = NULL;

Cleanup:
    ReleaseInterfaceNoNULL(pTemporaryCompNode);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Removes the peer to this element in the composition tree.
//
//------------------------------------------------------------------------
void
CUIElement::RemoveCompositionPeer()
{
    if (m_pCompositionPeer != NULL)
    {
        // Bug 16258012 <[Reliability] Microsoft Edge crashes if a window is closed while an animation is running (INVALID_POINTER_READ_c0000005_Microsoft.UI.Xaml.dll!CUIElement::IsKeepVisible)>
        // If we're the target of a Storyboard animation that just started this frame, we would be kept alive in the CTimeManager's
        // m_targetDOs collection. When that collection gets released during Xaml shutdown, we'll release our comp node. At this
        // point the window render target has already been released, so the DCompTreeHost won't be accessible. No-op in this case,
        // rather than making calls which assume that the DCompTreeHost is accessible.
        const bool hasRenderTarget = (GetContext()->NWGetWindowRenderTarget() != nullptr);

        if (hasRenderTarget)
        {
            // Cancel any ongoing implicit Show animations before we tear down the CompNode.
            CancelImplicitAnimation(ImplicitAnimationType::Show);
            if (!IsKeepVisible())
            {
                // If this element is being kept visible, then don't cancel its Hide animation. This RemoveCompositionPeer call comes
                // from a LeavePCSceneRecursive that's happening in the middle of moving this element into unloading storage.
                //
                // If this element isn't being kept visible, then we need to clean up any pending hide animations. This element might
                // be in a subtree that was being unloaded, but is now done with unloading. Everything in the subtree should be cleaned
                // up. Note that this implicit hide animation should have kept that ancestor in unloading storage until the hide
                // animation completes, so if the ancestor finished unloading while there's still an implicit hide animation going,
                // then it was canceled by something else.
                CancelImplicitAnimation(ImplicitAnimationType::Hide);
            }

            // In sprite visuals mode we don't have any render data comp nodes to merge or split, so we can remove comp nodes
            // synchronously. Previously we had to enqueue these commands and run them first because the removal could cause
            // a merge that depended on a render data node that wasn't inserted yet.
            HWCompTreeNodeWinRT* compNodeWinRT = static_cast<HWCompTreeNodeWinRT*>(m_pCompositionPeer);
            compNodeWinRT->RemoveSynchronous();

            TraceCompTreeRemoveCompositionPeerInfo(
                reinterpret_cast<XUINT64>(this),
                reinterpret_cast<XUINT64>(m_pCompositionPeer)
                );

            static_cast<HWCompTreeNodeWinRT*>(m_pCompositionPeer)->UntargetFromLights(GetDCompTreeHost());

            // If this compnode participated in projected shadow (as receiver or "complex caster"), mark the associated visual for removal from WUC ProjectedShadowScene

            if (!CThemeShadow::IsDropShadowMode())
            {
                // Remove from caster list if needed
                wrl::ComPtr<WUComp::IVisual> primaryVisual((static_cast<HWCompTreeNodeWinRT*>(m_pCompositionPeer))->GetPrimaryVisualNoRef());
                GetDCompTreeHost()->GetProjectedShadowManager()->RemoveCaster(primaryVisual.Get());

                // Remove from receiver list if needed
                // TODO_Shadows: Currently, receiver list is fully rebuilt on ever frame so we don't need any action here
                //               When we have incremental receiver list updates, dirtying - and possibly updating - the list here will be necessary
                // GetDCompTreeHost()->GetProjectedShadowManager()->SetReceiversDirty(true);
            }
        }

        ReleaseInterface(m_pCompositionPeer);
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns true if any of the independent animations on this element affect its
//      position on-screen.
//
//------------------------------------------------------------------------
bool
CUIElement::IsTransformOrOffsetAffectingPropertyIndependentlyAnimating() const
{
    return IsOffsetIndependentlyAnimating()
        || IsTransformAffectingPropertyIndependentlyAnimating();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns true if there are active opacity animations on the element.
//
//------------------------------------------------------------------------
bool
CUIElement::IsOpacityIndependentlyAnimating() const
{
    return IsLocalOpacityIndependentlyAnimating()
        || IsTransitionOpacityIndependentlyAnimating();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns true if there are active clip animations on this element.
//
//------------------------------------------------------------------------
bool
CUIElement::IsClipIndependentlyAnimating() const
{
    return IsLocalClipIndependentlyAnimating()
        || IsTransitionClipIndependentlyAnimating();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns true if there are active clip animations targeting
//      this element's Clip property.
//
//------------------------------------------------------------------------
bool
CUIElement::IsLocalClipIndependentlyAnimating() const
{
    return m_hasLocalClipIA;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns true if there are active clip animations targeting
//      this element's TransitionTarget clip transform.
//
//------------------------------------------------------------------------
bool
CUIElement::IsTransitionClipIndependentlyAnimating() const
{
    return m_hasTransitionClipIA;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns true if there are active color animations targeting
//      this element's brush properties.
//
//------------------------------------------------------------------------
bool
CUIElement::IsBrushIndependentlyAnimating() const
{
    return m_hasBrushColorIA;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns true if this element can be the target of an independent manipulation.
//
//------------------------------------------------------------------------
bool
CUIElement::IsManipulatable() const
{
    return m_isManipulatable;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns true if there are active clip manipulations targeting this
//      element.
//
//------------------------------------------------------------------------
bool
CUIElement::IsClipManipulatedIndependently() const
{
    return m_hasClipManipulation;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns true if this element is the root of the render walk.
//
//------------------------------------------------------------------------
bool
CUIElement::IsRenderWalkRoot() const
{
    return m_isRootElement;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns true if this element uses redirection to render its content.
//
//------------------------------------------------------------------------
bool
CUIElement::IsRedirectionElement() const
{
    return m_isRedirectionElement;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns true if this element has a transform, projection, or clip that could produce
//      polygonal content. These elements require composition nodes, so that all content
//      is rectangular and the more complex properties are pushed to the composition tree.
//
//------------------------------------------------------------------------
bool
CUIElement::HasComplexTransformation() const
{
    return m_hasNonIdentityProjection
        || m_hasTransform3D
        || HasAxisUnalignedTransform()
        || HasAxisUnalignedLocalClip();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns true if this element has a transform component that does not preserve
//      axis-alignment.
//
//------------------------------------------------------------------------
bool
CUIElement::HasAxisUnalignedTransform() const
{
    return m_hasAxisUnalignedTransform;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns true if this element has a clip set that isn't axis-aligned.
//
//------------------------------------------------------------------------
bool
CUIElement::HasAxisUnalignedLocalClip() const
{
    return m_hasAxisUnalignedLocalClip;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns true if this element has a separate swap chain to compose.
//
//------------------------------------------------------------------------
bool
CUIElement::HasSwapChainContent() const
{
    return m_hasSwapChainContent;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns true if this element needs a DComp node representative
//      because a handoff visual was requested for it via the
//      IXamlDCompInteropPrivate interface.
//
//------------------------------------------------------------------------
bool
CUIElement::IsUsingHandOffVisual() const
{
    return m_isUsingHandOffVisual;
}

// Returns true if this element needs a DComp node representative
// because a DComp visual is required to host the WinRT Visual
// provided via the ElementCompositionPreviewFactory::SetElementChildVisual method.
bool
CUIElement::IsUsingHandInVisual() const
{
    return m_isUsingHandInVisual;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns true if this element requires off-thread composition work.
//
//------------------------------------------------------------------------
bool
CUIElement::RequiresComposition() const
{
    return RequiresCompositionNode() || IsBrushIndependentlyAnimating();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns true if there are active independent animations or
//      independent manipulations on the element's render properties.
//
//------------------------------------------------------------------------
bool
CUIElement::HasIndependentAnimationsOrManipulations() const
{
    return IsOffsetIndependentlyAnimating()
        || IsTransformIndependentlyAnimating()
        || IsProjectionIndependentlyAnimating()
        || IsTransform3DIndependentlyAnimating()
        || IsOpacityIndependentlyAnimating()
        || IsClipIndependentlyAnimating()
        || IsManipulatedIndependently()
        || IsClipManipulatedIndependently();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns true if the element requires a HWCompTreeNode.
//
//------------------------------------------------------------------------
bool
CUIElement::RequiresCompositionNode() const
{
    return HasIndependentAnimationsOrManipulations()
        || IsManipulatable()
        || IsRenderWalkRoot()
        || IsRedirectionElement()
        || HasComplexTransformation()
        || HasSwapChainContent()
        || IsUsingHandOffVisual()
        || IsUsingHandInVisual()
        || IsUsingCompositeMode()
        || HasTransform3DCompositionRequirement()
        || RequiresHitTestInvisibleCompNode()
        || DCompTreeHost::IsFullCompNodeTree()
        || HasActiveConnectedAnimation()
        || HasImplicitShowAnimation()
        || HasImplicitHideAnimation()
        || m_isLightTargetOrHasLight
        || IsRenderTargetSource()
        || RequiresCompNodeForRoundedCorners()
        || HasFacadeAnimation()
        || HasTranslateZ()
        || HasNonZeroRotation()
        || HasScaleZ()
        || HasNonIdentityTransformMatrix()
        || HasNonZeroCenterPoint()
        || HasNonDefaultRotationAxis()
        || IsShadowCaster()
        || IsProjectedShadowDefaultReceiver()
        || IsProjectedShadowCustomReceiver()
        || IsTransitionRootWithChildren()
        ;
}

// Returns True when:
// - this UIElement is the manipulated element of a DManip viewport
// - that viewport belongs to a regular ScrollViewer or a touch-manipulatable root ScrollViewer
bool CUIElement::RequiresViewportInteraction()
{
    if (GetContext())
    {
        auto inputServices = GetContext()->GetInputServices();
        if (inputServices)
        {
            return inputServices->RequiresViewportInteraction(this);
        }
    }

    return false;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the cached realization.
//
//------------------------------------------------------------------------
_Ret_maybenull_ HWRealization*
CUIElement::GetHWRealizationCache()
{
    // If an element is cached but hasn't been rendered yet, the HW realization
    // cache may not exist yet.
    return (m_propertyRenderData.IsRenderWalkTypeForComposition())
         ? m_propertyRenderData.pc.pHWRealizationCache
         : NULL;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Replaces the cached realization.
//
//------------------------------------------------------------------------
void
CUIElement::SetHWRealizationCache(
    _In_opt_ HWRealization *pNewRenderingCache
    )
{
    ASSERT(m_propertyRenderData.IsRenderWalkTypeForComposition());

    ReplaceInterface(
        m_propertyRenderData.pc.pHWRealizationCache,
        pNewRenderingCache);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Moves this element into the PC scene. Must have ensured PC storage first.
//
//------------------------------------------------------------------------
void
CUIElement::EnterPCScene()
{
    ASSERT(m_propertyRenderData.IsRenderWalkTypeForComposition());
    m_propertyRenderData.pc.isInScene = TRUE;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Notifies this element that it is being re-rendered in a PC scene
//
//------------------------------------------------------------------------
void
CUIElement::ReEnterPCScene()
{
    ASSERT(IsInPCScene());

    // Reset bookkeeping flags, but keep primitive caches intact
    m_propertyRenderData.pc.ClearInSceneData();
}

// The recursive method to clean up all the device related resources
// like brushes, textures, primitive composition data etc. on
// the entire subtree.
void CUIElement::CleanupDeviceRelatedResourcesRecursive(_In_ bool cleanupDComp)
{
    const auto& transitionTarget = GetTransitionTarget();

    if (m_propertyRenderData.IsRenderWalkTypeForComposition())
    {
        EnsurePropertyRenderData(cleanupDComp ? RWT_None : RWT_NonePreserveDComp);
    }

    if (m_pTextFormatting != NULL &&
        m_pTextFormatting->m_pForeground != nullptr)
    {
        m_pTextFormatting->m_pForeground->CleanupDeviceRelatedResourcesRecursive(cleanupDComp);
    }

    __super::CleanupDeviceRelatedResourcesRecursive(cleanupDComp);

    if (m_pChildren != NULL)
    {
        m_pChildren->CleanupDeviceRelatedResourcesRecursive(cleanupDComp);
    }

    if (!cleanupDComp && m_pCompositionPeer)
    {
        // If we aren't actually resetting the DComp tree, then we need to release our
        // visual content from our composition peer since that could reference the old
        // D3D device.
        m_pCompositionPeer->ClearVisualContent();
    }

    // cleanupDComp == true only when MockDComp triggers device loss
    if (cleanupDComp)
    {
        // Any running implicit Show/Hide animation takes references on Composition objects, clean these up
        CancelImplicitAnimation(ImplicitAnimationType::Show);
        CancelImplicitAnimation(ImplicitAnimationType::Hide);

        if (IsUsingHandOffVisual())
        {
            // This UI element handed off a DComp visual through IXamlDCompInteropPrivate
            // or ElementCompositionPreviewFactory::GetElementVisual.
            // Discard it whether that handoff visual was already copied to the HWCompTreeNode
            // or not, because it's no longer usable on the new DComp device.
            DiscardHandOffVisual();
        }

        if (IsUsingHandInVisual())
        {
            // This UI element is hosting a WinRT Visual through ElementCompositionPreviewFactory::SetElementChildVisual.
            // Discard the hand-in visual whether it was already copied to the HWCompTreeNode
            // or not, because it's no longer usable on the new DComp device.
            DiscardHandInVisual();
        }
    }

    if (transitionTarget != nullptr)
    {
        transitionTarget->CleanupDeviceRelatedResourcesRecursive(cleanupDComp);
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Removes the render data for this element's subgraph from the scene.
//
//------------------------------------------------------------------------
void CUIElement::LeavePCSceneRecursive()
{
    // TODO: INCWALK: Walking pattern in HWWalk needs to match pattern here, refactor to share code?

    // This is a parallel walk to the render walk. The render walk ensures that elements are kept in
    // the PC scene. When branches of the tree can be culled out of the render walk entirely, they
    // leave the scene with a recursive walk here (that matches the walk logic/ordering).
    //
    // Additionally, the render walk only deals with the subset of elements that are in the visual tree,
    // so elements that leave the visual tree must also leave the PC scene.

    // Only need to proceed if the element is in the scene.
    if (IsInPCScene_IncludingDeviceLost())
    {
        // Clear the element's own render data.
        ClearPCRenderData();

        // RS5 Bug #18345068:  TextBlocks can incorrectly render at sub-pixel offsets in certain situations after transform animations complete.
        // Here's a simplified picture of the situation:
        //
        // GrandParent
        // |
        // Parent
        // |               \
        // TextBlock   <--- LTE
        //
        // GrandParent is animating its RenderTransform with a Storyboard.
        // Parent has a long-lived CompNode (eg for a ScrollViewer).
        // TextBlock is being targeted by an LTE which is animating its RenderTransform with a ThemeTransition.
        // If the LTE lands on a sub-pixel offset, this makes the TextBlock nudge itself by a sub-pixel amount to land on a whole pixel.
        // The TextBlock stores this sub-pixel "nudge" factor in its base realization.
        // When the LTE finishes its animation, we do a LeavePCSceneRecursive on the target tree, and re-render the TextBlock.
        // However the TextBlock will re-render using the same "nudge" factor because GrandParent is still animating its RenderTransform.
        // When GrandParent finishes its animation, it loses its CompNode and we walk to Parent, but the walk stops there because
        // no sub-pixel offsets or scales have changed at this level in the tree.  This leaves the TextBlock nudged to a now incorrect
        // sub-pixel offset, and thus renders fuzzy.
        //
        // The tactical fix is to release TextBlock's base realization in addition to its PCRenderData, this will force the TextBlock to
        // create a new realization on the next RenderWalk.  This realization will pick up a new, up-to-date transform-to-root.
        if (OfTypeByIndex<KnownTypeIndex::TextBlock>())
        {
            CTextBlock* textBlock = static_cast<CTextBlock*>(this);

            // We further scope this to only release the base realization if it has sub-pixel offsets, to limit the perf impact.
            if (textBlock->BaseRealizationHasSubPixelOffsets())
            {
                textBlock->ClearBaseRealization();
            }
        }

        // Since this node is leaving the scene, we clear the flags related to DComp hit-test-invisibility
        // as this node might be placed into another part of the tree.  We will re-evaluate these flags
        // when the node is render-walked again.
        SetRequiresHitTestInvisibleCompNode(false);
        SetIsRedirectedChildOfSwapChainPanel(false);

        // Since the element isn't being rendered with PC any longer, remove its composition peer from the tree.
        // If the element is rendered again and still requires composition, its peer will be re-created during the render walk.
        RemoveCompositionPeer();

        LeavePCSceneSubgraph();

        // Special case for LTE embedded in the tree in the HWWalk.
        // GetChildrenInRenderOrder does not return these transition roots to limit the impact of the
        // feature to just the HWWalk, since that's the only place they're supported.
        CTransitionRoot* localTransitionRootNoRef = GetLocalTransitionRoot(false);
        if (localTransitionRootNoRef)
        {
            localTransitionRootNoRef->LeavePCSceneRecursive();
        }

        // If there is a UIA client listening, register this element to have the StructureChanged
        // automation event fired for it.
        if (GetContext()->UIAClientsAreListening(UIAXcp::AEStructureChanged) == S_OK)
        {
            RegisterForStructureChangedEvent(
                AutomationEventsHelper::StructureChangedType::Removed);
        }
    }
#if DBG
    // The local transition root doesn't enter/leave the scene like a regular element, so make sure its state is kept in sync.
    else
    {
        CTransitionRoot* localTransitionRootNoRef = GetLocalTransitionRoot(false);
        if (localTransitionRootNoRef)
        {
            ASSERT(!localTransitionRootNoRef->IsInPCScene());
        }

    }
#endif
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Removes the render data for this element's subgraph from the scene.
//
//------------------------------------------------------------------------
void CUIElement::LeavePCSceneSubgraph()
{
    XUINT32 childCount;
    CUIElement **ppUIElements;

    GetChildrenInRenderOrder(&ppUIElements, &childCount);

    for (XUINT32 i = 0; i < childCount; i++)
    {
        const CUIElement *pChild = ppUIElements[i];

        // Skip walking Popups directly, they're only walked indirectly via the PopupRoot.
        // Skip walking LTE.Target directly, they're only walked indirectly via the LTE itself.
        if (!pChild->OfTypeByIndex<KnownTypeIndex::Popup>() && !pChild->IsHiddenForLayoutTransition())
        {
            ppUIElements[i]->LeavePCSceneRecursive();
        }
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Removes the render data for this element from the scene.
//
//------------------------------------------------------------------------
void
CUIElement::ClearPCRenderData()
{
    ASSERT(IsInPCScene_IncludingDeviceLost());

    // Untarget the sprite visuals from any lights before releasing the sprite visuals.
    if (GetContext()->NWGetWindowRenderTarget() != nullptr && GetDCompTreeHost() != nullptr)
    {
        auto& lightTargetMap = GetDCompTreeHost()->GetXamlLightTargetMap();
        lightTargetMap.RemoveTargetAndUnregisterElement(this);
    }

    m_propertyRenderData.pc.ClearRenderData(GetContext());
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns a pointer to the UIElement's post-children render data.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElement::GetPCPostChildrenRenderDataNoRef(_Outptr_ PCRenderDataList **ppRenderData)
{
    HRESULT hr = S_OK;

    ASSERT(IsInPCScene());

    if (m_propertyRenderData.pc.pPostChildrenRenderData == NULL)
    {
        m_propertyRenderData.pc.pPostChildrenRenderData = new PCRenderDataList();
    }

    *ppRenderData = m_propertyRenderData.pc.pPostChildrenRenderData;

    RRETURN(hr);//RRETURN_REMOVAL
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Caches a ptr to the last primitive added in this UIElement's subgraph.
//
//------------------------------------------------------------------------
void CUIElement::StoreLastSpriteVisual(_In_opt_ WUComp::IVisual* spriteVisual)
{
    ASSERT(IsInPCScene());

    m_propertyRenderData.pc.lastSpriteVisualInSubgraphNoRef = spriteVisual;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the cached ptr to the last primitive added in this UIElement's subgraph.
//
//------------------------------------------------------------------------
WUComp::IVisual* CUIElement::GetLastSpriteVisual()
{
    return IsInPCScene()
         ? m_propertyRenderData.pc.lastSpriteVisualInSubgraphNoRef
         : NULL;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Caches a ptr to the last comp node added in this UIElement's subgraph.
//
//------------------------------------------------------------------------
void
CUIElement::StoreLastCompNode(_In_opt_ HWCompNode *pCompNode)
{
    ASSERT(IsInPCScene());

    m_propertyRenderData.pc.pLastCompNodeInSubgraphNoRef = pCompNode;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the cached ptr to the last comp node added in this UIElement's subgraph.
//
//------------------------------------------------------------------------
HWCompNode* CUIElement::GetLastCompNode()
{
    return IsInPCScene()
         ? m_propertyRenderData.pc.pLastCompNodeInSubgraphNoRef
         : NULL;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Caches a bit tracking whether the inherited transform was animating last time this
//      element was rendered. Used for redirected elements only.
//
//------------------------------------------------------------------------
void
CUIElement::SetWasRedirectedTransformAnimating(
    bool wasRedirectedTransformAnimating
    )
{
    ASSERT(IsInPCScene());
    ASSERT(IsRedirectionElement());

    m_propertyRenderData.pc.wasRedirectedTransformAnimating = wasRedirectedTransformAnimating;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the cached bit stating whether the transform was animating last frame or
//      not.
//
//------------------------------------------------------------------------
bool
CUIElement::GetWasRedirectedTransformAnimating()
{
    ASSERT(IsRedirectionElement());

    return IsInPCScene()
         ? m_propertyRenderData.pc.wasRedirectedTransformAnimating
         : FALSE;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns TRUE if the transform to root on the composition peer has changed.
//
//------------------------------------------------------------------------
bool
CUIElement::UpdateCompositionPeerTransformToRoot(
    _In_ const CTransformToRoot *pTransformToRoot
    )
{
    ASSERT(m_pCompositionPeer != NULL);

    return m_pCompositionPeer->UpdateTransformToRoot(pTransformToRoot);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns independently animated brushes for rendering with PC.
//      The brushes returned will be cloned and sent to the compositor,
//      where they may be the targets of an independent animation. It's
//      also possible that they won't be animated.
//
//------------------------------------------------------------------------
void
CUIElement::GetIndependentlyAnimatedBrushes(
    _Outptr_result_maybenull_ CSolidColorBrush **ppFillBrush,
    _Outptr_result_maybenull_ CSolidColorBrush **ppStrokeBrush
    )
{
    *ppFillBrush = nullptr;
    *ppStrokeBrush = nullptr;
}

// Determine if pixel snapping should be disabled, to prevent jittering during transform animation
bool CUIElement::ShouldDisablePixelSnapping()
{
    // Enable pixel snapping by default
    bool disablePixelSnapping = false;

    if (IsTransformOrOffsetAffectingPropertyIndependentlyAnimating())
    {
        // Transform is being animated either by DManip or XAML

        if (IsManipulatedIndependently())
        {
            // DManip is animating the transform.

            XDMViewportStatus status = XcpDMViewportBuilding;
            IFCFAILFAST(GetContext()->GetInputServices()->GetDirectManipulationViewportStatus(this, &status));

            if (status == XcpDMViewportInertia)
            {
                // Element has inertia, so disable pixel snapping to prevent jittering.
                // In other states, like panning, enable pixel snapping, so content can be clearly rendered.
                disablePixelSnapping = true;
            }
        }
        else
        {
            // XAML is animating the transform, so disable pixel snapping to prevent jittering.
            disablePixelSnapping = true;
        }
    }

    return disablePixelSnapping;
}

//------------------------------------------------------------------------
//
//  Method:   CUIElement::OnKeyUp
//
//  Synopsis:
//      Static event handler for KeyUp event on the specified UIElement
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CUIElement::OnKeyUp(
    _In_ CDependencyObject *pSender,
    _In_ CEventArgs* pEventArgs)
{
    CUIElement* pUIElement = nullptr;

    IFC_RETURN(DoPointerCast(pUIElement, pSender));
    IFC_NOTRACE_RETURN(pUIElement->OnKeyUp(pEventArgs));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIElement::OnKeyDown
//
//  Synopsis:
//      Static event handler for KeyDown event on the specified UIElement
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CUIElement::OnKeyDown(
    _In_ CDependencyObject *pSender,
    _In_ CEventArgs* pEventArgs)
{
    CUIElement* pUIElement = nullptr;

    IFC_RETURN(DoPointerCast(pUIElement, pSender));
    IFC_NOTRACE_RETURN(pUIElement->OnKeyDown(pEventArgs));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIElement::OnKeyDown
//
//  Synopsis:
//      Virtual for KeyDown event on the specified UIElement
//
//------------------------------------------------------------------------
 _Check_return_ HRESULT CUIElement::OnKeyDown(_In_ CEventArgs* pEventArgs)
{
     /*
     1. We take different paths for raising events depending on whether the source is a UIElement or a Control
     2. The DXAML layer OnKeyDown virtual is defined on Control

     As a result, we execute similar logic to process KeyboardAccelerators in both CUIElement::OnKeyDown and Control::OnKeyDown
     One deals with controls, this deals with all other UIElements.
     */
     CKeyEventArgs* const pKeyRoutedEventArgs = static_cast<CKeyEventArgs*>(pEventArgs);
     BOOLEAN handled = FALSE;
     BOOLEAN handledShouldNotImpedeTextInput = FALSE;
     DirectUI::VirtualKey dxamlOriginalKey;
     wsy::VirtualKeyModifiers keyModifiers;

     IFC_RETURN(pKeyRoutedEventArgs->get_OriginalKey(&dxamlOriginalKey));

     const wsy::VirtualKey originalKey = static_cast<wsy::VirtualKey>(dxamlOriginalKey);

     IFC_RETURN(CoreImports::Input_GetKeyboardModifiers(&keyModifiers));

     if (KeyboardAcceleratorUtility::IsKeyValidForAccelerators(originalKey, KeyboardAcceleratorUtility::MapVirtualKeyModifiersToIntegersModifiers(keyModifiers)))
     {
         IFC_RETURN(KeyboardAcceleratorUtility::ProcessKeyboardAccelerators(
             originalKey,
             keyModifiers,
             VisualTree::GetContentRootForElement(this)->GetAllLiveKeyboardAccelerators(),
             this,
             &handled,
             &handledShouldNotImpedeTextInput,
             nullptr,
             false));

         if (handled)
         {
             IFC_RETURN(pKeyRoutedEventArgs->put_Handled(TRUE));
         }
         if (handledShouldNotImpedeTextInput)
         {
             IFC_RETURN(pKeyRoutedEventArgs->put_HandledShouldNotImpedeTextInput(TRUE));
         }
     }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIElement::OnPreviewKeyDown
//
//  Synopsis:
//      Static event handler for PreviewKeyDown event on the specified UIElement
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CUIElement::OnPreviewKeyDown(
    _In_ CDependencyObject *pSender,
    _In_ CEventArgs* pEventArgs
)
{
    // No core element overrides this yet, so no need to have a virtual in the vtable bloating us
    // until that is needed.  Note that overriding this in the DXAML layer or via the public
    // API is still fine, you just don't get a handy virtual in core until someone needs it.
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIElement::OnPreviewKeyUp
//
//  Synopsis:
//      Static event handler for PreviewKeyUp event on the specified UIElement
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CUIElement::OnPreviewKeyUp(
    _In_ CDependencyObject *pSender,
    _In_ CEventArgs* pEventArgs
)
{
    // No core element overrides this yet, so no need to have a virtual in the vtable bloating us
    // until that is needed.  Note that overriding this in the DXAML layer or via the public
    // API is still fine, you just don't get a handy virtual in core until someone needs it.
    return S_OK;
}

// Static event handler for ProcessKeyboardAccelerators event on the specified UIElement
/* static */
_Check_return_ HRESULT CUIElement::OnProcessKeyboardAccelerators(
    _In_ CDependencyObject *pSender,
    _In_ CEventArgs* pEventArgs
)
{
    // No core element overrides this yet, so no need to have a virtual in the vtable bloating us
    // until that is needed.  Note that overriding this in the DXAML layer or via the public
    // API is still fine, you just don't get a handy virtual in core until someone needs it.
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIElement::OnKeyboardAcceleratorInvoked
//
//  Synopsis:
//      Static event handler for KeyboardAcceleratorInvoked event on the specified UIElement
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CUIElement::OnKeyboardAcceleratorInvoked(
    _In_ CDependencyObject *pSender,
    _In_ CEventArgs* pEventArgs
)
{
    // No core element overrides this yet, so no need to have a virtual in the vtable bloating us
    // until that is needed.  Note that overriding this in the DXAML layer or via the public
    // API is still fine, you just don't get a handy virtual in core until someone needs it.
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIElement::OnGotFocus
//
//  Synopsis:
//      Static event handler for GotFocus event on the specified UIElement
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CUIElement::OnGotFocus(
    _In_ CDependencyObject *pSender,
    _In_ CEventArgs* pEventArgs)
{
    CUIElement* pUIElement = nullptr;

    IFC_RETURN(DoPointerCast(pUIElement, pSender));
    IFC_NOTRACE_RETURN(pUIElement->OnGotFocus(pEventArgs));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIElement::OnGettingFocus
//
//  Synopsis:
//      Static event handler for GettingFocus event on the specified UIElement
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CUIElement::OnGettingFocus(
    _In_ CDependencyObject *pSender,
    _In_ CEventArgs* pEventArgs
)
{
    // No core element overrides this yet, so no need to have a virtual in the vtable bloating us
    // until that is needed.  Note that overriding this in the DXAML layer or via the public
    // API is still fine, you just don't get a handy virtual in core until someone needs it.
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIElement::OnLostFocus
//
//  Synopsis:
//      Static event handler for LostFocus event on the specified UIElement
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CUIElement::OnLostFocus(
    _In_ CDependencyObject *pSender,
    _In_ CEventArgs* pEventArgs)
{
    CUIElement* pUIElement = nullptr;

    IFC_RETURN(DoPointerCast(pUIElement, pSender));
    IFC_NOTRACE_RETURN(pUIElement->OnLostFocus(pEventArgs));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIElement::OnLosingFocus
//
//  Synopsis:
//      Static event handler for LosingFocus event on the specified UIElement
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CUIElement::OnLosingFocus(
    _In_ CDependencyObject *pSender,
    _In_ CEventArgs* pEventArgs
)
{
    // No core element overrides this yet, so no need to have a virtual in the vtable bloating us
    // until that is needed.  Note that overriding this in the DXAML layer or via the public
    // API is still fine, you just don't get a handy virtual in core until someone needs it.
    return S_OK;
}

_Check_return_ HRESULT CUIElement::OnNoFocusCandidateFound(
    _In_ CDependencyObject *pSender,
    _In_ CEventArgs* pEventArgs
)
{
    // No core element overrides this yet, so no need to have a virtual in the vtable bloating us
    // until that is needed.  Note that overriding this in the DXAML layer or via the public
    // API is still fine, you just don't get a handy virtual in core until someone needs it.
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIElement::OnIsEnabledChanged
//
//  Synopsis:
//      Static event handler for IsEnabledChanged event on the specified UIElement
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CUIElement::OnIsEnabledChanged(
    _In_ CDependencyObject *pSender,
    _In_ CEventArgs* pEventArgs)
{
    CUIElement* pUIElement = nullptr;

    IFC_RETURN(DoPointerCast(pUIElement, pSender));
    IFC_NOTRACE_RETURN(pUIElement->OnIsEnabledChanged(pEventArgs));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIElement::OnInheritedPropertyChanged
//
//  Synopsis:
//      Static event handler for OnInheritedPropertyChanged event on the specified UIElement
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CUIElement::OnInheritedPropertyChanged(
    _In_ CDependencyObject *pSender,
    _In_ CEventArgs* pEventArgs)
{
    CUIElement* pUIElement = nullptr;

    IFC_RETURN(DoPointerCast(pUIElement, pSender));
    IFC_NOTRACE_RETURN(pUIElement->OnInheritedPropertyChanged(pEventArgs));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIElement::OnCharacterReceived
//
//  Synopsis:
//      Static event handler for CharacterReceived event on the specified UIElement
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CUIElement::OnCharacterReceived(
    _In_ CDependencyObject *pSender,
    _In_ CEventArgs* pEventArgs)
{
    CUIElement* pUIElement = nullptr;

    IFC_RETURN(DoPointerCast(pUIElement, pSender));
    IFC_NOTRACE_RETURN(pUIElement->OnCharacterReceived(pEventArgs));

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Method:   CUIElement::OnDragEnter
//
//  Synopsis:
//      Static event handler for DragEnter event on the
//      specified UIElement
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CUIElement::OnDragEnter(
    _In_ CDependencyObject *pSender,
    _In_ CEventArgs* pEventArgs)
{
    // No core element overrides this yet, so no need to have a virtual in the vtable bloating us
    // until that is needed.  Note that overriding this in the DXAML layer or via the public
    // API is still fine, you just don't get a handy virtual in core until someone needs it.
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIElement::OnDragLeave
//
//  Synopsis:
//      Static event handler for DragLeave event on the
//      specified UIElement
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CUIElement::OnDragLeave(
    _In_ CDependencyObject *pSender,
    _In_ CEventArgs* pEventArgs)
{
    // No core element overrides this yet, so no need to have a virtual in the vtable bloating us
    // until that is needed.  Note that overriding this in the DXAML layer or via the public
    // API is still fine, you just don't get a handy virtual in core until someone needs it.
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIElement::OnDragOver
//
//  Synopsis:
//      Static event handler for DragOver event on the
//      specified UIElement
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CUIElement::OnDragOver(
    _In_ CDependencyObject *pSender,
    _In_ CEventArgs* pEventArgs)
{
    // No core element overrides this yet, so no need to have a virtual in the vtable bloating us
    // until that is needed.  Note that overriding this in the DXAML layer or via the public
    // API is still fine, you just don't get a handy virtual in core until someone needs it.
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIElement::OnDrop
//
//  Synopsis:
//      Static event handler for Drop event on the
//      specified UIElement
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CUIElement::OnDrop(
    _In_ CDependencyObject *pSender,
    _In_ CEventArgs* pEventArgs)
{
    // No core element overrides this yet, so no need to have a virtual in the vtable bloating us
    // until that is needed.  Note that overriding this in the DXAML layer or via the public
    // API is still fine, you just don't get a handy virtual in core until someone needs it.
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIElement::OnBringIntoViewRequested
//
//  Synopsis:
//      Static event handler for BringIntoViewRequested event on the specified UIElement
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CUIElement::OnBringIntoViewRequested(
    _In_ CDependencyObject *pSender,
    _In_ CEventArgs* pEventArgs
)
{
    CUIElement* pUIElement = nullptr;
    CRoutedEventArgs* const pBringIntoViewRequestedArgs = static_cast<CRoutedEventArgs*>(pEventArgs);

    IFC_RETURN(DoPointerCast(pUIElement, pSender));
    IFC_RETURN(FxCallbacks::UIElement_OnBringIntoViewRequested(pUIElement, pBringIntoViewRequestedArgs));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIElement::OnPointerEntered
//
//  Synopsis:
//      Static event handler for PointerEntered event on the
//      specified UIElement
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CUIElement::OnPointerEntered(
    _In_ CDependencyObject *pSender,
    _In_ CEventArgs* pEventArgs)
{
    CUIElement* pUIElement = nullptr;
    IFC_RETURN(DoPointerCast(pUIElement, pSender));
    IFC_NOTRACE_RETURN(pUIElement->OnPointerEntered(pEventArgs));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIElement::OnPointerPressed
//
//  Synopsis:
//      Static event handler for PointerPressed event on the
//      specified UIElement
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CUIElement::OnPointerPressed(
    _In_ CDependencyObject *pSender,
    _In_ CEventArgs* pEventArgs)
{
    CUIElement* pUIElement = nullptr;

    IFC_RETURN(DoPointerCast(pUIElement, pSender));
    IFC_NOTRACE_RETURN(pUIElement->OnPointerPressed(pEventArgs));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIElement::OnPointerMoved
//
//  Synopsis:
//      Static event handler for PointerMoved event on the
//      specified UIElement
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CUIElement::OnPointerMoved(
    _In_ CDependencyObject *pSender,
    _In_ CEventArgs* pEventArgs)
{
    CUIElement* pUIElement = nullptr;

    IFC_RETURN(DoPointerCast(pUIElement, pSender));
    IFC_NOTRACE_RETURN(pUIElement->OnPointerMoved(pEventArgs));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIElement::OnPointerReleased
//
//  Synopsis:
//      Static event handler for PointerReleased event on the
//      specified UIElement
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CUIElement::OnPointerReleased(
    _In_ CDependencyObject *pSender,
    _In_ CEventArgs* pEventArgs)
{
    CUIElement* pUIElement = nullptr;

    IFC_RETURN(DoPointerCast(pUIElement, pSender));
    IFC_NOTRACE_RETURN(pUIElement->OnPointerReleased(pEventArgs));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIElement::OnPointerExited
//
//  Synopsis:
//      Static event handler for PointerExited event on the
//      specified UIElement
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CUIElement::OnPointerExited(
    _In_ CDependencyObject *pSender,
    _In_ CEventArgs* pEventArgs)
{
    CUIElement* pUIElement = nullptr;

    IFC_RETURN(DoPointerCast(pUIElement, pSender));
    IFC_NOTRACE_RETURN(pUIElement->OnPointerExited(pEventArgs));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIElement::OnPointerCaptureLost
//
//  Synopsis:
//      Static event handler for PointerCaptureLost event on the specified UIElement
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CUIElement::OnPointerCaptureLost(
    _In_ CDependencyObject *pSender,
    _In_ CEventArgs* pEventArgs)
{
    CUIElement* pUIElement = nullptr;

    IFC_RETURN(DoPointerCast(pUIElement, pSender));
    IFC_NOTRACE_RETURN(pUIElement->OnPointerCaptureLost(pEventArgs));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIElement::OnPointerCanceled
//
//  Synopsis:
//      Static event handler for PointerCanceled event on the
//      specified UIElement
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CUIElement::OnPointerCanceled(
    _In_ CDependencyObject *pSender,
    _In_ CEventArgs* pEventArgs)
{
    // No core element overrides this yet, so no need to have a virtual in the vtable bloating us
    // until that is needed.  Note that overriding this in the DXAML layer or via the public
    // API is still fine, you just don't get a handy virtual in core until someone needs it.
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIElement::OnPointerWheelChanged
//
//  Synopsis:
//      Static event handler for PointerWheelChanged event on the
//      specified UIElement
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CUIElement::OnPointerWheelChanged(
    _In_ CDependencyObject *pSender,
    _In_ CEventArgs* pEventArgs)
{
    // No core element overrides this yet, so no need to have a virtual in the vtable bloating us
    // until that is needed.  Note that overriding this in the DXAML layer or via the public
    // API is still fine, you just don't get a handy virtual in core until someone needs it.
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIElement::OnTapped
//
//  Synopsis:
//      Static event handler for Tapped event on the
//      specified UIElement
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CUIElement::OnTapped(
    _In_ CDependencyObject *pSender,
    _In_ CEventArgs* pEventArgs)
{
    CUIElement* pUIElement = nullptr;

    IFC_RETURN(DoPointerCast(pUIElement, pSender));
    IFC_NOTRACE_RETURN(pUIElement->OnTapped(pEventArgs));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIElement::OnDoubleTapped
//
//  Synopsis:
//      Static event handler for DoubleTapped event on the
//      specified UIElement
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CUIElement::OnDoubleTapped(
    _In_ CDependencyObject *pSender,
    _In_ CEventArgs* pEventArgs)
{
    CUIElement* pUIElement = nullptr;

    IFC_RETURN(DoPointerCast(pUIElement, pSender));
    IFC_NOTRACE_RETURN(pUIElement->OnDoubleTapped(pEventArgs));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIElement::OnHolding
//
//  Synopsis:
//      Static event handler for Holding event on the
//      specified UIElement
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CUIElement::OnHolding(
    _In_ CDependencyObject *pSender,
    _In_ CEventArgs* pEventArgs)
{
    CUIElement* pUIElement = nullptr;

    IFC_RETURN(DoPointerCast(pUIElement, pSender));
    IFC_NOTRACE_RETURN(pUIElement->OnHolding(pEventArgs));

    return S_OK;
}

// If the UIElement has the ContextFlyout property set, show the flyout and set the event to be "Handled". Otherwise, no-op.
_Check_return_ HRESULT CUIElement::OnContextRequested(
    _In_ CDependencyObject *pSender,
    _In_ CEventArgs* pEventArgs)
{
    IFC_RETURN(OnContextRequestedCore(pSender, pSender, pEventArgs));
    return S_OK;
}

// In list-based circumstances, we want to display a list control's ContextFlyout on a list item.
// Having a separate contextFlyoutObject allows us to separate the object that owns the ContextFlyout
// from the object that we want to display it on.
_Check_return_ HRESULT CUIElement::OnContextRequestedCore(
    _In_ CDependencyObject *sender,
    _In_ CDependencyObject *contextFlyoutObject,
    _In_ CEventArgs* eventArgs)
{
    CUIElement* uiElementNoRef = nullptr;
    IFC_RETURN(DoPointerCast(uiElementNoRef, sender));
    CUIElement* contextFlyoutElementNoRef = nullptr;
    IFC_RETURN(DoPointerCast(contextFlyoutElementNoRef, contextFlyoutObject));
    auto args = static_cast<CContextRequestedEventArgs*>(eventArgs);

    if (!args->m_bHandled)
    {
        xref_ptr<CFlyoutBase> flyout = contextFlyoutElementNoRef->GetContextFlyout();
        if (flyout)
        {
            CFrameworkElement *frameworkElementNoRef = nullptr;
            wf::Point point = { 0, 0 };
            BOOLEAN gotPoint = FALSE;

            IFC_RETURN(DoPointerCast(frameworkElementNoRef, uiElementNoRef));
            IFC_RETURN(args->TryGetPosition(uiElementNoRef, &point, &gotPoint));

            const bool isTextControl = CTextCore::IsTextControl(sender);
            const KnownTypeIndex senderTypeIndex = sender->GetTypeIndex();
            const bool isTextEditControl =
                senderTypeIndex == KnownTypeIndex::TextBox ||
                senderTypeIndex == KnownTypeIndex::RichEditBox ||
                senderTypeIndex == KnownTypeIndex::PasswordBox;

            if (gotPoint || isTextEditControl)
            {
                // If we're here but don't yet have a point, then we're a text edit control.
                // We'll set the point at which to open the flyout to be the point retrieved
                // from the text box based on the selection start or end position (depending on
                // LTR vs. RTL).
                if (!gotPoint)
                {
                    auto textBox = do_pointer_cast<CTextBoxBase>(uiElementNoRef);
                    ASSERT(textBox);

                    XPOINTF contextMenuShowPosition{};
                    IFC_RETURN(textBox->GetContextMenuShowPosition(&contextMenuShowPosition));

                    point = { contextMenuShowPosition.x, contextMenuShowPosition.y };
                }

                if (isTextControl)
                {
                    // If we are using the default text control ContextFlyout and TextSelection is not enabled, don't show the flyout:
                    if (sender->IsPropertyDefaultByIndex(KnownPropertyIndex::UIElement_ContextFlyout))
                    {
                        if (!CTextCore::IsTextSelectionEnabled(sender)) { return S_OK; }
                    }
                    IFC_RETURN(FxCallbacks::TextControlFlyout_ShowAt(flyout.get(), frameworkElementNoRef, point, RectUtil::CreateEmptyRect(), xaml_primitives::FlyoutShowMode_Standard));
                }
                else
                {
                    IFC_RETURN(FxCallbacks::FlyoutBase_ShowAt(flyout.get(), frameworkElementNoRef, point, RectUtil::CreateEmptyRect(), xaml_primitives::FlyoutShowMode_Standard));
                }
            }
            else if (flyout->GetTypeIndex() == KnownTypeIndex::MenuFlyout && gotPoint)
            {
                IFC_RETURN(FxCallbacks::MenuFlyout_ShowAt(static_cast<CMenuFlyout*>(flyout.get()), uiElementNoRef, point));
            }
            else
            {
                if (isTextControl)
                {
                    if (sender->IsPropertyDefaultByIndex(KnownPropertyIndex::UIElement_ContextFlyout))
                    {
                        if (!CTextCore::IsTextSelectionEnabled(sender)) { return S_OK; }
                    }
                    IFC_RETURN(FxCallbacks::TextControlFlyout_ShowAt(flyout.get(), frameworkElementNoRef, point, RectUtil::CreateEmptyRect(), xaml_primitives::FlyoutShowMode_Standard));
                }
                else
                {
                    IFC_RETURN(FxCallbacks::FlyoutBase_ShowAt(flyout.get(), frameworkElementNoRef));
                }
            }

            args->m_bHandled = TRUE;
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CUIElement::OnContextCanceled(
    _In_ CDependencyObject *pSender,
    _In_ CEventArgs* pEventArgs)
{
    // No core element overrides this yet, so no need to have a virtual in the vtable bloating us
    // until that is needed.  Note that overriding this in the DXAML layer or via the public
    // API is still fine, you just don't get a handy virtual in core until someone needs it.
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIElement::OnRightTapped
//
//  Synopsis:
//      Static event handler for RightTapped event on the
//      specified UIElement
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CUIElement::OnRightTapped(
    _In_ CDependencyObject *pSender,
    _In_ CEventArgs* pEventArgs)
{
    CUIElement* pUIElement = nullptr;

    IFC_RETURN(DoPointerCast(pUIElement, pSender));
    IFC_NOTRACE_RETURN(pUIElement->OnRightTapped(pEventArgs));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIElement::OnRightTappedUnhandled
//
//  Synopsis:
//      Static event handler for RightTappedUnhandled event on the
//      specified UIElement
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CUIElement::OnRightTappedUnhandled(
    _In_ CDependencyObject *pSender,
    _In_ CEventArgs* pEventArgs)
{
    // No core element overrides this yet, so no need to have a virtual in the vtable bloating us
    // until that is needed.  Note that overriding this in the DXAML layer or via the public
    // API is still fine, you just don't get a handy virtual in core until someone needs it.
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIElement::OnManipulationStarting
//
//  Synopsis:
//      Static event handler for OnManipulationStarting event on the
//      specified UIElement
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CUIElement::OnManipulationStarting(
    _In_ CDependencyObject *pSender,
    _In_ CEventArgs* pEventArgs)
{
    // No core element overrides this yet, so no need to have a virtual in the vtable bloating us
    // until that is needed.  Note that overriding this in the DXAML layer or via the public
    // API is still fine, you just don't get a handy virtual in core until someone needs it.
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIElement::OnManipulationInertiaStarting
//
//  Synopsis:
//      Static event handler for OnManipulationInertiaStarting event on the
//      specified UIElement
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CUIElement::OnManipulationInertiaStarting(
    _In_ CDependencyObject *pSender,
    _In_ CEventArgs* pEventArgs)
{
    // No core element overrides this yet, so no need to have a virtual in the vtable bloating us
    // until that is needed.  Note that overriding this in the DXAML layer or via the public
    // API is still fine, you just don't get a handy virtual in core until someone needs it.
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIElement::OnManipulationStarted
//
//  Synopsis:
//      Static event handler for ManipulationStarted event on the
//      specified UIElement
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CUIElement::OnManipulationStarted(
    _In_ CDependencyObject *pSender,
    _In_ CEventArgs* pEventArgs)
{
    // No core element overrides this yet, so no need to have a virtual in the vtable bloating us
    // until that is needed.  Note that overriding this in the DXAML layer or via the public
    // API is still fine, you just don't get a handy virtual in core until someone needs it.
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIElement::OnManipulationDelta
//
//  Synopsis:
//      Static event handler for ManipulationDelta event on the
//      specified UIElement
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CUIElement::OnManipulationDelta(
    _In_ CDependencyObject *pSender,
    _In_ CEventArgs* pEventArgs)
{
    // No core element overrides this yet, so no need to have a virtual in the vtable bloating us
    // until that is needed.  Note that overriding this in the DXAML layer or via the public
    // API is still fine, you just don't get a handy virtual in core until someone needs it.
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CUIElement::OnManipulationCompleted
//
//  Synopsis:
//      Static event handler for ManipulationCompleted event on the
//      specified UIElement
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CUIElement::OnManipulationCompleted(
    _In_ CDependencyObject *pSender,
    _In_ CEventArgs* pEventArgs)
{
    // No core element overrides this yet, so no need to have a virtual in the vtable bloating us
    // until that is needed.  Note that overriding this in the DXAML layer or via the public
    // API is still fine, you just don't get a handy virtual in core until someone needs it.
    return S_OK;
}

bool CUIElement::HasTransform3DInSubtree(_In_opt_ const HitTestParams *hitTestParams) const
{
    // TODO: HitTest: Method not needed. This can just be Has3DDepthOnSelfOrSubtree() in the new hit testing walk.
    return (hitTestParams != nullptr && hitTestParams->hasTransform3DInSubtree) || HasDepthLegacy() || Has3DDepthOnSelfOrSubtree();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Ensure that the elements inner content bounds are up to date.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CUIElement::EnsureContentInnerBounds(_In_opt_ HitTestParams *hitTestParams)
{
    //
    // Check if elements inner bounds are dirty.
    //
    if (AreContentInnerBoundsDirty())
    {
        if (hitTestParams != nullptr && hitTestParams->m_hitTestPerfData != nullptr)
        {
            hitTestParams->m_hitTestPerfData->m_contentBoundsRecalc++;
        }

        IFC_RETURN(GenerateContentBounds(&m_contentInnerBounds));

        CleanContentInnerBounds();
    }
    else if (hitTestParams != nullptr && hitTestParams->m_hitTestPerfData != nullptr)
    {
        hitTestParams->m_hitTestPerfData->m_contentBoundsReuse++;
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Ensure the elements child bounds are up to date.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElement::EnsureChildBounds(_In_opt_ HitTestParams *hitTestParams)
{
    if (AreChildBoundsDirty())
    {
        if (hitTestParams != nullptr && hitTestParams->m_hitTestPerfData != nullptr)
        {
            hitTestParams->m_hitTestPerfData->m_childBoundsRecalc++;
        }

        //
        // Get outer bounds of all child elements.
        //
        IFC_RETURN(GenerateChildOuterBounds(hitTestParams, &m_childBounds));

        CTransitionRoot* transitionRootNoRef = GetLocalTransitionRoot(false /*ensureTransitionRoot*/);
        if (transitionRootNoRef != NULL)
        {
            // Special case for transform changes that affect LayoutTransitionElements:
            // LayoutTransitionElements propagate bounds-change notifications for local changes and changes in
            // their subgraph to the TransitionRoot, and then to this element.
            // The LTE's bounds, however, depend on the entire transform-to-root from the target element. If
            // the transform changes on an ancestor, the LTE is not notified but it may still need its bounds updated.
            //
            // The target element is guaranteed to be in the subgraph of this element, since TransitionRoots are always
            // added somewhere in the parent chain of the target. The bounds of the TransitionRoot (combined bounds of
            // LTEs) are stored in the local space of this element so they can be combined with the other children's bounds.
            // This yields two cases for an inherited change. If the transform change occurred on or above _this_ element,
            // it's okay that the LTE wasn't notified and the bounds weren't recalculated, because the bounds are local.
            // The transforms above this element are ignored when bounding the LTEs.
            // If the transform change occurred on an ancestor of the LTE's target that was in the subgraph of this element,
            // the LTE's bounds do need to be recalculated. Fortunately, as mentioned before, the child bounds will already
            // be dirty in this case since the change happened in the subgraph - the only problem is that the TransitionRoot
            // and LTE themselves won't be dirty. The work-around is to invalidate their bounds directly so that the LTE
            // bounds are updated whenever this element's child bounds are recalculated.
            //
            // TODO: HWPC: The TransitionRoot might not be in the parent chain if there's an open Popup
            // TODO: HWPC: above the transition. The situation will be difficult to hit though; the
            // TODO: HWPC: TransitionRoot must be attached above the Popup in the tree, but not at the
            // TODO: HWPC: RootVisual. In this situation, the TransitionRoot's parent would potentially
            // TODO: HWPC: not be notified of bounds changes in the target element's subgraph, since the
            // TODO: HWPC: dirty flags would propagate up to the Popup.Child and directly to the PopupRoot.
            transitionRootNoRef->InvalidateChildBounds();

            XRECTF_RB lteBounds;
            IFC_RETURN(transitionRootNoRef->GetOuterBounds(hitTestParams, &lteBounds));
            UnionRectF(&m_childBounds, &lteBounds);
        }

        CleanChildBounds();
    }
    else if (hitTestParams != nullptr && hitTestParams->m_hitTestPerfData != nullptr)
    {
        hitTestParams->m_hitTestPerfData->m_childBoundsReuse++;
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Ensure the inner bounds of element are up to date.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CUIElement::EnsureInnerBounds(_In_opt_ HitTestParams *hitTestParams)
{
    if (AreInnerBoundsDirty())
    {
        if (hitTestParams != nullptr && hitTestParams->m_hitTestPerfData != nullptr)
        {
            hitTestParams->m_hitTestPerfData->m_innerBoundsRecalc++;
        }

        if (IsVisible())
        {
            // Ensure inner content bounds are up to date.
            IFC_RETURN(EnsureContentInnerBounds(hitTestParams));

            // Ensure child elements combined bounds are up to date.
            IFC_RETURN(EnsureChildBounds(hitTestParams));

            // Combine the child outer bounds with the element's content inner bounds.
            // Together these are the complete inner bounds of the element.
            m_combinedInnerBounds = m_contentInnerBounds;
            UnionRectF(&m_combinedInnerBounds, &m_childBounds);
        }
        else
        {
            // Collapsed elements have no inner bounds.
            // Note that the cached content and child bounds are preserved, along with
            // their dirtiness. This ensures the bounds can be correctly recalculated
            // if the element becomes visible again.
            EmptyRectF(&m_combinedInnerBounds);
        }

        CleanInnerBounds();
    }
    else if (hitTestParams != nullptr && hitTestParams->m_hitTestPerfData != nullptr)
    {
        hitTestParams->m_hitTestPerfData->m_innerBoundsReuse++;
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Get the inner bounds of the element.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElement::GetInnerBounds(
    _Out_ XRECTF_RB* pBounds
    )
{
    IFC_RETURN(GetInnerBounds(nullptr /*hitTestParams*/, pBounds));

    return S_OK;
}

_Check_return_ HRESULT
CUIElement::GetInnerBounds(
    _In_opt_ HitTestParams *hitTestParams,
    _Out_ XRECTF_RB* pBounds
    )
{
    IFC_RETURN(EnsureInnerBounds(hitTestParams));

    *pBounds = m_combinedInnerBounds;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Ensure the outer bounds of the element are up to date.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CUIElement::EnsureOuterBounds(_In_opt_ HitTestParams *hitTestParams)
{
    if (AreOuterBoundsDirty())
    {
        if (hitTestParams != nullptr && hitTestParams->m_hitTestPerfData != nullptr)
        {
            hitTestParams->m_hitTestPerfData->m_outerBoundsRecalc++;
        }

        if (Has3DDepth())
        {
            // Do not calculate outer bounds. The outer bounds aren't used to guide the hit testing walk.
            // Set to null bounds. UnionRectF knows to ignore rects with right < left.
            m_outerBounds = { 1, 1, 0, 0 };

            // Still recursively update bounds on children.
            IFC_RETURN(EnsureInnerBounds(hitTestParams));
        }
        else
        {
            IFC_RETURN(GetInnerBounds(hitTestParams, &m_outerBounds));
            if (!TransformToOuter2D(hitTestParams, m_outerBounds))
            {
                // We ended up with negative W values after transforming through the projection. The bounds are invalid.
                EmptyRectF(&m_outerBounds);
            }
        }

        CleanOuterBounds();
    }
    else
    {
        // Can't have clean outer bounds without clean inner bounds.
        ASSERT(!AreInnerBoundsDirty());

        if (hitTestParams != nullptr && hitTestParams->m_hitTestPerfData != nullptr)
        {
            hitTestParams->m_hitTestPerfData->m_outerBoundsReuse++;
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Get the outer bounds of the element.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElement::GetOuterBounds(
    _In_opt_ HitTestParams *hitTestParams,
    _Out_ XRECTF_RB* pBounds
    )
{
    IFC_RETURN(EnsureOuterBounds(hitTestParams));

    *pBounds = m_outerBounds;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Get the content bounds of the element.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElement::GetContentInnerBounds(
    _Out_ XRECTF_RB* pBounds
    )
{
    IFC_RETURN(EnsureContentInnerBounds(nullptr /* hitTestParams */));

    *pBounds = m_contentInnerBounds;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Get the combined outer bounds of all children.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CUIElement::GenerateChildOuterBounds(
    _In_opt_ HitTestParams *hitTestParams,
    _Out_ XRECTF_RB* pBounds
    )
{
    const auto pCollection = static_cast<const CUIElementCollection*>(GetChildren());

    EmptyRectF(pBounds);

    if (pCollection)
    {
        XUINT32 childCount = 0;
        CUIElement** ppUIElements = nullptr;

        GetChildrenInRenderOrder(&ppUIElements, &childCount);

        for (XUINT32 i = 0; i < childCount; ++i)
        {
            XRECTF_RB childBounds = { };
            const CUIElement* pElement = ppUIElements[i];

            // STRESS CRASH WORKAROUND:  This fixes the symptom (the crash) rather than the cause...UIAutomation
            // calling into the system to get the bounds during a focus change triggered by an element being removed from
            // the tree (at this point in the Leave walk the UIElement collection has NULL entries)
            // WIN8 Bug 701817:  Fix the cause
            if (pElement)
            {
                if (pElement->GetTypeIndex() != KnownTypeIndex::Popup && !pElement->IsHiddenForLayoutTransition())
                {
                    IFC_RETURN(ppUIElements[i]->GetOuterBounds(hitTestParams, &childBounds));

                    UnionRectF(pBounds, &childBounds);
                }
            }
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Generate the inner bounds of the elements content.
//
//  NOTE:
//      Overridden in derived classes to provide bounds.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElement::GenerateContentBounds(
    _Out_ XRECTF_RB* pBounds
    )
{
    HRESULT hr = S_OK;

    EmptyRectF(pBounds);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Walk the element and its children finding elements that intersect
//      with the given point/polygon.
//
//------------------------------------------------------------------------
template <typename HitType>
_Check_return_ HRESULT
CUIElement::BoundsTestEntry(
    const HitTestParams& hitTestParams,
    _In_ const HitType& target,
    _In_ CBoundedHitTestVisitor* pCallback,
    _In_ bool canHitDisabledElements,
    _In_ bool canHitInvisibleElements
    )
{
    HitTestParams myParams(hitTestParams);

    TraceHitTestBegin();

    // TODO: HitTest: Consider leaving layout stale and only updating layout during Tick
    IFC_RETURN(UpdateLayout());

    // Ensure all bounds in the subgraph are up-to-date for hit-testing.
    IFC_RETURN(EnsureOuterBounds(&myParams));   // Use the non-const cloned params

    IFC_RETURN(BoundsTestInternal(target, pCallback, &hitTestParams, canHitDisabledElements, canHitInvisibleElements, nullptr /* pResult */));

    TraceHitTestEnd();

    // TODO: HitTest: put the stats on CCoreServices or something
    TraceOuterBoundsStatsInfo(hitTestParams.m_hitTestPerfData->m_outerBoundsRecalc, hitTestParams.m_hitTestPerfData->m_outerBoundsReuse);
    TraceInnerBoundsStatsInfo(hitTestParams.m_hitTestPerfData->m_innerBoundsRecalc, hitTestParams.m_hitTestPerfData->m_innerBoundsReuse);
    TraceContentBoundsStatsInfo(hitTestParams.m_hitTestPerfData->m_contentBoundsRecalc, hitTestParams.m_hitTestPerfData->m_contentBoundsReuse);
    TraceChildBoundsStatsInfo(hitTestParams.m_hitTestPerfData->m_childBoundsRecalc, hitTestParams.m_hitTestPerfData->m_childBoundsReuse);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Walk the element and its children finding elements that intersect
//      with the given point.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElement::BoundsTestInternal(
    _In_ const XPOINTF& target,
    _In_ CBoundedHitTestVisitor* pCallback,
    _In_opt_ const HitTestParams *hitTestParams,
    _In_ bool canHitDisabledElements,
    _In_ bool canHitInvisibleElements,
    _Out_opt_ BoundsWalkHitResult* pResult
    )
{
    RRETURN(BoundsTestInternalImpl(target, pCallback, hitTestParams, canHitDisabledElements, canHitInvisibleElements, pResult));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Walk the element and its children finding elements that intersect
//      with the given polygon.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElement::BoundsTestInternal(
    _In_ const HitTestPolygon& target,
    _In_ CBoundedHitTestVisitor* pCallback,
    _In_opt_ const HitTestParams *hitTestParams,
    _In_ bool canHitDisabledElements,
    _In_ bool canHitInvisibleElements,
    _Out_opt_ BoundsWalkHitResult* pResult
    )
{
    RRETURN(BoundsTestInternalImpl(target, pCallback, hitTestParams, canHitDisabledElements, canHitInvisibleElements, pResult));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Walk the element and its children finding elements that intersect
//      with the given point/polygon.
//
//------------------------------------------------------------------------
template <typename HitType>
_Check_return_ HRESULT
CUIElement::BoundsTestInternalImpl(
    _In_ const HitType& target,
    _In_ CBoundedHitTestVisitor* pCallback,
    _In_opt_ const HitTestParams *hitTestParams,
    _In_ bool canHitDisabledElements,
    _In_ bool canHitInvisibleElements,
    _Out_opt_ BoundsWalkHitResult* pResult
    )
{
    BoundsWalkHitResult hitResult = BoundsWalkHitResult::Continue;

    // We can exit early out of the hit testing walk if we got clipped out or we couldn't do an inverse transform.
    bool continueHitTest = true;

    if (IsEnabledAndVisibleForHitTest(canHitDisabledElements, canHitInvisibleElements) && !IsHiddenForLayoutTransition())
    {
        continueHitTest =
            GetContext()->InvisibleHitTestMode()
            || DoesRectIntersectHitType(m_outerBounds, target)
            || HasTransform3DInSubtree(hitTestParams)
            || HasDepthLegacy()   // TODO: HitTest: combine this with GetHitTestingTransform3D
            || canHitInvisibleElements;

        if (continueHitTest)
        {
            HitTestParams newParams(hitTestParams);
            HitType testTarget = target;

            //
            // We start with the target point/rect in the parent coordinate space.
            //

            // Check against the layout clip, if the clip should be applied above transforms
            // In this case we should be testing the parent space point/rect against the layout clip.
            if (ShouldApplyLayoutClipAsAncestorClip() && HasLayoutClip())
            {
                IFC_RETURN(LayoutClipGeometry->ClipToFill(const_cast<HitType&>(target), nullptr, &continueHitTest));
            }

            // Transform the target point/rect into local space.
            if (continueHitTest)
            {
                CollectTransformsAndTransformToInner(newParams, testTarget);
            }

            //
            // At this point we have testTarget in local space coordinates.
            // Next we check against clips.
            //

            if (continueHitTest)
            {
                // Clip target. Prefer the hand off visual's clip over the Xaml clip, in case the app got the hand off
                // visual and updated the rect.
                const auto& clip = GetHandOffVisualClipOrElementClip();
                if (clip != nullptr)
                {
                    IFC_RETURN(clip->ClipToFill(testTarget, NULL, &continueHitTest));
                }

                // Check against the layout clip, if the clip should be applied below transforms
                if (continueHitTest && !ShouldApplyLayoutClipAsAncestorClip() && HasLayoutClip())
                {
                    IFC_RETURN(LayoutClipGeometry->ClipToFill(testTarget, NULL, &continueHitTest));
                }

                // Clip target to the implicit transition clip
                auto transitionTarget = GetTransitionTarget();
                if (continueHitTest && transitionTarget != nullptr)
                {
                    XRECTF transitionClip = {0, 0, GetActualWidth(), GetActualHeight()};
                    IFC_RETURN(transitionTarget->ClipToTransitionTarget(transitionClip, testTarget, &continueHitTest));
                }

                // Note: The ClipToFill calls above will return true and not clip the target
                // if the geometry is concave. We will still clip to our bounds rect below as a
                // first approximation.
                // Again, ignore the bounds if we're hit-testing invisible objects. Some objects like
                // shape may have an empty bounds if they have no fill/stroke and we still want to include
                // them as candidate hit elements in this case.

                if (continueHitTest && !GetContext()->InvisibleHitTestMode() && !HasTransform3DInSubtree(hitTestParams))
                {
                    // Use the pre-calculated inner bounds for fast culling if we're not in 3D mode.
                    // In 3D mode, m_combinedInnerBounds is not a valid distinguisher for fast culling.
                    ASSERT(!AreInnerBoundsDirty());
                    IFC_RETURN(ClipHitTypeToRect(testTarget, m_combinedInnerBounds, &continueHitTest));
                }

                if (continueHitTest && RequiresCompNodeForRoundedCorners())
                {
                    // Perform rounded corner hit-testing in this scenario, as the rounded corners clips children as well as content.
                    ASSERT(OfTypeByIndex<KnownTypeIndex::FrameworkElement>());
                    continueHitTest = CBorder::HitTestRoundedCornerClip(static_cast<CFrameworkElement*>(this), testTarget);
                }
            }

            // If we successfully transformed the point/rect into local space, and it passed all clip checks, proceed
            // with hit testing against content and children.
            if (continueHitTest)
            {
                IFC_RETURN(BoundsTestContentAndChildren(testTarget, pCallback, &newParams, canHitDisabledElements, canHitInvisibleElements, &hitResult));
            }
        }
    }

    if (pResult != NULL)
    {
        *pResult = hitResult;
    }

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Handles walking the TransitionRoot, children, and content and determining
//      if any were hit.
//
//------------------------------------------------------------------------
template <typename HitType>
_Check_return_ HRESULT
CUIElement::BoundsTestContentAndChildren(
    _In_ const HitType& testTarget,
    _In_ CBoundedHitTestVisitor* pCallback,
    _In_opt_ const HitTestParams *hitTestParams,
    _In_ bool canHitDisabledElements,
    _In_ bool canHitInvisibleElements,
    _Out_opt_ BoundsWalkHitResult* pResult
    )
{
    BoundsWalkHitResult hitResult = BoundsWalkHitResult::Continue;
    auto core = GetContext();

    // LayoutTransitionElements will call this path directly on their target.
    // We never expect to hit-test something that isn't hit-test visible, enabled, etc, but we do explicitly allow
    // hit-testing elements that are LTE targets via this method only.
    ASSERT(IsEnabledAndVisibleForHitTest(canHitDisabledElements, canHitInvisibleElements));

    // Test any post-children content drawn by this element (above its children in z-order).
    // Both pre- and post-children content are included in the content inner bounds.
    if (core->InvisibleHitTestMode() || DoesRectIntersectHitType(m_contentInnerBounds, testTarget) || canHitInvisibleElements)
    {
        IFC_RETURN(pCallback->OnElementHit(this, testTarget, TRUE /*hitPostChildren*/, &hitResult));
    }

    // Use the pre-calculated child bounds for fast culling.
    // Ignore the bounds if we're hit-testing invisible elements.
    ASSERT(!AreChildBoundsDirty());
    if (core->InvisibleHitTestMode() || DoesRectIntersectHitType(m_childBounds, testTarget) || HasTransform3DInSubtree(hitTestParams) || canHitInvisibleElements)
    {
        if (flags_enum::is_set(hitResult, BoundsWalkHitResult::Continue))
        {
            // Any active LayoutTransitions in this element's TransitionRoot draw on top of its children, so test them first.
            CTransitionRoot* transitionRootNoRef = GetLocalTransitionRoot(false /*ensureTransitionRoot*/);
            if (transitionRootNoRef)
            {
                IFC_RETURN(transitionRootNoRef->BoundsTestInternal(testTarget, pCallback, hitTestParams, canHitDisabledElements, canHitInvisibleElements, &hitResult));
            }
        }

        if (flags_enum::is_set(hitResult, BoundsWalkHitResult::Continue))
        {
            // Children draw in front of the element's own content, so they need to be tested next.
            IFC_RETURN(BoundsTestChildren(testTarget, pCallback, hitTestParams, canHitDisabledElements, canHitInvisibleElements, &hitResult));
        }
    }

    // Check if the parent should be included - this means a child was hit.
    // This element (the parent) will be included whether its content would be hit or not.
    if (flags_enum::is_set(hitResult, BoundsWalkHitResult::IncludeParents))
    {
        IFC_RETURN(pCallback->OnParentIncluded(this, testTarget, &hitResult));
    }
    // Otherwise, if bounds testing should continue test the element's content last.
    else if (flags_enum::is_set(hitResult, BoundsWalkHitResult::Continue))
    {
        // Use the pre-calculated content bounds for fast culling.
        ASSERT(!AreContentInnerBoundsDirty());

        // Ignore the bounds if we're hit-testing invisible elements. Otherwise check if the
        // test is in the bounding rectangle before recording the hit element.
        if (core->InvisibleHitTestMode() || DoesRectIntersectHitType(m_contentInnerBounds, testTarget) || canHitInvisibleElements)
        {
            IFC_RETURN(pCallback->OnElementHit(this, testTarget, FALSE /*hitPostChildren*/, &hitResult));
        }
    }

    if (pResult != NULL)
    {
        *pResult = hitResult;
    }

    return S_OK;
}

// Apply UIElement.Clip to the given bounding rect
_Check_return_ HRESULT
CUIElement::ApplyUIEClipToBounds(
    _Inout_ XRECTF_RB *bounds
    )
{
    const auto& clip = GetHandOffVisualClipOrElementClip();
    if (clip != nullptr)
    {
        XRECTF_RB clipBounds = { };
        IFC_RETURN(clip->GetBounds(&clipBounds));
        IntersectRect(bounds, &clipBounds);
    }

    return S_OK;
}

// Apply the LayoutClip to the given bounding rect
_Check_return_ HRESULT
CUIElement::ApplyLayoutClipToBounds(
    _Inout_ XRECTF_RB *bounds
    )
{
    if (HasLayoutClip())
    {
        XRECTF_RB clipBounds = { };

        // Note that the layout clip coordinate space varies depending on various factors,
        // and may act either as a self clip, or an ancestor clip. See notes in LayoutStorage.h
        IFC_RETURN(LayoutClipGeometry->GetBounds(&clipBounds));
        IntersectRect(bounds, &clipBounds);
    }

    return S_OK;
}

// Transform a point/polygon from parent element space to inner space for the element.
bool CUIElement::TransformOuterToInner(
    _In_reads_(numPoints) const XPOINTF* pOuterPoints,
    _Out_writes_(numPoints) XPOINTF* pInnerPoints,
    XUINT32 numPoints)
{
    bool transformed = true;
    CMILMatrix transform;

    for (XUINT32 i = 0; i < numPoints; i++)
    {
        pInnerPoints[i] = pOuterPoints[i];
    }

    // Transform the point to the local space of the element.
    if (!HasTransform3DForHitTestingLegacy() && !GetLocalTransform(TransformRetrievalOptions::IncludePropertiesSetInComposition, &transform))
    {
        if (transform.Invert())
        {
            transform.Transform(pInnerPoints, pInnerPoints, numPoints);
        }
        else
        {
            // Transform is not invertible.
            transformed = false;
        }
    }

    // Transform the point through the current projection.
    if (transformed && HasActiveProjection())
    {
        CMILMatrix4x4 overallProjectionMatrix = GetProjection()->GetOverallMatrix();
        ASSERT(!overallProjectionMatrix.IsIdentity());  // HasActiveProjection guarantees this
        const gsl::span<XPOINTF> points(pInnerPoints, numPoints);
        transformed = overallProjectionMatrix.TransformWorldToLocalWithInterpolation(points, points);
    }

    return transformed;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Transform points from parent element space to child element space
//      along an ancestor chain.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElement::TransformOuterToInnerChain(
    _In_ CUIElement* pDescendantElement,
    _In_opt_ const CUIElement* pAncestorElement,
    _In_ const XPOINTF* pOuterPoints,
    _Out_ XPOINTF* pInnerPoints,
    XUINT32 numPoints,
    _Out_ bool* pTransformedPoints)
{
    xvector<CUIElement*> parentChain;
    bool transformed = true;

    for (CUIElement* pElement = pDescendantElement;
        pElement != pAncestorElement && pElement != NULL;
        pElement = pElement->GetUIElementAdjustedParentInternal(FALSE))
    {
        IFC_RETURN(parentChain.push_back(pElement));
    }

    for (XUINT32 i = 0; i < numPoints; i++)
    {
        pInnerPoints[i] = pOuterPoints[i];
    }

    for (xvector<CUIElement*>::reverse_iterator it = parentChain.rbegin(); it != parentChain.rend() && transformed; ++it)
    {
        CUIElement* pElement = (*it);

        transformed = pElement->TransformOuterToInner(pInnerPoints, pInnerPoints, numPoints);
    }

    *pTransformedPoints = transformed;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Transform a rect from inner space to outer space for
//      the element.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CUIElement::TransformInnerToOuter(
    _In_ const XRECTF_RB* pInnerBounds,
    _Out_ XRECTF_RB* pOuterBounds,
    _In_ bool ignoreClipping
    )
{
    CMILMatrix localTransform;
    XRECTF_RB transformedBounds = { };

    transformedBounds = *pInnerBounds;

    // TODO: properly handle clips on 3D elements.
    if (!ignoreClipping)
    {
        IFC_RETURN(ApplyUIEClipToBounds(&transformedBounds));

        if (!ShouldApplyLayoutClipAsAncestorClip())
        {
            IFC_RETURN(ApplyLayoutClipToBounds(&transformedBounds));
        }
    }

    // Transform bounds through the current projection.
    if (HasActiveProjection())
    {
        // TODO: HitTest: VSIS uses this to calculate visible bounds. In that case maybe we do want the fresh matrix
        //      rather than what rendered last frame.

        XSIZEF elementSize;
        XRECTF_RB quadPadding;
        bool areAllWValuesPositive;

        // Note: We intentionally not pass ignoreClipping into GetElementSizeForProjection. That flag is used here to
        // transform a set of bounds from inside a popup or a layout transition element up to the root. Since popups
        // and LTEs don't inherit the clip from their ancestors, we ignore clipping on the way up. However, the
        // projections up to the ancestors should still be respected. The popup/LTE should have the same perspective
        // as the other children of the projection, which means the origin of the 3D transforms needs to be the same.
        // The origin is determined by elementSize, which is in turn determined by the transformed bounds. The other
        // children use clipped bounds to position the origin, which means that the popup/LTE also need to use the
        // same clipped bounds to position the origin, even if ignoreClipping is TRUE.
        IFC_RETURN(GetElementSizeForProjection(&elementSize));

        NWComputeProjectionQuadPadding(
            XcpFloor(transformedBounds.left),
            XcpFloor(transformedBounds.top),
            XcpCeiling(transformedBounds.right) - XcpFloor(transformedBounds.left),
            XcpCeiling(transformedBounds.bottom) - XcpFloor(transformedBounds.top),
            &elementSize,
            &quadPadding
            );

        IFC_RETURN(GetProjection()->GetContentBounds(elementSize, quadPadding, &transformedBounds, &areAllWValuesPositive));

        // If a projection produces negative W values (e.g. a PlaneProjection that crosses the near plane, or a strange
        // Matrix3DProjection), then the winding order of the polygon can become a figure 8. In these cases we should
        // abort hit testing. Return an empty set of bounds so that we fail the bounds check.
        if (!areAllWValuesPositive)
        {
            EmptyRectF(&transformedBounds);
        }
    }

    // Apply non-depth 3D transforms (scale, translate, etc) - they are effectively 2D transforms.
    if (HasTransform3DForHitTestingLegacy() && !HasDepthLegacy())
    {
        CMILMatrix4x4 transform3DMatrix = GetHitTestingTransform3DMatrix();
        CMILMatrix temp = transform3DMatrix.Get2DRepresentation();

        XRECTF bounds = ToXRectF(transformedBounds);
        XRECTF localTransformedBounds = { };

        temp.TransformBounds(&bounds, &localTransformedBounds);

        transformedBounds = ToXRectFRB(localTransformedBounds);
    }

    // Transform bounds by local transform.
    TransformRetrievalOptions options = TransformRetrievalOptions::IncludePropertiesSetInComposition;
    if (!GetLocalTransform(options, &localTransform))
    {
        XRECTF bounds = ToXRectF(transformedBounds);
        XRECTF localTransformedBounds = { };

        localTransform.TransformBounds(&bounds, &localTransformedBounds);

        transformedBounds = ToXRectFRB(localTransformedBounds);
    }

    if (!ignoreClipping && ShouldApplyLayoutClipAsAncestorClip())
    {
        IFC_RETURN(ApplyLayoutClipToBounds(&transformedBounds));
    }

    *pOuterBounds = transformedBounds;

    return S_OK;
}

// Determines if any elements in provided ancestor tree is a descendant of a 3D'd element with depth.
// This check is inclusive of the descendant and exclusive of the ancestor.
bool CUIElement::IsInTransform3DSubtree(
    _In_ CUIElement* pDescendantElement,
    _In_opt_ const CUIElement* pAncestorElement
    )
{
    for (CUIElement* pElement = pDescendantElement;
        pElement != pAncestorElement && pElement != nullptr;
        pElement = pElement->GetUIElementAdjustedParentInternal(FALSE))
    {
        if (pElement->HasDepthLegacy())
        {
            return true;
        }
    }
    return false;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Transform a point from element space to parent element space
//      along an ancestor chain.
//
//------------------------------------------------------------------------
/* static */ _Check_return_ HRESULT
CUIElement::TransformInnerToOuterChain(
    _In_ CUIElement* pDescendantElement,
    _In_opt_ const CUIElement* pAncestorElement,
    _In_ const XRECTF_RB* pInnerRect,
    _Out_ XRECTF_RB* pOuterRect
    )
{
    XRECTF_RB targetRect = { };

    targetRect = *pInnerRect;

    // This chain is inclusive of the descendant and exclusive of the ancestor.
    for (CUIElement* pElement = pDescendantElement;
        pElement != pAncestorElement && pElement != nullptr;
        pElement = pElement->GetUIElementAdjustedParentInternal(false))
    {
        if (pElement->OfTypeByIndex<KnownTypeIndex::XamlIslandRoot>())
        {
            // A XamlIslandRoot knows its accumulated scale.  Once we have this scale, we just use it, no
            // need to keep walking up the parent chain.
            const float islandScale = RootScale::GetRasterizationScaleForElement(pElement);
            ScaleRect(targetRect, islandScale);
            break;
        }

        pElement = TransformInnerToOuterChainThrough3DSubtree(
            pElement,
            pAncestorElement,
            &targetRect,
            &targetRect
            );

        // TransformInnerToOuterChainThrough3DSubtree went to the root or ancestor element (perhaps because of a Tx3D without a PTx3D above it)
        // Since we traversed all the way up to the root/ancestor and didn't find a perspective
        // transform3D, the bounds we have are what we should return.
        if (pElement == nullptr || pElement == pAncestorElement)
        {
            break;
        }

        // Workaround for the stickyheader case: the first item is clipped, unless we don't apply the parent's clip,
        // and instead redirect through the LTE.
        // If the request is for target information, we sill skip the LTE and work our way through the tree as if
        // we were not hosted in an LTE.  This will give us the natural position of the element rather than where
        // it happens to be in the animation.  Note also, this doesn't work for things like sticky headers were
        // the LTE isn't being used for animation, but instead being used for positioning (and escaping the clip).
        // However, we don't expect connected animation to be used in those scenarios (vs. navigation transitions).
        // A more holistic solution to this problem (of natural position) should be looked at at some point.
        if (pElement->IsHiddenForLayoutTransition())
        {
            auto layoutElements = pElement->GetLayoutTransitionElements();
            XUINT32 size = layoutElements->size();
            // Note: if size != 1, then we're not handling this correctly. The element is showing up in
            // multiple different places, so it's non-trivial to pass through the LTEs.
            if (size == 1)
            {
                CLayoutTransitionElement* pLTE = nullptr;
                IFC_RETURN(layoutElements->get_item(0, pLTE));
                ASSERT(pLTE != NULL);

                IFC_RETURN(pLTE->TransformInnerToOuter(&targetRect, &targetRect, true /* ignoreClipping */));
            }
        }
        else
        {
            // If "this" was the target of an LTE, then we ignore the transforms on "this".
            IFC_RETURN(pElement->TransformInnerToOuter(&targetRect, &targetRect, true /* ignoreClipping */));
        }
    }

    *pOuterRect = targetRect;

    return S_OK;
}

CUIElement* CUIElement::TransformInnerToOuterChainThrough3DSubtree(
    _Inout_ CUIElement* pDescendantElement,
    _In_opt_ const CUIElement* pAncestorElement,
    _In_ const XRECTF_RB* pInnerRect,
    _Out_ XRECTF_RB* pOuterRect
    )
{
    CUIElement* pElement = pDescendantElement;
    XRECTF_RB targetRect = { };
    HitTestPerfData hitTestPerfData;
    HitTestParams hitTestParams(&hitTestPerfData);

    targetRect = *pInnerRect;

    // TODO: [https://task.ms/4459458: HasDepth flag does not properly propagate up through LTEs]
    // Currently, we don't properly propagate HasDepthInSubtree through LTEs, meaning that if we
    // attempt to collect transforms up to the perspective, we won't properly know if the LTE "has depth"
    // or not, leading to miscalculation in the LTE case.
    if (!pElement->Has3DDepthInSubtree())
    {
        // If we're requesting a 2D element who has !HasDepthInSubtree and whose immediate parent has depth or depth in subtree,
        // then we need to transform the 2D bounds through the 3D subtree, and set pElement to be the perspective (and continue walking up)
        const CUIElement* pImmediateParent = pElement->GetUIElementParentInternal(false);
        if (pImmediateParent != nullptr)
        {
            if (pImmediateParent->HasDepthLegacy() || pImmediateParent->Has3DDepthInSubtree())
            {
                // Gather up the transforms from pElement to the perspective.
                pElement = GatherTransformsUpTreeAndMoveToPerspectiveAncestor(pElement, pAncestorElement, &hitTestParams);

                XRECTF_RB selfBounds = { };
                hitTestParams.TransformBounds(targetRect, &selfBounds);

                targetRect = selfBounds;
            }
        }
    }
    else if (pElement->HasDepthLegacy())
    {
        // If this has depth, then we've already calculated our outer bounds correctly relative to the nearest perspective.
        // Since we're relative to the nearest perspective (including the transforms on the perspective element itself)
        // we need to jump to the perspective's parent.
        // Note: There exists an edge case that starts at a node in a 3D subtree and goes to another node in the same 3D subtree.
        //       In this case, we can't go up to the Perspective, because that is too high. Instead, we'll return the Outer bounds
        //       of the first element, go up to the ancestor 3D element, and pass these bounds through TransformInnerToOuter once before
        //       finishing. This will double-count any 2D transforms on the ancestor node. Since we only expect Transform3D calls in
        //       TransformInnerToOuterChain during GetGlobalBounds (which goes up to the root) this edge-case is a non-issue for now.
        IFCFAILFAST(pElement->GetOuterBounds(&hitTestParams, &targetRect));
    }

    *pOuterRect = targetRect;

    return pElement;
}

CUIElement* CUIElement::GatherTransformsUpTreeAndMoveToPerspectiveAncestor(
    _In_ CUIElement* pDescendantElement,
    _In_opt_ const CUIElement* pAncestorElement,
    _Inout_ HitTestParams* hitTestParams
    )
{
    CUIElement* pElement = pDescendantElement;
    // Gather up the transforms from pElement to the perspective.
    // Note: if we hit the ancestor before we hit a perspective (ie. the ancestor is in a 3D subtree below the PTx3D but above descendant),
    //       we will collect the transforms from descendant through to ancestor, jump to the ancestor, and continue from there. This means
    //       that we will skip the transforms between the ancestor to its nearest perspective, resulting in incorrect hitTestParams. We can't avoid
    //       returning the wrong thing in this case (since this method isn't smart enough to know to go to the perspective even if it is past the ancestor),
    //       but since we assume that our only caller for now is GetGlobalBounds (and goes all the way to the root), this case isn't an issue for now.
    for (; pElement != pAncestorElement && pElement != nullptr;
           pElement = pElement->GetUIElementAdjustedParentInternal(false))
    {
        // Populate the builder if there is a 3D transform.
        if (pElement->HasTransform3DForHitTestingLegacy())
        {
            CMILMatrix4x4 transform3DMatrix = pElement->GetHitTestingTransform3DMatrix();
            // Note that since we're going up the tree, we use a modified UpdateCombinedTransformMatrix that appends rather than prepends.
            hitTestParams->AppendTransformMatrix(&transform3DMatrix);
        }

        // Populate the builder with any 2D transforms.
        CMILMatrix local2DTransform;
        if (!pElement->GetLocalTransform(TransformRetrievalOptions::IncludePropertiesSetInComposition, &local2DTransform))
        {
            hitTestParams->AppendTransformMatrix(&local2DTransform); // Apply the local transform in context of this element's transform3D.
        }
    }

    return pElement;
}

// Get bounds of the element in global space.
_Check_return_ HRESULT CUIElement::GetGlobalBounds(_Out_ XRECTF_RB* pBounds)
{
    return GetGlobalBoundsWithOptions(
        pBounds,
        false /* ignoreClipping */,
        false /* ignoreClippingOnScrollContentPresenters */,
        false /* useTargetInformation */
        );
}

// Get bounds of the element in global space.
_Check_return_ HRESULT CUIElement::GetGlobalBounds(_Out_ XRECTF_RB* pBounds, const bool ignoreClipping)
{
    return GetGlobalBoundsWithOptions(
        pBounds,
        ignoreClipping,
        false /* ignoreClippingOnScrollContentPresenters */,
        false /* useTargetInformation */
        );
}

// Get bounds of the element in global space.
_Check_return_ HRESULT
CUIElement::GetGlobalBoundsWithOptions(
    _Out_ XRECTF_RB* pBounds,
    _In_ bool ignoreClipping,
    const bool ignoreClippingOnScrollContentPresenters,
    _In_ bool useTargetInformation
    )
{
    IFC_RETURN(GetGlobalBoundsOnSelf(pBounds, ignoreClipping, ignoreClippingOnScrollContentPresenters, useTargetInformation));

    if (Has3DDepthInSubtree())
    {
        // Elements with 3D depth don't contribute to the outer bounds of their parent elements, so their bounds
        // are currently missing. Explicitly walk down the 3D branches and get their global bounds.
        XRECTF_RB subtreeGlobalBounds = { };
        IFC_RETURN(GetGlobalBoundsIn3DDescendants(&subtreeGlobalBounds, ignoreClipping, ignoreClippingOnScrollContentPresenters, useTargetInformation));
        UnionRectF(pBounds, &subtreeGlobalBounds);
    }

    return S_OK;
}

// Get bounds of the element in global space.
_Check_return_ HRESULT
CUIElement::GetGlobalBoundsOnSelf(
    _Out_ XRECTF_RB* pBounds,
    _In_ bool ignoreClipping,
    const bool ignoreClippingOnScrollContentPresenters,
    _In_ bool useTargetInformation  // Attempt to use target values for animations (e.g. manipulations/LTEs) instead of current values
    )
{
    XRECTF_RB innerBounds = { };

    {
        HitTestPerfData hitTestPerfData;
        HitTestParams hitTestParams(&hitTestPerfData);

        // Ensure that the entire tree has the correct bounds before querying for outer bounds.
        // In the 3D case, a partial walk from an element in a 3D subtree would return
        // incorrect bounds unless the entire tree is traversed up to the root.
        CRootVisual *pRootVisual = VisualTree::GetRootForElement(this);
        IFC_RETURN(pRootVisual->EnsureOuterBounds(&hitTestParams));

        IFC_RETURN(GetInnerBounds(&hitTestParams, &innerBounds));

        IFC_RETURN(TransformToWorldSpace(&innerBounds, pBounds, ignoreClipping, ignoreClippingOnScrollContentPresenters, useTargetInformation));
    }

    return S_OK;
}

_Check_return_ HRESULT CUIElement::GetGlobalBoundsIn3DDescendants(
    _Out_ XRECTF_RB* pBounds,
    _In_ bool ignoreClipping,
    const bool ignoreClippingOnScrollContentPresenters,
    _In_ bool useTargetInformation  // Attempt to use target values for animations (e.g. manipulations/LTEs) instead of current values
    )
{
    ASSERT(Has3DDepthInSubtree());

    // Don't look at this element's own bounds. They should have been taken care of already.

    XRECTF_RB combinedGlobalBounds = { };

    // If there are any children with 3D depth, make sure we walk to them. This happens regardless of whether this element
    // itself has 3D depth.
    if (m_pChildren != nullptr)
    {
        for (CDependencyObject* childDO : *m_pChildren)
        {
            CUIElement* child = static_cast<CUIElement*>(childDO);
            XRECTF_RB childGlobalBounds = { };

            // If the child has 3D depth, collect its bounds. Call the non-recursive method so we don't end up in a loop
            // when we encounter an element with 3D depth on both itself and its subtree.
            if (child->Has3DDepth())
            {
                IFC_RETURN(child->GetGlobalBoundsOnSelf(&childGlobalBounds, ignoreClipping, ignoreClippingOnScrollContentPresenters, useTargetInformation));
                UnionRectF(&combinedGlobalBounds, &childGlobalBounds);
            }

            if (child->Has3DDepthInSubtree())
            {
                IFC_RETURN(child->GetGlobalBoundsIn3DDescendants(&childGlobalBounds, ignoreClipping, ignoreClippingOnScrollContentPresenters, useTargetInformation));
                UnionRectF(&combinedGlobalBounds, &childGlobalBounds);
            }
        }
    }

    *pBounds = combinedGlobalBounds;

    return S_OK;
}

_Check_return_ HRESULT
CUIElement::GetGlobalBoundsLogical(
    _Out_ XRECTF_RB* pBounds,
    _In_ bool ignoreClipping,
    _In_ bool useTargetInformation)
{
    IFC_RETURN(GetGlobalBoundsWithOptions(pBounds, ignoreClipping, false /* ignoreClippingOnScrollContentPresenters */, useTargetInformation));

    // GetGlobalBounds will include the scales of all the elements up the tree.
    // To transform these coordinates to logical space, we apply the inverse scale of whatever root
    // it's connected to (whether island or root visual)
    const float scaleFactor = RootScale::GetRasterizationScaleForElement(this);
    ScaleRect(*pBounds, 1.0f / scaleFactor);

    return S_OK;
}

_Check_return_ HRESULT CUIElement::GetTightGlobalBounds(
        _Out_ XRECTF_RB* pBounds,
        _In_ bool ignoreClipping,
        _In_ bool useTargetInformation  // Attempt to use target values for animations (e.g. manipulations/LTEs) instead of current values
        )
{
    IFC_RETURN(GetGlobalBoundsWithOptions(pBounds, ignoreClipping, false /* ignoreClippingOnScrollContentPresenters */, useTargetInformation));
    return S_OK;
}

_Check_return_ HRESULT CUIElement::GetTightInnerBounds(
    _Out_ XRECTF_RB* pBounds
    )
{
    IFC_RETURN(GetInnerBounds(pBounds));
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Walk the children of the element finding elements that intersect
//      with the given point.
//
//  NOTE:
//      Uses reverse render order (front to back).
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElement::BoundsTestChildren(
    _In_ const XPOINTF& target,
    _In_ CBoundedHitTestVisitor* pCallback,
    _In_opt_ const HitTestParams *hitTestParams,
    _In_ bool canHitDisabledElements,
    _In_ bool canHitInvisibleElements,
    _Out_opt_ BoundsWalkHitResult* pResult
    )
{
    RRETURN(BoundsTestChildrenImpl(target, pCallback, hitTestParams, canHitDisabledElements, canHitInvisibleElements, pResult));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Walk the children of the element finding elements that intersect
//      with the given polygon.
//
//  NOTE:
//      Uses reverse render order (front to back).
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElement::BoundsTestChildren(
    _In_ const HitTestPolygon& target,
    _In_ CBoundedHitTestVisitor* pCallback,
    _In_opt_ const HitTestParams *hitTestParams,
    _In_ bool canHitDisabledElements,
    _In_ bool canHitInvisibleElements,
    _Out_opt_ BoundsWalkHitResult* pResult
    )
{
    RRETURN(BoundsTestChildrenImpl(target, pCallback, hitTestParams, canHitDisabledElements, canHitInvisibleElements, pResult));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Walk the children of the element finding elements that intersect
//      with the given point/polygon.
//
//  NOTE:
//      Uses reverse render order (front to back).
//
//------------------------------------------------------------------------
template <typename HitType>
_Check_return_ HRESULT CUIElement::BoundsTestChildrenImpl(
    _In_ const HitType& target,
    _In_ CBoundedHitTestVisitor* pCallback,
    _In_opt_ const HitTestParams *hitTestParams,
    _In_ bool canHitDisabledElements,
    _In_ bool canHitInvisibleElements,
    _Out_opt_ BoundsWalkHitResult* pResult
    )
{
    BoundsWalkHitResult hitResult = BoundsWalkHitResult::Continue;
    BoundsWalkHitResult childHitResult = BoundsWalkHitResult::Continue;

    XUINT32 childCount = 0;
    CUIElement** ppUIElements = nullptr;

    GetChildrenInRenderOrder(&ppUIElements, &childCount);

    // Test bounds in reverse render order (front to back).
    for (XUINT32 i = childCount; i > 0 && flags_enum::is_set(childHitResult, BoundsWalkHitResult::Continue); --i)
    {
        // Workaround for a crash in a scenario where items are removed while ListView reordering is in progress.
        // If element leaves the tree while it is being dragged, pointer capture loss event is fired which will trigger bounds walk.
        // If pointer was over the element leaving the tree it will be tested and since it was replaced with null
        // in children collection to prevent reentrancy (CDOCollection::Neat), the pointer can be null and this case needs to be guarded.
        // Bounds check can be skipped since it is called via CCollection::Destroy().
        if (ppUIElements[i - 1])
        {
            IFC_RETURN(ppUIElements[i - 1]->BoundsTestInternal(target, pCallback, hitTestParams, canHitDisabledElements, canHitInvisibleElements, &childHitResult));

            // If any child wanted to include its parent chain, copy the flag.
            if (flags_enum::is_set(childHitResult, BoundsWalkHitResult::IncludeParents))
            {
                hitResult = flags_enum::set(hitResult, BoundsWalkHitResult::IncludeParents);
            }
        }
    }

    // If the child element wanted the bounds walk to stop, remove the continue flag
    // from the result.
    if (!flags_enum::is_set(childHitResult, BoundsWalkHitResult::Continue))
    {
        hitResult = flags_enum::unset(hitResult, BoundsWalkHitResult::Continue);
    }

    if (pResult)
    {
        *pResult = hitResult;
    }

    return S_OK;
}

// Helper function to compute the visible bounds of a rect provided in the local coordinate space of this element
_Check_return_ HRESULT
CUIElement::ComputeVisibleBoundsOfRect(
    _In_ const XRECTF_RB* pRect,                    // The starting rect, in local coordinates
    _In_ const XRECTF_RB* pWindowBounds,            // The bounds of the window
    _Out_writes_all_(numPoints) XPOINTF* pPoints,   // Receives the resulting visible bounds
    UINT numPoints                                  // #points in pPoints array
    )
{
    XRECTF_RB globalBounds;
    bool didTransformPoints;
    CUIElement* pElementWithCachedBounds = nullptr;
    VisibleBoundsMap* pVisibleBoundsMap = GetContext()->GetVisibleBoundsMap();

    ASSERT(numPoints == 4);

    // Compute the clipped bounds of the UIElement in world space
    globalBounds = *pRect;

    for (CUIElement* pElement = this; pElement != NULL; pElement = pElement->GetUIElementAdjustedParentInternal(FALSE))
    {
        // Although currently this function is only ever used in the context of CCoreServices::VirtualSurfaceImageSourcePerFrameWork,
        // which always provides pVisibleBoundsMap, doing a null check here anyway since this function is general-purpose.
        if (pVisibleBoundsMap != nullptr)
        {
            VisibleBoundsMap::iterator itFind = pVisibleBoundsMap->find(pElement);
            if (itFind != pVisibleBoundsMap->end())
            {
                pElementWithCachedBounds = pElement;
                XRECTF_RB cachedBounds_RB = ToXRectFRB(itFind->second);
                IntersectRect(&globalBounds, &cachedBounds_RB);
                break;
            }
        }
        bool isCollapsed = !pElement->IsVisible();
        bool isTransparent = (0.0f == pElement->GetOpacityCombined()) && !pElement->IsOpacityIndependentlyAnimating() && !pElement->HasWUCOpacityAnimation();

        if (isCollapsed || isTransparent)
        {
            // If any parent element collapsed, transparent (without animating opacity), or is a resource dictionary
            // then the global bounds of this element is empty
            EmptyRectF(&globalBounds);
            break;
        }
        else
        {
            IFC_RETURN(pElement->TransformInnerToOuter(&globalBounds, &globalBounds, FALSE /*ignoreClipping*/));
        }
    }

   // If we didn't find a cached result for this element, do these extra pieces of work.
   // If we go on to cache the result for this element, these will be incorporated in the cached bounds.
   if (pElementWithCachedBounds == nullptr)
   {
        // It is possible that this UI element is inside of a resource dictionary
        // in which case, it contributes nothing to the visible bounds
        // Walk up the tree looking for resource dictionaries
        for (CDependencyObject *pDO = this; pDO != NULL; pDO = pDO->GetParentInternal(false))
        {
            if (pDO->OfTypeByIndex<KnownTypeIndex::ResourceDictionary>())
            {
                EmptyRectF(&globalBounds);
                break;
            }
        }

        // Clip the global bounds to the bounds of the window
        IntersectRect(&globalBounds, pWindowBounds);
   }

    // Transform the global bounds into the space of this UI element
    FillPointsFromRectCCW(pPoints, ToXRectF(globalBounds));

    IFC_RETURN(TransformOuterToInnerChain(
        this,
        pElementWithCachedBounds,
        pPoints,
        pPoints,
        numPoints,
        &didTransformPoints));

    // It is possible for TransformOuterToInnerChain to not transform points
    // because of an uninvertible transform.  In that case, return an empty rect.
    if (!didTransformPoints)
    {
        EmptyRectF(&globalBounds);
        FillPointsFromRectCCW(pPoints, ToXRectF(globalBounds));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the bounds of the visible region of the specified image brush.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElement::GetVisibleImageBrushBounds(
    _In_ CImageBrush *pImageBrush,
    _In_ const XRECTF_RB *pWindowBounds,
    _Out_ XRECTF *pBounds
    )
{
    XRECTF_RB imageSourceBounds_RB;
    CMILMatrix imageSourceToUIElement;
    CMILMatrix elementToImageSource;
    XRECTF naturalImageBrushBounds;
    XPOINTF points[4];
    CMILMatrix identityTransform(TRUE);

    // Compute the bounds of the element in element space
    XRECTF_WH elementBounds_WH;
    IFC_RETURN(GetBoundsForImageBrushVirtual(pImageBrush, &elementBounds_WH));

    XRECTF_RB elementBounds_RB = ToXRectFRB(elementBounds_WH);

    // Compute the clipped bounds of the UIElement
    IFC_RETURN(ComputeVisibleBoundsOfRect(&elementBounds_RB, pWindowBounds, points, ARRAYSIZE(points)));

    // It is legal for a VSIS to have zero area (which will cause ComputeDeviceToSource to fail).
    // Also, it is possible we'll get an empty rect back from ComputeVisibleBoundsOfRect.
    // In both cases, return an empty rect.
    pImageBrush->GetNaturalBounds(&naturalImageBrushBounds);

    XRECTF clippedBounds;
    BoundPoints(
        points,
        ARRAY_SIZE(points),
        &clippedBounds
        );

    if (IsEmptyRectF(clippedBounds) || IsEmptyRectF(naturalImageBrushBounds))
    {
        EmptyRectF(&imageSourceBounds_RB);
    }
    else
    {
        // Compute the ImageSource->UIElement transform
        IFC_RETURN(pImageBrush->ComputeDeviceToSource(
            &identityTransform,
            &elementBounds_WH,
            &imageSourceToUIElement
            ));

        // Compute the UIElement->ImageSource transform
        elementToImageSource = imageSourceToUIElement;
        if (elementToImageSource.Invert())
        {
            // Transform the points in UIElement space to ImageSource space
            elementToImageSource.Transform(
                points,
                points,
                ARRAY_SIZE(points)
                );

            // Compute the bounds of the points (in ImageSource space)
            XRECTF imageSourceBounds_WH;

            BoundPoints(
                points,
                ARRAY_SIZE(points),
                &imageSourceBounds_WH
                );

            imageSourceBounds_RB = ToXRectFRB(imageSourceBounds_WH);
        }
        else
        {
            // Assume that nothing is visible if the ImageSource->UIElement transform is not invertible.
            EmptyRectF(&imageSourceBounds_RB);
        }
    }

    *pBounds = ToXRectF(imageSourceBounds_RB);

    return S_OK;
}

// Compute and cache the visible bounds of this element
_Check_return_ HRESULT
CUIElement::CacheVisibleBounds(
    _In_ const XRECTF_RB *pWindowBounds
    )
{
    XPOINTF points[4];
    XRECTF elementBounds;

    // This function is only ever called in the context of CCoreServices::VirtualSurfaceImageSourcePerFrameWork,
    // which is responsible for setting a valid VisibleBoundsMap pointer on the Core.
    VisibleBoundsMap* pVisibleBoundsMap = GetContext()->GetVisibleBoundsMap();
    ASSERT(pVisibleBoundsMap != nullptr);

    if (pVisibleBoundsMap->find(this) == pVisibleBoundsMap->end())
    {
        // This function relies on the combined inner bounds to produce correct results.
        // Currently, the combined inner bounds cannot be correctly transformed up to the root because
        // the child bounds contribution to inner bounds takes 3D into account but the content bounds does not.
        // For now, detect this case and disable the optimization.  In the future we can handle this case by
        // performing 2 transforms, one for content bounds, one for child bounds, and combining the results.
        if (!IsInTransform3DSubtree(this, nullptr))
        {
            // Compute the clipped bounds of the UIElement's combined inner bounds.
            // Here we're using the last known combined inner bounds.  These might change on the next layout pass.
            XRECTF_RB combinedInnerBounds;
            IFC_RETURN(GetInnerBounds(&combinedInnerBounds));
            IFC_RETURN(ComputeVisibleBoundsOfRect(&combinedInnerBounds, pWindowBounds, points, ARRAYSIZE(points)));

            // Convert the points back into a rect.
            BoundPoints(
                points,
                ARRAY_SIZE(points),
                &elementBounds
                );
            pVisibleBoundsMap->insert(std::make_pair(this, elementBounds));
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Default implementation of GetBoundsForImageBrushVirtual.  This returns
//      the bounds used to compute the brush transform for the specified image brush.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElement::GetBoundsForImageBrushVirtual(
    _In_ const CImageBrush *,
    _Out_ XRECTF *pBounds
    )
{
    pBounds->X = 0.0f;
    pBounds->Y = 0.0f;
    pBounds->Width = GetActualWidth();
    pBounds->Height = GetActualHeight();

    RRETURN(S_OK);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Common entry point for local space hit testing.
//
//------------------------------------------------------------------------
template <typename HitType>
_Check_return_ HRESULT
CUIElement::HitTestLocal(
    _In_ const HitType& target,
    _Out_ bool* pHit
    )
{
    IFC_RETURN(HitTestLocalInternal(target, pHit));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Test if a point intersects with the element in local space.
//
//  NOTE:
//      Overridden in derived classes to provide more detailed hit testing.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElement::HitTestLocalInternal(
    _In_ const XPOINTF& target,
    _Out_ bool* pHit
    )
{
    HRESULT hr = S_OK;

    // Various uielements don't implement HitTestLocalInternal and rely on the base implementation.
    // Most of these require a very basic hit test algorithm - hit against the bounds.
    // Examples include ItemsPresenter, Page and UserControl. These don't hit by default, but
    // we will want them in InvisibleHitTestMode.

    if (GetContext()->InvisibleHitTestMode())
    {
         XRECTF rc;
         rc.X = 0.0f;
         rc.Y = 0.0f;
         rc.Width = GetActualWidth();
         rc.Height = GetActualHeight();
        *pHit = DoesRectContainPoint(rc, target);
    }
    else
    {
        *pHit = FALSE;
    }

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Common entry point for local space hit testing of content that's
//      rendered post-children (on top of, in z-order).
//
//------------------------------------------------------------------------
template <typename HitType>
_Check_return_ HRESULT
CUIElement::HitTestLocalPostChildren(
    _In_ const HitType& target,
    _Out_ bool* pHit
    )
{
    if(OfTypeByIndex<KnownTypeIndex::ListViewBaseItemPresenter>())
    {
        IFC_RETURN(static_cast<CListViewBaseItemChrome*>(this)->HitTestLocalInternalPostChildren(target, pHit));
    }
    else
    {
        *pHit = FALSE;
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Cancel any pending transitions and delete the layout transition
//      storage.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElement::DeleteLayoutTransitionStorage()
{
    if (m_pLayoutTransitionStorage)
    {
        m_pLayoutTransitionStorage->UnregisterElementForTransitions(this);
        IFC_RETURN(m_pLayoutTransitionStorage->CleanupBrushRepresentations(this));
    }

    // Will cleanup storyboards in ~LayoutTransitionStorage.
    SAFE_DELETE(m_pLayoutTransitionStorage);
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Walks the children collection to propagate inherited property changes
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CUIElement::MarkInheritedPropertyDirty(
    _In_ const CDependencyProperty* pdp,
    _In_ const CValue* pValue)
{
    // We use DependencyObject::NotifyOfDataContextChange to propagate DataContext changes.
    if (pdp->GetIndex() != KnownPropertyIndex::FrameworkElement_DataContext)
    {
        if (pdp->GetIndex() == KnownPropertyIndex::FrameworkElement_FlowDirection)
        {
            NWSetTransformDirty(this, DirtyFlags::None);
        }

        CUIElementCollection* children = GetChildren();
        if (children != nullptr)
        {
            for (auto& child : *children)
            {
                CUIElement* childUIE = static_cast<CUIElement*>(child);
                if (childUIE != nullptr)
                {
                    // pdp is the property to use on this element. Determine the
                    // corresponding property for the child element.
                    // E.g. pdp might be KnownPropertyIndex::Control_FontSize, but if
                    // the child is a RichTextBox we'll actually need
                    // KnownPropertyIndex::RichTextBlock_FontSize.
                    const CDependencyProperty *pCorrespondingDp = InheritedProperties::GetCorrespondingInheritedProperty(
                        childUIE,
                        pdp
                        );

                    if (!pCorrespondingDp || childUIE->IsPropertyDefault(pCorrespondingDp))
                    {
                        // The property is not set locally on the child.
                        // i.e. the child does not block inheritance of this property.

                        if (pCorrespondingDp != NULL)
                        {
                            // Raise property change notifications only on elements
                            // that support the property.
                            IFC_RETURN(childUIE->NotifyPropertyChanged(PropertyChangedParams(pCorrespondingDp, CValue::Empty(), *pValue)));
                        }

                        // Recursively propagate the change to the child's own
                        // children.
                        IFC_RETURN(childUIE->MarkInheritedPropertyDirty(pdp, pValue));
                    }
                }
            }
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Notify element and its subtree that theme has changed
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
CUIElement::NotifyThemeChangedCore(_In_ Theming::Theme theme, _In_ bool fForceRefresh)
{
    // Set opacity dirty to ensure it is correct when having HighContrastAdjustment opacity overrides.
    CUIElement::NWSetOpacityDirty(this, DirtyFlags::Render);

    // Notify element's properties that theme has changed
    IFC_RETURN(CDependencyObject::NotifyThemeChangedCore(theme, fForceRefresh));

     // Recursively notify element subtree that theme has changed
    auto pChildren = GetChildren();
    if (pChildren)
    {
        UINT32 cChildren =  pChildren->GetCount();

        for (UINT32 i = 0; i < cChildren; i++)
        {
            xref_ptr<CUIElement> child;
            child.attach(static_cast<CUIElement*>(pChildren->GetItemWithAddRef(i)));
            IFCPTR_RETURN(child);

            IFC_RETURN(child->NotifyThemeChanged(theme, fForceRefresh));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CUIElement::NotifyApplicationHighContrastAdjustmentChanged()
{
    return S_OK;
}

_Check_return_ HRESULT
CUIElement::NotifyApplicationHighContrastAdjustmentChangedCore()
{
    // Recursively notify element subtree that the application's
    // high contrast adjustment has changed.
    auto pChildren = GetChildren();
    if (pChildren)
    {
        for (auto& child : *pChildren)
        {
            CUIElement* childUIE = static_cast<CUIElement*>(child);
            if (childUIE != nullptr)
            {
                if (childUIE->IsPropertyDefaultByIndex(KnownPropertyIndex::UIElement_HighContrastAdjustment))
                {
                    IFC_RETURN(childUIE->NotifyApplicationHighContrastAdjustmentChanged());
                }

                IFC_RETURN(childUIE->NotifyApplicationHighContrastAdjustmentChangedCore());
            }
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Test if a polygon intersects with the element in local space.
//
//  NOTE:
//      Overridden in derived classes to provide more detailed hit testing.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElement::HitTestLocalInternal(
    _In_ const HitTestPolygon& target,
    _Out_ bool* pHit
    )
{
    HRESULT hr = S_OK;

    // Various uielements don't implement HitTestLocalInternal and rely on the base implementation.
    // Most of these require a very basic hit test algorithm - hit against the bounds.
    // Examples include ItemsPresenter, Page and UserControl. These don't hit by default, but the
    // designer will want them in InvisibleHitTestMode.

    if (GetContext()->InvisibleHitTestMode())
    {
         XRECTF rc;
         rc.X = 0.0f;
         rc.Y = 0.0f;
         rc.Width = GetActualWidth();
         rc.Height = GetActualHeight();
        *pHit = target.IntersectsRect(rc);
    }
    else
    {
        *pHit = FALSE;
    }

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns whether or not the rect intersects the given point.
//
//------------------------------------------------------------------------
template <>
bool DoesRectIntersectHitType(_In_ const XRECTF& rect, _In_ const XPOINTF& target)
{
    return DoesRectContainPoint(rect, target);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns whether or not the rect intersects the given polygon.
//
//------------------------------------------------------------------------
template <>
bool DoesRectIntersectHitType(_In_ const XRECTF& rect, _In_ const HitTestPolygon& target)
{
    return target.IntersectsRect(rect);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns whether or not the rect intersects the given point.
//
//------------------------------------------------------------------------
template <>
bool DoesRectIntersectHitType(_In_ const XRECTF_RB& rect, _In_ const XPOINTF& target)
{
    return DoesRectContainPoint(rect, target);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns whether or not the rect intersects the given polygon.
//
//------------------------------------------------------------------------
template <>
bool DoesRectIntersectHitType(_In_ const XRECTF_RB& rect, _In_ const HitTestPolygon& target)
{
    return target.IntersectsRect(rect);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Clips the point to a rect.
//
//------------------------------------------------------------------------
template <>
_Check_return_ HRESULT
ClipHitTypeToRect(
    _Inout_ XPOINTF& target,
    _In_ const XRECTF& rect,
    _Out_ bool* pHit
    )
{
    // No additional work for points.
    *pHit = DoesRectContainPoint(rect, target);
    RRETURN(S_OK);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Clips the polygon to a rect.
//
//------------------------------------------------------------------------
template <>
_Check_return_ HRESULT
ClipHitTypeToRect(
    _Inout_ HitTestPolygon& target,
    _In_ const XRECTF& rect,
    _Out_ bool* pHit
    )
{
    if (target.IntersectsRect(rect))
    {
        IFC_RETURN(target.ClipToRect(rect));
        *pHit = !target.IsEmpty();
    }
    else
    {
        *pHit = FALSE;
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Clips the point to a rect.
//
//------------------------------------------------------------------------
template <>
_Check_return_ HRESULT
ClipHitTypeToRect(
    _Inout_ XPOINTF& target,
    _In_ const XRECTF_RB& rect,
    _Out_ bool* pHit
    )
{
    // No additional work for points.
    *pHit = DoesRectContainPoint(rect, target);
    RRETURN(S_OK);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Clips the polygon to a rect.
//
//------------------------------------------------------------------------
template <>
_Check_return_ HRESULT
ClipHitTypeToRect(
    _Inout_ HitTestPolygon& target,
    _In_ const XRECTF_RB& rect,
    _Out_ bool* pHit
    )
{
    RRETURN(ClipHitTypeToRect(target, ToXRectF(rect), pHit));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Applies a transform to the given point.
//
//------------------------------------------------------------------------
template <>
void
ApplyTransformToHitType(
    _In_ CMILMatrix* pTransform,
    _In_ const XPOINTF* pSrcTarget,
    _Out_ XPOINTF* pDestTarget
    )
{
    pTransform->Transform(pSrcTarget, pDestTarget, 1);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Applies a transform to the given polygon.
//
//------------------------------------------------------------------------
template <>
void
ApplyTransformToHitType(
    _In_ CMILMatrix* pTransform,
    _In_ const HitTestPolygon* pSrcTarget,
    _Out_ HitTestPolygon* pDestTarget
    )
{
    pSrcTarget->CopyTo(pDestTarget);
    pDestTarget->Transform(pTransform != nullptr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      (ctor) - Create a new bounding region visitor for hit testing.
//
//------------------------------------------------------------------------
CBoundedHitTestVisitor::CBoundedHitTestVisitor(
    _In_ CHitTestResults* pHitElements,
    bool hitMultiple
    )
    : m_pHitElements(pHitElements)
    , m_hitMultiple(hitMultiple)
{
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Handler for an elements bounding region intersecting with the
//      hit test point. Performs a detailed hit test on the element.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CBoundedHitTestVisitor::OnElementHit(
    _In_ CUIElement* pElement,
    _In_ const XPOINTF& target,
    bool hitPostChildren,
    _Out_ BoundsWalkHitResult* pResult
    )
{
    IFC_RETURN(OnElementHitImpl(pElement, target, hitPostChildren, pResult));
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Handler for an elements bounding region intersecting with the
//      hit test polygon. Performs a detailed hit test on the element.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CBoundedHitTestVisitor::OnElementHit(
    _In_ CUIElement* pElement,
    _In_ const HitTestPolygon& target,
    bool hitPostChildren,
    _Out_ BoundsWalkHitResult* pResult
    )
{
    IFC_RETURN(OnElementHitImpl(pElement, target, hitPostChildren, pResult));
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Handler for an elements bounding region intersecting with the
//      hit test point/polygon. Performs a detailed hit test on the element.
//
//------------------------------------------------------------------------
template <typename HitType>
_Check_return_ HRESULT CBoundedHitTestVisitor::OnElementHitImpl(
    _In_ CUIElement* pElement,
    _In_ const HitType& target,
    bool hitPostChildren,
    _Out_ BoundsWalkHitResult* pResult
    )
{
    bool hitTestResult = false;
    bool continueWalk = false;
    BoundsWalkHitResult result = BoundsWalkHitResult::Stop;

    if (hitPostChildren)
    {
        IFC_RETURN(pElement->HitTestLocalPostChildren(target, &hitTestResult));
    }
    else
    {
        IFC_RETURN(pElement->HitTestLocal(target, &hitTestResult));
    }

    if (hitTestResult)
    {
        IFC_RETURN(m_pHitElements->Add(pElement));

        continueWalk = !!m_hitMultiple;
    }
    else
    {
        continueWalk = true;
    }

    //
    // If the element wasn't hit or multiple elements should be included, continue
    // the bounds walk.
    //
    result = (!hitTestResult || m_hitMultiple) ? BoundsWalkHitResult::Continue : BoundsWalkHitResult::Stop;

    //
    // If the element was hit and multiple elements should be included, force the
    // parent to be included.
    //
    if (hitTestResult && m_hitMultiple)
    {
        result = flags_enum::set(result, BoundsWalkHitResult::IncludeParents);
    }

    *pResult = result;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Handler for an elements parent being included due to the
//      return value from OnElementHit.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CBoundedHitTestVisitor::OnParentIncluded(
    _In_ CUIElement* pElement,
    _In_ const XPOINTF& target,
    _Out_ BoundsWalkHitResult* pResult
    )
{
    IFC_RETURN(OnParentIncludedImpl(pElement, target, pResult));
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Handler for an elements parent being included due to the
//      return value from OnElementHit.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CBoundedHitTestVisitor::OnParentIncluded(
    _In_ CUIElement* pElement,
    _In_ const HitTestPolygon& target,
    _Out_ BoundsWalkHitResult* pResult
    )
{
    IFC_RETURN(OnParentIncludedImpl(pElement, target, pResult));
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Handler for an elements parent being included due to the
//      return value from OnElementHit.
//
//------------------------------------------------------------------------
template <typename HitType>
_Check_return_ HRESULT CBoundedHitTestVisitor::OnParentIncludedImpl(
    _In_ CUIElement* pElement,
    _In_ const HitType& target,
    _Out_ BoundsWalkHitResult* pResult
    )
{
    ASSERT(m_hitMultiple);

    IFC_RETURN(m_pHitElements->Add(pElement));

    //
    // If the parent element is being included, then multiple hit test elements
    // must have been requested and the bounds walk should continue. Parent
    // elements should also be included from this element.
    //
    *pResult = BoundsWalkHitResult::Continue | BoundsWalkHitResult::IncludeParents;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Process the PointerExit event if the pointer entered element change
//      the state by leaving tree, disabled or collapsed.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElement::ProcessPointerExitedEventByPointerEnteredElementStateChange()
{
    if (GetContext())
    {
        auto inputServices = GetContext()->GetInputServices();
        if (inputServices)
        {
            IFC_RETURN(inputServices->ProcessPointerExitedEventByPointerEnteredElementStateChange(this));
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Determines and returns a bool to indicate if
//      the subgraph is ready to be captured by
//      render target element in this frame.
//      Returning FALSE results in retrying the
//      entire operation.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CUIElement::IsReadyForRenderTargetElementRender(
    _Out_ BOOLEAN *pIsReadyForRenderTargetElementRender)
{
    HRESULT hr = S_OK;
    auto core = GetContext();

    *pIsReadyForRenderTargetElementRender = TRUE;

    if (core->IsInBackgroundTask() && AreInnerBoundsDirty())
    {
        // In case of background tasks the responsibility to
        // wait until the tree is ready is upon the framework.
        // If the inner bounds are dirty at this moment (this
        // could happen because of CCoreServices::UpdateDirtyState),
        // then we might have used the inner bounds of the element
        // too early. Hence return false so that we may redo
        // the calculations again.
        *pIsReadyForRenderTargetElementRender = FALSE;
    }

    RRETURN(hr); // RRETURN_REMOVAL
}

bool CUIElement::IsRightToLeft()
{
    if (m_isRightToLeftGeneration != GetContext()->m_cIsRightToLeftGenerationCounter)
    {
        EvaluateIsRightToLeft();
    }

    return !!m_isRightToLeft;
}

/* static */
XUINT32 CUIElement::IsSameSize(_In_ const XSIZEF& size1, _In_ const XSIZEF& size2)
{
    return ((IsInfiniteF(size1.height) && IsInfiniteF(size2.height)) || (size1.height == size2.height)) &&
           ((IsInfiniteF(size1.width) && IsInfiniteF(size2.width)) || (size1.width == size2.width));
}

_Check_return_ HRESULT CUIElement::IsOccluded(
    _In_ CUIElement *pChildElement,
    _In_ const XRECTF_RB& elementBounds,
    _Out_ bool* isOccluded)
{
    OcclusivityTester<CUIElement, CUIElement, CFrameworkElement> tester(this);
    return tester.Test(pChildElement, elementBounds, isOccluded);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates a viewport interaction for this manipulatable element to
//      support cross process chaining.
//
//------------------------------------------------------------------------
void CUIElement::GetViewportInteraction(
    _In_ DCompTreeHost * dcompTreeHost,
    _Out_ IInspectable** interaction)
{
    ASSERT(IsManipulatable() || OfTypeByIndex<KnownTypeIndex::RootVisual>());
    Microsoft::WRL::ComPtr<IUnknown> unkInteraction;
    CInputServices* inputManagerNoRef = GetContext()->GetInputServices();
    inputManagerNoRef->GetViewportInteraction(this, dcompTreeHost->GetCompositor(), &unkInteraction);

    VERIFYHR(unkInteraction.Get()->QueryInterface(IID_PPV_ARGS(interaction)));
}

bool CUIElement::RaiseAccessKeyInvoked()
{
    if (ShouldRaiseEvent(KnownEventIndex::UIElement_AccessKeyInvoked))
    {
        CEventManager *pEventManager = GetContext()->GetEventManager();
        ASSERT(pEventManager);

        xref_ptr<CAccessKeyInvokedEventArgs> spArgs;
        VERIFYHR(spArgs.init(new CAccessKeyInvokedEventArgs()));

        pEventManager->Raise(
            EventHandle(KnownEventIndex::UIElement_AccessKeyInvoked),
            TRUE   /* bRefire */,
            this    /* pSender */,
            spArgs  /* pArgs */,
            TRUE    /* fRaiseSync */);

        return spArgs->GetHandled();
    }

    return false;
}

void CUIElement::RaiseAccessKeyShown(_In_z_ const wchar_t* strPressedKeys)
{
    GetContext()->GetInputServices()->GetKeyTipManager().ShowAutoKeyTipForElement(this, strPressedKeys);

    if (ShouldRaiseEvent(KnownEventIndex::UIElement_AccessKeyDisplayRequested))
    {
        CEventManager *pEventManager = GetContext()->GetEventManager();
        ASSERT(pEventManager);

        xref_ptr<CAccessKeyDisplayRequestedEventArgs> spArgs;
        IFCFAILFAST(spArgs.init(new CAccessKeyDisplayRequestedEventArgs(strPressedKeys)));

        pEventManager->Raise(
            EventHandle(KnownEventIndex::UIElement_AccessKeyDisplayRequested),
            TRUE   /* bRefire */,
            this    /* pSender */,
            spArgs  /* pArgs */,
            FALSE    /* fRaiseSync */);
    }
}

void CUIElement::RaiseAccessKeyHidden()
{
    GetContext()->GetInputServices()->GetKeyTipManager().HideAutoKeyTipForElement(this);

    if (ShouldRaiseEvent(KnownEventIndex::UIElement_AccessKeyDisplayDismissed))
    {
        CEventManager *pEventManager = GetContext()->GetEventManager();
        ASSERT(pEventManager);

        xref_ptr<CAccessKeyDisplayDismissedEventArgs> spArgs;
        IFCFAILFAST(spArgs.init(new CAccessKeyDisplayDismissedEventArgs()));

        pEventManager->Raise(
            EventHandle(KnownEventIndex::UIElement_AccessKeyDisplayDismissed),
            TRUE   /* bRefire */,
            this    /* pSender */,
            spArgs  /* pArgs */,
            FALSE    /* fRaiseSync */);
    }
}

// Implements a depth-first search of the element's sub-tree,
// looking for an accelerator that can be invoked
_Check_return_ HRESULT CUIElement::TryInvokeKeyboardAccelerator(
    _In_ const CDependencyObject* const pFocusedElement,
    _In_ CUIElement* const pElement,
    _In_ const wsy::VirtualKey key,
    _In_ const wsy::VirtualKeyModifiers keyModifiers,
    _Inout_ BOOLEAN& handled,
    _Inout_ BOOLEAN& handledShouldNotImpedeTextInput)
{
    //Try to process accelerators on current CUIElement.
    IFC_RETURN(KeyboardAcceleratorUtility::ProcessKeyboardAccelerators(
        key,
        keyModifiers,
        VisualTree::GetContentRootForElement(pElement)->GetAllLiveKeyboardAccelerators(),
        pElement,
        &handled,
        &handledShouldNotImpedeTextInput,
        pFocusedElement,
        true /*isCallFromTryInvoke*/ ));
    if (handled)
    {
        return S_OK;
    }
    CUIElementCollection* pCollection = nullptr;
    if (pElement->CanHaveChildren())
    {
        pCollection = pElement->GetChildren();
    }
    if (!pCollection)
    {
        return S_OK;
    }
    //For each child make recursive call
    for (auto& pDOChild : *pCollection)
    {
        auto pChild = do_pointer_cast<CUIElement>(pDOChild);
        if (pChild && pChild->IsEnabled())
        {
            IFC_RETURN(TryInvokeKeyboardAccelerator(pFocusedElement, pChild, key, keyModifiers, handled, handledShouldNotImpedeTextInput));
            if (handled)
            {
                return S_OK;
            }
        }
    }

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Return true if the pointer position is over the element
//
//-------------------------------------------------------------------------
bool CUIElement::IsPointerPositionOver(_In_ XPOINTF pointPosition)
{
    bool bPointerOver = false;
    xref_ptr<CDependencyObject> contact;

    CUIElement* element = this;
    CInputManager& inputManager = VisualTree::GetContentRootForElement(this)->GetInputManager();
    auto hitTestRoot = VisualTree::GetXamlIslandRootForElement(element);
    auto pointerInputProcessor = inputManager.GetPointerInputProcessor();

    if (SUCCEEDED(pointerInputProcessor.HitTestHelper(
        pointPosition,
        hitTestRoot,
        contact.ReleaseAndGetAddressOf())))
    {
        // If we've hit the popup root, then we don't want to consider that,
        // since it's used strictly as the light-dismiss layer to popups,
        // and is not considered to be a parent of any element for hit-testing
        // purposes.  We'll disable it for hit-testing and hit-test again.
        // We only ever call this method if we've already had this element
        // returned from pointer hit-testing, so it's fine to be a bit lax
        // on whether we return true here - this would technically return true
        // if the pointer were over a non-hit-testable popup, but in that case
        // the earlier hit-testing shouldn't have returned this element.
        if (auto popupRoot = do_pointer_cast<CPopupRoot>(contact))
        {
            CUIElementHitTestDisabler disablePopupRootHitTesting(popupRoot);

            if (!SUCCEEDED(pointerInputProcessor.HitTestHelper(pointPosition, hitTestRoot, contact.ReleaseAndGetAddressOf())))
            {
                return false;
            }
        }

        // InputManager ProcesPointerInput() reports the pointer events to the visual root in case of
        // hitting no element contact. IsPointerPositionOver also need to check the current touch element
        // is the visual root in case of no hitting element.
        if (!contact)
        {
            contact = GetPublicRootVisual();
        }

        if ((contact && (contact.get() == element || element->IsAncestorOf(contact.get()))) ||
            (element && element == element->GetContext()->GetMainRootVisual()))
        {
            bPointerOver = true;
        }
    }

    return bPointerOver;
}

bool CUIElement::HasShownHiddenHandlers() const
{
    DirectUI::UIElement* dxamlUIE = static_cast<DirectUI::UIElement*>(GetDXamlPeer());

    return dxamlUIE != nullptr
        && dxamlUIE->HasShownHiddenHandlers();
}

bool CUIElement::HasHiddenHandlers() const
{
    DirectUI::UIElement* dxamlUIE = static_cast<DirectUI::UIElement*>(GetDXamlPeer());

    return dxamlUIE != nullptr
        && dxamlUIE->HasHiddenHandlers();
}

void CUIElement::FireShownHiddenEvent(KnownEventIndex eventIndex)
{
    // Only fire the event if we know there are Shown/Hidden handlers attached. After firing the event we'll check
    // whether there are still handlers attached, and if there aren't then we'll stop monitoring this element's
    // effective visibility. In the ECP implicit show/hide animation scenario we'll also get here after detecting
    // an effective visibility change, and in that case we don't want to stop monitoring effective visibility on
    // this element because we see that there are no Shown/Hidden handlers attached - there weren't any to begin with.
    if (m_pEventList && HasShownHiddenHandlers())
    {
        auto core = GetContext();
        CEventManager* pEventManager = core->GetEventManager();

        xref_ptr<CEventArgs> args;
        args.attach(new CEventArgs());

        pEventManager->Raise(
            EventHandle(eventIndex),
            TRUE /* bRefire */,
            this /* pSender */,
            args /* pArgs */,
            TRUE /* fRaiseSync */);

        // It's possible for the app to return E_RPC_DISCONNECTED from an event handler and unregister that way.
        // Check whether there are still handlers attached.
        if (!HasShownHiddenHandlers())
        {
            AllShownHiddenHandlersRemoved();
        }
    }
}

void CUIElement::EnsureRootCanvasCompNode()
{
    VisualTree* visualTree = VisualTree::GetForElementNoRef(this);

    CUIElement* publicRoot = visualTree->GetPublicRootVisual();

    if (publicRoot->OfTypeByIndex<KnownTypeIndex::Canvas>() && !publicRoot->IsProjectedShadowDefaultReceiver())
    {
        publicRoot->SetRequiresComposition(CompositionRequirement::ProjectedShadowDefaultReceiver, IndependentAnimationType::None);
    }
}

namespace UIA { namespace Private {

xref_ptr<CDependencyObject> FindElementByName(
    _In_ CDependencyObject *pDO,
    _In_ const xstring_ptr_view& strName)
{
    if (!pDO->OfTypeByIndex<KnownTypeIndex::UIElement>())
    {
        return nullptr;
    }

    CUIElement* pUIElement = static_cast<CUIElement*>(pDO);
    auto namedObject = pUIElement->GetContext()->TryGetElementByName(strName, pUIElement);
    if (namedObject == nullptr)
    {
        CUIElementCollection* pChildren = pUIElement->GetChildren();
        if (pChildren != nullptr)
        {
            for (auto& pChild : *pChildren)
            {
                ASSERT(pChild);
                namedObject = FindElementByName(pChild, strName);
                if (namedObject != nullptr)
                {
                    break;
                }
            }
        }
    }

    return namedObject;
}

} }
