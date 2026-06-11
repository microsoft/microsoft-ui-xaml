// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <XamlTailored.h>

#include "IslandHelper.h"
#include "IXamlTestHooks-win.h"
#include "Hosting.h"
#include "RpcClient.h"
#include "HostingDispatcher.h"
#include <microsoft.ui.interop.h>
#include <windowscollections.h>
#include "RuntimeEnabledFeaturesEnum.h"

using namespace WEX::Common;
using namespace Microsoft::UI::Xaml::Tests::Common;

namespace Private::Infrastructure {

wil::unique_handle RunOnNewThread(std::function<void(void)> threadFunc)
{
    struct ThreadFuncWrapper
    {
        std::function<void(void)> m_threadFunc;
    };

    HANDLE handle = CreateThread(nullptr, 0, [](LPVOID param) -> DWORD  {
        ThreadFuncWrapper* wrapper = static_cast<ThreadFuncWrapper*>(param);
        wrapper->m_threadFunc();
        delete wrapper;
        return 0;
     }, new ThreadFuncWrapper {threadFunc}, 0, nullptr);
     return wil::unique_handle{handle};
}

void WaitForSingleObjectWithTimeout(wil::unique_handle& handle)
{
    const DWORD timeoutInMs = ::IsDebuggerPresent() ? INFINITE : 1000 * 60;
    auto result = ::WaitForSingleObject(handle.get(), timeoutInMs);
    if (result != WAIT_OBJECT_0)
    {
        VERIFY_FAIL(L"Timeout hit during WaitForSingleObject");
    }
}

HRESULT IslandHelperStatics::CreateOnNewUIThreadAndNewWindow(DWORD windowClassAtom, _Outptr_ test_infra::IIslandHelper** islandHelper)
{
    Event uiThreadReady(L"");

    HWND hwnd;
    wrl::ComPtr<msy::IDispatcherQueueController> dqc;
    wrl::ComPtr<msy::IDispatcherQueue> dq;
    wrl::ComPtr<xaml::Hosting::IWindowsXamlManager> wxm;
    wrl::ComPtr<xaml::Hosting::IDesktopWindowXamlSource> dwxs;

    auto uiThread = RunOnNewThread([&]()
    {
        VERIFY_SUCCEEDED(::RoInitialize(RO_INIT_SINGLETHREADED));

        ::SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

        hwnd = CreateWindowW(
            MAKEINTATOM(windowClassAtom),
            L"XamlIslandRootsTest Test Window",
            WS_OVERLAPPEDWINDOW,
            0,
            0,
            300 /* width */,
            300 /* height */,
            nullptr /* hwndParent */,
            nullptr /* hMenu */,
            nullptr /* hInstance */,
            nullptr /* lpParam */);
        if (!hwnd)
        {
            DWORD error = !hwnd ? ::GetLastError() : 0;
            LOG_OUTPUT(L"  ui> Failed to create hwnd. CreateWindow error is %u. Atom is %x.", error, windowClassAtom);
        }
        ::ShowWindow(hwnd, SW_SHOW);
        ::SetFocus(hwnd);

        wrl::ComPtr<msy::IDispatcherQueueControllerStatics> dqcStatics;
        LogThrow_IfFailed(wf::GetActivationFactory(wrl::Wrappers::HStringReference(RuntimeClass_Microsoft_UI_Dispatching_DispatcherQueueController).Get(), &dqcStatics));
        LogThrow_IfFailed(dqcStatics->CreateOnCurrentThread(&dqc));

        LogThrow_IfFailed(dqc->get_DispatcherQueue(&dq));

        wrl::ComPtr<xaml::Hosting::IWindowsXamlManagerStatics> wxmStatics;
        LogThrow_IfFailed(wf::GetActivationFactory(wrl::Wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Hosting_WindowsXamlManager).Get(), &wxmStatics));
        LogThrow_IfFailed(wxmStatics->InitializeForCurrentThread(&wxm));

        wrl::ComPtr<xaml::Hosting::IDesktopWindowXamlSourceFactory> dwxsFactory;
        LogThrow_IfFailed(wf::GetActivationFactory(wrl::Wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Hosting_DesktopWindowXamlSource).Get(), &dwxsFactory));
        LogThrow_IfFailed(dwxsFactory->CreateInstance(nullptr, nullptr, &dwxs));

        mu::WindowId windowId;
        LogThrow_IfFailed(GetWindowIdFromWindow(hwnd, &windowId));
        LogThrow_IfFailed(dwxs->Initialize(windowId));

        // Copy the hwnd value for debug output. The value of "hwnd" will change after we destroy the window.
        INT64 window = reinterpret_cast<INT64>(hwnd);
        LOG_OUTPUT(L"  ui> Island created on hwnd %x.", window);

        RpcClientEnsureConnected();
        LogThrow_IfFailed(RpcSetForegroundWindow(reinterpret_cast<LONG_PTR>(hwnd)));

        uiThreadReady.Set();

        // Note: This call lets DispatcherQueue run the message pump and turns this UI thread into a "live app" that
        // responds to input and messages. It exits when we receive the WM_QUIT message posted at the end of the test.
        wrl::ComPtr<msy::IDispatcherQueue> dq;
        LogThrow_IfFailed(dqc->get_DispatcherQueue(&dq));
        wrl::ComPtr<msy::IDispatcherQueue3> dq3;
        LogThrow_IfFailed(dq.As(&dq3));
        dq3->RunEventLoop();

        LOG_OUTPUT(L"  ui> Exiting UI thread for hwnd %I64x.", window);
    });

    uiThreadReady.WaitForDefault();

    return wrl::MakeAndInitialize<IslandHelper>(islandHelper, std::move(uiThread), dqc.Get(), wxm.Get(), hwnd, dq.Get(), dwxs.Get());
}

