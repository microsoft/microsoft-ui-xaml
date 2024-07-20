// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Page.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(Page)
    {
        friend class DxamlCoreTestHooks;

    public:

        _Check_return_ HRESULT OnNavigatedFromImpl(
            _In_ xaml::Navigation::INavigationEventArgs *pArgs);

        _Check_return_ HRESULT OnNavigatedToImpl(
            _In_ xaml::Navigation::INavigationEventArgs *pArgs);

        _Check_return_ HRESULT OnNavigatingFromImpl(
            _In_ xaml::Navigation::INavigatingCancelEventArgs *pArgs);

        _Check_return_ HRESULT
        InvokeOnNavigatedFrom(
            _In_ IInspectable *pContentIInspectable,
            _In_opt_ IInspectable *pParameterIInspectable,
            _In_opt_ xaml_animation::INavigationTransitionInfo *pTransitionInfo,
            _In_ HSTRING descriptor,
            _In_ xaml::Navigation::NavigationMode navigationMode);

        _Check_return_ HRESULT
        InvokeOnNavigatedTo(
            _In_ IInspectable *pContentIInspectable,
            _In_opt_ IInspectable *pParameterIInspectable,
            _In_opt_ xaml_animation::INavigationTransitionInfo *pTransitionInfo,
            _In_ HSTRING descriptor,
            _In_ xaml::Navigation::NavigationMode navigationMode);

        _Check_return_ HRESULT
        InvokeOnNavigatingFrom(
            _In_opt_ IInspectable *pParameterIInspectable,
            _In_opt_ xaml_animation::INavigationTransitionInfo *pTransitionInfo,
            _In_ HSTRING descriptor,
            _In_ xaml::Navigation::NavigationMode navigationMode,
            _Out_ BOOLEAN *pIsCanceled);

        _Check_return_ HRESULT OnTreeParentUpdated(
            _In_opt_ CDependencyObject *pNewParent,
            BOOLEAN isParentAlive) override;

        _Check_return_ HRESULT
        SetDescriptor(
            _In_ HSTRING descriptor);

        _Check_return_ HRESULT get_TopAppBarImpl(_Outptr_ xaml_controls::IAppBar** pValue);
        _Check_return_ HRESULT put_TopAppBarImpl(_In_ xaml_controls::IAppBar* value);

        _Check_return_ HRESULT AppBarClosedSizeChanged();

    protected:
        Page();

        ~Page() override;

        _Check_return_ HRESULT PrepareState() override;

        _Check_return_ HRESULT OnPropertyChanged2(_In_ const PropertyChangedParams& args) override;

        // Propagate the DataContext changes to the logical children
        void NotifyOfDataContextChange(_In_ const DataContextChangedParams& args) override;

        _Check_return_ HRESULT RegisterAppBars() override;
        _Check_return_ HRESULT UnregisterAppBars() override;

        IFACEMETHOD(MeasureOverride)(
            _In_ wf::Size availableSize,
            _Out_ wf::Size* pReturnValue) override;

        IFACEMETHOD(ArrangeOverride)(
            _In_ wf::Size finalSize,
            _Out_ wf::Size* pReturnValue) override;

    private:
        _Check_return_ HRESULT
        OnLoaded(
            _In_ IInspectable* pSender,
            _In_ xaml::IRoutedEventArgs* pArgs);

        _Check_return_ HRESULT
        OnUnloaded(
            _In_ IInspectable* pSender,
            _In_ xaml::IRoutedEventArgs* pArgs);

        _Check_return_ HRESULT
        InvalidateLayoutForAppBarSizeChange();

        _Check_return_ HRESULT
        QueryDesiredBoundsMode(
            _Out_ wuv::ApplicationViewBoundsMode* pBoundsMode);

        _Check_return_ HRESULT
        GetAppBarClosedHeight(
            _In_ DirectUI::AppBar* pAppBar,
            _Out_ double* pHeight);

        _Check_return_ HRESULT
        CalculateAppBarOcclusionDimensions(
            _Out_ wf::Rect* pDimensions);

        _Check_return_ HRESULT
        CalculateUpdatedBounds(
            _Inout_ wf::Rect* pArrangedBounds);

        _Check_return_ HRESULT
        UpdateWindowLayoutBoundsChangedEvent(
            _In_ bool isLayoutBoundsApplied);

        // certain devices require the focus rect to be shown on the first focused element
        bool ShouldShowInitialFocusRectangle();

        // Returns True when either:
        // - the current window's ShouldShrinkApplicationViewVisibleBounds() returns True for testing purposes
        // - the feature RuntimeEnabledFeature::ShrinkApplicationViewVisibleBounds is set for testing purposes
        bool IsLaidOutToWindowBounds();

        // only register newly set appbars when we are live in the tree
        BOOLEAN m_shouldRegisterNewAppbars;

        // Descriptor -- this is the type of the page that corresponds to this entry
        wrl_wrappers::HString m_descriptor;

        EventRegistrationToken m_tokLayoutBoundsChanged{0};

        ctl::ComPtr<wuv::IApplicationViewStatics2> m_spApplicationViewStatics;
        wf::Rect m_mostRecentLayoutBounds;
    };
}
