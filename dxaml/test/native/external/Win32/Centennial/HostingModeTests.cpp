// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "HostingModeTests.h"

#include <HostingModeOverride.h>
#include <MasterFileNameOverride.h>
#include <Microsoft.UI.Content.h>
#include <Microsoft.UI.Dispatching.h>
#include <Microsoft.UI.Interop.h>
#include <Microsoft.UI.Xaml.h>
#include <TestCleanupWrapper.h>
#include <XamlTailored.h>
#include <cwchar>

using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Hosting;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft::UI::Xaml::Tests::HostingMode {

namespace {

constexpr wchar_t c_explicitWindowClass[] = L"HostingModeWin32ExplicitTests_Window";
constexpr wchar_t c_wpfWindowClassPrefix[] = L"HwndWrapper[";

void VerifyNoHostingModeParameter()
{
    WEX::Common::String value;
    VERIFY_IS_TRUE(FAILED(WEX::TestExecution::RuntimeParameters::TryGetValue(L"HostingMode", value)),
        L"Run the hosting smoke tests without /p:HostingMode.");
}

void VerifyDeclaredHostingModeContract()
{
    namespace hosting = ::Private::Infrastructure::Hosting;

    std::wstring originalMode;
    const HRESULT originalResult = hosting::GetDeclaredHostingMode(originalMode);
    VERIFY_IS_TRUE(originalResult == S_OK || originalResult == S_FALSE);
    auto restoreMode = wil::scope_exit([&]()
    {
        VERIFY_SUCCEEDED(hosting::SetDeclaredHostingMode(
            originalResult == S_FALSE ? nullptr : originalMode.c_str()));
    });

    std::wstring mode = L"stale";
    VERIFY_SUCCEEDED(hosting::SetDeclaredHostingMode(nullptr));
    VERIFY_ARE_EQUAL(S_FALSE, hosting::GetDeclaredHostingMode(mode));
    VERIFY_IS_TRUE(mode.empty());

    for (const auto expected : {L"WPF", L"UAP", L"Win32Explicit", L"WinForms", L"wpf"})
    {
        VERIFY_SUCCEEDED(hosting::SetDeclaredHostingMode(expected));
        VERIFY_ARE_EQUAL(S_OK, hosting::GetDeclaredHostingMode(mode));
        VERIFY_IS_TRUE(mode == expected);
    }

    VERIFY_ARE_EQUAL(E_INVALIDARG, hosting::SetDeclaredHostingMode(L"unknown"));
    VERIFY_ARE_EQUAL(E_INVALIDARG, hosting::SetDeclaredHostingMode(L""));
    VERIFY_ARE_EQUAL(S_OK, hosting::GetDeclaredHostingMode(mode));
    VERIFY_IS_TRUE(mode == L"wpf");

    VERIFY_IS_TRUE(::SetEnvironmentVariableW(hosting::c_declaredHostingModeEnvVar, L"unknown"));
    VERIFY_ARE_EQUAL(E_INVALIDARG, hosting::GetDeclaredHostingMode(mode));
    VERIFY_IS_TRUE(mode.empty());

    const std::wstring tooLong(64, L'x');
    VERIFY_IS_TRUE(::SetEnvironmentVariableW(hosting::c_declaredHostingModeEnvVar, tooLong.c_str()));
    VERIFY_ARE_EQUAL(HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER), hosting::GetDeclaredHostingMode(mode));
    VERIFY_IS_TRUE(mode.empty());

    VERIFY_SUCCEEDED(hosting::SetDeclaredHostingMode(nullptr));
    VERIFY_ARE_EQUAL(S_FALSE, hosting::GetDeclaredHostingMode(mode));
    VERIFY_IS_TRUE(mode.empty());
}

void VerifyTestServicesHost(bool expectWpf)
{
    TestCleanupWrapper cleanup;
    TextBlock^ content;

    auto host = TestServices::Win32Host;
    if (expectWpf)
    {
        VERIFY_IS_NOT_NULL(host);
        const HWND window = reinterpret_cast<HWND>(host->MainWindowHandle);
        VERIFY_IS_TRUE(::IsWindow(window));

        wchar_t windowClass[256] = {};
        VERIFY_IS_TRUE(::GetClassNameW(window, windowClass, ARRAYSIZE(windowClass)) > 0);
        VERIFY_ARE_EQUAL(0, std::wcsncmp(windowClass, c_wpfWindowClassPrefix, ARRAYSIZE(c_wpfWindowClassPrefix) - 1));
    }
    else
    {
        VERIFY_IS_NULL(host);
    }

    RunOnUIThread([&]()
    {
        auto coreWindow = ::Windows::UI::Core::CoreWindow::GetForCurrentThread();
        if (expectWpf)
        {
            VERIFY_IS_NULL(coreWindow);
        }
        else
        {
            VERIFY_IS_NOT_NULL(coreWindow);
            VERIFY_IS_TRUE(coreWindow->Dispatcher->HasThreadAccess);
        }

        content = ref new TextBlock();
        content->Text = L"Hosting mode smoke test";
        TestServices::WindowHelper->WindowContent = content;
    });

    TestServices::WindowHelper->WaitForIdle();
    RunOnUIThread([&]()
    {
        VERIFY_IS_TRUE(TestServices::WindowHelper->WindowContent == content);
        VERIFY_IS_NOT_NULL(content->XamlRoot);
        VERIFY_IS_TRUE(content->ActualWidth > 0);
        VERIFY_IS_TRUE(content->ActualHeight > 0);
    });
}

void VerifyMasterFileNameContract()
{
    namespace masters = ::Private::Infrastructure::MasterFiles;
    std::wstring originalClass;
    const HRESULT originalResult = ::Private::Infrastructure::TestClassSettings::GetValue<256>(
        masters::c_classNameEnvVar, originalClass);
    VERIFY_IS_TRUE(originalResult == S_OK || originalResult == S_FALSE);
    auto restoreClass = wil::scope_exit([&]()
    {
        VERIFY_SUCCEEDED(masters::SetClassName(originalResult == S_FALSE ? nullptr : originalClass.c_str()));
    });

    const std::wstring actualName = L"Microsoft::UI::Xaml::Tests::OriginalTestsUap::VerifyImage";
    std::wstring name = actualName;
    VERIFY_SUCCEEDED(masters::SetClassName(nullptr));
    VERIFY_SUCCEEDED(masters::ApplyClassName(name));
    VERIFY_IS_TRUE(name == actualName);

    VERIFY_SUCCEEDED(masters::SetClassName(L"OriginalTests"));
    VERIFY_SUCCEEDED(masters::ApplyClassName(name));
    VERIFY_IS_TRUE(name == L"Microsoft::UI::Xaml::Tests::OriginalTests::VerifyImage");
    name = L"OriginalTestsWpf::VerifyImage";
    VERIFY_SUCCEEDED(masters::ApplyClassName(name));
    VERIFY_IS_TRUE(name == L"OriginalTests::VerifyImage");

    VERIFY_ARE_EQUAL(E_INVALIDARG, masters::SetClassName(L""));
    VERIFY_ARE_EQUAL(E_INVALIDARG, masters::SetClassName(L"..\\OtherTests"));
    name = L"NotANativeTestName";
    VERIFY_ARE_EQUAL(E_INVALIDARG, masters::ApplyClassName(name));

    VERIFY_SUCCEEDED(masters::SetClassName(nullptr));
    name = actualName;
    VERIFY_SUCCEEDED(masters::ApplyClassName(name));
    VERIFY_IS_TRUE(name == actualName);
}

}

