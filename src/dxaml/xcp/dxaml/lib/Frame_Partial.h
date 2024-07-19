// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Presents a sequence of objects and navigates between them.

#pragma once

#include "Frame.g.h"

namespace DirectUI
{
    class NavigationCache;
    class NavigationHistory;
    class FrameNavigationOptions;

    PARTIAL_CLASS(Frame)
    {
        enum NavigationStateOperation
        {
            NavigationStateOperation_Get,
            NavigationStateOperation_Set
        };

    private:
        static const UINT InitialTransientCacheSize;

        BOOLEAN m_isInNavigate;

        BOOLEAN m_isNavigationStackEnabledForPage = TRUE;

        BOOLEAN m_isCanceled;

        BOOLEAN m_isNavigationFromMethod;

        BOOLEAN m_isLastNavigationBack = FALSE;

        EventRegistrationToken m_nextClick;

        EventRegistrationToken m_previousClick;

        TrackerUniquePtr<NavigationCache> m_upNavigationCache;

        TrackerPtr<NavigationHistory> m_tpNavigationHistory;

        TrackerPtr<xaml_primitives::IButtonBase> m_tpNext;
        TrackerPtr<xaml_primitives::IButtonBase> m_tpPrevious;

        TrackerPtr<xaml_animation::INavigationTransitionInfo> m_tpNavigationTransitionInfo;

        _Check_return_ HRESULT
        StartNavigation();

        _Check_return_ HRESULT
        NotifyNavigation();

        _Check_return_ HRESULT
        ChangeContent(
            _In_ IInspectable *pOldIInspectable,
            _In_ IInspectable *pNewIInspectable,
            _In_ IInspectable *pParameterIInspectable,
            _In_ xaml_animation::INavigationTransitionInfo *pTransitionInfo);

        _Check_return_ HRESULT
        RaiseUnhandledException(
            _In_ XUINT32 errorCode,
            _In_ XUINT32 resourceStringID);

        _Check_return_ HRESULT
        ClickHandler(
            _In_ IInspectable *pSender,
            _In_ xaml::IRoutedEventArgs *pArgs);

        _Check_return_ HRESULT
        PerformNavigation();

        _Check_return_ HRESULT
        NotifyGetOrSetNavigationState(
            _In_ NavigationStateOperation navigationStateOperation);

        _Check_return_ HRESULT
        RaiseNavigated(
            _In_ IInspectable *pContentIInspectable,
            _In_opt_ IInspectable *pParameterIInspectable,
            _In_opt_ xaml_animation::INavigationTransitionInfo *pTransitionInfo,
            _In_ HSTRING descriptor,
            _In_ xaml::Navigation::NavigationMode navigationMode);

        _Check_return_ HRESULT
        RaiseNavigating(
            _In_opt_ IInspectable *pParameterIInspectable,
            _In_opt_ xaml_animation::INavigationTransitionInfo *pTransitionInfo,
            _In_ HSTRING descriptor,
            _In_ xaml::Navigation::NavigationMode navigationMode,
            _Out_ BOOLEAN *pIsCanceled);

        _Check_return_ HRESULT
        RaiseNavigationFailed(
            _In_ HSTRING descriptor,
            _In_ HRESULT errorResult,
            _Out_ BOOLEAN *pIsCanceled);

        _Check_return_ HRESULT
        RaiseNavigationStopped(
            _In_ IInspectable *pContentIInspectable,
            _In_ IInspectable *pParameterIInspectable,
            _In_ xaml_animation::INavigationTransitionInfo *pTransitionInfo,
            _In_ HSTRING descriptor,
            _In_ xaml::Navigation::NavigationMode navigationMode);

    protected:
        Frame();

        ~Frame() override;

        _Check_return_ HRESULT
        Initialize() override;

        _Check_return_ HRESULT OnPropertyChanged2(_In_ const PropertyChangedParams& args) override;

        void OnReferenceTrackerWalk(INT walkType) final;

    public:
        IFACEMETHOD(OnApplyTemplate)() override;

        _Check_return_ HRESULT get_BackStackImpl(
            _Outptr_ wfc::IVector<xaml::Navigation::PageStackEntry*>** pValue);

        _Check_return_ HRESULT get_ForwardStackImpl(
            _Outptr_ wfc::IVector<xaml::Navigation::PageStackEntry*>** pValue);

        _Check_return_ HRESULT GoBackImpl();

        _Check_return_ HRESULT GoBackWithTransitionInfoImpl(
            _In_opt_ xaml_animation::INavigationTransitionInfo* transitionDefinition);

        _Check_return_ HRESULT GoForwardImpl();

        _Check_return_ HRESULT NavigateImpl(
            _In_ wxaml_interop::TypeName sourcePageType,
            _Out_ BOOLEAN *pCanNavigate);

        using FrameGenerated::Navigate;

        _Check_return_ HRESULT NavigateImpl(
            _In_ wxaml_interop::TypeName sourcePageType,
            _In_opt_ IInspectable *pIInspectable,
            _Out_ BOOLEAN *pCanNavigate);

        _Check_return_ HRESULT NavigateWithTransitionInfoImpl(
            _In_ wxaml_interop::TypeName sourcePageType,
            _In_opt_ IInspectable* pIInspectable,
            _In_opt_ xaml_animation::INavigationTransitionInfo* transitionDefinition,
            _Out_ BOOLEAN* pCanNavigate);

        _Check_return_ HRESULT NavigateToTypeImpl(
            _In_ wxaml_interop::TypeName sourcePageType,
            _In_opt_ IInspectable* pIInspectable,
            _In_opt_ xaml::Navigation::IFrameNavigationOptions* frameNavigationOptions,
            _Out_ BOOLEAN* pCanNavigate);

        _Check_return_ HRESULT GetNavigationStateImpl(_Out_ HSTRING* pNavigationState);

        _Check_return_ HRESULT SetNavigationStateImpl(_In_ HSTRING navigationState);

        _Check_return_ HRESULT SetNavigationStateWithNavigationControlImpl(_In_ HSTRING navigationState, _In_ BOOLEAN suppressNavigate);

        _Check_return_ HRESULT GetNavigationTransitionInfoOverrideImpl(
            _Outptr_ xaml_animation::INavigationTransitionInfo** definitionOverride,
            _Out_ BOOLEAN* isBackNavigation,
            _Out_ BOOLEAN* isInitialPage);

        _Check_return_ HRESULT SetNavigationTransitionInfoOverrideImpl(
            _In_opt_ xaml_animation::INavigationTransitionInfo* definitionOverride);

        _Check_return_ HRESULT GetCurrentNavigationMode(
            _Out_ xaml::Navigation::NavigationMode *pNavigationMode);

        _Check_return_ HRESULT RemovePageFromCache(
            _In_ HSTRING descriptor);
    };
}
