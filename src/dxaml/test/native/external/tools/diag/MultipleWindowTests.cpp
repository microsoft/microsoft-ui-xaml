// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"

#include "MultipleWindowTests.h"
#include "XamlDiagnosticsTestHelpers.h"
#include <CustomUserControl.h>
#include "CustomTypes.XamlTypeInfo.g.h"
#include <XamlTailored.h>
#include <TestCleanupWrapper.h>
#include "FileLoader.h"
#include <TestEvent.h>
#include "MainPage.xaml.h"
#include "SafeEventRegistration.h"

#include "RuleTesterHelper.h"

using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Animation;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Shapes;
using namespace Microsoft::UI::Xaml::Data;
using namespace ::Tests::Tools::XamlDiagnostics;
using namespace test_infra;
using namespace ::Windows::Foundation;
using namespace ::Windows::Foundation::Collections;

namespace shared_types = ::Tests::Tools::Shared;

// Helper to call Close on an object that supports IClosable
void CloseObject(Platform::Object^ obj)
{
    Microsoft::WRL::ComPtr<IUnknown> unk(reinterpret_cast<IUnknown*>(obj));
    Microsoft::WRL::ComPtr<ABI::Windows::Foundation::IClosable> closable;

    VERIFY_SUCCEEDED(unk.As(&closable));
    VERIFY_SUCCEEDED(closable->Close());
}

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Tools {
    namespace XamlDiagnostics {

        #pragma region Test Methods

        bool MultipleWindowTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();


            return true;
        }

        bool MultipleWindowTests::ClassCleanup()
        {
            return true;
        }

        bool MultipleWindowTests::TestSetup()
        {
            TestServices::WindowHelper->InitializeXaml();

            return EnsureTapLoaded();
        }

        bool MultipleWindowTests::TestCleanup()
        {
            test_infra::TestServices::WindowHelper->ShutdownXaml();
            test_infra::TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        void MultipleWindowTests::BasicMultipleWindowTest_Notified()
        {
            // Advise and setup the initial view
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(XamlDiagnosticsTestHelpers::gridString, callback);

            VERIFY_ARE_EQUAL(callback->GetNumberOfRoots(), 1u);

            auto secondaryView = TestServices::WindowHelper->CreateNewView(ref new ViewCreatedCallback([&] (){
                auto grid = dynamic_cast<xaml_controls::RelativePanel^> (xaml_markup::XamlReader::Load(XamlDiagnosticsTestHelpers::relativePanelWithCollapsedElementsString));
                TestServices::WindowHelper->WindowContent = grid;
            }));

            // Bring the view to the front
            TestServices::WindowHelper->BringSecondaryViewToFront(secondaryView);

            VERIFY_ARE_EQUAL(callback->GetNumberOfRoots(), 2u);

            // Manually close the secondary view so that the window get's closed and our callback verification is happy
            CloseObject(secondaryView);
        }

        void MultipleWindowTests::BasicMultipleWindowTest_Attach()
        {
            // Advise and setup the initial view
            wrl::ComPtr<VisualTreeServiceCallback> callback;

            auto view = TestServices::WindowHelper->CreateNewView(ref new ViewCreatedCallback([&] {
                // RunOnUI thread is not needed, since we are already on the correct thread
                auto grid = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(XamlDiagnosticsTestHelpers::gridString));
                TestServices::WindowHelper->WindowContent = grid;
            }));

            // Bring the view to the front
            TestServices::WindowHelper->BringSecondaryViewToFront(view);

            auto cleanup = m_connectionHelper->AdviseOnMainQueue(XamlDiagnosticsTestHelpers::gridString, callback);

            // Switch back to the main view so that we get the loaded event. This test will be run twice with one time
            // this value being true, and the other time it being false.
            WEX::Common::String value;
            if (SUCCEEDED(WEX::TestExecution::TestData::TryGetValue(L"ShowMainView", value)) && value.CompareNoCase(L"True") == 0)
            {
                TestServices::WindowHelper->BringMainViewToFront();
            }

            VERIFY_ARE_EQUAL(callback->GetNumberOfRoots(), 2u);

            // Manually close the secondary view so that the window get's closed and our callback verification
            CloseObject(view);
        }

        void MultipleWindowTests::VerifyFailGettingWindowProperties()
        {
            // Advise and setup the initial view
            wrl::ComPtr<VisualTreeServiceCallback> callback;

            auto cleanup = m_connectionHelper->Advise(XamlDiagnosticsTestHelpers::gridString, callback);

            VERIFY_ARE_EQUAL(callback->GetNumberOfRoots(), 1u);

            auto windowHandle = callback->GetRoots().at(0);

            VERIFY_THROWS_SPECIFIC(GetPropertyChainValue(windowHandle, L"Content", BaseValueSourceLocal), WEX::Common::Exception, [](WEX::Common::Exception ex) {return ex.ErrorCode() == E_NOINTERFACE; });

            unsigned int wouldBeContentIndex = 0;
            HRESULT hr = m_tap->GetPropertyIndex(windowHandle, L"Microsoft.UI.Xaml.Window.Content", &wouldBeContentIndex);
            VERIFY_ARE_EQUAL(hr, E_NOINTERFACE);
            // pass in an arbitrarily low index, but one that is valid
            InstanceHandle wouldBeContent = 0;
            hr = m_tap->GetProperty(windowHandle, 550, &wouldBeContent);
            VERIFY_ARE_EQUAL(hr, E_NOINTERFACE);
        }

        void MultipleWindowTests::SetCoreDispatcherOnce()
        {
            TestCleanupWrapper cleanup;
            xaml_controls::Grid^ root;
            auto callback = m_connectionHelper->Advise();
            auto scopeGuard = wil::scope_exit([&]
            {
                VERIFY_SUCCEEDED(m_tap->UnadviseVisualTreeChange(callback.Get()));
            });

            RunOnUIThread([&]()
            {
                root = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                    L"<Grid x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"</Grid>"));
                TestServices::WindowHelper->WindowContent = root;
            });

            auto view = TestServices::WindowHelper->CreateNewView(ref new ViewCreatedCallback([&] {
                // RunOnUI thread is not needed, since we are already on the correct thread
                auto grid = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(XamlDiagnosticsTestHelpers::gridString));
                TestServices::WindowHelper->WindowContent = grid;
            }));

            wrl::ComPtr<IInspectable> uiLayer1;
            VERIFY_SUCCEEDED(m_tap->GetUiLayer(&uiLayer1));

            // Bring the view to the front
            TestServices::WindowHelper->BringSecondaryViewToFront(view);

            // After bringing the view to front, make sure the UI Layer's we get are the same
            wrl::ComPtr<IInspectable> uiLayer2;
            VERIFY_SUCCEEDED(m_tap->GetUiLayer(&uiLayer2));

            InstanceHandle uiLayer1Handle = 0;
            VERIFY_SUCCEEDED(m_tap->GetHandleFromIInspectable(uiLayer1.Get(), &uiLayer1Handle));
            InstanceHandle uiLayer2Handle = 0;
            VERIFY_SUCCEEDED(m_tap->GetHandleFromIInspectable(uiLayer2.Get(), &uiLayer2Handle));
            auto exit = wil::scope_exit([&](){
                m_tap->UnregisterInstance(uiLayer1Handle);
                m_tap->UnregisterInstance(uiLayer2Handle);
            });
            VERIFY_ARE_EQUAL(uiLayer1Handle, uiLayer2Handle);
            // Call GetPropertyChainValue, verify we can successfully get a property. Since we are only in single window mode
            auto rootHandle = ih_cast(root);
            VERIFY_NO_THROW(GetPropertyChainValue(rootHandle, L"Background"));

            CloseObject(view);
        }

        void MultipleWindowTests::ProperlyHandleApplicationResources()
        {
            // Dupe of existing bug:
            // Leak: TemplateContent peer not being unpegged
            // This may be timing related with our shutdown code since it doesn't repro all the time.
            // Hopefully it's not an actual leak due to some race condition, and instead we simply
            // need to allow another tick to properly clean up.
            TestServices::ErrorHandlingHelper->IgnoreLeaksForTest();

            auto xamlStr = ref new Platform::String(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name = 'parent'>\r\n"
                L"  <Button x:Name='button'/>"
                L"</Grid>\r\n");
            // Advise and setup the initial view
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(xamlStr, callback);

            auto primaryButtonElement = callback->GetElementByName(L"button");
            auto primaryButton = ih_cast<xaml_controls::Button>(primaryButtonElement.Handle);

            xaml_controls::Button^ secondaryButton = nullptr;
            auto secondaryView = TestServices::WindowHelper->CreateNewView(ref new ViewCreatedCallback([&]() {
                auto grid = safe_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(xamlStr));
                TestServices::WindowHelper->WindowContent = grid;

                secondaryButton = safe_cast<xaml_controls::Button^>(grid->FindName(L"button"));
                VERIFY_IS_NOT_NULL(secondaryButton);
            }));

            LOG_OUTPUT(L"Adding implicit style to main view");
            {
                auto buttonStyle = CreateStyle(L"Microsoft.UI.Xaml.Controls.Button");

                auto backgroundSetter = CreateSetterWithProperty(CreateInstance(L"Microsoft.UI.Xaml.Media.Brush", L"Red"), L"Microsoft.UI.Xaml.Controls.Control.Background");

                AddSetterToStyle(buttonStyle, backgroundSetter);

                auto mainViewResources = GetApplicationResources();
                AddImplicitStyle(mainViewResources, buttonStyle);
                // We have to remove this from application resources otherwise in stress mode trying to add it again will fail.
                auto removeFromApp = wil::scope_exit([&] {
                    RemoveImplicitStyle(mainViewResources, L"Microsoft.UI.Xaml.Controls.Button");
                });
                RunOnUIThread([&] {
                    auto background = safe_cast<xaml_media::SolidColorBrush^>(primaryButton->Background);
                    VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Red, background->Color);
                });
            }

            TestServices::WindowHelper->BringSecondaryViewToFront(secondaryView);
            // Bring the view to the front and add a style to Application.Resources. Note that this can't be the same style, since
            // the style is thread dependent
            LOG_OUTPUT(L"Adding implicit style to main view");
            {
                auto buttonStyle = CreateStyle(L"Microsoft.UI.Xaml.Controls.Button");
                auto backgroundSetter = CreateSetterWithProperty(CreateInstance(L"Microsoft.UI.Xaml.Media.Brush", L"Green"), L"Microsoft.UI.Xaml.Controls.Control.Background");
                AddSetterToStyle(buttonStyle, backgroundSetter);

                auto secondaryViewResources = GetApplicationResources();
                AddImplicitStyle(secondaryViewResources, buttonStyle);
                // We have to remove this from application resources otherwise in stress mode trying to add it again will fail.
                auto removeFromApp = wil::scope_exit([&] {
                    RemoveImplicitStyle(secondaryViewResources, L"Microsoft.UI.Xaml.Controls.Button");
                });
                RunOnUIThread([&] {
                    auto background = safe_cast<xaml_media::SolidColorBrush^>(secondaryButton->Background);
                    VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Green, background->Color);
                });
            }
            // Manually close the secondary view so that the window gets closed and our callback verification is happy
            CloseObject(secondaryView);
        }


        #pragma endregion

    }
} } } } }