bool HostingModeDefaultTests::ClassSetup()
{
    VerifyNoHostingModeParameter();
    // Do not mutate the process-wide hosting mode while a TestServices host is running.
    VerifyDeclaredHostingModeContract();
    VerifyMasterFileNameContract();
    VERIFY_SUCCEEDED(::Private::Infrastructure::MasterFiles::SetClassName(L"PreviousTests"));
    XAML_HOSTING_MODE_CLASS_SETUP();
    std::wstring name = L"HostingModeDefaultTests::CreatesDeclaredHostWithoutParameter";
    VERIFY_SUCCEEDED(::Private::Infrastructure::MasterFiles::ApplyClassName(name));
    VERIFY_IS_TRUE(name == L"HostingModeDefaultTests::CreatesDeclaredHostWithoutParameter");
    return true;
}

bool HostingModeDefaultTests::TestSetup()
{
    TestServices::WindowHelper->InitializeXaml();
    return true;
}

bool HostingModeDefaultTests::TestCleanup()
{
    TestServices::WindowHelper->ShutdownXaml();
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

void HostingModeDefaultTests::CreatesDeclaredHostWithoutParameter()
{
    VerifyTestServicesHost(true);
}

bool HostingModeUapTests::ClassSetup()
{
    VerifyNoHostingModeParameter();
    XAML_HOSTING_MODE_CLASS_SETUP();
    return true;
}

bool HostingModeUapTests::TestSetup()
{
    TestServices::WindowHelper->InitializeXaml();
    return true;
}

bool HostingModeUapTests::TestCleanup()
{
    TestServices::WindowHelper->ShutdownXaml();
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

void HostingModeUapTests::CreatesDeclaredHostWithoutParameter()
{
    VerifyTestServicesHost(false);
}

bool HostingModeWin32ExplicitTests::TestSetup()
{
    VerifyNoHostingModeParameter();

    WNDCLASSEXW windowClass = {};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.lpfnWndProc = DefWindowProcW;
    windowClass.lpszClassName = c_explicitWindowClass;
    m_windowClassAtom = ::RegisterClassExW(&windowClass);
    VERIFY_ARE_NOT_EQUAL(static_cast<ATOM>(0), m_windowClassAtom);
    return true;
}

bool HostingModeWin32ExplicitTests::TestCleanup()
{
    VERIFY_IS_FALSE(::IsWindow(m_window));
    m_window = nullptr;

    if (m_windowClassAtom != 0)
    {
        VERIFY_IS_TRUE(::UnregisterClassW(MAKEINTATOM(m_windowClassAtom), nullptr));
        m_windowClassAtom = 0;
    }
    return true;
}

void HostingModeWin32ExplicitTests::CreatesDeclaredHostWithoutParameter()
{
    VERIFY_SUCCEEDED(::RoInitialize(RO_INIT_MULTITHREADED));
    auto uninitialize = wil::scope_exit([]() { ::RoUninitialize(); });
    auto islandHelper = IslandHelper::CreateOnNewUIThreadAndNewWindow(m_windowClassAtom);
    VERIFY_IS_NOT_NULL(islandHelper);

    auto dispatcher = islandHelper->DispatcherQueue;
    VERIFY_IS_NOT_NULL(dispatcher);
    RunOnDispatcherThread(dispatcher, true, [&]()
    {
        VERIFY_IS_TRUE(dispatcher->HasThreadAccess);
        VERIFY_IS_NULL(::Windows::UI::Core::CoreWindow::GetForCurrentThread());
        VERIFY_IS_NOT_NULL(WindowsXamlManager::GetForCurrentThread());

        auto source = safe_cast<DesktopWindowXamlSource^>(islandHelper->DesktopWindowXamlSource);
        VERIFY_IS_NOT_NULL(source);
        auto content = ref new TextBlock();
        content->Text = L"Self-hosted hosting mode smoke test";
        source->Content = content;
        content->UpdateLayout();
        VERIFY_IS_NOT_NULL(content->XamlRoot);
        VERIFY_IS_TRUE(source->Content == content);

        VERIFY_SUCCEEDED(ABI::Microsoft::UI::GetWindowFromWindowId(
            ABI::Microsoft::UI::WindowId {content->XamlRoot->ContentIslandEnvironment->AppWindowId.Value},
            &m_window));
        VERIFY_IS_TRUE(::IsWindow(m_window));

        wchar_t windowClass[256] = {};
        VERIFY_IS_TRUE(::GetClassNameW(m_window, windowClass, ARRAYSIZE(windowClass)) > 0);
        VERIFY_ARE_EQUAL(0, std::wcscmp(c_explicitWindowClass, windowClass));

        HWND islandWindow = nullptr;
        VERIFY_SUCCEEDED(ABI::Microsoft::UI::GetWindowFromWindowId(
            ABI::Microsoft::UI::WindowId {source->SiteBridge->WindowId.Value},
            &islandWindow));
        VERIFY_IS_TRUE(::IsWindow(islandWindow));
        VERIFY_IS_TRUE(::IsChild(m_window, islandWindow));
    });
}

}
