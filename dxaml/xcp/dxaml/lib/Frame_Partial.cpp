// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Presents a sequence of objects and navigates between them.
//  Notes:
//      Composes NavigationHistory and provides asynchronicity. Two
//      ButtonBase template parts, BackButton and ForwardButton, provide
//      rudimentary navigation support. The default navigation history
//      cache size is 10.

#include "precomp.h"
#include "Frame.g.h"
#include "Page.g.h"
#include "NavigationFailedEventArgs.g.h"
#include "NavigationHistory.h"
#include "NavigationCache.h"
#include "NavigationHelpers.h"
#include "PageStackEntry.g.h"
#include "FrameNavigationOptions.g.h"
#include "ElementSoundPlayerService_Partial.h"

using namespace DirectUI;
using namespace DirectUISynonyms;
using namespace xaml_markup;

const UINT Frame::InitialTransientCacheSize = 10;

Frame::Frame() :
    m_isCanceled(FALSE),
    m_isNavigationFromMethod(FALSE),
    m_isInNavigate(FALSE)
{
}

Frame::~Frame()
{
    m_tpNavigationHistory.Clear();
}

_Check_return_ HRESULT
Frame::Initialize()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<NavigationHistory> spNavigationHistory;
    NavigationCache *pNavigationCache = NULL;

    IFC(FrameGenerated::Initialize());

    IFC(NavigationHistory::Create(this, &spNavigationHistory));
    SetPtrValue(m_tpNavigationHistory, spNavigationHistory );

    IFC(NavigationCache::Create(this, InitialTransientCacheSize, &pNavigationCache));
    m_upNavigationCache.Reset(pNavigationCache);
    pNavigationCache = NULL;

Cleanup:
    delete pNavigationCache;

    RRETURN(hr);
}

