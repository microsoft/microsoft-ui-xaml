// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
#pragma once

namespace DirectUI
{
    class LayoutBoundsChangedHelper
        : public ctl::WeakReferenceSource
    {
    public:
        LayoutBoundsChangedHelper() = default;
        ~LayoutBoundsChangedHelper() override;
        static _Check_return_ HRESULT GetApplicationView2(_Out_ wuv::IApplicationView2** appView2);
        static _Check_return_ HRESULT GetDesiredBoundsMode(_Out_ wuv::ApplicationViewBoundsMode* boundsMode);
        void AddLayoutBoundsChangedCallback(_In_ std::function<HRESULT()> callback, _Out_ EventRegistrationToken* tokParentBoundsChanged);
        void RemoveLayoutBoundsChangedCallback(_In_ EventRegistrationToken* tokParentBoundsChanged);

        void DisconnectWindowEventHandlers();
        _Check_return_ HRESULT InitializeSizeChangedHandlers(_In_ xaml::IXamlRoot* xamlRoot);

    private:
        _Check_return_ HRESULT OnXamlRootChanged(
            _In_ xaml::IXamlRoot* pSender,
            _In_ xaml::IXamlRootChangedEventArgs* pArgs);

        _Check_return_ HRESULT OnApplicationViewVisibleBoundsChanged(
            _In_ wuv::IApplicationView* pSender,
            _In_ IInspectable* pArgs);

        _Check_return_ HRESULT OnPotentialParentBoundsChanged(wuv::ApplicationViewBoundsMode boundsMode);

        _Check_return_ HRESULT UpdateApplicationBarServiceBounds();

        int m_eventTokenNumber {1};
        containers::vector_map<int, std::function<HRESULT()>> m_callbacks{};
        EventRegistrationToken m_tokAppViewVisibleBoundsChanged{0};

        ctl::EventPtr<XamlRootChangedEventCallback> m_xamlRootChangedEventHandler;

        // Store a reference to the xaml root used when subscribing to events
        ctl::WeakRefPtr m_weakXamlRoot;
    };
}
