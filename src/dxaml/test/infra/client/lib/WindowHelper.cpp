// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <XamlTailored.h>
#include <Handle.h>
#include <windows.ui.viewmanagement.h>
#include <inputpaneinterop.h>

#include "RpcClient.h"
#include "WindowHelper.h"
#include "TestServices.h"
#include "ETWWaiterClientHelper.h"
#include "Utilities.h"
#include "FontHelper.h"
#include "KeyboardHelper.h"
#include <dxgi1_3.h>
#include <dxgi1_2.h>
#include <dxgi.h>
#include <xmllite.h>
#include <stdio.h>
#include <dcomp.h>
#include "IXamlTestHooks-win.h"
#include <windows.applicationmodel.core.h>
#include <corewindow.h>
#include <RuntimeEnabledFeaturesEnum.h>
#include "TlHelp32.h"
#include "SecondaryView.h"
#include "ErrorHandlingHelper.h"

#include "HostingDispatcher.h"
#include "Hosting.h"
#include <PrivateModule.h>

#include <shellscalingapi.h>

// Test pool filters
#include "LayoutExceptionPoolFilter.h"
#include "DragDropTestPoolFilter.h"
#include "FailFastOnErrorsTestPoolFilter.h"
#include "KeyTipTestPoolFilter.h"
#include "KeyPressedTestPoolFilter.h"
#include "DPITestPoolFilter.h"
#include "WuxLoadedTestPoolFilter.h"

#include <wil/safecast.h>

#include "WindowHelperStatics.h"

#include <inputpaneinterop.h>

using namespace WEX::Common;
using namespace WEX::Logging;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace ::Private::Infrastructure::Hosting;

extern "C"
{
    HRESULT StartDetourMockDCompDevice();
    bool IsMockDCompDetourActive();
    HRESULT StopDetourMockDCompDevice();
}

