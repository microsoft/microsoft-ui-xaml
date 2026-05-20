// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "AutoSuggestBoxIntegrationTests.h"

#include <generic\DependencyObjectTests.h>
#include <generic\FrameworkElementTests.h>

#include <XamlTailored.h>
#include <TestCleanupWrapper.h>
#include <DisableErrorReportingScopeGuard.h>
#include <TreeHelper.h>
#include <Utils.h>
#include <ControlHelper.h>
#include <FileLoader.h>
#include <FocusTestHelper.h>
#include <PopupHelper.h>

#include <RuntimeEnabledFeatureOverride.h>
#include <CustomPropertySupport.h>
#include <WUCRenderingScopeGuard.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;
using namespace ::Windows::UI::ViewManagement;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace AutoSuggestBox {

    bool AutoSuggestBoxIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();

        m_bIsTracing = false; // initializing this flag

        return true;
    }

    bool AutoSuggestBoxIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool AutoSuggestBoxIntegrationTests::TestCleanup()
    {
        TestServices::WindowHelper->VerifyTestCleanup();
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        StopTracingTelemetry();
        return true;
    }

    ref class SuggestionListObject sealed : public Microsoft::UI::Xaml::Tests::Common::CustomPropertyProviderBase
    {
    public:
        SuggestionListObject(Platform::String^ title)
        {
            this->Title = title;
        }

    protected:
        void AddCustomProperties() override
        {
            AddCustomProperty(L"Title", Platform::String::typeid,
                MAKEPROPGET(SuggestionListObject^, Title),
                MAKEPROPSET(SuggestionListObject^, Title, Platform::String^)
                );
        }

    public:
        property Platform::String^ Title;
    };

    //
    // Function to Start tracing telemetry data
    //
    void AutoSuggestBoxIntegrationTests::StartTracingTelemetry()
    {
        try
        {
            // Stop tracing telemetry if it is already being traced
            if (m_bIsTracing)
            {
                TraceConsumer::Stop();
            }

            TraceConsumer::Start();
            m_bIsTracing = true;
        }
        catch (...)
        {
            m_bIsTracing = false;
        }
    }

    //
    // Function to stop tracing telemetry if it is already being traced
    //
    void AutoSuggestBoxIntegrationTests::StopTracingTelemetry()
    {
        try
        {
            if (m_bIsTracing)
            {
                TraceConsumer::Stop();
            }
        }
        catch (...)
        {

        }
        m_bIsTracing = false;
    }

    //
    // Test Cases
    //
    void AutoSuggestBoxIntegrationTests::CanInstantiate()
    {
        Generic::DependencyObjectTests<xaml_controls::AutoSuggestBox>::CanInstantiate();
    }

    void AutoSuggestBoxIntegrationTests::CanEnterAndLeaveLiveTree()
    {
        Generic::FrameworkElementTests<xaml_controls::AutoSuggestBox>::CanEnterAndLeaveLiveTree();
    }

    void AutoSuggestBoxIntegrationTests::CanGetAndSetProperties()
    {
        TestCleanupWrapper cleanup;

        auto itemList = ref new Platform::Collections::Vector<Platform::String^>();

        itemList->Append("Red");
        itemList->Append("Blue");
        itemList->Append("Yellow");

        RunOnUIThread([&]()
        {
            auto autoSuggestBox = ref new xaml_controls::AutoSuggestBox();

            VERIFY_IS_TRUE(autoSuggestBox->AutoMaximizeSuggestionArea);
            autoSuggestBox->AutoMaximizeSuggestionArea = false;
            VERIFY_IS_FALSE(autoSuggestBox->AutoMaximizeSuggestionArea);

            VERIFY_IS_NULL(autoSuggestBox->ItemsSource);
            autoSuggestBox->ItemsSource = itemList;
            VERIFY_IS_TRUE(autoSuggestBox->ItemsSource == itemList);

            VERIFY_IS_FALSE(autoSuggestBox->IsSuggestionListOpen);

            VERIFY_IS_TRUE(autoSuggestBox->UpdateTextOnSelect);
            autoSuggestBox->UpdateTextOnSelect = false;
            VERIFY_IS_FALSE(autoSuggestBox->UpdateTextOnSelect);

            VERIFY_IS_TRUE(autoSuggestBox->Text == "");
            autoSuggestBox->Text = "Auto Suggest Box";
            VERIFY_IS_TRUE(autoSuggestBox->Text == "Auto Suggest Box");

            VERIFY_IS_TRUE(autoSuggestBox->PlaceholderText == "");
            autoSuggestBox->PlaceholderText = "Placeholder Text";
            VERIFY_IS_TRUE(autoSuggestBox->PlaceholderText == "Placeholder Text");

            VERIFY_IS_TRUE(autoSuggestBox->TextMemberPath == "");
            autoSuggestBox->TextMemberPath = "Text Member Path";
            VERIFY_IS_TRUE(autoSuggestBox->TextMemberPath == "Text Member Path");
        });
    }

    void AutoSuggestBoxIntegrationTests::CanRaiseTextChangedEvent()
    {
        TestCleanupWrapper cleanup;
        DisableErrorReportingScopeGuard disableErrors;

        xaml_controls::AutoSuggestBox^ autoSuggestBox = nullptr;
        auto textChangedEvent = std::make_shared<Event>();
        auto textChangedRegistration = CreateSafeEventRegistration(xaml_controls::AutoSuggestBox, TextChanged);

        RunOnUIThread([&]()
        {
            autoSuggestBox = ref new xaml_controls::AutoSuggestBox();

            textChangedRegistration.Attach(autoSuggestBox,
                ref new wf::TypedEventHandler<xaml_controls::AutoSuggestBox^, xaml_controls::AutoSuggestBoxTextChangedEventArgs^>(
                    [&](Platform::Object^, xaml_controls::AutoSuggestBoxTextChangedEventArgs^)
            {
                VERIFY_IS_TRUE(autoSuggestBox->Text == "Test String");
                textChangedEvent->Set();
            }));

            TestServices::WindowHelper->WindowContent = autoSuggestBox;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            autoSuggestBox->Text = "Test String";
        });

        textChangedEvent->WaitForDefault();
    }

    void AutoSuggestBoxIntegrationTests::CanRaiseTextChangedEventInFlyout()
    {
        TestCleanupWrapper cleanup;
        DisableErrorReportingScopeGuard disableErrors;

        xaml_controls::Button^ button = nullptr;
        xaml_controls::AutoSuggestBox^ autoSuggestBox = nullptr;
        xaml_controls::Flyout^ flyout = nullptr;

        auto textChangedEvent = std::make_shared<Event>();
        auto flyoutOpenedEvent = std::make_shared<Event>();
        auto flyoutClosedEvent = std::make_shared<Event>();
        auto textChangedRegistration = CreateSafeEventRegistration(xaml_controls::AutoSuggestBox, TextChanged);
        auto openedRegistration = CreateSafeEventRegistration(xaml_controls::Flyout, Opened);
        auto closedRegistration = CreateSafeEventRegistration(xaml_controls::Flyout, Closed);

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
                L"      x:Name='root' Background='SlateBlue' Width='400' Height='200' VerticalAlignment='Top' HorizontalAlignment='Left'> "
                L"  <Button x:Name='button' Content='button.flyout' HorizontalAlignment='Left' FontSize='25' > "
                L"    <Button.Flyout> "
                L"      <Flyout Placement='Top'> "
                L"        <AutoSuggestBox x:Name='ASB'/> "
                L"      </Flyout> "
                L"    </Button.Flyout> "
                L"  </Button> "
                L"</Grid>"));

            autoSuggestBox = safe_cast<xaml_controls::AutoSuggestBox^>(rootPanel->FindName(L"ASB"));
            button = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button"));
            flyout = safe_cast<xaml_controls::Flyout^>(button->Flyout);

            textChangedRegistration.Attach(autoSuggestBox,
                ref new wf::TypedEventHandler<xaml_controls::AutoSuggestBox^, xaml_controls::AutoSuggestBoxTextChangedEventArgs^>(
                    [&](Platform::Object^, xaml_controls::AutoSuggestBoxTextChangedEventArgs^)
            {
                VERIFY_IS_TRUE(autoSuggestBox->Text == "Test String");
                textChangedEvent->Set();
            }));

            openedRegistration.Attach(flyout, ref new wf::EventHandler<Platform::Object^>([flyoutOpenedEvent](Platform::Object^, Platform::Object^)
            {
                LOG_OUTPUT(L"PopupOpenClose: Flyout Opened event is fired!");
                flyoutOpenedEvent->Set();
            }));

            closedRegistration.Attach(flyout, ref new wf::EventHandler<Platform::Object^>([flyoutClosedEvent](Platform::Object^, Platform::Object^)
            {
                LOG_OUTPUT(L"PopupOpenClose: Flyout Closed event is fired!");
                flyoutClosedEvent->Set();
            }));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Button Tap operation to show the Flyout.");
        TestServices::InputHelper->Tap(button);
        flyoutOpenedEvent->WaitForDefault();

        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            autoSuggestBox->Text = "Test String";
        });
        TestServices::WindowHelper->WaitForIdle();
        textChangedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"RootPanel Tap operation to close the Flyout.");
            flyout->Hide();
        });
        flyoutClosedEvent->WaitForDefault();
    }

    void AutoSuggestBoxIntegrationTests::ValidateTextBoxStyle()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::AutoSuggestBox^ autoSuggestBox = nullptr;
        xaml_controls::TextBox^ textBox = nullptr;

        RunOnUIThread([&]()
        {
            // The MUXC theme resources clash with these changes, so we'll clear them out first.
            Application::Current->Resources->MergedDictionaries->Clear();
                
            auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                L"<StackPanel.Resources> "
                L"  <Style x:Name='asbStyle' TargetType='TextBox'> "
                L"      <Setter Property='BorderBrush' Value='Red' /> "
                L"      <Setter Property='Background' Value='Green' /> "
                L"      <Setter Property='Margin' Value='50' /> "
                L"  </Style> "
                L"</StackPanel.Resources> "
                L"  <AutoSuggestBox x:Name='autoSuggestBox' TextBoxStyle='{StaticResource asbStyle}' /> "
                L"  <Button x:Name='button1' Content='Button' /> "
                L"</StackPanel>"));

            autoSuggestBox = safe_cast<xaml_controls::AutoSuggestBox^>(rootPanel->FindName(L"autoSuggestBox"));
            VERIFY_IS_NOT_NULL(autoSuggestBox);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            textBox = safe_cast<xaml_controls::TextBox^>(TreeHelper::GetVisualChildByName(autoSuggestBox, L"TextBox"));
            VERIFY_IS_NOT_NULL(textBox);
        });

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL((safe_cast<xaml_media::SolidColorBrush^>(textBox->Background)->Color), Microsoft::UI::Colors::Green);
        });

    }

    void AutoSuggestBoxIntegrationTests::CanRaiseSuggestionChosenEventProjectedShadow()
    {
        RuntimeEnabledFeatureOverride featureUseDropShadows(RuntimeFeatureBehavior::RuntimeEnabledFeature::ForceProjectedShadowsOnByDefault, true);
        CanRaiseSuggestionChosenEvent();
    }

    void AutoSuggestBoxIntegrationTests::CanRaiseSuggestionChosenEventDropShadow()
    {
        CanRaiseSuggestionChosenEvent();
    }

    void AutoSuggestBoxIntegrationTests::CanRaiseSuggestionChosenEvent()
    {
        TestCleanupWrapper cleanup;
        // disable caret rendering to stabilize tree dumps
        RuntimeEnabledFeatureOverride disableTextBoxCaret(RuntimeFeatureBehavior::RuntimeEnabledFeature::DisableTextBoxCaret, true);
        DisableErrorReportingScopeGuard disableErrors;

        xaml_controls::AutoSuggestBox^ autoSuggestBox = nullptr;
        xaml_controls::Button^ button1 = nullptr;
        xaml_controls::TextBox^ textBox = nullptr;
        xaml_primitives::Popup^ popup = nullptr;
        xaml_controls::ListView^ listView = nullptr;
        xaml_controls::TextBlock^ textBlock1 = nullptr;

        auto gotFocusEvent = std::make_shared<Event>();
        auto gotFocusButtonEvent = std::make_shared<Event>();
        auto textChangedEvent = std::make_shared<Event>();
        auto textChanged2Event = std::make_shared<Event>();
        auto popupOpenedEvent = std::make_shared<Event>();
        auto suggestionChosenEvent = std::make_shared<Event>();
        auto gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::AutoSuggestBox, GotFocus);
        auto gotFocusButtonRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);
        auto textChangedRegistration = CreateSafeEventRegistration(xaml_controls::AutoSuggestBox, TextChanged);
        auto textChanged2Registration = CreateSafeEventRegistration(xaml_controls::AutoSuggestBox, TextChanged);
        auto popupOpenedRegistration = CreateSafeEventRegistration(xaml_primitives::Popup, Opened);
        auto suggestionChosenRegistration = CreateSafeEventRegistration(xaml_controls::AutoSuggestBox, SuggestionChosen);
        auto itemList = ref new Platform::Collections::Vector<Platform::String^>();

        itemList->Append("Ruby");
        itemList->Append("Sapphire");
        itemList->Append("Emerald");

        RunOnUIThread([&]()
        {
            auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' Background='White' > "
                L"  <Rectangle Fill='Blue' />"
                L"  <AutoSuggestBox x:Name='autoSuggestBox' /> "
                L"  <Button x:Name='button1' Content='Button' /> "
                L"  <TextBlock x:Name='textBlock1' FontSize='20' /> "
                L"</StackPanel>"));

            autoSuggestBox = safe_cast<xaml_controls::AutoSuggestBox^>(rootPanel->FindName(L"autoSuggestBox"));
            VERIFY_IS_NOT_NULL(autoSuggestBox);

            textBlock1 = safe_cast<xaml_controls::TextBlock^>(rootPanel->FindName(L"textBlock1"));
            VERIFY_IS_NOT_NULL(textBlock1);

            button1 = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button1"));
            VERIFY_IS_NOT_NULL(button1);

            gotFocusRegistration.Attach(
                autoSuggestBox,
                ref new xaml::RoutedEventHandler(
                [gotFocusEvent](Platform::Object^ sender, xaml::IRoutedEventArgs^)
            {
                LOG_OUTPUT(L"CanRaiseSuggestionChosenEvent: GotFocus event fired on ASB!");
                gotFocusEvent->Set();
            }));

            gotFocusButtonRegistration.Attach(
                button1,
                ref new xaml::RoutedEventHandler(
                [gotFocusButtonEvent](Platform::Object^ sender, xaml::IRoutedEventArgs^)
            {
                LOG_OUTPUT(L"CanRaiseSuggestionChosenEvent: GotFocus event fired on Button!");
                gotFocusButtonEvent->Set();
            }));

            textChangedRegistration.Attach(autoSuggestBox,
                ref new wf::TypedEventHandler<xaml_controls::AutoSuggestBox^, xaml_controls::AutoSuggestBoxTextChangedEventArgs^>(
                [&](Platform::Object^, xaml_controls::AutoSuggestBoxTextChangedEventArgs^ args)
            {
                LOG_OUTPUT(L"CanRaiseSuggestionChosenEvent: TextChanged event fired on ASB! Input Text = %s", autoSuggestBox->Text->Data());
                textChangedEvent->Set();
            }));

            suggestionChosenRegistration.Attach(
                autoSuggestBox,
                ref new wf::TypedEventHandler<xaml_controls::AutoSuggestBox^, xaml_controls::AutoSuggestBoxSuggestionChosenEventArgs^>(
                [&](Platform::Object^, xaml_controls::AutoSuggestBoxSuggestionChosenEventArgs^ args)
            {
                LOG_OUTPUT(L"CanRaiseSuggestionChosenEvent: SuggestionChosen event fired!!!");
                textBlock1->Text = safe_cast<Platform::String^>(args->SelectedItem);
                VERIFY_IS_TRUE(textBlock1->Text == "Sapphire");
                suggestionChosenEvent->Set();
            }));

            autoSuggestBox->Margin = xaml::ThicknessHelper::FromUniformLength(20);
            autoSuggestBox->ItemsSource = itemList;

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            textBox = safe_cast<xaml_controls::TextBox^>(TreeHelper::GetVisualChildByName(autoSuggestBox, L"TextBox"));
            VERIFY_IS_NOT_NULL(textBox);
            popup = safe_cast<xaml_primitives::Popup^>(TreeHelper::GetVisualChildByName(autoSuggestBox, L"SuggestionsPopup"));
            VERIFY_IS_NOT_NULL(popup);

            popupOpenedRegistration.Attach(popup, ref new wf::EventHandler<Platform::Object^>([popupOpenedEvent](Platform::Object^, Platform::Object^)
            {
                LOG_OUTPUT(L"CanRaiseSuggestionChosenEvent: Opened event fired!");
                popupOpenedEvent->Set();
            }));
        });

        LOG_OUTPUT(L"CanRaiseSuggestionChosenEvent: Tap on ASB.");
        TestServices::InputHelper->Tap(textBox);

        gotFocusEvent->WaitForDefault();

        // Keyboard input "R" to show the suggestion list.
        LOG_OUTPUT(L"CanRaiseSuggestionChosenEvent: Keyboard Input - R");
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_shift#$d$_r#$u$_r#$u$_shift");

        textChangedEvent->WaitForDefault();
        popupOpenedEvent->WaitForDefault();

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(popup->Shadow != nullptr);

            textChanged2Registration.Attach(
                autoSuggestBox,
                ref new wf::TypedEventHandler<xaml_controls::AutoSuggestBox^, xaml_controls::AutoSuggestBoxTextChangedEventArgs^>(
                [&](Platform::Object^, xaml_controls::AutoSuggestBoxTextChangedEventArgs^ args)
            {
                LOG_OUTPUT(L"CanRaiseSuggestionChosenEvent: The second TextChanged event fired on ASB! Current Text = %s", autoSuggestBox->Text->Data());
                VERIFY_IS_TRUE(autoSuggestBox->Text == "Sapphire");
                VERIFY_IS_TRUE(args->Reason == xaml_controls::AutoSuggestionBoxTextChangeReason::SuggestionChosen);
                textChanged2Event->Set();
            }));

            LOG_OUTPUT(L"CanRaiseSuggestionChosenEvent: Get the listView object.");
            listView = safe_cast<xaml_controls::ListView^>(TreeHelper::GetVisualChildByName(
                safe_cast<FrameworkElement^>(popup->Child), L"SuggestionsList"));
            VERIFY_IS_NOT_NULL(listView);
        });

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"CanRaiseSuggestionChosenEvent: Tap on the suggestion list.");
        TestServices::InputHelper->Tap(listView);

        suggestionChosenEvent->WaitForDefault();

        textChanged2Event->WaitForDefault();

        // Ensure the close down the SIP and clear the content.
        LOG_OUTPUT(L"CanRaiseSuggestionChosenEvent: Tap on the button1.");
        TestServices::InputHelper->Tap(button1);

        gotFocusButtonEvent->WaitForDefault();

        TestServices::WindowHelper->WaitForIdle();
    }

    //
    // Test method to perform the following operations and verify certain telemetry traces
    // 1. Start tracing telemetry
    // 2. Load and Setup AutoSuggestBox
    // 3. Type a character in the editable field of AutoSuggestBox
    // 4. Choose a suggestion from the suggestion list
    // 5. Tap on a button to move focus away from the AutoSuggestBox (to dismiss the SIP)
    // 6. Stop tracing telemetry
    // 7. Verify if the telemetry traces contain expected data
    //
    void AutoSuggestBoxIntegrationTests::CanTraceTelemetryData()
    {
        // Start the trace consumer to trace telemetry before the tailored process starts.
        StartTracingTelemetry();

        // Peform test steps to choose a suggestion from AutoSuggestBox
        CanRaiseSuggestionChosenEvent();

        // Stop tracing telemetry
        StopTracingTelemetry();

        // Verify expected telemetry event names and their count
        TraceConsumer::VerifyEventTraced("ASBSuggestionListOpened", 1);
        TraceConsumer::VerifyEventTraced("ASBSuggestionSelectionChanged", 1);
    }

    void AutoSuggestBoxIntegrationTests::ValidateQuerySubmittedContainsCurrentText()
    {
        TestCleanupWrapper cleanup;

        auto autoSuggestBox = SetupAutoSuggestBoxWithQueryIcon();
        auto querySubmittedEvent = std::make_shared<Event>();
        auto querySubmittedRegistration = CreateSafeEventRegistration(xaml_controls::AutoSuggestBox, QuerySubmitted);
        Platform::String^ testString = "I am a string.";

        RunOnUIThread([&]()
        {
            querySubmittedRegistration.Attach(autoSuggestBox,
                ref new wf::TypedEventHandler<xaml_controls::AutoSuggestBox^, xaml_controls::AutoSuggestBoxQuerySubmittedEventArgs^>(
                    [&](Platform::Object^, xaml_controls::AutoSuggestBoxQuerySubmittedEventArgs^ args)
            {
                VERIFY_ARE_EQUAL(testString, args->QueryText);
                VERIFY_IS_NULL(args->ChosenSuggestion);
                querySubmittedEvent->Set();
            }));

            autoSuggestBox->Text = testString;
        });
        TestServices::WindowHelper->WaitForIdle();

        TestServices::KeyboardHelper->Enter();

        querySubmittedEvent->WaitForDefault();
    }

    void AutoSuggestBoxIntegrationTests::CanSetQueryButtonIcon()
    {
        TestCleanupWrapper cleanup;

        auto autoSuggestBox = SetupAutoSuggestBoxWithQueryIcon();
        xaml_controls::SymbolIcon^ symbolIcon = nullptr;

        RunOnUIThread([&]()
        {
            symbolIcon = ref new xaml_controls::SymbolIcon();
            symbolIcon->Symbol = xaml_controls::Symbol::Accept;
            autoSuggestBox->QueryIcon = symbolIcon;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            xaml_primitives::ButtonBase^ queryButton = safe_cast<xaml_primitives::ButtonBase^>(TreeHelper::GetVisualChildByName(autoSuggestBox, "QueryButton"));
            xaml_controls::SymbolIcon^ actualSymbolIcon = safe_cast<xaml_controls::SymbolIcon^>(queryButton->Content);
            VERIFY_IS_NOT_NULL(actualSymbolIcon);
            VERIFY_ARE_EQUAL(symbolIcon, actualSymbolIcon);
        });
    }

    void AutoSuggestBoxIntegrationTests::ValidateQueryButtonIsCollapsedWithNoQueryIcon()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::AutoSuggestBox^ autoSuggestBox = nullptr;

        RunOnUIThread([&]()
        {
            autoSuggestBox = ref new xaml_controls::AutoSuggestBox();

            TestServices::WindowHelper->WindowContent = autoSuggestBox;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            xaml_primitives::ButtonBase^ queryButton = safe_cast<xaml_primitives::ButtonBase^>(TreeHelper::GetVisualChildByName(autoSuggestBox, "QueryButton"));
            VERIFY_IS_NOT_NULL(queryButton);
            VERIFY_ARE_EQUAL(xaml::Visibility::Collapsed, queryButton->Visibility);
        });
    }

    void AutoSuggestBoxIntegrationTests::ValidateAutoSuggestBoxWorksWithoutQueryButton()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::AutoSuggestBox^ autoSuggestBox =
            safe_cast<xaml_controls::AutoSuggestBox^>(
                LoadXamlFileOnUIThread(GetPackageFolder() + L"resources\\native\\controls\\AutoSuggestBox\\AutoSuggestBoxWithoutQueryButton.xaml"));

        xaml_primitives::Popup^ popup = nullptr;
        auto gotFocusEvent = std::make_shared<Event>();
        auto textChangedEvent = std::make_shared<Event>();
        auto popupOpenedEvent = std::make_shared<Event>();
        auto popupClosedEvent = std::make_shared<Event>();
        auto suggestionChosenEvent = std::make_shared<Event>();
        auto querySubmittedEvent = std::make_shared<Event>();
        auto gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::AutoSuggestBox, GotFocus);
        auto textChangedRegistration = CreateSafeEventRegistration(xaml_controls::AutoSuggestBox, TextChanged);
        auto popupOpenedRegistration = CreateSafeEventRegistration(xaml_primitives::Popup, Opened);
        auto popupClosedRegistration = CreateSafeEventRegistration(xaml_primitives::Popup, Closed);
        auto suggestionChosenRegistration = CreateSafeEventRegistration(xaml_controls::AutoSuggestBox, SuggestionChosen);
        auto querySubmittedRegistration = CreateSafeEventRegistration(xaml_controls::AutoSuggestBox, QuerySubmitted);
        auto itemList = ref new Platform::Collections::Vector<Platform::String^>();
        Platform::String^ testString = "this";

        itemList->Append(testString);
        itemList->Append("that");
        itemList->Append("the other");

        RunOnUIThread([&]()
        {
            gotFocusRegistration.Attach(
                autoSuggestBox,
                ref new xaml::RoutedEventHandler(
                [gotFocusEvent](Platform::Object^ sender, xaml::IRoutedEventArgs^)
            {
                gotFocusEvent->Set();
            }));

            textChangedRegistration.Attach(autoSuggestBox,
                ref new wf::TypedEventHandler<xaml_controls::AutoSuggestBox^, xaml_controls::AutoSuggestBoxTextChangedEventArgs^>(
                [&](Platform::Object^, xaml_controls::AutoSuggestBoxTextChangedEventArgs^ args)
            {
                textChangedEvent->Set();
            }));

            suggestionChosenRegistration.Attach(
                autoSuggestBox,
                ref new wf::TypedEventHandler<xaml_controls::AutoSuggestBox^, xaml_controls::AutoSuggestBoxSuggestionChosenEventArgs^>(
                [&](Platform::Object^, xaml_controls::AutoSuggestBoxSuggestionChosenEventArgs^ args)
            {
                Platform::String^ actualString = safe_cast<Platform::String^>(args->SelectedItem);
                VERIFY_ARE_EQUAL(testString, actualString);
                suggestionChosenEvent->Set();
            }));

            querySubmittedRegistration.Attach(
                autoSuggestBox,
                ref new wf::TypedEventHandler<xaml_controls::AutoSuggestBox^, xaml_controls::AutoSuggestBoxQuerySubmittedEventArgs^>(
                [&](Platform::Object^, xaml_controls::AutoSuggestBoxQuerySubmittedEventArgs^ args)
            {
                querySubmittedEvent->Set();
            }));

            autoSuggestBox->ItemsSource = itemList;

            xaml_controls::Grid^ root = ref new xaml_controls::Grid();
            root->VerticalAlignment = xaml::VerticalAlignment::Top;
            root->Children->Append(autoSuggestBox);
            TestServices::WindowHelper->WindowContent = root;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            popup = safe_cast<xaml_primitives::Popup^>(TreeHelper::GetVisualChildByName(autoSuggestBox, L"SuggestionsPopup"));

            popupOpenedRegistration.Attach(popup,
                ref new wf::EventHandler<Platform::Object^>([popupOpenedEvent](Platform::Object^, Platform::Object^)
            {
                popupOpenedEvent->Set();
            }));

            popupClosedRegistration.Attach(popup,
                ref new wf::EventHandler<Platform::Object^>([popupClosedEvent](Platform::Object^, Platform::Object^)
            {
                popupClosedEvent->Set();
            }));

            autoSuggestBox->Focus(xaml::FocusState::Programmatic);
        });

        gotFocusEvent->WaitForDefault();

        TestServices::WindowHelper->WaitForIdle();

        TestServices::KeyboardHelper->PressKeySequence(L"$d$_t#$u$_t");

        textChangedEvent->WaitForDefault();
        popupOpenedEvent->WaitForDefault();

        TestServices::WindowHelper->WaitForIdle();

        TestServices::KeyboardHelper->Down();

        suggestionChosenEvent->WaitForDefault();
        textChangedEvent->WaitForDefault();

        TestServices::KeyboardHelper->Enter();

        querySubmittedEvent->WaitForDefault();
        popupClosedEvent->WaitForDefault();

        TestServices::WindowHelper->WaitForIdle();
    }

    void AutoSuggestBoxIntegrationTests::ValidateDataContextDoesNotPropagateIntoHeader()
    {
        TestCleanupWrapper cleanup;

        auto stackPanel = SetupAutoSuggestBoxTest(xaml::VerticalAlignment::Stretch, nullptr);

        RunOnUIThread([&]()
        {
            stackPanel->DataContext = stackPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // header content presenter has x:DeferLoadStrategy = "Lazy", therefore it does not exist unless Header property is set.
            auto headerContentPresenter = TreeHelper::GetVisualChildByName(stackPanel, L"HeaderContentPresenter");
            VERIFY_IS_NULL(headerContentPresenter);
        });
    }

    void AutoSuggestBoxIntegrationTests::CanTapOnItemAfterSelectingItWithKeyboard()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::AutoSuggestBox^ autoSuggestBox = nullptr;
        xaml_primitives::Popup^ popup = nullptr;
        auto gotFocusEvent = std::make_shared<Event>();
        auto textChangedEvent = std::make_shared<Event>();
        auto popupOpenedEvent = std::make_shared<Event>();
        auto popupClosedEvent = std::make_shared<Event>();
        auto querySubmittedEvent = std::make_shared<Event>();
        auto gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::AutoSuggestBox, GotFocus);
        auto textChangedRegistration = CreateSafeEventRegistration(xaml_controls::AutoSuggestBox, TextChanged);
        auto popupOpenedRegistration = CreateSafeEventRegistration(xaml_primitives::Popup, Opened);
        auto popupClosedRegistration = CreateSafeEventRegistration(xaml_primitives::Popup, Closed);
        auto querySubmittedRegistration = CreateSafeEventRegistration(xaml_controls::AutoSuggestBox, QuerySubmitted);
        auto itemList = ref new Platform::Collections::Vector<Platform::String^>();
        Platform::String^ testString = "this";

        itemList->Append(testString);
        itemList->Append("that");
        itemList->Append("the other");

        RunOnUIThread([&]()
        {
            autoSuggestBox = ref new xaml_controls::AutoSuggestBox();

            gotFocusRegistration.Attach(
                autoSuggestBox,
                ref new xaml::RoutedEventHandler(
                [gotFocusEvent](Platform::Object^ sender, xaml::IRoutedEventArgs^)
            {
                gotFocusEvent->Set();
            }));

            textChangedRegistration.Attach(autoSuggestBox,
                ref new wf::TypedEventHandler<xaml_controls::AutoSuggestBox^, xaml_controls::AutoSuggestBoxTextChangedEventArgs^>(
                [&](Platform::Object^, xaml_controls::AutoSuggestBoxTextChangedEventArgs^ args)
            {
                textChangedEvent->Set();
            }));

            querySubmittedRegistration.Attach(
                autoSuggestBox,
                ref new wf::TypedEventHandler<xaml_controls::AutoSuggestBox^, xaml_controls::AutoSuggestBoxQuerySubmittedEventArgs^>(
                [&](Platform::Object^, xaml_controls::AutoSuggestBoxQuerySubmittedEventArgs^ args)
            {
                VERIFY_ARE_EQUAL(testString, args->QueryText);
                VERIFY_IS_NOT_NULL(args->ChosenSuggestion);
                VERIFY_ARE_EQUAL(testString, safe_cast<Platform::String^>(args->ChosenSuggestion));

                querySubmittedEvent->Set();
            }));

            autoSuggestBox->ItemsSource = itemList;
            xaml_controls::Grid^ root = ref new xaml_controls::Grid();
            root->VerticalAlignment = xaml::VerticalAlignment::Top;
            root->Children->Append(autoSuggestBox);
            TestServices::WindowHelper->WindowContent = root;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            popup = safe_cast<xaml_primitives::Popup^>(TreeHelper::GetVisualChildByName(autoSuggestBox, L"SuggestionsPopup"));

            popupOpenedRegistration.Attach(popup,
                ref new wf::EventHandler<Platform::Object^>([popupOpenedEvent](Platform::Object^, Platform::Object^)
            {
                popupOpenedEvent->Set();
            }));

            popupClosedRegistration.Attach(popup,
                ref new wf::EventHandler<Platform::Object^>([popupClosedEvent](Platform::Object^, Platform::Object^)
            {
                popupClosedEvent->Set();
            }));

            autoSuggestBox->Focus(xaml::FocusState::Programmatic);
        });

        gotFocusEvent->WaitForDefault();

        TestServices::WindowHelper->WaitForIdle();

        TestServices::KeyboardHelper->PressKeySequence(L"$d$_t#$u$_t");

        textChangedEvent->WaitForDefault();
        popupOpenedEvent->WaitForDefault();

        TestServices::WindowHelper->WaitForIdle();

        TestServices::KeyboardHelper->Down();

        xaml::FrameworkElement^ firstSuggestion = nullptr;
        RunOnUIThread([&]()
        {
            auto suggestionsList = safe_cast<xaml_controls::ListView^>(TreeHelper::GetVisualChildByNameFromOpenPopups(L"SuggestionsList", autoSuggestBox));
            VERIFY_IS_NOT_NULL(suggestionsList);
            auto suggestionsListItemsSource = safe_cast<::Windows::Foundation::Collections::IVector<Platform::Object^>^>(suggestionsList->Items);
            VERIFY_IS_NOT_NULL(suggestionsListItemsSource);
            firstSuggestion = safe_cast<xaml_controls::ListViewItem^>(suggestionsList->ContainerFromItem(suggestionsListItemsSource->GetAt(0)));
        });

        VERIFY_IS_NOT_NULL(firstSuggestion);
        test_infra::TestServices::InputHelper->Tap(firstSuggestion);
        querySubmittedEvent->WaitForDefault();
        popupClosedEvent->WaitForDefault();
        textChangedEvent->WaitForDefault();

        TestServices::WindowHelper->WaitForIdle();
    }

    Platform::String^ CreateShortString(const wchar_t* format, ...)
    {
        va_list vargs;
        va_start(vargs, format);

        wchar_t buffer[100];
        LogThrow_IfFailed(::StringCchVPrintf(buffer, ARRAYSIZE(buffer), format, vargs));

        return ref new Platform::String(buffer);
    }

    void AutoSuggestBoxIntegrationTests::ValidateCollectionUpdates()
    {
        TestCleanupWrapper cleanup;

        Platform::Collections::Vector<Platform::String^>^ itemList = ref new Platform::Collections::Vector<Platform::String^>();

        auto rootPanel = SetupAutoSuggestBoxTest(xaml::VerticalAlignment::Bottom, itemList);
        xaml_controls::AutoSuggestBox^ asb;

        RunOnUIThread([&]()
        {
            asb = safe_cast<xaml_controls::AutoSuggestBox^>(rootPanel->FindName(L"autoSuggestBox"));

            for (int i = 0; i < 100; ++i)
            {
                itemList->Append(CreateShortString(L"item %d", i));
            }
            VERIFY_ARE_EQUAL(asb->Items->Size, 100u);
        });


        TestServices::WindowHelper->WaitForIdle();

        // Remove first 50 elements
        for (int i = 0; i < 10; ++i)
        {
            RunOnUIThread([&]()
            {
                for (int j = 0; j < 5; ++j)
                {
                    itemList->RemoveAt(0);
                }
            });

            TestServices::WindowHelper->WaitForIdle();
        }

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(asb->Items->Size, 50u);
        });

        RunOnUIThread([&]()
        {
            // Remove last 49 elements
            for (int i = 0; i < 49; ++i)
            {
                itemList->RemoveAtEnd();
            }
            VERIFY_ARE_EQUAL(asb->Items->Size, 1u);
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            for (int i = 0; i < 100; ++i)
            {
                itemList->Append(CreateShortString(L"new item %d", i));
            }
            VERIFY_ARE_EQUAL(asb->Items->Size, 101u);
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // Remove every other
            for (int i = 0; i < 50; i++)
            {
                itemList->RemoveAt(i);
            }
            VERIFY_ARE_EQUAL(asb->Items->Size, 51u);
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // Remove every other
            while (itemList->Size > 0)
            {
                itemList->RemoveAt(0);
            }
            VERIFY_ARE_EQUAL(asb->Items->Size, 0u);
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void AutoSuggestBoxIntegrationTests::ValidateSuggestionListChangeInTextChangedHandler()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::AutoSuggestBox^ asb;

        Platform::Collections::Vector<Platform::String^>^ itemList = ref new Platform::Collections::Vector<Platform::String^>();

        for (int i = 0; i < 200; ++i)
        {
            itemList->Append(CreateShortString(L"item %d", i));
        }

        auto rootPanel = SetupAutoSuggestBoxTest(xaml::VerticalAlignment::Bottom, itemList);

        auto textChangedEvent = std::make_shared<Event>();
        auto textChangedRegistration = CreateSafeEventRegistration(xaml_controls::AutoSuggestBox, TextChanged);

        RunOnUIThread([&]()
        {
            asb = safe_cast<xaml_controls::AutoSuggestBox^>(rootPanel->FindName(L"autoSuggestBox"));

            textChangedRegistration.Attach(
                asb,
                ref new wf::TypedEventHandler<xaml_controls::AutoSuggestBox^, xaml_controls::AutoSuggestBoxTextChangedEventArgs^>(
                [&](Platform::Object^, xaml_controls::AutoSuggestBoxTextChangedEventArgs^ args)
            {
                while (itemList->Size > 0)
                {
                    // We shouldn't get an exception here
                    itemList->RemoveAt(0);
                }
                textChangedEvent->Set();
            }));
        });


        TestServices::InputHelper->Tap(asb);
        TestServices::WindowHelper->WaitForIdle();

        // Type a key to trigger TextChanged callback
        TestServices::KeyboardHelper->PressKeySequence(L"W");
        textChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(asb->Items->Size, 0u);
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void AutoSuggestBoxIntegrationTests::ValidateUpdateItemsSource()
    {
        TestCleanupWrapper cleanup;

        // We show the suggestions list and while it is visible we update ItemsSource
        // We validate that the open suggestion list gets updated with the new ItemsSource
        // We also validate that tapping on an item in the suggestion list causes the suggestion list to
        // close and to stay closed.

        unsigned int numItemsBeforeChange = 5;
        unsigned int numItemsAfterChange = 3;

        Platform::Collections::Vector<Platform::String^>^ itemList = ref new Platform::Collections::Vector<Platform::String^>();
        for (unsigned int i = 0; i < numItemsBeforeChange; ++i)
        {
            itemList->Append(CreateShortString(L"item %d", i));
        }

        auto rootPanel = SetupAutoSuggestBoxTest(xaml::VerticalAlignment::Top, itemList);
        TestServices::WindowHelper->WaitForIdle();

        xaml_controls::AutoSuggestBox^ autoSuggestBox;
        xaml_primitives::Popup^ popup;
        xaml_controls::ListView^ suggestionsList;
        auto popupOpenedEvent = std::make_shared<Event>();
        auto popupOpenedRegistration = CreateSafeEventRegistration(xaml_primitives::Popup, Opened);
        auto popupClosedEvent = std::make_shared<Event>();
        auto popupClosedRegistration = CreateSafeEventRegistration(xaml_primitives::Popup, Closed);

        RunOnUIThread([&]()
        {
            autoSuggestBox = safe_cast<xaml_controls::AutoSuggestBox^>(rootPanel->FindName(L"autoSuggestBox"));
            popup = safe_cast<xaml_primitives::Popup^>(TreeHelper::GetVisualChildByName(autoSuggestBox, L"SuggestionsPopup"));
            suggestionsList = safe_cast<xaml_controls::ListView^>(TreeHelper::GetVisualChildByName(
                safe_cast<FrameworkElement^>(popup->Child), L"SuggestionsList"));

            popupOpenedRegistration.Attach(popup, ref new wf::EventHandler<Platform::Object^>([popupOpenedEvent](Platform::Object^, Platform::Object^){
                popupOpenedEvent->Set();
            }));

            popupClosedRegistration.Attach(popup,ref new wf::EventHandler<Platform::Object^>([popupClosedEvent](Platform::Object^, Platform::Object^){
                popupClosedEvent->Set();
            }));
        });

        // Tap on the ASB to focus it, and type in the TextBox to bring up the suggestion list.
        TestServices::InputHelper->Tap(autoSuggestBox);
        TestServices::WindowHelper->WaitForIdle();
        TestServices::KeyboardHelper->PressKeySequence(L"a");

        // Wait for the suggestion list to appear.
        popupOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        // We expect 5 suggestions
        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(suggestionsList->Items->Size, numItemsBeforeChange);
        });

        // Update ItemSource.
        RunOnUIThread([&]()
        {
            auto items = ref new Platform::Collections::Vector<Platform::String^>();
            for (unsigned int i = 0; i < numItemsAfterChange; ++i)
            {
                items->Append(CreateShortString(L"item %d", i));
            }
            autoSuggestBox->ItemsSource = items;
        });
        TestServices::WindowHelper->WaitForIdle();

        // After updating this item source, we expect the suggestionList to get updated.
        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(suggestionsList->Items->Size, numItemsAfterChange);
        });
        VERIFY_IS_FALSE(popupClosedEvent->HasFired()); // The Popup should not have closed at any point.
        popupOpenedEvent->Reset();

        TestServices::InputHelper->Tap(suggestionsList);
        popupClosedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        // The suggestion list should not re-appear after selecting an item.
        VERIFY_IS_FALSE(popupOpenedEvent->HasFired());
    }

    void AutoSuggestBoxIntegrationTests::ScrollWheelScrollsSuggestions()
    {
        TestCleanupWrapper cleanup;
        ScrollWheelScrollsSuggestionsWorker(xaml::VerticalAlignment::Top);
        ScrollWheelScrollsSuggestionsWorker(xaml::VerticalAlignment::Bottom);
    }

    void AutoSuggestBoxIntegrationTests::ScrollWheelScrollsSuggestionsWorker(xaml::VerticalAlignment alignment)
    {
        xaml_controls::AutoSuggestBox^ asb;
        xaml_controls::ListView^ suggestions;

        Platform::Collections::Vector<Platform::String^>^ itemList = ref new Platform::Collections::Vector<Platform::String^>();

        for (int i = 0; i < 200; ++i)
        {
            itemList->Append(CreateShortString(L"item %d", i));
        }

        auto rootPanel = SetupAutoSuggestBoxTest(alignment, itemList);

        auto textChangedEvent = std::make_shared<Event>();
        auto textChangedRegistration = CreateSafeEventRegistration(xaml_controls::AutoSuggestBox, TextChanged);

        RunOnUIThread([&]()
        {
            asb = safe_cast<xaml_controls::AutoSuggestBox^>(rootPanel->FindName(L"autoSuggestBox"));

            textChangedRegistration.Attach(
                asb,
                ref new wf::TypedEventHandler<xaml_controls::AutoSuggestBox^, xaml_controls::AutoSuggestBoxTextChangedEventArgs^>(
                [&](Platform::Object^, xaml_controls::AutoSuggestBoxTextChangedEventArgs^ args)
            {
                textChangedEvent->Set();
            }));
        });
        TestServices::WindowHelper->WaitForIdle();

        // Type a key to trigger TextChanged callback
        TestServices::KeyboardHelper->PressKeySequence(L"asb scroll wheel");
        textChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            suggestions = safe_cast<xaml_controls::ListView^>(TreeHelper::GetVisualChildByNameFromOpenPopups(L"SuggestionsList", asb));
            VERIFY_IS_NOT_NULL(suggestions);
        });

        TestServices::InputHelper->ScrollMouseWheel(suggestions, alignment == xaml::VerticalAlignment::Top ? -100 : +100);
        TestServices::WindowHelper->WaitForIdle();
        TestServices::InputHelper->Tap(suggestions, 0.5, 0.5);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            const wchar_t* str = asb->Text->Data();
            int itemNumber = ::_wtoi(str + strlen("item "));

            // The scroll wheel should take us past the 50th item
            VERIFY_IS_GREATER_THAN(itemNumber, 50);
        });

        TestServices::WindowHelper->WaitForIdle();
    }

    void AutoSuggestBoxIntegrationTests::ValidateUIElementTree()
    {
        DisableErrorReportingScopeGuard disableErrors;

        // Hide the textbox caret so it doesn't interfere with UIElement tree comparison
        RuntimeEnabledFeatureOverride disableTextBoxCaret(RuntimeFeatureBehavior::RuntimeEnabledFeature::DisableTextBoxCaret, true);

        ControlHelper::ValidateUIElementTree(
            PopupHelper::AreWindowedPopupsEnabled() ? L"Windowed" : L"Unwindowed",
            wf::Size(400, 600),
            1.f,
            [] () {
                xaml_controls::StackPanel^ rootPanel = nullptr;
                xaml_controls::AutoSuggestBox^ autoSuggestBoxNormal = nullptr;
                xaml_controls::TextBox^ autoSuggestBoxNormalTextBox = nullptr;
                xaml_controls::AutoSuggestBox^ autoSuggestBoxPointerOver = nullptr;
                xaml_controls::TextBox^ autoSuggestBoxPointerOverTextBox = nullptr;
                xaml_controls::AutoSuggestBox^ autoSuggestBoxPressed = nullptr;
                xaml_controls::TextBox^ autoSuggestBoxPressedTextBox = nullptr;
                xaml_primitives::ButtonBase^ queryButtonPointerOver = nullptr;
                xaml_primitives::ButtonBase^ queryButtonPressed = nullptr;
                xaml_primitives::ButtonBase^ deleteButtonPointerOver = nullptr;
                xaml_primitives::ButtonBase^ deleteButtonPressed = nullptr;

                // Have enough suggestions so that the SuggestionsList Maximum Height is reached.
                Platform::Collections::Vector<Platform::String^>^ itemList = ref new Platform::Collections::Vector<Platform::String^>();
                itemList->Append("January");
                itemList->Append("February");
                itemList->Append("March");
                itemList->Append("April");
                itemList->Append("May");
                itemList->Append("June");
                itemList->Append("July");
                itemList->Append("August");
                itemList->Append("September");
                itemList->Append("October");
                itemList->Append("November");
                itemList->Append("December");

                //Creates a stackpanel with the required AutoSuggestBox controls for the test.
                rootPanel = SetupAutoSuggestBoxForUIValidation(xaml::VerticalAlignment::Top, itemList);

                //Gets AutosuggestBox query and delete buttons for further state changes and enables the buttons for the AutoSuggestBox controls
                RunOnUIThread([&]()
                {
                    autoSuggestBoxNormal = safe_cast<Microsoft::UI::Xaml::Controls::AutoSuggestBox^>(rootPanel->FindName(L"autoSuggestBoxNormal"));
                    autoSuggestBoxPointerOver = safe_cast<Microsoft::UI::Xaml::Controls::AutoSuggestBox^>(rootPanel->FindName(L"autoSuggestBoxPointerOver"));
                    autoSuggestBoxPressed = safe_cast<Microsoft::UI::Xaml::Controls::AutoSuggestBox^>(rootPanel->FindName(L"autoSuggestBoxPressed"));
                    autoSuggestBoxNormalTextBox = safe_cast<Microsoft::UI::Xaml::Controls::TextBox^>(TreeHelper::GetVisualChildByName(autoSuggestBoxNormal, "TextBox"));
                    autoSuggestBoxPointerOverTextBox = safe_cast<Microsoft::UI::Xaml::Controls::TextBox^>(TreeHelper::GetVisualChildByName(autoSuggestBoxPointerOver, "TextBox"));
                    autoSuggestBoxPressedTextBox = safe_cast<Microsoft::UI::Xaml::Controls::TextBox^>(TreeHelper::GetVisualChildByName(autoSuggestBoxPressed, "TextBox"));
                    queryButtonPointerOver = safe_cast<xaml_primitives::ButtonBase^>(TreeHelper::GetVisualChildByName(autoSuggestBoxPointerOver, "QueryButton"));
                    queryButtonPressed = safe_cast<xaml_primitives::ButtonBase^>(TreeHelper::GetVisualChildByName(autoSuggestBoxPressed, "QueryButton"));
                    deleteButtonPointerOver = safe_cast<xaml_primitives::ButtonBase^>(TreeHelper::GetVisualChildByName(autoSuggestBoxPointerOver, "DeleteButton"));
                    deleteButtonPressed = safe_cast<xaml_primitives::ButtonBase^>(TreeHelper::GetVisualChildByName(autoSuggestBoxPressed, "DeleteButton"));

                    VisualStateManager::GoToState(autoSuggestBoxNormalTextBox, "Focused", false);
                    VisualStateManager::GoToState(autoSuggestBoxPointerOverTextBox, "Focused", false);
                    VisualStateManager::GoToState(autoSuggestBoxNormalTextBox, "ButtonVisible", false);
                    VisualStateManager::GoToState(autoSuggestBoxPointerOverTextBox, "ButtonVisible", false);
                });
                TestServices::WindowHelper->WaitForIdle();

                auto textChangedEvent = std::make_shared<Event>();
                auto textChangedRegistration = CreateSafeEventRegistration(xaml_controls::AutoSuggestBox, TextChanged);

                textChangedRegistration.Attach(
                    autoSuggestBoxPressed,
                    ref new wf::TypedEventHandler<xaml_controls::AutoSuggestBox^, xaml_controls::AutoSuggestBoxTextChangedEventArgs^>(
                        [&](Platform::Object^, xaml_controls::AutoSuggestBoxTextChangedEventArgs^ args)
                {
                    textChangedEvent->Set();
                }));

                // Type "S" to show the suggestion list
                TestServices::KeyboardHelper->PressKeySequence(L"S");
                textChangedEvent->WaitForDefault();

                //Set states for UI validation,
                RunOnUIThread([&]()
                {
                    auto suggestionsList = safe_cast<xaml_controls::ListView^>(TreeHelper::GetVisualChildByNameFromOpenPopups(L"SuggestionsList", rootPanel));
                    VERIFY_IS_NOT_NULL(suggestionsList);
                    auto suggestionsListItemsSource = safe_cast<::Windows::Foundation::Collections::IVector<Platform::Object^>^>(suggestionsList->Items);
                    VERIFY_IS_NOT_NULL(suggestionsListItemsSource);
                    auto firstSuggestion = static_cast<xaml_controls::ListViewItem^>(suggestionsList->ContainerFromItem(suggestionsListItemsSource->GetAt(0)));
                    VERIFY_IS_NOT_NULL(firstSuggestion);
                    auto secondSuggestion = static_cast<xaml_controls::ListViewItem^>(suggestionsList->ContainerFromItem(suggestionsListItemsSource->GetAt(1)));
                    VERIFY_IS_NOT_NULL(secondSuggestion);

                    VisualStateManager::GoToState(firstSuggestion, "PointerOver", false);
                    VisualStateManager::GoToState(secondSuggestion, "Pressed", false);
                    VisualStateManager::GoToState(queryButtonPointerOver, "PointerOver", false);
                    VisualStateManager::GoToState(deleteButtonPointerOver, "PointerOver", false);
                    VisualStateManager::GoToState(queryButtonPressed, "Pressed", false);
                    VisualStateManager::GoToState(deleteButtonPressed, "Pressed", false);
                });
                TestServices::WindowHelper->WaitForIdle();

                return rootPanel;
            }
        );
    }

    void AutoSuggestBoxIntegrationTests::ValidateSuggestionListNavigationUsingGamepad()
    {
        PerformValidateSuggestionListNavigation(xaml::VerticalAlignment::Top, InputDevice::Gamepad, /* goDownFirst */ true);
        PerformValidateSuggestionListNavigation(xaml::VerticalAlignment::Top, InputDevice::Gamepad, /* goDownFirst */ false);
        PerformValidateSuggestionListNavigation(xaml::VerticalAlignment::Bottom, InputDevice::Gamepad, /* goDownFirst */ true);
        PerformValidateSuggestionListNavigation(xaml::VerticalAlignment::Bottom, InputDevice::Gamepad, /* goDownFirst */ false);
    }

    void AutoSuggestBoxIntegrationTests::ValidateSuggestionListNavigationUsingKeyboard()
    {
        PerformValidateSuggestionListNavigation(xaml::VerticalAlignment::Top, InputDevice::Keyboard, /* goDownFirst */ true);
        PerformValidateSuggestionListNavigation(xaml::VerticalAlignment::Top, InputDevice::Keyboard, /* goDownFirst */ false);
        PerformValidateSuggestionListNavigation(xaml::VerticalAlignment::Bottom, InputDevice::Keyboard, /* goDownFirst */ true);
        PerformValidateSuggestionListNavigation(xaml::VerticalAlignment::Bottom, InputDevice::Keyboard, /* goDownFirst */ false);
    }

    void AutoSuggestBoxIntegrationTests::PerformValidateSuggestionListNavigation(xaml::VerticalAlignment verticalAlign, InputDevice inputDevice, bool goDownFirst)
    {
        TestCleanupWrapper cleanup;
        DisableErrorReportingScopeGuard disableErrors;

        auto asbGotFocusEvent = std::make_shared<Event>();
        auto textChangedEvent = std::make_shared<Event>();
        auto suggestionChosenEvent = std::make_shared<Event>();
        auto popupOpenedEvent = std::make_shared<Event>();
        auto popupClosedEvent = std::make_shared<Event>();
        auto button1GotFocusEvent = std::make_shared<Event>();
        auto asbGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::AutoSuggestBox, GotFocus);
        auto textChangedRegistration = CreateSafeEventRegistration(xaml_controls::AutoSuggestBox, TextChanged);
        auto suggestionChosenRegistration = CreateSafeEventRegistration(xaml_controls::AutoSuggestBox, SuggestionChosen);
        auto popupOpenedRegistration = CreateSafeEventRegistration(xaml_primitives::Popup, Opened);
        auto popupClosedRegistration = CreateSafeEventRegistration(xaml_primitives::Popup, Closed);
        auto button1GotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

        auto suggestionList = GetStandardSuggestionList();

        auto rootPanel = SetupAutoSuggestBoxTestWithEvents(verticalAlign, suggestionList, asbGotFocusEvent, asbGotFocusRegistration,
            textChangedEvent, textChangedRegistration, suggestionChosenEvent, suggestionChosenRegistration, popupOpenedEvent, popupOpenedRegistration, popupClosedEvent, popupClosedRegistration);
        xaml_controls::AutoSuggestBox^ autoSuggestBox = nullptr;
        xaml_controls::Button^ button1 = nullptr;

        RunOnUIThread([&]()
        {
            autoSuggestBox = safe_cast<xaml_controls::AutoSuggestBox^>(rootPanel->FindName(L"autoSuggestBox"));
            button1 = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button1"));

            button1GotFocusRegistration.Attach(button1, [&] { button1GotFocusEvent->Set(); });
        });
        TestServices::WindowHelper->WaitForIdle();

        // Type "R" to show the suggestion list
        Platform::String^ keySequence = ref new Platform::String(L"R");
        TestServices::KeyboardHelper->PressKeySequence(keySequence);
        textChangedEvent->WaitForDefault();
        popupOpenedEvent->WaitForDefault();

        textChangedEvent->Reset();

        // This bool indicates whether we are using Gamepad(where you can NOT loop through suggestions) AND the first key you have pressed does
        // not take you to the first suggestion (example: SuggestionList opens up and you pressed the down key first) but rather, you stay on the ASB TextBox.
        // Note that verticalAlign is the VerticalAlignment of the rootPanel so if it is Bottom, the SuggestionList opens UP.
        bool firstSuggestionEntered = (inputDevice == InputDevice::Gamepad) &&
            ((verticalAlign == xaml::VerticalAlignment::Top) ^ goDownFirst);

        NextInput(inputDevice, goDownFirst);
        if (!firstSuggestionEntered)
        {
            textChangedEvent->WaitForDefault();
        }
        NextInput(inputDevice, !goDownFirst);

        TestServices::KeyboardHelper->PressKeySequence(L"$d$_enter#$u$_enter");
        suggestionChosenEvent->WaitForDefault();
        textChangedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"VerifyKeyboardNavigation: The auto suggested text = %s", autoSuggestBox->Text->Data());
            VERIFY_ARE_EQUAL(autoSuggestBox->Text, firstSuggestionEntered ? suggestionList->GetAt(0) : keySequence);
        });

        switch (inputDevice)
        {
        case InputDevice::Keyboard:
            // For Keyboard, textChangedEvent is fired twice because we have looping and each Up/Down results in text getting changed.
            VERIFY_ARE_EQUAL(textChangedEvent->TimesFired(), 2);
            break;
        case InputDevice::Gamepad:
            // For Gamepad, firstSuggestionEntered=true means that the first Up/Down was blocked because there is no looping
            // and the second Up/Down results in text getting changed.
            VERIFY_ARE_EQUAL(textChangedEvent->TimesFired(), firstSuggestionEntered ? 1 : 2);
            break;
        }
    }

    void AutoSuggestBoxIntegrationTests::CanCloseSuggestionListUsingGamepad()
    {
        PerformCanCloseSuggestionList(InputDevice::Gamepad);
    }

    void AutoSuggestBoxIntegrationTests::CanCloseSuggestionListUsingKeyboard()
    {
        PerformCanCloseSuggestionList(InputDevice::Keyboard);
    }

    void AutoSuggestBoxIntegrationTests::PerformCanCloseSuggestionList(InputDevice inputDevice)
    {
        TestCleanupWrapper cleanup;

        auto gotFocusEvent = std::make_shared<Event>();
        auto textChangedEvent = std::make_shared<Event>();
        auto suggestionChosenEvent = std::make_shared<Event>();
        auto popupOpenedEvent = std::make_shared<Event>();
        auto popupClosedEvent = std::make_shared<Event>();
        auto gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::AutoSuggestBox, GotFocus);
        auto textChangedRegistration = CreateSafeEventRegistration(xaml_controls::AutoSuggestBox, TextChanged);
        auto suggestionChosenRegistration = CreateSafeEventRegistration(xaml_controls::AutoSuggestBox, SuggestionChosen);
        auto popupOpenedRegistration = CreateSafeEventRegistration(xaml_primitives::Popup, Opened);
        auto popupClosedRegistration = CreateSafeEventRegistration(xaml_primitives::Popup, Closed);

        auto suggestionList = GetStandardSuggestionList();

        auto rootPanel = SetupAutoSuggestBoxTestWithEvents(xaml::VerticalAlignment::Top, suggestionList, gotFocusEvent, gotFocusRegistration,
            textChangedEvent, textChangedRegistration, suggestionChosenEvent, suggestionChosenRegistration, popupOpenedEvent, popupOpenedRegistration, popupClosedEvent, popupClosedRegistration);
        xaml_controls::TextBox^ textBox = nullptr;

        RunOnUIThread([&]()
        {
            auto autoSuggestBox = safe_cast<xaml_controls::AutoSuggestBox^>(rootPanel->FindName(L"autoSuggestBox"));
            textBox = safe_cast<xaml_controls::TextBox^>(TreeHelper::GetVisualChildByName(autoSuggestBox, L"TextBox"));
        });
        TestServices::WindowHelper->WaitForIdle();

        // Type "R" to show the suggestion list and then "Cancel" to close it.
        Platform::String^ keySequence = ref new Platform::String(L"R");
        TestServices::KeyboardHelper->PressKeySequence(keySequence);
        textChangedEvent->WaitForDefault();
        popupOpenedEvent->WaitForDefault();

        CommonInputHelper::Cancel(inputDevice);
        // Here we wait for popupClosedEvent for all types of inputDevices because the keySequence is not entered through a SIP,
        // even when Gamepad is used, so "Cancel" closes the popup instead of dismissing the SIP because it does not exist.
        popupClosedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            // Verify that the textBox has focus, its text from pressing the keySequence
            // has not changed and the caret is at the end of the text.
            VERIFY_ARE_EQUAL((unsigned int)textBox->SelectionStart, keySequence->Length());
            VERIFY_ARE_EQUAL(textBox->SelectionLength, 0);
            VERIFY_ARE_EQUAL(textBox->Text, keySequence);
            VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(textBox));
        });

        // Type "e" to show the suggestion list, select a suggestion using "Down"
        // and then "Cancel" to close the suggestion list.
        TestServices::KeyboardHelper->PressKeySequence(L"e");
        keySequence += L"e";
        textChangedEvent->WaitForDefault();
        popupOpenedEvent->WaitForDefault();

        CommonInputHelper::Down(inputDevice);
        textChangedEvent->WaitForDefault();

        CommonInputHelper::Cancel(inputDevice);
        popupClosedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            // Verify that the textBox has focus, its text from from pressing the keySequence
            // has been set again and the caret is at the end of the text.
            Platform::String^ text = suggestionList->GetAt(0);
            VERIFY_ARE_EQUAL((unsigned int)textBox->SelectionStart, keySequence->Length());
            VERIFY_ARE_EQUAL(textBox->SelectionLength, 0);
            VERIFY_ARE_EQUAL(textBox->Text, keySequence);
            VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(textBox));
        });
    }

    void AutoSuggestBoxIntegrationTests::CanRaiseQuerySubmittedUsingGamepad()
    {
        // For Gamepad, we don't have goToFirstSuggestion=false case since the only way to do a QuerySubmitted on the original text typed is to press "Enter" from the SIP,
        // which gets registered as a Keyboard "Enter" and we have a test case for it already.
        PerformCanRaiseQuerySubmitted(InputMethod::Gamepad, /* goToFirstSuggestion */ true);
    }

    void AutoSuggestBoxIntegrationTests::CanRaiseQuerySubmittedUsingKeyboard()
    {
        PerformCanRaiseQuerySubmitted(InputMethod::Keyboard, /* goToFirstSuggestion */ false);
        PerformCanRaiseQuerySubmitted(InputMethod::Keyboard, /* goToFirstSuggestion */ true);
    }

    void AutoSuggestBoxIntegrationTests::CanRaiseQuerySubmittedUsingMouse()
    {
        PerformCanRaiseQuerySubmitted(InputMethod::Mouse, /* goToFirstSuggestion */ false);
        PerformCanRaiseQuerySubmitted(InputMethod::Mouse, /* goToFirstSuggestion */ true);
    }

    void AutoSuggestBoxIntegrationTests::PerformCanRaiseQuerySubmitted(InputMethod inputMethod, bool goToFirstSuggestion)
    {
        TestCleanupWrapper cleanup;

        auto autoSuggestBox = SetupAutoSuggestBoxWithQueryIcon();
        auto querySubmittedEvent = std::make_shared<Event>();
        auto querySubmittedRegistration = CreateSafeEventRegistration(xaml_controls::AutoSuggestBox, QuerySubmitted);

        RunOnUIThread([&]()
        {
            autoSuggestBox->ItemsSource = GetStandardSuggestionList();
            querySubmittedRegistration.Attach(autoSuggestBox, [&]{ querySubmittedEvent->Set(); });
        });
        TestServices::WindowHelper->WaitForIdle();

        // Type "R" to show the suggestion list
        TestServices::KeyboardHelper->PressKeySequence(L"R");
        TestServices::WindowHelper->WaitForIdle();

        if (inputMethod == InputMethod::Mouse)
        {
            if (goToFirstSuggestion)
            {
                TestServices::KeyboardHelper->Down();
                TestServices::WindowHelper->WaitForIdle();
            }

            xaml_primitives::ButtonBase^ queryButton = nullptr;
            RunOnUIThread([&]()
            {
                queryButton = safe_cast<xaml_primitives::ButtonBase^>(TreeHelper::GetVisualChildByName(autoSuggestBox, "QueryButton"));
                VERIFY_IS_NOT_NULL(queryButton);
            });
            ControlHelper::DoClickUsingTap<xaml_primitives::ButtonBase>(queryButton);
        }
        else
        {
            InputDevice inputDevice = InputDevice::Keyboard;
            switch (inputMethod)
            {
            case InputMethod::Gamepad:
                inputDevice = InputDevice::Gamepad;
                break;
            case InputMethod::Keyboard:
                inputDevice = InputDevice::Keyboard;
                break;
            }

            if (goToFirstSuggestion)
            {
                CommonInputHelper::Down(inputDevice);
                TestServices::WindowHelper->WaitForIdle();
            }

            if (inputMethod == InputMethod::Keyboard)
            {
                TestServices::KeyboardHelper->Enter();
            }
            else
            {
                CommonInputHelper::Accept(inputDevice);
            }
            TestServices::WindowHelper->WaitForIdle();
        }

        querySubmittedEvent->WaitForDefault();
    }

    void AutoSuggestBoxIntegrationTests::ValidateTraverseSuggestionListUsingGamepad()
    {
        PerformValidateTraverseSuggestionList(xaml::VerticalAlignment::Top, InputDevice::Gamepad, /* goDownFirst */ true);
        PerformValidateTraverseSuggestionList(xaml::VerticalAlignment::Top, InputDevice::Gamepad, /* goDownFirst */ false);
        PerformValidateTraverseSuggestionList(xaml::VerticalAlignment::Bottom, InputDevice::Gamepad, /* goDownFirst */ true);
        PerformValidateTraverseSuggestionList(xaml::VerticalAlignment::Bottom, InputDevice::Gamepad, /* goDownFirst */ false);
    }

    void AutoSuggestBoxIntegrationTests::ValidateTraverseSuggestionListUsingKeyboard()
    {
        PerformValidateTraverseSuggestionList(xaml::VerticalAlignment::Top, InputDevice::Keyboard, /* goDownFirst */ true);
        PerformValidateTraverseSuggestionList(xaml::VerticalAlignment::Top, InputDevice::Keyboard, /* goDownFirst */ false);
        PerformValidateTraverseSuggestionList(xaml::VerticalAlignment::Bottom, InputDevice::Keyboard, /* goDownFirst */ true);
        PerformValidateTraverseSuggestionList(xaml::VerticalAlignment::Bottom, InputDevice::Keyboard, /* goDownFirst */ false);
    }

    void AutoSuggestBoxIntegrationTests::PerformValidateTraverseSuggestionList(xaml::VerticalAlignment verticalAlign, InputDevice inputDevice, bool goDownFirst)
    {
        TestCleanupWrapper cleanup;

        auto gotFocusEvent = std::make_shared<Event>();
        auto textChangedEvent = std::make_shared<Event>();
        auto suggestionChosenEvent = std::make_shared<Event>();
        auto popupOpenedEvent = std::make_shared<Event>();
        auto popupClosedEvent = std::make_shared<Event>();
        auto gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::AutoSuggestBox, GotFocus);
        auto textChangedRegistration = CreateSafeEventRegistration(xaml_controls::AutoSuggestBox, TextChanged);
        auto suggestionChosenRegistration = CreateSafeEventRegistration(xaml_controls::AutoSuggestBox, SuggestionChosen);
        auto popupOpenedRegistration = CreateSafeEventRegistration(xaml_primitives::Popup, Opened);
        auto popupClosedRegistration = CreateSafeEventRegistration(xaml_primitives::Popup, Closed);

        auto suggestionList = GetStandardSuggestionList();

        auto rootPanel = SetupAutoSuggestBoxTestWithEvents(verticalAlign, suggestionList, gotFocusEvent, gotFocusRegistration,
            textChangedEvent, textChangedRegistration, suggestionChosenEvent, suggestionChosenRegistration, popupOpenedEvent, popupOpenedRegistration, popupClosedEvent, popupClosedRegistration);
        xaml_controls::AutoSuggestBox^ autoSuggestBox = nullptr;

        RunOnUIThread([&]()
        {
            autoSuggestBox = safe_cast<xaml_controls::AutoSuggestBox^>(rootPanel->FindName(L"autoSuggestBox"));
        });
        TestServices::WindowHelper->WaitForIdle();

        // Type "R" to show the suggestion list.
        TestServices::KeyboardHelper->PressKeySequence(L"R");
        textChangedEvent->WaitForDefault();
        popupOpenedEvent->WaitForDefault();

        textChangedEvent->Reset();

        // This bool indicates whether we are using Gamepad(where you can NOT loop through suggestions) AND the first key you have pressed does
        // not take you to the first suggestion (example: SuggestionList opens up and you pressed the down key first) but rather, you stay on the ASB TextBox.
        // Note that verticalAlign is the VerticalAlignment of the rootPanel so if it is Bottom, the SuggestionList opens UP.
        bool firstSuggestionEntered = (inputDevice == InputDevice::Gamepad) &&
            ((verticalAlign == xaml::VerticalAlignment::Top) ^ goDownFirst);

        // Go in one direction, for the length of the list, and wait for textChangedEvent unless the inputDevice is Gamepad and
        // we are moving in the direction opposite to the flow of the SuggestionList.
        for (unsigned int i = 0; i < suggestionList->Size; i++)
        {
            NextInput(inputDevice, goDownFirst);
            if (!firstSuggestionEntered)
            {
                textChangedEvent->WaitForDefault();
            }
        }

        // Go in the same direction once more and wait for textChangedEvent only when we have looping (inputDevice is Keyboard).
        NextInput(inputDevice, goDownFirst);
        if (inputDevice == InputDevice::Keyboard)
        {
            textChangedEvent->WaitForDefault();
        }

        // Go in the opposite direction, for the length of the list, and wait for textChangedEvent.
        for (unsigned int i = 0; i < suggestionList->Size; i++)
        {
            NextInput(inputDevice, !goDownFirst);
            textChangedEvent->WaitForDefault();
        }

        // Go in the opposite direction once more and wait for textChangedEvent only when we have looping (inputDevice is Keyboard).
        NextInput(inputDevice, !goDownFirst);
        if (inputDevice == InputDevice::Keyboard)
        {
            textChangedEvent->WaitForDefault();
        }

        switch (inputDevice)
        {
        case InputDevice::Keyboard:
            // For Keyboard, textChangedEvent is fired as many times as we go Up/Down.
            VERIFY_ARE_EQUAL(textChangedEvent->TimesFired(), 2 * ((int)suggestionList->Size + 1));
            break;
        case InputDevice::Gamepad:
            VERIFY_ARE_EQUAL(textChangedEvent->TimesFired(), firstSuggestionEntered ? (int)suggestionList->Size : (2 * (int)suggestionList->Size));
            break;
        }

        TestServices::KeyboardHelper->Enter();

        popupClosedEvent->WaitForDefault();

        TestServices::WindowHelper->WaitForIdle();
    }

    void AutoSuggestBoxIntegrationTests::CanMoveAwayUsingGamepadWhenSuggestionListIsClosed()
    {
        PerformMoveAwayFromAutoSuggestBox(InputDevice::Gamepad, /* moveHorizontally */ false, /* suggestionListOpen */ false, /* goToFirstSuggestion */ false);
        PerformMoveAwayFromAutoSuggestBox(InputDevice::Gamepad, /* moveHorizontally */ true, /* suggestionListOpen */ false, /* goToFirstSuggestion */ false);
    }

    void AutoSuggestBoxIntegrationTests::CanMoveAwayUsingKeyboardWhenSuggestionListIsClosed()
    {
        PerformMoveAwayFromAutoSuggestBox(InputDevice::Keyboard, /* moveHorizontally */ false, /* suggestionListOpen */ false, /* goToFirstSuggestion */ false);
        // Keyboard does not have moveHorizontally cases.
    }

    void AutoSuggestBoxIntegrationTests::CanNotMoveAwayUsingGamepadWhenSuggestionListIsOpen()
    {
        PerformMoveAwayFromAutoSuggestBox(InputDevice::Gamepad, /* moveHorizontally */ false, /* suggestionListOpen */ true, /* goToFirstSuggestion */ false);
        PerformMoveAwayFromAutoSuggestBox(InputDevice::Gamepad, /* moveHorizontally */ true, /* suggestionListOpen */ true, /* goToFirstSuggestion */ false);
        PerformMoveAwayFromAutoSuggestBox(InputDevice::Gamepad, /* moveHorizontally */ false, /* suggestionListOpen */ true, /* goToFirstSuggestion */ true);
        PerformMoveAwayFromAutoSuggestBox(InputDevice::Gamepad, /* moveHorizontally */ true, /* suggestionListOpen */ true, /* goToFirstSuggestion */ true);
    }

    void AutoSuggestBoxIntegrationTests::PerformMoveAwayFromAutoSuggestBox(InputDevice inputDevice, bool moveHorizontally, bool suggestionListOpen, bool goToFirstSuggestion)
    {
        TestCleanupWrapper cleanup;

        Platform::Collections::Vector<Platform::String^>^ itemList = ref new Platform::Collections::Vector<Platform::String^>();
        itemList->Append("Single Suggestion");
        xaml_controls::Grid^ rootPanel = nullptr;
        xaml_controls::AutoSuggestBox^ asb = nullptr;
        xaml_controls::Button^ button1 = nullptr;

        Platform::String^ expectedFocusSequence = suggestionListOpen ?
            (!moveHorizontally && goToFirstSuggestion && inputDevice != InputDevice::Keyboard ? L"[TextBox][TextBox]" : L"[TextBox]") :
            (moveHorizontally ? L"[TextBox][button2]" : L"[TextBox][button1]");
        Platform::String^ focusSequence = "";

        auto gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Grid, GotFocus);

        RunOnUIThread([&]()
        {
            rootPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                LR"(<Grid xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
                          xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
                          Width="400" Height="400">
                        <Grid.RowDefinitions>
                            <RowDefinition Height="*"/>
                            <RowDefinition Height="*"/>
                        </Grid.RowDefinitions>
                        <Grid.ColumnDefinitions>
                            <ColumnDefinition Width="*"/>
                            <ColumnDefinition Width="*"/>
                        </Grid.ColumnDefinitions>
                        <Button x:Name="button1" Content="Button1" Grid.Row="0" Grid.Column="0"/>
                        <AutoSuggestBox x:Name="autoSuggestBox" Grid.Row="1" Grid.Column="0"/>
                        <Button x:Name="button2" Content="Button2" Grid.Row="1" Grid.Column="1"/>
                    </Grid>)"));
            asb = safe_cast<xaml_controls::AutoSuggestBox^>(rootPanel->FindName(L"autoSuggestBox"));
            asb->ItemsSource = itemList;
            button1 = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button1"));
            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        FocusTestHelper::EnsureFocus(button1, FocusState::Programmatic);

        RunOnUIThread([&]()
        {
            gotFocusRegistration.Attach(rootPanel, ref new xaml::RoutedEventHandler([&](Platform::Object^ sender, xaml::RoutedEventArgs^ args)
            {
                focusSequence += "[" + safe_cast<xaml::FrameworkElement^>(args->OriginalSource)->Name + "]";
            }));
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            focusSequence = "";
            asb->Focus(xaml::FocusState::Keyboard);
        });
        TestServices::WindowHelper->WaitForIdle();

        if (suggestionListOpen)
        {
            TestServices::KeyboardHelper->PressKeySequence(L"R");
            TestServices::WindowHelper->WaitForIdle();

            if (goToFirstSuggestion)
            {
                CommonInputHelper::Down(inputDevice);
                TestServices::WindowHelper->WaitForIdle();
            }
        }

        switch (inputDevice)
        {
        case InputDevice::Keyboard:
            TestServices::KeyboardHelper->ShiftTab();
            TestServices::WindowHelper->WaitForIdle();
            if (suggestionListOpen && goToFirstSuggestion)
            {
                TestServices::KeyboardHelper->ShiftTab();
                TestServices::WindowHelper->WaitForIdle();
            }
            break;
        case InputDevice::Gamepad:
            if (moveHorizontally)
            {
                CommonInputHelper::Right(inputDevice);
            }
            else
            {
                CommonInputHelper::Up(inputDevice);
                if (suggestionListOpen && goToFirstSuggestion)
                {
                    CommonInputHelper::Up(inputDevice);
                }
            }
        }
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Expected focus sequence: %s", expectedFocusSequence->Data());
        LOG_OUTPUT(L"Actual focus sequence: %s", focusSequence->Data());
        VERIFY_ARE_EQUAL(focusSequence, expectedFocusSequence);
    }

    void AutoSuggestBoxIntegrationTests::ValidateDisplayMemberPathPropagatesToSuggestionsPopup()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::AutoSuggestBox^ autoSuggestBox = nullptr;
        Platform::String^ itemString = L"Item 1";

        Platform::Collections::Vector<SuggestionListObject^>^ itemList = ref new Platform::Collections::Vector<SuggestionListObject^>();
        itemList->Append(ref new SuggestionListObject(itemString));

        xaml_controls::StackPanel^ rootPanel = SetupAutoSuggestBoxTest(xaml::VerticalAlignment::Top, itemList);

        RunOnUIThread([&]()
        {
            autoSuggestBox = safe_cast<xaml_controls::AutoSuggestBox^>(rootPanel->FindName(L"autoSuggestBox"));
            autoSuggestBox->DisplayMemberPath = "Title";
        });

        auto popupOpenedEvent = std::make_shared<Event>();
        auto popupOpenedRegistration = CreateSafeEventRegistration(xaml_primitives::Popup, Opened);
        auto popupClosedEvent = std::make_shared<Event>();
        auto popupClosedRegistration = CreateSafeEventRegistration(xaml_primitives::Popup, Closed);

        RunOnUIThread([&]()
        {
            auto popup = safe_cast<xaml_primitives::Popup^>(TreeHelper::GetVisualChildByName(autoSuggestBox, L"SuggestionsPopup"));

            popupOpenedRegistration.Attach(
                popup,
                ref new wf::EventHandler<Platform::Object^>([popupOpenedEvent](Platform::Object^, Platform::Object^)
            {
                popupOpenedEvent->Set();
            }));

            popupClosedRegistration.Attach(
                popup,
                ref new wf::EventHandler<Platform::Object^>([popupClosedEvent](Platform::Object^, Platform::Object^)
            {
                popupClosedEvent->Set();
            }));
        });

        // Type "i" to show the suggestion list.
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_i#$u$_i");
        popupOpenedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            auto suggestionsListView = safe_cast<xaml_controls::ListView^>(TreeHelper::GetVisualChildByNameFromOpenPopups(L"SuggestionsList", autoSuggestBox));
            auto firstItemContainer = safe_cast<xaml_controls::ListViewItem^>(suggestionsListView->ContainerFromIndex(0));
            auto itemContainerTextBlock = TreeHelper::GetVisualChildByType<xaml_controls::TextBlock>(firstItemContainer);

            VERIFY_IS_NOT_NULL(itemContainerTextBlock);
            LOG_OUTPUT(L"Expecting first item to be '%s'.  Actual string was '%s'.", itemString->Data(), itemContainerTextBlock->Text->Data());
            VERIFY_IS_TRUE(Platform::String::CompareOrdinal(itemContainerTextBlock->Text, itemString) == 0);
        });

        // Hit escape to hide the suggestion list.
        TestServices::KeyboardHelper->Escape();
        popupClosedEvent->WaitForDefault();
    }

    void AutoSuggestBoxIntegrationTests::ValidatePopupOpensAsSoonAsItemsSourceChanges()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::AutoSuggestBox^ autoSuggestBox = nullptr;
        auto rootPanel = SetupAutoSuggestBoxTest(xaml::VerticalAlignment::Top, nullptr);

        auto textChangedEvent = std::make_shared<Event>();
        auto textChangedRegistration = CreateSafeEventRegistration(xaml_controls::AutoSuggestBox, TextChanged);
        auto popupOpenedEvent = std::make_shared<Event>();
        auto popupOpenedRegistration = CreateSafeEventRegistration(xaml_primitives::Popup, Opened);
        auto popupClosedEvent = std::make_shared<Event>();
        auto popupClosedRegistration = CreateSafeEventRegistration(xaml_primitives::Popup, Closed);

        RunOnUIThread([&]()
        {
            autoSuggestBox = safe_cast<xaml_controls::AutoSuggestBox^>(rootPanel->FindName(L"autoSuggestBox"));

            textChangedRegistration.Attach(autoSuggestBox,
                ref new wf::TypedEventHandler<xaml_controls::AutoSuggestBox^, xaml_controls::AutoSuggestBoxTextChangedEventArgs^>(
                [&](xaml_controls::AutoSuggestBox^ sender, xaml_controls::AutoSuggestBoxTextChangedEventArgs^ args)
            {
                sender->ItemsSource = GetStandardSuggestionList();
                VERIFY_IS_TRUE(sender->IsSuggestionListOpen);
            }));

            auto popup = safe_cast<xaml_primitives::Popup^>(TreeHelper::GetVisualChildByName(autoSuggestBox, L"SuggestionsPopup"));
            popupOpenedRegistration.Attach(popup, [&]{ popupOpenedEvent->Set(); });
            popupClosedRegistration.Attach(popup, [&]{ popupClosedEvent->Set(); });
        });

        // Type "t" to show the suggestion list.
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_t#$u$_t");
        popupOpenedEvent->WaitForDefault();

        // Hit escape to hide the suggestion list.
        TestServices::KeyboardHelper->Escape();
        popupClosedEvent->WaitForDefault();
    }

    void AutoSuggestBoxIntegrationTests::ValidateIsSuggestionListOpenChangesWhenPopupOpens()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::AutoSuggestBox^ autoSuggestBox = nullptr;
        long long propertyChangedCallbackToken = 0;

        auto rootPanel = SetupAutoSuggestBoxTest(xaml::VerticalAlignment::Top, GetStandardSuggestionList());

        auto popupOpenedEvent = std::make_shared<Event>();
        auto popupOpenedRegistration = CreateSafeEventRegistration(xaml_primitives::Popup, Opened);
        auto popupClosedEvent = std::make_shared<Event>();
        auto popupClosedRegistration = CreateSafeEventRegistration(xaml_primitives::Popup, Closed);

        int callbackOnOpenCount = 0;
        int callbackOnClosedCount = 0;

        RunOnUIThread([&]()
        {
            autoSuggestBox = safe_cast<xaml_controls::AutoSuggestBox^>(rootPanel->FindName(L"autoSuggestBox"));
            propertyChangedCallbackToken = autoSuggestBox->RegisterPropertyChangedCallback(
                xaml_controls::AutoSuggestBox::IsSuggestionListOpenProperty,
                ref new DependencyPropertyChangedCallback([&autoSuggestBox, &callbackOnOpenCount, &callbackOnClosedCount] (DependencyObject^ sender, DependencyProperty^ prop) {
                    if (autoSuggestBox->IsSuggestionListOpen)
                    {
                        callbackOnOpenCount++;
                    }
                    else
                    {
                        callbackOnClosedCount++;
                    }
                }));

            auto popup = safe_cast<xaml_primitives::Popup^>(TreeHelper::GetVisualChildByName(autoSuggestBox, L"SuggestionsPopup"));
            popupOpenedRegistration.Attach(popup, [&]{ popupOpenedEvent->Set(); });
            popupClosedRegistration.Attach(popup, [&]{ popupClosedEvent->Set(); });
        });

        // Type "t" to show the suggestion list.
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_t#$u$_t");
        popupOpenedEvent->WaitForDefault();

        // Hit escape to hide the suggestion list.
        TestServices::KeyboardHelper->Escape();
        popupClosedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(1, callbackOnOpenCount);
            VERIFY_ARE_EQUAL(1, callbackOnClosedCount);

            if (propertyChangedCallbackToken != 0)
            {
                autoSuggestBox->UnregisterPropertyChangedCallback(
                    xaml_controls::AutoSuggestBox::IsSuggestionListOpenProperty,
                    propertyChangedCallbackToken);
            }
        });
    }

    xaml_controls::StackPanel^ AutoSuggestBoxIntegrationTests::SetupAutoSuggestBoxTest(xaml::VerticalAlignment verticalAlign, Platform::Object^ itemList)
    {
        xaml_controls::StackPanel^ rootPanel = nullptr;
        xaml_controls::AutoSuggestBox^ autoSuggestBox = nullptr;

        auto gotFocusEvent = std::make_shared<Event>();
        auto gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::AutoSuggestBox, GotFocus);

        RunOnUIThread([&]()
        {
            rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                L"<StackPanel "
                L" xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"    <AutoSuggestBox x:Name='autoSuggestBox' Width='150'/> "
                L"    <Button x:Name='button1' Content='Button' /> "
                L"</StackPanel>"));

            rootPanel->VerticalAlignment = verticalAlign;

            autoSuggestBox = safe_cast<Microsoft::UI::Xaml::Controls::AutoSuggestBox^>(rootPanel->FindName(L"autoSuggestBox"));

            gotFocusRegistration.Attach(
                autoSuggestBox,
                ref new xaml::RoutedEventHandler(
                [gotFocusEvent](Platform::Object^ sender, xaml::IRoutedEventArgs^)
            {
                gotFocusEvent->Set();
            }));

            autoSuggestBox->ItemsSource = itemList;

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            autoSuggestBox->Focus(xaml::FocusState::Programmatic);
        });

        gotFocusEvent->WaitForDefault();

        return rootPanel;
    }

    xaml_controls::StackPanel^ AutoSuggestBoxIntegrationTests::SetupAutoSuggestBoxTestWithEvents(xaml::VerticalAlignment verticalAlign, Platform::Object^ itemList,
        std::shared_ptr<Microsoft::UI::Xaml::Tests::Common::Event> gotFocusEvent,
        SafeEventRegistrationType(xaml_controls::AutoSuggestBox, GotFocus)& gotFocusRegistration,
        std::shared_ptr<Microsoft::UI::Xaml::Tests::Common::Event> textChangedEvent,
        SafeEventRegistrationType(xaml_controls::AutoSuggestBox, TextChanged)& textChangedRegistration,
        std::shared_ptr<Microsoft::UI::Xaml::Tests::Common::Event> suggestionChosenEvent,
        SafeEventRegistrationType(xaml_controls::AutoSuggestBox, SuggestionChosen)& suggestionChosenRegistration,
        std::shared_ptr<Microsoft::UI::Xaml::Tests::Common::Event> popupOpenedEvent,
        SafeEventRegistrationType(xaml_primitives::Popup, Opened)& popupOpenedRegistration,
        std::shared_ptr<Microsoft::UI::Xaml::Tests::Common::Event> popupClosedEvent,
        SafeEventRegistrationType(xaml_primitives::Popup, Closed)& popupClosedRegistration)
    {
        xaml_controls::StackPanel^ rootPanel = nullptr;
        xaml_controls::AutoSuggestBox^ autoSuggestBox = nullptr;

        RunOnUIThread([&]()
        {
            rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                LR"(<StackPanel xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml">
                        <AutoSuggestBox x:Name="autoSuggestBox" Width="150"/>
                        <Button x:Name="button1" Content="Button" />
                    </StackPanel>)"));
            rootPanel->VerticalAlignment = verticalAlign;

            autoSuggestBox = safe_cast<xaml_controls::AutoSuggestBox^>(rootPanel->FindName(L"autoSuggestBox"));
            autoSuggestBox->ItemsSource = itemList;
            gotFocusRegistration.Attach(autoSuggestBox, [gotFocusEvent]{ gotFocusEvent->Set(); });
            textChangedRegistration.Attach(autoSuggestBox, [textChangedEvent]() { textChangedEvent->Set(); });
            suggestionChosenRegistration.Attach(autoSuggestBox, [suggestionChosenEvent]{ suggestionChosenEvent->Set(); });

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto popup = safe_cast<xaml_primitives::Popup^>(TreeHelper::GetVisualChildByName(autoSuggestBox, L"SuggestionsPopup"));
            popupOpenedRegistration.Attach(popup, [popupOpenedEvent]{ popupOpenedEvent->Set(); });
            popupClosedRegistration.Attach(popup, [popupClosedEvent]{ popupClosedEvent->Set(); });
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            autoSuggestBox->Focus(xaml::FocusState::Programmatic);
        });
        gotFocusEvent->WaitForDefault();

        return rootPanel;
    }

    xaml_controls::StackPanel^ AutoSuggestBoxIntegrationTests::SetupAutoSuggestBoxForUIValidation(xaml::VerticalAlignment verticalAlign, Platform::Collections::Vector<Platform::String^>^ itemList)
    {
        xaml_controls::StackPanel^ rootPanel = nullptr;
        xaml_controls::AutoSuggestBox^ autoSuggestBoxPressed = nullptr;

        auto gotFocusEvent = std::make_shared<Event>();
        auto gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::AutoSuggestBox, GotFocus);

        RunOnUIThread([&]()
        {
            rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                L"<StackPanel "
                L" xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"    <AutoSuggestBox x:Name='autoSuggestBoxDisabled' Width='150' QueryIcon='Find' IsEnabled='false' PlaceholderText='Search...' LightDismissOverlayMode='Off' /> "
                L"    <AutoSuggestBox x:Name='autoSuggestBoxUnfocused' Width='150' QueryIcon='Find' Text='Sample Text' LightDismissOverlayMode='Off' /> "
                L"    <AutoSuggestBox x:Name='autoSuggestBoxNormal' Width='150' QueryIcon='Find' Text='Sample Text' LightDismissOverlayMode='Off' /> "
                L"    <AutoSuggestBox x:Name='autoSuggestBoxPointerOver' Width='150' QueryIcon='Find' Text='Sample Text' LightDismissOverlayMode='Off' /> "
                L"    <AutoSuggestBox x:Name='autoSuggestBoxPressed' TabIndex='0' Width='150' QueryIcon='Find' LightDismissOverlayMode='Off' /> "
                L"</StackPanel>"));

            rootPanel->VerticalAlignment = verticalAlign;

            autoSuggestBoxPressed = safe_cast<Microsoft::UI::Xaml::Controls::AutoSuggestBox^>(rootPanel->FindName(L"autoSuggestBoxPressed"));

            gotFocusRegistration.Attach(
                autoSuggestBoxPressed,
                ref new xaml::RoutedEventHandler(
                [gotFocusEvent](Platform::Object^ sender, xaml::IRoutedEventArgs^)
            {
                gotFocusEvent->Set();
            }));

            autoSuggestBoxPressed->ItemsSource = itemList;

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        //Focus autoSuggestBoxPressed for typing event
        RunOnUIThread([&]()
        {
            autoSuggestBoxPressed->Focus(xaml::FocusState::Pointer);
        });

        gotFocusEvent->WaitForDefault();

        return rootPanel;
    }

    xaml_controls::AutoSuggestBox^ AutoSuggestBoxIntegrationTests::SetupAutoSuggestBoxWithQueryIcon()
    {
        xaml_controls::AutoSuggestBox^ autoSuggestBox = nullptr;
        auto gotFocusEvent = std::make_shared<Event>();
        auto gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::AutoSuggestBox, GotFocus);

        RunOnUIThread([&]()
        {
            auto symbolIcon = ref new xaml_controls::SymbolIcon();
            symbolIcon->Symbol = xaml_controls::Symbol::Find;

            autoSuggestBox = ref new xaml_controls::AutoSuggestBox();
            autoSuggestBox->QueryIcon = symbolIcon;
            // Add a top margin to push the ASB out from under the status bar on phone.
            autoSuggestBox->Margin = xaml::ThicknessHelper::FromLengths(0, 32, 0, 0);
            gotFocusRegistration.Attach(autoSuggestBox, [&] { gotFocusEvent->Set(); });

            xaml_controls::Grid^ root = ref new xaml_controls::Grid();
            root->VerticalAlignment = xaml::VerticalAlignment::Top;
            root->Children->Append(autoSuggestBox);
            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            autoSuggestBox->Focus(xaml::FocusState::Keyboard);
        });
        gotFocusEvent->WaitForDefault();

        return autoSuggestBox;
    }

    Platform::Collections::Vector<Platform::String^>^ AutoSuggestBoxIntegrationTests::GetStandardSuggestionList()
    {
        Platform::Collections::Vector<Platform::String^>^ suggestionList = ref new Platform::Collections::Vector<Platform::String^>();
        suggestionList->Append("Red");
        suggestionList->Append("Green");
        suggestionList->Append("Blue");
        return suggestionList;
    }

    void AutoSuggestBoxIntegrationTests::ValidateAutoSuggestBoxPosition()
    {
        PerformValidateAutoSuggestBoxPosition(xaml::Thickness({ 20, 50, 0, 0 }));
        PerformValidateAutoSuggestBoxPosition(xaml::Thickness({20, 200, 0, 0}));
    }

    void AutoSuggestBoxIntegrationTests::PerformValidateAutoSuggestBoxPosition(xaml::Thickness margin)
    {
        TestCleanupWrapper cleanup;
        DisableErrorReportingScopeGuard disableErrors;

        xaml_controls::StackPanel^ rootPanel = nullptr;
        xaml_controls::AutoSuggestBox^ autoSuggestBox = nullptr;
        xaml_controls::Button^ button1 = nullptr;

        wf::Rect originalBounds = {};
        wf::Rect newBounds = {};

        auto gotFocusButtonEvent = std::make_shared<Event>();
        auto gotFocusButtonRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

        auto tappedASBEvent = std::make_shared<Event>();
        auto tappedASBRegistration = CreateSafeEventRegistrationForHandledEvents(UIElement, TappedEvent);

        RunOnUIThread([&]()
        {
            rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                L"<StackPanel Background='AliceBlue' "
                L" xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"    <ScrollViewer x:Name='scrollViewer' >"
                L"        <StackPanel >"
                L"          <AutoSuggestBox x:Name='autoSuggestBox' />"
                L"          <Button x:Name='button1' Content='Button' /> "
                L"        </StackPanel >"
                L"    </ScrollViewer >"
                L"</StackPanel>"));

            autoSuggestBox = safe_cast<Microsoft::UI::Xaml::Controls::AutoSuggestBox^>(rootPanel->FindName(L"autoSuggestBox"));
            autoSuggestBox->Margin = margin;

            button1 = safe_cast<Microsoft::UI::Xaml::Controls::Button^>(rootPanel->FindName(L"button1"));

            gotFocusButtonRegistration.Attach(
                button1,
                ref new xaml::RoutedEventHandler(
                [gotFocusButtonEvent](Platform::Object^ sender, xaml::IRoutedEventArgs^)
            {
                gotFocusButtonEvent->Set();
            }));

            tappedASBRegistration.Attach(autoSuggestBox,
                ref new xaml_input::TappedEventHandler([&](Platform::Object^, xaml_input::TappedRoutedEventArgs^ args)
            {
                tappedASBEvent->Set();
            }));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            originalBounds = ControlHelper::GetBounds(autoSuggestBox);
        });

        TestServices::InputHelper->Tap(autoSuggestBox);
        tappedASBEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            newBounds = ControlHelper::GetBounds(autoSuggestBox);

            LOG_OUTPUT(L"Original top=%f New top=%f", originalBounds.Y, newBounds.Y);
            VERIFY_IS_TRUE(originalBounds.Y == newBounds.Y);
        });

        test_infra::TestServices::InputHelper->LeftMouseClick(button1);
        gotFocusButtonEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }

    void AutoSuggestBoxIntegrationTests::ValidateFootprint()
    {
        TestCleanupWrapper cleanup;

        double const expectedAutoSuggestBoxWidth = 200; // AutoSuggestBox Width equals the Width of its container.

        double const expectedAutoSuggestBoxHeight_WithNoHeader = 32; // TextBox Height
        double const expectedAutoSuggestBoxHeight_WithTextHeader = 23 + 4 + expectedAutoSuggestBoxHeight_WithNoHeader; // Header Height + 4px gap + TextBox Height
        double const expectedAutoSuggestBoxHeight_WithLargeHeader = 104 + 4 + expectedAutoSuggestBoxHeight_WithNoHeader; // Header Height + 4px gap + TextBox Height

        double const expectedSuggestionsListHeight_WithFewSuggestions = 40 * 3; // Item Height * Number of Items
        double const expectedSuggestionsListHeight_WithManySuggestions = 360; // SuggestionsList MaxHeight

        xaml_controls::AutoSuggestBox^ autoSuggestBoxWithTextHeader;
        xaml_controls::AutoSuggestBox^ autoSuggestBoxWithNoHeader;
        xaml_controls::AutoSuggestBox^ autoSuggestBoxWithLargeHeader;
        xaml_controls::AutoSuggestBox^ autoSuggestBoxWithFewSuggestions;
        xaml_controls::AutoSuggestBox^ autoSuggestBoxWithManySuggestions;

        xaml_controls::ListView^ suggestionsListWithFewSuggestions;
        xaml_controls::ListView^ suggestionsListWithManySuggestions;

        auto fewSuggestions = ref new Platform::Collections::Vector<Platform::String^>();
        for (unsigned int i = 0; i < 3; i++)
        {
            fewSuggestions->Append("Suggestion " + i);
        }
        auto manySuggestions = ref new Platform::Collections::Vector<Platform::String^>();
        for (unsigned int i = 0; i < 9; i++)
        {
            manySuggestions->Append("Suggestion " + i);
        }

        RunOnUIThread([&]()
        {
            // Note that we have autoSuggestBoxWithManySuggestions and autoSuggestBoxWithFewSuggestions at the top so that the size of
            // SuggestionsList can reach its maximum limits.
            auto rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                LR"(<StackPanel xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" Width="200">
                        <AutoSuggestBox x:Name="autoSuggestBoxWithManySuggestions" />
                        <AutoSuggestBox x:Name="autoSuggestBoxWithFewSuggestions" />
                        <AutoSuggestBox x:Name="autoSuggestBoxWithTextHeader" Header="AutoSuggestBox" />
                        <AutoSuggestBox x:Name="autoSuggestBoxWithNoHeader" />
                        <AutoSuggestBox x:Name="autoSuggestBoxWithLargeHeader" >
                            <AutoSuggestBox.Header>
                                <Rectangle Height="100" Width="100" Fill="Red" />
                            </AutoSuggestBox.Header>
                        </AutoSuggestBox>
                    </StackPanel>)"));

            autoSuggestBoxWithTextHeader = safe_cast<xaml_controls::AutoSuggestBox^>(rootPanel->FindName(L"autoSuggestBoxWithTextHeader"));
            autoSuggestBoxWithNoHeader = safe_cast<xaml_controls::AutoSuggestBox^>(rootPanel->FindName(L"autoSuggestBoxWithNoHeader"));
            autoSuggestBoxWithLargeHeader = safe_cast<xaml_controls::AutoSuggestBox^>(rootPanel->FindName(L"autoSuggestBoxWithLargeHeader"));
            autoSuggestBoxWithFewSuggestions = safe_cast<xaml_controls::AutoSuggestBox^>(rootPanel->FindName(L"autoSuggestBoxWithFewSuggestions"));
            autoSuggestBoxWithManySuggestions = safe_cast<xaml_controls::AutoSuggestBox^>(rootPanel->FindName(L"autoSuggestBoxWithManySuggestions"));

            autoSuggestBoxWithFewSuggestions->ItemsSource = fewSuggestions;
            autoSuggestBoxWithManySuggestions->ItemsSource = manySuggestions;

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            // Verify Footprint of AutoSuggestBox with text Header:
            VERIFY_ARE_EQUAL(expectedAutoSuggestBoxWidth, autoSuggestBoxWithTextHeader->ActualWidth);
            VERIFY_ARE_EQUAL(expectedAutoSuggestBoxHeight_WithTextHeader, autoSuggestBoxWithTextHeader->ActualHeight);

            // Verify Footprint of AutoSuggestBox with no Header:
            VERIFY_ARE_EQUAL(expectedAutoSuggestBoxWidth, autoSuggestBoxWithNoHeader->ActualWidth);
            VERIFY_ARE_EQUAL(expectedAutoSuggestBoxHeight_WithNoHeader, autoSuggestBoxWithNoHeader->ActualHeight);

            // Verify Footprint of AutoSuggestBox with large Header:
            VERIFY_ARE_EQUAL(expectedAutoSuggestBoxWidth, autoSuggestBoxWithLargeHeader->ActualWidth);
            VERIFY_ARE_EQUAL(expectedAutoSuggestBoxHeight_WithLargeHeader, autoSuggestBoxWithLargeHeader->ActualHeight);
        });

        RunOnUIThread([&]()
        {
            autoSuggestBoxWithFewSuggestions->IsSuggestionListOpen = true;
        });
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            suggestionsListWithFewSuggestions = safe_cast<xaml_controls::ListView^>(TreeHelper::GetVisualChildByNameFromOpenPopups(L"SuggestionsList", autoSuggestBoxWithTextHeader));
            // Verify Footprint of AutoSuggestBox's SuggestionsList with few suggestions:
            VERIFY_ARE_EQUAL(expectedAutoSuggestBoxWidth, suggestionsListWithFewSuggestions->ActualWidth);
            VERIFY_ARE_EQUAL(expectedSuggestionsListHeight_WithFewSuggestions, suggestionsListWithFewSuggestions->ActualHeight);
        });

        RunOnUIThread([&]()
        {
            autoSuggestBoxWithManySuggestions->IsSuggestionListOpen = true;
        });
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            suggestionsListWithManySuggestions = safe_cast<xaml_controls::ListView^>(TreeHelper::GetVisualChildByNameFromOpenPopups(L"SuggestionsList", autoSuggestBoxWithTextHeader));
            // Verify Footprint of AutoSuggestBox's SuggestionsList with many suggestions:
            VERIFY_ARE_EQUAL(expectedAutoSuggestBoxWidth, suggestionsListWithManySuggestions->ActualWidth);
            VERIFY_ARE_EQUAL(expectedSuggestionsListHeight_WithManySuggestions, suggestionsListWithManySuggestions->ActualHeight);
        });
    }

    void AutoSuggestBoxIntegrationTests::ValidateSipClosedOnLostFocus()
    {
        auto inputPaneShowingEvent = std::make_shared<Event>();
        wf::EventRegistrationToken inputPaneShowingToken;

        auto inputPaneHidingEvent = std::make_shared<Event>();
        wf::EventRegistrationToken inputPaneHidingToken;

        InputPane^ inputPane = nullptr;

        TestCleanupWrapper cleanup([&]()
        {
            RunOnUIThread([&]()
            {
                inputPane->Showing -= inputPaneShowingToken;
                inputPane->Hiding -= inputPaneHidingToken;

                inputPaneShowingToken = {};
                inputPaneHidingToken = {};

                inputPane = nullptr;
            });

            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        xaml_controls::Button^ btn = nullptr;
        xaml_controls::AutoSuggestBox^ asb = nullptr;

        auto gotFocusEventButton = std::make_shared<Event>();
        auto gotFocusRegistrationButton = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

        auto lostFocusEvent = std::make_shared<Event>();
        auto lostFocusRegistration = CreateSafeEventRegistration(xaml_controls::AutoSuggestBox, LostFocus);

        auto rootPanel = SetupAutoSuggestBoxTest(xaml::VerticalAlignment::Top, GetStandardSuggestionList());

        RunOnUIThread([&]
        {
            btn = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button1"));
            asb = safe_cast<xaml_controls::AutoSuggestBox^>(rootPanel->FindName(L"autoSuggestBox"));
            inputPane = InputPane::GetForCurrentView();

            gotFocusRegistrationButton.Attach(btn, [&]()
            {
                LOG_OUTPUT(L"btn got focus");
                gotFocusEventButton->Set();
            });

            lostFocusRegistration.Attach(asb, [&]()
            {
                LOG_OUTPUT(L"asb lost focus");
                lostFocusEvent->Set();
            });

            inputPaneShowingToken = inputPane->Showing += ref new wf::TypedEventHandler<InputPane^, InputPaneVisibilityEventArgs^>([&](InputPane^ pane, InputPaneVisibilityEventArgs^ e)
            {
                LOG_OUTPUT(L"InputPane is showing");
                inputPaneShowingEvent->Set();
            });

            inputPaneHidingToken = inputPane->Hiding += ref new wf::TypedEventHandler<InputPane^, InputPaneVisibilityEventArgs^>([&](InputPane^ pane, InputPaneVisibilityEventArgs^ e)
            {
                LOG_OUTPUT(L"InputPane is hiding");
                inputPaneHidingEvent->Set();
            });

            asb->Focus(FocusState::Keyboard);
        });

        inputPaneShowingEvent->WaitForDefault();

        RunOnUIThread([&]
        {
            btn->Focus(FocusState::Keyboard);
        });

        lostFocusEvent->WaitForDefault();
        inputPaneHidingEvent->WaitForDefault();
    }

    void AutoSuggestBoxIntegrationTests::NextInput(InputDevice inputDevice, bool goDown)
    {
        if (goDown)
        {
            CommonInputHelper::Down(inputDevice);
        }
        else
        {
            CommonInputHelper::Up(inputDevice);
        }
        TestServices::WindowHelper->WaitForIdle();
    }

    void AutoSuggestBoxIntegrationTests::ValidateLightDismissOverlayMode()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::AutoSuggestBox^ autoSuggestBox = nullptr;

        RunOnUIThread([&]()
        {
            autoSuggestBox = ref new xaml_controls::AutoSuggestBox();
            autoSuggestBox->IsSuggestionListOpen = true;

            TestServices::WindowHelper->WindowContent = autoSuggestBox;
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Validate that the default is Auto and the AutoSuggestBox's overlay is not visible (or visible if on Xbox)");
        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(autoSuggestBox->LightDismissOverlayMode, xaml_controls::LightDismissOverlayMode::Auto);
            ValidateVisibilityOfOverlayElement(autoSuggestBox, TestServices::Utilities->IsXBox);
        });

        LOG_OUTPUT(L"Validate that when set to On the AutoSuggestBox's overlay is visible.");
        RunOnUIThread([&]()
        {
            autoSuggestBox->LightDismissOverlayMode = xaml_controls::LightDismissOverlayMode::On;
            ValidateVisibilityOfOverlayElement(autoSuggestBox, true);
        });

        LOG_OUTPUT(L"Validate that when set to Off the AutoSuggestBox's overlay is not visible.");
        RunOnUIThread([&]()
        {
            autoSuggestBox->LightDismissOverlayMode = xaml_controls::LightDismissOverlayMode::Off;
            ValidateVisibilityOfOverlayElement(autoSuggestBox, false);
        });
    }

    void AutoSuggestBoxIntegrationTests::IsAutoLightDismissOverlayModeVisibleOnXbox()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::AutoSuggestBox^ autoSuggestBox = nullptr;

        RunOnUIThread([&]()
        {
            autoSuggestBox = ref new xaml_controls::AutoSuggestBox();
            autoSuggestBox->LightDismissOverlayMode = xaml_controls::LightDismissOverlayMode::On;
            autoSuggestBox->IsSuggestionListOpen = true;

            TestServices::WindowHelper->WindowContent = autoSuggestBox;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            ValidateVisibilityOfOverlayElement(autoSuggestBox, true);
        });
    }

    void AutoSuggestBoxIntegrationTests::ValidateOverlayBrush()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::AutoSuggestBox^ autoSuggestBox = nullptr;

        RunOnUIThread([&]()
        {
            autoSuggestBox = ref new xaml_controls::AutoSuggestBox();
            autoSuggestBox->LightDismissOverlayMode = xaml_controls::LightDismissOverlayMode::On;
            autoSuggestBox->IsSuggestionListOpen = true;

            TestServices::WindowHelper->WindowContent = autoSuggestBox;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto expectedBrush = safe_cast<xaml_media::SolidColorBrush^>(xaml::Application::Current->Resources->Lookup(L"AutoSuggestBoxLightDismissOverlayBackground"));

            auto overlayElement = GetAutoSuggestBoxOverlayElement(autoSuggestBox);
            THROW_IF_NULL_WITH_MSG(overlayElement, L"An overlay element should exist.");

            auto overlayRect = dynamic_cast<xaml_shapes::Rectangle^>(overlayElement);
            THROW_IF_NULL_WITH_MSG(overlayRect, L"The overlay element should be a rectangle.");

            auto overlayBrush = safe_cast<xaml_media::SolidColorBrush^>(overlayRect->Fill);
            VERIFY_IS_NOT_NULL(overlayBrush);
            VERIFY_IS_TRUE(overlayBrush->Equals(expectedBrush));
        });
    }

    void AutoSuggestBoxIntegrationTests::ValidateOverlayUIETree()
    {
        TestCleanupWrapper cleanup;

        // Hide the textbox caret so it doesn't interfere with UIElement tree comparison
        RuntimeEnabledFeatureOverride disableTextBoxCaret(RuntimeFeatureBehavior::RuntimeEnabledFeature::DisableTextBoxCaret, true);

        ControlHelper::ValidateUIElementTree(
            PopupHelper::AreWindowedPopupsEnabled() ? L"Windowed" : L"Unwindowed",
            wf::Size(400, 400),
            1.f,
            []()
            {
                return SetupOverlayTreeValidationTest();
            }
        );
    }

    xaml_controls::Panel^ AutoSuggestBoxIntegrationTests::SetupOverlayTreeValidationTest()
    {
        xaml_controls::Grid^ root = nullptr;
        xaml_controls::AutoSuggestBox^ autoSuggestBox = nullptr;

        RunOnUIThread([&]()
        {
            root = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                LR"(<Grid xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
                                Background="{ThemeResource SystemControlBackgroundAltHighBrush}" >
                               <AutoSuggestBox x:Name="autoSuggestBox" LightDismissOverlayMode="On"/>
                            </Grid>)"));

            autoSuggestBox = safe_cast<xaml_controls::AutoSuggestBox^>(root->FindName(L"autoSuggestBox"));

            Platform::Collections::Vector<Platform::String^>^ itemList = ref new Platform::Collections::Vector<Platform::String^>();
            itemList->Append("item 1");
            itemList->Append("item 2");
            itemList->Append("item 3");

            autoSuggestBox->ItemsSource = itemList;

            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            autoSuggestBox->Focus(xaml::FocusState::Pointer);
        });
        TestServices::WindowHelper->WaitForIdle();

        auto textChangedEvent = std::make_shared<Event>();
        auto textChangedRegistration = CreateSafeEventRegistration(xaml_controls::AutoSuggestBox, TextChanged);

        textChangedRegistration.Attach(autoSuggestBox, [&]() { textChangedEvent->Set(); });

        // Type "S" to show the suggestion list
        TestServices::KeyboardHelper->PressKeySequence(L"S");
        textChangedEvent->WaitForDefault();

        return root;
    }

    xaml::FrameworkElement^ AutoSuggestBoxIntegrationTests::GetAutoSuggestBoxOverlayElement(xaml_controls::AutoSuggestBox^ autoSuggestBox)
    {
        WEX::Common::Throw::IfFalse(autoSuggestBox->IsSuggestionListOpen, E_FAIL, L"AutoSuggestBox's suggestion list should be opened before calling this helper.");

        // Get the layoutRoot element of the AutoSuggestBox.
        auto layoutRoot = safe_cast<xaml_controls::Grid^>(xaml_media::VisualTreeHelper::GetChild(autoSuggestBox, 0));

        // When open, the overlay element should be the first child under the layout root.
        auto overlayElement = safe_cast<xaml::FrameworkElement^>(layoutRoot->Children->GetAt(0));

        return dynamic_cast<xaml_shapes::Rectangle^>(layoutRoot->Children->GetAt(0));
    }

    void AutoSuggestBoxIntegrationTests::ValidateVisibilityOfOverlayElement(xaml_controls::AutoSuggestBox^ autoSuggestBox, bool expectedIsVisible)
    {
        auto overlayElement = GetAutoSuggestBoxOverlayElement(autoSuggestBox);

        if (expectedIsVisible)
        {
            VERIFY_IS_NOT_NULL(overlayElement);
            THROW_IF_NULL_WITH_MSG(overlayElement, L"An overlay element should exist.");

            auto overlayRect = dynamic_cast<xaml_shapes::Rectangle^>(overlayElement);
            THROW_IF_NULL_WITH_MSG(overlayRect, L"The overlay element should be a rectangle.");

            auto overlayBrush = safe_cast<xaml_media::SolidColorBrush^>(overlayRect->Fill);
            THROW_IF_NULL_WITH_MSG(overlayBrush, L"The overlay element should have a brush.");

            auto brushColor = overlayBrush->Color;
        }
        else
        {
            VERIFY_IS_NULL(overlayElement);
        }
    }

    void AutoSuggestBoxIntegrationTests::CanTabOutWhileSuggestionListIsOpen()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::AutoSuggestBox^ autoSuggestBox = nullptr;
        xaml_controls::Button^ button = nullptr;

        Platform::Collections::Vector<Platform::String^>^ itemList = ref new Platform::Collections::Vector<Platform::String^>();
        itemList->Append("item 1");
        itemList->Append("item 2");
        itemList->Append("item 3");

        Platform::String^ expectedText = L"S";

        RunOnUIThread([&]()
        {
            autoSuggestBox = ref new xaml_controls::AutoSuggestBox();

            autoSuggestBox->ItemsSource = itemList;

            button = ref new xaml_controls::Button();
            button->Content = "Button";

            auto root = ref new xaml_controls::StackPanel;
            root->Children->Append(autoSuggestBox);
            root->Children->Append(button);

            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();

        auto runScenario = [&](bool moveForward)
        {
            RunOnUIThread([&]()
            {
                autoSuggestBox->Text = "";
                autoSuggestBox->Focus(xaml::FocusState::Keyboard);
            });
            TestServices::WindowHelper->WaitForIdle();

            // Type "S" to show the suggestion list
            TestServices::KeyboardHelper->PressKeySequence(expectedText);
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                WEX::Common::Throw::If(!autoSuggestBox->IsSuggestionListOpen, E_FAIL, L"The suggestion list should be open.");
            });

            // Press Down 2 times to select the second suggestion.
            TestServices::KeyboardHelper->Down();
            TestServices::WindowHelper->WaitForIdle();
            TestServices::KeyboardHelper->Down();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                WEX::Common::Throw::If(autoSuggestBox->Text != itemList->GetAt(1), E_FAIL, L"The second item is not selected.");
            });

            if (moveForward)
            {
                TestServices::KeyboardHelper->Tab();
            }
            else
            {
                TestServices::KeyboardHelper->ShiftTab();
            }
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(button->Equals(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)));
                VERIFY_IS_FALSE(autoSuggestBox->IsSuggestionListOpen);
                VERIFY_ARE_EQUAL(expectedText, autoSuggestBox->Text);
            });
        };

        LOG_OUTPUT(L"Validate that you can tab out of an open AutoSuggestBox.");
        runScenario(true /*moveForward*/);

        LOG_OUTPUT(L"Validate that you can shift-tab out of an open AutoSuggestBox.");
        runScenario(false /*moveForward*/);
    }

    void AutoSuggestBoxIntegrationTests::DoesNotClearTextWhenTabbedPast()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::AutoSuggestBox^ autoSuggestBox = nullptr;
        xaml_controls::Button^ beforeButton = nullptr;
        xaml_controls::Button^ afterButton = nullptr;

        Platform::String^ expectedText = "Expected text!";

        RunOnUIThread([&]()
        {
            autoSuggestBox = ref new xaml_controls::AutoSuggestBox();
            autoSuggestBox->Text = expectedText;

            beforeButton = ref new xaml_controls::Button();
            beforeButton->Content = "Before";

            afterButton = ref new xaml_controls::Button();
            afterButton->Content = "After";

            auto root = ref new xaml_controls::StackPanel;
            root->Children->Append(beforeButton);
            root->Children->Append(autoSuggestBox);
            root->Children->Append(afterButton);

            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            beforeButton->Focus(xaml::FocusState::Keyboard);
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Tab twice to move onto and then off of the AutoSuggestBox.");
        TestServices::KeyboardHelper->Tab();
        TestServices::WindowHelper->WaitForIdle();

        TestServices::KeyboardHelper->Tab();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(expectedText, autoSuggestBox->Text);
        });
    }

    void AutoSuggestBoxIntegrationTests::ValidateUnloadAndReloadReregistersQuerySubmittedEvent()
    {
        TestCleanupWrapper cleanup;
        DisableErrorReportingScopeGuard disableErrors;

        xaml_controls::Page^ page = nullptr;
        auto pageLoadedEvent = std::make_shared<Event>();
        auto pageLoadedRegistration = CreateSafeEventRegistration(xaml_controls::Page, Loaded);

        xaml_controls::StackPanel^ alphaPanel = nullptr;
        xaml_controls::StackPanel^ betaPanel = nullptr;

        xaml_controls::AutoSuggestBox^ autoSuggestBox = nullptr;
        auto querySubmittedEvent = std::make_shared<Event>();
        auto querySubmittedRegistration = CreateSafeEventRegistration(xaml_controls::AutoSuggestBox, QuerySubmitted);

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Loading Page XAML.");
            page = safe_cast<xaml_controls::Page^>(xaml_markup::XamlReader::Load(
                LR"(<Page xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml">
                        <StackPanel>
                            <StackPanel x:Name="alpha">
                                <TextBlock Text="alpha text" />
                                <AutoSuggestBox x:Name="autoSuggestBox" QueryIcon="Find"></AutoSuggestBox>
                            </StackPanel>
                            <StackPanel x:Name="beta">
                                <TextBlock Text="beta text" />
                            </StackPanel>
                        </StackPanel>
                    </Page>)"));

            autoSuggestBox = safe_cast<xaml_controls::AutoSuggestBox^>(page->FindName(L"autoSuggestBox"));
            VERIFY_IS_NOT_NULL(autoSuggestBox);

            alphaPanel = safe_cast<xaml_controls::StackPanel^>(page->FindName(L"alpha"));
            VERIFY_IS_NOT_NULL(alphaPanel);

            betaPanel = safe_cast<xaml_controls::StackPanel^>(page->FindName(L"beta"));
            VERIFY_IS_NOT_NULL(betaPanel);

            LOG_OUTPUT(L"Registering ASB QuerySubmitted event.");
            querySubmittedRegistration.Attach(autoSuggestBox,
                ref new wf::TypedEventHandler<xaml_controls::AutoSuggestBox^, xaml_controls::AutoSuggestBoxQuerySubmittedEventArgs^>(
                    [&](Platform::Object^, xaml_controls::AutoSuggestBoxQuerySubmittedEventArgs^)
            {
                LOG_OUTPUT(L"Enter ASB QuerySubmitted event.");
                querySubmittedEvent->Set();
            }));

            LOG_OUTPUT(L"Registering Page Loaded event.");
            pageLoadedRegistration.Attach(page,
                ref new xaml::RoutedEventHandler(
                    [&](Platform::Object^, xaml::RoutedEventArgs^)
            {
                RunOnUIThread([&]()
                {
                    LOG_OUTPUT(L"Enter Page Loaded event.");

                    LOG_OUTPUT(L"Remove ASB from alpha.");
                    alphaPanel->Children->RemoveAt(1);

                    LOG_OUTPUT(L"Add ASB to beta.");
                    betaPanel->Children->Append(autoSuggestBox);

                    pageLoadedEvent->Set();
                });
            }));

            TestServices::WindowHelper->WindowContent = page;
        });

        LOG_OUTPUT(L"Waiting for Window Idle.");
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Waiting for Page Loaded event.");
        pageLoadedEvent->WaitForDefault();

        LOG_OUTPUT(L"Page ready.");
        xaml::FrameworkElement^ queryIcon = nullptr;
        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Enter text into ASB.");
            autoSuggestBox->Text = L"asb text";
            queryIcon = autoSuggestBox->QueryIcon;
        });

        LOG_OUTPUT(L"Waiting for Window Idle.");
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Clicking mouse on QueryIcon.");
        TestServices::InputHelper->LeftMouseClick(queryIcon);

        LOG_OUTPUT(L"Waiting for Window Idle.");
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Waiting for ASB QuerySubmitted event.");
        querySubmittedEvent->WaitForDefault();

        LOG_OUTPUT(L"Waiting for Window Idle.");
        TestServices::WindowHelper->WaitForIdle();
    }

    void AutoSuggestBoxIntegrationTests::ValidateSuggestionListFitsInWindow()
    {
        TestCleanupWrapper cleanup;

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 200));

        Platform::Collections::Vector<Platform::String^>^ suggestionList = ref new Platform::Collections::Vector<Platform::String^>();
        suggestionList->Append("A");
        suggestionList->Append("B");
        suggestionList->Append("C");
        suggestionList->Append("D");
        suggestionList->Append("E");
        suggestionList->Append("F");
        suggestionList->Append("G");
        suggestionList->Append("H");
        suggestionList->Append("I");
        suggestionList->Append("J");
        suggestionList->Append("K");

        auto rootPanel = SetupAutoSuggestBoxTest(xaml::VerticalAlignment::Top, suggestionList);

        auto popupOpenedEvent = std::make_shared<Event>();
        auto popupOpenedRegistration = CreateSafeEventRegistration(xaml_primitives::Popup, Opened);

        RunOnUIThread([&]()
        {
            auto autoSuggestBox = safe_cast<xaml_controls::AutoSuggestBox^>(rootPanel->FindName(L"autoSuggestBox"));

            auto popup = safe_cast<xaml_primitives::Popup^>(TreeHelper::GetVisualChildByName(autoSuggestBox, L"SuggestionsPopup"));
            VERIFY_IS_NOT_NULL(popup);


            popupOpenedRegistration.Attach(popup,
                ref new wf::EventHandler<Platform::Object^>([popupOpenedEvent](Platform::Object^, Platform::Object^)
            {
                popupOpenedEvent->Set();
            }));

            autoSuggestBox->IsSuggestionListOpen = true;
        });

        popupOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto suggestionsContainer = safe_cast<xaml_controls::Border^>(TreeHelper::GetVisualChildByNameFromOpenPopups(L"SuggestionsContainer", rootPanel));
            VERIFY_IS_NOT_NULL(suggestionsContainer);

            VERIFY_ARE_EQUAL(suggestionsContainer->MaxHeight, 168);
        });
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::AutoSuggestBox
