// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Private { namespace Infrastructure {

    class DPISetter : public Microsoft::WRL::RuntimeClass<wf::IClosable>
    {
        InspectableClass(L"Private.Infrastructure.DPISetter", TrustLevel::BaseTrust);

    public:
        DPISetter(LUID displayAdapterId, UINT displayAdapterTargetId, test_infra::DisplayDPIRange newDpi);

        IFACEMETHOD(Init)();

        IFACEMETHOD(Close)() override;

        bool DpiHasChanged() const
        {
            return m_newDpi != m_oldDpi;
        }

    private:
        LUID m_displayAdapterId;
        UINT m_displayAdapterTargetId;
        test_infra::DisplayDPIRange m_newDpi = test_infra::DisplayDPIRange::DisplayDPIRange_Default;
        test_infra::DisplayDPIRange m_oldDpi = test_infra::DisplayDPIRange::DisplayDPIRange_Default;
    };

} } // namespace Private::Infrastructure