HRESULT IslandHelperStatics::CreateOnCurrentThreadAndNewWindow(DWORD windowClassAtom, _Outptr_ test_infra::IIslandHelper** islandHelper)
{
    HWND hwnd = CreateWindowW(
        MAKEINTATOM(windowClassAtom),
        L"XamlIslandRootsTest Test Window",
        WS_OVERLAPPEDWINDOW,
        0,
        0,
        300 /* width */,
        300 /* height */,
        nullptr /* hwndParent */,
        nullptr /* hMenu */,
        nullptr /* hInstance */,
        nullptr /* lpParam */);
    if (!hwnd)
    {
        DWORD error = !hwnd ? ::GetLastError() : 0;
        LOG_OUTPUT(L"  ui> Failed to create hwnd. CreateWindow error is %u. Atom is %x.", error, windowClassAtom);
    }
    ::ShowWindow(hwnd, SW_SHOW);
    ::SetFocus(hwnd);

    wrl::ComPtr<msy::IDispatcherQueue> dq;
    wrl::ComPtr<msy::IDispatcherQueueStatics> dqStatics;
    LogThrow_IfFailed(wf::GetActivationFactory(wrl::Wrappers::HStringReference(RuntimeClass_Microsoft_UI_Dispatching_DispatcherQueue).Get(), &dqStatics));
    LogThrow_IfFailed(dqStatics->GetForCurrentThread(&dq));

    wrl::ComPtr<xaml::Hosting::IDesktopWindowXamlSource> dwxs;
    wrl::ComPtr<xaml::Hosting::IDesktopWindowXamlSourceFactory> dwxsFactory;
    LogThrow_IfFailed(wf::GetActivationFactory(wrl::Wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Hosting_DesktopWindowXamlSource).Get(), &dwxsFactory));
    LogThrow_IfFailed(dwxsFactory->CreateInstance(nullptr, nullptr, &dwxs));

    mu::WindowId windowId;
    LogThrow_IfFailed(GetWindowIdFromWindow(hwnd, &windowId));
    LogThrow_IfFailed(dwxs->Initialize(windowId));

    RpcClientEnsureConnected();
    LogThrow_IfFailed(RpcSetForegroundWindow(reinterpret_cast<LONG_PTR>(hwnd)));

    return wrl::MakeAndInitialize<IslandHelper>(islandHelper, nullptr, nullptr, nullptr, hwnd, dq.Get(), dwxs.Get());
}

