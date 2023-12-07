// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Page.g.h"
#include "AppBar.g.h"
#include "JoltClasses.h"
#include "AutomationPeer.g.h"
#include "Frame.g.h"
#include "focusmgr.h"
#include "NavigationHelpers.h"
#include "Window_Partial.h"
#include <DependencyLocator.h>
#include <RuntimeEnabledFeatures.h>
#include "InitialFocusSIPSuspender.h"
#include "ApplicationBarService.g.h"
#include <DesktopUtility.h>
#include <XamlOneCoreTransforms.h>
#include "XamlRoot.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;
using namespace RuntimeFeatureBehavior;
using namespace std::placeholders;

namespace N = xaml::Navigation;

// Work around disruptive max/min macros
#undef max
#undef min

#define PageApplyingLayoutBoundsTolerance (0.1f)

Page::Page()
    : m_shouldRegisterNewAppbars(FALSE)
    , m_mostRecentLayoutBounds()
{
}

Page::~Page()
{
    if (const auto xamlRoot = XamlRoot::GetImplementationForElementStatic(this))
    {
        xamlRoot->GetLayoutBoundsHelperNoRef()->RemoveLayoutBoundsChangedCallback(&m_tokLayoutBoundsChanged);
    }
}

_Check_return_ HRESULT
Page::PrepareState()
{
    HRESULT hr = S_OK;
    EventRegistrationToken loadedToken;
    EventRegistrationToken unloadedToken;
    ctl::ComPtr<xaml::IRoutedEventHandler> spLoadedEventHandler;
    ctl::ComPtr<xaml::IRoutedEventHandler> spUnloadedEventHandler;

    IFC(PageGenerated::PrepareState());

    spLoadedEventHandler.Attach(
        new ClassMemberEventHandler<
            Page,
            IPage,
            xaml::IRoutedEventHandler,
            IInspectable,
            xaml::IRoutedEventArgs>(this, &Page::OnLoaded, true /* subscribingToSelf */ ));

    IFC(add_Loaded(spLoadedEventHandler.Get(), &loadedToken));

    spUnloadedEventHandler.Attach(
        new ClassMemberEventHandler<
        Page,
        IPage,
        xaml::IRoutedEventHandler,
        IInspectable,
        xaml::IRoutedEventArgs>(this, &Page::OnUnloaded, true /* subscribingToSelf */ ));

    IFC(add_Unloaded(spUnloadedEventHandler.Get(), &unloadedToken));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT Page::OnPropertyChanged2(_In_ const PropertyChangedParams& args)
{
    HRESULT hr = S_OK;

    // Ensure that this is not a disallowed property if this Page element's content is a SwapChainBackgroundPanel element.
    IFC(CoreImports::Page_ValidatePropertyIfSwapChainBackgroundPanelChild(
        static_cast<CCoreServices*>(DXamlCore::GetCurrent()->GetHandle()),
        static_cast<CUIElement*>(this->GetHandle()),
        args.m_pDP->GetIndex()
        ));

    AppBarMode newAppBarMode = AppBarMode_Top;
    switch (args.m_pDP->GetIndex())
    {
        case KnownPropertyIndex::Page_BottomAppBar:
            newAppBarMode = AppBarMode_Bottom;
        case KnownPropertyIndex::Page_TopAppBar:
            {
                ctl::ComPtr<xaml_controls::IAppBar> inspectable;
                ctl::ComPtr<AppBar> oldAppBar;
                ctl::ComPtr<AppBar> newAppBar;
                ctl::ComPtr<IApplicationBarService> applicationBarService;
                ctl::ComPtr<IInspectable> dataContext;
                double oldClosedHeight = 0.0; // how much space is consumed in the page content layout
                double newClosedHeight = 0.0;

                // first, grab the new AppBar Value and carry out important stateful operations
                if (args.m_pNewValue)
                {
                    IFC(CValueBoxer::UnboxObjectValue(args.m_pNewValue, args.m_pDP->GetPropertyType(), &inspectable));
                    IFC(inspectable.As(&newAppBar));
                    
                    if (newAppBar)
                    {
                        IFC(GetAppBarClosedHeight(newAppBar.Get(), &newClosedHeight));
                        newAppBar->SetMode(newAppBarMode);
                    }
                }

                // XamlRoot may not yet be available if called before OnLoaded. Nothing further to do.
                auto xamlRoot = XamlRoot::GetImplementationForElementStatic(this);
                if (!xamlRoot)
                {
                    break;
                }

                // unregister the old app bar
                IFC(xamlRoot->GetApplicationBarService(applicationBarService));
                ASSERT(applicationBarService);
                if (args.m_pOldValue)
                {
                    inspectable.Reset();
                    IFC(CValueBoxer::UnboxObjectValue(args.m_pOldValue, args.m_pDP->GetPropertyType(), &inspectable));
                    IFC(inspectable.As(&oldAppBar));
                    if (oldAppBar)
                    {
                        IFC(GetAppBarClosedHeight(oldAppBar.Get(), &oldClosedHeight));

                        IFC(applicationBarService->UnregisterApplicationBar(oldAppBar.Get()));
                        IFC(oldAppBar->SetOwner(nullptr));

                        // let the appbar know what type it is
                        oldAppBar->SetMode(AppBarMode_Inline);
                    }
                }

                // register the new app bar
                if (newAppBar && m_shouldRegisterNewAppbars)
                {
                    IFC(newAppBar->SetOwner(this));
                    IFC(applicationBarService->RegisterApplicationBar(newAppBar.Get(), newAppBarMode));

                    // Forward the data context to the new app bar only when we're on the live tree
                    // The DC is not guaranteed correct unless the Page is on the live tree for any other bindings
                    // we can save some time while building the tree
                    // Once the page enters the tree its DataContext will be propagated down, including to the AppBars
                    IFC(get_DataContext(&dataContext));
                    IFC(newAppBar->put_DataContext(dataContext.Get()));
                }

                if (abs(newClosedHeight - oldClosedHeight) > PageApplyingLayoutBoundsTolerance)
                {
                    IFC(InvalidateLayoutForAppBarSizeChange());
                }

                break;
            }

        case KnownPropertyIndex::Page_NavigationCacheMode:
            xaml_controls::IFrame* pIFrame = NULL;
            Frame* pFrame = NULL;
            IFC(get_Frame(&pIFrame));
            if (pIFrame)
            {
                pFrame = static_cast<Frame *>(pIFrame);
                N::NavigationCacheMode navigationCacheMode = N::NavigationCacheMode_Disabled;

                IFC(get_NavigationCacheMode(&navigationCacheMode));
                // Remove the page from Cache if the NavigationCacheMode is set to Diabled.
                // We dont handle the transition from Disabled to Enabled/Required since we
                // do not have any scenarios that need it. The Caching (if NavigationCacheMode
                // is Enabled/Required) that is done as a part navigation when content is loaded
                // covers all the scenarios.
                if (navigationCacheMode == N::NavigationCacheMode_Disabled)
                {
                    // If there is more than one page of the same type (descriptor) in the PageStack,
                    // it will be uncached even if one of the pages disables the CacheMode, even if the
                    // other pages of the same type have the CacheMode set to Enabled/Required.
                    // This is because content is cached per type and any number of pages with the
                    // same type will have only one entry in the Cache.
                    IFC(pFrame->RemovePageFromCache(m_descriptor.Get()));
                }
            }
            ReleaseInterface(pIFrame);
            break;
    }

    IFC(PageGenerated::OnPropertyChanged2(args));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT Page::get_TopAppBarImpl(_Outptr_ xaml_controls::IAppBar** pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::Page_TopAppBar, pValue));
}

