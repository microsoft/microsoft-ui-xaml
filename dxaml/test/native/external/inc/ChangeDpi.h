// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <vector>
#include <Closable.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Common {

ref class WaitForDPIChanged sealed
{
public:
    WaitForDPIChanged(Xaml::XamlRoot^ xamlRoot)
    {
        m_xamlRoot = xamlRoot;
        RunOnUIThread([&]()
        {
            m_currentRasterizationScale = m_xamlRoot->RasterizationScale;
            m_xamlRootChangedToken = m_xamlRoot->Changed += 
                    ref new ::Windows::Foundation::TypedEventHandler<Xaml::XamlRoot^, Xaml::XamlRootChangedEventArgs^>(this, &WaitForDPIChanged::OnXamlRootChanged);
        });
    }

    void Wait()
    {
        m_dpiChangedEvent.WaitForDefault();
    }

    virtual ~WaitForDPIChanged()
    {
        RunOnUIThread([&]()
        {
            m_xamlRoot->Changed -= m_xamlRootChangedToken;
        });
    }

private:
    void OnXamlRootChanged(Xaml::XamlRoot^ sender, Xaml::XamlRootChangedEventArgs^ args)
    {
        auto rasterizationScale = sender->RasterizationScale;
        if(rasterizationScale != m_currentRasterizationScale)
        {
            m_currentRasterizationScale = rasterizationScale;
            m_dpiChangedEvent.Set();
        }
    }

    ::Windows::Foundation::EventRegistrationToken m_xamlRootChangedToken;
    Event m_dpiChangedEvent;
    double m_currentRasterizationScale;
    Xaml::XamlRoot^ m_xamlRoot;
};

class ChangeDPI
{
public:
    ChangeDPI(test_infra::DisplayDPIRange dpiOverride = test_infra::DisplayDPIRange::AboveDefaultFirst, Xaml::XamlRoot^ xamlRoot = nullptr)
    {
        if (!xamlRoot)
        {
            RunOnUIThread([&]() {
                auto roots = test_infra::TestServices::WindowHelper->GetXamlRoots();
                xamlRoot = roots->GetAt(0);
            });
        }
        m_xamlRoot = xamlRoot;
        this->dpiOverride = dpiOverride;
        auto qsFilfer = ::Windows::Devices::Display::DisplayMonitor::GetDeviceSelector();
        auto deviceCollectionOp = ::Windows::Devices::Enumeration::DeviceInformation::FindAllAsync(qsFilfer);
        { Event completed; concurrency::create_task(deviceCollectionOp).then([&](::Windows::Devices::Enumeration::DeviceInformationCollection^){completed.Set();}); completed.WaitForDefault(); }
        auto devices = deviceCollectionOp->GetResults();
        for(uint32_t i = 0; i < devices->Size; i++)
        {
            auto device = devices->GetAt(i);
            auto id = device->Id;
            auto getMonitorOp = ::Windows::Devices::Display::DisplayMonitor::FromInterfaceIdAsync(id);
            { Event completed; concurrency::create_task(getMonitorOp).then([&](::Windows::Devices::Display::DisplayMonitor^){completed.Set();}); completed.WaitForDefault(); }
            auto displayMonitor = getMonitorOp->GetResults();
            auto displayAdapterId = displayMonitor->DisplayAdapterId;
            WaitForDPIChanged waitForDPIChanged(m_xamlRoot);
            auto restoreDpi = test_infra::TestServices::Utilities->SetDpi(displayAdapterId, 0, dpiOverride);
            if (restoreDpi!=nullptr)
            {
                auto restoreDpiAsObj = dynamic_cast<Platform::Object^>(restoreDpi);
                wrl::ComPtr<IUnknown> unknown(reinterpret_cast<IUnknown*>(restoreDpiAsObj));
                wrl::ComPtr<ABI::Windows::Foundation::IClosable> closable;
                LogThrow_IfFailed(unknown.As(&closable));
                m_dpiRestoreCollection.push_back(closable.Get());
                waitForDPIChanged.Wait();
            }
        }
    }

    ~ChangeDPI()
    {
        for(auto restoreDpi: m_dpiRestoreCollection)
        {
            WaitForDPIChanged waitForDPIChanged(m_xamlRoot);
            LogThrow_IfFailed(restoreDpi->Close());
            waitForDPIChanged.Wait();
        }
    }

    // Allow this. Used to create & return a TestCleanupWrapper from a helper method.
    ChangeDPI(ChangeDPI&& other) = default;

    // Disallow other copying/moving
    ChangeDPI(const ChangeDPI&) = delete;
    ChangeDPI& operator=(const ChangeDPI&) = delete;
    ChangeDPI& operator=(ChangeDPI&&) = delete;

private:
    std::vector<wrl::ComPtr<ABI::Windows::Foundation::IClosable>> m_dpiRestoreCollection;
    test_infra::DisplayDPIRange dpiOverride;
    Xaml::XamlRoot^ m_xamlRoot;
};

} } } } }
