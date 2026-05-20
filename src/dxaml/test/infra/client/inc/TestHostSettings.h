// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Private { namespace Infrastructure {
    class TestHostSettingsStatics
        : public wrl::AgileActivationFactory<test_infra::ITestHostSettingsStatics>
    {
        InspectableClassStatic(RuntimeClass_Private_Infrastructure_TestHostSettings, TrustLevel::BaseTrust);

    public:

        IFACEMETHOD(put_Win32HostFactory)(_In_ test_infra::Hosting::IWin32HostFactory* hostFactory) override;
        IFACEMETHOD(get_Win32HostFactory)(_Out_ test_infra::Hosting::IWin32HostFactory** hostFactory) override;

    private:

        static wrl::ComPtr<test_infra::Hosting::IWin32HostFactory> s_spWin32HostFactory;
    };

} }