namespace Private { namespace Infrastructure {

bool WindowHelper::s_foregroundWindowCraterArmed = false;
bool WindowHelper::s_isShutdownEnabled = false;

WindowHelper::WindowHelper(DWORD uiThreadId, wrl::ComPtr<test_infra::Hosting::IWin32Host> win32Host, test_infra::ITestServicesStatics* testServices)
    : m_pTestServices(testServices),
      m_win32Host(win32Host),
      m_gccollectCallback(nullptr),
      m_idleSynchronizer(uiThreadId, m_pTestServices, this)
{
    Hosting::HostingMode hostingMode = Hosting::HostingMode::UAP;
    LogThrow_IfFailed(GetHostingMode(&hostingMode));
    if (hostingMode == Hosting::HostingMode::Win32Explicit)
    {
        LOG_OUTPUT(L"Win32Explicit tests should not be touching WindowHelper.");
        LogThrow_IfFailed(E_INVALIDARG);
        return;
    }

    // The order is important, please DO NOT change
    // A change here will change the ExitCode when CraterJupiter is invoked and it will change the failure bucket info.
    // If you need to disable a pool filter, please override IsEnabled()
    REGISTER_TESTPOOLFILTER(KeyboardHelperTestPoolFilter);
    REGISTER_TESTPOOLFILTER(LayoutExceptionPoolFilter);
    REGISTER_TESTPOOLFILTER(DragDropTestPoolFilter);
    REGISTER_TESTPOOLFILTER(FailFastOnErrorsTestPoolFilter);
    REGISTER_TESTPOOLFILTER(DPITestPoolFilter);
    REGISTER_TESTPOOLFILTER(KeyPressedTestPoolFilter);
    REGISTER_TESTPOOLFILTER(KeyTipTestPoolFilter);
    REGISTER_TESTPOOLFILTER(WuxLoadedTestPoolFilter);

    if (testPoolFilters.size()!=NumberOfTestPoolFilters)
    {
        WEX::Logging::Log::Error(L"Registered filters do not match NumberOfTestPoolFilters, please update NumberOfTestPoolFilters in Utilities.h");
    }
}

WindowHelper::~WindowHelper()
{
    Hosting::HostingMode hostingMode = Hosting::HostingMode::UAP;
    LogThrow_IfFailed(GetHostingMode(&hostingMode));
    if (hostingMode == Hosting::HostingMode::WPF)
    {
        auto& moduleRef = PrivateInfraModule::GetModule();
        if (moduleRef.IsFinalRelease())
        {
            // CLR is no longer available, and releasing will hang the process.
            m_win32Host.Detach();
            m_gccollectCallback.Detach();
        }
    }
}

HRESULT WindowHelper::RuntimeClassInitialize()
{
    COM_START_GROUP(L"WindowHelper::RuntimeClassInitialize")
    {
        Hosting::HostingMode hostingMode = Hosting::HostingMode::UAP;
        LogThrow_IfFailed(GetHostingMode(&hostingMode));
        if (hostingMode == Hosting::HostingMode::Win32Explicit)
        {
            LOG_OUTPUT(L"Win32Explicit tests should not be touching WindowHelper.");
            LogThrow_IfFailed(E_INVALIDARG);
            return S_OK;
        }

        RunOnUIThread([this] () {
            for(TestPoolFilter* filter: testPoolFilters)
            {
                const auto hr = filter->Initialize();
                if (FAILED(hr))
                {
                    LOG_WARNING(L"Failed to initialize filter %s with error code 0x%x", filter->GetName(), hr);
                }
            }
        });
    }
    COM_END
}

HRESULT WindowHelper::SetupSimulatedAppPage(xaml_controls::IPage **ppPage)
{
    COM_START_GROUP(L"WindowHelper::SetupSimulatedAppPage")
    {
        // Set up the visual tree structure under the window just like test automation will
        wrl::ComPtr<xaml_controls::IFrame> spFrame;
        LogThrow_IfFailed(wf::ActivateInstance(
            wrl::Wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Controls_Frame).Get(),
            &spFrame));

        wrl::ComPtr<xaml_controls::IContentControl> spRootFrameAsCC;
        LogThrow_IfFailed(spFrame.As(&spRootFrameAsCC));

        wrl::ComPtr<xaml::IUIElement> spRootFrameAsUI;
        LogThrow_IfFailed(spRootFrameAsCC.As(&spRootFrameAsUI));
        WindowHelper::SetWindowContentStatic(spRootFrameAsUI.Get(), m_win32Host);

        wxaml_interop::TypeName typeName;
        typeName.Name = wrl::Wrappers::HStringReference(L"Microsoft.UI.Xaml.Controls.Page").Get();
        typeName.Kind = wxaml_interop::TypeKind_Primitive;

        BOOLEAN didSucceed = FALSE;
        LogThrow_IfFailed(spFrame->Navigate(typeName, nullptr, &didSucceed));

        wrl::ComPtr<IInspectable> spPageAsI;
        LogThrow_IfFailed(spRootFrameAsCC->get_Content(&spPageAsI));
        LogThrow_IfFailed(spPageAsI.CopyTo(ppPage));
    }
    COM_END
}

HRESULT WindowHelper::WaitForIdle()
{
    return WaitForIdle(true /*waitForBuildTreeWork*/);
}

HRESULT WindowHelper::WaitForIdleWithBuildTreeOption(BOOLEAN waitForBuildTreeWork)
{
    return WaitForIdle(!!waitForBuildTreeWork);
}

HRESULT WindowHelper::WaitForIdle(bool waitForBuildTreeWork)
{
    COM_START_GROUP(L"WindowHelper::WaitForIdle")
    {
        m_idleSynchronizer.WaitForIdle(HostingDispatcher::Get()->GetDispatcher().Get(), waitForBuildTreeWork);
        if ( m_win32Host != nullptr )
        {
            LogThrow_IfFailed( m_win32Host->DoEvents() );  // wait for idle dispatcher of WPF
        }
    }
    COM_END
}

HRESULT WindowHelper::WaitForTreeReset()
{
    COM_START
    {
        m_idleSynchronizer.WaitForRootVisualReset();
    }
    COM_END
}

HRESULT WindowHelper::PrepareForPopupMenuWait()
{
    COM_START
    {
        m_idleSynchronizer.PrepareForPopupMenuWait();
    }
    COM_END
}

HRESULT WindowHelper::WaitForPopupMenuCommandInvoked(_In_ UINT32 timeoutMilliseconds, _Out_ BOOLEAN* pSuccess)
{
    COM_START
    {
        *pSuccess = m_idleSynchronizer.WaitForPopupMenuCommandInvoked(std::chrono::milliseconds(timeoutMilliseconds));
    }
    COM_END
}

HRESULT WindowHelper::Popup_SetWindowed(
    _In_ xaml_primitives::IPopup* pPopup)
{
    COM_START_GROUP(L"WindowHelper::Popup_SetWindowed")
    {
        RunOnUIThread([&] () {
            wrl::ComPtr<wf::IPropertyValueStatics> spPropertyValueFactory;
            wrl::ComPtr<wf::IPropertyValue> spBooleanValue;
            wrl::ComPtr<xaml_primitives::IPopup> spPopup(pPopup);
            wrl::ComPtr<xaml::IDependencyObject> spPopupAsDO;

            LogThrow_IfFailed(spPopup.As(&spPopupAsDO));

            LogThrow_IfFailed(WindowHelper::GetTestHooks()->InvokeInternalCommand(
                spPopupAsDO.Get(),
                XamlInternalCommand::Popup_SetWindowed,
                nullptr /* pArgs */,
                nullptr /* ppReturnValue */));
        });
    }
    COM_END
}

HRESULT WindowHelper::FlyoutBase_GetWindow(
    _In_ xaml_primitives::IFlyoutBase* pFlyoutBase, _Out_ UINT64* hwnd)
{
    COM_START
    {
        RunOnUIThread([&]() {
            std::vector<wf::IPropertyValue*> argsOut;
            wrl::ComPtr<xaml_primitives::IFlyoutBase> spFlyoutBase(pFlyoutBase);
            wrl::ComPtr<xaml::IDependencyObject> spFlyoutBaseAsDO;
            wrl::ComPtr<wf::IPropertyValue> spRetVal;

            LogThrow_IfFailed(spFlyoutBase.As(&spFlyoutBaseAsDO));

            LogThrow_IfFailed(WindowHelper::GetTestHooks()->InvokeInternalCommand(
                spFlyoutBaseAsDO.Get(),
                XamlInternalCommand::FlyoutBase_GetWindow,
                nullptr, &spRetVal));

            uint64_t handle = 0;
            spRetVal->GetUInt64(&handle);
            *hwnd = handle;
        });
    }
    COM_END
}

HRESULT WindowHelper::Popup_GetWindow(
    _In_ xaml_primitives::IPopup* pPopup, _Out_ UINT64* hwnd)
{
    COM_START
    {
        RunOnUIThread([&]() {
            std::vector<wf::IPropertyValue*> argsOut;
            wrl::ComPtr<xaml_primitives::IPopup> spPopup(pPopup);
            wrl::ComPtr<xaml::IDependencyObject> spPopupAsDO;
            wrl::ComPtr<wf::IPropertyValue> spRetVal;

            LogThrow_IfFailed(spPopup.As(&spPopupAsDO));

            LogThrow_IfFailed(WindowHelper::GetTestHooks()->InvokeInternalCommand(
                spPopupAsDO.Get(),
                XamlInternalCommand::Popup_GetWindow,
                nullptr, &spRetVal));

            uint64_t handle = 0;
            spRetVal->GetUInt64(&handle);
            *hwnd = handle;
        });
    }
    COM_END
}

HRESULT WindowHelper::ToolTip_GetWindow(
    _In_ xaml_controls::IToolTip* pToolTip, _Out_ UINT64* hwnd)
{
    COM_START
    {
        RunOnUIThread([&]() {
        std::vector<wf::IPropertyValue*> argsOut;
        wrl::ComPtr<xaml_controls::IToolTip> spToolTip(pToolTip);
        wrl::ComPtr<xaml::IDependencyObject> spToolTipAsDO;
        wrl::ComPtr<wf::IPropertyValue> spRetVal;

        LogThrow_IfFailed(spToolTip.As(&spToolTipAsDO));

        LogThrow_IfFailed(WindowHelper::GetTestHooks()->InvokeInternalCommand(
            spToolTipAsDO.Get(),
            XamlInternalCommand::ToolTip_GetWindow,
            nullptr, &spRetVal));

        uint64_t handle = 0;
        spRetVal->GetUInt64(&handle);
        *hwnd = handle;
    });
    }
    COM_END
}

HRESULT WindowHelper::FrameworkElement_HasPeer(
    _In_ xaml::IFrameworkElement* pFrameworkElement,
    _In_ HSTRING name,
    _Out_ BOOLEAN *pHasPeer)
{
    COM_START
    {
        RunOnUIThread([&] () {

            std::vector<wf::IPropertyValue*> args;
            wrl::ComPtr<wf::IPropertyValueStatics> spPropertyValueFactory;
            wrl::ComPtr<wf::IPropertyValue> spStringValue;
            wrl::ComPtr<xaml::IFrameworkElement> spFrameworkElement(pFrameworkElement);
            wrl::ComPtr<xaml::IDependencyObject> spFrameworkElementAsDO;
            wrl::ComPtr<wf::IPropertyValue> spReturnValue;

            LogThrow_IfFailed(spFrameworkElement.As(&spFrameworkElementAsDO));

            LogThrow_IfFailed(wf::GetActivationFactory(
                wrl::Wrappers::HStringReference(RuntimeClass_Windows_Foundation_PropertyValue).Get(),
                &spPropertyValueFactory));
            LogThrow_IfFailed(spPropertyValueFactory->CreateString(name, &spStringValue));
            args.push_back(spStringValue.Get());

            LogThrow_IfFailed(WindowHelper::GetTestHooks()->InvokeInternalCommand(
                spFrameworkElementAsDO.Get(),
                XamlInternalCommand::FrameworkElement_HasPeer,
                &args,
                &spReturnValue));
            LogThrow_IfFailed(spReturnValue->GetBoolean(pHasPeer));
        });
    }
    COM_END
}


HRESULT WindowHelper::SynchronouslyTickUIThread(unsigned int ticks)
{
    COM_START
    {
        m_idleSynchronizer.SynchronouslyTickUIThread(ticks);
    }
    COM_END
}

HRESULT WindowHelper::SetPostTickCallback(_In_opt_ test_infra::IPostTickCallback* callback)
{
    COM_START
    {
        RunOnUIThread([&]()
        {
            m_spPostTickCallback = callback;
            GetTestHooks()->SetPostTickCallback(callback ? std::function<void()>(std::bind(&WindowHelper::PostTickCallbackWrapper, this)) : nullptr);
        });
    }
    COM_END
}

void WindowHelper::PostTickCallbackWrapper()
{
    if (m_spPostTickCallback)
    {
        m_spPostTickCallback->Invoke();
    }
}

HRESULT WindowHelper::SetPlayingSoundNodeCallback(_In_opt_ test_infra::IPlayingSoundNodeCallback* callback)
{
    COM_START
    {
        RunOnUIThread([&]()
        {
            m_spPlayingSoundNodeCallback = callback;
            GetTestHooks()->SetPlayingSoundNodeCallback(callback ? std::function<void(xaml::ElementSoundKind, BOOLEAN isSpatialAudio, float x, float y, float z, double volume)>(std::bind(&WindowHelper::PlayingSoundNodeCallbackWrapper, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6)) : nullptr);
        });
    }
    COM_END
}

HRESULT WindowHelper::TestGetActualToolTip(
    _In_ xaml::IUIElement* element,
    _Outptr_ xaml_controls::IToolTip** ppValue)
{
    COM_START
    {
        RunOnUIThread([&]()
        {
            GetTestHooks()->TestGetActualToolTip(element, ppValue);
        });
    }
    COM_END
}

HRESULT WindowHelper::GetAnimatedTranslation(_In_ xaml::IUIElement* element, _Out_ wfn::Vector3* translation)
{
    COM_START
    {
        RunOnUIThread([&]()
        {
            GetTestHooks()->GetAnimatedTranslation(element, translation);
        });
    }
    COM_END
}

HRESULT WindowHelper::GetAnimatedRotation(_In_ xaml::IUIElement* element, _Out_ float* rotation)
{
    COM_START
    {
        RunOnUIThread([&]()
        {
            GetTestHooks()->GetAnimatedRotation(element, rotation);
        });
    }
    COM_END
}

HRESULT WindowHelper::GetAnimatedScale(_In_ xaml::IUIElement* element, _Out_ wfn::Vector3* scale)
{
    COM_START
    {
        RunOnUIThread([&]()
        {
            GetTestHooks()->GetAnimatedScale(element, scale);
        });
    }
    COM_END
}

HRESULT WindowHelper::GetAnimatedTransformMatrix(_In_ xaml::IUIElement* element, _Out_ wfn::Matrix4x4* transformMatrix)
{
    COM_START
    {
        RunOnUIThread([&]()
        {
            GetTestHooks()->GetAnimatedTransformMatrix(element, transformMatrix);
        });
    }
    COM_END
}

HRESULT WindowHelper::GetAnimatedRotationAxis(_In_ xaml::IUIElement* element, _Out_ wfn::Vector3* rotationAxis)
{
    COM_START
    {
        RunOnUIThread([&]()
        {
            GetTestHooks()->GetAnimatedRotationAxis(element, rotationAxis);
        });
    }
    COM_END
}

HRESULT WindowHelper::GetAnimatedCenterPoint(_In_ xaml::IUIElement* element, _Out_ wfn::Vector3* centerPoint)
{
    COM_START
    {
        RunOnUIThread([&]()
        {
            GetTestHooks()->GetAnimatedCenterPoint(element, centerPoint);
        });
    }
    COM_END
}

HRESULT WindowHelper::WaitForAnimatedFacadePropertyChanges(int count)
{
    COM_START
    {
        RunOnUIThread([&]()
        {
            GetTestHooks()->ScheduleWaitForAnimatedFacadePropertyChanges(count);
        });
        m_idleSynchronizer.WaitForAnimatedFacadePropertyChangesComplete();
    }
    COM_END
}

HRESULT WindowHelper::GetUIAWindow(_Outptr_ IInspectable** uiaWindow_IRawElementProviderFragment)
{
    COM_START
    {
        RunOnUIThread([&]()
        {
            HWND hwnd = GetCurrentWindowHandle();

            wrl::ComPtr<xaml::IUIElement> spCurrentRoot;
            WindowHelper::GetWindowContentStatic(&spCurrentRoot, m_win32Host);
            wrl::ComPtr<xaml::IDependencyObject> elementDO;
            spCurrentRoot.As(&elementDO);
            // The test hook method returns the UIAWindow without a reference added. Add one manually.
            IInspectable* uiaWindowNoRef;
            GetTestHooks()->GetUIAWindow(elementDO.Get(), hwnd, &uiaWindowNoRef);
            *uiaWindow_IRawElementProviderFragment = uiaWindowNoRef;
            (*uiaWindow_IRawElementProviderFragment)->AddRef();
        });
    }
    COM_END
}

HRESULT WindowHelper::ApplyElevationEffect(
    _In_ xaml::IUIElement* element,
    UINT depth)
{
    COM_START
    {
        RunOnUIThread([&]()
        {
            GetTestHooks()->ApplyElevationEffect(element, depth);
        });
    }
    COM_END
}

void WindowHelper::PlayingSoundNodeCallbackWrapper(xaml::ElementSoundKind sound, BOOLEAN isSpatialAudio, float x, float y, float z, double volume)
{
    if (m_spPlayingSoundNodeCallback)
    {
        m_spPlayingSoundNodeCallback->Invoke(sound, isSpatialAudio, x, y, z, volume);
    }
}

HRESULT WindowHelper::put_WindowContent(xaml::IUIElement* pElement)
{
    COM_START_GROUP(L"WindowHelper::put_WindowContent")
    {
        WindowHelper::SetWindowContentStatic(pElement, m_win32Host);
    }
    COM_END
}

HRESULT WindowHelper::get_WindowContent(xaml::IUIElement** ppElement)
{
    COM_START
    {
        WindowHelper::GetWindowContentStatic(ppElement, m_win32Host);
    }
    COM_END
}

HRESULT WindowHelper::get_WindowBounds(wf::Rect* bounds)
{
    COM_START
    {
        WindowHelper::GetWindowBoundsStatic(bounds);
    }
    COM_END
}

HRESULT WindowHelper::get_VisibleBounds(wf::Rect* bounds)
{
    COM_START
    {
        wrl::ComPtr<xaml::IUIElement> spCurrentRoot;
        WindowHelper::GetWindowContentStatic(&spCurrentRoot, m_win32Host);
        WindowHelper::GetVisibleBoundsStatic(bounds, spCurrentRoot.Get());
    }
    COM_END
}

HRESULT WindowHelper::get_MonitorBounds(wf::Rect* bounds)
{
    const HWND appWindow = GetCurrentWindowHandle();

    HMONITOR hMonitor = MonitorFromWindow(appWindow, MONITOR_DEFAULTTONEAREST);
    if (hMonitor)
    {
        MONITORINFO monitorInfo = { sizeof(MONITORINFO) };
        if (GetMonitorInfo(hMonitor, &monitorInfo))
        {
            const RECT monitorScreenPhysical = monitorInfo.rcWork;
            *bounds = {
                static_cast<float>(monitorScreenPhysical.left),
                static_cast<float>(monitorScreenPhysical.top),
                static_cast<float>(monitorScreenPhysical.right - monitorScreenPhysical.left),
                static_cast<float>(monitorScreenPhysical.bottom - monitorScreenPhysical.top)
            };
            return S_OK;
        }
    }
    return E_FAIL;
}

HRESULT WindowHelper::ShowWindow()
{
    COM_START
    {
        WindowHelper::ShowWindowStatic();
    }
    COM_END
}

HRESULT WindowHelper::HideWindow()
{
    COM_START
    {
        WindowHelper::HideWindowStatic();
    }
    COM_END
}

HRESULT WindowHelper::MoveWindow(int x, int y, int width, int height)
{
    COM_START_GROUP(L"WindowHelper::MoveWindow")
    {
        WindowHelper::MoveWindowStatic(x, y, width, height);
    }
    COM_END
}

HRESULT WindowHelper::GetCurrentWindowScale(float* scale)
{
    COM_START_GROUP(L"WindowHelper::GetCurrentWindowScale")
    {
        RunOnUIThread([scale] () {
            wrl::ComPtr<IXamlTestHooks> spTestHooks = GetTestHooks();

            *scale = spTestHooks->GetZoomScale();
        });
    }
    COM_END
}

// Resets the root of the tree. Xaml will post itself a message to
// handle the full effects of changing the root, so wait for Xaml to
// complete that and go idle. Used to reset Xaml's state at the end
// of each test method.
HRESULT WindowHelper::ResetWindowContentAndWaitForIdle()
{
    return ResetWindowContentAndScaleWaitForIdle(0.0f);
}

// Resets the root of the tree. Xaml will post itself a message to
// handle the full effects of changing the root, so wait for Xaml to
// complete that and go idle. Used to reset Xaml's state at the end
// of each test method.
HRESULT WindowHelper::ResetWindowContentAndScaleWaitForIdle(float scale)
{
    COM_START_GROUP(L"WindowHelper::ResetWindowContentAndScaleWaitForIdle")
    {
        bool windowContentCleared = false;

        Hosting::HostingMode hostingMode = Hosting::HostingMode::UAP;
        LogThrow_IfFailed(Hosting::GetHostingMode(&hostingMode));

        RunOnUIThread([&]() {
            wrl::ComPtr<xaml::IUIElement> spCurrentRoot;

            GetTestHooks()->CloseAllPopupsForTreeReset();

            WindowHelper::GetWindowContentStatic(&spCurrentRoot, m_win32Host);
            if (spCurrentRoot != nullptr)
            {
                // This is not necessarily the best place for this, but it seems the least worst option.  Because we want
                // to start tests off with a clean slate we need to reset the last input state.  This means we need someplace
                // that is run between all (or most all) tests and that has access to content.
                //
                // So we will set our last input back to none here before we clear the content, so the next test (if there
                // is one) won't have some left over value.
                {
                    wrl::ComPtr<xaml::IXamlRoot> xamlRoot;
                    LogThrow_IfFailed(spCurrentRoot->get_XamlRoot(&xamlRoot));
                    LogThrow_IfFailed(SetLastInputMethod(test_infra::LastInputDeviceType_None, xamlRoot.Get()));
                }

                WindowHelper::SetWindowContentStatic(nullptr, m_win32Host);
                windowContentCleared = true;
            }

            // Reseting the content will tear down the visuals on the system compositor side as well
            // so next time we go to do input we need to ensure that new system visuals have been
            // created and we are ready for input.
            LogThrow_IfFailed(GetTestHooks()->EnablePointerInputEvent());
        });

        // If the Window content is null WaitForIdle will wait indefinitely. The pre-frame
        // callback doesn't occur if the window content is null because Jupiter skips
        // the layout pass entirely.
        if (windowContentCleared)
        {
            // Some controls have phased work queues and other non-obvious elements inside
            // Jupiter that will persist after we've released them. This callout gives Jupiter
            // a chance to teardown and release any objects that belong to the test DLL that
            // is about to get unloaded.
            LogThrow_IfFailed(WaitForIdle());
        }

        // do a GC collect on managed code
        LogThrow_IfFailed( GCCollect() );

        RunOnUIThread([this, scale] () {
            wrl::ComPtr<IXamlTestHooks> spTestHooks = GetTestHooks();

            spTestHooks->CleanupReleaseQueue();

            // Check to see if there is a drag/drop operation in progress
            if (spTestHooks->IsDragDropInProgress())
            {
                Log::Error(L"Drag/Drop Operation still in progress at cleanup");
            }

            LOG_OUTPUT(L"WindowHelper::ResetWindowContentAndScaleWaitForIdle: Setting scale to: %lf",scale);
            // Set the WindowSize to 0,0, which is a special size that tells Jupiter to
            // no longer override WindowSize.
            LogThrow_IfFailed(WindowHelper::SetHostSizeOverride(
                wf::Size(),
                wf::Rect(),
                scale,
                false
            ));

            // Clear any simulated SIP occluded rect.
            // TODO: XamlRoot appears not to be deleted when window content is cleared above.
            // One might think this would be a simple problem to fix - just clear the SIP rect from the XamlRoot here,
            // but unfortunately if we attempt to get a XamlRoot from here, the test framework will determine that
            // there is a memory leak in a seemingly unrelated section of code.

            // Restore theming to our default state.
            LogThrow_IfFailed(WindowHelper::ResetThemingToDefaultState());
        });
        LogThrow_IfFailed(WindowHelper::CancelAllConnectedAnimationsAndResetDefaults());

        BOOLEAN isOneCore = false;
        LogThrow_IfFailed(Utilities::IsOneCoreStatic(&isOneCore));
        if (!isOneCore)
        {
            LogThrow_IfFailed(WindowHelper::RestoreForegroundWindow());
        }

        if (hostingMode != Hosting::HostingMode::UAP || Utilities::IsBVT())
        {
            RpcClientEnsureConnected();
            LogThrow_IfFailed(RpcResetInputInjection());
        }
    }
    COM_END
}

HRESULT WindowHelper::WaitForImplicitShowHideComplete()
{
    COM_START
    {
        m_idleSynchronizer.WaitForImplicitShowHideComplete();
    }
    COM_END
}

// Makes sure Jupiter is the foreground window.
HRESULT WindowHelper::RestoreForegroundWindow()
{
    COM_START_GROUP(L"WindowHelper::RestoreForegroundWindow")
    {
        HWND appHandle = GetCurrentWindowHandle();
        HWND parentHandle = ::GetParent(appHandle);
        Hosting::HostingMode hostingMode = Hosting::HostingMode::UAP;
        LogThrow_IfFailed(GetHostingMode(&hostingMode));
        if (parentHandle == NULL && hostingMode != Hosting::HostingMode::UAP)
        {
            parentHandle = appHandle;
        }

        LogThrow_IfFailed(RpcSetForegroundWindow(reinterpret_cast<LONG_PTR>(parentHandle)));
    }
    COM_END
}

// Maximizes the desktop window hosting the Xaml application
HRESULT WindowHelper::MaximizeDesktopWindow()
{
    COM_START_GROUP(L"WindowHelper::MaximizeDesktopWindow")
    {
        HWND appHandle = GetCurrentWindowHandle();
        Hosting::HostingMode hostingMode = Hosting::HostingMode::UAP;
        LogThrow_IfFailed(GetHostingMode(&hostingMode));
        HWND hwndParent = ::GetParent(appHandle);
        if (hwndParent == NULL && (hostingMode != Hosting::HostingMode::UAP))
        {
            hwndParent = appHandle;
        }

        LogThrow_IfFailed(RpcMaximizeDesktopWindow(reinterpret_cast<LONG_PTR>(hwndParent)));
    }
    COM_END
}

// Verifies that a test has cleaned up correctly.
HRESULT WindowHelper::VerifyTestCleanup()
{
    COM_START_GROUP(L"WindowHelper::VerifyTestCleanup")
    {
         Hosting::HostingMode hostingMode = Hosting::HostingMode::UAP;
        LogThrow_IfFailed(GetHostingMode(&hostingMode));

        // TODO: Enable leak detection when in Win32 hosting modes
        if (IsLeakDetectionEnabled() && hostingMode == HostingMode::UAP)
        {
            RunOnUIThread([] () {
                // Be a plumber
                ErrorHandlingHelper::PerformLeakDetection();
            });
        }
        else
        {
            LOG_OUTPUT(L"> VerifyTestCleanup: SKIPPING leak detection for this test.");
        }

        // Leaving UI content behind is something a test shouldn't do and for a lot of controls
        // can indicate a serious error.
        RunOnUIThread([&]() {
            xaml::IUIElement *pCurrentRoot = nullptr;

            WindowHelper::GetWindowContentStatic(&pCurrentRoot, m_win32Host);
            if (pCurrentRoot != nullptr)
            {
                Log::Warning(L"The window content was not cleared properly at the end of the test.");
            }
        });

        unsigned int countofWaiters = 0;
        ETWWaiterHelperStatics::GetActiveWaiterCountStatic(&countofWaiters);
        Throw::IfFalse(countofWaiters == 0, E_NOTIMPL, L"Active ETWWaiterProxy objects were not cleared!");

        // In the case of open popups or the input pane still being open there isn't much we can
        // do to recover Jupiter. In these cases to preserve the of the rest of the test
        // run we'll intentionally crater Jupiter. TAEF will, upon encountering a structured
        // exception, terminate the hosting process and restart it. There is no other way to ask
        // TAEF to do this in the cleanup method.
        CraterJupiterErrorCode craterJupiterError = {};

        if (hostingMode == Hosting::HostingMode::UAP)
        {
            RunOnUIThread([&craterJupiterError]() {
                // Open popups will be a hard error. We can set Open = false but if there's
                // an open popup it's almost always from a control or other element that hasn't
                // been fully cleaned up and not from the test itself.
                wrl::ComPtr<wfc::IVectorView<xaml_primitives::Popup*>> spPopups;
                WindowHelper::GetOpenPopupsStatic(&spPopups);
                unsigned int count = 0;
                LogThrow_IfFailed(spPopups->get_Size(&count));
                craterJupiterError.Error.Flags.PopupOpen = count != 0;
                if (craterJupiterError.Error.Flags.PopupOpen)
                {
                    Log::Error(L"Open popups were left on the screen.");
                }
            });
        }

        RunOnUIThread([this, hostingMode, &craterJupiterError] () {
            auto testHooks = GetTestHooks();
            wrl::ComPtr<wfc::IVectorView<IInspectable*>> queue;
            LogThrow_IfFailed( testHooks->GetFinalReleaseQueue(&queue) );
            unsigned int count = 0;
            LogThrow_IfFailed( queue->get_Size( &count ) );
            craterJupiterError.Error.Flags.FinalReleaseQueueNotEmpty = (count > 0);
            if (craterJupiterError.Error.Flags.FinalReleaseQueueNotEmpty)
            {
                // Vector of class names of objects which were not cleaned up
                std::vector<std::wstring> objNames;
                for( unsigned int i = 0; i < count; i++ )
                {
                    wrl::ComPtr<IInspectable> obj;
                    LogThrow_IfFailed(queue->GetAt(i, &obj));
                    wrl::Wrappers::HString className;
                    LogThrow_IfFailed(obj->GetRuntimeClassName( className.ReleaseAndGetAddressOf() ));
                    std::wstring strName( ::WindowsGetStringRawBuffer(className.Get(), 0) );
                    objNames.push_back( strName );
                }

                // Creating a sorted(for consistent signature) comma separated string of class names of objects which didn't get cleaned up in final release queue
                // for a better error messages. It will be very helpful in debugging.
                std::sort( objNames.begin(), objNames.end() );
                std::wstring errorSignature;
                for ( auto& name: objNames)
                {
                    errorSignature += name + L",";
                }

                // The problem with the final release queue not being empty is that some custom DO could be hanging out in there
                // whose destructor is an DLL that TAEF has already unloaded before the queue gets cleaned up. Now that we manually
                // clear out the release queue after every test, we should never see this. However, we'll keep it just in case someone
                // does some very wonky stuff that breaks this.
                std::wstring errorString(L"The final release queue is not empty:");
                errorString += errorSignature;

                if (Utilities::IsBVT() && hostingMode != HostingMode::UAP)
                {
                    Log::Warning( errorString.c_str() );
                    craterJupiterError.Error.Flags.FinalReleaseQueueNotEmpty = false;
                }
                else
                {
                    Log::Error( errorString.c_str() );
                }
            }
        });

        BOOLEAN isOneCore = FALSE;
        LogThrow_IfFailed(Utilities::IsOneCoreStatic(&isOneCore));

        // Restoring the foreground window also doesn't work on OneCore.
        if (!Utilities::IsBVT() && !isOneCore)
        {
            // If neither Jupiter or ApplicationFrameHost are the active foreground
            // window at the end of the test we've likely clicked outside the bounds of
            // the test window and some other window is in the front, ready to ruin input
            // injection and fail many other tests.

            // We'll first attempt to restore the foreground window, and if that fails, we'll crater Jupiter.
            if (!WindowHelper::IsForegroundWindowStatic())
            {
                LogThrow_IfFailed(WindowHelper::RestoreForegroundWindow());
            }

            // Check that the window check is enabled since IsForegroundWindowStatic will output
            // the current windows
            if (s_foregroundWindowCraterArmed && !WindowHelper::IsForegroundWindowStatic())
            {
                craterJupiterError.Error.Flags.NoLongerForeground = true;
                Log::Error(L"Jupiter is no longer the foreground window.");
            }
        }

        size_t filterIndex = 0;
        for(TestPoolFilter* filter: testPoolFilters)
        {
            if (filter->IsEnabled())
            {
                bool poolDirty = filter->IsDirty();
                if (poolDirty)
                {
                    craterJupiterError.Error.Flags.TestPoolDirty |= 1 << filterIndex;
                    Log::Error(WEX::Common::String().Format(L"Pool is dirty!  Detected by %s.",  filter->GetName()));
                }
            }
            filterIndex++;
        }

        const bool testFontCollectionDeployed = !Utilities::IsBVT(); // BVTs don't deploy the custom font collection
        if (testFontCollectionDeployed)
        {
           BOOLEAN isCustomSystemFontCollectionInUse = false;
           FontHelper::IsCustomSystemFontCollectionInUseStatic(&isCustomSystemFontCollectionInUse);
           if (!isCustomSystemFontCollectionInUse)
           {
               craterJupiterError.Error.Flags.CustomSystemFontCollectionInUse = true;
           }
           // Never allow a test to exit with non-default font scale.
           if (FontHelper::GetFontScaleStatic() != 1.0f)
           {
               craterJupiterError.Error.Flags.UsingNonDefaultFontScale = true;
           }
        }

        // Briefly inject MockDComp to cause DComp device recreation and comp object leak detection via DebugDeviceFinalReleaseAsserter
        // The asserter runs when the old DComp device is being cleaned up, and verifies that it experiences its final release
        // (see DebugDeviceFinalReleaseAsserter::ReleaseAllWithAssert()). If it doesn't, most likely some other composition object
        // is holding a references to the device has been leaked. In addition to leaking resources, this situation can also cause
        // failures for subsequent tests running  in the same window, as we're not able to properly hook up the new device while
        // uncleaned remnants of the old one remain.
        if (Utilities::IsCompLeakDetectionEnabled())
        {
            // TODO: This should be a message rather than warning.
            Log::Warning(L"Briefly inject MockDComp to trigger check for composition leaks...");

            // If we are doing comp leak detection MockDComp should be disabled up to this point
            _ASSERT(Utilities::GetIsMockDCompDisabledForCompLeakDetection() == true);

            Utilities::SetIsMockDCompDisabledForCompLeakDetection(false);
            InjectMockDComp();      // Comp object leak validation occurs here, via DebugDeviceFinalReleaseAsserter::ReleaseAllWithAssert
            WaitForIdle();
            DetachMockDComp();
            WaitForIdle();
            Utilities::SetIsMockDCompDisabledForCompLeakDetection(true);
        }

        LogThrow_IfFailed(WindowHelper::CancelAllConnectedAnimationsAndResetDefaults());

        Utilities::CraterJupiter(craterJupiterError);
    }
    COM_END
}

void WindowHelper::SetWindowContentStatic(xaml::IUIElement* pElement, wrl::ComPtr<test_infra::Hosting::IWin32Host> win32Host)
{
    if (s_foregroundWindowCraterArmed == false)
    {
        LOG_OUTPUT(L"First window content set, arming the foreground window check.");
        s_foregroundWindowCraterArmed = true;
    }

    HostingMode hostingMode = HostingMode::UAP;
    LogThrow_IfFailed(GetHostingMode(&hostingMode));

    if (hostingMode == HostingMode::UAP)
    {
        LogThrow_IfFailed(GetXamlWindow()->put_Content(pElement));
    }
    else
    {
        LogThrow_IfFailed(win32Host->put_Content(pElement));
    }
}

void WindowHelper::GetWindowContentStatic(xaml::IUIElement** ppElement, wrl::ComPtr<test_infra::Hosting::IWin32Host> win32Host)
{
    *ppElement = nullptr;
    HostingMode hostingMode = HostingMode::UAP;
    LogThrow_IfFailed(GetHostingMode(&hostingMode));

    if (hostingMode == HostingMode::UAP)
    {
        auto window = GetXamlWindow();
        if (window)
        {
            LogThrow_IfFailed(window->get_Content(ppElement));
        }
    }
    else
    {
        wrl::ComPtr<IInspectable> spInsp;
        LogThrow_IfFailed(win32Host->get_Content(&spInsp));

        if(spInsp != nullptr)
        {
            wrl::ComPtr<xaml::IUIElement> spElement;
            LogThrow_IfFailed(spInsp.As(&spElement));

            *ppElement = spElement.Detach();
        }
    }
}

void WindowHelper::GetWindowBoundsStatic(wf::Rect* bounds)
{
    HostingMode hostingMode = HostingMode::UAP;
    LogThrow_IfFailed(GetHostingMode(&hostingMode));
    if (hostingMode == HostingMode::UAP)
    {
        LogThrow_IfFailed(GetXamlWindow()->get_Bounds(bounds));
    }
    else
    {
        const auto hWnd = GetCurrentWindowHandle();
        const auto dpi = ::GetDpiForWindow(hWnd);
        const auto scale = static_cast<float>(dpi) / static_cast<float>(USER_DEFAULT_SCREEN_DPI);
        RECT rect = {};
        ::GetWindowRect(hWnd, &rect);
        const auto physicalWidth = rect.right - rect.left;
        const auto physicalHeight = rect.bottom - rect.top;
        bounds->X = (float)((rect.left / scale) + 0.5);
        bounds->Y = (float)((rect.top / scale) + 0.5);
        bounds->Width = (float)((physicalWidth / scale) + 0.5);
        bounds->Height = (float)((physicalHeight / scale) + 0.5);
    }
}

void WindowHelper::GetVisibleBoundsStatic(wf::Rect* bounds, xaml::IUIElement* pElement)
{
    RunOnUIThread([&]
    {
        LogThrow_IfFailed(GetTestHooks()->GetVisibleContentBounds(pElement, bounds));
    });
}

void WindowHelper::ShowWindowStatic()
{
    LogThrow_IfFailed(GetXamlWindowPrivate()->Show());
}

void WindowHelper::HideWindowStatic()
{
    LogThrow_IfFailed(GetXamlWindowPrivate()->Hide());
}

void WindowHelper::MoveWindowStatic(int x, int y, int width, int height)
{
    if (auto windowPrivate = GetXamlWindowPrivate())
    {
        LogThrow_IfFailed(GetXamlWindowPrivate()->MoveWindow(x, y, width, height));
    }
    else
    {
        const auto hwnd = GetCurrentWindowHandle();

        SetWindowPos(hwnd, nullptr, x, y, width, height, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);
    }
}

double WindowHelper::GetPhysicalPixelsPerLogicalPixelStatic()
{
    double currentScale = 0;
    RunOnUIThread([&]
    {
        // Retrieve the plateau scale from XAML as it may have been overridden via SetWindowSizeOverride
        currentScale = GetTestHooks()->GetZoomScale();
        LOG_OUTPUT(L"    WindowHelper::GetPhysicalPixelsPerLogicalPixelStatic: %lf", currentScale);
    });

    return currentScale;
}

wf::Point WindowHelper::ConvertToPhysicalPixelsStatic(wf::Point point)
{
    double rawPixelsPerViewPixel = GetPhysicalPixelsPerLogicalPixelStatic();
    wf::Point returnPoint = {};
    returnPoint.X = static_cast<float>(point.X * rawPixelsPerViewPixel);
    returnPoint.Y = static_cast<float>(point.Y * rawPixelsPerViewPixel);
    return returnPoint;
}

wf::Point WindowHelper::ConvertToPhysicalDisplayLocationStatic(
    wf::Point point,
    bool validatePhysicalLocationIsInsideCurrentWindow /* = true */)
{
    wf::Point pointInScreenCoordinates = {};
    RunOnUIThread([&]()
    {
        wrl::ComPtr<wuc::ICoreWindow> spCoreWindow = HostingDispatcher::Get()->GetCoreWindow();

        LOG_OUTPUT(L"  Calling ConvertToPhysicalPixelsStatic with %lf,%lf", point.X, point.Y);
        auto pointInPhysicalPixels = ConvertToPhysicalPixelsStatic(point);
        LOG_OUTPUT(L"  Returned %lf,%lf", pointInPhysicalPixels.X, pointInPhysicalPixels.Y);

        //Experiment

        Hosting::HostingMode hostingMode = Hosting::HostingMode::UAP;
        LogThrow_IfFailed(Hosting::GetHostingMode(&hostingMode));

        HWND appHandle = NULL;
        if (hostingMode == Hosting::HostingMode::WPF)
        {
            // We want to use the parent (app) hwnd for WPF hosting
            test_infra::Hosting::IWin32Host* win32Host = nullptr;

            wrl::ComPtr<test_infra::ITestServicesStatics> testServicesStatics;
            LogThrow_IfFailed(wf::GetActivationFactory(
                wrl::Wrappers::HStringReference(RuntimeClass_Private_Infrastructure_TestServices).Get(),
                &testServicesStatics
            ));

            LogThrow_IfFailed(testServicesStatics->get_Win32Host(&win32Host));

            uint64_t handle = 0;
            FAIL_FAST_IF_FAILED(win32Host->get_MainWindowHandle(&handle));
            appHandle = reinterpret_cast<HWND>(handle);
        }
        else
        {
            wrl::ComPtr<ICoreWindowInterop> spCoreWindowInterop;
            LogThrow_IfFailed(spCoreWindow.As(&spCoreWindowInterop));
            LogThrow_IfFailed(spCoreWindowInterop->get_WindowHandle(&appHandle));
        }

        if (hostingMode == Hosting::HostingMode::WPF)
        {
            //
            // The thread's DPI awareness context also plays a role. If it's Unaware, System, or UnawareGdiScaled, then
            // the Xaml island will render at some initial scale (the system scale when the window was created - usually
            // 1.0 for tests) and the DWM will scale it up by the rest to reach the current system scale. When we inject input
            // we need to account for this DWM scale as well. The zoom scale that we got from Xaml above will not necessarily
            // be the system scale, so explicitly apply the DWM scale at the end.
            //
            //      For example, the test started at 1.0 scale, then changed the DPI to 1.25. Xaml will continue rendering
            //      its tree at 1.0, and the DWM will apply a 1.25 scale afterwards. We need to account for the 1.25.
            //
            //      As a second example, the test started at 1.5 scale, then changed the DPI to 3.0. Xaml will continue
            //      rendering its tree at 1.5, and the DWM will apply a 2.0 scale afterwards. We need to account for the 2.0.
            //
            // For completeness, Xaml also reports a 1.0 zoom scale in PerMonitor mode, but the DWM does not apply any
            // scaling in this mode. In PerMonitorV2, Xaml is aware of the current scale of the system, and the DWM doesn't
            // apply any scaling either. So there's no work that we need to do for PerMonitor or PerMonitorV2.
            //
            // See https://docs.microsoft.com/en-us/windows/desktop/hidpi/dpi-awareness-context for more details.
            //
            // None of this is necessary if we're not running in WPF mode. CoreWindow always runs in PerMonitorV2.
            //

            bool applyDwmScale = false;
            double xamlScale = 0;
            double systemScale = 0;

            // The only way to read the thread's DPI awareness context is to write it, so restore what it used to be.
            DPI_AWARENESS_CONTEXT oldDpiAwarenessContext = ::SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
            ::SetThreadDpiAwarenessContext(oldDpiAwarenessContext);

            // DPI awareness contexts contain informational flags and can't be bitwise compared.
            if (::AreDpiAwarenessContextsEqual(oldDpiAwarenessContext, DPI_AWARENESS_CONTEXT_UNAWARE)
                || ::AreDpiAwarenessContextsEqual(oldDpiAwarenessContext, DPI_AWARENESS_CONTEXT_SYSTEM_AWARE)
                || ::AreDpiAwarenessContextsEqual(oldDpiAwarenessContext, DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED))
            {
                RunOnUIThread([&]
                {
                    xamlScale = GetTestHooks()->GetZoomScale();
                });

                DEVICE_SCALE_FACTOR scaleFactor;
                LogThrow_IfFailed(GetScaleFactorForMonitor(
                    MonitorFromWindow(appHandle, MONITOR_DEFAULTTONEAREST),
                    &scaleFactor));

                int scalePercentage = static_cast<int>(scaleFactor);
                systemScale = static_cast<double>(scalePercentage) / 100.0;

                applyDwmScale = (xamlScale != systemScale);
            }

            if (applyDwmScale)
            {
                if (::AreDpiAwarenessContextsEqual(oldDpiAwarenessContext, DPI_AWARENESS_CONTEXT_UNAWARE))
                {
                    LOG_OUTPUT(L"  System scale is %.2f, Xaml applied %.2f. Applying additional scale for DPI_AWARENESS_CONTEXT_UNAWARE.", systemScale, xamlScale);
                }
                else if (::AreDpiAwarenessContextsEqual(oldDpiAwarenessContext, DPI_AWARENESS_CONTEXT_SYSTEM_AWARE))
                {
                    LOG_OUTPUT(L"  System scale is %.2f, Xaml applied %.2f. Applying additional scale for DPI_AWARENESS_CONTEXT_SYSTEM_AWARE.", systemScale, xamlScale);
                }
                else if (::AreDpiAwarenessContextsEqual(oldDpiAwarenessContext, DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED))
                {
                    LOG_OUTPUT(L"  System scale is %.2f, Xaml applied %.2f. Applying additional scale for DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED.", systemScale, xamlScale);
                }

                const double dwmScale = systemScale / xamlScale;
                pointInPhysicalPixels.X = static_cast<float>(pointInPhysicalPixels.X * dwmScale);
                pointInPhysicalPixels.Y = static_cast<float>(pointInPhysicalPixels.Y * dwmScale);

                LOG_OUTPUT(L"  Applied DWM scale -> (%.2f, %.2f)", pointInPhysicalPixels.X, pointInPhysicalPixels.Y);
            }
        }

        RECT clientRect = {};
        ::GetClientRect(appHandle, &clientRect);
        LOG_OUTPUT(L"  GetClientRect->{%ld,%ld,%ld,%ld}", clientRect.left, clientRect.top, clientRect.right, clientRect.bottom);
        MapWindowPoints(appHandle, nullptr, reinterpret_cast<LPPOINT>(&clientRect), 2);
        LOG_OUTPUT(L"  MapWindowPoints->{%ld,%ld,%ld,%ld}", clientRect.left, clientRect.top, clientRect.right, clientRect.bottom);

        pointInScreenCoordinates.X = pointInPhysicalPixels.X + static_cast<float>(clientRect.left);
        pointInScreenCoordinates.Y = pointInPhysicalPixels.Y + static_cast<float>(clientRect.top);

        LOG_OUTPUT(L"  pointInScreenCoordinates->(%lf,%lf)", pointInScreenCoordinates.X, pointInScreenCoordinates.Y);

        // Check if the physical location is outside of the screen bounds.
        RECT screenRect = { 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };
        LOG_OUTPUT(L"  screenRect->{%ld,%ld,%ld,%ld}", screenRect.left, screenRect.top, screenRect.right, screenRect.bottom);

        if (validatePhysicalLocationIsInsideCurrentWindow &&
            (pointInScreenCoordinates.X < screenRect.left ||
                pointInScreenCoordinates.Y < screenRect.top ||
                pointInScreenCoordinates.X >= screenRect.right ||
                pointInScreenCoordinates.Y >= screenRect.bottom))
        {
            Log::Error(L"Physical pixel location is outside of the window bounds.");
            CraterJupiterErrorCode craterJupiterError = {};
            craterJupiterError.Error.Flags.PhysicalLocationOutsideBounds = true;
            Utilities::CraterJupiter(craterJupiterError);
        }
    });

    return pointInScreenCoordinates;
}

HRESULT WindowHelper::ConvertToPhysicalPixels(wf::Point pointIn, wf::Point* pointOut)
{
    COM_START_GROUP(L"WindowHelper::ConvertToPhysicalPixels")
    {
        *pointOut = WindowHelper::ConvertToPhysicalPixelsStatic(pointIn);
    }
    COM_END
}

HRESULT WindowHelper::ConvertToPhysicalDisplayLocation(wf::Point pointIn, wf::Point* pointOut)
{
    COM_START_GROUP(L"WindowHelper::ConvertToPhysicalDisplayLocation")
    {
        BOOLEAN isDesktop = FALSE;
        LogThrow_IfFailed(Utilities::IsDesktopStatic(&isDesktop));
        Throw::IfFalse(!!isDesktop, E_FAIL, L"ConvertToPhysicalDisplayLocation should only be used on desktop.");

        *pointOut = WindowHelper::ConvertToPhysicalDisplayLocationStatic(pointIn, false /* validatePhysicalLocationIsInsideCurrentWindow */);
    }
    COM_END
}

wrl::ComPtr<IXamlTestHooks> WindowHelper::GetTestHooks()
{
    return TestServicesStatics::GetTestHooks();
}

wrl::ComPtr<xaml::IWindow> WindowHelper::GetXamlWindow()
{
    wrl::ComPtr<xaml::IWindowStatics> spWindowStatics;
    wrl::ComPtr<xaml::IWindow> spWindow;

    LogThrow_IfFailed(wf::GetActivationFactory(
        wrl::Wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Window).Get(), &spWindowStatics));
    LogThrow_IfFailed(spWindowStatics->get_Current(&spWindow));

