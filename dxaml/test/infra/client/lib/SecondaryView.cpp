// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <XamlTailored.h>
#include "SecondaryView.h"
#include <RuntimeEnabledFeaturesEnum.h>
#include <windows.applicationmodel.core.h>
#include "WindowHelper.h"

using namespace WEX::Common;
using namespace WEX::Logging;
using namespace Microsoft::UI::Xaml::Tests::Common;
namespace Private { namespace Infrastructure {

SecondaryView::SecondaryView()
    : m_closed(false)
{
}

SecondaryView::~SecondaryView()
{
    if (!m_closed)
    {
        Close();
    }
}

HRESULT SecondaryView::RuntimeClassInitialize()
{
    COM_START
    {
        wrl::ComPtr<wac::ICoreImmersiveApplication2> coreApplication;

        LogThrow_IfFailed(wf::GetActivationFactory(
            wrl::Wrappers::HStringReference(RuntimeClass_Windows_ApplicationModel_Core_CoreApplication).Get(),
            &coreApplication));

        LogThrow_IfFailed(coreApplication->CreateNewViewFromMainView(&m_view));

    }
    COM_END
}

HRESULT SecondaryView::Close()
{
    COM_START
    {
        if (m_closed) return S_OK;

        m_closed = true;

        wrl::ComPtr<test_infra::ITestServicesStatics> testServicesStatics;
        wrl::ComPtr<test_infra::IWindowHelper> windowHelper;
        LogThrow_IfFailed(wf::GetActivationFactory(
            wrl::Wrappers::HStringReference(RuntimeClass_Private_Infrastructure_TestServices).Get(),
            &testServicesStatics
        ));
        LogThrow_IfFailed(testServicesStatics->get_WindowHelper(&windowHelper));
        LogThrow_IfFailed(windowHelper->BringMainViewToFront());

        auto dispatcher = WindowHelper::GetDispatcherForView(m_view);

        auto closedEvent = std::make_shared<Event>(L"OnClosed");
        auto onClosed = wrl::Callback<wf::ITypedEventHandler<IInspectable*, xaml::WindowEventArgs*>>([&closedEvent](IInspectable*, xaml::IWindowEventArgs*) {
            closedEvent->Set();
            return S_OK;
        });

        wrl::ComPtr<xaml::IWindow> window;
        EventRegistrationToken token = {};
        RunOnDispatcherThread(dispatcher, [&]
        {
            window = WindowHelper::GetXamlWindow();
            LogThrow_IfFailed(window->add_Closed(onClosed.Get(), &token));
            return S_OK;
        });

        RunOnDispatcherThread(dispatcher, [&]
        {
            return window->Close();
        });

        // We can't use any sort of WaitForIdle events here as the window is closed, we can no longer
        // run anything on the dispatcher. For XamlDiagnostics tests, we need to make sure that closing
        // the window will actually result in the visual tree being cleaned up. For that reason, we don't
        // put a specific ResetWindowContentAndWaitForIdle call here, so we'll try waiting for the closed event.
        if (!closedEvent->WaitForNoThrow(std::chrono::milliseconds(2000)))
        {
            LOG_OUTPUT(L"Failed to wait for the closed event");
        }

        window->remove_Closed(token);
    }
    COM_END
}

HRESULT SecondaryView::get_View(_COM_Outptr_ wac::ICoreApplicationView** view)
{
    COM_START
    {
        LogThrow_IfFailed(m_view.CopyTo(view));
    }
    COM_END
}

HRESULT SecondaryView::get_Dispatcher(_COM_Outptr_ msy::IDispatcherQueue** dispatcherQueue)
{
    COM_START
    {
        *dispatcherQueue = WindowHelper::GetDispatcherForView(m_view).Detach();
    }
    COM_END
}

} }
