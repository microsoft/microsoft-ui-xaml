// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <WindowHelper.h>
#include <XamlTailored.h>
#include <Handle.h>
#include <TestEvent.h>

#include "RpcClient.h"

#include <WaitForDebugger.h>
#include <TestServices.h>
#include "Utilities.h"
#include "FontHelper.h"
#include "WindowHelper.h"
#include "InputHelper.h"
#include "KeyboardHelper.h"
#include "ErrorHandlingHelper.h"
#include "ThemingHelper.h"
#include "PredictableDManipEnabler.h"
#include <RuntimeEnabledFeaturesEnum.h>
#include <windows.applicationmodel.core.h>
#include <corewindow.h>
#include <IXamlTestHooks-win.h>
#include "HostingDispatcher.h"
#include "Hosting.h"
#include "Win32Hosting.h"
#include <ShellScalingApi.h>
#include <PrivateModule.h>

using namespace WEX::Common;
using namespace WEX::TestExecution;
using namespace Microsoft::UI::Xaml::Tests::Common;

namespace Private { namespace Infrastructure {

TestServicesStatics::TestServicesStatics() {}

TestServicesStatics::~TestServicesStatics()
{
    Hosting::HostingMode hostingMode = Hosting::HostingMode::UAP;
    LogThrow_IfFailed(GetHostingMode(&hostingMode));

    // In WPF hosting mode, we have already destructed the dispatcher
    // and closed our host. As a result, we dont have any cleanup to do,
    // and cannot dispatch to a thread.
    if (hostingMode == Hosting::HostingMode::UAP)
    {
        RunOnUIThread([&]() {
            if (m_activatedToken.value != 0)
            {
                LogThrow_IfFailed(m_spWindow->remove_Activated(m_activatedToken));
                m_activatedToken = {};
            }
        });
    }

    if (hostingMode == Hosting::HostingMode::WPF)
    {
        auto& moduleRef = PrivateInfraModule::GetModule();
        if (moduleRef.IsFinalRelease())
        {
            m_spWin32Host.Detach();
        }
    }
}

bool TestServicesStatics::s_isInitialized = false;

HRESULT TestServicesStatics::RuntimeClassInitialize()
{
    ::Private::Infrastructure::Utilities::CheckForBVTMode();
    COM_START_GROUP(L"TestServicesStatics::RuntimeClassInitialize");
    {
        WaitForDebugger();

        LogThrow_IfFailed(InitializeHost());

        Hosting::HostingMode hostingMode = Hosting::HostingMode::UAP;
        LogThrow_IfFailed(GetHostingMode(&hostingMode));

        if (hostingMode == Hosting::HostingMode::UAP)
        {
            RunOnUIThread([&] () {
                wrl::ComPtr<xaml::IWindowStatics> spWindowStatics;
                FAIL_FAST_IF_FAILED(wf::GetActivationFactory(
                    wrl::Wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Window).Get(), &spWindowStatics));
                FAIL_FAST_IF_FAILED(spWindowStatics->get_Current(&m_spWindow));
                FAIL_FAST_IF_FAILED(m_spWindow->add_Activated(
                    wrl::Callback<wf::ITypedEventHandler<IInspectable*, xaml::WindowActivatedEventArgs*>>(
                    [&] (IInspectable*, xaml::IWindowActivatedEventArgs* eventArgs) -> HRESULT {
                        return OnAppActivated(eventArgs);
                    }).Get(), &m_activatedToken));
            });
        }
        else if (hostingMode == Hosting::HostingMode::Win32Explicit)
        {
            LOG_OUTPUT(L"Win32Explicit tests should not be touching TestServices.");
            LogThrow_IfFailed(E_INVALIDARG);
            return S_OK;
        }

        BOOLEAN isOneCore = false;
        FAIL_FAST_IF_FAILED(Utilities::IsOneCoreStatic(&isOneCore));

        FAIL_FAST_IF_FAILED(wrl::MakeAndInitialize<Utilities>(&m_spUtilities));

        // Changing the default font is going to change the vast majority of our masters, and we could update those at this point, however, we don't have the
        // final font yet, so they could very well change again.  So, at least for now, in tests, we will turn of the DWrite typographic model.
        FAIL_FAST_IF_FAILED(m_spUtilities->SetRuntimeEnabledFeatureOverride(static_cast<int>(RuntimeFeatureBehavior::RuntimeEnabledFeature::DisableDWriteTypographicModel), true, nullptr));

        ClearPrimaryLanguageOverride();

        FAIL_FAST_IF_FAILED(wrl::MakeAndInitialize<InputHelper>(&m_spInputHelper));
        FAIL_FAST_IF_FAILED(wrl::MakeAndInitialize<FontHelper>(&m_spFontHelper));
        FAIL_FAST_IF_FAILED(wrl::MakeAndInitialize<ErrorHandlingHelper>(&m_spErrorHandlingHelper));
        FAIL_FAST_IF_FAILED(wrl::MakeAndInitialize<ThemingHelper>(&m_spThemingHelper));

        const bool testFontCollectionDeployed = !Utilities::IsBVT(); // BVTs dont deploy the custom font collection
        if (testFontCollectionDeployed)
        {
            SetCustomSystemFontCollection();
        }

        // Force our unit tests to send XAML text through the XBFv1->XBFv2 conversion codepath to
        // get extra coverage on new XBFv2 code paths.
        BOOLEAN wasEnforceXbfV2StreamPreviouslyEnabled;
        FAIL_FAST_IF_FAILED(m_spUtilities->SetRuntimeEnabledFeatureOverride(static_cast<int>(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream), true, &wasEnforceXbfV2StreamPreviouslyEnabled));

        // Turn on debug visual tags - helps DComp dumps be more readable
        BOOLEAN unused;
        FAIL_FAST_IF_FAILED(m_spUtilities->SetRuntimeEnabledFeatureOverride(static_cast<int>(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnableVisualDebugTags), true, &unused));

        // Force RTB code to use feature key to decide if it uses SpriteVisuals capture
        FAIL_FAST_IF_FAILED(m_spUtilities->SetRuntimeEnabledFeatureOverride(static_cast<int>(RuntimeFeatureBehavior::RuntimeEnabledFeature::RenderTargetBitmapUsingSpriteVisualsTestMode), true, &unused));

        // Turn on ThemeShadow feature for tests
        // Just setting the runtime feature is not enough, because ProjectedShadowManager already read and cached the feature
        // value when we did InitializeHost. We also can't force a full DComp visual device reset here because that creates
        // leaks in Xaml islands. Use the specialized test hook.
        FAIL_FAST_IF_FAILED(m_spWindowHelper->ForceShadowsPolicy(true));

        // For DManip tests to produce stable results, we need to tweak a few DManip settings.
        m_spPredictableDManipEnabler.reset(new PredictableDManipEnabler());

        // Wake up VM.
        ::SetThreadExecutionState(ES_DISPLAY_REQUIRED);

        if (!isOneCore)
        {
            // With MDA there exists a race condition in win32k where the console window and MDA window will
            // fight to end up on top. We call ShowWindow SW_SHOWMINNOACTIVE here to make the console window not active.
            const auto consoleWindow = GetConsoleWindow();
            // It's possible to load the test infrastructure client in normal apps as well as TAEF processes
            // and indeed our performance tests do this. In that case we don't have a Console window HWND and
            // need to perform no action here. Also, TAEF tests that use TE.ProcessHost.UAP.exe will not have
            // a console window.
            if (consoleWindow != NULL)
            {
                LOG_OUTPUT(L"Client: Minimizing console window");
                Throw::LastErrorIfFalse(!!::ShowWindow(consoleWindow, SW_SHOWMINNOACTIVE), L"Failed to call ShowWindow on the console window.");
            }

            FAIL_FAST_IF_FAILED(m_spWindowHelper->MaximizeDesktopWindow());
            LOG_OUTPUT(L"  > Setting Xaml as the foreground window.");
            FAIL_FAST_IF_FAILED(m_spWindowHelper->RestoreForegroundWindow());

            for (int i = 0; i < 10 && !WindowHelper::IsForegroundWindowStatic(); i++)
            {
                LOG_OUTPUT(L"  > Xaml is still not the foreground window. Wait 500ms.");
                Sleep(500);
            }

            if (WindowHelper::IsForegroundWindowStatic())
            {
                LOG_OUTPUT(L"  > Xaml is now the foreground window.");
            }
            else
            {
                LOG_WARNING(L"  > Test starting, but Xaml is not the foreground window. Input injection may fail to reach Xaml and cause test failures.");
            }

            // Hide focus rectangles by default
            FAIL_FAST_IF_FAILED(m_spWindowHelper->SendWindowMessage(WM_UPDATEUISTATE, MAKEWPARAM(UIS_SET, UISF_HIDEACCEL | UISF_HIDEFOCUS), 0, nullptr));
            FAIL_FAST_IF_FAILED(m_spWindowHelper->WaitForIdle());
        }
    }
    COM_END
}

HRESULT TestServicesStatics::InitializeHost()
{
    return InitializeHostAndDpiAwarenessContext(false /* initializeDpiAwarenessContext */);
}

HRESULT TestServicesStatics::InitializeHostAndDpiAwarenessContext(boolean initializeDpiAwarenessContext)
{
    return InitializeHostAndDpiAwarenessContextAndCore(initializeDpiAwarenessContext, true /* initCore*/);
}

HRESULT TestServicesStatics::InitializeHostAndDpiAwarenessContextAndCore(boolean initializeDpiAwarenessContext, boolean initCore)
{
    Hosting::HostingMode hostingMode = Hosting::HostingMode::UAP;
    LogThrow_IfFailed(GetHostingMode(&hostingMode));

    wrl::ComPtr<msy::IDispatcherQueue> dispatcher;

    if (hostingMode == Hosting::HostingMode::Win32Explicit)
    {
        LOG_OUTPUT(L"Win32Explicit tests should not be touching TestServices.");
        LogThrow_IfFailed(E_INVALIDARG);
    }

    LOG_OUTPUT(L"InitializeHost has been initiated.");
    FAIL_FAST_IF_FAILED(DeInitializeHost());

    HWND mainWindowHandle = {};

    if (hostingMode == Hosting::HostingMode::WPF)
    {
        LOG_OUTPUT(L"Hosting mode is WPF");

        String value;
        test_infra::Hosting::DpiAwarenessContext dpiAwarenessContext = test_infra::Hosting::DpiAwarenessContext::DpiAwarenessContext_PerMonitorAwareV2;

        // TAEF's TestData can only be read during a test method, test setup method, or test cleanup method. Tests that care about
        // DpiAwarenessContext must call InitializeHost again explicitly at the beginning of their test method. Don't bother trying
        // to read DpiAwarenessContext here when we're first initializing the host for a test.
        if (initializeDpiAwarenessContext)
        {
            if (SUCCEEDED(TestData::TryGetValue(L"DpiAwarenessContext", value)))
            {
                if (value.IsEmpty() || value.CompareNoCase(L"Unaware") == 0)
                {
                    dpiAwarenessContext = test_infra::Hosting::DpiAwarenessContext::DpiAwarenessContext_Unaware;
                    LOG_OUTPUT(L"DPI awareness context is \"Unaware\"");
                }
                else if (value.CompareNoCase(L"System") == 0)
                {
                    dpiAwarenessContext = test_infra::Hosting::DpiAwarenessContext::DpiAwarenessContext_SystemAware;
                    LOG_OUTPUT(L"DPI awareness context is \"System\"");
                }
                else if (value.CompareNoCase(L"PerMonitor") == 0)
                {
                    dpiAwarenessContext = test_infra::Hosting::DpiAwarenessContext::DpiAwarenessContext_PerMonitorAware;
                    LOG_OUTPUT(L"DPI awareness context is \"PerMonitor\"");
                }
                else if (value.CompareNoCase(L"UnawareGdiScaled") == 0)
                {
                    dpiAwarenessContext = test_infra::Hosting::DpiAwarenessContext::DpiAwarenessContext_UnawareGdiScaled;
                    LOG_OUTPUT(L"DPI awareness context is \"UnawareGdiScaled\"");
                }
                else
                {
                    // Default to PerMonitorAwareV2
                    LOG_OUTPUT(L"DPI awareness context is \"PerMonitorV2\"");
                }
            }
            else
            {
                LOG_OUTPUT(L"TestData not available");
            }
        }
        else
        {
            PROCESS_DPI_AWARENESS dpiAwareness = {};
            LogThrow_IfFailed(::GetProcessDpiAwareness(nullptr, &dpiAwareness));
            LOG_OUTPUT(L"Current DPI awareness is %d", static_cast<int>(dpiAwareness));
            if (dpiAwareness != PROCESS_PER_MONITOR_DPI_AWARE)
            {
                LOG_OUTPUT(L"Setting DPI awareness to PROCESS_PER_MONITOR_DPI_AWARE");
                HRESULT hr = ::SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
                // SetProcessDpiAwareness will return an E_ACCESSDENIED if we set dpi awareness more then once per process
                Throw::IfFalse(hr == S_OK || hr == E_ACCESSDENIED, hr);
            }

            LOG_OUTPUT(L"Setting thread DPI awareness context to DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2");
            DPI_AWARENESS_CONTEXT oldDpiAwarenessContext = ::SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

            // DPI awareness contexts contain informational flags and can't be bitwise compared.
            if (::AreDpiAwarenessContextsEqual(oldDpiAwarenessContext, DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2))
            {
                LOG_OUTPUT(L"Old DPI awareness was DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2");
            }
            else if (::AreDpiAwarenessContextsEqual(oldDpiAwarenessContext, DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE))
            {
                LOG_OUTPUT(L"Old DPI awareness was DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE");
            }
            else if (::AreDpiAwarenessContextsEqual(oldDpiAwarenessContext, DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED))
            {
                LOG_OUTPUT(L"Old DPI awareness was DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED");
            }
            else if (::AreDpiAwarenessContextsEqual(oldDpiAwarenessContext, DPI_AWARENESS_CONTEXT_SYSTEM_AWARE))
            {
                LOG_OUTPUT(L"Old DPI awareness was DPI_AWARENESS_CONTEXT_SYSTEM_AWARE");
            }
            else if (::AreDpiAwarenessContextsEqual(oldDpiAwarenessContext, DPI_AWARENESS_CONTEXT_UNAWARE))
            {
                LOG_OUTPUT(L"Old DPI awareness was DPI_AWARENESS_CONTEXT_UNAWARE");
            }
        }

        try
        {
            m_spWin32Host = Win32Hosting::StartWin32Host(L"Private.Infrastructure.Hosting.WPF.HostFactory", dpiAwarenessContext, initCore);
        }
        catch (const WEX::Common::Exception&)
        {
            LOG_OUTPUT(L"InitializeHost Failed to wait for the Host.");
            FAIL_FAST();
        }
        try
        {
            dispatcher = Win32Hosting::GetDispatcherQueueFromWin32XamlContentRoot(m_spWin32Host);
        }
        catch (const WEX::Common::Exception&)
        {
            LOG_OUTPUT(L"InitializeHost Failed to wait for the Core Window.");
            FAIL_FAST();
        }
        uint64_t handle = 0;
        FAIL_FAST_IF_FAILED(m_spWin32Host->get_MainWindowHandle(&handle));
        mainWindowHandle = reinterpret_cast<HWND>(handle);
    }
    else if (hostingMode == Hosting::HostingMode::UAP)
    {
        wrl::ComPtr<wac::ICoreImmersiveApplication> application;
        FAIL_FAST_IF_FAILED(wf::GetActivationFactory(
            wrl::Wrappers::HStringReference(RuntimeClass_Windows_ApplicationModel_Core_CoreApplication).Get(),
            &application));
        wrl::ComPtr<wac::ICoreApplicationView> view;
        FAIL_FAST_IF_FAILED(application->get_MainView(&view));

        wrl::ComPtr<wuc::ICoreWindow> coreWindow;
        FAIL_FAST_IF_FAILED(view->get_CoreWindow(&coreWindow));

        wrl::ComPtr<wuc::ICoreWindow5> coreWindow5;
        FAIL_FAST_IF_FAILED(coreWindow.As(&coreWindow5));

        wrl::ComPtr<wsy::IDispatcherQueue> windowsSystemDispatcher;
        FAIL_FAST_IF_FAILED(coreWindow5->get_DispatcherQueue(&windowsSystemDispatcher));

        RunOnDispatcherThread(windowsSystemDispatcher, [&]()
        {
             //get and store dispatcher queue for the local thread
            wrl::ComPtr<msy::IDispatcherQueueStatics> queueStatics;
            FAIL_FAST_IF_FAILED(wf::GetActivationFactory(
                wrl::Wrappers::HStringReference(RuntimeClass_Microsoft_UI_Dispatching_DispatcherQueue).Get(),
                &queueStatics));
            FAIL_FAST_IF_FAILED(queueStatics->GetForCurrentThread(&dispatcher));
        });
    }
    else if (hostingMode == Hosting::HostingMode::WinForms)
    {
        LOG_OUTPUT(L"Hosting mode is WinForms");

        m_spWin32Host = Win32Hosting::StartWin32Host(L"Private.Infrastructure.Hosting.WinForms.HostFactory", test_infra::Hosting::DpiAwarenessContext::DpiAwarenessContext_PerMonitorAwareV2, initCore);
        dispatcher = Win32Hosting::GetDispatcherQueueFromWin32XamlContentRoot(m_spWin32Host);
        uint64_t handle = 0;
        FAIL_FAST_IF_FAILED(m_spWin32Host->get_MainWindowHandle(&handle));
        mainWindowHandle = reinterpret_cast<HWND>(handle);
    }
    HostingDispatcher::Get()->Init(dispatcher, mainWindowHandle);

    DWORD uiThreadId = 0;
    RunOnUIThread([&]() {
        uiThreadId = ::GetCurrentThreadId();
    });

    auto windowHelper = wrl::Make<WindowHelper>(uiThreadId, m_spWin32Host, this);
    FAIL_FAST_IF_FAILED(windowHelper->RuntimeClassInitialize());
    m_spWindowHelper = windowHelper;

    auto keyboardHelper = wrl::Make<KeyboardHelper>(uiThreadId);
    FAIL_FAST_IF_FAILED(keyboardHelper->RuntimeClassInitialize());
    m_spKeyboardHelper = keyboardHelper;

    LOG_OUTPUT(L"InitializeHost has been successfully completed.");
    return S_OK;
}

HRESULT TestServicesStatics::TestServicesStatics::DeInitializeHost()
{
    CloseWin32Host();

    Hosting::HostingMode hostingMode = Hosting::HostingMode::UAP;
    LogThrow_IfFailed(GetHostingMode(&hostingMode));
    HostingDispatcher::Get()->DeInit();

    return S_OK;
}

void TestServicesStatics::CloseWin32Host()
{
    if (m_spWin32Host != nullptr)
    {
        wrl::ComPtr<wf::IClosable> spClosable;
        LogThrow_IfFailed(m_spWin32Host.As(&spClosable));
        LogThrow_IfFailed(spClosable->Close());
        m_spWin32Host = nullptr;
    }
}

HRESULT TestServicesStatics::EnsureInitialized()
{
    COM_START
    {
        s_isInitialized = true;

        // This method is called at the beginning of a class. It's possible the previous class had called ShutdownXaml and we need to make
        // sure that we initialize it if so.
        LogThrow_IfFailed(m_spWindowHelper->InitializeXaml());
    }
    COM_END
}

HRESULT TestServicesStatics::EnsureInitializedForBVT()
{
    Utilities::RunAsBVT();
    return TestServicesStatics::EnsureInitialized();
}

HRESULT TestServicesStatics::get_Utilities(test_infra::IUtilities** ppUtilities)
{
    COM_START
    {
        LogThrow_IfFailed(m_spUtilities.CopyTo(ppUtilities));
    }
    COM_END
}

HRESULT TestServicesStatics::get_FontHelper(test_infra::IFontHelper** ppFontHelper)
{
    COM_START
    {
        LogThrow_IfFailed(m_spFontHelper.CopyTo(ppFontHelper));
    }
    COM_END
}

HRESULT TestServicesStatics::get_InputHelper(test_infra::IInputHelper** ppInputHelper)
{
    COM_START
    {
        LogThrow_IfFailed(m_spInputHelper.CopyTo(ppInputHelper));
    }
    COM_END
}

HRESULT TestServicesStatics::get_WindowHelper(test_infra::IWindowHelper** ppWindowHelper)
{
    COM_START
    {
        LogThrow_IfFailed(m_spWindowHelper.CopyTo(ppWindowHelper));
    }
    COM_END
}

HRESULT TestServicesStatics::get_KeyboardHelper(test_infra::IKeyboardHelper** ppKeyboardHelper)
{
    COM_START
    {
        LogThrow_IfFailed(m_spKeyboardHelper.CopyTo(ppKeyboardHelper));
    }
    COM_END
}

HRESULT TestServicesStatics::get_ErrorHandlingHelper(test_infra::IErrorHandlingHelper** ppErrorHandlingHelper)
{
    COM_START
    {
        LogThrow_IfFailed(m_spErrorHandlingHelper.CopyTo(ppErrorHandlingHelper));
    }
    COM_END
}

HRESULT TestServicesStatics::get_ThemingHelper(test_infra::IThemingHelper** ppThemingHelper)
{
    COM_START
    {
        LogThrow_IfFailed(m_spThemingHelper.CopyTo(ppThemingHelper));
    }
    COM_END
}

HRESULT TestServicesStatics::get_Win32Host(test_infra::Hosting::IWin32Host** ppHost)
{
    COM_START
    {
        LogThrow_IfFailed(m_spWin32Host.CopyTo(ppHost));
    }
    COM_END
}

void TestServicesStatics::ClearPrimaryLanguageOverride()
{
    // Some tests set the ApplicationLanguages.PrimaryLanguageOverride property to enable localization testing.
    // This property is persisted across app sessions, so we need to make sure that we clear the property on start-up in case
    // a test crashed during execution without giving it a chance to run cleanup.
    wrl::Wrappers::HString strPrimaryLanguageOverride;
    wrl::ComPtr<wg::IApplicationLanguagesStatics> spApplicationLanguagesStatics;
    LogThrow_IfFailed(wf::GetActivationFactory(wrl::Wrappers::HStringReference(RuntimeClass_Windows_Globalization_ApplicationLanguages).Get(), &spApplicationLanguagesStatics));
    LogThrow_IfFailed(spApplicationLanguagesStatics->get_PrimaryLanguageOverride(strPrimaryLanguageOverride.ReleaseAndGetAddressOf()));

    if (!(::WindowsIsStringEmpty(strPrimaryLanguageOverride.Get())))
    {
        WEX::Logging::Log::Warning(L"Clearing ApplicationLanguages.PrimaryLanguageOverride.");
        LogThrow_IfFailed(spApplicationLanguagesStatics->put_PrimaryLanguageOverride(nullptr));
    }
}

void TestServicesStatics::SetCustomSystemFontCollection()
{
    // Set a static font collection that will ensure font change will not impact the rendering result.
    RunOnUIThread([&]()
    {
        wrl::ComPtr<IInspectable> customSystemFontCollection;
        wrl::ComPtr<IFontHelperNative> fontHelperNative;
        LogThrow_IfFailed(m_spFontHelper.As(&fontHelperNative));
        LogThrow_IfFailed(fontHelperNative->GetCustomSystemFontCollection(&customSystemFontCollection));
        LogThrow_IfFailed(fontHelperNative->SetSystemFontCollectionOverride(customSystemFontCollection.Get()));
    });
}

bool TestServicesStatics::IsInitialized()
{
    return TestServicesStatics::s_isInitialized;
}

HRESULT TestServicesStatics::OnAppActivated(xaml::IWindowActivatedEventArgs* eventArgs)
{
    COM_START
    {
        WEX::Logging::Log::Comment(L"TestServicesStatics::OnAppActivated()");
        xaml::WindowActivationState state = {};
        LogThrow_IfFailed(eventArgs->get_WindowActivationState(&state));
        LOG_OUTPUT(L"Activation state %d", state);
    }
    COM_END
}

wrl::ComPtr<IXamlTestHooks> TestServicesStatics::GetTestHooks()
{
    wrl::ComPtr<xaml::IDxamlCoreTestHooksStatics> spTestHooksStatics;
    wrl::ComPtr<xaml::IDxamlCoreTestHooks> spTestHooks;
    wrl::ComPtr<IXamlTestHooks> spXamlTestHooks;

    LogThrow_IfFailed(wf::GetActivationFactory(
        wrl::Wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_DxamlCoreTestHooks).Get(), &spTestHooksStatics));

    RunOnUIThread([&]()
    {
        LogThrow_IfFailed(spTestHooksStatics->GetForCurrentThread(&spTestHooks));
    });

    LogThrow_IfFailed(spTestHooks.As(&spXamlTestHooks));
    return std::move(spXamlTestHooks);
}

HANDLE OpenNamedEvent(DWORD processId, DWORD threadId, const wchar_t* eventNamePrefix)
{
    wchar_t eventName[_MAX_PATH];
    _snwprintf(eventName, _MAX_PATH, L"%s.%d.%d", eventNamePrefix, processId, threadId);
#pragma warning(suppress: 6053) // PREFast does not know eventName is normally null-terminated by _snwprintf(). This is test code, not investing into a more elaborate fix.
    return ::OpenEvent(EVENT_MODIFY_STATE | SYNCHRONIZE, false /* inherit handle */, eventName);
}

} }