HRESULT IslandHelperStatics::CreateOnCurrentThreadAndWindow(ABI::Microsoft::UI::WindowId windowId, _Outptr_ test_infra::IIslandHelper** islandHelper)
{
    wrl::ComPtr<msy::IDispatcherQueue> dq;
    wrl::ComPtr<msy::IDispatcherQueueStatics> dqStatics;
    LogThrow_IfFailed(wf::GetActivationFactory(wrl::Wrappers::HStringReference(RuntimeClass_Microsoft_UI_Dispatching_DispatcherQueue).Get(), &dqStatics));
    LogThrow_IfFailed(dqStatics->GetForCurrentThread(&dq));

    wrl::ComPtr<xaml::Hosting::IDesktopWindowXamlSource> dwxs;
    wrl::ComPtr<xaml::Hosting::IDesktopWindowXamlSourceFactory> dwxsFactory;
    LogThrow_IfFailed(wf::GetActivationFactory(wrl::Wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Hosting_DesktopWindowXamlSource).Get(), &dwxsFactory));
    LogThrow_IfFailed(dwxsFactory->CreateInstance(nullptr, nullptr, &dwxs));

    LogThrow_IfFailed(dwxs->Initialize(windowId));

    return wrl::MakeAndInitialize<IslandHelper>(islandHelper, nullptr, nullptr, nullptr, nullptr, dq.Get(), dwxs.Get());
}

HRESULT IslandHelperStatics::CreateWithExistingIsland(_In_ msy::IDispatcherQueue* dq, _Outptr_ test_infra::IIslandHelper** islandHelper)
{
    return wrl::MakeAndInitialize<IslandHelper>(islandHelper, nullptr, nullptr, nullptr, nullptr, dq, nullptr);
}

IslandHelper::~IslandHelper()
{
    switch (m_mode)
    {
        case Mode::UIThreadHwndIsland:
        {
            RunOnIslandUIThread([&]()
            {
                wrl::ComPtr<msy::IDispatcherQueueController2> dqc2;
                LogThrow_IfFailed(m_dispatcherQueueController.As(&dqc2));
                LogThrow_IfFailed(dqc2->ShutdownQueue());
            });

            ::DestroyWindow(m_hwnd);
            ::PostMessage(m_hwnd, WM_QUIT, 0, 0);
            WaitForSingleObjectWithTimeout(m_uiThreadHandle);
            break;
        }

        case Mode::HwndIsland:
        {
            ::DestroyWindow(m_hwnd);
            // Note: Do not post a WM_QUIT message. We only manage one of several hwnds on the UI thread. We don't
            // manage the UI thread, so we shouldn't post a WM_QUIT to stop the UI thread event loop.
            break;
        }

        case Mode::Island:
            // Do nothing. We just have to release the reference on the DWXS.
            break;
    }
}

HRESULT IslandHelper::RuntimeClassInitialize(
    wil::unique_handle uiThreadHandle,
    _In_ msy::IDispatcherQueueController* dqc,
    _In_ xaml::Hosting::IWindowsXamlManager* wxm,
    HWND hwnd,
    _In_ msy::IDispatcherQueue* dq,
    _In_ xaml::Hosting::IDesktopWindowXamlSource* dwxs)
{
    if (uiThreadHandle)
    {
        m_mode = Mode::UIThreadHwndIsland;
    }
    else if (hwnd)
    {
        m_mode = Mode::HwndIsland;
    }
    else if (dwxs)
    {
        m_mode = Mode::Island;
    }
    else
    {
        m_mode = Mode::None;
    }

    m_uiThreadHandle = std::move(uiThreadHandle);
    m_dispatcherQueueController = dqc;
    m_windowsXamlManager = wxm;

    m_hwnd = hwnd;

    m_dispatcherQueue = dq;
    m_desktopWindowXamlSource = dwxs;

    VERIFY_IS_NOT_NULL(m_dispatcherQueue);

    // Primes Xaml to set a "PointerInputReceived" event when it receives pointer input. This lets us know input is
    // ready to be injected. In non-Win32Explicit tests this flag can be primed when we set up WindowHelper, but there's
    // no WindowHelper in Win32Explicit. For now we prime the flag manually in tests that need input. See
    // IslandHelper::EnsureInputReady for more details.
    RunOnIslandUIThread([&]()
    {
        m_uiThreadId = ::GetCurrentThreadId();
        LogThrow_IfFailed(GetTestHooks()->EnablePointerInputEvent());

        bool wasPreviouslyEnabled_dontCare;
        GetTestHooks()->SetRuntimeEnabledFeatureOverride(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnableVisualDebugTags, true, &wasPreviouslyEnabled_dontCare);
    });

    VERIFY_IS_NOT_NULL(m_uiThreadId);

    return S_OK;
}

