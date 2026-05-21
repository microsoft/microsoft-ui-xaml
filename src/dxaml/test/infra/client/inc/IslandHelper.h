// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ICoordinateTransformer.h"
#include <microsoft.ui.input.h>
#include <Microsoft.UI.Interop.h>

namespace Private { namespace Infrastructure {

    class IslandHelperStatics
        : public wrl::ActivationFactory<test_infra::IIslandHelperStatics>
    {
    public:

        IFACEMETHOD(CreateOnNewUIThreadAndNewWindow)(DWORD windowClassAtom, _Outptr_ test_infra::IIslandHelper** islandHelper);

        IFACEMETHOD(CreateOnCurrentThreadAndNewWindow)(DWORD windowClassAtom, _Outptr_ test_infra::IIslandHelper** islandHelper);

        IFACEMETHOD(CreateOnCurrentThreadAndWindow)(ABI::Microsoft::UI::WindowId windowId, _Outptr_ test_infra::IIslandHelper** islandHelper);

        IFACEMETHOD(CreateWithExistingIsland)(_In_ msy::IDispatcherQueue* dq, _Outptr_ test_infra::IIslandHelper** islandHelper);
    };

    //
    // Helper class for island tests. IslandHelper works in multiple modes:
    //
    //   1. Manages the UI thread, the hwnd, and the Xaml island
    //
    //      This is the most convenient usage for tests that just want to put Xaml in an island. The IslandHelper will
    //      create a UI thread and an hwnd on behalf of the test, initialize the DispatcherQueueController,
    //      WindowXamlManager, and DesktopWindowXamlSource, and is capable of shutting them down automatically when the
    //      IslandHelper is deleted.
    //
    //   2. Uses an existing UI thread Manages an hwnd and the Xaml island
    //
    //      This is for tests that want to manage their own UI thread (and DispatcherQueueController and
    //      WindowsXamlManager). The IslandHelper will create an hwnd on behalf of the test and initialize the
    //      DesktopWindowXamlSource. It will clean up the hwnd and the DWXS automatically, but will not clean up the UI
    //      thread, the DQC, or the WXM.
    //
    //   3. Uses an existing UI thread and hwnd Manages a Xaml island
    //
    //      This is for tests that want to manage both their UI threads and their hwnds. The IslandHelper will only take
    //      care of creating and cleaning up the DesktopWindowXamlSource.
    //
    //   4. Uses an existing UI thread, hwnd, and Xaml island Manages nothing
    //
    //      This is for tests that control absolutely everything, probably to create timing issues during shutdown. The
    //      IslandHelper only does things like hook into Xaml or inject input. It doesn't help startup or shutdown at
    //      all.
    //
    class IslandHelper
        : public wrl::RuntimeClass<test_infra::IIslandHelper>
        , public ICoordinateTransformer
    {
        InspectableClass(RuntimeClass_Private_Infrastructure_IslandHelper, TrustLevel::BaseTrust);

    public:
        HRESULT RuntimeClassInitialize(
            wil::unique_handle uiThreadHandle,
            _In_ msy::IDispatcherQueueController* dqc,
            _In_ xaml::Hosting::IWindowsXamlManager* wxm,
            HWND hwnd,
            _In_ msy::IDispatcherQueue* dq,
            _In_ xaml::Hosting::IDesktopWindowXamlSource* dwxs);

        IFACEMETHOD(get_DesktopWindowXamlSource)(_Outptr_ IInspectable** dwxs) override;

        IFACEMETHOD(get_DispatcherQueue)(_Outptr_ msy::IDispatcherQueue** dq) override;

        IFACEMETHOD(LeftMouseClick)(_In_ xaml::IFrameworkElement* pElement) override;

        IFACEMETHOD(GetIslandAndBridge)(
            _In_ xaml::IFrameworkElement* element,
            BOOLEAN followPopups,
            _Outptr_opt_ IInspectable** island,
            _Outptr_opt_ IInspectable** bridge) override;

        static void EnsureInputReady(
            DWORD uiThreadId,
            const wrl::ComPtr<IXamlTestHooks>& testHooks,
            _In_ xaml::IFrameworkElement* element,
            _In_ ICoordinateTransformer* coordinateTransformer);

        // ICoordinateTransformer
        POINT ComputeScreenCoordinates(_In_ xaml::IFrameworkElement* elementInIsland, wf::Point point) override;

        IFACEMETHOD(ThrottleImageTaskDispatcher)(BOOLEAN enableThrottling, unsigned int numberOfTasksAllowedToDispatch) override;

        IFACEMETHOD(SimulateDeviceLost)() override;

        IFACEMETHOD(MarkDeviceInstanceLost)();

        IFACEMETHOD(GetD3D11GraphicsDeviceAddress)(_Out_ INT64* ppCD3D11Device);

        IFACEMETHOD(GetAllContentIslands)(_Outptr_ wfc::IVectorView<IInspectable*>** contentIslands);

    private:
        enum Mode
        {
            UIThreadHwndIsland,
            HwndIsland,
            Island,
            None
        };

        ~IslandHelper();

        // Note: This class should avoid calling RunOnUIThread, which assumes that there's just one UI thread rather
        // than potentially one per island. This method is provided as a replacement. It uses the m_dispatcherQueue,
        // which exists no matter what mode we're running in.
        template <typename TFunction> void RunOnIslandUIThread(const TFunction& function)
        {
            RunOnDispatcherThread(m_dispatcherQueue, function);
        }

        wrl::ComPtr<IXamlTestHooks> GetTestHooks();

        wf::Point GetCenter(_In_ xaml::IFrameworkElement* pElement);

        wf::Point GetElementPosition(
            _In_ xaml::IFrameworkElement* pElement,
            float fractionOfWidth,
            float fractionOfHeight);

        static const unsigned int s_defaultClickDurationMs = 32;

        Mode m_mode;

        // These exist only for m_mode == UIThreadHwndIsland
        wil::unique_handle m_uiThreadHandle;
        wrl::ComPtr<msy::IDispatcherQueueController> m_dispatcherQueueController;
        wrl::ComPtr<xaml::Hosting::IWindowsXamlManager> m_windowsXamlManager;

        // This exists for m_mode == UIThreadHwndIsland or HwndIsland
        HWND m_hwnd;

        // These always exist
        DWORD m_uiThreadId;
        wrl::ComPtr<msy::IDispatcherQueue> m_dispatcherQueue;
        wrl::ComPtr<xaml::Hosting::IDesktopWindowXamlSource> m_desktopWindowXamlSource;
    };

} }