    return std::move(spWindow);
}

wrl::ComPtr<xaml::IWindowPrivate> WindowHelper::GetXamlWindowPrivate()
{
    wrl::ComPtr<xaml::IWindowStatics> spWindowStatics;
    wrl::ComPtr<xaml::IWindow> spWindow;
    wrl::ComPtr<xaml::IWindowPrivate> spWindowPrivate;

    LogThrow_IfFailed(wf::GetActivationFactory(
        wrl::Wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Window).Get(), &spWindowStatics));
    LogThrow_IfFailed(spWindowStatics->get_Current(&spWindow));
    if (spWindow)
    {
        LogThrow_IfFailed(spWindow.As(&spWindowPrivate));
    }

    return std::move(spWindowPrivate);
}

//----------------------------------------------------------------------------
//
//  Synopsis:
//      Injects the mock DComp layer into Xaml via IXamlTestHooks::
//      SetCreateDCompDeviceFunction.
//
//----------------------------------------------------------------------------
HRESULT
WindowHelper::InjectMockDComp()
{
    COM_START_GROUP(L"WindowHelper::InjectMockDComp")
    {
        BOOLEAN isDisabled = FALSE;
        LogThrow_IfFailed(Utilities::IsMockDCompDisabled(&isDisabled));
        if (!isDisabled)
        {
            RunOnUIThread([&] () {
                LogThrow_IfFailed(StartDetourMockDCompDevice());

                // Now that we've detoured the DComp device creation APIs, we need to tickle XAML/DManip into
                // re-creating its DComp device.  We do this by reseting both the device and the DComp visuals.
                ResetDeviceAndVisualsHelper();
                GetTestHooks()->SetTransparentBackground(true);  // set DComp surface backround transparent for keeping same testing conditions (UWP and WPF)
            });

            TickUIThreadAfterDeviceLostIfNeeded();
        }
    }
    COM_END
}

