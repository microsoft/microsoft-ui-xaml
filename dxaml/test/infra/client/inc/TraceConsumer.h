// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Private { namespace Infrastructure {
    
    const GUID TELEMETRY_GUID = { 0x2dc72f6e, 0xe4d1, 0x5f58, { 0x32, 0x45, 0x09, 0xa4, 0x24, 0x37, 0x99, 0xdd } };
    class TraceConsumerStatics : public wrl::AgileActivationFactory<test_infra::ITraceConsumerStatics>
        {
            InspectableClassStatic(RuntimeClass_Private_Infrastructure_TraceConsumer, TrustLevel::BaseTrust);

        public:

            IFACEMETHOD(StartProvider)(GUID xamlProvider);
            IFACEMETHOD(Start)();
            IFACEMETHOD(Stop)();
            IFACEMETHOD(VerifyEventTraced)(_In_ HSTRING _event, _In_ UINT count);
            IFACEMETHOD(VerifyEventTracedById)(_In_ int eventId, _In_ UINT count);
            IFACEMETHOD(VerifyEventTracedMoreThanOnce)(_In_ int eventId);
            IFACEMETHOD(EnableTracingByEventId)(_In_ int eventId);
        };
    }
}
