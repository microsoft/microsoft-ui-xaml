// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ScrollViewer.g.h"
#include "PropertyMetadata.g.h"
#include "ScrollBar.g.h"
#include "ScrollContentPresenter.g.h"
#include "VisualStateGroup.g.h"
#include "Storyboard.g.h"
#include "TimelineCollection.g.h"
#include "ObjectAnimationUsingKeyFrames.g.h"
#include "ObjectKeyFrameCollection.g.h"
#include "DiscreteObjectKeyFrame.g.h"
#include "ScrollViewerAutomationPeer.g.h"
#include "PointerRoutedEventArgs.g.h"
#include "CarouselPanel.g.h"
#include "ItemsPresenter.g.h"
#include "ScrollViewerViewChangingEventArgs.g.h"
#include "ScrollViewerViewChangedEventArgs.g.h"
#include "ScrollViewerView.g.h"
#include "UIDMContainerHandler.h"
#include "RoutedEventArgs.h"
#include "InputPointEventArgs.h"
#include "BringIntoViewRequestedEventArgs.g.h"
#include "InputServices.h"
#include "KeyRoutedEventArgs.g.h"
#include "XboxUtility.h"
#include "FocusManager.g.h"
#include "FocusEngagedEventArgs.g.h"
#include "InputPaneHandler.h"
#include "DirectManipulationStateChangeHandler.h"
#include "VisualTreeHelper.h"
#include "focusmgr.h"
#include "CoreEventArgsGroup.h"
#include <windows.ui.viewmanagement.core.h>
#include "AnchorRequestedEventArgs.g.h"
#include "GeneralTransform.g.h"
#include "RootScale.h"
#include <XamlOneCoreTransforms.h>

using namespace DirectUI;
using namespace DirectUISynonyms;

// Define as 1 (i.e. XCP_TRACE_OUTPUT_MSG) to get DirectManipulation debug outputs, and 0 otherwise
#define DMSV_DBG 0
//#define DM_DEBUG

// Define as 1 (i.e. XCP_TRACE_OUTPUT_MSG) to get DirectManipulation verbose debug outputs, and 0 otherwise
#define DMSVv_DBG 0

// These keycodes are undefined in the VirtualKey enum, so define them here.
// - (on number row)
#define SCROLLVIEWER_KEYCODE_MINUS 189
// = (on number row)
#define SCROLLVIEWER_KEYCODE_EQUALS 187

// Returns True/False depending on the OS Settings "Play animations in Windows" value. A True OS value can be overridden by
// the RuntimeEnabledFeature::DisableGlobalAnimations key for testing purposes.
static bool IsAnimationEnabled()
{
    bool isAnimationEnabled = false;

    // When animations are turned off in the OS Settings, DManip will jump instead of animate when calling its ZoomToRect method.
    // To avoid mishandling the resulting XcpDMViewportReady viewport status in CInputServices::BringIntoViewport (see comment
    // "Else DManip did not take any action because the target transform is too close to the current one."), when fAnimate is True
    // and newStatus != XcpDMViewportInertia, animations are immediately turned off before calling CInputServices::BringIntoViewport.
    // Thus no animation and XcpDMViewportInertia status are expected in the first place.

    // HR result in being ignored here as for all other calls to FrameworkCallbacks_IsAnimationEnabled. In case of failure False is
    // returned to be on the safe side. CVisualStateGroupCollection::AreAnimationsEnabled() mentions that this method may return
    // error HRs for non-CoreApplication contexts.

    IGNOREHR(DXamlCore::GetCurrent()->IsAnimationEnabled(&isAnimationEnabled));

    return isAnimationEnabled;
}

// Initializes a new instance of the ScrollViewer class.
ScrollViewer::ScrollViewer()
    : m_templatedParentHandlesMouseButton(FALSE)
    , m_templatedParentHandlesScrolling(FALSE)
    , m_inMeasure(FALSE)
    , m_inChildInvalidateMeasure(FALSE)
    , m_ignoreSemanticZoomNavigationInput(FALSE)
    , m_handleScrollInfoWheelEvent(TRUE)
    , m_scrollVisibilityX(xaml::Visibility_Visible)
    , m_scrollVisibilityY(xaml::Visibility_Visible)
    , m_xOffset(0.0)
    , m_yOffset(0.0)
    , m_xMinOffset(0.0)
    , m_yMinOffset(0.0)
    , m_xPixelOffset(0.0)
    , m_yPixelOffset(0.0)
    , m_unboundHorizontalOffset(0.0)
    , m_unboundVerticalOffset(0.0)
    , m_xViewport(0.0)
    , m_yViewport(0.0)
    , m_xPixelViewport(0.0)
    , m_yPixelViewport(0.0)
    , m_xExtent(0.0)
    , m_yExtent(0.0)
    , m_xPixelExtent(0.0)
    , m_yPixelExtent(0.0)
    , m_contentWidthRequested(-1)
    , m_contentHeightRequested(-1)
    , m_xPixelOffsetRequested(-1)
    , m_yPixelOffsetRequested(-1)
    , m_cLevelsFromRootCallForZoom(0)
    , m_initialMaxZoomFactor(0.0)
    , m_initialZoomFactor(0.0)
    , m_requestedMaxZoomFactor(0.0)
    , m_requestedZoomFactor(0.0)
    , m_preDirectManipulationOffsetX(0.0f)
    , m_preDirectManipulationOffsetY(0.0f)
    , m_preDirectManipulationZoomFactor(0.0f)
    , m_preDirectManipulationNonVirtualizedTranslationCorrection(0.0f)
    , m_currentHorizontalScrollMode(xaml_controls::ScrollMode_Disabled)
    , m_currentVerticalScrollMode(xaml_controls::ScrollMode_Disabled)
    , m_currentZoomMode(xaml_controls::ZoomMode_Disabled)
    , m_currentHorizontalScrollBarVisibility(xaml_controls::ScrollBarVisibility_Disabled)
    , m_currentVerticalScrollBarVisibility(xaml_controls::ScrollBarVisibility_Disabled)
    , m_currentHorizontalAlignment(DMAlignmentNone)
    , m_currentVerticalAlignment(DMAlignmentNone)
    , m_currentIsHorizontalRailEnabled(FALSE)
    , m_currentIsVerticalRailEnabled(FALSE)
    , m_currentIsScrollInertiaEnabled(FALSE)
    , m_currentIsZoomInertiaEnabled(FALSE)
    , m_pDMStateChangeHandler(NULL)
    , m_hManipulationHandler(NULL)
    , m_canManipulateElementsByTouch(FALSE)
    , m_canManipulateElementsNonTouch(FALSE)
    , m_canManipulateElementsWithBringIntoViewport(FALSE)
    , m_canManipulateElementsWithAsyncBringIntoViewport(FALSE)
    , m_touchConfiguration(DMConfigurationNone)
    , m_nonTouchConfiguration(DMConfigurationNone)
    , m_activeHorizontalAlignment(DMAlignmentNone)
    , m_activeVerticalAlignment(DMAlignmentNone)
    , m_overridingMinZoomFactor(0.0f)
    , m_overridingMaxZoomFactor(0.0f)
    , m_dmanipState()
    , m_isInDirectManipulation(FALSE)
    , m_isDirectManipulationStopped(FALSE)
    , m_isInDirectManipulationCompletion(FALSE)
    , m_isInDirectManipulationZoom(FALSE)
    , m_isInDirectManipulationSync(FALSE)
    , m_isInChangeViewBringIntoViewport(FALSE)
    , m_isInZoomFactorSync(FALSE)
    , m_isInConstantVelocityPan(FALSE)
    , m_isManipulationHandlerInterestedInNotifications(FALSE)
    , m_isDirectManipulationZoomFactorChange(FALSE)
    , m_isDirectManipulationZoomFactorChangeIgnored(FALSE)
    , m_isOffsetChangeIgnored(FALSE)
    , m_areViewportConfigurationsInvalid(FALSE)
    , m_isCanManipulateElementsInvalid(FALSE)
    , m_isScrollBarIgnoringUserInputInvalid(FALSE)
    , m_isPointerLeftButtonPressed(FALSE)
    , m_shouldFocusOnRightTapUnhandled(FALSE)
    , m_showingMouseIndicators(FALSE)
    , m_preferMouseIndicators(FALSE)
    , m_isTargetHorizontalOffsetValid(FALSE)
    , m_isTargetVerticalOffsetValid(FALSE)
    , m_isTargetZoomFactorValid(FALSE)
    , m_targetHorizontalOffset(0.0)
    , m_targetVerticalOffset(0.0)
    , m_targetZoomFactor(1.0f)
    , m_targetChangeViewHorizontalOffset(-1.0)
    , m_targetChangeViewVerticalOffset(-1.0)
    , m_targetChangeViewZoomFactor(-1.0f)
    , m_inertiaEndHorizontalOffset(0.0f)
    , m_inertiaEndVerticalOffset(0.0f)
    , m_inertiaEndZoomFactor(1.0f)
    , m_isInertiaEndTransformValid(FALSE)
    , m_isInertial(FALSE)
    , m_isViewChangingDelayed(FALSE)
    , m_isViewChangedDelayed(FALSE)
    , m_isDelayedViewChangedIntermediate(FALSE)
    , m_isInIntermediateViewChangedMode(FALSE)
    , m_isViewChangedRaisedInIntermediateMode(FALSE)
    , m_iViewChangingDelay(0)
    , m_iViewChangedDelay(0)
    , m_inSemanticZoomAnimation(FALSE)
    , m_keepIndicatorsShowing(FALSE)
    , m_isPointerOverVerticalScrollbar(FALSE)
    , m_isPointerOverHorizontalScrollbar(FALSE)
    , m_isLoaded(FALSE)
    , m_blockIndicators(FALSE)
    , m_isDraggingThumb(FALSE)
    , m_horizontalOffsetCached(-1.0)
    , m_verticalOffsetCached(-1.0)
    , m_isFocusableOnFlyoutScrollViewer(FALSE)
    , m_isTopLeftHeaderAssociated(FALSE)
    , m_isTopHeaderAssociated(FALSE)
    , m_isLeftHeaderAssociated(FALSE)
    , m_isNearVerticalAlignmentForced(FALSE)
    , m_isHorizontalStretchAlignmentTreatedAsNear(FALSE)
    , m_isVerticalStretchAlignmentTreatedAsNear(FALSE)
    , m_cForcePanXConfiguration(0)
    , m_cForcePanYConfiguration(0)
{
    m_layoutSize.Width = m_layoutSize.Height = 0;

    ZeroMemory(&m_HorizontalScrollToken, sizeof(m_HorizontalScrollToken));
    ZeroMemory(&m_horizontalThumbDragStartedToken, sizeof(m_horizontalThumbDragStartedToken));
    ZeroMemory(&m_horizontalThumbDragCompletedToken, sizeof(m_horizontalThumbDragCompletedToken));
    ZeroMemory(&m_VerticalScrollToken, sizeof(m_VerticalScrollToken));
    ZeroMemory(&m_verticalThumbDragStartedToken, sizeof(m_verticalThumbDragStartedToken));
    ZeroMemory(&m_verticalThumbDragCompletedToken, sizeof(m_verticalThumbDragCompletedToken));
    ZeroMemory(&m_HorizontalSnapPointsChangedToken, sizeof(m_HorizontalSnapPointsChangedToken));
    ZeroMemory(&m_VerticalSnapPointsChangedToken, sizeof(m_VerticalSnapPointsChangedToken));
    ZeroMemory(&m_ZoomSnapPointsVectorChangedToken, sizeof(m_ZoomSnapPointsVectorChangedToken));
    ZeroMemory(&m_verticalScrollbarPointerEnteredToken, sizeof(m_verticalScrollbarPointerEnteredToken));
    ZeroMemory(&m_verticalScrollbarPointerExitedToken, sizeof(m_verticalScrollbarPointerExitedToken));
    ZeroMemory(&m_horizontalScrollbarPointerEnteredToken, sizeof(m_horizontalScrollbarPointerEnteredToken));
    ZeroMemory(&m_horizontalScrollbarPointerExitedToken, sizeof(m_horizontalScrollbarPointerExitedToken));
    ZeroMemory(&m_coreInputViewOcclusionsChangedToken, sizeof(m_coreInputViewOcclusionsChangedToken));

    m_horizontalOverpanMode = DMOverpanModeDefault;
    m_verticalOverpanMode = DMOverpanModeDefault;
}

// Destroys an instance of the ScrollViewer class.
ScrollViewer::~ScrollViewer()
{
    // Releasing and unhooking of template parts and events
    IGNOREHR(UnhookTemplate());

    m_trManipulatableElement.Clear();

    if (m_spZoomSnapPoints)
    {
        IGNOREHR(m_spZoomSnapPoints->remove_VectorChanged(m_ZoomSnapPointsVectorChangedToken));
        ZeroMemory(&m_ZoomSnapPointsVectorChangedToken, sizeof(m_ZoomSnapPointsVectorChangedToken));
    }

    if (m_coreInputViewOcclusionsChangedToken.value != 0)
    {
        wrl::ComPtr<wuv::Core::ICoreInputViewStatics> spCoreInputViewStatics;
        wrl::ComPtr<wuv::Core::ICoreInputView> spCoreInputView;

        IGNOREHR(wf::GetActivationFactory(
            wrl_wrappers::HStringReference(RuntimeClass_Windows_UI_ViewManagement_Core_CoreInputView).Get(),
            &spCoreInputViewStatics));
        IGNOREHR(spCoreInputViewStatics->GetForCurrentView(&spCoreInputView));

        if (spCoreInputView)
        {
            IGNOREHR(spCoreInputView->remove_OcclusionsChanged(m_coreInputViewOcclusionsChangedToken));
            ZeroMemory(&m_coreInputViewOcclusionsChangedToken, sizeof(m_coreInputViewOcclusionsChangedToken));
        }
    }

    IGNOREHR(UnhookScrollSnapPointsInfoEvents(TRUE /*isForHorizontalSnapPoints*/));
    IGNOREHR(UnhookScrollSnapPointsInfoEvents(FALSE /*isForHorizontalSnapPoints*/));

    if (auto dxamlCore = DXamlCore::GetCurrent())
    {
        dxamlCore->UnregisterFromDynamicScrollbarsSettingChanged(this);
    }
}

// Supports the IScrollOwner interface.
_Check_return_ HRESULT ScrollViewer::QueryInterfaceImpl(
    _In_ REFIID iid,
    _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(IScrollOwner)))
    {
        *ppObject = static_cast<IScrollOwner*>(this);
    }
    else
    {
        RRETURN(ScrollViewerGenerated::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}

// Prepares object's state
_Check_return_ HRESULT ScrollViewer::Initialize()
{
    ctl::ComPtr<wf::ITypedEventHandler<xaml::UIElement*, xaml::BringIntoViewRequestedEventArgs*>> spBringIntoViewRequestedEventHandler;
    ctl::ComPtr<xaml::IRoutedEventHandler> spLoadedEventHandler;
    ctl::ComPtr<xaml::IRoutedEventHandler> spUnloadedEventHandler;
    EventRegistrationToken token1;
    EventRegistrationToken token2;

    IFC_RETURN(ScrollViewerGenerated::Initialize());

    IFC_RETURN(get_ZoomMode(&m_currentZoomMode));


    IFC_RETURN(m_bringIntoViewRequestedHandler.AttachEventHandler(
        this,
        std::bind(&ScrollViewer::OnBringIntoViewRequested, this, std::placeholders::_1, std::placeholders::_2)));

    spLoadedEventHandler.Attach(
        new ClassMemberEventHandler <
        ScrollViewer,
        xaml_controls::IScrollViewer,
        xaml::IRoutedEventHandler,
        IInspectable,
        xaml::IRoutedEventArgs >(this, &ScrollViewer::OnLoaded, true /* subscribingToSelf */));
    IFC_RETURN(add_Loaded(spLoadedEventHandler.Get(), &token1));

    spUnloadedEventHandler.Attach(
        new ClassMemberEventHandler <
        ScrollViewer,
        xaml_controls::IScrollViewer,
        xaml::IRoutedEventHandler,
        IInspectable,
        xaml::IRoutedEventArgs >(this, &ScrollViewer::OnUnloaded, true /* subscribingToSelf */));
    IFC_RETURN(add_Unloaded(spUnloadedEventHandler.Get(), &token2));

    if (!IsRootScrollViewer())
    {
        DXamlCore::GetCurrent()->RegisterForChangeVisualStateOnDynamicScrollbarsSettingChanged(this);
    }

    return S_OK;
}

_Check_return_ HRESULT ScrollViewer::OnPropertyChanged2(_In_ const PropertyChangedParams& args)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> spOldValue;
    ctl::ComPtr<IInspectable> spNewValue;

    IFC(ScrollViewerGenerated::OnPropertyChanged2(args));

    switch (args.m_pDP->GetIndex())
    {
    case KnownPropertyIndex::ScrollViewer_HorizontalScrollBarVisibility:
    case KnownPropertyIndex::ScrollViewer_VerticalScrollBarVisibility:
        IFC(OnScrollBarVisibilityChanged());
        break;

    case KnownPropertyIndex::ScrollViewer_MinZoomFactor:
        IFC(CValueBoxer::UnboxObjectValue(args.m_pOldValue, /* pTargetType */ nullptr, &spOldValue));
        IFC(CValueBoxer::UnboxObjectValue(args.m_pNewValue, /* pTargetType */ nullptr, &spNewValue));
        IFC(OnMinZoomFactorPropertyChanged(spOldValue.Get(), spNewValue.Get()));
        break;

    case KnownPropertyIndex::ScrollViewer_HorizontalOffset:
        IFC(CValueBoxer::UnboxObjectValue(args.m_pOldValue, /* pTargetType */ nullptr, &spOldValue));
        IFC(CValueBoxer::UnboxObjectValue(args.m_pNewValue, /* pTargetType */ nullptr, &spNewValue));
        IFC(OnHorizontalOffsetPropertyChanged(spOldValue.Get(), spNewValue.Get()));
        break;

    case KnownPropertyIndex::ScrollViewer_VerticalOffset:
        IFC(CValueBoxer::UnboxObjectValue(args.m_pOldValue, /* pTargetType */ nullptr, &spOldValue));
        IFC(CValueBoxer::UnboxObjectValue(args.m_pNewValue, /* pTargetType */ nullptr, &spNewValue));
        IFC(OnVerticalOffsetPropertyChanged(spOldValue.Get(), spNewValue.Get()));
        break;

    case KnownPropertyIndex::ScrollViewer_ZoomFactor:
        IFC(CValueBoxer::UnboxObjectValue(args.m_pOldValue, /* pTargetType */ nullptr, &spOldValue));
        IFC(CValueBoxer::UnboxObjectValue(args.m_pNewValue, /* pTargetType */ nullptr, &spNewValue));
        IFC(OnZoomFactorPropertyChanged(spOldValue.Get(), spNewValue.Get()));
        break;

    case KnownPropertyIndex::ScrollViewer_MaxZoomFactor:
        IFC(CValueBoxer::UnboxObjectValue(args.m_pOldValue, /* pTargetType */ nullptr, &spOldValue));
        IFC(CValueBoxer::UnboxObjectValue(args.m_pNewValue, /* pTargetType */ nullptr, &spNewValue));
        IFC(OnMaxZoomFactorPropertyChanged(spOldValue.Get(), spNewValue.Get()));
        break;

    case KnownPropertyIndex::ScrollViewer_HorizontalSnapPointsAlignment:
    case KnownPropertyIndex::ScrollViewer_HorizontalSnapPointsType:
        IFC(OnSnapPointsAffectingPropertyChanged(DMMotionTypePanX, IsInDirectManipulation() && args.m_pDP->GetIndex() == KnownPropertyIndex::ScrollViewer_HorizontalSnapPointsType/*updateSnapPointsChangeSubscription*/));
        break;

    case KnownPropertyIndex::ScrollViewer_VerticalSnapPointsAlignment:
    case KnownPropertyIndex::ScrollViewer_VerticalSnapPointsType:
        IFC(OnSnapPointsAffectingPropertyChanged(DMMotionTypePanY, IsInDirectManipulation() && args.m_pDP->GetIndex() == KnownPropertyIndex::ScrollViewer_VerticalSnapPointsType/*updateSnapPointsChangeSubscription*/));
        break;

    case KnownPropertyIndex::ScrollViewer_ZoomSnapPointsType:
        IFC(OnSnapPointsAffectingPropertyChanged(DMMotionTypeZoom, FALSE /*updateSnapPointsChangeSubscription*/));
        break;

    case KnownPropertyIndex::ScrollViewer_HorizontalScrollMode:
    case KnownPropertyIndex::ScrollViewer_VerticalScrollMode:
        IFC(RefreshScrollBarIsIgnoringUserInput(args.m_pDP->GetIndex() == KnownPropertyIndex::ScrollViewer_HorizontalScrollMode/*isForHorizontalOrientation*/));

    case KnownPropertyIndex::ScrollViewer_ZoomMode:
        IFC(OnManipulatabilityAffectingPropertyChanged(
            NULL  /*pIsInLiveTree*/,
            TRUE  /*isCachedPropertyChanged*/,
            FALSE /*isContentChanged*/,
            TRUE  /*isAffectingConfigurations*/,
            FALSE /*isAffectingTouchConfiguration*/));
        // When the zoom factor changes from static to manipulatable or vice-versa,
        // a new content size may have to be pushed to DirectManipulation.
        IFC(OnPrimaryContentAffectingPropertyChanged(
            TRUE  /*boundsChanged*/,
            FALSE /*horizontalAlignmentChanged*/,
            FALSE /*verticalAlignmentChanged*/,
            FALSE /*zoomFactorBoundaryChanged*/));
        break;

    case KnownPropertyIndex::ScrollViewer_IsHorizontalScrollChainingEnabled:
    case KnownPropertyIndex::ScrollViewer_IsVerticalScrollChainingEnabled:
    case KnownPropertyIndex::ScrollViewer_IsZoomChainingEnabled:
        IFC(OnViewportAffectingPropertyChanged(
            FALSE /*boundsChanged*/,
            FALSE /*touchConfigurationChanged*/,
            FALSE /*nonTouchConfigurationChanged*/,
            FALSE /*configurationsChanged*/,
            TRUE  /*chainedMotionTypesChanged*/,
            FALSE /*horizontalOverpanModeChanged*/,
            FALSE /*verticalOverpanModeChanged*/,
            NULL  /*pAreConfigurationsUpdated*/));
        break;

    case KnownPropertyIndex::ScrollViewer_IsScrollInertiaEnabled:
    case KnownPropertyIndex::ScrollViewer_IsZoomInertiaEnabled:
    case KnownPropertyIndex::ScrollViewer_IsHorizontalRailEnabled:
    case KnownPropertyIndex::ScrollViewer_IsVerticalRailEnabled:
        IFC(OnViewportConfigurationsAffectingPropertyChanged());
        break;

    // TODO - Bug 17637741 - Consider removing this switch and using template binding in generic.xaml instead.
    case KnownPropertyIndex::ScrollViewer_CanContentRenderOutsideBounds:
        IFC(CValueBoxer::UnboxObjectValue(args.m_pNewValue, nullptr /*targetType*/, &spNewValue));
        IFC(OnCanContentRenderOutsideBoundsChanged(spNewValue.Get()));
        break;

    case KnownPropertyIndex::Control_HorizontalContentAlignment:
        IFC(OnPrimaryContentAffectingPropertyChanged(
            FALSE /*boundsChanged*/,
            TRUE  /*horizontalAlignmentChanged*/,
            FALSE /*verticalAlignmentChanged*/,
            FALSE /*zoomFactorBoundaryChanged*/));
        break;

    case KnownPropertyIndex::Control_VerticalContentAlignment:
        IFC(OnPrimaryContentAffectingPropertyChanged(
            FALSE /*boundsChanged*/,
            FALSE /*horizontalAlignmentChanged*/,
            TRUE  /*verticalAlignmentChanged*/,
            FALSE /*zoomFactorBoundaryChanged*/));
        break;

    case KnownPropertyIndex::ContentControl_Content:
        IFC(OnContentPropertyChanged());
        break;

    case KnownPropertyIndex::ScrollViewer_IsDeferredScrollingEnabled:
        // if we change the deferred property while thumbdragging,
        // we want to immediately synchronize and start keeping up.
        IFC(SynchronizeScrollOffsetsAfterThumbDeferring());
        break;

    case KnownPropertyIndex::ScrollViewer_ReduceViewportForCoreInputViewOcclusions:
        IFC(OnReduceViewportForCoreInputViewOcclusionsChanged());
        break;

    case KnownPropertyIndex::ScrollViewer_TopLeftHeader:
        IFC(CValueBoxer::UnboxObjectValue(args.m_pOldValue, /* pTargetType */ nullptr, &spOldValue));
        IFC(CValueBoxer::UnboxObjectValue(args.m_pNewValue, /* pTargetType */ nullptr, &spNewValue));
        IFC(OnHeaderPropertyChanged(TRUE /*isTopHeader*/, TRUE /*isLeftHeader*/, spOldValue.Get(), spNewValue.Get()));
        break;

    case KnownPropertyIndex::ScrollViewer_LeftHeader:
        IFC(CValueBoxer::UnboxObjectValue(args.m_pOldValue, /* pTargetType */ nullptr, &spOldValue));
        IFC(CValueBoxer::UnboxObjectValue(args.m_pNewValue, /* pTargetType */ nullptr, &spNewValue));
        IFC(OnHeaderPropertyChanged(FALSE /*isTopHeader*/, TRUE /*isLeftHeader*/, spOldValue.Get(), spNewValue.Get()));
        break;

    case KnownPropertyIndex::ScrollViewer_TopHeader:
        IFC(CValueBoxer::UnboxObjectValue(args.m_pOldValue, /* pTargetType */ nullptr, &spOldValue));
        IFC(CValueBoxer::UnboxObjectValue(args.m_pNewValue, /* pTargetType */ nullptr, &spNewValue));
        IFC(OnHeaderPropertyChanged(TRUE /*isTopHeader*/, FALSE /*isLeftHeader*/, spOldValue.Get(), spNewValue.Get()));
        break;
    }

Cleanup:
    RRETURN(hr);
}

// IsEnabled property changed handler.
_Check_return_ HRESULT ScrollViewer::OnIsEnabledChanged(
    _In_ IsEnabledChangedEventArgs* pArgs)
{
#ifdef DM_DEBUG
    if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
    {
        BOOLEAN isEnabledDbg = FALSE;
        IGNOREHR(get_IsEnabled(&isEnabledDbg));
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/,
            L"DMSV[0x%p]:  OnIsEnabledChanged IsEnabled=%d.", this, isEnabledDbg));
    }
#endif // DM_DEBUG

    HRESULT hr = S_OK;
    BOOLEAN isEnabled = FALSE;

    IFC(ScrollViewerGenerated::OnIsEnabledChanged(pArgs));

    // The new IsEnabled status most likely changes the result of
    // get_CanManipulateElements
    IFC(OnManipulatabilityAffectingPropertyChanged(
        NULL  /*pIsInLiveTree*/,
        FALSE /*isCachedPropertyChanged*/,
        FALSE /*isContentChanged*/,
        !IsInDirectManipulation() /*isAffectingConfigurations*/,
        FALSE /*isAffectingTouchConfiguration*/));

    IFC(get_IsEnabled(&isEnabled));
    if (!isEnabled)
    {
        IFC(SetConstantVelocities(0, 0));
    }
    IFC(UpdateVisualState());

Cleanup:
    RRETURN(hr);
}

// Called when the parent of this ScrollViewer changed. Makes sure the manipulation handler knows
// about the possible change in the get_CanManipulateElements return value.
_Check_return_ HRESULT ScrollViewer::OnTreeParentUpdated(
    _In_opt_ CDependencyObject *pNewParent,
    _In_ BOOLEAN isParentAlive)
{
#ifdef DM_DEBUG
    if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/,
            L"DMSV[0x%p]:  OnTreeParentUpdated pNewParent=0x%p, isParentAlive=%d.", this, pNewParent, isParentAlive));
    }
#endif // DM_DEBUG

    HRESULT hr = S_OK;

    IFC(ScrollViewerGenerated::OnTreeParentUpdated(pNewParent, isParentAlive));

    if (!m_hManipulationHandler)
    {
        // Register this ScrollViewer as a DM container when there is a parent, or pNewParent == NULL because
        // the ScrollViewer enters the tree as a RootVisual child.
        // Cancel this ScrollViewer's registration as a DM container in other cases.
        IFC(put_IsDirectManipulationContainer(isParentAlive || pNewParent /*isDirectManipulationContainer*/));
    }
    else
    {
        IFC(OnManipulatabilityAffectingPropertyChanged(
            NULL  /*pIsInLiveTree*/,
            FALSE /*isCachedPropertyChanged*/,
            FALSE /*isContentChanged*/,
            FALSE /*isAffectingConfigurations*/,
            FALSE /*isAffectingTouchConfiguration*/));
    }

Cleanup:
    RRETURN(hr);
}

// Gets or sets a value that indicates whether we should
// mark bring into view requests as handled or not.
_Check_return_ HRESULT ScrollViewer::get_IsRequestBringIntoViewIgnoredImpl(
    _Out_ BOOLEAN* value)
{
    *value = m_isRequestBringIntoViewIgnored;
    return S_OK;
}

_Check_return_ HRESULT ScrollViewer::put_IsRequestBringIntoViewIgnoredImpl(
    _In_ BOOLEAN value)
{
    m_isRequestBringIntoViewIgnored = value;
    return S_OK;
}

// Gets or sets the CanContentRenderOutsideBounds property.
_Check_return_ HRESULT ScrollViewer::get_CanContentRenderOutsideBoundsImpl(
    _Out_ BOOLEAN* value)
{
    IFC_RETURN(ScrollViewerFactory::GetCanContentRenderOutsideBoundsStatic(this, value));
    return S_OK;
}

_Check_return_ HRESULT ScrollViewer::put_CanContentRenderOutsideBoundsImpl(
    _In_ BOOLEAN value)
{
    IFC_RETURN(ScrollViewerFactory::SetCanContentRenderOutsideBoundsStatic(this, value));
    return S_OK;
}

// Gets or sets a value that indicates whether a horizontal ScrollBar
// should be displayed.
_Check_return_ HRESULT ScrollViewer::get_HorizontalScrollBarVisibilityImpl(
    _Out_ xaml_controls::ScrollBarVisibility* pValue)
{
    RRETURN(ScrollViewerFactory::GetHorizontalScrollBarVisibilityStatic(this, pValue));
}

_Check_return_ HRESULT ScrollViewer::put_HorizontalScrollBarVisibilityImpl(
    _In_ xaml_controls::ScrollBarVisibility value)
{
    RRETURN(ScrollViewerFactory::SetHorizontalScrollBarVisibilityStatic(this, value));
}

// Gets or sets a value that indicates whether a vertical ScrollBar
// should be displayed.
_Check_return_ HRESULT ScrollViewer::get_VerticalScrollBarVisibilityImpl(
    _Out_ xaml_controls::ScrollBarVisibility* pValue)
{
    RRETURN(ScrollViewerFactory::GetVerticalScrollBarVisibilityStatic(this, pValue));
}

_Check_return_ HRESULT ScrollViewer::put_VerticalScrollBarVisibilityImpl(
    _In_ xaml_controls::ScrollBarVisibility value)
{
    RRETURN(ScrollViewerFactory::SetVerticalScrollBarVisibilityStatic(this, value));
}

// Gets or sets a value that indicates whether we should attempt to handle
// scroll wheel events or ignore them and let them bubble up.
_Check_return_ HRESULT ScrollViewer::get_ArePointerWheelEventsIgnoredImpl(
    _Out_ BOOLEAN* value)
{
    *value = m_arePointerWheelEventsIgnored;
    return S_OK;
}

_Check_return_ HRESULT ScrollViewer::put_ArePointerWheelEventsIgnoredImpl(
    _In_ BOOLEAN value)
{
    m_arePointerWheelEventsIgnored = value;
    return S_OK;
}

// Gets or sets a value that indicates whether horizontal scroll chaining
// must be turned on during a DirectManipulation operation.
_Check_return_ HRESULT ScrollViewer::get_IsHorizontalScrollChainingEnabledImpl(
    _Out_ BOOLEAN* pValue)
{
    RRETURN(ScrollViewerFactory::GetIsHorizontalScrollChainingEnabledStatic(this, pValue));
}

_Check_return_ HRESULT ScrollViewer::put_IsHorizontalScrollChainingEnabledImpl(
    _In_ BOOLEAN value)
{
    RRETURN(ScrollViewerFactory::SetIsHorizontalScrollChainingEnabledStatic(this, value));
}

// Gets or sets a value that indicates whether vertical scroll chaining
// must be turned on during a DirectManipulation operation.
_Check_return_ HRESULT ScrollViewer::get_IsVerticalScrollChainingEnabledImpl(
    _Out_ BOOLEAN* pValue)
{
    RRETURN(ScrollViewerFactory::GetIsVerticalScrollChainingEnabledStatic(this, pValue));
}

_Check_return_ HRESULT ScrollViewer::put_IsVerticalScrollChainingEnabledImpl(
    _In_ BOOLEAN value)
{
    RRETURN(ScrollViewerFactory::SetIsVerticalScrollChainingEnabledStatic(this, value));
}

// Gets or sets a value that indicates whether zoom chaining must be
// turned on during a DirectManipulation operation.
_Check_return_ HRESULT ScrollViewer::get_IsZoomChainingEnabledImpl(
    _Out_ BOOLEAN* pValue)
{
    RRETURN(ScrollViewerFactory::GetIsZoomChainingEnabledStatic(this, pValue));
}

_Check_return_ HRESULT ScrollViewer::put_IsZoomChainingEnabledImpl(
    _In_ BOOLEAN value)
{
    RRETURN(ScrollViewerFactory::SetIsZoomChainingEnabledStatic(this, value));
}

// Gets or sets a value that indicates whether horizontal railing must be
// turned on during a DirectManipulation operation.
_Check_return_ HRESULT ScrollViewer::get_IsHorizontalRailEnabledImpl(
    _Out_ BOOLEAN* pValue)
{
    RRETURN(ScrollViewerFactory::GetIsHorizontalRailEnabledStatic(this, pValue));
}

_Check_return_ HRESULT ScrollViewer::put_IsHorizontalRailEnabledImpl(
    _In_ BOOLEAN value)
{
    RRETURN(ScrollViewerFactory::SetIsHorizontalRailEnabledStatic(this, value));
}

// Gets or sets a value that indicates whether verrtical railing must be
// turned on during a DirectManipulation operation.
_Check_return_ HRESULT ScrollViewer::get_IsVerticalRailEnabledImpl(
    _Out_ BOOLEAN* pValue)
{
    RRETURN(ScrollViewerFactory::GetIsVerticalRailEnabledStatic(this, pValue));
}

_Check_return_ HRESULT ScrollViewer::put_IsVerticalRailEnabledImpl(
    _In_ BOOLEAN value)
{
    RRETURN(ScrollViewerFactory::SetIsVerticalRailEnabledStatic(this, value));
}

// Gets or sets a value that indicates whether scroll inertia must be
// turned on during a DirectManipulation operation.
_Check_return_ HRESULT ScrollViewer::get_IsScrollInertiaEnabledImpl(
    _Out_ BOOLEAN* pValue)
{
    RRETURN(ScrollViewerFactory::GetIsScrollInertiaEnabledStatic(this, pValue));
}

_Check_return_ HRESULT ScrollViewer::put_IsScrollInertiaEnabledImpl(
    _In_ BOOLEAN value)
{
    RRETURN(ScrollViewerFactory::SetIsScrollInertiaEnabledStatic(this, value));
}

// Gets or sets a value that indicates whether zoom inertia must be
// turned on during a DirectManipulation operation.
_Check_return_ HRESULT ScrollViewer::get_IsZoomInertiaEnabledImpl(
    _Out_ BOOLEAN* pValue)
{
    RRETURN(ScrollViewerFactory::GetIsZoomInertiaEnabledStatic(this, pValue));
}

_Check_return_ HRESULT ScrollViewer::put_IsZoomInertiaEnabledImpl(
    _In_ BOOLEAN value)
{
    RRETURN(ScrollViewerFactory::SetIsZoomInertiaEnabledStatic(this, value));
}

// Gets or sets the horizontal scroll mode property.
_Check_return_ HRESULT ScrollViewer::get_HorizontalScrollModeImpl(
    _Out_ xaml_controls::ScrollMode* pValue)
{
    RRETURN(ScrollViewerFactory::GetHorizontalScrollModeStatic(this, pValue));
}

_Check_return_ HRESULT ScrollViewer::put_HorizontalScrollModeImpl(
    _In_ xaml_controls::ScrollMode value)
{
    RRETURN(ScrollViewerFactory::SetHorizontalScrollModeStatic(this, value));
}

// Gets or sets the vertical scroll mode property.
_Check_return_ HRESULT ScrollViewer::get_VerticalScrollModeImpl(
    _Out_ xaml_controls::ScrollMode* pValue)
{
    RRETURN(ScrollViewerFactory::GetVerticalScrollModeStatic(this, pValue));
}

_Check_return_ HRESULT ScrollViewer::put_VerticalScrollModeImpl(
    _In_ xaml_controls::ScrollMode value)
{
    RRETURN(ScrollViewerFactory::SetVerticalScrollModeStatic(this, value));
}

// Gets or sets the zoom mode property.
_Check_return_ HRESULT ScrollViewer::get_ZoomModeImpl(
    _Out_ xaml_controls::ZoomMode* pValue)
{
    RRETURN(ScrollViewerFactory::GetZoomModeStatic(this, pValue));
}

_Check_return_ HRESULT ScrollViewer::put_ZoomModeImpl(
    _In_ xaml_controls::ZoomMode value)
{
    RRETURN(ScrollViewerFactory::SetZoomModeStatic(this, value));
}

// Gets or sets the IsDeferredScrollingEnabled property.
_Check_return_ HRESULT ScrollViewer::get_IsDeferredScrollingEnabledImpl(
    _Out_ BOOLEAN* pValue)
{
    RRETURN(ScrollViewerFactory::GetIsDeferredScrollingEnabledStatic(this, pValue));
}

_Check_return_ HRESULT ScrollViewer::put_IsDeferredScrollingEnabledImpl(
    _In_ BOOLEAN value)
{
    RRETURN(ScrollViewerFactory::SetIsDeferredScrollingEnabledStatic(this, value));
}

// Gets or sets the BringIntoViewOnFocusChange property.
_Check_return_ HRESULT ScrollViewer::get_BringIntoViewOnFocusChangeImpl(
    _Out_ BOOLEAN* pValue)
{
    RRETURN(ScrollViewerFactory::GetBringIntoViewOnFocusChangeStatic(this, pValue));
}

_Check_return_ HRESULT ScrollViewer::put_BringIntoViewOnFocusChangeImpl(
    _In_ BOOLEAN value)
{
    RRETURN(ScrollViewerFactory::SetBringIntoViewOnFocusChangeStatic(this, value));
}

_Check_return_ HRESULT ScrollViewer::get_IsInDirectManipulationImpl(
    _Out_ BOOLEAN* pValue)
{
    *pValue = m_isInDirectManipulation;
    RRETURN(S_OK);
}

_Check_return_ HRESULT ScrollViewer::get_IsInActiveDirectManipulationImpl(
    _Out_ BOOLEAN* value)
{
    // If we are still in the starting state, it means we have pointer contact but
    // the viewport hasn't moved yet. In case of a touch tap for example, we will go
    // directly from DMManipulationStarting to DMManipulationCompleted.
    *value = m_isInDirectManipulation && m_dmanipState != DMManipulationStarting;
    return S_OK;
}

// Gets the value of the horizontal offset of the content.
IFACEMETHODIMP ScrollViewer::get_HorizontalOffset(
    _Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;

    *pValue = MAX(m_xOffset, m_xMinOffset);

    RRETURN(hr);
}

_Check_return_ HRESULT ScrollViewer::put_HorizontalOffset(
    _In_ DOUBLE value)
{
    HRESULT hr = S_OK;

    m_xOffset = MAX(value, m_xMinOffset);
    IFC(ScrollViewerGenerated::put_HorizontalOffset(m_xOffset));

    if (!IsInDirectManipulation())
    {
        m_unboundHorizontalOffset = m_xPixelOffset;
    }

#ifdef DM_DEBUG
    if (DMSVv_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE)) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE | DMSVv_DBG) /*traceType*/, L"DMSVv[0x%p]: put_HorizontalOffset m_xOffset=%f.", this, m_xOffset));
    }
#endif // DM_DEBUG

Cleanup:
    RRETURN(hr);
}

// Gets the value of the minimal horizontal offset of the content.
_Check_return_ HRESULT ScrollViewer::get_MinHorizontalOffset(
    _Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;

    *pValue = m_xMinOffset;

    RRETURN(hr);
}

_Check_return_ HRESULT ScrollViewer::put_MinHorizontalOffset(
    _In_ DOUBLE value)
{
    HRESULT hr = S_OK;

    m_xMinOffset = value;

    RRETURN(hr);
}

// Gets the value of the viewport width of the content.
IFACEMETHODIMP ScrollViewer::get_ViewportWidth(
    _Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;

    IFCPTR(pValue);
    *pValue = m_xViewport;

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ScrollViewer::put_ViewportWidth(
    _In_ DOUBLE value)
{
    HRESULT hr = S_OK;

    m_xViewport = value;
    IFC(ScrollViewerGenerated::put_ViewportWidth(value));

#ifdef DM_DEBUG
    if (DMSVv_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE)) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE | DMSVv_DBG) /*traceType*/, L"DMSVv[0x%p]: put_ViewportWidth new viewport width: m_xViewport=%f.", this, m_xViewport));
    }
#endif // DM_DEBUG

Cleanup:
    RRETURN(hr);
}

// Gets the value of the scrollable width of the content.
IFACEMETHODIMP ScrollViewer::get_ScrollableWidth(
    _Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;
    DOUBLE extent = 0.0;
    DOUBLE viewport = 0.0;

    IFCPTR(pValue);
    IFC(get_ExtentWidth(&extent));
    IFC(get_ViewportWidth(&viewport));

    *pValue = DoubleUtil::Max(m_xMinOffset, extent - viewport);

Cleanup:
    RRETURN(hr);
}

// Gets a value indicating ExtentWidth.
IFACEMETHODIMP ScrollViewer::get_ExtentWidth(
    _Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;

    IFCPTR(pValue);
    *pValue = m_xExtent;

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ScrollViewer::put_ExtentWidth(
    _In_ DOUBLE value)
{
    HRESULT hr = S_OK;

    m_xExtent = value;
    IFC(ScrollViewerGenerated::put_ExtentWidth(value));

Cleanup:
    RRETURN(hr);
}

// Gets the value of the vertical offset of the content.
IFACEMETHODIMP ScrollViewer::get_VerticalOffset(
    _Out_ DOUBLE* pValue)
{
    *pValue = MAX(m_yOffset, m_yMinOffset);

    RRETURN(S_OK);
}

_Check_return_ HRESULT ScrollViewer::put_VerticalOffset(
    _In_ DOUBLE value)
{
    HRESULT hr = S_OK;

    m_yOffset = MAX(value, m_yMinOffset);
    IFC(ScrollViewerGenerated::put_VerticalOffset(m_yOffset));

    if (!IsInDirectManipulation())
    {
        m_unboundVerticalOffset = m_yPixelOffset;
    }

#ifdef DM_DEBUG
    if (DMSVv_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE)) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE | DMSVv_DBG) /*traceType*/, L"DMSVv[0x%p]: put_VerticalOffset m_yOffset=%f.", this, m_yOffset));
    }
#endif // DM_DEBUG

Cleanup:
    RRETURN(hr);
}

// Gets the value of the minimal vertical offset of the content.
_Check_return_ HRESULT ScrollViewer::get_MinVerticalOffset(
    _Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;

    *pValue = m_yMinOffset;

    RRETURN(hr);
}

_Check_return_ HRESULT ScrollViewer::put_MinVerticalOffset(
    _In_ DOUBLE value)
{
    HRESULT hr = S_OK;

    m_yMinOffset = value;

    RRETURN(hr);
}

// Gets the value of the viewport height of the content.
IFACEMETHODIMP ScrollViewer::get_ViewportHeight(
    _Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;

    IFCPTR(pValue);
    *pValue = m_yViewport;

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ScrollViewer::put_ViewportHeight(
    _In_ DOUBLE value)
{
    HRESULT hr = S_OK;

    m_yViewport = value;
    IFC(ScrollViewerGenerated::put_ViewportHeight(value));

#ifdef DM_DEBUG
    if (DMSVv_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE)) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE | DMSVv_DBG) /*traceType*/, L"DMSVv[0x%p]: put_ViewportHeight new viewport height: m_yViewport=%f.", this, m_yViewport));
    }
#endif // DM_DEBUG

Cleanup:
    RRETURN(hr);
}

// Gets the value of the scrollable height of the content.
IFACEMETHODIMP ScrollViewer::get_ScrollableHeight(
    _Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;
    DOUBLE extent = 0.0;
    DOUBLE viewport = 0.0;

    IFCPTR(pValue);
    IFC(get_ExtentHeight(&extent));
    IFC(get_ViewportHeight(&viewport));

    *pValue = DoubleUtil::Max(m_yMinOffset, extent - viewport);

Cleanup:
    RRETURN(hr);
}

// Gets a value indicating the ExtentHeight.
IFACEMETHODIMP ScrollViewer::get_ExtentHeight(
    _Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;

    IFCPTR(pValue);
    *pValue = m_yExtent;

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ScrollViewer::put_ExtentHeight(
    _In_ DOUBLE value)
{
    HRESULT hr = S_OK;

    m_yExtent = value;
    IFC(ScrollViewerGenerated::put_ExtentHeight(value));

Cleanup:
    RRETURN(hr);
}

// Indicates whether the parent handles scrolling itself.
_Check_return_ HRESULT ScrollViewer::get_TemplatedParentHandlesScrolling(
    _Out_ BOOLEAN* pValue)
{
    HRESULT hr = S_OK;

    IFCPTR(pValue);
    *pValue = m_templatedParentHandlesScrolling;

Cleanup:
    RRETURN(hr);
}

void
ScrollViewer::put_TemplatedParentHandlesScrolling(
_In_ BOOLEAN value)
{
    m_templatedParentHandlesScrolling = value;
}

// Reference to the IScrollInfo implementation of ScrollContentPresenter child.
_Check_return_ HRESULT ScrollViewer::get_ScrollInfo(
    _Outptr_result_maybenull_ IScrollInfo** value)
{
    ctl::ComPtr<IScrollInfo> scrollInfo;

    IFCPTR_RETURN(value);
    IFC_RETURN(m_wrScrollInfo.As(&scrollInfo));
    IFC_RETURN(scrollInfo.MoveTo(value));

    return S_OK;
}

_Check_return_ HRESULT ScrollViewer::put_ScrollInfo(
    _In_opt_ IScrollInfo* value)
{
    IFC_RETURN(ctl::AsWeak(value, &m_wrScrollInfo));
    if (value)
    {
        IFC_RETURN(UpdateCanScroll(value));
    }

    return S_OK;
}

// Returns the reference to the touched dependency object set
// at the beginning of a touch-based manipulation.
_Check_return_ HRESULT ScrollViewer::get_PointedElement(
    _Outptr_result_maybenull_ IDependencyObject** ppValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IDependencyObject> spPointedElement;

    IFCPTR(ppValue);
    *ppValue = NULL;

    IFC(m_wrPointedElement.As(&spPointedElement));
    IFC(spPointedElement.MoveTo(ppValue));

Cleanup:
    RRETURN(hr);
}

// Sets the m_wrPointedElement field which represents
// the dependency object touched at the beginning of a
// touch-based manipulation.
_Check_return_ HRESULT ScrollViewer::put_PointedElement(
    _In_opt_ IDependencyObject* pValue)
{
    RRETURN(ctl::AsWeak(pValue, &m_wrPointedElement));
}

// Releases and unhooks template parts and their events
_Check_return_ HRESULT ScrollViewer::UnhookTemplate()
{
    ctl::ComPtr<IUIElement> spElementHorizontalScrollBarAsUIElement;
    ctl::ComPtr<IUIElement> spElementVerticalScrollBarAsUIElement;

    if (!IsRootScrollViewer() || IsRootScrollViewerAllowImplicitStyle())
    {
        // Cleanup any existing template parts
        if (auto peg = m_trElementHorizontalScrollBar.TryMakeAutoPeg())
        {
            m_trElementHorizontalScrollBar.Cast<ScrollBar>()->StopUseOfActualSizeAsExtent();
            IFC_RETURN(m_trElementHorizontalScrollBar->remove_Scroll(m_HorizontalScrollToken));
            ZeroMemory(&m_HorizontalScrollToken, sizeof(m_HorizontalScrollToken));
            IFC_RETURN(m_trElementHorizontalScrollBar.Cast<ScrollBar>()->remove_ThumbDragStarted(m_horizontalThumbDragStartedToken));
            ZeroMemory(&m_horizontalThumbDragStartedToken, sizeof(m_horizontalThumbDragStartedToken));
            IFC_RETURN(m_trElementHorizontalScrollBar.Cast<ScrollBar>()->remove_ThumbDragCompleted(m_horizontalThumbDragCompletedToken));
            ZeroMemory(&m_horizontalThumbDragCompletedToken, sizeof(m_horizontalThumbDragCompletedToken));

            spElementHorizontalScrollBarAsUIElement = m_trElementHorizontalScrollBar.AsOrNull<xaml::IUIElement>();
            if (spElementHorizontalScrollBarAsUIElement)
            {
                IFC_RETURN(spElementHorizontalScrollBarAsUIElement.Cast<IUIElement>()->remove_PointerEntered(m_horizontalScrollbarPointerEnteredToken));
                ZeroMemory(&m_horizontalScrollbarPointerEnteredToken, sizeof(m_horizontalScrollbarPointerEnteredToken));
                IFC_RETURN(spElementHorizontalScrollBarAsUIElement.Cast<IUIElement>()->remove_PointerExited(m_horizontalScrollbarPointerExitedToken));
                ZeroMemory(&m_horizontalScrollbarPointerExitedToken, sizeof(m_horizontalScrollbarPointerExitedToken));
            }
        }
        if (auto peg = m_trElementVerticalScrollBar.TryMakeAutoPeg())
        {
            m_trElementVerticalScrollBar.Cast<ScrollBar>()->StopUseOfActualSizeAsExtent();
            IFC_RETURN(m_trElementVerticalScrollBar->remove_Scroll(m_VerticalScrollToken));
            ZeroMemory(&m_VerticalScrollToken, sizeof(m_VerticalScrollToken));
            IFC_RETURN(m_trElementVerticalScrollBar.Cast<ScrollBar>()->remove_ThumbDragStarted(m_verticalThumbDragStartedToken));
            ZeroMemory(&m_verticalThumbDragStartedToken, sizeof(m_verticalThumbDragStartedToken));
            IFC_RETURN(m_trElementVerticalScrollBar.Cast<ScrollBar>()->remove_ThumbDragCompleted(m_verticalThumbDragCompletedToken));
            ZeroMemory(&m_verticalThumbDragCompletedToken, sizeof(m_verticalThumbDragCompletedToken));

            spElementVerticalScrollBarAsUIElement = m_trElementVerticalScrollBar.AsOrNull<xaml::IUIElement>();
            if (spElementVerticalScrollBarAsUIElement)
            {
                IFC_RETURN(spElementVerticalScrollBarAsUIElement.Cast<IUIElement>()->remove_PointerEntered(m_verticalScrollbarPointerEnteredToken));
                ZeroMemory(&m_verticalScrollbarPointerEnteredToken, sizeof(m_verticalScrollbarPointerEnteredToken));
                IFC_RETURN(spElementVerticalScrollBarAsUIElement.Cast<IUIElement>()->remove_PointerExited(m_verticalScrollbarPointerExitedToken));
                ZeroMemory(&m_verticalScrollbarPointerExitedToken, sizeof(m_verticalScrollbarPointerExitedToken));
            }
        }
        m_trElementRoot.Clear();
        m_trElementScrollContentPresenter.Clear();
        m_trElementHorizontalScrollBar.Clear();
        m_trElementVerticalScrollBar.Clear();
        m_tpElementScrollBarSeparator.Clear();
        m_trLayoutAdjustmentsForOcclusionsStoryboard.Clear();
    }

    return S_OK;
}

// Apply a template to the ScrollViewer.
IFACEMETHODIMP ScrollViewer::OnApplyTemplate() noexcept
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml::IDependencyObject> spElementRootAsDO;
    ctl::ComPtr<xaml::IDependencyObject> spElementScrollContentPresenterDO;
    ctl::ComPtr<xaml::IDependencyObject> spElementHorizontalScrollBarAsDO;
    ctl::ComPtr<xaml::IDependencyObject> spElementVerticalScrollBarAsDO;
    ctl::ComPtr<xaml::IDependencyObject> spElementScrollBarSeparatorAsDO;
    ctl::ComPtr<xaml_primitives::IScrollEventHandler> spHorizontalScrollHandler;
    ctl::ComPtr<xaml_primitives::IDragStartedEventHandler> spHorizontalThumbDragStartedHandler;
    ctl::ComPtr<xaml_primitives::IDragCompletedEventHandler> spHorizontalThumbDragCompletedHandler;
    ctl::ComPtr<xaml_primitives::IScrollEventHandler> spVerticalScrollHandler;
    ctl::ComPtr<xaml_primitives::IDragStartedEventHandler> spVerticalThumbDragStartedHandler;
    ctl::ComPtr<xaml_primitives::IDragCompletedEventHandler> spVerticalThumbDragCompletedHandler;
    ctl::ComPtr<xaml_input::IPointerEventHandler> spHorizontalScrollbarEnteredHandler;
    ctl::ComPtr<xaml_input::IPointerEventHandler> spHorizontalScrollbarExitedHandler;
    ctl::ComPtr<xaml_input::IPointerEventHandler> spVerticalScrollbarEnteredHandler;
    ctl::ComPtr<xaml_input::IPointerEventHandler> spVerticalScrollbarExitedHandler;
    ctl::ComPtr<IDependencyObject> spChild;
    ctl::ComPtr<IFrameworkElement> spChildAsFE;
    ctl::ComPtr<IVisualStateGroup> spGroup;
    ctl::ComPtr<wfc::IVector<xaml::VisualState*>> spVisualStates;
    ctl::ComPtr<wfc::IVector<xaml::VisualTransition*>> spVisualTransitions;
    ctl::ComPtr<wfc::IVector<xaml::VisualStateGroup*>> spChildVisualStateGroups;
    ctl::ComPtr<IVisualState> spState;
    ctl::ComPtr<IVisualTransition> spTransition;
    ctl::ComPtr<wf::IEventHandler<IInspectable*>> spNoIndicatorStateStoryboardCompletedHandler;
    ctl::ComPtr<wf::IEventHandler<IInspectable*>> spTouchIndicatorStateStoryboardCompletedHandler;
    ctl::ComPtr<wf::IEventHandler<IInspectable*>> spMouseIndicatorStateStoryboardCompletedHandler;
    ctl::ComPtr<wf::IEventHandler<IInspectable*>> spMouseIndicatorFullStateStoryboardCompletedHandler;
    ctl::ComPtr<IStoryboard> spNoIndicatorStateStoryboard;
    ctl::ComPtr<IStoryboard> spTouchIndicatorStateStoryboard;
    ctl::ComPtr<IStoryboard> spMouseIndicatorStateStoryboard;
    ctl::ComPtr<IStoryboard> spMouseIndicatorFullStateStoryboard;
    ctl::ComPtr<IStoryboard> spTransitionStoryboard;
    ctl::ComPtr<IFrameworkElement> spElementRoot;
    ctl::ComPtr<IScrollContentPresenter> spElementScrollContentPresenter;
    ctl::ComPtr<IScrollBar> spElementHorizontalScrollBar;
    ctl::ComPtr<IScrollBar> spElementVerticalScrollBar;
    ctl::ComPtr<IUIElement> spElementHorizontalScrollBarAsUIElement;
    ctl::ComPtr<IUIElement> spElementVerticalScrollBarAsUIElement;
    ctl::ComPtr<IUIElement> spElementScrollBarSeparator;
    INT childCount = 0;
    UINT nGroupCount = 0;
    UINT nStateCount = 0;
    UINT nTransitionCount = 0;
    EventRegistrationToken NoIndicatorStateStoryboardCompletedToken;
    EventRegistrationToken TouchIndicatorStateStoryboardCompletedToken;
    EventRegistrationToken MouseIndicatorStateStoryboardCompletedToken;
    EventRegistrationToken MouseIndicatorFullStateStoryboardCompletedToken;

#ifdef DM_DEBUG
    if (DMSVv_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE)) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE | DMSVv_DBG) /*traceType*/, L"DMSVv[0x%p]: OnApplyTemplate.", this));
    }
#endif // DM_DEBUG

    m_keepIndicatorsShowing = FALSE;

    IFC(UnhookTemplate());

    // no longer dragging a thumb
    m_isDraggingThumb = FALSE;

    m_hasNoIndicatorStateStoryboardCompletedHandler = false;

    // Apply the template to base classes
    IFC(ScrollViewerGenerated::OnApplyTemplate());

    if (!IsRootScrollViewer() || IsRootScrollViewerAllowImplicitStyle())
    {
        // Get the parts
        IFC(GetTemplateChild(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"Root")).Get(), &spElementRootAsDO));
        spElementRoot = spElementRootAsDO.AsOrNull<IFrameworkElement>();
        SetPtrValue(m_trElementRoot, spElementRoot);
        IFC(GetTemplateChild(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"ScrollContentPresenter")).Get(), &spElementScrollContentPresenterDO));
        spElementScrollContentPresenter = spElementScrollContentPresenterDO.AsOrNull<IScrollContentPresenter>();
        SetPtrValue(m_trElementScrollContentPresenter, spElementScrollContentPresenter);
        IFC(GetTemplateChild(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"HorizontalScrollBar")).Get(), &spElementHorizontalScrollBarAsDO));
        spElementHorizontalScrollBar = spElementHorizontalScrollBarAsDO.AsOrNull<xaml_primitives::IScrollBar>();
        spElementHorizontalScrollBarAsUIElement = spElementHorizontalScrollBarAsDO.AsOrNull<xaml::IUIElement>();
        SetPtrValue(m_trElementHorizontalScrollBar, spElementHorizontalScrollBar);
        IFC(GetTemplateChild(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"VerticalScrollBar")).Get(), &spElementVerticalScrollBarAsDO));
        spElementVerticalScrollBar = spElementVerticalScrollBarAsDO.AsOrNull<xaml_primitives::IScrollBar>();
        spElementVerticalScrollBarAsUIElement = spElementVerticalScrollBarAsDO.AsOrNull<xaml::IUIElement>();
        SetPtrValue(m_trElementVerticalScrollBar, spElementVerticalScrollBar);
        IFC(GetTemplateChild(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"ScrollBarSeparator")).Get(), &spElementScrollBarSeparatorAsDO));
        spElementScrollBarSeparator = spElementScrollBarSeparatorAsDO.AsOrNull<xaml::IUIElement>();
        SetPtrValue(m_tpElementScrollBarSeparator, spElementScrollBarSeparator);
    }

    // Attach the event handlers
    if (m_trElementHorizontalScrollBar)
    {
        if (m_trElementScrollContentPresenter && m_trElementScrollContentPresenter.Cast<ScrollContentPresenter>()->IsChildActualWidthUsedAsExtent())
        {
            m_trElementHorizontalScrollBar.Cast<ScrollBar>()->StartUseOfActualSizeAsExtent();
        }

        spHorizontalScrollHandler.Attach(
            new ClassMemberEventHandler <
            ScrollViewer,
            xaml_controls::IScrollViewer,
            xaml_primitives::IScrollEventHandler,
            IInspectable,
            xaml_primitives::IScrollEventArgs >(this, &ScrollViewer::OnHorizontalScrollBarScroll));
        IFC(m_trElementHorizontalScrollBar->add_Scroll(spHorizontalScrollHandler.Get(), &m_HorizontalScrollToken));

        spHorizontalThumbDragStartedHandler.Attach(
            new ClassMemberEventHandler <
            ScrollViewer,
            xaml_controls::IScrollViewer,
            xaml_primitives::IDragStartedEventHandler,
            IInspectable,
            xaml_primitives::IDragStartedEventArgs >(this, &ScrollViewer::OnScrollBarThumbDragStarted));
        IFC(m_trElementHorizontalScrollBar.Cast<ScrollBar>()->add_ThumbDragStarted(spHorizontalThumbDragStartedHandler.Get(), &m_horizontalThumbDragStartedToken));

        spHorizontalThumbDragCompletedHandler.Attach(
            new ClassMemberEventHandler <
            ScrollViewer,
            xaml_controls::IScrollViewer,
            xaml_primitives::IDragCompletedEventHandler,
            IInspectable,
            xaml_primitives::IDragCompletedEventArgs >(this, &ScrollViewer::OnScrollBarThumbDragCompleted));
        IFC(m_trElementHorizontalScrollBar.Cast<ScrollBar>()->add_ThumbDragCompleted(spHorizontalThumbDragCompletedHandler.Get(), &m_horizontalThumbDragCompletedToken));

        if (spElementHorizontalScrollBarAsUIElement)
        {
            spHorizontalScrollbarEnteredHandler.Attach(
                new ClassMemberEventHandler <
                ScrollViewer,
                xaml_controls::IScrollViewer,
                xaml_input::IPointerEventHandler,
                IInspectable,
                xaml_input::IPointerRoutedEventArgs>(this, &ScrollViewer::OnHorizontalScrollbarPointerEntered));
            IFC(spElementHorizontalScrollBarAsUIElement->add_PointerEntered(spHorizontalScrollbarEnteredHandler.Get(), &m_horizontalScrollbarPointerEnteredToken));

            spHorizontalScrollbarExitedHandler.Attach(
                new ClassMemberEventHandler <
                ScrollViewer,
                xaml_controls::IScrollViewer,
                xaml_input::IPointerEventHandler,
                IInspectable,
                xaml_input::IPointerRoutedEventArgs>(this, &ScrollViewer::OnHorizontalScrollbarPointerExited));
            IFC(spElementHorizontalScrollBarAsUIElement->add_PointerExited(spHorizontalScrollbarExitedHandler.Get(), &m_horizontalScrollbarPointerExitedToken));
        }

        IFC(RefreshScrollBarIsIgnoringUserInput(TRUE /*isForHorizontalOrientation*/));
    }

    if (m_trElementVerticalScrollBar)
    {
        if (m_trElementScrollContentPresenter && m_trElementScrollContentPresenter.Cast<ScrollContentPresenter>()->IsChildActualHeightUsedAsExtent())
        {
            m_trElementVerticalScrollBar.Cast<ScrollBar>()->StartUseOfActualSizeAsExtent();
        }

        spVerticalScrollHandler.Attach(
            new ClassMemberEventHandler <
            ScrollViewer,
            xaml_controls::IScrollViewer,
            xaml_primitives::IScrollEventHandler,
            IInspectable,
            xaml_primitives::IScrollEventArgs >(this, &ScrollViewer::OnVerticalScrollBarScroll));
        IFC(m_trElementVerticalScrollBar->add_Scroll(spVerticalScrollHandler.Get(), &m_VerticalScrollToken));

        spVerticalThumbDragStartedHandler.Attach(
            new ClassMemberEventHandler <
            ScrollViewer,
            xaml_controls::IScrollViewer,
            xaml_primitives::IDragStartedEventHandler,
            IInspectable,
            xaml_primitives::IDragStartedEventArgs >(this, &ScrollViewer::OnScrollBarThumbDragStarted));
        IFC(m_trElementVerticalScrollBar.Cast<ScrollBar>()->add_ThumbDragStarted(spVerticalThumbDragStartedHandler.Get(), &m_verticalThumbDragStartedToken));

        spVerticalThumbDragCompletedHandler.Attach(
            new ClassMemberEventHandler <
            ScrollViewer,
            xaml_controls::IScrollViewer,
            xaml_primitives::IDragCompletedEventHandler,
            IInspectable,
            xaml_primitives::IDragCompletedEventArgs >(this, &ScrollViewer::OnScrollBarThumbDragCompleted));
        IFC(m_trElementVerticalScrollBar.Cast<ScrollBar>()->add_ThumbDragCompleted(spVerticalThumbDragCompletedHandler.Get(), &m_verticalThumbDragCompletedToken));

        if (spElementVerticalScrollBarAsUIElement)
        {
            spVerticalScrollbarEnteredHandler.Attach(
                new ClassMemberEventHandler <
                ScrollViewer,
                xaml_controls::IScrollViewer,
                xaml_input::IPointerEventHandler,
                IInspectable,
                xaml_input::IPointerRoutedEventArgs>(this, &ScrollViewer::OnVerticalScrollbarPointerEntered));
            IFC(spElementVerticalScrollBarAsUIElement->add_PointerEntered(spVerticalScrollbarEnteredHandler.Get(), &m_verticalScrollbarPointerEnteredToken));

            spVerticalScrollbarExitedHandler.Attach(
                new ClassMemberEventHandler <
                ScrollViewer,
                xaml_controls::IScrollViewer,
                xaml_input::IPointerEventHandler,
                IInspectable,
                xaml_input::IPointerRoutedEventArgs>(this, &ScrollViewer::OnVerticalScrollbarPointerExited));
            IFC(spElementVerticalScrollBarAsUIElement->add_PointerExited(spVerticalScrollbarExitedHandler.Get(), &m_verticalScrollbarPointerExitedToken));
        }

        IFC(RefreshScrollBarIsIgnoringUserInput(FALSE /*isForHorizontalOrientation*/));
    }

    if (m_trElementScrollContentPresenter)
    {
        ScrollContentPresenter* scrollContentPresenter = m_trElementScrollContentPresenter.Cast<ScrollContentPresenter>();
        BOOLEAN oldCanContentRenderOutsideBounds = FALSE;
        BOOLEAN newCanContentRenderOutsideBounds = FALSE;
        FLOAT zoomFactor = 1.0;

        IFC(get_ZoomFactor(&zoomFactor));
        IFC(scrollContentPresenter->SetZoomFactor(zoomFactor));
        IFC(SetScrollContentPresenterHeaders());

        // TODO - Bug 17637741 - Consider using template binding in generic.xaml for ScrollViewer's ScrollContentPresenter
        //   CanContentRenderOutsideBounds="{TemplateBinding CanContentRenderOutsideBounds}"
        // and updating all DComp master files affected by adding
        //   <Property Name="CanContentRenderOutsideBounds" Value="False" />
        IFC(get_CanContentRenderOutsideBounds(&newCanContentRenderOutsideBounds));
        IFC(scrollContentPresenter->get_CanContentRenderOutsideBounds(&oldCanContentRenderOutsideBounds));
        if (oldCanContentRenderOutsideBounds != newCanContentRenderOutsideBounds)
        {
            IFC(scrollContentPresenter->put_CanContentRenderOutsideBounds(newCanContentRenderOutsideBounds));
        }
    }

    IFC(VisualTreeHelper::GetChildrenCountStatic(this, &childCount));
    if (childCount > 0)
    {
        IFC(VisualTreeHelper::GetChildStatic(this, 0, &spChild));
        spChildAsFE = spChild.AsOrNull<IFrameworkElement>();
        if (spChildAsFE)
        {
            ctl::ComPtr<VisualStateManagerFactory> spFactory;

            IFC(ctl::make<VisualStateManagerFactory>(&spFactory));

            IFC(spFactory->GetVisualStateGroups(spChildAsFE.Get(), &spChildVisualStateGroups));

            if (spChildVisualStateGroups)
            {
                wrl_wrappers::HString strStateName;
                wrl_wrappers::HStringReference strNoIndicatorStateName(STR_LEN_PAIR(L"NoIndicator"));
                wrl_wrappers::HStringReference strTouchIndicatorStateName(STR_LEN_PAIR(L"TouchIndicator"));
                wrl_wrappers::HStringReference strMouseIndicatorStateName(STR_LEN_PAIR(L"MouseIndicator"));
                wrl_wrappers::HStringReference strMouseIndicatorFullStateName(STR_LEN_PAIR(L"MouseIndicatorFull"));

                IFC(spChildVisualStateGroups->get_Size(&nGroupCount));
                for (UINT i = 0; i < nGroupCount; ++i)
                {
                    IFC(spChildVisualStateGroups->GetAt(i, &spGroup));
                    IFC(spGroup.Cast<VisualStateGroup>()->get_States(&spVisualStates));
                    if (spVisualStates)
                    {
                        IFC(spVisualStates->get_Size(&nStateCount));
                        for (UINT j = 0; j < nStateCount; ++j)
                        {
                            IFC(spVisualStates->GetAt(j, &spState));

                            IFC(spState->get_Name(strStateName.ReleaseAndGetAddressOf()));

                            if (strStateName == strNoIndicatorStateName)
                            {
                                IFC(spState->get_Storyboard(&spNoIndicatorStateStoryboard));
                                if (spNoIndicatorStateStoryboard)
                                {
                                    spNoIndicatorStateStoryboardCompletedHandler.Attach(
                                        new ClassMemberEventHandler <
                                        ScrollViewer,
                                        IScrollViewer,
                                        wf::IEventHandler<IInspectable*>,
                                        IInspectable,
                                        IInspectable >(this, &ScrollViewer::NoIndicatorStateStoryboardCompleted));

                                    IFC(spNoIndicatorStateStoryboard.Cast<Storyboard>()->add_Completed(
                                        spNoIndicatorStateStoryboardCompletedHandler.Get(), &NoIndicatorStateStoryboardCompletedToken));

                                    m_hasNoIndicatorStateStoryboardCompletedHandler = true;
                                }
                            }
                            else if (strStateName == strTouchIndicatorStateName)
                            {
                                IFC(spState->get_Storyboard(&spTouchIndicatorStateStoryboard));
                                if (spTouchIndicatorStateStoryboard)
                                {
                                    spTouchIndicatorStateStoryboardCompletedHandler.Attach(
                                        new ClassMemberEventHandler <
                                        ScrollViewer,
                                        IScrollViewer,
                                        wf::IEventHandler<IInspectable*>,
                                        IInspectable,
                                        IInspectable >(this, &ScrollViewer::IndicatorStateStoryboardCompleted));

                                    IFC(spTouchIndicatorStateStoryboard.Cast<Storyboard>()->add_Completed(
                                        spTouchIndicatorStateStoryboardCompletedHandler.Get(), &TouchIndicatorStateStoryboardCompletedToken));

                                    // The PVL animation inside the storyboard is essential for the pan indicator display
                                    IFC(spTouchIndicatorStateStoryboard.Cast<Storyboard>()->put_IsEssential(TRUE));
                                }
                            }
                            else if (strStateName == strMouseIndicatorStateName)
                            {
                                IFC(spState->get_Storyboard(&spMouseIndicatorStateStoryboard));
                                if (spMouseIndicatorStateStoryboard)
                                {
                                    spMouseIndicatorStateStoryboardCompletedHandler.Attach(
                                        new ClassMemberEventHandler <
                                        ScrollViewer,
                                        IScrollViewer,
                                        wf::IEventHandler<IInspectable*>,
                                        IInspectable,
                                        IInspectable >(this, &ScrollViewer::IndicatorStateStoryboardCompleted));

                                    IFC(spMouseIndicatorStateStoryboard.Cast<Storyboard>()->add_Completed(
                                        spMouseIndicatorStateStoryboardCompletedHandler.Get(), &MouseIndicatorStateStoryboardCompletedToken));
                                }
                            }
                            else if (strStateName == strMouseIndicatorFullStateName)
                            {
                                IFC(spState->get_Storyboard(&spMouseIndicatorFullStateStoryboard));
                                if (spMouseIndicatorFullStateStoryboard)
                                {
                                    spMouseIndicatorFullStateStoryboardCompletedHandler.Attach(
                                        new ClassMemberEventHandler <
                                        ScrollViewer,
                                        IScrollViewer,
                                        wf::IEventHandler<IInspectable*>,
                                        IInspectable,
                                        IInspectable >(this, &ScrollViewer::IndicatorStateStoryboardCompleted));

                                    IFC(spMouseIndicatorFullStateStoryboard.Cast<Storyboard>()->add_Completed(
                                        spMouseIndicatorFullStateStoryboardCompletedHandler.Get(), &MouseIndicatorFullStateStoryboardCompletedToken));
                                }
                            }
                        }
                    }

                    // The following transitions which used as timer are essential for the integrity of the control
                    // Even when the animations are disabled in the system, we will allow them to run
                    IFC(spGroup.Cast<VisualStateGroup>()->get_Transitions(&spVisualTransitions));
                    if (spVisualTransitions)
                    {
                        IFC(spVisualTransitions->get_Size(&nTransitionCount));
                        for (UINT j = 0; j < nTransitionCount; ++j)
                        {
                            IFC(spVisualTransitions->GetAt(j, &spTransition));
                            IFC(spTransition->get_To(strStateName.ReleaseAndGetAddressOf()));
                            if (strStateName == strNoIndicatorStateName)
                            {
                                IFC(spTransition->get_From(strStateName.ReleaseAndGetAddressOf()));
                                if (strStateName == strTouchIndicatorStateName || strStateName == strMouseIndicatorStateName || strStateName == strMouseIndicatorFullStateName)
                                {
                                    IFC(spTransition->get_Storyboard(&spTransitionStoryboard));
                                    if (spTransitionStoryboard)
                                    {
                                        IFC(spTransitionStoryboard.Cast<Storyboard>()->put_IsEssential(TRUE));
                                    }
                                }
                            }
                        }
                    }

                    if (spNoIndicatorStateStoryboard && spTouchIndicatorStateStoryboard && spMouseIndicatorStateStoryboard && spMouseIndicatorFullStateStoryboard)
                    {
                        break;
                    }
                }
            }
        }
    }

    // hijack this flag to also tell the ContentPresenter that it is under semanticzoom control.
    if (m_ignoreSemanticZoomNavigationInput)
    {
        IFC(RegisterAsSemanticZoomHost());
    }

    IFC(UpdateVisualState(FALSE));

Cleanup:
    RRETURN(hr);
}

// Handler for when the NoIndicator state's storyboard completes animating.
_Check_return_ HRESULT
ScrollViewer::NoIndicatorStateStoryboardCompleted(
_In_opt_ IInspectable* pUnused1,
_In_opt_ IInspectable* pUnused2)
{
    ASSERT(m_hasNoIndicatorStateStoryboardCompletedHandler);
    m_showingMouseIndicators = FALSE;

    RRETURN(S_OK);
}

// Create ScrollViewerAutomationPeer to represent the ScrollViewer.
IFACEMETHODIMP ScrollViewer::OnCreateAutomationPeer(
    _Outptr_ xaml_automation_peers::IAutomationPeer** ppAutomationPeer)
{
    IFCPTR_RETURN(ppAutomationPeer);
    *ppAutomationPeer = nullptr;

    ctl::ComPtr<xaml_automation_peers::IAutomationPeer> spAutomationPeer;

    // If an internal AutomationPeer is injected use that. One such example is CalendarScrollViewerAutomationPeer
    // uses for various ScrollViewers for different CalendarViews.
    IFC_RETURN(ScrollViewerGenerated::OnCreateAutomationPeer(&spAutomationPeer));

    if (spAutomationPeer != nullptr)
    {
        KnownTypeIndex typeIndex = spAutomationPeer.Cast<AutomationPeer>()->GetTypeIndex();

        // Check if the AP is the default AP for named or landmark container and if yes then do not use it.
        if (typeIndex == KnownTypeIndex::NamedContainerAutomationPeer || typeIndex == KnownTypeIndex::LandmarkTargetAutomationPeer)
        {
            spAutomationPeer = nullptr;
        }
    }

    if (spAutomationPeer == nullptr)
    {
        ctl::ComPtr<xaml_automation_peers::IScrollViewerAutomationPeer> spScrollViewerAutomationPeer;
        ctl::ComPtr<xaml_automation_peers::IScrollViewerAutomationPeerFactory> spScrollViewerAPFactory;
        ctl::ComPtr<IActivationFactory> spActivationFactory;
        ctl::ComPtr<IInspectable> spInner;

        spActivationFactory.Attach(ctl::ActivationFactoryCreator<DirectUI::ScrollViewerAutomationPeerFactory>::CreateActivationFactory());
        IFC_RETURN(spActivationFactory.As(&spScrollViewerAPFactory));

        IFC_RETURN(spScrollViewerAPFactory.Cast<ScrollViewerAutomationPeerFactory>()->CreateInstanceWithOwner(this,
            nullptr,
            &spInner,
            &spScrollViewerAutomationPeer));

        // If this ScrollViewer is a templated part of the ItemsControl, we want ItemsControl to be assigned as its EventsSource.
        // This means anytime SV needs to be returned to UIA, ItemsControl shall be returned and SVAP shall never exist on UIA
        // client space. All of this happens anyway once scrolling pattern is queried from ItemsControlAP or GetChildrenCore is
        // called. In some cases bottom up approach triggers first and hence we need to set EventsSource at the earliest in case
        // SV is hit-test target.
        ctl::ComPtr<DependencyObject> spTemplatedParent;
        IFC_RETURN(get_TemplatedParent(&spTemplatedParent));
        auto spTemplatedParentAsItemsControl = spTemplatedParent.AsOrNull<IItemsControl>();

        if (spScrollViewerAutomationPeer && spTemplatedParentAsItemsControl)
        {
            auto spTemplatedParentAsFrameworkElement = spTemplatedParentAsItemsControl.AsOrNull<IFrameworkElement>();

            ctl::ComPtr<xaml_automation_peers::IAutomationPeer> spAutomationPeerForItemsControl;
            IFC_RETURN(spTemplatedParentAsFrameworkElement.Cast<FrameworkElement>()->GetOrCreateAutomationPeer(&spAutomationPeerForItemsControl));
            auto spItemsControlAutomationPeer = spAutomationPeerForItemsControl.AsOrNull<IItemsControlAutomationPeer>();

            // This check is to target only ItemsControl that are using ItemsControlAutomationPeer as their AP.
            // Custom Control could override OnCreateAutomationPeer for ItemsControl and provide their own APs.
            if (spItemsControlAutomationPeer)
            {
                IFC_RETURN(spScrollViewerAutomationPeer.Cast<ScrollViewerAutomationPeer>()->put_EventsSource(spAutomationPeerForItemsControl.Get()));
            }
        }

        IFC_RETURN(spScrollViewerAutomationPeer.As(&spAutomationPeer));
    }

    if (spAutomationPeer != nullptr)
    {
        IFC_RETURN(spAutomationPeer.CopyTo(ppAutomationPeer));
    }

    return S_OK;
}

// Scrolls the view in the specified direction.
_Check_return_ HRESULT ScrollViewer::ScrollInDirection(
    _In_ wsy::VirtualKey key,
    _In_ BOOLEAN animate)
{
    HRESULT hr = S_OK;

    if (animate)
    {
        // Let DManip animate the scroll within a ListViewBase header or footer.

        // No special processing is required here for right-to-left scenarios,
        // CInputServices::ProcessInputMessageWithDirectManipulation does it instead.
        // For PageUp/Down, Home and End keys though, that method must ignore the RightToLeft
        // flow direction, otherwise a move to the opposite direction is performed.
        BOOLEAN isHandled = FALSE;
        IFC(ProcessInputMessage(key == wsy::VirtualKey_PageUp ||
                                key == wsy::VirtualKey_PageDown ||
                                key == wsy::VirtualKey_Home ||
                                key == wsy::VirtualKey_End /*ignoreFlowDirection*/,
                                isHandled));
    }
    else
    {
        BOOLEAN invert = FALSE;
        xaml::FlowDirection direction = xaml::FlowDirection_LeftToRight;

        IFC(get_FlowDirection(&direction));
        invert = direction == xaml::FlowDirection_RightToLeft;

        switch (key)
        {
        case wsy::VirtualKey_Up:
            IFC(LineUp());
            break;
        case wsy::VirtualKey_Down:
            IFC(LineDown());
            break;
        case wsy::VirtualKey_Left:
            if (invert)
            {
                IFC(LineRight());
            }
            else
            {
                IFC(LineLeft());
            }
            break;
        case wsy::VirtualKey_Right:
            if (invert)
            {
                IFC(LineLeft());
            }
            else
            {
                IFC(LineRight());
            }
            break;
        case wsy::VirtualKey_PageUp:
            IFC(PageUp());
            break;
        case wsy::VirtualKey_PageDown:
            IFC(PageDown());
            break;
        case wsy::VirtualKey_Home:
            IFC(PageHome());
            break;
        case wsy::VirtualKey_End:
            IFC(PageEnd());
            break;
        default:
            // Do nothing
            break;
        }
    }

Cleanup:
    RRETURN(hr);
}

// Public and deprecated version of ScrollToHorizontalOffsetInternal.
_Check_return_ HRESULT ScrollViewer::ScrollToHorizontalOffsetImpl(
    _In_ DOUBLE offset)
{
#ifdef DM_DEBUG
    if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/, L"DMSV[0x%p]:  ScrollToHorizontalOffsetImpl offset=%f.", this, offset));
    }
#endif // DM_DEBUG

    RRETURN(ScrollToHorizontalOffsetInternal(offset));
}

// Scrolls the content within the ScrollViewer to the specified
// horizontal offset position.
_Check_return_ HRESULT ScrollViewer::ScrollToHorizontalOffsetInternal(
    _In_ DOUBLE offset)
{
#ifdef DM_DEBUG
    if (DMSVv_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE)) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE | DMSVv_DBG) /*traceType*/,
            L"DMSVv[0x%p]: ScrollToHorizontalOffsetInternal offset=%f.", this, offset));
    }
#endif // DM_DEBUG

    RRETURN(HandleHorizontalScroll(xaml_primitives::ScrollEventType_ThumbPosition, offset));
}

// Public and deprecated version of ScrollToVerticalOffsetInternal.
_Check_return_ HRESULT ScrollViewer::ScrollToVerticalOffsetImpl(
    _In_ DOUBLE offset)
{
#ifdef DM_DEBUG
    if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/, L"DMSV[0x%p]:  ScrollToVerticalOffsetImpl offset=%f.", this, offset));
    }
#endif // DM_DEBUG

    RRETURN(ScrollToVerticalOffsetInternal(offset));
}

// Scrolls the content within the ScrollViewer to the specified vertical
// offset position.
_Check_return_ HRESULT ScrollViewer::ScrollToVerticalOffsetInternal(
    _In_ DOUBLE offset)
{
#ifdef DM_DEBUG
    if (DMSVv_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE)) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE | DMSVv_DBG) /*traceType*/,
            L"DMSVv[0x%p]: ScrollToVerticalOffsetInternal offset=%f.", this, offset));
    }
#endif // DM_DEBUG

    RRETURN(HandleVerticalScroll(xaml_primitives::ScrollEventType_ThumbPosition, offset));
}

// Scroll content by one page to the left.
_Check_return_ HRESULT ScrollViewer::PageLeft()
{
    RRETURN(HandleHorizontalScroll(xaml_primitives::ScrollEventType_LargeDecrement));
}

// Scroll content by one line to the left.
_Check_return_ HRESULT ScrollViewer::LineLeft()
{
    RRETURN(HandleHorizontalScroll(xaml_primitives::ScrollEventType_SmallDecrement));
}

// Scroll content by one line to the right.
_Check_return_ HRESULT ScrollViewer::LineRight()
{
    RRETURN(HandleHorizontalScroll(xaml_primitives::ScrollEventType_SmallIncrement));
}

// Scroll content by one page to the right.
_Check_return_ HRESULT ScrollViewer::PageRight()
{
    RRETURN(HandleHorizontalScroll(xaml_primitives::ScrollEventType_LargeIncrement));
}

// Scroll content by one page to the top.
_Check_return_ HRESULT ScrollViewer::PageUp()
{
    RRETURN(HandleVerticalScroll(xaml_primitives::ScrollEventType_LargeDecrement));
}

// Scroll content by one line to the top.
_Check_return_ HRESULT ScrollViewer::LineUp()
{
    RRETURN(HandleVerticalScroll(xaml_primitives::ScrollEventType_SmallDecrement));
}

// Scroll content by one line to the bottom.
_Check_return_ HRESULT ScrollViewer::LineDown()
{
    RRETURN(HandleVerticalScroll(xaml_primitives::ScrollEventType_SmallIncrement));
}

// Scroll content by one page to the bottom.
_Check_return_ HRESULT ScrollViewer::PageDown()
{
    RRETURN(HandleVerticalScroll(xaml_primitives::ScrollEventType_LargeIncrement));
}

// Scroll content to the beginning.
_Check_return_ HRESULT ScrollViewer::PageHome()
{
    RRETURN(HandleHorizontalScroll(xaml_primitives::ScrollEventType_First));
}

// Scroll content to the end.
_Check_return_ HRESULT ScrollViewer::PageEnd()
{
    RRETURN(HandleHorizontalScroll(xaml_primitives::ScrollEventType_Last));
}

// Measure the content.
IFACEMETHODIMP ScrollViewer::MeasureOverride(
    // Measurement constraints, a control cannot return a size
    // larger than the constraint.
    _In_ wf::Size availableSize,
    // The desired size of the control.
    _Out_ wf::Size* pDesired) noexcept
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IUIElement> spChild;
    ctl::ComPtr<IScrollInfo> spScrollInfo;
    wf::Size desiredSize = {};
    xaml_controls::ScrollBarVisibility vsbv = xaml_controls::ScrollBarVisibility_Disabled;
    xaml_controls::ScrollBarVisibility hsbv = xaml_controls::ScrollBarVisibility_Disabled;
    BOOLEAN vsbAuto = FALSE;
    BOOLEAN hsbAuto = FALSE;
    xaml::Visibility vv = xaml::Visibility_Visible;
    xaml::Visibility hv = xaml::Visibility_Visible;

    IFCPTR(pDesired);

    m_latestAvailableSize = availableSize;
    m_inMeasure = FALSE;
    IFC(GetVisualChild(&spChild));
    if (!spChild)
    {
        desiredSize.Width = desiredSize.Height = 0;
        *pDesired = desiredSize;
        goto Cleanup;
    }

    IFC(get_ScrollInfo(&spScrollInfo));
    IFC(get_VerticalScrollBarVisibility(&vsbv));
    IFC(get_HorizontalScrollBarVisibility(&hsbv));
    vsbAuto = vsbv == xaml_controls::ScrollBarVisibility_Auto;
    hsbAuto = hsbv == xaml_controls::ScrollBarVisibility_Auto;
    vv = (vsbv == xaml_controls::ScrollBarVisibility_Visible) ? xaml::Visibility_Visible : xaml::Visibility_Collapsed;
    hv = (hsbv == xaml_controls::ScrollBarVisibility_Visible) ? xaml::Visibility_Visible : xaml::Visibility_Collapsed;

    m_inMeasure = TRUE;

    // Set computed visibility property of ScrollBars.
    if (m_scrollVisibilityY != vv)
    {
        m_scrollVisibilityY = vv;
        IFC(put_ComputedVerticalScrollBarVisibility(m_scrollVisibilityY));
    }
    if (m_scrollVisibilityX != hv)
    {
        m_scrollVisibilityX = hv;
        IFC(put_ComputedHorizontalScrollBarVisibility(m_scrollVisibilityX));
    }

    if (spScrollInfo)
    {
        IFC(spScrollInfo->put_CanHorizontallyScroll(hsbv != xaml_controls::ScrollBarVisibility_Disabled));
        IFC(spScrollInfo->put_CanVerticallyScroll(vsbv != xaml_controls::ScrollBarVisibility_Disabled));
    }

    // Measure our visual tree.
    IFC(spChild->Measure(availableSize));

    // it could now be here as a result of visual template expansion that
    // happens during Measure
    IFC(get_ScrollInfo(&spScrollInfo));

    if (spScrollInfo && (vsbAuto || hsbAuto))
    {
        DOUBLE viewportWidth = 0.0;
        DOUBLE viewportHeight = 0.0;

        IFC(spScrollInfo->get_ViewportWidth(&viewportWidth));
        IFC(spScrollInfo->get_ViewportHeight(&viewportHeight));

        if (viewportWidth > 0.0 && viewportHeight > 0.0)
        {
            BOOLEAN makeHorizontalBarVisible = FALSE;
            BOOLEAN makeVerticalBarVisible = FALSE;
            DOUBLE extent = 0.0;
            DOUBLE minOffset = 0.0;

            // roundingStep is added to the viewport size to avoid rare infinite layout cycles when the ScrollBar visibility is Auto and the extent & viewport are within a physical pixel.
            const float roundingStep = 1.0f / RootScale::GetRasterizationScaleForElement(GetHandle());
            // extent, minOffset, viewportWidth & viewportHeight are potentially rounded values like FrameworkElement.ActualWidth, thus the use of the global scale.
            // get_UseLayoutRounding is intentionally not used because the ScrollViewer, ScrollContentPresenter and Content may use different settings, so the guard is simple and more universal.
            // The unfortunate consequence is that the Auto ScrollBar will not be shown when the extent is up to a physical pixel larger than the viewport and the user will not be able to scroll
            // to it using a scrollbar. Other means like keyboard and touch are still available to do so. This workaround is applied because the root cause of the sub-pixel discrepancy is unknown.

            IFC(spScrollInfo->get_ExtentWidth(&extent));
            IFC(spScrollInfo->get_MinHorizontalOffset(&minOffset));
            makeHorizontalBarVisible = hsbAuto && extent - minOffset > viewportWidth + roundingStep;

            IFC(spScrollInfo->get_ExtentHeight(&extent));
            IFC(spScrollInfo->get_MinVerticalOffset(&minOffset));
            makeVerticalBarVisible = vsbAuto && extent - minOffset > viewportHeight + roundingStep;

            if (makeHorizontalBarVisible && m_scrollVisibilityX != xaml::Visibility_Visible)
            {
                m_scrollVisibilityX = xaml::Visibility_Visible;
                IFC(put_ComputedHorizontalScrollBarVisibility(m_scrollVisibilityX));
            }

            if (makeVerticalBarVisible && m_scrollVisibilityY != xaml::Visibility_Visible)
            {
                m_scrollVisibilityY = xaml::Visibility_Visible;
                IFC(put_ComputedVerticalScrollBarVisibility(m_scrollVisibilityY));
            }

            if (makeHorizontalBarVisible || makeVerticalBarVisible)
            {
                // Remeasure our visual tree.
                m_inChildInvalidateMeasure = TRUE;
                IFC(spChild->InvalidateMeasure());
                IFC(spChild->Measure(availableSize));
                m_inChildInvalidateMeasure = FALSE;
            }

            // If both are Auto, then appearance of one scrollbar may causes
            // appearance of another.  If we don't re-check here, we may get some
            // part of content covered by auto scrollbar and can never reach to it
            // since another scrollbar may not appear (in cases when
            // viewport==extent).
            if (hsbAuto && vsbAuto && (makeHorizontalBarVisible != makeVerticalBarVisible))
            {
                IFC(spScrollInfo->get_ViewportWidth(&viewportWidth));
                IFC(spScrollInfo->get_ViewportHeight(&viewportHeight));

                if (viewportWidth > 0.0 && viewportHeight > 0.0)
                {
                    BOOLEAN makeHorizontalBarVisible2 = FALSE;
                    BOOLEAN makeVerticalBarVisible2 = FALSE;

                    IFC(spScrollInfo->get_ExtentWidth(&extent));
                    IFC(spScrollInfo->get_MinHorizontalOffset(&minOffset));
                    makeHorizontalBarVisible2 = !makeHorizontalBarVisible && extent - minOffset > viewportWidth + roundingStep;

                    IFC(spScrollInfo->get_ExtentHeight(&extent));
                    IFC(spScrollInfo->get_MinVerticalOffset(&minOffset));
                    makeVerticalBarVisible2 = !makeVerticalBarVisible && extent - minOffset > viewportHeight + roundingStep;

                    if (makeHorizontalBarVisible2)
                    {
                        if (m_scrollVisibilityX != xaml::Visibility_Visible)
                        {
                            m_scrollVisibilityX = xaml::Visibility_Visible;
                            IFC(put_ComputedHorizontalScrollBarVisibility(m_scrollVisibilityX));
                        }
                    }
                    else if (makeVerticalBarVisible2) //only one can be true
                    {
                        if (m_scrollVisibilityY != xaml::Visibility_Visible)
                        {
                            m_scrollVisibilityY = xaml::Visibility_Visible;
                            IFC(put_ComputedVerticalScrollBarVisibility(m_scrollVisibilityY));
                        }
                    }

                    if (makeHorizontalBarVisible2 || makeVerticalBarVisible2)
                    {
                        // Remeasure our visual tree.
                        m_inChildInvalidateMeasure = TRUE;
                        IFC(spChild->InvalidateMeasure());
                        IFC(spChild->Measure(availableSize));
                        m_inChildInvalidateMeasure = FALSE;
                    }
                }
            }
        }
    }

    if (m_tpElementScrollBarSeparator)
    {
        if (AreBothScrollBarsVisible())
        {
            IFC(m_tpElementScrollBarSeparator->put_Visibility(xaml::Visibility_Visible));
        }
        else
        {
            IFC(m_tpElementScrollBarSeparator->put_Visibility(xaml::Visibility_Collapsed));
        }
    }

    IFC(spChild->get_DesiredSize(&desiredSize));

    *pDesired = desiredSize;

Cleanup:
    m_inMeasure = FALSE;

#ifdef DM_DEBUG
    if (DMSVv_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE)) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE | DMSVv_DBG) /*traceType*/,
            L"DMSVv[0x%p]: MeasureOverride availableSize=%f,%f, IsRootSV=%d.", this, availableSize.Width, availableSize.Height, IsRootScrollViewer()));
        if (pDesired != nullptr)
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE | DMSVv_DBG) /*traceType*/,
                L"                   desiredSize=%f,%f.", pDesired->Width, pDesired->Height));
        }
    }
#endif // DM_DEBUG
    RRETURN(hr);
}

IFACEMETHODIMP ScrollViewer::ArrangeOverride(
    _In_ wf::Size finalSize,
    _Out_ wf::Size* returnValue) noexcept
{
    ANCHORING_DEBUG_TRACE(L"SV[0x%p]: ArrangeOverride finalSize (%f %f)", this, finalSize.Width, finalSize.Height);

    ctl::ComPtr<xaml::IUIElement> child;
    IFC_RETURN(GetContentUIElement(&child));

    if (child)
    {
        wf::Rect finalChildRect = { 0, 0, finalSize.Width, finalSize.Height };
        XRECTF postArrangeViewport{};
        XRECTF preArrangeViewport{};

        bool isAnchoringElementHorizontally = false;
        bool isAnchoringElementVertically = false;
        bool isAnchoringFarEdgeHorizontally = false;
        bool isAnchoringFarEdgeVertically = false;

        IFC_RETURN(IsAnchoring(&isAnchoringElementHorizontally, &isAnchoringElementVertically, &isAnchoringFarEdgeHorizontally, &isAnchoringFarEdgeVertically));

        ASSERT(!(isAnchoringElementHorizontally && isAnchoringFarEdgeHorizontally));
        ASSERT(!(isAnchoringElementVertically && isAnchoringFarEdgeVertically));

        if (isAnchoringElementHorizontally || isAnchoringElementVertically || isAnchoringFarEdgeHorizontally || isAnchoringFarEdgeVertically)
        {
            wf::Size preArrangeViewportToElementAnchorPointsDistance{ FloatUtil::NaN, FloatUtil::NaN };

            float preArrangeViewportX = 0.0f;
            IFC_RETURN(get_ZoomedHorizontalOffsetWithPendingShifts(&preArrangeViewportX));
            float preArrangeViewportY = 0.0f;
            IFC_RETURN(get_ZoomedVerticalOffsetWithPendingShifts(&preArrangeViewportY));
            DOUBLE preArrangeViewportWidth = 0.0f;
            IFC_RETURN(get_ViewportWidth(&preArrangeViewportWidth));
            DOUBLE preArrangeViewportHeight = 0.0f;
            IFC_RETURN(get_ViewportHeight(&preArrangeViewportHeight));
            preArrangeViewport = { preArrangeViewportX, preArrangeViewportY, static_cast<float>(preArrangeViewportWidth), static_cast<float>(preArrangeViewportHeight) };
            ANCHORING_DEBUG_TRACE(L"SV[0x%p]: ArrangeOverride PrearrangeViewport with pending shifts %f %f %lf %lf", this, preArrangeViewportX, preArrangeViewportY, preArrangeViewportWidth, preArrangeViewportHeight);

            if (isAnchoringElementHorizontally || isAnchoringElementVertically)
            {
                IFC_RETURN(EnsureAnchorElementSelection(preArrangeViewport));
                IFC_RETURN(ComputeViewportToElementAnchorPointsDistance(
                    preArrangeViewport,
                    true /*isForPreArrange*/,
                    &preArrangeViewportToElementAnchorPointsDistance));
            }
            else
            {
                ResetAnchorElement();
            }

            IFC_RETURN(ScrollViewerGenerated::ArrangeOverride(finalSize, returnValue));

            m_pendingViewportShiftX = 0.0;
            m_pendingViewportShiftY = 0.0;

            // Using the new viewport sizes to handle the cases where an adjustment needs to be performed because of a ScrollViewer size change.
            DOUBLE postArrangeViewportWidth = 0.0f;
            IFC_RETURN(get_ViewportWidth(&postArrangeViewportWidth));
            DOUBLE postArrangeViewportHeight = 0.0f;
            IFC_RETURN(get_ViewportHeight(&postArrangeViewportHeight));
            // Use preArrange X and Y since get_VerticalOffset/get_HorizontalOffset gives different values before and after arrange. We don't care
            // if a scroll happened. What we care about is if the arrange bounds changed or the size of the viewport changed.
            postArrangeViewport = { preArrangeViewportX, preArrangeViewportY, static_cast<float>(postArrangeViewportWidth), static_cast<float>(postArrangeViewportHeight) };

            FLOAT zoomFactor = 1.0;
            IFC_RETURN(get_ZoomFactor(&zoomFactor));

            if (!isnan(preArrangeViewportToElementAnchorPointsDistance.Width) || !isnan(preArrangeViewportToElementAnchorPointsDistance.Height))
            {
                wf::Size postArrangeViewportToElementAnchorPointsDistance;
                IFC_RETURN(ComputeViewportToElementAnchorPointsDistance(
                    postArrangeViewport,
                    false /*isForPreArrange*/,
                    &postArrangeViewportToElementAnchorPointsDistance));

                float postArrangeViewportX = 0.0f;
                IFC_RETURN(get_ZoomedHorizontalOffsetWithPendingShifts(&postArrangeViewportX));
                float postArrangeViewportY = 0.0f;
                IFC_RETURN(get_ZoomedVerticalOffsetWithPendingShifts(&postArrangeViewportY));
                postArrangeViewport = { postArrangeViewportX, postArrangeViewportY, static_cast<float>(postArrangeViewportWidth), static_cast<float>(postArrangeViewportHeight) };
                ANCHORING_DEBUG_TRACE(L"SV[0x%p]: ArrangeOverride PostarrangeViewport %f %f %lf %lf", this, postArrangeViewportX, postArrangeViewportY, postArrangeViewportWidth, postArrangeViewportHeight);

                if (isAnchoringElementHorizontally &&
                    !isnan(preArrangeViewportToElementAnchorPointsDistance.Width) &&
                    !isnan(postArrangeViewportToElementAnchorPointsDistance.Width) &&
                    preArrangeViewportToElementAnchorPointsDistance.Width != postArrangeViewportToElementAnchorPointsDistance.Width)
                {
                    // Perform horizontal offset adjustment due to element anchoring
                    float unzoomedAdjustment = static_cast<float>(postArrangeViewportToElementAnchorPointsDistance.Width - preArrangeViewportToElementAnchorPointsDistance.Width);
                    IFC_RETURN(PerformPositionAdjustment(
                        true /* isHorizontalDimension */,
                        unzoomedAdjustment,
                        postArrangeViewport));

                    float zoomedAdjustment = unzoomedAdjustment * zoomFactor;
                    m_pendingViewportShiftX = zoomedAdjustment;
                }

                if (isAnchoringElementVertically &&
                    !isnan(preArrangeViewportToElementAnchorPointsDistance.Height) &&
                    !isnan(postArrangeViewportToElementAnchorPointsDistance.Height) &&
                    preArrangeViewportToElementAnchorPointsDistance.Height != postArrangeViewportToElementAnchorPointsDistance.Height)
                {
                    float unzoomedAdjustment = static_cast<float>(postArrangeViewportToElementAnchorPointsDistance.Height - preArrangeViewportToElementAnchorPointsDistance.Height);
                    // Perform vertical offset adjustment due to element anchoring
                    IFC_RETURN(PerformPositionAdjustment(
                        false /* isHorizontalDimension */,
                        unzoomedAdjustment,
                        postArrangeViewport));

                    float zoomedAdjustment = unzoomedAdjustment * zoomFactor;
                    m_pendingViewportShiftY = zoomedAdjustment;
                }
            }

            ctl::ComPtr<xaml::IFrameworkElement> childAsFE = child.AsOrNull<xaml::IFrameworkElement>();
            xaml::Thickness childMargin{};

            if (childAsFE)
            {
                IFC_RETURN(childAsFE.Cast<FrameworkElement>()->get_Margin(&childMargin));
            }

            wf::Size childRenderSize{};
            IFC_RETURN(child.Cast<UIElement>()->get_RenderSize(&childRenderSize));

            // Take into account the actual resulting rendering size, in case it's larger than the desired size.
            finalChildRect.Width = std::max(
                finalChildRect.Width,
                std::max(0.0f, childRenderSize.Width + static_cast<float>(childMargin.Left + childMargin.Right)));
            finalChildRect.Height = std::max(
                finalChildRect.Height,
                std::max(0.0f, childRenderSize.Height + static_cast<float>(childMargin.Top + childMargin.Bottom)));

            if (isAnchoringFarEdgeHorizontally)
            {
                float unzoomedAdjustment = 0.0f;

                if (finalChildRect.Width > static_cast<float>(m_unzoomedExtentWidth))
                {
                    // ExtentWidth grew: Perform horizontal offset adjustment due to edge anchoring
                    unzoomedAdjustment = finalChildRect.Width - static_cast<float>(m_unzoomedExtentWidth);
                }

                if (m_viewportWidth > static_cast<float>(postArrangeViewport.Width))
                {
                    // Viewport width shrank: Perform horizontal offset adjustment due to edge anchoring
                    unzoomedAdjustment += static_cast<float>((m_viewportWidth - postArrangeViewport.Width) / zoomFactor);
                }

                if (unzoomedAdjustment != 0.0f)
                {
                    IFC_RETURN(PerformPositionAdjustment(
                        true /* isHorizontalDimension */,
                        unzoomedAdjustment,
                        postArrangeViewport));
                }
            }

            if (isAnchoringFarEdgeVertically)
            {
                float unzoomedAdjustment = 0.0f;

                if (finalChildRect.Height > static_cast<float>(m_unzoomedExtentHeight))
                {
                    // ExtentHeight grew: Perform vertical offset adjustment due to edge anchoring
                    unzoomedAdjustment = finalChildRect.Height - static_cast<float>(m_unzoomedExtentHeight);
                }

                if (m_viewportHeight > static_cast<float>(postArrangeViewport.Height))
                {
                    // Viewport height shrank: Perform vertical offset adjustment due to edge anchoring
                    unzoomedAdjustment += static_cast<float>((m_viewportHeight - postArrangeViewport.Height) / zoomFactor);
                }

                if (unzoomedAdjustment != 0.0f)
                {
                    IFC_RETURN(PerformPositionAdjustment(
                        false /* isHorizontalDimension */,
                        unzoomedAdjustment,
                        postArrangeViewport));
                }
            }

            m_unzoomedExtentWidth = static_cast<double>(finalChildRect.Width)  /*unzoomedExtentWidth*/;
            m_unzoomedExtentHeight = static_cast<double>(finalChildRect.Height) /*unzoomedExtentHeight*/;
            m_viewportWidth = postArrangeViewportWidth;
            m_viewportHeight = postArrangeViewportHeight;
        }
        else
        {
            ResetAnchorElement();
            IFC_RETURN(ScrollViewerGenerated::ArrangeOverride(finalSize, returnValue));
        }

        ANCHORING_DEBUG_TRACE(L"SV[0x%p]: ArrangeOverride child.ArrangeSize %lf %lf", this, finalChildRect.Width, finalChildRect.Height);
    }
    else
    {
        // No Child, just delegate to the base class.
        IFC_RETURN(ScrollViewerGenerated::ArrangeOverride(finalSize, returnValue));
    }

    m_isAnchorElementDirty = true;
    return S_OK;
}

// GotFocus event handler.
IFACEMETHODIMP ScrollViewer::OnGotFocus(
    _In_ xaml::IRoutedEventArgs* pArgs)
{
    if (IsRootScrollViewer())
    {
        return S_OK;
    }

    DXamlCore* pCore = DXamlCore::GetCurrent();

    IFCPTR_RETURN(pCore);
    IFCEXPECT_RETURN(pCore->GetHandle());

    CContentRoot* contentRoot = VisualTree::GetContentRootForElement(GetHandle());
    DirectUI::InputDeviceType lastInputDeviceType = contentRoot->GetInputManager().GetLastInputDeviceType();

    m_preferMouseIndicators =
        lastInputDeviceType == DirectUI::InputDeviceType::Mouse ||
        lastInputDeviceType == DirectUI::InputDeviceType::Pen;

    IFC_RETURN(ShowIndicators());

    // If we are here because a text-editable descendant got focus,
    // we might have to reflow the ScrollViewer around any existing occlusions.
    BOOLEAN reduceViewportForCoreInputViewOcclusions = FALSE;

    IFCFAILFAST(get_ReduceViewportForCoreInputViewOcclusions(&reduceViewportForCoreInputViewOcclusions));

    if (reduceViewportForCoreInputViewOcclusions)
    {
        IFC_RETURN(ReflowAroundCoreInputViewOcclusions());
    }

    return S_OK;
}

// KeyDown event handler.
IFACEMETHODIMP ScrollViewer::OnKeyDown(
    _In_ xaml_input::IKeyRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN handled = FALSE;
    BOOLEAN templatedParentHandlesScrolling = FALSE;
    wsy::VirtualKey key = wsy::VirtualKey_None;
    wsy::VirtualKey originalKey = wsy::VirtualKey_None;
    wsy::VirtualKeyModifiers keyModifiers = wsy::VirtualKeyModifiers_None;
    ZoomDirection messageZoomDirection = ZoomDirection_None;
    BOOLEAN continueRouting = FALSE;

    IFCPTR(pArgs);

    // Don't process the keyboard input on the root SV
    if (IsRootScrollViewer())
    {
        // Don't let DirectManipulation process the keystroke when this ScrollViewer is the root SV
        goto Cleanup;
    }

    m_preferMouseIndicators = FALSE;

#ifdef DM_DEBUG
    if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
    {
        IGNOREHR(pArgs->get_Key(&key));
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/, L"DMSV[0x%p]:  OnKeyDown - entry. key=%d.", this, key));
    }
#endif // DM_DEBUG

    IFC(ScrollViewerGenerated::OnKeyDown(pArgs));

    IFC(pArgs->get_Handled(&handled));
    if (handled)
    {
#ifdef DM_DEBUG
        if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/, L"Exits because handled after ScrollViewerGenerated::OnKeyDown."));
        }
#endif // DM_DEBUG

        goto Cleanup;
    }

    IFC(get_TemplatedParentHandlesScrolling(&templatedParentHandlesScrolling));
    if (templatedParentHandlesScrolling)
    {
#ifdef DM_DEBUG
        if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/, L"Exits because get_TemplatedParentHandlesScrolling returns true."));
        }
#endif // DM_DEBUG

        goto Cleanup;
    }

    if (m_ignoreSemanticZoomNavigationInput && m_inSemanticZoomAnimation)
    {
        // we'll just ignore all keyboard input while an animation is occurring
        // since DM will send out completed events if we don't.
        goto Cleanup;
    }

    IFC(pArgs->get_Key(&key));
    static_cast<KeyRoutedEventArgs*>(pArgs)->get_OriginalKey(&originalKey);
    IFC(GetKeyboardModifiers(&keyModifiers));

    if (XboxUtility::IsGamepadNavigationDirection(originalKey) ||
        XboxUtility::IsGamepadPageNavigationDirection(originalKey))
    {
        IFC(ProcessGamepadNavigation(key, originalKey, handled));
        IFC(pArgs->put_Handled(handled));
    }
    else
    {
        // Check if current key should trigger chaining
        IFC(ShouldContinueRoutingKeyDownEvent(key, continueRouting));

        if (continueRouting)
        {
            goto Cleanup;
        }

        messageZoomDirection = GetKeyboardMessageZoomAction(keyModifiers, key);
        if (!m_ignoreSemanticZoomNavigationInput || messageZoomDirection == ZoomDirection_None)
        {
            // Let the InputManager forward this keystroke to DirectManipulation for potential processing.
            IFC(ProcessPureInertiaInputMessage(messageZoomDirection, &handled));
            IFC(pArgs->put_Handled(handled));
#ifdef DM_DEBUG
            if (handled && (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK))
            {
                IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/, L"ProcessInputMessage returns true. Setting handled."));
            }
#endif // DM_DEBUG
        }
    }

Cleanup:
    RRETURN(hr);
}

// PointerPressed event handler.
IFACEMETHODIMP ScrollViewer::OnPointerPressed(
    _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<ixp::IPointerPoint> spPointerPoint;
    ctl::ComPtr<ixp::IPointerPointProperties> spPointerProperties;
    BOOLEAN handled = false;

    IFC(ScrollViewerGenerated::OnPointerPressed(pArgs));
    IFC(pArgs->get_Handled(&handled));
    if (handled)
    {
        goto Cleanup;
    }

    // If our templated parent is handling mouse button, we should not take
    // focus away.  They're handling it, not us.
    if (m_templatedParentHandlesMouseButton)
    {
        goto Cleanup;
    }

    IFC(pArgs->GetCurrentPoint(this, &spPointerPoint));
    IFCPTR(spPointerPoint);
    IFC(spPointerPoint->get_Properties(&spPointerProperties));
    IFCPTR(spPointerProperties);
    IFC(spPointerProperties->get_IsLeftButtonPressed(&m_isPointerLeftButtonPressed));


    // Don't handle PointerPressed event to raise up

Cleanup:
    RRETURN(hr);
}

// PointerReleased event handler.
IFACEMETHODIMP ScrollViewer::OnPointerReleased(
    _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN handled = FALSE;
    BOOLEAN focused = FALSE;
    bool isFocusedOnLightDismissPopupOfFlyout = false;

    IFC(ScrollViewerGenerated::OnPointerReleased(pArgs));
    IFC(pArgs->get_Handled(&handled));
    if (handled)
    {
        goto Cleanup;
    }

    if (m_isPointerLeftButtonPressed)
    {
        GestureModes gestureFollowing = GestureModes::None;

        // Reset the pointer left button pressed state
        m_isPointerLeftButtonPressed = FALSE;

        IFC(static_cast<PointerRoutedEventArgs*>(pArgs)->get_GestureFollowing(&gestureFollowing));

        if (gestureFollowing == GestureModes::RightTapped)
        {
            // Schedule the focus change for OnRightTappedUnhandled.
            m_shouldFocusOnRightTapUnhandled = TRUE;
        }
        else
        {
            // Set focus on the Flyout inner ScrollViewer to dismiss IHM.
            if (m_isFocusableOnFlyoutScrollViewer)
            {
                IFC(CoreImports::ScrollContentControl_SetFocusOnFlyoutLightDismissPopupByPointer(static_cast<CScrollContentControl*>(GetHandle()), &isFocusedOnLightDismissPopupOfFlyout));
            }
            if (isFocusedOnLightDismissPopupOfFlyout)
            {
                IFC(pArgs->put_Handled(TRUE));
            }
            else
            {
                // Focus now.
                // Set focus to the ScrollViewer to capture key input for scrolling
                IFC(Focus(xaml::FocusState_Pointer, &focused));
                IFC(pArgs->put_Handled(focused));
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

// RightTappedUnhandled event handler.
// Private event.
_Check_return_ HRESULT ScrollViewer::OnRightTappedUnhandled(
    _In_ IRightTappedRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;

    IFC(ScrollViewerGenerated::OnRightTappedUnhandled(pArgs));

    BOOLEAN isHandled = FALSE;
    IFC(pArgs->get_Handled(&isHandled));

    // New code.
    if (!isHandled &&
        m_shouldFocusOnRightTapUnhandled)
    {
        BOOLEAN focused = FALSE;

        // Set focus to the ScrollViewer to capture key input for scrolling
        IFC(Focus(xaml::FocusState_Pointer, &focused));
        IFC(pArgs->put_Handled(focused));
    }

Cleanup:
    m_shouldFocusOnRightTapUnhandled = FALSE;

    RRETURN(hr);
}

// PointerEnter event handler.
IFACEMETHODIMP ScrollViewer::OnPointerEntered(
    _In_ IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_input::IPointer> spPointer;
    mui::PointerDeviceType pointerDeviceType = mui::PointerDeviceType_Mouse;

    // Don't process the pointer input when IHM is hidden on the root SV
    if (IsRootScrollViewer() && !IsInputPaneShow())
    {
        goto Cleanup;
    }

    IFC(ScrollViewerGenerated::OnPointerEntered(pArgs));

    IFC(pArgs->get_Pointer(&spPointer));
    IFCPTR(spPointer);
    IFC(spPointer->get_PointerDeviceType(&pointerDeviceType));

    // Mouse input dominates. If we are showing panning indicators and then mouse comes into play, mouse indicators win.
    if (mui::PointerDeviceType_Touch != pointerDeviceType)
    {
        m_preferMouseIndicators = TRUE;
        IFC(ShowIndicators());
    }

Cleanup:
    RRETURN(hr);
}

// PointerMoved event handler.
IFACEMETHODIMP ScrollViewer::OnPointerMoved(
    _In_ IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_input::IPointer> spPointer;
    mui::PointerDeviceType pointerDeviceType = mui::PointerDeviceType_Mouse;

    // Don't process the pointer input on the root SV
    if (IsRootScrollViewer())
    {
        goto Cleanup;
    }

    IFC(ScrollViewerGenerated::OnPointerMoved(pArgs));

    // Don't process if this is a generated replay of the event.
    BOOLEAN isGenerated;
    IFC(static_cast<PointerRoutedEventArgs*>(pArgs)->get_IsGenerated(&isGenerated));
    if (isGenerated)
    {
        goto Cleanup;
    }

    IFC(pArgs->get_Pointer(&spPointer));
    IFCPTR(spPointer);
    IFC(spPointer->get_PointerDeviceType(&pointerDeviceType));

    // Mouse input dominates. If we are showing panning indicators and then mouse comes into play, mouse indicators win.
    if (mui::PointerDeviceType_Touch != pointerDeviceType)
    {
        m_preferMouseIndicators = TRUE;
        IFC(ShowIndicators());
    }

Cleanup:
    RRETURN(hr);
}

// PointerExitedevent handler.
IFACEMETHODIMP ScrollViewer::OnPointerExited(
    _In_ IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_input::IPointer> spPointer;
    mui::PointerDeviceType pointerDeviceType = mui::PointerDeviceType_Mouse;

    // Don't process the pointer input on the root SV
    if (IsRootScrollViewer())
    {
        goto Cleanup;
    }

    IFC(ScrollViewerGenerated::OnPointerExited(pArgs));

    IFC(pArgs->get_Pointer(&spPointer));
    IFCPTR(spPointer);
    IFC(spPointer->get_PointerDeviceType(&pointerDeviceType));

    // Mouse input dominates. If we are showing panning indicators and then mouse comes into play, mouse indicators win.
    if (mui::PointerDeviceType_Touch != pointerDeviceType)
    {
        m_isPointerOverVerticalScrollbar = FALSE;
        m_isPointerOverHorizontalScrollbar = FALSE;
        m_preferMouseIndicators = TRUE;
        IFC(ShowIndicators());
    }

Cleanup:
    RRETURN(hr);
}

// PointerWheelChanged event handler.
IFACEMETHODIMP ScrollViewer::OnPointerWheelChanged(
    _In_ IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IScrollInfo> spScrollInfo;
    ctl::ComPtr<ixp::IPointerPoint> spPointerPoint;
    ctl::ComPtr<ixp::IPointerPointProperties> spPointerProperties;
    BOOLEAN handled = FALSE;
    BOOLEAN isScrollContentPresenterScrollClient = FALSE;
    BOOLEAN isHorizontalMouseWheel = FALSE;
    ZoomDirection messageZoomDirection = ZoomDirection_None;

    // Don't process the pointer input when IHM is hidden on the root SV
    if (IsRootScrollViewer() && !IsInputPaneShow())
    {
        goto Cleanup;
    }

    IFCPTR(pArgs);

    IFC(pArgs->get_Handled(&handled));
    if (!handled)
    {
        INT mouseWheelDelta = 0;
        wsy::VirtualKeyModifiers keyModifiers = wsy::VirtualKeyModifiers_None;
        BOOLEAN isCtrlPressed = FALSE;

        IFC(GetKeyboardModifiers(&keyModifiers));
        isCtrlPressed = IsFlagSet(keyModifiers, wsy::VirtualKeyModifiers_Control);

        IFC(pArgs->GetCurrentPoint(this, &spPointerPoint));
        IFCPTR(spPointerPoint);
        IFC(spPointerPoint->get_Properties(&spPointerProperties));
        IFCPTR(spPointerProperties);
        IFC(spPointerProperties->get_MouseWheelDelta(&mouseWheelDelta));

        if (isCtrlPressed)
        {
            if (mouseWheelDelta < 0)
            {
                messageZoomDirection = ZoomDirection_Out;
            }
            else
            {
                messageZoomDirection = ZoomDirection_In;
            }
        }

        // Process the event if:
        //   Zooming AND zoom events are NOT ignored
        //   OR
        //   scrolling AND scroll events are NOT ignored

        if ((messageZoomDirection != ZoomDirection_None && !m_ignoreSemanticZoomNavigationInput) ||
            (messageZoomDirection == ZoomDirection_None && !m_arePointerWheelEventsIgnored))
        {
            bool isContentHorizontallyScrollable = false;
            bool isContentVerticallyScrollable = false;

            if (messageZoomDirection == ZoomDirection_None)
            {
                // ignoring scroll bar visibility property to keep compat //
                IFC(IsContentScrollable(false, /* ignoreScrollMode */
                                        true, /* ignoreScrollBarVisibility */
                                        &isContentHorizontallyScrollable,
                                        &isContentVerticallyScrollable));
            }

            if (!IsRootScrollViewer() || IsInputPaneShow())
            {
                m_preferMouseIndicators = TRUE;

                // No need to call ShowIndicators() since changing the scroll info takes care of this.
            }

            if (isContentHorizontallyScrollable || isContentVerticallyScrollable || messageZoomDirection != ZoomDirection_None)
            {
                // Determine whether the ScrollContentPresenter is the IScrollInfo implementer or not
                IFC(IsScrollContentPresenterScrollClient(isScrollContentPresenterScrollClient));

                if (isScrollContentPresenterScrollClient)
                {
                    // Give DirectManipulation an opportunity to handle the mouse wheel message
                    IFC(ProcessPureInertiaInputMessage(messageZoomDirection, &handled));
                    IFC(pArgs->put_Handled(handled));
                }
                else
                {
                    // Let the IScrollInfo implementation handle the wheel delta
                    IFC(get_ScrollInfo(&spScrollInfo));
                    if (spScrollInfo)
                    {
                        IFC(spPointerProperties->get_IsHorizontalMouseWheel(&isHorizontalMouseWheel));
                        if (mouseWheelDelta < 0)
                        {
                            if (isHorizontalMouseWheel)
                            {
                                IFC(spScrollInfo->MouseWheelLeft(-mouseWheelDelta));
                            }
                            else
                            {
                                IFC(spScrollInfo->MouseWheelDown(-mouseWheelDelta));
                            }
                        }
                        else
                        {
                            if (isHorizontalMouseWheel)
                            {
                                IFC(spScrollInfo->MouseWheelRight(mouseWheelDelta));
                            }
                            else
                            {
                                IFC(spScrollInfo->MouseWheelUp(mouseWheelDelta));
                            }
                        }

                        IFC(pArgs->put_Handled(m_handleScrollInfoWheelEvent));
                    }
                }
            }
        }
    }

Cleanup:
    m_handleScrollInfoWheelEvent = TRUE;
    RRETURN(hr);
}

// Bring a child element into view.
// Alignment ratios are either -1 (i.e. no alignment to apply) or between
// 0 and 1. For instance when the alignment ratio is 0, the near edge of
// the 'targetRect' needs to align with the near edge of the viewport.
// 'offsetX/offsetY' are additional amounts of scrolling requested, beyond the
// normal amount to bring the target into view and potentially align it.
// Those additional offsets are only applied when the 'targetRect' does not
// step outside the extents.
_Check_return_ HRESULT ScrollViewer::MakeVisible(
    // Child element to bring into view
    _In_ UIElement* element,
    // Target rectangle dimensions. If empty, bring the child element's
    // RenderSize dimensions into view.
    wf::Rect targetRect,
    // Pass on forceIntoView from sender to ancestor ScrollViewer
    BOOLEAN forceIntoView,
    // When set to True, the DManip ZoomToRect method is invoked.
    BOOLEAN useAnimation,
    // Forwarded to the BringIntoView method to indicate whether its own
    // MakeVisible calls should be skipped during an ongoing manipulation or not.
    BOOLEAN skipDuringManipulation,
    DOUBLE horizontalAlignmentRatio,
    DOUBLE verticalAlignmentRatio,
    DOUBLE offsetX,
    DOUBLE offsetY)
{
    ctl::ComPtr<IScrollInfo> spScrollInfo;
    wf::Rect visibleBounds = {};
    wf::Rect desiredView = {};
    wf::Point visiblePoint = {};
    wf::Point transformedPoint = {};

#ifdef DM_DEBUG
    if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/,
            L"DMSV[0x%p]:  MakeVisible - targetRect=(%f, %f, %f, %f), forceIntoView=%d, useAnimation=%d, skipDuringManipulation=%d.",
            this, targetRect.X, targetRect.Y, targetRect.Width, targetRect.Height, forceIntoView, useAnimation, skipDuringManipulation));
    }
#endif // DM_DEBUG

    if (element && m_trElementScrollContentPresenter)
    {
        BOOLEAN isAncestorOfChild = FALSE;
        BOOLEAN isAncestorOfPresenter = FALSE;

        IFC_RETURN(m_trElementScrollContentPresenter.Cast<ScrollContentPresenter>()->IsAncestorOf(element, &isAncestorOfChild));
        IFC_RETURN(IsAncestorOf(m_trElementScrollContentPresenter.Cast<ScrollContentPresenter>(), &isAncestorOfPresenter));

        if (isAncestorOfChild &&
            isAncestorOfPresenter &&
            IsInLiveTree())
        {
            wf::Rect target = {};

            BOOLEAN empty = TRUE;

            IFC_RETURN(RectUtil::GetIsEmpty(targetRect, &empty));

            if (empty)
            {
                wf::Size renderSize = {};
                IFC_RETURN(element->get_RenderSize(&renderSize));
                target.X = 0;
                target.Y = 0;
                target.Width = renderSize.Width;
                target.Height = renderSize.Height;
            }
            else
            {
                target = targetRect;
            }

            // Get the rectangle for the scroll content present after bringing
            // the containing child element into view. The new rectangle is the
            // parameter for bringing the ScrollViewer's content into view by a
            // ScrollViewer ancestor.
            IFC_RETURN(get_ScrollInfo(&spScrollInfo));
            if (spScrollInfo)
            {
                DOUBLE appliedOffsetX = 0.0;
                DOUBLE appliedOffsetY = 0.0;

                IFC_RETURN(spScrollInfo->MakeVisible(
                    element,
                    target,
                    useAnimation,
                    horizontalAlignmentRatio,
                    verticalAlignmentRatio,
                    offsetX,
                    offsetY,
                    &visibleBounds,
                    &appliedOffsetX,
                    &appliedOffsetY));

                // Compute the remaining offsets to apply by potential parent contributors. The amount
                // applied by the last contributor, spScrollInfo, must not be applied more than once.
                if (appliedOffsetX != 0.0)
                {
                    offsetX -= appliedOffsetX;
                }
                if (appliedOffsetY != 0.0)
                {
                    offsetY -= appliedOffsetY;
                }
            }

            IFC_RETURN(RectUtil::GetIsEmpty(visibleBounds, &empty));
            if (!empty)
            {
                ctl::ComPtr<xaml_media::IGeneralTransform> spTransform;

                IFC_RETURN(m_trElementScrollContentPresenter.Cast<ScrollContentPresenter>()->TransformToVisual(this, &spTransform));
                visiblePoint.X = visibleBounds.X;
                visiblePoint.Y = visibleBounds.Y;
                IFC_RETURN(spTransform->TransformPoint(visiblePoint, &transformedPoint));
                desiredView.X = transformedPoint.X;
                desiredView.Y = transformedPoint.Y;
                desiredView.Width = visibleBounds.Width;
                desiredView.Height = visibleBounds.Height;
            }
            else
            {
                desiredView = visibleBounds;
            }

            BringIntoView(
                desiredView,
                forceIntoView,
                useAnimation,
                skipDuringManipulation,
                horizontalAlignmentRatio,
                verticalAlignmentRatio,
                offsetX,
                offsetY);
        }
    }

    return S_OK;
}

// OnBringIntoViewRequested is called from the event handler ScrollViewer
// registers for the event.  The default implementation checks to make sure the
// visual is a child of the scroll viewer, and then delegates to a method there.
_Check_return_ HRESULT ScrollViewer::OnBringIntoViewRequested(
    _In_ IUIElement* /*sender*/,
    _In_ xaml::IBringIntoViewRequestedEventArgs* args)
{
    // In certain circumstances (currently only the Pivot ScrollViewer), we never want
    // the ScrollViewer to handle RequestBringIntoView, ever.  The reason in the case
    // of the Pivot ScrollViewer is that it is solely intended to hold the Pivot items
    // and provide a way to shift between them - we never want the Pivot ScrollViewer
    // to move in order to bring something inside it into view.
    // In such cases, we'll just ignore RequestBringIntoView events unconditionally.
    if (m_isRequestBringIntoViewIgnored)
    {
        return S_OK;
    }

    xref_ptr<CEventArgs> coreEventArgs;
    coreEventArgs.attach(static_cast<BringIntoViewRequestedEventArgs*>(args)->GetCorePeer());
    CBringIntoViewRequestedEventArgs* coreRequestEventArgs = static_cast<CBringIntoViewRequestedEventArgs*>(coreEventArgs.get());
    ctl::ComPtr<xaml::IUIElement> spTargetObject;
    IFC_RETURN(args->get_TargetElement(&spTargetObject));
    UIElement* elementNoRef = spTargetObject.Cast<UIElement>();

#ifdef DM_DEBUG
    if (args && (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK))
    {
        BOOLEAN forceIntoViewDbg = FALSE;
        BOOLEAN useAnimationDbg = FALSE;
        BOOLEAN skipDuringManipulationDbg = TRUE;
        DOUBLE horizontalAlignmentRatioDbg = DoubleUtil::NaN;
        DOUBLE verticalAlignmentRatioDbg = DoubleUtil::NaN;
        DOUBLE offsetXDbg = 0.0;
        DOUBLE offsetYDbg = 0.0;
        BOOLEAN bringIntoViewDbg = FALSE;
        wf::Rect rectDbg = {};

        IGNOREHR(coreRequestEventArgs->get_ForceIntoView(&forceIntoViewDbg));
        IGNOREHR(args->get_AnimationDesired(&useAnimationDbg));
        IGNOREHR(coreRequestEventArgs->get_InterruptDuringManipulation(&skipDuringManipulationDbg));
        IGNOREHR(args->get_TargetRect(&rectDbg));
        IGNOREHR(args->get_HorizontalAlignmentRatio(&horizontalAlignmentRatioDbg));
        IGNOREHR(args->get_VerticalAlignmentRatio(&verticalAlignmentRatioDbg));
        IGNOREHR(args->get_HorizontalOffset(&offsetXDbg));
        IGNOREHR(args->get_VerticalOffset(&offsetYDbg));
        IGNOREHR(get_BringIntoViewOnFocusChangeImpl(&bringIntoViewDbg));

        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/,
            L"DMSV[0x%p]:  OnBringIntoViewRequested - forceIntoView=%d, useAnimation=%d, skipDuringManipulation=%d, bringIntoView=%d, isInManipulation=%d.",
            this, forceIntoViewDbg, useAnimationDbg, skipDuringManipulationDbg, bringIntoViewDbg, IsInManipulation()));
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/,
            L"                   targetRect=(%lf, %lf, %lf, %lf), horizontalAlignmentRatio=%lf, verticalAlignmentRatio=%lf, offsetX=%lf, offsetY=%lf.",
            rectDbg.X, rectDbg.Y, rectDbg.Width, rectDbg.Height, horizontalAlignmentRatioDbg, verticalAlignmentRatioDbg, offsetXDbg, offsetYDbg));
    }
#endif // DM_DEBUG

    if (elementNoRef && this != elementNoRef)
    {
        BOOLEAN isAncestor = FALSE;

        IFC_RETURN(IsAncestorOf(elementNoRef, &isAncestor));
        if (isAncestor)
        {
            BOOLEAN forceIntoView = FALSE;
            BOOLEAN useAnimation = FALSE;
            BOOLEAN skipDuringManipulation = TRUE;
            DOUBLE horizontalAlignmentRatio = DoubleUtil::NaN;
            DOUBLE verticalAlignmentRatio = DoubleUtil::NaN;
            DOUBLE offsetX = 0.0;
            DOUBLE offsetY = 0.0;
            BOOLEAN bringIntoView = FALSE;

            // Don't bring into view if ScrollViewer.BringIntoViewOnFocusChange = FALSE,
            // unless forceIntoView is set. An app sets ScrollViewer.BringIntoViewOnFocusChange to FALSE
            // when it wants to handle BringIntoView.
            // To prevent incorrect scroll offsets, don't auto scroll into view when ScrollViewer
            // is being manipulated by user. For example, don't scroll into view during zoomin/out
            // in SemanticZoom using the keyboard.

            IFC_RETURN(coreRequestEventArgs->get_ForceIntoView(&forceIntoView));
            IFC_RETURN(args->get_AnimationDesired(&useAnimation));
            IFC_RETURN(coreRequestEventArgs->get_InterruptDuringManipulation(&skipDuringManipulation));
            IFC_RETURN(args->get_HorizontalAlignmentRatio(&horizontalAlignmentRatio));
            IFC_RETURN(args->get_VerticalAlignmentRatio(&verticalAlignmentRatio));
            IFC_RETURN(args->get_HorizontalOffset(&offsetX));
            IFC_RETURN(args->get_VerticalOffset(&offsetY));
            IFC_RETURN(get_BringIntoViewOnFocusChangeImpl(&bringIntoView));

            if (forceIntoView || (bringIntoView && (!skipDuringManipulation || !IsInManipulation())))
            {
                wf::Rect rect = {};

                IFC_RETURN(args->get_TargetRect(&rect));
                IFC_RETURN(MakeVisible(
                    elementNoRef,
                    rect,
                    forceIntoView,
                    useAnimation,
                    skipDuringManipulation,
                    horizontalAlignmentRatio,
                    verticalAlignmentRatio,
                    offsetX,
                    offsetY));
            }

            // Set handled as true since MakeVisible will invoke BringIntoView for parent contributors.
            static_cast<CRoutedEventArgs*>(coreEventArgs.get())->m_bHandled = TRUE;
        }
    }

    return S_OK;
}

// Handles the vertical ScrollBar.Scroll event and updates the UI.
_Check_return_ HRESULT ScrollViewer::HandleVerticalScroll(
    _In_ xaml_primitives::ScrollEventType scrollEventType,
    _In_ DOUBLE offset)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IScrollInfo> spScrollInfo;
    BOOLEAN isCarouselPanel = FALSE;
    DOUBLE oldOffset = 0.0;
    DOUBLE newOffset = 0.0;
    DOUBLE scrollable = 0.0;

#ifdef DM_DEBUG
    if (DMSVv_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE)) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE | DMSVv_DBG) /*traceType*/,
            L"DMSVv[0x%p]: HandleVerticalScroll - entry. offset=%f.", this, offset));
    }
#endif // DM_DEBUG

    // If style changes and Content cannot be found - just exit.
    IFC(get_ScrollInfo(&spScrollInfo));
    if (!spScrollInfo)
    {
        goto Cleanup;
    }

    IFC(spScrollInfo->get_VerticalOffset(&oldOffset));
    newOffset = oldOffset;

    switch (scrollEventType)
    {
    case xaml_primitives::ScrollEventType_EndScroll:
        IFC(LeaveIntermediateViewChangedMode(TRUE /*raiseFinalViewChanged*/));
        break;
    case xaml_primitives::ScrollEventType_ThumbPosition:
    case xaml_primitives::ScrollEventType_ThumbTrack:
        if (scrollEventType == xaml_primitives::ScrollEventType_ThumbTrack)
        {
            IFC(EnterIntermediateViewChangedMode());
        }
        newOffset = offset;
        break;
    case xaml_primitives::ScrollEventType_LargeDecrement:
        IFC(spScrollInfo->PageUp());
        break;
    case xaml_primitives::ScrollEventType_LargeIncrement:
        IFC(spScrollInfo->PageDown());
        break;
    case xaml_primitives::ScrollEventType_SmallDecrement:
        IFC(spScrollInfo->LineUp());
        break;
    case xaml_primitives::ScrollEventType_SmallIncrement:
        IFC(spScrollInfo->LineDown());
        break;
    case xaml_primitives::ScrollEventType_First:
        newOffset = DoubleUtil::MinValue;
        break;
    case xaml_primitives::ScrollEventType_Last:
        newOffset = DoubleUtil::MaxValue;
        break;
    }

    IFC(IsPanelACarouselPanel(FALSE, isCarouselPanel));
    // Do not clamp offset for carousel-ing panel.
    if (!isCarouselPanel)
    {
        newOffset = DoubleUtil::Max(newOffset, 0.0);
        // Clamp the new offset at this stage to prevent unnecessary layout.
        IFC(get_ScrollableHeight(&scrollable));
        newOffset = DoubleUtil::Min(scrollable, newOffset);
    }

    // If newOffset does not match oldOffset, apply it
    if (!DoubleUtil::AreClose(oldOffset, newOffset))
    {
        // potentially block updating the scrollinfo while thumb dragging is occurring
        BOOLEAN isDeferring = FALSE;
        if (m_isDraggingThumb)
        {
            IFC(get_IsDeferredScrollingEnabledImpl(&isDeferring));
            if (isDeferring)
            {
                m_verticalOffsetCached = newOffset;
            }
        }

        if (!isDeferring)
        {
#ifdef DM_DEBUG
            if (DMSVv_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE)) == S_OK)
            {
                IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE | DMSVv_DBG) /*traceType*/, L"DMSVv[0x%p]: HandleVerticalScroll calls SetVerticalOffset with newOffset=%f.", this, newOffset));
            }
#endif // DM_DEBUG

            IFC(spScrollInfo->SetVerticalOffset(newOffset));
        }
    }

Cleanup:
    RRETURN(hr);
}

// Handles the horizontal ScrollBar.Scroll event and updates the UI.
_Check_return_ HRESULT ScrollViewer::HandleHorizontalScroll(
    _In_ xaml_primitives::ScrollEventType scrollEventType,
    _In_ DOUBLE offset)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IScrollInfo> spScrollInfo;
    DOUBLE oldOffset = 0.0;
    DOUBLE newOffset = 0.0;
    DOUBLE scrollable = 0.0;

#ifdef DM_DEBUG
    if (DMSVv_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE)) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE | DMSVv_DBG) /*traceType*/,
            L"DMSVv[0x%p]: HandleHorizontalScroll - entry. offset=%f.", this, offset));
    }
#endif // DM_DEBUG

    // If style changes and Content cannot be found - just exit.
    IFC(get_ScrollInfo(&spScrollInfo));
    if (!spScrollInfo)
    {
        goto Cleanup;
    }

    IFC(spScrollInfo->get_HorizontalOffset(&oldOffset));
    newOffset = oldOffset;

    switch (scrollEventType)
    {
    case xaml_primitives::ScrollEventType_EndScroll:
        IFC(LeaveIntermediateViewChangedMode(TRUE /*raiseFinalViewChanged*/));
        break;
    case xaml_primitives::ScrollEventType_ThumbPosition:
    case xaml_primitives::ScrollEventType_ThumbTrack:
        if (scrollEventType == xaml_primitives::ScrollEventType_ThumbTrack)
        {
            IFC(EnterIntermediateViewChangedMode());
        }
        newOffset = offset;
        break;
    case xaml_primitives::ScrollEventType_LargeDecrement:
        IFC(spScrollInfo->PageLeft());
        break;
    case xaml_primitives::ScrollEventType_LargeIncrement:
        IFC(spScrollInfo->PageRight());
        break;
    case xaml_primitives::ScrollEventType_SmallDecrement:
        IFC(spScrollInfo->LineLeft());
        break;
    case xaml_primitives::ScrollEventType_SmallIncrement:
        IFC(spScrollInfo->LineRight());
        break;
    case xaml_primitives::ScrollEventType_First:
        newOffset = DoubleUtil::MinValue;
        break;
    case xaml_primitives::ScrollEventType_Last:
        newOffset = DoubleUtil::MaxValue;
        break;
    }

    newOffset = DoubleUtil::Max(newOffset, 0.0);

    // Clamp the new offset at this stage to prevent unnecessary layout.
    IFC(get_ScrollableWidth(&scrollable));
    newOffset = DoubleUtil::Min(scrollable, newOffset);

    // If newOffset does not match oldOffset, apply it
    if (!DoubleUtil::AreClose(oldOffset, newOffset))
    {
        // potentially block updating the scrollinfo while thumb dragging is occurring
        BOOLEAN isDeferring = FALSE;
        if (m_isDraggingThumb)
        {
            IFC(get_IsDeferredScrollingEnabledImpl(&isDeferring));
            if (isDeferring)
            {
                m_horizontalOffsetCached = newOffset;
            }
        }

        if (!isDeferring)
        {
#ifdef DM_DEBUG
            if (DMSVv_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE)) == S_OK)
            {
                IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE | DMSVv_DBG) /*traceType*/, L"DMSVv[0x%p]: HandleHorizontalScroll calls SetHorizontalOffset with newOffset=%f.", this, newOffset));
            }
#endif // DM_DEBUG

            IFC(spScrollInfo->SetHorizontalOffset(newOffset));
        }
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::ChangeViewImpl
//
//  Synopsis:
//    Combines the abilities of ScrollToHorizontalOffset, ScrollToVerticalOffset
//    and ZoomToFactor. Attempts to animate to the target view and snap to
//    mandatory snap points.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::ChangeViewImpl(
    _In_opt_ wf::IReference<DOUBLE>* pHorizontalOffset,
    _In_opt_ wf::IReference<DOUBLE>* pVerticalOffset,
    _In_opt_ wf::IReference<FLOAT>* pZoomFactor,
    _Out_ BOOLEAN* pReturnValue)
{
#ifdef DM_DEBUG
    if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/, L"DMSV[0x%p]:  ChangeViewImpl - entry.", this));
    }
#endif // DM_DEBUG

    RRETURN(ChangeViewInternal(
        pHorizontalOffset,
        pVerticalOffset,
        pZoomFactor,
        NULL  /*pOldZoomFactor*/,
        FALSE /*forceChangeToCurrentView*/,
        TRUE  /*adjustWithMandatorySnapPoints*/,
        TRUE  /*skipDuringTouchContact*/,
        TRUE  /*skipAnimationWhileRunning*/,
        FALSE /*disableAnimation*/,
        TRUE  /*applyAsManip*/,
        FALSE /*transformIsInertiaEnd*/,
        FALSE /*isForMakeVisible*/,
        pReturnValue));
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::ChangeViewWithOptionalAnimationImpl
//
//  Synopsis:
//    Combines the abilities of ScrollToHorizontalOffset, ScrollToVerticalOffset
//    and ZoomToFactor, with the option of animating to the target view and snapping
//    to mandatory snap points.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::ChangeViewWithOptionalAnimationImpl(
    _In_opt_ wf::IReference<DOUBLE>* pHorizontalOffset,
    _In_opt_ wf::IReference<DOUBLE>* pVerticalOffset,
    _In_opt_ wf::IReference<FLOAT>* pZoomFactor,
    _In_ BOOLEAN disableAnimation,
    _Out_ BOOLEAN* pReturnValue)
{
#ifdef DM_DEBUG
    if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/, L"DMSV[0x%p]:  ChangeViewWithOptionalAnimationImpl - entry.", this));
    }
#endif // DM_DEBUG

    RRETURN(ChangeViewInternal(
        pHorizontalOffset,
        pVerticalOffset,
        pZoomFactor,
        NULL  /*pOldZoomFactor*/,
        FALSE /*forceChangeToCurrentView*/,
        TRUE  /*adjustWithMandatorySnapPoints*/,
        TRUE  /*skipDuringTouchContact*/,
        TRUE  /*skipAnimationWhileRunning*/,
        disableAnimation,
        TRUE  /*applyAsManip*/,
        FALSE /*transformIsInertiaEnd*/,
        FALSE /*isForMakeVisible*/,
        pReturnValue));
}

_Check_return_ HRESULT ScrollViewer::ChangeViewInternal(
    _In_opt_ wf::IReference<DOUBLE>* pHorizontalOffset,
    _In_opt_ wf::IReference<DOUBLE>* pVerticalOffset,
    _In_opt_ wf::IReference<FLOAT>* pZoomFactor,
    _In_opt_ FLOAT* pOldZoomFactor,
    _In_ BOOLEAN forceChangeToCurrentView,
    _In_ BOOLEAN adjustWithMandatorySnapPoints,
    _In_ BOOLEAN skipDuringTouchContact,
    _In_ BOOLEAN skipAnimationWhileRunning,
    _In_ BOOLEAN disableAnimation,
    _In_ BOOLEAN applyAsManip,
    _In_ BOOLEAN transformIsInertiaEnd,
    _In_ BOOLEAN isForMakeVisible,
    _Out_ BOOLEAN* pReturnValue) noexcept
{
    HRESULT hr = S_OK;
    BOOLEAN isHandled = FALSE;
    BOOLEAN isBringIntoViewportCallAllowed = TRUE;
    BOOLEAN isBringIntoViewportCalled = FALSE;
    BOOLEAN isScrollContentPresenterScrollClient = FALSE;
    BOOLEAN isViewChangingDelayed = FALSE;
    BOOLEAN isViewChangedDelayed = FALSE;
    BOOLEAN canManipulateElementsByTouch = FALSE;
    BOOLEAN canManipulateElementsNonTouch = FALSE;
    BOOLEAN canManipulateElementsWithBringIntoViewport = FALSE;
    BOOLEAN canHorizontallyScroll = FALSE;
    BOOLEAN canVerticallyScroll = FALSE;
    BOOLEAN clearInChangeViewBringIntoViewport = FALSE;
    DOUBLE currentUnzoomedPixelExtentWidth = -1.0;
    DOUBLE currentUnzoomedPixelExtentHeight = -1.0;
    DOUBLE targetExtentWidth = -1.0;
    DOUBLE targetExtentHeight = -1.0;
    DOUBLE viewportWidth = 0.0;
    DOUBLE viewportHeight = 0.0;
    DOUBLE viewportPixelWidth = 0.0;
    DOUBLE viewportPixelHeight = 0.0;
    DOUBLE currentHorizontalOffset = 0.0;
    DOUBLE currentVerticalOffset = 0.0;
    DOUBLE currentTargetHorizontalOffset = 0.0;
    DOUBLE currentTargetVerticalOffset = 0.0;
    DOUBLE oldTargetChangeViewHorizontalOffset = -1.0;
    DOUBLE oldTargetChangeViewVerticalOffset = -1.0;
    DOUBLE minHorizontalOffset = -1.0;
    DOUBLE minVerticalOffset = -1.0;
    DOUBLE maxHorizontalOffset = -1.0;
    DOUBLE maxVerticalOffset = -1.0;
    DOUBLE targetHorizontalOffset = 0.0;
    DOUBLE targetVerticalOffset = 0.0;
    DOUBLE targetPixelHorizontalOffset = 0.0;
    DOUBLE targetPixelVerticalOffset = 0.0;
    DOUBLE adjustedTargetHorizontalOffset = 0.0;
    DOUBLE adjustedTargetVerticalOffset = 0.0;
    FLOAT adjustedTargetZoomFactor = 1.0f;
    FLOAT targetZoomFactor = 1.0f;
    FLOAT currentZoomFactor = 1.0f;
    FLOAT currentTargetZoomFactor = 1.0f;
    FLOAT oldTargetChangeViewZoomFactor = -1.0f;
    FLOAT minZoomFactor = 0.0f;
    FLOAT maxZoomFactor = 0.0f;
    FLOAT targetTranslateX = 0.0f;
    FLOAT targetTranslateY = 0.0f;
    XRECTF bounds = { 0, 0, 0, 0 };
    DMAlignment alignment;
    ctl::ComPtr<IUIElement> spContentUIElement = NULL;
    ctl::ComPtr<IUIElement> spScrollInfoAsElement = NULL;
    ctl::ComPtr<IManipulationDataProvider> spProvider = NULL;
    ctl::ComPtr<IScrollInfo> spScrollInfo = NULL;
    xaml_controls::Orientation orientation = xaml_controls::Orientation_Horizontal;
    wf::Size sizeFirstVisibleItem = { 0, 0 };

    ASSERT(!transformIsInertiaEnd || (forceChangeToCurrentView && disableAnimation));

    // The provided pOldZoomFactor value is only used when m_isInZoomFactorSync is True.
    // i.e. when a ZoomToFactor call causes us to push a new content transform to DManip.
    // The value is expected to be nullptr in all other scenarios.
    ASSERT((pOldZoomFactor == nullptr && !m_isInZoomFactorSync) || (pOldZoomFactor != nullptr && *pOldZoomFactor > 0.0f && m_isInZoomFactorSync));

    IFCPTR(pReturnValue);
    *pReturnValue = FALSE;

#ifdef DM_DEBUG
    if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
    {
        DOUBLE targetHorizontalOffsetDbg = 0.0;
        DOUBLE targetVerticalOffsetDbg = 0.0;
        FLOAT targetZoomFactorDbg = 1.0f;
        if (pHorizontalOffset)
        {
            IGNOREHR(pHorizontalOffset->get_Value(&targetHorizontalOffsetDbg));
        }
        if (pVerticalOffset)
        {
            IGNOREHR(pVerticalOffset->get_Value(&targetVerticalOffsetDbg));
        }
        if (pZoomFactor)
        {
            IGNOREHR(pZoomFactor->get_Value(&targetZoomFactorDbg));
        }
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/,
            L"DMSV[0x%p]:  ChangeViewInternal - IsHorizontalOffsetSet=%d, IsVerticalOffsetSet=%d, IsZoomFactorSet=%d.",
            this,
            pHorizontalOffset != NULL,
            pVerticalOffset != NULL,
            pZoomFactor != NULL));
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/,
            L"                   HorizontalOffset=%f, VerticalOffset=%f, ZoomFactor=%4.8lf, OldZoomFactor=%4.8lf.",
            targetHorizontalOffsetDbg,
            targetVerticalOffsetDbg,
            targetZoomFactorDbg,
            pOldZoomFactor == nullptr ? 0.0f : *pOldZoomFactor));
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/,
            L"                   forceChangeToCurrentView=%d, adjustWithMandatorySnapPoints=%d, skipDuringTouchContact=%d, skipAnimationWhileRunning=%d, disableAnimation=%d, applyAsManip=%d, transformIsInertiaEnd=%d, isForMakeVisible=%d.",
            forceChangeToCurrentView,
            adjustWithMandatorySnapPoints,
            skipDuringTouchContact,
            skipAnimationWhileRunning,
            disableAnimation,
            applyAsManip,
            transformIsInertiaEnd,
            isForMakeVisible));
    }
#endif // DM_DEBUG

    if (!pHorizontalOffset && !pVerticalOffset && !pZoomFactor)
    {
        if (forceChangeToCurrentView)
        {
            // XAML is pushing new transform to DManip.
            m_isInDirectManipulationSync = TRUE;
        }
        else
        {
            // Not a single view characteristic was provided. Do no attempt any view change.
            // Unless forceChangeToCurrentView is True, in which case invoke a BringIntoViewport
            // for the current view to synchronize XAML and DManip.
            goto Cleanup;
        }
    }

    IFC(get_ScrollInfo(&spScrollInfo));
    if (!spScrollInfo)
    {
        // No IScrollInfo to interact with. Return False.
        goto Cleanup;
    }

    IFC(get_HorizontalOffset(&currentHorizontalOffset));
    IFC(get_VerticalOffset(&currentVerticalOffset));
    IFC(get_ZoomFactor(&currentZoomFactor));

#ifdef DM_DEBUG
    if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/,
            L"                   Current HorizontalOffset=%f, VerticalOffset=%f, ZoomFactor=%4.8lf, m_xPixelOffsetRequested=%f, m_yPixelOffsetRequested=%f, m_targetChangeViewHorizontalOffset=%f, m_targetChangeViewVerticalOffset=%f, m_targetChangeViewZoomFactor=%4.8lf.",
            currentHorizontalOffset,
            currentVerticalOffset,
            currentZoomFactor,
            m_xPixelOffsetRequested,
            m_yPixelOffsetRequested,
            m_targetChangeViewHorizontalOffset,
            m_targetChangeViewVerticalOffset,
            m_targetChangeViewZoomFactor));
    }
#endif // DM_DEBUG

    if (transformIsInertiaEnd && m_targetChangeViewHorizontalOffset != -1.0)
    {
        // This ChangeViewInternal call follows a cancellation of inertia in CInputServices::StopInertialViewport.
        // Store the previous ChangeView target (pixel-based offsets) so it can be used if the newly requested
        // target is slightly different because of roundings required for ZoomToRect.
        oldTargetChangeViewHorizontalOffset = m_targetChangeViewHorizontalOffset;
        oldTargetChangeViewVerticalOffset = m_targetChangeViewVerticalOffset;
        oldTargetChangeViewZoomFactor = m_targetChangeViewZoomFactor;

        // Since inertia was cancelled, the previous ChangeView target is no longer valid.
        // These values can no longer be used for setting the 3 currentTarget*** variables below.
        m_targetChangeViewHorizontalOffset = -1.0;
        m_targetChangeViewVerticalOffset = -1.0;
        m_targetChangeViewZoomFactor = -1.0f;
    }

    // Take into account latest requests for offset and zoom factor changes, if any.
    if (m_isTargetHorizontalOffsetValid)
    {
        currentTargetHorizontalOffset = m_targetHorizontalOffset;
    }
    else if (m_targetChangeViewHorizontalOffset >= 0.0)
    {
        currentTargetHorizontalOffset = m_targetChangeViewHorizontalOffset;
    }
    else
    {
        currentTargetHorizontalOffset = currentHorizontalOffset;
    }

    if (m_isTargetVerticalOffsetValid)
    {
        currentTargetVerticalOffset = m_targetVerticalOffset;
    }
    else if (m_targetChangeViewVerticalOffset >= 0.0)
    {
        currentTargetVerticalOffset = m_targetChangeViewVerticalOffset;
    }
    else
    {
        currentTargetVerticalOffset = currentVerticalOffset;
    }

    if (m_isTargetZoomFactorValid)
    {
        currentTargetZoomFactor = m_targetZoomFactor;
    }
    else if (m_targetChangeViewZoomFactor > 0.0f)
    {
        currentTargetZoomFactor = m_targetChangeViewZoomFactor;
    }
    else
    {
        currentTargetZoomFactor = currentZoomFactor;
    }

    if (!m_canManipulateElementsWithAsyncBringIntoViewport)
    {
        // When a projection is set for instance, and DManip is partially turned off, only perform non-animated view changes.
        disableAnimation = TRUE;
    }

    if (!disableAnimation)
    {
        // Do not attempt to animate when OS Settings have turned off animations.
        disableAnimation = !IsAnimationEnabled();
    }

    IFC(GetInnerManipulationDataProvider(&spProvider));
    if (spProvider)
    {
        IFC(spProvider->get_PhysicalOrientation(&orientation));

        // When operating with a IManipulationDataProvider implementation, we do not support animations. Pretend the flag was set to False.
#ifdef DM_DEBUG
        if (!disableAnimation && (DMSVv_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE)) == S_OK))
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE | DMSVv_DBG) /*traceType*/, L"                   Ignoring animation request for IManipulationDataProvider implementation."));
        }
#endif // DM_DEBUG

        disableAnimation = TRUE;

        if ((orientation == xaml_controls::Orientation_Horizontal && pHorizontalOffset) ||
            (orientation == xaml_controls::Orientation_Vertical && pVerticalOffset))
        {
            // Also IManipulationDataProvider does not currently provide a means to compute a pixel-based
            // offset given a logical offset. So DManip ZoomToRect which are pixel-based are skipped in favor
            // of ScrollToHorizontalOffset/ScrollToVerticalOffset/ZoomToFactor calls.
            isBringIntoViewportCallAllowed = FALSE;
        }
    }

    IFC(ComputePixelViewportWidth((spProvider != NULL && orientation == xaml_controls::Orientation_Horizontal) ? spProvider.Get() : NULL, TRUE /*isProviderSet*/, &viewportPixelWidth));
    IFC(ComputePixelViewportHeight((spProvider != NULL && orientation == xaml_controls::Orientation_Vertical) ? spProvider.Get() : NULL, TRUE /*isProviderSet*/, &viewportPixelHeight));

    IFC(get_MinHorizontalOffset(&minHorizontalOffset));
    IFC(get_MinVerticalOffset(&minVerticalOffset));
    IFC(get_MinZoomFactor(&minZoomFactor));
    IFC(get_MaxZoomFactor(&maxZoomFactor));

    if (pZoomFactor)
    {
        IFC(pZoomFactor->get_Value(&targetZoomFactor));

        if (_isnan(targetZoomFactor) || !_finite(targetZoomFactor))
        {
            // Use standard error string "The value cannot be infinite or Not a Number (NaN)."
            IFC(ErrorHelper::OriginateErrorUsingResourceID(E_INVALIDARG, ERROR_INVALID_DOUBLE_VALUE));
        }

        // Clamp the provided value based on MinZoomFactor and MaxZoomFactor
        if (targetZoomFactor < minZoomFactor)
        {
            targetZoomFactor = minZoomFactor;
        }
        else if (targetZoomFactor > maxZoomFactor)
        {
            targetZoomFactor = maxZoomFactor;
        }
    }
    else
    {
        // Use current zoom factor, or latest requested zoom factor, since no target was specified.
        targetZoomFactor = currentTargetZoomFactor;

        // No need to clamp this value based on MinZoomFactor and MaxZoomFactor
        // since the current dependency property is already clamped.
    }

    if (disableAnimation && adjustWithMandatorySnapPoints)
    {
        IFC(AdjustZoomFactorWithMandatorySnapPoints(minZoomFactor, maxZoomFactor, &targetZoomFactor));
    }

    IFC(spScrollInfo->get_CanHorizontallyScroll(&canHorizontallyScroll));
    // Even when canHorizontallyScroll is False, the current horizontal offset may be greater than minHorizontalOffset.
    // This is because IScrollInfo implementations like ItemsPresenter, VirtualizingStackPanel and WrapGrip allow
    // their SetHorizontalOffset to set the offset to values other than minHorizontalOffset, via the ScrollToHorizontalOffset
    // method. The ScrollContentPresenter however forces the offset to be minHorizontalOffset when it is the IScrollInfo implementer.
    // When forceChangeToCurrentView is True (i.e. ScrollViewer::NotifyBringIntoViewportNeeded is being processed to sync up the XAML
    // and DManip transforms) and the ScrollContentPresenter is not the IScrollInfo implementer (i.e. spProvider is set), the fact that
    // get_CanHorizontallyScroll returned false must be overwritten such that a DManip ZoomToRect call with the current horizontal
    // offset is made.
    canHorizontallyScroll |= forceChangeToCurrentView && spProvider != nullptr;
    if (canHorizontallyScroll)
    {
        IFC(get_ViewportWidth(&viewportWidth));
    }
    if (canHorizontallyScroll && viewportWidth != DoubleUtil::PositiveInfinity)
    {
        if (pHorizontalOffset)
        {
            IFC(pHorizontalOffset->get_Value(&targetHorizontalOffset));
            if (_isnan(targetHorizontalOffset) || !_finite(targetHorizontalOffset))
            {
                // Use standard error string "The value cannot be infinite or Not a Number (NaN)."
                IFC(ErrorHelper::OriginateErrorUsingResourceID(E_INVALIDARG, ERROR_INVALID_DOUBLE_VALUE));
            }
            if (targetHorizontalOffset < minHorizontalOffset)
            {
                targetHorizontalOffset = minHorizontalOffset;
            }
        }
        else
        {
            // Use current horizontal offset, or latest requested offset, since no target was specified.
            targetHorizontalOffset = currentTargetHorizontalOffset;
        }
        if (!spProvider || orientation == xaml_controls::Orientation_Vertical)
        {
            // ScrollViewer operates with pixel-based horizontal offsets.
            if (disableAnimation || !_finite(static_cast<XFLOAT>(targetHorizontalOffset / targetZoomFactor)))
            {
                // Clamp the pixel-based horizontal offset so it does not exceed the maximum value.
                IFC(AdjustTargetHorizontalOffset(
                    disableAnimation,
                    adjustWithMandatorySnapPoints,
                    targetZoomFactor,
                    minHorizontalOffset,
                    currentHorizontalOffset,
                    viewportPixelWidth,
                    &targetHorizontalOffset,
                    &currentUnzoomedPixelExtentWidth,
                    &maxHorizontalOffset,
                    &targetExtentWidth));
            }
            targetPixelHorizontalOffset = targetHorizontalOffset;
        }
        else
        {
            // ScrollViewer operates with logical-based horizontal offsets.
            ASSERT(spProvider && orientation == xaml_controls::Orientation_Horizontal);
            ASSERT(disableAnimation);

            if (isBringIntoViewportCallAllowed)
            {
                // The horizontal offset cannot be altered since the panel operates in logical-based units.
                targetPixelHorizontalOffset = (m_xPixelOffsetRequested == -1) ? m_xPixelOffset : m_xPixelOffsetRequested;

                if (m_isInZoomFactorSync)
                {
                    // For horizontal virtualizing panels that use logical-based offsets, the pixel-based target horizontal
                    // offset needs to be adjusted based on the zoom factor change. The logical offset is unchanged though.
                    targetPixelHorizontalOffset *= targetZoomFactor / *pOldZoomFactor;
                }
            }

            if (adjustWithMandatorySnapPoints)
            {
                // Make sure the logical offset snaps to an integer if near mandatory scroll snap points are effective.
                IFC(AdjustLogicalOffsetWithMandatorySnapPoints(
                    TRUE /*isForHorizontalOffset*/,
                    &targetHorizontalOffset));
            }
        }
    }
    else
    {
        // canHorizontallyScroll == False or viewportWidth == DoubleUtil::PositiveInfinity. No matter the requested horizontal offset, it is assumed to be minHorizontalOffset.
        maxHorizontalOffset = minHorizontalOffset;
        targetHorizontalOffset = minHorizontalOffset;
        if (!spProvider || orientation == xaml_controls::Orientation_Vertical)
        {
            // ScrollViewer operates with pixel-based horizontal offsets.
            targetPixelHorizontalOffset = minHorizontalOffset;
        }
    }

    IFC(spScrollInfo->get_CanVerticallyScroll(&canVerticallyScroll));
    // Even when canVerticallyScroll is False, the current vertical offset may be greater than minVerticalOffset.
    // This is because IScrollInfo implementations like ItemsPresenter, VirtualizingStackPanel and WrapGrip allow
    // their SetVerticalOffset to set the offset to values other than minVerticalOffset, via the ScrollToVerticalOffset
    // method. The ScrollContentPresenter however forces the offset to be minVerticalOffset when it is the IScrollInfo
    // implementer. When forceChangeToCurrentView is True (i.e. ScrollViewer::NotifyBringIntoViewportNeeded is being
    // processed to sync up the XAML and DManip transforms) and the ScrollContentPresenter is not the IScrollInfo implementer
    // (i.e. spProvider is set), the fact that get_CanVerticallyScroll returned false must be overwritten such that a
    // DManip ZoomToRect call with the current vertical offset is made.
    canVerticallyScroll |= forceChangeToCurrentView && spProvider != nullptr;
    if (canVerticallyScroll)
    {
        IFC(get_ViewportHeight(&viewportHeight));
    }
    if (canVerticallyScroll && viewportHeight != DoubleUtil::PositiveInfinity)
    {
        if (pVerticalOffset)
        {
            IFC(pVerticalOffset->get_Value(&targetVerticalOffset));
            if (_isnan(targetVerticalOffset) || !_finite(targetVerticalOffset))
            {
                // Use standard error string "The value cannot be infinite or Not a Number (NaN)."
                IFC(ErrorHelper::OriginateErrorUsingResourceID(E_INVALIDARG, ERROR_INVALID_DOUBLE_VALUE));
            }
            if (targetVerticalOffset < minVerticalOffset)
            {
                targetVerticalOffset = minVerticalOffset;
            }
        }
        else
        {
            // Use current horizontal offset, or latest requested offset, since no target was specified.
            targetVerticalOffset = currentTargetVerticalOffset;
        }
        if (!spProvider || orientation == xaml_controls::Orientation_Horizontal)
        {
            // Clamp the pixel-based vertical offset so it does not exceed the maximum value.
            IFC(AdjustTargetVerticalOffset(
                disableAnimation,
                adjustWithMandatorySnapPoints,
                targetZoomFactor,
                minVerticalOffset,
                currentVerticalOffset,
                viewportPixelHeight,
                &targetVerticalOffset,
                &currentUnzoomedPixelExtentHeight,
                &maxVerticalOffset,
                &targetExtentHeight));
            targetPixelVerticalOffset = targetVerticalOffset;
        }
        else
        {
            // ScrollViewer operates with logical-based vertical offsets.
            ASSERT(spProvider && orientation == xaml_controls::Orientation_Vertical);
            ASSERT(disableAnimation);

            if (isBringIntoViewportCallAllowed)
            {
                // The vertical offset cannot be altered since the panel operates in logical-based units.
                targetPixelVerticalOffset = (m_yPixelOffsetRequested == -1) ? m_yPixelOffset : m_yPixelOffsetRequested;

                if (m_isInZoomFactorSync)
                {
                    // For vertical virtualizing panels that use logical-based offsets, the pixel-based target vertical
                    // offset needs to be adjusted based on the zoom factor change. The logical offset is unchanged though.
                    targetPixelVerticalOffset *= targetZoomFactor / *pOldZoomFactor;
                }
            }

            if (adjustWithMandatorySnapPoints)
            {
                // Make sure the logical offset snaps to an integer if near mandatory scroll snap points are effective.
                IFC(AdjustLogicalOffsetWithMandatorySnapPoints(
                    FALSE /*isForHorizontalOffset*/,
                    &targetVerticalOffset));
            }
        }
    }
    else
    {
        // canVerticallyScroll == False or viewportHeight == DoubleUtil::PositiveInfinity. No matter the requested horizontal offset, it is assumed to be minVerticalOffset.
        maxVerticalOffset = minVerticalOffset;
        targetVerticalOffset = minVerticalOffset;
        if (!spProvider || orientation == xaml_controls::Orientation_Horizontal)
        {
            // ScrollViewer operates with pixel-based vertical offsets.
            targetPixelVerticalOffset = minVerticalOffset;
        }
    }

    if (transformIsInertiaEnd &&
        DoubleUtil::Abs(oldTargetChangeViewHorizontalOffset - targetPixelHorizontalOffset) < ScrollViewerScrollRoundingToleranceForBringIntoViewport &&
        DoubleUtil::Abs(oldTargetChangeViewVerticalOffset - targetPixelVerticalOffset) < ScrollViewerScrollRoundingToleranceForBringIntoViewport &&
        DoubleUtil::Abs(oldTargetChangeViewZoomFactor - targetZoomFactor) < ScrollViewerZoomRoundingToleranceForBringIntoViewport)
    {
        // Because of roundings required for calling the DManip ZoomToRect method, the end-of-inertia transform returned by DManip might not exactly match
        // the target of the previous ChangeViewInternal call. Use the original target in order to land exactly where the original ChangeViewInternal caller
        // intended to. This is feasible since disableAnimation is True and DManip's SetContentTransformValues is now invoked instead of ZoomToRect.
        targetPixelHorizontalOffset = targetHorizontalOffset = oldTargetChangeViewHorizontalOffset;
        targetPixelVerticalOffset = targetVerticalOffset = oldTargetChangeViewVerticalOffset;
        targetZoomFactor = oldTargetChangeViewZoomFactor;
    }

    if (DoubleUtil::AreClose(currentTargetHorizontalOffset, targetHorizontalOffset) &&
        DoubleUtil::AreClose(currentTargetVerticalOffset, targetVerticalOffset) &&
        DoubleUtil::AreClose(currentTargetZoomFactor, targetZoomFactor) &&
        !forceChangeToCurrentView)
    {
        // Target view is the current or imminent view
        if (!disableAnimation)
        {
            ASSERT(!spProvider);

            // Check if the target view is illegal from a mandatory snap points respect.
            adjustedTargetHorizontalOffset = targetHorizontalOffset;
            adjustedTargetVerticalOffset = targetVerticalOffset;
            adjustedTargetZoomFactor = targetZoomFactor;

            IFC(AdjustViewWithMandatorySnapPoints(
                minHorizontalOffset,
                maxHorizontalOffset,
                currentHorizontalOffset,
                minVerticalOffset,
                maxVerticalOffset,
                currentVerticalOffset,
                minZoomFactor,
                maxZoomFactor,
                viewportPixelWidth,
                viewportPixelHeight,
                &currentUnzoomedPixelExtentWidth,
                &currentUnzoomedPixelExtentHeight,
                &adjustedTargetHorizontalOffset,
                &adjustedTargetVerticalOffset,
                &adjustedTargetZoomFactor));

            if (DoubleUtil::AreClose(currentTargetHorizontalOffset, adjustedTargetHorizontalOffset) &&
                DoubleUtil::AreClose(currentTargetVerticalOffset, adjustedTargetVerticalOffset) &&
                DoubleUtil::AreClose(currentTargetZoomFactor, adjustedTargetZoomFactor))
            {
                // Current or imminent view would be unaffected by mandatory snap points.
                if (currentTargetZoomFactor != adjustedTargetZoomFactor)
                {
                    // Since the current view is so close to the target view, move directly to it instead of animating.
                    disableAnimation = TRUE;
                    // Use the adjusted view based on the mandatory snap points.
                    targetHorizontalOffset = adjustedTargetHorizontalOffset;
                    targetVerticalOffset = adjustedTargetVerticalOffset;
                    targetZoomFactor = adjustedTargetZoomFactor;
                }
                else
                {
                    // The zoom factor is not changing at all and offsets are close. No need to perform a view change.
                    goto Cleanup;
                }
            }
        }
        else if (currentTargetZoomFactor == targetZoomFactor)
        {
            // The zoom factor is not changing at all and offsets are close. No need to perform a view change.
            goto Cleanup;
        }
    }

    if (disableAnimation && currentTargetZoomFactor == targetZoomFactor && !forceChangeToCurrentView)
    {
        // When only offset jumps are requested, do not use DManip's ZoomToRect
        // to avoid flickers when the developer changes both an extent and offset.
        // Instead use SetOffsetsWithExtents, ScrollToHorizontalOffset or
        // ScrollToVerticalOffset which results in DManip's SetContentRect and no
        // visual flicker.
        isBringIntoViewportCallAllowed = FALSE;
    }

    if (isBringIntoViewportCallAllowed)
    {
        IFC(get_CanManipulateElements(
            &canManipulateElementsByTouch,
            &canManipulateElementsNonTouch,
            &canManipulateElementsWithBringIntoViewport));

        if (canManipulateElementsWithBringIntoViewport && (!applyAsManip || m_canManipulateElementsWithAsyncBringIntoViewport))
        {
            ASSERT(!m_isLoaded || viewportPixelWidth > 0);
            ASSERT(!m_isLoaded || viewportPixelHeight > 0);

            if (viewportPixelWidth > 0 && viewportPixelHeight > 0)
            {
                if (DoubleUtil::Abs(currentTargetHorizontalOffset - targetPixelHorizontalOffset) < ScrollViewerScrollRoundingToleranceForBringIntoViewport &&
                    DoubleUtil::Abs(currentTargetVerticalOffset - targetPixelVerticalOffset) < ScrollViewerScrollRoundingToleranceForBringIntoViewport &&
                    DoubleUtil::Abs(currentTargetZoomFactor - targetZoomFactor) < ScrollViewerZoomRoundingToleranceForBringIntoViewport &&
                    !forceChangeToCurrentView &&
                    !disableAnimation)
                {
                    DOUBLE currentDManipHorizontalOffset = currentTargetHorizontalOffset;
                    DOUBLE currentDManipVerticalOffset = currentTargetVerticalOffset;
                    FLOAT  currentDManipZoomFactor = currentTargetZoomFactor;

                    if (IsInManipulation())
                    {
                        // Access the current DManip view from the DManip Service to check if it coincides with the target view.
                        // Only discard the animation request if those views are close.
                        IFC(GetDManipView(&currentDManipHorizontalOffset, &currentDManipVerticalOffset, &currentDManipZoomFactor));
                    }

                    if (adjustWithMandatorySnapPoints)
                    {
                        // Get adjusted target view based on potential mandatory snap points
                        ASSERT(targetHorizontalOffset == targetPixelHorizontalOffset);
                        ASSERT(targetVerticalOffset == targetPixelVerticalOffset);

                        adjustedTargetHorizontalOffset = targetHorizontalOffset;
                        adjustedTargetVerticalOffset = targetVerticalOffset;
                        adjustedTargetZoomFactor = targetZoomFactor;

                        IFC(AdjustViewWithMandatorySnapPoints(
                            minHorizontalOffset,
                            maxHorizontalOffset,
                            currentHorizontalOffset,
                            minVerticalOffset,
                            maxVerticalOffset,
                            currentVerticalOffset,
                            minZoomFactor,
                            maxZoomFactor,
                            viewportPixelWidth,
                            viewportPixelHeight,
                            &currentUnzoomedPixelExtentWidth,
                            &currentUnzoomedPixelExtentHeight,
                            &adjustedTargetHorizontalOffset,
                            &adjustedTargetVerticalOffset,
                            &adjustedTargetZoomFactor));

                        if (DoubleUtil::Abs(currentDManipHorizontalOffset - adjustedTargetHorizontalOffset) < ScrollViewerScrollRoundingToleranceForBringIntoViewport &&
                            DoubleUtil::Abs(currentDManipVerticalOffset - adjustedTargetVerticalOffset) < ScrollViewerScrollRoundingToleranceForBringIntoViewport &&
                            DoubleUtil::Abs(currentDManipZoomFactor - adjustedTargetZoomFactor) < ScrollViewerZoomRoundingToleranceForBringIntoViewport)
                        {
                            // Adjusted target and current views are too close to each other for BringIntoView to take action.
                            // Since the current view is so close to the target view, move directly to it instead of animating.
                            disableAnimation = TRUE;
                            // Use the adjusted view based on the mandatory snap points.
                            targetHorizontalOffset = adjustedTargetHorizontalOffset;
                            targetVerticalOffset = adjustedTargetVerticalOffset;
                            targetZoomFactor = adjustedTargetZoomFactor;
                        }
                        // Else: Mandatory scroll and zoom snap points affected the target view enough for BringIntoViewport to take effect.
                    }
                    else if (DoubleUtil::Abs(currentDManipHorizontalOffset - targetPixelHorizontalOffset) < ScrollViewerScrollRoundingToleranceForBringIntoViewport &&
                             DoubleUtil::Abs(currentDManipVerticalOffset - targetPixelVerticalOffset) < ScrollViewerScrollRoundingToleranceForBringIntoViewport &&
                             DoubleUtil::Abs(currentDManipZoomFactor - targetZoomFactor) < ScrollViewerZoomRoundingToleranceForBringIntoViewport)
                    {
                        // Target and current views are too close to each other for BringIntoViewport to take action.
                        // No adjustment requested for mandatory snap points. Since the current view is so close to the target view, move directly to it instead of animating.
                        disableAnimation = TRUE;
                    }
                }

                if (disableAnimation)
                {
                    if ((!spProvider || orientation == xaml_controls::Orientation_Vertical) && maxHorizontalOffset == -1.0)
                    {
                        // Clamp the pixel-based horizontal offset so it does not exceed the maximum value.
                        // That clamping was already done if maxHorizontalOffset is no longer initialized to -1.
                        IFC(AdjustTargetHorizontalOffset(
                            TRUE /*disableAnimation*/,
                            adjustWithMandatorySnapPoints,
                            targetZoomFactor,
                            minHorizontalOffset,
                            currentHorizontalOffset,
                            viewportPixelWidth,
                            &targetHorizontalOffset,
                            &currentUnzoomedPixelExtentWidth,
                            &maxHorizontalOffset,
                            &targetExtentWidth));
                        targetPixelHorizontalOffset = targetHorizontalOffset;
                    }

                    if ((!spProvider || orientation == xaml_controls::Orientation_Horizontal) && maxVerticalOffset == -1.0)
                    {
                        // Clamp the pixel-based vertical offset so it does not exceed the maximum value.
                        // That clamping was already done if maxVerticalOffset is no longer initialized to -1.
                        IFC(AdjustTargetVerticalOffset(
                            TRUE /*disableAnimation*/,
                            adjustWithMandatorySnapPoints,
                            targetZoomFactor,
                            minVerticalOffset,
                            currentVerticalOffset,
                            viewportPixelHeight,
                            &targetVerticalOffset,
                            &currentUnzoomedPixelExtentHeight,
                            &maxVerticalOffset,
                            &targetExtentHeight));
                        targetPixelVerticalOffset = targetVerticalOffset;
                    }
                }

                bounds.X = static_cast<XFLOAT>(targetPixelHorizontalOffset / targetZoomFactor);
                bounds.Width = static_cast<XFLOAT>(viewportPixelWidth / targetZoomFactor);

                bounds.Y = static_cast<XFLOAT>(targetPixelVerticalOffset / targetZoomFactor);
                bounds.Height = static_cast<XFLOAT>(viewportPixelHeight / targetZoomFactor);

                if (disableAnimation || skipAnimationWhileRunning)
                {
                    targetTranslateX = static_cast<FLOAT>(-targetPixelHorizontalOffset);

                    // Adjust target content bounds based on content alignments.
                    IFC(ComputeHorizontalAlignment(TRUE /*canUseCachedProperties*/, alignment));
                    alignment = static_cast<DMAlignment>(alignment & ~DMAlignmentUnlockCenter);
                    if (alignment != DMAlignmentNear)
                    {
                        // Compute target pixel extent width
                        if (currentUnzoomedPixelExtentWidth < 0)
                        {
                            IFC(ComputePixelExtentWidth(
                                true /*ignoreZoomFactor*/,
                                (spProvider && orientation == xaml_controls::Orientation_Horizontal) ? spProvider.Get() : NULL,
                                &currentUnzoomedPixelExtentWidth));
                        }

                        ASSERT(currentUnzoomedPixelExtentWidth >= 0);

                        // Round the content width up similarly to what the DManip Service does since DManip only consumes integral dimensions.
                        currentUnzoomedPixelExtentWidth = AdjustPixelContentDim(currentUnzoomedPixelExtentWidth);

                        targetExtentWidth = currentUnzoomedPixelExtentWidth * targetZoomFactor;

                        ASSERT(targetExtentWidth >= 0);

                        // Round the viewport width down similarly to what the DManip Service does
                        // since DManip only consumes integral dimensions.
                        viewportPixelWidth = AdjustPixelViewportDim(viewportPixelWidth);
                        if (targetExtentWidth < viewportPixelWidth)
                        {
                            bounds.X = static_cast<XFLOAT>((targetExtentWidth - viewportPixelWidth) / targetZoomFactor);
                            targetTranslateX = static_cast<FLOAT>(viewportPixelWidth - targetExtentWidth);
                            if (alignment == DMAlignmentCenter)
                            {
                                bounds.X /= 2.0f;
                                targetTranslateX /= 2.0f;
                            }
                        }
                    }

                    targetTranslateY = static_cast<FLOAT>(-targetPixelVerticalOffset);

                    IFC(ComputeVerticalAlignment(TRUE /*canUseCachedProperties*/, alignment));
                    alignment = static_cast<DMAlignment>(alignment & ~DMAlignmentUnlockCenter);
                    if (alignment != DMAlignmentNear)
                    {
                        // Compute target pixel extent height
                        if (currentUnzoomedPixelExtentHeight < 0)
                        {
                            IFC(ComputePixelExtentHeight(
                                true /*ignoreZoomFactor*/,
                                (spProvider && orientation == xaml_controls::Orientation_Vertical) ? spProvider.Get() : NULL,
                                &currentUnzoomedPixelExtentHeight));
                        }

                        ASSERT(currentUnzoomedPixelExtentHeight >= 0);

                        // Round the content height up similarly to what the DManip Service does since DManip only consumes integral dimensions.
                        currentUnzoomedPixelExtentHeight = AdjustPixelContentDim(currentUnzoomedPixelExtentHeight);

                        targetExtentHeight = currentUnzoomedPixelExtentHeight * targetZoomFactor;

                        ASSERT(targetExtentHeight >= 0);

                        // Round the viewport height down similarly to what the DManip Service does
                        // since DManip only consumes integral dimensions.
                        viewportPixelHeight = AdjustPixelViewportDim(viewportPixelHeight);
                        if (targetExtentHeight < viewportPixelHeight)
                        {
                            bounds.Y = static_cast<XFLOAT>((targetExtentHeight - viewportPixelHeight) / targetZoomFactor);
                            targetTranslateY = static_cast<FLOAT>(viewportPixelHeight - targetExtentHeight);
                            if (alignment == DMAlignmentCenter)
                            {
                                bounds.Y /= 2.0f;
                                targetTranslateY /= 2.0f;
                            }
                        }
                    }

                    // Batch up any potential ViewChanging and ViewChanged notifications resulting from the BringIntoViewport call,
                    // since the DManip setup may call UpdateLayout on the ScrollContentPresenter.
                    DelayViewChanging();
                    isViewChangingDelayed = TRUE;
                    DelayViewChanged();
                    isViewChangedDelayed = TRUE;

                    if (!m_isInChangeViewBringIntoViewport)
                    {
                        m_isInChangeViewBringIntoViewport = TRUE;
                        clearInChangeViewBringIntoViewport = TRUE;
                    }

                    ASSERT(isBringIntoViewportCallAllowed);
                    IFC(BringIntoViewportInternal(
                        bounds,
                        targetTranslateX, targetTranslateY, targetZoomFactor,
                        disableAnimation || skipAnimationWhileRunning /*transformIsValid*/,
                        skipDuringTouchContact,
                        skipAnimationWhileRunning,
                        !disableAnimation /*animate*/,
                        applyAsManip,
                        isForMakeVisible,
                        &isHandled));
                    isBringIntoViewportCalled = TRUE;

                    if (clearInChangeViewBringIntoViewport)
                    {
                        ASSERT(m_isInChangeViewBringIntoViewport);
                        m_isInChangeViewBringIntoViewport = FALSE;
                        clearInChangeViewBringIntoViewport = FALSE;
                    }

                    if (isHandled)
                    {
                        // Set the offset expectations for the first imminent HandleManipulationDelta execution
                        if (currentTargetHorizontalOffset != currentHorizontalOffset)
                        {
                            m_xPixelOffsetRequested = currentTargetHorizontalOffset;
#ifdef DM_DEBUG
                            if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
                            {
                                IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/,
                                    L"                   Set m_xPixelOffsetRequested=%f.", m_xPixelOffsetRequested));
                            }
#endif // DM_DEBUG
                        }
                        if (currentTargetVerticalOffset != currentVerticalOffset)
                        {
                            m_yPixelOffsetRequested = currentTargetVerticalOffset;
#ifdef DM_DEBUG
                            if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
                            {
                                IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/,
                                    L"                   Set m_yPixelOffsetRequested=%f.", m_yPixelOffsetRequested));
                            }
#endif // DM_DEBUG
                        }

                        if (m_isInDirectManipulationSync)
                        {
                            if (m_isTargetZoomFactorValid &&
                                targetZoomFactor == m_targetZoomFactor &&
                                currentZoomFactor != targetZoomFactor)
                            {
                                // Immediately update the ZoomFactor dependency property so that the next ViewChanged event exposes
                                // the value just synced with DManip instead of the prior one.
                                IFC(ScrollViewerGenerated::put_ZoomFactor(targetZoomFactor));
                            }
                        }
                        else
                        {
                            // Store the requested offsets/zoom factor so any subsequent ChangeView call to the same target can be skipped.
                            // No view change is expected though when this operation is sync'ing DManip with XAML.
                            m_targetChangeViewHorizontalOffset = targetPixelHorizontalOffset;
                            m_targetChangeViewVerticalOffset = targetPixelVerticalOffset;
                            m_targetChangeViewZoomFactor = targetZoomFactor;
                        }
                    }
                    else
                    {
                        if (!applyAsManip && m_isInDirectManipulationCompletion)
                        {
                            // The BringIntoViewport operation was delayed until the next UI thread tick or manipulation setup.
                            goto Cleanup;
                        }
                        if (m_targetChangeViewHorizontalOffset == targetPixelHorizontalOffset &&
                            m_targetChangeViewVerticalOffset == targetPixelVerticalOffset &&
                            m_targetChangeViewZoomFactor == targetZoomFactor)
                        {
                            // No action was taken by BringIntoViewport because a prior call to ChangeView already moved to that
                            // target view. No need to call ZoomToFactorInternal or SetOffsetsWithExtents.
                            goto Cleanup;
                        }
                    }
                }
            }
        }
    }

    if (!isBringIntoViewportCalled || !isHandled)
    {
        // This ScrollViewer or its Content might not be in the tree.
        ASSERT(spScrollInfo);
        IFC(GetContentUIElement(&spContentUIElement));
        if (spContentUIElement)
        {
            // Do not attempt a non-animated move to the requested view if there is no Content present.
            // This is to minimize the discrepancy between ZoomToFactor and ScrollToHorizontalOffset/
            // ScrollToVerticalOffset: ZoomToFactor is effective even when there is no Content, while
            // ScrollToHorizontalOffset/ScrollToVerticalOffset are not.

            // Batch up any potential ViewChanging and ViewChanged notifications resulting from calls to ZoomToFactor,
            // SetOffsetsWithExtents, SetHorizontalOffset and SetVerticalOffset into a single notification.
            if (!isViewChangingDelayed)
            {
                DelayViewChanging();
                isViewChangingDelayed = TRUE;
            }
            if (!isViewChangedDelayed)
            {
                DelayViewChanged();
                isViewChangedDelayed = TRUE;
            }

            IFC(GetScrollInfoAsElement(&spScrollInfoAsElement));
            IFC(IsScrollContentPresenterScrollClient(isScrollContentPresenterScrollClient));

            if (!disableAnimation && adjustWithMandatorySnapPoints)
            {
                IFC(AdjustViewWithMandatorySnapPoints(
                    minHorizontalOffset,
                    maxHorizontalOffset,
                    currentHorizontalOffset,
                    minVerticalOffset,
                    maxVerticalOffset,
                    currentVerticalOffset,
                    minZoomFactor,
                    maxZoomFactor,
                    viewportPixelWidth,
                    viewportPixelHeight,
                    &currentUnzoomedPixelExtentWidth,
                    &currentUnzoomedPixelExtentHeight,
                    &targetHorizontalOffset,
                    &targetVerticalOffset,
                    &targetZoomFactor));
            }

            bool zoomChanged = false;

            // First take care of the potential zoom factor change so offsets can be adjusted accordingly.
            IFC(ZoomToFactorInternal(targetZoomFactor, TRUE /*delayAndFlushViewChanged*/, &zoomChanged));

            if (isScrollContentPresenterScrollClient)
            {
                if (m_trElementScrollContentPresenter)
                {
                    // Jump to the target offsets
                    IFC(m_trElementScrollContentPresenter.Cast<ScrollContentPresenter>()->SetOffsetsWithExtents(targetHorizontalOffset, targetVerticalOffset, currentUnzoomedPixelExtentWidth * targetZoomFactor, currentUnzoomedPixelExtentHeight * targetZoomFactor));
                }
            }
            else
            {
                // Make sure the latest zoom factor gets applied so the SetHorizontalOffset/SetVerticalOffset
                // can operate with the accurate zoom factor.
                // We can skip this if the zoom factor didn't actually change.
                if (spScrollInfoAsElement && zoomChanged)
                {
                    IFC(spScrollInfoAsElement->UpdateLayout());
                }

                // Jump to the target offsets
                IFC(spScrollInfo->SetHorizontalOffset(targetHorizontalOffset));
                IFC(spScrollInfo->SetVerticalOffset(targetVerticalOffset));
            }

            ASSERT(isViewChangingDelayed);
            ASSERTSUCCEEDED(hr);
            isViewChangingDelayed = FALSE;
            IFC(FlushViewChanging(S_OK));

            ASSERT(isViewChangedDelayed);
            ASSERTSUCCEEDED(hr);
            isViewChangedDelayed = FALSE;
            IFC(FlushViewChanged(S_OK));

            if (IsInManipulation() && spScrollInfoAsElement)
            {
                // Make sure the offset change request gets applied through a ScrollViewer.InvalidateScrollInfo call
                // whenever there is an active manipulation, so the next DManip feedback does not override the changes
                // requested. This situation occurs for instance when DManip's ZoomToRect was skipped because of a touch contact.
                // A non-animated move was accomplished instead.
                ASSERT(!isHandled);
                IFC(spScrollInfoAsElement->UpdateLayout());
            }
            isHandled = TRUE;
        }
    }

    *pReturnValue = isHandled;

Cleanup:
    m_isInDirectManipulationSync = FALSE;
    if (isViewChangingDelayed)
    {
        hr = FlushViewChanging(hr);
    }
    if (isViewChangedDelayed)
    {
        hr = FlushViewChanged(hr);
    }
    if (clearInChangeViewBringIntoViewport)
    {
        ASSERT(m_isInChangeViewBringIntoViewport);
        m_isInChangeViewBringIntoViewport = FALSE;
    }

#ifdef DM_DEBUG
    if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/, L"DMSV[0x%p]:  ChangeViewInternal - exit with returnValue=%d.", this, *pReturnValue));
    }
#endif // DM_DEBUG

    RRETURN(hr);
}

// This function is called by an IScrollInfo attached to this
// ScrollViewer when any values of scrolling properties (Offset, Extent,
// and ViewportSize) change.  The function schedules invalidation of
// other elements like ScrollBars that are dependant on these properties.
_Check_return_ HRESULT ScrollViewer::InvalidateScrollInfoImpl() noexcept
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IManipulationDataProvider> spProvider;
    ctl::ComPtr<IScrollInfo> spScrollInfo;
    BOOLEAN changed = FALSE;
    BOOLEAN updateOffsetX = FALSE;
    BOOLEAN updateOffsetY = FALSE;
    BOOLEAN updatePixelViewportX = FALSE;
    BOOLEAN updatePixelViewportY = FALSE;
    BOOLEAN updatePixelExtentX = FALSE;
    BOOLEAN updatePixelExtentY = FALSE;
    DOUBLE xOldPixelOffset = m_xPixelOffset;
    DOUBLE yOldPixelOffset = m_yPixelOffset;
    HRESULT tryUpdateResult = S_OK;

#ifdef DM_DEBUG
    if (DMSVv_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE)) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE | DMSVv_DBG) /*traceType*/, L"DMSVv[0x%p]: InvalidateScrollInfoImpl entry.", this));
    }
#endif // DM_DEBUG

    // Batch up any potential ViewChanged events into a single notification
    DelayViewChanged();

    IFC(get_ScrollInfo(&spScrollInfo));
    if (!spScrollInfo)
    {
        goto Cleanup;
    }

    // !_inMeasure tells us that ScrollInfo has recomputed its extent/viewport
    // incrementally, not as a result of remeasuring by this ScrollViewer.
    // that means we should re-run the logic of determining visibility of
    // autoscrollbars, if we have them.
    if (!m_inMeasure)
    {
        // Check if we should remove/add scrollbars.
        DOUBLE extent = 0.0;
        DOUBLE viewportWidth = 0.0;
        DOUBLE viewportHeight = 0.0;
        DOUBLE minHorizontalOffset = 0.0;
        DOUBLE minVerticalOffset = 0.0;
        xaml_controls::ScrollBarVisibility visibility = xaml_controls::ScrollBarVisibility_Disabled;
        // As in ScrollViewer::MeasureOverride, a roundingStep is added to the viewport size to re-evaluate the scrollbar's effective visibility.
        // In particular, InvalidateMeasure needs to be called when the extent goes from viewport + roundingStep + epsilon to viewport + roundingStep - epsilon.
        const float roundingStep = 1.0f / RootScale::GetRasterizationScaleForElement(GetHandle());

        IFC(spScrollInfo->get_ViewportWidth(&viewportWidth));
        IFC(spScrollInfo->get_ViewportHeight(&viewportHeight));

        IFC(spScrollInfo->get_ExtentWidth(&extent));
        IFC(spScrollInfo->get_MinHorizontalOffset(&minHorizontalOffset));
        IFC(get_HorizontalScrollBarVisibility(&visibility));

        if (visibility == xaml_controls::ScrollBarVisibility_Auto &&
            ((m_scrollVisibilityX == xaml::Visibility_Collapsed && extent - minHorizontalOffset > viewportWidth + roundingStep && viewportWidth > 0.0 && viewportHeight > 0.0) ||
            (m_scrollVisibilityX == xaml::Visibility_Visible && extent - minHorizontalOffset <= viewportWidth + roundingStep)))
        {
            IFC(InvalidateMeasure());
        }
        else
        {
            IFC(spScrollInfo->get_ExtentHeight(&extent));
            IFC(spScrollInfo->get_MinVerticalOffset(&minVerticalOffset));
            IFC(get_VerticalScrollBarVisibility(&visibility));

            if (visibility == xaml_controls::ScrollBarVisibility_Auto &&
                ((m_scrollVisibilityY == xaml::Visibility_Collapsed && extent - minVerticalOffset > viewportHeight + roundingStep && viewportWidth > 0.0 && viewportHeight > 0.0) ||
                (m_scrollVisibilityY == xaml::Visibility_Visible && extent - minVerticalOffset <= viewportHeight + roundingStep)))
            {
                IFC(InvalidateMeasure());
            }
        }
    }

    tryUpdateResult = InvalidateScrollInfo_TryUpdateValues(
        spScrollInfo.Get(),
        changed,
        updateOffsetX,
        updateOffsetY,
        updatePixelViewportX,
        updatePixelViewportY,
        updatePixelExtentX,
        updatePixelExtentY);
    if (changed)
    {
        DOUBLE pixelDelta = 0;
        BOOLEAN dragging = FALSE;
        BOOLEAN isExtentXChangeExpected = FALSE;
        BOOLEAN isExtentYChangeExpected = FALSE;
        BOOLEAN areExtentChangesExpected = FALSE;
        BOOLEAN areExtentChangesExpectedEvaluated = FALSE;
        BOOLEAN translationXChanged = FALSE;
        BOOLEAN translationYChanged = FALSE;
        BOOLEAN zoomFactorChanged = FALSE;
        BOOLEAN isScrollContentPresenterScrollClient = FALSE;
        BOOLEAN areScrollOffsetsInSync = FALSE;

        if (m_trElementHorizontalScrollBar)
        {
            IFC(m_trElementHorizontalScrollBar.Cast<ScrollBar>()->get_IsDragging(&dragging));
            if (!dragging)
            {
                IFC(m_trElementHorizontalScrollBar.Cast<ScrollBar>()->put_Minimum(m_xMinOffset));
            }
        }

        if (m_trElementVerticalScrollBar)
        {
            IFC(m_trElementVerticalScrollBar.Cast<ScrollBar>()->get_IsDragging(&dragging));
            if (!dragging)
            {
                IFC(m_trElementVerticalScrollBar.Cast<ScrollBar>()->put_Minimum(m_yMinOffset));
            }
        }

        if (updateOffsetX)
        {
            if (m_trElementHorizontalScrollBar)
            {
                DOUBLE currentScrollBarValue = 0;
                IFC(m_trElementHorizontalScrollBar.Cast<ScrollBar>()->get_IsDragging(&dragging));
                if (dragging)
                {
                    // What appears to be a programmatic scroll request came in during dragging
                    // Possibly tell the ScrollBar to adjust its drag value
                    IFC(m_trElementHorizontalScrollBar.Cast<ScrollBar>()->get_Value(&currentScrollBarValue));
                }
                if (!dragging || !DoubleUtil::AreClose(currentScrollBarValue, m_xOffset))
                {
                    // ScrollBar doesn't respond to TemplateBinding bound Value
                    // changes during the Scroll event
                    IFC(m_trElementHorizontalScrollBar.Cast<ScrollBar>()->put_Value(m_xOffset));
                    if (dragging)
                    {
                        IFC(m_trElementHorizontalScrollBar.Cast<ScrollBar>()->AdjustDragValue(m_xOffset - currentScrollBarValue));
                    }
                }
            }

            IFC(UpdateInputPaneOffsetX());

            if (!m_isOffsetChangeIgnored)
            {
                if (IsInManipulation())
                {
                    pixelDelta = (m_xPixelOffsetRequested == -1) ? DoubleUtil::Abs(xOldPixelOffset - m_xPixelOffset) : DoubleUtil::Abs(m_xPixelOffsetRequested - m_xPixelOffset);
                    if (pixelDelta > ScrollViewerScrollRoundingTolerance)
                    {
                        // When there is a IManipulationDataProvider implementation in horizontal orientation, the tolerance is augmented to 1 pixel to workaround bug 791631.
                        IFC(GetInnerManipulationDataProvider(TRUE /*isForHorizontalOrientation*/, &spProvider));
                        if (!spProvider || pixelDelta >= ScrollViewerScrollRoundingToleranceForProvider)
                        {
#ifdef DM_DEBUG
                            if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
                            {
                                IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/,
                                    L"DMSV[0x%p]:  InvalidateScrollInfoImpl Unexpected horizontal pixel offset change xOldPixelOffset=%f m_xPixelOffsetRequested=%f m_xPixelOffset=%f.",
                                    this, xOldPixelOffset, m_xPixelOffsetRequested, m_xPixelOffset));
                            }
#endif // DM_DEBUG

                            translationXChanged = TRUE;
                        }
                        spProvider = nullptr;
                    }
                }
                else
                {
                    pixelDelta = (m_xPixelOffsetRequested == -1) ? DoubleUtil::Abs(xOldPixelOffset - m_xPixelOffset) : DoubleUtil::Abs(m_xPixelOffsetRequested - m_xPixelOffset);
                    if (!DoubleUtil::IsZero(pixelDelta))
                    {
                        // When there is a IManipulationDataProvider implementation in horizontal orientation, the tolerance is augmented to 1 pixel to workaround bug 791631.
                        IFC(GetInnerManipulationDataProvider(TRUE /*isForHorizontalOrientation*/, &spProvider));
                        if (!spProvider || pixelDelta >= ScrollViewerScrollRoundingToleranceForProvider)
                        {
#ifdef DM_DEBUG
                            if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
                            {
                                IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/,
                                    L"DMSV[0x%p]:  InvalidateScrollInfoImpl Out-of-manip horizontal pixel offset change xOldPixelOffset=%f m_xPixelOffsetRequested=%f m_xPixelOffset=%f.",
                                    this, xOldPixelOffset, m_xPixelOffsetRequested, m_xPixelOffset));
                            }
#endif // DM_DEBUG

                            translationXChanged = TRUE;
                        }
                        spProvider = nullptr;
                    }
                }
            }

            // No more pending request for a horizontal scroll.
#ifdef DM_DEBUG
            if ((DMSVv_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE)) == S_OK) && m_xPixelOffsetRequested != -1)
            {
                IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE | DMSVv_DBG) /*traceType*/,
                    L"DMSVv[0x%p]: InvalidateScrollInfoImpl resetting m_xPixelOffsetRequested=%f to -1.", this, m_xPixelOffsetRequested));
            }
#endif // DM_DEBUG

            m_xPixelOffsetRequested = -1;
        }

        if (updateOffsetY)
        {
            if (m_trElementVerticalScrollBar)
            {
                DOUBLE currentScrollBarValue = 0;
                IFC(m_trElementVerticalScrollBar.Cast<ScrollBar>()->get_IsDragging(&dragging));
                if (dragging)
                {
                    // What appears to be a programmatic scroll request came in during dragging
                    // Possibly tell the ScrollBar to adjust its drag value
                    IFC(m_trElementVerticalScrollBar.Cast<ScrollBar>()->get_Value(&currentScrollBarValue));
                }
                if (!dragging || !DoubleUtil::AreClose(currentScrollBarValue, m_yOffset))
                {
                    // ScrollBar doesn't respond to TemplateBinding bound Value
                    // changes during the Scroll event
                    IFC(m_trElementVerticalScrollBar.Cast<ScrollBar>()->put_Value(m_yOffset));
                    if (dragging)
                    {
                        IFC(m_trElementVerticalScrollBar.Cast<ScrollBar>()->AdjustDragValue(m_yOffset - currentScrollBarValue));
                    }
                }
            }

            IFC(UpdateInputPaneOffsetY());

            if (!m_isOffsetChangeIgnored)
            {
                if (IsInManipulation())
                {
                    pixelDelta = (m_yPixelOffsetRequested == -1) ? DoubleUtil::Abs(yOldPixelOffset - m_yPixelOffset) : DoubleUtil::Abs(m_yPixelOffsetRequested - m_yPixelOffset);
                    if (pixelDelta > ScrollViewerScrollRoundingTolerance)
                    {
                        // When there is a IManipulationDataProvider implementation in vertical orientation, the tolerance is augmented to 1 pixel to workaround bug 791631.
                        ASSERT(!spProvider);
                        IFC(GetInnerManipulationDataProvider(FALSE /*isForHorizontalOrientation*/, &spProvider));
                        if (!spProvider || pixelDelta >= ScrollViewerScrollRoundingToleranceForProvider)
                        {
#ifdef DM_DEBUG
                            if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
                            {
                                IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/,
                                    L"DMSV[0x%p]:  InvalidateScrollInfoImpl Unexpected vertical pixel offset change yOldPixelOffset=%f m_yPixelOffsetRequested=%f m_yPixelOffset=%f.",
                                    this, yOldPixelOffset, m_yPixelOffsetRequested, m_yPixelOffset));
                            }
#endif // DM_DEBUG

                            translationYChanged = TRUE;
                        }
                    }
                }
                else
                {
                    pixelDelta = (m_yPixelOffsetRequested == -1) ? DoubleUtil::Abs(yOldPixelOffset - m_yPixelOffset) : DoubleUtil::Abs(m_yPixelOffsetRequested - m_yPixelOffset);
                    if (!DoubleUtil::IsZero(pixelDelta))
                    {
                        // When there is a IManipulationDataProvider implementation in vertical orientation, the tolerance is augmented to 1 pixel to workaround bug 791631.
                        ASSERT(!spProvider);
                        IFC(GetInnerManipulationDataProvider(FALSE /*isForHorizontalOrientation*/, &spProvider));
                        if (!spProvider || pixelDelta >= ScrollViewerScrollRoundingToleranceForProvider)
                        {
#ifdef DM_DEBUG
                            if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
                            {
                                IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/,
                                    L"DMSV[0x%p]:  InvalidateScrollInfoImpl Out-of-manip vertical pixel offset change yOldPixelOffset=%f m_yPixelOffsetRequested=%f m_yPixelOffset=%f.",
                                    this, yOldPixelOffset, m_yPixelOffsetRequested, m_yPixelOffset));
                            }
#endif // DM_DEBUG

                            translationYChanged = TRUE;
                        }
                    }
                }
            }

            // No more pending request for a vertical scroll.
#ifdef DM_DEBUG
            if ((DMSVv_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE)) == S_OK) && m_yPixelOffsetRequested != -1)
            {
                IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE | DMSVv_DBG) /*traceType*/,
                    L"DMSVv[0x%p]: InvalidateScrollInfoImpl resetting m_yPixelOffsetRequested=%f to -1.", this, m_yPixelOffsetRequested));
            }
#endif // DM_DEBUG

            m_yPixelOffsetRequested = -1;
        }

        if (updatePixelViewportX || updatePixelExtentX || updatePixelViewportY || updatePixelExtentY)
        {
            // WPB bugs 261102 & 342668 - DManip does not update the output transform when a content size changes
            // and its pan configuration is turned off. Set a flag to force the pan configuration to be used when the
            // alignment is Center or Far. The normal configuration is then restored later when the viewport has been
            // active and the count reaches 0.
            // Changes are ignored outside the context of a manipulation otherwise panning may be incorrectly enabled at the next manipulation.
            if (IsInDirectManipulation())
            {
                if (!updatePixelViewportX && !updatePixelViewportY)
                {
                    // Determine if the extent changes are due to DManip-driven zooming.
                    ASSERT(updatePixelExtentX || updatePixelExtentY);
                    if (updatePixelExtentX)
                    {
                        IFC(IsExtentXChangeExpected(isExtentXChangeExpected));
                    }
                    if (updatePixelExtentY)
                    {
                        IFC(IsExtentYChangeExpected(isExtentYChangeExpected));
                    }
                    areExtentChangesExpected = (!updatePixelExtentX || isExtentXChangeExpected) && (!updatePixelExtentY || isExtentYChangeExpected);
                    areExtentChangesExpectedEvaluated = TRUE;
                }
                // Ignore expected width changes caused by DManip-driven zooming.
                if (m_cForcePanXConfiguration == 0 && ((updatePixelExtentX && !isExtentXChangeExpected) || updatePixelViewportX))
                {
                    DMAlignment alignment = DMAlignmentNone;
                    IFC(GetEffectiveHorizontalAlignment(TRUE /*canUseCachedProperty*/, alignment));
                    if ((alignment == DMAlignmentCenter || alignment == DMAlignmentFar) &&
                        (m_currentHorizontalScrollMode == xaml_controls::ScrollMode_Auto ||
                         (m_currentHorizontalScrollMode == xaml_controls::ScrollMode_Disabled && m_currentVerticalScrollMode != xaml_controls::ScrollMode_Disabled)))
                    {
                        m_cForcePanXConfiguration = 2; // Picking 2 instead of 1 to guarantee that DManip has time to update its output transform.
                    }
                }
                // Ignore expected height changes caused by DManip-driven zooming.
                if (m_cForcePanYConfiguration == 0 && ((updatePixelExtentY && !isExtentYChangeExpected) || updatePixelViewportY))
                {
                    DMAlignment alignment = DMAlignmentNone;
                    IFC(GetEffectiveVerticalAlignment(TRUE /*canUseCachedProperty*/, alignment));
                    if ((alignment == DMAlignmentCenter || alignment == DMAlignmentFar) &&
                        (m_currentVerticalScrollMode == xaml_controls::ScrollMode_Auto ||
                         (m_currentVerticalScrollMode == xaml_controls::ScrollMode_Disabled && m_currentHorizontalScrollMode != xaml_controls::ScrollMode_Disabled)))
                    {
                        m_cForcePanYConfiguration = 2;
                    }
                }
            }

            // content and viewport sizes can affect the configurations outside a manipulation,
            // and the active configuration in general.
            IFC(OnManipulatabilityAffectingPropertyChanged(
                NULL  /*pIsInLiveTree*/,
                FALSE /*isCachedPropertyChanged*/,
                FALSE /*isContentChanged*/,
                !IsInDirectManipulation() /*isAffectingConfigurations*/,
                TRUE  /*isAffectingTouchConfiguration*/));
            IFC(OnContentAlignmentAffectingPropertyChanged(
                updatePixelViewportX || updatePixelExtentX /*isForHorizontalAlignment*/,
                updatePixelViewportY || updatePixelExtentY /*isForVerticalAlignment*/));
        }

        if (updatePixelViewportX || updatePixelViewportY)
        {
            IFC(OnViewportAffectingPropertyChanged(
                TRUE  /*boundsChanged*/,
                FALSE /*touchConfigurationChanged*/,
                FALSE /*nonTouchConfigurationChanged*/,
                FALSE /*configurationsChanged*/,
                FALSE /*chainedMotionTypesChanged*/,
                FALSE /*horizontalOverpanModeChanged*/,
                FALSE /*verticalOverpanModeChanged*/,
                NULL  /*pAreConfigurationsUpdated*/));
            if (!IsInManipulation())
            {
                translationXChanged = translationYChanged = zoomFactorChanged = TRUE;
            }
            else
            {
                if (updatePixelViewportX && !updateOffsetX && m_xPixelOffset == 0)
                {
                    // Even though the horizontal offset remains 0, the viewport width may
                    // have gotten larger than the content width which requires a call
                    // to OnPrimaryContentTransformChanged.
                    translationXChanged = TRUE;
                }
                if (updatePixelViewportY && !updateOffsetY && m_yPixelOffset == 0)
                {
                    // Even though the vertical offset remains 0, the viewport height may
                    // have gotten larger than the content height which requires a call
                    // to OnPrimaryContentTransformChanged.
                    translationYChanged = TRUE;
                }
            }
        }

        if (updatePixelExtentX || updatePixelExtentY)
        {
            if (!areExtentChangesExpectedEvaluated)
            {
                IFC(AreExtentChangesExpected(areExtentChangesExpected));
            }
            if (!areExtentChangesExpected)
            {
#ifdef DM_DEBUG
                if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
                {
                    IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/, L"DMSV[0x%p]:  InvalidateScrollInfoImpl Unexpected extent change.", this));
                }
#endif // DM_DEBUG

                IFC(OnPrimaryContentAffectingPropertyChanged(
                    TRUE  /*boundsChanged*/,
                    FALSE /*horizontalAlignmentChanged*/,
                    FALSE /*verticalAlignmentChanged*/,
                    FALSE /*zoomFactorBoundaryChanged*/));
                if (!IsInManipulation())
                {
                    // When the zoom factor gets reduced when a scroll offset is large enough to no longer accommodate the lower zoom factor,
                    // the ScrollData::m_Offset and ScrollData::m_ComputedOffset fields can remain out of sync.
                    IFC(IsScrollContentPresenterScrollClient(isScrollContentPresenterScrollClient));
                    if (isScrollContentPresenterScrollClient)
                    {
                        // isScrollContentPresenterScrollClient==True guarantees that m_trElementScrollContentPresenter!=null
                        IFC(m_trElementScrollContentPresenter.Cast<ScrollContentPresenter>()->AreScrollOffsetsInSync(areScrollOffsetsInSync));
                        if (!areScrollOffsetsInSync)
                        {
#ifdef DM_DEBUG
                            if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
                            {
                                IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/, L"DMSV[0x%p]:  InvalidateScrollInfoImpl ScrollContentPresenter's AreScrollOffsetsInSync==FALSE.", this));
                            }
#endif // DM_DEBUG

                            // Synchronize the ScrollData::m_Offset and ScrollData::m_ComputedOffset fields.
                            IFC(SynchronizeScrollOffsets());
                        }
                    }

                    translationXChanged = translationYChanged = zoomFactorChanged = TRUE;
                }
                else
                {
                    if (updatePixelExtentX && !updateOffsetX && m_xPixelOffset == 0)
                    {
                        // Even though the horizontal offset remains 0, the content width may
                        // have gotten smaller than the viewport width which requires a call
                        // to OnPrimaryContentTransformChanged.
                        translationXChanged = TRUE;
                    }
                    if (updatePixelExtentY && !updateOffsetY && m_yPixelOffset == 0)
                    {
                        // Even though the vertical offset remains 0, the content height may
                        // have gotten smaller than the viewport height which requires a call
                        // to OnPrimaryContentTransformChanged.
                        translationYChanged = TRUE;
                    }
                }
            }

            if (IsInDirectManipulation())
            {
                // Update the pending width/height requests for a zoom factor change.
                if (!m_trElementScrollContentPresenter ||
                    !m_trElementScrollContentPresenter.Cast<ScrollContentPresenter>()->IsChildActualWidthUsedAsExtent() ||
                    m_trElementScrollContentPresenter.Cast<ScrollContentPresenter>()->IsChildActualWidthUpdated())
                {
#ifdef DM_DEBUG
                    if (DMSVv_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE)) == S_OK)
                    {
                        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE | DMSVv_DBG) /*traceType*/,
                            L"DMSVv[0x%p]: InvalidateScrollInfoImpl resetting m_contentWidthRequested=%f to -1.", this, m_contentWidthRequested));
                    }
#endif // DM_DEBUG
                    m_contentWidthRequested = -1;
                }

                if (!m_trElementScrollContentPresenter ||
                    !m_trElementScrollContentPresenter.Cast<ScrollContentPresenter>()->IsChildActualHeightUsedAsExtent() ||
                    m_trElementScrollContentPresenter.Cast<ScrollContentPresenter>()->IsChildActualHeightUpdated())
                {
#ifdef DM_DEBUG
                    if (DMSVv_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE)) == S_OK)
                    {
                        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE | DMSVv_DBG) /*traceType*/,
                            L"DMSVv[0x%p]: InvalidateScrollInfoImpl resetting m_contentHeightRequested=%f to -1.", this, m_contentHeightRequested));
                    }
#endif // DM_DEBUG
                    m_contentHeightRequested = -1;
                }
            }
        }

        if (translationXChanged || translationYChanged || zoomFactorChanged)
        {
            IFC(OnPrimaryContentTransformChanged(translationXChanged, translationYChanged, zoomFactorChanged));
        }

        // Fire internal ScrollChanged event, currently used by TextBox to update
        // TextBoxView's cached offsets.
        // Jolt bug 20632: Make ScrollChanged event public.
        // TODO: Fire ScrollChanged once TextBox is listening
        // if (ScrollChanged != null)
        // {
        //     ScrollChanged(HorizontalOffset, VerticalOffset);
        // }

        // Update the input pane transition status.
        UpdateInputPaneTransition();

        // When offset changes, we show the indicators.  For example, if you hit enter at the end of a RichEditBox and the
        // ScrollViewer scrolls as a new line enters the viewport, we need to show the indicators.
        //
        // As for the indicator type, do not change it if we are already showing indicators.  If indicators are not already showing,
        // use mouse indicators if the mouse is over the ScrollViewer, and use panning indicators otherwise.
        //
        // We want to avoid showing indicators when the IScrollInfo is modified during initialization,
        // e.g. in the case where the ComboBox is first opening.
        if ((updateOffsetX || updateOffsetY) &&
            m_isLoaded &&
            (!IsRootScrollViewer() || IsInputPaneShow()))
        {
            IFC(ShowIndicators());
        }
    }

Cleanup:
    if (SUCCEEDED(hr)) { hr = tryUpdateResult; }

    // Raise any potentially delayed ViewChanged notification
#ifdef DM_DEBUG
    if (DMSVv_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE)) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE | DMSVv_DBG) /*traceType*/,
            L"DMSVv[0x%p]: InvalidateScrollInfoImpl calling FlushViewChanged.", this));
    }
#endif // DM_DEBUG

    RRETURN(FlushViewChanged(hr));
}

// Member of the IScrollOwner internal contract. Allows the interface
// consumer to notify this ScrollViewer of an attempt to change the
// horizontal offset. Used for the ViewChanging notifications.
_Check_return_ HRESULT ScrollViewer::NotifyHorizontalOffsetChanging(
    _In_ DOUBLE targetHorizontalOffset,
    _In_ DOUBLE targetVerticalOffset)
{
#ifdef DM_DEBUG
    if (DMSVv_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE)) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE | DMSVv_DBG) /*traceType*/,
            L"DMSVv[0x%p]: NotifyHorizontalOffsetChanging - TargetHorizontalOffset=%f, TargetVerticalOffset=%f.", this, targetHorizontalOffset, targetVerticalOffset));
    }
#endif // DM_DEBUG

    RRETURN(NotifyOffsetChanging(targetHorizontalOffset, targetVerticalOffset));
}

// Member of the IScrollOwner internal contract. Allows the interface
// consumer to notify this ScrollViewer of an attempt to change the
// vertical offset. Used for the ViewChanging notifications.
_Check_return_ HRESULT ScrollViewer::NotifyVerticalOffsetChanging(
    _In_ DOUBLE targetHorizontalOffset,
    _In_ DOUBLE targetVerticalOffset)
{
#ifdef DM_DEBUG
    if (DMSVv_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE)) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE | DMSVv_DBG) /*traceType*/,
            L"DMSVv[0x%p]: NotifyVerticalOffsetChanging - TargetHorizontalOffset=%f, TargetVerticalOffset=%f.", this, targetHorizontalOffset, targetVerticalOffset));
    }
#endif // DM_DEBUG

    RRETURN(NotifyOffsetChanging(targetHorizontalOffset, targetVerticalOffset));
}

// Common method called by NotifyHorizontalOffsetChanging and NotifyVerticalOffsetChanging
// in order to invoke RaiseViewChanging with the correct zoom factor.
_Check_return_ HRESULT ScrollViewer::NotifyOffsetChanging(
    _In_ DOUBLE targetHorizontalOffset,
    _In_ DOUBLE targetVerticalOffset)
{
    HRESULT hr = S_OK;
    FLOAT targetZoomFactor = 1.0f;

    // Use the targeted zoom factor if it's changing, otherwise the current value.
    if (m_isTargetZoomFactorValid)
    {
        targetZoomFactor = m_targetZoomFactor;
    }
    else
    {
        IFC(get_ZoomFactor(&targetZoomFactor));
    }

    IFC(RaiseViewChanging(targetHorizontalOffset, targetVerticalOffset, targetZoomFactor));

Cleanup:
    RRETURN(hr);
}

// Called internally when a zoom factor change is processed in order to invoke RaiseViewChanging
// with the correct offsets.
_Check_return_ HRESULT ScrollViewer::NotifyZoomFactorChanging(
    _In_ FLOAT targetZoomFactor)
{
    HRESULT hr = S_OK;
    DOUBLE targetHorizontalOffset = 0.0;
    DOUBLE targetVerticalOffset = 0.0;

#ifdef DM_DEBUG
    if (DMSVv_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE)) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE | DMSVv_DBG) /*traceType*/,
            L"DMSVv[0x%p]: NotifyZoomFactorChanging - TargetZoomFactor=%f.", this, targetZoomFactor));
    }
#endif // DM_DEBUG

    if (m_isTargetHorizontalOffsetValid)
    {
        targetHorizontalOffset = m_targetHorizontalOffset;
    }
    else
    {
        IFC(get_HorizontalOffset(&targetHorizontalOffset));
    }

    if (m_isTargetVerticalOffsetValid)
    {
        targetVerticalOffset = m_targetVerticalOffset;
    }
    else
    {
        IFC(get_VerticalOffset(&targetVerticalOffset));
    }

    IFC(RaiseViewChanging(targetHorizontalOffset, targetVerticalOffset, targetZoomFactor));

Cleanup:
    RRETURN(hr);
}

// Gets a value indicating whether the current ScrollContentPresenter is the IScrollInfo implementer.
_Check_return_ HRESULT ScrollViewer::IsScrollContentPresenterScrollClient(
    _Out_ BOOLEAN& isScrollContentPresenterScrollClient)
{
    HRESULT hr = S_OK;
    BOOLEAN isScrollClient = FALSE;

    isScrollContentPresenterScrollClient = FALSE;

    if (m_trElementScrollContentPresenter)
    {
        IFC(m_trElementScrollContentPresenter.Cast<ScrollContentPresenter>()->IsScrollClient(&isScrollClient));
        isScrollContentPresenterScrollClient = isScrollClient;
    }

Cleanup:
    RRETURN(hr);
}

// Helper that updates the values of properties during InvalidateScrollInfo.
_Check_return_ HRESULT ScrollViewer::InvalidateScrollInfo_TryUpdateValues(
    _In_ IScrollInfo* pScrollInfo,
    _Out_ BOOLEAN& changed,
    _Out_ BOOLEAN& updateOffsetX,
    _Out_ BOOLEAN& updateOffsetY,
    _Out_ BOOLEAN& updatePixelViewportX,
    _Out_ BOOLEAN& updatePixelViewportY,
    _Out_ BOOLEAN& updatePixelExtentX,
    _Out_ BOOLEAN& updatePixelExtentY) noexcept
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IManipulationDataProvider> spProviderForHorizontalOrientation;
    ctl::ComPtr<IManipulationDataProvider> spProviderForVerticalOrientation;
    ctl::ComPtr<xaml_automation_peers::IAutomationPeer> spAutomationPeer;
    BOOLEAN bAutomationListener = FALSE;

    DOUBLE xOffset = 0.0;
    DOUBLE yOffset = 0.0;
    DOUBLE xMinOffset = 0.0;
    DOUBLE yMinOffset = 0.0;
    DOUBLE xPixelOffset = 0.0;
    DOUBLE yPixelOffset = 0.0;
    DOUBLE xViewport = 0.0;
    DOUBLE yViewport = 0.0;
    DOUBLE xPixelViewport = 0.0;
    DOUBLE yPixelViewport = 0.0;
    DOUBLE xExtent = 0.0;
    DOUBLE yExtent = 0.0;
    DOUBLE xPixelExtent = 0.0;
    DOUBLE yPixelExtent = 0.0;
    DOUBLE oldActualHorizontalOffset = 0.0;
    DOUBLE oldActualVerticalOffset = 0.0;
    DOUBLE oldMinHorizontalOffset = 0.0;
    DOUBLE oldMinVerticalOffset = 0.0;
    DOUBLE oldViewportWidth = 0.0;
    DOUBLE oldViewportHeight = 0.0;
    DOUBLE oldExtentWidth = 0.0;
    DOUBLE oldExtentHeight = 0.0;
    DOUBLE scrollableWidth = 0.0;
    DOUBLE scrollableHeight = 0.0;
    DOUBLE oldScrollableWidth = 0.0;
    DOUBLE oldScrollableHeight = 0.0;

    changed = FALSE;
    updateOffsetX = FALSE;
    updateOffsetY = FALSE;
    updatePixelViewportX = FALSE;
    updatePixelViewportY = FALSE;
    updatePixelExtentX = FALSE;
    updatePixelExtentY = FALSE;

    // Scroll property values - local cache of what was computed by ISI
    IFC(pScrollInfo->get_HorizontalOffset(&xOffset));
    IFC(pScrollInfo->get_VerticalOffset(&yOffset));
    IFC(pScrollInfo->get_MinHorizontalOffset(&xMinOffset));
    IFC(pScrollInfo->get_MinVerticalOffset(&yMinOffset));
    IFC(pScrollInfo->get_ViewportWidth(&xViewport));
    IFC(pScrollInfo->get_ViewportHeight(&yViewport));
    IFC(pScrollInfo->get_ExtentWidth(&xExtent));
    IFC(pScrollInfo->get_ExtentHeight(&yExtent));

    ASSERT(xOffset >= xMinOffset);
    ASSERT(yOffset >= yMinOffset);

    IFC(GetInnerManipulationDataProvider(TRUE /*isForHorizontalOrientation*/, &spProviderForHorizontalOrientation));
    if (!spProviderForHorizontalOrientation)
    {
        IFC(GetInnerManipulationDataProvider(FALSE /*isForHorizontalOrientation*/, &spProviderForVerticalOrientation));
    }

    if (spProviderForHorizontalOrientation)
    {
        IFC(ComputePixelViewportWidth(spProviderForHorizontalOrientation.Get(), TRUE /*isProviderSet*/, &xPixelViewport));
        IFC(ComputePixelExtentWidth(false /*ignoreZoomFactor*/, spProviderForHorizontalOrientation.Get(), &xPixelExtent));
        IFC(spProviderForHorizontalOrientation->ComputePixelOffset(TRUE /*isForHorizontalOrientation*/, xPixelOffset));
    }
    else
    {
        xPixelExtent = xExtent;
        xPixelOffset = xOffset;
        xPixelViewport = xViewport;

        if (xPixelViewport == DoubleUtil::PositiveInfinity && m_trElementScrollContentPresenter)
        {
            IFC(m_trElementScrollContentPresenter.Cast<ScrollContentPresenter>()->get_ActualWidth(&xPixelViewport));
        }
    }

    if (spProviderForVerticalOrientation)
    {
        IFC(ComputePixelViewportHeight(spProviderForVerticalOrientation.Get(), TRUE /*isProviderSet*/, &yPixelViewport));
        IFC(ComputePixelExtentHeight(false /*ignoreZoomFactor*/, spProviderForVerticalOrientation.Get(), &yPixelExtent));
        IFC(spProviderForVerticalOrientation->ComputePixelOffset(FALSE /*isForHorizontalOrientation*/, yPixelOffset));
    }
    else
    {
        yPixelExtent = yExtent;
        yPixelOffset = yOffset;
        yPixelViewport = yViewport;

        if (yPixelViewport == DoubleUtil::PositiveInfinity && m_trElementScrollContentPresenter)
        {
            IFC(m_trElementScrollContentPresenter.Cast<ScrollContentPresenter>()->get_ActualHeight(&yPixelViewport));
        }
    }

    if (!DoubleUtil::AreClose(m_xOffset, xOffset) ||
        !DoubleUtil::AreClose(m_yOffset, yOffset) ||
        !DoubleUtil::AreClose(m_xMinOffset, xMinOffset) ||
        !DoubleUtil::AreClose(m_yMinOffset, yMinOffset) ||
        !DoubleUtil::AreClose(m_xPixelOffset, xPixelOffset) ||
        !DoubleUtil::AreClose(m_yPixelOffset, yPixelOffset) ||
        !DoubleUtil::AreClose(m_xViewport, xViewport) ||
        !DoubleUtil::AreClose(m_yViewport, yViewport) ||
        !DoubleUtil::AreClose(m_xPixelViewport, xPixelViewport) ||
        !DoubleUtil::AreClose(m_yPixelViewport, yPixelViewport) ||
        !DoubleUtil::AreClose(m_xExtent, xExtent) ||
        !DoubleUtil::AreClose(m_yExtent, yExtent) ||
        !DoubleUtil::AreClose(m_xPixelExtent, xPixelExtent) ||
        !DoubleUtil::AreClose(m_yPixelExtent, yPixelExtent))
    {
        oldActualHorizontalOffset = m_xOffset;
        oldActualVerticalOffset = m_yOffset;
        oldMinHorizontalOffset = m_xMinOffset;
        oldMinVerticalOffset = m_yMinOffset;
        oldViewportWidth = m_xViewport;
        oldViewportHeight = m_yViewport;
        oldExtentWidth = m_xExtent;
        oldExtentHeight = m_yExtent;

        IFC(get_ScrollableWidth(&oldScrollableWidth));
        IFC(get_ScrollableHeight(&oldScrollableHeight));

        if (!DoubleUtil::AreClose(oldMinHorizontalOffset, xMinOffset))
        {
            IFC(put_MinHorizontalOffset(xMinOffset));
            // reevaluate horizontal offset
            IFC(pScrollInfo->get_HorizontalOffset(&xOffset));
            changed = TRUE;
        }

        if (!DoubleUtil::AreClose(oldMinVerticalOffset, yMinOffset))
        {
            IFC(put_MinVerticalOffset(yMinOffset));
            //reevaluate vertical offset
            IFC(pScrollInfo->get_VerticalOffset(&yOffset));
            changed = TRUE;
        }

        // Go through scrolling properties updating values.
        if (!DoubleUtil::AreClose(oldActualHorizontalOffset, xOffset) ||
            !DoubleUtil::AreClose(m_xPixelOffset, xPixelOffset))
        {
#ifdef DM_DEBUG
            if (DMSVv_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE)) == S_OK)
            {
                IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE | DMSVv_DBG) /*traceType*/,
                    L"DMSVv[0x%p]: InvalidateScrollInfo_TryUpdateValues horizontal offset changed: m_xPixelOffset=%f xPixelOffset=%f.",
                    this, m_xPixelOffset, xPixelOffset));
            }
#endif // DM_DEBUG

            m_xPixelOffset = xPixelOffset;
            IFC(put_HorizontalOffset(xOffset));
            updateOffsetX = TRUE;
            changed = TRUE;
        }

        if (!DoubleUtil::AreClose(oldActualVerticalOffset, yOffset) ||
            !DoubleUtil::AreClose(m_yPixelOffset, yPixelOffset))
        {
#ifdef DM_DEBUG
            if (DMSVv_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE)) == S_OK)
            {
                IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE | DMSVv_DBG) /*traceType*/,
                    L"DMSVv[0x%p]: InvalidateScrollInfo_TryUpdateValues vertical offset changed: m_yPixelOffset=%f yPixelOffset=%f.",
                    this, m_yPixelOffset, yPixelOffset));
            }
#endif // DM_DEBUG

            m_yPixelOffset = yPixelOffset;
            IFC(put_VerticalOffset(yOffset));
            updateOffsetY = TRUE;
            changed = TRUE;
        }

        if (!DoubleUtil::AreClose(oldViewportWidth, xViewport))
        {
            IFC(put_ViewportWidth(xViewport));
            // Refresh the xPixelViewport value since the put_ViewportWidth call might have impacted its valuation.
            IFC(ComputePixelViewportWidth(spProviderForHorizontalOrientation.Get(), TRUE /*isProviderSet*/, &xPixelViewport));
            changed = TRUE;
        }

        if (!DoubleUtil::AreClose(m_xPixelViewport, xPixelViewport))
        {
            m_xPixelViewport = xPixelViewport;

            updatePixelViewportX = TRUE;
            changed = TRUE;
        }

        if (!DoubleUtil::AreClose(oldViewportHeight, yViewport))
        {
            IFC(put_ViewportHeight(yViewport));
            // Refresh the yPixelViewport value since the put_ViewportHeight call might have impacted its valuation.
            IFC(ComputePixelViewportHeight(spProviderForVerticalOrientation.Get(), TRUE /*isProviderSet*/, &yPixelViewport));
            changed = TRUE;
        }

        if (!DoubleUtil::AreClose(m_yPixelViewport, yPixelViewport))
        {
            m_yPixelViewport = yPixelViewport;

            updatePixelViewportY = TRUE;
            changed = TRUE;
        }

        if (!DoubleUtil::AreClose(oldExtentWidth, xExtent))
        {
            IFC(put_ExtentWidth(xExtent));
            changed = TRUE;
        }

        if (!DoubleUtil::AreClose(m_xPixelExtent, xPixelExtent))
        {
#ifdef DM_DEBUG
            if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
            {
                IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/,
                    L"DMSV[0x%p]:  InvalidateScrollInfo_TryUpdateValues horizontal extent changed: m_xPixelExtent=%f xPixelExtent=%f.",
                    this, m_xPixelExtent, xPixelExtent));
            }
#endif // DM_DEBUG

            m_xPixelExtent = xPixelExtent;
            updatePixelExtentX = TRUE;
            changed = TRUE;
        }

        if (!DoubleUtil::AreClose(oldExtentHeight, yExtent))
        {
            IFC(put_ExtentHeight(yExtent));
            changed = TRUE;
        }

        if (!DoubleUtil::AreClose(m_yPixelExtent, yPixelExtent))
        {
#ifdef DM_DEBUG
            if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
            {
                IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/,
                    L"DMSV[0x%p]:  InvalidateScrollInfo_TryUpdateValues vertical extent changed: m_yPixelExtent=%f yPixelExtent=%f.",
                    this, m_yPixelExtent, yPixelExtent));
            }
#endif // DM_DEBUG

            m_yPixelExtent = yPixelExtent;
            updatePixelExtentY = TRUE;
            changed = TRUE;
        }

        // ScrollableWidth/Height are dependant on Viewport and Extent set
        // above.  This check must be done after those.
        IFC(get_ScrollableWidth(&scrollableWidth));
        if (!DoubleUtil::AreClose(oldScrollableWidth, scrollableWidth))
        {
            IFC(put_ScrollableWidth(scrollableWidth));
            changed = TRUE;
        }

        IFC(get_ScrollableHeight(&scrollableHeight));
        if (!DoubleUtil::AreClose(oldScrollableHeight, scrollableHeight))
        {
            IFC(put_ScrollableHeight(scrollableHeight));
            changed = TRUE;
        }

        if (changed)
        {
            IFC(AutomationPeer::ListenerExistsHelper(xaml_automation_peers::AutomationEvents_PropertyChanged, &bAutomationListener));
            if (bAutomationListener)
            {
                IFC(GetOrCreateAutomationPeer(&spAutomationPeer));
                if (spAutomationPeer)
                {
                    IFC((spAutomationPeer.Cast<ScrollViewerAutomationPeer>())->RaiseAutomationEvents(oldExtentWidth,
                        oldExtentHeight,
                        oldViewportWidth,
                        oldViewportHeight,
                        oldMinHorizontalOffset,
                        oldMinVerticalOffset,
                        oldActualHorizontalOffset,
                        oldActualVerticalOffset));
                }
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

// Called when the HorizontalScrollBarVisibility or
// VerticalScrollBarVisibility property has changed.
_Check_return_ HRESULT ScrollViewer::OnScrollBarVisibilityChanged()
{
    ctl::ComPtr<IScrollInfo> scrollInfo;

    IFC_RETURN(get_ScrollInfo(&scrollInfo))
    if (scrollInfo)
    {
        IFC_RETURN(UpdateCanScroll(scrollInfo.Get()));
    }

    // ScrollBar visibilities affect the DirectManipulation viewport configurations.
    IFC_RETURN(OnViewportConfigurationsAffectingPropertyChanged());

    IFC_RETURN(InvalidateMeasure());

    return S_OK;
}

// Updates the provided IScrollInfo's CanHorizontallyScroll & CanVerticallyScroll characteristics based
// on the scrollbars visibility, and resets the offset(s) when scrolling in not enabled.
_Check_return_ HRESULT ScrollViewer::UpdateCanScroll(
    _In_ IScrollInfo* pScrollInfo)
{
    HRESULT hr = S_OK;
    xaml_controls::ScrollBarVisibility horizontal = xaml_controls::ScrollBarVisibility_Disabled;
    xaml_controls::ScrollBarVisibility vertical = xaml_controls::ScrollBarVisibility_Disabled;

    IFCPTR(pScrollInfo);

    IFC(get_HorizontalScrollBarVisibility(&horizontal));
    IFC(get_VerticalScrollBarVisibility(&vertical));

    if (horizontal == xaml_controls::ScrollBarVisibility_Disabled)
    {
        // When the horizontal scrollbar becomes disabled, the horizontal offset needs to be reset to 0.
        IFC(pScrollInfo->SetHorizontalOffset(0.0));
    }
    IFC(pScrollInfo->put_CanHorizontallyScroll(horizontal != xaml_controls::ScrollBarVisibility_Disabled));

    if (vertical == xaml_controls::ScrollBarVisibility_Disabled)
    {
        // When the vertical scrollbar becomes disabled, the vertical offset needs to be reset to 0.
        IFC(pScrollInfo->SetVerticalOffset(0.0));
    }
    IFC(pScrollInfo->put_CanVerticallyScroll(vertical != xaml_controls::ScrollBarVisibility_Disabled));

Cleanup:
    RRETURN(hr);
}

// Handle the horizontal ScrollBar's Scroll event.
_Check_return_ HRESULT ScrollViewer::OnHorizontalScrollBarScroll(
    _In_ IInspectable* pSender,
    _In_ xaml_primitives::IScrollEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    DOUBLE newOffset = 0.0;
    xaml_primitives::ScrollEventType scrollEventType;
    xaml_controls::ScrollMode scrollMode;

    // Do not process this request when the effective HorizontalScrollMode is Disabled.
    IFC(GetEffectiveHorizontalScrollMode(TRUE /*canUseCachedProperty*/, scrollMode));
    if (scrollMode == xaml_controls::ScrollMode_Disabled)
    {
        goto Cleanup;
    }

    IFC(pArgs->get_ScrollEventType(&scrollEventType));
    IFC(pArgs->get_NewValue(&newOffset));

#ifdef DM_DEBUG
    if (DMSVv_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE)) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE | DMSVv_DBG) /*traceType*/,
            L"DMSVv[0x%p]: OnHorizontalScrollBarScroll - entry. scrollEventType=%d, newOffset=%f.", this, scrollEventType, newOffset));
    }
#endif // DM_DEBUG

    IFC(HandleHorizontalScroll(scrollEventType, newOffset));

Cleanup:
    RRETURN(hr);
}

// Handle the vertical ScrollBar's Scroll event.
_Check_return_ HRESULT ScrollViewer::OnVerticalScrollBarScroll(
    _In_ IInspectable* pSender,
    _In_ xaml_primitives::IScrollEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    DOUBLE newOffset = 0.0;
    xaml_primitives::ScrollEventType scrollEventType;
    xaml_controls::ScrollMode scrollMode;

    // Do not process this request when the effective VerticalScrollMode is Disabled.
    IFC(GetEffectiveVerticalScrollMode(TRUE /*canUseCachedProperty*/, scrollMode));
    if (scrollMode == xaml_controls::ScrollMode_Disabled)
    {
        goto Cleanup;
    }

    IFC(pArgs->get_ScrollEventType(&scrollEventType));
    IFC(pArgs->get_NewValue(&newOffset));

#ifdef DM_DEBUG
    if (DMSVv_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE)) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE | DMSVv_DBG) /*traceType*/,
            L"DMSVv[0x%p]: OnVerticalScrollBarScroll - entry. scrollEventType=%d, newOffset=%f.", this, scrollEventType, newOffset));
    }
#endif // DM_DEBUG

    IFC(HandleVerticalScroll(scrollEventType, newOffset));

Cleanup:
    RRETURN(hr);
}

// Handle DragStarted on the horizontal ScrollBar's Thumb.
_Check_return_ HRESULT
ScrollViewer::OnScrollBarThumbDragStarted(
_In_ IInspectable* pSender,
_In_ xaml_primitives::IDragStartedEventArgs* pArgs)
{
    HRESULT hr = S_OK;

    // Suppress the scrollbars from fading out while we are dragging.
    m_keepIndicatorsShowing = TRUE;

    // Suppress scrolling when dragging a thumb
    m_isDraggingThumb = TRUE;

    IFC(ShowIndicators());

Cleanup:
    RRETURN(hr);
}

// Handle DragCompleted on the horizontal ScrollBar's Thumb.
_Check_return_ HRESULT
ScrollViewer::OnScrollBarThumbDragCompleted(
_In_ IInspectable* pSender,
_In_ xaml_primitives::IDragCompletedEventArgs* pArgs)
{
    HRESULT hr = S_OK;

    // Make the scrollbars fade out, after the normal delay.
    m_keepIndicatorsShowing = FALSE;

    // Suppress scrolling when dragging a thumb
    m_isDraggingThumb = FALSE;

    // and synchronize
    IFC(SynchronizeScrollOffsetsAfterThumbDeferring());

    IFC(UpdateVisualState());

Cleanup:
    RRETURN(hr);
}

// Handle PointerEntered on the vertical scrollbar
_Check_return_ HRESULT
ScrollViewer::OnVerticalScrollbarPointerEntered(
    _In_ IInspectable* /*pSender unused*/,
    _In_ xaml_input::IPointerRoutedEventArgs* /* pArgs */)
{
    m_isPointerOverVerticalScrollbar = TRUE;

    return S_OK;
}

// Handle PointerExited on the vertical scrollbar
_Check_return_ HRESULT
ScrollViewer::OnVerticalScrollbarPointerExited(
    _In_ IInspectable* /*pSender unused*/,
    _In_ xaml_input::IPointerRoutedEventArgs* /*pArgs unused*/)
{
    m_isPointerOverVerticalScrollbar = FALSE;

    return S_OK;
}

// Handle PointerEntered on the horizontal scrollbar
_Check_return_ HRESULT
ScrollViewer::OnHorizontalScrollbarPointerEntered(
    _In_ IInspectable* /*pSender unused*/,
    _In_ xaml_input::IPointerRoutedEventArgs* /*pArgs unused*/)
{
    m_isPointerOverHorizontalScrollbar = TRUE;

    return S_OK;
}

// Handle PointerExited on the horizontal scrollbar
_Check_return_ HRESULT
ScrollViewer::OnHorizontalScrollbarPointerExited(
    _In_ IInspectable* /*pSender unused*/,
    _In_ xaml_input::IPointerRoutedEventArgs* /*pArgs unused*/)
{
    m_isPointerOverHorizontalScrollbar = FALSE;

    return S_OK;
}

// Get the visual child of the ScrollViewer.
_Check_return_ HRESULT ScrollViewer::GetVisualChild(
    _Outptr_ xaml::IUIElement** ppChild)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IUIElement> spChild;
    ctl::ComPtr<IDependencyObject> spChildAsDO;
    INT childCount = 0;

    IFCPTR(ppChild);
    IFC(VisualTreeHelper::GetChildrenCountStatic(this, &childCount));
    if (childCount)
    {
        IFC(VisualTreeHelper::GetChildStatic(this, 0, &spChildAsDO));
        spChild = spChildAsDO.AsOrNull<IUIElement>();
    }
    IFC(spChild.MoveTo(ppChild));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ScrollViewer::get_ElementHorizontalScrollBar(
    _Outptr_ xaml::IUIElement** ppElement)
{
    HRESULT hr = S_OK;
    IFCPTR(ppElement);

    IFC(m_trElementHorizontalScrollBar.CopyTo(ppElement));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ScrollViewer::get_ElementVerticalScrollBar(
    _Outptr_ xaml::IUIElement** ppElement)
{
    HRESULT hr = S_OK;
    IFCPTR(ppElement);

    IFC(m_trElementVerticalScrollBar.CopyTo(ppElement));

Cleanup:
    RRETURN(hr);
}

// Gets the horizontal extent in pixels even for logical scrolling scenarios.
_Check_return_ HRESULT ScrollViewer::ComputePixelExtentWidth(
    _In_ bool ignoreZoomFactor,
    _In_opt_ IManipulationDataProvider* pProvider,
    _Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;

    IFCPTR(pValue);

    if (pProvider)
    {
        IFC(pProvider->ComputePixelExtent(ignoreZoomFactor, *pValue));
    }
    else
    {
        IFC(get_ExtentWidth(pValue));
        if (ignoreZoomFactor && m_trElementScrollContentPresenter)
        {
            *pValue /= m_trElementScrollContentPresenter.Cast<ScrollContentPresenter>()->GetLastZoomFactorApplied();
        }
    }

Cleanup:
    RRETURN(hr);
}

// Gets the horizontal extent in pixels even for logical scrolling scenarios.
// Overload that attempts to determine the potential inner horizontal IManipulationDataProvider.
_Check_return_ HRESULT ScrollViewer::ComputePixelExtentWidth(
    _Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IManipulationDataProvider> spProvider;

    IFCPTR(pValue);

    IFC(GetInnerManipulationDataProvider(TRUE /*isForHorizontalOrientation*/, &spProvider));

    IFC(ComputePixelExtentWidth(false /*ignoreZoomFactor*/, spProvider.Get(), pValue));

Cleanup:
    RRETURN(hr);
}

// Gets the vertical extent in pixels even for logical scrolling scenarios.
_Check_return_ HRESULT ScrollViewer::ComputePixelExtentHeight(
    _In_ bool ignoreZoomFactor,
    _In_opt_ IManipulationDataProvider* pProvider,
    _Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;

    IFCPTR(pValue);

    if (pProvider)
    {
        IFC(pProvider->ComputePixelExtent(ignoreZoomFactor, *pValue));
    }
    else
    {
        IFC(get_ExtentHeight(pValue));
        if (ignoreZoomFactor && m_trElementScrollContentPresenter)
        {
            *pValue /= m_trElementScrollContentPresenter.Cast<ScrollContentPresenter>()->GetLastZoomFactorApplied();
        }
    }

Cleanup:
    RRETURN(hr);
}

// Gets the vertical extent in pixels even for logical scrolling scenarios.
// Overload that attempts to determine the potential inner vertical IManipulationDataProvider.
_Check_return_ HRESULT ScrollViewer::ComputePixelExtentHeight(
    _Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IManipulationDataProvider> spProvider;

    IFCPTR(pValue);

    IFC(GetInnerManipulationDataProvider(FALSE /*isForHorizontalOrientation*/, &spProvider));

    IFC(ComputePixelExtentHeight(false /*ignoreZoomFactor*/, spProvider.Get(), pValue));

Cleanup:
    RRETURN(hr);
}

// Gets the viewport width in pixels even for logical scrolling scenarios.
// When isProviderSet is True, the provided pProvider param is valid even when NULL.
_Check_return_ HRESULT ScrollViewer::ComputePixelViewportWidth(
    _In_opt_ IManipulationDataProvider* pProvider,
    _In_ BOOLEAN isProviderSet,
    _Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IManipulationDataProvider> spProviderLocal = pProvider;
    DOUBLE viewportWidth = 0.0;

    IFCPTR(pValue);
    *pValue = 0.0;

    if (!spProviderLocal && !isProviderSet)
    {
        IFC(GetInnerManipulationDataProvider(TRUE /*isForHorizontalOrientation*/, &spProviderLocal));
    }

    if (spProviderLocal)
    {
        ctl::ComPtr<IFrameworkElement> spFE;

        IFC(spProviderLocal.As(&spFE));
        IFC(spFE->get_ActualWidth(&viewportWidth));
    }
    else
    {
        IFC(get_ViewportWidth(&viewportWidth));
        if (viewportWidth == DoubleUtil::PositiveInfinity && m_trElementScrollContentPresenter)
        {
            // Since IScrollInfo reports an infinite horizontal viewport, fallback to using the
            // m_trElementScrollContentPresenter's width
            IFC(m_trElementScrollContentPresenter.Cast<ScrollContentPresenter>()->get_ActualWidth(&viewportWidth));
        }
    }

    *pValue = viewportWidth;

Cleanup:
    RRETURN(hr);
}

// Gets the viewport height in pixels even for logical scrolling scenarios.
// When isProviderSet is True, the provided pProvider param is valid even when NULL.
_Check_return_ HRESULT ScrollViewer::ComputePixelViewportHeight(
    _In_opt_ IManipulationDataProvider* pProvider,
    _In_ BOOLEAN isProviderSet,
    _Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;
    DOUBLE viewportHeight = 0.0;
    ctl::ComPtr<IManipulationDataProvider> spProviderLocal = pProvider;

    IFCPTR(pValue);
    *pValue = 0.0;

    if (!spProviderLocal && !isProviderSet)
    {
        IFC(GetInnerManipulationDataProvider(FALSE /*isForHorizontalOrientation*/, &spProviderLocal));
    }

    if (spProviderLocal)
    {
        ctl::ComPtr<IFrameworkElement> spFE;

        IFC(spProviderLocal.As(&spFE));
        IFC(spFE->get_ActualHeight(&viewportHeight));
    }
    else
    {
        IFC(get_ViewportHeight(&viewportHeight));
        if (viewportHeight == DoubleUtil::PositiveInfinity && m_trElementScrollContentPresenter)
        {
            // Since IScrollInfo reports an infinite vertical viewport, fallback to using the
            // m_trElementScrollContentPresenter's height
            IFC(m_trElementScrollContentPresenter.Cast<ScrollContentPresenter>()->get_ActualHeight(&viewportHeight));
        }
    }

    *pValue = viewportHeight;

Cleanup:
    RRETURN(hr);
}

// Gets the value of the scrollable width of the content in pixels even for logical scrolling scenarios.
_Check_return_ HRESULT ScrollViewer::ComputePixelScrollableWidth(
    _In_ IManipulationDataProvider* pProvider,
    _Out_ DOUBLE& pixelScrollableWidth)
{
    HRESULT hr = S_OK;
    DOUBLE extent = 0.0;
    DOUBLE viewport = 0.0;

    IFCPTR(pProvider);

    pixelScrollableWidth = 0.0;

    IFC(ComputePixelExtentWidth(false /*ignoreZoomFactor*/, pProvider, &extent));
    IFC(ComputePixelViewportWidth(pProvider, TRUE /*isProviderSet*/, &viewport));

    pixelScrollableWidth = DoubleUtil::Max(0.0, extent - viewport);

Cleanup:
    RRETURN(hr);
}

// Gets the value of the scrollable height of the content in pixels even for logical scrolling scenarios.
_Check_return_ HRESULT ScrollViewer::ComputePixelScrollableHeight(
    _In_ IManipulationDataProvider* pProvider,
    _Out_ DOUBLE& pixelScrollableHeight)
{
    HRESULT hr = S_OK;
    DOUBLE extent = 0.0;
    DOUBLE viewport = 0.0;

    IFCPTR(pProvider);

    pixelScrollableHeight = 0.0;

    IFC(ComputePixelExtentHeight(false /*ignoreZoomFactor*/, pProvider, &extent));
    IFC(ComputePixelViewportHeight(pProvider, TRUE /*isProviderSet*/, &viewport));

    pixelScrollableHeight = DoubleUtil::Max(0.0, extent - viewport);

Cleanup:
    RRETURN(hr);
}

// Returns a rounded down dimension for the viewport since DManip only accepts integer viewport values.
// A rounded down value is used to avoid bugs with unreachable mandatory scroll snap points.
DOUBLE ScrollViewer::AdjustPixelViewportDim(
    _In_ DOUBLE pixelViewportDim)
{
    // Round to the closest lower integer.
    // +3.000 --> +3.0
    // +8.731 --> +8.0
    // +5.999 --> +5.0
    ASSERT(pixelViewportDim >= 0.0f);
    return static_cast<DOUBLE>(static_cast<INT64>(pixelViewportDim));
}

// Returns a rounded up dimension for the content since DManip only accepts integer content values.
// A rounded up value is used to avoid bugs with unreachable mandatory scroll snap points.
DOUBLE ScrollViewer::AdjustPixelContentDim(
    _In_ DOUBLE pixelContentDim)
{
    // Round to the closest upper integer.
    // +3.000 --> +3.0
    // +8.731 --> +9.0
    // +5.001 --> +6.0
    ASSERT(pixelContentDim >= 0.0f);

    INT64 roundedDim = static_cast<INT64>(pixelContentDim);
    if (pixelContentDim == static_cast<DOUBLE>(roundedDim))
    {
        return static_cast<DOUBLE>(roundedDim);
    }
    else
    {
        return static_cast<DOUBLE>(roundedDim + 1);
    }
}

// Computes the required horizontal alignment to provide to DirectManipulation
_Check_return_ HRESULT ScrollViewer::ComputeHorizontalAlignment(
    _In_ BOOLEAN canUseCachedProperties,
    _Out_ DMAlignment& horizontalAlignment)
{
    HRESULT hr = S_OK;
    BOOLEAN isScrollContentPresenterScrollClient = FALSE;
    BOOLEAN resetHorizontalStretchAlignmentTreatedAsNear = TRUE;
    DOUBLE scrollable = 0.0;
    DMAlignment alignment = DMAlignmentCenter;
    DMAlignment contentAlignment = DMAlignmentCenter;
    xaml_controls::ScrollBarVisibility hsbv;
    xaml_controls::ScrollMode scrollMode;
    xaml::HorizontalAlignment horizontalContentAlignment = xaml::HorizontalAlignment_Center;
    xaml::HorizontalAlignment horizontalContentFEAlignment = xaml::HorizontalAlignment_Center;
    ScrollContentPresenter* pScrollContentPresenter = NULL;

    horizontalAlignment = DMAlignmentNone;

    // First access the ScrollViewer's HorizontalContentAlignment
    IFC(get_HorizontalContentAlignment(&horizontalContentAlignment));
    switch (horizontalContentAlignment)
    {
    case xaml::HorizontalAlignment_Left:
        contentAlignment = DMAlignmentNear;
        break;
    case xaml::HorizontalAlignment_Center:
        contentAlignment = DMAlignmentCenter;
        break;
    case xaml::HorizontalAlignment_Right:
        contentAlignment = DMAlignmentFar;
        break;
    case xaml::HorizontalAlignment_Stretch:
        contentAlignment = DMAlignmentCenter;
        break;
    }

    // Determine whether the ScrollContentPresenter is the IScrollInfo implementer or not
    IFC(IsScrollContentPresenterScrollClient(isScrollContentPresenterScrollClient));

    if (isScrollContentPresenterScrollClient)
    {
        ctl::ComPtr<xaml::IUIElement> spContentUIElement;
        ctl::ComPtr<xaml::IFrameworkElement> spContentFE;

        // When the ScrollContentPresenter is the IScrollInfo implementer,
        // use the horizontal alignment of the manipulated element by default.
        IFC(GetContentUIElement(&spContentUIElement));
        if (spContentUIElement)
        {
            spContentFE = spContentUIElement.AsOrNull<IFrameworkElement>();
            if (spContentFE)
            {
                IFC(spContentFE->get_HorizontalAlignment(&horizontalContentFEAlignment));
                switch (horizontalContentFEAlignment)
                {
                case xaml::HorizontalAlignment_Left:
                    alignment = DMAlignmentNear;
                    break;
                case xaml::HorizontalAlignment_Right:
                    alignment = DMAlignmentFar;
                    break;
                case xaml::HorizontalAlignment_Stretch:
                    resetHorizontalStretchAlignmentTreatedAsNear = FALSE;
                    if (m_isHorizontalStretchAlignmentTreatedAsNear)
                    {
                        alignment = DMAlignmentNear;
                    }
                    break;
                }
            }
        }

        IFC(GetEffectiveHorizontalScrollMode(canUseCachedProperties, scrollMode));
        if (scrollMode == xaml_controls::ScrollMode_Disabled)
        {
            // When horizontal panning is turned off, use None when the content is
            // larger than the viewport in order to keep the zoom center between the fingers.
            IFC(get_ScrollableWidth(&scrollable));
            if (scrollable > 0)
            {
                alignment = DMAlignmentNone;
            }
        }

        IFC(GetEffectiveHorizontalScrollBarVisibility(FALSE /*canUseCachedProperty*/, hsbv));
        if (hsbv == xaml_controls::ScrollBarVisibility_Disabled)
        {
            // When horizontal scrollbar visibility is disabled, the content is cut off if it's
            // wider than the viewport, and the HorizontalOffset can only be 0.0f. If the zoom factor
            // is larger than 1.0f, and the content is horizontally scrollable, pick a Left alignment
            IFC(get_ScrollableWidth(&scrollable));
            if (scrollable > 0)
            {
                alignment = DMAlignmentNear;
            }
        }
    }
    else
    {
        alignment = contentAlignment;
    }

    horizontalAlignment = alignment;

    if (horizontalAlignment == DMAlignmentNear && m_trElementScrollContentPresenter)
    {
        // Check if there is a header child.
        pScrollContentPresenter = m_trElementScrollContentPresenter.Cast<ScrollContentPresenter>();
        if (pScrollContentPresenter->IsTopLeftHeaderChild() ||
            pScrollContentPresenter->IsTopHeaderChild() ||
            pScrollContentPresenter->IsLeftHeaderChild())
        {
            // When a header child is present, the DMAlignmentUnlockCenter flag is required to avoid
            // offsets being reset to 0 when inertia starts.
            horizontalAlignment = static_cast<DMAlignment>(alignment + DMAlignmentUnlockCenter);
        }
    }

    if (resetHorizontalStretchAlignmentTreatedAsNear)
    {
        m_isHorizontalStretchAlignmentTreatedAsNear = FALSE;
    }

Cleanup:
    RRETURN(hr);
}

// Computes the required vertical alignment to provide to DirectManipulation
_Check_return_ HRESULT ScrollViewer::ComputeVerticalAlignment(
    _In_ BOOLEAN canUseCachedProperties,
    _Out_ DMAlignment& verticalAlignment)
{
    HRESULT hr = S_OK;
    BOOLEAN isScrollContentPresenterScrollClient = FALSE;
    BOOLEAN resetVerticalStretchAlignmentTreatedAsNear = TRUE;
    DOUBLE scrollable = 0.0;
    DMAlignment alignment = DMAlignmentCenter;
    DMAlignment contentAlignment = DMAlignmentCenter;
    xaml_controls::ScrollBarVisibility vsbv;
    xaml_controls::ScrollMode scrollMode;
    xaml::VerticalAlignment verticalContentAlignment = xaml::VerticalAlignment_Center;
    xaml::VerticalAlignment verticalContentFEAlignment = xaml::VerticalAlignment_Center;
    ScrollContentPresenter* pScrollContentPresenter = NULL;

    // Work around for ScrollViewer / ScrollContentPresenter bug Windows Blue 358691
    // Only forced by Hub on phone
    if (m_isNearVerticalAlignmentForced)
    {
        verticalAlignment = DMAlignmentNear;
        goto Cleanup;
    }

    verticalAlignment = DMAlignmentNone;

    // First access the ScrollViewer's VerticalContentAlignment
    IFC(get_VerticalContentAlignment(&verticalContentAlignment));
    switch (verticalContentAlignment)
    {
    case xaml::VerticalAlignment_Top:
        contentAlignment = DMAlignmentNear;
        break;
    case xaml::VerticalAlignment_Center:
        contentAlignment = DMAlignmentCenter;
        break;
    case xaml::VerticalAlignment_Bottom:
        contentAlignment = DMAlignmentFar;
        break;
    case xaml::VerticalAlignment_Stretch:
        contentAlignment = DMAlignmentCenter;
        break;
    }

    // Determine whether the ScrollContentPresenter is the IScrollInfo implementer or not
    IFC(IsScrollContentPresenterScrollClient(isScrollContentPresenterScrollClient));

    if (isScrollContentPresenterScrollClient)
    {
        ctl::ComPtr<xaml::IUIElement> spContentUIElement;
        ctl::ComPtr<xaml::IFrameworkElement> spContentFE;

        // When the ScrollContentPresenter is the IScrollInfo implementer,
        // use the vertical alignment of the manipulated element by default.
        IFC(GetContentUIElement(&spContentUIElement));
        if (spContentUIElement)
        {
            spContentFE = spContentUIElement.AsOrNull<IFrameworkElement>();
            if (spContentFE)
            {
                IFC(spContentFE->get_VerticalAlignment(&verticalContentFEAlignment));
                switch (verticalContentFEAlignment)
                {
                case xaml::VerticalAlignment_Top:
                    alignment = DMAlignmentNear;
                    break;
                case xaml::VerticalAlignment_Bottom:
                    alignment = DMAlignmentFar;
                    break;
                case xaml::VerticalAlignment_Stretch:
                    resetVerticalStretchAlignmentTreatedAsNear = FALSE;
                    if (m_isVerticalStretchAlignmentTreatedAsNear)
                    {
                        alignment = DMAlignmentNear;
                    }
                    break;
                }
            }
        }

        IFC(GetEffectiveVerticalScrollMode(canUseCachedProperties, scrollMode));
        if (scrollMode == xaml_controls::ScrollMode_Disabled)
        {
            // When vertical panning is turned off, use None when the content is
            // larger than the viewport in order to keep the zoom center between the fingers.
            IFC(get_ScrollableHeight(&scrollable));
            if (scrollable > 0)
            {
                alignment = DMAlignmentNone;
            }
        }

        IFC(GetEffectiveVerticalScrollBarVisibility(FALSE /*canUseCachedProperty*/, vsbv));
        if (vsbv == xaml_controls::ScrollBarVisibility_Disabled)
        {
            // When vertical scrollbar visibility is disabled, the content is cut off if it's
            // taller than the viewport, and the VerticalOffset can only be 0.0f. If the zoom factor
            // is larger than 1.0f, and the content is vertically scrollable, pick a Top alignment
            IFC(get_ScrollableHeight(&scrollable));
            if (scrollable > 0)
            {
                alignment = DMAlignmentNear;
            }
        }
    }
    else
    {
        alignment = contentAlignment;
    }

    verticalAlignment = alignment;

    if (verticalAlignment == DMAlignmentNear && m_trElementScrollContentPresenter)
    {
        // Check if there is a header child.
        pScrollContentPresenter = m_trElementScrollContentPresenter.Cast<ScrollContentPresenter>();
        if (pScrollContentPresenter->IsTopLeftHeaderChild() ||
            pScrollContentPresenter->IsTopHeaderChild() ||
            pScrollContentPresenter->IsLeftHeaderChild())
        {
            // When a header child is present, the DMAlignmentUnlockCenter flag is required to avoid
            // offsets being reset to 0 when inertia starts.
            verticalAlignment = static_cast<DMAlignment>(alignment + DMAlignmentUnlockCenter);
        }
    }

    if (resetVerticalStretchAlignmentTreatedAsNear)
    {
        m_isVerticalStretchAlignmentTreatedAsNear = FALSE;
    }

Cleanup:
    RRETURN(hr);
}

// Scrolls by the number of provided pixels, even for logical scrolling scenarios.
_Check_return_ HRESULT ScrollViewer::ScrollByPixelDelta(
    _In_ BOOLEAN isForHorizontalOrientation,
    _In_ DOUBLE newPixelOffset,
    _In_ DOUBLE pixelDelta,
    _In_ BOOLEAN isDManipInput)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IManipulationDataProvider> spProvider;
    ctl::ComPtr<IScrollInfo> spScrollInfo;
    BOOLEAN isCarouselPanel = FALSE;
    DOUBLE viewport = 0.0;
    DOUBLE scrollable = 0.0;
    DOUBLE pixelScrollable = 0.0;
    DOUBLE oldOffset = 0.0;
    DOUBLE newOffset = 0.0;
    DOUBLE newPixelDelta = pixelDelta;

#ifdef DM_DEBUG
    if (DMSVv_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE)) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE | DMSVv_DBG) /*traceType*/,
            L"DMSVv[0x%p]: ScrollByPixelDelta isForHorizontalOrientation=%d newPixelOffset=%f pixelDelta=%f, isDManipInput=%d",
            this, isForHorizontalOrientation, newPixelOffset, pixelDelta, isDManipInput));
    }
#endif // DM_DEBUG

    IFC(get_ScrollInfo(&spScrollInfo));
    if (!spScrollInfo)
    {
        goto Cleanup;
    }

    IFC(GetInnerManipulationDataProvider(isForHorizontalOrientation, &spProvider));

    if (spProvider)
    {
        IFC(spProvider->ComputeLogicalOffset(isForHorizontalOrientation, newPixelDelta, newOffset));
#ifdef DM_DEBUG
        if (DMSVv_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE)) == S_OK)
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE | DMSVv_DBG) /*traceType*/,
                L"DMSVv[0x%p]: ScrollByPixelDelta ComputeLogicalOffset returns newOffset=%f, newPixelDelta=%f for pixelDelta=%f.",
                this, newOffset, newPixelDelta, pixelDelta));
        }
#endif // DM_DEBUG

    }
    else
    {
        newOffset = newPixelOffset;
    }

    if (isForHorizontalOrientation)
    {
        IFC(spScrollInfo->get_HorizontalOffset(&oldOffset));

        newOffset = DoubleUtil::Max(newOffset, 0.0);
        if (spProvider || m_contentWidthRequested == -1)
        {
            IFC(get_ScrollableWidth(&scrollable));
        }
        else
        {
            // The ScrollContentPresenter's clamping of the newOffset is based on the smallest of m_xExtent & m_contentWidthRequested
            IFC(get_ViewportWidth(&viewport));
            scrollable = DoubleUtil::Max(m_xMinOffset, DoubleUtil::Min(m_xExtent, m_contentWidthRequested) - viewport);
        }

        newOffset = DoubleUtil::Min(scrollable, newOffset);

        if (!DoubleUtil::AreClose(oldOffset, newOffset))
        {
            if (spProvider)
            {
                IFC(ComputePixelScrollableWidth(spProvider.Get(), pixelScrollable));
            }
            else
            {
                pixelScrollable = scrollable;
            }

            // Do not set an expected horizontal offset when this call is not triggered by DManip feedback.
            if (isDManipInput)
            {
                m_xPixelOffsetRequested = DoubleUtil::Min(DoubleUtil::Max(newPixelOffset, 0), pixelScrollable);
            }
#ifdef DM_DEBUG
            if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
            {
                IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/,
                    L"DMSV[0x%p]:  ScrollByPixelDelta oldOffset=%f newOffset=%f, new m_xPixelOffsetRequested=%f.",
                    this, oldOffset, newOffset, m_xPixelOffsetRequested));
            }
#endif // DM_DEBUG

            IFC(ScrollToHorizontalOffsetInternal(newOffset));
        }
        else
        {
            // Since the current offset is now close to the target, no offset change is expected
#ifdef DM_DEBUG
            if ((DMSVv_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE)) == S_OK) && m_xPixelOffsetRequested != -1)
            {
                IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE | DMSVv_DBG) /*traceType*/,
                    L"DMSVv[0x%p]: ScrollByPixelDelta resetting m_xPixelOffsetRequested=%f to -1.", this, m_xPixelOffsetRequested));
            }
#endif // DM_DEBUG

            m_xPixelOffsetRequested = -1;
        }
    }
    else
    {
        IFC(spScrollInfo->get_VerticalOffset(&oldOffset));

        IFC(IsPanelACarouselPanel(FALSE, isCarouselPanel));
        // Do not clamp offset for carouseling panel.
        if (!isCarouselPanel)
        {
            newOffset = DoubleUtil::Max(newOffset, 0.0);
            if (spProvider || m_contentHeightRequested == -1)
            {
                IFC(get_ScrollableHeight(&scrollable));
            }
            else
            {
                // The ScrollContentPresenter's clamping of the newOffset is based on the smallest of m_xExtent & m_contentWidthRequested
                IFC(get_ViewportHeight(&viewport));
                scrollable = DoubleUtil::Max(m_yMinOffset, DoubleUtil::Min(m_yExtent, m_contentHeightRequested) - viewport);
            }

            newOffset = DoubleUtil::Min(scrollable, newOffset);
        }

        if (!DoubleUtil::AreClose(oldOffset, newOffset))
        {
            if (isCarouselPanel)
            {
                m_yPixelOffsetRequested = newPixelOffset;
            }
            else
            {
                if (spProvider)
                {
                    IFC(ComputePixelScrollableHeight(spProvider.Get(), pixelScrollable));
                }
                else
                {
                    pixelScrollable = scrollable;
                }

                // Do not set an expected vertical offset when this call is not triggered by DManip feedback.
                if (isDManipInput)
                {
                    m_yPixelOffsetRequested = DoubleUtil::Min(DoubleUtil::Max(newPixelOffset, 0.0), pixelScrollable);
                }
            }
#ifdef DM_DEBUG
            if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
            {
                IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/, L"DMSV[0x%p]:  ScrollByPixelDelta oldOffset=%f newOffset=%f, new m_yPixelOffsetRequested=%f.",
                    this, oldOffset, newOffset, m_yPixelOffsetRequested));
            }
#endif // DM_DEBUG

            IFC(ScrollToVerticalOffsetInternal(newOffset));
        }
        else
        {
            // Since the current offset is now close to the target, no offset change is expected
#ifdef DM_DEBUG
            if ((DMSVv_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE)) == S_OK) && m_yPixelOffsetRequested != -1)
            {
                IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE | DMSVv_DBG) /*traceType*/,
                    L"DMSVv[0x%p]: ScrollByPixelDelta resetting m_yPixelOffsetRequested=%f to -1.", this, m_yPixelOffsetRequested));
            }
#endif // DM_DEBUG

            m_yPixelOffsetRequested = -1;
        }
    }

Cleanup:
    RRETURN(hr);
}

// Returns the potential inner CarouselPanel if it's oriented according
// to the provided orientation.
_Check_return_ HRESULT ScrollViewer::IsPanelACarouselPanel(
    _In_ BOOLEAN isForHorizontalOrientation,
    _Out_ BOOLEAN& isCarouselPanel)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IScrollInfo> spScrollInfo;

    isCarouselPanel = FALSE;

    if (isForHorizontalOrientation) // CSP only works for vertical orientation.
    {
        goto Cleanup;
    }

    IFC(get_ScrollInfo(&spScrollInfo));

    if (spScrollInfo)
    {
        ctl::ComPtr<xaml_primitives::ICarouselPanel> spCarouselPanel;

        spCarouselPanel = spScrollInfo.AsOrNull<xaml_primitives::ICarouselPanel>();
        if (spCarouselPanel)
        {
            isCarouselPanel = spCarouselPanel.Cast<CarouselPanel>()->GetShouldCarousel();
        }
    }

Cleanup:
    RRETURN(hr);
}

// Returns the IScrollInfo implementation as a UIElement
_Check_return_ HRESULT ScrollViewer::GetScrollInfoAsElement(
    _Outptr_ xaml::IUIElement** ppElement)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IScrollInfo> spScrollInfo;

    ASSERT(ppElement);
    *ppElement = NULL;

    IFC(get_ScrollInfo(&spScrollInfo));
    if (spScrollInfo)
    {
        ctl::ComPtr<xaml::IUIElement> spElement = spScrollInfo.AsOrNull<xaml::IUIElement>();
        IFC(spElement.CopyTo(ppElement));
    }

Cleanup:
    RRETURN(hr);
}

// Returns the potential inner IManipulationDataProvider regardless of orientation
_Check_return_ HRESULT ScrollViewer::GetInnerManipulationDataProvider(
    _Outptr_result_maybenull_ IManipulationDataProvider** ppProvider)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IScrollInfo> spScrollInfo;

    *ppProvider = NULL;

    IFC(get_ScrollInfo(&spScrollInfo));

    if (spScrollInfo)
    {
        ctl::ComPtr<IManipulationDataProvider> spScrollInfoAsIMDP = spScrollInfo.AsOrNull<IManipulationDataProvider>();
        IFC(spScrollInfoAsIMDP.CopyTo(ppProvider));
    }

Cleanup:
    RRETURN(hr);
}

// Returns the potential inner IManipulationDataProvider if it's oriented according
// to the provided orientation.
_Check_return_ HRESULT ScrollViewer::GetInnerManipulationDataProvider(
    _In_ BOOLEAN isForHorizontalOrientation,
    _Outptr_result_maybenull_ IManipulationDataProvider** ppProvider)
{
    HRESULT hr = S_OK;
    xaml_controls::Orientation orientation = xaml_controls::Orientation_Horizontal;
    ctl::ComPtr<IManipulationDataProvider> spProvider;

    *ppProvider = NULL;

    IFC(GetInnerManipulationDataProvider(&spProvider));

    if (spProvider)
    {
        IFC(spProvider->get_PhysicalOrientation(&orientation));
        if (((orientation == xaml_controls::Orientation_Horizontal) && isForHorizontalOrientation) ||
            ((orientation == xaml_controls::Orientation_Vertical) && !isForHorizontalOrientation))
        {
            IFC(spProvider.MoveTo(ppProvider));
        }
    }

Cleanup:
    RRETURN(hr);
}

// Zoom-related implementations

_Check_return_ HRESULT ScrollViewer::get_ZoomSnapPointsImpl(
    _Outptr_ wfc::IVector<FLOAT>** pValue)
{
    HRESULT hr = S_OK;

    if (!m_spZoomSnapPoints)
    {
        ctl::ComPtr<wfc::VectorChangedEventHandler<FLOAT>> spZoomSnapPointsVectorChangedHandler;

        IFC(ctl::make < ValueTypeObservableCollection < FLOAT >>(&m_spZoomSnapPoints));

        spZoomSnapPointsVectorChangedHandler.Attach(
            new ClassMemberEventHandler <
            ScrollViewer,
            xaml_controls::IScrollViewer,
            wfc::VectorChangedEventHandler<FLOAT>,
            wfc::IObservableVector<FLOAT>,
            wfc::IVectorChangedEventArgs >(this, &ScrollViewer::OnZoomSnapPointsCollectionChanged));

        IFC(m_spZoomSnapPoints->add_VectorChanged(spZoomSnapPointsVectorChangedHandler.Get(), &m_ZoomSnapPointsVectorChangedToken));
    }

    IFC(m_spZoomSnapPoints.CopyTo(pValue));

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::put_ZoomFactor
//
//  Synopsis:
//    Override of the ZoomFactor property setter needed for coercion
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::put_ZoomFactor(
    _In_ FLOAT value)
{
    HRESULT hr = S_OK;

    ASSERT(value >= ScrollViewerMinimumZoomFactor);

    IFC(NotifyZoomFactorChanging(value /*targetZoomFactor*/));

    if (m_cLevelsFromRootCallForZoom == 0)
    {
        m_requestedZoomFactor = value;
    }

    IFC(ScrollViewerGenerated::put_ZoomFactor(value));

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::put_MaxZoomFactor
//
//  Synopsis:
//    Override of the MaxZoomFactor property setter needed for coercion
//
//-------------------------------------------------------------------------
IFACEMETHODIMP ScrollViewer::put_MaxZoomFactor(
    _In_ FLOAT value)
{
    HRESULT hr = S_OK;

    if (m_cLevelsFromRootCallForZoom == 0)
    {
        m_requestedMaxZoomFactor = value;
    }

    IFC(ScrollViewerGenerated::put_MaxZoomFactor(value));

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::ZoomToFactorImpl
//
//  Synopsis:
//    Public and deprecated version of ZoomToFactorInternal.
//    Updates the zoom factor value. Equivalent of ScrollToHorizontalOffset
//    and ScrollToVerticalOffset for the ZoomFactor dependency property.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::ZoomToFactorImpl(
    _In_ FLOAT value)
{
#ifdef DM_DEBUG
    if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/, L"DMSV[0x%p]:  ZoomToFactorImpl value=%4.8lf.", this, value));
    }
#endif // DM_DEBUG

    RRETURN(ZoomToFactorInternal(value, TRUE /*delayAndFlushViewChanged*/, nullptr /*pZoomChanged*/));
}

// Called internally to update the zoom factor property without batching the ViewChanged notifications.
_Check_return_ HRESULT ScrollViewer::ZoomToFactorInternal(
    _In_ FLOAT value,
    _In_ BOOLEAN delayAndFlushViewChanged,
    _Out_opt_ bool* pZoomChanged)
{
    HRESULT hr = S_OK;
    BOOLEAN result = FALSE;
    FLOAT minZoomFactor = 0.0;
    FLOAT maxZoomFactor = 0.0;
    FLOAT zoomFactor = 0.0;
    ctl::ComPtr<IInspectable> spZoomFactor;
    ctl::ComPtr<wf::IReference<FLOAT>> spZoomFactorReference;
    bool zoomChanged = false;

#ifdef DM_DEBUG
    if (DMSVv_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE)) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE | DMSVv_DBG) /*traceType*/, L"DMSVv[0x%p]: ZoomToFactorInternal value=%4.8lf, delayAndFlushViewChanged=%d.", this, value, delayAndFlushViewChanged));
    }
#endif // DM_DEBUG

    if (delayAndFlushViewChanged)
    {
        DelayViewChanged();
    }

    IFC(get_ZoomFactor(&zoomFactor));

    if (value != zoomFactor)
    {
        IFC(get_MinZoomFactor(&minZoomFactor));
        IFC(get_MaxZoomFactor(&maxZoomFactor));

        // Value gets put into the [MinZoomFactor, MaxZoomFactor] range,
        // in accordance to what ScrollToHorizontalOffset does for instance.
        if (value < minZoomFactor)
        {
            value = minZoomFactor;
        }
        else if (value > maxZoomFactor)
        {
            value = maxZoomFactor;
        }

        if (value != zoomFactor)
        {
            if (m_isDirectManipulationZoomFactorChange || !IsInDirectManipulation() || m_currentZoomMode != xaml_controls::ZoomMode_Disabled)
            {
                // Zoom factor change is triggered by DManip feedback,
                // or occurs outside a DManip operation,
                // or is a programmatic zoom factor change request that needs to cancel DManip,
                IFC(put_ZoomFactor(value));
                zoomChanged = true;
            }
            else
            {
                // Zoom factor change is programmatic, during a DManip operation
                IFC(PropertyValue::CreateFromSingle(value, &spZoomFactor));
                IFC(spZoomFactor.As(&spZoomFactorReference));
                IFC(ChangeViewInternal(
                    NULL  /*pHorizontalOffset*/,
                    NULL  /*pVerticalOffset*/,
                    spZoomFactorReference.Get(),
                    NULL  /*pOldZoomFactor*/,
                    FALSE /*forceChangeToCurrentView*/,
                    FALSE /*adjustWithMandatorySnapPoints*/,
                    FALSE /*skipDuringTouchContact*/,
                    FALSE /*skipAnimationWhileRunning*/,
                    TRUE  /*disableAnimation*/,
                    TRUE  /*applyAsManip*/,
                    FALSE /*transformIsInertiaEnd*/,
                    FALSE /*isForMakeVisible*/,
                    &result));

                if (result)
                {
                    zoomChanged = true;
                    if (!m_isTargetZoomFactorValid || m_targetZoomFactor != value)
                    {
                        // Raise the ViewChanging event with the new target zoom factor. This will set m_isTargetZoomFactorValid to True and
                        // m_targetZoomFactor to value, allowing m_targetZoomFactor to be used in any potential imminent call to ChangeViewInternal.
                        NotifyZoomFactorChanging(value);
                    }
                }
            }
        }
    }

Cleanup:
    if (pZoomChanged != nullptr)
    {
        *pZoomChanged = zoomChanged;
    }

    if (delayAndFlushViewChanged)
    {
        RRETURN(FlushViewChanged(hr));
    }
    else
    {
        RRETURN(hr);
    }
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::OnZoomSnapPointsCollectionChanged
//
//  Synopsis:
//    Event handler called when the zoom snap points observable collection
//    changed.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::OnZoomSnapPointsCollectionChanged(
    _In_ wfc::IObservableVector<FLOAT>* pSender,
    _In_ wfc::IVectorChangedEventArgs* e)
{
    RRETURN(OnSnapPointsChanged(DMMotionTypeZoom));
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::OnMinZoomFactorPropertyChanged
//
//  Synopsis:
//    Validates the MinZoomFactor property when its value is changed.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::OnMinZoomFactorPropertyChanged(
    _In_ IInspectable *pOldValue, _In_ IInspectable *pNewValue)
{
    HRESULT hr = S_OK;
    BOOLEAN bIsValidFloatValue;
    BOOLEAN bRestoreOldValue = FALSE;
    FLOAT oldValue = 0, newValue, maxZoomFactor, zoomFactor;

    DelayViewChanged();

    // Ensure it's a valid value
    IFC(ScrollViewer::IsValidFloatValue(pNewValue, newValue, &bIsValidFloatValue));
    if (!bIsValidFloatValue)
    {
        bRestoreOldValue = TRUE;
        // Using ERROR_INVALID_DOUBLE_VALUE even though property is a float, because the error string is "The value cannot be infinite or Not a Number (NaN)."
        IFC(ErrorHelper::OriginateErrorUsingResourceID(E_INVALIDARG, ERROR_INVALID_DOUBLE_VALUE));
    }

    if (newValue < ScrollViewerMinimumZoomFactor)
    {
        bRestoreOldValue = TRUE;
        // "The MinZoomFactor property cannot be set to a value smaller than 0.1."
        IFC(ErrorHelper::OriginateErrorUsingResourceID(E_INVALIDARG, ERROR_SCROLLVIEWER_MINZOOMFACTOR));
    }

    // Note: this section is a workaround, containing the
    // logic to hold all calls to the property changed
    // methods until after all coercion has completed
    // Logic was copied from RangeBase control, for its Minimum, Value, Maximum properties.
    // ----------
    if (m_cLevelsFromRootCallForZoom == 0)
    {
        IFC(get_MaxZoomFactor(&m_initialMaxZoomFactor));
        IFC(get_ZoomFactor(&m_initialZoomFactor));
    }
    m_cLevelsFromRootCallForZoom++;
    // ----------

    IFC(CoerceMaxZoomFactor());
    IFC(CoerceZoomFactor());

    // Note: this section completes the workaround to call
    // the property changed logic if all coercion has completed
    // ----------
    m_cLevelsFromRootCallForZoom--;
    if (m_cLevelsFromRootCallForZoom == 0)
    {
        IFC(GetFloatValue(pOldValue, oldValue));

        IFC(OnZoomFactorBoundaryChanged(TRUE /*isForLowerBound*/, oldValue, newValue));

        IFC(get_MaxZoomFactor(&maxZoomFactor));

        if (m_initialMaxZoomFactor != maxZoomFactor)
        {
            IFC(OnZoomFactorBoundaryChanged(FALSE /*isForLowerBound*/, m_initialMaxZoomFactor, maxZoomFactor));
        }

        IFC(get_ZoomFactor(&zoomFactor));

        if (m_initialZoomFactor != zoomFactor)
        {
            IFC(OnZoomFactorChanged(m_initialZoomFactor, zoomFactor));
        }
    }
    // ----------

Cleanup:
    if (bRestoreOldValue)
    {
        ASSERT(FAILED(hr));
        IGNOREHR(ScrollViewer::IsValidFloatValue(pOldValue, oldValue, &bIsValidFloatValue));
        if (bIsValidFloatValue)
        {
            // Restore the old MinZoomFactor value if it was valid.
            IGNOREHR(put_MinZoomFactor(oldValue));
        }
    }

    RRETURN(FlushViewChanged(hr));
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::OnMaxZoomFactorPropertyChanged
//
//  Synopsis:
//    Validates the MaxZoomFactor property when its value is changed.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::OnMaxZoomFactorPropertyChanged(
    _In_ IInspectable *pOldValue, _In_ IInspectable *pNewValue)
{
    HRESULT hr = S_OK;
    BOOLEAN bIsValidFloatValue;
    BOOLEAN bRestoreOldValue = FALSE;
    FLOAT oldValue = 0, newValue, maxZoomFactor, zoomFactor;

    DelayViewChanged();

    IFC(ScrollViewer::IsValidFloatValue(pNewValue, newValue, &bIsValidFloatValue));

    if (!bIsValidFloatValue)
    {
        bRestoreOldValue = TRUE;
        // Using ERROR_INVALID_DOUBLE_VALUE even though property is a float, because the error string is "The value cannot be infinite or Not a Number (NaN)."
        IFC(ErrorHelper::OriginateErrorUsingResourceID(E_INVALIDARG, ERROR_INVALID_DOUBLE_VALUE));
    }

    if (newValue < ScrollViewerMinimumZoomFactor)
    {
        bRestoreOldValue = TRUE;
        // "The MaxZoomFactor property cannot be set to a value smaller than 0.1."
        IFC(ErrorHelper::OriginateErrorUsingResourceID(E_INVALIDARG, ERROR_SCROLLVIEWER_MAXZOOMFACTOR));
    }

    IFC(GetFloatValue(pOldValue, oldValue));

    // Note: this section is a workaround, containing the
    // logic to hold all calls to the property changed
    // methods until after all coercion has completed
    // ----------
    if (m_cLevelsFromRootCallForZoom == 0)
    {
        m_requestedMaxZoomFactor = newValue;
        m_initialMaxZoomFactor = oldValue;
        IFC(get_ZoomFactor(&m_initialZoomFactor));
    }
    m_cLevelsFromRootCallForZoom++;
    // ----------

    IFC(CoerceMaxZoomFactor());
    IFC(CoerceZoomFactor());

    // Note: this section completes the workaround to call
    // the property changed logic if all coercion has completed
    // ----------
    m_cLevelsFromRootCallForZoom--;
    if (m_cLevelsFromRootCallForZoom == 0)
    {
        IFC(get_MaxZoomFactor(&maxZoomFactor));

        if (m_initialMaxZoomFactor != maxZoomFactor)
        {
            IFC(OnZoomFactorBoundaryChanged(FALSE /*isForLowerBound*/, m_initialMaxZoomFactor, maxZoomFactor));
        }

        IFC(get_ZoomFactor(&zoomFactor));

        if (m_initialZoomFactor != zoomFactor)
        {
            IFC(OnZoomFactorChanged(m_initialZoomFactor, zoomFactor));
        }
    }
    // ----------

Cleanup:
    if (bRestoreOldValue)
    {
        ASSERT(FAILED(hr));
        IGNOREHR(ScrollViewer::IsValidFloatValue(pOldValue, oldValue, &bIsValidFloatValue));
        if (bIsValidFloatValue)
        {
            // Restore the old MaxZoomFactor value if it was valid.
            IGNOREHR(put_MaxZoomFactor(oldValue));
        }
    }

    RRETURN(FlushViewChanged(hr));
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::OnHorizontalOffsetPropertyChanged
//
//  Synopsis:
//    Called when the HorizontalOffset dependency property changed.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::OnHorizontalOffsetPropertyChanged(
    _In_ IInspectable *pOldValue, _In_ IInspectable *pNewValue)
{
    m_isTargetHorizontalOffsetValid = FALSE;
    RRETURN(RaiseViewChanged(m_isInIntermediateViewChangedMode /*isIntermediate*/));
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::OnVerticalOffsetPropertyChanged
//
//  Synopsis:
//    Called when the VerticalOffset dependency property changed.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::OnVerticalOffsetPropertyChanged(
    _In_ IInspectable *pOldValue, _In_ IInspectable *pNewValue)
{
    m_isTargetVerticalOffsetValid = FALSE;
    RRETURN(RaiseViewChanged(m_isInIntermediateViewChangedMode /*isIntermediate*/));
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::OnZoomFactorPropertyChanged
//
//  Synopsis:
//    Validates the ZoomFactor property when its value is changed.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::OnZoomFactorPropertyChanged(
    _In_ IInspectable *pOldValue, _In_ IInspectable *pNewValue)
{
    HRESULT hr = S_OK;
    BOOLEAN bIsValidFloatValue;
    BOOLEAN bRestoreOldValue = FALSE;
    FLOAT oldValue = 0, newValue, zoomFactor;

    IFC(ScrollViewer::IsValidFloatValue(pNewValue, newValue, &bIsValidFloatValue));

    if (!bIsValidFloatValue)
    {
        bRestoreOldValue = TRUE;
        // Using ERROR_INVALID_DOUBLE_VALUE even though property is a float, because the error string is "The value cannot be infinite or Not a Number (NaN)."
        IFC(ErrorHelper::OriginateErrorUsingResourceID(E_INVALIDARG, ERROR_INVALID_DOUBLE_VALUE));
    }

    ASSERT(newValue >= ScrollViewerMinimumZoomFactor);

    IFC(GetFloatValue(pOldValue, oldValue));

    // Note: this section is a workaround, containing the
    // logic to hold all calls to the property changed
    // methods until after all coercion has completed
    // ----------
    if (m_cLevelsFromRootCallForZoom == 0)
    {
        m_requestedZoomFactor = newValue;
        m_initialZoomFactor = oldValue;
    }
    m_cLevelsFromRootCallForZoom++;
    // ----------

    IFC(CoerceZoomFactor());

    // Note: this section completes the workaround to call
    // the property changed logic if all coercion has completed
    // ----------
    m_cLevelsFromRootCallForZoom--;
    if (m_cLevelsFromRootCallForZoom == 0)
    {
        IFC(get_ZoomFactor(&zoomFactor));

        if (m_initialZoomFactor != zoomFactor)
        {
            IFC(OnZoomFactorChanged(m_initialZoomFactor, zoomFactor));
        }
    }
    // ----------

Cleanup:
    if (bRestoreOldValue)
    {
        ASSERT(FAILED(hr));
        IGNOREHR(ScrollViewer::IsValidFloatValue(pOldValue, oldValue, &bIsValidFloatValue));
        if (bIsValidFloatValue)
        {
            // Restore the old ZoomFactor value if it was valid.
            IGNOREHR(put_ZoomFactor(oldValue));
        }
    }
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::CoerceMaxZoomFactor
//
//  Synopsis:
//    Ensures the MaxZoomFactor is greater than or equal to the MinZoomFactor.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::CoerceMaxZoomFactor()
{
    HRESULT hr = S_OK;
    FLOAT minZoomFactor, maxZoomFactor;

    IFC(get_MinZoomFactor(&minZoomFactor));
    IFC(get_MaxZoomFactor(&maxZoomFactor));

    if (m_requestedMaxZoomFactor != maxZoomFactor && m_requestedMaxZoomFactor >= minZoomFactor)
    {
        IFC(ScrollViewerGenerated::put_MaxZoomFactor(m_requestedMaxZoomFactor));
    }
    else if (maxZoomFactor < minZoomFactor)
    {
        IFC(ScrollViewerGenerated::put_MaxZoomFactor(minZoomFactor));
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::CoerceZoomFactor
//
//  Synopsis:
//    Ensures the ZoomFactor falls between the MinZoomFactor and MaxZoomFactor values.
//    This function assumes that (MaxZoomFactor >= MinZoomFactor)
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::CoerceZoomFactor()
{
    HRESULT hr = S_OK;
    FLOAT minZoomFactor, maxZoomFactor, zoomFactor;

    IFC(get_MinZoomFactor(&minZoomFactor));
    IFC(get_MaxZoomFactor(&maxZoomFactor));
    IFC(get_ZoomFactor(&zoomFactor));

    if (m_requestedZoomFactor != zoomFactor && m_requestedZoomFactor >= minZoomFactor && m_requestedZoomFactor <= maxZoomFactor)
    {
        IFC(NotifyZoomFactorChanging(m_requestedZoomFactor /*targetZoomFactor*/));
        IFC(ScrollViewerGenerated::put_ZoomFactor(m_requestedZoomFactor));
    }
    else
    {
        if (zoomFactor < minZoomFactor)
        {
            IFC(NotifyZoomFactorChanging(minZoomFactor /*targetZoomFactor*/));
            IFC(ScrollViewerGenerated::put_ZoomFactor(minZoomFactor));
        }
        if (zoomFactor > maxZoomFactor)
        {
            IFC(NotifyZoomFactorChanging(maxZoomFactor /*targetZoomFactor*/));
            IFC(ScrollViewerGenerated::put_ZoomFactor(maxZoomFactor));
        }
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::GetFloatValue
//
//  Synopsis:
//    Returns a float if the IPropertyValue contains a FLOAT or DOUBLE.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::GetFloatValue(
    _In_ IInspectable *pValue, _Out_ FLOAT& floatValue)
{
    HRESULT hr = S_OK;
    DOUBLE doubleValue = 0.0;

    IFCPTR(pValue);

    if (S_OK != ctl::do_get_value(floatValue, pValue))
    {
        IFC(ctl::do_get_value(doubleValue, pValue));
        floatValue = static_cast<FLOAT>(doubleValue);
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::IsValidFloatValue
//
//  Synopsis:
//    Extracts a float from a IPropertyValue and checks if it's valid.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::IsValidFloatValue(
    _In_ IInspectable *pValue, _Out_ FLOAT& floatValue, _Out_ BOOLEAN* pbIsValidFloatValue)
{
    HRESULT hr = S_OK;

    IFCPTR(pValue);
    IFCPTR(pbIsValidFloatValue);

    *pbIsValidFloatValue = TRUE;

    IFC(GetFloatValue(pValue, floatValue));

    // Using the same code as xcp\win\dll\slm\math.cpp
    // WinNT & CE only use DOUBLE for _isnan and _finite.
    if (_isnan(static_cast<DOUBLE>(floatValue)) || !_finite(static_cast<DOUBLE>(floatValue)))
    {
        *pbIsValidFloatValue = FALSE;
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::OnZoomFactorBoundaryChanged
//
//  Synopsis:
//    Called when the MinZoomFactor or MaxZoomFactor value changed.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::OnZoomFactorBoundaryChanged(
    _In_ BOOLEAN isForLowerBound,
    _In_ FLOAT oldZoomFactorBoundary,
    _In_ FLOAT newZoomFactorBoundary)
{
    HRESULT hr = S_OK;

    // If a DirectManipulation manip is active, push the new boundary to DM.
    IFC(OnPrimaryContentAffectingPropertyChanged(
        FALSE /*boundsChanged*/,
        FALSE /*horizontalAlignmentChanged*/,
        FALSE /*verticalAlignmentChanged*/,
        TRUE  /*zoomFactorBoundaryChanged*/));

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::OnZoomFactorChanged
//
//  Synopsis:
//    Called when the ZoomFactor value changed.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::OnZoomFactorChanged(
    _In_ FLOAT oldZoomFactor,
    _In_ FLOAT newZoomFactor)
{
    HRESULT hr = S_OK;
    BOOLEAN clearInZoomFactorSync = FALSE;
    BOOLEAN isHorizontalScrollSnapPointsUpdateRequired = FALSE;
    BOOLEAN isVerticalScrollSnapPointsUpdateRequired = FALSE;
    BOOLEAN areSnapPointsRegular = FALSE;
    BOOLEAN result = FALSE;
    xaml_controls::ZoomMode zoomMode = xaml_controls::ZoomMode_Disabled;
    xaml_controls::SnapPointsType snapPointsType = xaml_controls::SnapPointsType_None;
    xaml_primitives::SnapPointsAlignment snapPointsAlignment = xaml_primitives::SnapPointsAlignment_Near;

    if (m_trElementScrollContentPresenter)
    {
        IFC(m_trElementScrollContentPresenter.Cast<ScrollContentPresenter>()->SetZoomFactor(newZoomFactor));
    }

    if (m_trScrollSnapPointsInfo)
    {
        IFC(GetEffectiveZoomMode(TRUE /*canUseCachedProperty*/, zoomMode));
        {
            // A manipulatable zoom factor affects:
            //   - Regular and irregular snap points when their alignment is SnapPointsAlignment_Center
            //   - Irregular snap points when their alignment is SnapPointsAlignment_Far

            IFC(ScrollViewerGenerated::get_HorizontalSnapPointsType(&snapPointsType));
            if (snapPointsType != xaml_controls::SnapPointsType_None)
            {
                IFC(ScrollViewerGenerated::get_HorizontalSnapPointsAlignment(&snapPointsAlignment));
                if (snapPointsAlignment == xaml_primitives::SnapPointsAlignment_Center)
                {
                    isHorizontalScrollSnapPointsUpdateRequired = TRUE;
                }
                else if (snapPointsAlignment == xaml_primitives::SnapPointsAlignment_Far)
                {
                    IFC(m_trScrollSnapPointsInfo.Get()->get_AreHorizontalSnapPointsRegular(&areSnapPointsRegular));
                    if (!areSnapPointsRegular)
                    {
                        isHorizontalScrollSnapPointsUpdateRequired = TRUE;
                    }
                }
            }

            IFC(ScrollViewerGenerated::get_VerticalSnapPointsType(&snapPointsType));
            if (snapPointsType != xaml_controls::SnapPointsType_None)
            {
                IFC(ScrollViewerGenerated::get_VerticalSnapPointsAlignment(&snapPointsAlignment));
                if (snapPointsAlignment == xaml_primitives::SnapPointsAlignment_Center)
                {
                    isVerticalScrollSnapPointsUpdateRequired = TRUE;
                }
                else if (snapPointsAlignment == xaml_primitives::SnapPointsAlignment_Far)
                {
                    IFC(m_trScrollSnapPointsInfo.Get()->get_AreVerticalSnapPointsRegular(&areSnapPointsRegular));
                    if (!areSnapPointsRegular)
                    {
                        isVerticalScrollSnapPointsUpdateRequired = TRUE;
                    }
                }
            }
        }

        if (isHorizontalScrollSnapPointsUpdateRequired)
        {
            IFC(OnSnapPointsAffectingPropertyChanged(DMMotionTypePanX, FALSE /*updateSnapPointsChangeSubscription*/));
        }

        if (isVerticalScrollSnapPointsUpdateRequired)
        {
            IFC(OnSnapPointsAffectingPropertyChanged(DMMotionTypePanY, FALSE /*updateSnapPointsChangeSubscription*/));
        }
    }

    if (IsInDirectManipulation())
    {
        if (!m_isDirectManipulationZoomFactorChange)
        {
            if (m_currentZoomMode != xaml_controls::ZoomMode_Disabled)
            {
                // Zoom factor was altered during a manipulation. This manipulation is going to be canceled.
                // Ignore the final zoom factor in HandleManipulationDelta, in order not to override the 'value' set here.
                m_isDirectManipulationZoomFactorChangeIgnored = TRUE;
            }

            // Let the InputManager know about the zoom factor change request.
            // A programmatic zoom factor change during a DM manipulation:
            //  - cancels that manipulation if ZoomMode is Enabled.
            IFC(OnPrimaryContentTransformChanged(FALSE /*translationXChanged*/, FALSE /*translationYChanged*/, TRUE /*zoomFactorChanged*/));
        }
    }
    else if (!m_isInDirectManipulationSync)
    {
        ASSERT(!m_isDirectManipulationZoomFactorChange);
        clearInZoomFactorSync = TRUE;
        m_isInZoomFactorSync = TRUE;
        // Zoom factor change has to be pushed to DManip as the XAML and DManip transforms are kept in sync.
        IFC(ChangeViewInternal(
            NULL  /*pHorizontalOffset*/,
            NULL  /*pVerticalOffset*/,
            NULL  /*pZoomFactor*/,
            &oldZoomFactor,
            TRUE  /*forceChangeToCurrentView*/,
            FALSE /*adjustWithMandatorySnapPoints*/,
            FALSE /*skipDuringTouchContact*/,
            FALSE /*skipAnimationWhileRunning*/,
            TRUE  /*disableAnimation*/,
            FALSE /*applyAsManip*/,
            FALSE /*transformIsInertiaEnd*/,
            FALSE /*isForMakeVisible*/,
            &result));
        // result returned is FALSE when the BringIntoViewport operation is delayed during a manipulation completion.
        clearInZoomFactorSync = FALSE;
        m_isInZoomFactorSync = FALSE;
    }

    m_isTargetZoomFactorValid = FALSE;
    IFC(RaiseViewChanged(m_isInIntermediateViewChangedMode /*isIntermediate*/));

Cleanup:
    if (clearInZoomFactorSync)
    {
        ASSERT(m_isInZoomFactorSync);
        m_isInZoomFactorSync = FALSE;
    }
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::OnPrimaryContentTransformChanged
//
//  Synopsis:
//    Called when the input manager needs to push a new primary content
//    transform to DirectManipulation.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::OnPrimaryContentTransformChanged(
    _In_ BOOLEAN translationXChanged,
    _In_ BOOLEAN translationYChanged,
    _In_ BOOLEAN zoomFactorChanged)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml::IUIElement> spContentUIElement;
    FLOAT translationX = 0.0f;
    FLOAT translationY = 0.0f;
    FLOAT zoomFactor = 1.0f;
    UIElement* pManipulatedElementNoRef = NULL;

    if (m_hManipulationHandler || zoomFactorChanged)
    {
        IFC(GetContentUIElement(&spContentUIElement));
        pManipulatedElementNoRef = spContentUIElement.Cast<UIElement>();
    }

    if (pManipulatedElementNoRef)
    {
        BOOLEAN isInUnstoppedManipulation = IsInUnstoppedManipulation();
        if (m_hManipulationHandler && !(isInUnstoppedManipulation && !m_trManipulatableElement))
        {
            IFC(CoreImports::ManipulationHandler_NotifyPrimaryContentTransformChanged(
                m_hManipulationHandler,
                static_cast<CUIElement*>(pManipulatedElementNoRef->GetHandle()),
                static_cast<bool>(isInUnstoppedManipulation),
                static_cast<bool>(translationXChanged),
                static_cast<bool>(translationYChanged),
                static_cast<bool>(zoomFactorChanged)));
        }
        else
        {
            // Set the manipulated element's static DM transform since the
            // InputManager has not declared itself as the manipulation handler.
            ASSERT(zoomFactorChanged);
            IFC(GetManipulationPrimaryContentTransform(
                pManipulatedElementNoRef,
                FALSE /*inManipulation*/,
                FALSE /*forInitialTransformationAdjustment*/,
                FALSE /*forMargins*/,
                &translationX,
                &translationY,
                &zoomFactor));
        }
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::GetContentUIElement
//
//  Synopsis:
//    Retrieves the UIElement for the ScrollViewer content if any.
//    Returns NULL when the ScrollViewer is not in the tree.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::GetContentUIElement(
    _Outptr_result_maybenull_ xaml::IUIElement** ppContentUIElement)
{
    HRESULT hr = S_OK;

    IFCPTR(ppContentUIElement);
    *ppContentUIElement = NULL;

    if (IsInLiveTree() && m_trElementScrollContentPresenter)
    {
        ctl::ComPtr<IInspectable> spContent;
        ctl::ComPtr<xaml::IUIElement> spContentUIElement;

        IFC(m_trElementScrollContentPresenter.Cast<ScrollContentPresenter>()->get_Content(&spContent));
        spContentUIElement = spContent.AsOrNull<xaml::IUIElement>();
        if (spContentUIElement)
        {
            IFC(spContentUIElement.MoveTo(ppContentUIElement));
        }
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::ComputeTranslationXCorrection
//
//  Synopsis:
//    Computes the effect of the left margin and horizontal alignment when
//    the content is smaller than the viewport.
//    The extent parameter provided is valid when positive, and needs to be
//    computed when strictly negative.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::ComputeTranslationXCorrection(
    _In_ BOOLEAN inManipulation,
    _In_ BOOLEAN forInitialTransformationAdjustment,
    _In_ BOOLEAN adjustDimensions,
    _In_opt_ IManipulationDataProvider* pProvider,
    _In_ DOUBLE leftMargin,
    _In_ DOUBLE extent,
    _In_ FLOAT zoomFactor,
    _Out_ FLOAT* pTranslationX)
{
    HRESULT hr = S_OK;
    DOUBLE viewport = 0;
    DOUBLE translationX = 0;
    DMAlignment alignment;

    ASSERT(zoomFactor > 0.0f);

    IFCPTR(pTranslationX);
    *pTranslationX = 0.0f;

    IFC(ComputeHorizontalAlignment(TRUE /*canUseCachedProperties*/, alignment));

    if (alignment == DMAlignmentCenter || alignment == DMAlignmentFar)
    {
        if (extent < 0)
        {
            IFC(ComputePixelExtentWidth(false /*ignoreZoomFactor*/, pProvider, &extent));
        }
        IFC(ComputePixelViewportWidth(pProvider, TRUE /*isProviderSet*/, &viewport));

        if (!forInitialTransformationAdjustment && extent < viewport)
        {
            if (!inManipulation && extent / zoomFactor < viewport)
            {
                translationX = (1.0 / static_cast<DOUBLE>(zoomFactor)-1.0) * extent;
            }
            else
            {
                translationX = viewport - extent;
            }

            if (alignment == DMAlignmentCenter)
            {
                translationX /= 2.0;
            }
        }
        else if (extent / zoomFactor < viewport && !inManipulation)
        {
            translationX = extent / zoomFactor - viewport;

            if (alignment == DMAlignmentCenter)
            {
                translationX /= 2.0;
            }
        }

        if (adjustDimensions && extent < viewport)
        {
            // Take into account the fact that DManip consumes integers instead of decimal numbers for content and viewport dimensions.

            // Evaluate exact offset used by XAML layout engine
            DOUBLE offsetX = extent / zoomFactor - viewport;
            if (alignment == DMAlignmentCenter)
            {
                offsetX /= 2.0;
            }

            // Evaluate approximated offset used by DManip based on adjusted content extent and viewport width
            DOUBLE adjustedOffsetX = AdjustPixelContentDim(extent / zoomFactor) - AdjustPixelViewportDim(viewport);
            if (alignment == DMAlignmentCenter)
            {
                adjustedOffsetX /= 2.0;
            }

            // Adjust the returned translation based on the difference between the two XAML/DManip evaluations.
            // translationX is computed differently above depending on forInitialTransformationAdjustment (a variation of
            // viewport-extent vs. extent-viewport), so the sign of the adjustment depends on forInitialTransformationAdjustment too.
            if (forInitialTransformationAdjustment)
            {
                translationX += adjustedOffsetX - offsetX;
            }
            else
            {
                translationX += offsetX - adjustedOffsetX;
            }
        }
    }

    // Skip the margin's contribution when the initial adjustment is computed - in that case
    // GetManipulationPrimaryContentTransform is also called with forMargins==True.
    if (!forInitialTransformationAdjustment)
    {
        translationX += (static_cast<DOUBLE>(zoomFactor)-1.0) * leftMargin;
    }

    *pTranslationX = static_cast<FLOAT>(translationX);

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::ComputeTranslationYCorrection
//
//  Synopsis:
//    Computes the effect of the top margin and vertical alignment when
//    the content is smaller than the viewport.
//    The extent parameter provided is valid when positive, and needs to be
//    computed when strictly negative.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::ComputeTranslationYCorrection(
    _In_ BOOLEAN inManipulation,
    _In_ BOOLEAN forInitialTransformationAdjustment,
    _In_ BOOLEAN adjustDimensions,
    _In_opt_ IManipulationDataProvider* pProvider,
    _In_ DOUBLE topMargin,
    _In_ DOUBLE extent,
    _In_ FLOAT zoomFactor,
    _Out_ FLOAT* pTranslationY)
{
    HRESULT hr = S_OK;
    DOUBLE viewport = 0;
    DOUBLE translationY = 0;
    DMAlignment alignment;

    IFCPTR(pTranslationY);
    *pTranslationY = 0.0f;

    IFC(ComputeVerticalAlignment(TRUE /*canUseCachedProperties*/, alignment));

    if (alignment == DMAlignmentCenter || alignment == DMAlignmentFar)
    {
        if (extent < 0)
        {
            IFC(ComputePixelExtentHeight(false /*ignoreZoomFactor*/, pProvider, &extent));
        }
        IFC(ComputePixelViewportHeight(pProvider, TRUE /*isProviderSet*/, &viewport));

        if (!forInitialTransformationAdjustment && extent < viewport)
        {
            if (!inManipulation && extent / zoomFactor < viewport)
            {
                translationY = (1.0 / static_cast<DOUBLE>(zoomFactor)-1.0) * extent;
            }
            else
            {
                translationY = viewport - extent;
            }

            if (alignment == DMAlignmentCenter)
            {
                translationY /= 2.0;
            }
        }
        else if (extent / zoomFactor < viewport && !inManipulation)
        {
            translationY = extent / zoomFactor - viewport;

            if (alignment == DMAlignmentCenter)
            {
                translationY /= 2.0;
            }
        }

        if (adjustDimensions && extent < viewport)
        {
            // Take into account the fact that DManip consumes integers instead of decimal numbers for content and viewport dimensions.

            // Evaluate exact offset used by XAML layout engine
            DOUBLE offsetY = extent / zoomFactor - viewport;
            if (alignment == DMAlignmentCenter)
            {
                offsetY /= 2.0;
            }

            // Evaluate approximated offset used by DManip based on adjusted content extent and viewport height
            DOUBLE adjustedOffsetY = AdjustPixelContentDim(extent / zoomFactor) - AdjustPixelViewportDim(viewport);
            if (alignment == DMAlignmentCenter)
            {
                adjustedOffsetY /= 2.0;
            }

            // Adjust the returned translation based on the difference between the two XAML/DManip evaluations.
            // translationY is computed differently above depending on forInitialTransformationAdjustment (a variation of
            // viewport-extent vs. extent-viewport), so the sign of the adjustment depends on forInitialTransformationAdjustment too.
            if (forInitialTransformationAdjustment)
            {
                translationY += adjustedOffsetY - offsetY;
            }
            else
            {
                translationY += offsetY - adjustedOffsetY;
            }
        }
    }

    // Skip the margin's contribution when the initial adjustment is computed - in that case
    // GetManipulationPrimaryContentTransform is also called with forMargins==True.
    if (!forInitialTransformationAdjustment)
    {
        translationY += (static_cast<DOUBLE>(zoomFactor)-1.0) * topMargin;
    }

    *pTranslationY = static_cast<FLOAT>(translationY);

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::GetEffectiveIsHorizontalRailEnabled
//
//  Synopsis:
//    Retrieves the effective IsHorizontalRailEnabled value:
//    m_currentIsHorizontalRailEnabled or get_IsHorizontalRailEnabled
//    depending on whether there is an active manip or not.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::GetEffectiveIsHorizontalRailEnabled(
    _In_ BOOLEAN canUseCachedProperty,
    _Out_ BOOLEAN& isHorizontalRailEnabled)
{
    HRESULT hr = S_OK;

    if (IsInDirectManipulation() && canUseCachedProperty)
    {
        isHorizontalRailEnabled = m_currentIsHorizontalRailEnabled;
    }
    else
    {
        IFC(get_IsHorizontalRailEnabled(&isHorizontalRailEnabled));
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::GetEffectiveIsVerticalRailEnabled
//
//  Synopsis:
//    Retrieves the effective IsVerticalRailEnabled value:
//    m_currentIsVerticalRailEnabled or get_IsVerticalRailEnabled
//    depending on whether there is an active manip or not.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::GetEffectiveIsVerticalRailEnabled(
    _In_ BOOLEAN canUseCachedProperty,
    _Out_ BOOLEAN& isVerticalRailEnabled)
{
    HRESULT hr = S_OK;

    if (IsInDirectManipulation() && canUseCachedProperty)
    {
        isVerticalRailEnabled = m_currentIsVerticalRailEnabled;
    }
    else
    {
        IFC(get_IsVerticalRailEnabled(&isVerticalRailEnabled));
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::GetEffectiveIsScrollInertiaEnabled
//
//  Synopsis:
//    Retrieves the effective IsScrollInertiaEnabled value:
//    m_currentIsScrollInertiaEnabled or get_IsScrollInertiaEnabled
//    depending on whether there is an active manip or not.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::GetEffectiveIsScrollInertiaEnabled(
    _In_ BOOLEAN canUseCachedProperty,
    _Out_ BOOLEAN& isScrollInertiaEnabled)
{
    HRESULT hr = S_OK;

    if (IsInDirectManipulation() && canUseCachedProperty)
    {
        isScrollInertiaEnabled = m_currentIsScrollInertiaEnabled;
    }
    else
    {
        IFC(get_IsScrollInertiaEnabled(&isScrollInertiaEnabled));
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::GetEffectiveIsZoomInertiaEnabled
//
//  Synopsis:
//    Retrieves the effective IsZoomInertiaEnabled value:
//    m_currentIsZoomInertiaEnabled or get_IsZoomInertiaEnabled depending
//    on whether there is an active manip or not.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::GetEffectiveIsZoomInertiaEnabled(
    _In_ BOOLEAN canUseCachedProperty,
    _Out_ BOOLEAN& isZoomInertiaEnabled)
{
    HRESULT hr = S_OK;

    if (IsInDirectManipulation() && canUseCachedProperty)
    {
        isZoomInertiaEnabled = m_currentIsZoomInertiaEnabled;
    }
    else
    {
        IFC(get_IsZoomInertiaEnabled(&isZoomInertiaEnabled));
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::GetEffectiveHorizontalScrollBarVisibility
//
//  Synopsis:
//    Retrieves the effective horizontal scrollbar visibility:
//    m_currentHorizontalScrollBarVisibility or get_HorizontalScrollBarVisibility
//    depending on whether there is an active manip or not.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::GetEffectiveHorizontalScrollBarVisibility(
    _In_ BOOLEAN canUseCachedProperty,
    _Out_ xaml_controls::ScrollBarVisibility& hsbv)
{
    HRESULT hr = S_OK;

    if (IsInDirectManipulation() && canUseCachedProperty)
    {
        hsbv = m_currentHorizontalScrollBarVisibility;
    }
    else
    {
        IFC(get_HorizontalScrollBarVisibility(&hsbv));
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::GetEffectiveVerticalScrollBarVisibility
//
//  Synopsis:
//    Retrieves the effective vertical scrollbar visibility:
//    m_currentVerticalScrollBarVisibility or get_VerticalScrollBarVisibility
//    depending on whether there is an active manip or not.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::GetEffectiveVerticalScrollBarVisibility(
    _In_ BOOLEAN canUseCachedProperty,
    _Out_ xaml_controls::ScrollBarVisibility& vsbv)
{
    HRESULT hr = S_OK;

    if (IsInDirectManipulation() && canUseCachedProperty)
    {
        vsbv = m_currentVerticalScrollBarVisibility;
    }
    else
    {
        IFC(get_VerticalScrollBarVisibility(&vsbv));
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::GetEffectiveHorizontalAlignment
//
//  Synopsis:
//    Retrieves the horizontal alignment at the beginning of the current
//    manipulation if any when canUseCachedProperty is True, or the result
//    of ComputeHorizontalAlignment otherwise.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::GetEffectiveHorizontalAlignment(
    _In_ BOOLEAN canUseCachedProperty,
    _Out_ DMAlignment& horizontalAlignment)
{
    HRESULT hr = S_OK;

    if (IsInDirectManipulation() && canUseCachedProperty)
    {
        horizontalAlignment = m_currentHorizontalAlignment;
    }
    else
    {
        IFC(ComputeHorizontalAlignment(FALSE /*canUseCachedProperties*/, horizontalAlignment));
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::GetEffectiveVerticalAlignment
//
//  Synopsis:
//    Retrieves the vertical alignment at the beginning of the current
//    manipulation if any when canUseCachedProperty is True, or the result
//    of ComputeHorizontalAlignment otherwise.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::GetEffectiveVerticalAlignment(
    _In_ BOOLEAN canUseCachedProperty,
    _Out_ DMAlignment& verticalAlignment)
{
    HRESULT hr = S_OK;

    if (IsInDirectManipulation() && canUseCachedProperty)
    {
        verticalAlignment = m_currentVerticalAlignment;
    }
    else
    {
        IFC(ComputeHorizontalAlignment(FALSE /*canUseCachedProperties*/, verticalAlignment));
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::GetEffectiveHorizontalScrollMode
//
//  Synopsis:
//    Retrieves the effective horizontal scroll mode: m_currentHorizontalScrollMode or
//    get_HorizontalScrollMode depending on whether there is an active manip or not.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::GetEffectiveHorizontalScrollMode(
    _In_ BOOLEAN canUseCachedProperty,
    _Out_ xaml_controls::ScrollMode& horizontalScrollMode)
{
    HRESULT hr = S_OK;

    if (IsInDirectManipulation() && canUseCachedProperty)
    {
        horizontalScrollMode = m_currentHorizontalScrollMode;
    }
    else
    {
        IFC(get_HorizontalScrollMode(&horizontalScrollMode));
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::GetEffectiveVerticalScrollMode
//
//  Synopsis:
//    Retrieves the effective vertical scroll mode: m_currentVerticalScrollMode or
//    get_VerticalScrollMode depending on whether there is an active manip or not.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::GetEffectiveVerticalScrollMode(
    _In_ BOOLEAN canUseCachedProperty,
    _Out_ xaml_controls::ScrollMode& verticalScrollMode)
{
    HRESULT hr = S_OK;

    if (IsInDirectManipulation() && canUseCachedProperty)
    {
        verticalScrollMode = m_currentVerticalScrollMode;
    }
    else
    {
        IFC(get_VerticalScrollMode(&verticalScrollMode));
    }

Cleanup:
    RRETURN(hr);
}


//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::GetEffectiveZoomMode
//
//  Synopsis:
//    Retrieves the effective zoom mode: m_currentZoomMode or get_ZoomMode
//    depending on whether there is an active manip or not.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::GetEffectiveZoomMode(
    _In_ BOOLEAN canUseCachedProperty,
    _Out_ xaml_controls::ZoomMode& zoomMode)
{
    HRESULT hr = S_OK;

    if (IsInDirectManipulation() && canUseCachedProperty)
    {
        zoomMode = m_currentZoomMode;
    }
    else
    {
        IFC(get_ZoomMode(&zoomMode));
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::GetTopLeftMargins
//
//  Synopsis:
//    Retrieves the left and top margins of the provided element.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::GetTopLeftMargins(
    _In_ UIElement* pElement,
    _Out_ DOUBLE& topMargin,
    _Out_ DOUBLE& leftMargin)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml::IFrameworkElement> spFrameworkElement;
    xaml::Thickness margins;

    IFCPTR(pElement);

    topMargin = leftMargin = 0;

    spFrameworkElement = ctl::query_interface_cast<xaml::IFrameworkElement>(pElement);
    if (spFrameworkElement)
    {
        IFC(spFrameworkElement->get_Margin(&margins));
        topMargin = margins.Top;
        leftMargin = margins.Left;
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::GetManipulationConfigurations
//
//  Synopsis:
//    Returns the DMConfigurations values to provide to DM based on the
//    DM-related properties and physical characteristics of the ScrollViewer
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::GetManipulationConfigurations(
    _In_opt_ BOOLEAN* pIsInLiveTree,
    _In_ BOOLEAN canUseCachedProperties,
    _Out_opt_ BOOLEAN* pCanManipulateElementsByTouch,
    _Out_opt_ BOOLEAN* pCanManipulateElementsWithAsyncBringIntoViewport,
    _Out_opt_ DMConfigurations* pTouchConfiguration,
    _Out_opt_ DMConfigurations* pNonTouchConfiguration,
    _Out_opt_ DMConfigurations* pBringIntoViewportConfiguration,
    _Out_opt_ UINT8* pcConfigurations,
    _Outptr_result_buffer_maybenull_(*pcConfigurations) DMConfigurations** ppConfigurations)
{
    static const UINT8 MaxConfigs = 6;

    HRESULT hr = S_OK;
    ctl::ComPtr<IManipulationDataProvider> spProvider;
    ctl::ComPtr<xaml_media::IGeneralTransform> spGeneralTransform;
    ctl::ComPtr<xaml_media::IMatrixTransform> spMatrixTransform;
    ctl::ComPtr<xaml::IUIElement> spContentUIElement;
    ctl::ComPtr<xaml::IUIElement> spHeaderUIElement;
    ctl::ComPtr<xaml::IUIElement> spPointedUIElement;
    ctl::ComPtr<xaml::IDependencyObject> spPointedDOElement;
    DOUBLE scrollableDim = 0;
    DOUBLE viewportDim = 0;
    BOOLEAN canManipulateElementsByTouch = FALSE;
    BOOLEAN isInLiveTree = FALSE;
    BOOLEAN isEnabled = FALSE;
    BOOLEAN isZoomInertiaEnabled = FALSE;
    BOOLEAN isScrollInertiaEnabled = FALSE;
    BOOLEAN isRailEnabled = FALSE;
    BOOLEAN isPanXOptional = FALSE;
    BOOLEAN isPanYOptional = FALSE;
    BOOLEAN isPointedElementInTopLeftHeader = FALSE;
    BOOLEAN isPointedElementInTopHeader = FALSE;
    BOOLEAN isPointedElementInLeftHeader = FALSE;
    UINT8 cConfigurations = 0;
    UINT8 iConfiguration = 0;
    DMConfigurations* pConfigurations = NULL;
    DMConfigurations touchConfiguration = DMConfigurationNone;
    DMConfigurations nonTouchConfiguration = DMConfigurationNone;
    DMConfigurations bringIntoViewportConfiguration = (DMConfigurations)(DMConfigurationPanX + DMConfigurationPanY + DMConfigurationZoom + DMConfigurationPanInertia + DMConfigurationZoomInertia);
    DMConfigurations zoomConfiguration = DMConfigurationNone;
    DMConfigurations panXConfiguration = DMConfigurationNone;
    DMConfigurations panYConfiguration = DMConfigurationNone;
    DMAlignment alignment = DMAlignmentNone;
    xaml_controls::ScrollMode orthoScrollMode = xaml_controls::ScrollMode_Disabled;
    xaml_controls::ScrollMode scrollMode = xaml_controls::ScrollMode_Disabled;
    xaml_controls::ZoomMode zoomMode = xaml_controls::ZoomMode_Disabled;

    ASSERT((pcConfigurations && ppConfigurations) || (!pcConfigurations && !ppConfigurations));

    if (pCanManipulateElementsByTouch)
    {
        *pCanManipulateElementsByTouch = FALSE;
    }
    if (pCanManipulateElementsWithAsyncBringIntoViewport)
    {
        *pCanManipulateElementsWithAsyncBringIntoViewport = FALSE;
    }
    if (pTouchConfiguration)
    {
        *pTouchConfiguration = DMConfigurationNone;
    }
    if (pNonTouchConfiguration)
    {
        *pNonTouchConfiguration = DMConfigurationNone;
    }
    if (pBringIntoViewportConfiguration)
    {
        *pBringIntoViewportConfiguration = DMConfigurationNone;
    }
    if (pcConfigurations)
    {
        *pcConfigurations = 0;
    }
    if (ppConfigurations)
    {
        *ppConfigurations = NULL;
    }

    if (!m_hManipulationHandler)
    {
        // No manipulation is feasible without a manipulation handler.
        goto Cleanup;
    }

    if (pIsInLiveTree)
    {
        isInLiveTree = *pIsInLiveTree;
    }
    else
    {
        isInLiveTree = IsInLiveTree();
    }

    IFC(ComputePixelViewportWidth(NULL /*pProvider*/, FALSE /*isProviderSet*/, &viewportDim));
    if (viewportDim == 0)
    {
        // When the ScrollViewer is not loaded yet, the ActualWidth of the IManipulationDataProvider
        // might still be 0 while the advertised IScrollInfo's ViewportWidth is strictly positive.
        // Ignore the nil ActualWidth in that case.
        IFC(get_ViewportWidth(&viewportDim));
        if (m_isLoaded || viewportDim == 0)
        {
            // DirectManipulation does not support a viewport with a nil width
            if (isInLiveTree)
            {
                goto Cleanup;
            }
            else
            {
                IFC(GetInnerManipulationDataProvider(TRUE /*isForHorizontalOrientation*/, &spProvider));
                if (spProvider)
                {
                    // This ScrollViewer contains an item-based horizontal panel that still has a nil ActualWidth.
                    // Reset the m_xPixelViewport value so that the next call to InvalidateScrollInfo_TryUpdateValues detects
                    // a viewport width change and causes a DManip viewport registration and refresh of its configurations.
                    m_xPixelViewport = 0.0;
                }
            }
        }
    }

    IFC(ComputePixelViewportHeight(NULL /*pProvider*/, FALSE /*isProviderSet*/, &viewportDim));
    if (viewportDim == 0)
    {
        // When the ScrollViewer is not loaded yet, the ActualHeight of the IManipulationDataProvider
        // might still be 0 while the advertised IScrollInfo's ViewportHeight is strictly positive.
        // Ignore the nil ActualHeight in that case.
        IFC(get_ViewportHeight(&viewportDim));
        if (m_isLoaded || viewportDim == 0)
        {
            // DirectManipulation does not support a viewport with a nil height
            if (isInLiveTree)
            {
                goto Cleanup;
            }
            else
            {
                IFC(GetInnerManipulationDataProvider(FALSE /*isForHorizontalOrientation*/, &spProvider));
                if (spProvider)
                {
                    // This ScrollViewer contains an item-based vertical panel that still has a nil ActualHeight.
                    // Reset the m_yPixelViewport value so that the next call to InvalidateScrollInfo_TryUpdateValues detects
                    // a viewport height change and causes a DManip viewport registration and refresh of its configurations.
                    m_yPixelViewport = 0.0;
                }
            }
        }
    }

    if (!isInLiveTree)
    {
        goto Cleanup;
    }

    // Do not allow DirectManipulation manips when IsEnabled is False
    // Except for programmatic BringIntoViewport calls.
    IFC(get_IsEnabled(&isEnabled));

    IFC(GetContentUIElement(&spContentUIElement));
    if (spContentUIElement)
    {
        ASSERT(m_trElementScrollContentPresenter);
        isInLiveTree = m_trElementScrollContentPresenter.Cast<ScrollContentPresenter>()->IsInLiveTree();
        if (!isInLiveTree)
        {
            // The ScrollContentPresenter is not in the tree. This turns off the manipulability of the ScrollViewer.
            goto Cleanup;
        }
        IFC(m_trElementScrollContentPresenter.Cast<ScrollContentPresenter>()->TransformToVisual(NULL, &spGeneralTransform));
        spMatrixTransform = spGeneralTransform.AsOrNull<xaml_media::IMatrixTransform>();
        xaml_media::Matrix matrix;
        if (spMatrixTransform)
        {
            IFC(spMatrixTransform->get_Matrix(&matrix));
        }
        // When spMatrixTransform is null or the determinant is 0, synchronous moves through BringIntoViewport are still allowed.
        if (spMatrixTransform != nullptr && matrix.M11 * matrix.M22 - matrix.M21 * matrix.M12 != 0.0)
        {
            if (pCanManipulateElementsWithAsyncBringIntoViewport)
            {
                *pCanManipulateElementsWithAsyncBringIntoViewport = TRUE;
            }

            if (isEnabled)
            {
                IFC(GetEffectiveZoomMode(canUseCachedProperties, zoomMode));
                if (zoomMode != xaml_controls::ZoomMode_Disabled)
                {
                    zoomConfiguration = DMConfigurationZoom;
                    IFC(GetEffectiveIsZoomInertiaEnabled(canUseCachedProperties, isZoomInertiaEnabled));
                    if (isZoomInertiaEnabled)
                    {
                        zoomConfiguration = (DMConfigurations)(zoomConfiguration + DMConfigurationZoomInertia);
                    }
                    touchConfiguration = zoomConfiguration;
                    canManipulateElementsByTouch = TRUE;
                }

                IFC(get_PointedElement(&spPointedDOElement));
                spPointedUIElement = spPointedDOElement.AsOrNull<IUIElement>();

                if (touchConfiguration == DMConfigurationNone || pTouchConfiguration || ppConfigurations || pcConfigurations)
                {
                    IFC(GetEffectiveHorizontalScrollMode(canUseCachedProperties, scrollMode));
                    alignment = DMAlignmentNone;
                    if (scrollMode == xaml_controls::ScrollMode_Disabled)
                    {
                        // Even when horizontal panning is turned off, check for vertical panning and horizontal alignment such
                        // that horizontal panning can be temporarily turned on to address scenarios of WPB bugs 261102 & 342668.
                        IFC(GetEffectiveVerticalScrollMode(canUseCachedProperties, orthoScrollMode));
                        if (orthoScrollMode != xaml_controls::ScrollMode_Disabled)
                        {
                            IFC(GetEffectiveHorizontalAlignment(canUseCachedProperties, alignment));
                        }
                    }
                    if (scrollMode != xaml_controls::ScrollMode_Disabled || alignment == DMAlignmentCenter || alignment == DMAlignmentFar)
                    {
                        // When horizontal scroll mode is Enabled, horizontal panning is always turned on,
                        // independently of ZoomMode's value and content size.

                        // When scroll mode is Auto, panning is on or off (i.e. optional)
                        // depending on whether the content is wider than the viewport or not.
                        isPanXOptional = (scrollMode == xaml_controls::ScrollMode_Auto || scrollMode == xaml_controls::ScrollMode_Disabled);

                        if (!isPanXOptional || spPointedUIElement)
                        {
                            // PanX also becomes optional when there is a TopLeftHeader or LeftHeader element.
                            IFC(get_LeftHeader(&spHeaderUIElement));
                            if (spHeaderUIElement)
                            {
                                isPanXOptional = TRUE;
                                if (spPointedUIElement)
                                {
                                    IFC(spHeaderUIElement.Cast<UIElement>()->IsAncestorOf(spPointedUIElement.Cast<UIElement>(), &isPointedElementInLeftHeader));
                                }
                            }
                            IFC(get_TopLeftHeader(&spHeaderUIElement));
                            if (spHeaderUIElement)
                            {
                                isPanXOptional = TRUE;
                                if (spPointedUIElement)
                                {
                                    IFC(spHeaderUIElement.Cast<UIElement>()->IsAncestorOf(spPointedUIElement.Cast<UIElement>(), &isPointedElementInTopLeftHeader));
                                }
                            }
                        }

                        if (scrollMode == xaml_controls::ScrollMode_Auto)
                        {
                            IFC(get_ScrollableWidth(&scrollableDim));
                        }
                        else
                        {
                            scrollableDim = 0;
                        }

                        panXConfiguration = DMConfigurationPanX;

                        IFC(GetEffectiveIsHorizontalRailEnabled(canUseCachedProperties, isRailEnabled));
                        if (isRailEnabled)
                        {
                            panXConfiguration = (DMConfigurations)(panXConfiguration + DMConfigurationRailsX);
                        }

                        IFC(GetEffectiveIsScrollInertiaEnabled(canUseCachedProperties, isScrollInertiaEnabled));
                        if (isScrollInertiaEnabled)
                        {
                            panXConfiguration = (DMConfigurations)(panXConfiguration + DMConfigurationPanInertia);
                        }

                        // If the content is smaller than the viewport and scroll mode is automatic, don't allow scrolling in that direction.
                        // m_cForcePanXConfiguration is used to address WPB bugs 261102 & 342668.
                        if ((scrollMode != xaml_controls::ScrollMode_Auto && scrollMode != xaml_controls::ScrollMode_Disabled) || scrollableDim != 0 || m_cForcePanXConfiguration > 0)
                        {
                            if (!isPointedElementInLeftHeader && !isPointedElementInTopLeftHeader)
                            {
                                touchConfiguration = (DMConfigurations)(touchConfiguration + panXConfiguration);
                            }
                        }
                        canManipulateElementsByTouch = TRUE;
                    }
                }
            }

            if (touchConfiguration == DMConfigurationNone || pTouchConfiguration || ppConfigurations || pcConfigurations)
            {
                IFC(GetEffectiveVerticalScrollMode(canUseCachedProperties, scrollMode));
                alignment = DMAlignmentNone;
                if (scrollMode == xaml_controls::ScrollMode_Disabled)
                {
                    // Even when vertical panning is turned off, check for horizontal panning and vertical alignment such
                    // that vertical panning can be temporarily turned on to address scenarios of WPB bugs 261102 & 342668.
                    IFC(GetEffectiveHorizontalScrollMode(canUseCachedProperties, orthoScrollMode));
                    if (orthoScrollMode != xaml_controls::ScrollMode_Disabled)
                    {
                        IFC(GetEffectiveVerticalAlignment(canUseCachedProperties, alignment));
                    }
                }
                if (scrollMode != xaml_controls::ScrollMode_Disabled || alignment == DMAlignmentCenter || alignment == DMAlignmentFar)
                {
                    // When vertical scroll mode is Enabled, vertical panning is always turned on,
                    // independently of ZoomMode's value and content size.

                    // When scroll mode is Auto, panning is on or off (i.e. optional)
                    // depending on whether the content is taller than the viewport or not.
                    isPanYOptional = (scrollMode == xaml_controls::ScrollMode_Auto || scrollMode == xaml_controls::ScrollMode_Disabled);

                    if (!isPanYOptional || spPointedUIElement)
                    {
                        // PanY also becomes optional when there is a TopLeftHeader or TopHeader element.
                        IFC(get_TopHeader(&spHeaderUIElement));
                        if (spHeaderUIElement)
                        {
                            isPanYOptional = TRUE;
                            if (spPointedUIElement)
                            {
                                IFC(spHeaderUIElement.Cast<UIElement>()->IsAncestorOf(spPointedUIElement.Cast<UIElement>(), &isPointedElementInTopHeader));
                            }
                        }
                        IFC(get_TopLeftHeader(&spHeaderUIElement));
                        if (spHeaderUIElement)
                        {
                            isPanYOptional = TRUE;
                            if (spPointedUIElement)
                            {
                                IFC(spHeaderUIElement.Cast<UIElement>()->IsAncestorOf(spPointedUIElement.Cast<UIElement>(), &isPointedElementInTopLeftHeader));
                            }
                        }
                    }

                    if (scrollMode == xaml_controls::ScrollMode_Auto)
                    {
                        IFC(get_ScrollableHeight(&scrollableDim));
                    }
                    else
                    {
                        scrollableDim = 0;
                    }

                    panYConfiguration = DMConfigurationPanY;

                    IFC(GetEffectiveIsVerticalRailEnabled(canUseCachedProperties, isRailEnabled));
                    if (isRailEnabled)
                    {
                        panYConfiguration = (DMConfigurations)(panYConfiguration + DMConfigurationRailsY);
                    }

                    IFC(GetEffectiveIsScrollInertiaEnabled(canUseCachedProperties, isScrollInertiaEnabled));
                    if (isScrollInertiaEnabled)
                    {
                        panYConfiguration = (DMConfigurations)(panYConfiguration + DMConfigurationPanInertia);
                    }

                    // If the content is smaller than the viewport and scroll mode is automatic, don't allow scrolling in that direction.
                    // m_cForcePanYConfiguration is used to address WPB bugs 261102 & 342668.
                    if ((scrollMode != xaml_controls::ScrollMode_Auto && scrollMode != xaml_controls::ScrollMode_Disabled) || scrollableDim != 0 || m_cForcePanYConfiguration > 0)
                    {
                        if (!isPointedElementInTopHeader && !isPointedElementInTopLeftHeader)
                        {
                            if ((touchConfiguration & DMConfigurationPanInertia) == DMConfigurationPanInertia)
                            {
                                touchConfiguration = (DMConfigurations)(touchConfiguration - DMConfigurationPanInertia);
                            }
                            touchConfiguration = (DMConfigurations)(touchConfiguration + panYConfiguration);
                        }
                    }
                    canManipulateElementsByTouch = TRUE;
                }
            }

            if (touchConfiguration == DMConfigurationNone && canManipulateElementsByTouch)
            {
                // touchConfiguration == DMConfigurationNone because the content extent is too small for now.
                // The viewport is still declared manipulatable by touch for the cases where a non-touch manipulation
                // causes the extent(s) to increase which enables touch interactivity.
                touchConfiguration = DMConfigurationInteraction;
            }

            IFC(GetNonTouchManipulationConfiguration(canUseCachedProperties, &nonTouchConfiguration));
        }

        // Configurations may have to be merged when a touch or non-touch configuration was set.
        if (touchConfiguration != DMConfigurationNone || nonTouchConfiguration != DMConfigurationNone)
        {
            if (pTouchConfiguration)
            {
                *pTouchConfiguration = touchConfiguration;
            }
            if (pNonTouchConfiguration)
            {
                *pNonTouchConfiguration = nonTouchConfiguration;
            }
            if (pBringIntoViewportConfiguration)
            {
                *pBringIntoViewportConfiguration = bringIntoViewportConfiguration;
            }
            if (ppConfigurations)
            {
                if (touchConfiguration != DMConfigurationNone)
                {
                    cConfigurations = 1 * (isPanXOptional ? 2 : 1) * (isPanYOptional ? 2 : 1);
                }
                pConfigurations = new DMConfigurations[MaxConfigs];
                // Initialize the configuration with DMConfigurationNone
                for (iConfiguration = 0; iConfiguration < MaxConfigs; iConfiguration++)
                {
                    pConfigurations[iConfiguration] = DMConfigurationNone;
                }

                if (touchConfiguration == DMConfigurationNone)
                {
                    cConfigurations = 0;
                }
                else
                {
                    if (isPanXOptional)
                    {
                        if (isPanYOptional)
                        {
                            pConfigurations[0] = zoomConfiguration == DMConfigurationNone ? DMConfigurationInteraction : zoomConfiguration;
                            pConfigurations[1] = (DMConfigurations)(zoomConfiguration + panXConfiguration);
                            pConfigurations[2] = (DMConfigurations)(zoomConfiguration + panYConfiguration);
                            pConfigurations[3] = (DMConfigurations)(zoomConfiguration + RemoveDuplicatePanInertia(isScrollInertiaEnabled, panXConfiguration, panYConfiguration));
                        }
                        else
                        {
                            pConfigurations[0] = (DMConfigurations)((zoomConfiguration + panYConfiguration) == DMConfigurationNone ? DMConfigurationInteraction : (zoomConfiguration + panYConfiguration));
                            pConfigurations[1] = (DMConfigurations)(zoomConfiguration + RemoveDuplicatePanInertia(isScrollInertiaEnabled, panXConfiguration, panYConfiguration));
                        }
                    }
                    else
                    {
                        if (isPanYOptional)
                        {
                            pConfigurations[0] = (DMConfigurations)((zoomConfiguration + panXConfiguration) == DMConfigurationNone ? DMConfigurationInteraction : (zoomConfiguration + panXConfiguration));
                            pConfigurations[1] = (DMConfigurations)(zoomConfiguration + RemoveDuplicatePanInertia(isScrollInertiaEnabled, panXConfiguration, panYConfiguration));
                        }
                        else
                        {
                            pConfigurations[0] = (DMConfigurations)(zoomConfiguration + RemoveDuplicatePanInertia(isScrollInertiaEnabled, panXConfiguration, panYConfiguration));
                        }
                    }

                    // Remove all the occurrences of DMConfigurationNone.
                    IFC(CompactManipulationConfigurations(pConfigurations, cConfigurations));
                    ASSERT(cConfigurations > 0);
                }

                if (nonTouchConfiguration != DMConfigurationNone)
                {
                    // Merge with the non-touch configuration value. Make sure it's not already present in the array.
                    for (iConfiguration = 0; iConfiguration < cConfigurations; iConfiguration++)
                    {
                        if (pConfigurations[iConfiguration] == nonTouchConfiguration)
                        {
                            break;
                        }
                    }
                    if (iConfiguration == cConfigurations)
                    {
                        // The non-touch configuration value was not found. Add it to the array.
                        pConfigurations[iConfiguration] = nonTouchConfiguration;
                        cConfigurations++;
                    }
                }

                ASSERT(bringIntoViewportConfiguration != DMConfigurationNone);
                // Merge with the bring-into-viewport configuration value. Make sure it's not already present in the array.
                for (iConfiguration = 0; iConfiguration < cConfigurations; iConfiguration++)
                {
                    if (pConfigurations[iConfiguration] == bringIntoViewportConfiguration)
                    {
                        break;
                    }
                }
                if (iConfiguration == cConfigurations)
                {
                    // The bring-into-viewport configuration value was not found. Add it to the array.
                    pConfigurations[iConfiguration] = bringIntoViewportConfiguration;
                    cConfigurations++;
                }

                *ppConfigurations = pConfigurations;
                pConfigurations = NULL;
            }
            if (pcConfigurations)
            {
                ASSERT(cConfigurations > 0);
                *pcConfigurations = cConfigurations;
            }
        }
        else
        {
            ASSERT(!pTouchConfiguration || *pTouchConfiguration == DMConfigurationNone);
            ASSERT(!pNonTouchConfiguration || *pNonTouchConfiguration == DMConfigurationNone);

            pConfigurations = new DMConfigurations[1];

            pConfigurations[0] = bringIntoViewportConfiguration;

            if (pBringIntoViewportConfiguration)
            {
                *pBringIntoViewportConfiguration = bringIntoViewportConfiguration;
            }
            if (ppConfigurations)
            {
                *ppConfigurations = pConfigurations;
                pConfigurations = NULL;
                ASSERT(pcConfigurations);
                *pcConfigurations = 1;
            }
        }
    }

    if (pCanManipulateElementsByTouch)
    {
        *pCanManipulateElementsByTouch = canManipulateElementsByTouch;
    }

Cleanup:
    delete[] pConfigurations;
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::GetNonTouchManipulationConfiguration
//
//  Synopsis:
//    Returns the DMConfigurations value to provide to DM for keyboard/mouse wheel operations.
//    Configuration is based on the DM-related properties of the ScrollViewer.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::GetNonTouchManipulationConfiguration(
    _In_ BOOLEAN canUseCachedProperties,
    _Out_ DMConfigurations* pNonTouchConfiguration)
{
    HRESULT hr = S_OK;
    BOOLEAN isZoomInertiaEnabled = FALSE;
    BOOLEAN isScrollInertiaEnabled = FALSE;
    DMConfigurations nonTouchConfiguration = DMConfigurationNone;
    xaml_controls::ZoomMode zoomMode;
    xaml_controls::ScrollMode horizontalScrollMode;
    xaml_controls::ScrollMode verticalScrollMode;

    IFCPTR(pNonTouchConfiguration);

    *pNonTouchConfiguration = DMConfigurationNone;

    IFC(GetEffectiveIsScrollInertiaEnabled(canUseCachedProperties, isScrollInertiaEnabled));
    IFC(GetEffectiveIsZoomInertiaEnabled(canUseCachedProperties, isZoomInertiaEnabled));
    IFC(GetEffectiveZoomMode(canUseCachedProperties, zoomMode));
    IFC(GetEffectiveHorizontalScrollMode(canUseCachedProperties, horizontalScrollMode));
    IFC(GetEffectiveVerticalScrollMode(canUseCachedProperties, verticalScrollMode));

    if (horizontalScrollMode != xaml_controls::ScrollMode_Disabled)
    {
        nonTouchConfiguration = (DMConfigurations)(nonTouchConfiguration + DMConfigurationPanX);
    }

    if (verticalScrollMode != xaml_controls::ScrollMode_Disabled)
    {
        nonTouchConfiguration = (DMConfigurations)(nonTouchConfiguration + DMConfigurationPanY);
    }

    if (isScrollInertiaEnabled && nonTouchConfiguration != DMConfigurationNone)
    {
        nonTouchConfiguration = (DMConfigurations)(nonTouchConfiguration + DMConfigurationPanInertia);
    }

    if (zoomMode != xaml_controls::ZoomMode_Disabled)
    {
        nonTouchConfiguration = (DMConfigurations)(nonTouchConfiguration + DMConfigurationZoom);
        if (isZoomInertiaEnabled)
        {
            nonTouchConfiguration = (DMConfigurations)(nonTouchConfiguration + DMConfigurationZoomInertia);
        }
    }

    *pNonTouchConfiguration = nonTouchConfiguration;

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::CompactManipulationConfigurations
//
//  Synopsis:
//    Removes all the configurations in the array equal to DMConfigurationNone,
//    potentially resulting in a compacted array.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::CompactManipulationConfigurations(
    _In_reads_(cConfigurations) DMConfigurations* pConfigurations,
    _Inout_ UINT8& cConfigurations)
{
    HRESULT hr = S_OK;
    UINT8 iValidConfiguration = 0;

    IFCPTR(pConfigurations);

    for (UINT8 iConfiguration = 0; iConfiguration < cConfigurations; iConfiguration++)
    {
        if (pConfigurations[iConfiguration] != DMConfigurationNone)
        {
            if (iValidConfiguration != iConfiguration)
            {
                pConfigurations[iValidConfiguration] = pConfigurations[iConfiguration];
            }
            iValidConfiguration++;
        }
    }
    cConfigurations = iValidConfiguration;

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::HookScrollSnapPointsInfoEvents
//
//  Synopsis:
//    Hooks up one snap points change event
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::HookScrollSnapPointsInfoEvents(
    _In_ BOOLEAN isForHorizontalSnapPoints)
{
    HRESULT hr = S_OK;

    if (m_trScrollSnapPointsInfo)
    {
        ctl::ComPtr<wf::IEventHandler<IInspectable*>> spHorizontalSnapPointsChangedEventHandler;
        ctl::ComPtr<wf::IEventHandler<IInspectable*>> spVerticalSnapPointsChangedEventHandler;

        if (!m_spHorizontalSnapPointsChangedEventHandler && isForHorizontalSnapPoints)
        {
            spHorizontalSnapPointsChangedEventHandler.Attach(new HorizontalSnapPointsChangedHandler<ScrollViewer>(this));
            IFC(m_trScrollSnapPointsInfo.Get()->add_HorizontalSnapPointsChanged(spHorizontalSnapPointsChangedEventHandler.Get(), &m_HorizontalSnapPointsChangedToken));
            m_spHorizontalSnapPointsChangedEventHandler = spHorizontalSnapPointsChangedEventHandler;
        }

        if (!m_spVerticalSnapPointsChangedEventHandler && !isForHorizontalSnapPoints)
        {
            spVerticalSnapPointsChangedEventHandler.Attach(new VerticalSnapPointsChangedHandler<ScrollViewer>(this));
            IFC(m_trScrollSnapPointsInfo.Get()->add_VerticalSnapPointsChanged(spVerticalSnapPointsChangedEventHandler.Get(), &m_VerticalSnapPointsChangedToken));
            m_spVerticalSnapPointsChangedEventHandler = spVerticalSnapPointsChangedEventHandler;
        }
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::UnhookScrollSnapPointsInfoEvents
//
//  Synopsis:
//    Unhooks one snap points change event
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::UnhookScrollSnapPointsInfoEvents(
    _In_ BOOLEAN isForHorizontalSnapPoints)
{
    HRESULT hr = S_OK;

    if (auto peg = m_trScrollSnapPointsInfo.TryMakeAutoPeg())
    {
        if (m_spHorizontalSnapPointsChangedEventHandler && isForHorizontalSnapPoints)
        {
            IFC(m_trScrollSnapPointsInfo.Get()->remove_HorizontalSnapPointsChanged(m_HorizontalSnapPointsChangedToken));
            ZeroMemory(&m_HorizontalSnapPointsChangedToken, sizeof(m_HorizontalSnapPointsChangedToken));
            m_spHorizontalSnapPointsChangedEventHandler = nullptr;
        }

        if (m_spVerticalSnapPointsChangedEventHandler && !isForHorizontalSnapPoints)
        {
            IFC(m_trScrollSnapPointsInfo.Get()->remove_VerticalSnapPointsChanged(m_VerticalSnapPointsChangedToken));
            ZeroMemory(&m_VerticalSnapPointsChangedToken, sizeof(m_VerticalSnapPointsChangedToken));
            m_spVerticalSnapPointsChangedEventHandler = nullptr;
        }
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::RefreshScrollBarIsIgnoringUserInput
//
//  Synopsis:
//    Updates the ScrollBar's IsIgnoringUserInput flag based on the scroll mode
//    setting. Delays the update when there is an ongoing manipulation.
//    The horizontal or vertical ScrollBar is affected depending on the param.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::RefreshScrollBarIsIgnoringUserInput(
    _In_ BOOLEAN isForHorizontalOrientation)
{
    HRESULT hr = S_OK;
    xaml_controls::ScrollMode scrollMode;

    if (IsInDirectManipulation())
    {
        // Refresh the ScrollBar after the current manipulation
        m_isScrollBarIgnoringUserInputInvalid = TRUE;
        goto Cleanup;
    }

    if (isForHorizontalOrientation)
    {
        if (m_trElementHorizontalScrollBar)
        {
            IFC(get_HorizontalScrollMode(&scrollMode));
            IFC(m_trElementHorizontalScrollBar.Cast<ScrollBar>()->put_IsIgnoringUserInput(scrollMode == xaml_controls::ScrollMode_Disabled));
        }
    }
    else
    {
        if (m_trElementVerticalScrollBar)
        {
            IFC(get_VerticalScrollMode(&scrollMode));
            IFC(m_trElementVerticalScrollBar.Cast<ScrollBar>()->put_IsIgnoringUserInput(scrollMode == xaml_controls::ScrollMode_Disabled));
        }
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::RefreshScrollSnapPointsInfo
//
//  Synopsis:
//    Checks if the ScrollContentPresenter's content implements IScrollSnapPointsInfo
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::RefreshScrollSnapPointsInfo()
{
    HRESULT hr = S_OK;

    IFC(UnhookScrollSnapPointsInfoEvents(TRUE /*isForHorizontalSnapPoints*/));
    IFC(UnhookScrollSnapPointsInfoEvents(FALSE /*isForHorizontalSnapPoints*/));
    m_trScrollSnapPointsInfo.Clear();

    if (m_trElementScrollContentPresenter)
    {
        ctl::ComPtr<IInspectable> spContent;

        IFC(m_trElementScrollContentPresenter.Cast<ScrollContentPresenter>()->get_Content(&spContent));
        if (spContent)
        {
            ctl::ComPtr<xaml_primitives::IScrollSnapPointsInfo> spScrollSnapPointsInfo;

            // First check if the ScrollContentPresenter's Content is an IScrollSnapPointsInfo
            spScrollSnapPointsInfo = spContent.AsOrNull<xaml_primitives::IScrollSnapPointsInfo>();

            if (!spScrollSnapPointsInfo)
            {
                ctl::ComPtr<xaml_controls::IItemsPresenter> spItemsPresenter;

                // Then check if it's an ItemsPresenter with an IScrollSnapPointsInfo child
                spItemsPresenter = spContent.AsOrNull<xaml_controls::IItemsPresenter>();
                if (spItemsPresenter)
                {
                    if (spItemsPresenter)
                    {
                        ItemsPresenter* pContentAsIPNoRef = spItemsPresenter.Cast<ItemsPresenter>();
                        INT childCount = 0;
                        BOOLEAN bTemplateApplied = FALSE;
                        IFC(pContentAsIPNoRef->InvokeApplyTemplate(&bTemplateApplied));

                        IFC(VisualTreeHelper::GetChildrenCountStatic(pContentAsIPNoRef, &childCount));
                        if (childCount > 0)
                        {
                            ctl::ComPtr<IDependencyObject> spChildAsDO;

                            IFC(VisualTreeHelper::GetChildStatic(pContentAsIPNoRef, 0, &spChildAsDO));
                            spScrollSnapPointsInfo = spChildAsDO.AsOrNull<xaml_primitives::IScrollSnapPointsInfo>();
                        }
                    }
                }
            }

            if (spScrollSnapPointsInfo)
            {
                SetPtrValue(m_trScrollSnapPointsInfo, spScrollSnapPointsInfo);
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

// Public version of BringIntoViewportInternal that first checks if the ScrollViewer can
// manipulate its content with a bring-into-viewport call.
// Brings the specified bounds of the content into the viewport using DirectManipulation.
// Uses an animation when possible.
_Check_return_ HRESULT ScrollViewer::BringIntoViewport(
    _In_ XRECTF& bounds,
    _In_ BOOLEAN skipDuringTouchContact,
    _In_ BOOLEAN skipAnimationWhileRunning,
    _In_ BOOLEAN animate,
    _Out_ BOOLEAN* pHandled)
{
#ifdef DM_DEBUG
    if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/, L"DMSV[0x%p]:  BringIntoViewport - bounds=(%f, %f, %f, %f), skipDuringTouchContact=%d, skipAnimationWhileRunning=%d.",
            this, bounds.X, bounds.Y, bounds.Width, bounds.Height, skipDuringTouchContact, skipAnimationWhileRunning));
    }
#endif // DM_DEBUG

    *pHandled = FALSE;

    if (m_hManipulationHandler)
    {
        // Make sure the ScrollViewer can manipulate its content with bring-into-viewport calls.
        BOOLEAN canManipulateElementsByTouch = FALSE;
        BOOLEAN canManipulateElementsNonTouch = FALSE;
        BOOLEAN canManipulateElementsWithBringIntoViewport = FALSE;

        IFC_RETURN(get_CanManipulateElements(
            &canManipulateElementsByTouch,
            &canManipulateElementsNonTouch,
            &canManipulateElementsWithBringIntoViewport));

        if (canManipulateElementsWithBringIntoViewport)
        {
            IFC_RETURN(BringIntoViewportInternal(
                bounds,
                0.0f /*translateX*/,
                0.0f /*translateY*/,
                1.0f /*zoomFactor*/,
                FALSE /*transformIsValid*/,
                skipDuringTouchContact,
                skipAnimationWhileRunning,
                animate && IsAnimationEnabled() /*animate*/,
                TRUE /*applyAsManip*/,
                FALSE /*isForMakeVisible*/,
                pHandled));
        }
    }

    return S_OK;
}

// Brings the specified bounds of the content into the viewport using DirectManipulation.
// If animate is True, a DM animation is used.
_Check_return_ HRESULT ScrollViewer::BringIntoViewportInternal(
    _In_ XRECTF& bounds,
    _In_ FLOAT translateX,
    _In_ FLOAT translateY,
    _In_ FLOAT zoomFactor,
    _In_ BOOLEAN transformIsValid,
    _In_ BOOLEAN skipDuringTouchContact,
    _In_ BOOLEAN skipAnimationWhileRunning,
    _In_ BOOLEAN animate,
    _In_ BOOLEAN applyAsManip,
    _In_ BOOLEAN isForMakeVisible,
    _Out_ BOOLEAN* pHandled)
{
    HRESULT hr = S_OK;
    bool fHandled = false;

#ifdef DM_DEBUG
    if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/, L"DMSV[0x%p]:  BringIntoViewportInternal - bounds=(%f, %f, %f, %f), translateX=%f, translateY=%f, zoomFactor=%f,",
            this, bounds.X, bounds.Y, bounds.Width, bounds.Height, translateX, translateY, zoomFactor));
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/, L"                   transformIsValid=%d, skipDuringTouchContact=%d, skipAnimationWhileRunning=%d, animate=%d, applyAsManip=%d, isForMakeVisible=%d.",
            transformIsValid, skipDuringTouchContact, skipAnimationWhileRunning, animate, applyAsManip, isForMakeVisible));
    }
#endif // DM_DEBUG

    ASSERT(pHandled);
    *pHandled = FALSE;

    if (m_hManipulationHandler)
    {
        ctl::ComPtr<xaml::IUIElement> spContentUIElement;

        IFC(GetContentUIElement(&spContentUIElement));
        if (spContentUIElement)
        {
            IFC(CoreImports::ManipulationHandler_BringIntoViewport(
                m_hManipulationHandler,
                static_cast<CUIElement*>(spContentUIElement.Cast<UIElement>()->GetHandle()),
                bounds,
                translateX,
                translateY,
                zoomFactor,
                static_cast<bool>(transformIsValid),
                static_cast<bool>(skipDuringTouchContact),
                static_cast<bool>(skipAnimationWhileRunning),
                static_cast<bool>(animate),
                static_cast<bool>(applyAsManip),
                static_cast<bool>(isForMakeVisible),
                &fHandled));
            *pHandled = static_cast<BOOLEAN>(fHandled);
        }
    }

Cleanup:
    RRETURN(hr);
}

// Temporary workaround for DManip bug 799346
// Called by internal controls to override the MinZoomFactor or MaxZoomFactor value
_Check_return_ HRESULT ScrollViewer::SetDirectManipulationOverridingZoomBoundaries()
{
    HRESULT hr = S_OK;
    FLOAT translationX = 0.0f;
    FLOAT translationY = 0.0f;
    FLOAT uncompressedZoomFactor = 1.0f;
    FLOAT zoomFactorX = 1.0f;
    FLOAT zoomFactorY = 1.0f;
    FLOAT minZoomFactor = 1.0f;
    FLOAT maxZoomFactor = 1.0f;

    if (m_hManipulationHandler)
    {
        ctl::ComPtr<xaml::IUIElement> spContentUIElement;

        IFC(GetContentUIElement(&spContentUIElement));
        if (spContentUIElement)
        {
            IFC(CoreImports::ManipulationHandler_GetPrimaryContentTransform(
                m_hManipulationHandler,
                static_cast<CUIElement*>(spContentUIElement.Cast<UIElement>()->GetHandle()),
                FALSE /*fForBringIntoViewport*/,
                translationX,
                translationY,
                uncompressedZoomFactor,
                zoomFactorX,
                zoomFactorY));

            IFC(get_MinZoomFactor(&minZoomFactor));
            IFC(get_MaxZoomFactor(&maxZoomFactor));

            if (uncompressedZoomFactor < minZoomFactor)
            {
                m_overridingMinZoomFactor = MAX(ScrollViewerMinimumZoomFactor, uncompressedZoomFactor - ScrollViewerZoomRoundingTolerance);
                m_overridingMaxZoomFactor = 0.0f;
            }
            else if (uncompressedZoomFactor > maxZoomFactor)
            {
                m_overridingMinZoomFactor = 0.0f;
                m_overridingMaxZoomFactor = uncompressedZoomFactor + ScrollViewerZoomRoundingTolerance;
            }
            else
            {
                m_overridingMinZoomFactor = 0.0f;
                m_overridingMaxZoomFactor = 0.0f;
            }

#ifdef DM_DEBUG
            if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
            {
                IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/,
                    L"DMSV[0x%p]:  SetDirectManipulationOverridingZoomBoundaries - Notifying zoom factor boundary change. m_overridingMinZoomFactor=%f m_overridingMaxZoomFactor=%f.",
                    this, m_overridingMinZoomFactor, m_overridingMaxZoomFactor));
            }
#endif // DM_DEBUG

            IFC(OnPrimaryContentAffectingPropertyChanged(
                FALSE /*boundsChanged*/,
                FALSE /*horizontalAlignmentChanged*/,
                FALSE /*verticalAlignmentChanged*/,
                TRUE  /*zoomFactorBoundaryChanged*/));
        }
    }

Cleanup:
    RRETURN(hr);
}

// Temporary workaround for DManip bug 799346
// Called by internal controls to undo the overriding of the MinZoomFactor or MaxZoomFactor value
_Check_return_ HRESULT ScrollViewer::ResetDirectManipulationOverridingZoomBoundaries()
{
#ifdef DM_DEBUG
    if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/, L"DMSV[0x%p]:  ResetDirectManipulationOverridingZoomBoundaries - Resetting zoom boundary overrides.", this));
    }
#endif // DM_DEBUG

    m_overridingMinZoomFactor = 0.0f;
    m_overridingMaxZoomFactor = 0.0f;
    RRETURN(S_OK);
}

// Called by internal controls to apply a pseudo-LayoutTransform
// to the ScrollViewer.Content element.
// That transform is a scale transform with identical X and Y factors: layoutScale.
// That layout scale is treated by the ScrollContentPresenter like the DM ZoomFactor.
// The scale center is always the left/top corner.
_Check_return_ HRESULT ScrollViewer::SetLayoutSize(
    _In_ wf::Size layoutSize)
{
    if (layoutSize.Width != m_layoutSize.Width || layoutSize.Height != m_layoutSize.Height)
    {
        DOUBLE viewport = 0;
        wf::Size availableSize = { 0.0f, 0.0f };

#ifdef DM_DEBUG
        if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/, L"DMSV[0x%p]:  SetLayoutSize - layoutSize=%f, %f.", this, layoutSize.Width, layoutSize.Height));
        }
#endif // DM_DEBUG

        m_layoutSize = layoutSize;

        if (m_trElementScrollContentPresenter)
        {
            IFC_RETURN(m_trElementScrollContentPresenter.Cast<ScrollContentPresenter>()->InvalidateMeasure());

            IFC_RETURN(ComputePixelViewportWidth(NULL /*pProvider*/, FALSE /*isProviderSet*/, &viewport));
            availableSize.Width = static_cast<FLOAT>(viewport);
            IFC_RETURN(ComputePixelViewportHeight(NULL /*pProvider*/, FALSE /*isProviderSet*/, &viewport));
            availableSize.Height = static_cast<FLOAT>(viewport);
            IFC_RETURN(m_trElementScrollContentPresenter.Cast<ScrollContentPresenter>()->Measure(availableSize));
        }
    }

    return S_OK;
}

// Called at the end of a DM manipulation when the first layout occurs
// after receiving the DMManipulationCompleted notification.
_Check_return_ HRESULT ScrollViewer::PostDirectManipulationLayoutRefreshed()
{
    m_isInDirectManipulationCompletion = FALSE;
    RRETURN(NotifyLayoutRefreshed());
}

// Member of the IScrollOwner internal contract. Allows the interface consumer to notify this ScrollViewer
// that an ArrangeOverride occurred after the consumer gets an IManipulationDataProvider::UpdateInManipulation(...)
// call with isInLiveTree=True. Also called by PostDirectManipulationLayoutRefreshed during the first
// ScrollContentPresenter layout after a DManip completes.
_Check_return_ HRESULT ScrollViewer::NotifyLayoutRefreshed()
{
#ifdef DM_DEBUG
    if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/, L"DMSV[0x%p]:  NotifyLayoutRefreshed.", this));
    }
#endif // DM_DEBUG

    RRETURN(OnPrimaryContentChanged(
        TRUE  /*layoutRefreshed*/,
        FALSE /*boundsChanged*/,
        FALSE /*horizontalAlignmentChanged*/,
        FALSE /*verticalAlignmentChanged*/,
        FALSE /*zoomFactorBoundaryChanged*/));
}

// Register this instance as being under control of a semantic zoom.
_Check_return_ HRESULT ScrollViewer::RegisterAsSemanticZoomHost()
{
    m_ignoreSemanticZoomNavigationInput = TRUE;

    if (m_trElementScrollContentPresenter)
    {
        RRETURN(m_trElementScrollContentPresenter.Cast<ScrollContentPresenter>()->RegisterAsSemanticZoomPresenter());
    }
    RRETURN(S_OK);
}


//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::SetConstantVelocities
//
//  Synopsis:
//    Starts a constant-velocity pan on this ScrollViewer with the given X and Y
//    velocities, in pixels/second. If both parameters are 0, the pan is stopped.
//    NOTE: Currently, it is not supported to call this method with a pan already
//    in progress. This means modifying the velocities is not currently possible
//    without first completely stopping the pan (by calling this method with both
//    parameters = 0).
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::SetConstantVelocities(
    _In_ XFLOAT dx,
    _In_ XFLOAT dy)
{
    HRESULT hr = S_OK;

    if (m_hManipulationHandler)
    {
        ctl::ComPtr<xaml::IUIElement> spContentUIElement;

        IFC(GetContentUIElement(&spContentUIElement));
        if (spContentUIElement)
        {
            IFC(CoreImports::ManipulationHandler_SetConstantVelocities(
                m_hManipulationHandler,
                static_cast<CUIElement*>(spContentUIElement.Cast<UIElement>()->GetHandle()),
                dx,
                dy));
        }
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::ProcessInputMessage
//
//  Synopsis:
//    Called when this DM container wants the DM handler to process the current
//    input message, by forwarding it to DirectManipulation.
//    The handler must set the isHandled flag to True if the message was handled.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::ProcessInputMessage(
    _In_ bool ignoreFlowDirection,
    _Out_ BOOLEAN& isHandled)
{
    HRESULT hr = S_OK;
    bool fHandled = false;

    isHandled = FALSE;
    if (m_hManipulationHandler)
    {
        ctl::ComPtr<xaml::IUIElement> spContentUIElement;

        IFC(GetContentUIElement(&spContentUIElement));
        if (spContentUIElement)
        {
            IFC(CoreImports::ManipulationHandler_ProcessInputMessage(
                m_hManipulationHandler,
                static_cast<CUIElement*>(spContentUIElement.Cast<UIElement>()->GetHandle()),
                ignoreFlowDirection,
                fHandled));
            isHandled = static_cast<BOOLEAN>(fHandled);
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ScrollViewer::CanScrollForFocusNavigation(_In_ wsy::VirtualKey key,
    _In_  xaml_input::FocusNavigationDirection direction,
    _Out_ bool* canScroll)
{
    // check if we can scroll
    BOOLEAN continueRouting = FALSE;
    // Check if current key should trigger chaining
    IFC_RETURN(ShouldContinueRoutingKeyDownEvent(key, continueRouting));

    *canScroll = !continueRouting;
    if (*canScroll)
    {
        xaml_controls::ScrollMode scrollMode = xaml_controls::ScrollMode_Disabled;

        switch (direction)
        {
        case xaml_input::FocusNavigationDirection::FocusNavigationDirection_Up:
        case xaml_input::FocusNavigationDirection::FocusNavigationDirection_Down:
            IFC_RETURN(get_VerticalScrollMode(&scrollMode));
            break;

        case xaml_input::FocusNavigationDirection::FocusNavigationDirection_Left:
        case xaml_input::FocusNavigationDirection::FocusNavigationDirection_Right:
            IFC_RETURN(get_HorizontalScrollMode(&scrollMode));
            break;
        }

        // if scroll mode is disabled, we cannot scroll
        *canScroll = (scrollMode != xaml_controls::ScrollMode_Disabled);
    }

    return S_OK;
}

_Check_return_ HRESULT ScrollViewer::GetScrollViewerGlobalBoundsWithoutHeaders(_Out_ XRECTF_RB* scrollViewerGlobalBoundsWithoutHeaders)
{
    ctl::ComPtr<IInspectable> spContent;
    XRECTF_RB scrollViewerGlobalBounds = { 0.0f, 0.0f, 0.0f, 0.0f };
    *scrollViewerGlobalBoundsWithoutHeaders = scrollViewerGlobalBounds;

    IFC_RETURN(GetHandle()->GetGlobalBounds(&scrollViewerGlobalBounds, TRUE /* ignoreClipping */));

    // account for top and left header //
    // Determine how large the SV's viewport is compared to the SV's entire size.

    XSIZEF sizeViewportRatios = { 0.0f, 0.0f };
    IFC_RETURN(GetViewportRatios(nullptr, &sizeViewportRatios));

    // Set the real width and height on the SV's viewport.
    float physicalViewportWidth = (scrollViewerGlobalBounds.right - scrollViewerGlobalBounds.left) * sizeViewportRatios.width;
    float physicalViewportHeight = (scrollViewerGlobalBounds.bottom - scrollViewerGlobalBounds.top) * sizeViewportRatios.height;
    xaml::FlowDirection flowDirection = xaml::FlowDirection_LeftToRight;

    // Switch left/right for flow direction
    IFC_RETURN(get_FlowDirection(&flowDirection));
    if (flowDirection == xaml::FlowDirection_RightToLeft)
    {
        scrollViewerGlobalBounds.right = scrollViewerGlobalBounds.left + physicalViewportWidth;
    }
    else
    {
        scrollViewerGlobalBounds.left = scrollViewerGlobalBounds.right - physicalViewportWidth;
    }
    scrollViewerGlobalBounds.top = scrollViewerGlobalBounds.bottom - physicalViewportHeight;

    if (XamlOneCoreTransforms::IsEnabled())
    {
        // In OneCoreTransforms mode, GetGlobalBounds returns logical pixels so we must convert to RasterizedClient.
        const float scale = RootScale::GetRasterizationScaleForElement(GetHandle());
        scrollViewerGlobalBounds.left *= scale;
        scrollViewerGlobalBounds.top *= scale;
        scrollViewerGlobalBounds.right *= scale;
        scrollViewerGlobalBounds.bottom *= scale;
    }

    *scrollViewerGlobalBoundsWithoutHeaders = scrollViewerGlobalBounds;

    return S_OK;
}

_Check_return_ HRESULT ScrollViewer::GetUnobstructedScrollViewerGlobalBounds(_Out_ wf::Rect* unobstructedScrollViewerGlobalBounds)
{
    CInputPaneHandler* pInputPaneHandler = NULL;
    XRECTF_RB viewportGlobalBounds = { 0.0f, 0.0f, 0.0f, 0.0f };
    XRECTF_RB tvSafePadding = { 0.0f, 0.0f, 0.0f, 0.0f };
    unobstructedScrollViewerGlobalBounds->X = 0.0f;
    unobstructedScrollViewerGlobalBounds->Y = 0.0f;
    unobstructedScrollViewerGlobalBounds->Width = 0.0f;
    unobstructedScrollViewerGlobalBounds->Height = 0.0f;

    IFC_RETURN(GetScrollViewerGlobalBoundsWithoutHeaders(&viewportGlobalBounds));

    CContentRoot* contentRoot = VisualTree::GetContentRootForElement(GetHandle());
    pInputPaneHandler = reinterpret_cast<CInputPaneHandler*>(contentRoot->GetInputManager().GetInputPaneHandler());
    if (pInputPaneHandler)
    {
        IFC_RETURN(pInputPaneHandler->GetVisibleBoundsAdjustment(
            viewportGlobalBounds,
            &tvSafePadding));
    }

    unobstructedScrollViewerGlobalBounds->X = viewportGlobalBounds.left;
    unobstructedScrollViewerGlobalBounds->Y = viewportGlobalBounds.top;
    unobstructedScrollViewerGlobalBounds->Width = viewportGlobalBounds.right - viewportGlobalBounds.left;
    unobstructedScrollViewerGlobalBounds->Height = viewportGlobalBounds.bottom - viewportGlobalBounds.top;

    if (unobstructedScrollViewerGlobalBounds->Width > tvSafePadding.left && tvSafePadding.left > 0)
    {
        unobstructedScrollViewerGlobalBounds->X += tvSafePadding.left;
        unobstructedScrollViewerGlobalBounds->Width -= tvSafePadding.left;
    }

    if (unobstructedScrollViewerGlobalBounds->Width > tvSafePadding.right && tvSafePadding.right > 0)
    {
        unobstructedScrollViewerGlobalBounds->Width -= tvSafePadding.right;
    }

    if (unobstructedScrollViewerGlobalBounds->Height > tvSafePadding.top && tvSafePadding.top > 0)
    {
        unobstructedScrollViewerGlobalBounds->Y += tvSafePadding.top;
        unobstructedScrollViewerGlobalBounds->Height -= tvSafePadding.top;
    }

    if (unobstructedScrollViewerGlobalBounds->Height > tvSafePadding.bottom && tvSafePadding.bottom > 0)
    {
        unobstructedScrollViewerGlobalBounds->Height -= tvSafePadding.bottom;
    }

    return S_OK;
}

_Check_return_ HRESULT ScrollViewer::GetGamepadNavigationCandidate(
    _In_ xaml_input::FocusNavigationDirection direction,
    _In_ bool isPageNavigation,
    _In_ int numPagesLookAhead,
    _In_ float verticalViewportPadding,
    _Outptr_ IInspectable** ppCandidate)
{
    ctl::ComPtr<xaml_input::IFocusManagerStaticsPrivate> spFocusManager;
    ctl::ComPtr<IInspectable> spScrollContentPresenterAsI;
    ctl::ComPtr<IInspectable> spCandidate;
    IFCPTR_RETURN(ppCandidate);
    IFC_RETURN(m_trElementScrollContentPresenter.As(&spScrollContentPresenterAsI));

    // get the next candidate from auto focus scoping candidates to just descendants of the ScrollViewer
    IFC_RETURN(ctl::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Input_FocusManager).Get(),
        &spFocusManager));

    if (isPageNavigation)
    {
        xaml_input::FocusNavigationDirection oppositeDirection;
        wf::Rect unobstructedViewportGlobalBounds = { 0.0f, 0.0f, 0.0f, 0.0f };

        IFC_RETURN(GetUnobstructedScrollViewerGlobalBounds(&unobstructedViewportGlobalBounds));
        const double viewportWidth = unobstructedViewportGlobalBounds.Width;
        const float scale = RootScale::GetRasterizationScaleForElement(GetHandle());
        const double viewportHeight = unobstructedViewportGlobalBounds.Height - (verticalViewportPadding * scale);

        wf::Rect hintRect = unobstructedViewportGlobalBounds;
        switch (direction)
        {
        case xaml_input::FocusNavigationDirection::FocusNavigationDirection_Down:
            hintRect.Y += static_cast<float>(viewportHeight * numPagesLookAhead);
            oppositeDirection = xaml_input::FocusNavigationDirection::FocusNavigationDirection_Up;
            break;
        case xaml_input::FocusNavigationDirection::FocusNavigationDirection_Up:
            hintRect.Y -= static_cast<float>(viewportHeight * numPagesLookAhead);
            oppositeDirection = xaml_input::FocusNavigationDirection::FocusNavigationDirection_Down;
            break;
        case xaml_input::FocusNavigationDirection::FocusNavigationDirection_Left:
            hintRect.X -= static_cast<float>(viewportWidth * numPagesLookAhead);
            oppositeDirection = xaml_input::FocusNavigationDirection::FocusNavigationDirection_Right;
            break;
        case xaml_input::FocusNavigationDirection::FocusNavigationDirection_Right:
            hintRect.X += static_cast<float>(viewportWidth * numPagesLookAhead);
            oppositeDirection = xaml_input::FocusNavigationDirection::FocusNavigationDirection_Left;
            break;
        }

        // FindNextFocusWithSearchRootIgnoreEngagementWithHintRect expect rects in Dips.
        DXamlCore::GetCurrent()->PhysicalPixelsToDips(scale, &hintRect, &hintRect);

        // let the exclusion rect be the same as the hint rect
        // that way, we look for an item that is completely inside the region in the direction specified
        // in other words, we will not get items that are intersecting with the hint rect
        const wf::Rect exclusionRect = hintRect;

        IFC_RETURN(spFocusManager->FindNextFocusWithSearchRootIgnoreEngagementWithHintRect(
            oppositeDirection /* focusNavigationDirection */,
            spScrollContentPresenterAsI.Get(), /* searchStart */
            hintRect,
            exclusionRect,
            spCandidate.GetAddressOf()));
    }
    else
    {
        // item navigation - use the currently focused element as the hint (default)
        IFC_RETURN(spFocusManager->FindNextFocusWithSearchRootIgnoreEngagement(direction,
            spScrollContentPresenterAsI.Get(), /* searchRoot */
            spCandidate.GetAddressOf()));
    }

    IFC_RETURN(spCandidate.MoveTo(ppCandidate));

    return S_OK;
}

_Check_return_ HRESULT ScrollViewer::ScrollForFocusNavigation(
    _In_ wsy::VirtualKey key,
    _In_ xaml_input::FocusNavigationDirection direction,
    _In_ double viewportScrollPercent,
    _Out_ BOOLEAN* isHandled)
{
    bool canScroll = false;
    *isHandled = FALSE;
    IFC_RETURN(CanScrollForFocusNavigation(key, direction, &canScroll));
    if (canScroll)
    {
        ctl::ComPtr<IInspectable> spHorizontalOffset;
        ctl::ComPtr<IInspectable> spVerticalOffset;
        ctl::ComPtr<wf::IReference<DOUBLE>> spHorizontalOffsetReference;
        ctl::ComPtr<wf::IReference<DOUBLE>> spVerticalOffsetReference;
        double horizontalOffset;
        double verticalOffset;
        float zoomFactor = -1.0f;
        wf::Rect unobstructedScrollViewerGlobalBounds = { 0.0f, 0.0f, 0.0f, 0.0f };

        IFC_RETURN(GetUnobstructedScrollViewerGlobalBounds(&unobstructedScrollViewerGlobalBounds));
        const float scale = RootScale::GetRasterizationScaleForElement(GetHandle());
        const double viewportWidth = unobstructedScrollViewerGlobalBounds.Width / scale;
        const double viewportHeight = unobstructedScrollViewerGlobalBounds.Height / scale;

        if (IsInManipulation())
        {
            // When there is an ongoing inertia, do not use the current HorizontalOffset & VerticalOffset
            // dependency property values as the starting point, but rather the end-of-inertia transform.
            IFC_RETURN(GetTargetView(&horizontalOffset, &verticalOffset, &zoomFactor));
        }

        if (zoomFactor == -1.0f)
        {
            // No end-of-inertia transform was retrieved. Use the current dependency property values.
            IFC_RETURN(get_HorizontalOffset(&horizontalOffset));
            IFC_RETURN(get_VerticalOffset(&verticalOffset));
            IFC_RETURN(get_ZoomFactor(&zoomFactor));
        }

        // Switch left/right for flow direction
        xaml::FlowDirection flowDirection = xaml::FlowDirection_LeftToRight;
        IFC_RETURN(get_FlowDirection(&flowDirection));
        if (flowDirection == xaml::FlowDirection_RightToLeft)
        {
            if (direction == xaml_input::FocusNavigationDirection::FocusNavigationDirection_Left)
            {
                direction = xaml_input::FocusNavigationDirection::FocusNavigationDirection_Right;
            }
            else if (direction == xaml_input::FocusNavigationDirection::FocusNavigationDirection_Right)
            {
                direction = xaml_input::FocusNavigationDirection::FocusNavigationDirection_Left;
            }
        }

        switch (direction)
        {
        case xaml_input::FocusNavigationDirection::FocusNavigationDirection_Up:
            IFC_RETURN(PropertyValue::CreateFromDouble(verticalOffset - viewportHeight*viewportScrollPercent, &spVerticalOffset));
            IFC_RETURN(spVerticalOffset.As(&spVerticalOffsetReference));
            break;

        case xaml_input::FocusNavigationDirection::FocusNavigationDirection_Down:
            IFC_RETURN(PropertyValue::CreateFromDouble(verticalOffset + viewportHeight*viewportScrollPercent, &spVerticalOffset));
            IFC_RETURN(spVerticalOffset.As(&spVerticalOffsetReference));
            break;

        case xaml_input::FocusNavigationDirection::FocusNavigationDirection_Left:
            IFC_RETURN(PropertyValue::CreateFromDouble(horizontalOffset - viewportWidth*viewportScrollPercent, &spHorizontalOffset));
            IFC_RETURN(spHorizontalOffset.As(&spHorizontalOffsetReference));
            break;

        case xaml_input::FocusNavigationDirection::FocusNavigationDirection_Right:
            IFC_RETURN(PropertyValue::CreateFromDouble(horizontalOffset + viewportWidth*viewportScrollPercent, &spHorizontalOffset));
            IFC_RETURN(spHorizontalOffset.As(&spHorizontalOffsetReference));
            break;
        }

        IFC_RETURN(ChangeViewWithOptionalAnimation(
            spHorizontalOffsetReference.Get(),
            spVerticalOffsetReference.Get(),
            nullptr /* pZoomFactor */,
            FALSE /* disableAnimation */,
            isHandled));
    }

    return S_OK;
}

//
// IScrollAnchorProvider
//

_Check_return_ HRESULT ScrollViewer::get_CurrentAnchorImpl(_Outptr_result_maybenull_ xaml::IUIElement** value)
{
    bool isAnchoringElementHorizontally = false;
    bool isAnchoringElementVertically = false;

    IFC_RETURN(IsAnchoring(&isAnchoringElementHorizontally, &isAnchoringElementVertically));

    if (isAnchoringElementHorizontally || isAnchoringElementVertically)
    {
        float viewportX = 0.0f;
        IFC_RETURN(get_ZoomedHorizontalOffsetWithPendingShifts(&viewportX));
        float viewportY = 0.0f;
        IFC_RETURN(get_ZoomedVerticalOffsetWithPendingShifts(&viewportY));
        DOUBLE viewportWidth = 0.0f;
        IFC_RETURN(get_ViewportWidth(&viewportWidth));
        DOUBLE viewportHeight = 0.0f;
        IFC_RETURN(get_ViewportHeight(&viewportHeight));
        XRECTF preArrangeViewport = { viewportX, viewportY, static_cast<float>(viewportWidth), static_cast<float>(viewportHeight) };
        IFC_RETURN(EnsureAnchorElementSelection(preArrangeViewport));
    }

    m_anchorElement.CopyTo(value);

    ANCHORING_DEBUG_TRACE(L"SV[0x%p]: get_AnchorElementImpl 0x%p", this, *value);

    return S_OK;
}

_Check_return_ HRESULT ScrollViewer::RegisterAnchorCandidateImpl(_In_ xaml::IUIElement* element)
{
    if (!element)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    IFC_RETURN(EnsureAnchorCandidateVector());

    ctl::ComPtr<TrackerCollection<xaml::UIElement*>> anchorCandidates = m_anchorCandidates.Cast<TrackerCollection<xaml::UIElement*>>();

#ifdef _DEBUG
    // We should not be registering the same element twice. Even though it is functionally ok,
    // we will end up spending more time during arrange than we must.
    // However checking if an element is already in the list every time a new element is registered is worse for perf.
    // So, I'm leaving an assert here to catch regression in our code but in release builds we run without the check.
    UINT count = 0;
    IFC_RETURN(anchorCandidates->get_Size(&count));
    bool found = false;
    for (UINT i = 0; i < count; i++)
    {
        ctl::ComPtr<xaml::IUIElement> candidate;
        IFC_RETURN(anchorCandidates->GetAt(i, &candidate));
        if (candidate.Get() == element)
        {
            found = true;
            break;
        }
    }

    ASSERT(!found);
#endif // _DEBUG

    anchorCandidates->Append(element);
    m_isAnchorElementDirty = true;

    return S_OK;
}

_Check_return_ HRESULT ScrollViewer::UnregisterAnchorCandidateImpl(_In_ xaml::IUIElement* element)
{
    ANCHORING_DEBUG_TRACE(L"SV[0x%p]: UnregisterAnchorCandidateImpl 0x%p", this, element);
    if (!element)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    IFC_RETURN(EnsureAnchorCandidateVector());

    ctl::ComPtr<TrackerCollection<xaml::UIElement*>> anchorCandidates = m_anchorCandidates.Cast<TrackerCollection<xaml::UIElement*>>();
    UINT count = 0;
    IFC_RETURN(anchorCandidates->get_Size(&count));
    for (UINT i = 0; i < count; i++)
    {
        ctl::ComPtr<xaml::IUIElement> candidate;
        IFC_RETURN(anchorCandidates->GetAt(i, &candidate));
        if (candidate.Get() == element)
        {
            anchorCandidates->RemoveAt(i);
            m_isAnchorElementDirty = true;
            break;
        }
    }

    return S_OK;
}

_Check_return_ HRESULT ScrollViewer::OnScrollContentPresenterMeasured()
{
    bool isAnchoringElementHorizontally = false;
    bool isAnchoringElementVertically = false;
    bool isAnchoringFarEdgeHorizontally = false;
    bool isAnchoringFarEdgeVertically = false;

    IFC_RETURN(IsAnchoring(&isAnchoringElementHorizontally, &isAnchoringElementVertically, &isAnchoringFarEdgeHorizontally, &isAnchoringFarEdgeVertically));

    ASSERT(!(isAnchoringElementHorizontally && isAnchoringFarEdgeHorizontally));
    ASSERT(!(isAnchoringElementVertically && isAnchoringFarEdgeVertically));

    bool isAnchoring = isAnchoringElementHorizontally ||
        isAnchoringElementVertically ||
        isAnchoringFarEdgeHorizontally ||
        isAnchoringFarEdgeVertically;

    if (isAnchoring)
    {
        // We are currently anchoring and the child content presenter got measured potentially causing items to shift around.
        // Invalidate arrange so that we get a chance to do the anchoring as necessary.
        IFC_RETURN(InvalidateArrange());
    }

    return S_OK;
}

_Check_return_ HRESULT ScrollViewer::ProcessGamepadNavigation(
    _In_ wsy::VirtualKey key,
    _In_ wsy::VirtualKey originalKey,
    _Out_ BOOLEAN& isHandled)
{
    isHandled = FALSE;
    xaml_input::FocusNavigationDirection direction = xaml_input::FocusNavigationDirection::FocusNavigationDirection_None;
    bool isPageNavigation = XboxUtility::IsGamepadPageNavigationDirection(originalKey);
    const double viewportLookaheadPercent = 1.0f; // 100%  extend by one full viewport
    double viewportScrollPercent = 0.0;
    bool shouldFocusCandidate = false;
    bool shouldProcessNavigationKey = true;

    if (isPageNavigation)
    {
        // page navigation
        direction = XboxUtility::GetPageNavigationDirection(originalKey);
        viewportScrollPercent = 1.0f; // scroll by one viewport

        // only attempt to handle the page navigation key if we can scroll in the direction provided
        IFC_RETURN(CanScrollForFocusNavigation(key, direction, &shouldProcessNavigationKey /*canScroll */));
    }
    else
    {
        // item navigation
        direction = XboxUtility::GetNavigationDirection(originalKey);
        viewportScrollPercent = 0.5f; // scroll by half a viewport
    }

    if (direction != xaml_input::FocusNavigationDirection::FocusNavigationDirection_None &&
        shouldProcessNavigationKey)
    {
        // ask auto focus for next item, scoping for items only under the scrollViewer

        ctl::ComPtr<IInspectable> spCandidate;
        ctl::ComPtr<DependencyObject> spCandidateAsDO;
        double currentDManipHorizontalOffset;
        double currentDManipVerticalOffset;
        float  currentDManipZoomFactor;
        double horizontalOffset;
        double verticalOffset;
        float  zoomFactor = -1.0f;
        bool shouldScroll = false;
        wf::Rect unobstructedScrollViewerGlobalBounds = { 0.0f, 0.0f, 0.0f, 0.0f };

        IFC_RETURN(GetGamepadNavigationCandidate(
            direction,
            isPageNavigation,
            2 /* numPagesLookAhead */,
            0.0 /* verticalViewportPadding */,
            &spCandidate));

        if (IsInManipulation())
        {
            // When there is an ongoing inertia, do not use the current HorizontalOffset & VerticalOffset
            // dependency property values as the starting point, but rather the end-of-inertia transform.
            // Adding 15% of the viewport to the current target results in a smooth curve adjustment and
            // prevents landing on random offsets.
            IFC_RETURN(GetTargetView(&horizontalOffset, &verticalOffset, &zoomFactor));
        }
        if (zoomFactor == -1.0f)
        {
            // No end-of-inertia transform was retrieved. Use the current dependency property values.
            IFC_RETURN(get_HorizontalOffset(&horizontalOffset));
            IFC_RETURN(get_VerticalOffset(&verticalOffset));
            IFC_RETURN(get_ZoomFactor(&zoomFactor));
            currentDManipHorizontalOffset = horizontalOffset;
            currentDManipVerticalOffset = verticalOffset;
        }
        else if (spCandidate)
        {
            // An end-of-inertia transform was retrieved. Access the current DManip transform in order to compute an accurate
            // position of the candidate.
            IFC_RETURN(GetDManipView(&currentDManipHorizontalOffset, &currentDManipVerticalOffset, &currentDManipZoomFactor));
        }

        if (spCandidate)
        {
            // auto focus gave us a candidate.
            ctl::ComPtr<IGeneralTransform> spTransform;
            ctl::ComPtr<IUIElement> spCandidateAsUI;
            ctl::ComPtr<FrameworkElement> spCandidateAsFE;
            wf::Point offsetRelativeToScrollViewer;
            double candidateWidth;
            double candidateHeight;
            XRECTF_RB scrollViewerGlobalBounds = { 0.0f, 0.0f, 0.0f, 0.0f };
            const wf::Point zeroPoint{ 0,0 };
            double viewportWidth;
            double viewportHeight;

            // check if the candidate is within the viewport
            IFC_RETURN(spCandidate.As(&spCandidateAsDO));
            IFC_RETURN(UIElement::GetUIElementFocusCandidate(spCandidate.Get(), &spCandidateAsUI));

            IFC_RETURN(spCandidateAsUI->TransformToVisual(this, &spTransform));
            IFC_RETURN(spTransform->TransformPoint(zeroPoint, &offsetRelativeToScrollViewer));

            IFC_RETURN(spCandidateAsUI.As(&spCandidateAsFE));
            IFC_RETURN(spCandidateAsFE->get_ActualWidth(&candidateWidth));
            IFC_RETURN(spCandidateAsFE->get_ActualHeight(&candidateHeight));
            candidateWidth *= zoomFactor;
            candidateHeight *= zoomFactor;

            IFC_RETURN(get_ViewportWidth(&viewportWidth));
            IFC_RETURN(get_ViewportHeight(&viewportHeight));
            IFC_RETURN(GetUnobstructedScrollViewerGlobalBounds(&unobstructedScrollViewerGlobalBounds));
            const float scale = RootScale::GetRasterizationScaleForElement(GetHandle());
            const double unobstructedViewportWidth = unobstructedScrollViewerGlobalBounds.Width / scale;
            const double unobstructedViewportHeight = unobstructedScrollViewerGlobalBounds.Height / scale;

            // candidate rectangle top left and bottom right points
            // These points are in relation to the top/left corner of the scrollable content,
            // whether the viewport is in inertia or not.
            wf::Point candidateTopLeft{ static_cast<float>(offsetRelativeToScrollViewer.X + currentDManipHorizontalOffset),
                static_cast<float>(offsetRelativeToScrollViewer.Y + currentDManipVerticalOffset) };
            wf::Point candidateBottomRight{ static_cast<float>(candidateTopLeft.X + candidateWidth),
                static_cast<float>(candidateTopLeft.Y + candidateHeight) };

            IFC_RETURN(GetHandle()->GetGlobalBounds(&scrollViewerGlobalBounds, TRUE /* ignoreClipping */));
            wf::Point unobstructedViewportOffsetFromScrollViewer =
            {
                (unobstructedScrollViewerGlobalBounds.X - scrollViewerGlobalBounds.left) / scale,
                (unobstructedScrollViewerGlobalBounds.Y - scrollViewerGlobalBounds.top) / scale
            };

            // viewport top left and bottom right points
            // These are the current viewport points, or the end-of-inertia points if the viewport is in inertia.
            // offset of tvsafe viewport in ScrollViewer = ScrollViewer global bounds - tv safe viewport global bounds
            wf::Point viewportTopLeft =
            {
                static_cast<float>(horizontalOffset),
                static_cast<float>(verticalOffset)
            };

            wf::Point viewportBottomRight{ static_cast<float>(viewportTopLeft.X + viewportWidth),
                static_cast<float>(viewportTopLeft.Y + viewportHeight) };

            // extend viewport by 100% in target direction and check if the candidate falls
            // within the next page.
            switch (direction)
            {
            case xaml_input::FocusNavigationDirection::FocusNavigationDirection_Down:
                viewportTopLeft.Y += unobstructedViewportOffsetFromScrollViewer.Y;
                viewportBottomRight.Y = static_cast<float>(viewportTopLeft.Y + unobstructedViewportHeight);
                viewportBottomRight.Y += static_cast<float>(unobstructedViewportHeight*viewportLookaheadPercent);
                break;
            case xaml_input::FocusNavigationDirection::FocusNavigationDirection_Up:
                viewportTopLeft.Y += unobstructedViewportOffsetFromScrollViewer.Y;
                viewportTopLeft.Y -= static_cast<float>(unobstructedViewportHeight*viewportLookaheadPercent);
                break;
            case xaml_input::FocusNavigationDirection::FocusNavigationDirection_Right:
                viewportTopLeft.X += unobstructedViewportOffsetFromScrollViewer.X;
                viewportBottomRight.X = static_cast<float>(viewportTopLeft.X + unobstructedViewportWidth);
                viewportBottomRight.X += static_cast<float>(viewportTopLeft.X + unobstructedViewportWidth*viewportLookaheadPercent);
                break;
            case xaml_input::FocusNavigationDirection::FocusNavigationDirection_Left:
                viewportTopLeft.X += unobstructedViewportOffsetFromScrollViewer.X;
                viewportTopLeft.X -= static_cast<float>(unobstructedViewportWidth*viewportLookaheadPercent);
                break;
            }

            // does the viewport + extention in the target direction overlap the chosen candidate
            bool candidateOverlapsViewport = !(candidateTopLeft.X > viewportBottomRight.X ||
                viewportTopLeft.X > candidateBottomRight.X ||
                candidateTopLeft.Y > viewportBottomRight.Y ||
                viewportTopLeft.Y > candidateBottomRight.Y);

            if (candidateOverlapsViewport)
            {
                if (isPageNavigation)
                {
                    bool candidateInsideViewport = candidateTopLeft.X >= viewportTopLeft.X &&
                        candidateTopLeft.Y >= viewportTopLeft.Y &&
                        candidateBottomRight.X <= viewportBottomRight.X &&
                        candidateBottomRight.Y <= viewportBottomRight.Y;
                    bool candidateFitsViewportInDirection = false;

                    if (direction == xaml_input::FocusNavigationDirection::FocusNavigationDirection_Down ||
                        direction == xaml_input::FocusNavigationDirection::FocusNavigationDirection_Up)
                    {
                        candidateFitsViewportInDirection = candidateHeight <= unobstructedViewportHeight;
                    }
                    else
                    {
                        candidateFitsViewportInDirection = candidateWidth <= unobstructedViewportWidth;
                    }

                    shouldFocusCandidate = candidateInsideViewport || !candidateFitsViewportInDirection;
                    shouldScroll = true;
                }
                else
                {
                    // Allow focusing partially visible candidates for non-paging directional navigation.
                    shouldFocusCandidate = true;
                }
            }
            else
            {
                // candidate does not overlap the viewport, scroll if possible
                shouldScroll = true;
            }
        }
        else
        {
            // No candidate was found. Scroll if possible
            shouldScroll = true;
        }

        // We need to scroll
        if (shouldScroll)
        {
            IFC_RETURN(ScrollForFocusNavigation(key, direction, viewportScrollPercent, &isHandled));
        }

        if (shouldFocusCandidate)
        {
            // candidate overlaps the ScrollViewer viewport, set focus on the candidate.
            BOOLEAN focusUpdated;
            ctl::ComPtr<DependencyObject> spOldFocusedElement;
            ctl::ComPtr<IInspectable> spOldFocusedElementAsI;

            CFocusManager* focusManager = VisualTree::GetFocusManagerForElement(GetHandle());
            if (CDependencyObject* coreFocusedElement = focusManager->GetFocusedElementNoRef())
            {
                IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(coreFocusedElement, &spOldFocusedElement));
            }

            IFC_RETURN(SetFocusedElementWithDirection(spCandidateAsDO.Get(),
                xaml::FocusState_Keyboard,
                !isPageNavigation /* animateIfBringIntoView */,
                &focusUpdated,
                direction,
                true /* forceBringIntoView */));

            IFC_RETURN(spOldFocusedElement.As(&spOldFocusedElementAsI));

            //Make sure that the candidate for focus was not the item which already had focus before marking the navigation as handled.
            isHandled = isHandled || !(spCandidate == spOldFocusedElementAsI);
        }
    }

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::ProcessPureInertiaInputMessage
//
//  Synopsis:
//      Called when this DM container wants the DM handler to process the current
//      pure inertia input message, by forwarding it to DirectManipulation.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::ProcessPureInertiaInputMessage(
    _In_ ZoomDirection zoomDirection)
{
    HRESULT hr = S_OK;
    BOOLEAN isHandled = FALSE;

    IFC(ProcessPureInertiaInputMessage(zoomDirection, &isHandled));
    m_handleScrollInfoWheelEvent = isHandled;

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::ProcessPureInertiaInputMessage
//
//  Synopsis:
//      Called when this DM container wants the DM handler to process the current
//      pure inertia input message, by forwarding it to DirectManipulation.
//      The handler must set the isHandled flag to True if the message was handled.
//      Unfortunately, callers to this method must determine whether or not DM will treat
//      the current input message as a pure inertia manipulation.
//      PLEASE NOTE: You won't find an input message as a parameter to this function.
//      The implementation just calls ProcessInputMessage on the manipulation handler
//      (in most cases).
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::ProcessPureInertiaInputMessage(
    _In_ ZoomDirection zoomDirection,
    _Out_ BOOLEAN* pIsHandled)
{
    HRESULT hr = S_OK;
    BOOLEAN stopProcessing = FALSE;
    BOOLEAN isHandled = FALSE;

    *pIsHandled = FALSE;

    // Pass the event to DM, except if all these hold:
    // - it's a zoom event, AND
    // - we have zoom enabled, AND
    // - we have zoom chaining enabled, AND
    // - won't result in a zoom change (eg, it's a zoom in event but we're already at maximum zoom).
    // This allows us to implement zoom chaining via our regular routed events. DM doesn't provide
    // for chaining of inertia-only manipulations (as in, anything not related to a touch pointer).
    if (zoomDirection != ZoomDirection_None)
    {
        xaml_controls::ZoomMode zoomMode = xaml_controls::ZoomMode_Disabled;
        BOOLEAN isZoomEnabled = FALSE;

        IFC(get_ZoomMode(&zoomMode));
        isZoomEnabled = zoomMode != xaml_controls::ZoomMode_Disabled;

        if (isZoomEnabled)
        {
            BOOLEAN isZoomChainingEnabled = FALSE;

            IFC(get_IsZoomChainingEnabled(&isZoomChainingEnabled));
            if (isZoomChainingEnabled)
            {
                if (zoomDirection == ZoomDirection_In)
                {
                    IFC(get_IsAtMaxZoom(stopProcessing));
                }
                else if (zoomDirection == ZoomDirection_Out)
                {
                    IFC(get_IsAtMinZoom(stopProcessing));
                }
            }
        }
    }

    if (!stopProcessing)
    {
        IFC(ProcessInputMessage(false /*ignoreFlowDirection*/, isHandled));
        *pIsHandled = isHandled;
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::get_IsAtMaxZoom
//
//  Synopsis:
//    Indicates whether we're at our highest zoom factor
//    (as defined by MaxZoomFactor).
//
//-------------------------------------------------------------------------
//
_Check_return_ HRESULT ScrollViewer::get_IsAtMaxZoom(
    _Out_ BOOLEAN& isAtMaxZoom)
{
    HRESULT hr = S_OK;

    FLOAT maxZoomFactor = 0;
    FLOAT currentZoomFactor = 0;

    isAtMaxZoom = FALSE;

    IFC(get_MaxZoomFactor(&maxZoomFactor));
    IFC(get_ZoomFactor(&currentZoomFactor));

    isAtMaxZoom = DoubleUtil::GreaterThanOrClose(currentZoomFactor, maxZoomFactor);

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::get_IsAtMinZoom
//
//  Synopsis:
//    Indicates whether we're at our lowest zoom factor
//    (as defined by MinZoomFactor).
//
//-------------------------------------------------------------------------
//
_Check_return_ HRESULT ScrollViewer::get_IsAtMinZoom(
    _Out_ BOOLEAN& isAtMinZoom)
{
    HRESULT hr = S_OK;

    FLOAT minZoomFactor = 0;
    FLOAT currentZoomFactor = 0;

    isAtMinZoom = FALSE;

    IFC(get_MinZoomFactor(&minZoomFactor));
    IFC(get_ZoomFactor(&currentZoomFactor));

    isAtMinZoom = DoubleUtil::LessThanOrClose(currentZoomFactor, minZoomFactor);

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::OnSnapPointsChanged
//
//  Synopsis:
//    Called by HorizontalSnapPointsChangedHandler and
//    VerticalSnapPointsChangedHandler when snap points changed
//    or by OnZoomSnapPointsCollectionChanged when the ZoomSnapPoints observable collection changed,
//    or by OnSnapPointsAffectingPropertyChanged when a property affecting snap points changed.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::OnSnapPointsChanged(
    _In_ DMMotionTypes motionType)
{
    HRESULT hr = S_OK;

    if (m_hManipulationHandler)
    {
        ctl::ComPtr<xaml::IUIElement> spContentUIElement;

        IFC(GetContentUIElement(&spContentUIElement));
        if (spContentUIElement)
        {
            // Let the ManipulationHandler know about these snap point changes
            IFC(CoreImports::ManipulationHandler_NotifySnapPointsChanged(
                m_hManipulationHandler,
                static_cast<CUIElement*>(spContentUIElement.Cast<UIElement>()->GetHandle()),
                static_cast<XUINT8>(motionType)));
        }
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::OnSnapPointsAffectingPropertyChanged
//
//  Synopsis:
//    Called when a property that affects the snap points changed
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::OnSnapPointsAffectingPropertyChanged(
    _In_ DMMotionTypes motionType,
    _In_ BOOLEAN updateSnapPointsChangeSubscription)
{
    HRESULT hr = S_OK;

    switch (motionType)
    {
    case DMMotionTypePanX:
        if (m_trScrollSnapPointsInfo &&
            (m_spHorizontalSnapPointsChangedEventHandler || m_isManipulationHandlerInterestedInNotifications || updateSnapPointsChangeSubscription))
        {
            // Refresh the horizontal scroll snap points either because the ScrollViewer is actively listening
            // to IScrollSnapPointsInfo's HorizontalSnapPointsChanged event or because the manipulation handler
            // declared itself as interested in changes.
            IFC(OnSnapPointsChanged(DMMotionTypePanX));
        }
        break;

    case DMMotionTypePanY:
        if (m_trScrollSnapPointsInfo &&
            (m_spVerticalSnapPointsChangedEventHandler || m_isManipulationHandlerInterestedInNotifications || updateSnapPointsChangeSubscription))
        {
            // Refresh the vertical scroll snap points either because the ScrollViewer is actively listening
            // to IScrollSnapPointsInfo's VerticalSnapPointsChanged event or because the manipulation handler
            // declared itself as interested in changes.
            IFC(OnSnapPointsChanged(DMMotionTypePanY));
        }
        break;

    case DMMotionTypeZoom:
        IFC(OnSnapPointsChanged(DMMotionTypeZoom));
        break;
    }

Cleanup:
    RRETURN(hr);
}

// Called when the Content property is changing. The current content is still the old one at this point.
IFACEMETHODIMP ScrollViewer::OnContentChanged(
    _In_ IInspectable* pOldContent,
    _In_ IInspectable* pNewContent)
{
    IFC_RETURN(ScrollViewerGenerated::OnContentChanged(pOldContent, pNewContent));

    if (m_trElementScrollContentPresenter)
    {
        IFC_RETURN(m_trElementScrollContentPresenter.Cast<ScrollContentPresenter>()->OnContentChanging(pOldContent));
    }

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::OnContentPropertyChanged
//
//  Synopsis:
//    Called when the Content property changed.
//    The current content is the new one at this point.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::OnContentPropertyChanged()
{
#ifdef DM_DEBUG
    if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/,
            L"DMSV[0x%p]:  OnContentPropertyChanged.", this));
    }
#endif // DM_DEBUG

    m_isHorizontalStretchAlignmentTreatedAsNear = FALSE;
    m_isVerticalStretchAlignmentTreatedAsNear = FALSE;

    RRETURN(OnManipulatabilityAffectingPropertyChanged(
        NULL  /*pIsInLiveTree*/,
        FALSE /*isCachedPropertyChanged*/,
        TRUE  /*isContentChanged*/,
        FALSE /*isAffectingConfigurations*/,
        FALSE /*isAffectingTouchConfiguration*/));
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::OnManipulatabilityAffectingPropertyChanged
//
//  Synopsis:
//    Called when a characteristic changes that might affect the result
//    of get_CanManipulateElements.
//    isAffectingTouchConfiguration is True when the change might affect
//    the active touch DirectManipulation configuration.
//
//    isCachedPropertyChanged is True when HorizontalScrollMode, VerticalScrollMode,
//    ZoomMode, HorizontalScrollBarVisibility, VerticalScrollBarVisibility,
//    IsScrollInertiaEnabled or IsZoomInertiaEnabled changed.
//    isContentChanged is True when Content changed.
//
//    Note that we don't detect changes in the general transform of the
//    viewport.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::OnManipulatabilityAffectingPropertyChanged(
    _In_opt_ BOOLEAN* pIsInLiveTree,
    _In_ BOOLEAN isCachedPropertyChanged,
    _In_ BOOLEAN isContentChanged,
    _In_ BOOLEAN isAffectingConfigurations,
    _In_ BOOLEAN isAffectingTouchConfiguration)
{
    HRESULT hr = S_OK;
    BOOLEAN canManipulateElementsByTouch = FALSE;
    BOOLEAN canManipulateElementsNonTouch = FALSE;
    BOOLEAN canManipulateElementsWithBringIntoViewport = FALSE;
    BOOLEAN canManipulateElementsWithAsyncBringIntoViewport = FALSE;
    DMConfigurations touchConfiguration = DMConfigurationNone;
    DMConfigurations nonTouchConfiguration = DMConfigurationNone;
    DMConfigurations bringIntoViewportConfiguration = DMConfigurationNone;

#ifdef DM_DEBUG
    if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/,
            L"DMSV[0x%p]:  OnManipulatabilityAffectingPropertyChanged pIsInLiveTree=%d, isCachedPropertyChanged=%d, isContentChanged=%d, isAffectingConfigurations=%d, isAffectingTouchConfiguration=%d.",
            this, pIsInLiveTree ? *pIsInLiveTree : -1, isCachedPropertyChanged, isContentChanged, isAffectingConfigurations, isAffectingTouchConfiguration));
    }
#endif // DM_DEBUG

    if (m_hManipulationHandler)
    {
        IFC(GetManipulationConfigurations(
            pIsInLiveTree,
            !isCachedPropertyChanged /*canUseCachedProperties*/,
            &canManipulateElementsByTouch,
            &canManipulateElementsWithAsyncBringIntoViewport,
            &touchConfiguration,
            &nonTouchConfiguration,
            &bringIntoViewportConfiguration,
            NULL /*pcConfigurations*/,
            NULL /*ppConfigurations*/));
        canManipulateElementsNonTouch = nonTouchConfiguration != DMConfigurationNone;
        canManipulateElementsWithBringIntoViewport = bringIntoViewportConfiguration != DMConfigurationNone;

        // canManipulateElementsWithAsyncBringIntoViewport == True implies canManipulateElementsWithBringIntoViewport == True
        ASSERT(!(canManipulateElementsWithAsyncBringIntoViewport && !canManipulateElementsWithBringIntoViewport));

        // canManipulateElementsByTouch == True implies canManipulateElementsNonTouch == True
        ASSERT(!(canManipulateElementsByTouch && !canManipulateElementsNonTouch));

        // canManipulateElementsByTouch == True implies canManipulateElementsWithBringIntoViewport == True
        ASSERT(!(canManipulateElementsByTouch && !canManipulateElementsWithBringIntoViewport));

        // canManipulateElementsNonTouch == False implies canManipulateElementsByTouch == False
        ASSERT(!(!canManipulateElementsNonTouch && canManipulateElementsByTouch));

        if ((canManipulateElementsByTouch || canManipulateElementsNonTouch || canManipulateElementsWithBringIntoViewport) !=
            (m_canManipulateElementsByTouch || m_canManipulateElementsNonTouch || m_canManipulateElementsWithBringIntoViewport))
        {
            // Global manipulability of the content has changed.
            if (IsInDirectManipulation() && isCachedPropertyChanged)
            {
                // A cached property changed. Delay the notification until the ongoing manipulation completes.
                ASSERT(!canManipulateElementsByTouch);
                ASSERT(!canManipulateElementsNonTouch);
                ASSERT(!canManipulateElementsWithBringIntoViewport);
                m_isCanManipulateElementsInvalid = TRUE;
            }
            else
            {
                m_isCanManipulateElementsInvalid = FALSE;
                if (!canManipulateElementsByTouch)
                {
                    m_touchConfiguration = DMConfigurationNone;
                }
                if (!canManipulateElementsNonTouch)
                {
                    m_nonTouchConfiguration = DMConfigurationNone;
                }
                if (m_canManipulateElementsByTouch || m_canManipulateElementsNonTouch || m_canManipulateElementsWithBringIntoViewport)
                {
                    IFC(NotifyManipulatableElementChanged());
                }

                m_canManipulateElementsWithAsyncBringIntoViewport = canManipulateElementsWithAsyncBringIntoViewport;
                IFC(CoreImports::ManipulationHandler_NotifyCanManipulateElements(
                    m_hManipulationHandler,
                    m_canManipulateElementsByTouch = canManipulateElementsByTouch,
                    m_canManipulateElementsNonTouch = canManipulateElementsNonTouch,
                    m_canManipulateElementsWithBringIntoViewport = canManipulateElementsWithBringIntoViewport));
                if (m_canManipulateElementsByTouch || m_canManipulateElementsNonTouch || m_canManipulateElementsWithBringIntoViewport)
                {
                    IFC(NotifyManipulatableElementChanged());
                    if (isAffectingConfigurations)
                    {
                        IFC(OnViewportConfigurationsAffectingPropertyChanged());
                    }
                }
            }
        }
        else
        {
            // Global manipulability of the content has not changed.
            ASSERT((canManipulateElementsByTouch || canManipulateElementsNonTouch || canManipulateElementsWithBringIntoViewport) ==
                (m_canManipulateElementsByTouch || m_canManipulateElementsNonTouch || m_canManipulateElementsWithBringIntoViewport));

            if (m_canManipulateElementsByTouch != canManipulateElementsByTouch)
            {
                ASSERT(m_canManipulateElementsWithBringIntoViewport == canManipulateElementsWithBringIntoViewport);

                if (IsInDirectManipulation() && isCachedPropertyChanged)
                {
                    // A cached property changed. Delay the notification until the ongoing manipulation completes.
                    m_isCanManipulateElementsInvalid = TRUE;
                }
                else
                {
                    m_isCanManipulateElementsInvalid = FALSE;
                    m_canManipulateElementsWithAsyncBringIntoViewport = canManipulateElementsWithAsyncBringIntoViewport;
                    IFC(CoreImports::ManipulationHandler_NotifyCanManipulateElements(
                        m_hManipulationHandler,
                        m_canManipulateElementsByTouch = canManipulateElementsByTouch,
                        m_canManipulateElementsNonTouch = canManipulateElementsNonTouch,
                        m_canManipulateElementsWithBringIntoViewport));
                }
            }
            else
            {
                // Touch manipulability has not changed.
                ASSERT(m_canManipulateElementsByTouch == canManipulateElementsByTouch);

                // OnViewportConfigurationsAffectingPropertyChanged() is expected to be called further down
                // to push the new non-touch manipulability to the DM handler.
                ASSERT(m_canManipulateElementsNonTouch == canManipulateElementsNonTouch || isAffectingConfigurations);

                m_isCanManipulateElementsInvalid = FALSE;
            }

            if (isContentChanged && (canManipulateElementsByTouch || canManipulateElementsNonTouch || canManipulateElementsWithBringIntoViewport))
            {
                IFC(NotifyManipulatableElementChanged());
            }
            if (isAffectingConfigurations)
            {
                // The manipulability has not changed but the viewport configurations have.
                IFC(OnViewportConfigurationsAffectingPropertyChanged());
            }
            else if (isAffectingTouchConfiguration && m_touchConfiguration != touchConfiguration)
            {
                if (IsInDirectManipulation())
                {
                    IFC(OnViewportAffectingPropertyChanged(
                        FALSE /*boundsChanged*/,
                        TRUE  /*touchConfigurationChanged*/,
                        FALSE /*nonTouchConfigurationChanged*/,
                        FALSE /*configurationsChanged*/,
                        FALSE /*chainedMotionTypesChanged*/,
                        FALSE /*horizontalOverpanModeChanged*/,
                        FALSE /*verticalOverpanModeChanged*/,
                        NULL  /*pAreConfigurationsUpdated*/));
                }
                else if (m_isManipulationHandlerInterestedInNotifications)
                {
                    IFC(OnViewportAffectingPropertyChanged(
                        FALSE /*boundsChanged*/,
                        TRUE  /*touchConfigurationChanged*/,
                        FALSE /*nonTouchConfigurationChanged*/,
                        TRUE  /*configurationsChanged*/,
                        FALSE /*chainedMotionTypesChanged*/,
                        FALSE /*horizontalOverpanModeChanged*/,
                        FALSE /*verticalOverpanModeChanged*/,
                        NULL  /*pAreConfigurationsUpdated*/));
                }
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

// Called when a property that might change the required horizontal or vertical
// DirectManipulation alignment has changed. For instance when a pixel-based
// viewport or extent dimension has changed.
_Check_return_ HRESULT ScrollViewer::OnContentAlignmentAffectingPropertyChanged(
    _In_ BOOLEAN isForHorizontalAlignment,
    _In_ BOOLEAN isForVerticalAlignment)
{
    HRESULT hr = S_OK;
    BOOLEAN isHorizontalAlignmentValid = TRUE;
    BOOLEAN isVerticalAlignmentValid = TRUE;
    DMAlignment alignment;

    if (isForHorizontalAlignment)
    {
        IFC(ComputeHorizontalAlignment(TRUE /*canUseCachedProperties*/, alignment));
        isHorizontalAlignmentValid = (alignment == m_activeHorizontalAlignment);
    }

    if (isForVerticalAlignment)
    {
        IFC(ComputeVerticalAlignment(TRUE /*canUseCachedProperties*/, alignment));
        isVerticalAlignmentValid = (alignment == m_activeVerticalAlignment);
    }

    if (!isHorizontalAlignmentValid || !isVerticalAlignmentValid)
    {
        IFC(OnPrimaryContentAffectingPropertyChanged(
            FALSE /*boundsChanged*/,
            !isHorizontalAlignmentValid /*horizontalAlignmentChanged*/,
            !isVerticalAlignmentValid   /*verticalAlignmentChanged*/,
            FALSE /*zoomFactorBoundaryChanged*/));
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::OnViewportConfigurationsAffectingPropertyChanged
//
//  Synopsis:
//    Called when a characteristic changes that affects the DM viewport configurations.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::OnViewportConfigurationsAffectingPropertyChanged()
{
    HRESULT hr = S_OK;
    BOOLEAN areConfigurationsUpdated = FALSE;
    DMConfigurations touchConfiguration = DMConfigurationNone;
    DMConfigurations nonTouchConfiguration = DMConfigurationNone;
    DMConfigurations bringIntoViewportConfiguration = DMConfigurationNone;

#ifdef DM_DEBUG
    if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/,
            L"DMSV[0x%p]:  OnViewportConfigurationsAffectingPropertyChanged - entry.", this));
    }
#endif // DM_DEBUG

    if (m_hManipulationHandler)
    {
        if (IsInDirectManipulation())
        {
            // Viewport configurations need to be refreshed after the current manipulation completes.
            m_areViewportConfigurationsInvalid = TRUE;
        }
        else if (m_canManipulateElementsByTouch || m_canManipulateElementsNonTouch || m_canManipulateElementsWithBringIntoViewport)
        {
            m_areViewportConfigurationsInvalid = FALSE;
            IFC(GetManipulationConfigurations(
                NULL /*pIsInLiveTree*/,
                TRUE /*canUseCachedProperties*/,
                NULL /*pCanManipulateElementsByTouch*/,
                NULL /*pCanManipulateElementsWithAsyncBringIntoViewport*/,
                &touchConfiguration,
                &nonTouchConfiguration,
                &bringIntoViewportConfiguration,
                NULL /*pcConfigurations*/,
                NULL /*ppConfigurations*/));
            if (touchConfiguration != DMConfigurationNone || nonTouchConfiguration != DMConfigurationNone || bringIntoViewportConfiguration != DMConfigurationNone)
            {
                IFC(OnViewportAffectingPropertyChanged(
                    FALSE /*boundsChanged*/,
                    m_touchConfiguration != touchConfiguration /*touchConfigurationChanged*/,
                    m_nonTouchConfiguration != nonTouchConfiguration /*nonTouchConfigurationChanged*/,
                    TRUE  /*configurationsChanged*/,
                    FALSE /*chainedMotionTypesChanged*/,
                    FALSE /*horizontalOverpanModeChanged*/,
                    FALSE /*verticalOverpanModeChanged*/,
                    &areConfigurationsUpdated));
                if (!areConfigurationsUpdated)
                {
#ifdef DM_DEBUG
                    if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
                    {
                        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/,
                            L"DMSV[0x%p]:  OnViewportConfigurationsAffectingPropertyChanged sets m_areViewportConfigurationsInvalid=True.", this));
                    }
#endif // DM_DEBUG

                    m_areViewportConfigurationsInvalid = TRUE;
                }
            }
        }
    }
Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::OnViewportAffectingPropertyChanged
//
//  Synopsis:
//    Called when a viewport-affecting property changed.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::OnViewportAffectingPropertyChanged(
    _In_ BOOLEAN boundsChanged,
    _In_ BOOLEAN touchConfigurationChanged,
    _In_ BOOLEAN nonTouchConfigurationChanged,
    _In_ BOOLEAN configurationsChanged,
    _In_ BOOLEAN chainedMotionTypesChanged,
    _In_ BOOLEAN horizontalOverpanModeChanged,
    _In_ BOOLEAN verticalOverpanModeChanged,
    _Out_opt_ BOOLEAN* pAreConfigurationsUpdated)
{
    HRESULT hr = S_OK;
    bool fConfigurationsUpdated = false;

    // Configurations are not expected to change during a manipulation
    ASSERT(!(configurationsChanged && IsInDirectManipulation()));

    if (pAreConfigurationsUpdated)
    {
        *pAreConfigurationsUpdated = FALSE;
    }

    // When DManip-on-DComp is turned on, DManip needs to be aware of the latest viewport sizes so that its content can be properly aligned based on
    // the current alignments, even outside a manipulation.
    if (m_hManipulationHandler &&
        (m_isManipulationHandlerInterestedInNotifications || IsInManipulation() || (configurationsChanged && m_trManipulatableElement) ||
         (boundsChanged && m_trManipulatableElement)))
    {
        ctl::ComPtr<xaml::IUIElement> spContentUIElement;

        IFC(GetContentUIElement(&spContentUIElement));
        if (spContentUIElement)
        {
            IFC(CoreImports::ManipulationHandler_NotifyViewportChanged(
                m_hManipulationHandler,
                static_cast<CUIElement*>(spContentUIElement.Cast<UIElement>()->GetHandle()),
                static_cast<bool>(IsInManipulation()),
                static_cast<bool>(boundsChanged),
                static_cast<bool>(touchConfigurationChanged),
                static_cast<bool>(nonTouchConfigurationChanged),
                static_cast<bool>(configurationsChanged),
                static_cast<bool>(chainedMotionTypesChanged),
                static_cast<bool>(horizontalOverpanModeChanged),
                static_cast<bool>(verticalOverpanModeChanged),
                &fConfigurationsUpdated));
            if (pAreConfigurationsUpdated)
            {
                *pAreConfigurationsUpdated = static_cast<BOOLEAN>(fConfigurationsUpdated);
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::OnReduceViewportForCoreInputViewOcclusionsChanged
//
//  Synopsis:
//    Called when a property that changes responsiveness to occlusions changes.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::OnReduceViewportForCoreInputViewOcclusionsChanged()
{

    // If we're in the context of XAML islands, then we don't want to use GetForCurrentView -
    // that requires CoreWindow or application view, which is not supported in islands. Hence we prefer to no-op and return early.
    if(GetHandle()->GetContext()->GetInitializationType() != InitializationType::IslandsOnly)
    {
        return S_OK;
    }

    BOOLEAN reduceViewportForCoreInputViewOcclusions = FALSE;
    wrl::ComPtr<wuv::Core::ICoreInputView> spCoreInputView;

    IFCFAILFAST(get_ReduceViewportForCoreInputViewOcclusions(&reduceViewportForCoreInputViewOcclusions));

    bool hookOcclusionsChanged = reduceViewportForCoreInputViewOcclusions && (m_coreInputViewOcclusionsChangedToken.value == 0);
    bool unhookOcclusionsChanged = !reduceViewportForCoreInputViewOcclusions && (m_coreInputViewOcclusionsChangedToken.value != 0);
    
    if (hookOcclusionsChanged || unhookOcclusionsChanged)
    {
        wrl::ComPtr<wuv::Core::ICoreInputViewStatics> spCoreInputViewStatics;

        IFC_RETURN(wf::GetActivationFactory(
            wrl_wrappers::HStringReference(RuntimeClass_Windows_UI_ViewManagement_Core_CoreInputView).Get(),
            &spCoreInputViewStatics));
        IFC_RETURN(spCoreInputViewStatics->GetForCurrentView(&spCoreInputView));
    }

    if (hookOcclusionsChanged)
    {
        auto coreInputViewOcclusionsChangedCallback = wrl::Callback<wf::ITypedEventHandler<wuv::Core::CoreInputView*, wuv::Core::CoreInputViewOcclusionsChangedEventArgs*>>
            ([this](_In_ wuv::Core::ICoreInputView* pContext, _In_ wuv::Core::ICoreInputViewOcclusionsChangedEventArgs* pArgs)
        {
            IFC_RETURN(ReflowAroundCoreInputViewOcclusions());
            return S_OK;
        });

        IFC_RETURN(spCoreInputView->add_OcclusionsChanged(
            coreInputViewOcclusionsChangedCallback.Get(),
            &m_coreInputViewOcclusionsChangedToken));
    }
    else if (unhookOcclusionsChanged)
    {
        IFC_RETURN(spCoreInputView->remove_OcclusionsChanged(m_coreInputViewOcclusionsChangedToken));
        ZeroMemory(&m_coreInputViewOcclusionsChangedToken, sizeof(m_coreInputViewOcclusionsChangedToken));
    }

    return S_OK;
}

// Called when the ScrollViewer.CanContentRenderOutsideBounds property changed.
// TODO - Bug 17637741 - Consider removing method.
_Check_return_ HRESULT ScrollViewer::OnCanContentRenderOutsideBoundsChanged(
    _In_ IInspectable* newValue)
{
    if (m_trElementScrollContentPresenter)
    {
        BOOLEAN canContentRenderOutsideBounds = FALSE;

        IFCPTR_RETURN(newValue);
        IFC_RETURN(ctl::do_get_value(canContentRenderOutsideBounds, newValue));
        IFC_RETURN(m_trElementScrollContentPresenter.Cast<ScrollContentPresenter>()->put_CanContentRenderOutsideBounds(canContentRenderOutsideBounds));
    }

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::ReflowAroundCoreInputViewOcclusions
//
//  Synopsis:
//    Changes the bottom margin of the ScrollViewer's child to reflow the
//    content around docked CoreInputView occlusions such as the software
//    keyboard. Note: Bug #14087045 currently prevents us from reflowing
//    properly when the window is moved or resized because CoreInputView
//    is not re-firing the OcclusionsChanged event.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::ReflowAroundCoreInputViewOcclusions()
{
    bool isOccluded = false;
    xaml::Thickness margin = {};

    if (m_trElementRoot)
    {
        BOOLEAN focusedElementIsTextEditableAndDescendant = FALSE;
        const CFocusManager* const focusManager = VisualTree::GetFocusManagerForElement(GetHandle());
        const CDependencyObject* const pFocusedElement = focusManager ? focusManager->GetFocusedElementNoRef() : nullptr;
        ctl::ComPtr<IDependencyObject> spFocusedElement = pFocusedElement ? pFocusedElement->GetDXamlPeer() : nullptr;

        // Verify that the focused element is a text-editable control and that
        // it is also a descendant of this ScrollViewer.
        if (pFocusedElement && CInputServices::IsTextEditableControl(pFocusedElement))
        {
            IFC_RETURN(IsAncestorOf(spFocusedElement.Cast<DependencyObject>(), &focusedElementIsTextEditableAndDescendant));
        }

        if (focusedElementIsTextEditableAndDescendant)
        {
            wf::Rect finalRect = DirectUI::RectUtil::CreateEmptyRect();

            // If we're in the context of XAML islands, then we don't want to use GetForCurrentView -
            // that requires CoreWindow or application view, which is not supported in islands.
            if (GetHandle()->GetContext()->GetInitializationType() != InitializationType::IslandsOnly)
            {
                // Get the current list of CoreInputView occlusions.
                wrl::ComPtr<wuv::Core::ICoreInputViewStatics> spCoreInputViewStatics;
                wrl::ComPtr<wuv::Core::ICoreInputView> spCoreInputView;
                wrl::ComPtr<wfc::IVectorView<wuv::Core::CoreInputViewOcclusion*>> spOcclusions;
                UINT occlusionCount = 0;

                IFC_RETURN(wf::GetActivationFactory(
                    wrl_wrappers::HStringReference(RuntimeClass_Windows_UI_ViewManagement_Core_CoreInputView).Get(),
                    &spCoreInputViewStatics));
                IFC_RETURN(spCoreInputViewStatics->GetForCurrentView(&spCoreInputView));
                IFC_RETURN(spCoreInputView->GetCoreInputViewOcclusions(&spOcclusions));

                if (spOcclusions)
                {
                    IFC_RETURN(spOcclusions->get_Size(&occlusionCount));

                    if (occlusionCount > 0)
                    {
                        ctl::ComPtr<IUIElement> spFocusedElementAsIUIElement;
                        ctl::ComPtr<IFrameworkElement> spFocusedElementAsIFrameworkElement;
                        ctl::ComPtr<xaml_media::IGeneralTransform> spFocusedElementTransform;
                        wf::Rect focusedElementRect = {};
                        double focusedElementActualWidth;
                        double focusedElementActualHeight;

                        // Get the global bounds of the focused element, so that we
                        // can later compare them to the bounds of the occlusions.
                        IFC_RETURN(spFocusedElement.As(&spFocusedElementAsIUIElement));
                        IFC_RETURN(spFocusedElement.As(&spFocusedElementAsIFrameworkElement));
                        IFC_RETURN(spFocusedElementAsIFrameworkElement->get_ActualWidth(&focusedElementActualWidth));
                        IFC_RETURN(spFocusedElementAsIFrameworkElement->get_ActualHeight(&focusedElementActualHeight));
                        focusedElementRect.Width = static_cast<float>(focusedElementActualWidth);
                        focusedElementRect.Height = static_cast<float>(focusedElementActualHeight);
                        IFC_RETURN(spFocusedElementAsIUIElement->TransformToVisual(nullptr, &spFocusedElementTransform));
                        IFC_RETURN(spFocusedElementTransform->TransformBounds(focusedElementRect, &focusedElementRect));

                        for (UINT32 i = 0; i < occlusionCount; i++)
                        {
                            wuv::Core::ICoreInputViewOcclusion* currentOcclusion = nullptr;
                            wuv::Core::CoreInputViewOcclusionKind currentOcclusionKind;

                            IFC_RETURN(spOcclusions->GetAt(i, &currentOcclusion));
                            IFC_RETURN(currentOcclusion->get_OcclusionKind(&currentOcclusionKind));

                            // We should only reflow around docked occlusions, such
                            // as the software keyboard, so we ignore everything else.
                            if (currentOcclusionKind == wuv::Core::CoreInputViewOcclusionKind::CoreInputViewOcclusionKind_Docked)
                            {
                                wf::Rect occludedElementRect = focusedElementRect;
                                wf::Rect currentOccludingRect = DirectUI::RectUtil::CreateEmptyRect();

                                IFC_RETURN(currentOcclusion->get_OccludingRect(&currentOccludingRect));
                                IFC_RETURN(DirectUI::RectUtil::Intersect(occludedElementRect, currentOccludingRect));

                                // If the focused element is occluded by this docked occlusion,
                                // do a union of the current occluding rect and the occluding rects
                                // of all the other intersecting docked occlusions.
                                if (!DirectUI::RectUtil::GetIsEmpty(occludedElementRect))
                                {
                                    IFC_RETURN(DirectUI::RectUtil::Union(finalRect, currentOccludingRect));
                                }
                            }
                        }
                    }
                }
            }

            // If the focused element is occluded, we will reflow by setting
            // a bottom Margin on the root of the ScrollViewer equal to the
            // height of the intersection between the ScrollViewer and the
            // occluding rect.
            if (!DirectUI::RectUtil::GetIsEmpty(finalRect))
            {
                ctl::ComPtr<xaml_media::IGeneralTransform> spScrollViewerTransform;
                wf::Rect scrollViewerRect = {};
                double scrollViewerActualWidth;
                double scrollViewerActualHeight;

                IFC_RETURN(get_ActualWidth(&scrollViewerActualWidth));
                IFC_RETURN(get_ActualHeight(&scrollViewerActualHeight));
                scrollViewerRect.Width = static_cast<float>(scrollViewerActualWidth);
                scrollViewerRect.Height = static_cast<float>(scrollViewerActualHeight);
                IFC_RETURN(TransformToVisual(nullptr, &spScrollViewerTransform));
                IFC_RETURN(spScrollViewerTransform->TransformBounds(scrollViewerRect, &scrollViewerRect));
                IFC_RETURN(DirectUI::RectUtil::Intersect(finalRect, scrollViewerRect));

                // We want to make sure that the ScrollViewer retains an
                // appropriate size in order to avoid compromising the
                // user experience. If reflowing would shrink the ScrollViewer
                // past this threshold, we would rather not do it and fallback
                // to the RootScrollViewer scrolling the root visual as a whole.
                if (scrollViewerRect.Height - finalRect.Height >= ScrollViewerMinHeightToReflowAroundOcclusions)
                {
                    IFC_RETURN(m_trElementRoot->get_Margin(&margin));
                    margin.Bottom = finalRect.Height;
                    isOccluded = true;
                }
            }
        }
    }

    // If we have an active Storyboard to reflow around occlusions, stop it.
    if (m_trLayoutAdjustmentsForOcclusionsStoryboard)
    {
        IFC_RETURN(m_trLayoutAdjustmentsForOcclusionsStoryboard->Stop());
        m_trLayoutAdjustmentsForOcclusionsStoryboard.Clear();
    }

    // If we have concluded that we must, indeed, reflow, we will do so by
    // building a Storyboard to animate the bottom Margin of the root. This way
    // we keep the original user value intact.
    if (isOccluded)
    {
        ctl::ComPtr<xaml_animation::IStoryboard> storyboard;
        ctl::ComPtr<Storyboard> storyboardLocal;
        IFC_RETURN(ctl::make(&storyboardLocal));

        ctl::ComPtr<wfc::IVector<xaml_animation::Timeline*>> storyboardChildren;
        IFC_RETURN(storyboardLocal->get_Children(&storyboardChildren));
        ctl::ComPtr<ObjectAnimationUsingKeyFrames> objectAnimation;
        IFC_RETURN(ctl::make(&objectAnimation));

        IFC_RETURN(CoreImports::Storyboard_SetTarget(static_cast<CTimeline*>(objectAnimation->GetHandle()), m_trElementRoot.Cast<FrameworkElement>()->GetHandle()));
        IFC_RETURN(StoryboardFactory::SetTargetPropertyStatic(objectAnimation.Get(), wrl_wrappers::HStringReference(L"Margin").Get()));

        ctl::ComPtr<wfc::IVector<xaml_animation::ObjectKeyFrame*>> objectKeyFrames;
        IFC_RETURN(objectAnimation->get_KeyFrames(&objectKeyFrames));

        ctl::ComPtr<DiscreteObjectKeyFrame> discreteObjectKeyFrame;
        IFC_RETURN(ctl::make(&discreteObjectKeyFrame));

        xaml_animation::KeyTime keyTime = {};
        keyTime.TimeSpan.Duration = 0;

        ctl::ComPtr<IInspectable> value;
        IFC_RETURN(IValueBoxer::BoxValue(&value, margin));

        IFC_RETURN(discreteObjectKeyFrame->put_KeyTime(keyTime));
        IFC_RETURN(discreteObjectKeyFrame->put_Value(value.Get()));

        IFC_RETURN(objectKeyFrames->Append(discreteObjectKeyFrame.Cast<DiscreteObjectKeyFrame>()));
        IFC_RETURN(storyboardChildren->Append(objectAnimation.Cast<ObjectAnimationUsingKeyFrames>()));

        storyboard = storyboardLocal.Detach();
        IFC_RETURN(storyboard->Begin());
        IFC_RETURN(storyboard->SkipToFill());
        IFC_RETURN(SetPtrValueWithQI(m_trLayoutAdjustmentsForOcclusionsStoryboard, storyboard.Get()));
    }

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::OnPrimaryContentAffectingPropertyChanged
//
//  Synopsis:
//    Called when a primary content-affecting property changed.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::OnPrimaryContentAffectingPropertyChanged(
    _In_ BOOLEAN boundsChanged,
    _In_ BOOLEAN horizontalAlignmentChanged,
    _In_ BOOLEAN verticalAlignmentChanged,
    _In_ BOOLEAN zoomFactorBoundaryChanged)
{
    if (!IsInManipulation() && (horizontalAlignmentChanged || verticalAlignmentChanged))
    {
        // Viewport configurations are dependent on the content alignment because of
        // WPB bugs 261102 & 342668. Thus when a content alignment changes outside of
        // a manipulation, the viewport configurations may change.
        IFC_RETURN(OnViewportConfigurationsAffectingPropertyChanged());
    }

    RRETURN(OnPrimaryContentChanged(
        FALSE /*layoutRefreshed*/,
        boundsChanged,
        horizontalAlignmentChanged,
        verticalAlignmentChanged,
        zoomFactorBoundaryChanged));
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::OnPrimaryContentChanged
//
//  Synopsis:
//    Called when a primary content characteristic has changed.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::OnPrimaryContentChanged(
    _In_ BOOLEAN layoutRefreshed,
    _In_ BOOLEAN boundsChanged,
    _In_ BOOLEAN horizontalAlignmentChanged,
    _In_ BOOLEAN verticalAlignmentChanged,
    _In_ BOOLEAN zoomFactorBoundaryChanged)
{
    HRESULT hr = S_OK;

    // When DManip-on-DComp is turned on, DManip needs to be aware of the latest content sizes so that the content can be properly aligned based on
    // the current alignments, even outside a manipulation.
    if (m_hManipulationHandler &&
        (m_isManipulationHandlerInterestedInNotifications ||
         IsInManipulation() ||
         (layoutRefreshed && m_trManipulatableElement) ||
         ((boundsChanged || horizontalAlignmentChanged || verticalAlignmentChanged) && m_trManipulatableElement)))
    {
        ctl::ComPtr<xaml::IUIElement> spContentUIElement;

        IFC(GetContentUIElement(&spContentUIElement));
        if (spContentUIElement)
        {
            IFC(CoreImports::ManipulationHandler_NotifyPrimaryContentChanged(
                m_hManipulationHandler,
                static_cast<CUIElement*>(spContentUIElement.Cast<UIElement>()->GetHandle()),
                static_cast<bool>(IsInManipulation()),
                static_cast<bool>(layoutRefreshed),
                static_cast<bool>(boundsChanged),
                static_cast<bool>(horizontalAlignmentChanged),
                static_cast<bool>(verticalAlignmentChanged),
                static_cast<bool>(zoomFactorBoundaryChanged)));
        }
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::AreExtentChangesExpected
//
//  Synopsis:
//    Determines if a change in the content's extents, if any, is expected
//    based on DirectManipulation feedback.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::AreExtentChangesExpected(
    _Out_ BOOLEAN& areExtentChangesExpected)
{
    HRESULT hr = S_OK;
    BOOLEAN isExtentChangeExpected = FALSE;

    areExtentChangesExpected = FALSE;

    IFC(IsExtentXChangeExpected(isExtentChangeExpected));
    if (!isExtentChangeExpected)
    {
        goto Cleanup;
    }

    IFC(IsExtentYChangeExpected(isExtentChangeExpected));
    if (!isExtentChangeExpected)
    {
        goto Cleanup;
    }

    areExtentChangesExpected = TRUE;

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::IsExtentXChangeExpected
//
//  Synopsis:
//    Determines if a change in the content's width is expected based on
//    DirectManipulation feedback.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::IsExtentXChangeExpected(
    _Out_ BOOLEAN& isExtentXChangeExpected)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IManipulationDataProvider> spProvider;
    DOUBLE extent = 0;

    isExtentXChangeExpected = FALSE;

    if (!IsInDirectManipulation() || m_contentWidthRequested == -1)
    {
#ifdef DM_DEBUG
        if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/,
                L"DMSV[0x%p]:  IsExtentXChangeExpected IsInDirectManipulation=%d m_contentWidthRequested=%f.",
                this, IsInDirectManipulation(), m_contentWidthRequested));
        }
#endif // DM_DEBUG

        goto Cleanup;
    }

    IFC(GetInnerManipulationDataProvider(TRUE /*isForHorizontalOrientation*/, &spProvider));

    IFC(ComputePixelExtentWidth(false /*ignoreZoomFactor*/, spProvider.Get(), &extent));
    if (DoubleUtil::Abs(m_contentWidthRequested - extent) / extent > ScrollViewerZoomExtentRoundingTolerance)
    {
#ifdef DM_DEBUG
        if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/,
                L"DMSV[0x%p]:  IsExtentXChangeExpected extent=%f m_contentWidthRequested=%f.",
                this, extent, m_contentWidthRequested));
        }
#endif // DM_DEBUG

        goto Cleanup;
    }

    isExtentXChangeExpected = TRUE;

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::IsExtentYChangeExpected
//
//  Synopsis:
//    Determines if a change in the content's height is expected based on
//    DirectManipulation feedback.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::IsExtentYChangeExpected(
    _Out_ BOOLEAN& isExtentYChangeExpected)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IManipulationDataProvider> spProvider;
    DOUBLE extent = 0;

    isExtentYChangeExpected = FALSE;

    if (!IsInDirectManipulation() || m_contentHeightRequested == -1)
    {
#ifdef DM_DEBUG
        if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/,
                L"DMSV[0x%p]:  IsExtentYChangeExpected IsInDirectManipulation=%d m_contentHeightRequested=%f.",
                this, IsInDirectManipulation(), m_contentHeightRequested));
        }
#endif // DM_DEBUG

        goto Cleanup;
    }

    IFC(GetInnerManipulationDataProvider(FALSE /*isForHorizontalOrientation*/, &spProvider));

    IFC(ComputePixelExtentHeight(false /*ignoreZoomFactor*/, spProvider.Get(), &extent));
    if (DoubleUtil::Abs(m_contentHeightRequested - extent) / extent > ScrollViewerZoomExtentRoundingTolerance)
    {
#ifdef DM_DEBUG
        if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/,
                L"DMSV[0x%p]:  IsExtentYChangeExpected extent=%f m_contentHeightRequested=%f.",
                this, extent, m_contentHeightRequested));
        }
#endif // DM_DEBUG

        goto Cleanup;
    }

    isExtentYChangeExpected = TRUE;

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::get_CanManipulateElements
//
//  Synopsis:
//    Returns True when the GetManipulationConfigurations method can return
//    an active configuration.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::get_CanManipulateElements(
    _Out_ BOOLEAN* pCanManipulateElementsByTouch,
    _Out_ BOOLEAN* pCanManipulateElementsNonTouch,
    _Out_ BOOLEAN* pCanManipulateElementsWithBringIntoViewport)
{
    HRESULT hr = S_OK;
    BOOLEAN canManipulateElementsWithAsyncBringIntoViewport = FALSE;
    DMConfigurations nonTouchConfiguration = DMConfigurationNone;
    DMConfigurations bringIntoViewportConfiguration = DMConfigurationNone;

    IFCPTR(pCanManipulateElementsByTouch);
    *pCanManipulateElementsByTouch = FALSE;
    IFCPTR(pCanManipulateElementsNonTouch);
    *pCanManipulateElementsNonTouch = FALSE;
    IFCPTR(pCanManipulateElementsWithBringIntoViewport);
    *pCanManipulateElementsWithBringIntoViewport = FALSE;

    IFC(GetManipulationConfigurations(
        NULL /*pIsInLiveTree*/,
        TRUE /*canUseCachedProperties*/,
        pCanManipulateElementsByTouch,
        &canManipulateElementsWithAsyncBringIntoViewport,
        NULL /*pTouchConfiguration*/,
        &nonTouchConfiguration,
        &bringIntoViewportConfiguration,
        NULL /*pcConfigurations*/,
        NULL /*ppConfigurations*/));

    *pCanManipulateElementsNonTouch = (nonTouchConfiguration != DMConfigurationNone);
    *pCanManipulateElementsWithBringIntoViewport = (bringIntoViewportConfiguration != DMConfigurationNone);

    if (m_canManipulateElementsByTouch != *pCanManipulateElementsByTouch ||
        m_canManipulateElementsNonTouch != *pCanManipulateElementsNonTouch ||
        m_canManipulateElementsWithBringIntoViewport != *pCanManipulateElementsWithBringIntoViewport)
    {
        // The ScrollViewer's manipulability changed without the manipulation handler's knowledge.
        IFC(OnManipulatabilityAffectingPropertyChanged(
            NULL /*pIsInLiveTree*/,
            FALSE /*isCachedPropertyChanged*/,
            FALSE /*isContentChanged*/,
            TRUE  /*isAffectingConfigurations*/,
            FALSE /*isAffectingTouchConfiguration*/));
    }

    ASSERT(m_canManipulateElementsByTouch == *pCanManipulateElementsByTouch);
    ASSERT(m_canManipulateElementsNonTouch == *pCanManipulateElementsNonTouch);
    ASSERT(m_canManipulateElementsWithBringIntoViewport == *pCanManipulateElementsWithBringIntoViewport);
    ASSERT(m_canManipulateElementsWithAsyncBringIntoViewport == canManipulateElementsWithAsyncBringIntoViewport);

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::put_ManipulationHandler
//
//  Synopsis:
//    Called to set a manipulation handler used in control-to-InputManager notifications.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::put_ManipulationHandler(
    _In_opt_ HANDLE hManipulationHandler)
{
    HRESULT hr = S_OK;

#ifdef DM_DEBUG
    if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/,
            L"DMSV[0x%p]:  put_ManipulationHandler - setting hManipulationHandler=0x%p.", this, hManipulationHandler));
    }
#endif // DM_DEBUG

    m_hManipulationHandler = hManipulationHandler;
    if (m_hManipulationHandler)
    {
        IFC(OnManipulatabilityAffectingPropertyChanged(
            NULL  /*pIsInLiveTree*/,
            FALSE /*isCachedPropertyChanged*/,
            FALSE /*isContentChanged*/,
            FALSE /*isAffectingConfigurations*/,
            FALSE /*isAffectingTouchConfiguration*/));
        // make sure any changes that were set while the handler was not
        // available are reflected.
        IFC(OnPrimaryContentTransformChanged(TRUE /*translationXChanged*/, TRUE /*translationYChanged*/, TRUE /*zoomFactorChanged*/));
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::NotifyManipulatableElementChanged
//
//  Synopsis:
//    Called when the input manager may need to be told that a
//    manipulatable element has changed.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::NotifyManipulatableElementChanged()
{
    HRESULT hr = S_OK;

    if (m_hManipulationHandler)
    {
        ctl::ComPtr<xaml::IUIElement> spContentUIElement;
        UIElement* pManipulatableElementNoRef = NULL;
        ScrollContentPresenter* pScrollContentPresenter = NULL;

        IFC(GetContentUIElement(&spContentUIElement));
        pManipulatableElementNoRef = spContentUIElement.Cast<UIElement>();

        if (m_trManipulatableElement.Get() != pManipulatableElementNoRef)
        {
            // Refresh the m_trScrollSnapPointsInfo member based on the new content.
            // The snap point event handlers are unhooked. The NotifyManipulatableElementChanged
            // call below will cause the manipulation handler to stop listening to notifications.
            // i.e. m_isManipulationHandlerInterestedInNotifications will be reset to FALSE.
            // Any subsequent manipulation will cause the potentially new snap points to be pushed
            // to DirectManipulation.
            IFC(RefreshScrollSnapPointsInfo());

            if (!m_trManipulatableElement && m_trElementScrollContentPresenter)
            {
                // The old manipulatable element was null. Unparent the potential headers so
                // they get added to the CDMViewport that is about to be created.
                pScrollContentPresenter = m_trElementScrollContentPresenter.Cast<ScrollContentPresenter>();
                if (pScrollContentPresenter->IsTopLeftHeaderChild() ||
                    pScrollContentPresenter->IsTopHeaderChild() ||
                    pScrollContentPresenter->IsLeftHeaderChild())
                {
                    IFC(pScrollContentPresenter->UnparentHeaders());
                    IFC(pScrollContentPresenter->InvalidateMeasure());
                }
            }

            // Let the ManipulationHandler know about this manipulatable element change
            IFC(CoreImports::ManipulationHandler_NotifyManipulatableElementChanged(
                m_hManipulationHandler,
                static_cast<CUIElement*>(m_trManipulatableElement ? m_trManipulatableElement.Cast<UIElement>()->GetHandle() : NULL /*pOldManipulatableElement*/),
                static_cast<CUIElement*>(pManipulatableElementNoRef ? pManipulatableElementNoRef->GetHandle() : NULL /*pNewManipulatableElement*/)));

            SetPtrValue(m_trManipulatableElement, pManipulatableElementNoRef);
        }
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::SetManipulationHandlerWantsNotifications
//
//  Synopsis:
//    Used to tell the container if the manipulation handler wants to be
//    aware of manipulation characteristic changes even though no manipulation
//    is in progress.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::SetManipulationHandlerWantsNotifications(
    _In_ UIElement* pManipulatedElement,
    _In_ BOOLEAN wantsNotifications)
{
    HRESULT hr = S_OK;

    IFCPTR(pManipulatedElement);

    m_isManipulationHandlerInterestedInNotifications = wantsNotifications;
    if (!wantsNotifications && !IsInDirectManipulation())
    {
        IFC(UnhookScrollSnapPointsInfoEvents(TRUE /*isForHorizontalSnapPoints*/));
        IFC(UnhookScrollSnapPointsInfoEvents(FALSE /*isForHorizontalSnapPoints*/));
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::SetPointedElement
//
//  Synopsis:
//    Caches the dependency object that is touched during the initiation of
//    a touch-based manipulation.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::SetPointedElement(
    _In_ DependencyObject* pPointedElement) // Element touched by the user
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IDependencyObject> spPointedElement;

    IFCPTR(pPointedElement);
    IFC(ctl::do_query_interface(spPointedElement, pPointedElement));
    IFC(put_PointedElement(spPointedElement.Get()));

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::GetManipulatedElement
//
//  Synopsis:
//    Returns the manipulated element.
//    Caller does an AddRef on the returned ppManipulatedElement if it
//    consumes it.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::GetManipulatedElement(
    _In_opt_ DependencyObject* pPointedElement, // Element touched by the user
    _In_opt_ UIElement* pChildElement,          // Last element on the parent chain before this ScrollViewer
    _Out_ UIElement** ppManipulatedElement)     // Placeholder for manipulated element
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml::IUIElement> spContentUIElement;

    IFCPTR(ppManipulatedElement);
    *ppManipulatedElement = NULL;

    IFC(GetContentUIElement(&spContentUIElement));
    if (spContentUIElement)
    {
        // BEWARE! This param has odd lifetime management! The CALLER takes a ref!
        *ppManipulatedElement = spContentUIElement.Cast<UIElement>();
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::GetManipulationViewport
//
//  Synopsis:
//    Returns information about a manipulated element's viewport
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::GetManipulationViewport(
    _In_ UIElement* pManipulatedElement,
    _Out_opt_ XRECTF* pBounds,
    _Out_opt_ CMILMatrix* pInputTransform,
    _Out_opt_ DMConfigurations* pTouchConfiguration,
    _Out_opt_ DMConfigurations* pNonTouchConfiguration,
    _Out_opt_ DMConfigurations* pBringIntoViewportConfiguration,
    _Out_opt_ DMOverpanMode* pHorizontalOverpanMode,
    _Out_opt_ DMOverpanMode* pVerticalOverpanMode,
    _Out_opt_ UINT8* pcConfigurations,
    _Outptr_result_buffer_maybenull_(*pcConfigurations) DMConfigurations** ppConfigurations,
    _Out_opt_ DMMotionTypes* pChainedMotionTypes)
{
    HRESULT hr = S_OK;
    DOUBLE viewport = 0;
    BOOLEAN isChainingEnabled = FALSE;
    DMMotionTypes chainedMotionTypes = DMMotionTypeNone;
    xaml_media::Matrix matrix;

    IFCPTR(pManipulatedElement);

    if (pBounds)
    {
        pBounds->X = pBounds->Y = pBounds->Width = pBounds->Height = 0.0f;
    }
    if (pInputTransform)
    {
        pInputTransform->SetToIdentity();
    }
    if (pTouchConfiguration)
    {
        *pTouchConfiguration = DMConfigurationNone;
    }
    if (pNonTouchConfiguration)
    {
        *pNonTouchConfiguration = DMConfigurationNone;
    }
    if (pBringIntoViewportConfiguration)
    {
        *pBringIntoViewportConfiguration = DMConfigurationNone;
    }
    if (pHorizontalOverpanMode)
    {
        IFC(GetEffectiveDMOverpanMode(TRUE /*isForHorizontal*/, pHorizontalOverpanMode))
    }
    if (pVerticalOverpanMode)
    {
        IFC(GetEffectiveDMOverpanMode(FALSE /*isForHorizontal*/, pVerticalOverpanMode))
    }
    if (pcConfigurations)
    {
        *pcConfigurations = 0;
    }
    if (ppConfigurations)
    {
        *ppConfigurations = NULL;
    }
    if (pChainedMotionTypes)
    {
        *pChainedMotionTypes = DMMotionTypeNone;
    }

    if (pBounds)
    {
        IFC(ComputePixelViewportWidth(NULL /*pProvider*/, FALSE /*isProviderSet*/, &viewport));
        pBounds->Width = static_cast<XFLOAT>(viewport);
        IFC(ComputePixelViewportHeight(NULL /*pProvider*/, FALSE /*isProviderSet*/, &viewport));
        pBounds->Height = static_cast<XFLOAT>(viewport);
    }
    if (pInputTransform && m_trElementScrollContentPresenter)
    {
        ctl::ComPtr<xaml_media::IGeneralTransform> spGeneralTransform;
        ctl::ComPtr<xaml_media::IMatrixTransform> spMatrixTransform;

        // m_trElementScrollContentPresenter is expected to be in the tree otherwise there is not manipulatable element.
        ASSERT(m_trElementScrollContentPresenter.Cast<ScrollContentPresenter>()->IsInLiveTree());

        // The input matrix returned is the global affine transform from ScrollContentPresenter to the root.
        IFC(m_trElementScrollContentPresenter.Cast<ScrollContentPresenter>()->TransformToVisual(NULL, &spGeneralTransform));
        spMatrixTransform = spGeneralTransform.AsOrNull<xaml_media::IMatrixTransform>();
        if (spMatrixTransform)
        {
            IFC(spMatrixTransform->get_Matrix(&matrix));
            pInputTransform->SetM11(static_cast<FLOAT>(matrix.M11));
            pInputTransform->SetM12(static_cast<FLOAT>(matrix.M12));
            pInputTransform->SetM21(static_cast<FLOAT>(matrix.M21));
            pInputTransform->SetM22(static_cast<FLOAT>(matrix.M22));
            pInputTransform->SetDx(static_cast<FLOAT>(matrix.OffsetX));
            pInputTransform->SetDy(static_cast<FLOAT>(matrix.OffsetY));
        }
    }
    if (pcConfigurations || ppConfigurations || pTouchConfiguration || pNonTouchConfiguration || pBringIntoViewportConfiguration)
    {
        IFC(GetManipulationConfigurations(
            NULL /*pIsInLiveTree*/,
            TRUE /*canUseCachedProperties*/,
            NULL /*pCanManipulateElementsByTouch*/,
            NULL /*pCanManipulateElementsWithAsyncBringIntoViewport*/,
            pTouchConfiguration,
            pNonTouchConfiguration,
            pBringIntoViewportConfiguration,
            pcConfigurations,
            ppConfigurations));
        if (pTouchConfiguration)
        {
            m_touchConfiguration = *pTouchConfiguration;
        }
        if (pNonTouchConfiguration)
        {
            m_nonTouchConfiguration = *pNonTouchConfiguration;
        }
    }
    if (pChainedMotionTypes)
    {
        IFC(get_IsHorizontalScrollChainingEnabled(&isChainingEnabled));
        if (isChainingEnabled)
        {
            chainedMotionTypes = (DMMotionTypes)DMMotionTypePanX;
        }
        IFC(get_IsVerticalScrollChainingEnabled(&isChainingEnabled));
        if (isChainingEnabled)
        {
            chainedMotionTypes = (DMMotionTypes)(chainedMotionTypes + DMMotionTypePanY);
        }
        IFC(get_IsZoomChainingEnabled(&isChainingEnabled));
        if (isChainingEnabled)
        {
            chainedMotionTypes = (DMMotionTypes)(chainedMotionTypes + DMMotionTypeZoom);
        }
        *pChainedMotionTypes = chainedMotionTypes;
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::GetManipulationPrimaryContent
//
//  Synopsis:
//    Returns information about a manipulated element's primary content
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::GetManipulationPrimaryContent(
    _In_ UIElement* pManipulatedElement,
    _Out_opt_ XSIZEF* pOffsets,
    _Out_opt_ XRECTF* pBounds,
    _Out_opt_ DMAlignment* pHorizontalAligment,
    _Out_opt_ DMAlignment* pVerticalAligment,
    _Out_opt_ FLOAT* pMinZoomFactor,
    _Out_opt_ FLOAT* pMaxZoomFactor,
    _Out_opt_ BOOLEAN* pIsHorizontalStretchAlignmentTreatedAsNear,
    _Out_opt_ BOOLEAN* pIsVerticalStretchAlignmentTreatedAsNear,
    _Out_opt_ BOOLEAN* pIsLayoutRefreshed)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IManipulationDataProvider> spProvider;
    ctl::ComPtr<IUIElement> spScrollInfoAsElement;
    BOOLEAN isLayoutRefreshed = FALSE;
    DOUBLE extentX = 0;
    DOUBLE extentY = 0;
    DOUBLE boundsW = 0;
    DOUBLE boundsH = 0;
    FLOAT zoomFactor = 1.0f;
    XSIZEF offsets = {};
    xaml_controls::ZoomMode zoomMode;

    IFCPTR(pManipulatedElement);

    if (pIsHorizontalStretchAlignmentTreatedAsNear != nullptr)
    {
        *pIsHorizontalStretchAlignmentTreatedAsNear = m_isHorizontalStretchAlignmentTreatedAsNear;
    }
    if (pIsVerticalStretchAlignmentTreatedAsNear != nullptr)
    {
        *pIsVerticalStretchAlignmentTreatedAsNear = m_isVerticalStretchAlignmentTreatedAsNear;
    }
    if (pIsLayoutRefreshed)
    {
        *pIsLayoutRefreshed = FALSE;
    }
    if (pOffsets)
    {
        pOffsets->width = pOffsets->height = 0.0f;
    }
    if (pBounds)
    {
        pBounds->X = pBounds->Y = pBounds->Width = pBounds->Height = 0.0f;
    }
    if (pHorizontalAligment)
    {
        *pHorizontalAligment = DMAlignmentNear;
    }
    if (pVerticalAligment)
    {
        *pVerticalAligment = DMAlignmentNear;
    }
    if (pMinZoomFactor)
    {
        *pMinZoomFactor = 0.0f;
    }
    if (pMaxZoomFactor)
    {
        *pMaxZoomFactor = 0.0f;
    }

    if (pBounds || pHorizontalAligment || pVerticalAligment)
    {
        if ((!m_isInDirectManipulationSync && !m_isInChangeViewBringIntoViewport) || m_isInZoomFactorSync)
        {
            // Make sure the extents are computed based on the latest zoom factor change request, if any.
            // This will cause a ScrollViewer::InvalidateScrollInfo call if extents are out of date.
            // During a synchronization operation though, this update is skipped to avoid nested sync requests,
            // and it is skipped during a ChangeView's BringIntoViewport call to avoid the invalidation of its parameters,
            // unless a ZoomToFactor call is being processed.
            // Any subsequent ScrollViewer::InvalidateScrollInfo call may trigger a new synchronization.
            IFC(GetScrollInfoAsElement(&spScrollInfoAsElement));
            if (spScrollInfoAsElement)
            {
                IFC(spScrollInfoAsElement->UpdateLayout());
            }
        }

        IFC(GetEffectiveZoomMode(TRUE /*canUseCachedProperty*/, zoomMode));

        IFC(get_ZoomFactor(&zoomFactor));

        IFC(GetInnerManipulationDataProvider(TRUE /*isForHorizontalOrientation*/, &spProvider));
        IFC(ComputePixelExtentWidth(false /*ignoreZoomFactor*/, spProvider.Get(), &extentX));

        if (spProvider)
        {
            isLayoutRefreshed = TRUE;
            spProvider = nullptr;
        }
        else
        {
            IFC(GetInnerManipulationDataProvider(FALSE /*isForHorizontalOrientation*/, &spProvider));
        }
        if (spProvider)
        {
            isLayoutRefreshed = TRUE;
        }
        IFC(ComputePixelExtentHeight(false /*ignoreZoomFactor*/, spProvider.Get(), &extentY));

        boundsW = extentX / zoomFactor;
        boundsH = extentY / zoomFactor;
    }
    else if (pIsLayoutRefreshed)
    {
        IFC(GetInnerManipulationDataProvider(TRUE /*isForHorizontalOrientation*/, &spProvider));
        if (spProvider)
        {
            isLayoutRefreshed = TRUE;
        }
        else
        {
            IFC(GetInnerManipulationDataProvider(FALSE /*isForHorizontalOrientation*/, &spProvider));
            if (spProvider)
            {
                isLayoutRefreshed = TRUE;
            }
        }
    }

    if (pHorizontalAligment)
    {
        IFC(ComputeHorizontalAlignment(TRUE /*canUseCachedProperties*/, m_activeHorizontalAlignment));

        // WPB bugs 261102 & 342668 - stop exceptional usage of PanX configuration when horizontal alignment is no longer Center or Far.
        if (m_cForcePanXConfiguration != 0 && (m_activeHorizontalAlignment != DMAlignmentCenter && m_activeHorizontalAlignment != DMAlignmentFar))
        {
            m_cForcePanXConfiguration = 0;
        }

        *pHorizontalAligment = m_activeHorizontalAlignment;
    }

    if (pVerticalAligment)
    {
        IFC(ComputeVerticalAlignment(TRUE /*canUseCachedProperties*/, m_activeVerticalAlignment));

        // WPB bugs 261102 & 342668 - stop exceptional usage of PanY configuration when vertical alignment is no longer Center or Far.
        if (m_cForcePanYConfiguration != 0 && (m_activeVerticalAlignment != DMAlignmentCenter && m_activeVerticalAlignment != DMAlignmentFar))
        {
            m_cForcePanYConfiguration = 0;
        }

        *pVerticalAligment = m_activeVerticalAlignment;
    }

    if (pMinZoomFactor)
    {
        if (m_overridingMinZoomFactor != 0.0f)
        {
            *pMinZoomFactor = m_overridingMinZoomFactor;
        }
        else
        {
            IFC(get_MinZoomFactor(pMinZoomFactor));
        }
    }

    if (pMaxZoomFactor)
    {
        if (m_overridingMaxZoomFactor != 0.0f)
        {
            *pMaxZoomFactor = m_overridingMaxZoomFactor;
        }
        else
        {
            IFC(get_MaxZoomFactor(pMaxZoomFactor));
        }
    }

    if (pIsLayoutRefreshed)
    {
        *pIsLayoutRefreshed = isLayoutRefreshed;
    }

    if (pOffsets)
    {
        IFC(GetHeadersSize(pOffsets));
    }

    if (pBounds)
    {
        pBounds->Width = static_cast<XFLOAT>(boundsW);
        pBounds->Height = static_cast<XFLOAT>(boundsH);
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::GetManipulationSecondaryContent
//
//  Synopsis:
//    Returns information about a secondary content.
//    pOffsets represents the offset of the secondary content based on
//    the presence of other secondary contents.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::GetManipulationSecondaryContent(
    _In_ UIElement* pContentElement,
    _Out_ XSIZEF* pOffsets)
{
    HRESULT hr = S_OK;
    XSIZEF size = {};
    ScrollContentPresenter* pScrollContentPresenter = NULL;
    ctl::ComPtr<IUIElement> spHeader;

    IFCPTR(pContentElement);
    IFCPTR(pOffsets);
    pOffsets->width = pOffsets->height = 0.0f;

#ifdef DM_DEBUG
    if (DMSVv_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE)) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE | DMSVv_DBG) /*traceType*/, L"DMSV[0x%p]:  GetManipulationSecondaryContent - entry. pContentElement=0x%p.",
            this, pContentElement));
    }
#endif // DM_DEBUG

    IFC(GetHeadersSize(&size));

    if (m_trElementScrollContentPresenter)
    {
        pScrollContentPresenter = m_trElementScrollContentPresenter.Cast<ScrollContentPresenter>();

        if (pScrollContentPresenter->IsTopHeaderChild())
        {
            IFC(get_TopHeader(&spHeader));
            ASSERT(spHeader);
            if (spHeader.Cast<UIElement>() == pContentElement)
            {
                pOffsets->width = size.width;
            }
        }

        if (pScrollContentPresenter->IsLeftHeaderChild())
        {
            IFC(get_LeftHeader(&spHeader));
            ASSERT(spHeader);
            if (spHeader.Cast<UIElement>() == pContentElement)
            {
                pOffsets->height = size.height;
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::GetManipulationPrimaryContentTransform
//
//  Synopsis:
//    Called when the input manager needs to setup DM with the current
//    transform for the provided primary content.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::GetManipulationPrimaryContentTransform(
    _In_ UIElement* pManipulatedElement,
    _In_ BOOLEAN inManipulation,
    _In_ BOOLEAN forInitialTransformationAdjustment,
    _In_ BOOLEAN forMargins,
    _Out_opt_ FLOAT* pTranslationX,
    _Out_opt_ FLOAT* pTranslationY,
    _Out_opt_ FLOAT* pZoomFactor)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IManipulationDataProvider> spProviderForHorizontalOrientation;
    ctl::ComPtr<IManipulationDataProvider> spProviderForVerticalOrientation;
    BOOLEAN hasInnerManipulationDataProvider = FALSE;
    FLOAT zoomFactor = 0.0f;
    DOUBLE offset = 0;
    DOUBLE topMargin = 0;
    DOUBLE leftMargin = 0;
    xaml_controls::ZoomMode zoomMode;

#ifdef DM_DEBUG
    if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/,
            L"DMSV[0x%p]:  GetManipulationPrimaryContentTransform - entry. inManipulation=%d, forInitialTransformationAdjustment=%d, forMargins=%d.",
            this, inManipulation, forInitialTransformationAdjustment, forMargins));
    }
#endif // DM_DEBUG

    IFCPTR(pManipulatedElement);

    if (pTranslationX)
    {
        *pTranslationX = 0.0f;
    }
    if (pTranslationY)
    {
        *pTranslationY = 0.0f;
    }
    if (pZoomFactor)
    {
        *pZoomFactor = 1.0f;
    }

    if ((forMargins || !inManipulation) && (pTranslationX || pTranslationY))
    {
        IFC(GetTopLeftMargins(pManipulatedElement, topMargin, leftMargin));
    }

    IFC(get_ZoomFactor(&zoomFactor));

    if (pZoomFactor)
    {
        *pZoomFactor = zoomFactor;
    }

    IFC(get_ZoomMode(&zoomMode));

    if (!forMargins && (pTranslationX || pTranslationY))
    {
        // Figure out if this ScrollViewer holds a IManipulationDataProvider
        IFC(GetInnerManipulationDataProvider(TRUE /*isForHorizontalOrientation*/, &spProviderForHorizontalOrientation));
        IFC(GetInnerManipulationDataProvider(FALSE /*isForHorizontalOrientation*/, &spProviderForVerticalOrientation));
        hasInnerManipulationDataProvider = spProviderForHorizontalOrientation || spProviderForVerticalOrientation;
    }

    if (pTranslationX)
    {
        if (forMargins)
        {
            *pTranslationX = static_cast<FLOAT>(leftMargin);
        }
        else
        {
            if (inManipulation || !hasInnerManipulationDataProvider)
            {
                ASSERT(spProviderForHorizontalOrientation || m_xPixelOffset == m_xOffset);
                offset = m_xPixelOffset;
            }

            IFC(ComputeTranslationXCorrection(
                inManipulation,
                forInitialTransformationAdjustment,
                forInitialTransformationAdjustment /*adjustDimensions*/,
                spProviderForHorizontalOrientation.Get(),
                leftMargin,
                -1.0 /*extent*/,
                zoomFactor,
                pTranslationX));

            if (inManipulation)
            {
                *pTranslationX -= static_cast<FLOAT>(offset);
            }
            else if (forInitialTransformationAdjustment)
            {
                *pTranslationX += static_cast<FLOAT>(offset);
            }
        }
    }

    if (pTranslationY)
    {
        if (forMargins)
        {
            *pTranslationY = static_cast<FLOAT>(topMargin);
        }
        else
        {
            offset = 0;
            if (inManipulation || !hasInnerManipulationDataProvider)
            {
                ASSERT(spProviderForVerticalOrientation || m_yPixelOffset == m_yOffset);
                offset = m_yPixelOffset;
            }

            IFC(ComputeTranslationYCorrection(
                inManipulation,
                forInitialTransformationAdjustment,
                forInitialTransformationAdjustment /*adjustDimensions*/,
                spProviderForVerticalOrientation.Get(),
                topMargin,
                -1.0 /*extent*/,
                zoomFactor,
                pTranslationY));

            if (inManipulation)
            {
                *pTranslationY -= static_cast<FLOAT>(offset);
            }
            else if (forInitialTransformationAdjustment)
            {
                *pTranslationY += static_cast<FLOAT>(offset);
            }
        }
    }

#ifdef DM_DEBUG
    if ((DMSV_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG)) == S_OK) && pTranslationX && pTranslationY && pZoomFactor)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/,
            L"DMSV[0x%p]:  GetManipulationPrimaryContentTransform - returns translationX=%4.6lf, translationY=%4.6lf, zoomFactor=%4.8lf.",
            this, *pTranslationX, *pTranslationY, *pZoomFactor));
    }
#endif // DM_DEBUG

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::GetManipulationSecondaryContentTransform
//
//  Synopsis:
//    Returns information about a secondary content's transform.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::GetManipulationSecondaryContentTransform(
    _In_ UIElement* pContentElement,
    _Out_ FLOAT* pTranslationX,
    _Out_ FLOAT* pTranslationY,
    _Out_ FLOAT* pZoomFactor)
{
    HRESULT hr = S_OK;
    DOUBLE topMargin = 0;
    DOUBLE leftMargin = 0;
    FLOAT zoomFactor = 0.0f;

#ifdef DM_DEBUG
    if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/,
            L"DMSV[0x%p]:  GetManipulationSecondaryContentTransform - entry. pContentElement=0x%p.", this, pContentElement));
    }
#endif // DM_DEBUG

    IFCPTR(pContentElement);
    IFCPTR(pTranslationX)
        *pTranslationX = 0.0f;
    IFCPTR(pTranslationY)
        *pTranslationY = 0.0f;
    IFCPTR(pZoomFactor)
        *pZoomFactor = 1.0f;

    IFC(GetTopLeftMargins(pContentElement, topMargin, leftMargin));

    IFC(get_ZoomFactor(&zoomFactor));

    *pTranslationX = static_cast<FLOAT>(leftMargin)* (zoomFactor - 1.0f);
    *pTranslationY = static_cast<FLOAT>(topMargin)* (zoomFactor - 1.0f);
    *pZoomFactor = zoomFactor;

#ifdef DM_DEBUG
    if ((DMSV_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG)) == S_OK) && pTranslationX && pTranslationY && pZoomFactor)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/,
            L"                   GetManipulationSecondaryContentTransform - returns translationX=%4.6lf, translationY=%4.6lf, zoomFactor=%4.8lf.",
            *pTranslationX, *pTranslationY, *pZoomFactor));
    }
#endif // DM_DEBUG

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::GetManipulationSnapPoints
//
//  Synopsis:
//    Returns the snap points for the provided manipulated element and motion type.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::GetManipulationSnapPoints(
    _In_ UIElement* pManipulatedElement,   // Manipulated element for which the snap points are requested
    _In_ DMMotionTypes motionType,         // Motion type for which the snap points are requested
    _Out_ BOOLEAN* pAreSnapPointsOptional, // Set to True when returned snap points are optional
    _Out_ BOOLEAN* pAreSnapPointsSingle,   // Set to True when returned snap points are single (i.e. breaking inertia)
    _Out_ BOOLEAN* pAreSnapPointsRegular,  // Set to True when returned snap points are equidistant
    _Out_ FLOAT* pRegularOffset,           // Offset of regular snap points
    _Out_ FLOAT* pRegularInterval,         // Interval of regular snap points
    _Out_ UINT32* pcIrregularSnapPoints,   // Number of irregular snap points
    _Outptr_result_buffer_(*pcIrregularSnapPoints) FLOAT** ppIrregularSnapPoints,   // Array of irregular snap points
    _Out_ DMSnapCoordinate* pSnapCoordinate) // coordinate system of the snap points
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IVectorView<FLOAT>> spSnapPointsVector;
    FLOAT zoomFactor = 1.0f;
    xaml_controls::ZoomMode zoomMode = xaml_controls::ZoomMode_Disabled;
    xaml_controls::SnapPointsType snapPointsType = xaml_controls::SnapPointsType_None;

    IFCPTR(pManipulatedElement);
    IFCPTR(pAreSnapPointsOptional);
    *pAreSnapPointsOptional = FALSE;
    IFCPTR(pAreSnapPointsSingle);
    *pAreSnapPointsSingle = FALSE;
    IFCPTR(pAreSnapPointsRegular);
    *pAreSnapPointsRegular = FALSE;
    IFCPTR(pRegularOffset);
    *pRegularOffset = 0.0;
    IFCPTR(pRegularInterval);
    *pRegularInterval = 0.0;
    IFCPTR(pcIrregularSnapPoints);
    *pcIrregularSnapPoints = 0;
    IFCPTR(ppIrregularSnapPoints);
    *ppIrregularSnapPoints = NULL;
    IFCPTR(pSnapCoordinate);
    *pSnapCoordinate = (motionType == DMMotionTypeZoom) ? DMSnapCoordinateOrigin : DMSnapCoordinateBoundary;

    ASSERT(motionType != DMMotionTypeNone);

    if ((motionType == DMMotionTypePanX || motionType == DMMotionTypePanY) && m_trScrollSnapPointsInfo)
    {
        IFC(get_ZoomFactor(&zoomFactor));
        ASSERT(zoomFactor != 0.0f);
        IFC(GetEffectiveZoomMode(TRUE /*canUseCachedProperty*/, zoomMode));

        IFC(GetScrollSnapPoints(
            motionType == DMMotionTypePanX /*isForHorizontalSnapPoints*/,
            zoomFactor,
            1.0f,
            -1 /*extentDim*/,
            TRUE /*updateSnapPointsChangeSubscription*/,
            pAreSnapPointsOptional,
            pAreSnapPointsSingle,
            pAreSnapPointsRegular,
            pRegularOffset,
            pRegularInterval,
            pcIrregularSnapPoints,
            ppIrregularSnapPoints,
            pSnapCoordinate));
    }

    if (motionType == DMMotionTypeZoom && m_spZoomSnapPoints)
    {
        IFC(ScrollViewerGenerated::get_ZoomSnapPointsType(&snapPointsType));

        if (snapPointsType != xaml_controls::SnapPointsType_None)
        {
            spSnapPointsVector = m_spZoomSnapPoints.AsOrNull<wfc::IVectorView<FLOAT>>();
            IFC(CopyMotionSnapPointsFromVector(
                true/*isForZoomSnapPoints*/,
                spSnapPointsVector.Get(),
                xaml_primitives::SnapPointsAlignment_Near,
                0 /*viewportDim*/,
                0 /*extentDim*/,
                1.0f /*zoomFactor*/,
                1.0f /*staticZoomFactor*/,
                pcIrregularSnapPoints,
                ppIrregularSnapPoints));

            *pAreSnapPointsRegular = FALSE;
            *pAreSnapPointsOptional = (snapPointsType == xaml_controls::SnapPointsType_Optional ||
                snapPointsType == xaml_controls::SnapPointsType_OptionalSingle);
            *pAreSnapPointsSingle = (snapPointsType == xaml_controls::SnapPointsType_MandatorySingle ||
                snapPointsType == xaml_controls::SnapPointsType_OptionalSingle);
            // pSnapCoordinate stays what it is.
        }
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::GetScrollSnapPoints
//
//  Synopsis:
//    Returns the scroll snap points and their characteristics for the provided orientation.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::GetScrollSnapPoints(
    _In_ BOOLEAN isForHorizontalSnapPoints,// True to retrieve the horizontal scroll snap points
    _In_ FLOAT zoomFactor,
    _In_ FLOAT staticZoomFactor,
    _In_ DOUBLE extentDim,                 // Set to -1 when the extent was not pre-computed
    _In_ BOOLEAN updateSnapPointsChangeSubscription,
    _Out_ BOOLEAN* pAreSnapPointsOptional, // Set to True when returned snap points are optional
    _Out_ BOOLEAN* pAreSnapPointsSingle,   // Set to True when returned snap points are single (i.e. breaking inertia)
    _Out_ BOOLEAN* pAreSnapPointsRegular,  // Set to True when returned snap points are equidistant
    _Out_ FLOAT* pRegularOffset,           // Offset of regular snap points
    _Out_ FLOAT* pRegularInterval,         // Interval of regular snap points
    _Out_ UINT32* pcIrregularSnapPoints,   // Number of irregular snap points
    _Outptr_result_buffer_(*pcIrregularSnapPoints) FLOAT** ppIrregularSnapPoints, // Array of irregular snap points
    _Out_ DMSnapCoordinate* pSnapCoordinate) // coordinate system of the snap points
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IVectorView<FLOAT>> spSnapPointsVector;
    BOOLEAN areSnapPointsRegular = FALSE;
    FLOAT offset = 0.0f;
    FLOAT interval = 0.0f;
    DOUBLE viewportDim = 0;
    xaml_controls::SnapPointsType snapPointsType = xaml_controls::SnapPointsType_None;
    xaml_primitives::SnapPointsAlignment snapPointsAlignment = xaml_primitives::SnapPointsAlignment_Near;
    BOOLEAN shouldProcessSnapPoint = FALSE;

    IFCPTR(pAreSnapPointsOptional);
    *pAreSnapPointsOptional = FALSE;
    IFCPTR(pAreSnapPointsSingle);
    *pAreSnapPointsSingle = FALSE;
    IFCPTR(pAreSnapPointsRegular);
    *pAreSnapPointsRegular = FALSE;
    IFCPTR(pRegularOffset);
    *pRegularOffset = 0.0;
    IFCPTR(pRegularInterval);
    *pRegularInterval = 0.0;
    IFCPTR(pcIrregularSnapPoints);
    *pcIrregularSnapPoints = 0;
    IFCPTR(ppIrregularSnapPoints);
    *ppIrregularSnapPoints = NULL;
    IFCPTR(pSnapCoordinate);
    *pSnapCoordinate = DMSnapCoordinateBoundary;

    ASSERT(m_trScrollSnapPointsInfo);

    if (isForHorizontalSnapPoints)
    {
        IFC(ScrollViewerGenerated::get_HorizontalSnapPointsType(&snapPointsType));

        if (snapPointsType == xaml_controls::SnapPointsType_None)
        {
            if (updateSnapPointsChangeSubscription)
            {
                IFC(UnhookScrollSnapPointsInfoEvents(TRUE /*isForHorizontalSnapPoints*/));
            }
        }
        else
        {
            IFC(ScrollViewerGenerated::get_HorizontalSnapPointsAlignment(&snapPointsAlignment));
            if (snapPointsAlignment != xaml_primitives::SnapPointsAlignment_Near)
            {
                IFC(ComputePixelViewportWidth(NULL /*pProvider*/, FALSE /*isProviderSet*/, &viewportDim));
            }

            IFC(m_trScrollSnapPointsInfo.Get()->get_AreHorizontalSnapPointsRegular(&areSnapPointsRegular));
            if (areSnapPointsRegular)
            {
                IFC(m_trScrollSnapPointsInfo.Get()->GetRegularSnapPoints(
                    xaml_controls::Orientation_Horizontal,
                    snapPointsAlignment,
                    &offset,
                    &interval));
            }
            else
            {
                if (extentDim < 0)
                {
                    IFC(ComputePixelExtentWidth(&extentDim));
                }
                IFC(m_trScrollSnapPointsInfo.Get()->GetIrregularSnapPoints(
                    xaml_controls::Orientation_Horizontal,
                    snapPointsAlignment,
                    &spSnapPointsVector));
            }
            if (updateSnapPointsChangeSubscription)
            {
                IFC(HookScrollSnapPointsInfoEvents(TRUE /*isForHorizontalSnapPoints*/));
            }
            shouldProcessSnapPoint = TRUE;
        }
    }
    else
    {
        IFC(ScrollViewerGenerated::get_VerticalSnapPointsType(&snapPointsType));

        if (snapPointsType == xaml_controls::SnapPointsType_None)
        {
            IFC(UnhookScrollSnapPointsInfoEvents(FALSE /*isForHorizontalSnapPoints*/));
        }
        else
        {
            IFC(ScrollViewerGenerated::get_VerticalSnapPointsAlignment(&snapPointsAlignment));
            if (snapPointsAlignment != xaml_primitives::SnapPointsAlignment_Near)
            {
                IFC(ComputePixelViewportHeight(NULL /*pProvider*/, FALSE /*isProviderSet*/, &viewportDim));
            }

            IFC(m_trScrollSnapPointsInfo.Get()->get_AreVerticalSnapPointsRegular(&areSnapPointsRegular));
            if (areSnapPointsRegular)
            {
                IFC(m_trScrollSnapPointsInfo.Get()->GetRegularSnapPoints(
                    xaml_controls::Orientation_Vertical,
                    snapPointsAlignment,
                    &offset,
                    &interval));
            }
            else
            {
                if (extentDim < 0)
                {
                    IFC(ComputePixelExtentHeight(&extentDim));
                }
                IFC(m_trScrollSnapPointsInfo.Get()->GetIrregularSnapPoints(
                    xaml_controls::Orientation_Vertical,
                    snapPointsAlignment,
                    &spSnapPointsVector));
            }
            IFC(HookScrollSnapPointsInfoEvents(FALSE /*isForHorizontalSnapPoints*/));

            shouldProcessSnapPoint = TRUE;
        }
    }

    if (shouldProcessSnapPoint)
    {
        if (areSnapPointsRegular)
        {
            switch (snapPointsAlignment)
            {
            case xaml_primitives::SnapPointsAlignment_Near:
                *pRegularOffset = offset * staticZoomFactor;
                // pSnapCoordinate stays what it is.
                break;

            case xaml_primitives::SnapPointsAlignment_Center:
                // When snap points alignment is Center, the snap points need to align
                // with the center of the viewport. Adjust the offset accordingly.
                // Both static and manipulatable zoom factors need to be taken into account.
                if (interval <= 0.0f)
                {
                    // Do not handle negative internal in this case
                    interval = 0.0f;
                    *pRegularOffset = 0.0f;
                }
                else
                {
                    if (viewportDim >= interval * zoomFactor)
                    {
                        offset *= zoomFactor;
                        offset -= static_cast<FLOAT>(viewportDim / 2);
                        if (staticZoomFactor == 1.0f)
                        {
                            offset /= zoomFactor;
                        }

                        while (offset < 0)
                        {
                            offset += interval * staticZoomFactor;
                        }
                    }
                    else
                    {
                        offset -= static_cast<FLOAT>(viewportDim / (2 * zoomFactor));
                        offset *= staticZoomFactor;
                    }
                    *pRegularOffset = offset;
                }
                // pSnapCoordinate stays what it is.
                break;

            case xaml_primitives::SnapPointsAlignment_Far:
                *pRegularOffset = offset * staticZoomFactor;
                *pSnapCoordinate = static_cast<DMSnapCoordinate>(*pSnapCoordinate | DMSnapCoordinateMirrored);
                break;
            }
            *pRegularInterval = interval * staticZoomFactor;
            *pAreSnapPointsRegular = TRUE;
        }
        else
        {
            IFC(CopyMotionSnapPointsFromVector(
                false/*isForZoomSnapPoints*/,
                spSnapPointsVector.Get(),
                snapPointsAlignment,
                viewportDim,
                extentDim,
                zoomFactor,
                staticZoomFactor,
                pcIrregularSnapPoints,
                ppIrregularSnapPoints));
            *pAreSnapPointsRegular = FALSE;
            // pSnapCoordinate stays what it is.
        }
        *pAreSnapPointsOptional = (snapPointsType == xaml_controls::SnapPointsType_Optional ||
            snapPointsType == xaml_controls::SnapPointsType_OptionalSingle);
        *pAreSnapPointsSingle = (snapPointsType == xaml_controls::SnapPointsType_MandatorySingle ||
            snapPointsType == xaml_controls::SnapPointsType_OptionalSingle);
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::CopyMotionSnapPointsFromVector
//
//  Synopsis:
//    Converts the provided vector of floats into an array of floats to be
//    handed off to the core by GetManipulationSnapPoints.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::CopyMotionSnapPointsFromVector(
    _In_ BOOLEAN isForZoomSnapPoints,
    _In_opt_ wfc::IVectorView<FLOAT>* pSnapPointsVector,
    _In_ xaml_primitives::SnapPointsAlignment snapPointsAlignment, // Near/Center/Far alignment of the snap points
    _In_ DOUBLE viewportDim,                                 // Dimension of the viewport
    _In_ DOUBLE extentDim,                                   // Dimension of the extent
    _In_ FLOAT zoomFactor,                                   // Zoom factor to apply
    _In_ FLOAT staticZoomFactor,                             // Static zoom factor to apply
    _Out_ UINT32* pcSnapPoints,                              // Number of snap points
    _Outptr_result_buffer_(*pcSnapPoints) FLOAT** ppSnapPoints)  // Array of snap points
{
    HRESULT hr = S_OK;
    BOOLEAN hasCurrent = FALSE;
    UINT32 iSnapPoint = 0;
    UINT32 cSnapPoints = 0;
    FLOAT alignedSnapPoint = 0.0;
    FLOAT snapPoint = 0.0;
    FLOAT* pSnapPoints = NULL;

    ASSERT(staticZoomFactor == 1.0f || staticZoomFactor == zoomFactor);

    IFCPTR(pcSnapPoints);
    *pcSnapPoints = 0;
    IFCPTR(ppSnapPoints);
    *ppSnapPoints = NULL;

    if (pSnapPointsVector)
    {
        IFC(pSnapPointsVector->get_Size(&cSnapPoints));
        if (cSnapPoints > 0)
        {
            ctl::ComPtr<wfc::IIterable<FLOAT>> spSnapPointsIterable;

            spSnapPointsIterable = ctl::query_interface_cast<wfc::IIterable<FLOAT>>(pSnapPointsVector);
            if (spSnapPointsIterable)
            {
                ctl::ComPtr<wfc::IIterator<FLOAT>> spSnapPointsIterator;

                IFC(spSnapPointsIterable->First(&spSnapPointsIterator));
                if (spSnapPointsIterator)
                {
                    pSnapPoints = new FLOAT[cSnapPoints];

                    do
                    {
                        IFC(spSnapPointsIterator->get_Current(&snapPoint));

                        if (isForZoomSnapPoints)
                        {
                            pSnapPoints[iSnapPoint] = snapPoint * staticZoomFactor;
                            iSnapPoint++;
                        }
                        else
                        {
                            // When snap points alignment is Center or Far, the irregular snap points need
                            // to be adjusted based on the static or manipulatable zoom factor. In the Near case
                            // it doesn't matter so do it for consistency.
                            snapPoint *= zoomFactor;

                            // Adjust snap point value according to alignment choice.
                            switch (snapPointsAlignment)
                            {
                            case xaml_primitives::SnapPointsAlignment_Near:
                                alignedSnapPoint = snapPoint;
                                break;

                            case xaml_primitives::SnapPointsAlignment_Center:
                                alignedSnapPoint = static_cast<FLOAT>(snapPoint - viewportDim / 2);
                                break;

                            case xaml_primitives::SnapPointsAlignment_Far:
                                alignedSnapPoint = static_cast<FLOAT>(snapPoint - viewportDim);
                                break;
                            }

                            FLOAT distanceFromFarEdge = static_cast<FLOAT>(extentDim - viewportDim - alignedSnapPoint);

                            // With certain scale factors and resolutions the snap points sometimes get pushed out of bounds
                            // by rounding errors, in these cases we need to intervene and return them to the boundary.
                            if (distanceFromFarEdge < 0 &&
                                distanceFromFarEdge >= (-1.0 * (ScrollViewerSnapPointLocationTolerance * DoubleUtil::Max(1.0, zoomFactor))))
                            {
                                alignedSnapPoint = static_cast<FLOAT>(extentDim - viewportDim);
                                distanceFromFarEdge = 0;
                            }
                            if (alignedSnapPoint >= 0 && distanceFromFarEdge >= 0)
                            {
                                if (staticZoomFactor == 1.0f)
                                {
                                    alignedSnapPoint /= zoomFactor;
                                }
                                pSnapPoints[iSnapPoint] = alignedSnapPoint;
                                iSnapPoint++;
                            }
                        }

                        IFC(spSnapPointsIterator->MoveNext(&hasCurrent));
                    }
                    while (hasCurrent);

                    if (iSnapPoint > 0)
                    {
                        *pcSnapPoints = iSnapPoint;
                        *ppSnapPoints = pSnapPoints;
                        pSnapPoints = NULL;
                    }
                }
            }
        }
    }

Cleanup:
    delete[] pSnapPoints;
    RRETURN(hr);
}

// Ensures the target horizontal offset for a ChangeView request does
// not exceed the maximum offset given the target zoom factor, viewport
// width, extent width and optionally the mandatory scroll snap points.
_Check_return_ HRESULT ScrollViewer::AdjustTargetHorizontalOffset(
    _In_ BOOLEAN disableAnimation,
    _In_ BOOLEAN adjustWithMandatorySnapPoints,
    _In_ FLOAT targetZoomFactor,
    _In_ DOUBLE minHorizontalOffset,
    _In_ DOUBLE currentHorizontalOffset,
    _In_ DOUBLE viewportPixelWidth,
    _Inout_ DOUBLE* pTargetHorizontalOffset,
    _Out_ DOUBLE* pCurrentUnzoomedPixelExtentWidth,
    _Out_ DOUBLE* pMaxHorizontalOffset,
    _Out_ DOUBLE* pTargetExtentWidth)
{
    DOUBLE targetHorizontalOffset = *pTargetHorizontalOffset;
    DOUBLE currentUnzoomedPixelExtentWidth = 0.0;
    DOUBLE maxHorizontalOffset = 0.0;
    DOUBLE targetExtentWidth = 0.0;

    *pCurrentUnzoomedPixelExtentWidth = 0.0;
    *pMaxHorizontalOffset = 0.0;
    *pTargetExtentWidth = 0.0;

    // Compute current extent width
    IFC_RETURN(ComputePixelExtentWidth(true /*ignoreZoomFactor*/, nullptr /*pProvider*/, &currentUnzoomedPixelExtentWidth));

    // Compute expected extent width
    targetExtentWidth = currentUnzoomedPixelExtentWidth * targetZoomFactor;

    // Compute expected max offset
    maxHorizontalOffset = DoubleUtil::Max(minHorizontalOffset, targetExtentWidth - viewportPixelWidth);

    // Clamp target offset with expected max offset
    if (targetHorizontalOffset > maxHorizontalOffset)
    {
        targetHorizontalOffset = maxHorizontalOffset;
    }

    // Adjust offset with mandatory scroll snap points
    if (disableAnimation && adjustWithMandatorySnapPoints)
    {
        if (!m_trScrollSnapPointsInfo)
        {
            IFC_RETURN(RefreshScrollSnapPointsInfo());
        }

        IFC_RETURN(AdjustOffsetWithMandatorySnapPoints(
            TRUE /*isForHorizontalOffset*/,
            minHorizontalOffset,
            maxHorizontalOffset,
            currentHorizontalOffset,
            targetExtentWidth /*targetExtentDim*/,
            viewportPixelWidth,
            targetZoomFactor,
            &targetHorizontalOffset));
    }

    *pTargetHorizontalOffset = targetHorizontalOffset;
    *pCurrentUnzoomedPixelExtentWidth = currentUnzoomedPixelExtentWidth;
    *pMaxHorizontalOffset = maxHorizontalOffset;
    *pTargetExtentWidth = targetExtentWidth;

    return S_OK;
}

// Ensures the target vertical offset for a ChangeView request does
// not exceed the maximum offset given the target zoom factor, viewport
// height, extent height and optionally the mandatory scroll snap points.
_Check_return_ HRESULT ScrollViewer::AdjustTargetVerticalOffset(
    _In_ BOOLEAN disableAnimation,
    _In_ BOOLEAN adjustWithMandatorySnapPoints,
    _In_ FLOAT targetZoomFactor,
    _In_ DOUBLE minVerticalOffset,
    _In_ DOUBLE currentVerticalOffset,
    _In_ DOUBLE viewportPixelHeight,
    _Inout_ DOUBLE* pTargetVerticalOffset,
    _Out_ DOUBLE* pCurrentUnzoomedPixelExtentHeight,
    _Out_ DOUBLE* pMaxVerticalOffset,
    _Out_ DOUBLE* pTargetExtentHeight)
{
    DOUBLE targetVerticalOffset = *pTargetVerticalOffset;
    DOUBLE currentUnzoomedPixelExtentHeight = 0.0;
    DOUBLE maxVerticalOffset = 0.0;
    DOUBLE targetExtentHeight = 0.0;

    *pCurrentUnzoomedPixelExtentHeight = 0.0;
    *pMaxVerticalOffset = 0.0;
    *pTargetExtentHeight = 0.0;

    // Compute current extent height
    IFC_RETURN(ComputePixelExtentHeight(true /*ignoreZoomFactor*/, nullptr /*pProvider*/, &currentUnzoomedPixelExtentHeight));

    // Compute expected extent height
    targetExtentHeight = currentUnzoomedPixelExtentHeight * targetZoomFactor;

    // Compute expected max offset
    maxVerticalOffset = DoubleUtil::Max(minVerticalOffset, targetExtentHeight - viewportPixelHeight);

    // Clamp target offset with expected max offset
    if (targetVerticalOffset > maxVerticalOffset)
    {
        targetVerticalOffset = maxVerticalOffset;
    }

    // Adjust offset with mandatory scroll snap points
    if (disableAnimation && adjustWithMandatorySnapPoints)
    {
        if (!m_trScrollSnapPointsInfo)
        {
            IFC_RETURN(RefreshScrollSnapPointsInfo());
        }

        IFC_RETURN(AdjustOffsetWithMandatorySnapPoints(
            FALSE /*isForHorizontalOffset*/,
            minVerticalOffset,
            maxVerticalOffset,
            currentVerticalOffset,
            targetExtentHeight /*targetExtentDim*/,
            viewportPixelHeight,
            targetZoomFactor,
            &targetVerticalOffset));
    }

    *pTargetVerticalOffset = targetVerticalOffset;
    *pCurrentUnzoomedPixelExtentHeight = currentUnzoomedPixelExtentHeight;
    *pMaxVerticalOffset = maxVerticalOffset;
    *pTargetExtentHeight = targetExtentHeight;

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::AdjustLogicalOffsetWithMandatorySnapPoints
//
//  Synopsis:
//    Adjusts the provided logical targetOffset based on potential mandatory near-aligned scroll snap points.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::AdjustLogicalOffsetWithMandatorySnapPoints(
    _In_ BOOLEAN isForHorizontalOffset,
    _Inout_ DOUBLE* pTargetOffset)
{
    xaml_controls::SnapPointsType snapPointsType = xaml_controls::SnapPointsType_None;
    xaml_primitives::SnapPointsAlignment snapPointsAlignment = xaml_primitives::SnapPointsAlignment_Near;

    ASSERT(pTargetOffset);

    if (!m_trScrollSnapPointsInfo)
    {
        // No scroll snap point implementation to operate against.
        return S_OK;
    }

    if (isForHorizontalOffset)
    {
        IFC_RETURN(ScrollViewerGenerated::get_HorizontalSnapPointsType(&snapPointsType));
        IFC_RETURN(ScrollViewerGenerated::get_HorizontalSnapPointsAlignment(&snapPointsAlignment));
    }
    else
    {
        IFC_RETURN(ScrollViewerGenerated::get_VerticalSnapPointsType(&snapPointsType));
        IFC_RETURN(ScrollViewerGenerated::get_VerticalSnapPointsAlignment(&snapPointsAlignment));
    }

    if (snapPointsType != xaml_controls::SnapPointsType_MandatorySingle &&
        snapPointsType != xaml_controls::SnapPointsType_Mandatory)
    {
        // Scroll snap points are not mandatory.
        return S_OK;
    }

    if (snapPointsAlignment != xaml_primitives::SnapPointsAlignment_Near)
    {
        // Scroll snap points are not near-aligned. We don't know how to alter the logical offset.
        return S_OK;
    }

    // Truncate targetOffset to the lower integer - this corresponds to the snap point
    // separating two consecutive items.

    // That truncation is not performed though when the target offset is very slightly
    // smaller than an integer because of a float rounding.

    // Tolerated rounding delta in logical unit for skipping the adjustment to the lower integer.
    const DOUBLE ScrollViewerScrollSnapPointRoundingToleranceForProvider = 0.00001;
    DOUBLE adjustedTargetOffset = DoubleUtil::Floor(*pTargetOffset);

    if (*pTargetOffset - adjustedTargetOffset < 1.0 - ScrollViewerScrollSnapPointRoundingToleranceForProvider)
    {
        *pTargetOffset = adjustedTargetOffset;
    }

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::AdjustOffsetWithMandatorySnapPoints
//
//  Synopsis:
//    Adjusts the provided targetOffset based on potential mandatory scroll snap points.
//    The provided minOffset and maxOffset guarantee that the adjustment remains within
//    the allowed boundaries.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::AdjustOffsetWithMandatorySnapPoints(
    _In_ BOOLEAN isForHorizontalOffset,
    _In_ DOUBLE minOffset,
    _In_ DOUBLE maxOffset,
    _In_ DOUBLE currentOffset,
    _In_ DOUBLE targetExtentDim,
    _In_ DOUBLE viewportDim,
    _In_ FLOAT targetZoomFactor,
    _Inout_ DOUBLE* pTargetOffset)
{
    HRESULT hr = S_OK;
    BOOLEAN areSnapPointsOptional = FALSE;
    BOOLEAN areSnapPointsSingle = FALSE;
    BOOLEAN areSnapPointsRegular = FALSE;
    DOUBLE closestSnapPoint = 0;
    DOUBLE smallestDistance = DoubleUtil::MaxValue;
    FLOAT regularOffset = 0.0f;
    FLOAT regularInterval = 0.0f;
    FLOAT* pIrregularSnapPoints = NULL;
    UINT32 cIrregularSnapPoints = 0;
    DMSnapCoordinate snapCoordinate;
    xaml_controls::SnapPointsType snapPointsType = xaml_controls::SnapPointsType_None;

    ASSERT(pTargetOffset);
    closestSnapPoint = *pTargetOffset;

    if (!m_trScrollSnapPointsInfo)
    {
        // No scroll snap point implementation to operate against.
        goto Cleanup;
    }

    if (isForHorizontalOffset)
    {
        IFC(ScrollViewerGenerated::get_HorizontalSnapPointsType(&snapPointsType));
    }
    else
    {
        IFC(ScrollViewerGenerated::get_VerticalSnapPointsType(&snapPointsType));
    }

    if (snapPointsType != xaml_controls::SnapPointsType_MandatorySingle &&
        snapPointsType != xaml_controls::SnapPointsType_Mandatory)
    {
        // Scroll snap points are not mandatory.
        goto Cleanup;
    }

    IFC(GetScrollSnapPoints(
        isForHorizontalOffset /*isForHorizontalSnapPoints*/,
        targetZoomFactor,
        targetZoomFactor /* staticZoomFactor */,
        targetExtentDim /*extentDim*/,
        FALSE /*updateSnapPointsChangeSubscription*/,
        &areSnapPointsOptional,
        &areSnapPointsSingle,
        &areSnapPointsRegular,
        &regularOffset,
        &regularInterval,
        &cIrregularSnapPoints,
        &pIrregularSnapPoints,
        &snapCoordinate));

    ASSERT(!areSnapPointsOptional);

    {
        bool useCurrentOffsetForMandatorySingleSnapPoints =
            snapPointsType == xaml_controls::SnapPointsType_MandatorySingle &&
            *pTargetOffset != currentOffset;
        double signFactor = (*pTargetOffset > currentOffset) ? 1.0 : -1.0;

        if (areSnapPointsRegular && regularInterval > 0)
        {
            // There are regular snap points. Determine the closest one to *pTargetOffset when snapPointsType==SnapPointsType_Mandatory,
            // and the closest one to currentOffset when snapPointsType==SnapPointsType_MandatorySingle.
            if (useCurrentOffsetForMandatorySingleSnapPoints)
            {
                *pTargetOffset = currentOffset + signFactor * regularInterval;
            }

            if (snapCoordinate == DMSnapCoordinateMirrored)
            {
                // Far alignment
                closestSnapPoint = targetExtentDim - viewportDim - regularOffset - DoubleUtil::Round((targetExtentDim - viewportDim - *pTargetOffset) / regularInterval, 0 /*numDecimalPlaces*/) * regularInterval;
                if (closestSnapPoint < 0.0)
                {
                    // Handle attempt to move prior to first snap point
                    closestSnapPoint = regularInterval - viewportDim;
                }
            }
            else
            {
                // Near and Center alignments
                if (*pTargetOffset <= regularOffset)
                {
                    closestSnapPoint = regularOffset;
                }
                else
                {
                    closestSnapPoint = DoubleUtil::Round((*pTargetOffset - regularOffset) / regularInterval, 0 /*numDecimalPlaces*/) * regularInterval + regularOffset;
                }
                if (closestSnapPoint > maxOffset)
                {
                    // Handle attempt to move past the last snap point
                    closestSnapPoint -= regularInterval;
                }
            }
            if (closestSnapPoint >= minOffset && closestSnapPoint <= maxOffset)
            {
                *pTargetOffset = closestSnapPoint;
            }
        }
        else if (cIrregularSnapPoints > 0)
        {
            // There are irregular snap points. Determine the closest one to *pTargetOffset when snapPointsType==SnapPointsType_Mandatory,
            // and the closest one to currentOffset when snapPointsType==SnapPointsType_MandatorySingle.
            if (useCurrentOffsetForMandatorySingleSnapPoints)
            {
                // When *pTargetOffset > currentOffset:
                //   Target offset is after current offset. First try to find a snap point located after currentOffset within the acceptable boundaries.
                //   Second when no snap point after currentOffset was within the acceptable boundaries. Select a snap point before currentOffset instead.
                // When *pTargetOffset < currentOffset:
                //   Target offset is before current offset. First try to find a snap point located before currentOffset within the acceptable boundaries.
                //   Second when no snap point before currentOffset was within the acceptable boundaries. Select a snap point after currentOffset instead.
                for (UINT32 iIrregularSnapPoint = 0; iIrregularSnapPoint < cIrregularSnapPoints; iIrregularSnapPoint++)
                {
                    if (pIrregularSnapPoints[iIrregularSnapPoint] >= minOffset &&
                        pIrregularSnapPoints[iIrregularSnapPoint] <= maxOffset &&
                        signFactor * (pIrregularSnapPoints[iIrregularSnapPoint] - currentOffset) > 0.0 &&
                        signFactor * (pIrregularSnapPoints[iIrregularSnapPoint] - currentOffset) < smallestDistance)
                    {
                        smallestDistance = signFactor * (pIrregularSnapPoints[iIrregularSnapPoint] - currentOffset);
                        closestSnapPoint = pIrregularSnapPoints[iIrregularSnapPoint];
                    }
                }
                if (smallestDistance == DoubleUtil::MaxValue)
                {
                    for (UINT32 iIrregularSnapPoint = 0; iIrregularSnapPoint < cIrregularSnapPoints && smallestDistance != 0.0; iIrregularSnapPoint++)
                    {
                        if (pIrregularSnapPoints[iIrregularSnapPoint] >= minOffset &&
                            pIrregularSnapPoints[iIrregularSnapPoint] <= maxOffset &&
                            signFactor * (currentOffset - pIrregularSnapPoints[iIrregularSnapPoint]) >= 0.0 &&
                            signFactor * (currentOffset - pIrregularSnapPoints[iIrregularSnapPoint]) < smallestDistance)
                        {
                            smallestDistance = signFactor * (currentOffset - pIrregularSnapPoints[iIrregularSnapPoint]);
                            closestSnapPoint = pIrregularSnapPoints[iIrregularSnapPoint];
                        }
                    }
                }
            }
            else
            {
                for (UINT32 iIrregularSnapPoint = 0; iIrregularSnapPoint < cIrregularSnapPoints; iIrregularSnapPoint++)
                {
                    if (pIrregularSnapPoints[iIrregularSnapPoint] >= minOffset &&
                        pIrregularSnapPoints[iIrregularSnapPoint] <= maxOffset &&
                        DoubleUtil::Abs(*pTargetOffset - pIrregularSnapPoints[iIrregularSnapPoint]) < smallestDistance)
                    {
                        smallestDistance = DoubleUtil::Abs(*pTargetOffset - pIrregularSnapPoints[iIrregularSnapPoint]);
                        closestSnapPoint = pIrregularSnapPoints[iIrregularSnapPoint];
                    }
                }
            }
            *pTargetOffset = closestSnapPoint;
        }
    }

Cleanup:
    delete[] pIrregularSnapPoints;
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::AdjustZoomFactorWithMandatorySnapPoints
//
//  Synopsis:
//    Adjusts the provided targetZoomFactor zoom factor based on potential mandatory
//    zoom snap points. The provided minZoomFactor and maxZoomFactor values guarantee
//    that the adjustment remains within the allowed boundaries.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::AdjustZoomFactorWithMandatorySnapPoints(
    _In_ FLOAT minZoomFactor,
    _In_ FLOAT maxZoomFactor,
    _Inout_ FLOAT* pTargetZoomFactor)
{
    HRESULT hr = S_OK;
    BOOLEAN hasCurrent = FALSE;
    UINT32 cZoomSnapPoints = 0;
    FLOAT zoomSnapPoint = 0.0f;
    FLOAT adjustedZoomFactor = 0.0f;
    DOUBLE smallestDistance = DBL_MAX;
    xaml_controls::SnapPointsType snapPointsType = xaml_controls::SnapPointsType_None;
    ctl::ComPtr<wfc::IVectorView<FLOAT>> spSnapPointsVector;

    ASSERT(pTargetZoomFactor);
    adjustedZoomFactor = *pTargetZoomFactor;

    if (m_spZoomSnapPoints)
    {
        IFC(ScrollViewerGenerated::get_ZoomSnapPointsType(&snapPointsType));

        if (snapPointsType == xaml_controls::SnapPointsType_MandatorySingle ||
            snapPointsType == xaml_controls::SnapPointsType_Mandatory)
        {
            // Pick the mandatory zoom snap point closest to *pTargetZoomFactor
            spSnapPointsVector = m_spZoomSnapPoints.AsOrNull<wfc::IVectorView<FLOAT>>();
            IFC(spSnapPointsVector->get_Size(&cZoomSnapPoints));
            if (cZoomSnapPoints > 0)
            {
                ctl::ComPtr<wfc::IIterable<FLOAT>> spSnapPointsIterable = spSnapPointsVector.AsOrNull<wfc::IIterable<FLOAT>>();
                if (spSnapPointsIterable)
                {
                    ctl::ComPtr<wfc::IIterator<FLOAT>> spSnapPointsIterator;

                    IFC(spSnapPointsIterable->First(&spSnapPointsIterator));
                    if (spSnapPointsIterator)
                    {
                        do
                        {
                            IFC(spSnapPointsIterator->get_Current(&zoomSnapPoint));

                            if (zoomSnapPoint >= minZoomFactor && zoomSnapPoint <= maxZoomFactor &&
                                DoubleUtil::Abs(zoomSnapPoint - *pTargetZoomFactor) < smallestDistance)
                            {
                                smallestDistance = DoubleUtil::Abs(zoomSnapPoint - *pTargetZoomFactor);
                                adjustedZoomFactor = zoomSnapPoint;
                            }

                            IFC(spSnapPointsIterator->MoveNext(&hasCurrent));
                        }
                        while (hasCurrent);
                        *pTargetZoomFactor = adjustedZoomFactor;
                    }
                }
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::AdjustViewWithMandatorySnapPoints
//
//  Synopsis:
//    Adjusts the provided target HorizontalOffset, target VerticalOffset and target
//    ZoomFactor based on potential mandatory scroll and zoom snap points.
//    The provided min & max offsets and factor guarantee that the adjustment remains
//    within the allowed boundaries.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::AdjustViewWithMandatorySnapPoints(
    _In_ DOUBLE minHorizontalOffset,
    _In_ DOUBLE maxHorizontalOffset,
    _In_ DOUBLE currentHorizontalOffset,
    _In_ DOUBLE minVerticalOffset,
    _In_ DOUBLE maxVerticalOffset,
    _In_ DOUBLE currentVerticalOffset,
    _In_ FLOAT minZoomFactor,
    _In_ FLOAT maxZoomFactor,
    _In_ DOUBLE viewportPixelWidth,
    _In_ DOUBLE viewportPixelHeight,
    _Inout_ DOUBLE* pCurrentUnzoomedPixelExtentWidth,
    _Inout_ DOUBLE* pCurrentUnzoomedPixelExtentHeight,
    _Inout_ DOUBLE* pTargetHorizontalOffset,
    _Inout_ DOUBLE* pTargetVerticalOffset,
    _Inout_ FLOAT* pTargetZoomFactor)
{
    HRESULT hr = S_OK;
    DOUBLE targetExtentWidth = 0.0;
    DOUBLE targetExtentHeight = 0.0;

    ASSERT(pCurrentUnzoomedPixelExtentWidth);
    ASSERT(pCurrentUnzoomedPixelExtentHeight);
    ASSERT(pTargetHorizontalOffset);
    ASSERT(pTargetVerticalOffset);
    ASSERT(pTargetZoomFactor);
    ASSERT(viewportPixelWidth > 0);
    ASSERT(viewportPixelHeight > 0);

    // Adjust the target zoom factor based on potential mandatory zoom snap points
    IFC(AdjustZoomFactorWithMandatorySnapPoints(minZoomFactor, maxZoomFactor, pTargetZoomFactor));

    // Compute expected extent width
    if (*pCurrentUnzoomedPixelExtentWidth == -1.0)
    {
        IFC(ComputePixelExtentWidth(true /*ignoreZoomFactor*/, nullptr /*pProvider*/, pCurrentUnzoomedPixelExtentWidth));
    }
    ASSERT(*pCurrentUnzoomedPixelExtentWidth != -1.0);
    targetExtentWidth = *pCurrentUnzoomedPixelExtentWidth * (*pTargetZoomFactor);

    // ...and expected extent height
    if (*pCurrentUnzoomedPixelExtentHeight == -1.0)
    {
        IFC(ComputePixelExtentHeight(true /*ignoreZoomFactor*/, nullptr /*pProvider*/, pCurrentUnzoomedPixelExtentHeight));
    }
    ASSERT(*pCurrentUnzoomedPixelExtentHeight != -1.0);
    targetExtentHeight = *pCurrentUnzoomedPixelExtentHeight * (*pTargetZoomFactor);

    // Compute expected max horizontal offset
    if (maxHorizontalOffset == -1.0)
    {
        maxHorizontalOffset = DoubleUtil::Max(minHorizontalOffset, targetExtentWidth - viewportPixelWidth);
    }

    // ... and expected max vertical offset
    if (maxVerticalOffset == -1.0)
    {
        maxVerticalOffset = DoubleUtil::Max(minVerticalOffset, targetExtentHeight - viewportPixelHeight);
    }

    ASSERT(minHorizontalOffset >= 0.0);
    ASSERT(minVerticalOffset >= 0.0);
    ASSERT(maxHorizontalOffset >= 0.0);
    ASSERT(maxVerticalOffset >= 0.0);
    ASSERT(targetExtentWidth >= 0.0);
    ASSERT(targetExtentHeight >= 0.0);

    // Adjust the target offsets based on potential mandatory scroll snap points
    IFC(AdjustOffsetWithMandatorySnapPoints(
        TRUE /*isForHorizontalOffset*/,
        minHorizontalOffset,
        maxHorizontalOffset,
        currentHorizontalOffset,
        targetExtentWidth /*targetExtentDim*/,
        viewportPixelWidth,
        *pTargetZoomFactor,
        pTargetHorizontalOffset));

    IFC(AdjustOffsetWithMandatorySnapPoints(
        FALSE /*isForHorizontalOffset*/,
        minVerticalOffset,
        maxVerticalOffset,
        currentVerticalOffset,
        targetExtentHeight /*targetExtentDim*/,
        viewportPixelHeight,
        *pTargetZoomFactor,
        pTargetVerticalOffset));

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::NotifyManipulatabilityAffectingPropertyChanged
//
//  Synopsis:
//    Called to notify of a characteristic change that may affect the
//    manipulability of inner elements.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::NotifyManipulatabilityAffectingPropertyChanged(
    _In_ BOOLEAN isInLiveTree)
{
#ifdef DM_DEBUG
    if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/,
            L"DMSV[0x%p]:  NotifyManipulatabilityAffectingPropertyChanged isInLiveTree=%d.", this, isInLiveTree));
    }
#endif // DM_DEBUG

    HRESULT hr = S_OK;

    IFC(OnManipulatabilityAffectingPropertyChanged(
        &isInLiveTree /*pIsInLiveTree*/,
        FALSE /*isCachedPropertyChanged*/,
        FALSE /*isContentChanged*/,
        FALSE /*isAffectingConfigurations*/,
        FALSE /*isAffectingTouchConfiguration*/));

    if (isInLiveTree)
    {
        // The ScrollViewer control is entering the tree - check if its zoom factor was previously altered.
        ASSERT(!IsInManipulation());
        // This will result in a CDMViewport::SetCompositorTransformationValues call that scales the ScrollViewer Content according
        // to the current zoom factor. When the zoom factor is 1.0, no call to SetCompositorTransformationValues is required.
        // X and Y translations are declared changed as well so that left/top margins are taken into account by the compositor.
        // DManip needs to be aware of the content transform immediately via a ZoomToRect call.
        IFC(OnPrimaryContentTransformChanged(TRUE /*translationXChanged*/, TRUE /*translationYChanged*/, TRUE /*zoomFactorChanged*/));

        // Push any potential headers to the manipulation handler.
        IFC(NotifyHeadersParented());
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::NotifyContentAlignmentAffectingPropertyChanged
//
//  Synopsis:
//    Called to notify of a characteristic change that may affect the
//    alignment of the provided manipulated element.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::NotifyContentAlignmentAffectingPropertyChanged(
    _In_ UIElement* pManipulatedElement,
    _In_ BOOLEAN isForHorizontalAlignment,
    _In_ BOOLEAN isForStretchAlignment,
    _In_ BOOLEAN isStretchAlignmentTreatedAsNear)
{
    HRESULT hr = S_OK;
    BOOLEAN isHeaderSet = FALSE;
    xaml::HorizontalAlignment horizontalContentAlignment = xaml::HorizontalAlignment_Center;
    xaml::VerticalAlignment verticalContentAlignment = xaml::VerticalAlignment_Center;
    ctl::ComPtr<xaml::IFrameworkElement> spContentFE;
    ctl::ComPtr<xaml::IUIElement> spHeader;

#ifdef DM_DEBUG
    if ((DMSV_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG)) == S_OK))
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/,
            L"DMSV[0x%p]:  NotifyContentAlignmentAffectingPropertyChanged - pManipulatedElement=0x%p, isForHorizontalAlignment=%d, isForStretchAlignment=%d, isStretchAlignmentTreatedAsNear=%d.",
            this, pManipulatedElement, isForHorizontalAlignment, isForStretchAlignment, isStretchAlignmentTreatedAsNear));
    }
#endif // DM_DEBUG

    IFCPTR(pManipulatedElement);

    if (isForStretchAlignment)
    {
        if (isForHorizontalAlignment)
        {
            m_isHorizontalStretchAlignmentTreatedAsNear = isStretchAlignmentTreatedAsNear;
        }
        else
        {
            m_isVerticalStretchAlignmentTreatedAsNear = isStretchAlignmentTreatedAsNear;
        }
    }

    if (m_trElementScrollContentPresenter)
    {
        IFC(m_trElementScrollContentPresenter.Cast<ScrollContentPresenter>()->RefreshUseOfActualSizeAsExtent(pManipulatedElement));
    }

    // When DManip-on-DComp is turned on, DManip needs to be aware of the latest content alignments, even outside a manipulation,
    // in order to reflect the latest data in the visual tree.
    if (IsInManipulation() || m_isManipulationHandlerInterestedInNotifications || m_trManipulatableElement)
    {
        // Push new alignment to DirectManipulation
        IFC(OnContentAlignmentAffectingPropertyChanged(isForHorizontalAlignment, !isForHorizontalAlignment /*isForVerticalAlignment*/));
    }

    if (!IsInManipulation())
    {
        // This will result in a CDMViewport::SetCompositorTransformationValues call that repositions the ScrollViewer Content according
        // to its new alignment.
        IFC(OnPrimaryContentTransformChanged(TRUE /*translationXChanged*/, TRUE /*translationYChanged*/, TRUE /*zoomFactorChanged*/));
    }

    // Check if a header element is set and the content alignment is not Top/Left.
    IFC(get_TopLeftHeader(&spHeader));
    if (spHeader)
    {
        isHeaderSet = TRUE;
    }
    else
    {
        IFC(get_TopHeader(&spHeader));
        if (spHeader)
        {
            isHeaderSet = TRUE;
        }
        else
        {
            IFC(get_LeftHeader(&spHeader));
            if (spHeader)
            {
                isHeaderSet = TRUE;
            }
        }
    }

    if (isHeaderSet)
    {
        IFC(ctl::do_query_interface(spContentFE, pManipulatedElement));
        if (spContentFE)
        {
            if (isForHorizontalAlignment)
            {
                IFC(spContentFE->get_HorizontalAlignment(&horizontalContentAlignment));
                if (horizontalContentAlignment != xaml::HorizontalAlignment_Left)
                {
                    IFC(ErrorHelper::OriginateErrorUsingResourceID(E_NOT_SUPPORTED, ERROR_SCROLLVIEWER_UNSUPPORTED_HORIZONTALALIGNMENT_WITH_HEADER));
                }
            }
            else
            {
                IFC(spContentFE->get_VerticalAlignment(&verticalContentAlignment));
                if (verticalContentAlignment != xaml::VerticalAlignment_Top)
                {
                    IFC(ErrorHelper::OriginateErrorUsingResourceID(E_NOT_SUPPORTED, ERROR_SCROLLVIEWER_UNSUPPORTED_VERTICALALIGNMENT_WITH_HEADER));
                }
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::NotifyManipulationProgress
//
//  Synopsis:
//    Processes the DirectManipulation feedback for the provided
//    manipulated element.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::NotifyManipulationProgress(
    _In_ UIElement* pManipulatedElement,
    _In_ DMManipulationState state,
    _In_ FLOAT xCumulativeTranslation,
    _In_ FLOAT yCumulativeTranslation,
    _In_ FLOAT zCumulativeFactor,
    _In_ FLOAT xInertiaEndTranslation,
    _In_ FLOAT yInertiaEndTranslation,
    _In_ FLOAT zInertiaEndFactor,
    _In_ FLOAT xCenter,
    _In_ FLOAT yCenter,
    _In_ BOOLEAN isInertiaEndTransformValid,
    _In_ BOOLEAN isInertial,
    _In_ BOOLEAN isTouchConfigurationActivated,
    _In_ BOOLEAN isBringIntoViewportConfigurationActivated)
{
    HRESULT hr = S_OK;
    BOOLEAN wasInDirectManipulation = FALSE;
    BOOLEAN wasInDirectManipulationZoom = FALSE;

#ifdef DM_DEBUG
    if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/,
            L"DMSV[0x%p]:  NotifyManipulationProgress pManipulatedElement=0x%p, state=%d, xCumulativeTranslation=%f, yCumulativeTranslation=%f, zCumulativeFactor=%4.8lf.",
            this, pManipulatedElement, state, xCumulativeTranslation, yCumulativeTranslation, zCumulativeFactor));
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/,
            L"                   xInertiaEndTranslation=%f, yInertiaEndTranslation=%f, zInertiaEndFactor=%4.8lf, xCenter=%f, yCenter=%f.",
            xInertiaEndTranslation, yInertiaEndTranslation, zInertiaEndFactor, xCenter, yCenter));
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/,
            L"                   isInertiaEndTransformValid=%d, isInertial=%d, isTouchConfigurationActivated=%d, isBringIntoViewportConfigurationActivated=%d.",
            isInertiaEndTransformValid, isInertial, isTouchConfigurationActivated, isBringIntoViewportConfigurationActivated));
    }
#endif // DM_DEBUG

    m_dmanipState = state;

    if (m_isInertial && !isInertial)
    {
        // Inertia stops
        m_isInertial = FALSE;
        // When transitioning from inertia to non-inertia, the end-of-inertia transform becomes invalid.
        // Reset m_isInertiaEndTransformValid so that the old end-of-inertia transform is not incorrectly used if inertia restarts.
        m_isInertiaEndTransformValid = FALSE;
    }
    else if (!m_isInertial && isInertial)
    {
        // Inertia starts
        m_isInertial = TRUE;
    }

    switch (state)
    {
    case DMManipulationStarting:
        wasInDirectManipulation = m_isInDirectManipulation;
        m_isInDirectManipulation = TRUE;
        m_isDirectManipulationStopped = FALSE;
        m_isInDirectManipulationCompletion = FALSE;
        ASSERT(!m_isInDirectManipulationZoom);
        IFC(HandleManipulationStarting(pManipulatedElement, wasInDirectManipulation));

        if (isTouchConfigurationActivated)
        {
            m_preferMouseIndicators = FALSE;
        }
        break;
    case DMManipulationStarted:
        ASSERT(IsInManipulation());
        // Switch to Intermediate mode since any ViewChanged event raised
        // until the DMManipulationCompleted notification is intermediate.
        IFC(EnterIntermediateViewChangedMode());
        break;
    case DMManipulationDelta:
    case DMManipulationLastDelta:
        // Special processing for scenarios of WPB bugs 261102 & 342668
        if (m_cForcePanXConfiguration > 0 || m_cForcePanYConfiguration > 0)
        {
            BOOLEAN refreshTouchConfiguration = FALSE;
            if (m_cForcePanXConfiguration > 0)
            {
                m_cForcePanXConfiguration--;
                if (m_cForcePanXConfiguration == 0)
                {
                    refreshTouchConfiguration = TRUE;
                }
            }
            if (m_cForcePanYConfiguration > 0)
            {
                m_cForcePanYConfiguration--;
                if (m_cForcePanYConfiguration == 0)
                {
                    refreshTouchConfiguration = TRUE;
                }
            }
            if (refreshTouchConfiguration)
            {
                // Re-establish normal DManip configuration now that the viewport has been active for a bit.
                IFC(OnManipulatabilityAffectingPropertyChanged(
                    NULL  /*pIsInLiveTree*/,
                    FALSE /*isCachedPropertyChanged*/,
                    FALSE /*isContentChanged*/,
                    FALSE /*isAffectingConfigurations*/,
                    TRUE  /*isAffectingTouchConfiguration*/));
            }
        }

        IFC(HandleManipulationDelta(
            pManipulatedElement,
            xInertiaEndTranslation,
            yInertiaEndTranslation,
            zInertiaEndFactor,
            isInertiaEndTransformValid,
            isBringIntoViewportConfigurationActivated,
            state == DMManipulationLastDelta /*isLastDelta*/));
        break;
    case DMManipulationCompleted:
        ASSERT(IsInManipulation());

        // Special processing for scenarios of WPB bugs 261102 & 342668
        m_cForcePanXConfiguration = 0;
        m_cForcePanYConfiguration = 0;

        wasInDirectManipulationZoom = m_isInDirectManipulationZoom;
        m_isInDirectManipulation = FALSE;
        m_isInDirectManipulationZoom = FALSE;
        IFC(HandleManipulationCompleted(pManipulatedElement, wasInDirectManipulationZoom, xCumulativeTranslation, yCumulativeTranslation));
        m_isDirectManipulationZoomFactorChangeIgnored = FALSE;
        // Resetting the m_isInertial flag so that potential subsequent ScrollToHorizontalOffset, ScrollToVerticalOffset and ZoomToFactor calls
        // raise ViewChanging events with IsInertial==False.
        m_isInertial = FALSE;
        break;
    case ConstantVelocityScrollStarted:
        // Switch to Intermediate mode since any ViewChanged event raised
        // until the ConstantVelocityScrollStopped notification is intermediate.
        IFC(EnterIntermediateViewChangedMode());
        m_isInConstantVelocityPan = TRUE;
        IFC(HandleManipulationStarting(pManipulatedElement, m_isInDirectManipulation));
        break;
    case ConstantVelocityScrollStopped:
        ASSERT(IsInManipulation());
        ASSERT(!m_isInDirectManipulationZoom);
        m_isInConstantVelocityPan = FALSE;
        IFC(HandleManipulationCompleted(pManipulatedElement, FALSE /*wasInDirectManipulationZoom*/, xCumulativeTranslation, yCumulativeTranslation));
        break;
    }

    if (m_pDMStateChangeHandler)
    {
        // Let the state change handler know about this progress.
        IFC(m_pDMStateChangeHandler->NotifyStateChange(
            state,
            xCumulativeTranslation,
            yCumulativeTranslation,
            zCumulativeFactor,
            xCenter,
            yCenter,
            isInertial,
            isTouchConfigurationActivated,
            isBringIntoViewportConfigurationActivated));
    }

Cleanup:
    RRETURN(hr);
}

// Called to raise the DirectManipulationStarted/Completed events
_Check_return_ HRESULT ScrollViewer::NotifyManipulationStateChanged(
    _In_ DMManipulationState state)
{
    HRESULT hr = S_OK;

#ifdef DM_DEBUG
    if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/,
            L"DMSV[0x%p]:  NotifyManipulationStateChanged state=%d.", this, state));
    }
#endif // DM_DEBUG

    switch (state)
    {
    case DMManipulationStarted:
        IFC(RaiseDirectManipulationStarted());
        break;
    case DMManipulationCompleted:
        IFC(RaiseDirectManipulationCompleted());
        break;
    default:
        // Unexpected state value
        ASSERT(FALSE);
    }

Cleanup:
    return hr;
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::IsBringIntoViewportNeeded
//
//  Synopsis:
//    This is basically the conditional check that occurs in NotifyBringIntoViewportNeeded
//    but greatly simplified so that other components (such as connected animations)
//    can determine if there is a bring into view pending.
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::IsBringIntoViewportNeeded(_Out_ bool * bringIntoViewportNeeded)
{
    *bringIntoViewportNeeded = false;

    // We might not have a manipulation handler yet.  If not, assume we need a bring into view.
    if (!m_hManipulationHandler)
    {
        *bringIntoViewportNeeded = true;
        return S_OK;
    }

    // Get the manipulated element
    ctl::ComPtr<xaml::IUIElement> spContentUIElement;
    IFC_RETURN(GetContentUIElement(&spContentUIElement));

    UIElement* pManipulatedElementNoRef = spContentUIElement.Cast<UIElement>();

    if (pManipulatedElementNoRef == nullptr)
    {
        // No content element so content can't exceed viewport
        return S_OK;
    }

    // Get the current transform values
    XFLOAT manipulationHandlerTranslationX = 0.0f;
    XFLOAT manipulationHandlerTranslationY = 0.0f;
    XFLOAT manipulationHandlerUncompressedZoomFactor = 1.0f;
    FLOAT manipulationHandlerZoomFactorX = 1.0f;
    FLOAT manipulationHandlerZoomFactorY = 1.0f;
    IFC_RETURN(CoreImports::ManipulationHandler_GetPrimaryContentTransform(
        m_hManipulationHandler,
        static_cast<CUIElement*>(pManipulatedElementNoRef->GetHandle()),
        TRUE /*fForBringIntoViewport*/,
        manipulationHandlerTranslationX,
        manipulationHandlerTranslationY,
        manipulationHandlerUncompressedZoomFactor,
        manipulationHandlerZoomFactorX,
        manipulationHandlerZoomFactorY));

    // Get the desired transform values

    FLOAT translationX = 0.0f;
    FLOAT translationY = 0.0f;
    FLOAT zoomFactor = 1.0f;
    IFC_RETURN(GetManipulationPrimaryContentTransform(
        pManipulatedElementNoRef,
        TRUE  /*inManipulation*/,
        FALSE /*forInitialTransformationAdjustment*/,
        FALSE /*forMargins*/,
        &translationX,
        &translationY,
        &zoomFactor));

    // Compare to see if we will need to bring into view
    if (DoubleUtil::Abs(manipulationHandlerTranslationX - translationX) >= ScrollViewerScrollRoundingToleranceForBringIntoViewport ||
        DoubleUtil::Abs(manipulationHandlerTranslationY - translationY) >= ScrollViewerScrollRoundingToleranceForBringIntoViewport ||
        DoubleUtil::Abs(manipulationHandlerUncompressedZoomFactor - zoomFactor) >= ScrollViewerZoomRoundingToleranceForBringIntoViewport)
    {
        *bringIntoViewportNeeded = true;
    }

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::NotifyBringIntoViewportNeeded
//
//  Synopsis:
//    Called to notify this manipulation handler that it needs to
//    call IDirectManipulationContainerHandler::BringIntoViewport
//    either to synchronize the DManip primary content transform with XAML
//    when transformIsValid==False, or to jump to the provided transform
//    when transformIsValid==True.
//    When transformIsInertiaEnd==True, the call is made after cancelling
//    inertia and the transform provided was the expected end-of-inertia
//    transform.
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::NotifyBringIntoViewportNeeded(
    _In_ UIElement* pManipulatedElement,
    _In_ FLOAT translationX,
    _In_ FLOAT translationY,
    _In_ FLOAT zoomFactor,
    _In_ BOOLEAN transformIsValid,
    _In_ BOOLEAN transformIsInertiaEnd)
{
    HRESULT hr = S_OK;

#ifdef DM_DEBUG
    if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/,
            L"DMSV[0x%p]:  NotifyBringIntoViewportNeeded pManipulatedElement=0x%p, translationX=%f, translationY=%f, zoomFactor=%4.8lf, transformIsValid=%d, transformIsInertiaEnd=%d.",
            this, pManipulatedElement, translationX, translationY, zoomFactor, transformIsValid, transformIsInertiaEnd));
    }
#endif // DM_DEBUG

    IFCPTR(pManipulatedElement);

    if (m_hManipulationHandler && !m_isInDirectManipulationSync)
    {
        XFLOAT manipulationHandlerTranslationX = 0.0f;
        XFLOAT manipulationHandlerTranslationY = 0.0f;
        XFLOAT manipulationHandlerUncompressedZoomFactor = 1.0f;
        FLOAT manipulationHandlerZoomFactorX = 1.0f;
        FLOAT manipulationHandlerZoomFactorY = 1.0f;
        BOOLEAN result = FALSE;
        ctl::ComPtr<wf::IReference<DOUBLE>> spHorizontalOffsetReference;
        ctl::ComPtr<wf::IReference<DOUBLE>> spVerticalOffsetReference;
        ctl::ComPtr<wf::IReference<FLOAT>> spZoomFactorReference;

        // The manipulation handler requested that the update be made through a BringIntoViewport operation.
        // This allows the DManip ZoomToRect method to be called instead of SetContentRect to preserve a (0, 0)
        // content offset if possible.
        // Access the current DManip transform and check if it's different from the ScrollViewer's current one.
        IFC(CoreImports::ManipulationHandler_GetPrimaryContentTransform(
            m_hManipulationHandler,
            static_cast<CUIElement*>(pManipulatedElement->GetHandle()),
            TRUE /*fForBringIntoViewport*/,
            manipulationHandlerTranslationX,
            manipulationHandlerTranslationY,
            manipulationHandlerUncompressedZoomFactor,
            manipulationHandlerZoomFactorX,
            manipulationHandlerZoomFactorY));

        if (transformIsValid)
        {
            // When a valid target transform is provided (indicated by transformIsValid==True),
            // jump to it via a ChangeView call.
            ctl::ComPtr<IInspectable> spHorizontalOffset;
            ctl::ComPtr<IInspectable> spVerticalOffset;
            ctl::ComPtr<IInspectable> spZoomFactor;

            // When translationX is positive because of a non-left alignment, the target HorizontalOffset is 0.
            IFC(PropertyValue::CreateFromDouble(DoubleUtil::Max(0.0f, -translationX), &spHorizontalOffset));
            IFC(spHorizontalOffset.As(&spHorizontalOffsetReference));

            // When translationY is positive because of a non-top alignment, the target VerticalOffset is 0.
            IFC(PropertyValue::CreateFromDouble(DoubleUtil::Max(0.0f, -translationY), &spVerticalOffset));
            IFC(spVerticalOffset.As(&spVerticalOffsetReference));

            ASSERT(zoomFactor > 0.0f);
            IFC(PropertyValue::CreateFromSingle(zoomFactor, &spZoomFactor));
            IFC(spZoomFactor.As(&spZoomFactorReference));
        }
        else
        {
            // Adjust manipulation container offsets based on alignments.
            IFC(GetManipulationPrimaryContentTransform(
                pManipulatedElement,
                TRUE  /*inManipulation*/,
                FALSE /*forInitialTransformationAdjustment*/,
                FALSE /*forMargins*/,
                &translationX,
                &translationY,
                &zoomFactor));
        }

#ifdef DM_DEBUG
        if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/,
                L"                   translationX=%f, translationY=%f, zoomFactor=%4.8lf.",
                translationX, translationY, zoomFactor));
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/,
                L"                   manipulationHandlerTranslationX=%f, manipulationHandlerTranslationY=%f, manipulationHandlerUncompressedZoomFactor=%4.8lf.",
                manipulationHandlerTranslationX, manipulationHandlerTranslationY, manipulationHandlerUncompressedZoomFactor));
        }
#endif // DM_DEBUG

        // Only invoke BringIntoViewport when the manipulation container and handler are not in sync.
        if (DoubleUtil::Abs(manipulationHandlerTranslationX - translationX) >= ScrollViewerScrollRoundingToleranceForBringIntoViewport ||
            DoubleUtil::Abs(manipulationHandlerTranslationY - translationY) >= ScrollViewerScrollRoundingToleranceForBringIntoViewport ||
            DoubleUtil::Abs(manipulationHandlerUncompressedZoomFactor - zoomFactor) >= ScrollViewerZoomRoundingToleranceForBringIntoViewport)
        {
            IFC(ChangeViewInternal(
                spHorizontalOffsetReference.Get() /*pHorizontalOffset*/,
                spVerticalOffsetReference.Get()   /*pVerticalOffset*/,
                spZoomFactorReference.Get()       /*pZoomFactor*/,
                nullptr                           /*pOldZoomFactor*/,
                TRUE  /*forceChangeToCurrentView*/,
                FALSE /*adjustWithMandatorySnapPoints*/,
                FALSE /*skipDuringTouchContact*/,
                FALSE /*skipAnimationWhileRunning*/,
                TRUE  /*disableAnimation*/,
                FALSE /*applyAsManip*/,
                transformIsInertiaEnd,
                FALSE /*isForMakeVisible*/,
                &result));
        }
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::HandleManipulationStarting
//
//  Synopsis:
//    Called by the InputManager when a manipulation may start.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::HandleManipulationStarting(
    _In_ UIElement* pManipulatedElement,
    _In_ BOOLEAN wasInDirectManipulation)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IManipulationDataProvider> spProvider;
    DOUBLE offset = 0;

    IFCPTR(pManipulatedElement);

    m_xPixelOffsetRequested = -1;
    m_yPixelOffsetRequested = -1;
    m_contentWidthRequested = -1;
    m_contentHeightRequested = -1;

    // Ensure the m_inertiaEndHorizontalOffset/m_inertiaEndVerticalOffset/m_inertiaEndZoomFactor fields are not being used yet as they are still invalid at this point.
    m_isInertiaEndTransformValid = FALSE;

    m_preDirectManipulationNonVirtualizedTranslationCorrection = 0.0f;

    IFC(GetInnerManipulationDataProvider(TRUE /*isForHorizontalOrientation*/, &spProvider));
    if (spProvider)
    {
#ifdef DM_DEBUG
        if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/,
                L"DMSV[0x%p]:  HandleManipulationStarting - calling UpdateInManipulation with isInManipulation=1, isInLiveTree=1, nonVirtualizingOffset=-1.", this));
        }
#endif // DM_DEBUG

        offset = m_xPixelOffset;
        IFC(spProvider->UpdateInManipulation(IsInManipulation(), TRUE /*isInLiveTree*/, -1.0f /*nonVirtualizingOffset*/));

        // Cache the translation correction in the non-virtualized direction for usage at the end of the manipulation.
        IFC(GetManipulationPrimaryContentTransform(
            pManipulatedElement,
            FALSE /*inManipulation*/,
            FALSE /*forInitialTransformationAdjustment*/,
            FALSE /*forMargins*/,
            NULL /*pTranslationX*/,
            &m_preDirectManipulationNonVirtualizedTranslationCorrection,
            NULL /*pZoomFactor*/));
    }
    else
    {
        offset = m_xOffset;
    }
    if (!wasInDirectManipulation)
    {
        m_preDirectManipulationOffsetX = static_cast<FLOAT>(offset);
    }

    if (spProvider)
    {
        spProvider = nullptr;
    }
    else
    {
        IFC(GetInnerManipulationDataProvider(FALSE /*isForHorizontalOrientation*/, &spProvider));
    }

    if (spProvider)
    {
#ifdef DM_DEBUG
        if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/,
                L"DMSV[0x%p]:  HandleManipulationStarting - calling UpdateInManipulation with isInManipulation=1, isInLiveTree=1, nonVirtualizingOffset=-1.", this));
        }
#endif // DM_DEBUG

        offset = m_yPixelOffset;
        IFC(spProvider->UpdateInManipulation(IsInManipulation(), TRUE /*isInLiveTree*/, -1.0f /*nonVirtualizingOffset*/));

        // Cache the translation correction in the non-virtualized direction for usage at the end of the manipulation.
        IFC(GetManipulationPrimaryContentTransform(
            pManipulatedElement,
            FALSE /*inManipulation*/,
            FALSE /*forInitialTransformationAdjustment*/,
            FALSE /*forMargins*/,
            &m_preDirectManipulationNonVirtualizedTranslationCorrection,
            NULL /*pTranslationY*/,
            NULL /*pZoomFactor*/));
    }
    else
    {
        offset = m_yOffset;
    }
    if (!wasInDirectManipulation)
    {
        m_preDirectManipulationOffsetY = static_cast<FLOAT>(offset);
    }

    if (!wasInDirectManipulation)
    {
        IFC(get_ZoomFactor(&m_preDirectManipulationZoomFactor));
    }

#ifdef DM_DEBUG
    if (m_areViewportConfigurationsInvalid && (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK))
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/,
            L"DMSV[0x%p]:  HandleManipulationStarting m_areViewportConfigurationsInvalid=True, pManipulatedElement=0x%p.",
            this, pManipulatedElement));
    }
#endif // DM_DEBUG

    if (!m_isInConstantVelocityPan && !m_areViewportConfigurationsInvalid)
    {
        // Remember all the variables that affect the computation of the active DM configuration
        IFC(get_HorizontalScrollMode(&m_currentHorizontalScrollMode));
        IFC(get_VerticalScrollMode(&m_currentVerticalScrollMode));
        IFC(get_ZoomMode(&m_currentZoomMode));
        IFC(get_HorizontalScrollBarVisibility(&m_currentHorizontalScrollBarVisibility));
        IFC(get_VerticalScrollBarVisibility(&m_currentVerticalScrollBarVisibility));
        IFC(get_IsHorizontalRailEnabled(&m_currentIsHorizontalRailEnabled));
        IFC(get_IsVerticalRailEnabled(&m_currentIsVerticalRailEnabled));
        IFC(get_IsScrollInertiaEnabled(&m_currentIsScrollInertiaEnabled));
        IFC(get_IsZoomInertiaEnabled(&m_currentIsZoomInertiaEnabled));
        IFC(ComputeHorizontalAlignment(FALSE /*canUseCachedProperties*/, m_currentHorizontalAlignment));
        IFC(ComputeVerticalAlignment(FALSE /*canUseCachedProperties*/, m_currentVerticalAlignment));
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::HandleManipulationDelta
//
//  Synopsis:
//    Called by the InputManager when a manipulation progresses.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::HandleManipulationDelta(
    _In_ UIElement* pManipulatedElement,
    _In_ FLOAT xInertiaEndTranslation,
    _In_ FLOAT yInertiaEndTranslation,
    _In_ FLOAT zInertiaEndFactor,
    _In_ BOOLEAN isInertiaEndTransformValid,
    _In_ BOOLEAN isBringIntoViewportConfigurationActivated,
    _In_ BOOLEAN isLastDelta)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IManipulationDataProvider> spProviderForHorizontalOrientation;
    ctl::ComPtr<IManipulationDataProvider> spProviderForVerticalOrientation;
    BOOLEAN hasInnerManipulationDataProvider = FALSE;
    BOOLEAN isScrollContentPresenterScrollClient = FALSE;
    BOOLEAN hasZoomFactorChanged = FALSE;
    BOOLEAN isBringIntoViewportCalled = FALSE;
    BOOLEAN areScrollOffsetsInSync = FALSE;
    DOUBLE xCurrentOffset = 0;
    DOUBLE yCurrentOffset = 0;
    DOUBLE xNewOffset = 0;
    DOUBLE yNewOffset = 0;
    DOUBLE xNewRoundedOffset = 0;
    DOUBLE yNewRoundedOffset = 0;
    DOUBLE extentX = 0;
    DOUBLE extentY = 0;
    FLOAT translationX = 0.0f;
    FLOAT translationY = 0.0f;
    FLOAT inertiaEndTranslationXCorrection = 0.0f;
    FLOAT inertiaEndTranslationYCorrection = 0.0f;
    FLOAT translationXCorrection = 0.0f;
    FLOAT translationYCorrection = 0.0f;
    FLOAT oldZoomFactor = 0.0f;
    FLOAT newUncompressedZoomFactor = 1.0f;
    FLOAT newZoomFactorX = 1.0f;
    FLOAT newZoomFactorY = 1.0f;
    FLOAT newUncompressedZoomFactorClamped = 1.0f;

#ifdef DM_DEBUG
    if (DMSVv_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE)) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE | DMSVv_DBG) /*traceType*/,
            L"DMSVv[0x%p]: HandleManipulationDelta xInertiaEndTranslation=%f, yInertiaEndTranslation=%f, zInertiaEndFactor=%4.8lf, isInertiaEndTransformValid=%d, isBringIntoViewportConfigurationActivated=%d, isLastDelta=%d.",
            this, xInertiaEndTranslation, yInertiaEndTranslation, zInertiaEndFactor, isInertiaEndTransformValid, isBringIntoViewportConfigurationActivated, isLastDelta));
    }
#endif // DM_DEBUG

    // Batch up any potential ViewChanging/ViewChanged events during HandleManipulationDelta into a single notification
    DelayViewChanging();
    DelayViewChanged();

    IFCPTR(pManipulatedElement);

    ASSERT(m_isInertial || xInertiaEndTranslation == 0.0f);
    ASSERT(m_isInertial || yInertiaEndTranslation == 0.0f);
    ASSERT(m_isInertial || zInertiaEndFactor == 1.0f);
    ASSERT(m_hManipulationHandler);
    ASSERT(IsInManipulation());

    IFC(CoreImports::ManipulationHandler_GetPrimaryContentTransform(
        m_hManipulationHandler,
        static_cast<CUIElement*>(pManipulatedElement->GetHandle()),
        FALSE /*fForBringIntoViewport*/,
        translationX,
        translationY,
        newUncompressedZoomFactor,
        newZoomFactorX,
        newZoomFactorY));

    if (!m_isDirectManipulationZoomFactorChangeIgnored)
    {
        IFC(get_ZoomFactor(&oldZoomFactor));
        hasZoomFactorChanged =
            DoubleUtil::Abs(oldZoomFactor - newUncompressedZoomFactor) > ScrollViewerZoomRoundingTolerance || (isLastDelta && m_isInDirectManipulationZoom && !DoubleUtil::AreClose(oldZoomFactor, newUncompressedZoomFactor));
    }

#ifdef DM_DEBUG
    if (DMSVv_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE)) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE | DMSVv_DBG) /*traceType*/,
            L"DMSVv[0x%p]: HandleManipulationDelta hasZoomFactorChanged=%d, m_isDirectManipulationZoomFactorChangeIgnored=%d.",
            this, hasZoomFactorChanged, m_isDirectManipulationZoomFactorChangeIgnored));
    }
#endif // DM_DEBUG

    if (hasZoomFactorChanged)
    {
        // The zoom factor has changed during this manipulation. Allow the potential inner virtualizing panel to be aware of this.
        m_isInDirectManipulationZoom = TRUE;
        m_isDirectManipulationZoomFactorChange = TRUE;

        IFC(ZoomToFactorInternal(newUncompressedZoomFactor, FALSE /*delayAndFlushViewChanged*/, nullptr /*pZoomChanged*/));
        IFC(get_ZoomFactor(&newUncompressedZoomFactorClamped)); // We restrict value to [MinZoomFactor, MaxZoomFactor].

#ifdef DM_DEBUG
        if (DMSVv_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE)) == S_OK)
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE | DMSVv_DBG) /*traceType*/,
                L"DMSVv[0x%p]: HandleManipulationDelta updating old m_contentWidthRequested=%f, m_contentHeightRequested=%f.",
                this, m_contentWidthRequested, m_contentHeightRequested));
        }
#endif // DM_DEBUG

    }

    IFC(GetInnerManipulationDataProvider(TRUE /*isForHorizontalOrientation*/, &spProviderForHorizontalOrientation));
    if (!spProviderForHorizontalOrientation)
    {
        IFC(GetInnerManipulationDataProvider(FALSE /*isForHorizontalOrientation*/, &spProviderForVerticalOrientation));
    }
    hasInnerManipulationDataProvider = spProviderForHorizontalOrientation || spProviderForVerticalOrientation;

    if (hasZoomFactorChanged)
    {
        // Expected adjust width is based on the clamped zoom factor.
        if (m_contentWidthRequested == -1)
        {
            IFC(ComputePixelExtentWidth(false /*ignoreZoomFactor*/, spProviderForHorizontalOrientation.Get(), &extentX));
            if (!spProviderForHorizontalOrientation)
            {
                extentX *= newUncompressedZoomFactorClamped / oldZoomFactor;
            }
            m_contentWidthRequested = extentX;
        }
        else
        {
            m_contentWidthRequested *= newUncompressedZoomFactorClamped / oldZoomFactor;
        }
#ifdef DM_DEBUG
        if (DMSVv_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE)) == S_OK)
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE | DMSVv_DBG) /*traceType*/,
                L"DMSVv[0x%p]: HandleManipulationDelta new m_contentWidthRequested=%f.", this, m_contentWidthRequested));
        }
#endif // DM_DEBUG

    }

    // No need to evaluate inertiaEndTranslationXCorrection when working against an IManipulationDataProvider implementation
    if (m_isInertial && !hasInnerManipulationDataProvider)
    {
        IFC(ComputeTranslationXCorrection(
            TRUE  /*inManipulation*/,
            FALSE /*forInitialTransformationAdjustment*/,
            TRUE  /*adjustDimensions*/,
            NULL  /*pProvider*/,
            0.0   /*leftMargin*/,
            m_contentWidthRequested * zInertiaEndFactor / newUncompressedZoomFactorClamped,
            zInertiaEndFactor,
            &inertiaEndTranslationXCorrection));
    }

    IFC(ComputeTranslationXCorrection(
        TRUE  /*inManipulation*/,
        FALSE /*forInitialTransformationAdjustment*/,
        TRUE  /*adjustDimensions*/,
        spProviderForHorizontalOrientation.Get() /*pProvider*/,
        0.0   /*leftMargin*/,
        m_contentWidthRequested,
        newUncompressedZoomFactorClamped,
        &translationXCorrection));

    ASSERT(spProviderForHorizontalOrientation || m_xOffset == m_xPixelOffset);

    xCurrentOffset = (m_xPixelOffsetRequested == -1) ? m_xPixelOffset : m_xPixelOffsetRequested;
    if (spProviderForHorizontalOrientation && hasZoomFactorChanged)
    {
        xCurrentOffset *= newUncompressedZoomFactorClamped / oldZoomFactor;
    }

    if (hasZoomFactorChanged)
    {
        // Expected adjust height is based on the clamped zoom factor.
        if (m_contentHeightRequested == -1)
        {
            IFC(ComputePixelExtentHeight(false /*ignoreZoomFactor*/, spProviderForVerticalOrientation.Get(), &extentY));
            if (!spProviderForVerticalOrientation)
            {
                extentY *= newUncompressedZoomFactorClamped / oldZoomFactor;
            }
            m_contentHeightRequested = extentY;
        }
        else
        {
            m_contentHeightRequested *= newUncompressedZoomFactorClamped / oldZoomFactor;
        }
#ifdef DM_DEBUG
        if (DMSVv_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE)) == S_OK)
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE | DMSVv_DBG) /*traceType*/,
                L"DMSVv[0x%p]: HandleManipulationDelta new m_contentHeightRequested=%f.", this, m_contentHeightRequested));
        }
#endif // DM_DEBUG

    }

    // No need to evaluate inertiaEndTranslationYCorrection when working against an IManipulationDataProvider implementation
    if (m_isInertial && !hasInnerManipulationDataProvider)
    {
        IFC(ComputeTranslationYCorrection(
            TRUE  /*inManipulation*/,
            FALSE /*forInitialTransformationAdjustment*/,
            TRUE  /*adjustDimensions*/,
            NULL  /*pProvider*/,
            0.0   /*topMargin*/,
            m_contentHeightRequested * zInertiaEndFactor / newUncompressedZoomFactorClamped,
            zInertiaEndFactor,
            &inertiaEndTranslationYCorrection));
    }

    IFC(ComputeTranslationYCorrection(
        TRUE  /*inManipulation*/,
        FALSE /*forInitialTransformationAdjustment*/,
        TRUE  /*adjustDimensions*/,
        spProviderForVerticalOrientation.Get() /*pProvider*/,
        0.0   /*topMargin*/,
        m_contentHeightRequested,
        newUncompressedZoomFactorClamped,
        &translationYCorrection));

    ASSERT(spProviderForVerticalOrientation || m_yOffset == m_yPixelOffset);

    yCurrentOffset = (m_yPixelOffsetRequested == -1) ? m_yPixelOffset : m_yPixelOffsetRequested;
    if (spProviderForVerticalOrientation && hasZoomFactorChanged)
    {
        yCurrentOffset *= newUncompressedZoomFactorClamped / oldZoomFactor;
    }

    // Because of the lack of a IScrollAndZoomInfo::SetScrollOffsetsAndZoomFactor(Double horizontalOffset, Double verticalOffset, Double zoomFactor) API,
    // we need to ensure that the IScrollInfo implementation updates its knowledge of the extents based on the final zoom factor. Otherwise, the imminent
    // calls to ScrollByPixelDelta may clamp the offsets based on an old zoom factor. This situation occurs when the manipulation is very quick or the UI
    // thread is too busy to execute a ScrollContentPresenter::MeasureOverride() and ScrollViewer::InvalidateScrollInfo().
    // A layout is forced only when there is an uncommitted zoom factor change and the ScrollContentPresenter is the IScrollInfo implementer (other
    // implementations force an update in UpdateInManipulation).
    if (isLastDelta && m_trElementScrollContentPresenter && (m_contentWidthRequested != -1 || m_contentHeightRequested != -1))
    {
        IFC(IsScrollContentPresenterScrollClient(isScrollContentPresenterScrollClient));
        if (isScrollContentPresenterScrollClient)
        {
            // Because the layout update is based on the final zoom factor and non-final offsets,
            // some unexpected offset changes may occur that must not be propagated to the manipulation handler.
            m_isOffsetChangeIgnored = TRUE;

#ifdef DM_DEBUG
            if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
            {
                IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/,
                    L"DMSV[0x%p]:  HandleManipulationDelta before UpdateLayout m_xPixelOffsetRequested=%f m_xPixelOffset=%f translationX=%f.",
                    this, m_xPixelOffsetRequested, m_xPixelOffset, translationX));
            }
#endif // DM_DEBUG

            IFC(m_trElementScrollContentPresenter.Cast<ScrollContentPresenter>()->UpdateLayout());

            // Now that the synchronous layout update completed, unexpected offset changes must be propagated to the manipulation handler again.
            m_isOffsetChangeIgnored = FALSE;

#ifdef DM_DEBUG
            if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
            {
                IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/,
                    L"DMSV[0x%p]:  HandleManipulationDelta after UpdateLayout m_xPixelOffsetRequested=%f m_xPixelOffset=%f.",
                    this, m_xPixelOffsetRequested, m_xPixelOffset));
            }
#endif // DM_DEBUG

            // Reevalutate current offsets since they may have changed during the layout
            xCurrentOffset = (m_xPixelOffsetRequested == -1) ? m_xPixelOffset : m_xPixelOffsetRequested;
            yCurrentOffset = (m_yPixelOffsetRequested == -1) ? m_yPixelOffset : m_yPixelOffsetRequested;

            IFC(m_trElementScrollContentPresenter.Cast<ScrollContentPresenter>()->AreScrollOffsetsInSync(areScrollOffsetsInSync));
            if (!areScrollOffsetsInSync)
            {
                IFC(SynchronizeScrollOffsets());
            }
        }
    }

    xNewOffset = static_cast<DOUBLE>(-translationX);
    yNewOffset = static_cast<DOUBLE>(-translationY);

    if (isLastDelta)
    {
        // Execute potential special handling of last delta in current manipulation
        if (isBringIntoViewportConfigurationActivated)
        {
            IFC(HandleManipulationLastDelta(pManipulatedElement, xNewOffset, yNewOffset, isBringIntoViewportCalled));
        }
        else
        {
            xNewRoundedOffset = xNewOffset;
            yNewRoundedOffset = yNewOffset;
            IFC(HandleManipulationLastDelta(pManipulatedElement, xNewRoundedOffset, yNewRoundedOffset, isBringIntoViewportCalled));
        }
    }

    if (isLastDelta && !isBringIntoViewportCalled && !isBringIntoViewportConfigurationActivated)
    {
        xNewOffset = xNewRoundedOffset;
        yNewOffset = yNewRoundedOffset;
    }

    if (xNewOffset != xCurrentOffset)
    {
        IFC(ScrollByPixelDelta(TRUE /*isForHorizontalOrientation*/, xNewOffset, xNewOffset - xCurrentOffset, TRUE /*isDManipInput*/));
    }

    if (yNewOffset != yCurrentOffset)
    {
        IFC(ScrollByPixelDelta(FALSE /*isForHorizontalOrientation*/, yNewOffset, yNewOffset - yCurrentOffset, TRUE /*isDManipInput*/));
    }

    // No need to evaluate m_inertiaEndXXX fields when working against an IManipulationDataProvider implementation
    if (m_isInertial && !hasInnerManipulationDataProvider)
    {
        m_isInertiaEndTransformValid = TRUE;

        if (isLastDelta && !isBringIntoViewportCalled)
        {
            m_inertiaEndHorizontalOffset = (m_xPixelOffsetRequested == -1) ? m_xPixelOffset : m_xPixelOffsetRequested;
            m_inertiaEndVerticalOffset = (m_yPixelOffsetRequested == -1) ? m_yPixelOffset : m_yPixelOffsetRequested;

            // Use the latest ZoomFactor property value whether ZoomMode is Enabled or Disabled, and whether this is a ChangeView operation or not.
            // The ScrollViewerViewChangingEventArgs.FinalView.ZoomFactor will reflect the correct value, even for example when ZoomToFactor is called during a pan.
            IFC(get_ZoomFactor(&m_inertiaEndZoomFactor));
        }
        else
        {
            if (m_currentHorizontalScrollBarVisibility == xaml_controls::ScrollBarVisibility_Disabled)
            {
                // When HorizontalScrollBarVisibility is Disabled, the final HorizontalOffset after inertia is necessarily 0
                // since a final BringIntoViewport call is made to animate the HorizontalOffset to 0.
                m_inertiaEndHorizontalOffset = 0.0f;
            }
            else
            {
                if (isInertiaEndTransformValid)
                {
                    m_inertiaEndHorizontalOffset = MAX(0.0f, inertiaEndTranslationXCorrection - xInertiaEndTranslation);
                    if (m_inertiaEndHorizontalOffset < ScrollViewerScrollRoundingTolerance)
                    {
                        m_inertiaEndHorizontalOffset = 0.0f;
                    }
                }
                else
                {
                    m_isInertiaEndTransformValid = FALSE;
                }
            }

            if (m_currentVerticalScrollBarVisibility == xaml_controls::ScrollBarVisibility_Disabled)
            {
                // When VerticalScrollBarVisibility is Disabled, the final VerticalOffset after inertia is necessarily 0
                // since a final BringIntoViewport call is made to animate the VerticalOffset to 0.
                m_inertiaEndVerticalOffset = 0.0f;
            }
            else
            {
                if (isInertiaEndTransformValid)
                {
                    m_inertiaEndVerticalOffset = MAX(0.0f, inertiaEndTranslationYCorrection - yInertiaEndTranslation);
                    if (m_inertiaEndVerticalOffset < ScrollViewerScrollRoundingTolerance)
                    {
                        m_inertiaEndVerticalOffset = 0.0f;
                    }
                }
                else
                {
                    m_isInertiaEndTransformValid = FALSE;
                }
            }

            if (m_currentZoomMode != xaml_controls::ZoomMode_Disabled || isBringIntoViewportConfigurationActivated)
            {
                if (isInertiaEndTransformValid)
                {
                    // Consume the final zoom factor value provided by DManip.
                    m_inertiaEndZoomFactor = zInertiaEndFactor;
                }
                else
                {
                    m_isInertiaEndTransformValid = FALSE;
                }
            }
            else
            {
                // When ZoomMode is Disabled and the zoom factor is not being changed by a ChangeView operation, the ZoomFactor property remains unchanged. Consume its current value.
                IFC(get_ZoomFactor(&m_inertiaEndZoomFactor));
            }
        }
    }

    if (isLastDelta && !isBringIntoViewportCalled)
    {
        m_unboundHorizontalOffset = (m_xPixelOffsetRequested == -1) ? m_xPixelOffset : m_xPixelOffsetRequested;
        m_unboundVerticalOffset = (m_yPixelOffsetRequested == -1) ? m_yPixelOffset : m_yPixelOffsetRequested;
    }
    else
    {
        m_unboundHorizontalOffset = translationXCorrection - translationX;
        m_unboundVerticalOffset = translationYCorrection - translationY;
    }

Cleanup:
    m_isDirectManipulationZoomFactorChange = FALSE;
    m_isOffsetChangeIgnored = FALSE;

    // Raise any potentially delayed ViewChanging/ViewChanged notifications
    hr = FlushViewChanging(hr);
    RRETURN(FlushViewChanged(hr));
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::HandleManipulationLastDelta
//
//  Synopsis:
//    Called when a DirectManipulation manipulation raises its last delta
//    notification. Calls BringIntoViewport if the final layout is not valid
//    for the ScrollViewer, in order to bring the content into a valid location.
//    Sets isBringIntoViewportCalled to True only when a call to
//    BringIntoViewport was made.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::HandleManipulationLastDelta(
    _In_ UIElement* pManipulatedElement,
    _In_ DOUBLE xNewOffset,
    _In_ DOUBLE yNewOffset,
    _Out_ BOOLEAN& isBringIntoViewportCalled)
{
    HRESULT hr = S_OK;
    BOOLEAN isScrollContentPresenterScrollClient = FALSE;
    BOOLEAN animateHorizontally = FALSE;
    BOOLEAN animateVertically = FALSE;
    FLOAT zoomFactor = 1.0f;
    DOUBLE extent = 0.0f;
    DOUBLE viewport = 0.0f;
    XRECTF bounds = { 0, 0, 0, 0 };

    isBringIntoViewportCalled = FALSE;

    IFCPTR(pManipulatedElement);

    ASSERT(m_hManipulationHandler);
    ASSERT(IsInManipulation());

    // Determine whether the ScrollContentPresenter is the IScrollInfo implementer or not
    IFC(IsScrollContentPresenterScrollClient(isScrollContentPresenterScrollClient));
    if (!isScrollContentPresenterScrollClient)
    {
        // No animation needs to occur in a custom IScrollInfo implementation case (VSP, etc...)
        goto Cleanup;
    }

    IFC(get_ZoomFactor(&zoomFactor));

#ifdef DM_DEBUG
    if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/,
            L"DMSV[0x%p]:  HandleManipulationLastDelta - entry. xNewOffset=%f, yNewOffset=%f, zoomFactor=%4.8lf.",
            this, xNewOffset, yNewOffset, zoomFactor));
    }
#endif // DM_DEBUG

    if (zoomFactor < 1.0f)
    {
        // No correction is ever needed when the zoom factor is smaller than 1.
        goto Cleanup;
    }

    if (m_currentHorizontalScrollBarVisibility == xaml_controls::ScrollBarVisibility_Disabled &&
        DoubleUtil::Abs(xNewOffset) > ScrollViewerScrollRoundingTolerance)
    {
        IFC(get_ExtentWidth(&extent));
        IFC(get_ViewportWidth(&viewport));

        if (extent > viewport)
        {
            animateHorizontally = TRUE;
            bounds.Width = static_cast<XFLOAT>(viewport / zoomFactor);
        }
    }

    if (m_currentVerticalScrollBarVisibility == xaml_controls::ScrollBarVisibility_Disabled &&
        DoubleUtil::Abs(yNewOffset) > ScrollViewerScrollRoundingTolerance)
    {
        IFC(get_ExtentHeight(&extent));
        IFC(get_ViewportHeight(&viewport));

        if (extent > viewport)
        {
            animateVertically = TRUE;
            bounds.Height = static_cast<XFLOAT>(viewport / zoomFactor);
        }
    }

    if (animateHorizontally || animateVertically)
    {
        if (!animateHorizontally)
        {
            IFC(get_ViewportWidth(&viewport));

            bounds.X = static_cast<XFLOAT>(xNewOffset / zoomFactor);
            bounds.Width = static_cast<XFLOAT>(viewport / zoomFactor);
        }

        if (!animateVertically)
        {
            IFC(get_ViewportHeight(&viewport));

            bounds.Y = static_cast<XFLOAT>(yNewOffset / zoomFactor);
            bounds.Height = static_cast<XFLOAT>(viewport / zoomFactor);
        }

#ifdef DM_DEBUG
        if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/,
                L"DMSV[0x%p]:  HandleManipulationLastDelta - calling BringIntoViewportInternal(X=%f, Y=%f, W=%f, H=%f, transformIsValid=TRUE, skipDuringTouchContact=FALSE, skipAnimationWhileRunning=FALSE, animate=%d, applyAsManip=TRUE, isForMakeVisible=FALSE).",
                this, bounds.X, bounds.Y, bounds.Width, bounds.Height, m_currentIsScrollInertiaEnabled));
        }
#endif // DM_DEBUG

        ASSERT(bounds.Width > 0.0f);
        ASSERT(bounds.Height > 0.0f);
        IFC(BringIntoViewportInternal(
            bounds,
            animateHorizontally ? 0.0f : static_cast<FLOAT>(-xNewOffset), /*translateX*/
            animateVertically ? 0.0f : static_cast<FLOAT>(-yNewOffset), /*translateY*/
            zoomFactor,
            TRUE  /*transformIsValid*/,
            FALSE /*skipDuringTouchContact*/,
            FALSE /*skipAnimationWhileRunning*/,
            m_currentIsScrollInertiaEnabled && IsAnimationEnabled() /*animate*/,
            TRUE /*applyAsManip*/,
            FALSE /*isForMakeVisible*/,
            &isBringIntoViewportCalled));
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::HandleManipulationCompleted
//
//  Synopsis:
//    Called by the InputManager when a manipulation has completed.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::HandleManipulationCompleted(
    _In_ UIElement* pManipulatedElement,
    _In_ BOOLEAN wasInDirectManipulationZoom,
    _In_ FLOAT xCumulativeTranslation,
    _In_ FLOAT yCumulativeTranslation)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IManipulationDataProvider> spProvider;
    ctl::ComPtr<IUIElement> spElement;
    BOOLEAN isInLiveTree = FALSE;
    BOOLEAN isForHorizontalOrientation = TRUE;
    BOOLEAN isScrollContentPresenterScrollClient = FALSE;
    DOUBLE nonVirtualizingOffset = -1.0;
    DOUBLE virtualizingPixelOffset = -1.0;
    DOUBLE virtualizingLogicalOffset = -1.0;

#ifdef DM_DEBUG
    if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/,
            L"DMSV[0x%p]:  HandleManipulationCompleted - entry. wasInDirectManipulationZoom=%d, xCumulativeTranslation=%f, yCumulativeTranslation=%f.",
            this, wasInDirectManipulationZoom, xCumulativeTranslation, yCumulativeTranslation));
    }
#endif // DM_DEBUG

    // Prevent the usage of the m_inertiaEndHorizontalOffset/m_inertiaEndVerticalOffset/m_inertiaEndZoomFactor fields as they become invalid at this point.
    m_isInertiaEndTransformValid = FALSE;

    // Reset the pointed element
    IFC(put_PointedElement(NULL));

    if (!m_isManipulationHandlerInterestedInNotifications)
    {
        IFC(UnhookScrollSnapPointsInfoEvents(TRUE /*isForHorizontalSnapPoints*/));
        IFC(UnhookScrollSnapPointsInfoEvents(FALSE /*isForHorizontalSnapPoints*/));
    }

    isInLiveTree = IsInLiveTree();

    IFC(GetInnerManipulationDataProvider(TRUE /*isForHorizontalOrientation*/, &spProvider));
    if (spProvider)
    {
        if (wasInDirectManipulationZoom)
        {
            nonVirtualizingOffset = m_preDirectManipulationOffsetY - yCumulativeTranslation - m_preDirectManipulationNonVirtualizedTranslationCorrection;
            virtualizingPixelOffset = m_preDirectManipulationOffsetX - xCumulativeTranslation;
        }
    }
    else
    {
        IFC(GetInnerManipulationDataProvider(FALSE /*isForHorizontalOrientation*/, &spProvider));
        if (spProvider)
        {
            isForHorizontalOrientation = FALSE;
            if (wasInDirectManipulationZoom)
            {
                nonVirtualizingOffset = m_preDirectManipulationOffsetX - xCumulativeTranslation - m_preDirectManipulationNonVirtualizedTranslationCorrection;
                virtualizingPixelOffset = m_preDirectManipulationOffsetY - yCumulativeTranslation;
            }
        }
    }
    if (spProvider)
    {
#ifdef DM_DEBUG
        if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/,
                L"DMSV[0x%p]:  HandleManipulationCompleted - calling UpdateInManipulation with isInManipulation=%d, isInLiveTree=%d, nonVirtualizingOffset=%f.",
                this, IsInManipulation(), isInLiveTree, nonVirtualizingOffset));
        }
#endif // DM_DEBUG

        IFC(spProvider->UpdateInManipulation(IsInManipulation(), isInLiveTree, nonVirtualizingOffset));

        if (wasInDirectManipulationZoom &&
            DoubleUtil::Abs(virtualizingPixelOffset - (isForHorizontalOrientation ? m_xPixelOffset : m_yPixelOffset)) > ScrollViewerScrollRoundingTolerance)
        {
            // Because the zoom factor changes were not taken into account by the virtualized panel during the manipulation, the resulting offset might have been
            // clamped by the original zoom factor. Now that the virtualized panel did an UpdateLayout() during its UpdateInManipulation processing, the new zoom
            // factor is finally taken into account and a move to the desired offset must be attempted.
            if (isForHorizontalOrientation)
            {
                if (DoubleUtil::Abs(m_contentWidthRequested - m_xPixelExtent) / m_xPixelExtent < ScrollViewerZoomExtentRoundingTolerance)
                {
                    DOUBLE pixelDelta = virtualizingPixelOffset - m_xPixelOffset;
                    IFC(spProvider->ComputeLogicalOffset(
                        TRUE /*isForHorizontalOrientation*/,
                        pixelDelta /*pixelDelta*/,
                        virtualizingLogicalOffset));
                    IFC(ScrollToHorizontalOffsetInternal(virtualizingLogicalOffset));
                    IFC(spProvider.As(&spElement));
                    IFC(spElement->UpdateLayout());
                }
            }
            else
            {
                if (DoubleUtil::Abs(m_contentHeightRequested - m_yPixelExtent) / m_yPixelExtent < ScrollViewerZoomExtentRoundingTolerance)
                {
                    DOUBLE pixelDelta = virtualizingPixelOffset - m_yPixelOffset;
                    IFC(spProvider->ComputeLogicalOffset(
                        FALSE /*isForHorizontalOrientation*/,
                        pixelDelta /*pixelDelta*/,
                        virtualizingLogicalOffset));
                    IFC(ScrollToVerticalOffsetInternal(virtualizingLogicalOffset));
                    IFC(spProvider.As(&spElement));
                    IFC(spElement->UpdateLayout());
                }
            }
        }
    }
    else
    {
        // Determine whether the ScrollContentPresenter is the IScrollInfo implementer or not
        IFC(IsScrollContentPresenterScrollClient(isScrollContentPresenterScrollClient));
        if (isScrollContentPresenterScrollClient)
        {
            // This flag which is only used when ScrollContentPresenter is the IScrollInfo implementer
            // will be reset to False when the ScrollContentPresenter calls PostDirectManipulationLayoutRefreshed().
            m_isInDirectManipulationCompletion = TRUE;
        }
        else
        {
            // The content is a custom IScrollInfo implementation which does not implement IManipulationDataProvider.
            // It's therefore a TextBoxView which does not re-layout after a manipulation. Instead, the CTextBoxView::OnDirectManipulationCompleted()
            // method which is about to be invoked by DirectManipulationStateChangeHandler::NotifyStateChange is flipping a flag which differentiates
            // in-manipulation versus not-in-manipulation behaviors. Since no layout is expected, set the InputManager's IsPrimaryContentLayoutRefreshedAfterCompletion
            // flag right away.
            IFC(NotifyLayoutRefreshed());
        }
    }

    if (isInLiveTree && m_trElementScrollContentPresenter)
    {
        IFC(m_trElementScrollContentPresenter.Cast<ScrollContentPresenter>()->InvalidateMeasure());
    }

    if (m_isCanManipulateElementsInvalid)
    {
        IFC(OnManipulatabilityAffectingPropertyChanged(
            NULL  /*pIsInLiveTree*/,
            TRUE  /*isCachedPropertyChanged*/,
            FALSE /*isContentChanged*/,
            TRUE  /*isAffectingConfigurations*/,
            FALSE /*isAffectingTouchConfiguration*/));
    }

    if (m_isScrollBarIgnoringUserInputInvalid)
    {
        // At least one ScrollBar's update was delayed. Update both.
        IFC(RefreshScrollBarIsIgnoringUserInput(TRUE /*isForHorizontalOrientation*/));
        IFC(RefreshScrollBarIsIgnoringUserInput(FALSE /*isForHorizontalOrientation*/));
        m_isScrollBarIgnoringUserInputInvalid = FALSE;
    }

    if (m_areViewportConfigurationsInvalid)
    {
        IFC(OnViewportConfigurationsAffectingPropertyChanged());
    }

    // Switch back to non-Intermediate mode since the control leaves the current manipulation
    // The latest ManipulationDelta might trigger a view change later. Only raise the final ViewChanged
    // if there are no pending changes in the offsets.
#ifdef DM_DEBUG
    if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/,
            L"DMSV[0x%p]:  HandleManipulationCompleted - calling LeaveIntermediateViewChangedMode(raiseFinalViewChanged=%d), m_xPixelOffsetRequested=%f, m_yPixelOffsetRequested=%f.",
            this, m_xPixelOffsetRequested == -1 && m_yPixelOffsetRequested == -1, m_xPixelOffsetRequested, m_yPixelOffsetRequested));
    }
#endif // DM_DEBUG

    IFC(LeaveIntermediateViewChangedMode(m_xPixelOffsetRequested == -1 &&
        m_yPixelOffsetRequested == -1 /*raiseFinalViewChanged*/));

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::GetDManipElementAndProperty
//
//  Synopsis:
//    Retrieves the DirectManipulation element and property associated with this element
//    and the target property that we want to animate.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::GetDManipElementAndProperty(
    _In_ KnownPropertyIndex targetProperty,
    _Outptr_ CDependencyObject** ppDManipElement,
    _Out_ XUINT32 *pDManipProperty)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IUIElement> spContentUIElement;

    IFCPTR(ppDManipElement);
    *ppDManipElement = NULL;
    IFCPTR(pDManipProperty);
    *pDManipProperty = static_cast<XUINT32>(XcpDMPropertyNone);

    IFC(GetContentUIElement(&spContentUIElement));

    if (spContentUIElement)
    {
        ASSERT(targetProperty == KnownPropertyIndex::ScrollViewer_HorizontalOffset ||
            targetProperty == KnownPropertyIndex::ScrollViewer_VerticalOffset ||
            targetProperty == KnownPropertyIndex::ScrollViewer_ZoomFactor);

        *ppDManipElement = spContentUIElement.Cast<UIElement>()->GetHandle();
        AddRefInterface(*ppDManipElement);

        switch (targetProperty)
        {
            case KnownPropertyIndex::ScrollViewer_HorizontalOffset:
                *pDManipProperty = static_cast<XUINT32>(XcpDMPropertyTranslationX);
                break;

            case KnownPropertyIndex::ScrollViewer_VerticalOffset:
                *pDManipProperty = static_cast<XUINT32>(XcpDMPropertyTranslationY);
                break;

            case KnownPropertyIndex::ScrollViewer_ZoomFactor:
                *pDManipProperty = static_cast<XUINT32>(XcpDMPropertyZoom);
                break;
        }
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::GetDManipElement
//
//  Synopsis:
//    Retrieves the DirectManipulation element associated with this element.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::GetDManipElement(
    _Outptr_ CDependencyObject** ppDManipElement)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IUIElement> spContentUIElement;

    IFCPTR(ppDManipElement);
    *ppDManipElement = NULL;

    IFC(GetContentUIElement(&spContentUIElement));

    if (spContentUIElement)
    {
        *ppDManipElement = spContentUIElement.Cast<UIElement>()->GetHandle();
        AddRefInterface(*ppDManipElement);
    }

Cleanup:
    RRETURN(hr);
}

// Returns the current view from the manipulation handler.
// The uncompressed zoom factor is returned.
_Check_return_ HRESULT ScrollViewer::GetDManipView(
    _Out_ DOUBLE* pHorizontalOffset,
    _Out_ DOUBLE* pVerticalOffset,
    _Out_ FLOAT* pZoomFactor)
{
    *pHorizontalOffset = 0.0;
    *pVerticalOffset = 0.0;
    *pZoomFactor = 1.0f;

    if (m_hManipulationHandler)
    {
        ctl::ComPtr<xaml::IUIElement> spContentUIElement;

        IFC_RETURN(GetContentUIElement(&spContentUIElement));
        if (spContentUIElement)
        {
            FLOAT translationX = 0.0f;
            FLOAT translationY = 0.0f;
            FLOAT translationCorrectionX = 0.0f;
            FLOAT translationCorrectionY = 0.0f;
            FLOAT uncompressedZoomFactor = 1.0f;
            FLOAT zoomFactorX = 1.0f;
            FLOAT zoomFactorY = 1.0f;

            // Retrieve the DManip current transform from the DManip Service.
            IFC_RETURN(CoreImports::ManipulationHandler_GetPrimaryContentTransform(
                m_hManipulationHandler,
                static_cast<CUIElement*>(spContentUIElement.Cast<UIElement>()->GetHandle()),
                TRUE /*fForBringIntoViewport*/,
                translationX,
                translationY,
                uncompressedZoomFactor,
                zoomFactorX,
                zoomFactorY));

            // Figure out if this ScrollViewer holds a IManipulationDataProvider.
            ctl::ComPtr<IManipulationDataProvider> spProviderForHorizontalOrientation;
            ctl::ComPtr<IManipulationDataProvider> spProviderForVerticalOrientation;
            IFC_RETURN(GetInnerManipulationDataProvider(TRUE /*isForHorizontalOrientation*/, &spProviderForHorizontalOrientation));
            IFC_RETURN(GetInnerManipulationDataProvider(FALSE /*isForHorizontalOrientation*/, &spProviderForVerticalOrientation));

            // Compute the translation corrections due to center or right/bottom alignments.
            IFC_RETURN(ComputeTranslationXCorrection(
                IsInManipulation() /*inManipulation*/,
                FALSE /*forInitialTransformationAdjustment*/,
                FALSE /*adjustDimensions*/,
                spProviderForHorizontalOrientation.Get(),
                0.0   /*leftMargin*/,
                -1.0  /*extent*/,
                uncompressedZoomFactor,
                &translationCorrectionX));

            IFC_RETURN(ComputeTranslationYCorrection(
                IsInManipulation() /*inManipulation*/,
                FALSE /*forInitialTransformationAdjustment*/,
                FALSE /*adjustDimensions*/,
                spProviderForVerticalOrientation.Get(),
                0.0   /*topMargin*/,
                -1.0  /*extent*/,
                uncompressedZoomFactor,
                &translationCorrectionY));

            *pHorizontalOffset = static_cast<DOUBLE>(translationCorrectionX - translationX);
            *pVerticalOffset = static_cast<DOUBLE>(translationCorrectionY - translationY);
            *pZoomFactor = uncompressedZoomFactor;
        }
    }

#ifdef DM_DEBUG
    if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/, L"DMSV[0x%p]:  GetDManipView - horizontalOffset=%f, verticalOffset=%f, zoomFactor=%f.",
            this, *pHorizontalOffset, *pVerticalOffset, *pZoomFactor));
    }
#endif // DM_DEBUG

    return S_OK;
}

// Returns the target view for the current manipulation.
_Check_return_ HRESULT ScrollViewer::GetTargetView(
    _Out_ DOUBLE* pTargetHorizontalOffset,
    _Out_ DOUBLE* pTargetVerticalOffset,
    _Out_ FLOAT* pTargetZoomFactor)
{
    ASSERT(IsInManipulation());

    // Return the target view of the latest ChangeView request if set.
    if (m_targetChangeViewHorizontalOffset != -1.0)
    {
        // No ViewChanging event was raised since the last ChangeView request.
        ASSERT(m_targetChangeViewVerticalOffset != -1.0);
        ASSERT(m_targetChangeViewZoomFactor != -1.0f);
        *pTargetHorizontalOffset = m_targetChangeViewHorizontalOffset;
        *pTargetVerticalOffset = m_targetChangeViewVerticalOffset;
        *pTargetZoomFactor = m_targetChangeViewZoomFactor;
    }
    // Else return the end-of-inertia view if set.
    else if (m_isInertial && m_isInertiaEndTransformValid)
    {
        *pTargetHorizontalOffset = m_inertiaEndHorizontalOffset;
        *pTargetVerticalOffset = m_inertiaEndVerticalOffset;
        *pTargetZoomFactor = m_inertiaEndZoomFactor;
    }
    // Else no target view is available.
    else
    {
        *pTargetHorizontalOffset = -1.0;
        *pTargetVerticalOffset = -1.0;
        *pTargetZoomFactor = -1.0f;
    }

#ifdef DM_DEBUG
    if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/, L"DMSV[0x%p]:  GetTargetView - targetHorizontalOffset=%f, targetVerticalOffset=%f, targetZoomFactor=%f.",
            this, *pTargetHorizontalOffset, *pTargetVerticalOffset, *pTargetZoomFactor));
    }
#endif // DM_DEBUG

    return S_OK;
}

// Stops the current manipulation if it's in inertia phase.
_Check_return_ HRESULT ScrollViewer::StopInertialManipulation()
{
#ifdef DM_DEBUG
    if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/, L"DMSV[0x%p]:  StopInertialManipulation - m_isInertial=%d.", this, m_isInertial));
    }
#endif // DM_DEBUG

    ASSERT(IsInManipulation());

    if (m_isInertial && m_hManipulationHandler)
    {
        ctl::ComPtr<xaml::IUIElement> spContentUIElement;
        IFC_RETURN(GetContentUIElement(&spContentUIElement));
        if (spContentUIElement)
        {
            bool isHandled = false;
            CUIDMContainerHandler* pUIDMContainerHandler = static_cast<CUIDMContainerHandler*>(m_hManipulationHandler);
            IFC_RETURN(pUIDMContainerHandler->StopInertialViewport(static_cast<CUIElement*>(spContentUIElement.Cast<UIElement>()->GetHandle()), &isHandled));
            if (isHandled)
            {
                // Even though DManip stopped the current inertial manipulation, this ScrollViewer is still in an active manipulation state, ManipulationDelta.
                // m_isInertial is still set to True as well. The status change to Ready & state change to ManipulationCompleted are going to be handled asynchronously.
                // In the meantime, if the primary content transform does change unexpectedly (i.e. without DManip request), OnPrimaryContentTransformChanged
                // can use IsInUnstoppedManipulation() instead of IsInManipulation() to avoid pushing non-default content offsets to DManip.
                m_isDirectManipulationStopped = TRUE;
            }
        }
    }

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::SynchronizeScrollOffsets
//
//  Synopsis:
//    Synchronizes the ScrollData's m_ComputedOffset and m_Offset fields.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::SynchronizeScrollOffsets()
{
    HRESULT hr = S_OK;
    DOUBLE offset = 0.0;
    ctl::ComPtr<IScrollInfo> spScrollInfo;

    IFC(get_ScrollInfo(&spScrollInfo));
    if (!spScrollInfo)
    {
        goto Cleanup;
    }

    // Synchonize the ScrollData's m_ComputedOffset.X and m_Offset.X fields
    IFC(spScrollInfo->get_HorizontalOffset(&offset));
    IFC(spScrollInfo->SetHorizontalOffset(offset));

    // Synchonize the ScrollData's m_ComputedOffset.Y and m_Offset.Y fields
    IFC(spScrollInfo->get_VerticalOffset(&offset));
    IFC(spScrollInfo->SetVerticalOffset(offset));

Cleanup:
    RRETURN(hr);
}

// after the thumb drag completes, we need to push the cached values to the scrollinfo
_Check_return_ HRESULT ScrollViewer::SynchronizeScrollOffsetsAfterThumbDeferring()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IScrollInfo> spScrollInfo;

    IFC(get_ScrollInfo(&spScrollInfo));
    if (spScrollInfo)
    {
        // we have completed a drag, so synchronize
        if (m_horizontalOffsetCached != -1.0)
        {
            IFC(spScrollInfo->SetHorizontalOffset(m_horizontalOffsetCached));
        }
        if (m_verticalOffsetCached != -1.0)
        {
            IFC(spScrollInfo->SetVerticalOffset(m_verticalOffsetCached));
        }
    }

    m_horizontalOffsetCached = m_verticalOffsetCached = -1.0;

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::GetKeyboardMessageZoomAction
//
//  Synopsis:
//    Obtains the zoom action (if any) DM will attempt if given the provided key combination.
//
//-------------------------------------------------------------------------
ZoomDirection
ScrollViewer::GetKeyboardMessageZoomAction(
_In_ wsy::VirtualKeyModifiers keyModifiers,
_In_ wsy::VirtualKey key)
{
    ZoomDirection messageZoomDirection = ZoomDirection_None;

    // Filter out the shift key, we are not sensitive to it.
    // This is the design for now, will be reviewed for RC.
    keyModifiers &= ~wsy::VirtualKeyModifiers_Shift;

    if (keyModifiers == wsy::VirtualKeyModifiers_Control)
    {
        if (key == wsy::VirtualKey_Subtract
            || key == SCROLLVIEWER_KEYCODE_MINUS)
        {
            messageZoomDirection = ZoomDirection_Out;
        }
        else if (key == wsy::VirtualKey_Add
            || key == SCROLLVIEWER_KEYCODE_EQUALS)
        {
            messageZoomDirection = ZoomDirection_In;
        }
    }

    return messageZoomDirection;
}

// Called by a control interested in knowning DirectManipulation state changes.
// Only one listener can declare itself at once.
_Check_return_ HRESULT
ScrollViewer::SetDirectManipulationStateChangeHandler(
_In_opt_ DirectManipulationStateChangeHandler* pDMStateChangeHandler)
{
    ASSERT(!pDMStateChangeHandler || !m_pDMStateChangeHandler);
    m_pDMStateChangeHandler = pDMStateChangeHandler;
    RRETURN(S_OK);
}


// block the indicators from showing (during SeZo view change for instance)
_Check_return_ HRESULT
ScrollViewer::BlockIndicatorsFromShowing()
{
    HRESULT hr = S_OK;
    BOOLEAN bIgnored = FALSE;

    if (!m_blockIndicators && IsConscious())
    {
        m_blockIndicators = TRUE;

        IFC(GoToState(FALSE, L"NoIndicator", &bIgnored));
        if (!m_hasNoIndicatorStateStoryboardCompletedHandler)
        {
            m_showingMouseIndicators = FALSE;
        }

        if (m_trElementHorizontalScrollBar)
        {
            IFC(m_trElementHorizontalScrollBar.Cast<ScrollBar>()->BlockIndicatorFromShowing());
        }
        if (m_trElementVerticalScrollBar)
        {
            IFC(m_trElementVerticalScrollBar.Cast<ScrollBar>()->BlockIndicatorFromShowing());
        }
        m_keepIndicatorsShowing = FALSE;
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
ScrollViewer::ResetBlockIndicatorsFromShowing()
{
    HRESULT hr = S_OK;
    m_blockIndicators = FALSE;
    if (m_trElementHorizontalScrollBar)
    {
        IFC(m_trElementHorizontalScrollBar.Cast<ScrollBar>()->ResetBlockIndicatorFromShowing());
    }
    if (m_trElementVerticalScrollBar)
    {
        IFC(m_trElementVerticalScrollBar.Cast<ScrollBar>()->ResetBlockIndicatorFromShowing());
    }

Cleanup:
    RRETURN(hr);
}


// Change to the correct visual state for the ScrollViewer.
_Check_return_ HRESULT
ScrollViewer::ChangeVisualState(
// true to use transitions when updating the visual state, false
// to snap directly to the new visual state.
_In_ bool bUseTransitions)
{
    HRESULT hr = S_OK;
    BOOLEAN bIgnored = FALSE;

    if (!IsConscious())
    {
        ShowIndicators();
    }
    else if (!m_keepIndicatorsShowing)
    {
        IFC(GoToState(bUseTransitions, L"NoIndicator", &bIgnored));
        IFC(GoToState(bUseTransitions, L"ScrollBarSeparatorCollapsed", &bIgnored));
        if (!m_hasNoIndicatorStateStoryboardCompletedHandler)
        {
            m_showingMouseIndicators = FALSE;
        }
    }

Cleanup:
    RRETURN(hr);
}

// Show the appropriate scrolling indicators.
_Check_return_ HRESULT ScrollViewer::ShowIndicators()
{
    BOOLEAN bIgnored = FALSE;
    bool showScrollBarSeparator = !IsConscious();

    if (IsRootScrollViewer())
    {
        ASSERT(IsInputPaneShow(), L"We should only call ShowIndicators() on the root SV when the on-screen keyboard is showing.");
    }

    if ((!m_blockIndicators || !IsConscious()) &&
        !AreBothScrollBarsCollapsed())
    {
        // Mouse indicators dominate if they are already showing or if we have set the flag to prefer them.
        if (m_preferMouseIndicators || m_showingMouseIndicators || !IsConscious())
        {
            if (AreBothScrollBarsVisible() &&
               (m_isPointerOverVerticalScrollbar || m_isPointerOverHorizontalScrollbar))
            {
                IFC_RETURN(GoToState(TRUE, L"MouseIndicatorFull", &bIgnored));
                showScrollBarSeparator = true;
            }
            else
            {
                IFC_RETURN(GoToState(TRUE, L"MouseIndicator", &bIgnored));
            }

            m_showingMouseIndicators = TRUE;
        }
        else
        {
            IFC_RETURN(GoToState(TRUE, L"TouchIndicator", &bIgnored));
        }
    }

    BOOLEAN isEnabled = FALSE;

    IFC_RETURN(get_IsEnabled(&isEnabled));

    // Select the proper state for the ScrollBar separator square within the ScrollBarSeparatorStates group:
    if (IsAnimationEnabled())
    {
        // When OS animations are turned on, show the square when a ScrollBar is shown unless the ScrollViewer is disabled, using an animation.
        IFC_RETURN(GoToState(TRUE /*useTransitions*/, (showScrollBarSeparator && isEnabled) ? L"ScrollBarSeparatorExpanded" : L"ScrollBarSeparatorCollapsed", &bIgnored));
    }
    else
    {
        // OS animations are turned off. Show or hide the square depending on the presence of a ScrollBar, without an animation.
        // When the ScrollViewer is disabled, hide the square in sync with the ScrollBar(s).
        if (showScrollBarSeparator)
        {
            IFC_RETURN(GoToState(TRUE /*useTransitions*/, isEnabled ? L"ScrollBarSeparatorExpandedWithoutAnimation" : L"ScrollBarSeparatorCollapsed", &bIgnored));
        }
        else
        {
            IFC_RETURN(GoToState(TRUE /*useTransitions*/, isEnabled ? L"ScrollBarSeparatorCollapsedWithoutAnimation" : L"ScrollBarSeparatorCollapsed", &bIgnored));
        }
    }

    return S_OK;
}

bool ScrollViewer::AreBothScrollBarsCollapsed()
{
    return m_scrollVisibilityX == xaml::Visibility_Collapsed
        && m_scrollVisibilityY == xaml::Visibility_Collapsed;
}

bool ScrollViewer::AreBothScrollBarsVisible()
{
    return m_scrollVisibilityX == xaml::Visibility_Visible
        && m_scrollVisibilityY == xaml::Visibility_Visible;
}

// Handler for when the TouchIndicator or MouseIndicator state's storyboard completes animating.
_Check_return_ HRESULT
ScrollViewer::IndicatorStateStoryboardCompleted(
    _In_opt_ IInspectable* /*pUnused1*/,
    _In_opt_ IInspectable* /*pUnused2*/)
{
    // If the cursor is currently directly over either scrollbar then don't automatically hide the indicators
    if (!m_keepIndicatorsShowing &&
        !(m_isPointerOverVerticalScrollbar || m_isPointerOverHorizontalScrollbar))
    {
        // Go to the NoIndicator state using transitions.  There should be a delay before the NoIndicator state actually shows.
        IFC_RETURN(UpdateVisualState());
    }

    return S_OK;
}

// Raises or delays the ViewChanging event with the provided target transform and IsInertial flag.
// The event is delayed when m_iViewChangingDelay is strictly positive. In that case the event is
// raised later when m_iViewChangingDelay reaches 0 again.
_Check_return_ HRESULT
ScrollViewer::RaiseViewChanging(
_In_ DOUBLE targetHorizontalOffset,
_In_ DOUBLE targetVerticalOffset,
_In_ FLOAT targetZoomFactor)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_controls::IScrollViewerView> spNextView;
    ctl::ComPtr<xaml_controls::IScrollViewerView> spFinalView;
    ctl::ComPtr<IManipulationDataProvider> spProvider;
    ViewChangingEventSourceType* pEventSource = nullptr;

    m_isTargetHorizontalOffsetValid = m_isTargetVerticalOffsetValid = m_isTargetZoomFactorValid = TRUE;

    m_targetHorizontalOffset = targetHorizontalOffset;
    m_targetVerticalOffset = targetVerticalOffset;
    m_targetZoomFactor = targetZoomFactor;

    // Now that the view is about to change, clear the potential latest view change request.
    m_targetChangeViewHorizontalOffset = -1.0;
    m_targetChangeViewVerticalOffset = -1.0;
    m_targetChangeViewZoomFactor = -1.0f;

    if (m_iViewChangingDelay > 0)
    {
#ifdef DM_DEBUG
        if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/,
                L"DMSV[0x%p]:  RaiseViewChanging - Delaying ViewChanging(TargetHorizontalOffset=%f, TargetVerticalOffset=%f, TargetZoomFactor=%4.8lf, IsInertial=%d) event.",
                this, targetHorizontalOffset, targetVerticalOffset, targetZoomFactor, m_isInertial));
        }
#endif // DM_DEBUG

        m_isViewChangingDelayed = TRUE;
    }
    else
    {
#ifdef DM_DEBUG
        if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/,
                L"DMSV[0x%p]:  RaiseViewChanging - Raising ViewChanging(TargetHorizontalOffset=%f, TargetVerticalOffset=%f, TargetZoomFactor=%4.8lf",
                this, targetHorizontalOffset, targetVerticalOffset, targetZoomFactor));
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/,
                L"                   InertiaEndHorizontalOffset=%f, InertiaEndVerticalOffset=%f, InertiaEndZoomFactor=%4.8lf, IsInertial=%d, IsInertiaEndTransformValid=%d) event.",
                m_inertiaEndHorizontalOffset, m_inertiaEndVerticalOffset, m_inertiaEndZoomFactor, m_isInertial, m_isInertiaEndTransformValid));
        }
#endif // DM_DEBUG

        m_isViewChangingDelayed = FALSE;

        IFC(GetViewChangingEventSourceNoRef(&pEventSource));
        if (pEventSource->HasHandlers())
        {
            ctl::ComPtr<ScrollViewerViewChangingEventArgs> spArgs;
            IFC(ctl::make(&spArgs));
            IFC(spArgs->put_IsInertial(m_isInertial));

            IFC(ctl::ComObject<ScrollViewerView>::CreateInstance(spNextView.ReleaseAndGetAddressOf()));
            IFC(spNextView.Cast<ScrollViewerView>()->put_HorizontalOffset(m_targetHorizontalOffset));
            IFC(spNextView.Cast<ScrollViewerView>()->put_VerticalOffset(m_targetVerticalOffset));
            IFC(spNextView.Cast<ScrollViewerView>()->put_ZoomFactor(m_targetZoomFactor));
            IFC(spArgs->put_NextView(spNextView.Get()));

            IFC(GetInnerManipulationDataProvider(&spProvider));
            if (m_isInertial && m_isInertiaEndTransformValid && !spProvider)
            {
                IFC(ctl::ComObject<ScrollViewerView>::CreateInstance(spFinalView.ReleaseAndGetAddressOf()));
                IFC(spFinalView.Cast<ScrollViewerView>()->put_HorizontalOffset(m_inertiaEndHorizontalOffset));
                IFC(spFinalView.Cast<ScrollViewerView>()->put_VerticalOffset(m_inertiaEndVerticalOffset));
                IFC(spFinalView.Cast<ScrollViewerView>()->put_ZoomFactor(m_inertiaEndZoomFactor));
                IFC(spArgs->put_FinalView(spFinalView.Get()));
            }
            else
            {
                // When the ScrollViewer is not in inertial mode, or the ScrollViewer operates with a
                // logical-based IManipulationDataProvider, the FinalView component of the event args
                // is built with the NextView content.
                IFC(spArgs->put_FinalView(spNextView.Get()));
            }
            IFC(pEventSource->Raise(ctl::as_iinspectable(this), spArgs.Get()));
        }
    }

Cleanup:
    RRETURN(hr);
}

// Raises the ViewChanged event with the provided IsIntermediate value
// unless m_iViewChangedDelay is strictly positive. In that case
// the event is delayed and raised when m_iViewChangedDelay reaches 0 again.
_Check_return_ HRESULT
ScrollViewer::RaiseViewChanged(
_In_ BOOLEAN isIntermediate)
{
    HRESULT hr = S_OK;
    ViewChangedEventSourceType* pEventSource = nullptr;

    if (m_iViewChangedDelay > 0)
    {
#ifdef DM_DEBUG
        if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/, L"DMSV[0x%p]:  RaiseViewChanged - Delaying ViewChanged(IsIntermediate=%s) event.", this, isIntermediate ? L"True" : L"False"));
        }
#endif // DM_DEBUG

        m_isViewChangedDelayed = TRUE;
        // The batched up & delayed ViewChanged event will use the isIntermediate value
        // of the latest request.
        m_isDelayedViewChangedIntermediate = isIntermediate;
    }
    else
    {
        if (m_isInIntermediateViewChangedMode)
        {
#ifdef DM_DEBUG
            if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
            {
                IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/, L"DMSV[0x%p]:  RaiseViewChanged - Raising ViewChanged event while in intermediate mode.", this));
            }
#endif // DM_DEBUG

            // ViewChanged is raised during an 'intermediate mode'
            // This means that ViewChanged with IsIntermediate==False needs to be raised
            // at the end of this 'intermediate mode'.
            m_isViewChangedRaisedInIntermediateMode = TRUE;
        }

#ifdef DM_DEBUG
        if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/, L"DMSV[0x%p]:  RaiseViewChanged - Raising ViewChanged(IsIntermediate=%s) event.", this, isIntermediate ? L"True" : L"False"));
        }
#endif // DM_DEBUG

        m_isViewChangedDelayed = FALSE;

        IFC(GetViewChangedEventSourceNoRef(&pEventSource));
        if (pEventSource->HasHandlers())
        {
            ctl::ComPtr<ScrollViewerViewChangedEventArgs> spArgs;
            IFC(ctl::make(&spArgs));
            IFC(spArgs->put_IsIntermediate(isIntermediate));
            IFC(pEventSource->Raise(ctl::as_iinspectable(this), spArgs.Get()));
        }
    }

    if (!isIntermediate)
    {
        // Reset pending shifts.
        m_pendingViewportShiftX = 0.0;
        m_pendingViewportShiftY = 0.0;
    }

Cleanup:
    RRETURN(hr);
}

// Increments m_iViewChangingDelay to delay any
// potential attempt at raising the ViewChanging event.
void
ScrollViewer::DelayViewChanging()
{
    m_iViewChangingDelay++;
}

// Increments m_iViewChangedDelay to delay any
// potential attempt at raising the ViewChanged event.
void
ScrollViewer::DelayViewChanged()
{
    m_iViewChangedDelay++;
}

// Decrements m_iViewChangingDelay and checks if a
// ViewChanging notification was delayed and can
// now be raised. DelayViewChanging and FlushViewChanging
// need to go in pairs.
_Check_return_ HRESULT
ScrollViewer::FlushViewChanging(
_In_ HRESULT hr)
{
#ifdef DM_DEBUG
    if (DMSVv_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE)) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE | DMSVv_DBG) /*traceType*/, L"DMSVv[0x%p]: FlushViewChanging isInertial=%s, can raise=%s, must raise=%s.",
            this, m_isInertial ? L"True" : L"False", m_iViewChangingDelay == 1 ? L"True" : L"False", m_isViewChangingDelayed ? L"True" : L"False"));
    }
#endif // DM_DEBUG

    ASSERT(m_iViewChangingDelay > 0);

    m_iViewChangingDelay--;

    if (SUCCEEDED(hr))
    {
        if (m_iViewChangingDelay == 0 && m_isViewChangingDelayed)
        {
#ifdef DM_DEBUG
            if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
            {
                IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/, L"DMSV[0x%p]:  FlushViewChanging - Raising delayed ViewChanging.", this));
            }
#endif // DM_DEBUG

            IFC(RaiseViewChanging(m_targetHorizontalOffset, m_targetVerticalOffset, m_targetZoomFactor));
        }
    }

Cleanup:
    RRETURN(hr);
}

// Decrements m_iViewChangedDelay and checks if a
// ViewChanged notification was delayed and can
// now be raised. DelayViewChanged and FlushViewChanged
// need to go in pairs.
_Check_return_ HRESULT
ScrollViewer::FlushViewChanged(
_In_ HRESULT hr)
{
#ifdef DM_DEBUG
    if (DMSVv_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE)) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE | DMSVv_DBG) /*traceType*/, L"DMSVv[0x%p]: FlushViewChanged can raise=%s, must raise=%s.",
            this, m_iViewChangedDelay == 1 ? L"True" : L"False", m_isViewChangedDelayed ? L"True" : L"False"));
    }
#endif // DM_DEBUG

    ASSERT(m_iViewChangedDelay > 0);

    m_iViewChangedDelay--;

    if (m_iViewChangedDelay == 0 &&
        m_isViewChangedDelayed &&
        SUCCEEDED(hr))
    {
#ifdef DM_DEBUG
        if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/, L"DMSV[0x%p]:  FlushViewChanged - Raising delayed ViewChanged.", this));
        }
#endif // DM_DEBUG

        IFC(RaiseViewChanged(m_isDelayedViewChangedIntermediate /*isIntermediate*/));
    }

Cleanup:
    RRETURN(hr);
}

// Called at the beginning of an operation that may cause several ViewChanged events, like a DM manip.
_Check_return_ HRESULT
ScrollViewer::EnterIntermediateViewChangedMode()
{
#ifdef DM_DEBUG
    if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/, L"DMSV[0x%p]:  EnterIntermediateViewChangedMode.", this));
    }
#endif // DM_DEBUG

    if (!m_isInIntermediateViewChangedMode)
    {
        m_isInIntermediateViewChangedMode = TRUE;

        // This flag is set to True the first time ViewChanged is raised during this multi-notification operation.
        m_isViewChangedRaisedInIntermediateMode = FALSE;
    }
    RRETURN(S_OK);
}

// Called at the end of an operation that may have caused several ViewChanged events, like a DM manip.
_Check_return_ HRESULT
ScrollViewer::LeaveIntermediateViewChangedMode(
_In_ BOOLEAN raiseFinalViewChanged)
{
    HRESULT hr = S_OK;

#ifdef DM_DEBUG
    if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/, L"DMSV[0x%p]:  LeaveIntermediateViewChangedMode.", this));
    }
#endif // DM_DEBUG

    if (m_isInIntermediateViewChangedMode)
    {
        m_isInIntermediateViewChangedMode = FALSE;

        if (m_isViewChangedRaisedInIntermediateMode)
        {
            m_isViewChangedRaisedInIntermediateMode = FALSE;

            if (raiseFinalViewChanged)
            {
#ifdef DM_DEBUG
                if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
                {
                    IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/, L"DMSV[0x%p]:  LeaveIntermediateViewChangedMode - Raising final ViewChanged.", this));
                }
#endif // DM_DEBUG

                // Mark the end of a multi-notification operation
                IFC(RaiseViewChanged((BOOLEAN)FALSE /*isIntermediate*/));
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
ScrollViewer::RaiseDirectManipulationStarted()
{
    HRESULT hr = S_OK;
    DirectManipulationStartedEventSourceType* pEventSource = nullptr;

    IFC(GetDirectManipulationStartedEventSourceNoRef(&pEventSource));
    if (pEventSource->HasHandlers())
    {
        ctl::ComPtr<EventArgs> spArgs;
        IFC(ctl::make(&spArgs));
        IFC(pEventSource->Raise(ctl::as_iinspectable(this), spArgs.Get()));
    }

Cleanup:
    return hr;
}

_Check_return_ HRESULT
ScrollViewer::RaiseDirectManipulationCompleted()
{
    HRESULT hr = S_OK;
    DirectManipulationCompletedEventSourceType* pEventSource = nullptr;

    IFC(GetDirectManipulationCompletedEventSourceNoRef(&pEventSource));
    if (pEventSource->HasHandlers())
    {
        ctl::ComPtr<EventArgs> spArgs;
        IFC(ctl::make(&spArgs));
        IFC(pEventSource->Raise(ctl::as_iinspectable(this), spArgs.Get()));
    }

Cleanup:
    return hr;
}

// Loaded event handler.
_Check_return_ HRESULT
ScrollViewer::OnLoaded(
_In_ IInspectable* pSender,
_In_ xaml::IRoutedEventArgs* pArgs)
{
    m_isLoaded = TRUE;

    // DManip needs to be aware of the content transform immediately via a ZoomToRect call.
    // Prior attempts at synchronizing the XAML and DManip transforms may have failed because a viewport size, in pixels, was still 0.
    IFC_RETURN(OnPrimaryContentTransformChanged(TRUE /*translationXChanged*/, TRUE /*translationYChanged*/, TRUE /*zoomFactorChanged*/));

    RRETURN(S_OK);
}

// Unloaded event handler.
_Check_return_ HRESULT
ScrollViewer::OnUnloaded(
_In_ IInspectable* pSender,
_In_ xaml::IRoutedEventArgs* pArgs)
{
    m_isLoaded = FALSE;

    m_showingMouseIndicators = FALSE;
    m_keepIndicatorsShowing = FALSE;

    RRETURN(S_OK);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::ShouldContinueRoutingKeyDownEvent
//
//  Synopsis:
//    Determines if key press should be forwarded or processed by DM.
//    The determination is made based on whether scrolling and chaining are
//    enabled and viewport is near an edge within the current item.
//
//-------------------------------------------------------------------------

_Check_return_ HRESULT
ScrollViewer::ShouldContinueRoutingKeyDownEvent(
_In_ wsy::VirtualKey key,
_Out_ BOOLEAN& continueRouting)
{
    HRESULT hr = S_OK;
    DOUBLE offset = 0.0;
    DOUBLE edge = 0.0;
    BOOLEAN chainingEnabled = FALSE;
    xaml_controls::ScrollMode scrollMode = xaml_controls::ScrollMode_Disabled;
    xaml::FlowDirection direction = xaml::FlowDirection_LeftToRight;

    IFC(get_FlowDirection(&direction));

    switch (key)
    {
    case wsy::VirtualKey_GamepadLeftTrigger:
    case wsy::VirtualKey_Up:
        IFC(get_VerticalOffset(&offset));
        IFC(get_IsVerticalScrollChainingEnabled(&chainingEnabled));
        IFC(get_VerticalScrollMode(&scrollMode));
        break;

    case wsy::VirtualKey_GamepadRightTrigger:
    case wsy::VirtualKey_Down:
        IFC(get_VerticalOffset(&offset));
        IFC(get_ScrollableHeight(&edge));
        IFC(get_IsVerticalScrollChainingEnabled(&chainingEnabled));
        IFC(get_VerticalScrollMode(&scrollMode));
        break;

    case wsy::VirtualKey_GamepadLeftShoulder:
    case wsy::VirtualKey_Left:
        IFC(get_HorizontalOffset(&offset));
        if (direction == xaml::FlowDirection_RightToLeft)
        {
            IFC(get_ScrollableWidth(&edge));
        }
        IFC(get_IsHorizontalScrollChainingEnabled(&chainingEnabled));
        IFC(get_HorizontalScrollMode(&scrollMode));
        break;

    case wsy::VirtualKey_GamepadRightShoulder:
    case wsy::VirtualKey_Right:
        IFC(get_HorizontalOffset(&offset));
        if (direction == xaml::FlowDirection_LeftToRight)
        {
            IFC(get_ScrollableWidth(&edge));
        }
        IFC(get_IsHorizontalScrollChainingEnabled(&chainingEnabled));
        IFC(get_HorizontalScrollMode(&scrollMode));
        break;
    }

    // Methods get_xxxOffset() do not return fractional parts and get_ScrollableXxx() do,
    // define 'near' as within radius of 1 unit.

    if (scrollMode != xaml_controls::ScrollMode_Disabled &&
        chainingEnabled &&
        DoubleUtil::LessThanOrClose(DoubleUtil::Abs(edge - offset), 1.0))
    {
        continueRouting = TRUE;
    }
    else
    {
        continueRouting = FALSE;
    }

Cleanup:
    RRETURN(hr);
}


//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::GetShouldClearFocus
//
//  Synopsis:
//    Determines if the ScrollViewer should clear focus on pointer released or right tapped.
//    This determination is made based on whether or not it is part of a template for a
//    text control - if it is not, then we should clear focus.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::GetShouldClearFocus(
    _Out_ BOOLEAN *pShouldClearFocus)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IDependencyObject> spParent = NULL;
    ctl::ComPtr<IDependencyObject> spCurrent = static_cast<DependencyObject *>(this);

    *pShouldClearFocus = TRUE;

    while (spCurrent)
    {
        CDependencyObject *pCurrentDO = static_cast<DependencyObject *>(spCurrent.Get())->GetHandle();
        if (pCurrentDO->GetTypeIndex() == KnownTypeIndex::TextBox ||
            pCurrentDO->GetTypeIndex() == KnownTypeIndex::PasswordBox ||
            pCurrentDO->GetTypeIndex() == KnownTypeIndex::RichEditBox)
        {
            *pShouldClearFocus = FALSE;
            goto Cleanup;
        }

        IFC(VisualTreeHelper::GetParentStatic(spCurrent.Get(), &spParent));
        spCurrent = spParent;
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ScrollViewer::IsThumbDragging(
    _Out_ BOOLEAN* pThumbDragging)
{
    HRESULT hr = S_OK;
    BOOLEAN result = FALSE;

    if (m_trElementHorizontalScrollBar)
    {
        BOOLEAN dragging = FALSE;
        IFC(m_trElementHorizontalScrollBar.Cast<ScrollBar>()->get_IsDragging(&dragging));
        result |= dragging;
    }
    if (m_trElementVerticalScrollBar)
    {
        BOOLEAN dragging = FALSE;
        IFC(m_trElementVerticalScrollBar.Cast<ScrollBar>()->get_IsDragging(&dragging));
        result |= dragging;
    }

    *pThumbDragging = result;

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::OnHeaderPropertyChanged
//
//  Synopsis:
//    Event handler called when one of the three header properties changed.
//    Sets the header on the inner ScrollContentPresenter element.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::OnHeaderPropertyChanged(
    _In_ BOOLEAN isTopHeader, _In_ BOOLEAN isLeftHeader,
    _In_ IInspectable *pOldValue, _In_ IInspectable *pNewValue)
{
    HRESULT hr = S_OK;
    BOOLEAN restoreOldValue = FALSE;
    BOOLEAN isTopLeftHeader = isTopHeader && isLeftHeader;
    bool isAssociated = false;
    bool allowsMultipleAssociations = false;
    ctl::ComPtr<IUIElement> spOldHeader;
    ctl::ComPtr<IUIElement> spNewHeader;
    ctl::ComPtr<IScrollInfo> spScrollInfo;
    ctl::ComPtr<IDependencyObject> spDependencyObject;
    ScrollContentPresenter* pScrollContentPresenter = NULL;

#ifdef DM_DEBUG
    if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/, L"DMSV[0x%p]:  OnHeaderPropertyChanged isTopHeader=%d, isLeftHeader=%d.",
            this, isTopHeader, isLeftHeader, isAssociated));
    }
#endif // DM_DEBUG

    ASSERT(isTopHeader || isLeftHeader);
    ASSERT(pOldValue != pNewValue);

    if (pOldValue == pNewValue)
    {
        goto Cleanup;
    }

    IFC(ctl::do_query_interface<xaml::IUIElement>(spOldHeader, pOldValue));
    IFC(ctl::do_query_interface<xaml::IUIElement>(spNewHeader, pNewValue));

    if (m_trElementScrollContentPresenter)
    {
        pScrollContentPresenter = m_trElementScrollContentPresenter.Cast<ScrollContentPresenter>();
    }

    if (spOldHeader)
    {
        if (pScrollContentPresenter)
        {
            // Unparent old header if needed
            if (isTopLeftHeader)
            {
                IFC(pScrollContentPresenter->put_TopLeftHeader(NULL /*spTopLeftHeader*/, this));
            }
            else if (isTopHeader)
            {
                IFC(pScrollContentPresenter->put_TopHeader(NULL /*spTopHeader*/, this));
            }
            else
            {
                ASSERT(isLeftHeader);
                IFC(pScrollContentPresenter->put_LeftHeader(NULL /*spLeftHeader*/, this));
            }
        }

        // Un-associate the old header if it was previously associated by this ScrollViewer.
        IFC(UpdateHeaderAssociatedStatus(spOldHeader.Cast<UIElement>(), isTopHeader, isLeftHeader, FALSE /*associate*/));
    }

    // Check if new header is already associated
    if (spNewHeader)
    {
        IFC(spNewHeader.As<IDependencyObject>(&spDependencyObject));
        IFC(CoreImports::DependencyObject_GetIsAssociated(
            spDependencyObject.Cast<DependencyObject>()->GetHandle(),
            &isAssociated,
            &allowsMultipleAssociations));
        if (!allowsMultipleAssociations)
        {
            if (isAssociated)
            {
                // Header is already associated and does not allow multiple associations
                restoreOldValue = TRUE;
                IFC(ErrorHelper::OriginateErrorUsingResourceID(E_NER_INVALID_OPERATION /*E_INVALIDARG*/, ERROR_ELEMENT_ASSOCIATED));
            }
            else
            {
                // Mark the new header as associated
                IFC(UpdateHeaderAssociatedStatus(spNewHeader.Cast<UIElement>(), isTopHeader, isLeftHeader, TRUE /*associate*/));
            }
        }
    }

    if (pScrollContentPresenter)
    {
        if (isTopLeftHeader)
        {
            IFC(pScrollContentPresenter->put_TopLeftHeader(spNewHeader, this));
        }
        else if (isTopHeader)
        {
            IFC(pScrollContentPresenter->put_TopHeader(spNewHeader, this));
        }
        else
        {
            ASSERT(isLeftHeader);
            IFC(pScrollContentPresenter->put_LeftHeader(spNewHeader, this));
        }

        if (spNewHeader)
        {
            IFC(get_ScrollInfo(&spScrollInfo));
            if (!spScrollInfo)
            {
                // Ensure the ScrollViewer and its inner ScrollContentPresenter are introduced
                // to each other so the ScrollContentPresenter can display the header.
                IFC(pScrollContentPresenter->HookupScrollingComponents());
            }
        }
    }

    if (IsInDirectManipulation())
    {
        // Make sure the DMAlignmentUnlockCenter flag is included or removed on the fly.
        IFC(OnPrimaryContentAffectingPropertyChanged(
            FALSE /*boundsChanged*/,
            TRUE  /*horizontalAlignmentChanged*/,
            TRUE  /*verticalAlignmentChanged*/,
            FALSE /*zoomFactorBoundaryChanged*/));

        // Also make sure the m_areViewportConfigurationsInvalid flag gets set so the
        // configurations get updated after the current manipulation completes.
        IFC(OnViewportConfigurationsAffectingPropertyChanged());
    }

Cleanup:
    if (restoreOldValue)
    {
        ASSERT(FAILED(hr));
        // Restore the old header value.
        if (isTopLeftHeader)
        {
            IGNOREHR(put_TopLeftHeader(spOldHeader.Get()));
        }
        else if (isTopHeader)
        {
            IGNOREHR(put_TopHeader(spOldHeader.Get()));
        }
        else
        {
            ASSERT(isLeftHeader);
            IGNOREHR(put_LeftHeader(spOldHeader.Get()));
        }
    }
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::NotifyHeaderParenting
//
//  Synopsis:
//    Called internally by the inner ScrollContentPresenter when it is about
//    to become the parent of a header.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::NotifyHeaderParenting(
    _In_ IUIElement* pHeader,
    _In_ BOOLEAN isTopHeader,
    _In_ BOOLEAN isLeftHeader)
{
    // Un-associate the header so it can become the child of the ScrollContentPresenter
    RRETURN(UpdateHeaderAssociatedStatus(
        static_cast<UIElement*>(pHeader),
        isTopHeader,
        isLeftHeader,
        FALSE /*associate*/));
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::NotifyHeadersParented
//
//  Synopsis:
//    Pushes any parented headers to the manipulation container.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::NotifyHeadersParented()
{
    HRESULT hr = S_OK;
    ScrollContentPresenter* pScrollContentPresenter = NULL;
    UIElement* pManipulatableElementNoRef = NULL;
    UIElement* pContentElementNoRef = NULL;
    ctl::ComPtr<IUIElement> spManipulatableElement;
    ctl::ComPtr<IUIElement> spHeader;

#ifdef DM_DEBUG
    if (DMSVv_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE)) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE | DMSVv_DBG) /*traceType*/, L"DMSV[0x%p]:  NotifyHeadersParented.", this));
    }
#endif // DM_DEBUG

    if (m_hManipulationHandler && m_trElementScrollContentPresenter)
    {
        pScrollContentPresenter = m_trElementScrollContentPresenter.Cast<ScrollContentPresenter>();

        IFC(GetContentUIElement(&spManipulatableElement));
        pManipulatableElementNoRef = spManipulatableElement.Cast<UIElement>();

        if (pScrollContentPresenter->IsTopLeftHeaderChild())
        {
            IFC(get_TopLeftHeader(&spHeader));
            ASSERT(spHeader);
            pContentElementNoRef = static_cast<UIElement*>(spHeader.Get());

            IFC(CoreImports::ManipulationHandler_NotifySecondaryContentAdded(
                m_hManipulationHandler,
                pManipulatableElementNoRef ? static_cast<CUIElement*>(pManipulatableElementNoRef->GetHandle()) : NULL,
                static_cast<CUIElement*>(pContentElementNoRef->GetHandle()),
                XcpDMContentTypeTopLeftHeader));
        }

        if (pScrollContentPresenter->IsTopHeaderChild())
        {
            IFC(get_TopHeader(&spHeader));
            ASSERT(spHeader);
            pContentElementNoRef = static_cast<UIElement*>(spHeader.Get());

            IFC(CoreImports::ManipulationHandler_NotifySecondaryContentAdded(
                m_hManipulationHandler,
                pManipulatableElementNoRef ? static_cast<CUIElement*>(pManipulatableElementNoRef->GetHandle()) : NULL,
                static_cast<CUIElement*>(pContentElementNoRef->GetHandle()),
                XcpDMContentTypeTopHeader));
        }

        if (pScrollContentPresenter->IsLeftHeaderChild())
        {
            IFC(get_LeftHeader(&spHeader));
            ASSERT(spHeader);
            pContentElementNoRef = static_cast<UIElement*>(spHeader.Get());

            IFC(CoreImports::ManipulationHandler_NotifySecondaryContentAdded(
                m_hManipulationHandler,
                pManipulatableElementNoRef ? static_cast<CUIElement*>(pManipulatableElementNoRef->GetHandle()) : NULL,
                static_cast<CUIElement*>(pContentElementNoRef->GetHandle()),
                XcpDMContentTypeLeftHeader));
        }
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::NotifyHeaderParented
//
//  Synopsis:
//    Called internally by the inner ScrollContentPresenter when it became
//    the parent of a header.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::NotifyHeaderParented(
    _In_ IUIElement* pHeader,
    _In_ BOOLEAN isTopHeader,
    _In_ BOOLEAN isLeftHeader)
{
    HRESULT hr = S_OK;
    XDMContentType contentType = XcpDMContentTypeLeftHeader;
    ctl::ComPtr<xaml::IUIElement> spManipulatableElement;
    UIElement* pManipulatableElementNoRef = NULL;
    UIElement* pContentElementNoRef = static_cast<UIElement*>(pHeader);

    ASSERT(pHeader);

    if (m_hManipulationHandler)
    {
        IFC(GetContentUIElement(&spManipulatableElement));
        pManipulatableElementNoRef = spManipulatableElement.Cast<UIElement>();

        if (isTopHeader)
        {
            contentType = (isLeftHeader) ? XcpDMContentTypeTopLeftHeader : XcpDMContentTypeTopHeader;
        }

        IFC(CoreImports::ManipulationHandler_NotifySecondaryContentAdded(
            m_hManipulationHandler,
            pManipulatableElementNoRef ? static_cast<CUIElement*>(pManipulatableElementNoRef->GetHandle()) : NULL,
            static_cast<CUIElement*>(pContentElementNoRef->GetHandle()),
            contentType));
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::NotifyHeaderUnparented
//
//  Synopsis:
//    Called internally by the inner ScrollContentPresenter when a header
//    is no longer parented.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::NotifyHeaderUnparented(
    _In_ IUIElement* pHeader,
    _In_ BOOLEAN isTopHeader,
    _In_ BOOLEAN isLeftHeader)
{
    HRESULT hr = S_OK;
    UIElement* pManipulatableElementNoRef = NULL;
    UIElement* pContentElementNoRef = static_cast<UIElement*>(pHeader);

    ASSERT(pHeader);

    // Associate the header so it cannot become the child of an element.
    IFC(UpdateHeaderAssociatedStatus(
        static_cast<UIElement*>(pHeader),
        isTopHeader,
        isLeftHeader,
        TRUE /*associate*/));

    if (m_hManipulationHandler)
    {
        pManipulatableElementNoRef = m_trManipulatableElement.Cast<UIElement>();

        IFC(CoreImports::ManipulationHandler_NotifySecondaryContentRemoved(
            m_hManipulationHandler,
            pManipulatableElementNoRef ? static_cast<CUIElement*>(pManipulatableElementNoRef->GetHandle()) : NULL,
            static_cast<CUIElement*>(pContentElementNoRef->GetHandle())));
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::SetScrollContentPresenterHeaders
//
//  Synopsis:
//    Pushes the potential header elements to the inner ScrollContentPresenter.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::SetScrollContentPresenterHeaders()
{
    HRESULT hr = S_OK;
    BOOLEAN isHeaderSet = FALSE;
    ScrollContentPresenter* pScrollContentPresenter = NULL;
    ctl::ComPtr<IScrollInfo> spScrollInfo;
    ctl::ComPtr<IUIElement> spHeader;

#ifdef DM_DEBUG
    if (DMSVv_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE)) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE | DMSVv_DBG) /*traceType*/, L"DMSVv[0x%p]: SetScrollContentPresenterHeaders.", this));
    }
#endif // DM_DEBUG

    if (m_trElementScrollContentPresenter)
    {
        pScrollContentPresenter = m_trElementScrollContentPresenter.Cast<ScrollContentPresenter>();

        IFC(get_TopLeftHeader(&spHeader));
        if (spHeader)
        {
            IFC(pScrollContentPresenter->put_TopLeftHeader(spHeader, this));
            isHeaderSet = TRUE;
        }

        IFC(get_TopHeader(&spHeader));
        if (spHeader)
        {
            IFC(pScrollContentPresenter->put_TopHeader(spHeader, this));
            isHeaderSet = TRUE;
        }

        IFC(get_LeftHeader(&spHeader));
        if (spHeader)
        {
            IFC(pScrollContentPresenter->put_LeftHeader(spHeader, this));
            isHeaderSet = TRUE;
        }

        if (isHeaderSet)
        {
            IFC(get_ScrollInfo(&spScrollInfo));
            if (!spScrollInfo)
            {
                // Ensure the ScrollViewer and its inner ScrollContentPresenter are introduced
                // to each other so the ScrollContentPresenter can display the header.
                IFC(pScrollContentPresenter->HookupScrollingComponents());
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::UpdateHeaderAssociatedStatus
//
//  Synopsis:
//    isAssociated==True: Associate the header so it cannot become the
//    child of an element.
//    isAssociated==False: Un-associate the header so it can become the
//    child of the ScrollContentPresenter.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::UpdateHeaderAssociatedStatus(
    _In_ UIElement* pObject,
    _In_ BOOLEAN isTopHeader,
    _In_ BOOLEAN isLeftHeader,
    _In_ BOOLEAN associate)
{
    HRESULT hr = S_OK;
    BOOLEAN isTopLeftHeader = isTopHeader && isLeftHeader;
    bool isAssociated = false;
    bool allowsMultipleAssociations = false;

#ifdef DM_DEBUG
    if (DMSVv_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE)) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | XCP_TRACE_VERBOSE | DMSVv_DBG) /*traceType*/, L"DMSVv[0x%p]: UpdateHeaderAssociatedStatus pObject=0x%p, isTopHeader=%d, isLeftHeader=%d, associate=%d.",
            this, pObject, isTopHeader, isLeftHeader, associate));
    }
#endif // DM_DEBUG

    ASSERT(pObject);
    ASSERT(isTopHeader || isLeftHeader);

    CUIElement* pObjectCore = pObject->GetHandle();
    ASSERT(pObjectCore);

    pObjectCore->SetIsNonClippingSubtree(false);

    if (!associate)
    {
        // Skip the un-association if no successful association was previously done
        if (isTopLeftHeader)
        {
            if (!m_isTopLeftHeaderAssociated)
            {
                goto Cleanup;
            }
        }
        else if (isTopHeader)
        {
            if (!m_isTopHeaderAssociated)
            {
                goto Cleanup;
            }
        }
        else
        {
            ASSERT(isLeftHeader);
            if (!m_isLeftHeaderAssociated)
            {
                goto Cleanup;
            }
        }
    }

    IFC(CoreImports::DependencyObject_GetIsAssociated(
        pObjectCore,
        &isAssociated,
        &allowsMultipleAssociations));

    if (!allowsMultipleAssociations && isAssociated != !!associate)
    {
        pObjectCore->SetAssociated(
            !!associate,
            associate ? GetHandle() : nullptr);

        // Remember the association successfully completed for future attempts
        // at un-associating this header.
        if (isTopLeftHeader)
        {
            m_isTopLeftHeaderAssociated = associate;
        }
        else if (isTopHeader)
        {
            m_isTopHeaderAssociated = associate;
        }
        else
        {
            ASSERT(isLeftHeader);
            m_isLeftHeaderAssociated = associate;
        }
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::GetHeadersSize
//
//  Synopsis:
//    Returns the combined size of the headers:
//    - horizontal size is max(TopLeftHeader's width, LeftHeader's width)
//    - vertical size is max(TopLeftHeader's height, TopHeader's height)
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::GetHeadersSize(
    _Out_ XSIZEF* pSize)
{
    HRESULT hr = S_OK;
    DOUBLE width = 0.0f;
    DOUBLE height = 0.0f;
    DOUBLE sizeX = 0.0f;
    DOUBLE sizeY = 0.0f;
    xaml::Thickness margins;
    ScrollContentPresenter* pScrollContentPresenter = NULL;
    ctl::ComPtr<IUIElement> spHeader;
    ctl::ComPtr<IFrameworkElement> spHeaderAsFE;

    IFCPTR(pSize);
    pSize->width = pSize->height = 0.0f;

    if (m_trElementScrollContentPresenter)
    {
        pScrollContentPresenter = m_trElementScrollContentPresenter.Cast<ScrollContentPresenter>();

        if (pScrollContentPresenter->IsTopLeftHeaderChild())
        {
            IFC(get_TopLeftHeader(&spHeader));
            ASSERT(spHeader);
            spHeaderAsFE = spHeader.AsOrNull<IFrameworkElement>();
            if (spHeaderAsFE)
            {
                IFC(spHeaderAsFE->get_ActualWidth(&width));
                IFC(spHeaderAsFE->get_ActualHeight(&height));
                IFC(spHeaderAsFE->get_Margin(&margins));
                sizeX = MAX(sizeX, width + margins.Left + margins.Right);
                sizeY = MAX(sizeY, height + margins.Top + margins.Bottom);
            }
        }

        if (pScrollContentPresenter->IsTopHeaderChild())
        {
            IFC(get_TopHeader(&spHeader));
            ASSERT(spHeader);
            spHeaderAsFE = spHeader.AsOrNull<IFrameworkElement>();
            if (spHeaderAsFE)
            {
                IFC(spHeaderAsFE->get_ActualHeight(&height));
                IFC(spHeaderAsFE->get_Margin(&margins));
                sizeY = MAX(sizeY, height + margins.Top + margins.Bottom);
            }
        }

        if (pScrollContentPresenter->IsLeftHeaderChild())
        {
            IFC(get_LeftHeader(&spHeader));
            ASSERT(spHeader);
            spHeaderAsFE = spHeader.AsOrNull<IFrameworkElement>();
            if (spHeaderAsFE)
            {
                IFC(spHeaderAsFE->get_ActualWidth(&width));
                IFC(spHeaderAsFE->get_Margin(&margins));
                sizeX = MAX(sizeX, width + margins.Left + margins.Right);
            }
        }
    }

    pSize->width = static_cast<XFLOAT>(sizeX);
    pSize->height = static_cast<XFLOAT>(sizeY);

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ScrollViewer::GetViewportRatios
//
//  Synopsis:
//    Returns the horizontal and vertical ratios between the ScrollViewer effective viewport
//    and its actual size. That viewport is potentially reduced by the presence of headers.
//    Ratios returned depend on the quadrant owning the provided child.
//    If child is not provided, we assume the child is in the content.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ScrollViewer::GetViewportRatios(
    _In_opt_ DependencyObject* pChild,
    _Out_ XSIZEF* pRatios)
{
    HRESULT hr = S_OK;
    XSIZEF sizeHeaders = {};
    FLOAT zoomFactor = 1.0f;
    DOUBLE viewportDim = 0.0;
    DOUBLE controlDim = 0.0;
    BOOLEAN isDirectChild = FALSE;
    BOOLEAN isChildInTopLeftHeader = FALSE;
    BOOLEAN isChildInTopHeader = FALSE;
    BOOLEAN isChildInLeftHeader = FALSE;
    BOOLEAN isChildInContent = FALSE;

    IFCPTR(pRatios);
    pRatios->width = pRatios->height = 1.0f;

    if (m_trElementScrollContentPresenter)
    {
        IFC(GetHeadersSize(&sizeHeaders));

        if (sizeHeaders.width > 0.0f || sizeHeaders.height > 0.0f)
        {
            // Check if pChild belongs to a header.
            if (pChild)
            {
                IFC(m_trElementScrollContentPresenter.Cast<ScrollContentPresenter>()->GetHeaderOwnership(
                    pChild,
                    &isDirectChild,
                    &isChildInTopLeftHeader,
                    &isChildInTopHeader,
                    &isChildInLeftHeader,
                    &isChildInContent));
            }

            if (!isChildInTopLeftHeader)
            {
                IFC(get_ZoomFactor(&zoomFactor));

                if (sizeHeaders.width > 0.0f && !isChildInLeftHeader)
                {
                    IFC(get_ActualWidth(&controlDim));
                    if (controlDim > 0)
                    {
                        IFC(ComputePixelViewportWidth(
                            NULL /*pProvider*/,
                            TRUE /*isProviderSet*/, // Since header is present, there is no IManipulationProvider implementation
                            &viewportDim));
                        pRatios->width = static_cast<XFLOAT>(MAX(0.0f, viewportDim - sizeHeaders.width * zoomFactor) / controlDim);
                    }
                }
                if (sizeHeaders.height > 0.0f && !isChildInTopHeader)
                {
                    IFC(get_ActualHeight(&controlDim));
                    if (controlDim > 0)
                    {
                        IFC(ComputePixelViewportHeight(
                            NULL /*pProvider*/,
                            TRUE /*isProviderSet*/, // Since header is present, there is no IManipulationProvider implementation
                            &viewportDim));
                        pRatios->height = static_cast<XFLOAT>(MAX(0.0f, viewportDim - sizeHeaders.height * zoomFactor) / controlDim);
                    }
                }
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

// Determine if content can be scrolled.
// It can, if for either dimension scrolling is enabled AND content size is greater than size of viewport.
_Check_return_ HRESULT ScrollViewer::IsContentScrollable(
    _In_ bool ignoreScrollMode,
    _In_ bool ignoreScrollBarVisibility,
    _Out_ bool* isContentHorizontallyScrollable,
    _Out_ bool* isContentVerticallyScrollable)
{
    HRESULT hr = S_OK;
    BOOLEAN isEnabled = FALSE;

    *isContentHorizontallyScrollable = false;
    *isContentVerticallyScrollable = false;

    IFC(get_IsEnabled(&isEnabled));

    if (isEnabled)
    {
        ctl::ComPtr<IScrollInfo> spScrollInfo;
        DOUBLE minOffset = 0.0;
        DOUBLE viewportSize = 0.0;
        DOUBLE contentExtentSize = 0.0;
        xaml_controls::ScrollMode scrollMode = xaml_controls::ScrollMode_Disabled;
        xaml_controls::ScrollBarVisibility horizontalScrollBarVisibility = xaml_controls::ScrollBarVisibility_Disabled;
        xaml_controls::ScrollBarVisibility verticalScrollBarVisibility = xaml_controls::ScrollBarVisibility_Disabled;

        IFC(get_ScrollInfo(&spScrollInfo));

        IFC(GetEffectiveVerticalScrollMode(TRUE /*canUseCachedProperty*/, scrollMode));

        if (spScrollInfo)
        {
            IFC(spScrollInfo->get_MinVerticalOffset(&minOffset));
            IFC(spScrollInfo->get_ViewportHeight(&viewportSize));
            IFC(spScrollInfo->get_ExtentHeight(&contentExtentSize));
        }
        else
        {
            IFC(get_ViewportHeight(&viewportSize));
            IFC(get_ExtentHeight(&contentExtentSize));
        }

        if (!ignoreScrollBarVisibility)
        {
            IFC(get_HorizontalScrollBarVisibility(&horizontalScrollBarVisibility));
            IFC(get_VerticalScrollBarVisibility(&verticalScrollBarVisibility));
        }

        if ((ignoreScrollMode || scrollMode != xaml_controls::ScrollMode_Disabled) &&
            (ignoreScrollBarVisibility || verticalScrollBarVisibility != xaml_controls::ScrollBarVisibility_Disabled) &&
            (contentExtentSize - minOffset) > viewportSize)
        {
            *isContentVerticallyScrollable = true;
        }

        IFC(GetEffectiveHorizontalScrollMode(TRUE /*canUseCachedProperty*/, scrollMode));
        if (spScrollInfo)
        {
            IFC(spScrollInfo->get_MinHorizontalOffset(&minOffset));
            IFC(spScrollInfo->get_ViewportWidth(&viewportSize));
            IFC(spScrollInfo->get_ExtentWidth(&contentExtentSize));
        }
        else
        {
            IFC(get_ViewportWidth(&viewportSize));
            IFC(get_ExtentWidth(&contentExtentSize));
        }

        if ((ignoreScrollMode || scrollMode != xaml_controls::ScrollMode_Disabled) &&
            (ignoreScrollBarVisibility || horizontalScrollBarVisibility != xaml_controls::ScrollBarVisibility_Disabled) &&
            (contentExtentSize - minOffset) > viewportSize)
        {
            *isContentHorizontallyScrollable = true;
        }
    }

Cleanup:
    RRETURN(hr);
}

// Sticky Headers
_Check_return_ HRESULT
ScrollViewer::get_HeaderHeight(
    _Out_ DOUBLE* pHeaderHeight)
{
    HRESULT hr = S_OK;
    *pHeaderHeight = 0.0;

    if (m_trElementScrollContentPresenter)
    {
        ctl::ComPtr<IInspectable> spContent;
        IFC(m_trElementScrollContentPresenter.Cast<ScrollContentPresenter>()->get_Content(&spContent));
        if (spContent)
        {
            ctl::ComPtr<xaml_controls::IItemsPresenter> spItemsPresenter = spContent.AsOrNull<xaml_controls::IItemsPresenter>();
            if (spItemsPresenter)
            {
                wf::Size headerSize;
                IFC(spItemsPresenter.Cast<ItemsPresenter>()->GetHeaderSize(headerSize));
                *pHeaderHeight = headerSize.Height;
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

// Set explicit DMOverpanMode flags to configure overpan modes for horizontal and vertical directions.
_Check_return_ HRESULT
ScrollViewer::SetOverpanModes(
    _In_ DMOverpanMode horizontalOverpanMode,
    _In_ DMOverpanMode verticalOverpanMode)
{
    HRESULT hr = S_OK;
    const bool horizontalOverpanModeChanged = horizontalOverpanMode != m_horizontalOverpanMode;
    const bool verticalOverpanModeChanged = verticalOverpanMode != m_verticalOverpanMode;

#ifdef DM_DEBUG
    if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/, L"DMSV[0x%p]:  SetOverpanModes - horizontalOverpanMode=%d, verticalOverpanMode=%d, horizontalOverpanModeChanged=%d, verticalOverpanModeChanged=%d.",
            this, horizontalOverpanMode, verticalOverpanMode, horizontalOverpanModeChanged, verticalOverpanModeChanged));
    }
#endif // DM_DEBUG

    if (horizontalOverpanModeChanged || verticalOverpanModeChanged)
    {
        m_horizontalOverpanMode = horizontalOverpanMode;
        m_verticalOverpanMode = verticalOverpanMode;
        IFC(OnViewportAffectingPropertyChanged(
            FALSE /*boundsChanged*/,
            FALSE /*touchConfigurationChanged*/,
            FALSE /*nonTouchConfigurationChanged*/,
            FALSE /*configurationsChanged*/,
            FALSE /*chainedMotionTypesChanged*/,
            horizontalOverpanModeChanged,
            verticalOverpanModeChanged,
            NULL  /*pAreConfigurationsUpdated*/));
    }

Cleanup:
    RRETURN(hr);
}

// Prevents overpan effect so that panning will hard-stop at the boundaries of the scrollable region.
_Check_return_ HRESULT
ScrollViewer::DisableOverpanImpl()
{
#ifdef DM_DEBUG
    if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/, L"DMSV[0x%p]:  DisableOverpanImpl - entry.", this));
    }
#endif // DM_DEBUG

    RRETURN(SetOverpanModes(DMOverpanModeNone, DMOverpanModeNone));
}

// Reenables overpan.
_Check_return_ HRESULT
ScrollViewer::EnableOverpanImpl()
{
#ifdef DM_DEBUG
    if (DMSV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_SCROLLVIEWER) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_SCROLLVIEWER | DMSV_DBG) /*traceType*/, L"DMSV[0x%p]:  EnableOverpanImpl - entry.", this));
    }
#endif // DM_DEBUG

    HRESULT hr = S_OK;

    IFC(SetOverpanModes(DMOverpanModeDefault, DMOverpanModeDefault));

Cleanup:
    RRETURN(hr);
}

// Helper method to get the effective DMOverpanMode for the given orientation.
// Returns the stored DMOverpanMode if the extent is smaller than the viewport;
// returns the default DMOverpanMode otherwise (which is necessary to work around
// a DM limitation we have when the extent is smaller than the viewport).
_Check_return_ HRESULT
ScrollViewer::GetEffectiveDMOverpanMode(
    _In_ BOOLEAN isForHorizontal,
    _Out_ DMOverpanMode* pDMOverpanMode)
{
    // for DMOverpanModeNone, the effective overpan mode is always DMOverpanModeNone
    if (isForHorizontal && m_horizontalOverpanMode == DMOverpanModeNone)
    {
        *pDMOverpanMode = DMOverpanModeNone;
    }
    else if (!isForHorizontal && m_verticalOverpanMode == DMOverpanModeNone)
    {
        *pDMOverpanMode = DMOverpanModeNone;
    }
    else
    {
        // Overpan suppression is turned off, use the default behavior.
        ASSERT(m_horizontalOverpanMode == DMOverpanModeDefault);
        ASSERT(m_verticalOverpanMode == DMOverpanModeDefault);
        *pDMOverpanMode = DMOverpanModeDefault;
    }

    return S_OK;//RRETURN_REMOVAL
}

// Enters the mode where the child's actual size is used for
// the extent exposed through IScrollInfo.
void ScrollViewer::StartUseOfActualSizeAsExtent(_In_ bool isHorizontal)
{
    if (isHorizontal && m_trElementHorizontalScrollBar)
    {
        m_trElementHorizontalScrollBar.Cast<ScrollBar>()->StartUseOfActualSizeAsExtent();
    }
    else if (!isHorizontal && m_trElementVerticalScrollBar)
    {
        m_trElementVerticalScrollBar.Cast<ScrollBar>()->StartUseOfActualSizeAsExtent();
    }
}

// Leaves the mode where the child's actual size is used for
// the extent exposed through IScrollInfo.
void ScrollViewer::StopUseOfActualSizeAsExtent(_In_ bool isHorizontal)
{
    if (isHorizontal && m_trElementHorizontalScrollBar)
    {
        m_trElementHorizontalScrollBar.Cast<ScrollBar>()->StopUseOfActualSizeAsExtent();
    }
    else if (!isHorizontal && m_trElementVerticalScrollBar)
    {
        m_trElementVerticalScrollBar.Cast<ScrollBar>()->StopUseOfActualSizeAsExtent();
    }
}

bool ScrollViewer::IsDraggableOrPannableImpl()
{
    // If both vertical & horizontal scrolling are disabled, return false.
    return !(
        (m_currentVerticalScrollBarVisibility == xaml_controls::ScrollBarVisibility_Disabled ||
            m_currentVerticalScrollMode == xaml_controls::ScrollMode_Disabled)

        &&

        (m_currentHorizontalScrollBarVisibility == xaml_controls::ScrollBarVisibility_Disabled ||
            m_currentHorizontalScrollMode == xaml_controls::ScrollMode_Disabled)
        );
}

// ScrollAnchoring

// Used when ScrollViewer.AnchorAtHorizontalExtent or ScrollViewer.AnchorAtVerticalExtent is True to determine whether the Content is scrolled to an edge.
// It is declared at an edge if it's within 1/10th of a pixel.
const double c_edgeDetectionTolerance = 0.1;

_Check_return_ HRESULT ScrollViewer::RaiseAnchorRequested()
{
    AnchorRequestedEventSourceType* eventSource = nullptr;

    IFC_RETURN(GetAnchorRequestedEventSourceNoRef(&eventSource));
    if (eventSource->HasHandlers())
    {
        ANCHORING_DEBUG_TRACE(L"SV[0x%p]: RaiseAnchorRequested", this);
        ctl::ComPtr<AnchorRequestedEventArgs> args;

        if (!m_anchorRequestedEventArgs)
        {
            IFC_RETURN(ctl::make<AnchorRequestedEventArgs>(&args));
            // Set anchor candidates just once. we are reusing the same vector for the life of this ScrollViewer.
            IFC_RETURN(EnsureAnchorCandidateVector());

            IFC_RETURN(args->put_AnchorCandidates(m_anchorCandidatesForArgs.Get()));
            SetPtrValue(m_anchorRequestedEventArgs, args.AsOrNull<xaml_controls::IAnchorRequestedEventArgs>());
        }

        // Initialize the anchor candidates for args. Note that
        // m_anchorCandidates has a stable set of candidates that are populated by the
        // framework. We make a copy of that onto the args so that the app can potentially
        // filter it. We do not want to loose the original set we had.
        auto anchorCandidates = m_anchorCandidates.Get();
        auto anchorCandidatesForArgs = m_anchorCandidatesForArgs.Get();
        UINT count = 0;
        IFC_RETURN(anchorCandidatesForArgs->Clear());
        IFC_RETURN(anchorCandidates->get_Size(&count));
        for (UINT i = 0; i < count; i++)
        {
            ctl::ComPtr<xaml::IUIElement> candidate;
            IFC_RETURN(anchorCandidates->GetAt(i, &candidate));
            IFC_RETURN(anchorCandidatesForArgs->Append(candidate.Get()));
        }

        args = m_anchorRequestedEventArgs.Cast<AnchorRequestedEventArgs>();
        IFC_RETURN(args->put_Anchor(nullptr));
        IFC_RETURN(eventSource->Raise(this, m_anchorRequestedEventArgs.Get()));

        m_useCandidatesFromArgs = true;
    }
    else
    {
        m_useCandidatesFromArgs = false;
    }

    return S_OK;
}

// Computes the type of anchoring to perform, if any, based on ScrollViewer.HorizontalAnchorRatio, ScrollViewer.VerticalAnchorRatio,
// ScrollViewer.AnchorAtHorizontalExtent, ScrollViewer.AnchorAtVerticalExtent, the current offsets, zoomFactor, viewport size, Content size and state.
// When all 4 returned booleans are False, no element anchoring is performed, no far edge anchoring is performed. There may still be anchoring at near edges.
_Check_return_ HRESULT ScrollViewer::IsAnchoring(
    _Out_ bool* isAnchoringElementHorizontally,
    _Out_ bool* isAnchoringElementVertically,
    _Out_opt_ bool* isAnchoringFarEdgeHorizontally,
    _Out_opt_ bool* isAnchoringFarEdgeVertically)
{
    *isAnchoringElementHorizontally = false;
    *isAnchoringElementVertically = false;
    if (isAnchoringFarEdgeHorizontally)
    {
        *isAnchoringFarEdgeHorizontally = false;
    }
    if (isAnchoringFarEdgeVertically)
    {
        *isAnchoringFarEdgeVertically = false;
    }

    double horizontalAnchorRatio = 0.0;
    IFC_RETURN(get_HorizontalAnchorRatio(&horizontalAnchorRatio));
    double verticalAnchorRatio = 0.0;
    IFC_RETURN(get_VerticalAnchorRatio(&verticalAnchorRatio));

    // For edge anchoring, the near edge is considered when HorizontalAnchorRatio or VerticalAnchorRatio is 0.0.
    // When the property is 1.0, the far edge is considered.
    if (!isnan(horizontalAnchorRatio))
    {
        ASSERT(horizontalAnchorRatio >= 0.0);
        ASSERT(horizontalAnchorRatio <= 1.0);

        DOUBLE zoomedViewportWidth = 0.0;
        IFC_RETURN(get_ViewportWidth(&zoomedViewportWidth));
        if (zoomedViewportWidth != std::numeric_limits<double>::infinity())
        {
            float anticipatedZoomedHorizontalOffset = 0.0f;
            IFC_RETURN(get_ZoomedHorizontalOffsetWithPendingShifts(&anticipatedZoomedHorizontalOffset));

            FLOAT zoomFactor = 1.0f;
            IFC_RETURN(get_ZoomFactor(&zoomFactor));

            if (horizontalAnchorRatio == 1.0 && anticipatedZoomedHorizontalOffset + zoomedViewportWidth - m_unzoomedExtentWidth * zoomFactor > -c_edgeDetectionTolerance)
            {
                if (isAnchoringFarEdgeHorizontally)
                {
                    *isAnchoringFarEdgeHorizontally = true;
                }
            }
            else if (!(horizontalAnchorRatio == 0.0 && anticipatedZoomedHorizontalOffset < c_edgeDetectionTolerance))
            {
                *isAnchoringElementHorizontally = true;
            }
        }
    }

    if (!isnan(verticalAnchorRatio))
    {
        ASSERT(verticalAnchorRatio >= 0.0);
        ASSERT(verticalAnchorRatio <= 1.0);

        DOUBLE zoomedViewportHeight = 0.0;
        IFC_RETURN(get_ViewportHeight(&zoomedViewportHeight));
        if (zoomedViewportHeight != std::numeric_limits<double>::infinity())
        {
            float anticipatedZoomedVerticalOffset = 0.0f;
            IFC_RETURN(get_ZoomedVerticalOffsetWithPendingShifts(&anticipatedZoomedVerticalOffset));

            FLOAT zoomFactor = 1.0f;
            IFC_RETURN(get_ZoomFactor(&zoomFactor));

            if (verticalAnchorRatio == 1.0 && anticipatedZoomedVerticalOffset + zoomedViewportHeight - m_unzoomedExtentHeight * zoomFactor > -c_edgeDetectionTolerance)
            {
                if (isAnchoringFarEdgeVertically)
                {
                    *isAnchoringFarEdgeVertically = true;
                }
            }
            else if (!(verticalAnchorRatio == 0.0 && anticipatedZoomedVerticalOffset < c_edgeDetectionTolerance))
            {
                *isAnchoringElementVertically = true;
            }
        }
    }

    return S_OK;
}

// Returns:
// - viewportAnchorPointHorizontalOffset: unzoomed horizontal offset of the anchor point within the ScrollViewer.Content. NaN if there is no horizontal anchoring.
// - viewportAnchorPointVerticalOffset: unzoomed vertical offset of the anchor point within the ScrollViewer.Content. NaN if there is no vertical anchoring.
_Check_return_ HRESULT ScrollViewer::ComputeViewportAnchorPoint(
    const XRECTF& zoomedViewport,
    _Out_ double* viewportAnchorPointHorizontalOffset,
    _Out_ double* viewportAnchorPointVerticalOffset)
{
    *viewportAnchorPointHorizontalOffset = DoubleUtil::NaN;
    *viewportAnchorPointVerticalOffset = DoubleUtil::NaN;

    FLOAT zoomFactor = 1.0f;
    IFC_RETURN(get_ZoomFactor(&zoomFactor));

    wf::Rect viewportAnchorBounds{
        zoomedViewport.X / zoomFactor,
        zoomedViewport.Y / zoomFactor,
        zoomedViewport.Width / zoomFactor,
        zoomedViewport.Height / zoomFactor
    };

    IFC_RETURN(ComputeAnchorPoint(viewportAnchorBounds, viewportAnchorPointHorizontalOffset, viewportAnchorPointVerticalOffset));

    ANCHORING_DEBUG_TRACE(L"SV[0x%p]: ComputeViewportAnchorPoint -> %lf %lf", this, *viewportAnchorPointHorizontalOffset, *viewportAnchorPointVerticalOffset);

    return S_OK;
}

// Returns:
// - elementAnchorPointHorizontalOffset: unzoomed horizontal offset of the anchor element's anchor point within the ScrollViewer.Content. NaN if there is no horizontal anchoring.
// - elementAnchorPointVerticalOffset: unzoomed vertical offset of the anchor element's point within the ScrollViewer.Content. NaN if there is no vertical anchoring.
_Check_return_ HRESULT ScrollViewer::ComputeElementAnchorPoint(
    bool isForPreArrange,
    _Out_ double* elementAnchorPointHorizontalOffset,
    _Out_ double* elementAnchorPointVerticalOffset)
{
    *elementAnchorPointHorizontalOffset = DoubleUtil::NaN;
    *elementAnchorPointVerticalOffset = DoubleUtil::NaN;

    ASSERT(!m_isAnchorElementDirty);

    if (m_anchorElement.Get())
    {
        wf::Rect anchorElementBounds = m_anchorElementBounds;
        if (!isForPreArrange)
        {
            ctl::ComPtr<xaml::IUIElement> child;
            IFC_RETURN(GetContentUIElement(&child));
            IFC_RETURN(GetDescendantBounds(child.Get(), m_anchorElement.Get(), &anchorElementBounds));
        }

        IFC_RETURN(ComputeAnchorPoint(anchorElementBounds, elementAnchorPointHorizontalOffset, elementAnchorPointVerticalOffset));
        ANCHORING_DEBUG_TRACE(L"SV[0x%p]: ComputeElementAnchorPoint -> %lf %lf", this, *elementAnchorPointHorizontalOffset, *elementAnchorPointVerticalOffset);
    }

    return S_OK;
}

_Check_return_ HRESULT ScrollViewer::ComputeAnchorPoint(
    const wf::Rect& anchorBounds,
    _Out_ double* anchorPointX,
    _Out_ double* anchorPointY)
{
    double horizontalAnchorRatio = 0.0;
    IFC_RETURN(get_HorizontalAnchorRatio(&horizontalAnchorRatio));
    if (isnan(horizontalAnchorRatio))
    {
        *anchorPointX = DoubleUtil::NaN;
    }
    else
    {
        ASSERT(horizontalAnchorRatio >= 0.0);
        ASSERT(horizontalAnchorRatio <= 1.0);

        *anchorPointX = anchorBounds.X + horizontalAnchorRatio * anchorBounds.Width;
    }

    double verticalAnchorRatio = 0.0;
    IFC_RETURN(get_VerticalAnchorRatio(&verticalAnchorRatio));
    if (isnan(verticalAnchorRatio))
    {
        *anchorPointY = DoubleUtil::NaN;
    }
    else
    {
        ASSERT(verticalAnchorRatio >= 0.0);
        ASSERT(verticalAnchorRatio <= 1.0);

        *anchorPointY = anchorBounds.Y + verticalAnchorRatio * anchorBounds.Height;
    }

    return S_OK;
}

// Computes the distance between the viewport's anchor point and the anchor element's anchor point.
_Check_return_ HRESULT ScrollViewer::ComputeViewportToElementAnchorPointsDistance(
    const XRECTF& zoomedViewport,
    bool isForPreArrange,
    _Out_ wf::Size* distance)
{
    if (m_anchorElement.Get())
    {
        bool isValid = false;
        IFC_RETURN(IsElementValidAnchor(m_anchorElement.Get(), &isValid));
        ASSERT(!isForPreArrange || isValid);

        if (!isForPreArrange && !isValid)
        {
            *distance = { FloatUtil::NaN, FloatUtil::NaN };
        }
        else
        {
            double elementAnchorPointHorizontalOffset{ 0.0 };
            double elementAnchorPointVerticalOffset{ 0.0 };
            double viewportAnchorPointHorizontalOffset{ 0.0 };
            double viewportAnchorPointVerticalOffset{ 0.0 };

            IFC_RETURN(ComputeElementAnchorPoint(
                isForPreArrange,
                &elementAnchorPointHorizontalOffset,
                &elementAnchorPointVerticalOffset));
            IFC_RETURN(ComputeViewportAnchorPoint(
                zoomedViewport,
                &viewportAnchorPointHorizontalOffset,
                &viewportAnchorPointVerticalOffset));

            ASSERT(!isnan(viewportAnchorPointHorizontalOffset) || !isnan(viewportAnchorPointVerticalOffset));
            ASSERT(isnan(viewportAnchorPointHorizontalOffset) == isnan(elementAnchorPointHorizontalOffset));
            ASSERT(isnan(viewportAnchorPointVerticalOffset) == isnan(elementAnchorPointVerticalOffset));

            // Rounding the distance to 6 precision digits to avoid layout cycles due to float/double conversions.
            wf::Size viewportToElementAnchorPointsDistance = wf::Size{
                isnan(viewportAnchorPointHorizontalOffset) ?
                FloatUtil::NaN : static_cast<float>(round((elementAnchorPointHorizontalOffset - viewportAnchorPointHorizontalOffset) * 1000000) / 1000000),
                isnan(viewportAnchorPointVerticalOffset) ?
                FloatUtil::NaN : static_cast<float>(round((elementAnchorPointVerticalOffset - viewportAnchorPointVerticalOffset) * 1000000) / 1000000)
            };

            ANCHORING_DEBUG_TRACE(L"SV[0x%p]: ComputeViewportToElementAnchorPointsDistance -> %f %f", this, viewportToElementAnchorPointsDistance.Width, viewportToElementAnchorPointsDistance.Height);

            *distance = viewportToElementAnchorPointsDistance;
        }
    }
    else
    {
        *distance = { FloatUtil::NaN, FloatUtil::NaN };
    }

    return S_OK;
}

_Check_return_ HRESULT ScrollViewer::ClearAnchorCandidates()
{
    ANCHORING_DEBUG_TRACE(L"SV[0x%p]: ClearAnchorCandidates", this);
    IFC_RETURN(EnsureAnchorCandidateVector());

    m_anchorCandidates.Cast<TrackerCollection<xaml::UIElement*>>()->Clear();
    m_isAnchorElementDirty = true;
    return S_OK;
}

void ScrollViewer::ResetAnchorElement()
{
    if (m_anchorElement.Get())
    {
        ANCHORING_DEBUG_TRACE(L"SV[0x%p]: ResetAnchorElement", this);

        m_anchorElement.Clear();
        m_anchorElementBounds = wf::Rect{};
        m_isAnchorElementDirty = false;
    }
}

// Raises the ScrollViewer.AnchorRequested event. If no anchor element was specified,
// selects an anchor among the candidates vector that may have been altered
// in the AnchorRequested event handler.
_Check_return_ HRESULT ScrollViewer::EnsureAnchorElementSelection(const XRECTF& zoomedViewport)
{
    if (m_isAnchorElementDirty)
    {
        m_anchorElement.Clear();
        m_anchorElementBounds = wf::Rect{};
        m_isAnchorElementDirty = false;

        DOUBLE viewportWidth = 0.0;
        DOUBLE viewportHeight = 0.0;
        IFC_RETURN(get_ViewportWidth(&viewportWidth));
        IFC_RETURN(get_ViewportHeight(&viewportHeight));

        double viewportAnchorPointHorizontalOffset{ 0.0 };
        double viewportAnchorPointVerticalOffset{ 0.0 };

        IFC_RETURN(ComputeViewportAnchorPoint(
            zoomedViewport,
            &viewportAnchorPointHorizontalOffset,
            &viewportAnchorPointVerticalOffset));

        ASSERT(!isnan(viewportAnchorPointHorizontalOffset) || !isnan(viewportAnchorPointVerticalOffset));

        IFC_RETURN(RaiseAnchorRequested());

        ctl::ComPtr<xaml::IUIElement> requestedAnchorElement{ nullptr };
        if (m_anchorRequestedEventArgs)
        {
            ctl::ComPtr<AnchorRequestedEventArgs> anchorRequestedEventArgs = m_anchorRequestedEventArgs.Cast<AnchorRequestedEventArgs>();
            IFC_RETURN(anchorRequestedEventArgs->get_Anchor(&requestedAnchorElement));
        }

        ctl::ComPtr<xaml::IUIElement> child;
        IFC_RETURN(GetContentUIElement(&child));

        if (requestedAnchorElement)
        {
            SetPtrValue(m_anchorElement, requestedAnchorElement);
            IFC_RETURN(GetDescendantBounds(child.Get(), requestedAnchorElement.Get(), &m_anchorElementBounds));
        }
        else
        {
            wf::Rect bestAnchorCandidateBounds{};

            float zoomedHorizontalOffset = 0.0f;
            IFC_RETURN(get_ZoomedHorizontalOffsetWithPendingShifts(&zoomedHorizontalOffset));
            float zoomedVerticalOffset = 0.0f;
            IFC_RETURN(get_ZoomedVerticalOffsetWithPendingShifts(&zoomedVerticalOffset));
            FLOAT zoomFactor = 1.0f;
            IFC_RETURN(get_ZoomFactor(&zoomFactor));

            double bestAnchorCandidateDistance = std::numeric_limits<float>::max();
            const wf::Rect viewportAnchorBounds{
                zoomedHorizontalOffset / zoomFactor,
                zoomedVerticalOffset / zoomFactor,
                static_cast<float>(viewportWidth / zoomFactor),
                static_cast<float>(viewportHeight / zoomFactor)
            };

            ASSERT(child);
            IFC_RETURN(EnsureAnchorCandidateVector());

            // We are just picking a candidate from the vector. Using a ComPtr
            // seems to lead to en extra release (maybe because it is used as an in and out paramter?)
            xaml::IUIElement* bestAnchorCandidateNoRef = nullptr;
            ctl::ComPtr<TrackerCollection<xaml::UIElement*>> anchorCandidates  = m_useCandidatesFromArgs?
                                                             m_anchorCandidatesForArgs.Cast<TrackerCollection<xaml::UIElement*>>() :
                                                             m_anchorCandidates.Cast<TrackerCollection<xaml::UIElement*>>();

            UINT count = 0;
            IFC_RETURN(anchorCandidates->get_Size(&count));
            for (UINT i = 0; i < count; i++)
            {
                ctl::ComPtr<xaml::IUIElement> anchorCandidate;
                IFC_RETURN(anchorCandidates->GetAt(i, &anchorCandidate));
                IFC_RETURN(ProcessAnchorCandidate(
                    anchorCandidate.Get(),
                    child.Get(),
                    viewportAnchorBounds,
                    viewportAnchorPointHorizontalOffset,
                    viewportAnchorPointVerticalOffset,
                    &bestAnchorCandidateDistance,
                    &bestAnchorCandidateNoRef,
                    &bestAnchorCandidateBounds));
            }

            if (bestAnchorCandidateNoRef)
            {
                SetPtrValue(m_anchorElement, bestAnchorCandidateNoRef);
                m_anchorElementBounds = bestAnchorCandidateBounds;
            }
        }
    }

    return S_OK;
}

// Checks if the provided anchor candidate is better than the current best, based on its distance to the viewport anchor point,
// and potentially updates the best candidate and its bounds.
_Check_return_ HRESULT ScrollViewer::ProcessAnchorCandidate(
    _In_ xaml::IUIElement* anchorCandidate,
    _In_ xaml::IUIElement* child,
    wf::Rect viewportAnchorBounds,
    double viewportAnchorPointHorizontalOffset,
    double viewportAnchorPointVerticalOffset,
    _Inout_ double* bestAnchorCandidateDistance,
    _Inout_ xaml::IUIElement** bestAnchorCandidate,
    _Inout_ wf::Rect* bestAnchorCandidateBounds)
{
    ASSERT(anchorCandidate);
    ASSERT(child);

    bool isValid = false;
    IFC_RETURN(IsElementValidAnchor(anchorCandidate, child, &isValid));
    // Ignore candidates that are collapsed or do not belong to the Content element and are not the Content itself.
    if (isValid)
    {
        wf::Rect anchorCandidateBounds;
        IFC_RETURN(GetDescendantBounds(child, anchorCandidate, &anchorCandidateBounds));

        // Ignore candidates that do not intersect with the viewport in order to favor those that do.
        if (DoRectsIntersect(viewportAnchorBounds, anchorCandidateBounds))
        {
            // Using the distances from the viewport anchor point to the four corners of the anchor candidate.
            double anchorCandidateDistance{ 0.0 };

            if (!isnan(viewportAnchorPointHorizontalOffset))
            {
                anchorCandidateDistance += std::pow(viewportAnchorPointHorizontalOffset - anchorCandidateBounds.X, 2);
                anchorCandidateDistance += std::pow(viewportAnchorPointHorizontalOffset - (anchorCandidateBounds.X + anchorCandidateBounds.Width), 2);
            }

            if (!isnan(viewportAnchorPointVerticalOffset))
            {
                anchorCandidateDistance += std::pow(viewportAnchorPointVerticalOffset - anchorCandidateBounds.Y, 2);
                anchorCandidateDistance += std::pow(viewportAnchorPointVerticalOffset - (anchorCandidateBounds.Y + anchorCandidateBounds.Height), 2);
            }

            if (anchorCandidateDistance <= *bestAnchorCandidateDistance)
            {
                *bestAnchorCandidate = anchorCandidate;
                *bestAnchorCandidateBounds = anchorCandidateBounds;
                *bestAnchorCandidateDistance = anchorCandidateDistance;
            }
        }
    }

    return S_OK;
}

// Returns the bounds of a ScrollViewer.Content descendant in respect to that child.
_Check_return_ HRESULT ScrollViewer::GetDescendantBounds(
    _In_ xaml::IUIElement* child,
    _In_ xaml::IUIElement* descendant,
    _Out_ wf::Rect* descendantBounds)
{
    ASSERT(child);

    ctl::ComPtr<xaml::IUIElement> descendantUI(descendant);
    ctl::ComPtr<xaml::IFrameworkElement> descendantAsFE;
    IFC_RETURN(descendantUI.As<xaml::IFrameworkElement>(&descendantAsFE));
    DOUBLE width = 0.0;
    DOUBLE height = 0.0;

    if (descendantAsFE)
    {
        IFC_RETURN(descendantAsFE->get_ActualWidth(&width));
        IFC_RETURN(descendantAsFE->get_ActualHeight(&height));
    }

    const wf::Rect descendantRect{
        0.0f,
        0.0f,
        static_cast<float>(width),
        static_cast<float>(height)
    };

    IFC_RETURN(GetDescendantBounds(child, descendant, descendantRect, descendantBounds));

    return S_OK;
}

_Check_return_ HRESULT ScrollViewer::GetDescendantBounds(
    _In_ xaml::IUIElement* child,
    _In_ xaml::IUIElement* descendant,
    const wf::Rect& descendantRect,
    _Out_ wf::Rect* descendantBounds)
{
    ASSERT(child);
    xaml::Thickness childMargin{};

    ctl::ComPtr<xaml::IUIElement> descendantUI(descendant);
    ctl::ComPtr<xaml_media::IGeneralTransform> transform;
    IFC_RETURN(descendantUI.Cast<UIElement>()->TransformToVisual(child, &transform));

    ctl::ComPtr<xaml::IUIElement> childAsUI(child);
    ctl::ComPtr<xaml::IFrameworkElement> childAsFE = childAsUI.AsOrNull<xaml::IFrameworkElement>();
    if (childAsFE)
    {
        IFC_RETURN(childAsFE.Cast<FrameworkElement>()->get_Margin(&childMargin));
    }

    auto bounds = wf::Rect{
        static_cast<float>(childMargin.Left + descendantRect.X),
        static_cast<float>(childMargin.Top + descendantRect.Y),
        descendantRect.Width,
        descendantRect.Height };
    IFC_RETURN(transform.Cast<GeneralTransform>()->TransformBounds(bounds, descendantBounds));

    return S_OK;
}

_Check_return_ HRESULT ScrollViewer::IsElementValidAnchor(
    _In_ xaml::IUIElement* element,
    _Out_ bool* result)
{
    ctl::ComPtr<xaml::IUIElement> child;
    IFC_RETURN(GetContentUIElement(&child));
    IFC_RETURN(IsElementValidAnchor(element, child.Get(), result));
    return S_OK;
}

_Check_return_ HRESULT ScrollViewer::IsElementValidAnchor(
    _In_ xaml::IUIElement* element,
    _In_ xaml::IUIElement* child,
    _Out_ bool* isValid)
{
    *isValid = false;
    ASSERT(element);
    ASSERT(child);

    xaml::Visibility visibility = xaml::Visibility_Collapsed;
    ctl::ComPtr<xaml::IUIElement> elementAsUI(element);
    IFC_RETURN(elementAsUI.Cast<UIElement>()->get_Visibility(&visibility));
    if (visibility == xaml::Visibility_Visible)
    {
        if (element == child)
        {
            *isValid = true;
        }
        else
        {
            ctl::ComPtr<xaml::IUIElement> childAsUI(child);
            BOOLEAN isAncestor = FALSE;
            IFC_RETURN(childAsUI.AsOrNull<xaml::IDependencyObject>().Cast<DependencyObject>()->IsAncestorOf(elementAsUI.AsOrNull<xaml::IDependencyObject>().Cast<DependencyObject>(), &isAncestor));
            *isValid = !!isAncestor;
        }
    }

    return S_OK;
}

bool ScrollViewer::DoRectsIntersect(
    const wf::Rect& rect1,
    const wf::Rect& rect2)
{
    auto doIntersect =
        !(rect1.Width <= 0 || rect1.Height <= 0 || rect2.Width <= 0 || rect2.Height <= 0) &&
        (rect2.X <= rect1.X + rect1.Width) &&
        (rect2.X + rect2.Width >= rect1.X) &&
        (rect2.Y <= rect1.Y + rect1.Height) &&
        (rect2.Y + rect2.Height >= rect1.Y);
    return doIntersect;
}


// Used to perform a flickerless shift of ScrollViewer's offset.
_Check_return_ HRESULT ScrollViewer::PerformPositionAdjustment(bool isHorizontalDimension, float unzoomedAdjustment, const XRECTF& zoomedViewport)
{
    FLOAT zoomFactor = 1.0f;
    IFC_RETURN(get_ZoomFactor(&zoomFactor));
    float zoomedAdjustment = unzoomedAdjustment * zoomFactor;

    ctl::ComPtr<IInspectable> spHorizontalOffset;
    ctl::ComPtr<IInspectable> spVerticalOffset;
    ctl::ComPtr<wf::IReference<DOUBLE>> spHorizontalOffsetReference;
    ctl::ComPtr<wf::IReference<DOUBLE>> spVerticalOffsetReference;

    if (isHorizontalDimension)
    {
        ANCHORING_DEBUG_TRACE(L"SV[0x%p]: PerformPositionAdjustment X -> %f %lf", this, unzoomedAdjustment, zoomedViewport.X);
        DOUBLE zoomedHorizontalOffset = zoomedViewport.X;

        if (zoomedAdjustment < 0.0f && -zoomedAdjustment > zoomedHorizontalOffset)
        {
            // Do not let the HorizontalOffset step into negative territory.
            zoomedAdjustment = static_cast<float>(-zoomedHorizontalOffset);
        }

        DOUBLE newHorizontalOffset = zoomedAdjustment + zoomedHorizontalOffset;
        ANCHORING_DEBUG_TRACE(L"SV[0x%p]: NewHorizontalOffset -> %lf", this, newHorizontalOffset);

        IFC_RETURN(PropertyValue::CreateFromDouble(newHorizontalOffset, &spHorizontalOffset));
        IFC_RETURN(spHorizontalOffset.As(&spHorizontalOffsetReference));
    }
    else
    {
        ANCHORING_DEBUG_TRACE(L"SV[0x%p]: PerformPositionAdjustment Y -> %f %lf", this, unzoomedAdjustment, zoomedViewport.Y);
        DOUBLE zoomedVerticalOffset = zoomedViewport.Y;

        if (zoomedAdjustment < 0.0f && -zoomedAdjustment > zoomedVerticalOffset)
        {
            // Do not let the VerticalOffset step into negative territory.
            zoomedAdjustment = static_cast<float>(-zoomedVerticalOffset);
        }

        DOUBLE newVerticalOffset = zoomedAdjustment + zoomedVerticalOffset;
        ANCHORING_DEBUG_TRACE(L"SV[0x%p]: NewVerticalOffset -> %lf", this, newVerticalOffset);
        IFC_RETURN(PropertyValue::CreateFromDouble(newVerticalOffset, &spVerticalOffset));
        IFC_RETURN(spVerticalOffset.As(&spVerticalOffsetReference));
    }

    BOOLEAN handled = false;
    RRETURN(ChangeViewInternal(
        spHorizontalOffsetReference.Get(),
        spVerticalOffsetReference.Get(),
        nullptr /* pZoomFactor */,
        nullptr /*pOldZoomFactor*/,
        false /*forceChangeToCurrentView*/,
        true /*adjustWithMandatorySnapPoints*/,
        true /*skipDuringTouchContact*/,
        true /*skipAnimationWhileRunning*/,
        true /*disableAnimation*/,
        true /*applyAsManip*/,
        false /*transformIsInertiaEnd*/,
        false /*isForMakeVisible*/,
        &handled));

    return S_OK;
}

_Check_return_ HRESULT ScrollViewer::get_ZoomedHorizontalOffsetWithPendingShifts(_Out_ float* value)
{
    DOUBLE zoomedHorizontalOffset;
    IFC_RETURN(get_HorizontalOffset(&zoomedHorizontalOffset));
    zoomedHorizontalOffset += m_pendingViewportShiftX;
    *value = static_cast<float>(zoomedHorizontalOffset);
    return S_OK;
}

_Check_return_ HRESULT ScrollViewer::get_ZoomedVerticalOffsetWithPendingShifts(_Out_ float* value)
{
    DOUBLE zoomedVerticalOffset;
    IFC_RETURN(get_VerticalOffset(&zoomedVerticalOffset));
    zoomedVerticalOffset += m_pendingViewportShiftY;
    *value = static_cast<float>(zoomedVerticalOffset);
    return S_OK;
}

_Check_return_ HRESULT ScrollViewer::EnsureAnchorCandidateVector()
{
    if (!m_anchorCandidates.Get())
    {
        ctl::ComPtr<TrackerCollection<xaml::UIElement*>> candidates;
        IFC_RETURN(ctl::make<TrackerCollection<xaml::UIElement*>>(&candidates));
        SetPtrValue(m_anchorCandidates, candidates);

        ctl::ComPtr<TrackerCollection<xaml::UIElement*>> candidatesForArgs;
        IFC_RETURN(ctl::make<TrackerCollection<xaml::UIElement*>>(&candidatesForArgs));
        SetPtrValue(m_anchorCandidatesForArgs, candidatesForArgs);
    }

    return S_OK;
}
