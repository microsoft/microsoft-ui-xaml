// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Copyright (c) Microsoft Corporation.  All rights reserved.

#pragma once

#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include "FocusTestHelper.h"

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

#define VERIFY_ARE_EQUAL_ON_UITHREAD(expected, actual)  \
    RunOnUIThread([&](){                                \
        VERIFY_ARE_EQUAL((expected), (actual));         \
    });

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace AccessKeys {

    class EnableAccessKeyMode
    {
    public:
        EnableAccessKeyMode()
            : m_exitAccessKeyMode(true), m_enterAccessKeyMode(true)
        {
            InitializeAccessMode();
        }

        EnableAccessKeyMode(bool exitAccessKeyMode)
            : m_enterAccessKeyMode(true)
        {
            m_exitAccessKeyMode = exitAccessKeyMode;
            InitializeAccessMode();
        }

        EnableAccessKeyMode(bool exitAccessKeyMode, bool ignoreDisplayModeCheck)
            : m_enterAccessKeyMode(true)
        {
            m_exitAccessKeyMode = exitAccessKeyMode;
            InitializeAccessMode(ignoreDisplayModeCheck);
        }

        EnableAccessKeyMode(bool exitAccessKeyMode, bool ignoreDisplayModeCheck, bool enterAccessKeyMode)
        {
            m_exitAccessKeyMode = exitAccessKeyMode;
            m_enterAccessKeyMode = enterAccessKeyMode;
            InitializeAccessMode(ignoreDisplayModeCheck);
        }

        void InitializeAccessMode(bool ignoreDisplayModeCheck = false)
        {
            Platform::Object^ aknavigationObject;
            VERIFY_SUCCEEDED(wf::GetActivationFactory(wrl_wrappers::HStringReference(L"Microsoft.UI.Xaml.Input.AccessKeyManager").Get(),
                reinterpret_cast<IInspectable**>(&aknavigationObject)));
            m_nav = safe_cast<Microsoft::UI::Xaml::Input::IAccessKeyManagerStatics^>(aknavigationObject);

            RunOnUIThread([&]()
            {
                // Previous test could left us in bad state

                if (m_nav->IsDisplayModeEnabled)
                {
                    LOG_OUTPUT(L"The test has started in AK mode. Disable AK mode");
                    m_nav->ExitDisplayMode();
                }

                VERIFY_IS_FALSE(m_nav->IsDisplayModeEnabled);
                m_isActiveChangedToken = m_nav->IsDisplayModeEnabledChanged += ref new wf::TypedEventHandler<Platform::Object^, Platform::Object^>(
                    [&](Platform::Object^, Platform::Object^)
                {
                    LOG_OUTPUT(L"IsActiveChanged fired. Active:%ws", m_nav->IsDisplayModeEnabled?L"True":L"False");
                    m_isActiveChangedFired.Set();
                });
            });

            if (m_enterAccessKeyMode)
            {
                TestServices::KeyboardHelper->Alt();
                m_isActiveChangedFired.WaitForDefault();
            }
            
            // Cannot exit display mode with this enabled. The Alt key fix requires this to be disabled for WPF mode
            // TODO: find a better solution
            WEX::Common::String value;
            bool runningInWPFMode = false;
            if (SUCCEEDED(WEX::TestExecution::RuntimeParameters::TryGetValue(L"HostingMode", value)))
                {
                    if (value == L"WPF")
                    {
                        runningInWPFMode = true;
                    }
            }

            if (!runningInWPFMode && !ignoreDisplayModeCheck && m_enterAccessKeyMode)
            {
                RunOnUIThread([&]()
                {
                    VERIFY_IS_TRUE(m_nav->IsDisplayModeEnabled);
                });

                TestServices::WindowHelper->WaitForIdle();
            }
        }

        void ExitAccessMode()
        {
            TestServices::KeyboardHelper->Alt();
            m_isActiveChangedFired.WaitForDefault();

            RunOnUIThread([&]()
            {
                VERIFY_IS_FALSE(m_nav->IsDisplayModeEnabled);
            });

            TestServices::WindowHelper->WaitForIdle();
        }

        void ExitAccessModeWithAKManager()
        {
            RunOnUIThread([&]()
            {
                m_nav->ExitDisplayMode();
            });
            m_isActiveChangedFired.WaitForDefault();

            RunOnUIThread([&]()
            {
                VERIFY_IS_FALSE(m_nav->IsDisplayModeEnabled);
            });

            TestServices::WindowHelper->WaitForIdle();
        }

        void ExitAccessModeUsingEscKey()
        {
            TestServices::KeyboardHelper->Escape();
            m_isActiveChangedFired.WaitForDefault();

            RunOnUIThread([&]()
            {
                VERIFY_IS_FALSE(m_nav->IsDisplayModeEnabled);
            });

            TestServices::WindowHelper->WaitForIdle();
        }

        void VerifyAKModeHasExited()
        {
            RunOnUIThread([&]()
            {
                VERIFY_IS_FALSE(m_nav->IsDisplayModeEnabled);
            });
        }

        void VerifyAKModeHasNotExited()
        {
            // It is the failure reason for cannot exit displaymode. The Alt key fix requirs this to be disabled for WPF mode now
            // until a better solution is found
            WEX::Common::String value;
            bool runningInWPFMode = false;
            if (SUCCEEDED(WEX::TestExecution::RuntimeParameters::TryGetValue(L"HostingMode", value)))
                {
                    if (value == L"WPF")
                    {
                        runningInWPFMode = true;
                    }
            }
            if (!runningInWPFMode)
            {
                RunOnUIThread([&]()
                {
                    VERIFY_IS_TRUE(m_nav->IsDisplayModeEnabled);
                });
            }
        }

        ~EnableAccessKeyMode()
        {
            if (m_exitAccessKeyMode)
            {
                ExitAccessMode();
            }

            RunOnUIThread([&]()
            {
                m_nav->IsDisplayModeEnabledChanged -= m_isActiveChangedToken;
            });
            TestServices::WindowHelper->WaitForIdle();
        }

    private:

        bool m_exitAccessKeyMode;
        bool m_enterAccessKeyMode;
        wf::EventRegistrationToken m_isActiveChangedToken = {};
        Event m_isActiveChangedFired;
        Microsoft::UI::Xaml::Input::IAccessKeyManagerStatics^ m_nav;
    };

    class AccessKeyTestHelper
    {
    public:

        enum class SubTestEnum
        {
            SubTest_TestAll,
            SubTest_AccessKeyPressed,
            SubTest_MultipleAccessKeysPressed,
            SubTest_LowerCaseAccessKeyPressed,
            SubTest_UpperCaseAccessKeyPressed,
            SubTest_DuplicateAccessKeyPressed,
            SubTest_TwoCharactersAccessKey,
            SubTest_ThreeCharactersAccessKey,
            SubTest_UnicodeCharactersAccessKey,
            SubTest_NullAccessKey,
            SubTest_End
        };

        static void InjectAccessKey (
            Platform::String ^ strToType,
            xaml::UIElement^ element,
            bool expectingAccessKeyInvoked = true,
            bool handleAccessKeyInvokedEvent = false);

        static void InjectAccessKey (
            Platform::String ^ strToType,
            xaml_docs::TextElement^ element,
            bool expectingAccessKeyInvoked = true,
            bool handleAccessKeyInvokedEvent = false);

        template <typename TClassUnderTest>
        static TClassUnderTest^ CreateControl()
        {
            TClassUnderTest^ control = nullptr;

            RunOnUIThread([&]()
            {
                auto rootPanel = ref new xaml_controls::StackPanel();
                TestServices::WindowHelper->WindowContent = rootPanel;

                control = ref new TClassUnderTest();
                rootPanel->Children->Append(control);

                xaml_controls::Button^ btn = ref new xaml_controls::Button();
                btn->AccessKey = L"~~~"; //We create a dummy AK so that we can always enter AK mode
                rootPanel->Children->Append(btn);
            });
            TestServices::WindowHelper->WaitForIdle();
            FocusTestHelper::EnsureFocus(control, FocusState::Programmatic);

            return control;
        }

        static void BasicAccessKeyTest(SubTestEnum subTestEnum, UIElement^ fe, std::shared_ptr<Event> invocationEvent);

        static void InvokeAccessKeysOnMultipleButtons(std::vector<xaml_primitives::ButtonBase^>);
        static void TryMovingFocusToXamlForInit();

        static Microsoft::UI::Xaml::Input::IAccessKeyManagerStatics^ GetAccessKeyManagerStatics();
    };

    class AccessKeyTester
    {
    public:
        AccessKeyTester(const wchar_t* name)
            : m_name(ref new Platform::String(name)),
            m_isShown(false),
            m_executeCount(0)
        {
        }

        bool m_isShown;
        int m_executeCount;
        std::shared_ptr<Event> keyShownEvent = std::make_shared<Event>();
        std::shared_ptr<Event> keyHiddenEvent = std::make_shared<Event>();
        std::shared_ptr<Event> keyInvokedEvent = std::make_shared<Event>();

        template<typename Element>
        Element^ GetOwner() const
        {
            return dynamic_cast<Element^>(m_owner);
        }

        void SetOwner(xaml::UIElement^ owner)
        {
            m_owner = owner;
            VERIFY_IS_NOT_NULL(m_owner);

            Attach();
        }

        void Attach(xaml_controls::Panel^ rootPanel)
        {
            SetOwner(safe_cast<xaml::UIElement^>(rootPanel->FindName(m_name)));
        }

    private:
        Platform::String^ m_name = nullptr;
        xaml::UIElement^ m_owner = nullptr;

        SafeEventRegistrationType(Microsoft::UI::Xaml::UIElement, AccessKeyDisplayRequested) keyShownRegistration =
            CreateSafeEventRegistration(Microsoft::UI::Xaml::UIElement, AccessKeyDisplayRequested);

        SafeEventRegistrationType(Microsoft::UI::Xaml::UIElement, AccessKeyDisplayDismissed) keyHiddenRegistration =
            CreateSafeEventRegistration(Microsoft::UI::Xaml::UIElement, AccessKeyDisplayDismissed);

        SafeEventRegistrationType(Microsoft::UI::Xaml::UIElement, AccessKeyInvoked) keyInvokedRegistration =
            CreateSafeEventRegistration(Microsoft::UI::Xaml::UIElement, AccessKeyInvoked);

        void Attach()
        {
            keyShownRegistration.Attach(m_owner, [&]() { m_isShown=true; keyShownEvent->Set(); });
            keyHiddenRegistration.Attach(m_owner, [&]() { m_isShown=false; keyHiddenEvent->Set();});
            keyInvokedRegistration.Attach(m_owner, [&]() { m_executeCount++; keyInvokedEvent->Set();});
        }

    };

    }
} } } }