wrl::ComPtr<IXamlTestHooks> IslandHelper::GetTestHooks()
{
    wrl::ComPtr<xaml::IDxamlCoreTestHooksStatics> testHooksStatics;
    LogThrow_IfFailed(wf::GetActivationFactory(wrl::Wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_DxamlCoreTestHooks).Get(), &testHooksStatics));

    wrl::ComPtr<xaml::IDxamlCoreTestHooks> testHooks;
    RunOnIslandUIThread([&]()
    {
        LogThrow_IfFailed(testHooksStatics->GetForCurrentThread(&testHooks));
    });

    wrl::ComPtr<IXamlTestHooks> xamlTestHooks;
    LogThrow_IfFailed(testHooks.As(&xamlTestHooks));
    return xamlTestHooks;
}

HRESULT IslandHelper::get_DesktopWindowXamlSource(_Outptr_ IInspectable** dwxs)
{
    LogThrow_IfFailed(m_desktopWindowXamlSource->QueryInterface(IID_PPV_ARGS(dwxs)));
    return S_OK;
}

HRESULT IslandHelper::get_DispatcherQueue(_Outptr_ msy::IDispatcherQueue** dq)
{
    LogThrow_IfFailed(m_dispatcherQueue->QueryInterface(IID_PPV_ARGS(dq)));
    return S_OK;
}

HRESULT IslandHelper::GetIslandAndBridge(_In_ xaml::IFrameworkElement* element, BOOLEAN followPopups, _Outptr_opt_ IInspectable** island, _Outptr_opt_ IInspectable** bridge)
{
    return GetTestHooks()->GetIslandAndBridge(element, followPopups, island, bridge);
}