IFACEMETHODIMP
Frame::OnApplyTemplate()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml::IDependencyObject> spNextIDependencyObject;
    ctl::ComPtr<xaml::IDependencyObject> spPreviousIDependencyObject;
    ctl::ComPtr<xaml::IRoutedEventHandler> spClickIRoutedEventHandler;

    IFC(CheckThread());

    IFC(FrameGenerated::OnApplyTemplate());

    if (m_tpNext)
    {
        IFC(m_tpNext->remove_Click(m_nextClick));
    }

    if (m_tpPrevious)
    {
        IFC(m_tpPrevious->remove_Click(m_previousClick));
    }

    m_tpNext.Clear();
    m_tpPrevious.Clear();

    IFC(GetTemplateChild(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"ForwardButton")).Get(), &spNextIDependencyObject));
    IFC(GetTemplateChild(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"BackButton")).Get(), &spPreviousIDependencyObject));

    IFC(SetPtrValueWithQI(m_tpNext, spNextIDependencyObject.Get()));
    IFC(SetPtrValueWithQI(m_tpPrevious, spPreviousIDependencyObject.Get()));

    if (m_tpNext || m_tpPrevious)
    {
        spClickIRoutedEventHandler.Attach(
            new ClassMemberEventHandler<
                Frame,
                xaml_controls::IFrame,
                xaml::IRoutedEventHandler,
                IInspectable,
                xaml::IRoutedEventArgs>(this, &Frame::ClickHandler));

        if (m_tpNext)
        {
            IFC(m_tpNext->add_Click(spClickIRoutedEventHandler.Get(), &m_nextClick));
        }

        if (m_tpPrevious)
        {
            IFC(m_tpPrevious->add_Click(spClickIRoutedEventHandler.Get(), &m_previousClick));
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
Frame::OnPropertyChanged2(_In_ const PropertyChangedParams& args)
{
    HRESULT hr = S_OK;
    wxaml_interop::TypeName sourcePageTypeName = {};
    UINT transientCacheSize = 0;
    BOOLEAN canNavigate = FALSE;

    IFC(FrameGenerated::OnPropertyChanged2(args));

    switch (args.m_pDP->GetIndex())
    {
        case KnownPropertyIndex::Frame_SourcePageType:
            if (m_isNavigationFromMethod)
            {
                m_isNavigationFromMethod = FALSE;
            }
            else
            {
                IFC(get_SourcePageType(&sourcePageTypeName));
                IFC(Navigate(sourcePageTypeName, NULL, &canNavigate));
            }

            break;

        case KnownPropertyIndex::Frame_CacheSize:
            transientCacheSize = static_cast<UINT>(args.m_pNewValue->AsSigned());    // We use AsEnum for all unsigned integers
            IFC(m_upNavigationCache->ChangeTransientCacheSize(transientCacheSize));
            break;
    }

Cleanup:
    DELETE_STRING(sourcePageTypeName.Name);
    RRETURN(hr);
}

_Check_return_ HRESULT Frame::GetNavigationTransitionInfoOverrideImpl(
    _Outptr_ xaml_animation::INavigationTransitionInfo** definitionOverride,
    _Out_ BOOLEAN* isBackNavigation,
    _Out_ BOOLEAN* isInitialPage)
{
    HRESULT hr = S_OK;

    IFCPTR(definitionOverride);
    IFCPTR(isBackNavigation);
    IFCPTR(isInitialPage);

    m_tpNavigationTransitionInfo.CopyTo(definitionOverride);
    *isBackNavigation = m_isLastNavigationBack;
    *isInitialPage = true;

    if (m_tpNavigationHistory.Get())
    {
        wrl::ComPtr<wfc::IVector<xaml::Navigation::PageStackEntry*>> backstack;
        IFC(m_tpNavigationHistory.Cast<NavigationHistory>()->GetBackStack(&backstack));
        UINT count = 0;
        if (backstack)
        {
            IFC(backstack->get_Size(&count));
            *isInitialPage = count == 0;
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT Frame::SetNavigationTransitionInfoOverrideImpl(
    _In_opt_ xaml_animation::INavigationTransitionInfo* definitionOverride)
{
    HRESULT hr = S_OK;
    PageStackEntry* pPageStackEntry = NULL;

    IFC(m_tpNavigationHistory.Cast<NavigationHistory>()->GetCurrentPageStackEntry(&pPageStackEntry));
    IFCPTR(pPageStackEntry);
    IFC(pPageStackEntry->put_NavigationTransitionInfo(definitionOverride));

Cleanup:
    pPageStackEntry = NULL;
    RRETURN(hr);
}

_Check_return_ HRESULT Frame::get_BackStackImpl(
    _Outptr_ wfc::IVector<xaml::Navigation::PageStackEntry*>** pValue)
{
    HRESULT hr = S_OK;

    IFCPTR(m_tpNavigationHistory.Get());

    IFC(m_tpNavigationHistory.Cast<NavigationHistory>()->GetBackStack(pValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT Frame::get_ForwardStackImpl(
    _Outptr_ wfc::IVector<xaml::Navigation::PageStackEntry*>** pValue)
{
    HRESULT hr = S_OK;

    IFCPTR(m_tpNavigationHistory.Get());

    IFC(m_tpNavigationHistory.Cast<NavigationHistory>()->GetForwardStack(pValue));

Cleanup:
    RRETURN(hr);
}


_Check_return_ HRESULT Frame::GoBackImpl()
{
    return GoBackWithTransitionInfoImpl(nullptr);
}

_Check_return_ HRESULT Frame::GoBackWithTransitionInfoImpl(
    _In_opt_ xaml_animation::INavigationTransitionInfo* transitionDefinition)
{
    HRESULT hr = S_OK;
    BOOLEAN reentrancyDetected = FALSE;

    IFC(CheckThread());

    // Prevent reentrancy caused by app navigating while being
    // notified of a previous navigate
    if (m_isInNavigate)
    {
        reentrancyDetected = TRUE;
        goto Cleanup;
    }
    m_isInNavigate = TRUE;

    IFCPTR(m_tpNavigationHistory.Get());

    m_isNavigationFromMethod = TRUE;

    // Update the NavigationTransitionInfo
    if (transitionDefinition)
    {
        m_tpNavigationTransitionInfo.Clear();
        IFC(SetPtrValueWithQI(m_tpNavigationTransitionInfo, transitionDefinition));
        IFC(SetNavigationTransitionInfoOverrideImpl(transitionDefinition));
    }

    IFC(m_tpNavigationHistory.Cast<NavigationHistory>()->NavigatePrevious());

    IFC(StartNavigation());

    IFC(DirectUI::ElementSoundPlayerService::RequestInteractionSoundForElementStatic(xaml::ElementSoundKind_GoBack, this));

Cleanup:
    if (!reentrancyDetected)
    {
        m_isNavigationFromMethod = FALSE;
        m_isInNavigate = FALSE;
    }
    RRETURN(hr);
}

_Check_return_ HRESULT Frame::GoForwardImpl()
{
    HRESULT hr = S_OK;
    BOOLEAN reentrancyDetected = FALSE;

    IFC(CheckThread());

    // Prevent reentrancy caused by app navigating while being
    // notified of a previous navigate
    if (m_isInNavigate)
    {
        reentrancyDetected = TRUE;
        goto Cleanup;
    }
    m_isInNavigate = TRUE;

    IFCPTR(m_tpNavigationHistory.Get());

    m_isNavigationFromMethod = TRUE;

    IFC(m_tpNavigationHistory.Cast<NavigationHistory>()->NavigateNext());

    IFC(StartNavigation());

Cleanup:
    if (!reentrancyDetected)
    {
        m_isNavigationFromMethod = FALSE;
        m_isInNavigate = FALSE;
    }
    RRETURN(hr);
}

_Check_return_ HRESULT Frame::NavigateImpl(
    _In_ wxaml_interop::TypeName sourcePageType,
    _Out_ BOOLEAN *pCanNavigate)
{
    HRESULT hr = S_OK;

    IFC(Navigate(sourcePageType, NULL, pCanNavigate));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT Frame::NavigateImpl(
    _In_ wxaml_interop::TypeName sourcePageType,
    _In_opt_ IInspectable *pIInspectable,
    _Out_ BOOLEAN *pCanNavigate)
{
    HRESULT hr = S_OK;

    IFC(NavigateWithTransitionInfoImpl(sourcePageType, pIInspectable, NULL, pCanNavigate));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT Frame::NavigateWithTransitionInfoImpl(
    _In_ wxaml_interop::TypeName sourcePageType,
    _In_opt_ IInspectable* pIInspectable,
    _In_opt_ xaml_animation::INavigationTransitionInfo* navigationTransitionInfo,
    _Out_ BOOLEAN* pCanNavigate)
{
    HRESULT hr = S_OK;
    xruntime_string_ptr strDescriptor;
    BOOLEAN reentrancyDetected = FALSE;
    const CClassInfo* pType = nullptr;

    IFC(CheckThread());

    IFCPTR(pCanNavigate);
    *pCanNavigate = FALSE;

    // Prevent reentrancy caused by app navigating while being
    // notified of a previous navigate
    if (m_isInNavigate)
    {
        reentrancyDetected = TRUE;
        goto Cleanup;
    }
    m_isInNavigate = TRUE;

    IFCPTR(sourcePageType.Name);
    IFCPTR(m_tpNavigationHistory.Get());

    m_isNavigationFromMethod = TRUE;

    IFC(MetadataAPI::GetClassInfoByTypeName(sourcePageType, &pType));
    IFC(pType->GetFullName().Promote(&strDescriptor));

    IFC(SetPtrValueWithQI(m_tpNavigationTransitionInfo, navigationTransitionInfo));
    IFC(m_tpNavigationHistory.Cast<NavigationHistory>()->NavigateNew(strDescriptor.GetHSTRING(), pIInspectable, navigationTransitionInfo));
    hr = StartNavigation();
    *pCanNavigate = ((SUCCEEDED(hr)) ? TRUE : FALSE);

Cleanup:
    if (!reentrancyDetected)
    {
        m_isNavigationFromMethod = FALSE;
        m_isInNavigate = FALSE;
    }
    RRETURN(hr);
}

_Check_return_ HRESULT Frame::NavigateToTypeImpl(
    _In_ wxaml_interop::TypeName sourcePageType,
    _In_opt_ IInspectable* pIInspectable,
    _In_opt_ xaml::Navigation::IFrameNavigationOptions* frameNavigationOptions,
    _Out_ BOOLEAN* pCanNavigate)
{
    auto cleanup = wil::scope_exit([this]()
    {
        this->m_isNavigationStackEnabledForPage = true; // reseting it for the next navigation because the default is true if the user used the unspecified implementation.
    });

    ctl::ComPtr<INavigationTransitionInfo> transitionInfoOverride;

    if (frameNavigationOptions)
    {
        frameNavigationOptions->get_IsNavigationStackEnabled(&m_isNavigationStackEnabledForPage);
        frameNavigationOptions->get_TransitionInfoOverride(&transitionInfoOverride);
    }

    IFC_RETURN(NavigateWithTransitionInfoImpl(sourcePageType, pIInspectable, transitionInfoOverride.Get(), pCanNavigate));

    return S_OK;
}

_Check_return_ HRESULT
Frame::StartNavigation()
{
    HRESULT hr = S_OK;

    IFC(NotifyNavigation());

    if (!m_isCanceled)
    {
        IFC(PerformNavigation());
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
Frame::NotifyNavigation()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IPage> spIPage;
    ctl::ComPtr<IInspectable> spPageIInspectable;
    ctl::ComPtr<IInspectable> spParameterIInspectable;
    ctl::ComPtr<INavigationTransitionInfo> spNavigationTransitionInfo;
    wrl_wrappers::HString strDescriptor;
    PageStackEntry *pPageStackEntry = NULL;
    xaml::Navigation::NavigationMode navigationMode = xaml::Navigation::NavigationMode_New;

    IFCPTR(m_tpNavigationHistory.Get());

    IFC(m_tpNavigationHistory.Cast<NavigationHistory>()->GetPendingNavigationMode(&navigationMode));
    IFC(m_tpNavigationHistory.Cast<NavigationHistory>()->GetPendingPageStackEntry(&pPageStackEntry));
    IFCPTR(pPageStackEntry);
    IFC(pPageStackEntry->GetDescriptor(strDescriptor.GetAddressOf()));
    IFCPTR(strDescriptor.Get());
    IFC(pPageStackEntry->get_Parameter(&spParameterIInspectable));
    IFC(pPageStackEntry->get_NavigationTransitionInfo(&spNavigationTransitionInfo));

    IFC(RaiseNavigating(spParameterIInspectable.Get(), spNavigationTransitionInfo.Get(), strDescriptor.Get(), navigationMode, &m_isCanceled));

    if (m_isCanceled)
    {
        IFC(RaiseNavigationStopped(NULL, spParameterIInspectable.Get(), spNavigationTransitionInfo.Get(), strDescriptor.Get(), navigationMode));
        goto Cleanup;
    }

    IFC(get_Content(&spPageIInspectable));

    spIPage = spPageIInspectable.AsOrNull<IPage>();

    if (spIPage)
    {
        IFC(spIPage.Cast<Page>()->InvokeOnNavigatingFrom(spParameterIInspectable.Get(), spNavigationTransitionInfo.Get(), strDescriptor.Get(), navigationMode, &m_isCanceled));

        if (m_isCanceled)
        {
            IFC(RaiseNavigationStopped(NULL, spParameterIInspectable.Get(), spNavigationTransitionInfo.Get(), strDescriptor.Get(), navigationMode));
        }
    }

Cleanup:
    pPageStackEntry = NULL;

    RRETURN(hr);
}

_Check_return_ HRESULT
Frame::PerformNavigation()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> spOldIInspectable;
    ctl::ComPtr<IInspectable> spNewIInspectable;
    ctl::ComPtr<IInspectable> spParameterIInspectable;
    ctl::ComPtr<INavigationTransitionInfo> spNavigationTransitionInfo;
    PageStackEntry *pPageStackEntry = NULL;

    if (m_isCanceled)
    {
        goto Cleanup;
    }

    IFCPTR(m_tpNavigationHistory.Get());

    IFC(m_tpNavigationHistory.Cast<NavigationHistory>()->GetPendingPageStackEntry(&pPageStackEntry));
    IFCPTR(pPageStackEntry);

    IFC(get_Content(&spOldIInspectable));
    IFC(m_upNavigationCache->GetContent(pPageStackEntry, &spNewIInspectable));
    IFC(pPageStackEntry->get_Parameter(&spParameterIInspectable));
    IFC(pPageStackEntry->get_NavigationTransitionInfo(&spNavigationTransitionInfo));

    IFC(ChangeContent(spOldIInspectable.Get(), spNewIInspectable.Get(), spParameterIInspectable.Get(), spNavigationTransitionInfo.Get()));

Cleanup:
    pPageStackEntry = NULL;
    m_isNavigationFromMethod = FALSE;

    RRETURN(hr);
}

_Check_return_ HRESULT
Frame::RemovePageFromCache(
    _In_ HSTRING descriptor)
{
    HRESULT hr = S_OK;
    IFC(m_upNavigationCache->UncachePageContent(descriptor));

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method: Frame::NotifyGetOrSetNavigationState
//
//  Synopsis:
//     Notify current page of Get/SetNavigationState.
//  Frame.GetNavigationState is typically called when the App is suspended
//  Frame.SetNavigationState is typically called when the App is resumed
//
//  This notification is to give the current page the opportunity to:
//  1. Serialize page content in Page.OnNavigatedFrom.
//  2. Deserialize page content and get navigation param in
//     Page.OnNavigatedTo.
//
//  This allows the current page to serialize/de-serialize like any other
//  page in the navstack. If this notification is not done, the current page
//  would not get Page.OnNavigatedFrom when the App is suspended or
//  Page.OnNavigatedTo when the App is resumed. Other pages in the navstack
//  get Page.OnNavigatedFrom when the page is navigated away from, and
//  Page.OnNavigatedTo when the page is navigated to.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
Frame::NotifyGetOrSetNavigationState(
    _In_ NavigationStateOperation navigationStateOperation)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> spContentIInspectable;
    ctl::ComPtr<IPage> spIPage;
    ctl::ComPtr<IInspectable> spParameterIInspectable;
    ctl::ComPtr<xaml_animation::INavigationTransitionInfo> spNavigationTransitionInfo;
    wrl_wrappers::HString strDescriptor;
    PageStackEntry *pPageStackEntry = NULL;

    // Get current navigation entry
    if (!m_tpNavigationHistory)
    {
        goto Cleanup;
    }
    IFC(m_tpNavigationHistory.Cast<NavigationHistory>()->GetCurrentPageStackEntry(&pPageStackEntry));
    if (!pPageStackEntry)
    {
        goto Cleanup;
    }

    // Get current page, param and page type
    IFC(get_Content(&spContentIInspectable));
    IFC(pPageStackEntry->get_Parameter(&spParameterIInspectable));
    IFC(pPageStackEntry->get_NavigationTransitionInfo(&spNavigationTransitionInfo));
    IFC(pPageStackEntry->GetDescriptor(strDescriptor.GetAddressOf()));

    spIPage = spContentIInspectable.AsOrNull<IPage>();
    IFCPTR(spIPage);

    if (navigationStateOperation == NavigationStateOperation_Get)
    {
        // Call Page.OnNavigatedFrom. Use Forward as navigation mode, which
        // is the best fit among available modes -- we are going forward from
        // the page to App's suspend mode.
        IFC(spIPage.Cast<Page>()->InvokeOnNavigatedFrom(spIPage.Get(), spParameterIInspectable.Get(), spNavigationTransitionInfo.Get(), strDescriptor.Get(),
                        xaml::Navigation::NavigationMode_Forward));
    }
    else
    {
        // Call Page.OnNavigatedTo. Use Back as navigation mode,which
        // is the best fit among available modes -- we are going back to the
        // page on App Resume.
        IFC(spIPage.Cast<Page>()->InvokeOnNavigatedTo(spIPage.Get(), spParameterIInspectable.Get(), spNavigationTransitionInfo.Get(), strDescriptor.Get(),
                        xaml::Navigation::NavigationMode_Back));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
Frame::ChangeContent(
    _In_ IInspectable *pOldIInspectable,
    _In_ IInspectable *pNewIInspectable,
    _In_ IInspectable *pParameterIInspectable,
    _In_ xaml_animation::INavigationTransitionInfo *pTransitionInfo)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IPage> spOldIPage;
    ctl::ComPtr<IPage> spNewIPage;
    wrl_wrappers::HString strDescriptor;
    BOOLEAN isHandled = FALSE;
    BOOLEAN wasContentChanged = FALSE;
    PageStackEntry *pPageStackEntry = NULL;
    xaml::Navigation::NavigationMode navigationMode = xaml::Navigation::NavigationMode_New;

    BOOLEAN isNavigationStackEnabled = FALSE;
    IFC(get_IsNavigationStackEnabled(&isNavigationStackEnabled));

    IFCPTR(pNewIInspectable);
    IFCPTR(m_tpNavigationHistory.Get());

    IFC(m_tpNavigationHistory.Cast<NavigationHistory>()->GetPendingNavigationMode(&navigationMode));
    IFC(m_tpNavigationHistory.Cast<NavigationHistory>()->GetPendingPageStackEntry(&pPageStackEntry));
    IFCPTR(pPageStackEntry);
    IFC(pPageStackEntry->GetDescriptor(strDescriptor.GetAddressOf()));
    IFCPTR(strDescriptor.Get());

    // If this is a back navigation, cache the navigation mode
    // and override the NavigationTransitionInfo with the
    // transition that took place upon navigation from that page.
    m_isLastNavigationBack = FALSE;
    if(navigationMode == xaml::Navigation::NavigationMode_Back)
    {
        PageStackEntry *pCurrentPageStackEntry = NULL;
        ctl::ComPtr<xaml_animation::INavigationTransitionInfo> spNavigationTransitionInfo;

        IFC(m_tpNavigationHistory.Cast<NavigationHistory>()->GetCurrentPageStackEntry(&pCurrentPageStackEntry));

        if (pCurrentPageStackEntry)
        {
            IFC(pCurrentPageStackEntry->get_NavigationTransitionInfo(&spNavigationTransitionInfo));

            m_isLastNavigationBack = TRUE;
            IFC(SetPtrValueWithQI(m_tpNavigationTransitionInfo, spNavigationTransitionInfo.Get()));
        }
    }

    spOldIPage = ctl::query_interface_cast<IPage>(pOldIInspectable);
    spNewIPage = ctl::query_interface_cast<IPage>(pNewIInspectable);
    IFCPTR(spNewIPage);

    IFC(put_Content(pNewIInspectable));
    wasContentChanged = TRUE;

    if (isNavigationStackEnabled && m_isNavigationStackEnabledForPage)
    {
        IFC(m_tpNavigationHistory.Cast<NavigationHistory>()->CommitNavigation());
    }

    IFC(RaiseNavigated(spNewIPage.Get(), pParameterIInspectable, pTransitionInfo, strDescriptor.Get(), navigationMode));

    if (spOldIPage)
    {
        IFC(spOldIPage.Cast<Page>()->InvokeOnNavigatedFrom(spNewIPage.Get(), pParameterIInspectable, pTransitionInfo, strDescriptor.Get(), navigationMode));
    }

    IFC(spNewIPage.Cast<Page>()->InvokeOnNavigatedTo(spNewIPage.Get(), pParameterIInspectable, pTransitionInfo, strDescriptor.Get(), navigationMode));

Cleanup:
    if (FAILED(hr))
    {
        IGNOREHR(RaiseNavigationFailed(strDescriptor.Get(), hr, &isHandled));

        if (!isHandled)
        {
            IGNOREHR(RaiseUnhandledException(E_UNEXPECTED, TEXT_FRAME_NAVIGATION_FAILED_UNHANDLED));
        }

        if (wasContentChanged)
        {
            IGNOREHR(put_Content(pOldIInspectable));
        }
    }

    RRETURN(hr);
}

_Check_return_ HRESULT
Frame::RaiseUnhandledException(
    _In_ XUINT32 errorCode,
    _In_ XUINT32 resourceStringID)
{
    HRESULT hr = S_OK;
    xstring_ptr strMessage;
    HRESULT hrToReport;
    bool fIsHandled = false;

    IFC(ErrorHelper::MapHresult(errorCode, &hrToReport));
    IFC(ErrorHelper::GetNonLocalizedErrorString(resourceStringID, &strMessage));

    IFC(ErrorHelper::RaiseUnhandledExceptionEvent(hrToReport, strMessage, &fIsHandled));

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method: Frame::GetNavigationState
//
//  Synopsis:
//     Serialize frame's navigation history into an HSTRING
//
//------------------------------------------------------------------------

_Check_return_ HRESULT Frame::GetNavigationStateImpl(
    _Out_ HSTRING* pNavigationState)
{
    HRESULT hr = S_OK;

    IFCPTR(pNavigationState);

    IFC(NotifyGetOrSetNavigationState(NavigationStateOperation_Get));

    IFCPTR(m_tpNavigationHistory.Get());
    IFC(m_tpNavigationHistory.Cast<NavigationHistory>()->GetNavigationState(pNavigationState));

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method: Frame::SetNavigationState
//
//  Synopsis:
//     Read and restore frame's navigation history from HSTRING
//
//------------------------------------------------------------------------

_Check_return_ HRESULT Frame::SetNavigationStateImpl(
    _In_ HSTRING navigationState)
{
    IFC_RETURN(SetNavigationStateWithNavigationControlImpl(navigationState, FALSE));

    return S_OK;
}

_Check_return_ HRESULT Frame::SetNavigationStateWithNavigationControlImpl(
    _In_ HSTRING navigationState, _In_ BOOLEAN suppressNavigate)
{
    IFCPTR_RETURN(navigationState);

    // SetNavigationState of navigation history
    IFCPTR_RETURN(m_tpNavigationHistory.Get());
    IFC_RETURN(m_tpNavigationHistory.Cast<NavigationHistory>()->SetNavigationState(navigationState, suppressNavigate));

    if (suppressNavigate)
    {
        IFC_RETURN(put_Content(nullptr));
    }
    else
    {
        ctl::ComPtr<IInspectable> spContentIInspectable;
        PageStackEntry *pPageStackEntry = nullptr;

        // Create or get page corresponding to current navigation entry and set it as
        // frame's content. NavigationCache.GetContent will create the current page.
        IFC_RETURN(m_tpNavigationHistory.Cast<NavigationHistory>()->GetCurrentPageStackEntry(&pPageStackEntry));
        if (pPageStackEntry)
        {
            m_isNavigationFromMethod = TRUE;

            IFC_RETURN(m_upNavigationCache->GetContent(pPageStackEntry, &spContentIInspectable));
            IFC_RETURN(put_Content(spContentIInspectable.Get()));
        }

        // Commit SetNavigationState of navigation history
        IFC_RETURN(m_tpNavigationHistory.Cast<NavigationHistory>()->CommitSetNavigationState(m_upNavigationCache.Get()));
    }

    IFC_RETURN(NotifyGetOrSetNavigationState(NavigationStateOperation_Set));

    return S_OK;
}

_Check_return_ HRESULT
Frame::ClickHandler(
    _In_ IInspectable *pSender,
    _In_ xaml::IRoutedEventArgs *pArgs)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_primitives::IButtonBase> spSenderIButtonBase;

    IFC(ctl::do_query_interface(spSenderIButtonBase, pSender));

    if (m_tpNext && m_tpNext.Get() == spSenderIButtonBase.Get())
    {
        IFC(GoForward());
    }
    else if (m_tpPrevious && m_tpPrevious.Get() == spSenderIButtonBase.Get())
    {
        IFC(GoBack());
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
Frame::RaiseNavigated(
    _In_ IInspectable *pContentIInspectable,
    _In_opt_ IInspectable *pParameterIInspectable,
    _In_opt_ xaml_animation::INavigationTransitionInfo *pTransitionInfo,
    _In_ HSTRING descriptor,
    _In_ xaml::Navigation::NavigationMode navigationMode)
{
    HRESULT hr = S_OK;
    NavigatedEventSourceType* pEventSource = nullptr;
    ctl::ComPtr<INavigationEventArgs> spINavigationEventArgs;

    IFCPTR(descriptor);
    IFCPTR(pContentIInspectable);

    IFC(NavigationHelpers::CreateINavigationEventArgs(pContentIInspectable, pParameterIInspectable, pTransitionInfo, descriptor, navigationMode, &spINavigationEventArgs));
    IFCPTR(spINavigationEventArgs);

    IFC(GetNavigatedEventSourceNoRef(&pEventSource));
    IFC(pEventSource->Raise(ctl::as_iinspectable(this), spINavigationEventArgs.Get()));

    TraceFrameNavigatedInfo(WindowsGetStringRawBuffer(descriptor, NULL), static_cast<const unsigned char>(navigationMode));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
Frame::RaiseNavigating(
    _In_opt_ IInspectable *pParameterIInspectable,
    _In_opt_ xaml_animation::INavigationTransitionInfo *pTransitionInfo,
    _In_ HSTRING descriptor,
    _In_ xaml::Navigation::NavigationMode navigationMode,
    _Out_ BOOLEAN *pIsCanceled)
{
    HRESULT hr = S_OK;
    NavigatingEventSourceType* pEventSource = nullptr;
    ctl::ComPtr<INavigatingCancelEventArgs> spINavigatingCancelEventArgs;

    IFCPTR(pIsCanceled);
    *pIsCanceled = NULL;

    IFCPTR(descriptor);

    IFC(NavigationHelpers::CreateINavigatingCancelEventArgs(pParameterIInspectable, pTransitionInfo, descriptor, navigationMode, &spINavigatingCancelEventArgs));
    IFCPTR(spINavigatingCancelEventArgs);

    IFC(GetNavigatingEventSourceNoRef(&pEventSource));
    IFC(pEventSource->Raise(ctl::as_iinspectable(this), spINavigatingCancelEventArgs.Get()));

    IFC(spINavigatingCancelEventArgs->get_Cancel(pIsCanceled));

    TraceFrameNavigatingInfo(WindowsGetStringRawBuffer(descriptor, NULL), static_cast<const unsigned char>(navigationMode));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
Frame::RaiseNavigationFailed(
    _In_ HSTRING descriptor,
    _In_ HRESULT errorResult,
    _Out_ BOOLEAN *pIsCanceled)
{
    HRESULT hr = S_OK;
    NavigationFailedEventSourceType* pEventSource = nullptr;
    ctl::ComPtr<NavigationFailedEventArgs> spNavigationFailedEventArgs;
    wxaml_interop::TypeName sourcePageType = {};

    IFCPTR(pIsCanceled);
    *pIsCanceled = NULL;

    IFCPTR(descriptor);

    IFC(ctl::make<NavigationFailedEventArgs>(&spNavigationFailedEventArgs));
    IFCPTR(spNavigationFailedEventArgs);

    IFC(MetadataAPI::GetTypeNameByFullName(XSTRING_PTR_EPHEMERAL_FROM_HSTRING(descriptor), &sourcePageType));
    IFC(spNavigationFailedEventArgs->put_SourcePageType(sourcePageType));
    IFC(spNavigationFailedEventArgs->put_Exception(errorResult));

    IFC(GetNavigationFailedEventSourceNoRef(&pEventSource));

    IFC(pEventSource->Raise(ctl::as_iinspectable(this), spNavigationFailedEventArgs.Get()));

    IFC(spNavigationFailedEventArgs->get_Handled(pIsCanceled));

Cleanup:
    DELETE_STRING(sourcePageType.Name);
    RRETURN(hr);
}

_Check_return_ HRESULT
Frame::RaiseNavigationStopped(
    _In_ IInspectable *pContentIInspectable,
    _In_ IInspectable *pParameterIInspectable,
    _In_ xaml_animation::INavigationTransitionInfo *pTransitionInfo,
    _In_ HSTRING descriptor,
    _In_ xaml::Navigation::NavigationMode navigationMode)
{
    HRESULT hr = S_OK;
    NavigationStoppedEventSourceType* pEventSource = nullptr;
    ctl::ComPtr<INavigationEventArgs> spINavigationEventArgs;

    IFCPTR(descriptor);

    IFC(NavigationHelpers::CreateINavigationEventArgs(pContentIInspectable, pParameterIInspectable, pTransitionInfo, descriptor, navigationMode, &spINavigationEventArgs));
    IFCPTR(spINavigationEventArgs);

    IFC(GetNavigationStoppedEventSourceNoRef(&pEventSource));
    IFC(pEventSource->Raise(ctl::as_iinspectable(this), spINavigationEventArgs.Get()));

Cleanup:
    RRETURN(hr);
}

void
Frame::OnReferenceTrackerWalk(INT walkType) // override
{
    // Walk field references

    // TODO: Change this into a TrackerPtr
    if (m_upNavigationCache)
    {
        m_upNavigationCache.ReferenceTrackerWalk(static_cast<EReferenceTrackerWalkType>(walkType));
    }

    // Walk remaining references

    FrameGenerated::OnReferenceTrackerWalk( walkType );
}

_Check_return_ HRESULT
Frame::GetCurrentNavigationMode(
_Out_ xaml::Navigation::NavigationMode *pNavigationMode)
{
    HRESULT hr = S_OK;

    IFCPTR(m_tpNavigationHistory.Get());

    IFC(m_tpNavigationHistory.Cast<NavigationHistory>()->GetCurrentNavigationMode(pNavigationMode));

Cleanup:
    RRETURN(hr);
}

