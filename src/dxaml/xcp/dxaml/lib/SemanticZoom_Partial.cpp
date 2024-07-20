// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Represents a scrollable area that can contain either a zoomed in view of
//      content or a zoomed out view used to navigate around the content via zoom
//      gestures.

#include "precomp.h"
#include "SemanticZoom.g.h"
#include "SemanticZoomAutomationPeer.g.h"
#include "ScrollViewer.g.h"
#include "Button.g.h"
#include "VisualStateGroup.g.h"
#include "Storyboard.g.h"
#include "ListViewBase.g.h"
#include "Window.g.h"
#include "Border.g.h"
#include "Popup.g.h"
#include "SemanticZoomLocation.g.h"
#include "DispatcherTimer.g.h"
#include "SemanticZoomViewChangedEventArgs.g.h"
#include "CommandBar.g.h"
#include "Page.g.h"
#include <XamlTraceLogging.h>
#include <FrameworkUdk/BackButtonIntegration.h>
#include <DesignMode.h>
#include <XamlOneCoreTransforms.h>
#include <windows.graphics.display.h>
#include "DesktopUtility.h"
#include "ElementSoundPlayerService_Partial.h"

// Uncomment to get SemanticZoom debug traces
// #define SEZO_DBG


#ifdef SEZO_DBG
#define g_szSEZOLen 600
WCHAR g_szSEZODbg[g_szSEZOLen];
#endif // SEZO_DBG

// values that influence the switching point
// naming to match DUI implementation. Please leave as-is.
// these values should be kept in sync with the DUI values once they
// have been chosen
#define c_thresholdDeltaMin 0.05
#define c_thresholdDeltaMax 0.95
#define c_thresholdBufferDeltaMin 0.01
#define c_thresholdBufferDeltaMax 0.10
#define _upperThresholdDelta 0.05
#define _lowerThresholdDelta 0.05
#define _thresholdBufferDelta 0.05

#define zoomDeltaThreshold 0.01  // No semantic switch will occur if the zoom factor has changed less than this value

// the ScrollViewer will apply this zoomfactor in the zoomed-in view
#define _zoomMax 1.0
// the ScrollViewer will apply this zoomfactor in the zoomed-out view
#define _zoomMin 0.5

// These values are used by tracing to note how a user zoomed in or out
#define _zoomedClick 0
#define _zoomedWheel 1
#define _zoomedPinch 2

// Calling BringIntoViewport unnecessarily is an issue
// these constants define the minimum delta we want to see before
// making the call
#define c_minimumZoomDelta 0.001
#define c_minimumBoundsDelta 1.0

// in several location we have to calculate how the zoomedinview is centered
#define ZOOMEDINVIEWCENTERINGCORRECTIONX(inZoomedInView, viewportWidth) (inZoomedInView ? viewportWidth * _zoomMin : 0)
#define ZOOMEDINVIEWCENTERINGCORRECTIONY(inZoomedInView, viewportHeight) (inZoomedInView ? viewportHeight * _zoomMin : 0)

using namespace DirectUI;
using namespace DirectUISynonyms;

// Initializes a new instance of the SemanticZoom class.
SemanticZoom::SemanticZoom()
    : m_isInitializing(TRUE)
    , m_isPendingViewChange(FALSE)
    , m_isProcessingKeyboardInput(FALSE)
    , m_isProcessingPointerInput(FALSE)
    , m_isCancellingJumpList(FALSE)
    , m_zoomOriginatesFromZoomedInView(FALSE)
    , _upperThresholdLow(0.0)
    , _upperThresholdHigh(0.0)
    , _lowerThresholdLow(0.0)
    , _lowerThresholdHigh(0.0)
    , m_isZoomedInViewAnimationHooked(FALSE)
    , m_isZoomedOutViewAnimationHooked(FALSE)
    , m_emulatingGesture(FALSE)
    , m_changePhase(SemanticZoomPhase_Idle)
    , m_phaseChangeLockDuringViewSwitch(FALSE)
    , m_calledInitializeViewChangeSinceManipulationStart(FALSE)
    , m_cumulativeZoomFactorAtStartOfManipulation(1.0)
    , m_isProcessingViewChange(FALSE)
    , m_isZoomOutButtonEnabled(TRUE)
    , m_hasAutomationPeer(FALSE)
{
    m_zoomPoint.X = 0;
    m_zoomPoint.Y = 0;
    m_zoomPointForZoomedInView.X = 0;
    m_zoomPointForZoomedInView.Y = 0;
    m_zoomPointForZoomedOutView.X = 0;
    m_zoomPointForZoomedOutView.Y = 0;
    m_sizeChangedToken.value = 0;
    m_zoomedInViewSizeChangedToken.value = 0;
    m_zoomedOutViewSizeChangedToken.value = 0;
    m_elementZoomOutButtonClickToken.value = 0;
    m_manipulatedElementOffset.X = 0;
    m_manipulatedElementOffset.Y = 0;
}

// Destroys an instance of the SemanticZoom class.
SemanticZoom::~SemanticZoom()
{
    m_tpCompletedArgs.Clear();

    if (auto peg = m_tpScrollViewer.TryMakeAutoPeg())
    {
        VERIFYHR(m_tpScrollViewer.Cast<ScrollViewer>()->SetDirectManipulationStateChangeHandler(NULL));
    }

    if (DXamlCore::GetCurrent() != nullptr)
    {
        VERIFYHR(BackButtonIntegration_UnregisterListener(this));
    }

    // Makes sure the alternate view timer is stopped.
    auto spAlternateViewTimer = m_tpAlternateViewTimer.GetSafeReference();
    if (spAlternateViewTimer)
    {
        IGNOREHR(spAlternateViewTimer->Stop());
    }
}

// Handles custom property changed events and calls their OnPropertyChanged2
// methods.
_Check_return_ HRESULT SemanticZoom::OnPropertyChanged2(_In_ const PropertyChangedParams& args)
{
    HRESULT hr = S_OK;

    IFC(SemanticZoomGenerated::OnPropertyChanged2(args));

    switch (args.m_pDP->GetIndex())
    {
        case KnownPropertyIndex::SemanticZoom_IsZoomedInViewActive:
            {
                BOOLEAN canChangeViews = TRUE;

                IFC(get_CanChangeViews(&canChangeViews));
                IFCEXPECT(canChangeViews);

                if (!m_isInitializing)
                {
                    // Only change views if we're not in the middle of initialization.
                    // initialization is now defined from ctor until template has been applied.
                    if (!m_phaseChangeLockDuringViewSwitch)
                    {
                        m_changePhase = SemanticZoomPhase_API_SwitchingViews;
                    }
                    IFC(ChangeViews());
                }
                else
                {
                    // Otherwise, postpone the view change until EndInit is called.
                    // If a change was already scheduled, cancel the view change so we don't
                    // toggle to the incorrect view.
                    m_isPendingViewChange = !m_isPendingViewChange;
                }
                break;
            }
        case KnownPropertyIndex::SemanticZoom_ZoomedInView:
            {
                ctl::ComPtr<IInspectable> spOldValue, spNewValue;
                IFC(CValueBoxer::UnboxObjectValue(args.m_pOldValue, args.m_pDP->GetPropertyType(), &spOldValue));
                IFC(CValueBoxer::UnboxObjectValue(args.m_pNewValue, args.m_pDP->GetPropertyType(), &spNewValue));
                IFC(InitializeSemanticZoomInformation(spOldValue.Get(), spNewValue.Get(), /* isZoomedInView */ TRUE));
                break;
            }
        case KnownPropertyIndex::SemanticZoom_ZoomedOutView:
            {
                ctl::ComPtr<IInspectable> spOldValue, spNewValue;
                IFC(CValueBoxer::UnboxObjectValue(args.m_pOldValue, args.m_pDP->GetPropertyType(), &spOldValue));
                IFC(CValueBoxer::UnboxObjectValue(args.m_pNewValue, args.m_pDP->GetPropertyType(), &spNewValue));
                IFC(InitializeSemanticZoomInformation(spOldValue.Get(), spNewValue.Get(), /* isZoomedInView */ FALSE));
                break;
            }
        case KnownPropertyIndex::SemanticZoom_IsZoomOutButtonEnabled:
            {
                BOOLEAN isZoomOutButtonEnabled = FALSE;

                IFC(get_IsZoomOutButtonEnabled(&isZoomOutButtonEnabled));
                m_isZoomOutButtonEnabled = isZoomOutButtonEnabled;
                break;
            }
    }

Cleanup:
    RRETURN(hr);
}

// Associate the SemanticZoom with an ISemanticZoomInformation view.
_Check_return_ HRESULT SemanticZoom::InitializeSemanticZoomInformation(
    _In_ IInspectable* pOldValue,
    _In_ IInspectable* pNewValue,
    _In_ BOOLEAN isZoomedInView)
{
    HRESULT hr = S_OK;

    if (pOldValue)
    {
        ctl::ComPtr<IInspectable> spOldValue = NULL;
        ctl::ComPtr<ISemanticZoomInformation> spOldView = NULL;

        spOldValue = pOldValue;
        IFC(spOldValue.As<ISemanticZoomInformation>(&spOldView));
        if (spOldView)
        {
            IFC(spOldView->put_SemanticZoomOwner(NULL));
            IFC(spOldView->put_IsActiveView(FALSE));
            IFC(spOldView->put_IsZoomedInView(TRUE));
        }
    }

    if (pNewValue)
    {
        ctl::ComPtr<IInspectable> spNewValue = NULL;
        ctl::ComPtr<ISemanticZoomInformation> spNewView = NULL;

        spNewValue = pNewValue;
        IFC(spNewValue.As<ISemanticZoomInformation>(&spNewView));
        if (spNewView)
        {
            BOOLEAN isZoomedInViewActive = FALSE;

            IFC(spNewView->put_SemanticZoomOwner(this));
            IFC(spNewView->put_IsZoomedInView(isZoomedInView));

            IFC(get_IsZoomedInViewActive(&isZoomedInViewActive));
            IFC(spNewView->put_IsActiveView(isZoomedInView == isZoomedInViewActive));
        }
    }

Cleanup:
    RRETURN(hr);
}