_Check_return_ HRESULT Page::put_TopAppBarImpl(_In_ xaml_controls::IAppBar* value)
{
    HRESULT hr = S_OK;

    IFC(SetValueByKnownIndex(KnownPropertyIndex::Page_TopAppBar, value));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT Page::OnNavigatedFromImpl(
    _In_ xaml::Navigation::INavigationEventArgs *pArgs)
{
    HRESULT hr = S_OK;

    IFC(CheckThread());

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT Page::OnNavigatedToImpl(
    _In_ xaml::Navigation::INavigationEventArgs *pArgs)
{
    HRESULT hr = S_OK;

    IFC(CheckThread());

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT Page::OnNavigatingFromImpl(
    _In_ xaml::Navigation::INavigatingCancelEventArgs *pArgs)
{
    HRESULT hr = S_OK;

    IFC(CheckThread());

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
Page::InvokeOnNavigatedFrom(
    _In_ IInspectable *pContentIInspectable,
    _In_opt_ IInspectable *pParameterIInspectable,
    _In_opt_ xaml_animation::INavigationTransitionInfo *pTransitionInfo,
    _In_ HSTRING descriptor,
    _In_ xaml::Navigation::NavigationMode navigationMode)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<INavigationEventArgs> spINavigationEventArgs;

    IFCPTR(descriptor);
    IFCPTR(pContentIInspectable);

    IFC(NavigationHelpers::CreateINavigationEventArgs(pContentIInspectable, pParameterIInspectable, pTransitionInfo, descriptor, navigationMode, &spINavigationEventArgs));
    IFC(OnNavigatedFromProtected(spINavigationEventArgs.Get()));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
Page::InvokeOnNavigatedTo(
    _In_ IInspectable *pContentIInspectable,
    _In_opt_ IInspectable *pParameterIInspectable,
    _In_opt_ xaml_animation::INavigationTransitionInfo *pTransitionInfo,
    _In_ HSTRING descriptor,
    _In_ xaml::Navigation::NavigationMode navigationMode)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<INavigationEventArgs> spINavigationEventArgs;

    IFCPTR(descriptor);
    IFCPTR(pContentIInspectable);

    IFC(NavigationHelpers::CreateINavigationEventArgs(pContentIInspectable, pParameterIInspectable, pTransitionInfo, descriptor, navigationMode, &spINavigationEventArgs));
    IFC(OnNavigatedToProtected(spINavigationEventArgs.Get()));


    // Set Automation Page Navigation complete event.
    if (DXamlCore::GetCurrent()->HasPageNavigationCompleteEvent())
    {
        IFC(DXamlCore::GetCurrent()->SetPageNavigationCompleteEvent());
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
Page::InvokeOnNavigatingFrom(
    _In_opt_ IInspectable *pParameterIInspectable,
    _In_opt_ xaml_animation::INavigationTransitionInfo *pTransitionInfo,
    _In_ HSTRING descriptor,
    _In_ xaml::Navigation::NavigationMode navigationMode,
    _Out_ BOOLEAN *pIsCanceled)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<INavigatingCancelEventArgs> spINavigatingCancelEventArgs;

    IFCPTR(pIsCanceled);
    *pIsCanceled = FALSE;

    IFCPTR(descriptor);

    IFC(NavigationHelpers::CreateINavigatingCancelEventArgs(pParameterIInspectable, pTransitionInfo, descriptor, navigationMode, &spINavigatingCancelEventArgs));
    IFC(OnNavigatingFromProtected(spINavigatingCancelEventArgs.Get()));
    IFC(spINavigatingCancelEventArgs->get_Cancel(pIsCanceled));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
Page::SetDescriptor(
    _In_ HSTRING descriptor)
{
    HRESULT hr = S_OK;

    IFC(m_descriptor.Set(descriptor));

Cleanup:
    RRETURN(hr);
}

void Page::NotifyOfDataContextChange(_In_ const DataContextChangedParams& args)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IAppBar> spTopBar;
    ctl::ComPtr<IAppBar> spBottomBar;
    ctl::ComPtr<IInspectable> spNewDataContext;

    PageGenerated::NotifyOfDataContextChange(args);

    IFC(get_TopAppBar(&spTopBar));
    IFC(get_BottomAppBar(&spBottomBar));

    if (!spTopBar && !spBottomBar)
    {
        goto Cleanup;
    }

    // Push the data context to the appbar
    // TODO: We should be using the logical parent instead of this
    if (args.m_fResolvedNewDataContext)
    {
        IFC(args.GetNewDataContext(&spNewDataContext));
    }
    else
    {
        IFC(get_DataContext(&spNewDataContext));
    }
    if (spTopBar)
    {
        IFC(spTopBar.Cast<AppBar>()->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_DataContext, spNewDataContext.Get()));
    }
    if (spBottomBar)
    {
        IFC(spBottomBar.Cast<AppBar>()->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_DataContext, spNewDataContext.Get()));
    }

Cleanup:
    return;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Ensure that this Page element is root of visual tree, if its content is
//      a SwapChainBackgroundPanel element.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
Page::OnTreeParentUpdated(
    _In_opt_ CDependencyObject *pNewParent,
    BOOLEAN isParentAlive)
{
    HRESULT hr = S_OK;

    IFC(CoreImports::Page_EnsureIsRootVisualIfSwapChainBackgroundPanelChild(
            static_cast<CCoreServices*>(DXamlCore::GetCurrent()->GetHandle()),
            static_cast<CUIElement*>(this->GetHandle())
            ));

    IFC(PageGenerated::OnTreeParentUpdated(pNewParent, isParentAlive));

Cleanup:

    RRETURN(hr);
}

_Check_return_ HRESULT
Page::RegisterAppBars()
{
    ctl::ComPtr<IAppBar> topBar;
    IFC_RETURN(get_TopAppBar(&topBar));

    ctl::ComPtr<IAppBar> bottomBar;
    IFC_RETURN(get_BottomAppBar(&bottomBar));

    if (topBar || bottomBar)
    {
        ctl::ComPtr<IApplicationBarService> applicationBarService;

        if (const auto xamlRoot = XamlRoot::GetImplementationForElementStatic(this))
        {
            IFC_RETURN(xamlRoot->GetApplicationBarService(applicationBarService));
        }
        ASSERT(applicationBarService);

        if (topBar)
        {
            IFC_RETURN(topBar.Cast<AppBar>()->SetOwner(this));
            IFC_RETURN(applicationBarService->RegisterApplicationBar(topBar.Cast<AppBar>(), AppBarMode_Top));
        }

        if (bottomBar)
        {
            IFC_RETURN(bottomBar.Cast<AppBar>()->SetOwner(this));
            IFC_RETURN(applicationBarService->RegisterApplicationBar(bottomBar.Cast<AppBar>(), AppBarMode_Bottom));
        }
    }

    m_shouldRegisterNewAppbars = TRUE;

    return S_OK;
}

_Check_return_ HRESULT
 Page::UnregisterAppBars()
{
    ctl::ComPtr<IAppBar> topBar;
    ctl::ComPtr<IAppBar> bottomBar;

    IFC_RETURN(get_TopAppBar(&topBar));
    IFC_RETURN(get_BottomAppBar(&bottomBar));

    if (topBar || bottomBar)
    {
        ctl::ComPtr<IApplicationBarService> applicationBarService;

        if (const auto xamlRoot = XamlRoot::GetImplementationForElementStatic(this))
        {
            IFC_RETURN(xamlRoot->GetApplicationBarService(applicationBarService));
        }
        ASSERT(applicationBarService);

        if (topBar)
        {
            IFC_RETURN(applicationBarService->UnregisterApplicationBar(topBar.Cast<AppBar>()));
            IFC_RETURN(topBar.Cast<AppBar>()->SetOwner(nullptr));
        }
        if (bottomBar)
        {
            IFC_RETURN(applicationBarService->UnregisterApplicationBar(bottomBar.Cast<AppBar>()));
            IFC_RETURN(bottomBar.Cast<AppBar>()->SetOwner(nullptr));
        }
    }

    m_shouldRegisterNewAppbars = FALSE;
    return S_OK;
}


_Check_return_ HRESULT
 Page::OnLoaded(
    _In_ IInspectable* pSender,
    _In_ IRoutedEventArgs* pArgs)
{
    BOOLEAN bAutomationListener = FALSE;
    ctl::ComPtr<DependencyObject> spCurrentFocusedElement;
    ctl::ComPtr<DependencyObject> spFirstFocusableElementDO;
    xref_ptr<CDependencyObject> spFirstFocusableElementCDO;

    BOOLEAN focusUpdated = FALSE;

    IFC_RETURN(GetFocusedElement(&spCurrentFocusedElement));

    IFC_RETURN(CoreImports::Page_ValidatePropertiesIfSwapChainBackgroundPanelChild(
        static_cast<CCoreServices*>(DXamlCore::GetCurrent()->GetHandle()),
        static_cast<CUIElement*>(GetHandle())
        ));

    const bool setDefaultFocus = VisualTree::GetFocusManagerForElement(GetHandle())->IsPluginFocused();

    if (setDefaultFocus && !spCurrentFocusedElement)
    {
        // Set the focus on the first focusable control
        IFC_RETURN(CoreImports::FocusManager_GetFirstFocusableElement(
            this->GetHandle(),
            spFirstFocusableElementCDO.ReleaseAndGetAddressOf()));

        if (spFirstFocusableElementCDO != nullptr)
        {
            IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(spFirstFocusableElementCDO, &spFirstFocusableElementDO));
            CFocusManager *pFocusManager = VisualTree::GetFocusManagerForElement(GetHandle());

            InitialFocusSIPSuspender setInitalFocusTrue(pFocusManager);
            IFC_RETURN(DependencyObject::SetFocusedElement(
                spFirstFocusableElementDO.Get(),
                xaml::FocusState_Programmatic,
                FALSE /*animateIfBringIntoView*/,
                &focusUpdated));
        }

        if (spFirstFocusableElementCDO == nullptr)
        {
            // Narrator listens for focus changed events to determine when the UI Automation tree needs refreshed. If we don't set default focus (on Phone) or if we fail to find a focusable element,
            // we will need notify the narror of the UIA tree change when page is loaded.
            IFC_RETURN(AutomationPeer::ListenerExistsHelper(xaml_automation_peers::AutomationEvents_AutomationFocusChanged, &bAutomationListener));

            if (bAutomationListener)
            {
                IFC_RETURN(CoreImports::AutomationRaiseFocusChangedOnUIAWindow(DXamlCore::GetCurrent()->GetHandle(), this->GetHandle()));
            }
        }
    }

    // Page is the only UserControl that renders its Background.
    static_cast<CUserControl*>(this->GetHandle())->SetAllowForBackgroundRender(true);

    return S_OK;
}

_Check_return_ HRESULT
Page::OnUnloaded(
    _In_ IInspectable* pSender,
    _In_ IRoutedEventArgs* pArgs)
{
    if (const auto xamlRoot = XamlRoot::GetImplementationForElementStatic(this))
    {
        xamlRoot->GetLayoutBoundsHelperNoRef()->RemoveLayoutBoundsChangedCallback(&m_tokLayoutBoundsChanged);
    }

    return S_OK;
}

IFACEMETHODIMP
Page::MeasureOverride(
    _In_ wf::Size availableSize,
    _Out_ wf::Size* pReturnValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml::IUIElement> spChild;

    IFC(get_Content(&spChild));

    if (spChild)
    {
        wf::Rect availableBounds = { 0, 0, availableSize.Width, availableSize.Height };

        // Get the new available bounds that can also applied the core window's layout bounds if the
        // current page is the same size of the full core window size.
        IFC(CalculateUpdatedBounds(&availableBounds));

        availableSize.Width = availableBounds.Width;
        availableSize.Height = availableBounds.Height;

        IFC(spChild->Measure(availableSize));
        IFC(spChild->get_DesiredSize(pReturnValue));
    }
    else
    {
        pReturnValue->Height = pReturnValue->Width = 0.0f;
    }

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP
Page::ArrangeOverride(
    _In_ wf::Size arrangeSize,
    _Out_ wf::Size* pReturnValue)
{
    HRESULT hr = S_OK;

    ctl::ComPtr<xaml::IUIElement> spChild;

    IFC(get_Content(&spChild));

    if (spChild)
    {
        wf::Rect arrangeBounds = { 0, 0, arrangeSize.Width, arrangeSize.Height };

        // Get the new arranged bounds that applied the core window's layout bounds if the current
        // page is the same size of the full core window size.
        IFC(CalculateUpdatedBounds(&arrangeBounds));

        IFC(spChild->Arrange(arrangeBounds));
    }

Cleanup:
    pReturnValue->Width = arrangeSize.Width;
    pReturnValue->Height = arrangeSize.Height;

    RRETURN(hr);
}

_Check_return_ HRESULT Page::AppBarClosedSizeChanged()
{
    IFC_RETURN(InvalidateLayoutForAppBarSizeChange());

    return S_OK;
}

_Check_return_ HRESULT Page::InvalidateLayoutForAppBarSizeChange()
{
    if (IsInLiveTree())
    {
        wuv::ApplicationViewBoundsMode boundsMode;
        IFC_RETURN(QueryDesiredBoundsMode(&boundsMode));
        if (wuv::ApplicationViewBoundsMode_UseVisible == boundsMode)
        {
            IFC_RETURN(InvalidateMeasure());
            IFC_RETURN(InvalidateArrange());
        }
    }

    return S_OK;
}

// Returns a default value of UseVisible unless this method can get a valid ApplicationView
// object to request the real desired bounds mode value.
_Check_return_ HRESULT
Page::QueryDesiredBoundsMode(
    _Out_ wuv::ApplicationViewBoundsMode* pBoundsMode)
{
    // default bounds mode is UseVisible
    *pBoundsMode = wuv::ApplicationViewBoundsMode_UseVisible;

    if (nullptr == m_spApplicationViewStatics)
    {
        IFC_RETURN(ctl::GetActivationFactory(
            wrl_wrappers::HStringReference(RuntimeClass_Windows_UI_ViewManagement_ApplicationView).Get(),
            &m_spApplicationViewStatics));
    }

    // If available get the ApplicationView.DesiredBoundsMode.  Otherwise fall back to using UseVisible
    ctl::ComPtr<wuv::IApplicationView> spApplicationView;

    if (FAILED(m_spApplicationViewStatics->GetForCurrentView(&spApplicationView)))
    {
        // Some hosting scenarios such as login UI don't implement ApplicationView and can return a
        // failure HRESULT for GetForCurrentView instead of only returning a null output.  If we get
        // any failure simply fall back to using UseVisible instead of ApplicationView.DesiredBoundsMode.
        spApplicationView = nullptr;
    }

    if (nullptr != spApplicationView)
    {
        ctl::ComPtr<wuv::IApplicationView2> spApplicationView2;
        IFC_RETURN(spApplicationView.As(&spApplicationView2));

        IFC_RETURN(spApplicationView2->get_DesiredBoundsMode(pBoundsMode));
    }

    return S_OK;
}

_Check_return_ HRESULT
Page::GetAppBarClosedHeight(_In_ DirectUI::AppBar* pAppBar, _Out_ double* pHeight)
{
    *pHeight = 0.0;

    // visibility Collapsed doesn't change ActualHeight, but for layout
    // purposes we need the height to be reported as 0.0
    xaml::Visibility visibility;
    IFC_RETURN(pAppBar->get_Visibility(&visibility));
    if (xaml::Visibility_Collapsed != visibility)
    {
        IFC_RETURN(pAppBar->get_ActualHeight(pHeight));
    }

    return S_OK;
}

// Calculates how much size needs to be subtracted from arrange bounds to account for
// appbar occlusion.  X and Y will be added to arrange bounds X and Y as an offset.
// Width and Height will be subtracted from arrange bounds Width and Height as a space
// consumed calculation.  So the values returned are tailored to be added and subtracted
// in this way.
_Check_return_ HRESULT
Page::CalculateAppBarOcclusionDimensions(
    _Out_ wf::Rect* pDimensions)
{
    // For threshold it is assumed that the top appbar will always be at the top and be
    // displayed as a horizontal bar from left to right edge and the bottom appbar will
    // always be at the bottom and be edge to edge regardless of the device orientation.
    // So this method will only populate the Y and Height values of the output pDimensions
    // Rect and will set X and Width to 0.0f.

    ZeroMemory(pDimensions, sizeof(wf::Rect));

    double topAppBarHeight = 0.0;
    double bottomAppBarHeight = 0.0;

    ctl::ComPtr<IAppBar> spTopAppBar;
    if (SUCCEEDED(get_TopAppBar(&spTopAppBar)) && spTopAppBar)
    {
        IFC_RETURN(GetAppBarClosedHeight(spTopAppBar.Cast<AppBar>(), &topAppBarHeight));
    }

    ctl::ComPtr<IAppBar> spBottomAppBar;
    if (SUCCEEDED(get_BottomAppBar(&spBottomAppBar)) && spBottomAppBar)
    {
        IFC_RETURN(GetAppBarClosedHeight(spBottomAppBar.Cast<AppBar>(), &bottomAppBarHeight));
    }

    pDimensions->Y = static_cast<float>(topAppBarHeight);
    pDimensions->Height = static_cast<float>(topAppBarHeight + bottomAppBarHeight);

    return S_OK;
}

_Check_return_ HRESULT
Page::CalculateUpdatedBounds(
    _Inout_ wf::Rect* pArrangedBounds)
{
    Window* currentWindow = nullptr;

    if (FAILED(DXamlCore::GetCurrent()->GetAssociatedWindowNoRef(this, &currentWindow)) || !currentWindow)
    {
        return S_FALSE;
    }

    auto dxamlCore = DXamlCore::GetCurrent();

    wf::Rect currentWindowBounds;
    IFC_RETURN(dxamlCore->GetContentBoundsForElement(GetHandle(), &currentWindowBounds));

    wuv::ApplicationViewBoundsMode boundsMode;
    IFC_RETURN(QueryDesiredBoundsMode(&boundsMode));

    // This flag is added for Xbox, and is false by default. On Xbox, VisibleBounds represent the
    // "Title-Safe" area - which excludes regions along the edges that some TV's cannot show ("overscan").
    //
    // When false (default value), Page.Content layout will use layout bounds as usual. This ensures
    // that content will be visible on all TV's, but may leave unused edges around it.
    // 3rd party apps will generally use this option so that they don't have to think about overscan issues.
    //
    // When true, Page.Content layout will use CoreWindow bounds, which means drawing in the overscan
    // region. 1st party Xbox apps will generally use this option, since it gives them greater flexibility
    // and screen real estate, though they have to take care to manually layout items in title-safe area as needed.
    //
    // The reason the property is separate from Windows.UI.ViewManagement.ApplicationView.DesiredBoundsMode
    // is popup-based Xaml Controls that do their own placement.Apps cannot easily ensure that ciritcal popups appear
    // in the title-safe area. Thus, we need to separate bounds used in page layout from those used internally in popup placement.
    const bool layoutToWindowBounds = IsLaidOutToWindowBounds();

    // Applied the core window's layout bounds margin to the child if the current page size is the same
    // core window size.
    bool isLayoutBoundsApplied = false;
    if (DoubleUtil::Abs(currentWindowBounds.Width - pArrangedBounds->Width) < PageApplyingLayoutBoundsTolerance &&
        DoubleUtil::Abs(currentWindowBounds.Height - pArrangedBounds->Height) < PageApplyingLayoutBoundsTolerance)
    {
        isLayoutBoundsApplied = true;

        if (wuv::ApplicationViewBoundsMode_UseVisible == boundsMode && !layoutToWindowBounds)
        {
            // Get the current window layout bounds which is smaller than Window.Bounds by the size
            // of the OS rendered chrome (tray/navigation bar).  We'll additionally need to reduce this
            // rectangle by the size of the page appbars.
            IFC_RETURN(dxamlCore->GetContentLayoutBoundsForElement(GetHandle(), &m_mostRecentLayoutBounds));

            if (DesktopUtility::IsOnDesktop())
            {
                // RS5 bug #16839635:  We're still seeing evidence of the bounds changing between the two calls
                // to retrieve CoreWindow Bounds and LayoutBounds.  This is particularly noticeable in Microsoft Edge.
                // The tactical fix is to use LayoutBounds as Bounds when running on Desktop, as currently these
                // two bounds are guaranteed to be the same.
                XamlOneCoreTransforms::FailFastIfEnabled();  // OneCoreTransforms mode should be off on Desktop.
                currentWindowBounds = m_mostRecentLayoutBounds;
            }

            m_mostRecentLayoutBounds.Width = std::min(m_mostRecentLayoutBounds.Width, currentWindowBounds.Width);
            m_mostRecentLayoutBounds.Height = std::min(m_mostRecentLayoutBounds.Height, currentWindowBounds.Height);

            // if flow direction is RTL use the left margin between bounds and layoutbounds
            // if flow direction is LTR use the right margin between bounds and layoutbounds
            xaml::FlowDirection flowDirection;
            IFC_RETURN(get_FlowDirection(&flowDirection));
            float arrangeX = flowDirection == xaml::FlowDirection_LeftToRight ?
                m_mostRecentLayoutBounds.X - currentWindowBounds.X :
                currentWindowBounds.Width - m_mostRecentLayoutBounds.Width - (m_mostRecentLayoutBounds.X - currentWindowBounds.X);
            float arrangeY = m_mostRecentLayoutBounds.Y - currentWindowBounds.Y;

            // get the arrange offsets to account for appbar occlusion
            wf::Rect appBarOcclusion;
            IFC_RETURN(CalculateAppBarOcclusionDimensions(&appBarOcclusion));

            wf::Rect contentBounds = {
                arrangeX + appBarOcclusion.X,
                arrangeY + appBarOcclusion.Y,
                m_mostRecentLayoutBounds.Width - appBarOcclusion.Width,
                m_mostRecentLayoutBounds.Height - appBarOcclusion.Height
                };
            *pArrangedBounds = contentBounds;
        }
        else // ApplicationViewBoundsMode_UseCoreWindow == boundsMode
        {
            // In this case Window.Bounds and Window.LayoutBounds are the same by definition.  The page
            // content should be occluded by both the OS chrome as well as the appbars.  So there's
            // no need to include the appbar occlusion in the returned value.

            // remove positional information from window bounds to translate into window coordinates
            m_mostRecentLayoutBounds = currentWindowBounds;
            wf::Rect contentBounds = {
                0,
                0,
                currentWindowBounds.Width,
                currentWindowBounds.Height
                };
            *pArrangedBounds = contentBounds;
        }
    }
    else if (wuv::ApplicationViewBoundsMode_UseVisible == boundsMode)
    {
        // if this page isn't the full size of the window it shouldn't attempt to avoid the
        // OS chrome but still should avoid its own appbars if boundsMode is UseVisible

        // get the arrange offsets to account for appbar occlusion
        wf::Rect appBarOcclusion;
        IFC_RETURN(CalculateAppBarOcclusionDimensions(&appBarOcclusion));

        pArrangedBounds->X += appBarOcclusion.X;
        pArrangedBounds->Y += appBarOcclusion.Y;
        pArrangedBounds->Width -= appBarOcclusion.Width;
        pArrangedBounds->Height -= appBarOcclusion.Height;
    }

    // Update the core window layout bounds changed event handler for add/remove event.
    IFC_RETURN(UpdateWindowLayoutBoundsChangedEvent(isLayoutBoundsApplied));

    return S_OK;
}

_Check_return_ HRESULT
Page::UpdateWindowLayoutBoundsChangedEvent(
    _In_ bool isLayoutBoundsApplied)
{
    Window* currentWindow = nullptr;

    if (SUCCEEDED(DXamlCore::GetCurrent()->GetAssociatedWindowNoRef(this, &currentWindow)) && currentWindow)
    {
        if (isLayoutBoundsApplied)
        {
            if(m_tokLayoutBoundsChanged.value == 0)
            {
                if (const auto xamlRoot = XamlRoot::GetImplementationForElementStatic(this))
                {
                    auto layoutBoundsHelper = xamlRoot->GetLayoutBoundsHelperNoRef();
                    
                    ctl::WeakRefPtr weakInstance;
                    IFC_RETURN(ctl::AsWeak(this, &weakInstance));

                    layoutBoundsHelper->AddLayoutBoundsChangedCallback(
                        [weakInstance]() mutable
                        {
                            ctl::ComPtr<Page> spThis;
                            IFC_RETURN(weakInstance.As(&spThis));
                            if(spThis.Get())
                            {
                                if (spThis->IsInLiveTree())
                                {
                                    IFC_RETURN(spThis->InvalidateMeasure());
                                    IFC_RETURN(spThis->InvalidateArrange());
                                }
                            }
                            return S_OK;
                        }, &m_tokLayoutBoundsChanged);
                }
            }
        }
        else if (!isLayoutBoundsApplied && m_tokLayoutBoundsChanged.value)
        {
            if (const auto xamlRoot = XamlRoot::GetImplementationForElementStatic(this))
            {
                xamlRoot->GetLayoutBoundsHelperNoRef()->RemoveLayoutBoundsChangedCallback(&m_tokLayoutBoundsChanged);
            }
        }
    }
    return S_OK;
}

// Returns True when either:
// - the current window's ShouldShrinkApplicationViewVisibleBounds() returns True for testing purposes
// - the feature RuntimeEnabledFeature::ShrinkApplicationViewVisibleBounds is set for testing purposes
bool
Page::IsLaidOutToWindowBounds()
{
    BOOLEAN layoutToWindowBounds = FALSE;

    // For testing purposes, return True when IXamlTestHooks::ShrinkApplicationViewVisibleBounds was called with True.
    Window* currentWindow = nullptr;

    if (SUCCEEDED(DXamlCore::GetCurrent()->GetAssociatedWindowNoRef(this, &currentWindow)) && currentWindow)
    {
        layoutToWindowBounds = currentWindow->ShouldShrinkApplicationViewVisibleBounds();
    }
#ifdef DBG
    if (!layoutToWindowBounds)
    {
        static auto runtimeEnabledFeatureDetector = RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector();
        // For testing purposes, return True when the RuntimeEnabledFeature::ShrinkApplicationViewVisibleBounds feature is set in a debug build.
        layoutToWindowBounds = runtimeEnabledFeatureDetector->IsFeatureEnabled(RuntimeEnabledFeature::ShrinkApplicationViewVisibleBounds);
    }
#endif

    return !!layoutToWindowBounds;
}