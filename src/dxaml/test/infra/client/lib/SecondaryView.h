// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Private.Infrastructure.h>
namespace Private { namespace Infrastructure {

    // The SecondaryView is a nice RAII helper that can be used when writing tests that need to create multiple
    // views.
    class SecondaryView : public Microsoft::WRL::RuntimeClass<test_infra::ISecondaryView, wf::IClosable>
    {
        InspectableClass(RuntimeClass_Private_Infrastructure_SecondaryView, TrustLevel::BaseTrust);
    public:
        SecondaryView();
        virtual ~SecondaryView();

        IFACEMETHOD(RuntimeClassInitialize)();

        IFACEMETHOD(get_View)(_COM_Outptr_ wac::ICoreApplicationView** view) override;
        IFACEMETHOD(get_Dispatcher)(_COM_Outptr_ msy::IDispatcherQueue** dispatcherQueue) override;

        IFACEMETHOD(Close)() override;


    private:
        bool m_closed;
        wrl::ComPtr<wac::ICoreApplicationView> m_view;
    };

} }