// Get the view container template parts.
IFACEMETHODIMP SemanticZoom::OnApplyTemplate()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IScrollViewer> spScrollViewer = NULL;
    ctl::ComPtr<IButton> spZoomOutButton = NULL;
    ctl::ComPtr<IPopup> spPopup = NULL;
    ctl::ComPtr<IBorder> spBorder = NULL;

    // getting to the VisualState
    ctl::ComPtr<wfc::IVector<xaml::VisualTransition*>> spVisualTransitions;
    ctl::ComPtr<IVisualTransition> spTransition;
    ctl::ComPtr<IStoryboard> spTransitionStoryboard;

    ctl::ComPtr<IFrameworkElement> spZoomedInPresenterPart;
    ctl::ComPtr<IFrameworkElement> spZoomedOutPresenterPart;
    ctl::ComPtr<ICompositeTransform> spZoomedOutTransform;
    ctl::ComPtr<ICompositeTransform> spZoomedInTransform;
    ctl::ComPtr<ICompositeTransform> spManipulatedElementTransform;
    UINT nTransitionCount = 0;

    ctl::ComPtr<IVisualState> spZoomedInState;
    ctl::ComPtr<IVisualState> spZoomedOutState;
    ctl::ComPtr<IVisualState> spZoomOutButtonVisibleState;
    ctl::ComPtr<IVisualStateGroup> spGroup;

    BOOLEAN hasZoomedOutState = FALSE;
    BOOLEAN hasZoomedInState = FALSE;
    BOOLEAN hasZoomOutButtonVisibleState = FALSE;

    // reset the flags that indicate whether we have hooked the events
    m_isZoomedInViewAnimationHooked = m_isZoomedOutViewAnimationHooked = FALSE;

    if (m_tpScrollViewer)
    {
        IFC(m_tpScrollViewer.Cast<ScrollViewer>()->put_ArePointerWheelEventsIgnored(FALSE));
        IFC(m_tpScrollViewer.Cast<ScrollViewer>()->SetDirectManipulationStateChangeHandler(NULL));
    }

    if (m_elementZoomOutButtonClickToken.value != 0 && m_tpZoomOutButton)
    {
        IFC(m_tpZoomOutButton.Cast<Button>()->remove_Click(m_elementZoomOutButtonClickToken));
        ZeroMemory(&m_elementZoomOutButtonClickToken, sizeof(m_elementZoomOutButtonClickToken));
    }

    m_tpScrollViewer.Clear();
    m_tpZoomOutButton.Clear();

    // unhook events on templateparts
    if (m_zoomedInViewSizeChangedToken.value != 0 && m_tpZoomedInPresenterPart)
    {
        IFC(m_tpZoomedInPresenterPart.Cast<FrameworkElement>()->remove_SizeChanged(m_zoomedInViewSizeChangedToken));
        ZeroMemory(&m_zoomedInViewSizeChangedToken, sizeof(m_zoomedInViewSizeChangedToken));
    }
    if (m_zoomedOutViewSizeChangedToken.value != 0 && m_tpZoomedOutPresenterPart)
    {
        IFC(m_tpZoomedOutPresenterPart.Cast<FrameworkElement>()->remove_SizeChanged(m_zoomedOutViewSizeChangedToken));
        ZeroMemory(&m_zoomedOutViewSizeChangedToken, sizeof(m_zoomedOutViewSizeChangedToken));
    }

    IFC(SemanticZoomGenerated::OnApplyTemplate());

    IFC(GetTemplatePart<IFrameworkElement>(STR_LEN_PAIR(L"ZoomedInPresenter"), spZoomedInPresenterPart.GetAddressOf()));
    IFC(GetTemplatePart<IFrameworkElement>(STR_LEN_PAIR(L"ZoomedOutPresenter"), spZoomedOutPresenterPart.GetAddressOf()));
    IFC(GetTemplatePart<ICompositeTransform>(STR_LEN_PAIR(L"ZoomedOutTransform"), spZoomedOutTransform.GetAddressOf()));
    IFC(GetTemplatePart<ICompositeTransform>(STR_LEN_PAIR(L"ZoomedInTransform"), spZoomedInTransform.GetAddressOf()));
    IFC(GetTemplatePart<ICompositeTransform>(STR_LEN_PAIR(L"ManipulatedElementTransform"), spManipulatedElementTransform.GetAddressOf()));

    SetPtrValue(m_tpZoomedInPresenterPart, spZoomedInPresenterPart.Get());
    SetPtrValue(m_tpZoomedOutPresenterPart, spZoomedOutPresenterPart.Get());
    SetPtrValue(m_tpZoomedOutTransform, spZoomedOutTransform.Get());
    SetPtrValue(m_tpZoomedInTransform, spZoomedInTransform.Get());
    SetPtrValue(m_tpManipulatedElementTransform, spManipulatedElementTransform.Get());

    // reset the state we are in
    m_changePhase = SemanticZoomPhase_Idle;
    m_phaseChangeLockDuringViewSwitch = FALSE;

    // set the thresholds for the first time.
    IFC(UpdateThresholds());

    IFC(GetTemplatePart<IScrollViewer>(STR_LEN_PAIR(L"ScrollViewer"), spScrollViewer.ReleaseAndGetAddressOf()));
    if (spScrollViewer)
    {
        SetPtrValue(m_tpScrollViewer, spScrollViewer.Get());
        m_tpScrollViewer.Cast<ScrollViewer>()->m_templatedParentHandlesMouseButton = TRUE;
        m_tpScrollViewer.Cast<ScrollViewer>()->m_ignoreSemanticZoomNavigationInput = TRUE;

        // Ignore mouse wheel (except for zooming)
        IFC(m_tpScrollViewer.Cast<ScrollViewer>()->put_ArePointerWheelEventsIgnored(TRUE));

        IFC(m_tpScrollViewer.Cast<ScrollViewer>()->SetDirectManipulationStateChangeHandler(static_cast<DirectManipulationStateChangeHandler*>(this)));

        IFC(m_tpScrollViewer->put_BringIntoViewOnFocusChange(FALSE));

        // set correction scales
        // Two views:
        // 1. ZoomedInView, we will be at _zoomMax, which is a zoomfactor of 1.
        //    In that zoomfactor, we wish the zoomedinview to visually look like it is unscaled.
        // 2. ZoomedOutView, we will be at _zoomMin, which is a zoomfactor of 0.5.
        //    In that zoomfactor, we wish the zoomedoutview to visually look like it is unscaled.

        // we do this 'trick' so that we can gradually go from a zoomfactor of 1 to 0.5 and bring in the zoomedoutview at some
        // zoomfactor in between (let's say 0.7). At that zoomfactor the zoomedoutview will look as though it is scaled UP.
        // when the user releases the fingers, we will settle at 0.5. At that point the zoomedoutview will look perfect.

        // the trick is to scale everything up by 2 and apply a correction on the zoomedinview
        if (m_tpManipulatedElementTransform)
        {
            // this zooms the whole tree up by 2 (and in generic.xaml the origin is specified to be (0,0) )
            // at zoomfactor 1 this would look weird (but we're faded out anyway).
            IFC(m_tpManipulatedElementTransform.Get()->put_ScaleX(1/_zoomMin));
            IFC(m_tpManipulatedElementTransform.Get()->put_ScaleY(1/_zoomMin));
        }
        if (m_tpZoomedInTransform)
        {
            // correction scale so that zoomedinview looks good at at zoomfactor 1
            IFC(m_tpZoomedInTransform.Get()->put_ScaleX(_zoomMin));
            IFC(m_tpZoomedInTransform.Get()->put_ScaleY(_zoomMin));
        }
        // please look at the SizeChanged handler for the continuation of this setup. That part needs
        // to occur when we have a valid size.
    }

    IFC(GetTemplatePart<IButton>(STR_LEN_PAIR(L"ZoomOutButton"), spZoomOutButton.ReleaseAndGetAddressOf()));
    if (spZoomOutButton)
    {
        ctl::ComPtr<xaml::IRoutedEventHandler> spZoomOutButtonClickHandler;

        SetPtrValue(m_tpZoomOutButton, spZoomOutButton.Get());

        spZoomOutButtonClickHandler.Attach(
            new ClassMemberEventHandler<
                SemanticZoom,
                ISemanticZoom,
                xaml::IRoutedEventHandler,
                IInspectable,
                xaml::IRoutedEventArgs>(
                    this,
                    &SemanticZoom::OnZoomOutButtonClick));
        IFC(m_tpZoomOutButton.Cast<Button>()->add_Click(spZoomOutButtonClickHandler.Get(), &m_elementZoomOutButtonClickToken));
    }

    // and subscribe to size changed so that we can position our views once we have a size
    if (m_sizeChangedToken.value == 0)
    {
            ctl::ComPtr<xaml::ISizeChangedEventHandler> spSizeChangedHandler = NULL;

            spSizeChangedHandler.Attach(
                new ClassMemberEventHandler<
                SemanticZoom,
                xaml_controls::ISemanticZoom,
                xaml::ISizeChangedEventHandler,
                IInspectable,
                xaml::ISizeChangedEventArgs>(this, &SemanticZoom::OnSizeChanged, true /* subscribingToSelf */ ));

            IFC(add_SizeChanged(spSizeChangedHandler.Get(), &m_sizeChangedToken));
    }

    // also subscribe to the size of the views changing
    // since it can change without changing the size of this element
    if (m_tpZoomedInPresenterPart)
    {
        ctl::ComPtr<xaml::ISizeChangedEventHandler> spSizeChangedHandler = NULL;

        spSizeChangedHandler.Attach(
            new ClassMemberEventHandler<
            SemanticZoom,
            xaml_controls::ISemanticZoom,
            xaml::ISizeChangedEventHandler,
            IInspectable,
            xaml::ISizeChangedEventArgs>(this, &SemanticZoom::OnSizeChanged));

        IFC(m_tpZoomedInPresenterPart.Cast<FrameworkElement>()->add_SizeChanged(spSizeChangedHandler.Get(), &m_zoomedInViewSizeChangedToken));
    }

    if (m_tpZoomedOutPresenterPart)
    {
        ctl::ComPtr<xaml::ISizeChangedEventHandler> spSizeChangedHandler = NULL;

        spSizeChangedHandler.Attach(
            new ClassMemberEventHandler<
            SemanticZoom,
            xaml_controls::ISemanticZoom,
            xaml::ISizeChangedEventHandler,
            IInspectable,
            xaml::ISizeChangedEventArgs>(this, &SemanticZoom::OnSizeChanged));

        IFC(m_tpZoomedOutPresenterPart.Cast<FrameworkElement>()->add_SizeChanged(spSizeChangedHandler.Get(), &m_zoomedOutViewSizeChangedToken));
    }

    // Now that XAML parsing is over we will display our proper view.
    // All views start out collapsed, to not pay huge layout costs that might
    // be unnecessary.
    if (m_isPendingViewChange)
    {
        if (m_tpZoomedOutPresenterPart)
        {
            IFC(m_tpZoomedOutPresenterPart.Cast<FrameworkElement>()->put_Visibility(xaml::Visibility_Visible));
        }

        // ChangeViews will show a nice transition when we switch. In this case we need to
        // preempt that by going to the new VisualState immediately.
        // The UpdateVisualState(TRUE) call inside of ChangeViews will be a no-op.
        IFC(UpdateVisualState(FALSE));
        IFC(ChangeViews());
    }
    else
    {
        // we will just display the content view
        if (m_tpZoomedInPresenterPart)
        {
            IFC(m_tpZoomedInPresenterPart.Cast<FrameworkElement>()->put_Visibility(xaml::Visibility_Visible));
            IFC(m_tpZoomedInPresenterPart.Cast<FrameworkElement>()->put_IsHitTestVisible(TRUE));
            static_cast<CUIElement*>(m_tpZoomedInPresenterPart.Cast<FrameworkElement>()->GetHandle())->SetSkipFocusSubtree(false);

            if (m_tpZoomedOutPresenterPart)
            {
                static_cast<CUIElement*>(m_tpZoomedOutPresenterPart.Cast<FrameworkElement>()->GetHandle())->SetSkipFocusSubtree(true);
            }
        }

        // Ensure we display the correct view. If IsZoomedInViewActive is set while our template is not yet initialized,
        // we could show the user the incorrect view on startup.
        IFC(UpdateVisualState(FALSE));
    }

    IFC(VisualStateManager::TryGetState(this, L"ZoomInView", nullptr, &spZoomedInState, &hasZoomedInState));
    if (hasZoomedInState)
    {
        IFC(AddStoryboardCompletedHandler(spZoomedInState.Get(), &SemanticZoom::ViewChangeAnimationFinished));
        m_isZoomedInViewAnimationHooked = TRUE;
    }

    IFC(VisualStateManager::TryGetState(this, L"ZoomOutView", nullptr, &spZoomedOutState, &hasZoomedOutState));
    if (hasZoomedOutState)
    {
        IFC(AddStoryboardCompletedHandler(spZoomedOutState.Get(), &SemanticZoom::ViewChangeAnimationFinished));
        m_isZoomedOutViewAnimationHooked = TRUE;
    }

    IFC(VisualStateManager::TryGetState(this, L"ZoomOutButtonVisible", &spGroup, &spZoomOutButtonVisibleState, &hasZoomOutButtonVisibleState));
    if (hasZoomOutButtonVisibleState)
    {
        // Get button timer transition, mark it essential
        IFC(spGroup.Cast<VisualStateGroup>()->get_Transitions(&spVisualTransitions));
        if (spVisualTransitions)
        {
            IFC(spVisualTransitions->get_Size(&nTransitionCount));
            for (UINT j = 0; j < nTransitionCount; ++j)
            {
                IFC(spVisualTransitions->GetAt(j, &spTransition));
                IFC(spTransition->get_Storyboard(&spTransitionStoryboard));
                IFC(spTransitionStoryboard.Cast<Storyboard>()->put_IsEssential(TRUE));
            }
        }

        IFC(AddStoryboardCompletedHandler(spZoomOutButtonVisibleState.Get(), &SemanticZoom::OnZoomOutButtonVisibleStoryboardCompleted));
    }

    IFC(VisualStateManager::TryGetState(this,
                                        L"ZoomOutButtonVisible",
                                        spGroup.ReleaseAndGetAddressOf(),
                                        spZoomOutButtonVisibleState.ReleaseAndGetAddressOf(),
                                        &hasZoomOutButtonVisibleState));
    if (hasZoomOutButtonVisibleState)
    {
        wrl_wrappers::HString strStateName;
        wrl_wrappers::HStringReference strZoomOutButtonVisibleStateName(STR_LEN_PAIR(L"ZoomOutButtonVisible"));
        wrl_wrappers::HStringReference strZoomOutButtonHiddenStateName(STR_LEN_PAIR(L"ZoomOutButtonHidden"));
        // Get button timer transition, mark it essential
        IFC(spGroup.Cast<VisualStateGroup>()->get_Transitions(&spVisualTransitions));
        if (spVisualTransitions)
        {
            IFC(spVisualTransitions->get_Size(&nTransitionCount));
            for (UINT j = 0; j < nTransitionCount; ++j)
            {
                IFC(spVisualTransitions->GetAt(j, &spTransition));
                IFC(spTransition->get_To(strStateName.ReleaseAndGetAddressOf()));
                if (strStateName == strZoomOutButtonHiddenStateName)
                {
                    IFC(spTransition->get_From(strStateName.ReleaseAndGetAddressOf()));
                    if (strStateName == strZoomOutButtonVisibleStateName)
                    {
                        IFC(spTransition->get_Storyboard(&spTransitionStoryboard));
                        IFC(spTransitionStoryboard.Cast<Storyboard>()->put_IsEssential(TRUE));
                    }
                }
            }
        }

        IFC(AddStoryboardCompletedHandler(spZoomOutButtonVisibleState.Get(), &SemanticZoom::OnZoomOutButtonVisibleStoryboardCompleted));
    }

    IFC(HideZoomOutButton(false /* bUseTransitions */));

    // create a timer that will trigger the creation of our alternate view
    IFC(SetAlternateViewTimer());

