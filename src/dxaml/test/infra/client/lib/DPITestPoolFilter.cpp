// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "DPITestPoolFilter.h"
#include "XamlTailored.h"
#include <Windows.Devices.Display.h>
#include <Windows.Devices.Enumeration.h>
#include "RpcClient.h"

using namespace WEX::Common;
using namespace WEX::Logging;
using namespace Microsoft::UI::Xaml::Tests::Common;

namespace Private { namespace Infrastructure {

template<typename TDelegateInterface, typename TCallback>
wrl::ComPtr<TDelegateInterface> MakeAgileCallback(TCallback callback) noexcept
{
    return wrl::Callback<wrl::Implements<wrl::RuntimeClassFlags<::Microsoft::WRL::ClassicCom>, TDelegateInterface, wrl::FtmBase>>(callback);
}

template<typename TReturnType, typename THandlerType>
wrl::ComPtr<TReturnType> WaitAndGetResult(wf::IAsyncOperation<THandlerType*>* asyncOperation) noexcept
{
    Event operationCompleteEvent(L"AsyncOpCompleted");
    auto callback = MakeAgileCallback<wf::IAsyncOperationCompletedHandler<THandlerType*>>(
        [&operationCompleteEvent] (wf::IAsyncOperation<THandlerType*>*, wf::AsyncStatus) -> HRESULT
        {
            operationCompleteEvent.Set();
            return S_OK;
        });
    LogThrow_IfFailed(asyncOperation->put_Completed(callback.Get()));
    operationCompleteEvent.WaitForDefault();
    wrl::ComPtr<TReturnType> result;
    LogThrow_IfFailed(asyncOperation->GetResults(&result));
    return result;
}

HRESULT DPITestPoolFilter::Initialize()
{
    wrl::ComPtr<wded::IDisplayMonitorStatics> displayMonitorStatics;
    RETURN_IF_FAILED(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(L"Windows.Devices.Display.DisplayMonitor").Get(),
        &displayMonitorStatics));

    wrl::Wrappers::HString qsFilfer;
    RETURN_IF_FAILED(displayMonitorStatics->GetDeviceSelector(qsFilfer.ReleaseAndGetAddressOf()));

    wrl::ComPtr<wdee::IDeviceInformationStatics> deviceInformationStatics;
    RETURN_IF_FAILED(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(L"Windows.Devices.Enumeration.DeviceInformation").Get(),
        &deviceInformationStatics));
    wrl::ComPtr<wf::IAsyncOperation<wdee::DeviceInformationCollection*>> deviceCollectionOp;
    RETURN_IF_FAILED(deviceInformationStatics->FindAllAsyncAqsFilter(qsFilfer.Get(), &deviceCollectionOp));

    auto devices = WaitAndGetResult<wfc::IVectorView<wdee::DeviceInformation*>, wdee::DeviceInformationCollection>(deviceCollectionOp.Get());

    RpcClientEnsureConnected();

    uint32_t nDevices = 0;
    RETURN_IF_FAILED(devices->get_Size(&nDevices));
    for(uint32_t i=0; i<nDevices; i++)
    {
        int newDpi = 0;
        int oldDpi = 0;
        wrl::ComPtr<wdee::IDeviceInformation> device;
        RETURN_IF_FAILED(devices->GetAt(i, &device));

        wrl::Wrappers::HString id;
        RETURN_IF_FAILED(device->get_Id(id.ReleaseAndGetAddressOf()));
        wrl::ComPtr<wf::IAsyncOperation<wded::DisplayMonitor*>> getMonitorOp;
        RETURN_IF_FAILED(displayMonitorStatics->FromInterfaceIdAsync(id.Get(), &getMonitorOp));
        auto monitor = WaitAndGetResult<wded::IDisplayMonitor, wded::DisplayMonitor>(getMonitorOp.Get());

        wgr::DisplayAdapterId displayAdapterId;
        RETURN_IF_FAILED(monitor->get_DisplayAdapterId(&displayAdapterId));
        LUID luid{ displayAdapterId.LowPart, displayAdapterId.HighPart };

        HRESULT hr = RpcSetDpi(luid, 0, newDpi, &oldDpi);
        if (hr == E_FAIL) {
            // When connected via HyperV Monitor there is an additional adapter that is added to the device that
            // we are unable to set (or even query) the dpi on.  So we will report that we can't query it, but we won't.
            // add the entry to our adapter list so we won't fail later when we try to confirm that the dpi hasn't
            // changed (which it can't because well, this adapter won't let you).  For any test that doesn't actually
            // set the dpi, it doesn't make any difference.  For those that do, the setting of the dpi will throw
            // an error.  In the case that we don't start with the correct dpi, this warning will let us know
            // that we were in an unexpected state when we started.
            Log::Warning(String().Format(L"Unable to validate the initial DPI for all monitor devices/adapters."));
            continue;
        }
        RETURN_IF_FAILED(hr);
        if (oldDpi != 0)
        {
            Log::Warning(String().Format(L"Previous DPI was not the default: 0x%x", oldDpi));
        }

        m_displayAdapters.push_back(luid);
    }

    return S_OK;
}

bool DPITestPoolFilter::IsDirty()
{
    bool dpiChanged = false;

    for(auto displayAdapter: m_displayAdapters)
    {
        int newDpi = 0;
        int oldDpi = 0;
        LogThrow_IfFailed(RpcSetDpi(displayAdapter, 0, newDpi, &oldDpi));
        if (oldDpi!=0)
        {
            dpiChanged = true;
            Log::Error(String().Format(L"Previous DPI was not the default: 0x%x", oldDpi));
        }
    }

    return dpiChanged;
}

} }

