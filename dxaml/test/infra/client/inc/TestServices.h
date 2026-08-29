// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class PredictableDManipEnabler;
class SmartStackLogger;
interface IXamlTestHooks;

namespace Private { namespace Infrastructure {

    class WindowHelper;

    HANDLE OpenNamedEvent(DWORD processId, DWORD threadId, const wchar_t* eventName);

    inline HANDLE OpenNamedEvent(DWORD threadId, const wchar_t* eventName)
    {
        return OpenNamedEvent(::GetCurrentProcessId(), threadId, eventName);
    }

    class TestServicesStatics
        : public wrl::AgileActivationFactory<test_infra::ITestServicesStatics>
    {
        InspectableClassStatic(RuntimeClass_Private_Infrastructure_TestServices, TrustLevel::BaseTrust);

    public:
        IFACEMETHOD(RuntimeClassInitialize)();

        // Nondefault ctor/dtor to support unique_ptr type forward declaration.
        TestServicesStatics();
        ~TestServicesStatics();

        // When TestServices is first accessed RuntimeClassInitialize
        // is called. We do our initialization there. Because it's not
        // easy to explicitly force a factory to be instantiated we
        // provide this method. The idea is that if you call this method
        // the class will be instantiated and RuntimeClassInitialize will
        // be called. We'll leave that as a detail of the implementation and
        // suggest that part of this class's public contract is that EnsureInitialized
        // MUST be called.
        // With MDA we also use this method as a convienient place to bring the test window
        // back to the foreground if it ever happens to fall behind.
        IFACEMETHOD(EnsureInitialized)();

        // Since InputHelper and WindowHelper are essentially singletons in this
        // design they are only available as static properties on TestServices.
        IFACEMETHOD(get_Utilities)(test_infra::IUtilities** ppUtilities);
        IFACEMETHOD(get_FontHelper)(test_infra::IFontHelper** ppFontHelper);
        IFACEMETHOD(get_InputHelper)(test_infra::IInputHelper** ppInputHelper);
        IFACEMETHOD(get_WindowHelper)(test_infra::IWindowHelper** ppWindowHelper);
        IFACEMETHOD(get_KeyboardHelper)(test_infra::IKeyboardHelper** ppKeyboardHelper);
        IFACEMETHOD(get_ErrorHandlingHelper)(test_infra::IErrorHandlingHelper** ppErrorHandlingHelper);
        IFACEMETHOD(get_ThemingHelper)(test_infra::IThemingHelper** ppThemingHelper);
        IFACEMETHOD(get_Win32Host)(test_infra::Hosting::IWin32Host** ppHost);

        IFACEMETHOD(EnsureInitializedForBVT)();

        IFACEMETHOD(InitializeHost)() override;

        IFACEMETHOD(InitializeHostAndDpiAwarenessContext)(boolean initializeDpiAwarenessContext) override;

        IFACEMETHOD(InitializeHostAndDpiAwarenessContextAndCore)(boolean initializeDpiAwarenessContext, boolean initCore) override;

        IFACEMETHOD(DeInitializeHost)() override;

        static bool IsInitialized();

        static wrl::ComPtr<IXamlTestHooks> GetTestHooks();

    private:
        void ClearPrimaryLanguageOverride();

        void SetCustomSystemFontCollection();

        static HRESULT OnAppActivated(xaml::IWindowActivatedEventArgs* eventArgs);

        void CloseWin32Host();

        wrl::ComPtr<test_infra::IUtilities> m_spUtilities;
        wrl::ComPtr<test_infra::IFontHelper> m_spFontHelper;
        wrl::ComPtr<test_infra::IInputHelper> m_spInputHelper;
        wrl::ComPtr<WindowHelper> m_spWindowHelper;
        wrl::ComPtr<test_infra::IKeyboardHelper> m_spKeyboardHelper;
        wrl::ComPtr<test_infra::IErrorHandlingHelper> m_spErrorHandlingHelper;
        wrl::ComPtr<test_infra::IThemingHelper> m_spThemingHelper;

        std::unique_ptr<PredictableDManipEnabler> m_spPredictableDManipEnabler;

        wrl::ComPtr<xaml::IWindow> m_spWindow;
        EventRegistrationToken m_activatedToken = {};

        wrl::ComPtr<test_infra::Hosting::IWin32Host> m_spWin32Host;

        static bool s_isInitialized;
    };

} }
