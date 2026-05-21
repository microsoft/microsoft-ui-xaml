// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Private { namespace Infrastructure {

    class RenderingScopeGuard : public Microsoft::WRL::RuntimeClass<wf::IClosable>
    {
        InspectableClass(L"Private.Infrastructure.RenderingScopeGuard", TrustLevel::BaseTrust);

    public:
        RenderingScopeGuard(
            _In_ test_infra::DCompRendering rendering,
            _In_ BOOLEAN resizeWindow,
            _In_ BOOLEAN injectMockDComp,
            _In_ BOOLEAN resetDevice,
            _In_ BOOLEAN resetWindowContent);

        IFACEMETHOD(Close)() override;

    private:
        wrl::ComPtr<test_infra::ITestServicesStatics> m_testServices;
        wrl::ComPtr<test_infra::IWindowHelper> m_spWindowHelper;
        wrl::ComPtr<test_infra::IUtilities> m_spUtilities;

        test_infra::DCompRendering m_rendering;
        BOOLEAN m_wereDebugTagsEnabled;
        BOOLEAN m_injectMockDComp;
        BOOLEAN m_resetDevice;
        BOOLEAN m_wereWUCShapesEnabled;
        BOOLEAN m_wasSynchronousCompTreeEnabled;
        BOOLEAN m_wasSynchronousCompTreeTestModeEnabled;
    };

} } // namespace Private::Infrastructure