//----------------------------------------------------------------------------
//
//  Synopsis:
//      Detaches the mock DComp layer from Xaml via IXamlTestHooks::
//      SetCreateDCompDeviceFunction.
//
//----------------------------------------------------------------------------
HRESULT
WindowHelper::DetachMockDComp()
{
    COM_START_GROUP(L"WindowHelper::DetachMockDComp")
    {
        BOOLEAN isDisabled = FALSE;
        LogThrow_IfFailed(Utilities::IsMockDCompDisabled(&isDisabled));
        if (!isDisabled)
        {
            RunOnUIThread([&] () {

                // It's possible a test has already detached the detours via StopMockDCompDetours().
                // Only detach the detour if it's currently active.
                if (IsMockDCompDetourActive())
                {
                    LogThrow_IfFailed(StopDetourMockDCompDevice());
                }

                // Now that we've removed the detours, tickle XAML/DManip into re-creating its DComp device.
                // We do this by reseting both the device and the DComp visuals.
                ResetDeviceAndVisualsHelper();
            });

            TickUIThreadAfterDeviceLostIfNeeded();
        }
    }
    COM_END
}

//----------------------------------------------------------------------------
//
//  Synopsis:
//      Stops MockDComp from detouring DComp calls
//
//----------------------------------------------------------------------------
HRESULT
WindowHelper::StopMockDCompDetours()
{
    COM_START_GROUP(L"WindowHelper::StopMockDCompDetours")
    {
        BOOLEAN isDisabled = FALSE;
        LogThrow_IfFailed(Utilities::IsMockDCompDisabled(&isDisabled));
        if (!isDisabled)
        {
            RunOnUIThread([&]() {
                // It's expected this function will only ever be used to explicitly stop active detours.
                // Assert the detours are currently active, to help ensure test stability.
                _ASSERT(IsMockDCompDetourActive());
                LogThrow_IfFailed(StopDetourMockDCompDevice());
            });
        }
    }
    COM_END
}

//----------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the mock DComp device, if it exists.
//
//----------------------------------------------------------------------------
HRESULT
WindowHelper::get_MockDCompDevice(
    _Outptr_ mdc::IMockDCompDevice **ppMockDCompDevice
    )
{
    COM_START
    {
        wrl::ComPtr<mdc::IMockDCompDevice> spMockDCompDevice = TryGetMockDCompDevice();
        spMockDCompDevice.CopyTo(ppMockDCompDevice);
    }
    COM_END
}

// Returns the mock DComp device if it exists, otherwise returns a nullptr
wrl::ComPtr<mdc::IMockDCompDevice>
WindowHelper::TryGetMockDCompDevice()
{
    wrl::ComPtr<IDCompositionDesktopDevice> spDCompDevice;
    wrl::ComPtr<mdc::IMockDCompDevice> spMockDCompDevice;

    RunOnUIThread([&] () {
        GetTestHooks()->GetDCompDevice(&spDCompDevice);
    });

    spDCompDevice.As(&spMockDCompDevice);

    return spMockDCompDevice;
}

HRESULT
WindowHelper::SimulateDeviceLost()
{
    COM_START_GROUP(L"WindowHelper::SimulateDeviceLost")
    {
        BOOLEAN isDisabled = FALSE;
        LogThrow_IfFailed(Utilities::IsMockDCompDisabled(&isDisabled));
        if (!isDisabled)
        {
            RunOnUIThread([&]() {
                LogThrow_IfFailed(GetTestHooks()->SimulateDeviceLost());
                LogThrow_IfFailed(OnSimulatedDeviceLost());
            });
        }
    }
    COM_END
}

HRESULT WindowHelper::ResetDeviceAndVisuals()
{
    COM_START_GROUP(L"WindowHelper::ResetDeviceAndVisuals")
    {
        RunOnUIThread([&]() {
            ResetDeviceAndVisualsHelper();
            LogThrow_IfFailed(OnSimulatedDeviceLost());
        });

        TickUIThreadAfterDeviceLostIfNeeded();
    }
    COM_END
}

HRESULT WindowHelper::ResetDeviceOnly()
{
    COM_START_GROUP(L"WindowHelper::ResetDeviceOnly")
    {
        RunOnUIThread([&]()
        {
            LogThrow_IfFailed(GetTestHooks()->ResetDeviceOnly());
            LogThrow_IfFailed(OnSimulatedDeviceLost());
        });
    }
    COM_END
}

HRESULT WindowHelper::OnSimulatedDeviceLost()
{
    COM_START_GROUP(L"WindowHelper::OnSimulatedDeviceLost")
    {
        BOOLEAN isDisabled = FALSE;
        LogThrow_IfFailed(Utilities::IsMockDCompDisabled(&isDisabled));
        if (!isDisabled)
        {
            // If we have injected a mock DComp device, inform it that we've simulated device lost.
            wrl::ComPtr<mdc::IMockDCompDevice> spMockDCompDevice = TryGetMockDCompDevice();
            if (spMockDCompDevice)
            {
                LogThrow_IfFailed(spMockDCompDevice->OnSimulatedDeviceLost());
            }
        }
    }
    COM_END
}

void WindowHelper::GetOpenPopupsStatic(wfc::IVectorView<xaml_controls::Primitives::Popup*>** ppPopups)
{
    wrl::ComPtr<xaml_media::IVisualTreeHelperStatics> spVisualTreeHelper;
    LogThrow_IfFailed(wf::GetActivationFactory(
        wrl::Wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_VisualTreeHelper).Get(),
        &spVisualTreeHelper));
    LogThrow_IfFailed(spVisualTreeHelper->GetOpenPopups(GetXamlWindow().Get(), ppPopups));
}

void WindowHelper::GetOpenPopupsInXamlRootStatic(
    _In_ xaml::IXamlRoot* xamlRoot,
    wfc::IVectorView<xaml_controls::Primitives::Popup*>** ppPopups)
{
    wrl::ComPtr<xaml_media::IVisualTreeHelperStatics> spVisualTreeHelperStatics;
    LogThrow_IfFailed(wf::GetActivationFactory(
        wrl::Wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_VisualTreeHelper).Get(),
        &spVisualTreeHelperStatics));

    LogThrow_IfFailed(spVisualTreeHelperStatics->GetOpenPopupsForXamlRoot(xamlRoot, ppPopups));
}

void WindowHelper::GetAllRootVisualsNoRef(_In_ wfc::IVectorView<IInspectable*>** visualsNoRef)
{
    GetTestHooks()->GetAllRootVisualsNoRef(visualsNoRef);
}

HRESULT WindowHelper::get_IsForegroundWindow(_Out_ BOOLEAN* pIsForegroundWindow)
{
    COM_START
    {
        *pIsForegroundWindow = WindowHelper::IsForegroundWindowStatic();
    }
    COM_END
}

bool WindowHelper::IsForegroundWindowStatic()
{
    HWND appHandle = GetCurrentWindowHandle();

    HWND activeWindow = GetForegroundWindow();
    HWND parentWindow = GetParent(appHandle);

    Hosting::HostingMode hostingMode = Hosting::HostingMode::UAP;
    LogThrow_IfFailed(GetHostingMode(&hostingMode));
    if (parentWindow == NULL && hostingMode != Hosting::HostingMode::UAP)
    {
        parentWindow = appHandle;
    }

    HWND focusedWindow = nullptr;
    RunOnUIThread([&focusedWindow]() {
        focusedWindow = GetFocus();
    });

    LOG_OUTPUT(L"IsForegroundWindowStatic appHandle:0x%x, activeWindow:0x%x, parentWindow:0x%x, focusedWindow:0x%x.", appHandle, activeWindow, parentWindow, focusedWindow);

    if ((appHandle == activeWindow || parentWindow == activeWindow))
    {
        return true;
    }
    else
    {
        WCHAR windowTitle[MAX_PATH + 1];
        if (GetWindowText(activeWindow, windowTitle, MAX_PATH))
        {
            LOG_OUTPUT(L"Active foreground Window title is [%s].", windowTitle);
        }
        if (GetWindowText(focusedWindow, windowTitle, MAX_PATH))
        {
            LOG_OUTPUT(L"Focused Window title is [%s].", windowTitle);
        }

        WCHAR className[MAX_PATH + 1];
        if (GetClassName(activeWindow, className, MAX_PATH))
        {
            LOG_OUTPUT(L"Active foreground Window is %s.", className);
        }
        if (GetClassName(appHandle, className, MAX_PATH))
        {
            LOG_OUTPUT(L"Current Window is %s.", className);
        }
        if (GetClassName(parentWindow, className, MAX_PATH))
        {
            LOG_OUTPUT(L"Current parent Window is %s.", className);
        }
        if(GetClassName(focusedWindow, className, MAX_PATH))
        {
            LOG_OUTPUT(L"Current focus Window is %s.", className);
        }

        return false;
    }
}

HRESULT WindowHelper::get_IsFocusedWindow(_Out_ BOOLEAN* pIsFocusedWindow)
{
    COM_START
    {
        *pIsFocusedWindow = WindowHelper::IsFocusedWindowStatic();
    }
    COM_END
}

bool WindowHelper::IsFocusedWindowStatic()
{
    HostingMode hostingMode = HostingMode::UAP;
    LogThrow_IfFailed(GetHostingMode(&hostingMode));

    HWND appHandle = GetCurrentWindowHandle();

    HWND focusedWindow = nullptr;
    RunOnUIThread([&focusedWindow]() {
        focusedWindow = GetFocus();
    });

    LOG_OUTPUT(L"IsFocusedWindowStatic appHandle:0x%X, focusedWindow:0x%X.", appHandle, focusedWindow);

    if (appHandle == focusedWindow)
    {
        return true;
    }
    else if (hostingMode == HostingMode::WPF)
    {
        // When using XamlIslandRoots, the Focused Window will not be the app handle.
        return true;
    }
    else
    {
        LOG_OUTPUT(L"App Handle Window does not have window focus!");
        return false;
    }
}

HRESULT WindowHelper::get_IsInputPaneOpen(_Out_ BOOLEAN* pIsInputPaneOpen)
{
    COM_START
    {
        WindowHelper::IsInputPaneOpenStatic(pIsInputPaneOpen);
    }
    COM_END
}

void WindowHelper::IsInputPaneOpenStatic(_Out_ BOOLEAN* pIsInputPaneOpen)
{
    *pIsInputPaneOpen = false;

    wrl::ComPtr<wuv::IInputPaneStatics> spInputPaneStatics;
    wrl::ComPtr<wuv::IInputPane> spInputPane;
    LogThrow_IfFailed(wf::GetActivationFactory(
        wrl::Wrappers::HStringReference(RuntimeClass_Windows_UI_ViewManagement_InputPane).Get(),
        &spInputPaneStatics));

    // If we're in the context of XAML islands, then we don't want to use GetForCurrentView -
    // that requires CoreWindow, which is not supported in islands. Hence, we switched to use interop
    Hosting::HostingMode hostingMode = Hosting::HostingMode::UAP;
    LogThrow_IfFailed(GetHostingMode(&hostingMode));
    if (hostingMode == Hosting::HostingMode::WPF)
    {
        ComPtr<IInputPaneInterop> spInputPaneInterop;
        LogThrow_IfFailed(spInputPaneStatics.As(&spInputPaneInterop));

        LogThrow_IfFailed(spInputPaneInterop->GetForWindow(GetCurrentWindowHandle(), ABI::Windows::UI::ViewManagement::IID_IInputPane, &spInputPane));
    }
    else
    {
        LogThrow_IfFailed(spInputPaneStatics->GetForCurrentView(&spInputPane));
    }

    wf::Rect occludedRect = {};
    LogThrow_IfFailed(spInputPane->get_OccludedRect(&occludedRect));
    *pIsInputPaneOpen = occludedRect.Width != 0.0 && occludedRect.Height != 0.0;
}

