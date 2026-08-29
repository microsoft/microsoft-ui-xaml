// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ThemingTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>

#include <roapi.h>
#include <detours.h>

#include <wrl/client.h>

#include <windows.ui.viewmanagement.h>

using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Input;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Tests::Common;

using namespace test_infra;
using namespace MockDComp;
using namespace ::Windows::Storage::Streams;

namespace wrl = ::Microsoft::WRL;
namespace awf = ABI::Windows::Foundation;
namespace awuv = ABI::Windows::UI::ViewManagement;

decltype(RoGetActivationFactory)* TrueRoGetActivationFactory = RoGetActivationFactory;
decltype(RoActivateInstance)* TrueRoActivateInstance = RoActivateInstance;
decltype(RegGetValueW)* TrueRegGetValueW = RegGetValueW;

DWORD g_currentTheme = 0;

// Detour of RegGetValueW.  Xaml calls this to detect light/dark theme.
LSTATUS RegGetValueWDetour(
    HKEY    hkey,
    LPCWSTR lpSubKey,
    LPCWSTR lpValue,
    DWORD   dwFlags,
    LPDWORD pdwType,
    PVOID   pvData,
    LPDWORD pcbData)
{
    if (wcscmp(L"AppsUseLightTheme", lpValue) == 0)
    {
        LOG_OUTPUT(L"[Detour] RegGetValueW of %s", lpValue);
        *(static_cast<DWORD*>(pvData)) = g_currentTheme;

        return ERROR_SUCCESS;
    }
    return TrueRegGetValueW(hkey, lpSubKey, lpValue, dwFlags, pdwType, pvData, pcbData);
}

// MockUISettings is returned to Xaml when it creates a Windows.UI.ViewManagement.UISettings object.
class MockUISettings
    : public wrl::RuntimeClass<awuv::IUISettings, awuv::IUISettings2, awuv::IUISettings3, wrl::FtmBase>
{
    InspectableClass(nullptr /*internal*/, BaseTrust);

public:
    MockUISettings()
    {
        VERIFY_SUCCEEDED(::TrueRoActivateInstance(
            wrl_wrappers::HStringReference(RuntimeClass_Windows_UI_ViewManagement_UISettings).Get(),
            &m_realUISettings));
    }

    // IUISettings implementation
    IFACEMETHOD(get_HandPreference)(__out awuv::HandPreference *preference) override{ return E_NOTIMPL; }
    IFACEMETHOD(get_CursorSize)(__out awf::Size *size) override { return E_NOTIMPL; }
    IFACEMETHOD(get_ScrollBarSize)(__out awf::Size *size) override { return E_NOTIMPL; }
    IFACEMETHOD(get_ScrollBarArrowSize)(__out awf::Size *size) override { return E_NOTIMPL; }
    IFACEMETHOD(get_ScrollBarThumbBoxSize)(__out awf::Size *size) override { return E_NOTIMPL; }
    IFACEMETHOD(get_MessageDuration)(__out UINT32 *value) override { return E_NOTIMPL; }
    IFACEMETHOD(get_AnimationsEnabled)(__out boolean *value) override
        { return m_realUISettings->get_AnimationsEnabled(value); }
    IFACEMETHOD(get_CaretBrowsingEnabled)(__out boolean *value) override { return E_NOTIMPL; }
    IFACEMETHOD(get_CaretBlinkRate)(__out UINT32 *value) override { return E_NOTIMPL; }
    IFACEMETHOD(get_CaretWidth)(__out UINT32 *value) override { return E_NOTIMPL; }
    IFACEMETHOD(get_DoubleClickTime)(__out UINT32 *value) override { return E_NOTIMPL; }
    IFACEMETHOD(get_MouseHoverTime)(__out UINT32 *value) override { return E_NOTIMPL; }
    IFACEMETHOD(UIElementColor)(__in awuv::UIElementType desiredElement, __out ABI::Windows::UI::Color *value) override { return E_NOTIMPL; }

    // IUISettings2 implementation
    IFACEMETHOD(get_TextScaleFactor)(__out DOUBLE *textScaleFactor) override { return E_NOTIMPL; }

    IFACEMETHOD(add_TextScaleFactorChanged)(
        __in awf::ITypedEventHandler<awuv::UISettings*, IInspectable*> *handler,
        __out EventRegistrationToken *pCookie) override { return E_NOTIMPL; }

    IFACEMETHOD(remove_TextScaleFactorChanged)(
        __in EventRegistrationToken iCookie) override { return E_NOTIMPL; }

    // IUISettings3 implementation
    IFACEMETHOD(GetColorValue)(__in awuv::UIColorType desirecColor, __out ABI::Windows::UI::Color *value) override { return E_NOTIMPL; }

    IFACEMETHOD(add_ColorValuesChanged)(
        __in awf::ITypedEventHandler<awuv::UISettings*, IInspectable*> *handler,
        __out EventRegistrationToken *pCookie) override
    {
        LOG_OUTPUT(L"Caller subscribing to ColorValuesChanged");

        // We only track one handler per instance.
        VERIFY_IS_NULL(m_colorValuesChangedHandler);
        m_colorValuesChangedHandler = handler;
        *pCookie = { Cookie };
        return S_OK;
    }

    IFACEMETHOD(remove_ColorValuesChanged)(
        __in EventRegistrationToken iCookie) override
    {
        LOG_OUTPUT(L"Caller unsubscribing from ColorValuesChanged");
        VERIFY_ARE_EQUAL(Cookie, iCookie.value);
        return S_OK;
    }

    wrl::ComPtr<IUISettings> m_realUISettings;
    static constexpr __int64 Cookie = 0x12345678deadbeef;
    wrl::ComPtr<awf::ITypedEventHandler<awuv::UISettings*, IInspectable*>> m_colorValuesChangedHandler;
};

