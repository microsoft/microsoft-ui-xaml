// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace DirectUI
{
    class TextControlFlyout
    {
    public:
        TextControlFlyout(_In_ CFlyoutBase* flyout, bool isProofingFlyout);
        ~TextControlFlyout();

        _Check_return_ HRESULT OnClosing(_In_ xaml_primitives::IFlyoutBase* sender, _In_ xaml_primitives::IFlyoutBaseClosingEventArgs* eventArgs);
        _Check_return_ HRESULT OnClosed(_In_ IInspectable* sender, _In_ IInspectable* eventArgs);
        _Check_return_ HRESULT OnOpened(_In_ IInspectable* sender, _In_ IInspectable* eventArgs);

        _Check_return_ HRESULT OnActiveOwnerLosingFocus(_In_ xaml::IUIElement* sender, _In_ xaml_input::ILosingFocusEventArgs* eventArgs);
        _Check_return_ HRESULT OnActiveOwnerGettingFocus(_In_ xaml::IUIElement* sender, _In_ xaml_input::IGettingFocusEventArgs* eventArgs);
        _Check_return_ HRESULT OnActiveOwnerGotFocus(_In_ IInspectable* sender, _In_ xaml::IRoutedEventArgs* eventArgs);

        bool IsOpened() { return m_isOpened; }
        bool IsTransient() { return m_isTransient; }
        bool IsGettingFocus() { return m_isGettingFocus; }
        bool IsProofingFlyout() { return m_isProofingFlyout; }

        void SetActiveOwner(_In_ CFrameworkElement* owner);
        CFrameworkElement* GetActiveOwnerNoRef() { return m_activeOwnerWeakRef.lock().get(); }
        CFlyoutBase* GetFlyoutNoRef() { return m_flyoutWeakRef.lock().get(); }

        bool IsOpen();
        _Check_return_ HRESULT CloseIfOpen();
        _Check_return_ HRESULT ShowAt(wf::Point point, wf::Rect exclusionRect, xaml_primitives::FlyoutShowMode showMode);

    private:
        ctl::EventPtr<FlyoutBaseClosingEventCallback> m_closingHandler;
        ctl::EventPtr<FlyoutBaseClosedEventCallback> m_closedHandler;
        ctl::EventPtr<FlyoutBaseOpenedEventCallback> m_openedHandler;

        ctl::EventPtr<UIElementLosingFocusEventCallback> m_activeOwnerLosingFocusEventHandler;

        xref::weakref_ptr<CFrameworkElement> m_activeOwnerWeakRef;
        xref::weakref_ptr<CFlyoutBase> m_flyoutWeakRef;

        bool m_isOpened = false;
        bool m_isTransient = false;
        bool m_isGettingFocus = false;
        bool m_isProofingFlyout = false;

        _Check_return_ HRESULT ClearActiveOwnerEventHandlers();
    };

    namespace TextControlFlyoutHelper
    {
        bool IsGettingFocus(_In_opt_ CFlyoutBase* flyout, _In_ CFrameworkElement* owner);
        bool IsOpen(_In_ CFlyoutBase* flyout);
        bool IsElementChildOfOpenedFlyout(_In_opt_ CUIElement* element);
        bool IsElementChildOfTransientOpenedFlyout(_In_opt_ CUIElement* element);
        bool IsElementChildOfProofingFlyout(_In_opt_ CUIElement* element);

        _Check_return_ HRESULT DismissAllFlyoutsForOwner(_In_opt_ CUIElement *element);
        _Check_return_ HRESULT CloseIfOpen(_In_opt_ CFlyoutBase* flyout);
        _Check_return_ HRESULT ShowAt(_In_ CFlyoutBase* flyout, _In_ CFrameworkElement* owner, wf::Point point, wf::Rect exclusionRect, xaml_primitives::FlyoutShowMode showMode);
        _Check_return_ HRESULT AddProofingFlyout(_In_ CFlyoutBase* flyout, _In_ CFrameworkElement* owner);
    };
}