Cleanup:
    m_isPendingViewChange = FALSE;
    m_isInitializing = FALSE;
    RRETURN(hr);
}

// brings the alternate view into layout.
_Check_return_ HRESULT SemanticZoom::SetupAlternateView(
    _In_opt_ IInspectable* pUnused1,
    _In_opt_ IInspectable* pUnused2)
{
    HRESULT hr = S_OK;

    IFC(m_tpAlternateViewTimer->Stop());

    // instead of finding out which view is active, I just set visibility to true
    // on both. Setting a property to the same value causes only little overhead.
    if (m_tpZoomedOutPresenterPart)
    {
        IFC(m_tpZoomedOutPresenterPart.Cast<FrameworkElement>()->put_Visibility(xaml::Visibility_Visible));
    }
    if (m_tpZoomedInPresenterPart)
    {
        IFC(m_tpZoomedInPresenterPart.Cast<FrameworkElement>()->put_Visibility(xaml::Visibility_Visible));
    }


Cleanup:
    RRETURN(hr);
}

// sets a timer to bring the currently not active view into layout
_Check_return_ HRESULT SemanticZoom::SetAlternateViewTimer()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<DispatcherTimer> spDispatcherTimer;
    ctl::ComPtr<wf::IEventHandler<IInspectable*>> spAlternateTimerTickEventHandler;
    EventRegistrationToken alternateViewTimerTickToken;
    wf::TimeSpan showDurationTimeSpan = { 150 };

    IFC(ctl::make(&spDispatcherTimer));

    SetPtrValue(m_tpAlternateViewTimer, spDispatcherTimer);

    spAlternateTimerTickEventHandler.Attach(
        new ClassMemberEventHandler<
            SemanticZoom,
            xaml_controls::ISemanticZoom,
            wf::IEventHandler<IInspectable*>,
            IInspectable,
            IInspectable>(this, &SemanticZoom::SetupAlternateView));

    IFC(m_tpAlternateViewTimer->add_Tick(spAlternateTimerTickEventHandler.Get(), &alternateViewTimerTickToken));

    IFC(m_tpAlternateViewTimer->put_Interval(showDurationTimeSpan));

    IFC(m_tpAlternateViewTimer->Start());

Cleanup:
    RRETURN(hr);
}

// Change to the correct visual state for the SemanticZoom.
_Check_return_ HRESULT SemanticZoom::ChangeVisualState(
    // true to use transitions when updating the visual state, false to snap
    // directly to the new visual state.
    _In_ bool bUseTransitions)
{
    HRESULT hr = S_OK;
    BOOLEAN bIgnored = FALSE;
    BOOLEAN isZoomedInViewActive = FALSE;

    IFC(get_IsZoomedInViewActive(&isZoomedInViewActive));
    if (!isZoomedInViewActive)
    {
        IFC(GoToState(bUseTransitions, L"ZoomOutView", &bIgnored));
    }
    else
    {
        IFC(GoToState(bUseTransitions, L"ZoomInView", &bIgnored));
    }

Cleanup:
    RRETURN(hr);
}

// Toggle the active view.
_Check_return_ HRESULT SemanticZoom::ToggleActiveViewImpl()
{
    BOOLEAN canChangeViews = TRUE;
    BOOLEAN isZoomedInViewActive = FALSE;
    BOOLEAN bAutomationListener = FALSE;

    IFC_RETURN(get_CanChangeViews(&canChangeViews));
    IFCEXPECT_RETURN(canChangeViews);

    IFC_RETURN(get_IsZoomedInViewActive(&isZoomedInViewActive));
    isZoomedInViewActive = !isZoomedInViewActive;
    IFC_RETURN(put_IsZoomedInViewActive(isZoomedInViewActive));

    IFC_RETURN(AutomationPeer::ListenerExistsHelper(xaml_automation_peers::AutomationEvents_PropertyChanged, &bAutomationListener));
    if (bAutomationListener)
    {
        ctl::ComPtr<xaml_automation_peers::IAutomationPeer> spAutomationPeer;
        ctl::ComPtr<xaml_automation_peers::ISemanticZoomAutomationPeer> spSemanticZoomAutomationPeer;

        IFC_RETURN(GetOrCreateAutomationPeer(&spAutomationPeer));
        if(spAutomationPeer)
        {
            IFC_RETURN(spAutomationPeer.As(&spSemanticZoomAutomationPeer));
            IFC_RETURN(spSemanticZoomAutomationPeer.Cast<SemanticZoomAutomationPeer>()->RaiseToggleStatePropertyChangedEvent(!isZoomedInViewActive));
        }
    }

    // Request a play show/hide sound for toggle active view
    IFC_RETURN(DirectUI::ElementSoundPlayerService::RequestInteractionSoundForElementStatic(
            isZoomedInViewActive ? xaml::ElementSoundKind_Show : xaml::ElementSoundKind_Hide, this));

    return S_OK;
}