void WindowHelper::TryInputPaneHideStatic()
{
    wrl::ComPtr<wuv::IInputPaneStatics> spInputPaneStatics;
    wrl::ComPtr<wuv::IInputPane> spInputPane;
    wrl::ComPtr<wuv::IInputPane2> spInputPane2;
    BOOLEAN isHidden = false;

    LogThrow_IfFailed(wf::GetActivationFactory(
        wrl::Wrappers::HStringReference(RuntimeClass_Windows_UI_ViewManagement_InputPane).Get(),
        &spInputPaneStatics));

    // If we're in the context of XAML islands, then we don't want to use GetForCurrentView -
    // that requires CoreWindow, which is not supported in islands. Hence, we switched to use interop
    Hosting::HostingMode hostingMode = Hosting::HostingMode::UAP;
    LogThrow_IfFailed(GetHostingMode(&hostingMode));
    if (hostingMode == Hosting::HostingMode::WPF)
    {
        ComPtr<IInputPaneInterop> spInputPaneInterop;
        LogThrow_IfFailed(spInputPaneStatics.As(&spInputPaneInterop));

        LogThrow_IfFailed(spInputPaneInterop->GetForWindow(GetCurrentWindowHandle(), ABI::Windows::UI::ViewManagement::IID_IInputPane, &spInputPane));
    }
    else
    {
        LogThrow_IfFailed(spInputPaneStatics->GetForCurrentView(&spInputPane));
    }

    LogThrow_IfFailed(spInputPane.As(&spInputPane2));

    LogThrow_IfFailed(spInputPane2->TryHide(&isHidden));
}

HRESULT WindowHelper::SetHostSizeOverride(
    const wf::Size& size,
    const wf::Rect& layoutBounds,
    float zoomScaleOverride,
    bool scaleWindowSizeByScaleFactor)
{
    HostingMode hostingMode = HostingMode::UAP;
    LogThrow_IfFailed(GetHostingMode(&hostingMode));

    if (hostingMode != HostingMode::UAP)
    {
        m_win32Host->SetWindowSizeOverride(size.Width, size.Height);
    }

    LogThrow_IfFailed(GetTestHooks()->SetWindowSizeOverride(
        size,
        layoutBounds,
        zoomScaleOverride,
        scaleWindowSizeByScaleFactor /* scaleWindowSizeByScaleFactor */
    ));

    return S_OK;
}

// Forces the window to a certain size. Used to get consistent output for visual validation tests.
//
// Notes:
// - 0x0 is a special value that removes the size override
// - 0 is a special value that removes the zoom scale override
// - The change will be reported by get_WindowBounds, but it does not affect the bounds of the real ICoreWindow.
HRESULT WindowHelper::SetWindowSizeOverride(wf::Size size)
{
    COM_START_GROUP(L"WindowHelper::SetWindowSizeOverride")
    {
        RunOnUIThread([&] () {
            LOG_OUTPUT(L"Setting size:%lfx%lf", size.Width, size.Height);

            // TODO: SetWindowSizeOverride does not need to lie anymore
            //  We dont need to do this, instead we can use SetSize in IOverlappedPositioner
            //  GetForCurrentView().TryGetDefaultPositioner()
            //  interface IOverlappedPositioner : IInspectable requires IPositioner
            //  {
            //     HRESULT SetSize([in] int viewId, [in] Windows.Foundation.Size size);
            //  }
            LogThrow_IfFailed(WindowHelper::SetHostSizeOverride(
                size,
                { 0, 0, size.Width, size.Height },
                0,
                true /* scaleWindowSizeByScaleFactor */
            ));
        });
    }
    COM_END
}

HRESULT WindowHelper::SetWindowSizeOverrideWithScale(wf::Size size, float zoomScaleOverride)
{
    COM_START_GROUP(L"WindowHelper::SetWindowSizeOverrideWithScale")
    {
        RunOnUIThread([&] () {
            LOG_OUTPUT(L"Setting size:%lfx%lf", size.Width, size.Height);
            LOG_OUTPUT(L"Setting scale:%lf", zoomScaleOverride);

            LogThrow_IfFailed(WindowHelper::SetHostSizeOverride(
                size,
                { 0, 0, size.Width, size.Height },
                zoomScaleOverride,
                true /* scaleWindowSizeByScaleFactor */
            ));
        });
    }
    COM_END
}

HRESULT WindowHelper::SetWindowSizeOverrideWithLayoutBounds(wf::Size size, wf::Rect layoutBounds)
{
    COM_START_GROUP(L"WindowHelper::SetWindowSizeOverrideWithLayoutBounds")
    {
        RunOnUIThread([&] () {
            LOG_OUTPUT(L"Setting size:%lfx%lf", size.Width, size.Height);

            LogThrow_IfFailed(WindowHelper::SetHostSizeOverride(
                size,
                layoutBounds,
                0.0f,
                true /* scaleWindowSizeByScaleFactor */
            ));
        });
    }
    COM_END
}

HRESULT WindowHelper::SetWindowSizeOverrideWithWindowScale(wf::Size size, float zoomScaleOverride)
{
    COM_START_GROUP(L"WindowHelper::SetWindowSizeOverrideWithWindowScale")
    {
        RunOnUIThread([&]() {
            LOG_OUTPUT(L"Setting size:%lfx%lf", size.Width, size.Height);
            LOG_OUTPUT(L"Setting scale:%lf", zoomScaleOverride);

            LogThrow_IfFailed(WindowHelper::SetHostSizeOverride(
                size,
                { 0, 0, size.Width, size.Height },
                zoomScaleOverride,
                false /*scaleWindowSizeByScaleFactor*/
            ));
        });
    }
    COM_END
}

HRESULT WindowHelper::RequestReplayPreviousPointerUpdate_TempTestHook()
{
    COM_START
    {
        RunOnUIThread([&] () {
            GetTestHooks()->RequestReplayPreviousPointerUpdate_TempTestHook();
        });
    }
    COM_END
}

HRESULT WindowHelper::SimulateSuspendToPauseAnimations()
{
    COM_START
    {
        RunOnUIThread([&] () {
            GetTestHooks()->SimulateSuspendToPauseAnimations();
        });
    }
    COM_END
}

HRESULT WindowHelper::SimulateResumeToResumeAnimations()
{
    COM_START
    {
        RunOnUIThread([&] () {
            GetTestHooks()->SimulateResumeToResumeAnimations();
        });
    }
    COM_END
}

HRESULT WindowHelper::ForceDisconnectRootOnSuspend(_In_ BOOLEAN forceDisconnectRootOnSuspend)
{
    COM_START
    {
        RunOnUIThread([&] () {
            GetTestHooks()->ForceDisconnectRootOnSuspend(!!forceDisconnectRootOnSuspend);
        });
    }
    COM_END
}

HRESULT WindowHelper::TriggerSuspend(_In_ BOOLEAN isTriggeredByResourceTimer, _In_ BOOLEAN allowOfferResources)
{
    COM_START
    {
        RunOnUIThread([&] () {
            GetTestHooks()->TriggerSuspend(!!isTriggeredByResourceTimer, !!allowOfferResources);
        });
    }
    COM_END
}

HRESULT WindowHelper::TriggerResume()
{
    COM_START
    {
        RunOnUIThread([&] () {
            GetTestHooks()->TriggerResume();
        });
    }
    COM_END
}

HRESULT WindowHelper::TriggerLowMemory()
{
    COM_START
    {
        RunOnUIThread([&] () {
            GetTestHooks()->TriggerLowMemory();
        });
    }
    COM_END
}

HRESULT WindowHelper::SetIsSuspended(BOOLEAN isSuspended)
{
    COM_START
    {
        RunOnUIThread([&] () {
            GetTestHooks()->SetIsSuspended(!!isSuspended);
        });
    }
    COM_END
}

HRESULT WindowHelper::SetIsRenderEnabled(BOOLEAN value)
{
    COM_START
    {
        RunOnUIThread([&] () {
            GetTestHooks()->SetIsRenderEnabled(!!value);
        });
    }
    COM_END
}


HRESULT WindowHelper::SetIsHolographic(BOOLEAN value)
{
    COM_START
    {
        RunOnUIThread([&] () {
            GetTestHooks()->SetIsHolographic(!!value);
        });
    }
    COM_END
}

HRESULT WindowHelper::SetTimeManagerClockOverrideConstant(double newTime)
{
    COM_START
    {
        RunOnUIThread([&] () {
            GetTestHooks()->SetTimeManagerClockOverrideConstant(newTime);
        });
    }
    COM_END
}

HRESULT WindowHelper::FireDCompAnimationCompleted(_In_ xaml_animation::IStoryboard* storyboard)
{
    COM_START
    {
        RunOnUIThread([&] () {
            GetTestHooks()->FireDCompAnimationCompleted(storyboard);
        });
    }
    COM_END
}

HRESULT WindowHelper::CleanUpAfterTest()
{
    COM_START_GROUP(L"WindowHelper::CleanUpAfterTest")
    {
        RunOnUIThread([&]() {
            GetTestHooks()->CleanUpAfterTest();
        });
    }
    COM_END
}

HRESULT WindowHelper::InitializeXaml()
{
    COM_START_GROUP(L"WindowHelper::InitializeXaml")
    {
        // Since we're being initialized without a custom provider, we'll use the MUXC provider.
        wrl::ComPtr<xaml_markup::IXamlMetadataProvider> xamlControlsXamlMetadataProvider;

        LogThrow_IfFailed(wf::ActivateInstance(
            wrl::Wrappers::HStringReference(L"Microsoft.UI.Xaml.XamlTypeInfo.XamlControlsXamlMetaDataProvider").Get(),
            &xamlControlsXamlMetadataProvider));

        InitializeXamlCore(xamlControlsXamlMetadataProvider.Get());
    }
    COM_END
}

void WindowHelper::InitializeXamlCore(_In_ xaml_markup::IXamlMetadataProvider* customProvider)
{
    // Since we shutdown xaml, we need to reset the foreground window check in case this test doesn't
    // actually set any window content. We also reset the shutdown enabled, since we run initialization at the beginning
    // of each test class in case the previous class shut us down.
    s_isShutdownEnabled = false;
    s_foregroundWindowCraterArmed = false;
    m_ensureSatelliteDLLCustomDPCleanup = false;

    // Make sure we are tracking leaks for this test in case a previous test had disabled it.
    ErrorHandlingHelper::TrackLeaksForTest();

    // Check for a Data:PerfOptIn light-weight data-driven parameter.  Tests declare
    // TEST_METHOD_PROPERTY(L"Data:PerfOptIn", L"{true}") or L"{false}" in the header,
    // and TAEF makes the value available via TestData during TestSetup.
    // Currently PerfOptIn defaults to true.
    bool perfOptInValue = true;
    {
        WEX::Common::String value;
        if (SUCCEEDED(WEX::TestExecution::TestData::TryGetValue(L"PerfOptIn", value)))
        {
            perfOptInValue = (value.CompareNoCase(L"true") == 0);
            LOG_OUTPUT(L"PerfOptIn test data found: \"%s\" -> %s", (const wchar_t*)value, perfOptInValue ? L"ON" : L"OFF");
        }
    }

    bool wasPreviouslyEnabled = false;

    wrl::ComPtr<IXamlTestHooks> windowTestHooks = nullptr;

    RunOnUIThread([&]() {
        windowTestHooks = WindowHelper::GetTestHooks();

        windowTestHooks->SetRuntimeEnabledFeatureOverride(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnableCoreShutdown, true, &wasPreviouslyEnabled);

        // Set PerfOptIn override if a test data parameter was specified.
        windowTestHooks->SetRuntimeEnabledFeatureOverride(
            RuntimeFeatureBehavior::RuntimeEnabledFeature::ForcePerfOptIn,
            perfOptInValue,
            nullptr);
        });

    HostingMode hostingMode = HostingMode::UAP;
    LogThrow_IfFailed(GetHostingMode(&hostingMode));

    // Call into initialize xaml, this will mark all outstanding allocations as ignorable and initialize the framework.
    // It will no-op framework initialization if it's already initialized.
    LOG_OUTPUT(L"Initializing xaml.");
    RunOnUIThread([&]() {
        LogThrow_IfFailed(windowTestHooks->InitializeXaml());
    });

    // Instantiating the XamlControlsResources causes type lookups to occur,
    // so we'll override our metadata provider before we do that.
    OverrideMetadataProvider(customProvider);

    RunOnUIThread([&]() {
        // In order to resolve MUXC styles, we'll add an instance of XamlControlsResources to our
        // application's merged dictionary.
        wrl::ComPtr<xaml::IApplicationStatics> applicationStatics;
        wrl::ComPtr<xaml::IApplication> application;
        wrl::ComPtr<xaml::IResourceDictionary> resourceDictionary;

        LogThrow_IfFailed(wf::GetActivationFactory(
            wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Application).Get(),
            &applicationStatics));
        LogThrow_IfFailed(applicationStatics->get_Current(&application));
        LogThrow_IfFailed(application->get_Resources(&resourceDictionary));

        wrl::ComPtr<wfc::IVector<xaml::ResourceDictionary*>> mergedDictionaries;

        LogThrow_IfFailed(resourceDictionary->get_MergedDictionaries(&mergedDictionaries));
        LogThrow_IfFailed(mergedDictionaries->Clear());

        wrl::ComPtr<xaml::IResourceDictionary> xamlControlsResources;

        LogThrow_IfFailed(wf::ActivateInstance(
            wrl::Wrappers::HStringReference(L"Microsoft.UI.Xaml.Controls.XamlControlsResources").Get(),
            &xamlControlsResources));

        LogThrow_IfFailed(mergedDictionaries->Append(xamlControlsResources.Get()));

        // Enable the pointer input event so the InputHelper knows that it should ensure that input system is
        // ready to handle events before sending input.  See InputHelper::EnsureInputReady for more details.
        LogThrow_IfFailed(windowTestHooks->EnablePointerInputEvent());
    });

    // If this was previously enabled, then we need to finish the initialization
    if (wasPreviouslyEnabled)
    {
        LOG_OUTPUT(L"Shutdown previously enabled. Resetting theming and window size");
        // Restore theming to our default state.
        LogThrow_IfFailed(WindowHelper::ResetThemingToDefaultState());

        RunOnUIThread([&]() {
            // Set the WindowSize to 0,0, which is a special size that tells Jupiter to
            // no longer override WindowSize. This is also how we ensure our target window
            // is hooked up so we can render
            LogThrow_IfFailed(WindowHelper::SetHostSizeOverride(
                wf::Size(),
                wf::Rect(),
                0.0,
                false
            ));
            });
    }

    RunOnUIThread([&]() {
        // Now that we are initialized, reset the feature.
        windowTestHooks->SetRuntimeEnabledFeatureOverride(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnableCoreShutdown, s_isShutdownEnabled, nullptr);
        });

#ifdef MUX_PRERELEASE
    if (hostingMode == Hosting::HostingMode::UAP)
    {
        RunOnUIThread([&]() {
            if (m_spApp && m_suspendedToken.value != 0)
            {
                wrl::ComPtr<xaml::IApplicationFeature_UwpSupportApi> applicationUwp;
                FAIL_FAST_IF_FAILED(m_spApp.As(&applicationUwp));
                LogThrow_IfFailed(applicationUwp->remove_Suspending(m_suspendedToken));
                m_suspendedToken = {};
            }

            wrl::ComPtr<xaml::IApplicationStatics> spAppStatics;
            FAIL_FAST_IF_FAILED(wf::GetActivationFactory(
                wrl::Wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Application).Get(), &spAppStatics));
            FAIL_FAST_IF_FAILED(spAppStatics->get_Current(&m_spApp));

            wrl::ComPtr<xaml::IApplicationFeature_UwpSupportApi> applicationUwp;
            FAIL_FAST_IF_FAILED(m_spApp.As(&applicationUwp));
            FAIL_FAST_IF_FAILED(applicationUwp->add_Suspending(wrl::Callback<xaml::ISuspendingEventHandler>(

                [&](IInspectable*, wa::ISuspendingEventArgs*) -> HRESULT {
                    return OnAppSuspended();
                }).Get(), &m_suspendedToken));
        });
    }
#endif

     ClearKeyState();
}

