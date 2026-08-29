// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "HostingDispatcher.h"
#include "WindowHelper.h"
#include "XamlTailored.h"

using namespace WEX::Common;
using namespace WEX::TestExecution;
using namespace Microsoft::WRL;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace ::Private::Infrastructure;

void HostingDispatcher::Init(wrl::ComPtr<msy::IDispatcherQueue> dispatcher, HWND windowHandle)
{
    mainWindowHandle = windowHandle;
    m_dispatcherQueue = dispatcher;
    RunOnUIThread([&] {
        m_uiThreadId = ::GetCurrentThreadId();
    });
}

HWND HostingDispatcher::GetMainWindowHandle() const
{
    return mainWindowHandle;
}

void HostingDispatcher::SetMainWindowHandle(HWND handle)
{
    mainWindowHandle = handle;
}

void HostingDispatcher::DeInit()
{
    m_coreWindow = nullptr;
    m_dispatcherQueue = nullptr;
}

HostingDispatcher* HostingDispatcher::Get()
{
    static HostingDispatcher hostingHelper;
    return &hostingHelper;
}

void HostingDispatcher::SetSecondaryView(wac::ICoreApplicationView* view)
{
    LogThrow_IfFailed(view->get_CoreWindow(&m_secondaryViewCoreWindow));
    wrl::ComPtr<wuc::ICoreWindow5> spCoreWindow5;
    LogThrow_IfFailed(m_secondaryViewCoreWindow.As(&spCoreWindow5));

    RunOnUIThread([&] {
        m_uiThreadId = ::GetCurrentThreadId();
    });
}

void HostingDispatcher::ResetSecondaryView()
{
    m_secondaryViewCoreWindow.Reset();

    RunOnUIThread([&] {
        m_uiThreadId = ::GetCurrentThreadId();
    });
}

wrl::ComPtr<wuc::ICoreWindow> HostingDispatcher::GetCoreWindow()
{
    if (m_secondaryViewCoreWindow != nullptr)
    {
        return m_secondaryViewCoreWindow;
    }
    else
    {
        if (m_coreWindow.Get() == nullptr)
        {
            wrl::ComPtr<wuc::ICoreWindow> coreWindow;
            RunOnUIThread([&] {
                wrl::ComPtr<wuc::ICoreWindowStatic> spCoreWindowStatics;
                LogThrow_IfFailed(wf::GetActivationFactory(wrl_wrappers::HStringReference(L"Windows.UI.Core.CoreWindow").Get(),
                    &spCoreWindowStatics));
                LogThrow_IfFailed(spCoreWindowStatics->GetForCurrentThread(&coreWindow));
            });
            m_coreWindow = coreWindow;
        }
        return m_coreWindow;
    }
}

void HostingDispatcher::SetDispatcher(_In_ msy::IDispatcherQueue* dispatcher)
{
    m_dispatcherQueue = dispatcher;
}

wrl::ComPtr<msy::IDispatcherQueue> HostingDispatcher::GetDispatcher() const
{
    if (m_secondaryViewCoreWindow != nullptr)
    {
        wrl::ComPtr<wuc::ICoreWindow5> spCoreWindow5;
        LogThrow_IfFailed(m_secondaryViewCoreWindow.As(&spCoreWindow5));

        wrl::ComPtr<wsy::IDispatcherQueue> windowsSystemDispatcher;
        LogThrow_IfFailed(spCoreWindow5->get_DispatcherQueue(&windowsSystemDispatcher));

        wrl::ComPtr<msy::IDispatcherQueue> dispatcher;
        RunOnDispatcherThread(windowsSystemDispatcher, [&]()
        {
             //get and store dispatcher queue for the local thread
            wrl::ComPtr<msy::IDispatcherQueueStatics> queueStatics;
            LogThrow_IfFailed(wf::GetActivationFactory(
                wrl::Wrappers::HStringReference(RuntimeClass_Microsoft_UI_Dispatching_DispatcherQueue).Get(),
                &queueStatics));
            LogThrow_IfFailed(queueStatics->GetForCurrentThread(&dispatcher));
        });

        return dispatcher;
    }
    else
    {
        return m_dispatcherQueue;
    }
}