/* static */ void IslandHelper::EnsureInputReady(
    DWORD uiThreadId,
    const wrl::ComPtr<IXamlTestHooks>& testHooks,
    _In_ xaml::IFrameworkElement* element,
    _In_ ICoordinateTransformer* coordinateTransformer)
{
    RpcClientEnsureConnected();

    // We have a timing issue with input in that in order for input to be processed, not only do we need to complete a
    // full dcomp commit on the application side (Lifted IXP) but also on the OS side.  However, we don't have way to
    // wait for the commit to complete on the OS side.  So, to work around this, before we do our initial input
    // following a reset of the of the XAML Environment we will push through some dummy mouse moves until one
    // successfuly completes.

    if (testHooks->CanFirePointerInputEvent()) // todo: In the future this will need to check per-island
    {
        // If we are requesting from the UI thread, then we can't wait for the input to be processed because we will be blocking
        // the UI thread from processing it.  Since not ensuring that input is ready can cause multiple different intermittent errors,
        // we will fail here and have the test owner change the test to inject the input from a different thread (i.e. the test thread).
        VERIFY_ARE_NOT_EQUAL(uiThreadId, ::GetCurrentThreadId(), L"Input injection should not be requested from the UI thread so we may validate that the UI thread is ready for input");

        LOG_OUTPUT(L"> Waiting for pointer input ready");

        Handle pointerInputHandle(OpenNamedEvent(uiThreadId, L"PointerInputReceived"));
        Throw::LastErrorIf(!pointerInputHandle.IsValid(), L"Failed to create PointerInputReceived handle.");

        Throw::LastErrorIf(!::ResetEvent(pointerInputHandle));

        // Move the pointer and see if we get the pointerInputHandle signaled, meaning the input made its way into
        // into XAML and everything is properly set up.
        POINT ptPosition = coordinateTransformer->ComputeScreenCoordinates(element, { 0, 0 });
        bool pointerReady = false;
        for (int i = 0; i < 40; i++) {
            // It is possible that there is a control of some kind in the upper left corner (0,0) of the window.  Some controls
            // have a hover state that persists in the dcomp tree, even after the pointer leaves the control.  In order to
            // keep predictability, we want to try to make sure that our "test" pointer moves don't move the control over anything
            // other than what is in the upper left corner, even if we need to make numerous attempts.  Since most of the time we
            // are able to resolve this check less than 4 attempts, we will just make this move around the 4x4 point box in the upper
            // left corner. No control should be this small if it expects user interaction.  However, we could still have an issue if
            // if a test puts (for example) a 2px padding on the the container that holds the control.
            ptPosition.x +=  (i % 6 < 3 ? 1 : -1);
            ptPosition.y += (i % 6 < 3 ? 1 : -1);
            LogThrow_IfFailed(RpcSendMouseMoveInput(ptPosition));
            DWORD waitResult = ::WaitForSingleObject(pointerInputHandle, 30);
            if (waitResult == WAIT_OBJECT_0)
            {
                pointerReady = true;
                LOG_OUTPUT(L"  > Pointer input ready");
                break;
            }
            LogThrow_LastErrorIfFalse(waitResult == WAIT_TIMEOUT);

            // Typically we are ready in the first few frames.  If at this point we still haven't received any input, then it is possible that shell
            // has left something on the screen that is eating our input.  This is most likely a light dismiss overlay that is left over from some
            // first run or startup process.  We will attempt to get rid of it by sending mouse up.  This should clear any overlay without accidentally
            // invoking some action.
            if (i == 20)
            {
                LOG_OUTPUT(L"  > Attempting to do a mouse up (without a down) to clear overlay");
                LogThrow_IfFailed(RpcSendMouseButtonInput(static_cast<::MouseButton>(test_infra::MouseButton_Left), ptPosition, FALSE /*down*/));
            }
        }
        VERIFY_IS_TRUE(pointerReady, L"Pointer Input Ready");
        testHooks->DisablePointerInputEvent();

        // Send one more pointer move event off of our window.  This is because the system will periodically generate mouse messages at the last mouse
        // position.  This can mess up tests that are injecting input of other types (e.g. pen/touch) and expecting the correct last input type.
        // By moving the mouse to outside of the Xaml surface, then subsequent generated mouse messages won't alter Xaml's last input type.
        // We send the pointer to lower part of the screen which should be the task bar.  Lower left is not valid because shell is putting
        // a widget icon there which pops a large piece of UI on hover that completely covers the XAML area and only dismisses on mouse leave.  Center
        // is iffy because this could hit some icon that pops UI (especially on local vms that might have other windows open).  So we will go
        // with lower right, which is (currently) a "Show Desktop" button that should not have any UI of its own.
        LOG_OUTPUT(L"  > Moving pointer out of XAML window");
        ptPosition.x = GetSystemMetrics(SM_CXSCREEN) - 1;
        ptPosition.y = GetSystemMetrics(SM_CYSCREEN) - 1;
        LogThrow_IfFailed(RpcSendMouseMoveInput(ptPosition));
    }
}

wf::Point IslandHelper::GetCenter(_In_ xaml::IFrameworkElement* pElement)
{
    return GetElementPosition(pElement, 0.5f, 0.5f);
}

wf::Point IslandHelper::GetElementPosition(_In_ xaml::IFrameworkElement* pElement, float fractionOfWidth, float fractionOfHeight)
{
    wf::Point point;

    RunOnIslandUIThread([&]()
    {
        wrl::ComPtr<xaml::IUIElement> spElementAsUIE;
        LogThrow_IfFailed((wrl::ComPtr<xaml::IFrameworkElement>(pElement).As(&spElementAsUIE)));

        double height = 0;
        double width = 0;
        LogThrow_IfFailed(pElement->get_ActualHeight(&height));
        LogThrow_IfFailed(pElement->get_ActualWidth(&width));

        Throw::IfFalse(height != 0.0, E_FAIL, L"Element has no height.");
        Throw::IfFalse(width != 0.0, E_FAIL, L"Element has no width.");

        // Start with the point at the specified fraction into the element, and then
        // transform that to global coordinates.
        point.X = static_cast<float>(width * fractionOfWidth);
        point.Y = static_cast<float>(height * fractionOfHeight);

        wrl::ComPtr<xaml_media::IGeneralTransform> spTransform;
        LogThrow_IfFailed(spElementAsUIE->TransformToVisual(nullptr, &spTransform));
        LogThrow_IfFailed(spTransform->TransformPoint(point, &point));
    });

    return point;
}