HRESULT WindowHelper::OnAppSuspended()
{
    COM_START
    {
        WEX::Logging::Log::Comment(L"WindowHelper::OnAppSuspended()");
        if (TestServicesStatics::IsInitialized())
        {
            if (Utilities::IsBVT())
            {
                WEX::Logging::Log::Error(L"A BVT test was suspended while running, this should never happen");
            }
            else
            {
                WEX::Logging::Log::Warning(L"A test was suspended while running, this should never happen");
            }
        }
        WEX::Logging::Log::Comment(L"WindowHelper::OnAppSuspended() ended");
    }
        COM_END
}

bool WindowHelper::IsKeyLockedOrDown(_In_ ixp::IInputKeyboardSourceStatics* const keyboardInputStatics, _In_ const wsy::VirtualKey key) const
{
    wuc::CoreVirtualKeyStates virtualKeyState;
    LogThrow_IfFailed(keyboardInputStatics->GetKeyStateForCurrentThread(key, &virtualKeyState));
    return !!(virtualKeyState & wuc::CoreVirtualKeyStates::CoreVirtualKeyStates_Locked)
        || !!(virtualKeyState & wuc::CoreVirtualKeyStates::CoreVirtualKeyStates_Down);
}

bool WindowHelper::IsKeyDown(_In_ ixp::IInputKeyboardSourceStatics* const keyboardInputStatics, _In_ const wsy::VirtualKey key) const
{
    wuc::CoreVirtualKeyStates virtualKeyState;
    LogThrow_IfFailed(keyboardInputStatics->GetKeyStateForCurrentThread(key, &virtualKeyState));
    return !!(virtualKeyState & wuc::CoreVirtualKeyStates::CoreVirtualKeyStates_Down);
}

void WindowHelper::ClearKeyState()
{
    bool numLockEnabled = false;
    bool capsLockEnabled = false;
    bool scrollLockEnabled = false;
    bool controlEnabled = false;
    bool shiftEnabled = false;
    bool altEnabled = false;

    RunOnUIThread([&]()
    {
        wrl::ComPtr<ixp::IInputKeyboardSourceStatics> keyboardInputStatics;
        LogThrow_IfFailed(wf::GetActivationFactory(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Input_InputKeyboardSource).Get(), &keyboardInputStatics));

        numLockEnabled = IsKeyLockedOrDown(keyboardInputStatics.Get(), wsy::VirtualKey::VirtualKey_NumberKeyLock);
        capsLockEnabled = IsKeyLockedOrDown(keyboardInputStatics.Get(), wsy::VirtualKey::VirtualKey_CapitalLock);
        scrollLockEnabled = IsKeyLockedOrDown(keyboardInputStatics.Get(), wsy::VirtualKey::VirtualKey_Scroll);
        controlEnabled = IsKeyDown(keyboardInputStatics.Get(), wsy::VirtualKey::VirtualKey_Control);
        shiftEnabled = IsKeyDown(keyboardInputStatics.Get(), wsy::VirtualKey::VirtualKey_Shift);
        altEnabled = IsKeyDown(keyboardInputStatics.Get(), wsy::VirtualKey::VirtualKey_Menu);
    });

    wrl::ComPtr<test_infra::IKeyboardHelper> spKeyboardHelper;
    LogThrow_IfFailed(m_pTestServices->get_KeyboardHelper(&spKeyboardHelper));
    spKeyboardHelper->SetWaitKind(test_infra::KeyboardWaitKind::KeyboardWaitKind_Sleep);

    if (numLockEnabled)
    {
        LOG_OUTPUT(L"NumLock was on, turning off.");
        LogThrow_IfFailed(spKeyboardHelper->PressKeySequence(wrl::Wrappers::HStringReference(L"$d$_numlock#$u$_numlock").Get()));
    }
    if (capsLockEnabled)
    {
        LOG_OUTPUT(L"CapsLock was on, turning off.");
        LogThrow_IfFailed(spKeyboardHelper->PressKeySequence(wrl::Wrappers::HStringReference(L"$d$_capslock#$u$_capslock").Get()));
    }
    if (scrollLockEnabled)
    {
        LOG_OUTPUT(L"ScrollLock was on, turning off.");
        LogThrow_IfFailed(spKeyboardHelper->PressKeySequence(wrl::Wrappers::HStringReference(L"$d$_scrolllock#$u$_scrolllock").Get()));
    }
    if (controlEnabled)
    {
        LOG_OUTPUT(L"Control key was on, turning off.");
        LogThrow_IfFailed(spKeyboardHelper->Control());
    }
    if (shiftEnabled)
    {
        LOG_OUTPUT(L"Shift key was on, turning off.");
        LogThrow_IfFailed(spKeyboardHelper->PressKeySequence(wrl::Wrappers::HStringReference(L"$d$_shift#$u$_shift").Get()));
    }
    if (altEnabled)
    {
        LOG_OUTPUT(L"Alt key was on, turning off.");
        LogThrow_IfFailed(spKeyboardHelper->Alt());
    }

    spKeyboardHelper->SetWaitKind(test_infra::KeyboardWaitKind::KeyboardWaitKind_Default);

    RunOnUIThread([&]()
    {
        BYTE pbKeyState[256];
        ZeroMemory(pbKeyState, 256);
        if (!!::SetKeyboardState((LPBYTE)&pbKeyState))
        {
            LOG_OUTPUT(L"KeyboardState cleared.");
        }
        else
        {
            LOG_OUTPUT(L"SetKeyboardState failed:GLE:0x%x", GetLastError());
        }
    });
}

HRESULT WindowHelper::InitializeXamlWithCustomMetadata(_In_ xaml_markup::IXamlMetadataProvider* customProvider, _In_ test_infra::ICustomMetadataRegistrar* registrar)
{
    COM_START
    {
        Throw::IfNull(registrar);

        LogThrow_IfFailed(InitializeXamlWithProvider(customProvider));

        // Now tell the registrar to register
        LogThrow_IfFailedWithMessage(registrar->RegisterMetadata(), L"Failed to register custom metadata");

        LogThrow_IfFailed(registrar->QueryInterface<wf::IClosable>(&m_spClosableMetadataRegistrar));
    }
    COM_END
}

HRESULT WindowHelper::InitializeXamlWithProvider(_In_ xaml_markup::IXamlMetadataProvider* customProvider)
{
    COM_START
    {
        InitializeXamlCore(customProvider);
    }
    COM_END
}

HRESULT WindowHelper::TestGetGlobalBoundsForUIElement(_In_ xaml::IUIElement* element, _In_ BOOLEAN ignoreClipping, _Out_ wf::Rect* bounds)
{
    COM_START
    {
        RunOnUIThread([&] () {
            GetTestHooks()->TestGetGlobalBoundsForUIElement(element, ignoreClipping, bounds);
        });
    }
    COM_END
}

HRESULT WindowHelper::GetAvailableMonitorBounds(
    _In_ xaml::IUIElement* element,
    _In_ wf::Point targetPointClientLogical,
    _Out_ wf::Rect* availableMonitorRectClientLogicalResult,
    _Out_ wf::Point* screenOffset,
    _Out_ wf::Point* targetPointScreenPhysical,
    _Out_ wf::Rect* inputPaneOccludeRectScreenLogical)
{
    COM_START
    {
        RunOnUIThread([&] () {
            GetTestHooks()->CalculateAvailableMonitorRect(
                element,
                targetPointClientLogical,
                availableMonitorRectClientLogicalResult,
                screenOffset,
                targetPointScreenPhysical,
                inputPaneOccludeRectScreenLogical);
        });
    }
    COM_END
}

HRESULT WindowHelper::SetLastInputMethod(_In_ test_infra::LastInputDeviceType lastInputType, _In_ xaml::IXamlRoot* xamlRoot)
{
    COM_START
    {
        RunOnUIThread([&] () {
            GetTestHooks()->SetLastInputMethod((xaml_input::LastInputDeviceType)lastInputType, xamlRoot);
        });
    }
    COM_END
}

HRESULT WindowHelper::GetLastInputMethod(_In_ xaml::IXamlRoot* xamlRoot, _Out_ test_infra::LastInputDeviceType* lastDeviceType)
{
    COM_START
    {
        RunOnUIThread([&] () {
            *lastDeviceType = static_cast<test_infra::LastInputDeviceType>(GetTestHooks()->GetLastInputMethod(xamlRoot));
        });
    }
    COM_END
}

HRESULT WindowHelper::SetLastLayoutExceptionElement(_In_ xaml::IUIElement* element)
{
    COM_START
    {
        RunOnUIThread([&]() {
            GetTestHooks()->SetLastLayoutExceptionElement(element);
        });
    }
    COM_END
}

HRESULT WindowHelper::SetPrimaryPointerLastPositionOverride(_In_ wf::Point value, _In_ xaml::IXamlRoot* xamlRoot)
{
    COM_START
    {
        RunOnUIThread([&] () {
            GetTestHooks()->SetPrimaryPointerLastPositionOverride(value, xamlRoot);
        });
    }
    COM_END
}

HRESULT WindowHelper::ClearPrimaryPointerLastPositionOverride(_In_ xaml::IXamlRoot* xamlRoot)
{
    COM_START
    {
        RunOnUIThread([&] () {
            GetTestHooks()->ClearPrimaryPointerLastPositionOverride(xamlRoot);
        });
    }
    COM_END
}


HRESULT WindowHelper::ResetVisualTree()
{
    COM_START_GROUP(L"WindowHelper::ResetVisualTree")
    {
        bool waitForDispatcherIdle = false;
        RunOnUIThread([&] () {
            // Important: Set the window content to null before resetting the visual tree. During test cleanup we'll call
            // ResetWindowContentAndWaitForIdle, which will clear the window content and potentially wait for a frame from
            // Xaml. Since the entire visual tree has been reset, Xaml will skip layout and rendering and the pre-frame
            // callback, which means the wait will wait indefinitely. The test infrastructure is smart enough to skip the
            // wait if the window content is already null, so clear the window content before resetting the visual tree.
            //
            // Also note that the window content is not the root of the visual tree, but a child under it. So clearing the
            // window content is not the same as resetting the visual tree. We only want to set the window content to null
            // if it hasn already been set.
            wrl::ComPtr<xaml::IUIElement> currentRoot;
            WindowHelper::GetWindowContentStatic(&currentRoot, m_win32Host);
            if (currentRoot != nullptr)
            {
                WindowHelper::SetWindowContentStatic(nullptr, m_win32Host);
                waitForDispatcherIdle = true;
            }
        });

        if (waitForDispatcherIdle)
        {
            // Ideally we wouldn't need to do this.  CoreServices has the ability to indicate that we are shutting down
            // and if so indicate will not raise events.  In a perfect world we would simply tell CoreSerices that we
            // are shutting down before we null out the content and then the events wouldn't be raised.  However, it is
            // also used to determine how and when third party components are registered/unregistered and  I don't want
            // to mess with this without more bake time.  So, what we will do is make sure the dispatcher is idle before
            // we reset the visual tree so all event will have been processed.
            m_idleSynchronizer.WaitForIdleDispatcher(HostingDispatcher::Get()->GetDispatcher().Get());
        }

        RunOnUIThread([&]() {
            GetTestHooks()->ResetVisualTree();
        });
    }
    COM_END
}

HRESULT WindowHelper::ShutdownXaml()
{
    COM_START_GROUP(L"WindowHelper::ShutdownXaml")
    {
        RunOnUIThread([&]() {
            HMODULE hModuleMuxc = GetModuleHandle(L"Microsoft.UI.Xaml.Controls.dll");
            typedef void(__stdcall* PfnDeinitializeMUXC)();
            PfnDeinitializeMUXC pfnDeinitializeMUXC = reinterpret_cast<PfnDeinitializeMUXC>(GetProcAddress(hModuleMuxc, "DeinitializeMUXC"));
            pfnDeinitializeMUXC();
        });

        Hosting::HostingMode hostingMode = Hosting::HostingMode::UAP;
        LogThrow_IfFailed(GetHostingMode(&hostingMode));

        if (hostingMode != Hosting::HostingMode::UAP)
        {
            RunOnUIThread([&]() {
                wrl::ComPtr<xaml::IUIElement> currentRoot;
                WindowHelper::GetWindowContentStatic(&currentRoot, m_win32Host);
                if (currentRoot != nullptr)
                {
                    WindowHelper::SetWindowContentStatic(nullptr, m_win32Host);
                }
            });
        }
        else
        {
            RunOnUIThread([&]() {
#ifdef MUX_PRERELEASE
                if (m_suspendedToken.value != 0)
                {
                    wrl::ComPtr<xaml::IApplicationFeature_UwpSupportApi> applicationUwp;
                    LogThrow_IfFailed(m_spApp.As(&applicationUwp));
                    LogThrow_IfFailed(applicationUwp->remove_Suspending(m_suspendedToken));
                    m_suspendedToken = {};
                }
#endif
                m_spApp.Reset();
            });
        }

        // Before we start shutting anything down, we need to tell the registrar (if we have one) to
        // clean up it's DP's.
        if (m_spClosableMetadataRegistrar)
        {
            LogThrow_IfFailedWithMessage(m_spClosableMetadataRegistrar->Close(), L"Failed cleaning up custom metadata");

            // Below methods have the potential to fail which would cause us to leave this method early
            // so we want to reset the registrar here. We don't want to hold onto this guy in case the test class is finally
            // cleaned up and we are holding a reference to it.
            m_spClosableMetadataRegistrar.Reset();
        }

        wrl::ComPtr<IXamlTestHooks> testHooks = nullptr;

        RunOnUIThread([&] () {
            testHooks = GetTestHooks();
        });

        // Enable core shutdown to track memory leaks.
        // Although this is not yet supported on OneCore and phone (see comment below), we still
        // need it for ResetMetadata to clear third party metadata providers.
        s_isShutdownEnabled = true;

        RunOnUIThread([&] () {
            testHooks->SetRuntimeEnabledFeatureOverride(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnableCoreShutdown, true, nullptr);
        });

        BOOLEAN isOneCore = FALSE;
        LogThrow_IfFailed(Utilities::IsOneCoreStatic(&isOneCore));

        RunOnUIThread([&] () {
            // Make sure we cleanup the release queue on every platform, this will be the last thing we do in case any of the following
            // shutdown code puts something in the release queue (probably super unlikely, but doesn't hurt).
            auto cleanupReleaseQueueOnExit = wil::scope_exit([&]{
                RunOnUIThread([&testHooks]() {
                    testHooks->CleanupReleaseQueue();
                });
            });
        });

        if (!!isOneCore)
        {
            // We can't call ShutdownXaml on OneCore due to stale DComp tree issues.
            // This effectively disables leak detection on OneCore. We also skip shutdown on phone
            // because it seems the phone runs timeout due to
            LOG_OUTPUT(L"Skipping shutdown on non-Desktop SKU.");
            RunOnUIThread([this, &testHooks]() {
                if (m_ensureSatelliteDLLCustomDPCleanup)
                {
                    LogThrow_IfFailed(testHooks->EnsureSatelliteDLLCustomDPCleanup());
                    m_ensureSatelliteDLLCustomDPCleanup = false;
                }

                LogThrow_IfFailed(testHooks->ResetMetadata());

                // Controls like CommandBar can open popups when they leave the tree. Close any such resurrected popups.
                testHooks->CloseAllPopupsForTreeReset();
            });

            // Disable core shutdown since it's not supported yet on OneCore and Phone.
            s_isShutdownEnabled = false;

            RunOnUIThread([&testHooks]() {
                testHooks->SetRuntimeEnabledFeatureOverride(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnableCoreShutdown, false, nullptr);
            });

            return S_OK;
        }

        auto postTickEvent = std::make_shared<Event>(L"PostTick");
        if (hostingMode == Hosting::HostingMode::UAP)
        {
            // Setup the post tick callback event. If things aren't cleaned up, it won't be because of
            // timing issues. We can't wait for idle because that relies on the PerFrameCallback which only
            // gets called while we are rendering.
            auto resetCallbackOnExit = wil::scope_exit([&] {
                SetPostTickCallback(nullptr);
                LogThrow_IfFailed(SetIsRenderEnabled(true));
            });

            auto callback = wrl::Callback<test_infra::IPostTickCallback>([&postTickEvent] { postTickEvent->Set(); return S_OK; });
            LogThrow_IfFailed(SetPostTickCallback(callback.Get()));

            // Simulate a device lost so we clean up resources
            LOG_OUTPUT(L"Simulating device lost. Waiting for the post tick event");
            LogThrow_IfFailed(ResetDeviceAndVisuals());

            // Wait for the post tick event. We need to make sure that the device lost and reset visual tree don't happen
            // on the same tick.
            if (s_foregroundWindowCraterArmed)
            {
                postTickEvent->WaitForDefault();
                postTickEvent->Reset();
            }

            // Reset the Visual Tree. This should result in everything the core is holding onto being released.
            // This has to be done after recovering from a device lost because the deployment tree will be reset,
            // causing us to not reset the application object when we recover from device lost.
            LOG_OUTPUT(L"Device lost successful. Resetting the visual tree.");
            LogThrow_IfFailed(ResetVisualTree());

            // Disable rendering so we don't rebuild resources on DeviceLost
            LogThrow_IfFailed(SetIsRenderEnabled(false));

            // Wait for the post tick event. We should have cleaned up device related resources at this point
            // and can proceed to shutting things down.
            LOG_OUTPUT(L"Visual tree reset. Waiting for the post tick event");
            if (s_foregroundWindowCraterArmed)
            {
                // If a test manually reset the visual tree we won't get this event
                postTickEvent->WaitForNoThrow(std::chrono::milliseconds(1000));
            }
        }
        else if (hostingMode == Hosting::HostingMode::WPF)
        {
            LogThrow_IfFailed(ResetVisualTree());
        }

        LOG_OUTPUT(L"Tick event fired: %s. Shutting down xaml and cleaning up release queue", postTickEvent->HasFired() ? L"true" : L"false");
        RunOnUIThread([this, &testHooks]() {
            if (m_ensureSatelliteDLLCustomDPCleanup)
            {
                LogThrow_IfFailed(testHooks->EnsureSatelliteDLLCustomDPCleanup());
                m_ensureSatelliteDLLCustomDPCleanup = false;
            }

            LogThrow_IfFailed(testHooks->ShutdownXaml());
        });

        LOG_OUTPUT(L"Shutdown complete");

        LOG_OUTPUT(L"Resetting Host");
        if (hostingMode == Hosting::HostingMode::WPF)
        {
            LogThrow_IfFailed(m_pTestServices->InitializeHost());
            LogThrow_IfFailed(RestoreForegroundWindow());
        }

        LOG_OUTPUT(L"Reset complete");

        if (hostingMode != Hosting::HostingMode::UAP || Utilities::IsBVT())
        {
            RpcClientEnsureConnected();
            LogThrow_IfFailed(RpcResetInputInjection());
        }
    }
    COM_END
}

