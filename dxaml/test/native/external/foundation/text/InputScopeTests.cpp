// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <MUX-ETWEvents.h>
#include <InputScope.h>
#include "InputScopeTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <ppltasks.h>
#include "FileLoader.h"
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include <array>
#include "ETWWaiterProxy.h"
#include <WUCRenderingScopeGuard.h>


using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Text;
using namespace test_infra;
using namespace std;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Text {

        bool InputScopeTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool InputScopeTests::ClassCleanup()
        {
            return true;
        }

        bool InputScopeTests::TestSetup()
        {
            test_infra::TestServices::WindowHelper->InitializeXaml();
            return true;
        }

        bool InputScopeTests::TestCleanup()
        {
            test_infra::TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        Platform::String^ InputScopeTests::GetPathToFiles() const
        {
            // Get the deployment directory, and then append our test's directory to the end
            auto deploymentDir = GetTestDeploymentDir();
            return ref new Platform::String(deploymentDir + L"resources\\native\\foundation\\text\\");
        }


        //---------------------------------------------------------------------------------
        // Test case: Validates the TextBox controls's input scope property set/get.
        // On desktop, verify correct input scope IS enum is delivered to SIP by listening to its ETW event
        //----------------------------------------------------------------------------------
        void InputScopeTests::InputScopePropertyTest()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

            ::Windows::Foundation::Size size(400, 600);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            // focus events
            auto buttonGotFocusEvent = std::make_shared<Event>();

            auto buttonGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            // root loaded event
            auto loadedEvent = std::make_shared<Event>();
            auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::StackPanel, Loaded);

            xaml_controls::TextBox^ textBox = nullptr;
            xaml_controls::Button^ button = nullptr;

            xaml_controls::StackPanel^ rootStackPanel = safe_cast<xaml_controls::StackPanel^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"InputScopeTests.xaml"));
            VERIFY_IS_NOT_NULL(rootStackPanel);
            RunOnUIThread([&]()
            {
                loadedRegistration.Attach(rootStackPanel, ref new xaml::RoutedEventHandler([loadedEvent](Platform::Object^, xaml::RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Loaded event fired on StackPanel");
                    loadedEvent->Set();
                }));

                TestServices::WindowHelper->WindowContent = rootStackPanel;

                textBox = safe_cast<xaml_controls::TextBox^>(rootStackPanel->FindName("textBox"));
                VERIFY_IS_NOT_NULL(textBox);

                button = safe_cast<xaml_controls::Button^>(rootStackPanel->FindName("button"));
                VERIFY_IS_NOT_NULL(button);

                buttonGotFocusRegistration.Attach(
                    button,
                    ref new xaml::RoutedEventHandler(
                    [buttonGotFocusEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Button control GotFocus handler.");
                    buttonGotFocusEvent->Set();
                }));

            });

            TestServices::WindowHelper->WaitForIdle();
            loadedEvent->WaitForDefault();

            //due to the long running time for this test, only test first and last inputscope defined in IS ENUM.
            std::array<InputScopeTestStruct, 2> inputScopeValues =
            {
                {
                    { Xaml::Input::InputScopeNameValue::Default, L"Default", IS_DEFAULT},
                  //{ Xaml::Input::InputScopeNameValue::Hanja, L"Hanja", IS_HANJA},
                  //{ Xaml::Input::InputScopeNameValue::Number, L"Number", IS_NUMBER },
                  //{ Xaml::Input::InputScopeNameValue::Url, L"Url", IS_URL},
                  //{ Xaml::Input::InputScopeNameValue::EmailSmtpAddress, L"EmailSmtpAddress",IS_EMAIL_SMTPEMAILADDRESS},
                  //{ Xaml::Input::InputScopeNameValue::Maps, L"Maps", IS_MAPS},
                  //{ Xaml::Input::InputScopeNameValue::NumericPin, L"NumericPin", IS_NUMERIC_PIN },
                  //{ Xaml::Input::InputScopeNameValue::AlphanumericPin, L"AlphanumericPin", IS_ALPHANUMERIC_PIN },
                  //  { Xaml::Input::InputScopeNameValue::FormulaNumber, L"FormulaNumber", IS_FORMULA_NUMBER },
                    { Xaml::Input::InputScopeNameValue::ChatWithoutEmoji, L"ChatWithoutEmoji", IS_CHAT_WITHOUT_EMOJI },
                }
            };

            for (const auto& scopeStruct : inputScopeValues) //loop through the test structure for testing a few input scopes on textbox
            {
                WCHAR scopeFilter[32];
                StringCchPrintf(scopeFilter, 32, L"@InputScopeValue=%d", static_cast<int>(scopeStruct.IS));
                Platform::String ^ filterString = ref new Platform::String (scopeFilter);
                ETWWaiterProxy inputScopeETWWaiter(WINDOWS_UI_XAML_ETW_PROVIDER, SendInputScopeToRichEditInfo_value, filterString);

                RunOnUIThread([&]()
                {
                    Xaml::Input::InputScope ^scope1 = ref new Xaml::Input::InputScope();
                    Xaml::Input::InputScopeName ^scopeName1 = ref new Xaml::Input::InputScopeName(scopeStruct.XAML_IS);
                    scope1->Names->Append(scopeName1);
                    textBox->InputScope = scope1;
                });

                TestServices::WindowHelper->WaitForIdle();
                VERIFY_NO_THROW(inputScopeETWWaiter.WaitForDefault());

                RunOnUIThread([&]()
                {
                    VERIFY_ARE_EQUAL(textBox->InputScope->Names->GetAt(0)->NameValue, scopeStruct.XAML_IS);
                });
            }

            // disable DComp comparison while waiting for resolution indepdent comparison logic, a known test issue
            //LOG_OUTPUT(L"Recording DComp tree to check textbox was scrolled into view...");
            //TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison);

            RunOnUIThread([&]()
            {
                button->Focus(FocusState::Pointer);
            });
            buttonGotFocusEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // Prevent those text controls from getting focus again so input pane remains hidden.
                textBox->IsEnabled = false;
                LOG_OUTPUT(L"TextBox disabled.");
            });

            TestServices::WindowHelper->WaitForIdle();
        }
    } }
} } } }
