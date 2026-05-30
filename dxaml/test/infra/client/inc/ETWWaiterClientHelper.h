// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Private { namespace Infrastructure {
    
    class ETWWaiterHelperStatics : public wrl::AgileActivationFactory<test_infra::IETWWaiterHelperStatics>
        {
            InspectableClassStatic(RuntimeClass_Private_Infrastructure_ETWWaiterHelper, TrustLevel::BaseTrust);

        public:

            IFACEMETHOD(Start)(GUID providerGuid, unsigned long eventId) override;
            IFACEMETHOD(StartWithTaskName)(GUID providerGuid, HSTRING taskName) override;
            IFACEMETHOD(StartWithPayload)(GUID providerGuid, unsigned long eventId, HSTRING payloadCriteria) override;
            IFACEMETHOD(Wait)(GUID providerGuid, unsigned long eventId, unsigned int timeoutMs) override;
            IFACEMETHOD(WaitForTaskName)(GUID providerGuid, HSTRING taskName, unsigned int timeoutMs) override;
            IFACEMETHOD(Stop)(GUID providerGuid, unsigned long eventId) override;
            IFACEMETHOD(StopTaskName)(GUID providerGuid, HSTRING TaskName) override;
            IFACEMETHOD(GetActiveWaiterCount)(unsigned int *waiterCount) override;
            static HRESULT GetActiveWaiterCountStatic(unsigned int *waiterCount);
        };
    }
}