HRESULT WindowHelper::EnsureSatelliteDLLCustomDPCleanup()
{
    m_ensureSatelliteDLLCustomDPCleanup = true;
    return S_OK;
}

HRESULT WindowHelper::InjectWindowMessage(_In_ UINT msg, _In_ UINT wParam, _In_ UINT lParam, _In_ xaml::IXamlRoot* xamlRoot)
{
    COM_START_GROUP(L"WindowHelper::InjectWindowMessage")
    {
        RunOnUIThread([&] () {
            GetTestHooks()->InjectWindowMessage(msg, wParam, lParam, xamlRoot);
        });
    }
    COM_END
}

HRESULT WindowHelper::SendWindowMessage(_In_ UINT msg, _In_ UINT wParam, _In_ UINT lParam, _Out_opt_ UINT* returnValue)
{
    LRESULT result = ::SendMessage(GetCurrentWindowHandle(), msg, wParam, lParam);
    if (returnValue)
    {
        *returnValue = wil::safe_cast_failfast<UINT>(result);
    }
    return S_OK;
}

HWND WindowHelper::GetCurrentWindowHandle()
{
    HWND handle = {};

    Hosting::HostingMode hostingMode = Hosting::HostingMode::UAP;
    LogThrow_IfFailed(GetHostingMode(&hostingMode));
    if (hostingMode != Hosting::HostingMode::UAP)
    {
        handle = HostingDispatcher::Get()->GetMainWindowHandle();
    }
    else
    {
        RunOnUIThread([&]()
        {
            wrl::ComPtr<ICoreWindowInterop> spCoreWindowInterop;
            wrl::ComPtr<wuc::ICoreWindow> spCoreWindow = HostingDispatcher::Get()->GetCoreWindow();

            LogThrow_IfFailed(spCoreWindow.As(&spCoreWindowInterop));
            LogThrow_IfFailed(spCoreWindowInterop->get_WindowHandle(&handle));
        });
    }

    return handle;
}

HRESULT WindowHelper::SetDesktopWindowSize(
    unsigned int width,
    unsigned int height
    )
{
    COM_START
    {
        HWND hwnd = GetCurrentWindowHandle();
        Hosting::HostingMode hostingMode = Hosting::HostingMode::UAP;
        LogThrow_IfFailed(GetHostingMode(&hostingMode));
        HWND hwndParent = ::GetParent(hwnd);
        if (hwndParent == NULL && hostingMode != Hosting::HostingMode::UAP)
        {
            hwndParent = hwnd;
        }
        LogThrow_IfFailed(RpcSetDesktopWindowSize(reinterpret_cast<LONG_PTR>(hwndParent), width, height));
    }
    COM_END
}

HRESULT WindowHelper::MoveDesktopWindow(
    unsigned int x,
    unsigned int y
    )
{
    COM_START
    {
        HWND hwnd = GetCurrentWindowHandle();
        Hosting::HostingMode hostingMode = Hosting::HostingMode::UAP;
        Throw::IfFailed(GetHostingMode(&hostingMode));
        HWND hwndParent = ::GetParent(hwnd);
        if (hwndParent == NULL && hostingMode != Hosting::HostingMode::UAP)
        {
            hwndParent = hwnd;
        }
        Throw::IfFailed(RpcMoveDesktopWindow(reinterpret_cast<LONG_PTR>(hwndParent), x, y));
    }
    COM_END
}

HRESULT WindowHelper::IsDesktopWindowMaximized(
        _Out_ BOOL* isMaximized
    )
{
    COM_START
    {
        HWND hwnd = GetCurrentWindowHandle();
        Hosting::HostingMode hostingMode = Hosting::HostingMode::UAP;
        LogThrow_IfFailed(GetHostingMode(&hostingMode));
        HWND hwndParent = ::GetParent(hwnd);
        if (hwndParent == NULL && hostingMode != Hosting::HostingMode::UAP)
        {
            hwndParent = hwnd;
        }
        LogThrow_IfFailed(RpcIsDesktopWindowMaximized(reinterpret_cast<LONG_PTR>(hwndParent), isMaximized));
    }
    COM_END
}

HRESULT WindowHelper::IsWindowActivated(
    _In_ xaml::IXamlRoot* xamlRoot,
    _Out_ BOOLEAN* active
)
{
    COM_START
    {
        RunOnUIThread([&]() {
            GetTestHooks()->IsWindowActivated(xamlRoot, active);
        });
    }
    COM_END
}

HRESULT WindowHelper::SetVisibleBounds(_In_ wf::Rect visibleBounds)
{
    COM_START
    {
        RunOnUIThread([&]() { GetTestHooks()->SetVisibleBounds(visibleBounds); });
    }
    COM_END
}

HRESULT
WindowHelper::SetForceIsFullScreen(BOOLEAN forceIsFullScreen)
{
    COM_START
    {
        RunOnUIThread([&]() {
            GetTestHooks()->SetForceIsFullScreen(!!forceIsFullScreen);
        });
    }
    COM_END
}

HRESULT
WindowHelper::CancelAllConnectedAnimationsAndResetDefaults()
{
    COM_START_GROUP(L"WindowHelper::CancelAllConnectedAnimationsAndResetDefaults")
    {
        RunOnUIThread([&]() {
            LogThrow_IfFailed(GetTestHooks()->CancelAllConnectedAnimationsAndResetDefaults());
        });
    }
    COM_END
}

HRESULT
WindowHelper::ResetThemingToDefaultState()
{
    COM_START_GROUP(L"WindowHelper::ResetThemingToDefaultState")
    {
        wrl::ComPtr<test_infra::IThemingHelper> themingHelper;
        LogThrow_IfFailed(m_pTestServices->get_ThemingHelper(&themingHelper));
        if (themingHelper)
        {
            LogThrow_IfFailed(themingHelper->RestoreDefaultState());
        }
     }
     COM_END
}

bool
WindowHelper::IsLeakDetectionEnabled()
{
    // Leak detection is on by default when shutdown is enabled if not opted out
    if (ErrorHandlingHelper::ShouldIgnoreLeaks())
    {
        return false;
    }

    return s_isShutdownEnabled;
}

void
WindowHelper::OverrideMetadataProvider(xaml_markup::IXamlMetadataProvider* provider)
{
    typedef void (WINAPI * PfnOverrideXamlMetadataProvider)(_In_opt_ xaml_markup::IXamlMetadataProvider*);
    PfnOverrideXamlMetadataProvider pfnOverrideXamlMetadataProvider;

    HMODULE hModuleDXaml = GetModuleHandle(L"Microsoft.UI.Xaml.dll");
    WEX::Common::Throw::IfNull(hModuleDXaml, L"Failed to get module handle for Microsoft.UI.Xaml.dll");

    pfnOverrideXamlMetadataProvider =
        reinterpret_cast<PfnOverrideXamlMetadataProvider>(GetProcAddress(hModuleDXaml, "OverrideXamlMetadataProvider"));

    if (pfnOverrideXamlMetadataProvider)
    {
        pfnOverrideXamlMetadataProvider(provider);
    }
    else
    {
        WEX::Common::Throw::LastError(L"GetProcAddress failed for OverrideXamlMetadataProvider");
    }
}

HRESULT
WindowHelper::CreateNewView(_In_ test_infra::IViewCreatedCallback* uiSetupCallback, _COM_Outptr_ test_infra::ISecondaryView** secondaryView)
{
    COM_START_GROUP(L"WindowHelper::CreateNewView")
    {
        // In order to display a view and bring it to the front we need a few things:
        //  1. We need to get current ApplicationView Id so that we can restore it
        //  2. We need to create a new view

        wrl::ComPtr<test_infra::ISecondaryView> newView;
        wrl::ComPtr<msy::IDispatcherQueue> newDispatcher;

        LogThrow_IfFailed(wrl::MakeAndInitialize<SecondaryView>(&newView));

        // Get the view and subscribe to it's activated event

        LogThrow_IfFailed(newView->get_Dispatcher(&newDispatcher));

        wrl::ComPtr<xaml::IWindow> window;
        EventRegistrationToken activatedToken = {};
        std::shared_ptr<Event> activatedEvent = std::make_shared<Event>(L"Activated");

        RunOnDispatcherThread(newDispatcher, [&]
        {
            // First call the callback. This should only setup the visual tree.
            LogThrow_IfFailed(uiSetupCallback->Invoke());

            // Activate the window.
            window = GetXamlWindow();

            LogThrow_IfFailed(window->add_Activated(
                wrl::Callback<wf::ITypedEventHandler<IInspectable*, xaml::WindowActivatedEventArgs*>>(
                [&](IInspectable*, xaml::IWindowActivatedEventArgs*) -> HRESULT {
                activatedEvent->Set();
                return S_OK;
            }).Get(), &activatedToken));

            LOG_OUTPUT(L"Activating secondary window");
            return window->Activate();
        });

        if (!activatedEvent->WaitForNoThrow(std::chrono::milliseconds(2000)))
        {
            LOG_WARNING(L"Failed to wait for the activated event");
        }

        RunOnDispatcherThread(newDispatcher, [&]
        {
            LogThrow_IfFailed(window->remove_Activated(activatedToken));
        });


        *secondaryView = newView.Detach();
    }
    COM_END

}

HRESULT
WindowHelper::BringSecondaryViewToFront(_In_ test_infra::ISecondaryView* view)
{
    COM_START_GROUP(L"WindowHelper::BringSecondaryViewToFront")
    {
        wrl::ComPtr<wac::ICoreApplicationView> coreView;
        LogThrow_IfFailed(view->get_View(&coreView));

        HostingDispatcher::Get()->SetSecondaryView(coreView.Get());

        SwitchToView(coreView);
    }
    COM_END
}

HRESULT
WindowHelper::BringMainViewToFront()
{
    COM_START_GROUP(L"WindowHelper::BringMainViewToFront")
    {
        HostingDispatcher::Get()->ResetSecondaryView();

        wrl::ComPtr<wac::ICoreImmersiveApplication> application;
        LogThrow_IfFailed(wf::GetActivationFactory(
        wrl::Wrappers::HStringReference(RuntimeClass_Windows_ApplicationModel_Core_CoreApplication).Get(),
        &application));
        wrl::ComPtr<wac::ICoreApplicationView> view;
        LogThrow_IfFailed(application->get_MainView(&view));

        SwitchToView(view);
    }
    COM_END

}

HRESULT
WindowHelper::get_CurrentDispatcher(_COM_Outptr_ msy::IDispatcherQueue** dispatcherQueue)
{
    COM_START
    {
        *dispatcherQueue = HostingDispatcher::Get()->GetDispatcher().Detach();
    }
    COM_END
}

HRESULT WindowHelper::GetUIAWindowHandle(_Out_ UINT64* uiWindowHandle)
{
    COM_START
    {
        *uiWindowHandle = reinterpret_cast<UINT64>(GetCurrentWindowHandle());
    }
    COM_END
}

HRESULT WindowHelper::SimulateInputPaneOccludedRect(_In_ xaml::IXamlRoot* xamlRoot, wf::Rect occludedRect)
{
    COM_START
    {
        RunOnUIThread([&]() { LogThrow_IfFailed(GetTestHooks()->SimulateInputPaneOccludedRect(xamlRoot, occludedRect)); });
    }
    COM_END
}

HRESULT WindowHelper::GetLightsTargetingElement(
    xaml::IUIElement* target,
    wfc::IVector<xaml_media::XamlLight*>* lights)
{
    COM_START
    {
        RunOnUIThread([&]() { GetTestHooks()->GetLightsTargetingElement(target, lights); });
    }
    COM_END
}

HRESULT WindowHelper::GetElementsTargetedByLight(
    xaml_media::IXamlLight* light,
    wfc::IVector<xaml::UIElement*>* targets)
{
    COM_START
    {
        RunOnUIThread([&]() { GetTestHooks()->GetElementsTargetedByLight(light, targets); });
    }
    COM_END
}

HRESULT WindowHelper::GetCountOfVisualsTargeted(
    _In_ xaml_media::IXamlLight* light,
    _In_ xaml::IUIElement* element,
    _Out_ int* count)
{
    COM_START
    {
        RunOnUIThread([&]() { GetTestHooks()->GetCountOfVisualsTargeted(light, element, count); });
    }
    COM_END
}

HRESULT WindowHelper::GetRealCompositionSurface(
    _In_ xaml_media::ILoadedImageSurface *loadedImageSurface,
    _Outptr_ WUComp::ICompositionSurface **realCompositionSurface)
{
    COM_START
    {
        RunOnUIThread([&]() {
            LogThrow_IfFailed(GetTestHooks()->GetRealCompositionSurface(loadedImageSurface, realCompositionSurface));
        });
    }
    COM_END
}

HRESULT WindowHelper::SetHdrOutputOverride(BOOLEAN value)
{
    COM_START
    {
        RunOnUIThread([&]() {
            LogThrow_IfFailed(GetTestHooks()->SetHdrOutputOverride(!!value));
        });
    }
    COM_END
}

HRESULT WindowHelper::GetWantsRenderingEvent(_Out_ BOOLEAN* wantsRenderingEvent)
{
    COM_START
    {
        RunOnUIThread([&]() {
            GetTestHooks()->GetWantsRenderingEvent(wantsRenderingEvent);
        });
    }
    COM_END
}

HRESULT WindowHelper::GetWantsCompositionTargetRenderedEvent(_Out_ BOOLEAN* wantsCompositionTargetRenderedEvent)
{
    COM_START
    {
        RunOnUIThread([&]() {
            GetTestHooks()->GetWantsCompositionTargetRenderedEvent(wantsCompositionTargetRenderedEvent);
        });
    }
    COM_END
}

HRESULT WindowHelper::SetThreadingAssertOverride(BOOLEAN value)
{
    COM_START
    {
        RunOnUIThread([&]
        {
            GetTestHooks()->SetThreadingAssertOverride(!!value);
        });
    }
    COM_END
}