_Check_return_ HRESULT
SemanticZoom::ToggleBackKeyListener(_In_ BOOLEAN isZoomedOutViewActive)
{
    if (DXamlCore::GetCurrent()->GetHandle()->BackButtonSupported())
    {
        if (isZoomedOutViewActive)
        {
            IFC_RETURN(BackButtonIntegration_RegisterListener(this));
        }
        else
        {
            IFC_RETURN(BackButtonIntegration_UnregisterListener(this));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
SemanticZoom::OnBackButtonPressedImpl(_Out_ BOOLEAN* pHandled)
{
    HRESULT hr = S_OK;
    m_isCancellingJumpList = TRUE;

    IFC(ToggleActiveView());
    *pHandled = TRUE;

Cleanup:
    m_isCancellingJumpList = FALSE;
    RRETURN(hr);
}


// Sets internal flags and then calls ToggleActiveView(), so that when
// focus changes as a result of ToggleActiveView(), the new item is focused
// using the specified FocusState
_Check_return_ HRESULT
SemanticZoom::ToggleActiveViewWithFocusState(
    _In_ xaml::FocusState focusState)
{
    HRESULT hr = S_OK;
    BOOLEAN canChangeViews = FALSE;

    IFC(get_CanChangeViews(&canChangeViews));

    if (canChangeViews)
    {
        if (xaml::FocusState_Keyboard == focusState)
        {
            m_isProcessingKeyboardInput = TRUE;
        }
        else if (xaml::FocusState_Pointer == focusState)
        {
            m_isProcessingPointerInput = TRUE;
        }
        else
        {
            ASSERT(FALSE, L"Error: ToggleActiveViewWithFocusState() only supports Keyboard and Pointer FocusStates");
        }

        IFC(ToggleActiveView());
    }

Cleanup:
    m_isProcessingKeyboardInput = FALSE;
    m_isProcessingPointerInput = FALSE;
    RRETURN(hr);
}

// DirectManipulationStateChangeHandler implementation
_Check_return_ HRESULT SemanticZoom::NotifyStateChange(
    _In_ DMManipulationState state,
    _In_ FLOAT xCumulativeTranslation,
    _In_ FLOAT yCumulativeTranslation,
    _In_ FLOAT zCumulativeFactor,
    _In_ FLOAT xCenter,
    _In_ FLOAT yCenter,
    _In_ BOOLEAN isInertial,
    _In_ BOOLEAN isTouchConfigurationActivated,
    _In_ BOOLEAN isBringIntoViewportConfigurationActivated) noexcept
{
    HRESULT hr = S_OK;
    FLOAT zoomFactor = 1.0;
    BOOLEAN isHandled = FALSE;
    BOOLEAN canChangeViews = TRUE;
    BOOLEAN inZoomedInView = FALSE;

    #ifdef SEZO_DBG
    DOUBLE dbg_width = 0;
    DOUBLE dbg_height = 0;

    if (m_tpScrollViewer)
    {
        // notice use of viewport width for debugging ease
        IFC(m_tpScrollViewer.Cast<ScrollViewer>()->ComputePixelViewportWidth(NULL, FALSE, &dbg_width));
        IFC(m_tpScrollViewer.Cast<ScrollViewer>()->ComputePixelViewportHeight(NULL, FALSE, &dbg_height));
    }
    #endif // SEZO_DBG

    IFC(get_IsZoomedInViewActive(&inZoomedInView));

    switch (state)
    {
        case DMManipulationStarting:
            #ifdef SEZO_DBG
            swprintf_s(g_szSEZODbg, g_szSEZOLen,
                L"SeZo is getting the Starting DM state, width: %f, height %f.", dbg_width, dbg_height);
            Trace(g_szSEZODbg);
            #endif // SEZO_DBG
            if (m_tpScrollViewer)
            {
                m_tpScrollViewer.Cast<ScrollViewer>()->m_inSemanticZoomAnimation = TRUE;
            }

            // compare against this factor
            m_cumulativeZoomFactorAtStartOfManipulation = zCumulativeFactor;
            break;

        case DMManipulationStarted:

            // stores how we started this manipulation. This is being used during DManipulationDelta
            // to determine which thresholds to use

            // this value is not trustworthy if we did an api change. We have already set the inZoomedInView value
            m_zoomOriginatesFromZoomedInView = m_changePhase == SemanticZoomPhase_API_SwitchingViews ? !inZoomedInView : inZoomedInView;
            IFC(UpdateThresholds());

            // write down the center point so we might use that information during ChangeView
            m_zoomPoint.X = xCenter;
            m_zoomPoint.Y = yCenter;

            // setting the other zoompoints to (0,0) indicates that they need to be calculated at ChangeView time
            m_zoomPointForZoomedInView.X = m_zoomPointForZoomedInView.Y = m_zoomPointForZoomedOutView.X = m_zoomPointForZoomedOutView.Y = 0;

            #ifdef SEZO_DBG
            swprintf_s(g_szSEZODbg, g_szSEZOLen,
                L"SeZo is getting the Started DM state, width: %f, height %f, xCenter %f, yCenter %f.", dbg_width, dbg_height, xCenter, yCenter);
            Trace(g_szSEZODbg);
            #endif // SEZO_DBG

            break;

        case DMManipulationDelta:
        case DMManipulationLastDelta:
            if (state == DMManipulationDelta)
            {
                IFC(get_CanChangeViews(&canChangeViews));
                if (canChangeViews &&
                    !DoubleUtil::AreWithinTolerance(m_cumulativeZoomFactorAtStartOfManipulation, zCumulativeFactor, zoomDeltaThreshold) &&
                    (m_changePhase == SemanticZoomPhase_Idle || m_changePhase == SemanticZoomPhase_DM_SwitchingViews) &&
                    !isInertial)
                {
                    if (m_tpScrollViewer)
                    {
                        // the cumulative zoomfactor is calculated by the start of the manipulation and treating
                        // that zoomfactor as 1.0. This is not what we use to determine viewchange point by.
                        // Thresholds are calculated based on the actual zoomfactor.
                        IFC(m_tpScrollViewer.Cast<ScrollViewer>()->get_ZoomFactor(&zoomFactor));
                    }

                    if (m_zoomOriginatesFromZoomedInView)
                    {
                        if (inZoomedInView && zoomFactor < _upperThresholdLow)
                        {
                            isHandled = TRUE;
                        }
                        else if (!inZoomedInView && zoomFactor > _upperThresholdHigh)
                        {
                            isHandled = TRUE;
                        }
                    }
                    else
                    {
                        if (!inZoomedInView && zoomFactor > _lowerThresholdHigh)
                        {
                            isHandled = TRUE;
                        }
                        else if (inZoomedInView && zoomFactor < _lowerThresholdLow)
                        {
                            isHandled = TRUE;
                        }
                    }

                    if (isHandled)
                    {
                        #ifdef SEZO_DBG
                        swprintf_s(g_szSEZODbg, g_szSEZOLen,
                            L"SeZo is in a delta that crosses a threshold. Calling ChangeView. zoomFactor %f.",
                                   zoomFactor);
                        Trace(g_szSEZODbg);
                        #endif // SEZO_DBG

                        // by putting a lock on this, and calling put_IsZoomedInViewActive, we tell the handler to not
                        // change the phase. By calling the property, normally we would get a phase of _API_SwitchingViews.
                        // This is the only place where that is not correct.
                        m_phaseChangeLockDuringViewSwitch = TRUE;
                        m_changePhase = SemanticZoomPhase_DM_SwitchingViews;
                        IFC(put_IsZoomedInViewActive(!inZoomedInView));
                    }

                    // initialize the viewchange as soon as a DM manipulation starts
                    if (canChangeViews && !m_calledInitializeViewChangeSinceManipulationStart)
                    {
                            ctl::ComPtr<ISemanticZoomInformation> spSourceView = NULL;
                            ctl::ComPtr<ISemanticZoomInformation> spDestinationView = NULL;

                            // capture that any ChangeView will have been caused by pointer input
                            m_isProcessingPointerInput = TRUE;

                            // Get the source and destination views/parts
                            if (inZoomedInView)
                            {
                                IFC(get_ZoomedInView(&spSourceView));
                                IFC(get_ZoomedOutView(&spDestinationView));
                            }
                            else
                            {
                                IFC(get_ZoomedInView(&spDestinationView));
                                IFC(get_ZoomedOutView(&spSourceView));
                            }

                            if (spSourceView)
                            {
                                IFC(spSourceView->InitializeViewChange());
                            }
                            if (spDestinationView)
                            {
                                IFC(spDestinationView->InitializeViewChange());
                            }

                            m_calledInitializeViewChangeSinceManipulationStart = TRUE;
                    }

                }  // switching because of DM input
            } // state is delta


            if (m_tpScrollViewer)
            {
                // user interrupts when we see that we are not using a BringIntoViewportConfiguration
                // even though we are in a state where we absolutely expect one
                if ((m_changePhase == SemanticZoomPhase_API_SwitchingViews ||
                     m_changePhase == SemanticZoomPhase_DM_CompletingViews) &&
                    !isBringIntoViewportConfigurationActivated)
                {
                    // we interrupt, so we wish to go to idle again.
                    // notice that we do not call Initialize again
                    // since we are guaranteed a completed state still.
                    m_changePhase = SemanticZoomPhase_Idle;

                    #ifdef SEZO_DBG
                    swprintf_s(g_szSEZODbg, g_szSEZOLen,
                        L"SeZo is interrupting the biv call DMState: %d, changephase %d.",
                               state, m_changePhase);
                    Trace(g_szSEZODbg);
                    #endif // SEZO_DBG

                    // we have interrupted, potentially a zoom. The factor that we currently are in,
                    // should be used to indicate whether we have truly zoomed
                    m_cumulativeZoomFactorAtStartOfManipulation = zCumulativeFactor;

                    // Temporary workaround for DManip bug 799346
                    // Undo the zoom factor boundary adjustments
                    IFC(m_tpScrollViewer.Cast<ScrollViewer>()->SetDirectManipulationOverridingZoomBoundaries());
                }

                // nicely animate to our final position when the fingers were let go (we entered inertia mode)
                // Inertia may start in DMManipulationDelta or DMManipulationLastDelta
                else if (((state == DMManipulationDelta && isInertial) || state == DMManipulationLastDelta) &&
                    (m_changePhase == SemanticZoomPhase_Idle || m_changePhase == SemanticZoomPhase_DM_SwitchingViews ))
                {
                    // Since we live in a stretched up ScrollViewer, we take control by calling BringIntoViewport
                    XRECTF bounds = {0, 0, 0, 0 };
                    DOUBLE offsetX = 0;
                    DOUBLE offsetY = 0;
                    BOOLEAN handled = FALSE;

                    IFC(CalculateBounds(&bounds));

                    IFC(m_tpScrollViewer.Cast<ScrollViewer>()->get_ZoomFactor(&zoomFactor));
                    IFC(m_tpScrollViewer.Cast<ScrollViewer>()->get_HorizontalOffset(&offsetX));
                    IFC(m_tpScrollViewer.Cast<ScrollViewer>()->get_VerticalOffset(&offsetY));

                    // crucial to only call BringIntoViewport if there is a change to scroll to.
                    // if we do not, we get into deadlocks, infinite cycles and asserts.
                    if ( DoubleUtil::Abs(zoomFactor - (inZoomedInView ? _zoomMax : _zoomMin)) > c_minimumZoomDelta ||
                         DoubleUtil::Abs(bounds.X * (inZoomedInView ? _zoomMax : _zoomMin) - offsetX) >= c_minimumBoundsDelta ||
                         DoubleUtil::Abs(bounds.Y * (inZoomedInView ? _zoomMax : _zoomMin) - offsetY) >= c_minimumBoundsDelta)
                    {
                        #ifdef SEZO_DBG
                        swprintf_s(g_szSEZODbg, g_szSEZOLen,
                            L"SeZo is doing a biv animation to finish a gesture, DMState: %d, bounds.X: %f, bounds.Y: %f, bounds.Width: %f, bounds.Height: %f, zf: %f, offsetX: %f, offsetY: %f.",
                                   state, bounds.X, bounds.Y, bounds.Width, bounds.Height, zoomFactor, offsetX, offsetY);
                        Trace(g_szSEZODbg);
                        #endif // SEZO_DBG

                        m_changePhase = SemanticZoomPhase_DM_CompletingViews;

                        // Temporary workaround for DManip bug 799346
                        IFC(m_tpScrollViewer.Cast<ScrollViewer>()->SetDirectManipulationOverridingZoomBoundaries());

                        IFC(m_tpScrollViewer.Cast<ScrollViewer>()->BringIntoViewport(
                            bounds,
                            FALSE /*skipDuringTouchContact*/,
                            FALSE /*skipAnimationWhileRunning*/,
                            TRUE  /*animate*/,
                            &handled));
                    }
                    else
                    {
                        // did not have to complete this for some reason, so reset to idle
                        m_changePhase = SemanticZoomPhase_Idle;
                    }
                }
            }
            break;

        case DirectUI::DMManipulationCompleted:

            XRECTF bounds = {0, 0, 0, 0 };

            #ifdef SEZO_DBG
            swprintf_s(g_szSEZODbg, g_szSEZOLen,
                L"SeZo is getting a completed DM state, width: %f, height %f",
                       dbg_width, dbg_height);
            Trace(g_szSEZODbg);
            #endif // SEZO_DBG

            if (m_tpScrollViewer)
            {
                // Temporary workaround for DManip bug 799346
                IFC(m_tpScrollViewer.Cast<ScrollViewer>()->ResetDirectManipulationOverridingZoomBoundaries());
            }

            // a complete comes as either the end of the BringIntoView call by ChangeView
            // or by the completion of a gesture.
            // We complete the session only after we have completely finished our viewchange.
            // This is indicated by a cleared out m_tpCompletedArgs.
            // That means if the fadein/fadeout is still occurring
            // (we have not called OnViewChangeCompleted yet), we should not complete here, but
            // rely on ViewChangeAnimationFinished
            if (m_calledInitializeViewChangeSinceManipulationStart && !m_tpCompletedArgs.Get())
            {
                ctl::ComPtr<ISemanticZoomInformation> spSourceView = NULL;
                ctl::ComPtr<ISemanticZoomInformation> spDestinationView = NULL;

                m_calledInitializeViewChangeSinceManipulationStart = FALSE;

                // Get the source and destination views/parts
                if (inZoomedInView)
                {
                    IFC(get_ZoomedInView(&spDestinationView));
                    IFC(get_ZoomedOutView(&spSourceView));
                }
                else
                {
                    IFC(get_ZoomedInView(&spSourceView));
                    IFC(get_ZoomedOutView(&spDestinationView));
                }

                // Cleanup the views
                if (spSourceView)
                {
                    IFC(spSourceView->CompleteViewChange());
                }
                if (spDestinationView)
                {
                    IFC(spDestinationView->CompleteViewChange());
                }
            }

            // mark that DM is completely done with the animation.
            m_changePhase = SemanticZoomPhase_Idle;

            // get rid of all the corrections.
            if (m_tpZoomedInTransform)
            {
                IFC(m_tpZoomedInTransform.Get()->put_TranslateX(0));
                IFC(m_tpZoomedInTransform.Get()->put_TranslateY(0));
            }

            if (m_tpZoomedOutTransform)
            {
                IFC(m_tpZoomedOutTransform.Get()->put_TranslateX(0));
                IFC(m_tpZoomedOutTransform.Get()->put_TranslateY(0));
            }


            if (m_tpScrollViewer)
            {
                IFC(CalculateBounds(&bounds));

                IFC(m_tpScrollViewer.Cast<ScrollViewer>()->ScrollToHorizontalOffsetInternal(bounds.X * (!inZoomedInView ? _zoomMin : 1)));
                IFC(m_tpScrollViewer.Cast<ScrollViewer>()->ScrollToVerticalOffsetInternal(bounds.Y * (!inZoomedInView ? _zoomMin : 1)));
                IFC(m_tpScrollViewer.Cast<ScrollViewer>()->ZoomToFactorInternal((FLOAT) (inZoomedInView ? _zoomMax : _zoomMin), TRUE /*delayAndFlushViewChanged*/, nullptr /*pZoomChanged*/));
            }

            #ifdef SEZO_DBG
            {
                swprintf_s(g_szSEZODbg, g_szSEZOLen,
                    L"Completed DM State has removed corrections. Set offset to x: %f, y: %f",
                           bounds.X * (!inZoomedInView ? _zoomMin : 1), bounds.Y * (!inZoomedInView ? _zoomMin : 1));
                Trace(g_szSEZODbg);
            }
            #endif // SEZO_DBG

            break;
    }

Cleanup:
    m_isProcessingPointerInput = FALSE;
    m_phaseChangeLockDuringViewSwitch = FALSE;
    RRETURN(hr);
}

_Check_return_ HRESULT SemanticZoom::AutomationSemanticZoomOnToggle()
{
    HRESULT hr = S_OK;
    BOOLEAN canChangeViews = FALSE;

    IFC(get_CanChangeViews(&canChangeViews));

    if (canChangeViews)
    {
        // Jump to focused item through UIAutomation
        IFC(this->ToggleActiveView());
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT SemanticZoom::AutomationGetActivePresenter(
    _Outptr_opt_ xaml::IFrameworkElement** ppActivePresenter)
{
    HRESULT hr = S_OK;
    BOOLEAN isZoomedIn = FALSE;

    IFC(get_IsZoomedInViewActive(&isZoomedIn));

    if (isZoomedIn && m_tpZoomedInPresenterPart)
    {
        IFC(m_tpZoomedInPresenterPart.CopyTo(ppActivePresenter));
    }
    else if (!isZoomedIn && m_tpZoomedOutPresenterPart)
    {
        IFC(m_tpZoomedOutPresenterPart.CopyTo(ppActivePresenter));
    }
    else
    {
        ppActivePresenter = nullptr;
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT SemanticZoom::AutomationReparentPresenters(
    _In_ xaml_automation_peers::ISemanticZoomAutomationPeer* sezoPeer)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_automation_peers::ISemanticZoomAutomationPeer> spSemanticZoomAutomationPeer(sezoPeer);

    if (m_tpZoomedInPresenterPart)
    {
        ctl::ComPtr<xaml::IUIElement> spPresenterAsUIElement;
        ctl::ComPtr<TrackerCollection<xaml_automation_peers::AutomationPeer*>> spAPChildren;
        UINT childCount = 0;

        IFC(m_tpZoomedInPresenterPart.As<xaml::IUIElement>(&spPresenterAsUIElement));

        IFC(ctl::make<TrackerCollection<xaml_automation_peers::AutomationPeer*>>(&spAPChildren));
        IFC(spSemanticZoomAutomationPeer.Cast<SemanticZoomAutomationPeer>()->GetAutomationPeerChildren(
        spPresenterAsUIElement.Get(), spAPChildren.Get()));

        IFC(spAPChildren->get_Size(&childCount));

        for (UINT childIdx = 0; childIdx < childCount; childIdx++)
        {
            ctl::ComPtr<xaml_automation_peers::IAutomationPeer> spPresenterSubpeer;
            IFC(spAPChildren->GetAt(childIdx, &spPresenterSubpeer));

            IFC(CoreImports::SetAutomationPeerParent(
            static_cast<CAutomationPeer*>(spPresenterSubpeer.Cast<AutomationPeer>()->GetHandle()),
            static_cast<CAutomationPeer*>(spSemanticZoomAutomationPeer.Cast<SemanticZoomAutomationPeer>()->GetHandle())));
        }
    }

    if (m_tpZoomedOutPresenterPart)
    {
        ctl::ComPtr<xaml::IUIElement> spPresenterAsUIElement;
        ctl::ComPtr<TrackerCollection<xaml_automation_peers::AutomationPeer*>> spAPChildren;
        UINT childCount = 0;

        IFC(m_tpZoomedOutPresenterPart.As<xaml::IUIElement>(&spPresenterAsUIElement));

        IFC(ctl::make<TrackerCollection<xaml_automation_peers::AutomationPeer*>>(&spAPChildren));
        IFC(spSemanticZoomAutomationPeer.Cast<SemanticZoomAutomationPeer>()->GetAutomationPeerChildren(
        spPresenterAsUIElement.Get(), spAPChildren.Get()));

        IFC(spAPChildren->get_Size(&childCount));

        for (UINT childIdx = 0; childIdx < childCount; childIdx++)
        {
            ctl::ComPtr<xaml_automation_peers::IAutomationPeer> spPresenterSubpeer;
            IFC(spAPChildren->GetAt(childIdx, &spPresenterSubpeer));

            IFC(CoreImports::SetAutomationPeerParent(
            static_cast<CAutomationPeer*>(spPresenterSubpeer.Cast<AutomationPeer>()->GetHandle()),
            static_cast<CAutomationPeer*>(spSemanticZoomAutomationPeer.Cast<SemanticZoomAutomationPeer>()->GetHandle())));
        }
    }

    Cleanup:
    RRETURN(hr);
}

// Create SemanticZoomAutomationPeer to represent the SemanticZoom.
IFACEMETHODIMP SemanticZoom::OnCreateAutomationPeer(_Outptr_ xaml_automation_peers::IAutomationPeer** ppAutomationPeer)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_automation_peers::ISemanticZoomAutomationPeer> spSemanticZoomAutomationPeer;
    ctl::ComPtr<xaml_automation_peers::ISemanticZoomAutomationPeerFactory> spSemanticZoomAPFactory;
    ctl::ComPtr<IActivationFactory> spActivationFactory;
    ctl::ComPtr<IInspectable> spInner;

    spActivationFactory.Attach(ctl::ActivationFactoryCreator<DirectUI::SemanticZoomAutomationPeerFactory>::CreateActivationFactory());
    IFC(spActivationFactory.As(&spSemanticZoomAPFactory));

    IFC(spSemanticZoomAPFactory.Cast<SemanticZoomAutomationPeerFactory>()->CreateInstanceWithOwner(this,
        NULL,
        &spInner,
        &spSemanticZoomAutomationPeer));

    IFC(spSemanticZoomAutomationPeer.CopyTo(ppAutomationPeer));
    m_hasAutomationPeer = TRUE;

    IFC(AutomationReparentPresenters(spSemanticZoomAutomationPeer.Get()));

Cleanup:
    RRETURN(hr);
}

// Handles when a key is pressed down on the SemanticZoom.
IFACEMETHODIMP SemanticZoom::OnKeyDown(
    _In_ IKeyRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN isHandled = FALSE;

    m_isProcessingKeyboardInput = TRUE;

    IFCPTR(pArgs);
    IFC(pArgs->get_Handled(&isHandled));
    if (!isHandled)
    {
        BOOLEAN canChangeViews = TRUE;
        IFC(get_CanChangeViews(&canChangeViews));
        if (canChangeViews)
        {
            ZoomDirection messageZoomDirection = ZoomDirection_None;
            wsy::VirtualKeyModifiers modifiers = wsy::VirtualKeyModifiers_None;
            wsy::VirtualKey key = wsy::VirtualKey_None;
            BOOLEAN isZoomedInViewActive = FALSE;

            // Get the current view so we can determine whether the desired key
            // press can navigate to the desired view
            IFC(get_IsZoomedInViewActive(&isZoomedInViewActive));
            IFC(GetKeyboardModifiers(&modifiers));
            IFC(pArgs->get_Key(&key));
            messageZoomDirection = ScrollViewer::GetKeyboardMessageZoomAction(modifiers, key);

            if (messageZoomDirection == ZoomDirection_Out)
            {
                // We can only change to the ZoomedOutView if we're already
                // in the ZoomedInView
                if (isZoomedInViewActive)
                {
                    isHandled = TRUE;
                    IFC(put_IsZoomedInViewActive(FALSE));
                }
            }
            else if (messageZoomDirection == ZoomDirection_In)
            {
                // We can only change to the ZoomedInView if we're
                // already in the ZoomedOutView
                if (!isZoomedInViewActive)
                {
                    isHandled = TRUE;
                    IFC(put_IsZoomedInViewActive(TRUE));
                }
            }

            // Update when we've handled the event
            if (isHandled)
            {
                IFC(pArgs->put_Handled(isHandled));
            }
        }
    }

Cleanup:
    m_isProcessingKeyboardInput = FALSE;
    RRETURN(hr);
}

// Handles when the mouse wheel spins to change active views.
IFACEMETHODIMP SemanticZoom::OnPointerWheelChanged(
    _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN isHandled = FALSE;

    IFCPTR(pArgs);

    m_isProcessingPointerInput = TRUE;

    IFC(SemanticZoomGenerated::OnPointerWheelChanged(pArgs));

    IFC(pArgs->get_Handled(&isHandled));
    if (!isHandled)
    {
        // allow zooming using mouse in Win8 desktop apps. Phone does not have mouse in phone blue.
        bool shouldAllowMouseZoom = true;

        // in threshold mouse zoom is disabled by default. unless you enable zoom mode on the
        // ScrollViewer
        if (m_tpScrollViewer)
        {
            xaml_controls::ZoomMode zoomMode = xaml_controls::ZoomMode_Disabled;

            IFC(m_tpScrollViewer->get_ZoomMode(&zoomMode));
            shouldAllowMouseZoom = zoomMode != xaml_controls::ZoomMode_Disabled;
        }
        else
        {
            shouldAllowMouseZoom = false;
        }

        if (shouldAllowMouseZoom)
        {
            BOOLEAN canChangeViews = TRUE;
            IFC(get_CanChangeViews(&canChangeViews));
            if (canChangeViews)
            {
                // Only use Ctrl+Mousewheel to change the zoom
                wsy::VirtualKeyModifiers modifiers = wsy::VirtualKeyModifiers_None;
                IFC(GetKeyboardModifiers(&modifiers));
                if (IsFlagSet(modifiers, wsy::VirtualKeyModifiers_Control))
                {
                    INT delta = 0;
                    BOOLEAN isZoomedInViewActive = FALSE;
                    ctl::ComPtr<ixp::IPointerPointProperties> spPointerProperties = NULL;
                    ctl::ComPtr<ixp::IPointerPoint> spPointerPoint = NULL;

                    // Get the amount scrolled
                    IFC(pArgs->GetCurrentPoint(this, &spPointerPoint));
                    IFC(spPointerPoint->get_Properties(&spPointerProperties));
                    IFCPTR(spPointerProperties);
                    IFC(spPointerProperties->get_MouseWheelDelta(&delta));

                    // get the point that we scrolled on
                    IFC(spPointerPoint->get_Position(&m_zoomPoint));

                    // Get the current view so we can determine whether the desired zoom gesture
                    // can navigate to the desired view
                    IFC(get_IsZoomedInViewActive(&isZoomedInViewActive));

                    // set to gesture emulation
                    m_emulatingGesture = TRUE;

                    // We can only change to the ZoomedInView if we're already in the
                    // ZoomedOutView
                    if (!isZoomedInViewActive && delta > 0)
                    {
                        isHandled = TRUE;
                        IFC(put_IsZoomedInViewActive(TRUE));
                    }
                    // We can only change to the ZoomedOutView if we're already in the
                    // ZoomedInView
                    else if (isZoomedInViewActive && delta < 0)
                    {
                        isHandled = TRUE;
                        IFC(put_IsZoomedInViewActive(FALSE));
                    }

                    // Update when we've handled the event
                    if (isHandled)
                    {
                        IFC(pArgs->put_Handled(isHandled));
                    }
                }
            }
        }
    }


    Cleanup:
    m_emulatingGesture = FALSE;
    m_isProcessingPointerInput = FALSE;
    RRETURN(hr);
}

// clamps a value to be within a min and max value
FLOAT Clamp(_In_ DOUBLE value, _In_ DOUBLE min, _In_ DOUBLE max)
{
    if (value >= min && value <= max)
    {
        return(FLOAT)value;
    }
    if (value < min)
    {
        return(FLOAT)min;
    }
    else
    {
        return(FLOAT)max;
    }
}

// matches DUI implementation that calculates the threshold points where we switch
_Check_return_ HRESULT SemanticZoom::UpdateThresholds()
{
    HRESULT hr = S_OK;

    FLOAT zoomRange = _zoomMax - _zoomMin;
    if (zoomRange > 0)
    {
        FLOAT upperDelta = Clamp((zoomRange * _upperThresholdDelta), c_thresholdDeltaMin, c_thresholdDeltaMax);
        FLOAT lowerDelta = Clamp((zoomRange * _lowerThresholdDelta), c_thresholdDeltaMin, c_thresholdDeltaMax);
        FLOAT thresholdBufferDelta = Clamp(_thresholdBufferDelta, c_thresholdBufferDeltaMin, c_thresholdBufferDeltaMax);

        _upperThresholdLow = (FLOAT) (_zoomMax - upperDelta - thresholdBufferDelta);
        _upperThresholdHigh = (FLOAT) (_zoomMax - upperDelta);
        _lowerThresholdLow = (FLOAT) (_zoomMin + lowerDelta);
        _lowerThresholdHigh = (FLOAT) (_zoomMin + lowerDelta + thresholdBufferDelta);

        ASSERT((_upperThresholdLow > _zoomMin) && (_lowerThresholdHigh < _zoomMax));
    }
    else
    {
        _upperThresholdHigh = _upperThresholdLow = _lowerThresholdHigh = _lowerThresholdLow = _zoomMax;
    }

    RRETURN(hr);
}

// Raise the ViewChangeStarted event.
_Check_return_ HRESULT SemanticZoom::OnViewChangeStarted(
    _In_ ISemanticZoomViewChangedEventArgs* e)
{
    HRESULT hr = S_OK;
    ViewChangeStartedEventSourceType* pEventSource = nullptr;

    IFCPTR(e);

    // Raise the event
    IFC(GetViewChangeStartedEventSourceNoRef(&pEventSource));
    IFC(pEventSource->Raise(ctl::as_iinspectable(this), e));

Cleanup:
    RRETURN(hr);
}

// Raise the ViewChangeCompleted event.
_Check_return_ HRESULT SemanticZoom::OnViewChangeCompleted(
    _In_ ISemanticZoomViewChangedEventArgs* e)
{
    HRESULT hr = S_OK;
    ViewChangeCompletedEventSourceType* pEventSource = nullptr;

    IFCPTR(e);

    // Raise the event
    IFC(GetViewChangeCompletedEventSourceNoRef(&pEventSource));
    IFC(pEventSource->Raise(ctl::as_iinspectable(this), e));

Cleanup:
    m_isProcessingViewChange = FALSE;
    RRETURN(hr);
}

// This method does nothing on Windows no-op (it's under an APISet
// SemanticZoom_JumpList) to toggle the active view if we're in jump
// list behavior.
_Check_return_ HRESULT
    SemanticZoom::ToggleActiveViewFromHeaderItem(
    _Out_ BOOLEAN* bToggled)
{
    HRESULT hr = S_OK;

    IFCPTR(bToggled);
    *bToggled = FALSE;

    IFC(ToggleActiveView());
    *bToggled = TRUE;

Cleanup:
    RRETURN(hr);
}

// Change from one view to another.
_Check_return_ HRESULT SemanticZoom::ChangeViews() noexcept
{
    // calls to this method are deferred until applytemplate has run
    // therefore spSourcePart and spDestinationPart can be guaranteed to have
    // been initialized.

    HRESULT hr = S_OK;
    BOOLEAN canChangeViews = TRUE;
    BOOLEAN changingToZoomedOutView = FALSE;
    ctl::ComPtr<ISemanticZoomInformation> spSourceView;
    ctl::ComPtr<ISemanticZoomInformation> spDestinationView;
    ctl::ComPtr<FrameworkElement> spSourcePart;
    ctl::ComPtr<FrameworkElement> spDestinationPart;
    ctl::ComPtr<ISemanticZoomLocation> spSourceItem;
    ctl::ComPtr<ISemanticZoomLocation> spDestinationItem;
    ctl::ComPtr<SemanticZoomViewChangedEventArgs> spArgs;

    wf::Rect sourceCoordinateSystem = { };
    wf::Rect destinationCoordinateSystem = { };    // we will compare this the end result after make visible has run and setup correction animations

    IFC(get_CanChangeViews(&canChangeViews));
    IFCEXPECT(canChangeViews);

    m_isProcessingViewChange = TRUE;

    if (DesignerInterop::GetDesignerMode(DesignerMode::V2Only))
    {
        // ChangeViews will show a nice transition when we switch. In this case we need to
        // preempt that by going to the new visualstate immediately as we do not want
        // any visual transitions in design mode.
        IFC(UpdateVisualState(FALSE));
    }

    // Determine if we're flipping from ZoomedInView to ZoomedOutView (note: This
    // method is only called via the property changed handler when
    // IsZoomedInViewActive is updated - so the value has already been updated
    // before we change the view and we need to flip it here)
    IFC(get_IsZoomedInViewActive(&changingToZoomedOutView));
    changingToZoomedOutView = !changingToZoomedOutView;

    // Get the source and destination views/parts
    if (changingToZoomedOutView)
    {
        IFC(get_ZoomedInView(&spSourceView));
        IFC(get_ZoomedOutView(&spDestinationView));
        spSourcePart = m_tpZoomedInPresenterPart.Cast<FrameworkElement>();
        spDestinationPart = m_tpZoomedOutPresenterPart.Cast<FrameworkElement>();

        if (m_tpZoomedOutTransform)
        {
            IFC(m_tpZoomedOutTransform.Get()->put_TranslateX(0));
            IFC(m_tpZoomedOutTransform.Get()->put_TranslateY(0));
        }
    }
    else
    {
        IFC(get_ZoomedOutView(&spSourceView));
        IFC(get_ZoomedInView(&spDestinationView));
        spSourcePart = m_tpZoomedOutPresenterPart.Cast<FrameworkElement>();
        spDestinationPart = m_tpZoomedInPresenterPart.Cast<FrameworkElement>();

        if (m_tpZoomedInTransform)
        {
            IFC(m_tpZoomedInTransform.Get()->put_TranslateX(0));
            IFC(m_tpZoomedInTransform.Get()->put_TranslateY(0));
        }
    }

    IFC(ToggleBackKeyListener(changingToZoomedOutView));

    // Initialize the views
    // this has already been taken care of by NotifyStateChange
    // in the case of a DM driven change
    if (m_changePhase == SemanticZoomPhase_API_SwitchingViews &&
        !m_calledInitializeViewChangeSinceManipulationStart)
    {
        if (spSourceView)
        {
            IFC(spSourceView->InitializeViewChange());
        }
        if (spDestinationView)
        {
            IFC(spDestinationView->InitializeViewChange());
        }
        m_calledInitializeViewChangeSinceManipulationStart = TRUE;
    }

    // Allow the SemanticZoomInformation views to setup the view change
    IFC(ctl::ComObject<SemanticZoomLocation>::CreateInstance(spSourceItem.ReleaseAndGetAddressOf()));
    IFC(ctl::ComObject<SemanticZoomLocation>::CreateInstance(spDestinationItem.ReleaseAndGetAddressOf()));

    // initialize to correct values
    if (m_tpScrollViewer && spSourceView && spDestinationView && !m_isPendingViewChange)
    {
        ctl::ComPtr<ISemanticZoomInformation> spZoomedInView = NULL;
        ctl::ComPtr<IUIElement> spZoomedInContent = NULL;

        ctl::ComPtr<ISemanticZoomInformation> spZoomedOutView = NULL;
        ctl::ComPtr<IUIElement> spZoomedOutContent = NULL;

        wf::Point zoomPointZoomedInView = {0, 0};
        wf::Point zoomPointZoomedOutView = {0, 0};

        // todo: move this outside of this if().
        IFC(get_ZoomedInView(&spZoomedInView));
        spZoomedInContent = spZoomedInView.AsOrNull<IUIElement>();
        IFC(get_ZoomedOutView(&spZoomedOutView));
        spZoomedOutContent = spZoomedOutView.AsOrNull<IUIElement>();

        // the zoompoint (if relevant) can come from two places:
        // 1. ctrl-mousewheel, in which case it is just a point relative to this element
        //    this point is in layoutpixels, relative to SeZo
        //    We will need to hand it off relative to the correct view in their coordinate system
        //
        // 2. DM (during pinch gesture) in which case it is a point relative to the manipulated element
        //    normalized to DM factor 1 (natural size/layout pixels)


        // track which method was used to zoom
        short zoomType = _zoomedClick;

        if (m_changePhase == SemanticZoomPhase_DM_SwitchingViews || m_emulatingGesture)
        {
            if (m_changePhase == SemanticZoomPhase_API_SwitchingViews)
            {
                // case 1: point is relative to sezo

                ctl::ComPtr<xaml_media::IGeneralTransform> spTransformFromSezoToZoomedInContent = NULL;
                ctl::ComPtr<xaml_media::IGeneralTransform> spTransformFromSezoToZoomedOutContent = NULL;

                // get the transforms between these two visuals
                IFC(TransformToVisual(static_cast<UIElement*>(spZoomedInContent.Get()), spTransformFromSezoToZoomedInContent.ReleaseAndGetAddressOf()));
                IFC(TransformToVisual(static_cast<UIElement*>(spZoomedOutContent.Get()), spTransformFromSezoToZoomedOutContent.ReleaseAndGetAddressOf()));

                IFC(spTransformFromSezoToZoomedInContent->TransformPoint(m_zoomPoint, &m_zoomPointForZoomedInView));
                IFC(spTransformFromSezoToZoomedOutContent->TransformPoint(m_zoomPoint, &m_zoomPointForZoomedOutView));

                // no corrections to take in
                zoomPointZoomedInView = m_zoomPointForZoomedInView;
                zoomPointZoomedOutView = m_zoomPointForZoomedOutView;

                zoomType = _zoomedWheel;
            }
            else
            {
                // case 2: point is relative to manipulated element
                DOUBLE width = 0;
                DOUBLE height = 0;
                DOUBLE offsetX = 0;
                DOUBLE offsetY = 0;

                DOUBLE correctionX = 0;
                DOUBLE correctionY = 0;

                ctl::ComPtr<FrameworkElement> spZoomedInFE = NULL;
                ctl::ComPtr<FrameworkElement> spZoomedOutFE = NULL;

                wf::Point zoomPoint = m_zoomPoint;
                ctl::ComPtr<IInspectable> spManipulatedElement = NULL;
                ctl::ComPtr<FrameworkElement> spManipulatedFE = NULL;


                IFC(m_tpScrollViewer.Cast<ScrollViewer>()->get_Content(spManipulatedElement.GetAddressOf()));
                spManipulatedFE = spManipulatedElement.AsOrNull<IFrameworkElement>().Cast<FrameworkElement>();

                spZoomedInFE = m_tpZoomedInPresenterPart.Cast<FrameworkElement>();
                spZoomedOutFE = m_tpZoomedOutPresenterPart.Cast<FrameworkElement>();

                if (m_tpManipulatedElementTransform)
                {
                    IFC(m_tpManipulatedElementTransform.Get()->get_TranslateX(&offsetX));
                    IFC(m_tpManipulatedElementTransform.Get()->get_TranslateY(&offsetY));
                }
                zoomPoint.X -= (FLOAT) offsetX;
                zoomPoint.Y -= (FLOAT) offsetY;

                // only calculate once (in case we're doing many changeviews)
                if (DoubleUtil::AreClose(m_zoomPointForZoomedInView.X, 0) && DoubleUtil::AreClose(m_zoomPointForZoomedInView.Y, 0) )
                {
                    if (spZoomedInFE)
                    {
                        IFC(spZoomedInFE->get_ActualWidth(&width));
                        IFC(spZoomedInFE->get_ActualHeight(&height));
                    }

                    m_zoomPointForZoomedInView = zoomPoint;

                    // deduct the offsets
                    m_zoomPointForZoomedInView.X -= (FLOAT) width*(FLOAT)_zoomMin;
                    m_zoomPointForZoomedInView.Y -= (FLOAT) height*(FLOAT)_zoomMin;

                }

                zoomPointZoomedInView = m_zoomPointForZoomedInView;

                // and each time correction
                if (m_tpZoomedInTransform)
                {
                        IFC(m_tpZoomedInTransform.Get()->get_TranslateX(&correctionX));
                        IFC(m_tpZoomedInTransform.Get()->get_TranslateY(&correctionY));
                        zoomPointZoomedInView.X -= (FLOAT) correctionX/(FLOAT)_zoomMin;
                        zoomPointZoomedInView.Y -= (FLOAT) correctionY/(FLOAT)_zoomMin;
                }


                // work on the zoomedoutviews point
                if (DoubleUtil::AreClose(m_zoomPointForZoomedOutView.X, 0) && DoubleUtil::AreClose(m_zoomPointForZoomedOutView.Y, 0))
                {
                    m_zoomPointForZoomedOutView = zoomPoint;

                    // there is a factor of 2 involved
                    m_zoomPointForZoomedOutView.X *= _zoomMin;
                    m_zoomPointForZoomedOutView.Y *= _zoomMin;
                }

                zoomPointZoomedOutView = m_zoomPointForZoomedOutView;

                // and its correction
                if (m_tpZoomedOutTransform)
                {
                    IFC(m_tpZoomedOutTransform.Get()->get_TranslateX(&correctionX));
                    IFC(m_tpZoomedOutTransform.Get()->get_TranslateY(&correctionY));

                    zoomPointZoomedOutView.X -= (FLOAT) correctionX;
                    zoomPointZoomedOutView.Y -= (FLOAT) correctionY;
                }

                zoomType = _zoomedPinch;
            }

            IFC(spSourceItem.Cast<SemanticZoomLocation>()->put_ZoomPoint(changingToZoomedOutView ? zoomPointZoomedInView : zoomPointZoomedOutView));
            IFC(spDestinationItem.Cast<SemanticZoomLocation>()->put_ZoomPoint(changingToZoomedOutView ? zoomPointZoomedOutView : zoomPointZoomedInView));
        }

        TraceLoggingWrite(g_hTraceProvider, "SeZoZoom", TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance), TraceLoggingKeyword(MICROSOFT_KEYWORD_TELEMETRY), TraceLoggingBoolean(changingToZoomedOutView, "IsZoomingOut"), TraceLoggingInt16(zoomType, "Type"));

        #ifdef SEZO_DBG
        swprintf_s(g_szSEZODbg, g_szSEZOLen,
            L"doing changeview with x %f, y %f. ZoomedInViewCoordinates x %f, y %f. ZoomedOutViewCoordinates x %f, y %f.",
                   m_zoomPoint.X, m_zoomPoint.Y, zoomPointZoomedInView.X, zoomPointZoomedInView.Y, zoomPointZoomedOutView.X, zoomPointZoomedOutView.Y);
        Trace(g_szSEZODbg);
        #endif //SEZO_DBG

    }

    // give the source view the chance to determine the item that was being pressed
    // possibly even suggest a destination item
    if (spSourceView)
    {
        IFC(spSourceView->StartViewChangeFrom(spSourceItem.Get(), spDestinationItem.Get()));
    }

    // take the output from StartViewChangeFrom and convert coordinate systems between source and destination
    // the difficulty here really is about how you wish to handoff: (A) in the case of a direct manipulation (pinch)
    // we wish to do the handoff immediately, at the zoomfactor that is currently being applied. A
    // simple transformToVisual will suffice.
    // (B) in the case of a programmatic switch (ctrl-mousewheel for instance), we wish to do a handoff
    // where the destination will _endup_ in the location that sourceitem returned.
    if (m_tpScrollViewer && !m_isPendingViewChange)
    {
        // case (A): we wish to have the destination element show up at the current location as
        // dictated by the sourceitem
        IFC(spSourceItem->get_Bounds(&sourceCoordinateSystem));
        if (spSourcePart)
        {
            ctl::ComPtr<xaml_media::IGeneralTransform> spTransformFromSourceToDestination = NULL;
            IFC(spSourcePart->TransformToVisual(static_cast<UIElement*>(spDestinationPart.Get()), spTransformFromSourceToDestination.GetAddressOf()));
            IFC(spTransformFromSourceToDestination->TransformBounds(sourceCoordinateSystem, &destinationCoordinateSystem));
        }
        else
        {
            destinationCoordinateSystem = sourceCoordinateSystem;
        }


        if (m_changePhase == SemanticZoomPhase_API_SwitchingViews)
        {
            // case (B): we wish to have the destination element show up at a location such that when we reach
            // the final zoomfactor we will be at the point that the source item is in right now
            FLOAT zoomfactor = 0;
            ctl::ComPtr<IInspectable> spManipulatedElement = NULL;
            ctl::ComPtr<FrameworkElement> spManipulatedFE = NULL;
            DOUBLE width = 0;
            DOUBLE height = 0;
            wf::Point distanceToCenter = { };

            IFC(m_tpScrollViewer.Cast<ScrollViewer>()->get_ZoomFactor(&zoomfactor));

            IFC(m_tpScrollViewer.Cast<ScrollViewer>()->get_Content(spManipulatedElement.GetAddressOf()));
            spManipulatedFE = spManipulatedElement.AsOrNull<IFrameworkElement>().Cast<FrameworkElement>();

            IFC(spManipulatedFE->get_ActualWidth(&width));
            IFC(spManipulatedFE->get_ActualHeight(&height));


            // the manipulated element is currently centered and at some zoomfactor (0.5)
            // the distance we have is the distance to the left of that manipulated element

            // calculate the distance to the center
            distanceToCenter.X = (FLOAT) (destinationCoordinateSystem.X - width/2);
            distanceToCenter.Y = (FLOAT) (destinationCoordinateSystem.Y - height/2);

            // this is how it would be if the pixels would remain at this zoomfactor
            // however, they will change zoomfactor

            // get the distanceToCenter if we go to our destination zoomfactor
            distanceToCenter.X *= zoomfactor;
            distanceToCenter.X /= (FLOAT) (changingToZoomedOutView ? _zoomMin : _zoomMax);
            distanceToCenter.Y *= zoomfactor;
            distanceToCenter.Y /= (FLOAT) (changingToZoomedOutView ? _zoomMin : _zoomMax);

            destinationCoordinateSystem.X = (FLOAT) (distanceToCenter.X + width/2);
            destinationCoordinateSystem.Y = (FLOAT) (distanceToCenter.Y + height/2);
        }
        IFC(spDestinationItem->put_Bounds(destinationCoordinateSystem));
    }

    // give the destination view the chance to determine a destination item
    if (spDestinationView)
    {
        IFC(spDestinationView->StartViewChangeTo(spSourceItem.Get(), spDestinationItem.Get()));
    }

    // no need to transform this output, since it will already be in the destination coordinate system


    // Raise the ViewChangeStarted event
    IFC(ctl::make(&spArgs));
    IFC(spArgs->put_IsSourceZoomedInView(changingToZoomedOutView));
    IFC(spArgs->put_SourceItem(spSourceItem.Get()));
    IFC(spArgs->put_DestinationItem(spDestinationItem.Get()));
    IFC(OnViewChangeStarted(spArgs.Get()));

    // Make the destination the active view
    if (spSourceView)
    {
        IFC(spSourceView->put_IsActiveView(FALSE));
    }
    if (spDestinationView)
    {
        IFC(spDestinationView->put_IsActiveView(TRUE));
    }

    // Move the destination item into view (we're doing it as we start the
    // transition animation so any animation it does will take place as the
    // cross fade happens)
    IFC(spArgs->get_DestinationItem(spDestinationItem.ReleaseAndGetAddressOf()));

    if (spDestinationView)
    {
        wf::Rect remainder = {0, 0, 0, 0};

        // the main call that will attempt to overlap the two views.
        IFC(spDestinationView->MakeVisible(spDestinationItem.Get()));

        // set correction transforms only during a pinch gesture.
        if (m_changePhase == SemanticZoomPhase_DM_SwitchingViews)
        {
            ctl::ComPtr<SemanticZoomLocation> spConcreteLocation = NULL;

            spConcreteLocation = spDestinationItem.Cast<SemanticZoomLocation>();
            if (spConcreteLocation)
            {
                // great, we have gotten a destination container as close to where it needs to be as possible
                // however, it could be that it was not able to completely get the destination container to
                // where it needed to be.

                // the delta that we still have to scroll is now in the destination bounds. We are introducing
                // that distance is in destination views coordinate system
                IFC(spConcreteLocation->get_Remainder(&remainder));

                #ifdef SEZO_DBG
                swprintf_s(g_szSEZODbg, g_szSEZOLen,
                    L"SeZo got a remainder of %f, %f that we could not get into view.", remainder.X, remainder.Y);
                Trace(g_szSEZODbg);
                #endif // SEZO_DBG

                if (m_tpScrollViewer)
                {
                    xaml_controls::ScrollMode horizontalScrollMode = xaml_controls::ScrollMode_Disabled;
                    xaml_controls::ScrollMode verticalScrollMode = xaml_controls::ScrollMode_Disabled;

                    IFC(m_tpScrollViewer.Cast<ScrollViewer>()->get_HorizontalScrollMode(&horizontalScrollMode));
                    IFC(m_tpScrollViewer.Cast<ScrollViewer>()->get_VerticalScrollMode(&verticalScrollMode));

                    if (changingToZoomedOutView)
                    {
                        if (m_tpZoomedOutTransform)
                        {
                            // the zoomedoutview was our target
                            if (horizontalScrollMode == xaml_controls::ScrollMode_Enabled)
                            {
                                IFC(m_tpZoomedOutTransform->put_TranslateX(remainder.X));
                            }
                            if (verticalScrollMode == xaml_controls::ScrollMode_Enabled)
                            {
                                IFC(m_tpZoomedOutTransform->put_TranslateY(remainder.Y));
                            }
                        }
                    }
                    else if (m_tpZoomedInTransform)
                    {
                        // the zoomedinview was our target
                        if (horizontalScrollMode == xaml_controls::ScrollMode_Enabled)
                        {
                            IFC(m_tpZoomedInTransform.Get()->put_TranslateX(remainder.X * _zoomMin));
                        }
                        if (verticalScrollMode == xaml_controls::ScrollMode_Enabled)
                        {
                            IFC(m_tpZoomedInTransform.Get()->put_TranslateY(remainder.Y * _zoomMin));
                        }
                    }
                }
            }
        }
    }

    // if programmatic, we will animate ourselves
    if (m_tpScrollViewer &&
        !m_isPendingViewChange &&                                   // if during Applying of template, no animation is used
        m_changePhase == SemanticZoomPhase_API_SwitchingViews &&    // only when we are switching for because of an API call.
        m_tpZoomedInTransform &&                                    // needed for a correct call to BringIntoViewport
        m_tpZoomedOutTransform)
    {
        XRECTF bounds = {0, 0, 0, 0 };
        BOOLEAN handled = FALSE;

        IFC(CalculateBounds(&bounds));
        if (DesignerInterop::GetDesignerMode(DesignerMode::V2Only))
        {
           IFC(ResetViewsAndSnapToActiveView());
           #ifdef SEZO_DBG
           swprintf_s(g_szSEZODbg, g_szSEZOLen,
               L"SeZo is skipping the animation for view change as we are in design mode.");
           Trace(g_szSEZODbg);
           #endif // SEZO_DBG
        }
        else
        {
            IFC(m_tpScrollViewer.Cast<ScrollViewer>()->BringIntoViewport(
                bounds,
                FALSE /*skipDuringTouchContact*/,
                FALSE /*skipAnimationWhileRunning*/,
                TRUE  /*animate*/,
                &handled));
#ifdef SEZO_DBG
            swprintf_s(g_szSEZODbg, g_szSEZOLen,
                L"SeZo is doing a biv animation to finish a API call, bounds.X %f, bounds.Y %f, bounds.Width %f, bounds.Height %f.",
                       bounds.X, bounds.Y, bounds.Width, bounds.Height);
            Trace(g_szSEZODbg);
            #endif // SEZO_DBG
        }
    }

    // set hit-test visibility so that the front view (that might not be shown)
    // doesn't eat hits
    if (spSourcePart)
    {
        IFC(spSourcePart->put_IsHitTestVisible(FALSE));
        // This part is going out of view. We do not want it grabbing focus at that state.
        static_cast<CUIElement*>(spSourcePart->GetHandle())->SetSkipFocusSubtree(true);
    }
    if (spDestinationPart)
    {
        IFC(spDestinationPart->put_IsHitTestVisible(TRUE));
        // This part is coming into view, it can start grabbing the focus once more.
        static_cast<CUIElement*>(spDestinationPart->GetHandle())->SetSkipFocusSubtree(false);
    }


    // Go to the destination's visual state
    IFC(HideZoomOutButton(false /* bUseTransitions */));
    IFC(UpdateVisualState(true));

    SetPtrValue(m_tpCompletedArgs, spArgs.Get()); // store for later, used when the animation has ended

    // Raise the ViewChangeCompleted event
    IFC(m_tpCompletedArgs->get_SourceItem(spSourceItem.ReleaseAndGetAddressOf()));
    IFC(m_tpCompletedArgs->get_DestinationItem(spDestinationItem.ReleaseAndGetAddressOf()));

    if (spSourceView)
    {
        IFC(spSourceView->CompleteViewChangeFrom(spSourceItem.Get(), spDestinationItem.Get()));
    }
    if (spDestinationView)
    {
        IFC(spDestinationView->CompleteViewChangeTo(spSourceItem.Get(), spDestinationItem.Get()));
    }

    IFC(m_tpCompletedArgs->put_SourceItem(spSourceItem.Get()));
    IFC(m_tpCompletedArgs->put_DestinationItem(spDestinationItem.Get()));

    // normally we are deferred, but if the animation is not hooked we need to complete immediately
    if (m_isPendingViewChange || (changingToZoomedOutView && !m_isZoomedOutViewAnimationHooked) || (!changingToZoomedOutView && !m_isZoomedInViewAnimationHooked) )
    {
        IFC(OnViewChangeCompleted(m_tpCompletedArgs.Get()));

        // we will ultimately always use DM to bring a SemanticZoomView into view.
        // so the complete should occur at the end of that animation.
        // If there was a pending view change or if there is no ScrollViewer in the
        // default template (such as the Phone's), we will not have used DM
        if ((m_isPendingViewChange || !m_tpScrollViewer) && m_calledInitializeViewChangeSinceManipulationStart)
        {
            m_calledInitializeViewChangeSinceManipulationStart = FALSE;
            if (spSourceView)
            {
                IFC(spSourceView->CompleteViewChange());
            }
            if (spDestinationView)
            {
                IFC(spDestinationView->CompleteViewChange());
            }
        }
        m_tpCompletedArgs.Clear();
    }

Cleanup:
    RRETURN(hr);
}


_Check_return_
HRESULT
SemanticZoom::ViewChangeAnimationFinished(
    _In_opt_ IInspectable* pUnused1,
    _In_opt_ IInspectable* pUnused2)
{
    HRESULT hr = S_OK;
    BOOLEAN inZoomedInView = FALSE;
    ctl::ComPtr<ISemanticZoomInformation> spSourceView = NULL;
    ctl::ComPtr<ISemanticZoomInformation> spDestinationView = NULL;

    if (m_tpCompletedArgs)
    {

        // if contentview is active now, the source was the jumpview
        IFC(get_IsZoomedInViewActive(&inZoomedInView));

        // Get the source and destination views/parts
        if (inZoomedInView)
        {
            IFC(get_ZoomedInView(&spDestinationView));
            IFC(get_ZoomedOutView(&spSourceView));
        }
        else
        {
            IFC(get_ZoomedInView(&spSourceView));
            IFC(get_ZoomedOutView(&spDestinationView));
        }

        // this marks the event that we always raise after the
        // animation is done.
        IFC(OnViewChangeCompleted(m_tpCompletedArgs.Get()));

        // Cleanup the views only when DM session has already completed.
        // if DM has not yet completed, it means the session has not yet
        // ended, and we will call CompleteViewChange from DM.
        //
        // In it's turn, DM (NotifyStateChange) will forego calling CompleteViewChange
        // if it notices the animation has not finished yet (marked by m_tpCompletedArgs != null
        if (m_calledInitializeViewChangeSinceManipulationStart &&
            m_changePhase == SemanticZoomPhase_Idle)
        {
            m_calledInitializeViewChangeSinceManipulationStart = FALSE;
            if (spSourceView)
            {
                IFC(spSourceView->CompleteViewChange());
            }
            if (spDestinationView)
            {
                IFC(spDestinationView->CompleteViewChange());
            }
        }

       // clear out the args, this allows NotifyStateChange to complete ViewChanges if needed.
       m_tpCompletedArgs.Clear();
    }

Cleanup:
    RRETURN(hr);
}

// Calculate the position and bring the active view to view without
// animations by re-evaluating the bounds we should be.
_Check_return_ HRESULT SemanticZoom::ResetViewsAndSnapToActiveView()
{
    // this continues the setup that was started in OnApplyTemplate. We need to have the ScrollViewers
    // template be expanded and have valid widths on our views.
    HRESULT hr = S_OK;
    DOUBLE extentWidth = 0;
    DOUBLE extentHeight = 0;
    DOUBLE viewportWidth = 0;
    DOUBLE viewportHeight = 0;
    BOOLEAN roundOffsets = FALSE;
    XFLOAT horizontalOffset = 0;
    XFLOAT verticalOffset = 0;
    BOOLEAN inZoomedInView = FALSE;
    wf::Size layoutSize{};
    wf::Point targetPointClientDips{};
    wf::Rect availableMonitorRect{};

    if (!m_tpScrollViewer)
    {
        goto Cleanup;
    }

    if (m_hasAutomationPeer)
    {
        ctl::ComPtr<xaml_automation_peers::IAutomationPeer> spAutomationPeer;

        IFC(GetOrCreateAutomationPeer(&spAutomationPeer));
        if (spAutomationPeer)
        {
            ctl::ComPtr<xaml_automation_peers::ISemanticZoomAutomationPeer> spSemanticZoomAutomationPeer;

            IFC(spAutomationPeer.As<xaml_automation_peers::ISemanticZoomAutomationPeer>(&spSemanticZoomAutomationPeer));
            IFC(AutomationReparentPresenters(spSemanticZoomAutomationPeer.Get()));
        }
    }

    IFC(get_IsZoomedInViewActive(&inZoomedInView));

    if (!XamlOneCoreTransforms::IsEnabled())
    {
        // In some cases (XAML-on-win32) this code can get called during a size-changed event, before the island has a valid
        // rasterization scale.  If the rasterization scale is still unset, fall through and get the monitor size from DisplayInformation.
        // This could be wrong in the case that the island is on a different monitor than the CoreWindow.
        // http://osgvsowi/19285997 Semantic zoom inside an island may initialize itself incorrectly
        VisualTree* visualTree = VisualTree::GetForElementNoRef(GetHandle());
        if (visualTree && visualTree->GetRasterizationScale() != 0.0f)
        {
            IFC(DXamlCore::GetCurrent()->CalculateAvailableMonitorRect(this, targetPointClientDips, &availableMonitorRect));
        }
    }

    if (availableMonitorRect.Width == 0.0f)
    {
        // In OneCoreStrict/OneCoreTransforms mode the CalculateAvailableMonitorRect function isn't available.
        ctl::ComPtr<wgrd::IDisplayInformationStatics> displayInformationStatics;
        IFC(ctl::GetActivationFactory(wrl_wrappers::HStringReference(
            RuntimeClass_Windows_Graphics_Display_DisplayInformation).Get(),
            &displayInformationStatics));

        ctl::ComPtr<wgrd::IDisplayInformation> displayInformation;
        IFC(displayInformationStatics->GetForCurrentView(&displayInformation));

        ctl::ComPtr<wgrd::IDisplayInformation4> displayInformation4;
        IFC(displayInformation.As(&displayInformation4));

        unsigned int widthPixels;
        unsigned int heightPixels;
        IFC(displayInformation4->get_ScreenWidthInRawPixels(&widthPixels));
        IFC(displayInformation4->get_ScreenHeightInRawPixels(&heightPixels));

        const float zoomScale = RootScale::GetRasterizationScaleForElement(GetHandle());

        // This isn't as good as the non-OneCoreTransforms path yet (CalculateAvailableMonitorRect has more logic than this)
        // but is good enough for now for our purposes.
        // http://osgvsowi/12283211 -- Remove use of win32-based monitor functions, use display regions instead
        availableMonitorRect = {
            0.0f,
            0.0f,
            widthPixels / zoomScale,
            heightPixels / zoomScale };
    }


    // Picking 6 times the screen size because of x2 for the scaling and 3x to have enough manipulation room around the displayed middle.
    layoutSize.Width = availableMonitorRect.Width * 6;
    layoutSize.Height = availableMonitorRect.Height * 6;

    // this will stretch up the extent manually
    IFC(m_tpScrollViewer.Cast<ScrollViewer>()->SetLayoutSize(layoutSize));

    IFC(m_tpScrollViewer.Cast<ScrollViewer>()->ComputePixelViewportWidth(NULL, FALSE, &viewportWidth));
    IFC(m_tpScrollViewer.Cast<ScrollViewer>()->ComputePixelViewportHeight(NULL, FALSE, &viewportHeight));

    IFC(m_tpScrollViewer.Cast<ScrollViewer>()->ComputePixelExtentWidth(&extentWidth));
    IFC(m_tpScrollViewer.Cast<ScrollViewer>()->ComputePixelExtentHeight(&extentHeight));

    // the manipulated element is scaled up twice, so takes up two widths
    m_manipulatedElementOffset.X = (FLOAT) (extentWidth/2 - viewportWidth);
    m_manipulatedElementOffset.Y = (FLOAT) (extentHeight/2 - viewportHeight);

    // so we place it smack in the middle of our stretched up ScrollViewer
    if (m_tpManipulatedElementTransform)
    {
        IFC(m_tpManipulatedElementTransform.Get()->put_TranslateX(m_manipulatedElementOffset.X));
        IFC(m_tpManipulatedElementTransform.Get()->put_TranslateY(m_manipulatedElementOffset.Y));
    }

    // calculate the horizontal and vertical offsets to scroll the ScrollViewer to. Note that we might end up in a sub-pixel boundary and cause jiggling when you resize
    // the window. To avoid this, we layout round the offsets if layout rounding is enabled (which by default it is).
    horizontalOffset = static_cast<XFLOAT>((m_manipulatedElementOffset.X + ZOOMEDINVIEWCENTERINGCORRECTIONX(inZoomedInView, viewportWidth)) * (inZoomedInView ? _zoomMax : _zoomMin));
    verticalOffset = static_cast<XFLOAT>((m_manipulatedElementOffset.Y + ZOOMEDINVIEWCENTERINGCORRECTIONY(inZoomedInView, viewportHeight)) * (inZoomedInView ? _zoomMax : _zoomMin));
    IFC(get_UseLayoutRounding(&roundOffsets));
    if (roundOffsets)
    {
        IFC(LayoutRound(horizontalOffset, &horizontalOffset));
        IFC(LayoutRound(verticalOffset, &verticalOffset));
    }

    // start off scrolled to either the zoomed in or the zoomed out view
    IFC(m_tpScrollViewer.Cast<ScrollViewer>()->ScrollToHorizontalOffsetInternal(horizontalOffset));
    IFC(m_tpScrollViewer.Cast<ScrollViewer>()->ScrollToVerticalOffsetInternal(verticalOffset));
    IFC(m_tpScrollViewer.Cast<ScrollViewer>()->ZoomToFactorInternal(inZoomedInView ? (FLOAT) _zoomMax : (FLOAT) _zoomMin, TRUE /*delayAndFlushViewChanged*/, nullptr /*pZoomChanged*/));

    // remove individual corrections
    if (m_tpZoomedInTransform)
    {
        IFC(m_tpZoomedInTransform.Get()->put_TranslateX(0));
        IFC(m_tpZoomedInTransform.Get()->put_TranslateY(0));
    }
    if (m_tpZoomedOutTransform)
    {
        IFC(m_tpZoomedOutTransform.Get()->put_TranslateX(0));
        IFC(m_tpZoomedOutTransform.Get()->put_TranslateY(0));
    }

Cleanup:
    RRETURN(hr);
}

// handle the size changed event on the SeZo itself
// by re-evaluating the bounds we should be at.
_Check_return_ HRESULT SemanticZoom::OnSizeChanged(
    _In_ IInspectable* pSender,
    _In_ xaml::ISizeChangedEventArgs* pArgs)
{
    IFC_RETURN(ResetViewsAndSnapToActiveView());

    return S_OK;
}

// helper method that will calculate the bounds that we would like to be at
_Check_return_ HRESULT SemanticZoom::CalculateBounds(_Out_ XRECTF* pBounds)
{
    HRESULT hr = S_OK;

    DOUBLE viewportWidth = 0;
    DOUBLE viewportHeight = 0;
    XRECTF bounds = {0, 0, 0, 0 };
    DOUBLE correctionX = 0;
    DOUBLE correctionY = 0;
    BOOLEAN inZoomedInView = FALSE;

    *pBounds = bounds;

    IFC(get_IsZoomedInViewActive(&inZoomedInView));
    if (m_tpScrollViewer && m_tpZoomedInTransform && m_tpZoomedOutTransform)
    {
        IFC(m_tpScrollViewer.Cast<ScrollViewer>()->ComputePixelViewportWidth(NULL, FALSE, &viewportWidth));
        IFC(m_tpScrollViewer.Cast<ScrollViewer>()->ComputePixelViewportHeight(NULL, FALSE, &viewportHeight));

        if (inZoomedInView)
        {
              IFC(m_tpZoomedInTransform.Get()->get_TranslateX(&correctionX));
              IFC(m_tpZoomedInTransform.Get()->get_TranslateY(&correctionY));

              // this correction was applied on a surface that was scaled up
              // so will calculate appropriately
              correctionX /= _zoomMin;
              correctionY /= _zoomMin;

              // we are going to _zoomMax
              bounds.Width = (FLOAT)viewportWidth/(FLOAT)_zoomMax;
              bounds.Height = (FLOAT)viewportHeight/(FLOAT)_zoomMax;
        }
        else
        {
              IFC(m_tpZoomedOutTransform.Get()->get_TranslateX(&correctionX));
              IFC(m_tpZoomedOutTransform.Get()->get_TranslateY(&correctionY));

              // this correction was applied on a surface that was scaled up
              // so will calculate appropriately
              correctionX /= _zoomMin;
              correctionY /= _zoomMin;

              // we are going to _zoomMin
              bounds.Width = (FLOAT) (viewportWidth/_zoomMin);
              bounds.Height = (FLOAT) (viewportHeight/_zoomMin);
        }

        bounds.X = (FLOAT) (m_manipulatedElementOffset.X + ZOOMEDINVIEWCENTERINGCORRECTIONX(inZoomedInView, viewportWidth)) + (FLOAT)correctionX;
        bounds.Y = (FLOAT) (m_manipulatedElementOffset.Y + ZOOMEDINVIEWCENTERINGCORRECTIONY(inZoomedInView, viewportHeight)) + (FLOAT)correctionY;
    }

    *pBounds = bounds;

Cleanup:
    RRETURN(hr);
}

// Called whenever the ZoomOutButton is clicked.
_Check_return_ HRESULT
SemanticZoom::OnZoomOutButtonClick(
    _In_ IInspectable* pSender,
    _In_ xaml::IRoutedEventArgs* pArgs)
{
    RRETURN(ToggleActiveViewWithFocusState(xaml::FocusState_Pointer));
}

// Helper function to add an EventHandler to the Completed event of the given VisualState.
_Check_return_ HRESULT
SemanticZoom::AddStoryboardCompletedHandler(
    _In_ xaml::IVisualState* pState,
    _In_ HRESULT (SemanticZoom::*pHandler)(IInspectable*, IInspectable*))
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IStoryboard> spStoryboard = NULL;
    EventRegistrationToken ignoredToken;
    ctl::ComPtr<wf::IEventHandler<IInspectable*>> spStoryboardCompletedHandler = NULL;

    IFC(pState->get_Storyboard(spStoryboard.GetAddressOf()));
    if (spStoryboard)
    {
        spStoryboardCompletedHandler.Attach(
            new ClassMemberEventHandler<
                SemanticZoom,
                ISemanticZoom,
                wf::IEventHandler<IInspectable*>,
                IInspectable,
                IInspectable>(this, pHandler));

        IFC(spStoryboard.Cast<Storyboard>()->add_Completed(spStoryboardCompletedHandler.Get(), &ignoredToken));
    }

Cleanup:
    RRETURN(hr);
}

// When the ZoomOutButtonVisible state's Storyboard completes, kicks off the delay-hide transition.
_Check_return_ HRESULT
SemanticZoom::OnZoomOutButtonVisibleStoryboardCompleted(
    _In_ IInspectable*,
    _In_ IInspectable*)
{
    RRETURN(HideZoomOutButton(true /* bUseTransitions */));
}

// PointerMoved event handler.
IFACEMETHODIMP
SemanticZoom::OnPointerMoved(
    _In_ IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;

    IFCPTR(pArgs);

    IFC(SemanticZoomGenerated::OnPointerMoved(pArgs));

    if (!m_isProcessingViewChange)
    {
        BOOLEAN isZoomedInViewActive = FALSE;

        IFC(get_IsZoomedInViewActive(&isZoomedInViewActive));
        if (isZoomedInViewActive && m_isZoomOutButtonEnabled)
        {
            ctl::ComPtr<xaml_input::IPointer> spPointer = NULL;
            mui::PointerDeviceType pointerDeviceType = mui::PointerDeviceType_Mouse;

            IFC(pArgs->get_Pointer(spPointer.GetAddressOf()));
            IFCPTR(spPointer);
            IFC(spPointer->get_PointerDeviceType(&pointerDeviceType));

            // Mouse input dominates. If we are showing panning indicators and then mouse comes into play, mouse indicators win.
            if (mui::PointerDeviceType_Touch != pointerDeviceType)
            {
                // Even though we are taking action here, we choose not to handle the event and to let it keep routing.
                // We just use this event to detect that the ZoomOutButton needs to be re-shown (and its fade-out reset if
                // it is currently showing).
                // This is consistent with ScrollViewer::OnPointerMoved() displaying the scrolling indicators but marking the args as handled.
                IFC(ShowZoomOutButton());
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

// Shows the ZoomOutButton.
_Check_return_ HRESULT
SemanticZoom::ShowZoomOutButton()
{
    BOOLEAN bIgnored = FALSE;

    ASSERT(m_isZoomOutButtonEnabled);
    RRETURN(GoToState(true, L"ZoomOutButtonVisible", &bIgnored));
}

// Hides the ZoomOutButton.
_Check_return_ HRESULT
SemanticZoom::HideZoomOutButton(
    _In_ bool bUseTransitions)
{
    HRESULT hr = S_OK;
    BOOLEAN bIgnored = FALSE;

    IFC(GoToState(bUseTransitions, L"ZoomOutButtonHidden", &bIgnored));

    if (m_tpZoomOutButton)
    {
        IFC(m_tpZoomOutButton.Cast<Button>()->put_IsPointerOver(FALSE));
    }

Cleanup:
    RRETURN(hr);
}

// Called when the element enters the tree.
_Check_return_ HRESULT
SemanticZoom::EnterImpl(
    _In_ bool isLive,
    _In_ bool skipNameRegistration,
    _In_ bool coercedIsEnabled,
    _In_ bool useLayoutRounding)
{
    IFC_RETURN(SemanticZoomGenerated::EnterImpl(isLive, skipNameRegistration, coercedIsEnabled, useLayoutRounding));

    // During CUIElement::LeaveImpl we clear the skip focus subtree.
    // The code below ensures that upon entering back into scope the
    // SemanticZoom will SetSkipFocusSubtree on the correct portions.
    if (isLive)
    {
        BOOLEAN zoomedInViewActive = FALSE;
        IFC_RETURN(get_IsZoomedInViewActive(&zoomedInViewActive));

        if (m_tpZoomedInPresenterPart)
        {
            static_cast<CUIElement*>(m_tpZoomedInPresenterPart.Cast<FrameworkElement>()->GetHandle())->SetSkipFocusSubtree(!zoomedInViewActive);
        }

        if (m_tpZoomedOutPresenterPart)
        {
            static_cast<CUIElement*>(m_tpZoomedOutPresenterPart.Cast<FrameworkElement>()->GetHandle())->SetSkipFocusSubtree(zoomedInViewActive);
        }
    }

    return S_OK;
}
