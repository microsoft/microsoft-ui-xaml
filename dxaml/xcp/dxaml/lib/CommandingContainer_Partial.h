// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Provides a connection between commands and the controls they operate on.

#pragma once

#include "CommandingContainer.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(CommandingContainer)
    {
        public:
            xaml::IDependencyObject* GetCommandTargetNoRef();
            xaml_controls::IItemsControl* GetListCommandTargetNoRef();

            static _Check_return_ HRESULT NotifyContextChangedStatic(_In_ xaml::IDependencyObject* commandTarget);

        protected:
            ~CommandingContainer() override { }

            _Check_return_ HRESULT OnPropertyChanged2(_In_ const PropertyChangedParams& args) override;

            _Check_return_ HRESULT EnterImpl(
                _In_ bool bLive,
                _In_ bool bSkipNameRegistration,
                _In_ bool bCoercedIsEnabled,
                _In_ bool bUseLayoutRounding) final;

            _Check_return_ HRESULT LeaveImpl(
                _In_ bool bLive,
                _In_ bool bSkipNameRegistration,
                _In_ bool bCoercedIsEnabled,
                _In_ bool bVisualTreeBeingReset) final;

        private:
            _Check_return_ HRESULT OnCommandingTargetChanged(_In_ xaml::IDependencyObject* oldTarget, _In_ xaml::IDependencyObject* newTarget);

            _Check_return_ HRESULT OnGotFocus(_In_ xaml::IRoutedEventArgs* args);
            _Check_return_ HRESULT OnCommandingTargetGotFocus(_In_ xaml::IRoutedEventArgs* args);

            ctl::WeakRefPtr m_wrCommandTarget;
            ctl::WeakRefPtr m_wrListCommandTarget;
            ctl::EventPtr<UIElementGotFocusEventCallback> m_gotFocusHandler;
            ctl::EventPtr<UIElementGotFocusEventCallback> m_commandingTargetGotFocusHandler;
    };
}