HRESULT IslandHelper::LeftMouseClick(_In_ xaml::IFrameworkElement* element)
{
    COM_START_GROUP(L"IslandHelper::LeftMouseClick")
    {
        EnsureInputReady(m_uiThreadId, GetTestHooks(), element, this);

        wf::Point elementCenter = GetCenter(element);
        POINT screenCoordinates = ComputeScreenCoordinates(element, elementCenter);

        LogThrow_IfFailed(RpcInjectPress(InputDevice::Mouse, screenCoordinates, s_defaultClickDurationMs, 1/*pressCount*/, 0 /* pressDelta*/));
    }
    COM_END
}

POINT IslandHelper::ComputeScreenCoordinates(_In_ xaml::IFrameworkElement* elementInIsland, wf::Point point)
{
    wgr::PointInt32 screenCoordinates = {};
    RunOnIslandUIThread([&]()
    {
        // Ensure that we're in DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2. Otherwise ComputeScreenCoordinatesWithPoint
        // will not account for the display scale and we'll inject the wrong coordinates if the scale isn't 1.0.
        DPI_AWARENESS_CONTEXT oldDpiAwarenessContext = ::SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        bool isPerMonV2 = ::AreDpiAwarenessContextsEqual(oldDpiAwarenessContext, DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        Throw::IfFalse(isPerMonV2, E_FAIL, L"Thread DPI awareness needs to be PerMonitorAwareV2 for coordinate conversion to work correctly.");

        wrl::ComPtr<IInspectable> islandAsInspectable;
        LogThrow_IfFailed(GetIslandAndBridge(elementInIsland, false /* followPopups */, &islandAsInspectable, nullptr /* bridge */));

        wrl::ComPtr<ixp::IContentIsland> island;
        LogThrow_IfFailed(islandAsInspectable.As(&island));

        wrl::ComPtr<ixp::IContentCoordinateConverter> coordinates;
        LogThrow_IfFailed(island->get_CoordinateConverter(&coordinates));

        LogThrow_IfFailed(coordinates->ConvertLocalToScreenWithPoint(point, &screenCoordinates));
    });
    return { static_cast<LONG>(screenCoordinates.X), static_cast<LONG>(screenCoordinates.Y) };
}

HRESULT IslandHelper::ThrottleImageTaskDispatcher(BOOLEAN enableThrottling, unsigned int numberOfTasksAllowedToDispatch)
{
    RunOnIslandUIThread([&]()
    {
        GetTestHooks()->ThrottleImageTaskDispatcher(enableThrottling, numberOfTasksAllowedToDispatch);
    });
    return S_OK;
}

HRESULT IslandHelper::SimulateDeviceLost()
{
    RunOnIslandUIThread([&]()
    {
        LogThrow_IfFailed(GetTestHooks()->SimulateDeviceLost());
    });
    return S_OK;
}

HRESULT IslandHelper::MarkDeviceInstanceLost()
{
    RunOnIslandUIThread([&]()
    {
        LogThrow_IfFailed(GetTestHooks()->MarkDeviceInstanceLost());
    });
    return S_OK;
}

HRESULT IslandHelper::GetD3D11GraphicsDeviceAddress(_Out_ INT64* ppCD3D11Device)
{
    RunOnIslandUIThread([&]()
    {
        LogThrow_IfFailed(GetTestHooks()->GetD3D11GraphicsDeviceAddress(ppCD3D11Device));
    });
    return S_OK;
}

HRESULT IslandHelper::GetAllContentIslands(_Outptr_ wfc::IVectorView<IInspectable*>** contentIslands)
{
    wrl::ComPtr<wfci_::Vector<IInspectable*>> vector;
    LogThrow_IfFailed(wfci_::Vector<IInspectable*>::Make(&vector));

    wrl::ComPtr<wfc::IVector<IInspectable*>> contentIslandsVector;
    LogThrow_IfFailed(vector.As(&contentIslandsVector));

    RunOnIslandUIThread([&]()
    {
        LogThrow_IfFailed(GetTestHooks()->GetAllContentIslands(m_desktopWindowXamlSource.Get(), contentIslandsVector.Get()));
    });

    LogThrow_IfFailed(vector->GetView(contentIslands));
    return S_OK;
}

}
