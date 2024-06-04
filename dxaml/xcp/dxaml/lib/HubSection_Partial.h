// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "HubSection.g.h"

namespace DirectUI
{
    class Hub;

    // Represents the class for HubSection control.
    PARTIAL_CLASS(HubSection)
    {
    private:
        TrackerPtr<xaml::IFrameworkElement> m_tpHubHeaderPlaceHolderPart;
        TrackerPtr<xaml_controls::IContentPresenter> m_tpContentPresenterPart;

        ctl::WeakRefPtr m_wrParentHub;

    protected:
        HubSection();

        IFACEMETHOD(OnApplyTemplate)() override;

        // Handle the custom property changed event and call the OnPropertyChanged methods.
        _Check_return_ HRESULT OnPropertyChanged2(_In_ const PropertyChangedParams& args) override;

        // Override OnCreateAutomationPeer()
        IFACEMETHOD(OnCreateAutomationPeer)(_Outptr_ xaml_automation_peers::IAutomationPeer** returnValue);

    public:
        _Check_return_ HRESULT SetParentHub(_In_opt_ Hub* pHub);

        // Fetches the Hub.Header height from the parent hub and sets this value on m_tpHubHeaderPlaceHolderPart.
        _Check_return_ HRESULT RefreshHubHeaderPlaceholderHeight();

        // Returns the HubSection's ContentPresenter template part.
        _Check_return_ HRESULT GetContentPresenterPart(_Outptr_result_maybenull_ xaml_controls::IContentPresenter** ppContentPresenter);

        // Returns the HubSections's HeaderButton template part.
        _Check_return_ HRESULT GetHeaderButtonPart(_Outptr_result_maybenull_ xaml_controls::IButton** ppHeaderButton);

        // Returns the HubSections's SeeMoreButton template part.
        _Check_return_ HRESULT GetSeeMoreButtonPart(_Outptr_result_maybenull_ xaml_controls::IButton** ppSeeMoreButton);

        _Check_return_ HRESULT GetParentHub(_Outptr_result_maybenull_ Hub** ppHub);

    private:
        // Callback for when the HubSection.Header property changes.
        _Check_return_ HRESULT OnHeaderChanged();

        // Evaluates whether or not to collapse the header area based on the Header and HeaderTemplate properties.
        _Check_return_ HRESULT UpdateHeaderPresenterVisibility();

        // Override Arrange so that we update header button's style with respect to whether
        // we are running in an old template and in a SemanticZoom or not.
        _Check_return_ HRESULT UpdateHeaderButtonStyle();

        TrackerPtr<xaml_controls::IButton> m_tpHeaderButtonPart;
        TrackerPtr<xaml_controls::IButton> m_tpHeaderSeeMoreButtonPart;

        ctl::EventPtr<FrameworkElementLoadedEventCallback> m_epLoadedHandler;
        ctl::EventPtr<FrameworkElementUnloadedEventCallback> m_epUnloadedHandler;

        ctl::EventPtr<ButtonBaseClickEventCallback> m_epHeaderButtonClickHandler;
        ctl::EventPtr<UIElementKeyDownEventCallback> m_epHeaderButtonKeyDownHandler;
        ctl::EventPtr<FrameworkElementLoadedEventCallback> m_epHeaderButtonLoadedHandler;

        ctl::EventPtr<ButtonBaseClickEventCallback> m_epHeaderSeeMoreButtonClickHandler;

        // Whether or not the HubSection is currently loaded in the visual tree.
        BOOLEAN m_isLoaded;

        // Whether or not the HubSection should get focus when it loads.
        BOOLEAN m_takeFocusWhenLoaded;

        // If this HubSection fails to take focus, we call back to the Hub and tell it to try to focus
        // the next HubSection so that focus does transfer from the SeZo to the Hub.
        BOOLEAN m_transferFocusForSemanticZoomMode;

        // Which FocusState the HubSection should use it if is marked to take focus when it loads.
        xaml::FocusState m_focusStateToUseWhenLoaded;

    public:
        // Override Arrange so that we update our chevron's visibility and direction.
        // When effective FlowDirection (i.e. inherited FlowDirection) changes it causes layout to rerun, which is
        // why we update the FlowDirection state of the header button here.
        // Also note that the HeaderButton template part is not available at the end of HubSection::OnApplyTemplate()
        // yet, but it is available here in ArrangeOverride().
        IFACEMETHOD(ArrangeOverride)(
            _In_ wf::Size finalSize,
            _Out_ wf::Size* returnValue)
            override;

        // Calls TakeFocus() with the provided arguments if the HubSection is already loaded,
        // otherwise assumes the HubSection is expected to be loaded and caches the arguments
        // and then calls TakeFocus() on the HubSection once it is loaded.
        //
        // semanticZoomMode param means that if this HubSection fails to take focus, we call
        // back to the Hub and tell it to try to focus the next HubSection so that focus does transfer
        // from the SeZo to the Hub.
        _Check_return_ HRESULT TakeFocusOnLoaded(_In_ xaml::FocusState focusState, _In_ BOOLEAN semanticZoomMode);

        _Check_return_ HRESULT FocusHeaderButton(_In_ xaml::FocusState focusState, _Out_ BOOLEAN* pWasFocused);

    protected:
        _Check_return_ HRESULT Initialize() override;

    private:
        _Check_return_ HRESULT OnLoaded();

        // Attempts to focus the HeaderButton, and if that fails then attempts to focus the HubSection itself.
        //
        // semanticZoomMode param means that if this HubSection fails to take focus, we call
        // back to the Hub and tell it to try to focus the next HubSection so that focus does transfer
        // from the SeZo to the Hub.
        _Check_return_ HRESULT TakeFocus(_In_ xaml::FocusState focusState, _In_ BOOLEAN semanticZoomMode);

        _Check_return_ HRESULT OnHeaderButtonClick();
        _Check_return_ HRESULT OnHeaderSeeMoreButtonClick();

        _Check_return_ HRESULT OnHeaderButtonKeyDown(_In_ xaml_input::IKeyRoutedEventArgs* pArgs);
    };

}
