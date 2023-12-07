// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <RuntimeProfiler.h>

#include "Pivot_Partial.h"
#include "PivotPanel_Partial.h"
#include "PivotHeaderPanel_Partial.h"
#include "PivotHeaderItem.h"
#include "PivotAutomationPeer_Partial.h"
#include "PivotItem_Partial.h"
#include "PivotItemEventArgs.h"
#include "DoubleUtil.h"
#include <windows.ui.viewmanagement.h>

#include "XboxUtility.h"
#include <DesignMode.h>
#include <XamlOneCoreTransforms.h>

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Controls
{

//#define PVTRACE(...) TRACE(TraceAlways, __VA_ARGS__)
#define PVTRACE(...)

const WCHAR Pivot::c_PivotItemsPresenterName[] = L"PivotItemPresenter";
const WCHAR Pivot::c_TitleControlName[] = L"TitleContentControl";
const WCHAR Pivot::c_HeadersControlName[] = L"Header";
const WCHAR Pivot::c_StaticHeadersControlName[] = L"StaticHeader";
const WCHAR Pivot::c_ScrollViewerName[] = L"ScrollViewer";
const WCHAR Pivot::c_PanelName[] = L"Panel";
const WCHAR Pivot::c_PivotItemsPresenterTranslateTransformName[] = L"ItemsPresenterTranslateTransform";
const WCHAR Pivot::c_PivotItemsPresenterCompositeTransformName[] = L"ItemsPresenterCompositeTransform";
const WCHAR Pivot::c_HeaderTranslateTransformName[] = L"HeaderTranslateTransform";
const WCHAR Pivot::c_StaticHeaderTranslateTransformName[] = L"StaticHeaderTranslateTransform";
const WCHAR Pivot::c_HeaderOffsetTranslateTransformName[] = L"HeaderOffsetTranslateTransform";
const WCHAR Pivot::c_VisualStateLandscape[] = L"Landscape";
const WCHAR Pivot::c_VisualStatePortrait[] = L"Portrait";
const WCHAR Pivot::c_NextButtonName[] = L"NextButton";
const WCHAR Pivot::c_PreviousButtonName[] = L"PreviousButton";
const WCHAR Pivot::c_LayoutElementName[] = L"PivotLayoutElement";
const WCHAR Pivot::c_LayoutElementTranslateTransformName[] = L"PivotLayoutElementTranslateTransform";
const WCHAR Pivot::c_HeaderClipperName[] = L"HeaderClipper";
const WCHAR Pivot::c_HeaderClipperGeometryName[] = L"HeaderClipperGeometry";
const WCHAR Pivot::c_LeftHeaderPresenterName[] = L"LeftHeaderPresenter";
const WCHAR Pivot::c_RightHeaderPresenterName[] = L"RightHeaderPresenter";
const WCHAR Pivot::c_FocusFollowerName[] = L"FocusFollower";


wrl::ComPtr<xaml_media::IVisualTreeHelperStatics> Pivot::s_spVisualTreeHelperStatics;
wrl::ComPtr<xaml_media::ICompositionTargetStatics> Pivot::s_spCompositionTargetStatics;
wrl::ComPtr<xaml_animation::IStoryboardStatics> Pivot::s_spStoryboardStatics;
wrl::ComPtr<xaml_animation::IThemeAnimationBaseFactory> PivotSlideInThemeAnimation::s_spThemeAnimationBaseFactory;

Pivot::Pivot()
    : m_changeViewPreRenderToken()
    , m_slideInPreRenderToken()
    , m_stateMachine(this)
    , m_animator(this, Private::ReferenceTrackerHelper<Pivot>(this))
    , m_headerManager(this, Private::ReferenceTrackerHelper<Pivot>(this))
    , m_pThisAsControlNoRef(nullptr)
    , m_viewChangingToken()
    , m_viewChangedToken()
    , m_directManipulationStartedToken()
    , m_directManipulationCompletedToken()
    , m_scrollViewerLoadedToken()
    , m_headerKeyDownToken()
    , m_layoutElementKeyDownToken()
    , m_orientationChangedToken()
    , m_fIndexChangeReentryGuard(FALSE)
    , m_fItemChangeReentryGuard(FALSE)
    , m_automationIsLocked(FALSE)
    , m_automationItemCount(0)
    , m_automationSelectedIndex(-1)
    , m_pointerEnteredHeaderToken()
    , m_pointerExitedHeaderToken()
    , m_pointerEnteredNextButtonToken()
    , m_pointerExitedNextButtonToken()
    , m_pointerEnteredPreviousButtonToken()
    , m_pointerExitedPreviousButtonToken()
    , m_previousButtonClickedToken()
    , m_nextButtonClickedToken()
    , m_usingStaticHeaders(false)
    , m_isControlKeyPressed(false)
    , m_isShiftKeyPressed(false)
    , m_isMouseOrPenPointerOverHeaders(false)
    , m_keyboardFocusToNextPivotItemPending(false)
    , m_isHeaderItemsCarouselEnabled(true)
    , m_cachedSelectedItemHeaderBoundingRectangle()
    , m_previousSelectedIndex(-1)
    , m_itemCount(0u)
    , m_navigationButtonsState(NavigationButtonsStates::NavigationButtonsHidden)
    , m_pivotDragDirection(PivotAnimationDirection_Center)
{
    __RP_Marker_ClassByName("Pivot");
}

Pivot::~Pivot()
{
    if (m_orientationChangedToken.value != 0 && Private::GetIsDllInitialized())
    {
        wrl::ComPtr<wgrd::IDisplayInformationStatics> spDisplayInformationStatics;
        wrl::ComPtr<wgrd::IDisplayInformation> spDisplayInformation;
        VERIFYHR(wf::GetActivationFactory(wrl_wrappers::HStringReference(
            RuntimeClass_Windows_Graphics_Display_DisplayInformation).Get(), &spDisplayInformationStatics));
        VERIFYHR(spDisplayInformationStatics->GetForCurrentView(&spDisplayInformation));
        VERIFYHR(spDisplayInformation->remove_OrientationChanged(m_orientationChangedToken));
        m_orientationChangedToken.value = 0;
    }

    VERIFYHR(UnregisterNavigationButtonEvents());
    VERIFYHR(m_headerManager.Dispose());
}

_Check_return_ HRESULT
Pivot::UnregisterNavigationButtonEvents()
{
    // This method is called by the Pivot destructor, we we need to use TryGetSafeReference throughout to avoid
    // crashes where the TrackerPtr target is a collected object in a zombie state.

    wrl::ComPtr<xaml_primitives::IPivotHeaderPanel> headerPanel;
    if (m_tpHeaderPanel.TryGetSafeReference(&headerPanel) && headerPanel)
    {
        wrl::ComPtr<xaml::IUIElement> headerPanelAsUIE;
        IFC_RETURN(headerPanel.As(&headerPanelAsUIE));
        if (m_pointerEnteredHeaderToken.value != 0)
        {
            IFC_RETURN(headerPanelAsUIE->remove_PointerEntered(m_pointerEnteredHeaderToken));
            m_pointerEnteredHeaderToken.value = 0;
        }
        if (m_pointerExitedHeaderToken.value != 0)
        {
            IFC_RETURN(headerPanelAsUIE->remove_PointerExited(m_pointerExitedHeaderToken));
            m_pointerExitedHeaderToken.value = 0;
        }
    }

    wrl::ComPtr<xaml_controls::IButton> nextButton;
    if (m_tpNextButton.TryGetSafeReference(&nextButton) && nextButton)
    {
        wrl::ComPtr<xaml::IUIElement> nextButtonAsUIE;
        IFC_RETURN(nextButton.As(&nextButtonAsUIE));
        if (m_pointerEnteredNextButtonToken.value != 0)
        {
            IFC_RETURN(nextButtonAsUIE->remove_PointerEntered(m_pointerEnteredNextButtonToken));
            m_pointerEnteredNextButtonToken.value = 0;
        }

        if (m_pointerExitedNextButtonToken.value != 0)
        {
            IFC_RETURN(nextButtonAsUIE->remove_PointerExited(m_pointerExitedNextButtonToken));
            m_pointerExitedNextButtonToken.value = 0;
        }

        if (m_nextButtonClickedToken.value != 0)
        {
            wrl::ComPtr<xaml_primitives::IButtonBase> nextButtonAsButtonBase;
            IFC_RETURN(nextButton.As(&nextButtonAsButtonBase));
            IFC_RETURN(nextButtonAsButtonBase->remove_Click(m_nextButtonClickedToken));

            m_nextButtonClickedToken.value = 0;
        }
    }


    wrl::ComPtr<xaml_controls::IButton> previousButton;
    if (m_tpPreviousButton.TryGetSafeReference(&previousButton) && previousButton)
    {
        wrl::ComPtr<xaml::IUIElement> previousButtonAsUIE;
        IFC_RETURN(previousButton.As(&previousButtonAsUIE));
        if (m_pointerEnteredPreviousButtonToken.value != 0)
        {
            IFC_RETURN(previousButtonAsUIE->remove_PointerEntered(m_pointerEnteredPreviousButtonToken));
            m_pointerEnteredPreviousButtonToken.value = 0;
        }

        if (m_pointerExitedPreviousButtonToken.value != 0)
        {
            IFC_RETURN(previousButtonAsUIE->remove_PointerExited(m_pointerExitedPreviousButtonToken));
            m_pointerExitedPreviousButtonToken.value = 0;
        }

        if (m_previousButtonClickedToken.value != 0)
        {
            wrl::ComPtr<xaml_primitives::IButtonBase> previousButtonAsButtonBase;
            IFC_RETURN(previousButton.As(&previousButtonAsButtonBase));
            IFC_RETURN(previousButtonAsButtonBase->remove_Click(m_previousButtonClickedToken));

            m_previousButtonClickedToken.value = 0;
        }
    }


    return S_OK;

}

_Check_return_ HRESULT
Pivot::UpdateHeaderState()
{
    const bool previousUsingStaticHeaders = m_usingStaticHeaders;
    IFC_RETURN(ShouldUseStaticHeaders(&m_usingStaticHeaders));

    // If we don't actually have the header panel that we need for the state we're in,
    // then we don't want to do anything that assumes the existence of that header panel.
    if ((m_usingStaticHeaders && m_tpStaticHeaderPanel) ||
        (!m_usingStaticHeaders && m_tpHeaderPanel))
    {
        if (m_pThisAsControlNoRef)
        {
            boolean returnValue = false;
            if (m_usingStaticHeaders)
            {
                IFC_RETURN(m_spVSMStatics->GoToState(m_pThisAsControlNoRef, wrl_wrappers::HStringReference(L"HeaderStatic").Get(), true, &returnValue));
            }
            else
            {
                IFC_RETURN(m_spVSMStatics->GoToState(m_pThisAsControlNoRef, wrl_wrappers::HStringReference(L"HeaderDynamic").Get(), true, &returnValue));
            }
        }

        IFC_RETURN(m_headerManager.HeaderStateChangedEvent(m_usingStaticHeaders));
        IFC_RETURN(m_stateMachine.HeaderStateChangedEvent(m_usingStaticHeaders));

        if (m_tpPanel && previousUsingStaticHeaders != m_usingStaticHeaders)
        {
            // We need to manually invalidate measure on the pivot panel because it's inside
            // a ScrollViewer and a simple change to the available size on the pivot will not
            // necessarily invalidate measure on it.
            IFC_RETURN(static_cast<xaml_primitives::PivotPanel*>(m_tpPanel.Get())->InvalidateMeasure());
        }
    }

    if (m_tpHeaderClipper && m_tpHeaderClipperGeometry)
    {
        double actualWidth, actualHeight;
        IFC_RETURN(m_tpHeaderClipper->get_ActualWidth(&actualWidth));
        IFC_RETURN(m_tpHeaderClipper->get_ActualHeight(&actualHeight));
        IFC_RETURN(m_tpHeaderClipperGeometry->put_Rect(wf::Rect{ 0.0f, 0.0f, static_cast<float>(actualWidth), static_cast<float>(actualHeight) }));
    }

    IFC_RETURN(UpdateVisualStates());

    return S_OK;
}

_Check_return_ HRESULT
Pivot::UpdateVisualStates()
{
    if (!m_pThisAsControlNoRef) return S_OK;
    BOOLEAN goToStateReturnValue; // Ignored

    // Navigation buttons' states
    {
        if (m_isMouseOrPenPointerOverHeaders)
        {
            BOOLEAN isLocked;
            IFC_RETURN(get_IsLocked(&isLocked));

            if (isLocked)
            {
                m_navigationButtonsState = NavigationButtonsStates::NavigationButtonsHidden;
            }
            else
            {
                bool showPreviousButton = false;
                bool showNextButton = false;

                if (m_usingStaticHeaders)
                {
                    ASSERT(m_tpStaticHeaderPanel);
                    if (static_cast<xaml_primitives::PivotHeaderPanel*>(m_tpStaticHeaderPanel.Get())->IsContentClipped() &&
                        m_itemCount > 1)
                    {
                        int selectedIndex;
                        IFC_RETURN(get_SelectedIndex(&selectedIndex));
                        if (selectedIndex >= 0)
                        {
                            // The content doesn't fit in the static headers so we should show the
                            // previous and/or the next navigation button(s) depending on the selected index and the items count.
                            showPreviousButton = (selectedIndex > 0);
                            showNextButton = (selectedIndex < static_cast<int>(m_itemCount) - 1);
                        }
                    }
                }
                else
                {
                    showPreviousButton = showNextButton = true;
                }

                m_navigationButtonsState =
                    showPreviousButton && showNextButton ? NavigationButtonsStates::NavigationButtonsVisible :
                    showPreviousButton ? NavigationButtonsStates::PreviousButtonVisible :
                    showNextButton ? NavigationButtonsStates::NextButtonVisible : NavigationButtonsStates::NavigationButtonsHidden;
            }
        }
        else
        {
            m_navigationButtonsState = NavigationButtonsStates::NavigationButtonsHidden;
        }

        const WCHAR* navigationButtonsState =
            m_navigationButtonsState == NavigationButtonsStates::NavigationButtonsVisible ? L"NavigationButtonsVisible" :
            m_navigationButtonsState == NavigationButtonsStates::PreviousButtonVisible ? L"PreviousButtonVisible" :
            m_navigationButtonsState == NavigationButtonsStates::NextButtonVisible ? L"NextButtonVisible" : L"NavigationButtonsHidden";
        IFC_RETURN(m_spVSMStatics->GoToState(m_pThisAsControlNoRef, wrl_wrappers::HStringReference(navigationButtonsState).Get(), true, &goToStateReturnValue));

        // Pivot gets retemplated a lot. A developer upgrading its app with a retemplated pivot from TH2 to RS1 will not
        // have the new PreviousButtonVisible/NextButtonVisible states. And, if we go to those state in that case, it will no-op
        // and both buttons will be hidden.
        // Indeed, NavigationButtonsHidden/NavigationButtonsVisible were added in TH while NextButtonVisible/PreviousButtonVisible were added in RS.
        // We will be friendly to developers in this situation by falling back to NavigationButtonsVisible.
        if (!goToStateReturnValue &&
            (m_navigationButtonsState == NavigationButtonsStates::PreviousButtonVisible ||
             m_navigationButtonsState == NavigationButtonsStates::NextButtonVisible))
        {
            IFC_RETURN(m_spVSMStatics->GoToState(m_pThisAsControlNoRef, wrl_wrappers::HStringReference(L"NavigationButtonsVisible").Get(), true, &goToStateReturnValue));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
Pivot::InitializeImpl(_In_opt_ IInspectable* pOuter)
{
    HRESULT hr = S_OK;
    wrl::ComPtr<xaml_controls::IItemsControlFactory> spInnerFactory;
    wrl::ComPtr<xaml_controls::IItemsControl> spDelegatingInnerInstance;
    wrl::ComPtr<IInspectable> spNonDelegatingInnerInspectable;
    IInspectable* aggregateOuter = pOuter ? pOuter : static_cast<IPivot*>(this);

    IFC(PivotGenerated::InitializeImpl(aggregateOuter));

    IFC(wf::GetActivationFactory(
            wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Controls_ItemsControl).Get(),
            &spInnerFactory));
    IFC(spInnerFactory->CreateInstance(
            aggregateOuter,
            &spNonDelegatingInnerInspectable,
            &spDelegatingInnerInstance));
    IFC(SetComposableBasePointers(
            spNonDelegatingInnerInspectable.Get(),
            spInnerFactory.Get()));

    // Any QI operation for the rest of this method will be delegated to the outer object and fail.
    // The outer object only sets its ComposableBasePointer after initializing the inner object and
    // cannot delegate calls to this inner object. Use spNonDelegatingInnerInspectable instead
    // to obtain the interfaces of base types.

    IFC(Private::SetDefaultStyleKey(
            spNonDelegatingInnerInspectable.Get(),
            L"Microsoft.UI.Xaml.Controls.Pivot"));

    IFC(m_stateMachine.Initialize(0 /*startingIndex */, 0 /* itemCount */, FALSE /* isLocked */));
    IFC(m_headerManager.Initialize());

    {
        EventRegistrationToken tempToken = {};

        wrl::ComPtr<xaml::IFrameworkElement> spThisAsFE;
        IFC(spNonDelegatingInnerInspectable.As(&spThisAsFE));

        IFC(spThisAsFE->add_Unloaded(
            wrl::Callback<xaml::IRoutedEventHandler>
            (this, &Pivot::OnUnloaded).Get(),
            &tempToken));

        wrl::ComPtr<xaml::IUIElement> spThisAsUIE;
        IFC(spNonDelegatingInnerInspectable.As(&spThisAsUIE));
    }

    {
        wrl::ComPtr<wgrd::IDisplayInformationStatics> spDisplayInformationStatics;
        wrl::ComPtr<wgrd::IDisplayInformation> spDisplayInformation;
        wrl::WeakRef wrThis;

        IFC(wf::GetActivationFactory(wrl_wrappers::HStringReference(
            RuntimeClass_Windows_Graphics_Display_DisplayInformation).Get(),
            &spDisplayInformationStatics
        ));
        IFCEXPECT(spDisplayInformationStatics);

        if (SUCCEEDED(spDisplayInformationStatics->GetForCurrentView(&spDisplayInformation)))
        {
            IFCEXPECT(spDisplayInformation);

            IFC(spDisplayInformation->add_OrientationChanged(
                wrl::Callback<wf::ITypedEventHandler<wgrd::DisplayInformation*, IInspectable*>>(
                    [wrThis](_In_ wgrd::IDisplayInformation* pSender, _In_ IInspectable* pArgs) mutable
                    {
                        HRESULT hr = S_OK;
                        wrl::ComPtr<xaml_controls::IPivot> spThis;

                        IGNOREHR(wrThis.As(&spThis));
                        if (spThis)
                        {
                            Pivot* pThis = static_cast<Pivot*>(spThis.Get());
                            IFC(pThis->OnDisplayOrientationChanged(pSender, pArgs));
                        }

                    Cleanup:
                        RRETURN(hr);
                    }).Get(),
                        &m_orientationChangedToken));
        }
    }

    {
        wrl::ComPtr<xaml::IRectHelperStatics> spRectHelperStatics;

        IFC(wf::GetActivationFactory(
            wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_RectHelper).Get(),
            &spRectHelperStatics));

        IFC(spRectHelperStatics->get_Empty(&m_cachedSelectedItemHeaderBoundingRectangle));
    }

    {
        wrl::ComPtr<xaml::IFrameworkElement> spThisAsFE;
        IFC(spNonDelegatingInnerInspectable.As(&spThisAsFE));

        IFC(spThisAsFE->add_SizeChanged(
            wrl::Callback<xaml::ISizeChangedEventHandler>
            (this, &Pivot::OnSizeChanged).Get(),
            &m_sizeChangedToken));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
Pivot::GetDefaultIsHeaderItemsCarouselEnabled(_Outptr_ IInspectable** value)
{
    return Private::ValueBoxer::CreateBoolean(true, value);
}

#pragma region IFrameworkElementOverrides
_Check_return_ HRESULT
Pivot::MeasureOverrideImpl(_In_ wf::Size availableSize, _Out_ wf::Size* returnValue)
{
    HRESULT hr = S_OK;

    PVTRACE(L"[Measure]: avaialbleSize: %f x %f", availableSize.Width, availableSize.Height);

    if (availableSize.Width == std::numeric_limits<float>::infinity())
    {
        DOUBLE screenWidth = 0.0;
        IFC(GetScreenWidth(this, &screenWidth));
        PVTRACE(L"[Measure]: overriding available size with screen width of %f.", screenWidth);
        availableSize.Width = static_cast<float>(screenWidth);
    }

    IFC(m_stateMachine.MeasureEvent(availableSize));
    IFC(PivotGenerated::MeasureOverrideImpl(availableSize, returnValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
Pivot::ArrangeOverrideImpl(_In_ wf::Size finalSize, _Out_ wf::Size* returnValue)
{
    PVTRACE(L"[Arrange]: finalSize: %f x %f", finalSize.Width, finalSize.Height);
    if (finalSize.Width == std::numeric_limits<float>::infinity())
    {
        DOUBLE screenWidth = 0.0;
        IFC_RETURN(GetScreenWidth(this, &screenWidth));
        PVTRACE(L"[Arrange]: overriding final size with screen width of %f.", screenWidth);
        finalSize.Width = static_cast<float>(screenWidth);
    }

    IFC_RETURN(PivotGenerated::ArrangeOverrideImpl(finalSize, returnValue));

    // The ArrangeEvent is fired after the first arrange pass. This is the first point
    // in time that ScrollViewer knows its horizontal extent and will accept a ChangeView
    // command.
    IFC_RETURN(m_stateMachine.ArrangeEvent(finalSize));

    IFC_RETURN(UpdateHeaderState());

    return S_OK;
}

_Check_return_ HRESULT
Pivot::OnApplyTemplateImpl()
{
    HRESULT hr = S_OK;

    wrl::ComPtr<xaml_controls::IControlProtected> spThisAsControlProtected;

    wrl::ComPtr<xaml_controls::IItemsPresenter> spItemsPresenter;
    wrl::ComPtr<xaml_controls::IContentControl> titleControl;
    wrl::ComPtr<xaml_primitives::IPivotHeaderPanel> spHeaderPanel;
    wrl::ComPtr<xaml_primitives::IPivotHeaderPanel> spStaticHeaderPanel;
    wrl::ComPtr<xaml_controls::IScrollViewer> spScrollViewer;
    wrl::ComPtr<xaml_primitives::IPivotPanel> spPanel;
    wrl::ComPtr<xaml_media::ITranslateTransform> spItemsPanelTranslateTransform;
    wrl::ComPtr<xaml_media::ICompositeTransform> spItemsPanelCompositeTransform;
    wrl::ComPtr<xaml_media::ICompositeTransform> spHeaderTransform;
    wrl::ComPtr<xaml_media::ICompositeTransform> spStaticHeaderTransform;
    wrl::ComPtr<xaml_media::ICompositeTransform> spHeaderOffsetTransform;
    wrl::ComPtr<xaml_controls::IButton> spNextButton;
    wrl::ComPtr<xaml_controls::IButton> spPreviousButton;
    wrl::ComPtr<xaml_primitives::IButtonBase> spNextButtonAsButtonBase;
    wrl::ComPtr<xaml_primitives::IButtonBase> spPreviousButtonAsButtonBase;
    wrl::ComPtr<xaml::IUIElement> spLayoutElement;
    wrl::ComPtr<xaml_media::ICompositeTransform> spLayoutElementTransform;
    wrl::ComPtr<xaml::IFrameworkElement> spHeaderClipper;
    wrl::ComPtr<xaml_media::IRectangleGeometry> spHeaderClipperGeometry;
    wrl::ComPtr<xaml::IFrameworkElement> spLeftHeaderPresenter;
    wrl::ComPtr<xaml::IFrameworkElement> spRightHeaderPresenter;
    wrl::ComPtr<xaml::IFrameworkElement> spFocusFollower;

    IFC(QueryInterface(__uuidof(xaml_controls::IControlProtected), &spThisAsControlProtected));

    IFC(InitializeVisualStateInterfaces());

    if (m_tpScrollViewer)
    {
        wrl::ComPtr<xaml::IFrameworkElement> spScrollViewerAsFE;
        IFC(m_tpScrollViewer.As(&spScrollViewerAsFE));

        IFC(m_tpScrollViewer->remove_ViewChanged(m_viewChangedToken));
        IFC(m_tpScrollViewer->remove_ViewChanging(m_viewChangingToken));

        {
            IFC(m_tpScrollViewer->remove_DirectManipulationStarted(m_directManipulationStartedToken));
            IFC(m_tpScrollViewer->remove_DirectManipulationCompleted(m_directManipulationCompletedToken));
        }

        IFC(spScrollViewerAsFE->remove_Loaded(m_scrollViewerLoadedToken));

        {
            wrl::ComPtr<xaml_controls::IScrollViewerPrivate> spScrollViewerPrivate;

            IFC(m_tpScrollViewer.As(&spScrollViewerPrivate));
            IFCEXPECT(spScrollViewerPrivate);

            IFC(spScrollViewerPrivate->SetIsNearVerticalAlignmentForced(FALSE));
            IFC(spScrollViewerPrivate->put_ArePointerWheelEventsIgnored(FALSE));
            IFC(spScrollViewerPrivate->put_IsRequestBringIntoViewIgnored(FALSE));
        }
    }

    IFC(UnregisterNavigationButtonEvents());

    if (m_tpPanel)
    {
        IFC(static_cast<xaml_primitives::PivotPanel*>(m_tpPanel.Get())->SetParentPivot(nullptr));
    }

    if (m_tpLayoutElement)
    {
        IFC(m_tpLayoutElement->remove_KeyDown(m_layoutElementKeyDownToken));
    }

    if (m_tpHeaderClipper)
    {
        wrl::ComPtr<xaml::IUIElement> headerClipperAsUE;
        IFC(m_tpHeaderClipper.As(&headerClipperAsUE));
        IFC(headerClipperAsUE->remove_GotFocus(m_headerGotFocusToken));
        IFC(headerClipperAsUE->remove_LostFocus(m_headerLostFocusToken));
        IFC(headerClipperAsUE->remove_KeyDown(m_headerKeyDownToken));
    }

    if (m_tpFocusFollowerExpresssionAnimation)
    {
        if (m_tpFocusFollower)
        {
            wrl::ComPtr<xaml_hosting::IElementCompositionPreviewStatics> elementCompositionPreviewStatics;
            IFC(wf::GetActivationFactory(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Hosting_ElementCompositionPreview).Get(), &elementCompositionPreviewStatics));

            wrl::ComPtr<WUComp::IVisual> spFocusFollowerVisual;
            wrl::ComPtr<WUComp::ICompositionObject> spCompObj;

            wrl::ComPtr<xaml::IUIElement> spFocusFollowerAsUIElement;
            IFC(m_tpFocusFollower.As(&spFocusFollowerAsUIElement));
            IFC(elementCompositionPreviewStatics->GetElementVisual(spFocusFollowerAsUIElement.Get(), &spFocusFollowerVisual));
            IFC(spFocusFollowerVisual.As(&spCompObj));

            IFC(spCompObj.Get()->StopAnimation(wrl_wrappers::HStringReference(L"Translation.X").Get()));
        }

        m_tpFocusFollowerExpresssionAnimation.Clear();
    }

    IFC(PivotGenerated::OnApplyTemplateImpl());

    IFC(Private::AttachTemplatePart<xaml_controls::IItemsPresenter>(
        spThisAsControlProtected.Get(),
        c_PivotItemsPresenterName,
        &spItemsPresenter));
    IFC(Private::AttachTemplatePart<xaml_controls::IContentControl>(
        spThisAsControlProtected.Get(),
        c_TitleControlName,
        &titleControl));
    IFC(Private::AttachTemplatePart<xaml_primitives::IPivotHeaderPanel>(
        spThisAsControlProtected.Get(),
        c_HeadersControlName,
        &spHeaderPanel));
    IFC(Private::AttachTemplatePart<xaml_primitives::IPivotHeaderPanel>(
        spThisAsControlProtected.Get(),
        c_StaticHeadersControlName,
        &spStaticHeaderPanel));
    IFC(Private::AttachTemplatePart<xaml_controls::IScrollViewer>(
        spThisAsControlProtected.Get(),
        c_ScrollViewerName,
        &spScrollViewer));
    IFC(Private::AttachTemplatePart<xaml_primitives::IPivotPanel>(
        spThisAsControlProtected.Get(),
        c_PanelName,
        &spPanel));
    IFC(Private::AttachTemplatePart<xaml_media::ITranslateTransform>(
        spThisAsControlProtected.Get(),
        c_PivotItemsPresenterTranslateTransformName,
        &spItemsPanelTranslateTransform));
    IFC(Private::AttachTemplatePart<xaml_media::ICompositeTransform>(
        spThisAsControlProtected.Get(),
        c_PivotItemsPresenterCompositeTransformName,
        &spItemsPanelCompositeTransform));
    IFC(Private::AttachTemplatePart<xaml_media::ICompositeTransform>(
        spThisAsControlProtected.Get(),
        c_HeaderTranslateTransformName,
        &spHeaderTransform));
    IFC(Private::AttachTemplatePart<xaml_media::ICompositeTransform>(
        spThisAsControlProtected.Get(),
        c_StaticHeaderTranslateTransformName,
        &spStaticHeaderTransform));
    IFC(Private::AttachTemplatePart<xaml_media::ICompositeTransform>(
        spThisAsControlProtected.Get(),
        c_HeaderOffsetTranslateTransformName,
        &spHeaderOffsetTransform));
    IFC(Private::AttachTemplatePart<xaml_controls::IButton>(
        spThisAsControlProtected.Get(),
        c_NextButtonName,
        &spNextButton));
    IFC(Private::AttachTemplatePart<xaml_controls::IButton>(
        spThisAsControlProtected.Get(),
        c_PreviousButtonName,
        &spPreviousButton));
    IFC(Private::AttachTemplatePart<xaml::IUIElement>(
        spThisAsControlProtected.Get(),
        c_LayoutElementName,
        &spLayoutElement));
    IFC(Private::AttachTemplatePart<xaml_media::ICompositeTransform>(
        spThisAsControlProtected.Get(),
        c_LayoutElementTranslateTransformName,
        &spLayoutElementTransform));
    IFC(Private::AttachTemplatePart<xaml::IFrameworkElement>(
        spThisAsControlProtected.Get(),
        c_HeaderClipperName,
        &spHeaderClipper));
    IFC(Private::AttachTemplatePart<xaml_media::IRectangleGeometry>(
        spThisAsControlProtected.Get(),
        c_HeaderClipperGeometryName,
        &spHeaderClipperGeometry));
    IFC(Private::AttachTemplatePart<xaml::IFrameworkElement>(
        spThisAsControlProtected.Get(),
        c_LeftHeaderPresenterName,
        &spLeftHeaderPresenter));
    IFC(Private::AttachTemplatePart<xaml::IFrameworkElement>(
        spThisAsControlProtected.Get(),
        c_RightHeaderPresenterName,
        &spRightHeaderPresenter));
    IFC(Private::AttachTemplatePart<xaml::IFrameworkElement>(
        spThisAsControlProtected.Get(),
        c_FocusFollowerName,
        &spFocusFollower));

    IFC(SetPtrValue(m_tpItemsPresenter, spItemsPresenter.Get()));
    IFC(SetPtrValue(m_titleControl, titleControl.Get()));
    IFC(SetPtrValue(m_tpHeaderPanel, spHeaderPanel.Get()));
    IFC(SetPtrValue(m_tpStaticHeaderPanel, spStaticHeaderPanel.Get()));
    IFC(SetPtrValue(m_tpScrollViewer, spScrollViewer.Get()));
    IFC(SetPtrValue(m_tpPanel, spPanel.Get()));
    IFC(SetPtrValue(m_tpItemsPanelTranslateTransform, spItemsPanelTranslateTransform.Get()));
    IFC(SetPtrValue(m_tpItemsPanelCompositeTransform, spItemsPanelCompositeTransform.Get()));
    IFC(SetPtrValue(m_tpHeaderTransform, spHeaderTransform.Get()));
    IFC(SetPtrValue(m_tpStaticHeaderTransform, spStaticHeaderTransform.Get()));
    IFC(SetPtrValue(m_tpNextButton, spNextButton.Get()));
    IFC(SetPtrValue(m_tpPreviousButton, spPreviousButton.Get()));
    IFC(SetPtrValue(m_tpLayoutElement, spLayoutElement.Get()));
    IFC(SetPtrValue(m_tpLayoutElementTransform, spLayoutElementTransform.Get()));
    IFC(SetPtrValue(m_tpHeaderClipper, spHeaderClipper.Get()));
    IFC(SetPtrValue(m_tpHeaderClipperGeometry, spHeaderClipperGeometry.Get()));
    IFC(SetPtrValue(m_tpLeftHeaderPresenter, spLeftHeaderPresenter.Get()));
    IFC(SetPtrValue(m_tpRightHeaderPresenter, spRightHeaderPresenter.Get()));
    IFC(SetPtrValue(m_tpFocusFollower, spFocusFollower.Get()));

    if (spScrollViewer)
    {
        wrl::ComPtr<xaml_controls::IScrollViewerPrivate> spScrollViewerPrivate;
        wrl::ComPtr<xaml::IFrameworkElement> spScrollViewerAsFE;
        IFC(spScrollViewer.As(&spScrollViewerAsFE));

        IFC(spScrollViewer->add_ViewChanged(
            wrl::Callback<wf::IEventHandler<xaml_controls::ScrollViewerViewChangedEventArgs*>>
            (this, &Pivot::OnViewChanged).Get(),
            &m_viewChangedToken));

        IFC(spScrollViewer->add_ViewChanging(
            wrl::Callback<wf::IEventHandler<xaml_controls::ScrollViewerViewChangingEventArgs*>>
            (this, &Pivot::OnViewChanging).Get(),
            &m_viewChangingToken));

        {
            IFC(spScrollViewer->add_DirectManipulationStarted(
                wrl::Callback<wf::IEventHandler<IInspectable*>>
                (this, &Pivot::OnDirectManipulationStarted).Get(),
                &m_directManipulationStartedToken));

            IFC(spScrollViewer->add_DirectManipulationCompleted(
                wrl::Callback<wf::IEventHandler<IInspectable*>>
                (this, &Pivot::OnDirectManipulationCompleted).Get(),
                &m_directManipulationCompletedToken));
        }

        IFC(spScrollViewerAsFE->add_Loaded(
            wrl::Callback<xaml::IRoutedEventHandler>
            (this, &Pivot::OnScrollViewerLoaded).Get(),
            &m_scrollViewerLoadedToken));

        IGNOREHR(m_tpScrollViewer.As(&spScrollViewerPrivate));
        if (spScrollViewerPrivate)
        {
            // WPB: 273985. We use this override mode because of timing issues
            // in DManip/inputmanager/ScrollViewer that are out of scope for this
            // release.
            // TODO: Re-evaluate the need for this workaround when DManip-on-DComp is turned on.
            // See task 946804: Re-evaluate the need for workaround in Pivot ctrl when DManip-on-DComp is on
            IFC(spScrollViewerPrivate->SetIsNearVerticalAlignmentForced(TRUE));

            IFC(spScrollViewerPrivate->put_ArePointerWheelEventsIgnored(TRUE));
            IFC(spScrollViewerPrivate->put_IsRequestBringIntoViewIgnored(TRUE));
        }
    }

    if (m_tpLayoutElement)
    {
        IFC(m_tpLayoutElement->add_KeyDown(
            wrl::Callback<xaml_input::IKeyEventHandler>
            (this, &Pivot::OnLayoutElementKeyDown).Get(),
            &m_layoutElementKeyDownToken));
    }

    if (spHeaderClipper)
    {
        wrl::ComPtr<xaml::IUIElement> spHeaderClipperAsUIE;

        // We subscribe to the pointer events on the header clipper instead of the header panel because
        // we arrange the latter's children way outside of its final arrange bounds.
        IFC(spHeaderClipper.As(&spHeaderClipperAsUIE));
        IFC(spHeaderClipperAsUIE->add_PointerEntered(wrl::Callback<xaml_input::IPointerEventHandler>(this, &Pivot::OnPointerEnteredHeader).Get(), &m_pointerEnteredHeaderToken));
        IFC(spHeaderClipperAsUIE->add_PointerExited(wrl::Callback<xaml_input::IPointerEventHandler>(this, &Pivot::OnPointerExitedHeader).Get(), &m_pointerExitedHeaderToken));
        IFC(spHeaderClipperAsUIE->add_GotFocus(wrl::Callback<xaml::IRoutedEventHandler> (this, &Pivot::OnHeaderGotFocus).Get(), &m_headerGotFocusToken));
        IFC(spHeaderClipperAsUIE->add_LostFocus(wrl::Callback<xaml::IRoutedEventHandler>(this, &Pivot::OnHeaderLostFocus).Get(), &m_headerLostFocusToken));
        IFC(spHeaderClipperAsUIE->add_KeyDown(wrl::Callback<xaml_input::IKeyEventHandler> (this, &Pivot::OnHeaderKeyDown).Get(), &m_headerKeyDownToken));

        IFC(OnHeaderFocusVisualPlacementChanged());
    }

    if (spNextButton)
    {
        wrl::ComPtr<xaml::IUIElement> spNextButtonAsUIE;
        IFC(spNextButton.As(&spNextButtonAsUIE));
        IFC(spNextButtonAsUIE->add_PointerEntered(wrl::Callback<xaml_input::IPointerEventHandler>(this, &Pivot::OnPointerEnteredHeader).Get(), &m_pointerEnteredNextButtonToken));
        IFC(spNextButtonAsUIE->add_PointerExited(wrl::Callback<xaml_input::IPointerEventHandler>(this, &Pivot::OnPointerExitedHeader).Get(), &m_pointerExitedNextButtonToken));
        IFC(m_tpNextButton.As(&spNextButtonAsButtonBase));
        IFC(spNextButtonAsButtonBase->add_Click(wrl::Callback<xaml::IRoutedEventHandler>(this, &Pivot::OnNextButtonClick).Get(), &m_nextButtonClickedToken));
    }

    if (spPreviousButton)
    {
        wrl::ComPtr<xaml::IUIElement> spPreviousButtonAsUIE;
        IFC(spPreviousButton.As(&spPreviousButtonAsUIE));
        IFC(spPreviousButtonAsUIE->add_PointerEntered(wrl::Callback<xaml_input::IPointerEventHandler>(this, &Pivot::OnPointerEnteredHeader).Get(), &m_pointerEnteredPreviousButtonToken));
        IFC(spPreviousButtonAsUIE->add_PointerExited(wrl::Callback<xaml_input::IPointerEventHandler>(this, &Pivot::OnPointerExitedHeader).Get(), &m_pointerExitedPreviousButtonToken));
        IFC(m_tpPreviousButton.As(&spPreviousButtonAsButtonBase));
        IFC(spPreviousButtonAsButtonBase->add_Click(wrl::Callback<xaml::IRoutedEventHandler>(this, &Pivot::OnPreviousButtonClick).Get(), &m_previousButtonClickedToken));
    }

    IFC(ValidateItemIndex());

    IFC(GoToOrientationState());
    IFC(UpdateTitleControlVisibility());

    IFC(m_animator.SetTargets(spItemsPanelTranslateTransform.Get(), spItemsPresenter.Get(), spHeaderOffsetTransform.Get()));
    IFC(m_stateMachine.ApplyTemplateEvent(HasValidTemplate()));
    IFC(m_headerManager.ApplyTemplateEvent(
        Private::As<xaml_controls::IPanel, xaml_primitives::IPivotHeaderPanel>(spHeaderPanel).Get(),
        Private::As<xaml_controls::IPanel, xaml_primitives::IPivotHeaderPanel>(spStaticHeaderPanel).Get()));

    if (m_tpPanel)
    {
        IFC(static_cast<xaml_primitives::PivotPanel*>(m_tpPanel.Get())->SetParentPivot(this));
    }

Cleanup:
    RRETURN(hr);
}
#pragma endregion

#pragma region IItemsControlOverrides methods
_Check_return_ HRESULT
Pivot::PrepareContainerForItemOverrideImpl(
    _In_ xaml::IDependencyObject* element,
    _In_ IInspectable *item)
{
    wrl::ComPtr<xaml::IDependencyObject> spElement(element);
    wrl::ComPtr<xaml_controls::IPivotItem> spPivotItem;

    IFC_RETURN(PivotGenerated::PrepareContainerForItemOverrideImpl(element, item));

    // The container instance comes from GetContainerForItem below and
    // must be typed as a PivotItem.
    IFC_RETURN(spElement.As(&spPivotItem));

    PivotItem* pPivotItemNoRef = static_cast<xaml_controls::PivotItem*>(spPivotItem.Get());
    pPivotItemNoRef->SetParent(this);

    // NOTE: This event occurs before we update Pivot with the correct index when the
    // ItemsCollection has changed due to the ordering of event subscription (ItemsControl
    // fires this before callings its override OnItemsChanged method). We can't do anything
    // involving the index or selected item here.
    IFC_RETURN(pPivotItemNoRef->SetContentVisibility(xaml::Visibility_Collapsed));
    IFC_RETURN((Private::As<xaml::IUIElement, xaml_controls::IPivotItem>(spPivotItem))->
        put_Opacity(0.0));

     return S_OK;
}

_Check_return_ HRESULT
Pivot::OnItemsChangedImpl(_In_ IInspectable* e)
{
    wrl::ComPtr<IInspectable> spArgsAsInsp(e);
    wrl::ComPtr<wfc::IVectorChangedEventArgs> spVectorChangedEventArgs;
    wrl::ComPtr<wfc::IVector<IInspectable*>> spItems;
    wfc::CollectionChange collectionChange = wfc::CollectionChange_Reset;
    UINT nIndex = 0;

    IFC_RETURN(PivotGenerated::OnItemsChangedImpl(e));

    IFC_RETURN(spArgsAsInsp.As(&spVectorChangedEventArgs));
    IFC_RETURN(spVectorChangedEventArgs->get_CollectionChange(&collectionChange));
    IFC_RETURN(spVectorChangedEventArgs->get_Index(&nIndex));
    IFC_RETURN(GetItems(&m_itemCount, &spItems));

    if (collectionChange == wfc::CollectionChange_Reset)
    {
        //
        // The ItemsHost has been invalidated at this point.
        // Pivot::HasItemsHost uses the value of m_tpItemsPanel to cache
        // whether the ItemsHost is valid or not. By clearing m_tpItemsPanel,
        // we make sure HasItemsHost returns a fresh answer.
        // This will impact what UpdateVisibleContent does.
        // Indeed, in case of a reset, we don't want UpdateVisibleContent
        // to do anything until the next layout pass, when it fires the unload events
        // and, if there is a selected item, the loaded events.
        //
        m_tpItemsPanel.Clear();
    }

    IFC_RETURN(AutomationRaisePropertyChangedEvents());
    IFC_RETURN(m_headerManager.ItemsCollectionChangedEvent(spItems.Get(), m_itemCount, nIndex, collectionChange));
    IFC_RETURN(m_stateMachine.ItemsCollectionChangedEvent(m_itemCount, nIndex, collectionChange));

    // We need to call this method to process the new size of our static/dynamic headers and decide which one we should be using
    IFC_RETURN(InvalidateArrange());
    if (m_tpPanel && m_isHeaderItemsCarouselEnabled == false)
    {
        // When IsHeaderItemsCarouselEnabled is false, invalidate measure on PivotPanel because its
        // desired size depends on the items count as well.
        IFC_RETURN(static_cast<xaml_primitives::PivotPanel*>(m_tpPanel.Get())->InvalidateMeasure());
    }

    IFC_RETURN(UpdateVisualStates());

    return S_OK;
}

_Check_return_ HRESULT
Pivot::IsItemItsOwnContainerOverrideImpl(
    _In_opt_ IInspectable* item,
    _Out_ BOOLEAN* returnValue)
{
    wrl::ComPtr<IInspectable> spItem(item);
    wrl::ComPtr<xaml_controls::IPivotItem> spPivotItem;

    if(item != nullptr)
    {
        IGNOREHR(spItem.As(&spPivotItem));
    }

    *returnValue = (nullptr != spPivotItem);

    RRETURN(S_OK);
}

_Check_return_ HRESULT
Pivot::GetContainerForItemOverrideImpl(
    _Outptr_ xaml::IDependencyObject** returnValue)
{
    HRESULT hr = S_OK;

    *returnValue = nullptr;
    wrl::ComPtr<xaml_controls::IPivotItem> spPivotItem;
    IFC(wrl::MakeAndInitialize<xaml_controls::PivotItem>(&spPivotItem));
    IFC(spPivotItem.CopyTo(returnValue));

Cleanup:
    RRETURN(hr);
}
#pragma endregion

#pragma region IUIElementOverrides methods

_Check_return_ HRESULT
Pivot::OnCreateAutomationPeerImpl(
    _Outptr_ xaml::Automation::Peers::IAutomationPeer **returnValue)
{
    HRESULT hr = S_OK;
    wrl::ComPtr<xaml_controls::Pivot> spThis(this);
    wrl::ComPtr<xaml_controls::IPivot> spThisAsIPivot;
    wrl::ComPtr<xaml_automation_peers::PivotAutomationPeer> spPivotAutomationPeer;

    IFC(spThis.As(&spThisAsIPivot));
    IFC(wrl::MakeAndInitialize<xaml_automation_peers::PivotAutomationPeer>
            (&spPivotAutomationPeer, spThisAsIPivot.Get()));

    IFC(spPivotAutomationPeer.CopyTo(returnValue));

Cleanup:
    RRETURN(hr);
}

#pragma endregion

#pragma region IControlOverrides methods

_Check_return_ HRESULT
Pivot::OnKeyDownImpl(_In_ xaml_input::IKeyRoutedEventArgs* args)
{
    BOOLEAN handled;
    IFC_RETURN(args->get_Handled(&handled));
    if (handled)
    {
        return S_OK;
    }

    BOOLEAN isEnabled;
    IFC_RETURN(m_pThisAsControlNoRef->get_IsEnabled(&isEnabled));
    if (!isEnabled)
    {
        return S_OK;
    }

    wsy::VirtualKey pressedKey;
    wsy::VirtualKey originalKey;
    IFC_RETURN(args->get_Key(&pressedKey));
    IFC_RETURN(args->get_OriginalKey(&originalKey));

    wrl::ComPtr<xaml::IFrameworkElement> spThisAsFE;
    IFC_RETURN(QueryInterface(__uuidof(xaml::IFrameworkElement), &spThisAsFE));

    xaml::FlowDirection flowDirection = xaml::FlowDirection_LeftToRight;
    IFC_RETURN(spThisAsFE->get_FlowDirection(&flowDirection));

    // Switch GamepadRightShoulder/GamepadLeftShoulder when in RTL.
    if(flowDirection == xaml::FlowDirection_RightToLeft)
    {
        if (pressedKey == wsy::VirtualKey_GamepadRightShoulder)
        {
            pressedKey = wsy::VirtualKey_GamepadLeftShoulder;
        }
        else if (pressedKey == wsy::VirtualKey_GamepadLeftShoulder)
        {
            pressedKey = wsy::VirtualKey_GamepadRightShoulder;
        }
    }

    bool selectedPivotItemIsChanging = false;

    switch (pressedKey)
    {
      case wsy::VirtualKey_Control:
          m_isControlKeyPressed = true;
          IFC_RETURN(args->put_Handled(true));
          break;

      case wsy::VirtualKey_Shift:
          m_isShiftKeyPressed = true;
          IFC_RETURN(args->put_Handled(true));
          break;

      case wsy::VirtualKey_PageDown:
          if (m_isControlKeyPressed)
          {
              IFC_RETURN(MoveToNextItem(ShouldWrap()));
              IFC_RETURN(args->put_Handled(true));
              selectedPivotItemIsChanging = true;
          }
          break;

      case wsy::VirtualKey_PageUp:
          if (m_isControlKeyPressed)
          {
              IFC_RETURN(MoveToPreviousItem(ShouldWrap()));
              IFC_RETURN(args->put_Handled(true));
              selectedPivotItemIsChanging = true;
          }
          break;

      case wsy::VirtualKey_Tab:
          if (m_isControlKeyPressed)
          {
              if (m_isShiftKeyPressed)
              {
                  IFC_RETURN(MoveToPreviousItem(ShouldWrap()));
              }
              else
              {
                  IFC_RETURN(MoveToNextItem(ShouldWrap()));
              }

              IFC_RETURN(args->put_Handled(true));
              selectedPivotItemIsChanging = true;
          }
          break;

      case wsy::VirtualKey_GamepadLeftShoulder:
          IFC_RETURN(MoveToPreviousItem(false /* shouldWrap */));
          IFC_RETURN(args->put_Handled(true));
          break;

      case wsy::VirtualKey_GamepadRightShoulder:
          IFC_RETURN(MoveToNextItem(false /* shouldWrap */));
          IFC_RETURN(args->put_Handled(true));
          break;
    }

    // If the current pivot item has focus and we
    // we are selecting a new one after handling this keyboard
    // event, then we transfer focus to the pivot itself temporarily
    // until the new pivot item is selected. At which point, we will
    // give it focus back.
    if (selectedPivotItemIsChanging &&
        m_tpCurrentPivotItem &&
        m_pThisAsControlNoRef)
    {
        bool currentPivotItemHasFocus;
        IFC_RETURN(HasFocus(m_tpCurrentPivotItem.Get(), &currentPivotItemHasFocus));

        if (currentPivotItemHasFocus)
        {
            m_keyboardFocusToNextPivotItemPending = true;
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
Pivot::OnKeyUpImpl(_In_ xaml_input::IKeyRoutedEventArgs* args)
{
    wsy::VirtualKey pressedKey;
    IFC_RETURN(args->get_Key(&pressedKey));

    switch (pressedKey)
    {
    case wsy::VirtualKey_Control:
        m_isControlKeyPressed = false;
        break;

    case wsy::VirtualKey_Shift:
        m_isShiftKeyPressed = false;
        break;
    }

    return S_OK;
}

#pragma endregion

#pragma region PivotItemsPanel Helpers
_Check_return_ HRESULT
Pivot::UpdateVisibleContent(_In_ INT index, _In_ PivotAnimationDirection animationHint)
{
    HRESULT hr = S_OK;
    BOOLEAN bHasItemsHost = FALSE;

    IFC(HasItemsHost(&bHasItemsHost));

    if (bHasItemsHost)
    {
        wrl::ComPtr<xaml_controls::IItemsControl> spThisAsIC;
        wrl::ComPtr<xaml_controls::IItemContainerGenerator> spGenerator;
        wrl::ComPtr<xaml::IDependencyObject> spContainer;
        wrl::ComPtr<wfc::IVector<IInspectable*>> spItemsAsVector;
        wrl::ComPtr<xaml::IUIElement> spContainerAsUE;
        wrl::ComPtr<xaml_hosting::IElementCompositionPreviewStatics> elementCompositionPreviewStatics;
        UINT itemCount = 0;

        IFC(QueryInterface(__uuidof(xaml_controls::IItemsControl), &spThisAsIC));
        IFC(spThisAsIC->get_ItemContainerGenerator(&spGenerator));

        IFC(GetItems(&itemCount, &spItemsAsVector));

        if (m_tpCurrentPivotItem)
        {
            wrl::ComPtr<xaml::IDependencyObject> spNewItemContainer;
            wrl::ComPtr<xaml_controls::IPivotItem> spNewItemContainerAsPI;
            IFC(spGenerator->ContainerFromIndex(index, &spNewItemContainer));
            // If there is no new item (e.g. removing the last item in the collection)
            // then spNewItemContainer will be null.
            if (spNewItemContainer)
            {
                IFC(spNewItemContainer.As(&spNewItemContainerAsPI));
            }

            if (spNewItemContainerAsPI.Get() != m_tpCurrentPivotItem.Get())
            {
                IFC(RaiseOnPivotItemUnloaded(m_tpCurrentPivotItem.Get()));
                m_tpCurrentPivotItem.Clear();
            }
        }

        IFC(wf::GetActivationFactory(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Hosting_ElementCompositionPreview).Get(), &elementCompositionPreviewStatics));

        // By setting the visibility of only a few items in a Grid and
        // moving all but one of the visible items outside the bounds of the ItemsPresenter
        // we can make the ItemsPresenter operate like a single ContentPresenter
        // with some extra content on the sides revealed when swiped in.
        for (UINT i=0; i < itemCount; i++)
        {
           BOOLEAN firstExpansion = FALSE;

           BOOLEAN toVisible = FALSE;

           {
               BOOLEAN isCurrentItem = i == static_cast<UINT>(index);
               BOOLEAN isNeighbor = i == static_cast<UINT>(PositiveMod((index - 1), itemCount)) || i == static_cast<UINT>(PositiveMod((index + 1), itemCount));

               // We only want neighbors to be visible during the swipe, they should be set back to collapsed state after swiping,
               // because otherwise narrator will pickup those "invisible"(invisible to user but visible in visual tree) items.
               // OnDirectManipulationCompleted will set m_isDirectManipulationInProgress to false and call this function, we use that opportunity to collapse neighbors.
               toVisible = (isCurrentItem ||
                   (isNeighbor && m_isDirectManipulationInProgress));
           }

            IFC(spGenerator->ContainerFromIndex(i, &spContainer));

            // When the ItemsSource is Reset ItemsControl clears the set of realized ItemsContainers.
            // This is unlike all the other ItemsChanged events, where it properly adds/removes the items
            // before firing the ItemsCollectionChanged events to us.
            // We've added code in PivotStateMachine to properly handle this condition (see
            // the m_fPendingItemHostValidation flag).
#if DBG
            ASSERT(spContainer || m_stateMachine.IsPendingItemsHostValidation());
#endif
            if (spContainer)
            {
                IFC(spContainer.As(&spContainerAsUE));

                IFC(UpdateItemVisibility(spContainerAsUE.Get(), toVisible, &firstExpansion));

                {
                    if (toVisible)
                    {
                        wrl::ComPtr<xaml::IFrameworkElement> spContainerAsFE;
                        wrl::ComPtr<WUComp::IVisual> spVisual;
                        wrl::ComPtr<WUComp::ICompositionObject> spCompObj;
                        wrl::ComPtr<WUComp::ICompositionPropertySet> spCompPropSet;
                        wfn::Vector3 translation;
                        DOUBLE xOffset = 0;

                        elementCompositionPreviewStatics->GetElementVisual(spContainerAsUE.Get(), &spVisual);
                        elementCompositionPreviewStatics->SetIsTranslationEnabled(spContainerAsUE.Get(), true);
                        IFC(spVisual.As(&spCompObj));
                        spCompObj.Get()->get_Properties(&spCompPropSet);

                        if (m_tpItemsPresenter)
                        {
                            IFC(m_tpItemsPresenter.As(&spContainerAsFE));
                        }
                        else
                        {
                            IFC(spContainer.As(&spContainerAsFE));
                        }

                        if (itemCount == 1)
                        {
                            xOffset = 0;
                        }
                        else if (itemCount == 2)
                        {
                            if (i != static_cast<UINT>(index))
                            {
                                if (animationHint == PivotAnimationDirection_Left)
                                {
                                    spContainerAsFE.Get()->get_ActualWidth(&xOffset);
                                    xOffset *= -1;
                                }
                                else
                                {
                                    spContainerAsFE.Get()->get_ActualWidth(&xOffset);
                                }
                            }
                            else
                            {
                                xOffset = 0;
                            }
                        }
                        else
                        {
                            if (i == static_cast<UINT>(PositiveMod((index + 1), itemCount)))
                            {
                                spContainerAsFE.Get()->get_ActualWidth(&xOffset);
                            }
                            else if (i == static_cast<UINT>(PositiveMod((index - 1), itemCount)))
                            {
                                spContainerAsFE.Get()->get_ActualWidth(&xOffset);
                                xOffset *= -1;
                            }
                            else
                            {
                                xOffset = 0;
                            }
                        }

                        translation = { static_cast<float>(xOffset), 0, 0 };
                        spCompPropSet.Get()->InsertVector3(wrl_wrappers::HStringReference(L"Translation").Get(), translation);
                    }
                }

                if (i == static_cast<UINT>(index) && animationHint != PivotAnimationDirection_Reset && !DesignerInterop::GetDesignerMode(DesignerMode::V2Only))
                {
                    wrl::ComPtr<xaml_controls::IPivotItem> spPivotItem;
                    IFC(spContainer.As(&spPivotItem));

                    if (firstExpansion)
                    {
                        wrl::ComPtr<Pivot> spThis(this);
                        wrl::WeakRef wrThis;
                        IFC(spThis.AsWeak(&wrThis));
                        IFC(SetPtrValue(m_tpPendingSlideInItem, spPivotItem.Get()));
                        IFC(EnsureStaticsAndFactories());

                        // When the items collection is updated multiple times
                        // before the first render this code can be executed more
                        // than once, leaving dangling copies of the event handler
                        // registered, we catch that by examining the value of the token.
                        if (!m_slideInPreRenderToken.value)
                        {
                            IFC(s_spCompositionTargetStatics->add_Rendering(
                                wrl::Callback<wf::IEventHandler<IInspectable*>>([wrThis, animationHint] (
                                        _In_ IInspectable* /* pSender */, _In_ IInspectable* /* pArgs */) mutable -> HRESULT
                                    {
                                        HRESULT hr = S_OK;

                                        wrl::ComPtr<IPivot> spThis;
                                        IFC(wrThis.As(&spThis));
                                        if (spThis)
                                        {
                                            Pivot* pPivotNoRef = static_cast<Pivot*>(spThis.Get());

                                            ASSERT(pPivotNoRef->m_slideInPreRenderToken.value);
                                            IFC(s_spCompositionTargetStatics->remove_Rendering(pPivotNoRef->m_slideInPreRenderToken));
                                            pPivotNoRef->m_slideInPreRenderToken.value = 0;

                                            if (pPivotNoRef->m_tpPendingSlideInItem)
                                            {
                                                IFC(PivotSlideInManager::ApplySlideInAnimation(pPivotNoRef->m_tpPendingSlideInItem.Get(), animationHint));
                                                pPivotNoRef->m_tpPendingSlideInItem.Clear();
                                            }
                                        }

                                    Cleanup:
                                        RRETURN(hr);
                                    }).Get()
                                , &m_slideInPreRenderToken));
                        }
                    }
                    else
                    {
                        IFC(PivotSlideInManager::ApplySlideInAnimation(spPivotItem, animationHint));
                    }
                }
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
Pivot::UpdateItemVisibility(_In_ xaml::IUIElement* pContainer, _In_ BOOLEAN toVisible, _Out_ BOOLEAN* pFirstExpand)
{
    wrl::ComPtr<xaml::IUIElement> spContainer(pContainer);
    wrl::ComPtr<IPivotItem> spPivotItem;

    ASSERT(pContainer);

    *pFirstExpand = FALSE;

    IFC_RETURN(spContainer->put_IsHitTestVisible(toVisible));

    DOUBLE currentOpacity = 0.0;
    IFC_RETURN(spContainer->get_Opacity(&currentOpacity));
    IFC_RETURN(spContainer->put_Opacity(toVisible ? 1.0 : 0.0));

    IFC_RETURN(spContainer.As(&spPivotItem));

    PivotItem* pPivotItemNoRef = static_cast<PivotItem*>(spPivotItem.Get());
    xaml::Visibility visibility = xaml::Visibility_Visible;

    IFC_RETURN(pPivotItemNoRef->GetContentVisibility(&visibility));

    if (toVisible && visibility == xaml::Visibility_Collapsed)
    {
        IFC_RETURN(pPivotItemNoRef->SetContentVisibility(xaml::Visibility_Visible));
        // Content of the pivot item isn't loaded yet.
        // Set this flag to postpone setting up the animation until the layout
        // pass completes.
        *pFirstExpand = TRUE;
    }
    else if (!toVisible && visibility == xaml::Visibility_Visible)
    {
        // This pivot item is no longer visible, so
        // we want to collapse this pivot item in order to make tabbing ignore it.
        IFC_RETURN(pPivotItemNoRef->SetContentVisibility(xaml::Visibility_Collapsed));
    }

    if (toVisible && currentOpacity == 0.0)
    {
        // We want to make sure that any ItemTemplates have been realized
        // before we raise PivotItemLoading/Loaded, so we'll do so now.
        IFC_RETURN(pPivotItemNoRef->RealizeContent());

        IFC_RETURN(RaiseOnPivotItemLoaded(spPivotItem.Get()));

        // Store this item for later when updating item visibility such
        // that we can fire the unloaded event.
        IFC_RETURN(SetPtrValue(m_tpCurrentPivotItem, spPivotItem.Get()));
    }

    // Should we give keyboard focus to this pivot item?
    if (toVisible && m_keyboardFocusToNextPivotItemPending)
    {
        m_keyboardFocusToNextPivotItemPending = false;
        IFC_RETURN(pPivotItemNoRef->SetKeyboardFocus(!!(*pFirstExpand) /* postponeUntilNextMeasure */));
    }

    return S_OK;
}

_Check_return_ HRESULT
Pivot::IsLoaded(_Out_ bool* isLoaded)
{
    wrl::ComPtr<xaml::IDependencyObject> spParent;
    wrl::ComPtr<xaml::IDependencyObject> thisAsDO;

    *isLoaded = false;
    IFC_RETURN(QueryInterface(__uuidof(xaml::IDependencyObject), &thisAsDO));
    IFC_RETURN(EnsureStaticsAndFactories());
    IFC_RETURN(s_spVisualTreeHelperStatics->GetParent(thisAsDO.Get(), &spParent));
    *isLoaded = spParent != nullptr;

    return S_OK;
}

_Check_return_ HRESULT
Pivot::HasItemsHost(_Out_ BOOLEAN* pValue)
{
    HRESULT hr = S_OK;
    BOOLEAN ret = FALSE;
    wrl::ComPtr<xaml_controls::IItemsControl> spThisAsIC;
    wrl::ComPtr<xaml_controls::IItemContainerGenerator> spGenerator;
    wrl::ComPtr<xaml::IDependencyObject> spContainer;
    wrl::ComPtr<xaml::IDependencyObject> spParent;

    if (m_tpItemsPanel)
    {
        *pValue = TRUE;
        goto Cleanup;
    }

    IFC(QueryInterface(__uuidof(xaml_controls::IItemsControl), &spThisAsIC));
    IFC(spThisAsIC->get_ItemContainerGenerator(&spGenerator));

    if (nullptr != spGenerator)
    {
        // Get the parent of any live container
        IFC(spGenerator->ContainerFromIndex(0, &spContainer));

        if (nullptr != spContainer)
        {
            IFC(EnsureStaticsAndFactories());
            IFC(s_spVisualTreeHelperStatics->GetParent(spContainer.Get(), &spParent));

            if (nullptr != spParent)
            {
                wrl::ComPtr<xaml_controls::IPanel> spItemsPanel;

                IFC(spParent.As(&spItemsPanel));
                IFC(SetPtrValue(m_tpItemsPanel, spItemsPanel.Get()));

                ret = (nullptr != spItemsPanel);
            }
        }
    }

    *pValue = ret;

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
Pivot::GetItems(_Out_opt_ UINT* pItemsCount, _Outptr_opt_result_maybenull_ wfc::IVector<IInspectable*>** ppItems)
{
    HRESULT hr = S_OK;

    wrl::ComPtr<Pivot> spPivot(this);
    wrl::ComPtr<xaml_controls::IItemsControl> spPivotAsIC;
    wrl::ComPtr<wfc::IObservableVector<IInspectable*>> spItems;
    wrl::ComPtr<wfc::IVector<IInspectable*>> spItemsAsVector;

    if (pItemsCount)
    {
        *pItemsCount = 0;
    }

    if (ppItems)
    {
        *ppItems = nullptr;
    }

    IFC(spPivot.As(&spPivotAsIC));
    IFC(spPivotAsIC->get_Items(&spItems));
    IFC(spItems.As(&spItemsAsVector));

    if (pItemsCount)
    {
        IFC(spItemsAsVector->get_Size(pItemsCount));
    }

    if (ppItems)
    {
        IFC(spItemsAsVector.CopyTo(ppItems));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
Pivot::GiveFocusTo(_In_ xaml_controls::IControl* target, _In_ xaml::FocusState focusState)
{
    BOOLEAN ignored;
    wrl::ComPtr<xaml::IUIElement> thisAsUIE;
    IFC_RETURN(target->QueryInterface(IID_PPV_ARGS(&thisAsUIE)));

    IFC_RETURN(thisAsUIE->Focus(focusState, &ignored));
    return S_OK;
}

_Check_return_ HRESULT
Pivot::GiveKeyboardFocusTo(_In_ xaml_controls::IControl* target)
{
    return GiveFocusTo(target, xaml::FocusState::FocusState_Keyboard);
}

_Check_return_ HRESULT
Pivot::PerformAutoFocus(_In_ xaml_input::FocusNavigationDirection direction)
{
    wrl::ComPtr<xaml::IUIElement> thisAsUIE;
    IFC_RETURN(this->QueryInterface(IID_PPV_ARGS(&thisAsUIE)));

    wrl::ComPtr<xaml::IXamlRoot> xamlRoot;
    IFC_RETURN(thisAsUIE->get_XamlRoot(&xamlRoot));

    if (xamlRoot)
    {
        wrl::ComPtr<xaml_input::IFocusManagerStatics> spFocusManager;
        IFC_RETURN(wf::GetActivationFactory(
            wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Input_FocusManager).Get(),
            &spFocusManager));

        wrl::ComPtr<xaml_input::IFindNextElementOptions> findNextElementOptions;
        IFC_RETURN(wf::ActivateInstance(
            wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Input_FindNextElementOptions).Get(),
            &findNextElementOptions));

        wrl::ComPtr<xaml::IUIElement> xamlRootContent;
        IFC_RETURN(xamlRoot->get_Content(&xamlRootContent));

        wrl::ComPtr<xaml::IDependencyObject> xamlRootContentAsDO;
        IFC_RETURN(xamlRootContent.As(&xamlRootContentAsDO));

        IFC_RETURN(findNextElementOptions->put_SearchRoot(xamlRootContentAsDO.Get()));

        BOOLEAN returnValueIgnored;
        IFC_RETURN(spFocusManager->TryMoveFocusWithOptions(direction, findNextElementOptions.Get(), &returnValueIgnored));
    }

    return S_OK;
}

_Check_return_ HRESULT
Pivot::HasFocus(_Out_ bool* hasFocus)
{
    wrl::ComPtr<xaml::IDependencyObject> thisAsDO;
    IFC_RETURN(this->QueryInterface(__uuidof(xaml::IDependencyObject), &thisAsDO));
    return HasFocus(this /* parent */, hasFocus);
}

_Check_return_ HRESULT
Pivot::HasFocus(_In_ xaml::IDependencyObject* parent, _Out_ bool* hasFocus)
{
    wrl::ComPtr<IInspectable> spFocusedElt;

    *hasFocus = false;

    wrl::ComPtr<xaml::IUIElement> thisAsUIE;
    IFC_RETURN(this->QueryInterface(IID_PPV_ARGS(&thisAsUIE)));

    wrl::ComPtr<xaml::IXamlRoot> xamlRoot;
    IFC_RETURN(thisAsUIE->get_XamlRoot(&xamlRoot));

    if (xamlRoot)
    {
        wrl::ComPtr<xaml_input::IFocusManagerStatics> spFocusManager;
        IFC_RETURN(wf::GetActivationFactory(
            wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Input_FocusManager).Get(),
            &spFocusManager));

        IFC_RETURN(spFocusManager->GetFocusedElementWithRoot(xamlRoot.Get(), &spFocusedElt));

        if (spFocusedElt)
        {
            wrl::ComPtr<xaml::IDependencyObject> spFocusedEltAsDO;
            IFC_RETURN(spFocusedElt.As(&spFocusedEltAsDO));
            IFC_RETURN(IsAscendantOfTarget(parent, spFocusedEltAsDO.Get(), hasFocus));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
Pivot::IsAscendantOfTarget(_In_ xaml::IDependencyObject* parent, _In_ xaml::IDependencyObject* child, _Out_ bool* isAscendantOfTarget)
{
    wrl::ComPtr<xaml::IDependencyObject> spCurrentDO(child);
        wrl::ComPtr<xaml_media::IVisualTreeHelperStatics> spVTHStatics;

    bool isFound = false;
    *isAscendantOfTarget = false;

    IFC_RETURN(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_VisualTreeHelper).Get(),
        &spVTHStatics));

    while (spCurrentDO && !isFound)
    {
        if (spCurrentDO.Get() == parent)
        {
            isFound = true;
        }
        else
        {
            wrl::ComPtr<xaml::IDependencyObject> spParent;
            IFC_RETURN(spVTHStatics->GetParent(spCurrentDO.Get(), &spParent));
            spCurrentDO = std::move(spParent);
        }
    }

    *isAscendantOfTarget = isFound;
    return S_OK;
}

_Check_return_ HRESULT
Pivot::ValidateItemIndex()
{
    HRESULT hr = S_OK;

    UINT itemCount = 0;
    IFC(GetItems(&itemCount, nullptr));

    if (itemCount > 0)
    {
        INT selectedIndex = 0;
        wrl::ComPtr<IInspectable> spSelectedItem;

        IFC(get_SelectedIndex(&selectedIndex));
        IFC(get_SelectedItem(&spSelectedItem));

        BOOLEAN itemValid = TRUE;
        BOOLEAN indexValid = FALSE;

        IFC(ValidateSelectedIndex(selectedIndex, &indexValid));

        if (!indexValid)
        {
            IFC(E_FAIL);
        }

        if (spSelectedItem)
        {
            INT32 itemIdx = 0;
            IFC(ValidateSelectedItem(spSelectedItem.Get(), &itemIdx, &itemValid));

            if (itemIdx != selectedIndex)
            {
                IFC(E_FAIL);
            }
        }
    }

Cleanup:
    RRETURN(hr);
}
#pragma endregion

_Check_return_ HRESULT
Pivot::EnsureStaticsAndFactories()
{
    HRESULT hr = S_OK;

    if (!s_spStoryboardStatics)
    {
        IFC(wf::GetActivationFactory(
            wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_Storyboard).Get(),
            &s_spStoryboardStatics));
    }

    if (!s_spVisualTreeHelperStatics)
    {
        IFC(wf::GetActivationFactory(
            wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_VisualTreeHelper).Get(),
            &s_spVisualTreeHelperStatics));
    }

    if (!s_spCompositionTargetStatics)
    {
        IFC(wf::GetActivationFactory(
            wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_CompositionTarget).Get(),
            &s_spCompositionTargetStatics));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
/* static */ Pivot::GetScreenWidth(_In_ Pivot* const pivot, _Out_ DOUBLE* pWidth)
{
    *pWidth = 0.0;

    wrl::ComPtr<xaml::IUIElement> thisAsUIE;
    IFC_RETURN(pivot->QueryInterface(IID_PPV_ARGS(&thisAsUIE)));

    wrl::ComPtr<xaml::IXamlRoot> xamlRoot;
    IFC_RETURN(thisAsUIE->get_XamlRoot(&xamlRoot));

    if (xamlRoot)
    {
        wf::Size xamlRootSize;
        IFC_RETURN(xamlRoot->get_Size(&xamlRootSize));
        *pWidth = xamlRootSize.Width;
    }

    return S_OK;
}

_Check_return_ HRESULT
/* static */ Pivot::GetZoomScale(_Out_ DOUBLE* zoomScale)
{
    wrl::ComPtr<wgrd::IDisplayInformationStatics> spDisplayInformationStatics;
    wrl::ComPtr<wgrd::IDisplayInformation> spDisplayInformation;

    IFC_RETURN(wf::GetActivationFactory(wrl_wrappers::HStringReference(
        RuntimeClass_Windows_Graphics_Display_DisplayInformation).Get(),
        &spDisplayInformationStatics
    ));

    IFC_RETURN(spDisplayInformationStatics->GetForCurrentView(&spDisplayInformation));
    IFC_RETURN(Private::As<wgrd::IDisplayInformation2>(spDisplayInformation)->get_RawPixelsPerViewPixel(zoomScale));

    return S_OK;
}

#pragma region StateMachineCallbacks
_Check_return_ HRESULT
Pivot::SetPivotSectionOffset(_In_ DOUBLE offset)
{
    HRESULT hr = S_OK;

    IFC(m_animator.Stop());

    if (m_tpItemsPanelTranslateTransform)
    {
        IFC(m_tpItemsPanelTranslateTransform->put_X(offset));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
Pivot::SetViewportOffset(_In_ DOUBLE offset, _In_ BOOLEAN animate, _Out_ bool *success)
{
    HRESULT hr = S_OK;

    *success = false;

    if (m_tpScrollViewer)
    {
        wrl::ComPtr<IInspectable> spHorizontalOffsetAsInspectable;
        wrl::ComPtr<wf::IReference<double>> spHorizontalOffset;

        IFC(Private::ValueBoxer::CreateDouble(offset, &spHorizontalOffsetAsInspectable));
        IFC(spHorizontalOffsetAsInspectable.As(&spHorizontalOffset));

        if (!animate)
        {
            BOOLEAN returnValue = FALSE;
            xaml_controls::SnapPointsType snapPointsType;

            // When we jump to an offset, we know that the offset is valid in regards to our snap points
            // configuration. However, we could be in the middle of a Pivot resize operation in which case
            // the ScrollViewer still hasn't updated its viewport size and, consequently, will mess with the
            // offset we are asking to scroll to. To avoid that, let's temporarily disable snap points.
            IFC(m_tpScrollViewer->get_HorizontalSnapPointsType(&snapPointsType));
            IFC(m_tpScrollViewer->put_HorizontalSnapPointsType(xaml_controls::SnapPointsType_None));

            IFC(m_tpScrollViewer->ChangeViewWithOptionalAnimation(
                spHorizontalOffset.Get() /* horizontalOffset */,
                nullptr /* verticalOffset */,
                nullptr /* zoomFactor */,
                TRUE /* disableAnimation */,
                &returnValue));

            // Restore the original snap points type.
            IFC(m_tpScrollViewer->put_HorizontalSnapPointsType(snapPointsType));

            IFC(m_headerManager.SyncParallax());

            *success = !!returnValue;
        }
        else
        {
            // This is truly evil. Calling ZoomToRect is an immediate thing. Setting
            // and submitting a viewport to the compositor is not. This might be fixed
            // with first touch improvements, but right now we delay submitting our ZoomToRect
            // call until immediately before submitting the new frame to render to minimize this
            // time and get as smooth of an animation as possible.
            wrl::ComPtr<Pivot> spThis(this);
            wrl::WeakRef wrThis;
            IFC(spThis.AsWeak(&wrThis));
            IFC(EnsureStaticsAndFactories());
            IFC(s_spCompositionTargetStatics->add_Rendering(
                wrl::Callback<wf::IEventHandler<IInspectable*>>([wrThis, spHorizontalOffset]
                (_In_ IInspectable* /* pSender */, _In_ IInspectable* /* pArgs */) mutable -> HRESULT
                    {
                        HRESULT hr = S_OK;

                        wrl::ComPtr<IPivot> spThis;
                        IFC(wrThis.As(&spThis));
                        if (spThis)
                        {
                            Pivot* pPivotNoRef = static_cast<Pivot*>(spThis.Get());
                            BOOLEAN returnValue = FALSE;

                            IFC(s_spCompositionTargetStatics->remove_Rendering(pPivotNoRef->m_changeViewPreRenderToken));
                            pPivotNoRef->m_changeViewPreRenderToken.value = 0;
                            IFC(pPivotNoRef->m_tpScrollViewer->ChangeViewWithOptionalAnimation(
                                spHorizontalOffset.Get() /* horizontalOffset */,
                                nullptr /* verticalOffset */,
                                nullptr /* zoomFactor */,
                                FALSE /* disableAnimation */,
                                &returnValue));
                        }
                    Cleanup:
                        RRETURN(hr);
                    }).Get()
                , &m_changeViewPreRenderToken));

            // Because of the need for the above callback, we have no way of knowing
            // if the call to ChangeViewWithOptionalAnimation succeeded or not.
            // We're forced to just assume that it did.
            *success = true;
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
Pivot::SetPivotSectionWidth(_In_ FLOAT width)
{
    HRESULT hr = S_OK;

    if (m_tpPanel)
    {
        IFC(static_cast<xaml_primitives::PivotPanel*>(m_tpPanel.Get())->SetSectionWidth(width));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
Pivot::SetSelectedIndex(_In_ INT32 idx, _In_ BOOLEAN updateVisual, _In_ BOOLEAN updateIndex, _In_ BOOLEAN updateItem, _In_ PivotAnimationDirection animationHint)
{
    // If we're about to update the index, we should cache the current
    // selected item header bounding rectangle, since we may need it later
    // if we have dynamic headers.
    if (updateIndex)
    {
        INT selectedIndex = 0;
        wf::Rect boundingRectangle = {};

        IFC_RETURN(get_SelectedIndex(&selectedIndex));
        IFC_RETURN(GetItemHeaderBoundingRectangle(selectedIndex, &boundingRectangle));

        m_cachedSelectedItemHeaderBoundingRectangle = boundingRectangle;
    }

    if (!m_fIndexChangeReentryGuard && updateIndex)
    {
        m_fIndexChangeReentryGuard = TRUE;

        IFC_RETURN(put_SelectedIndex(idx));
        m_fIndexChangeReentryGuard = FALSE;
    }

    if (updateItem)
    {
        // If we're responding to either a user-initiated
        // index change or item change we have a guard to
        // prevent a loop. This allows us to use the same
        // PivotStateMachine callback for user changes and
        // changes from manipulations.
        if (!m_fItemChangeReentryGuard)
        {
            ASSERT(!m_tpOldItem);
            wrl::ComPtr<IInspectable> spOldItem;

            IFC_RETURN(get_SelectedItem(&spOldItem));
            IFC_RETURN(SetPtrValue(m_tpOldItem, spOldItem.Get()));

            // Setting the guard here prevents the event change handler
            // from calling back into the state machine.
            m_fItemChangeReentryGuard = TRUE;
            IFC_RETURN(SyncItemToIndex(idx));
            m_fItemChangeReentryGuard = FALSE;
        }

        wrl::ComPtr<IInspectable> spNewItem;
        IFC_RETURN(get_SelectedItem(&spNewItem));

        // The index can change when the selection doesn't. Shifting and adding/removing
        // items can cause that to happen. When the item changes however we want to fire
        // a selection changed event off to the user. We only do this if the item has
        // actually changed.
        if (m_tpOldItem.Get() != spNewItem.Get())
        {
            wrl::ComPtr<IInspectable> spOldItem = m_tpOldItem.Get();
            m_tpOldItem.Clear();

            IFC_RETURN(AutomationOnSelectionChanged(spOldItem.Get(), spNewItem.Get()));
            IFC_RETURN(RaiseOnSelectionChanged(spOldItem.Get(), spNewItem.Get()));
        }

        m_tpOldItem.Clear();
    }

    // Given that the above raises an event that any app can handle and use
    // to modify the selected index, we need to make sure that we're
    // still working with the actual selected index when we go to update visuals.
    IFC_RETURN(get_SelectedIndex(&idx));

    if (updateVisual)
    {
        if (m_previousSelectedIndex >= 0)
        {
            wrl::ComPtr<xaml_controls::IItemsControl> spThisAsIC;
            wrl::ComPtr<xaml_controls::IItemContainerGenerator> spGenerator;
            wrl::ComPtr<xaml::IDependencyObject> spContainer;

            IFC_RETURN(QueryInterface(__uuidof(xaml_controls::IItemsControl), &spThisAsIC));
            IFC_RETURN(spThisAsIC->get_ItemContainerGenerator(&spGenerator));

            IFC_RETURN(spGenerator->ContainerFromIndex(idx, &spContainer));

            if (spContainer)
            {
                wrl::ComPtr<xaml::IDependencyObject> spPreviousSelectedContainer;

                IFC_RETURN(spGenerator->ContainerFromIndex(m_previousSelectedIndex, &spPreviousSelectedContainer));

                if (spPreviousSelectedContainer)
                {
                    wrl::ComPtr<IPivotItem> spPivotItem;
                    PivotItem* pPivotItemNoRef = nullptr;

                    IFC_RETURN(spContainer.As(&spPivotItem));
                    pPivotItemNoRef = static_cast<PivotItem*>(spPivotItem.Get());

                    wrl::ComPtr<IPivotItem> spPreviousSelectedPivotItem;
                    PivotItem* pPreviousSelectedPivotItemNoRef = nullptr;

                    // If this PivotItem being made visible contains the currently focused element,
                    // and if a UIA client is listening for FocusChanged events, then we want to set
                    // focus to the header panel instead of giving focus to the next focusable element.
                    // This avoids the situation in Narrator where navigating to the next element can
                    // automatically give focus to a TextBox in that element, which prevents the PivotItem
                    // itself from being read.
                    bool hasFocusedElement = false;
                    BOOLEAN focusChangedListenerExists = FALSE;

                    IFC_RETURN(spPreviousSelectedContainer.As(&spPreviousSelectedPivotItem));
                    pPreviousSelectedPivotItemNoRef = static_cast<PivotItem*>(spPreviousSelectedPivotItem.Get());

                    IFC_RETURN(pPreviousSelectedPivotItemNoRef->HasFocusedElement(&hasFocusedElement));

                    IFC_RETURN(Private::AutomationHelper::ListenerExistsHelper(
                        xaml::Automation::Peers::AutomationEvents_SelectionItemPatternOnElementSelected,
                        &focusChangedListenerExists));

                    if (hasFocusedElement)
                    {
                        if (focusChangedListenerExists)
                        {
                            IFC_RETURN(GiveFocusTo(m_tpHeaderClipper.Get(), xaml::FocusState_Programmatic));
                        }
                        else
                        {
                            wrl::ComPtr<xaml_input::IFocusManagerStatics> focusManager;
                            wrl::ComPtr<xaml::IDependencyObject> firstFocusableElementDO;

                            IFC_RETURN(wf::GetActivationFactory(
                                wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Input_FocusManager).Get(),
                                &focusManager));

                            IFC_RETURN(focusManager->FindFirstFocusableElement(spContainer.Get(), &firstFocusableElementDO));

                            bool hasFirstFocusableElement = false;

                            if (firstFocusableElementDO)
                            {
                                IFC_RETURN(pPivotItemNoRef->HasElement(firstFocusableElementDO.Get(), &hasFirstFocusableElement));
                            }

                            if (hasFirstFocusableElement)
                            {
                                //This is the return type of TryFocusAsync, we don't care about the result as we know it will succeed since we are calling it
                                //on the return value of FindFirstFocusableElement so we ignore the result but the call throws if you don't pass a valid pointer.
                                wrl::ComPtr<wf::__FIAsyncOperation_1_Microsoft__CUI__CXaml__CInput__CFocusMovementResult_t> focusMovementResult;
                                IFC_RETURN(focusManager->TryFocusAsync(firstFocusableElementDO.Get(), xaml::FocusState_Programmatic, &focusMovementResult));
                            }
                            else
                            {
                                IFC_RETURN(GiveFocusTo(m_tpHeaderClipper.Get(), xaml::FocusState_Programmatic));
                            }
                        }
                    }
                }
            }
        }

        IFC_RETURN(UpdateVisibleContent(idx, animationHint));
        IFC_RETURN(m_headerManager.SetSelectedIndex(idx, animationHint));

        m_previousSelectedIndex = -1;
    }
    else
    {
        // Even if we're not fully updating visuals, we still want to
        // de-collapse the new item and add its contents to the visual tree,
        // since otherwise things that plug into us such as Narrator
        // will be confused by the fact that we've set a selected index
        // but there isn't actually anything there to give focus to.
        // We need to ensure that it's not yet visible or hit-testable, however,
        // because when we do this it will be added to the visual tree
        // right on top of the current Pivot contents.
        // It will be made visible and hit-testable once the fade-out animation
        // for the previous item completes.
        wrl::ComPtr<xaml_controls::IItemsControl> spThisAsIC;
        wrl::ComPtr<xaml_controls::IItemContainerGenerator> spGenerator;
        wrl::ComPtr<xaml::IDependencyObject> spContainer;

        IFC_RETURN(QueryInterface(__uuidof(xaml_controls::IItemsControl), &spThisAsIC));
        IFC_RETURN(spThisAsIC->get_ItemContainerGenerator(&spGenerator));

        IFC_RETURN(spGenerator->ContainerFromIndex(idx, &spContainer));

        if (spContainer)
        {
            wrl::ComPtr<xaml::IUIElement> spContainerAsUE;
            wrl::ComPtr<IPivotItem> spPivotItem;
            PivotItem* pPivotItemNoRef = nullptr;

            IFC_RETURN(spContainer.As(&spContainerAsUE));

            IFC_RETURN(spContainerAsUE->put_IsHitTestVisible(FALSE));
            IFC_RETURN(spContainerAsUE->put_Opacity(0.0));

            IFC_RETURN(spContainer.As(&spPivotItem));
            pPivotItemNoRef = static_cast<PivotItem*>(spPivotItem.Get());

            IFC_RETURN(pPivotItemNoRef->SetContentVisibility(xaml::Visibility_Visible));
        }
    }

    // After everything else is done, we should notify UI automation
    // of the newly selected item if we've updated the selected index.
    if (updateIndex)
    {
        IFC_RETURN(AutomationFocusSelectedItem());
    }

    return S_OK;
}

_Check_return_ HRESULT
Pivot::UpdateFocusFollowerImpl(_In_ INT idx)
{
    xaml_controls::PivotHeaderFocusVisualPlacement focusVisualPlacement;

    IFC_RETURN(get_HeaderFocusVisualPlacement(&focusVisualPlacement));

    if (focusVisualPlacement == PivotHeaderFocusVisualPlacement_SelectedItemHeader
        && m_tpFocusFollower
        && m_tpHeaderClipper)
    {
        // We do two things here, first we set up an expression animation to translate the focusFollower to the correct location, despite xaml's best efforts to relayout it where we don't want it.
        // Then we set the FocusFollowers Width and Height to match the appropriate header item.
        wrl::ComPtr<xaml_hosting::IElementCompositionPreviewStatics> elementCompositionPreviewStatics;
        IFC_RETURN(wf::GetActivationFactory(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Hosting_ElementCompositionPreview).Get(), &elementCompositionPreviewStatics));

        wrl::ComPtr<WUComp::IVisual> spFocusFollowerVisual;
        wrl::ComPtr<WUComp::ICompositionObject> spCompObj;

        wrl::ComPtr<xaml::IUIElement> spFocusFollowerAsUIElement;
        IFC_RETURN(m_tpFocusFollower.As(&spFocusFollowerAsUIElement));
        IFC_RETURN(elementCompositionPreviewStatics->GetElementVisual(spFocusFollowerAsUIElement.Get(), &spFocusFollowerVisual));
        IFC_RETURN(spFocusFollowerVisual.As(&spCompObj));

        elementCompositionPreviewStatics->SetIsTranslationEnabled(spFocusFollowerAsUIElement.Get(), true);

        wrl::ComPtr<xaml::Media::IGeneralTransform> spTransform;
        wf::Point transformedPosition = { 0,0 };

        wrl::ComPtr<xaml::IUIElement> spHeaderClipperAsUIElement;
        IFC_RETURN(m_tpHeaderClipper.As(&spHeaderClipperAsUIElement));

        xaml::IUIElement* headerNoRef;
        wrl::ComPtr<xaml::IFrameworkElement> headerAsFE;
        IFC_RETURN(GetHeaderAt(idx, &headerNoRef));
        if (headerNoRef)
        {
            wrl::ComPtr<xaml::IUIElement> headerAsUIElement(headerNoRef);
            if (headerAsUIElement)
            {
                IFC_RETURN(spHeaderClipperAsUIElement->TransformToVisual(headerAsUIElement.Get(), &spTransform));
                IFC_RETURN(spTransform->TransformPoint(transformedPosition, &transformedPosition));
                //So we can get the width and height
                IFC_RETURN(headerAsUIElement.As(&headerAsFE));
            }
        }

        //We only need to make the animation once, after that we can instead update just the Scalar parameter
        if (!m_tpFocusFollowerExpresssionAnimation)
        {
            wrl::ComPtr<WUComp::ICompositor> spCompositor;
            IFC_RETURN(spCompObj.Get()->get_Compositor(&spCompositor));

            wrl::ComPtr<WUComp::IExpressionAnimation> spExpAnim;
            // TransformedPosition = startingPosition(0,0) - currentPosition, so it's a negative value.
            // FocusFollower is centered by default, Target.Offset is the offset from (0,0) to the default position.
            // Since FocusFollower is centered, the offset will change when windows size changes, this ExpressionAnimation is used to compensate that offset change and keep FocusFollower at the right position.
            IFC_RETURN(spCompositor.Get()->CreateExpressionAnimationWithExpression(wrl_wrappers::HStringReference(L"(-1 * TransformedPosition) - this.Target.Offset.X").Get(), &spExpAnim));
            wrl::ComPtr<WUComp::ICompositionAnimation>spCompositionAnimation;
            IFC_RETURN(spExpAnim.As(&spCompositionAnimation));
            IFC_RETURN(spCompositionAnimation.Get()->SetScalarParameter(wrl_wrappers::HStringReference(L"TransformedPosition").Get(), transformedPosition.X));

            IFC_RETURN(spCompObj.Get()->StartAnimation(wrl_wrappers::HStringReference(L"Translation.X").Get(), spCompositionAnimation.Get()));

            IFC_RETURN(SetPtrValue(m_tpFocusFollowerExpresssionAnimation, spExpAnim.Get()));
        }
        else
        {
            wrl::ComPtr<WUComp::ICompositionAnimation>spCompositionAnimation;
            IFC_RETURN(m_tpFocusFollowerExpresssionAnimation.As(&spCompositionAnimation));
            IFC_RETURN(spCompositionAnimation.Get()->SetScalarParameter(wrl_wrappers::HStringReference(L"TransformedPosition").Get(), transformedPosition.X));

            IFC_RETURN(spCompObj.Get()->StartAnimation(wrl_wrappers::HStringReference(L"Translation.X").Get(), spCompositionAnimation.Get()));
        }


        //Done with the animation, now just set the width and height
        DOUBLE width = 0;
        DOUBLE height = 0;
        if (headerAsFE)
        {
            headerAsFE->get_ActualWidth(&width);
            headerAsFE->get_ActualHeight(&height);
        }

        IFC_RETURN(m_tpFocusFollower->put_Width(width));
        IFC_RETURN(m_tpFocusFollower->put_Height(height));

    }
    return S_OK;
}

_Check_return_ HRESULT
Pivot::SetSnappingBehavior(_In_ BOOLEAN single)
{
    HRESULT hr = S_OK;

    if (m_tpScrollViewer)
    {
        IFC(m_tpScrollViewer->put_HorizontalSnapPointsType(
            single ? xaml_controls::SnapPointsType_MandatorySingle : xaml_controls::SnapPointsType_Mandatory));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
Pivot::SetViewportEnabled(_In_ BOOLEAN enabled)
{
    HRESULT hr = S_OK;

    if (m_tpScrollViewer)
    {
        wrl::ComPtr<xaml::IUIElement> spScrollViewerAsUIE;
        IFC(m_tpScrollViewer.As(&spScrollViewerAsUIE));

        if (enabled)
        {
            IFC(m_tpScrollViewer->put_HorizontalScrollMode(
                xaml_controls::ScrollMode_Enabled));
        }
        else
        {
            BOOLEAN didCancel = FALSE;

            IFC(spScrollViewerAsUIE->CancelDirectManipulations(&didCancel));
            IFC(m_tpScrollViewer->put_HorizontalScrollMode(
                xaml_controls::ScrollMode_Disabled));
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
Pivot::StartFlyOutAnimation(_In_ DOUBLE from, _In_ DOUBLE headerOffset, _In_ bool toLeft)
{
#ifdef DBG
    bool isLoaded = false;
    IGNOREHR(IsLoaded(&isLoaded));
    ASSERT(isLoaded);
#endif //DBG

    return m_animator.AnimateOut(from, headerOffset, toLeft);
}

_Check_return_ HRESULT
Pivot::StartFlyInAnimation(_In_ DOUBLE to, _In_ bool fromLeft)
{
#ifdef DBG
    bool isLoaded = false;
    IGNOREHR(IsLoaded(&isLoaded));
    ASSERT(isLoaded);
#endif //DBG

    return m_animator.AnimateIn(to, fromLeft);
}

_Check_return_ HRESULT
Pivot::SetParallaxRelationship(double sectionOffset, double sectionWidth, float viewportSize)
{
    std::vector<double> headerWidths;

    if (m_tpScrollViewer)
    {
        if (!m_usingStaticHeaders && m_tpHeaderPanel && m_tpHeaderTransform)
        {
            IFC_RETURN(m_headerManager.ApplyParallax(
                m_tpScrollViewer.Get(),
                m_tpHeaderPanel.AsOrNull<xaml_controls::IPanel>().Get(),
                m_tpHeaderTransform.Get(),
                sectionOffset,
                sectionWidth,
                HasValidThresholdTemplate(),
                true /* isDynamicHeader */,
                viewportSize));
        }
        else if (m_usingStaticHeaders && m_tpStaticHeaderPanel && m_tpStaticHeaderTransform)
        {
            ASSERT(HasValidThresholdTemplate());

            IFC_RETURN(m_headerManager.ApplyParallax(
                m_tpScrollViewer.Get(),
                m_tpStaticHeaderPanel.AsOrNull<xaml_controls::IPanel>().Get(),
                m_tpStaticHeaderTransform.Get(),
                sectionOffset,
                sectionWidth,
                true /* isHeaderPanelInsideLayoutElementTemplatePart */,
                false /* isDynamicHeader */,
                viewportSize));
        }

        if (m_tpLayoutElement && m_tpLayoutElementTransform)
        {
            IFC_RETURN(m_headerManager.ApplyStaticLayoutRelationship(
                m_tpScrollViewer.Get(),
                m_tpLayoutElement.Get(),
                m_tpLayoutElementTransform.Get(),
                sectionWidth));
        }

        if (m_tpItemsPresenter && m_tpItemsPanelCompositeTransform)
        {
            wrl::ComPtr<xaml::IUIElement> spItemsPresenterAsUE;
            IFC_RETURN(m_tpItemsPresenter.As<xaml::IUIElement>(&spItemsPresenterAsUE));
            IFC_RETURN(m_headerManager.ApplyInverseStaticLayoutRelationship(
                m_tpScrollViewer.Get(),
                spItemsPresenterAsUE.Get(),
                m_tpItemsPanelCompositeTransform.Get(),
                sectionWidth));
        }
    }


    IFC_RETURN(m_headerManager.GetHeaderWidths(&headerWidths));
    m_stateMachine.HeaderWidthsChangedEvent(headerWidths);

    return S_OK;
}


_Check_return_ HRESULT
Pivot::GetIsInDManipAnimation(_Out_ bool *isInDManipAnimation)
{
    *isInDManipAnimation = false;

    if (m_tpScrollViewer)
    {
        wrl::ComPtr<xaml_controls::IScrollViewerPrivate> spPrivateScrollViewer;
        BOOLEAN isInDirectManipulation;

        IFC_RETURN(m_tpScrollViewer.As(&spPrivateScrollViewer));
        IFC_RETURN(spPrivateScrollViewer->get_IsInActiveDirectManipulation(&isInDirectManipulation));

        *isInDManipAnimation = !!isInDirectManipulation;
    }

    return S_OK;
}

_Check_return_ HRESULT
Pivot::CancelDManipAnimations()
{
    if (m_tpScrollViewer)
    {
        BOOLEAN returnValue = FALSE;
        wrl::ComPtr<xaml::IUIElement> spScrollViewerAsUIE;

        IFC_RETURN(m_tpScrollViewer.As(&spScrollViewerAsUIE));
        IFC_RETURN(spScrollViewerAsUIE->CancelDirectManipulations(&returnValue));
    }

    return S_OK;
}

_Check_return_ HRESULT
Pivot::UpdateScrollViewerDragDirection(PivotAnimationDirection direction)
{
    wrl::ComPtr<wfc::IVector<IInspectable*>> spItemsAsVector;
    UINT itemCount = 0;
    IFC_RETURN(GetItems(&itemCount, &spItemsAsVector));

    if (itemCount == 2 && m_pivotDragDirection != direction)
    {
        INT selectedIndex = 0;
        IFC_RETURN(get_SelectedIndex(&selectedIndex));

        m_pivotDragDirection = direction;
        IFC_RETURN(UpdateVisibleContent(selectedIndex, direction));
    }

    return S_OK;
}

_Check_return_ HRESULT
Pivot::UpdateFocusFollower()
{
    INT selectedIndex = 0;
    IFC_RETURN(get_SelectedIndex(&selectedIndex));
    IFC_RETURN(UpdateFocusFollowerImpl(selectedIndex));

    return S_OK;
}

#pragma endregion

#pragma region AnimatorCallbacks
_Check_return_ HRESULT
Pivot::OnAnimationComplete()
{
    RRETURN(m_stateMachine.AnimationCompleteEvent());
}
#pragma endregion

#pragma region HeaderManagerCallbacks
_Check_return_ HRESULT
Pivot::OnHeaderItemTapped(
    _In_ INT32 idx,
    _In_ bool shouldPlaySound)
{
    BOOLEAN isLocked = FALSE;
    IFC_RETURN(get_IsLocked(&isLocked));

    if (!isLocked)
    {
        INT oldIdx = 0;
        IFC_RETURN(get_SelectedIndex(&oldIdx));

        IFC_RETURN(m_stateMachine.SelectedIndexChangedEvent(idx, TRUE /* isFromHeaderTap */ ));

        if (shouldPlaySound)
        {
            xaml::ElementSoundKind elementSoundKind = xaml::ElementSoundKind_MoveNext;

            if (idx < oldIdx)
            {
                elementSoundKind = xaml::ElementSoundKind_MovePrevious;
            }

            wrl::ComPtr<xaml::IDependencyObject> spThisAsDo;
            IFC_RETURN(QueryInterface(__uuidof(xaml::IDependencyObject), &spThisAsDo));
            IFC_RETURN(PlatformHelpers::RequestInteractionSoundForElement(elementSoundKind, spThisAsDo.Get()));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
Pivot::GetHeaderTemplate(_Outptr_result_maybenull_ xaml::IDataTemplate** ppTemplate)
{
    RRETURN(get_HeaderTemplate(ppTemplate));
}

_Check_return_ HRESULT
Pivot::OnHeaderPanelMeasure(float viewportSize)
{
    RRETURN(m_stateMachine.HeaderMeasureEvent(viewportSize));
}

#pragma endregion

#pragma region Event Firing Helpers
_Check_return_ HRESULT
Pivot::RaiseOnPivotItemLoaded(
    _In_ xaml_controls::IPivotItem* pItem)
{
    HRESULT hr = S_OK;
    wrl::ComPtr<xaml_controls::PivotItemEventArgs> spPivotItemEventArgs;

    IFC(wrl::MakeAndInitialize<xaml_controls::PivotItemEventArgs>(&spPivotItemEventArgs));
    IFC(spPivotItemEventArgs->put_Item(pItem));

    IFC(m_PivotItemLoadingEventSource.InvokeAll(
        this,
        spPivotItemEventArgs.Get()));
    IFC(m_PivotItemLoadedEventSource.InvokeAll(
        this,
        spPivotItemEventArgs.Get()));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
Pivot::RaiseOnPivotItemUnloaded(
    _In_ xaml_controls::IPivotItem* pItem)
{
    HRESULT hr = S_OK;
    wrl::ComPtr<xaml_controls::PivotItemEventArgs> spPivotItemEventArgs;

    IFC(wrl::MakeAndInitialize<xaml_controls::PivotItemEventArgs>(&spPivotItemEventArgs));
    IFC(spPivotItemEventArgs->put_Item(pItem));

    IFC(m_PivotItemUnloadingEventSource.InvokeAll(
        this,
        spPivotItemEventArgs.Get()));
    IFC(m_PivotItemUnloadedEventSource.InvokeAll(
        this,
        spPivotItemEventArgs.Get()));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
Pivot::RaiseOnSelectionChanged(
    _In_ IInspectable* pOldItem,
    _In_ IInspectable* pNewItem)
{
    HRESULT hr = S_OK;
    wrl::ComPtr<xaml_controls::ISelectionChangedEventArgsFactory> spSelectionChangedEventArgsFactory;
    wrl::ComPtr<xaml_controls::ISelectionChangedEventArgs> spSelectionChangedEventArgs;
    wrl::ComPtr<wfci_::Vector<IInspectable*>> spRemovedItems;
    wrl::ComPtr<wfci_::Vector<IInspectable*>> spAddedItems;
    wrl::ComPtr<IInspectable> spInner;
    wrl::ComPtr<IInspectable> spThisAsI;

    IFC(wfci_::Vector<IInspectable*>::Make(&spRemovedItems));
    IFC(wfci_::Vector<IInspectable*>::Make(&spAddedItems));

    if (pNewItem)
    {
        IFC(spAddedItems->Append(pNewItem));
    }
    if (pOldItem)
    {
        IFC(spRemovedItems->Append(pOldItem));
    }

    IFC(wf::GetActivationFactory(
            wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Controls_SelectionChangedEventArgs).Get(),
            &spSelectionChangedEventArgsFactory));
    IFC(spSelectionChangedEventArgsFactory->CreateInstanceWithRemovedItemsAndAddedItems(
           spRemovedItems.Get(),
           spAddedItems.Get(),
           nullptr,
           &spInner,
           &spSelectionChangedEventArgs));
    IFC(QueryInterface(__uuidof(IInspectable), &spThisAsI));
    IFC(m_SelectionChangedEventSource.InvokeAll(
             spThisAsI.Get(),
             spSelectionChangedEventArgs.Get()));

Cleanup:
    RRETURN(hr);
}
#pragma endregion

#pragma region Event Handlers
_Check_return_ HRESULT
Pivot::OnPropertyChanged(
    _In_ xaml::IDependencyPropertyChangedEventArgs* pArgs)
{
    IFCPTR_RETURN(pArgs);

    wrl::ComPtr<xaml::IDependencyProperty> spPropertyInfo;
    IFC_RETURN(pArgs->get_Property(&spPropertyInfo));

    if (spPropertyInfo.Get() == PivotFactory::s_SelectedIndexProperty)
    {
        IFC_RETURN(OnSelectedIndexChanged(pArgs));
    }
    else if (spPropertyInfo.Get() == PivotFactory::s_SelectedItemProperty)
    {
        IFC_RETURN(OnSelectedItemChanged(pArgs));
    }
    else if (spPropertyInfo.Get() == PivotFactory::s_IsLockedProperty)
    {
        IFC_RETURN(OnIsLockedChanged(pArgs));
    }
    else if (spPropertyInfo.Get() == PivotFactory::s_HeaderTemplateProperty)
    {
        IFC_RETURN(m_headerManager.HeaderTemplateChangedEvent());
    }
    else if (spPropertyInfo.Get() == PivotFactory::s_TitleProperty)
    {
        IFC_RETURN(UpdateTitleControlVisibility());
    }
    else if (spPropertyInfo.Get() == PivotFactory::s_HeaderFocusVisualPlacementProperty)
    {
        IFC_RETURN(OnHeaderFocusVisualPlacementChanged());
    }
    else if (spPropertyInfo.Get() == PivotFactory::s_IsHeaderItemsCarouselEnabledProperty)
    {
        IFC_RETURN(OnIsHeaderItemsCarouselEnabledChanged());
    }

    return S_OK;
}

_Check_return_ HRESULT
Pivot::OnSelectedIndexChanged(
    _In_ xaml::IDependencyPropertyChangedEventArgs* pArgs)
{
    wrl::ComPtr<IInspectable> spValueAsInsp;
    wrl::ComPtr<wf::IPropertyValue> spValue;
    INT32 oldIdx = 0;

    IFC_RETURN(pArgs->get_OldValue(&spValueAsInsp));
    IFC_RETURN(spValueAsInsp.As(&spValue));
    IFC_RETURN(spValue->GetInt32(&oldIdx));

    if (!m_fIndexChangeReentryGuard)
    {
        // Setting the guard here prevents the state machine callback from setting this
        // property again (see SetSelectedIndex for more information).
        m_fIndexChangeReentryGuard = TRUE;

        INT32 newIdx = 0;

        IFC_RETURN(pArgs->get_NewValue(&spValueAsInsp));
        IFC_RETURN(spValueAsInsp.As(&spValue));
        IFC_RETURN(spValue->GetInt32(&newIdx));

        BOOLEAN isValid = TRUE;
        UINT itemCount = 0;
        IFC_RETURN(GetItems(&itemCount, nullptr));

        if (itemCount > 0)
        {
            IFC_RETURN(ValidateSelectedIndex(newIdx, &isValid));
        }

        if (isValid)
        {
            bool isLoaded = false;

            IFC_RETURN(IsLoaded(&isLoaded));
            IFC_RETURN(m_stateMachine.SelectedIndexChangedEvent(newIdx, FALSE /*isFromHeaderTap*/, !isLoaded /*skipAnimation*/));
            m_fIndexChangeReentryGuard = FALSE;
        }
        else
        {
            IFC_RETURN(put_SelectedIndex(oldIdx));
            m_fIndexChangeReentryGuard = FALSE;
            // TODO: Return a more appropriate error code here.
            IFC_RETURN(E_FAIL);
        }
    }

    // Once we're updating visuals, we'll want to check to see whether
    // the previously selected index contains the currently focused element,
    // and move it to the newly selected index if so.
    // At that time, we'll have no way to actually get at the previous selected
    // index unless we save it away, so we'll do that now.
    m_previousSelectedIndex = oldIdx;

    IFC_RETURN(UpdateVisualStates());

    return S_OK;
}

_Check_return_ HRESULT
Pivot::OnSelectedItemChanged(
    _In_ xaml::IDependencyPropertyChangedEventArgs* pArgs)
{
    HRESULT hr = S_OK;

    if (!m_fItemChangeReentryGuard)
    {
        m_fItemChangeReentryGuard = TRUE;

        wrl::ComPtr<IInspectable> spNewValueAsInsp;
        wrl::ComPtr<IInspectable> spOldValueAsInsp;

        IFC(pArgs->get_NewValue(&spNewValueAsInsp));
        IFC(pArgs->get_OldValue(&spOldValueAsInsp));

        INT32 itemIdx = 0;
        BOOLEAN isValid = TRUE;
        UINT itemCount = 0;
        IFC(GetItems(&itemCount, nullptr));

        if (itemCount > 0)
        {
            IFC(ValidateSelectedItem(spNewValueAsInsp.Get(), &itemIdx, &isValid));
        }

        if (isValid)
        {
            bool isLoaded = false;

            // Temporarily copy this to m_tpOldItem for use
            // in firing the SelectionChanged event.
            ASSERT(!m_tpOldItem);
            IFC(SetPtrValue(m_tpOldItem, spOldValueAsInsp.Get()));
            IFC(IsLoaded(&isLoaded));
            IFC(m_stateMachine.SelectedIndexChangedEvent(itemIdx, FALSE /*isFromHeaderTap*/, !isLoaded /*skipAnimation*/));
            m_fItemChangeReentryGuard = FALSE;
            m_tpOldItem.Clear();
        }
        else
        {
            IFC(put_SelectedItem(spOldValueAsInsp.Get()));
            m_fItemChangeReentryGuard = FALSE;
            // TODO: Return a more appropriate error code here.
            IFC(E_FAIL);
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
Pivot::OnIsLockedChanged(_In_ xaml::IDependencyPropertyChangedEventArgs* pArgs)
{
    HRESULT hr = S_OK;

    BOOLEAN isLocked = FALSE;

    wrl::ComPtr<IInspectable> spNewValueAsInsp;
    wrl::ComPtr<wf::IPropertyValue> spValue;
    IFC(pArgs->get_NewValue(&spNewValueAsInsp));
    IFC(spNewValueAsInsp.As(&spValue));
    IFC(spValue->GetBoolean(&isLocked));

    IFC(m_headerManager.IsLockedChangedEvent(isLocked));
    IFC(m_stateMachine.IsLockedChangedEvent(isLocked));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
Pivot::ValidateSelectedIndex(_In_ INT32 newIdx, _Out_ BOOLEAN* isValid)
{
    HRESULT hr = S_OK;
    UINT itemCount = 0;

    *isValid = FALSE;

    IFC(GetItems(&itemCount, nullptr));

    *isValid = (itemCount > 0 && newIdx >= 0 && newIdx < static_cast<INT32>(itemCount));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
Pivot::ValidateSelectedItem(_In_ IInspectable* pNewItem, _Out_ INT32* itemIdx, _Out_ BOOLEAN* isValid)
{
    HRESULT hr = S_OK;

    wrl::ComPtr<wfc::IVector<IInspectable*>> spItemsAsVector;
    UINT idx = 0;
    UINT itemCount = 0;

    *isValid = FALSE;
    *itemIdx = 0;

    IFC(GetItems(&itemCount, &spItemsAsVector));

    if (itemCount > 0)
    {
        IFC(spItemsAsVector->IndexOf(pNewItem, &idx, isValid));

        if (*isValid)
        {
            *itemIdx = static_cast<INT32>(idx);
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
Pivot::SyncItemToIndex(_In_ INT32 newIdx)
{
    HRESULT hr = S_OK;

    wrl::ComPtr<wfc::IVector<IInspectable*>> spItemsAsVector;
    wrl::ComPtr<IInspectable> spItem;
    UINT itemCount = 0;

    IFC(GetItems(&itemCount, &spItemsAsVector));

    // Purposely set SelectedItem to nullptr if the
    // collection is empty.
    if (itemCount > 0)
    {
        IFC(spItemsAsVector->GetAt(static_cast<UINT>(newIdx), &spItem));
    }

    IFC(put_SelectedItem(spItem.Get()));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
Pivot::UpdateTitleControlVisibility()
{
    if (m_titleControl)
    {
        wrl::ComPtr<IUIElement> titleControlAsUI;
        wrl::ComPtr<xaml::IDataTemplate> titleTemplate;
        wrl::ComPtr<IInspectable> title;

        IFC_RETURN(m_titleControl.As(&titleControlAsUI));
        IFC_RETURN(get_TitleTemplate(&titleTemplate));
        IFC_RETURN(get_Title(&title));

        if (title || titleTemplate)
        {
            IFC_RETURN(titleControlAsUI->put_Visibility(xaml::Visibility_Visible));
        }
        else
        {
            IFC_RETURN(titleControlAsUI->put_Visibility(xaml::Visibility_Collapsed));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
Pivot::OnViewChanged(
    _In_ IInspectable* pSender,
    _In_ xaml_controls::IScrollViewerViewChangedEventArgs* pEventArgs)
{
    UNREFERENCED_PARAMETER(pSender);
    HRESULT hr = S_OK;

    BOOLEAN isIntermediate = FALSE;
    IFC(pEventArgs->get_IsIntermediate(&isIntermediate));

    if (!isIntermediate)
    {
        IFC(m_stateMachine.FinalViewEvent());
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
Pivot::OnViewChanging(
    _In_ IInspectable* pSender,
    _In_ xaml_controls::IScrollViewerViewChangingEventArgs* pEventArgs)
{
    UNREFERENCED_PARAMETER(pSender);
    HRESULT hr = S_OK;

    BOOLEAN isInertial = FALSE;
    DOUBLE nextOffset = 0.0;
    DOUBLE finalOffset = 0.0;
    DOUBLE sectionOffset = 0.0;

    wrl::ComPtr<xaml_controls::IScrollViewerView> spView;

    IFC(pEventArgs->get_IsInertial(&isInertial));
    IFC(pEventArgs->get_NextView(&spView));
    IFC(spView->get_HorizontalOffset(&nextOffset));
    IFC(pEventArgs->get_FinalView(&spView));
    IFC(spView->get_HorizontalOffset(&finalOffset));

    if (m_tpItemsPanelTranslateTransform)
    {
        IFC(m_tpItemsPanelTranslateTransform->get_X(&sectionOffset));
    }

    IFC(m_stateMachine.ViewChangedEvent(isInertial, nextOffset, finalOffset, sectionOffset));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
Pivot::OnScrollViewerLoaded(_In_ IInspectable* pSender, _In_ xaml::IRoutedEventArgs* pArgs)
{
    UNREFERENCED_PARAMETER(pSender);
    UNREFERENCED_PARAMETER(pArgs);

    // This handled an edge condition where ScrollViewer wouldn't
    // sync secondary content relationship information correctly.
    IFC_RETURN(m_headerManager.SyncParallax());
    return S_OK;
}

_Check_return_ HRESULT
Pivot::OnDirectManipulationStarted(
    _In_opt_ IInspectable* pUnused1,
    _In_opt_ IInspectable* pUnused2)
{
    UNREFERENCED_PARAMETER(pUnused1);
    UNREFERENCED_PARAMETER(pUnused2);
    IFC_RETURN(OnDirectManipulationWork(true));
    return S_OK;
}

_Check_return_ HRESULT
Pivot::OnDirectManipulationCompleted(
    _In_opt_ IInspectable* pUnused1,
    _In_opt_ IInspectable* pUnused2)
{
    UNREFERENCED_PARAMETER(pUnused1);
    UNREFERENCED_PARAMETER(pUnused2);
    IFC_RETURN(OnDirectManipulationWork(false));
    return S_OK;
}

_Check_return_ HRESULT
Pivot::OnDirectManipulationWork(bool dmanipInProgress)
{
    m_isDirectManipulationInProgress = dmanipInProgress;

    INT selectedIndex = 0;
    IFC_RETURN(get_SelectedIndex(&selectedIndex));
    IFC_RETURN(UpdateVisibleContent(selectedIndex, m_pivotDragDirection));

    return S_OK;
}

_Check_return_ HRESULT Pivot::GetDefaultHeaderFocusVisualPlacement(_Outptr_ IInspectable** ppValue)
{
    RRETURN(Private::ValueBoxer::CreateReference<PivotHeaderFocusVisualPlacement>(PivotHeaderFocusVisualPlacement_ItemHeaders, ppValue));
}

_Check_return_ HRESULT Pivot::OnHeaderFocusVisualPlacementChanged()
{
    if (m_tpFocusFollower)
    {
        xaml_controls::PivotHeaderFocusVisualPlacement focusVisualPlacement;
        IFC_RETURN(get_HeaderFocusVisualPlacement(&focusVisualPlacement));

        wrl::ComPtr<xaml_controls::IControlStatics> spIControlStatics;
        IFC_RETURN(wf::GetActivationFactory(
            wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Controls_Control).Get(),
            &spIControlStatics));

        if (spIControlStatics)
        {
            if (focusVisualPlacement == PivotHeaderFocusVisualPlacement_SelectedItemHeader)
            {
                // For SelectedItemHeader, we use the FocusFollower as the template focus target
                spIControlStatics->SetIsTemplateFocusTarget(m_tpFocusFollower.Get(), true);
                UpdateFocusFollower();
            }
            else
            {
                // For ItemHeaders we restore default focus visual placement
                spIControlStatics->SetIsTemplateFocusTarget(m_tpFocusFollower.Get(), false);
            }
        }
    }
    return S_OK;
}

_Check_return_ HRESULT Pivot::OnIsHeaderItemsCarouselEnabledChanged()
{
    BOOLEAN isHeaderItemsCarouselEnabled;
    IFC_RETURN(get_IsHeaderItemsCarouselEnabled(&isHeaderItemsCarouselEnabled));
    m_isHeaderItemsCarouselEnabled = !!isHeaderItemsCarouselEnabled;

    // Let the state machine know about the IsHeaderItemsCarouselEnabled property value
    // change because we need to toggle the ScrollViewer extent between constrained (when
    // IsHeaderItemsCarouselEnabled is false) and unconstrained (when IsHeaderItemsCarouselEnabled
    // is true).
    IFC_RETURN(m_stateMachine.IsHeaderItemsCarouselEnabledChangedEvent());

    // We need to manually invalidate measure on the pivot panel because it's inside
    // a ScrollViewer and a simple change to the available size on the pivot will not
    // necessarily invalidate measure on it.
    if (m_tpPanel)
    {
        IFC_RETURN(static_cast<xaml_primitives::PivotPanel*>(m_tpPanel.Get())->InvalidateMeasure());
    }

    IFC_RETURN(InvalidateMeasure());

    return S_OK;
}

_Check_return_ HRESULT Pivot::OnHeaderGotFocus(_In_ IInspectable* /*sender*/, _In_ xaml::IRoutedEventArgs* /*args*/)
{
    // We should clear the cached bounding rectangle if we're just getting focus,
    // since it may have changed since we last had focus.
    wrl::ComPtr<xaml::IRectHelperStatics> spRectHelperStatics;

    IFC_RETURN(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_RectHelper).Get(),
        &spRectHelperStatics));

    IFC_RETURN(spRectHelperStatics->get_Empty(&m_cachedSelectedItemHeaderBoundingRectangle));

    IFC_RETURN(UpdateHeaderManagerFocusState());

    IFC_RETURN(AutomationFocusSelectedItem());

    return S_OK;
}

_Check_return_ HRESULT Pivot::OnHeaderLostFocus(_In_ IInspectable* /*sender*/, _In_ xaml::IRoutedEventArgs* /*args*/)
{
    return UpdateHeaderManagerFocusState();
}

_Check_return_ HRESULT Pivot::UpdateHeaderManagerFocusState()
{
    xaml_controls::PivotHeaderFocusVisualPlacement focusVisualPlacement;
    IFC_RETURN(get_HeaderFocusVisualPlacement(&focusVisualPlacement));

    if (focusVisualPlacement == PivotHeaderFocusVisualPlacement_SelectedItemHeader)
    {
        wrl::ComPtr<xaml::IUIElement> headerClipperAsElement;
        IGNOREHR(m_tpHeaderClipper.As(&headerClipperAsElement));
        if (headerClipperAsElement)
        {
            xaml::FocusState focusState;
            IFC_RETURN(headerClipperAsElement->get_FocusState(&focusState));
            IFC_RETURN(m_headerManager.FocusStateChangedEvent(focusState == xaml::FocusState::FocusState_Keyboard));
        }
    }
    else
    {
        IFC_RETURN(m_headerManager.FocusStateChangedEvent(false));

    }

    return S_OK;
}

void Pivot::PhysicalPixelsToDips(_In_ wf::Rect* physicalPixels, _Out_ wf::Rect* dipPixels)
{
    DOUBLE scale = 0;
    IFCFAILFAST(GetZoomScale(&scale));

    dipPixels->X = (float)(physicalPixels->X / scale);
    dipPixels->Y = (float)(physicalPixels->Y / scale);
    dipPixels->Width = (float)(physicalPixels->Width / scale);
    dipPixels->Height = (float)(physicalPixels->Height / scale);
}

_Check_return_ HRESULT Pivot::OnHeaderKeyDown(_In_ IInspectable* /*sender*/, _In_ xaml_input::IKeyRoutedEventArgs* args)
{
    BOOLEAN handled;
    IFC_RETURN(args->get_Handled(&handled));
    if (handled)
    {
        return S_OK;
    }

    wsy::VirtualKey pressedKey;
    wsy::VirtualKey originalKey;
    IFC_RETURN(args->get_Key(&pressedKey));
    IFC_RETURN(args->get_OriginalKey(&originalKey));

    wrl::ComPtr<xaml::IFrameworkElement> spThisAsFE;
    IFC_RETURN(QueryInterface(__uuidof(xaml::IFrameworkElement), &spThisAsFE));

    xaml::FlowDirection flowDirection = xaml::FlowDirection_LeftToRight;
    IFC_RETURN(spThisAsFE->get_FlowDirection(&flowDirection));

    // Switch Left/Right keys when in RTL:
    if (flowDirection == xaml::FlowDirection_RightToLeft)
    {
        if (pressedKey == wsy::VirtualKey_Left)
        {
            pressedKey = wsy::VirtualKey_Right;
        }
        else if (pressedKey == wsy::VirtualKey_Right)
        {
            pressedKey = wsy::VirtualKey_Left;
        }
    }

    bool shouldWrap = !XboxUtility::IsGamepadNavigationDirection(originalKey) && ShouldWrap();

    switch (pressedKey)
    {
        case wsy::VirtualKey_Left:
            {
                bool selectedIndexMoved = false;
                IFC_RETURN(MoveToPreviousItem(shouldWrap, &selectedIndexMoved));

                // We'll only mark this as handled if we actually moved the selected index.
                // Otherwise, leaving this un-marked will cause us to perform auto-focus
                // within OnLayoutElementKeyDown(), which we want to happen.
                IFC_RETURN(args->put_Handled(selectedIndexMoved));
            }
            break;

        case wsy::VirtualKey_Right:
            {
                bool selectedIndexMoved = false;
                IFC_RETURN(MoveToNextItem(shouldWrap, &selectedIndexMoved));

                // We'll only mark this as handled if we actually moved the selected index.
                // Otherwise, leaving this un-marked will cause us to perform auto-focus
                // within OnLayoutElementKeyDown(), which we want to happen.
                IFC_RETURN(args->put_Handled(selectedIndexMoved));
            }
            break;

        case wsy::VirtualKey_Home:
            {
                bool selectedIndexMoved = false;
                IFC_RETURN(MoveToFirstItem(&selectedIndexMoved));

                // We'll only mark this as handled if we actually moved the selected index.
                IFC_RETURN(args->put_Handled(selectedIndexMoved));
            }
            break;

        case wsy::VirtualKey_End:
            {
                bool selectedIndexMoved = false;
                IFC_RETURN(MoveToLastItem(&selectedIndexMoved));

                // We'll only mark this as handled if we actually moved the selected index.
                IFC_RETURN(args->put_Handled(selectedIndexMoved));
            }
            break;

        case wsy::VirtualKey_Down:
            if (XboxUtility::IsGamepadNavigationDown(originalKey))
            {
                // When navigating down from the header, we want to use just the selected header's bounds
                // when looking up for a candidate with auto-focus. This avoids skipping a bunch of elements
                // because something below has a better shadow. Do this only when Pivot has SelectedItemHeader placement
                bool fallbackToDefaultAutoFocusBehavior = true;
                xaml_controls::PivotHeaderFocusVisualPlacement headerFocusVisualPlacement = xaml_controls::PivotHeaderFocusVisualPlacement_ItemHeaders;

                IFC_RETURN(get_HeaderFocusVisualPlacement(&headerFocusVisualPlacement));
                if (headerFocusVisualPlacement == xaml_controls::PivotHeaderFocusVisualPlacement_SelectedItemHeader)
                {
                    wrl::ComPtr<IInspectable> spCandidate;
                    xaml_input::FocusNavigationDirection gamepadDirection = XboxUtility::GetNavigationDirection(originalKey);
                    wrl::ComPtr<xaml_input::IFocusManagerStaticsPrivate> spFocusManager;
                    INT selectedIndex = 0;
                    wf::Rect boundingRectangle = { 0, 0, 0, 0 };

                    // get the selected header bounding rectangle as the hint rect to ask auto-focus
                    IFC_RETURN(get_SelectedIndex(&selectedIndex));
                    IFC_RETURN(GetItemHeaderBoundingRectangle(selectedIndex, &boundingRectangle));

                    IFC_RETURN(wf::GetActivationFactory(
                        wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Input_FocusManager).Get(),
                        &spFocusManager));

                    // XYFocus requires that any rects be in DIPS
                    PhysicalPixelsToDips(&boundingRectangle, &boundingRectangle);

                    ASSERT(spFocusManager);
                    IFC_RETURN(spFocusManager->FindNextFocusWithSearchRootIgnoreEngagementWithHintRect(gamepadDirection,
                        static_cast<IPivot*>(this), /* searchRoot */
                        boundingRectangle, /* hint Rect */
                        boundingRectangle, /* exclusion Rect */
                        spCandidate.GetAddressOf()));

                    if (spCandidate)
                    {
                        // we got a candidate
                        wrl::ComPtr<xaml::IDependencyObject> spCandidateDO;
                        BOOLEAN focusUpdated = FALSE;

                        // Set Focused element
                        IFC_RETURN(spCandidate.As(&spCandidateDO));
                        IFC_RETURN(spFocusManager->SetFocusedElementWithDirection(
                            spCandidateDO.Get(),
                            xaml::FocusState::FocusState_Keyboard,
                            TRUE /* animateIfBringIntoView */,
                            FALSE /* forceBringIntoView */,
                            xaml_input::FocusNavigationDirection::FocusNavigationDirection_None,
                            TRUE /* requestActivation */,
                            &focusUpdated));
                        fallbackToDefaultAutoFocusBehavior = false;
                    }
                }

                if (fallbackToDefaultAutoFocusBehavior)
                {
                    // fallback for compat
                    IFC_RETURN(PerformAutoFocus(xaml_input::FocusNavigationDirection_Down));
                }

                IFC_RETURN(args->put_Handled(true));
            }
            break;
    }

    return S_OK;
}

_Check_return_ HRESULT Pivot::OnLayoutElementKeyDown(_In_ IInspectable* sender, _In_ xaml_input::IKeyRoutedEventArgs* args)
{
    UNREFERENCED_PARAMETER(sender);

    wsy::VirtualKey key;
    IFC_RETURN(args->get_Key(&key));

    switch (key)
    {
        case wsy::VirtualKey_Control:
        case wsy::VirtualKey_Shift:
        case wsy::VirtualKey_PageUp:
        case wsy::VirtualKey_PageDown:
        case wsy::VirtualKey_GamepadLeftShoulder:
        case wsy::VirtualKey_GamepadRightShoulder:
            IFC_RETURN(OnKeyDownImpl(args));
            IFC_RETURN(args->put_Handled(true));
            break;
        case wsy::VirtualKey_Left:
        case wsy::VirtualKey_Right:
        case wsy::VirtualKey_Home:
        case wsy::VirtualKey_End:
            {
                wsy::VirtualKey originalKey;
                BOOLEAN wasHandled = FALSE;

                IFC_RETURN(args->get_OriginalKey(&originalKey));

                IFC_RETURN(args->get_Handled(&wasHandled));

                // If we haven't already handled this key press, and if it's a gamepad navigation, then
                // then we want to auto-focus off of the Pivot.
                // We have to do this ourselves instead of just letting the key press bubble up to get
                // this behavior by default, since there's a ScrollViewer between the header panel
                // and the visual root that we don't want handling this key press.
                if (!wasHandled &&
                    (XboxUtility::IsGamepadNavigationLeft(originalKey) ||
                     XboxUtility::IsGamepadNavigationRight(originalKey)))
                {
                    IFC_RETURN(PerformAutoFocus(
                        XboxUtility::IsGamepadNavigationLeft(originalKey) ?
                            xaml_input::FocusNavigationDirection_Left :
                            xaml_input::FocusNavigationDirection_Right));
                }

                // We'll now mark the key press as handled to make sure the ScrollViewer doesn't handle it.
                IFC_RETURN(args->put_Handled(true));
            }
            break;
    }

    return S_OK;
}

_Check_return_ HRESULT
Pivot::OnUnloaded(_In_ IInspectable* pSender, _In_ xaml::IRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;

    UNREFERENCED_PARAMETER(pSender);
    UNREFERENCED_PARAMETER(pArgs);

    // When this control is unloaded we complete all animations to
    // ensure that the animation complete callback doesn't attempt
    // to modify state.
    IFC(m_animator.Complete());
    IFC(m_headerManager.UnloadedEvent());

Cleanup:
    RRETURN(hr);
}


_Check_return_ HRESULT
Pivot::OnSizeChanged(_In_ IInspectable* pSender, _In_ xaml::ISizeChangedEventArgs* pArgs)
{
    UNREFERENCED_PARAMETER(pSender);
    UNREFERENCED_PARAMETER(pArgs);

    INT selectedIndex = 0;
    IFC_RETURN(get_SelectedIndex(&selectedIndex));
    IFC_RETURN(UpdateVisibleContent(selectedIndex, m_pivotDragDirection));

    return S_OK;
}

_Check_return_ HRESULT
Pivot::OnDisplayOrientationChanged(_In_ wgrd::IDisplayInformation* pSender, _In_ IInspectable* pArgs)
{
    UNREFERENCED_PARAMETER(pSender);
    UNREFERENCED_PARAMETER(pArgs);
    RRETURN(GoToOrientationState());
}

_Check_return_ HRESULT Pivot::OnPointerEnteredHeader(_In_ IInspectable* /*sender*/, _In_ xaml_input::IPointerRoutedEventArgs* args)
{
    bool isPointerTouch;
    IFC_RETURN(IsPointerTouchPointer(args, &isPointerTouch));
    if (!isPointerTouch)
    {
        m_isMouseOrPenPointerOverHeaders = true;
        IFC_RETURN(UpdateVisualStates());
    }
    return S_OK;
}

_Check_return_ HRESULT Pivot::OnPointerExitedHeader(_In_ IInspectable* sender, _In_ xaml_input::IPointerRoutedEventArgs* /*args*/)
{
    wrl::ComPtr<IInspectable> previousButtonAsII, nextButtonAsII;
    IFC_RETURN(m_tpPreviousButton.As(&previousButtonAsII));
    IFC_RETURN(m_tpNextButton.As(&nextButtonAsII));

    if (!(m_navigationButtonsState == NavigationButtonsStates::PreviousButtonVisible && sender == nextButtonAsII.Get()) &&
        !(m_navigationButtonsState == NavigationButtonsStates::NextButtonVisible && sender == previousButtonAsII.Get()))
    {
        m_isMouseOrPenPointerOverHeaders = false;
        IFC_RETURN(UpdateVisualStates());
    }
    else
    {
        // Pointer exited a navigation button that we just hid. No need to call UpdateVisualStates.
    }

    return S_OK;
}

_Check_return_ HRESULT Pivot::OnPreviousButtonClick(_In_ IInspectable* /*pSender*/, _In_ xaml::IRoutedEventArgs* /*pArgs*/)
{
    return MoveToPreviousItem(ShouldWrap());
}

_Check_return_ HRESULT Pivot::OnNextButtonClick(_In_ IInspectable* /*pSender*/, _In_ xaml::IRoutedEventArgs* /*pArgs*/)
{
    return MoveToNextItem(ShouldWrap());
}
#pragma endregion


_Check_return_ HRESULT
Pivot::InvalidateHeaderSecondaryContentRelationships()
{
    // Invalidate the header panels to ensure that the header panel apply the secondary content
    // relationship(SCR). In general, the secondary content relation is completed during the
    // header Arrange() but it can be skipped if the child->Arrange() is already done before but DM's
    // initialization container isn't completed yet by InitializeDirectManipulationContainer() API call.

    if (m_tpHeaderPanel)
    {
        wrl::ComPtr<xaml::IUIElement> spDynamicHeaderAsUE;
        IFC_RETURN(m_tpHeaderPanel.As(&spDynamicHeaderAsUE));
        IFC_RETURN(spDynamicHeaderAsUE->InvalidateArrange());
    }

    if (m_tpStaticHeaderPanel)
    {
        wrl::ComPtr<xaml::IUIElement> spStaticHeaderAsUE;
        IFC_RETURN(m_tpStaticHeaderPanel.As(&spStaticHeaderAsUE));
        IFC_RETURN(spStaticHeaderAsUE->InvalidateArrange());
    }

    return S_OK;
}

_Check_return_ HRESULT
Pivot::HeaderPanelIsKeyboardFocusable(_Out_ bool *isKeyboardFocusable)
{
    *isKeyboardFocusable = false;

    if (m_tpHeaderClipper)
    {
        wrl::ComPtr<xaml::IUIElement> headerClipperAsUIE;
        wrl::ComPtr<xaml_controls::IControl> headerClipperAsControl;

        IGNOREHR(m_tpHeaderClipper.As(&headerClipperAsUIE));
        IGNOREHR(m_tpHeaderClipper.As(&headerClipperAsControl));

        if (headerClipperAsUIE && headerClipperAsControl)
        {
            // We don't have direct access to CControl::IsFocusable(), and the header panel
            // has no automation peer associated with it, so we need to manually retrieve
            // the properties associated with keyboard focusability.
            // If the header panel is visible, is a tab stop, and is enabled, then
            // it's keyboard focusable.
            xaml::Visibility headerClipperVisibility = xaml::Visibility_Collapsed;
            BOOLEAN headerClipperIsTabStop = FALSE;
            BOOLEAN headerClipperIsEnabled = FALSE;

            IFC_RETURN(headerClipperAsUIE->get_Visibility(&headerClipperVisibility));
            IFC_RETURN(headerClipperAsUIE->get_IsTabStop(&headerClipperIsTabStop));
            IFC_RETURN(headerClipperAsControl->get_IsEnabled(&headerClipperIsEnabled));

            *isKeyboardFocusable =
                headerClipperVisibility == xaml::Visibility_Visible &&
                headerClipperIsTabStop &&
                headerClipperIsEnabled;
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
Pivot::GetItemHeaderBoundingRectangle(_In_ INT index, _Out_ wf::Rect *boundingRectangle)
{
    xaml::IUIElement* headerNoRef = nullptr;

    *boundingRectangle = {0, 0, 0, 0};

    IFC_RETURN(GetHeaderAt(index, &headerNoRef));

    if (headerNoRef)
    {
        wrl::ComPtr<xaml::IUIElement> header(headerNoRef);
        wrl::ComPtr<xaml_controls::IContentControlPrivate> headerAsContentControlPrivate;
        bool shouldUseStaticHeaders = false;
        INT selectedIndex = false;

        IGNOREHR(header.As(&headerAsContentControlPrivate));

        if (headerAsContentControlPrivate)
        {
            IFC_RETURN(headerAsContentControlPrivate->GetGlobalBounds(boundingRectangle));

            if (XamlOneCoreTransforms::IsEnabled())
            {
                // In OneCoreTransforms mode, GetGlobalBounds returns logical pixels so we must convert to RasterizedClient.
                FLOAT zoomScale;
                IFC_RETURN(headerAsContentControlPrivate->GetRasterizationScale(&zoomScale));
                boundingRectangle->X =  boundingRectangle->X * zoomScale;
                boundingRectangle->Y =  boundingRectangle->Y * zoomScale;
                boundingRectangle->Width =  boundingRectangle->Width * zoomScale;
                boundingRectangle->Height =  boundingRectangle->Height * zoomScale;
            }

            // If we're using dynamic headers, if we're asking for the bounding rectangle of the selected header,
            // and if we have a cached bounding rectangle from the previously selected header,
            // then we'll use the X/Y-coordinates of that rectangle, since the current header may be
            // animating its position right now, and will ultimately move to the position of the previous
            // selected header once the transition animation completes.
            IFC_RETURN(ShouldUseStaticHeaders(&shouldUseStaticHeaders));
            IFC_RETURN(get_SelectedIndex(&selectedIndex));

            if (!shouldUseStaticHeaders && index == selectedIndex)
            {
                wrl::ComPtr<xaml::IRectHelperStatics> spRectHelperStatics;
                BOOLEAN isCachedBoundingRectangleEmpty = FALSE;

                IFC_RETURN(wf::GetActivationFactory(
                    wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_RectHelper).Get(),
                    &spRectHelperStatics));

                IFC_RETURN(spRectHelperStatics->GetIsEmpty(m_cachedSelectedItemHeaderBoundingRectangle, &isCachedBoundingRectangleEmpty));

                if (!isCachedBoundingRectangleEmpty)
                {
                    boundingRectangle->X = m_cachedSelectedItemHeaderBoundingRectangle.X;
                    boundingRectangle->Y = m_cachedSelectedItemHeaderBoundingRectangle.Y;
                }
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
Pivot::GetItemHeaderClickablePoint(_In_ INT index, _Out_ wf::Point *clickablePoint)
{
    wf::Rect boundingRectangle = {0, 0, 0, 0};

    IFC_RETURN(GetItemHeaderBoundingRectangle(index, &boundingRectangle));

    *clickablePoint = { boundingRectangle.X + boundingRectangle.Width / 2, boundingRectangle.Y + boundingRectangle.Height / 2};

    return S_OK;
}

_Check_return_ HRESULT
Pivot::IsItemHeaderOffscreen(_In_ INT index, _Out_ bool* isOffscreen)
{
    wf::Rect headerPanelBoundingRectangle = {};

    *isOffscreen = true;

    IFC_RETURN(GetItemHeaderBoundingRectangle(index, &headerPanelBoundingRectangle));

    // The selected item header is offscreen if one of two things are true:
    // either its global bounding rectangle has a width or height of 0,
    // or its visibility is not Visible (i.e., it's not being displayed on screen at all).
    if (headerPanelBoundingRectangle.Width > 0 && headerPanelBoundingRectangle.Height > 0)
    {
        xaml::IUIElement* headerNoRef = nullptr;
        xaml::Visibility headerVisibility = xaml::Visibility_Collapsed;

        IFC_RETURN(GetHeaderAt(index, &headerNoRef));

        IFC_RETURN(headerNoRef->get_Visibility(&headerVisibility));

        *isOffscreen = headerVisibility != xaml::Visibility_Visible;
    }

    return S_OK;
}

_Check_return_ HRESULT
Pivot::HeaderPanelHasKeyboardFocus(_Out_ bool *hasKeyboardFocus)
{
    *hasKeyboardFocus = false;

    if (m_tpHeaderClipper)
    {
        wrl::ComPtr<IInspectable> focusedElement;
        wrl::ComPtr<IInspectable> headerClipperAsI;

        wrl::ComPtr<xaml::IUIElement> thisAsUIE;
        IFC_RETURN(this->QueryInterface(IID_PPV_ARGS(&thisAsUIE)));

        wrl::ComPtr<xaml::IXamlRoot> xamlRoot;
        IFC_RETURN(thisAsUIE->get_XamlRoot(&xamlRoot));

        if (xamlRoot)
        {
            wrl::ComPtr<xaml_input::IFocusManagerStatics> focusManager;
            IFC_RETURN(wf::GetActivationFactory(
                wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Input_FocusManager).Get(),
                &focusManager));

            IFC_RETURN(focusManager->GetFocusedElementWithRoot(xamlRoot.Get(), &focusedElement));
            IFC_RETURN(m_tpHeaderClipper.As(&headerClipperAsI));

            *hasKeyboardFocus = headerClipperAsI == focusedElement;
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
Pivot::SetFocusToItem(_In_ INT index)
{
    // If we want to set focus to a header item, we'll do that by setting focus to the header panel
    // and then setting automation focus to the item in question.
    IFC_RETURN(GiveFocusTo(m_tpHeaderClipper.Get(), xaml::FocusState_Programmatic));
    IFC_RETURN(AutomationFocusItem(index));

    return S_OK;
}

unsigned Pivot::GetPivotPanelMultiplierImpl() const
{
    // This method MUST return an even number unless, starting with Redstone,
    // IsHeaderItemsCarouselEnabled is false. In the latter case, we return the item count
    // so that the PivotPanel's desired width and, thus, the ScrollViewer's
    // horizontal extent is just enough to hold all the pivot items.
    // This, in turns, will disable wrapping (e.g. going from the first item to the last item by wrapping around and vice-versa).
    // With IsHeaderItemsCarouselEnabled set to true, the multiplier should be big enough that we never reach the ends.
    // Indeed, the pivot state machine will take care of normalizing and re-centering us at the middle of the extent
    // before we get to the edges.
    return m_isHeaderItemsCarouselEnabled ? xaml_controls::GetPivotPanelMultiplier() : m_itemCount;
}

_Check_return_ HRESULT Pivot::GetTitleControl(_Outptr_result_maybenull_ xaml_controls::IContentControl** titleControl)
{
    IFCPTR_RETURN(titleControl);
    *titleControl = nullptr;

    if (m_titleControl)
    {
        m_titleControl.CopyTo(titleControl);
    }

    return S_OK;
}

_Check_return_ HRESULT Pivot::GetLeftHeaderPresenter(_Outptr_result_maybenull_ xaml::IFrameworkElement** leftHeaderPresenter)
{
    IFCPTR_RETURN(leftHeaderPresenter);
    *leftHeaderPresenter = nullptr;

    if (m_tpLeftHeaderPresenter)
    {
        m_tpLeftHeaderPresenter.CopyTo(leftHeaderPresenter);
    }

    return S_OK;
}

_Check_return_ HRESULT Pivot::GetRightHeaderPresenter(_Outptr_result_maybenull_ xaml::IFrameworkElement** rightHeaderPresenter)
{
    IFCPTR_RETURN(rightHeaderPresenter);
    *rightHeaderPresenter = nullptr;

    if (m_tpRightHeaderPresenter)
    {
        m_tpRightHeaderPresenter.CopyTo(rightHeaderPresenter);
    }

    return S_OK;
}

_Check_return_ HRESULT Pivot::InitializeVisualStateInterfaces()
{
    HRESULT hr = S_OK;

    if (!m_spVSMStatics)
    {
        IFC(wf::GetActivationFactory(
            wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_VisualStateManager).Get(),
            &m_spVSMStatics));
    }

    if (!m_pThisAsControlNoRef)
    {
        wrl::ComPtr<xaml::Controls::IControl> spThisAsControl;
        IFC(QueryInterface(__uuidof(xaml::Controls::IControl), &spThisAsControl));

        // QueryInterface adds a reference, which we need to release,
        // since otherwise Pivot will be holding a reference to itself.
        // Allowing spThisAsControl to fall out of scope while still
        // holding a reference to the QI'd Pivot does this automatically.
        m_pThisAsControlNoRef = spThisAsControl.Get();
    }

Cleanup:
    return hr;
}

_Check_return_ HRESULT Pivot::MoveToPreviousItem(_In_ bool shouldWrap, _Out_opt_ bool *selectedIndexMoved)
{
    BOOLEAN isLocked = FALSE;

    if (selectedIndexMoved)
    {
        *selectedIndexMoved = false;
    }

    IFC_RETURN(get_IsLocked(&isLocked));

    if (!isLocked)
    {
        UINT itemCount = 0;
        IFC_RETURN(GetItems(&itemCount, nullptr));
        if (itemCount > 0)
        {
            INT oldIdx = 0;
            IFC_RETURN(get_SelectedIndex(&oldIdx));

            if (oldIdx > 0 || shouldWrap)
            {
                INT newIdx = (oldIdx == 0) ? itemCount - 1 : oldIdx - 1;
                IFC_RETURN(MoveToItem(newIdx, xaml::ElementSoundKind_MovePrevious, selectedIndexMoved));
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT Pivot::MoveToNextItem(_In_ bool shouldWrap, _Out_opt_ bool *selectedIndexMoved)
{
    BOOLEAN isLocked = FALSE;

    if (selectedIndexMoved)
    {
        *selectedIndexMoved = false;
    }

    IFC_RETURN(get_IsLocked(&isLocked));

    if (!isLocked)
    {
        UINT itemCount = 0;
        IFC_RETURN(GetItems(&itemCount, nullptr));
        if (itemCount > 0)
        {
            INT oldIdx = 0;
            IFC_RETURN(get_SelectedIndex(&oldIdx));

            if (static_cast<UINT>(oldIdx) < (itemCount - 1) || shouldWrap)
            {
                INT newIdx = (static_cast<UINT>(oldIdx) == (itemCount - 1)) ? 0 : oldIdx + 1;
                IFC_RETURN(MoveToItem(newIdx, xaml::ElementSoundKind_MoveNext, selectedIndexMoved));
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT Pivot::MoveToFirstItem(_Out_opt_ bool *selectedIndexMoved)
{
    BOOLEAN isLocked = FALSE;

    if (selectedIndexMoved)
    {
        *selectedIndexMoved = false;
    }

    IFC_RETURN(get_IsLocked(&isLocked));

    if (!isLocked)
    {
        UINT itemCount = 0;
        IFC_RETURN(GetItems(&itemCount, nullptr));
        if (itemCount > 0)
        {
            INT oldIdx = 0;
            IFC_RETURN(get_SelectedIndex(&oldIdx));

            if (oldIdx > 0)
            {
                IFC_RETURN(MoveToItem(0, xaml::ElementSoundKind_MovePrevious, nullptr /* selectedIndexMoved */));
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT Pivot::MoveToLastItem(_Out_opt_ bool *selectedIndexMoved)
{
    BOOLEAN isLocked = FALSE;

    if (selectedIndexMoved)
    {
        *selectedIndexMoved = false;
    }

    IFC_RETURN(get_IsLocked(&isLocked));

    if (!isLocked)
    {
        UINT itemCount = 0;
        IFC_RETURN(GetItems(&itemCount, nullptr));
        if (itemCount > 0)
        {
            INT oldIdx = 0;
            IFC_RETURN(get_SelectedIndex(&oldIdx));

            if (static_cast<UINT>(oldIdx) < (itemCount - 1))
            {
                IFC_RETURN(MoveToItem(static_cast<INT>(itemCount - 1), xaml::ElementSoundKind_MoveNext, nullptr /* selectedIndexMoved */));
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT Pivot::MoveToItem(_In_ INT newIndex, _In_ xaml::ElementSoundKind soundToPlay, _Out_opt_ bool *selectedIndexMoved)
{
    if (selectedIndexMoved)
    {
        *selectedIndexMoved = false;
    }

    BOOLEAN isValid = TRUE;
    IFC_RETURN(ValidateSelectedIndex(newIndex, &isValid));

    if (isValid)
    {
        IFC_RETURN(m_stateMachine.SelectedIndexChangedEvent(newIndex, FALSE /* isFromHeaderTap */));

        // Play movement sound:
        wrl::ComPtr<xaml::IDependencyObject> spThisAsDo;
        IFC_RETURN(QueryInterface(__uuidof(xaml::IDependencyObject), &spThisAsDo));
        IFC_RETURN(PlatformHelpers::RequestInteractionSoundForElement(soundToPlay, spThisAsDo.Get()));

        if (selectedIndexMoved)
        {
            *selectedIndexMoved = true;
        }
    }

    return S_OK;
}

bool Pivot::ShouldWrap() const
{
    // Starting with Redstone, we no longer wrap (on keyboard nav, navigation buttons click or pan/flick)
    // when using static headers.
    return !m_usingStaticHeaders;
}

_Check_return_ HRESULT
Pivot::ShouldUseStaticHeaders(_Out_ bool* result)
{
    const bool usingStaticHeaders = m_usingStaticHeaders;

    *result = false;

    if (m_tpHeaderPanel)
    {
        if (m_tpStaticHeaderPanel)
        {
            BOOLEAN isHeaderItemsCarouselEnabled;
            IFC_RETURN(get_IsHeaderItemsCarouselEnabled(&isHeaderItemsCarouselEnabled));

            if (isHeaderItemsCarouselEnabled == false)
            {
                // If Pivot.IsHeaderItemsCarouselEnabled is false, it doesn't matter if the headers
                // fit in the viewport or not, we are going to be using the static headers.
                // If they don't fit, we are going to scroll as the selection changes to make sure
                // the selected item is visible and that the length ratio to its left and right is
                // preserved.
                *result = true;
            }
            else
            {
                if (usingStaticHeaders &&
                    !static_cast<xaml_primitives::PivotHeaderPanel*>(m_tpStaticHeaderPanel.Get())->IsContentClipped())
                {
                    *result = true;
                }
                else if (
                    !usingStaticHeaders &&
                    !static_cast<xaml_primitives::PivotHeaderPanel*>(m_tpHeaderPanel.Get())->IsContentClipped())
                {
                    *result = true;
                }
            }
        }
    }
    else
    {
        *result = true;
    }

    return S_OK;
}

_Check_return_ HRESULT
Pivot::GetHeaderAt(_In_ INT index, _Outptr_result_maybenull_ xaml::IUIElement** headerNoRef)
{
    bool shouldUseStaticHeaders = false;
    wrl::ComPtr<xaml_primitives::IPivotHeaderPanel> currentHeaderPanel;

    *headerNoRef = nullptr;

    IFC_RETURN(ShouldUseStaticHeaders(&shouldUseStaticHeaders));

    currentHeaderPanel =
        shouldUseStaticHeaders ?
        m_tpStaticHeaderPanel.Get() :
        m_tpHeaderPanel.Get();

    if (currentHeaderPanel)
    {
        wrl::ComPtr<xaml_controls::IPanel> currentHeaderPanelAsPanel;
        wrl::ComPtr<wfc::IVector<xaml::UIElement*>> currentHeaderPanelChildren;

        IFC_RETURN(currentHeaderPanel.As(&currentHeaderPanelAsPanel));
        IFC_RETURN(currentHeaderPanelAsPanel->get_Children(&currentHeaderPanelChildren));

        if (currentHeaderPanelChildren)
        {
            unsigned int childrenCount = 0;
            wrl::ComPtr<xaml::IUIElement> header;

            IFC_RETURN(currentHeaderPanelChildren->get_Size(&childrenCount));

            // We get sometimes called from stale automation peers asking for invalid indices.
            if (index < static_cast<int>(childrenCount))
            {
                IFC_RETURN(currentHeaderPanelChildren->GetAt(index, &header));
                *headerNoRef = header.Get();
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
Pivot::InvalidateMeasure()
{
    wrl::ComPtr<xaml::IUIElement> pivotAsUE;
    IFC_RETURN(QueryInterface(__uuidof(xaml::IUIElement), &pivotAsUE));
    IFC_RETURN(pivotAsUE->InvalidateMeasure());
    return S_OK;
}

_Check_return_ HRESULT
Pivot::InvalidateArrange()
{
    wrl::ComPtr<xaml::IUIElement> pivotAsUE;
    IFC_RETURN(QueryInterface(__uuidof(xaml::IUIElement), &pivotAsUE));
    IFC_RETURN(pivotAsUE->InvalidateArrange());
    return S_OK;
}

_Check_return_ HRESULT
Pivot::IsPointerTouchPointer(
    _In_ xaml_input::IPointerRoutedEventArgs* pointerEventArgs,
    _Out_ bool* isPointerTouchPointer)
{
    *isPointerTouchPointer = false;

    wrl::ComPtr<xaml_input::IPointer> inputPointer;
    mui::PointerDeviceType pointerDeviceType;

    IFC_RETURN(pointerEventArgs->get_Pointer(&inputPointer));
    IFC_RETURN(inputPointer->get_PointerDeviceType(&pointerDeviceType));

    *isPointerTouchPointer = (pointerDeviceType == mui::PointerDeviceType_Touch);
    return S_OK;
}

#pragma region LayoutBoundsMargin Adjustment
_Check_return_ HRESULT
Pivot::GoToOrientationState()
{
    HRESULT hr = S_OK;

    const WCHAR* pNewStateName = nullptr;
    wgrd::DisplayOrientations orientation = wgrd::DisplayOrientations_Landscape;
    IFC(GetOrientation(&orientation));

    if (orientation == wgrd::DisplayOrientations_Landscape ||
        orientation == wgrd::DisplayOrientations_LandscapeFlipped)
    {
        pNewStateName = c_VisualStateLandscape;
    }
    else if (orientation == wgrd::DisplayOrientations_Portrait ||
        orientation == wgrd::DisplayOrientations_PortraitFlipped)
    {
        pNewStateName = c_VisualStatePortrait;
    }

    {
        wrl::ComPtr<xaml::IVisualStateManagerStatics> spVSMStatics;
        wrl::ComPtr<xaml::Controls::IControl> spThisAsControl;
        BOOLEAN succeeded = FALSE;

        IFC(QueryInterface(__uuidof(xaml::Controls::IControl), &spThisAsControl));
        IFC(wf::GetActivationFactory(
            wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_VisualStateManager).Get(),
            &spVSMStatics));

        IFC(spVSMStatics->GoToState(spThisAsControl.Get(),
            wrl_wrappers::HStringReference(pNewStateName).Get(),
            true, &succeeded));
    }

Cleanup:
    RRETURN(hr);
}

/* static */
_Check_return_ HRESULT
Pivot::GetOrientation(_Out_ wgrd::DisplayOrientations* pOrientation)
{
    HRESULT hr = S_OK;

    wrl::ComPtr<wgrd::IDisplayInformationStatics> spDisplayInformationStatics;
    wrl::ComPtr<wgrd::IDisplayInformation> spDisplayInformation;

    *pOrientation = wgrd::DisplayOrientations_Portrait;

    IGNOREHR(wf::GetActivationFactory(wrl_wrappers::HStringReference(
        RuntimeClass_Windows_Graphics_Display_DisplayInformation).Get(), &spDisplayInformationStatics));
    if (spDisplayInformationStatics)
    {
        IGNOREHR(spDisplayInformationStatics->GetForCurrentView(&spDisplayInformation));
        if (spDisplayInformation)
        {
            IFC(spDisplayInformation->get_CurrentOrientation(pOrientation));
        }
    }

Cleanup:
    RRETURN(hr);
}

#pragma endregion

#pragma region PivotAnimator

//
// The interaction between PivotAnimator and PivotStateMachine is complex enough that it probably merits
// a comment in this space for the sake of future maintenance.
//
// PivotAnimator is more or less a utility that performs animations at the behest of PivotStateMachine
// depending on what state the latter is in.  When PivotStateMachine detects that the user is intending
// to change the selected pivot item, it transitions to the fly-out state (InFlyOutInertial),
// which then causes us to animate out the current content.  Once that's done, it transitions to the fly-in state
// (InFlyInInertial), which causes us to animate in the new content.  Once *that's* done, then it returns to
// the idle state to await further user input.
//
// When we first transition to fly-out, the PivotStateMachine calls StartFlyOutAnimation, which causes Pivot
// to call AnimateOut on PivotAnimator to do the animation.  Once that animation completes, then OnCompleted
// is called, which calls back into PivotStateMachine to inform it that the animation has finished.
// It then transitions to the fly-in state and calls StartFlyInAnimation, which causes Pivot to call
// AnimateIn on PivotAnimator.  Once that animation completes, then OnCompleted is called again, which
// causes PivotStateMachine to transition to the idle state.
//
// Since PivotStateMachine controls all calls into and handles all calls out of PivotAnimator,
// we can just have a single OnComplete callback that all storyboards use, since PivotStateMachine
// knows which animation was running and which state to transition to when the animation it started completes.
//

const DOUBLE PivotAnimator::c_easingExponent = 4.0;

const UINT32 PivotAnimator::c_flyOutTranslationAnimationDuration = 83; // ms
const DOUBLE PivotAnimator::c_flyOutTranslationAnimationDistance = 7.0; // px
const UINT32 PivotAnimator::c_flyOutOpacityAnimationDuration = 67; // ms

const UINT32 PivotAnimator::c_flyInTranslationAnimationDuration = 767; // ms
const DOUBLE PivotAnimator::c_flyInTranslationAnimationDistance = 20.0; // px
const UINT32 PivotAnimator::c_flyInOpacityAnimationDuration = 333; // ms

PivotAnimator::PivotAnimator(IPivotAnimatorCallbacks* pCallbacks, Private::ReferenceTrackerHelper<Pivot> referenceTrackerHelper)
    : m_pCallbackPtr(pCallbacks)
    , m_referenceTrackerHelper(referenceTrackerHelper)
    , m_flyOutCompletedToken()
    , m_flyInCompletedToken()
{}

PivotAnimator::~PivotAnimator()
{
    ASSERT(m_flyOutCompletedToken.value == 0);
    ASSERT(m_flyInCompletedToken.value == 0);
}

_Check_return_ HRESULT
PivotAnimator::SetTargets(
    _In_opt_ xaml_media::ITranslateTransform* pTranslateTransform,
    _In_opt_ xaml_controls::IItemsPresenter* pItemsPresenter,
    _In_opt_ xaml_media::ICompositeTransform* pHeaderTransform)
{
    HRESULT hr = S_OK;

    m_tpFlyOutStoryboardRunning.Clear();
    m_tpFlyOutTranslationAnimation.Clear();
    m_tpFlyOutTranslationAnimationKeyFrameStart.Clear();
    m_tpFlyOutTranslationAnimationKeyFrameFinish.Clear();

    m_tpFlyOutHeaderStoryboardRunning.Clear();
    m_tpFlyOutHeaderTranslationAnimation.Clear();
    m_tpFlyOutHeaderTranslationAnimationKeyFrameStart.Clear();
    m_tpFlyOutHeaderTranslationAnimationKeyFrameFinish.Clear();

    m_tpFlyInStoryboardRunning.Clear();
    m_tpFlyInTranslationAnimation.Clear();
    m_tpFlyInTranslationAnimationKeyFrameStart.Clear();
    m_tpFlyInTranslationAnimationKeyFrameFinish.Clear();

    if (pTranslateTransform && pItemsPresenter)
    {
        wrl::ComPtr<xaml_media::ITranslateTransform> spTranslateTransform(pTranslateTransform);
        wrl::ComPtr<xaml_controls::IItemsPresenter> spItemsPresenter(pItemsPresenter);
        wrl::ComPtr<xaml::IDependencyObject> spTranslateTransformAsDO;
        wrl::ComPtr<xaml::IDependencyObject> spItemsPresenterAsDO;

        wrl::ComPtr<xaml_animation::IStoryboard> spFlyOutStoryboardRunning;
        wrl::ComPtr<wfc::IVector<xaml_animation::Timeline*>> spFlyOutChildren;
        wrl::ComPtr<xaml_animation::IDoubleAnimationUsingKeyFrames> spFlyOutTranslationAnimation;
        wrl::ComPtr<xaml_animation::ITimeline> spFlyOutTranslationAnimationAsTimeline;
        wrl::ComPtr<xaml_animation::IDoubleKeyFrame> spFlyOutTranslationAnimationKeyFrameStart;
        wrl::ComPtr<xaml_animation::IDoubleKeyFrame> spFlyOutTranslationAnimationKeyFrameFinish;
        wrl::ComPtr<xaml_animation::IDoubleAnimationUsingKeyFrames> spFlyOutOpacityAnimation;
        wrl::ComPtr<xaml_animation::ITimeline> spFlyOutOpacityAnimationAsTimeline;
        wrl::ComPtr<xaml_animation::IDoubleKeyFrame> spFlyOutOpacityAnimationKeyFrameStart;
        wrl::ComPtr<xaml_animation::IDoubleKeyFrame> spFlyOutOpacityAnimationKeyFrameFinish;

        wrl::ComPtr<xaml_animation::IStoryboard> spFlyOutHeaderStoryboardRunning;
        wrl::ComPtr<wfc::IVector<xaml_animation::Timeline*>> spFlyOutHeaderChildren;
        wrl::ComPtr<xaml_animation::IDoubleAnimationUsingKeyFrames> spFlyOutHeaderTranslationAnimation;
        wrl::ComPtr<xaml_animation::ITimeline> spFlyOutHeaderTranslationAnimationAsTimeline;
        wrl::ComPtr<xaml_animation::IDoubleKeyFrame> spFlyOutHeaderTranslationAnimationKeyFrameStart;
        wrl::ComPtr<xaml_animation::IDoubleKeyFrame> spFlyOutHeaderTranslationAnimationKeyFrameFinish;

        wrl::ComPtr<xaml_animation::IStoryboard> spFlyInStoryboardRunning;
        wrl::ComPtr<wfc::IVector<xaml_animation::Timeline*>> spFlyInChildren;
        wrl::ComPtr<xaml_animation::IDoubleAnimationUsingKeyFrames> spFlyInTranslationAnimation;
        wrl::ComPtr<xaml_animation::ITimeline> spFlyInTranslationAnimationAsTimeline;
        wrl::ComPtr<xaml_animation::IDoubleKeyFrame> spFlyInTranslationAnimationKeyFrameStart;
        wrl::ComPtr<xaml_animation::IDoubleKeyFrame> spFlyInTranslationAnimationKeyFrameFinish;
        wrl::ComPtr<xaml_animation::IDoubleAnimationUsingKeyFrames> spFlyInOpacityAnimation;
        wrl::ComPtr<xaml_animation::ITimeline> spFlyInOpacityAnimationAsTimeline;
        wrl::ComPtr<xaml_animation::IDoubleKeyFrame> spFlyInOpacityAnimationKeyFrameStart;
        wrl::ComPtr<xaml_animation::IDoubleKeyFrame> spFlyInOpacityAnimationKeyFrameFinish;

        IFC(spTranslateTransform.As(&spTranslateTransformAsDO));
        IFC(spItemsPresenter.As(&spItemsPresenterAsDO));

        IFC(wf::ActivateInstance(
                wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_Storyboard).Get(),
                &spFlyOutStoryboardRunning));

        IFC(spFlyOutStoryboardRunning->get_Children(&spFlyOutChildren));

        IFC(CreateAnimation(
            spTranslateTransformAsDO.Get(),
            wrl_wrappers::HStringReference(L"X").Get(),
            c_flyOutTranslationAnimationDuration,
            false /* useSpline */,
            &spFlyOutTranslationAnimation,
            &spFlyOutTranslationAnimationKeyFrameStart,
            &spFlyOutTranslationAnimationKeyFrameFinish));

        IFC(CreateAnimation(
            spItemsPresenterAsDO.Get(),
            wrl_wrappers::HStringReference(L"Opacity").Get(),
            c_flyOutOpacityAnimationDuration,
            false /* useSpline */,
            &spFlyOutOpacityAnimation,
            &spFlyOutOpacityAnimationKeyFrameStart,
            &spFlyOutOpacityAnimationKeyFrameFinish));

        IFC(spFlyOutOpacityAnimationKeyFrameStart->put_Value(1));
        IFC(spFlyOutOpacityAnimationKeyFrameFinish->put_Value(0));

        IFC(spFlyOutTranslationAnimation.As(&spFlyOutTranslationAnimationAsTimeline));
        IFC(spFlyOutOpacityAnimation.As(&spFlyOutOpacityAnimationAsTimeline));

        IFC(spFlyOutChildren->Append(spFlyOutTranslationAnimationAsTimeline.Get()));
        IFC(spFlyOutChildren->Append(spFlyOutOpacityAnimationAsTimeline.Get()));

        IFC(wf::ActivateInstance(
                wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_Storyboard).Get(),
                &spFlyInStoryboardRunning));

        IFC(spFlyInStoryboardRunning->get_Children(&spFlyInChildren));

        IFC(CreateAnimation(
            spTranslateTransformAsDO.Get(),
            wrl_wrappers::HStringReference(L"X").Get(),
            c_flyInTranslationAnimationDuration,
            true /* useSpline */,
            &spFlyInTranslationAnimation,
            &spFlyInTranslationAnimationKeyFrameStart,
            &spFlyInTranslationAnimationKeyFrameFinish));

        IFC(CreateAnimation(
            spItemsPresenterAsDO.Get(),
            wrl_wrappers::HStringReference(L"Opacity").Get(),
            c_flyInOpacityAnimationDuration,
            true /* useSpline */,
            &spFlyInOpacityAnimation,
            &spFlyInOpacityAnimationKeyFrameStart,
            &spFlyInOpacityAnimationKeyFrameFinish));

        IFC(spFlyInOpacityAnimationKeyFrameStart->put_Value(0));
        IFC(spFlyInOpacityAnimationKeyFrameFinish->put_Value(1));

        IFC(spFlyInTranslationAnimation.As(&spFlyInTranslationAnimationAsTimeline));
        IFC(spFlyInOpacityAnimation.As(&spFlyInOpacityAnimationAsTimeline));

        IFC(spFlyInChildren->Append(spFlyInTranslationAnimationAsTimeline.Get()));
        IFC(spFlyInChildren->Append(spFlyInOpacityAnimationAsTimeline.Get()));

        IFC(SetPtrValue(m_tpFlyOutStoryboardRunning, spFlyOutStoryboardRunning.Get()));
        IFC(SetPtrValue(m_tpFlyOutTranslationAnimation, spFlyOutTranslationAnimation.Get()));
        IFC(SetPtrValue(m_tpFlyOutTranslationAnimationKeyFrameStart, spFlyOutTranslationAnimationKeyFrameStart.Get()));
        IFC(SetPtrValue(m_tpFlyOutTranslationAnimationKeyFrameFinish, spFlyOutTranslationAnimationKeyFrameFinish.Get()));

        IFC(SetPtrValue(m_tpFlyInStoryboardRunning, spFlyInStoryboardRunning.Get()));
        IFC(SetPtrValue(m_tpFlyInTranslationAnimation, spFlyInTranslationAnimation.Get()));
        IFC(SetPtrValue(m_tpFlyInTranslationAnimationKeyFrameStart, spFlyInTranslationAnimationKeyFrameStart.Get()));
        IFC(SetPtrValue(m_tpFlyInTranslationAnimationKeyFrameFinish, spFlyInTranslationAnimationKeyFrameFinish.Get()));

        if (pHeaderTransform)
        {
            wrl::ComPtr<xaml_media::ICompositeTransform> spHeaderTransform(pHeaderTransform);
            wrl::ComPtr<xaml::IDependencyObject> spHeaderTransformAsDO;

            IFC(spHeaderTransform.As(&spHeaderTransformAsDO));

            IFC(wf::ActivateInstance(
                    wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_Storyboard).Get(),
                    &spFlyOutHeaderStoryboardRunning));

            IFC(spFlyOutHeaderStoryboardRunning->get_Children(&spFlyOutHeaderChildren));

            IFC(CreateAnimation(
                spHeaderTransformAsDO.Get(),
                wrl_wrappers::HStringReference(L"TranslateX").Get(),
                c_flyOutTranslationAnimationDuration + c_flyInTranslationAnimationDuration,
                true /* useSpline */,
                &spFlyOutHeaderTranslationAnimation,
                &spFlyOutHeaderTranslationAnimationKeyFrameStart,
                &spFlyOutHeaderTranslationAnimationKeyFrameFinish));

            IFC(spFlyOutHeaderTranslationAnimation.As(&spFlyOutHeaderTranslationAnimationAsTimeline));

            IFC(spFlyOutHeaderChildren->Append(spFlyOutHeaderTranslationAnimationAsTimeline.Get()));

            IFC(SetPtrValue(m_tpFlyOutHeaderStoryboardRunning, spFlyOutHeaderStoryboardRunning.Get()));
            IFC(SetPtrValue(m_tpFlyOutHeaderTranslationAnimation, spFlyOutHeaderTranslationAnimation.Get()));
            IFC(SetPtrValue(m_tpFlyOutHeaderTranslationAnimationKeyFrameStart, spFlyOutHeaderTranslationAnimationKeyFrameStart.Get()));
            IFC(SetPtrValue(m_tpFlyOutHeaderTranslationAnimationKeyFrameFinish, spFlyOutHeaderTranslationAnimationKeyFrameFinish.Get()));
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
PivotAnimator::AnimateOut(_In_ DOUBLE startOffset, _In_ DOUBLE headerOffset, _In_ bool toLeft)
{
    HRESULT hr = S_OK;

    IFC(Stop());

    if (m_tpFlyOutTranslationAnimation)
    {
        IFC(m_tpFlyOutTranslationAnimationKeyFrameStart->put_Value(startOffset));
        IFC(m_tpFlyOutTranslationAnimationKeyFrameFinish->put_Value(startOffset + (toLeft ? -c_flyOutTranslationAnimationDistance : c_flyOutTranslationAnimationDistance)));
    }

    if (headerOffset > 0 && m_tpFlyOutHeaderTranslationAnimation)
    {
        IFC(m_tpFlyOutHeaderTranslationAnimationKeyFrameStart->put_Value(0));
        IFC(m_tpFlyOutHeaderTranslationAnimationKeyFrameFinish->put_Value(toLeft ? -headerOffset : headerOffset));
    }

    if (m_tpFlyOutStoryboardRunning)
    {
        if (m_flyOutCompletedToken.value == 0)
        {
            wrl::ComPtr<xaml_animation::ITimeline> spStoryboardAsTL;
            IFC(m_tpFlyOutStoryboardRunning.As(&spStoryboardAsTL));
            IFC(spStoryboardAsTL->add_Completed(
                wrl::Callback<wf::IEventHandler<IInspectable*>>(
                    this, &PivotAnimator::OnCompleted).Get(),
                &m_flyOutCompletedToken));
        }

        IFC(m_tpFlyOutStoryboardRunning->Begin());
    }

    if (headerOffset > 0 && m_tpFlyOutHeaderStoryboardRunning)
    {
        IFC(m_tpFlyOutHeaderStoryboardRunning->Begin());
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
PivotAnimator::AnimateIn(_In_ DOUBLE targetOffset, _In_ bool fromLeft)
{
    HRESULT hr = S_OK;

    IFC(Stop());

    if (m_tpFlyInTranslationAnimation)
    {
        IFC(m_tpFlyInTranslationAnimationKeyFrameStart->put_Value(targetOffset + (fromLeft ? -c_flyInTranslationAnimationDistance : c_flyInTranslationAnimationDistance)));
        IFC(m_tpFlyInTranslationAnimationKeyFrameFinish->put_Value(targetOffset));
    }

    if (m_tpFlyInStoryboardRunning)
    {
        if (m_flyInCompletedToken.value == 0)
        {
            wrl::ComPtr<xaml_animation::ITimeline> spStoryboardAsTL;
            IFC(m_tpFlyInStoryboardRunning.As(&spStoryboardAsTL));
            IFC(spStoryboardAsTL->add_Completed(
                wrl::Callback<wf::IEventHandler<IInspectable*>>(
                    this, &PivotAnimator::OnCompleted).Get(),
                    &m_flyInCompletedToken));
        }

        IFC(m_tpFlyInStoryboardRunning->Begin());
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
PivotAnimator::Stop()
{
    HRESULT hr = S_OK;

    if (m_tpFlyOutStoryboardRunning)
    {
        IFC(m_tpFlyOutStoryboardRunning->Stop());
    }

    if (m_tpFlyOutHeaderStoryboardRunning)
    {
        IFC(m_tpFlyOutHeaderStoryboardRunning->Stop());
    }

    if (m_tpFlyInStoryboardRunning)
    {
        IFC(m_tpFlyInStoryboardRunning->Stop());
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
PivotAnimator::Complete()
{
    HRESULT hr = S_OK;

    if (m_tpFlyOutStoryboardRunning)
    {
        IFC(m_tpFlyOutStoryboardRunning->SkipToFill());

        // We unsubscribe here. Part of the reason we complete an animation early
        // is to prevent the animation's Completed callback from calling into a
        // destructed Pivot. We remove the event handler and add it back if
        // Pivot is later animated again.
        if (m_flyOutCompletedToken.value != 0)
        {
            wrl::ComPtr<xaml_animation::ITimeline> spStoryboardAsTL;
            IFC(m_tpFlyOutStoryboardRunning.As(&spStoryboardAsTL));
            IFC(spStoryboardAsTL->remove_Completed(
                m_flyOutCompletedToken));
            m_flyOutCompletedToken.value = 0;
        }
    }

    if (m_tpFlyOutHeaderStoryboardRunning)
    {
        IFC(m_tpFlyOutHeaderStoryboardRunning->SkipToFill());
    }

    if (m_tpFlyInStoryboardRunning)
    {

        IFC(m_tpFlyInStoryboardRunning->SkipToFill());

        // We unsubscribe here. Part of the reason we complete an animation early
        // is to prevent the animation's Completed callback from calling into a
        // destructed Pivot. We remove the event handler and add it back if
        // Pivot is later animated again.
        if (m_flyInCompletedToken.value != 0)
        {
            wrl::ComPtr<xaml_animation::ITimeline> spStoryboardAsTL;
            IFC(m_tpFlyInStoryboardRunning.As(&spStoryboardAsTL));
            IFC(spStoryboardAsTL->remove_Completed(
                m_flyInCompletedToken));
            m_flyInCompletedToken.value = 0;
        }
    }

    ASSERT(m_flyOutCompletedToken.value == 0);
    ASSERT(m_flyInCompletedToken.value == 0);

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
PivotAnimator::OnCompleted(
    _In_opt_ IInspectable* pUnused1,
    _In_opt_ IInspectable* pUnused2)
{
    HRESULT hr = S_OK;

    UNREFERENCED_PARAMETER(pUnused1);
    UNREFERENCED_PARAMETER(pUnused2);

    IFC(m_pCallbackPtr->OnAnimationComplete());

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
PivotAnimator::CreateAnimation(
    _In_ xaml::IDependencyObject* pTarget,
    _In_ HSTRING targetProperty,
    _In_ unsigned int duration,
    _In_ bool useSpline,
    _Out_ xaml_animation::IDoubleAnimationUsingKeyFrames **ppAnimation,
    _Out_ xaml_animation::IDoubleKeyFrame **ppAnimationKeyFrameStart,
    _Out_ xaml_animation::IDoubleKeyFrame **ppAnimationKeyFrameFinish)
{
    wrl::ComPtr<xaml_animation::IDoubleAnimationUsingKeyFrames> spAnimation;
    wrl::ComPtr<xaml_animation::ITimeline> spAnimationAsTL;
    wrl::ComPtr<xaml_animation::IStoryboardStatics> spStoryboardStatics;
    wrl::ComPtr<wfc::IVector<xaml_animation::DoubleKeyFrame*>> spAnimationKeyFrames;
    wrl::ComPtr<xaml_animation::IDiscreteDoubleKeyFrame> spAnimationKeyFrameStart;
    wrl::ComPtr<xaml_animation::IDoubleKeyFrame> spAnimationKeyFrameStartAsDoubleKeyFrame;
    wf::TimeSpan animationKeyFrameStartTimeSpan;
    xaml_animation::KeyTime animationKeyFrameStartKeyTime;
    wrl::ComPtr<xaml_animation::IDoubleKeyFrame> spAnimationKeyFrameFinishAsDoubleKeyFrame;
    wf::TimeSpan animationKeyFrameFinishTimeSpan;
    xaml_animation::KeyTime animationKeyFrameFinishKeyTime;

    wrl::ComPtr<wuv::IUISettings> uiSettings;
    IFC_RETURN(wf::ActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Windows_UI_ViewManagement_UISettings).Get(), &uiSettings));
    boolean animationsEnabled = false;
    IFC_RETURN(uiSettings->get_AnimationsEnabled(&animationsEnabled));

    IFC_RETURN(wf::ActivateInstance(
          wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_DoubleAnimationUsingKeyFrames).Get(),
          &spAnimation));

    IFC_RETURN(wf::GetActivationFactory(
          wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_Storyboard).Get(),
          &spStoryboardStatics));

    IFC_RETURN(spAnimation.As(&spAnimationAsTL));

    IFC_RETURN(spStoryboardStatics->SetTargetProperty(spAnimationAsTL.Get(), targetProperty));
    IFC_RETURN(spStoryboardStatics->SetTarget(spAnimationAsTL.Get(), pTarget));

    IFC_RETURN(spAnimation->get_KeyFrames(&spAnimationKeyFrames));

    // Start it with a discrete double key frame to give it an initial value.
    IFC_RETURN(wf::ActivateInstance(
          wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_DiscreteDoubleKeyFrame).Get(),
          &spAnimationKeyFrameStart));

    IFC_RETURN(spAnimationKeyFrameStart.As(&spAnimationKeyFrameStartAsDoubleKeyFrame));

    animationKeyFrameStartTimeSpan.Duration = 0;
    animationKeyFrameStartKeyTime.TimeSpan = animationKeyFrameStartTimeSpan;

    IFC_RETURN(spAnimationKeyFrameStartAsDoubleKeyFrame->put_KeyTime(animationKeyFrameStartKeyTime));

    IFC_RETURN(spAnimationKeyFrames->Append(spAnimationKeyFrameStartAsDoubleKeyFrame.Get()));

    if (useSpline)
    {
        // Give it a spline double key frame to rapidly bring it into view, if we want that.
        wrl::ComPtr<xaml_animation::ISplineDoubleKeyFrame> spAnimationKeyFrameFinish;
        wrl::ComPtr<xaml_animation::IKeySpline> spAnimationKeyFrameKeySpline;
        wf::Point animationKeyFrameSplineControlPoint1 = { 0.1f, 0.9f };
        wf::Point animationKeyFrameSplineControlPoint2 = { 0.2f, 1.0f };

        IFC_RETURN(wf::ActivateInstance(
              wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_SplineDoubleKeyFrame).Get(),
              &spAnimationKeyFrameFinish));

        IFC_RETURN(wf::ActivateInstance(
              wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_KeySpline).Get(),
              &spAnimationKeyFrameKeySpline));

        IFC_RETURN(spAnimationKeyFrameKeySpline->put_ControlPoint1(animationKeyFrameSplineControlPoint1));
        IFC_RETURN(spAnimationKeyFrameKeySpline->put_ControlPoint2(animationKeyFrameSplineControlPoint2));

        IFC_RETURN(spAnimationKeyFrameFinish->put_KeySpline(spAnimationKeyFrameKeySpline.Get()));

        IFC_RETURN(spAnimationKeyFrameFinish.As(&spAnimationKeyFrameFinishAsDoubleKeyFrame));
    }
    else
    {
        // Otherwise, make it linear.
        wrl::ComPtr<xaml_animation::ILinearDoubleKeyFrame> spAnimationKeyFrameFinish;

        IFC_RETURN(wf::ActivateInstance(
              wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_LinearDoubleKeyFrame).Get(),
              &spAnimationKeyFrameFinish));

        IFC_RETURN(spAnimationKeyFrameFinish.As(&spAnimationKeyFrameFinishAsDoubleKeyFrame));
    }

    animationKeyFrameFinishTimeSpan.Duration = animationsEnabled ? duration * 10000 : 0; // ms to ticks conversion
    animationKeyFrameFinishKeyTime.TimeSpan = animationKeyFrameFinishTimeSpan;

    IFC_RETURN(spAnimationKeyFrameFinishAsDoubleKeyFrame->put_KeyTime(animationKeyFrameFinishKeyTime));

    IFC_RETURN(spAnimationKeyFrames->Append(spAnimationKeyFrameFinishAsDoubleKeyFrame.Get()));

    *ppAnimation = spAnimation.Detach();
    *ppAnimationKeyFrameStart = spAnimationKeyFrameStartAsDoubleKeyFrame.Detach();
    *ppAnimationKeyFrameFinish = spAnimationKeyFrameFinishAsDoubleKeyFrame.Detach();

    return S_OK;
}
#pragma endregion

} } } } XAML_ABI_NAMESPACE_END