// Vector of MockUISettings objects we've given out.
std::vector<wrl::ComPtr<MockUISettings>> g_mockUISettingsVector;

// Detour of RoActivateInstance.  Return our mock of UISettings, otherwise pass through.
HRESULT WINAPI RoActivateInstanceDetour(HSTRING activatableClassId, IInspectable** instance)
{
    UINT32 length{};
    PCWSTR buffer = WindowsGetStringRawBuffer(activatableClassId, &length);

    if (wcscmp(L"Windows.UI.ViewManagement.UISettings", buffer) == 0)
    {
        LOG_OUTPUT(L"[Detour] RoActivateInstance of %s.  Returning MockUISettings.", buffer);
        wrl::ComPtr<MockUISettings> newMockUISettings = wrl::Make<MockUISettings>();
        g_mockUISettingsVector.push_back(newMockUISettings);
        newMockUISettings.CopyTo(instance);
        return S_OK;
    }
    
    return TrueRoActivateInstance(activatableClassId, instance);
}

// Not used for anything, just left in case we want to copy/paste later.
HRESULT WINAPI RoGetActivationFactoryDetour(HSTRING activatableClassId, REFIID iid, void** factory)
{   
    return TrueRoGetActivationFactory(activatableClassId, iid, factory);
}

void AttachDetours()
{
    LOG_OUTPUT(L"[Detour] Attaching detours.");
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(&(PVOID&)TrueRoGetActivationFactory, RoGetActivationFactoryDetour);
    DetourAttach(&(PVOID&)TrueRoActivateInstance, RoActivateInstanceDetour);
    DetourAttach(&(PVOID&)TrueRegGetValueW, RegGetValueWDetour);
    DetourTransactionCommit();
}

void DetachDetours()
{
    LOG_OUTPUT(L"[Detour] Detaching detours.");
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourDetach(&(PVOID&)TrueRoGetActivationFactory, RoGetActivationFactoryDetour);
    DetourDetach(&(PVOID&)TrueRoActivateInstance, RoActivateInstanceDetour);
    DetourDetach(&(PVOID&)TrueRegGetValueW, RegGetValueWDetour);
    DetourTransactionCommit();
}

// Simulate a theme change via our detours/mocks.
void TriggerThemeChange(DWORD newTheme)
{
    g_currentTheme = newTheme;

    LOG_OUTPUT(L"Calling MockUISettings , %d mocks", g_mockUISettingsVector.size());
    for (auto mockUISettings : g_mockUISettingsVector)
    {
        if (auto handler = mockUISettings->m_colorValuesChangedHandler)
        {
            VERIFY_SUCCEEDED(handler->Invoke(nullptr, nullptr));
        }
    }
}

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Shell {

        bool ThemingTests::ClassSetup()
        {
            AttachDetours();
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool ThemingTests::ClassCleanup()
        {
            DetachDetours();
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool ThemingTests::TestSetup()
        {            
            test_infra::TestServices::WindowHelper->InitializeXaml();
            return true;
        }

        bool ThemingTests::TestCleanup()
        {
            test_infra::TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        void ThemingTests::LightDarkThemeChangeDetected()
        {
            TestCleanupWrapper cleanup;
            StackPanel^ rootPanel;

            RunOnUIThread([&]()
            {
                test_infra::TestServices::ThemingHelper->RemoveThemingOverrides();

                rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                    L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                    L"  <Button x:Name='btn1' Content='Button 1'/>"
                    L"  <Button x:Name='btn2' Content='Button 2'/>"
                    L"  <Button x:Name='btn3' Content='Button 3'/>"
                    L"</StackPanel>"));

                TestServices::WindowHelper->WindowContent = rootPanel;
            });
            TestServices::WindowHelper->WaitForIdle();

            TriggerThemeChange(1); // purposefully off the ui thread
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(ApplicationTheme::Light, Application::Current->RequestedTheme);
            });
            TestServices::WindowHelper->WaitForIdle();

            TriggerThemeChange(0); // purposefully off the ui thread
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(ApplicationTheme::Dark, Application::Current->RequestedTheme);
            });
            TestServices::WindowHelper->WaitForIdle();
        }

    } }
} } } }