HRESULT WindowHelper::AddTestLTE(
    xaml::IUIElement* lteTarget,
    xaml::IUIElement* lteParent,
    test_infra::LTEParentMode lteParentMode,
    BOOLEAN isAbsolutelyPositioned,
    xaml::IUIElement** lte)
{
    COM_START
    {
        RunOnUIThread([&]() { GetTestHooks()->AddTestLTE(lteTarget, lteParent, lteParentMode == test_infra::LTEParentMode::LTEParentMode_RootVisual, lteParentMode == test_infra::LTEParentMode::LTEParentMode_PopupRoot, isAbsolutelyPositioned, lte); });
    }
    COM_END
}

HRESULT WindowHelper::RemoveTestLTE(xaml::IUIElement* lte)
{
    COM_START
    {
        RunOnUIThread([&]() { GetTestHooks()->RemoveTestLTE(lte); });
    }
    COM_END
}

HRESULT WindowHelper::ClearTestLTEs()
{
    COM_START
    {
        RunOnUIThread([&]() { GetTestHooks()->ClearTestLTEs(); });
    }
    COM_END
}

HRESULT WindowHelper::IsTrackingEffectiveVisibility(xaml::IUIElement* element, _Out_ BOOLEAN* isTracking)
{
    COM_START
    {
        RunOnUIThread([&]() { *isTracking = GetTestHooks()->IsTrackingEffectiveVisibility(element); });
    }
    COM_END
}

HRESULT WindowHelper::IsKeepingVisible(xaml::IUIElement* element, _Out_ BOOLEAN* isKeepingVisible)
{
    COM_START
    {
        RunOnUIThread([&]() { *isKeepingVisible = GetTestHooks()->IsKeepingVisible(element); });
    }
    COM_END
}

HRESULT WindowHelper::RequestKeepAlive(xaml::IUIElement* element)
{
    COM_START
    {
        RunOnUIThread([&]() { GetTestHooks()->RequestKeepAlive(element); });
    }
    COM_END
}

HRESULT WindowHelper::ReleaseKeepAlive(xaml::IUIElement* element)
{
    COM_START
    {
        RunOnUIThread([&]() { GetTestHooks()->ReleaseKeepAlive(element); });
    }
    COM_END
}

HRESULT WindowHelper::IsStoryboardActive(_In_ xaml_animation::IStoryboard* board, _Out_ BOOLEAN* isActive)
{
    COM_START
    {
        RunOnUIThread([&]() { *isActive = GetTestHooks()->IsStoryboardActive(board); });
    }
    COM_END
}

HRESULT WindowHelper::GetElementInputWindow(_In_ xaml::IUIElement* element, _Out_ UINT64* inputHwnd)
{
    COM_START
    {
        RunOnUIThread([&]() {
            HWND window = NULL;
            GetTestHooks()->GetElementInputWindow(element, &window);
            *inputHwnd = reinterpret_cast<UINT64>(window);
        });
    }
    COM_END
}

int
WindowHelper::GetApplicationViewIdForWindow(const wrl::ComPtr<wuc::ICoreWindow>& window)
{
    int id = 0;

    wrl::ComPtr<wuv::IApplicationViewInteropStatics> statics;
    LogThrow_IfFailed(wf::GetActivationFactory(wrl::Wrappers::HStringReference(RuntimeClass_Windows_UI_ViewManagement_ApplicationView).Get(), &statics));

    LogThrow_IfFailedWithMessage(statics->GetApplicationViewIdForWindow(window.Get(), &id), L"Failed getting current ApplicationViewId for CoreWindow");

    return id;
}

void
WindowHelper::SwitchToView(const wrl::ComPtr<wac::ICoreApplicationView>& view)
{
    auto dispatcher = GetDispatcherForView(view);

    auto completedEvent = std::make_shared<Event>(L"SwitchCompleted");
    auto spCompletionCallback = wrl::Callback<wf::IAsyncOperationCompletedHandler<bool>>(
        [completedEvent](wf::IAsyncOperation<bool>*, AsyncStatus) -> HRESULT
    {
        completedEvent->Set();
        return S_OK;
    });

    RunOnDispatcherThread(dispatcher, [&]
    {
        wrl::ComPtr<wuc::ICoreWindow> coreWindow = HostingDispatcher::Get()->GetCoreWindow();

        int viewId = GetApplicationViewIdForWindow(coreWindow);
        wrl::ComPtr<wuv::IApplicationViewSwitcherStatics> viewSwitcher;

        wrl::ComPtr<wf::IAsyncOperation<bool>> action;

        LogThrow_IfFailed(wf::GetActivationFactory(
            wrl::Wrappers::HStringReference(RuntimeClass_Windows_UI_ViewManagement_ApplicationViewSwitcher).Get(),
            &viewSwitcher));

        // We use TryShowAsStandalone since SwitchView (non-intuitively) causes explorer.exe to crash when switching
        // between views that aren't in the same window.
        LogThrow_IfFailed(viewSwitcher->TryShowAsStandaloneAsync(viewId, &action));
        LogThrow_IfFailed(action->put_Completed(spCompletionCallback.Get()));
    });

    completedEvent->WaitForDefault();

    // WaitForIdle now that we've updated the dispatcher.
    LogThrow_IfFailed(WaitForIdle());
}

wrl::ComPtr<msy::IDispatcherQueue> WindowHelper::GetDispatcherForView(const wrl::ComPtr<wac::ICoreApplicationView>& view)
{
    wrl::ComPtr<wuc::ICoreWindow> coreWindow;
    LogThrow_IfFailed(view->get_CoreWindow(&coreWindow));

    wrl::ComPtr<wuc::ICoreWindow5> spCoreWindow5;
    LogThrow_IfFailed(coreWindow.As(&spCoreWindow5));

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

// Reset the DComp device and make sure to clean up MaterialHelper
void WindowHelper::ResetDeviceAndVisualsHelper()
{
    RunOnUIThread([&]() { LogThrow_IfFailed(GetTestHooks()->ResetDeviceAndVisualsAndDManip()); });
}

// Not meant to be called on the UI thread
void WindowHelper::TickUIThreadAfterDeviceLostIfNeeded()
{
    Hosting::HostingMode hostingMode;
    LogThrow_IfFailed(Hosting::GetHostingMode(&hostingMode));
    if (hostingMode != Hosting::HostingMode::UAP)
    {
        // In Xaml islands mode we have a race condition between injecting a device lost at the beginning of the test
        // and setting the window content. Setting the window content will instantiate a Xaml island, and the device
        // lost will be handled on the UI thread by releasing the compositor, which will in turn close the Xaml island
        // before we get a chance to use it. Therefore, it's important that the device lost be handled first, before
        // we set the window content and create the Xaml island. Force a tick here to make that happen.
        LogThrow_IfFailed(SynchronouslyTickUIThread(1));
    }
}

HRESULT WindowHelper::SetSuspendOffThreadDecoding(_In_ BOOLEAN isOffThreadDecodingSuspended)
{
    COM_START
    {
        RunOnUIThread([&] () {
            GetTestHooks()->SetSuspendOffThreadDecoding(!!isOffThreadDecodingSuspended);
        });
    }
    COM_END
}

HRESULT WindowHelper::SetSuspendSurfaceUpdates(_In_ BOOLEAN isSuspended)
{
    COM_START
    {
        RunOnUIThread([&] () {
            GetTestHooks()->SetSuspendSurfaceUpdates(!!isSuspended);
        });
    }
    COM_END
}

HRESULT WindowHelper::ThrottleImageTaskDispatcher(BOOLEAN enableThrottling, unsigned int numberOfTasksAllowedToDispatch)
{
    RunOnUIThread([&]()
    {
        TestServicesStatics::GetTestHooks()->ThrottleImageTaskDispatcher(enableThrottling, numberOfTasksAllowedToDispatch);
    });
    return S_OK;
}

HRESULT WindowHelper::RequestExecuteImageTaskDispatcher()
{
    RunOnUIThread([&]()
    {
        TestServicesStatics::GetTestHooks()->RequestExecuteImageTaskDispatcher();
    });
    return S_OK;
}

HRESULT WindowHelper::SetCaretBrowsingModeGlobal(BOOLEAN caretBrowsingModeEnable, BOOLEAN caretBrowsingDialogNotPopAgain)
{
    COM_START
    {
        RunOnUIThread([&] () {
            GetTestHooks()->SetCaretBrowsingModeGlobal(!!caretBrowsingModeEnable, !!caretBrowsingDialogNotPopAgain);
        });
    }
    COM_END
}

HRESULT WindowHelperStatics::WrapInAgileDispatcherQueueHandler(_In_ test_infra::IManagedDispatcherQueueCallback* callback, _COM_Outptr_ msy::IDispatcherQueueHandler** handler)
{
    wrl::ComPtr<test_infra::IManagedDispatcherQueueCallback> spCallback(callback);

    *handler = AgileCallback<msy::IDispatcherQueueHandler>([spCallback]() -> HRESULT
    {
        HRESULT invokeResult = S_OK;
        // Don't want to throw an exception into the Dispatcher queue, hence, wrapped it up in a SafeInvoke
        invokeResult = WEX::SafeInvoke([&]() -> bool
        {
          LogThrow_IfFailed( spCallback->Invoke() );
          return true;
        });

        return invokeResult;
    }).Detach();

    return S_OK;
}

HRESULT WindowHelper::SetGCCollectCallback(_In_ test_infra::IGCCollectCallback* callback )
{
    m_gccollectCallback = callback;

    return S_OK;
}

// Calling garbage collect for managed code in native code at specific spots
// to ensure consistent memory cleanup and avoid any leaks
HRESULT WindowHelper::GCCollect()
{
    // Only require this callback to run when the caller is a managed test
    // TODO: Need a consistent way to distinguish between native and managed code for running GC check
    if (m_win32Host != nullptr)
    {
        // WPF tests (even native ones) run in a managed host, and also need GC.Collect.
        LogThrow_IfFailed(m_win32Host->GCCollect());
    }
    else if (m_gccollectCallback != nullptr)
    {
        LogThrow_IfFailed(m_gccollectCallback->Invoke());
    }
    return S_OK;
}

HRESULT WindowHelper::ForceShadowsPolicy(_In_ BOOLEAN forceShadowsOn)
{
    COM_START
    {
        RunOnUIThread([&]()
        {
            wrl::ComPtr<IXamlTestHooks> windowTestHooks = WindowHelper::GetTestHooks();
            windowTestHooks->ForceShadowsPolicy(forceShadowsOn);
        });
    }
    COM_END
}

HRESULT WindowHelper::ClearShadowPolicyOverrides()
{
    COM_START
    {
        RunOnUIThread([&]()
        {
            wrl::ComPtr<IXamlTestHooks> windowTestHooks = WindowHelper::GetTestHooks();
            windowTestHooks->ClearShadowPolicyOverrides();
        });
    }
    COM_END
}

HRESULT WindowHelper::SetXamlVisibilityOverride(_In_ BOOLEAN isVisible)
{
    COM_START
    {
        RunOnUIThread([&]()
        {
            wrl::ComPtr<IXamlTestHooks> windowTestHooks = WindowHelper::GetTestHooks();
            windowTestHooks->SetXamlVisibilityOverride(isVisible);
        });
    }
    COM_END
}

HRESULT WindowHelper::SetBrushForXamlRoot(
    _In_ xaml_media::IXamlCompositionBrushBase* xcbb,
    _In_ IInspectable* xamlRoot,
    _In_ IInspectable* compBrush)
{
    COM_START
    {
        RunOnUIThread([&]()
        {
            GetTestHooks()->SetBrushForXamlRoot(xcbb, xamlRoot, compBrush);
        });
    }
    COM_END
}

HRESULT WindowHelper::GetBrushForXamlRoot(
    _In_ xaml_media::IXamlCompositionBrushBase* xcbb,
    _In_ IInspectable* xamlRoot,
    _Outptr_ IInspectable** compBrush)
{
    COM_START
    {
        RunOnUIThread([&]()
        {
            GetTestHooks()->GetBrushForXamlRoot(xcbb, xamlRoot, compBrush);
        });
    }
    COM_END
}

HRESULT WindowHelper::GetElementsRenderedCount(_Out_ int* elementsRendered)
{
    COM_START
    {
        RunOnUIThread([&]()
        {
            wrl::ComPtr<IXamlTestHooks> windowTestHooks = WindowHelper::GetTestHooks();
            windowTestHooks->GetElementsRenderedCount(elementsRendered);
        });
    }
    COM_END
}

HRESULT WindowHelper::PauseNewDispatchForTest()
{
    COM_START
    {
        RunOnUIThread([&]()
        {
            wrl::ComPtr<IXamlTestHooks> windowTestHooks = WindowHelper::GetTestHooks();
            windowTestHooks->PauseNewDispatchForTest();
        });
    }
    COM_END
}

HRESULT WindowHelper::ResumeNewDispatchForTest()
{
    COM_START
    {
        RunOnUIThread([&]()
        {
            wrl::ComPtr<IXamlTestHooks> windowTestHooks = WindowHelper::GetTestHooks();
            windowTestHooks->ResumeNewDispatchForTest();
        });
    }
    COM_END
}

HRESULT WindowHelper::GetImageSourceMaxSize(
    _In_ xaml_media::IImageSource* imageSource,
    _Out_ wf::Size* maxSize)
{
    COM_START
    {
        RunOnUIThread([&]()
        {
            GetTestHooks()->GetImageSourceMaxSize(imageSource, maxSize);
        });
    }
    COM_END
}

HRESULT WindowHelper::GetInputPaneForMainView(
    _Outptr_ wuv::IInputPane** inputPane)
{
    COM_START

    wrl::ComPtr<wuv::IInputPaneStatics> inputPaneStatics;

    LogThrow_IfFailed(wf::GetActivationFactory(
        wrl::Wrappers::HStringReference(RuntimeClass_Windows_UI_ViewManagement_InputPane).Get(),
        &inputPaneStatics));

    Hosting::HostingMode hostingMode = Hosting::HostingMode::UAP;
    LogThrow_IfFailed(GetHostingMode(&hostingMode));
    if (hostingMode == Hosting::HostingMode::WPF)
    {
        HWND hwnd = GetCurrentWindowHandle();

        wrl::ComPtr<::IInputPaneInterop> interop;
        LogThrow_IfFailed(inputPaneStatics.As(&interop));
        LogThrow_IfFailed(interop->GetForWindow(hwnd, IID_PPV_ARGS(inputPane)));
    }
    else
    {
        LogThrow_IfFailed(inputPaneStatics->GetForCurrentView(inputPane));
    }

    COM_END
}

HRESULT WindowHelper::GetXamlRoots(
    _Outptr_ wfc::IVectorView<xaml::XamlRoot*>** xamlRoots)
{
    COM_START

    RunOnUIThread([&]() {
        GetTestHooks()->GetAllXamlRoots(xamlRoots);
    });

    COM_END
}

HRESULT WindowHelper::GetWindows(_Outptr_ wfc::IVectorView<xaml::Window*>** windows)
{
    COM_START_GROUP(L"WindowHelper::GetWindows")
    {
        RunOnUIThread([&] () {
            wrl::ComPtr<xaml::IApplicationStatics> applicationStatics;
            wrl::ComPtr<xaml::IApplication> application;
            wrl::ComPtr<xaml::IFrameworkApplicationPrivate> applicationPrivate;

            LogThrow_IfFailed(wf::GetActivationFactory(
                wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Application).Get(),
                &applicationStatics));
            LogThrow_IfFailed(applicationStatics->get_Current(&application));
            LogThrow_IfFailed(application.As(&applicationPrivate));

            LogThrow_IfFailed(applicationPrivate->get_Windows(windows));
        });
    }
    COM_END
}

HRESULT WindowHelper::DetachMemoryManagerEvents()
{
    COM_START

    RunOnUIThread([&]() {
        GetTestHooks()->DetachMemoryManagerEvents();
    });

    COM_END
}

HRESULT WindowHelper::GetElementRenderedVisuals(
    _In_ xaml::IUIElement* element,
    wfc::IVector<IInspectable*>* visuals)
{
    COM_START
    {
        RunOnUIThread([&]() { GetTestHooks()->GetElementRenderedVisuals(element, visuals); });
    }
    COM_END
}

HRESULT WindowHelper::GetErrorHandlingTestHooks(_Outptr_opt_ test_infra::IXamlErrorTestHooks** errorTestHooks)
{
    COM_START

    RunOnUIThread([&]() {
        GetTestHooks()->GetErrorHandlingTestHooks(reinterpret_cast<::IXamlErrorTestHooks**>(errorTestHooks));
    });

    COM_END
}

} }
