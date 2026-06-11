// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

void WaitForDebugger();

namespace Private { namespace Infrastructure {

    class WindowHelperStatics : public wrl::AgileActivationFactory<test_infra::IWindowHelperStatics>
    {
        InspectableClassStatic(RuntimeClass_Private_Infrastructure_WindowHelper, TrustLevel::BaseTrust);
    public:
        WindowHelperStatics()
        {
            WaitForDebugger();
        }
        IFACEMETHOD(WrapInAgileDispatcherQueueHandler)(_In_ test_infra::IManagedDispatcherQueueCallback* callback, _COM_Outptr_ msy::IDispatcherQueueHandler** handler) override;
    };

} }
