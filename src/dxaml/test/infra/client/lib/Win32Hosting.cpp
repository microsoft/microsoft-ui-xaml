// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <Win32Hosting.h>
#include <Activation.h>
#include <wil\Result.h>

#include <TestEvent.h>

using namespace WEX::Common;
using namespace WEX::TestExecution;
using namespace Microsoft::WRL;
using namespace Microsoft::UI::Xaml::Tests::Common;

wrl::ComPtr<test_infra::Hosting::IWin32Host> Win32Hosting::StartWin32Host(const wchar_t* hostFactoryType, test_infra::Hosting::DpiAwarenessContext dpiAwarenessContext, bool initCore)
{
    wrl::ComPtr<IInspectable> spInsp;

    wrl::ComPtr<test_infra::ITestHostSettingsStatics> testHostSettings;
    LogThrow_IfFailed(wf::GetActivationFactory(wrl::Wrappers::HStringReference(RuntimeClass_Private_Infrastructure_TestHostSettings).Get(), &testHostSettings));

    wrl::ComPtr<test_infra::Hosting::IWin32HostFactory> spHostFactory;
    LogThrow_IfFailed(testHostSettings->get_Win32HostFactory(&spHostFactory));

    auto exceptionHandlerCallback = wrl::Callback<test_infra::Hosting::IExceptionHandler>(
        [&](HSTRING exceptionString) -> HRESULT
        {
            WEX::Logging::Log::Warning(::WindowsGetStringRawBuffer(exceptionString, nullptr));
            return S_OK;
        });
    LogThrow_IfFailed(spHostFactory->SetExceptionHandler(exceptionHandlerCallback.Get()));

    wrl::ComPtr<wf::IAsyncOperation<IInspectable*>> spAsyncOperation;

    Event hostReadyEvent(L"HostReady");
    auto completionCallback = wrl::Callback<wf::IAsyncOperationCompletedHandler<IInspectable*>>(
        [&](wf::IAsyncOperation<IInspectable*>*, AsyncStatus) -> HRESULT
    {
        hostReadyEvent.Set();
        return S_OK;
    });

    LogThrow_IfFailed(spHostFactory->Create(dpiAwarenessContext, initCore, &spAsyncOperation));

    spAsyncOperation->put_Completed(completionCallback.Get());

    // We need to wait a fairly long time because this may be the first time the CLR is spinning up
    // in the process and loading all our managed test infrastructure code - which can take a while on some slower VMs.
    LOG_OUTPUT(L"Awaiting for host ready event.");
    hostReadyEvent.WaitFor(std::chrono::milliseconds(30000), false /* enforceUnderDebugger*/ );
    LOG_OUTPUT(L"host is now ready.");

    LogThrow_IfFailed(spAsyncOperation->GetResults(&spInsp));

    wrl::ComPtr<test_infra::Hosting::IWin32Host> spWin32Host;
    LogThrow_IfFailed(spInsp.As(&spWin32Host));

    return spWin32Host;
}

wrl::ComPtr<msy::IDispatcherQueue> Win32Hosting::GetDispatcherQueueFromWin32XamlContentRoot(wrl::ComPtr<test_infra::Hosting::IWin32Host> spHost)
{
    wrl::ComPtr<wf::IAsyncOperation<IInspectable*>> spAsyncOperation;
    LogThrow_IfFailed(spHost->GetDispatcherQueue(&spAsyncOperation));

    Event dispatcherReady(L"DispatcherQueueReady");
    auto completionCallback = wrl::Callback<wf::IAsyncOperationCompletedHandler<IInspectable*>>(
        [&](wf::IAsyncOperation<IInspectable*>*, AsyncStatus) -> HRESULT
    {
        dispatcherReady.Set();
        return S_OK;
    });
    spAsyncOperation->put_Completed(completionCallback.Get());
    dispatcherReady.WaitForDefault();

    wrl::ComPtr<IInspectable> spInsp;
    LogThrow_IfFailed(spAsyncOperation->GetResults(&spInsp));

    wrl::ComPtr<msy::IDispatcherQueue> dispatcher;
    LogThrow_IfFailed(spInsp.As(&dispatcher));

    return dispatcher;
}
