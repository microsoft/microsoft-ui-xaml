// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ComboBoxIntegrationTests.h"

#include <generic\DependencyObjectTests.h>
#include <generic\FrameworkElementTests.h>

#include <XamlTailored.h>
#include <TestEvent.h>
#include <TestCleanupWrapper.h>
#include "FeatureFlags.h"

#include <ComboBoxHelper.h>
#include <ControlHelper.h>
#include <TreeHelper.h>
#include <PopupHelper.h>

#include <random>
#include <CustomPropertySupport.h>
#include <WUCRenderingScopeGuard.h>
#include <FocusTestHelper.h>
#include "KeyboardInjectionOverride.h"
#include <Utils.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Media;
using namespace test_infra;
using namespace ::Windows::UI::ViewManagement;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace ComboBox {

    bool ComboBoxIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool ComboBoxIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool ComboBoxIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    ref class PersonObject sealed : public Microsoft::UI::Xaml::Tests::Common::CustomPropertyProviderBase
    {
    public:
        PersonObject(Platform::String^ firstName, Platform::String^ lastName)
        {
            this->FirstName = firstName;
            this->LastName = lastName;
        }

    protected:
        void AddCustomProperties() override
        {
            AddCustomProperty(L"FirstName", Platform::String::typeid,
                MAKEPROPGET(PersonObject^, FirstName),
                MAKEPROPSET(PersonObject^, FirstName, Platform::String^)
                );

            AddCustomProperty(L"LastName", Platform::String::typeid,
                MAKEPROPGET(PersonObject^, LastName),
                MAKEPROPSET(PersonObject^, LastName, Platform::String^)
                );
        }

    public:
        property Platform::String^ FirstName;
        property Platform::String^ LastName;
    };

    //
    // Test Cases
    //
    void ComboBoxIntegrationTests::CanInstantiate()
    {
        Generic::DependencyObjectTests<xaml_controls::ComboBox>::CanInstantiate();
    }

    void ComboBoxIntegrationTests::CanEnterAndLeaveLiveTree()
    {
        Generic::FrameworkElementTests<xaml_controls::ComboBox>::CanEnterAndLeaveLiveTree();
    }

    void ComboBoxIntegrationTests::CanComboBoxLoadFromXaml()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::ComboBox^ comboBox = nullptr;

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"  <ComboBox x:Name='comboBox' Background='RoyalBlue' SelectedIndex='1'  Width='350' > "
                L"    <ComboBoxItem Content='item one' />"
                L"    <ComboBoxItem Content='item two' />"
                L"    <ComboBoxItem Content='item three' />"
                L"  </ComboBox>"
                L"</Grid>"));

            comboBox = dynamic_cast<xaml_controls::ComboBox^>(rootPanel->FindName(L"comboBox"));
            VERIFY_IS_NOT_NULL(comboBox);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"CanComboBoxLoadFromXaml: ComboBox Width=%f Height=%f", comboBox->ActualWidth, comboBox->ActualHeight);
            VERIFY_IS_GREATER_THAN(comboBox->ActualWidth, 0);
            VERIFY_IS_GREATER_THAN(comboBox->ActualHeight, 0);
        });

        LOG_OUTPUT(L"CanComboBoxLoadFromXaml: OpenComboBox().");
        ComboBoxHelper::OpenComboBox(comboBox, ComboBoxHelper::OpenMethod::Programmatic);

        LOG_OUTPUT(L"CanComboBoxLoadFromXaml: VerifySelectedIndex().");
        ComboBoxHelper::VerifySelectedIndex(comboBox, 1);

        RunOnUIThread([&]()
        {
            comboBox->SelectedIndex = 2;
        });

        TestServices::WindowHelper->WaitForIdle();

        ComboBoxHelper::VerifySelectedIndex(comboBox, 2);
        ComboBoxHelper::CloseComboBox(comboBox);
    }

    void ComboBoxIntegrationTests::VerifyDefaultProperties()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]()
        {
            auto rootPanel = ref new xaml_controls::Grid();
            VERIFY_IS_NOT_NULL(rootPanel);

            auto comboBox = ref new xaml_controls::ComboBox();
            VERIFY_IS_NOT_NULL(comboBox);

            rootPanel->Children->Append(comboBox);
            TestServices::WindowHelper->WindowContent = rootPanel;

            VERIFY_IS_FALSE(comboBox->IsDropDownOpen);
            VERIFY_IS_FALSE(comboBox->IsEditable);
            VERIFY_IS_TRUE(comboBox->IsHitTestVisible);
            VERIFY_IS_FALSE(comboBox->IsSelectionBoxHighlighted);

            VERIFY_IS_NULL(comboBox->ItemContainerStyle);
            VERIFY_IS_NOT_NULL(comboBox->Items);

            VERIFY_IS_NULL(comboBox->Items->GetAt(0));
            VERIFY_IS_TRUE(comboBox->IsTabStop);
            VERIFY_IS_NULL(comboBox->ItemsSource);
            VERIFY_IS_NULL(comboBox->ItemTemplate);
            VERIFY_IS_NOT_NULL(comboBox->MaxDropDownHeight);
            VERIFY_IS_NULL(comboBox->SelectedItem);
            VERIFY_ARE_EQUAL(-1, comboBox->SelectedIndex);
            VERIFY_IS_NOT_NULL(comboBox->FontFamily);
            VERIFY_IS_NOT_NULL(comboBox->Foreground);
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void ComboBoxIntegrationTests::CanExpandAndCloseProjectedShadow()
    {
        RuntimeEnabledFeatureOverride featureUseDropShadows(RuntimeFeatureBehavior::RuntimeEnabledFeature::ForceProjectedShadowsOnByDefault, true);
        CanExpandAndClose();
    }

    void ComboBoxIntegrationTests::CanExpandAndCloseDropShadow()
    {
        CanExpandAndClose();
    }

    void ComboBoxIntegrationTests::CanExpandAndClose()
    {
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
        xaml_controls::ComboBox^ comboBox = nullptr;

        RunOnUIThread([&]()
        {
            auto rootPanel = ref new xaml_controls::Grid();
            rootPanel->Background = ref new SolidColorBrush(Microsoft::UI::Colors::White);
            VERIFY_IS_NOT_NULL(rootPanel);

            comboBox = ref new xaml_controls::ComboBox();
            VERIFY_IS_NOT_NULL(comboBox);
            comboBox->Width = 222;
            comboBox->Margin = xaml::ThicknessHelper::FromUniformLength(25);

            rootPanel->Children->Append(comboBox);
            TestServices::WindowHelper->WindowContent = rootPanel;

            for (auto i = 0; i < 5; i++)
            {
                auto item = ref new xaml_controls::ComboBoxItem();
                auto stringItem = ref new Platform::String(L"ComboBox Item ");
                stringItem += i;
                item->Content = stringItem;
                comboBox->Items->Append(item);
            }
        });

        TestServices::WindowHelper->WaitForIdle();

        ComboBoxHelper::OpenComboBox(comboBox, ComboBoxHelper::OpenMethod::Programmatic);
        ComboBoxHelper::VerifySelectedIndex(comboBox, -1);
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
        ComboBoxHelper::CloseComboBox(comboBox);
    }

    void ComboBoxIntegrationTests::CanInsertItemAfterExpandAndClose()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::ComboBox^ comboBox = nullptr;
        auto growingList = ref new Platform::Collections::Vector<Platform::String^>();

        RunOnUIThread([&]()
        {
            auto rootPanel = ref new xaml_controls::Grid();
            VERIFY_IS_NOT_NULL(rootPanel);

            comboBox = ref new xaml_controls::ComboBox();
            VERIFY_IS_NOT_NULL(comboBox);
            comboBox->Width = 222;
            comboBox->Margin = xaml::ThicknessHelper::FromUniformLength(25);

            rootPanel->Children->Append(comboBox);
            TestServices::WindowHelper->WindowContent = comboBox;

            growingList->Append(L"StartingItem");
            comboBox->ItemsSource = growingList;
            comboBox->SelectedIndex = 0;

            growingList->InsertAt(0, "Prepend");
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            comboBox->IsDropDownOpen = true;
            growingList->InsertAt(0, "Prepend2");
            comboBox->IsDropDownOpen = false;
            growingList->InsertAt(0, "Prepend3");
        });
    }

    void ComboBoxIntegrationTests::ValidateUIElementTree()
    {
        TestCleanupWrapper cleanup;
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 600));

        // We need a special rule for ComboBox to exclude IsSelectionBoxHighlighted in
        // addition to FocusState.
        auto validationRules = ref new Platform::String(
            LR"(<?xml version='1.0' encoding='UTF-8'?>
                <Rules>
                    <Rule Applicability=\"//Element[@Type='Microsoft.UI.Xaml.Controls.ComboBox']\" Inclusion='Blacklist'>
                        <Property Name='FocusState'/>
                        <Property Name='IsSelectionBoxHighlighted'/>
                    </Rule>
                </Rules>)");

        xaml_controls::ComboBox^ restComboBox = nullptr;
        xaml_controls::ComboBox^ pointerOverComboBox = nullptr;
        xaml_controls::ComboBox^ pressedComboBox = nullptr;
        xaml_controls::ComboBox^ focusedComboBox = nullptr;
        xaml_controls::ComboBox^ focusedPointerOverComboBox = nullptr;
        xaml_controls::ComboBox^ disabledComboBox = nullptr;

        xaml_controls::ComboBox^ openedComboBox = nullptr;
        xaml_controls::ComboBoxItem^ restUnselectedComboBoxItem = nullptr;
        xaml_controls::ComboBoxItem^ pointerOverUnselectedComboBoxItem = nullptr;
        xaml_controls::ComboBoxItem^ pressedUnselectedComboBoxItem = nullptr;
        xaml_controls::ComboBoxItem^ disabledUnselectedComboBoxItem = nullptr;

        xaml_controls::ComboBoxItem^ restSelectedComboBoxItem = nullptr;
        xaml_controls::ComboBoxItem^ pointerOverSelectedComboBoxItem = nullptr;
        xaml_controls::ComboBoxItem^ pressedSelectedComboBoxItem = nullptr;
        xaml_controls::ComboBoxItem^ focusedSelectedComboBoxItem = nullptr;
        xaml_controls::ComboBoxItem^ disabledSelectedComboBoxItem = nullptr;

#if WI_IS_FEATURE_PRESENT(Feature_HeaderPlacement)
        xaml_controls::ComboBox^ topHeaderComboBox = nullptr;
        xaml_controls::ComboBox^ leftHeaderComboBox = nullptr;
#endif

        xaml_controls::StackPanel^ rootPanel = nullptr;

        RunOnUIThread([&]()
        {
            rootPanel = ref new xaml_controls::StackPanel();

#if WI_IS_FEATURE_PRESENT(Feature_HeaderPlacement)
            topHeaderComboBox = ref new xaml_controls::ComboBox();
            topHeaderComboBox->Header = "Top Header";
            topHeaderComboBox->HeaderPlacement = xaml_controls::ControlHeaderPlacement::Top;
            topHeaderComboBox->PlaceholderText = "Placeholder";
            topHeaderComboBox->LightDismissOverlayMode = xaml_controls::LightDismissOverlayMode::Off;
            rootPanel->Children->Append(topHeaderComboBox);

            leftHeaderComboBox = ref new xaml_controls::ComboBox();
            leftHeaderComboBox->Header = "Left Header";
            leftHeaderComboBox->HeaderPlacement = xaml_controls::ControlHeaderPlacement::Left;
            leftHeaderComboBox->PlaceholderText = "Placeholder";
            leftHeaderComboBox->LightDismissOverlayMode = xaml_controls::LightDismissOverlayMode::Off;
            rootPanel->Children->Append(leftHeaderComboBox);
#endif

            restComboBox = ref new xaml_controls::ComboBox();
            restComboBox->Items->Append("Rest Combo Box");
            restComboBox->SelectedIndex = 0;
            restComboBox->LightDismissOverlayMode = xaml_controls::LightDismissOverlayMode::Off;
            rootPanel->Children->Append(restComboBox);

            pointerOverComboBox = ref new xaml_controls::ComboBox();
            pointerOverComboBox->Items->Append("Pointer Over Combo Box");
            pointerOverComboBox->SelectedIndex = 0;
            pointerOverComboBox->LightDismissOverlayMode = xaml_controls::LightDismissOverlayMode::Off;
            rootPanel->Children->Append(pointerOverComboBox);

            pressedComboBox = ref new xaml_controls::ComboBox();
            pressedComboBox->Items->Append("Pressed Combo Box");
            pressedComboBox->SelectedIndex = 0;
            pressedComboBox->LightDismissOverlayMode = xaml_controls::LightDismissOverlayMode::Off;
            rootPanel->Children->Append(pressedComboBox);

            focusedComboBox = ref new xaml_controls::ComboBox();
            focusedComboBox->Items->Append("Focused Combo Box");
            focusedComboBox->SelectedIndex = 0;
            focusedComboBox->LightDismissOverlayMode = xaml_controls::LightDismissOverlayMode::Off;
            rootPanel->Children->Append(focusedComboBox);

            focusedPointerOverComboBox = ref new xaml_controls::ComboBox();
            focusedPointerOverComboBox->Items->Append("Focused Pointer Over Combo Box");
            focusedPointerOverComboBox->SelectedIndex = 0;
            focusedPointerOverComboBox->LightDismissOverlayMode = xaml_controls::LightDismissOverlayMode::Off;
            rootPanel->Children->Append(focusedPointerOverComboBox);

            disabledComboBox = ref new xaml_controls::ComboBox();
            disabledComboBox->Items->Append("Disabled Combo Box");
            disabledComboBox->SelectedIndex = 0;
            disabledComboBox->LightDismissOverlayMode = xaml_controls::LightDismissOverlayMode::Off;
            disabledComboBox->IsEnabled = false;
            rootPanel->Children->Append(disabledComboBox);

            openedComboBox = ref new xaml_controls::ComboBox();
            openedComboBox->Items->Append("Opened Combo Box");
            openedComboBox->Margin = xaml::ThicknessHelper::FromLengths(0,350,0,0);
            openedComboBox->SelectedIndex = 0;
            openedComboBox->LightDismissOverlayMode = xaml_controls::LightDismissOverlayMode::Off;
            rootPanel->Children->Append(openedComboBox);

            restUnselectedComboBoxItem = ref new xaml_controls::ComboBoxItem();
            auto restUnselectedComboBoxItemContent = ref new xaml_controls::TextBlock();
            restUnselectedComboBoxItemContent->Text = "Rest Unselected ComboBoxItem";
            restUnselectedComboBoxItem->Content = restUnselectedComboBoxItemContent;
            openedComboBox->Items->Append(restUnselectedComboBoxItem);

            pointerOverUnselectedComboBoxItem = ref new xaml_controls::ComboBoxItem();
            auto pointerOverUnselectedComboBoxItemContent = ref new xaml_controls::TextBlock();
            pointerOverUnselectedComboBoxItemContent->Text = "Pointer Over Unselected ComboBoxItem";
            pointerOverUnselectedComboBoxItem->Content = pointerOverUnselectedComboBoxItemContent;
            openedComboBox->Items->Append(pointerOverUnselectedComboBoxItem);

            pressedUnselectedComboBoxItem = ref new xaml_controls::ComboBoxItem();
            auto pressedUnselectedComboBoxItemContent = ref new xaml_controls::TextBlock();
            pressedUnselectedComboBoxItemContent->Text = "Pressed Unselected ComboBoxItem";
            pressedUnselectedComboBoxItem->Content = pressedUnselectedComboBoxItemContent;
            openedComboBox->Items->Append(pressedUnselectedComboBoxItem);

            disabledUnselectedComboBoxItem = ref new xaml_controls::ComboBoxItem();
            auto disabledUnselectedComboBoxItemContent = ref new xaml_controls::TextBlock();
            disabledUnselectedComboBoxItemContent->Text = "Disabled Unselected ComboBoxItem";
            disabledUnselectedComboBoxItem->Content = disabledUnselectedComboBoxItemContent;
            disabledUnselectedComboBoxItem->IsEnabled = false;
            openedComboBox->Items->Append(disabledUnselectedComboBoxItem);

            restSelectedComboBoxItem = ref new xaml_controls::ComboBoxItem();
            auto restSelectedComboBoxItemContent = ref new xaml_controls::TextBlock();
            restSelectedComboBoxItemContent->Text = "Rest Selected ComboBoxItem";
            restSelectedComboBoxItem->Content = restSelectedComboBoxItemContent;
            restSelectedComboBoxItem->IsSelected = true;
            openedComboBox->Items->Append(restSelectedComboBoxItem);

            pointerOverSelectedComboBoxItem = ref new xaml_controls::ComboBoxItem();
            auto pointerOverSelectedComboBoxItemContent = ref new xaml_controls::TextBlock();
            pointerOverSelectedComboBoxItemContent->Text = "Pointer Over Selected ComboBoxItem";
            pointerOverSelectedComboBoxItem->Content = pointerOverSelectedComboBoxItemContent;
            pointerOverSelectedComboBoxItem->IsSelected = true;
            openedComboBox->Items->Append(pointerOverSelectedComboBoxItem);

            pressedSelectedComboBoxItem = ref new xaml_controls::ComboBoxItem();
            auto pressedSelectedComboBoxItemContent = ref new xaml_controls::TextBlock();
            pressedSelectedComboBoxItemContent->Text = "Pressed Selected ComboBoxItem";
            pressedSelectedComboBoxItem->Content = pressedSelectedComboBoxItemContent;
            pressedSelectedComboBoxItem->IsSelected = true;
            openedComboBox->Items->Append(pressedSelectedComboBoxItem);

            focusedSelectedComboBoxItem = ref new xaml_controls::ComboBoxItem();
            auto focusedSelectedComboBoxItemContent = ref new xaml_controls::TextBlock();
            focusedSelectedComboBoxItemContent->Text = "Focused Selected ComboBoxItem";
            focusedSelectedComboBoxItem->Content = focusedSelectedComboBoxItemContent;
            focusedSelectedComboBoxItem->IsSelected = true;
            openedComboBox->Items->Append(focusedSelectedComboBoxItem);

            disabledSelectedComboBoxItem = ref new xaml_controls::ComboBoxItem();
            auto disabledSelectedComboBoxItemContent = ref new xaml_controls::TextBlock();
            disabledSelectedComboBoxItemContent->Text = "Disabled Selected ComboBoxItem";
            disabledSelectedComboBoxItem->Content = disabledSelectedComboBoxItemContent;
            disabledSelectedComboBoxItem->IsSelected = true;
            disabledSelectedComboBoxItem->IsEnabled = false;
            openedComboBox->Items->Append(disabledSelectedComboBoxItem);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        ComboBoxHelper::OpenComboBox(openedComboBox, ComboBoxHelper::OpenMethod::Programmatic);
        TestServices::WindowHelper->WaitForIdle();

        auto setupValidationState = [&]()
        {
            RunOnUIThread([&]()
            {
                // ComboBox states
                VisualStateManager::GoToState(pointerOverComboBox, "PointerOver", false);
                VisualStateManager::GoToState(pressedComboBox, "Pressed", false);
                VisualStateManager::GoToState(focusedComboBox, "Focused", false);
                VisualStateManager::GoToState(focusedPointerOverComboBox, "Focused", false);
                VisualStateManager::GoToState(focusedPointerOverComboBox, "PointerOver", false);

                // ComboBoxItem unselected states
                VisualStateManager::GoToState(pointerOverUnselectedComboBoxItem, "PointerOver", false);
                VisualStateManager::GoToState(pressedUnselectedComboBoxItem, "Pressed", false);

                // ComboBoxItem selected states
                VisualStateManager::GoToState(restSelectedComboBoxItem, "Selected", false);
                VisualStateManager::GoToState(pointerOverSelectedComboBoxItem, "SelectedPointerOver", false);
                VisualStateManager::GoToState(pressedSelectedComboBoxItem, "SelectedPressed", false);
                VisualStateManager::GoToState(focusedSelectedComboBoxItem, "Focused", false);
            });
            TestServices::WindowHelper->WaitForIdle();
        };

        // Validate the Dark theme of controls.
        {
            setupValidationState();
            TestServices::Utilities->VerifyUIElementTreeWithRulesInline("Dark", validationRules);
        }

        // Validate the light theme of controls.
        {
            RunOnUIThread([&]()
            {
                rootPanel->RequestedTheme = xaml::ElementTheme::Light;
                rootPanel->Background = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::White);
            });
            TestServices::WindowHelper->WaitForIdle();

            setupValidationState();
            TestServices::Utilities->VerifyUIElementTreeWithRulesInline("Light", validationRules);
        }

        // Validate the high-contrast theme of controls.
        {
            RunOnUIThread([&]()
            {
                TestServices::ThemingHelper->HighContrastTheme = HighContrastTheme::Test;
                rootPanel->Background = ref new xaml_media::SolidColorBrush(mu::Colors::Black);
            });
            TestServices::WindowHelper->WaitForIdle();

            setupValidationState();

            // This method will turn on high-contrast mode before it does the validation.
            TestServices::Utilities->VerifyUIElementTreeWithRulesInline("HC", validationRules);

            LOG_OUTPUT(L"Validate High-Contrast Colors.");
            TestServices::Utilities->VerifyOutputFileHighContrastColors("HC", HighContrastTheme::Test);
        }
    }

    void ComboBoxIntegrationTests::ValidateVeryWideComboBoxItems()
    {
        TestCleanupWrapper cleanup;

        // Regression coverage: "ComboBox crashes Apps if text in them is wider than the screen resolution for mobile phones"
        // We were hitting an issue where a ComboBox that contained an item that was too wide to fit on the screen caused a layout cycle exception.
        // To test this we just open a ComboBox containing a very wide ComboBox item and verify that no exceptions get thrown.

        auto comboBox = SetupBasicComboBoxTest();

        auto veryLongString = ref new Platform::String(L"Human reason, in one sphere of its cognition, is called upon to consider questions, which it cannot decline, as they are presented by its own nature, but which it cannot answer, as they transcend every faculty of the mind.");

        RunOnUIThread([&]()
        {
            auto item = ref new xaml_controls::ComboBoxItem();
            item->Content = veryLongString;
            comboBox->Items->Append(item);
            comboBox->FontSize = 30;
        });
        TestServices::WindowHelper->WaitForIdle();

        ComboBoxHelper::OpenComboBox(comboBox, ComboBoxHelper::OpenMethod::Programmatic);
        TestServices::WindowHelper->WaitForIdle();

        ComboBoxHelper::CloseComboBox(comboBox);
        TestServices::WindowHelper->WaitForIdle();
    }

    void ComboBoxIntegrationTests::ValidateOpenComboBoxWithAllItemsLongWidth()
    {
        TestCleanupWrapper cleanup;

        // This is regression coverage for "ComboBox hits a layout cycle crash with having long item width in the all items.
        // This ComboBox items is the block shape that has all long item width instead of having one item width.
        // This is the different coverage with above ValidateVeryWideComboBoxItems() by having the all items long width.
        xaml_controls::ComboBox^ comboBox = nullptr;

        RunOnUIThread([&]()
        {
            auto rootPanel = ref new xaml_controls::Grid();
            comboBox = ref new xaml_controls::ComboBox();
            comboBox->VerticalAlignment = VerticalAlignment::Center;
            comboBox->Height = 100;
            rootPanel->Children->Append(comboBox);
            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"ValidateOpenComboBoxWithAllItemsLongWidth: Generate Items.");

        RunOnUIThread([&]()
        {
            // Create the long item
            auto stringItem = ref new Platform::String(L"ComboBox Item ");
            for (UINT j = 0; j < 200; j++)
            {
                stringItem += j;
            }

            for (UINT i = 0; i < 100; i++)
            {
                auto item = ref new xaml_controls::ComboBoxItem();
                item->Content = stringItem;
                comboBox->Items->Append(item);
            }
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"ValidateOpenComboBoxWithAllItemsLongWidth: Open ComboBox.");
        ComboBoxHelper::OpenComboBox(comboBox, ComboBoxHelper::OpenMethod::Touch);

        // Validate no layout crash by opening the ComboBox that has a long item width.

        LOG_OUTPUT(L"ValidateOpenComboBoxWithAllItemsLongWidth: Close ComboBox.");
        ComboBoxHelper::CloseComboBox(comboBox);
        TestServices::WindowHelper->WaitForIdle();
    }

    void ComboBoxIntegrationTests::DropDownClosesOnComboBoxUnloaded()
    {
        TestCleanupWrapper cleanup;

        // We need to verify that the DropDown closes when the ComboBox is removed from the visual tree.
        // If this does not happen, ComobBox is left in an invalid state and so if it gets added back
        // to the visual tree it does not behave correctly.

        auto comboBox = SetupBasicComboBoxTest();

        auto comboBoxClosedEvent = std::make_shared<Event>();
        auto closedRegistration = CreateSafeEventRegistration(xaml_controls::ComboBox, DropDownClosed);

        closedRegistration.Attach(comboBox, ref new wf::EventHandler<Platform::Object^>([comboBoxClosedEvent](Platform::Object^, Platform::Object^)
        {
            comboBoxClosedEvent->Set();
        }));

        ComboBoxHelper::OpenComboBox(comboBox, ComboBoxHelper::OpenMethod::Programmatic);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Panel^>(comboBox->Parent);
            rootPanel->Children->Clear();
        });

        comboBoxClosedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            VERIFY_IS_FALSE(comboBox->IsDropDownOpen);
        });
    }

    void ComboBoxIntegrationTests::EventsHandledOnGamepadB()
    {
        CloseComboBoxWithParentHandler("$d$_GamepadB#$u$_GamepadB", true /* expectedKeyDownHandledValue */, true /* verifyKeyDown */, true /* expectedKeyUpHandledValue */, true /* verifyKeyUp */);
    }

    void ComboBoxIntegrationTests::EventHandledOnEscape()
    {
        CloseComboBoxWithParentHandler("$d$_esc#$u$_esc", true /* expectedKeyDownHandledValue */, true /* verifyKeyDown */, true /* expectedKeyUpHandledValue */, true /* verifyKeyUp */);
    }

    void ComboBoxIntegrationTests::CloseComboBoxWithParentHandler(Platform::String^ keySequence, bool expectedKeyDownHandledValue, bool verifyKeyDown, bool expectedKeyUpHandledValue, bool verifyKeyUp)
    {
        TestCleanupWrapper cleanup;

        UIElement^ parent = nullptr;
        auto comboBox = SetupBasicComboBoxTest();
        auto comboBoxClosedEvent = std::make_shared<Event>();
        auto closedRegistration = CreateSafeEventRegistration(xaml_controls::ComboBox, DropDownClosed);
        auto comboBoxParentKeyDownRegistration = CreateSafeEventRegistrationForHandledEvents(Microsoft::UI::Xaml::UIElement, KeyDownEvent);
        auto comboBoxParentKeyUpRegistration = CreateSafeEventRegistrationForHandledEvents(Microsoft::UI::Xaml::UIElement, KeyUpEvent);

        RunOnUIThread([&]()
        {
            parent = dynamic_cast<UIElement^>(xaml_media::VisualTreeHelper::GetParent(comboBox));
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            if (verifyKeyDown)
            {
                comboBoxParentKeyDownRegistration.Attach(parent,
                    ref new xaml_input::KeyEventHandler([expectedKeyDownHandledValue](Platform::Object^ sender, xaml_input::KeyRoutedEventArgs^ e)
                {
                    VERIFY_ARE_EQUAL(e->Handled, expectedKeyDownHandledValue);
                }));
            }

            if (verifyKeyUp)
            {
                comboBoxParentKeyUpRegistration.Attach(parent,
                    ref new xaml_input::KeyEventHandler([expectedKeyUpHandledValue](Platform::Object^ sender, xaml_input::KeyRoutedEventArgs^ e)
                {
                    VERIFY_ARE_EQUAL(e->Handled, expectedKeyUpHandledValue);
                }));
            }

            closedRegistration.Attach(comboBox,
                ref new wf::EventHandler<Platform::Object^>([comboBoxClosedEvent](Platform::Object^, Platform::Object^)
            {
                comboBoxClosedEvent->Set();
            }));
        });

        ComboBoxHelper::OpenComboBox(comboBox, ComboBoxHelper::OpenMethod::Programmatic);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            comboBox->Focus(FocusState::Keyboard);
        });

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"ComboBox: Injecting key sequence");
        LOG_OUTPUT(L"%s", keySequence->Data());

        TestServices::KeyboardHelper->PressKeySequence(keySequence);

        TestServices::WindowHelper->WaitForIdle();
        comboBoxClosedEvent->WaitFor(std::chrono::milliseconds(8000)); // Known issue: input injection is taking too long

        RunOnUIThread([&]()
        {
            if (verifyKeyDown)
            {
                comboBoxParentKeyDownRegistration.Detach();
                comboBoxParentKeyDownRegistration.Attach(parent,
                    ref new xaml_input::KeyEventHandler([](Platform::Object^ sender, xaml_input::KeyRoutedEventArgs^ e)
                {
                    VERIFY_ARE_EQUAL(e->Handled, false);
                }));
            }

            if (verifyKeyUp)
            {
                comboBoxParentKeyUpRegistration.Detach();
                comboBoxParentKeyUpRegistration.Attach(parent,
                    ref new xaml_input::KeyEventHandler([](Platform::Object^ sender, xaml_input::KeyRoutedEventArgs^ e)
                {
                    VERIFY_ARE_EQUAL(e->Handled, false);
                }));
            }
        });

        LOG_OUTPUT(L"ComboBox: Injecting key sequence again to make sure no events are handled with the drop-down closed.");
        LOG_OUTPUT(L"%s", keySequence->Data());

        TestServices::KeyboardHelper->PressKeySequence(keySequence);

        TestServices::WindowHelper->WaitForIdle();
    }

    void ComboBoxIntegrationTests::EventsContinuallyHandledOnPressAndHoldGamepadB()
    {
        EventsContinuallyHandledOnPressAndHold("$d$_GamepadB", "$u$_GamepadB");
    }

    void ComboBoxIntegrationTests::EventsContinuallyHandledOnPressAndHold(Platform::String^ keyDownSequence, Platform::String^ keyUpSequence)
    {
        TestCleanupWrapper cleanup;

        UIElement^ parent = nullptr;
        auto comboBox = SetupBasicComboBoxTest();
        auto comboBoxClosedEvent = std::make_shared<Event>();
        auto comboBoxKeyDownRepeatedEvent = std::make_shared<Event>();
        auto closedRegistration = CreateSafeEventRegistration(xaml_controls::ComboBox, DropDownClosed);
        auto comboBoxParentKeyDownRegistration = CreateSafeEventRegistrationForHandledEvents(Microsoft::UI::Xaml::UIElement, KeyDownEvent);
        auto comboBoxParentKeyUpRegistration = CreateSafeEventRegistrationForHandledEvents(Microsoft::UI::Xaml::UIElement, KeyUpEvent);

        int keyDownCount = 0;

        RunOnUIThread([&]()
        {
            parent = dynamic_cast<UIElement^>(xaml_media::VisualTreeHelper::GetParent(comboBox));
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            comboBoxParentKeyDownRegistration.Attach(parent,
                ref new xaml_input::KeyEventHandler([&keyDownCount, comboBoxKeyDownRepeatedEvent](Platform::Object^ sender, xaml_input::KeyRoutedEventArgs^ e)
            {
                VERIFY_ARE_EQUAL(e->Handled, true);

                keyDownCount++;

                if (keyDownCount >= 2)
                {
                    comboBoxKeyDownRepeatedEvent->Set();
                }
            }));

            comboBoxParentKeyUpRegistration.Attach(parent,
                ref new xaml_input::KeyEventHandler([](Platform::Object^ sender, xaml_input::KeyRoutedEventArgs^ e)
            {
                VERIFY_ARE_EQUAL(e->Handled, true);
            }));

            closedRegistration.Attach(comboBox,
                ref new wf::EventHandler<Platform::Object^>([comboBoxClosedEvent](Platform::Object^, Platform::Object^)
            {
                comboBoxClosedEvent->Set();
            }));
        });

        ComboBoxHelper::OpenComboBox(comboBox, ComboBoxHelper::OpenMethod::Programmatic);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            comboBox->Focus(FocusState::Keyboard);
        });

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Injecting key-down sequence.");
        LOG_OUTPUT(L"%s", keyDownSequence->Data());

        TestServices::KeyboardHelper->PressKeySequence(keyDownSequence);

        TestServices::WindowHelper->WaitForIdle();
        comboBoxClosedEvent->WaitFor(std::chrono::milliseconds(8000)); // Known issue: input injection is taking too long

        LOG_OUTPUT(L"Injecting key-down sequence again to simulate the situation where the user is holding down the key.");
        LOG_OUTPUT(L"%s", keyDownSequence->Data());

        TestServices::KeyboardHelper->PressKeySequence(keyDownSequence);

        TestServices::WindowHelper->WaitForIdle();
        comboBoxKeyDownRepeatedEvent->WaitFor(std::chrono::milliseconds(8000));

        LOG_OUTPUT(L"Injecting key-up sequence.");
        LOG_OUTPUT(L"%s", keyUpSequence->Data());

        TestServices::KeyboardHelper->PressKeySequence(keyUpSequence);

        TestServices::WindowHelper->WaitForIdle();
    }

    void ComboBoxIntegrationTests::ValidateResettingSelectedIndexBringsBackPlaceholderText()
    {
        TestCleanupWrapper cleanup;

        auto comboBox = SetupBasicComboBoxTest();
        xaml_controls::TextBlock^ placeholderTextBlock = nullptr;

        RunOnUIThread([&]()
        {
            placeholderTextBlock = safe_cast<xaml_controls::TextBlock^>(TreeHelper::GetVisualChildByName(comboBox, L"PlaceholderTextBlock"));
        });

        LOG_OUTPUT(L"Open the ComboBox's drop down and set the selected index to 3, then close it.");
        ComboBoxHelper::OpenComboBox(comboBox, ComboBoxHelper::OpenMethod::Programmatic);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            comboBox->SelectedIndex = 3;
        });

        TestServices::WindowHelper->WaitForIdle();
        ComboBoxHelper::CloseComboBox(comboBox);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Since there's a selected item, the placeholder TextBlock should be transparent.");
            VERIFY_ARE_EQUAL(0, placeholderTextBlock->Opacity);
        });

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Now set the selected index to -1.");
            comboBox->SelectedIndex = -1;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Since there no longer is a selected item, the placeholder TextBlock should be visible.");
            VERIFY_ARE_EQUAL(1, placeholderTextBlock->Opacity);

            LOG_OUTPUT(L"Now set the selected index to 3.");
            comboBox->SelectedIndex = 3;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Since there is now a selected item again, the placeholder TextBlock should again be transparent.");
            VERIFY_ARE_EQUAL(0, placeholderTextBlock->Opacity);
        });
    }

    void ComboBoxIntegrationTests::CanSmoothlyScrollOpenComboBoxWithVariableWidthItems()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::ComboBox^ comboBox;
        xaml_controls::ScrollViewer^ scrollViewer;
        auto comboBoxOpenedEvent = std::make_shared<Event>();
        auto openedRegistration = CreateSafeEventRegistration(xaml_controls::ComboBox, DropDownOpened);
        auto verticalSnapPointsChangedRegistration = CreateSafeEventRegistration(xaml_controls::ItemsPresenter, VerticalSnapPointsChanged);

        RunOnUIThread([&]()
        {
            auto rootPanel = ref new xaml_controls::Grid();
            comboBox = ref new xaml_controls::ComboBox();
            comboBox->HorizontalAlignment = HorizontalAlignment::Center;
            comboBox->VerticalAlignment = VerticalAlignment::Center;

            openedRegistration.Attach(comboBox, ref new wf::EventHandler<Platform::Object^>([comboBoxOpenedEvent](Platform::Object^, Platform::Object^)
            {
                LOG_OUTPUT(L"ComboBox DropDownOpened event raised.");
                comboBoxOpenedEvent->Set();
            }));

            rootPanel->Children->Append(comboBox);
            TestServices::WindowHelper->WindowContent = rootPanel;

            std::mt19937 gen(2);
            std::uniform_real_distribution<> distr(0, 200);

            LOG_OUTPUT(L"Populating ComboBox...");
            for (int i = 0; i < 100; ++i)
            {
                auto item = ref new xaml_controls::ComboBoxItem();
                auto stringItem = ref new Platform::String(L"ComboBox Item ") + i;

                // Variable width items are important for this bug to repro.
                for (int j = 0; j < distr(gen); ++j)
                {
                    stringItem += L"_";
                }

                item->Content = stringItem;
                comboBox->Items->Append(item);
                comboBox->SelectedIndex = 0;
            }
        });

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Tapping ComboBox...");
        TestServices::InputHelper->Tap(comboBox);
        TestServices::WindowHelper->WaitForIdle();
        comboBoxOpenedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            auto popup = TreeHelper::GetVisualChildByType<xaml_primitives::Popup>(comboBox);
            VERIFY_IS_NOT_NULL(popup);

            auto popupChild = safe_cast<FrameworkElement^>(popup->Child);
            VERIFY_IS_NOT_NULL(popupChild);

            scrollViewer = TreeHelper::GetVisualChildByType<xaml_controls::ScrollViewer>(popupChild);
            VERIFY_IS_NOT_NULL(scrollViewer);

            auto itemsPresenter = TreeHelper::GetVisualChildByType<xaml_controls::ItemsPresenter>(scrollViewer);
            VERIFY_IS_NOT_NULL(itemsPresenter);

            verticalSnapPointsChangedRegistration.Attach(
                itemsPresenter,
                ref new wf::EventHandler<Platform::Object^>([](Platform::Object^, Platform::Object^)
            {
                VERIFY_FAIL(L"VerticalSnapPointsChanged event should not be raised.");
            }));
        });

        for (int i = 0; i < 10; ++i)
        {
            LOG_OUTPUT(L"Flicking ComboBox's dropdown...");
            TestServices::InputHelper->Flick(scrollViewer, FlickDirection::North);
        }

        LOG_OUTPUT(L"Closing ComboBox's dropdown...");
        ComboBoxHelper::CloseComboBox(comboBox);
    }

    xaml_controls::ComboBox^ ComboBoxIntegrationTests::SetupBasicComboBoxTest(UINT numberOfItems, bool adjustMargin, bool isEditable)
    {
        xaml_controls::ComboBox^ comboBox = nullptr;

        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::ComboBox, Loaded);

        RunOnUIThread([&]()
        {
            comboBox = ref new xaml_controls::ComboBox();
            comboBox->Width = 222;
            comboBox->Margin = xaml::ThicknessHelper::FromUniformLength(25);

            loadedRegistration.Attach(comboBox, [loadedEvent]()
            {
                LOG_OUTPUT(L"ComboBox.Loaded event raised.");
                loadedEvent->Set();
            });

            auto rootPanel = ref new xaml_controls::Grid();
            if (adjustMargin)
            {
                rootPanel->Margin = xaml::ThicknessHelper::FromLengths(0,25,0,0);
            }
            rootPanel->Children->Append(comboBox);
            TestServices::WindowHelper->WindowContent = rootPanel;

            for (UINT i = 0; i < numberOfItems; i++)
            {
                auto item = ref new xaml_controls::ComboBoxItem();
                auto stringItem = ref new Platform::String(L"ComboBox Item ");
                stringItem += i;
                item->Content = stringItem;
                comboBox->Items->Append(item);
            }

            if (isEditable)
            {
                comboBox->IsEditable = true;
            }
        });

        LOG_OUTPUT(L"Waiting for ComboBox.Loaded event...");
        loadedEvent->WaitForDefault();

        TestServices::WindowHelper->WaitForIdle();

        return comboBox;
    }

    // For these purposes, an "Ascending combobox" is a box that has data in a triangular pattern.
    // This forces us to resize the combobox to get bigger every time we navigate down since we realize wider items.
    // It resembles the following:
    // X
    // XX
    // XXX
    // XXXX
    xaml_controls::ComboBox^ ComboBoxIntegrationTests::SetupAscendingComboBoxTest(UINT numberOfItems)
    {
        xaml_controls::ComboBox^ comboBox = nullptr;

        RunOnUIThread([&]()
        {
            auto rootPanel = ref new xaml_controls::Grid();
            VERIFY_IS_NOT_NULL(rootPanel);

            comboBox = ref new xaml_controls::ComboBox();
            VERIFY_IS_NOT_NULL(comboBox);

            rootPanel->Children->Append(comboBox);
            TestServices::WindowHelper->WindowContent = rootPanel;

            for (UINT i = 0; i < numberOfItems; i++)
            {
                auto item = ref new xaml_controls::ComboBoxItem();

                auto stringItem = ref new Platform::String(L"");
                for (UINT j = 1; j <= i + 1; j++)
                {
                    stringItem += "X";
                }

                item->Content = stringItem;
                comboBox->Items->Append(item);
            }
        });

        TestServices::WindowHelper->WaitForIdle();

        return comboBox;
    }

    void ComboBoxIntegrationTests::CanOpenWithTouch()
    {
        TestCleanupWrapper cleanup;

        auto comboBox = SetupBasicComboBoxTest(20 /* itemSize */);

        RunOnUIThread([&]()
        {
            comboBox->Height = 50;
            comboBox->SelectedIndex = 1;
        });

        TestServices::WindowHelper->WaitForIdle();

        auto comboBoxOpenedEvent = std::make_shared<Event>();
        auto openedRegistration = CreateSafeEventRegistration(xaml_controls::ComboBox, DropDownOpened);

        openedRegistration.Attach(comboBox, ref new wf::EventHandler<Platform::Object^>([comboBoxOpenedEvent](Platform::Object^, Platform::Object^)
        {
            LOG_OUTPUT(L"CanOpenWithTouch: Fire the ComboBox Opened event!");
            comboBoxOpenedEvent->Set();
        }));

        LOG_OUTPUT(L"CanOpenWithTouch: Open the ComboBox with touch input.");
        TestServices::InputHelper->Tap(comboBox);

        TestServices::WindowHelper->WaitForIdle();
        comboBoxOpenedEvent->WaitForDefault();
        ComboBoxHelper::VerifySelectedIndex(comboBox, 1);
        ComboBoxHelper::CloseComboBox(comboBox);

        RunOnUIThread([&]()
        {
            comboBox->SelectedIndex = 19;
        });

        LOG_OUTPUT(L"CanOpenWithTouch: Open the ComboBox with touch input.");
        TestServices::InputHelper->Tap(comboBox);

        TestServices::WindowHelper->WaitForIdle();
        comboBoxOpenedEvent->WaitForDefault();
        ComboBoxHelper::VerifySelectedIndex(comboBox, 19);
        ComboBoxHelper::CloseComboBox(comboBox);
    }

    void ComboBoxIntegrationTests::ValidateKeyboardInteraction()
    {
        TestCleanupWrapper cleanup;

        auto comboBox = SetupBasicComboBoxTest();

        auto dropDownOpenedEvent = std::make_shared<Event>();
        auto dropDownOpenedRegistration = CreateSafeEventRegistration(xaml_controls::ComboBox, DropDownOpened);
        dropDownOpenedRegistration.Attach(comboBox, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^, Platform::Object^){
            dropDownOpenedEvent->Set();
        }));

        auto dropDownClosedEvent = std::make_shared<Event>();
        auto dropDownClosedRegistration = CreateSafeEventRegistration(xaml_controls::ComboBox, DropDownClosed);
        dropDownClosedRegistration.Attach(comboBox, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^, Platform::Object^){
            dropDownClosedEvent->Set();
        }));

        RunOnUIThread([&]()
        {
            comboBox->SelectionChangedTrigger = xaml_controls::ComboBoxSelectionChangedTrigger::Always;
            comboBox->Focus(xaml::FocusState::Programmatic);
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Open the ComboBox with the Space key");
        TestServices::KeyboardHelper->PressKeySequence(L" ");
        TestServices::WindowHelper->WaitForIdle();
        dropDownOpenedEvent->WaitForDefault();

        ComboBoxHelper::VerifySelectedIndex(comboBox, 0);

        LOG_OUTPUT(L"Select the second item with the Down key");
        TestServices::KeyboardHelper->Down();
        TestServices::WindowHelper->WaitForIdle();
        ComboBoxHelper::VerifySelectedIndex(comboBox, 1);

        LOG_OUTPUT(L"Select the third item with the Down key");
        TestServices::KeyboardHelper->Down();
        TestServices::WindowHelper->WaitForIdle();
        ComboBoxHelper::VerifySelectedIndex(comboBox, 2);

        // We verify ComboBoxItem::GotFocus and ComboBoxItem::LostFocus on the 4th ComboBoxItem.
        xaml_controls::ComboBoxItem^ comboBoxItem3 = nullptr;
        RunOnUIThread([&]()
        {
            comboBoxItem3 = safe_cast<xaml_controls::ComboBoxItem^>(comboBox->ContainerFromIndex(3));
        });

        auto comboBoxItem3gotFocusEvent = std::make_shared<Event>();
        auto comboBoxItem3gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::ComboBoxItem, GotFocus);
        comboBoxItem3gotFocusRegistration.Attach(comboBoxItem3, ref new xaml::RoutedEventHandler([&](Platform::Object^, xaml::IRoutedEventArgs^){
            comboBoxItem3gotFocusEvent->Set();
        }));

        auto comboBoxItem3lostFocusEvent = std::make_shared<Event>();
        auto comboBoxItem3lostFocusRegistration = CreateSafeEventRegistration(xaml_controls::ComboBoxItem, LostFocus);
        comboBoxItem3lostFocusRegistration.Attach(comboBoxItem3, ref new xaml::RoutedEventHandler([&](Platform::Object^, xaml::IRoutedEventArgs^){
            comboBoxItem3lostFocusEvent->Set();
        }));

        LOG_OUTPUT(L"Change selection with the Down key");
        TestServices::KeyboardHelper->Down();
        comboBoxItem3gotFocusEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
        ComboBoxHelper::VerifySelectedIndex(comboBox, 3);

        LOG_OUTPUT(L"Change selection with the Up key");
        TestServices::KeyboardHelper->Up();
        comboBoxItem3lostFocusEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
        ComboBoxHelper::VerifySelectedIndex(comboBox, 2);

        LOG_OUTPUT(L"Accept selection with the Space key");
        TestServices::KeyboardHelper->PressKeySequence(L" ");
        TestServices::WindowHelper->WaitForIdle();
        dropDownClosedEvent->WaitForDefault();

        ComboBoxHelper::VerifySelectedIndex(comboBox, 2);
    }

    void ComboBoxIntegrationTests::CanCloseComboBoxWithAltDown()
    {
        TestCleanupWrapper cleanup;
        CanCloseComboBoxWithKeySequence(ref new Platform::String(L"$d$_alt#$d$_down#$u$_down#$u$_alt"));
    }

    void ComboBoxIntegrationTests::CanCloseComboBoxWithF4()
    {
        TestCleanupWrapper cleanup;
        CanCloseComboBoxWithKeySequence(ref new Platform::String(L"$d$_f4#$u$_f4"));
    }

    void ComboBoxIntegrationTests::CanCloseComboBoxWithKeySequence(Platform::String^ keySequence)
    {
        auto comboBox = SetupBasicComboBoxTest();

        auto dropDownClosedEvent = std::make_shared<Event>();
        auto dropDownClosedRegistration = CreateSafeEventRegistration(xaml_controls::ComboBox, DropDownClosed);
        dropDownClosedRegistration.Attach(comboBox, [&](){dropDownClosedEvent->Set(); });

        RunOnUIThread([&]()
        {
            comboBox->Focus(xaml::FocusState::Programmatic);
            comboBox->IsDropDownOpen = true;
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Close ComboBox using specified key sequence");
        TestServices::KeyboardHelper->PressKeySequence(keySequence);
        TestServices::WindowHelper->WaitForIdle();
        dropDownClosedEvent->WaitForDefault();

        ComboBoxHelper::VerifySelectedIndex(comboBox, -1);
    }

    void ComboBoxIntegrationTests::CanCycleThroughItemsWithMouseWheel()
    {
        TestCleanupWrapper cleanup;

        auto comboBox = SetupBasicComboBoxTest();

        RunOnUIThread([&]()
        {
            comboBox->Focus(xaml::FocusState::Programmatic);
        });
        TestServices::WindowHelper->WaitForIdle();

        ComboBoxHelper::VerifySelectedIndex(comboBox, -1);

        TestServices::InputHelper->MoveMouse(comboBox);
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Select the first item with the mouse wheel");
        TestServices::InputHelper->ScrollMouseWheel(comboBox, -1);
        TestServices::WindowHelper->WaitForIdle();
        ComboBoxHelper::VerifySelectedIndex(comboBox, 0);

        LOG_OUTPUT(L"Select the second item with the mouse wheel");
        TestServices::InputHelper->ScrollMouseWheel(comboBox, -1);
        TestServices::WindowHelper->WaitForIdle();
        ComboBoxHelper::VerifySelectedIndex(comboBox, 1);

        LOG_OUTPUT(L"Select the first item again by scrolling the mouse wheel back the other way");
        TestServices::InputHelper->ScrollMouseWheel(comboBox, 1);
        TestServices::WindowHelper->WaitForIdle();
        ComboBoxHelper::VerifySelectedIndex(comboBox, 0);
    }

    void ComboBoxIntegrationTests::CanLightDismissDropdown()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::ComboBox^ comboBox = nullptr;
        xaml_controls::Button^ button = nullptr;

        RunOnUIThread([&]()
        {
            comboBox = ref new xaml_controls::ComboBox();

            for (size_t i = 0; i < 5; ++i)
            {
                auto item = ref new xaml_controls::ComboBoxItem;
                item->Content = "Item";
                comboBox->Items->Append(item);
            }

            button = ref new xaml_controls::Button;
            button->Width = 222;
            // Add a top margin to push the button out from under the status bar on phone.
            button->Margin = xaml::ThicknessHelper::FromLengths(0, 32, 0, 0);

            auto root = ref new xaml_controls::StackPanel();
            root->Children->Append(button);
            root->Children->Append(comboBox);

            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();

        auto dropDownClosedEvent = std::make_shared<Event>();
        auto dropDownClosedRegistration = CreateSafeEventRegistration(xaml_controls::ComboBox, DropDownClosed);
        dropDownClosedRegistration.Attach(comboBox, [&](){dropDownClosedEvent->Set(); });

        RunOnUIThread([&]()
        {
            comboBox->Focus(xaml::FocusState::Programmatic);
            comboBox->IsDropDownOpen = true;
        });
        TestServices::WindowHelper->WaitForIdle();

        // Click the button and see if the drop down light dismisses
        TestServices::InputHelper->Tap(button);
        TestServices::WindowHelper->WaitForIdle();
        dropDownClosedEvent->WaitForDefault();
    }

    void ComboBoxIntegrationTests::ValidateResetItemsSource()
    {
        TestCleanupWrapper cleanup;

        auto comboBox = SetupBasicComboBoxTest();

        // Reset the ComboBox ItemsSource with new items
        RunOnUIThread([&]()
        {
            comboBox->SelectedIndex = 0;

            auto itemList = ref new Platform::Collections::Vector<Platform::String^>();

            for (int i = 0; i < 5; i++)
            {
                Platform::String^ itemText = L"Item " + i;
                itemList->Append(itemText);
            }
            comboBox->ItemsSource = itemList;
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"ValidateResetItemsSource: Open and Close ComboBox.");

        ComboBoxHelper::OpenComboBox(comboBox, ComboBoxHelper::OpenMethod::Programmatic);
        TestServices::WindowHelper->WaitForIdle();

        ComboBoxHelper::CloseComboBox(comboBox);
        TestServices::WindowHelper->WaitForIdle();

        // The selected index must be -1 by reset ItemsSource
        ComboBoxHelper::VerifySelectedIndex(comboBox, -1);
    }

    void ComboBoxIntegrationTests::ValidateDropdownPlacement()
    {
        TestCleanupWrapper cleanup;

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 600));

        // This test validates that ComboBox can open its Popup outside of the bounds of the window.
        // The test relies on the DoValidateDropdownPlacement helper function to do the validation, passing in various combinations of params to test.
        //
        // All combinations of values for the parameter are valid test cases, but we do not want to execute the full set of all possible combinations (2^5=32 test cases).
        // Instead we:
        //   1. Explicitly test the specific scenarios that we are particularly interested in
        //   2. Ensure pairwise coverage in the parameters by adding a set of test cases with parameters generated by PICT

        // If a ComboBox is near the Right edge of the window and the ComboBoxItems are wider than the ComboBox, the popup position should be allowed to go outside the window:
        DoValidateDropdownPlacement(xaml::VerticalAlignment::Top,    xaml::HorizontalAlignment::Right, true  /*useWideItems*/, 0   /*rotationAngle*/, xaml::FlowDirection::LeftToRight, PopupHelper::AreWindowedPopupsEnabled() /* expectOutside */);

        // If a ComboBox is near the Bottom of the window, the popup position should be allowed to go outside the window:
        DoValidateDropdownPlacement(xaml::VerticalAlignment::Bottom, xaml::HorizontalAlignment::Right, false /*useWideItems*/, 0   /*rotationAngle*/, xaml::FlowDirection::LeftToRight, PopupHelper::AreWindowedPopupsEnabled() /* expectOutside */);

        // ComboBox popup placement should correctly handle RightToLeft flow direction:
        DoValidateDropdownPlacement(xaml::VerticalAlignment::Top,    xaml::HorizontalAlignment::Left,  false /*useWideItems*/, 0   /*rotationAngle*/, xaml::FlowDirection::RightToLeft, PopupHelper::AreWindowedPopupsEnabled() /* expectOutside */);

        // Validate cases where a parent of the ComboBox has a RotateTransform of 90, 180 or 270 degrees.
        // Note: ComboBox does not handle rotation angles other than multiples of 90, so we only test these supported values.
        DoValidateDropdownPlacement(xaml::VerticalAlignment::Bottom, xaml::HorizontalAlignment::Left,  false /*useWideItems*/, 90  /*rotationAngle*/, xaml::FlowDirection::LeftToRight, false /* expectOutside */);
        DoValidateDropdownPlacement(xaml::VerticalAlignment::Bottom, xaml::HorizontalAlignment::Left,  false /*useWideItems*/, 180 /*rotationAngle*/, xaml::FlowDirection::LeftToRight, false /* expectOutside */);
        DoValidateDropdownPlacement(xaml::VerticalAlignment::Top,    xaml::HorizontalAlignment::Left,  false /*useWideItems*/, 270 /*rotationAngle*/, xaml::FlowDirection::LeftToRight, false /* expectOutside */);

        // Extra scenarios from PICT:
        DoValidateDropdownPlacement(xaml::VerticalAlignment::Bottom, xaml::HorizontalAlignment::Right,  true  /*useWideItems*/, 180 /*rotationAngle*/, xaml::FlowDirection::RightToLeft, false /* expectOutside */);
        DoValidateDropdownPlacement(xaml::VerticalAlignment::Top,    xaml::HorizontalAlignment::Left,   true  /*useWideItems*/, 180 /*rotationAngle*/, xaml::FlowDirection::LeftToRight, false /* expectOutside */);
        DoValidateDropdownPlacement(xaml::VerticalAlignment::Bottom, xaml::HorizontalAlignment::Right,  true  /*useWideItems*/, 270 /*rotationAngle*/, xaml::FlowDirection::RightToLeft, false /* expectOutside */);
        DoValidateDropdownPlacement(xaml::VerticalAlignment::Top,    xaml::HorizontalAlignment::Right,  true  /*useWideItems*/, 90  /*rotationAngle*/, xaml::FlowDirection::RightToLeft, false /* expectOutside */);
    }

    void ComboBoxIntegrationTests::DoValidateDropdownPlacement(xaml::VerticalAlignment verticalAlignment, xaml::HorizontalAlignment horizontalAlignment, bool useWideItems, int rotationAngle, xaml::FlowDirection flowDirection, bool expectOutside)
    {
        LOG_OUTPUT(L"DoValidateDropdownPlacement: VerticalAlignment=%s, HorizontalAlignment=%s, useWideItems=%d, rotationAngle=%d, FlowDirection=%s ", verticalAlignment.ToString()->Data(), horizontalAlignment.ToString()->Data(), useWideItems, rotationAngle, flowDirection.ToString()->Data());

        xaml_controls::ComboBox^ comboBox;
        xaml_controls::Grid^ rootPanel;

        RunOnUIThread([&]()
        {
            // Note: The ComboBox container needs a 6px Margin applied to it due to:
            // "ComboBox dropdown can be placed slightly offscreen when ComboBox is near edge of window".
            //
            // We set ComboBox.SelectedIndex=2 so that there are items both above and below the selected item
            // (ComboBox tries to open the popup with the selected item centered over the ComboBox).
            rootPanel = safe_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                LR"(<Grid xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" >
                        <Grid x:Name="comboBoxContainer" Width="300" Height="300" Background="Green" Margin="8" RenderTransformOrigin="0.5, 0.5"  >
                            <Grid.RenderTransform>
                                <RotateTransform Angle="0" />
                            </Grid.RenderTransform>
                            <ComboBox x:Name="comboBox" Width="300" SelectedIndex="2" >
                                <ComboBoxItem Content="item one" />
                                <ComboBoxItem Content="item two" />
                                <ComboBoxItem Content="item three" />
                                <ComboBoxItem Content="item four" />
                                <ComboBoxItem Content="item five" />
                            </ComboBox>
                        </Grid>
                    </Grid>)"));

            comboBox = safe_cast<xaml_controls::ComboBox^>(rootPanel->FindName(L"comboBox"));
            comboBox->HorizontalAlignment = horizontalAlignment;
            comboBox->VerticalAlignment = verticalAlignment;

            auto comboBoxContainer = safe_cast<xaml_controls::Grid^>(rootPanel->FindName(L"comboBoxContainer"));
            comboBoxContainer->HorizontalAlignment = horizontalAlignment;
            comboBoxContainer->VerticalAlignment = verticalAlignment;

            auto rotateTransform = safe_cast<xaml_media::RotateTransform^>(comboBoxContainer->RenderTransform);
            rotateTransform->Angle = rotationAngle;

            if (useWideItems)
            {
                // We set the ComboBoxItems to a width wider than the ComboBox:
                for (auto&& item : comboBox->Items)
                {
                    auto comboBoxItem = safe_cast<xaml_controls::ComboBoxItem^>(static_cast<Platform::Object^>(item));
                    comboBoxItem->Width = 380;
                }
            }

            rootPanel->FlowDirection = flowDirection;

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        ComboBoxHelper::OpenComboBox(comboBox, ComboBoxHelper::OpenMethod::Programmatic);

        RunOnUIThread([&]()
        {
            auto dropdownScrollViewer = safe_cast<xaml_controls::ScrollViewer^>(TreeHelper::GetVisualChildByNameFromOpenPopups(L"ScrollViewer", rootPanel));
            wf::Rect dropdownBounds = ControlHelper::GetBounds(dropdownScrollViewer);
            wf::Rect rootBounds = ControlHelper::GetBounds(rootPanel);

            LOG_OUTPUT(L"dropdownBounds: (%f, %f, %f, %f)", dropdownBounds.X, dropdownBounds.Y, dropdownBounds.Width, dropdownBounds.Height);
            LOG_OUTPUT(L"rootBounds:     (%f, %f, %f, %f)", rootBounds.X, rootBounds.Y, rootBounds.Width, rootBounds.Height);

            if (expectOutside)
            {
                // We still expect the ComboBox to be contained within the monitor bounds.
                wf::Point windowPosition = TestServices::WindowHelper->ConvertToPhysicalDisplayLocation({ 0, 0 });
                wf::Rect monitorBounds = TestServices::WindowHelper->MonitorBounds;
                wf::Rect physicalDropdownBounds = dropdownBounds;

                // We need to convert the drop-down bounds to screen coordinates here in order to
                // be able to compare them to the monitor bounds.
                physicalDropdownBounds.X += windowPosition.X;
                physicalDropdownBounds.Y += windowPosition.Y;

                LOG_OUTPUT(L"windowPosition:         (%f, %f)", windowPosition.X, windowPosition.Y);
                LOG_OUTPUT(L"physicalDropdownBounds: (%f, %f, %f, %f)", physicalDropdownBounds.X, physicalDropdownBounds.Y, physicalDropdownBounds.Width, physicalDropdownBounds.Height);
                LOG_OUTPUT(L"monitorBounds:          (%f, %f, %f, %f)", monitorBounds.X, monitorBounds.Y, monitorBounds.Width, monitorBounds.Height);

                VERIFY_IS_FALSE(ControlHelper::IsContainedIn(dropdownBounds, rootBounds));
                VERIFY_IS_TRUE(ControlHelper::IsContainedIn(physicalDropdownBounds, monitorBounds));
            }
            else
            {
                VERIFY_IS_TRUE(ControlHelper::IsContainedIn(dropdownBounds, rootBounds));
            }
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void ComboBoxIntegrationTests::DoesGetFocusWhenProgrammaticallyOpened()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::ComboBox^ comboBox = nullptr;
        xaml_controls::ComboBoxItem^ firstComboBoxItem = nullptr;
        xaml_controls::Button^ button = nullptr;

        RunOnUIThread([&]()
        {
            comboBox = ref new xaml_controls::ComboBox();

            for (size_t i = 0; i < 5; ++i)
            {
                auto item = ref new xaml_controls::ComboBoxItem;
                item->Content = "Item";
                comboBox->Items->Append(item);
            }

            firstComboBoxItem = safe_cast<xaml_controls::ComboBoxItem^>(comboBox->Items->GetAt(0));

            button = ref new xaml_controls::Button;
            button->VerticalAlignment = xaml::VerticalAlignment::Top;

            auto root = ref new xaml_controls::Grid();
            root->Children->Append(button);
            root->Children->Append(comboBox);

            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // Explicitly set focus to a button to make sure ComboBox
            // is actually grabbing focus rather than just starting
            // with focus.
            button->Focus(xaml::FocusState::Programmatic);

            comboBox->IsDropDownOpen = true;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(firstComboBoxItem));
        });
    }

    void ComboBoxIntegrationTests::CanNavigateAscendingComboBoxesWithGamepad()
    {
        CanNavigateAscendingComboBoxes(InputDevice::Gamepad);
    }

    void ComboBoxIntegrationTests::CanNavigateAscendingComboBoxesWithKeyboard()
    {
        CanNavigateAscendingComboBoxes(InputDevice::Keyboard);
    }

    void ComboBoxIntegrationTests::CanNavigateAscendingComboBoxes(InputDevice device)
    {
        TestCleanupWrapper cleanup;

        auto comboBox = SetupAscendingComboBoxTest();
        RunOnUIThread([&]()
        {
            comboBox->SelectedIndex = 0;
        });
        TestServices::WindowHelper->WaitForIdle();

        auto dropDownOpenedEvent = std::make_shared<Event>();
        auto dropDownOpenedRegistration = CreateSafeEventRegistration(xaml_controls::ComboBox, DropDownOpened);
        dropDownOpenedRegistration.Attach(comboBox, [&](){ dropDownOpenedEvent->Set(); });

        auto dropDownClosedEvent = std::make_shared<Event>();
        auto dropDownClosedRegistration = CreateSafeEventRegistration(xaml_controls::ComboBox, DropDownClosed);
        dropDownClosedRegistration.Attach(comboBox, [&](){ dropDownClosedEvent->Set(); });

        RunOnUIThread([&]()
        {
            comboBox->Focus(xaml::FocusState::Programmatic);
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Open the ComboBox with the accept button.");
        CommonInputHelper::Accept(device);

        TestServices::WindowHelper->WaitForIdle();
        dropDownOpenedEvent->WaitForDefault();

        ComboBoxHelper::VerifySelectedIndex(comboBox, 0);

        for (int i = 0; i < 10; i++)
        {
            LOG_OUTPUT(L"Move focus down.");
            CommonInputHelper::Down(device);

            TestServices::WindowHelper->WaitForIdle();
        }

        LOG_OUTPUT(L"Close the ComboBox with the accept button.");
        CommonInputHelper::Accept(device);

        dropDownClosedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
        ComboBoxHelper::VerifySelectedIndex(comboBox, 10);
    }

    void ComboBoxIntegrationTests::SelectionChangedIsNotRaisedUntilClose()
    {
        TestCleanupWrapper cleanup;

        InputDevice device = InputDevice::Gamepad;

        auto comboBox = SetupBasicComboBoxTest();

        auto dropDownOpenedEvent = std::make_shared<Event>();
        auto dropDownOpenedRegistration = CreateSafeEventRegistration(xaml_controls::ComboBox, DropDownOpened);
        dropDownOpenedRegistration.Attach(comboBox, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^, Platform::Object^){
            dropDownOpenedEvent->Set();
        }));

        auto dropDownClosedEvent = std::make_shared<Event>();
        auto dropDownClosedRegistration = CreateSafeEventRegistration(xaml_controls::ComboBox, DropDownClosed);
        dropDownClosedRegistration.Attach(comboBox, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^, Platform::Object^){
            dropDownClosedEvent->Set();
        }));

        RunOnUIThread([&]()
        {
            comboBox->Focus(xaml::FocusState::Programmatic);
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Open the ComboBox with the accept button.");
        CommonInputHelper::Accept(device);

        TestServices::WindowHelper->WaitForIdle();
        dropDownOpenedEvent->WaitForDefault();

        ComboBoxHelper::VerifySelectedIndex(comboBox, -1);

        LOG_OUTPUT(L"Focus the second item with down button.");
        CommonInputHelper::Down(device);

        TestServices::WindowHelper->WaitForIdle();
        ComboBoxHelper::VerifySelectedIndex(comboBox, -1);

        LOG_OUTPUT(L"Focus the third item with down button.");
        CommonInputHelper::Down(device);

        TestServices::WindowHelper->WaitForIdle();
        ComboBoxHelper::VerifySelectedIndex(comboBox, -1);

        LOG_OUTPUT(L"Close the ComboBox with the cancel button. No selection change should have happened.");
        CommonInputHelper::Cancel(device);

        dropDownClosedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
        ComboBoxHelper::VerifySelectedIndex(comboBox, -1);

        LOG_OUTPUT(L"Open the ComboBox with the accept button.");
        CommonInputHelper::Accept(device);

        TestServices::WindowHelper->WaitForIdle();
        dropDownOpenedEvent->WaitForDefault();

        LOG_OUTPUT(L"Focus the second item with down button.");
        CommonInputHelper::Down(device);

        TestServices::WindowHelper->WaitForIdle();
        ComboBoxHelper::VerifySelectedIndex(comboBox, -1);

        LOG_OUTPUT(L"Focus the third item with down button.");
        CommonInputHelper::Down(device);

        TestServices::WindowHelper->WaitForIdle();
        ComboBoxHelper::VerifySelectedIndex(comboBox, -1);

        LOG_OUTPUT(L"Close the ComboBox with the accept button.");
        CommonInputHelper::Accept(device);

        dropDownClosedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
        ComboBoxHelper::VerifySelectedIndex(comboBox, 2);
    }

    void ComboBoxIntegrationTests::ValidateDropdownSizingForDifferentInputModes()
    {
        TestCleanupWrapper cleanup;

        const double expectedDropdownWidth_Touch = 240;
        const double expectedDropdownWidth_NonTouch = 80;
        const double expectedComboBoxItemHeight_Touch = 40;
        const double expectedComboBoxItemHeight_NonTouch = 32;
        const xaml::Thickness expectedDropdownContentMargin_TouchAndCarousel = xaml::Thickness({ 0, 0, 0, 0 });
        const xaml::Thickness expectedDropdownContentMargin_NonTouchAndCarousel = xaml::Thickness({ 0, 4, 0, 4 });

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 600));

        xaml_controls::ComboBox^ comboBox;
        xaml_controls::ComboBoxItem^ comboBoxItem;
        xaml::FrameworkElement^ comboBoxDropdownRoot;
        xaml_controls::ItemsPresenter^ comboBoxDropdownItemsPresenter;

        RunOnUIThread([&]()
        {
            auto root = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                LR"(<Grid xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
                          xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" >
                        <ComboBox x:Name="comboBox" Height="100" >
                            <ComboBoxItem x:Name="comboBoxItem" Content="i0" />
                        </ComboBox>
                    </Grid>)"));

            comboBox = safe_cast<xaml_controls::ComboBox^>(root->FindName(L"comboBox"));
            comboBoxItem = safe_cast<xaml_controls::ComboBoxItem^>(root->FindName(L"comboBoxItem"));

            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();

        // Open via Touch.
        ComboBoxHelper::OpenComboBox(comboBox, ComboBoxHelper::OpenMethod::Touch);
        RunOnUIThread([&]()
        {
            comboBoxDropdownRoot = safe_cast<xaml::FrameworkElement^>(TreeHelper::GetVisualChildByNameFromOpenPopups(L"ScrollViewer", comboBox));
            comboBoxDropdownItemsPresenter = TreeHelper::GetVisualChildByType<xaml_controls::ItemsPresenter>(comboBoxDropdownRoot);
            VERIFY_IS_NOT_NULL(comboBoxDropdownRoot);
            VERIFY_IS_NOT_NULL(comboBoxDropdownItemsPresenter);

            VERIFY_ARE_EQUAL(expectedDropdownWidth_Touch, comboBoxDropdownRoot->MinWidth);
            VERIFY_ARE_EQUAL(expectedDropdownWidth_Touch, comboBoxDropdownRoot->ActualWidth);
            VERIFY_ARE_EQUAL(expectedComboBoxItemHeight_Touch, comboBoxItem->ActualHeight);
            VERIFY_ARE_EQUAL(expectedDropdownContentMargin_NonTouchAndCarousel, comboBoxDropdownItemsPresenter->Margin);
        });
        ComboBoxHelper::CloseComboBox(comboBox);

        // Open via Gamepad.
        ComboBoxHelper::OpenComboBox(comboBox, ComboBoxHelper::OpenMethod::Gamepad);
        RunOnUIThread([&]()
        {
            comboBoxDropdownRoot = safe_cast<xaml::FrameworkElement^>(TreeHelper::GetVisualChildByNameFromOpenPopups(L"ScrollViewer", comboBox));
            comboBoxDropdownItemsPresenter = TreeHelper::GetVisualChildByType<xaml_controls::ItemsPresenter>(comboBoxDropdownRoot);
            VERIFY_ARE_EQUAL(expectedDropdownWidth_Touch, comboBoxDropdownRoot->MinWidth);
            VERIFY_ARE_EQUAL(expectedDropdownWidth_Touch, comboBoxDropdownRoot->ActualWidth);
            VERIFY_ARE_EQUAL(expectedComboBoxItemHeight_Touch, comboBoxItem->ActualHeight);
            VERIFY_ARE_EQUAL(expectedDropdownContentMargin_NonTouchAndCarousel, comboBoxDropdownItemsPresenter->Margin);
        });
        ComboBoxHelper::CloseComboBox(comboBox);

        // Open via Mouse.
        ComboBoxHelper::OpenComboBox(comboBox, ComboBoxHelper::OpenMethod::Mouse);
        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(expectedDropdownWidth_NonTouch, comboBoxDropdownRoot->MinWidth);
            VERIFY_ARE_EQUAL(expectedDropdownWidth_NonTouch, comboBoxDropdownRoot->ActualWidth);
            VERIFY_ARE_EQUAL(expectedComboBoxItemHeight_NonTouch, comboBoxItem->ActualHeight);
            VERIFY_ARE_EQUAL(expectedDropdownContentMargin_NonTouchAndCarousel, comboBoxDropdownItemsPresenter->Margin);
        });
        ComboBoxHelper::CloseComboBox(comboBox);

        // Open via Keyboard.
        ComboBoxHelper::OpenComboBox(comboBox, ComboBoxHelper::OpenMethod::Keyboard);
        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(expectedDropdownWidth_NonTouch, comboBoxDropdownRoot->MinWidth);
            VERIFY_ARE_EQUAL(expectedDropdownWidth_NonTouch, comboBoxDropdownRoot->ActualWidth);
            VERIFY_ARE_EQUAL(expectedComboBoxItemHeight_NonTouch, comboBoxItem->ActualHeight);
            VERIFY_ARE_EQUAL(expectedDropdownContentMargin_NonTouchAndCarousel, comboBoxDropdownItemsPresenter->Margin);
        });
        ComboBoxHelper::CloseComboBox(comboBox);

        // Open programmatically.
        ComboBoxHelper::OpenComboBox(comboBox, ComboBoxHelper::OpenMethod::Programmatic);
        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(expectedDropdownWidth_NonTouch, comboBoxDropdownRoot->MinWidth);
            VERIFY_ARE_EQUAL(expectedDropdownWidth_NonTouch, comboBoxDropdownRoot->ActualWidth);
            VERIFY_ARE_EQUAL(expectedComboBoxItemHeight_NonTouch, comboBoxItem->ActualHeight);
            VERIFY_ARE_EQUAL(expectedDropdownContentMargin_NonTouchAndCarousel, comboBoxDropdownItemsPresenter->Margin);
        });
        ComboBoxHelper::CloseComboBox(comboBox);

        // Adding 15 more items so that the max limit of ComboBoxItems to show is reached.
        // After 15 items, the Dropdown starts showing a ScrollBar to see other ComboBoxItems.
        RunOnUIThread([&]()
        {
            for (unsigned int i = 1; i < 16; ++i)
            {
                auto cbItem = ref new xaml_controls::ComboBoxItem();
                cbItem->Content = "i" + i;
                comboBox->Items->Append(cbItem);
            }
        });

        // Open via Touch. Now, since the Dropdown have a ScrollBar, when opened with touch, the Dropdown will Carousel,
        // that is, it loops around the ComboBoxItems. In this case, we want to make sure there is no extra Top/Bottom
        // Padding/Margin on the Content.
        ComboBoxHelper::OpenComboBox(comboBox, ComboBoxHelper::OpenMethod::Touch);
        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(expectedDropdownWidth_Touch, comboBoxDropdownRoot->MinWidth);
            VERIFY_ARE_EQUAL(expectedDropdownWidth_Touch, comboBoxDropdownRoot->ActualWidth);
            VERIFY_ARE_EQUAL(expectedComboBoxItemHeight_Touch, comboBoxItem->ActualHeight);
            VERIFY_ARE_EQUAL(expectedDropdownContentMargin_TouchAndCarousel, comboBoxDropdownItemsPresenter->Margin);
        });
        ComboBoxHelper::CloseComboBox(comboBox);
    }

    void ComboBoxIntegrationTests::ValidateFootprint()
    {
        TestCleanupWrapper cleanup;

        double const expectedComboBoxWidth = 70;
        double const expectedComboBoxWidth_WithWideContent = 200 + 50;
        double const expectedComboBoxWidth_WithWideHeader = 200;

        double const expectedComboBoxHeight_NoHeader = 32;
        double const expectedComboBoxHeight_WithHeader = 24 + 4 + expectedComboBoxHeight_NoHeader;
        double const expectedComboBoxHeight_WithWideHeader = 23 + 4 + expectedComboBoxHeight_NoHeader;

        xaml_controls::ComboBox^ comboBox;
        xaml_controls::ComboBox^ comboBoxWithHeader;
        xaml_controls::ComboBox^ comboBoxWithWideContent;
        xaml_controls::ComboBox^ comboBoxWithWideHeader;
        xaml_controls::StackPanel^ rootPanel;

        RunOnUIThread([&]()
        {
            rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                LR"(<StackPanel xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" >
                        <ComboBox x:Name="comboBox" >
                            <ComboBoxItem Content="1" />
                            <ComboBoxItem Content="2" />
                            <ComboBoxItem Content="3" />
                        </ComboBox>
                        <ComboBox x:Name="comboBoxWithHeader" Header="H" >
                            <ComboBoxItem Content="1" />
                            <ComboBoxItem Content="2" />
                            <ComboBoxItem Content="3" />
                        </ComboBox>
                        <ComboBox x:Name="comboBoxWithWideContent" >
                            <ComboBoxItem IsSelected="True">
                                <Rectangle Height="10" Width="200" Fill="Red" />
                            </ComboBoxItem>
                            <ComboBoxItem Content="2" />
                            <ComboBoxItem Content="3" />
                        </ComboBox>
                        <ComboBox x:Name="comboBoxWithWideHeader" >
                            <ComboBox.Header>
                                <Rectangle Height="19" Width="200" Fill="Red" />
                            </ComboBox.Header>
                            <ComboBoxItem Content="1" />
                            <ComboBoxItem Content="2" />
                            <ComboBoxItem Content="3" />
                        </ComboBox>
                    </StackPanel>)"));

            comboBox = safe_cast<xaml_controls::ComboBox^>(rootPanel->FindName(L"comboBox"));
            comboBoxWithHeader = safe_cast<xaml_controls::ComboBox^>(rootPanel->FindName(L"comboBoxWithHeader"));
            comboBoxWithWideContent = safe_cast<xaml_controls::ComboBox^>(rootPanel->FindName(L"comboBoxWithWideContent"));
            comboBoxWithWideHeader = safe_cast<xaml_controls::ComboBox^>(rootPanel->FindName(L"comboBoxWithWideHeader"));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(expectedComboBoxWidth, comboBox->ActualWidth);
            VERIFY_ARE_EQUAL(expectedComboBoxHeight_NoHeader, comboBox->ActualHeight);

            VERIFY_ARE_EQUAL(expectedComboBoxWidth, comboBoxWithHeader->ActualWidth);
            VERIFY_ARE_EQUAL(expectedComboBoxHeight_WithHeader, comboBoxWithHeader->ActualHeight);

            VERIFY_ARE_EQUAL(expectedComboBoxWidth_WithWideContent, comboBoxWithWideContent->ActualWidth);
            VERIFY_ARE_EQUAL(expectedComboBoxHeight_NoHeader, comboBoxWithWideContent->ActualHeight);

            VERIFY_ARE_EQUAL(expectedComboBoxWidth_WithWideHeader, comboBoxWithWideHeader->ActualWidth);
            VERIFY_ARE_EQUAL(expectedComboBoxHeight_WithWideHeader, comboBoxWithWideHeader->ActualHeight);
        });
    }

    void ComboBoxIntegrationTests::CanSelectItemWithTap()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::ComboBoxItem^ comboBoxItem = nullptr;

        auto comboBox = SetupBasicComboBoxTest();

        ComboBoxHelper::OpenComboBox(comboBox, ComboBoxHelper::OpenMethod::Programmatic);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            comboBoxItem = static_cast<xaml_controls::ComboBoxItem^>(comboBox->ContainerFromIndex(0));
            VERIFY_IS_NOT_NULL(comboBoxItem);
        });
        TestServices::WindowHelper->WaitForIdle();

        TestServices::InputHelper->Tap(comboBoxItem);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(comboBox->SelectedIndex, 0);
            VERIFY_IS_FALSE(comboBox->IsDropDownOpen);
        });
    }

    void ComboBoxIntegrationTests::VallidateMaximumHeightIsHonored()
    {
        TestCleanupWrapper cleanup;

        auto comboBox = SetupBasicComboBoxTest(10, true);

        RunOnUIThread([&]()
        {
            comboBox->MaxDropDownHeight = 100;
        });
        TestServices::WindowHelper->WaitForIdle();

        ComboBoxHelper::OpenComboBox(comboBox, ComboBoxHelper::OpenMethod::Programmatic);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto popupBorder = TreeHelper::GetVisualChildByNameFromOpenPopups(L"PopupBorder", comboBox);
            VERIFY_IS_LESS_THAN(popupBorder->ActualHeight, 100);
        });
    }

    void ComboBoxIntegrationTests::ValidateSelectedValuePathProperty()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::ComboBox^ comboBox = nullptr;

        RunOnUIThread([&]()
        {
            auto rootPanel = ref new xaml_controls::Grid();
            VERIFY_IS_NOT_NULL(rootPanel);

            Platform::Collections::Vector<PersonObject^>^ itemList = ref new Platform::Collections::Vector<PersonObject^>();
            itemList->Append(ref new PersonObject(L"Roger", L"Federer"));
            itemList->Append(ref new PersonObject(L"Cristiano", L"Ronaldo"));
            itemList->Append(ref new PersonObject(L"LeBron", L"James"));

            comboBox = ref new xaml_controls::ComboBox();
            VERIFY_IS_NOT_NULL(comboBox);

            comboBox->ItemsSource = itemList;
            comboBox->DisplayMemberPath = L"FirstName";
            comboBox->SelectedValuePath = L"LastName";

            rootPanel->Children->Append(comboBox);
            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            comboBox->SelectedIndex = 1;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto expectedSelectedValueString = ref new Platform::String(L"Ronaldo");
            auto selectedValueString = dynamic_cast<Platform::String^>(comboBox->SelectedValue);

            VERIFY_IS_NOT_NULL(selectedValueString);
            VERIFY_ARE_EQUAL(expectedSelectedValueString, selectedValueString);
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // changing the SelectedValue to something invalid (not in collection)
            comboBox->SelectedValue = L"Hello";
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // after changing the SelectedValue to something unrecognizable, SelectedValue should change to NULL
            VERIFY_IS_NULL(comboBox->SelectedValue);
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void ComboBoxIntegrationTests::ValidateCustomizedPaddingWithTouch()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::ScrollViewer^ scrollViewer = nullptr;

        auto comboBox = SetupComboBoxCustomizedPaddingTest();

        // Validate the touch input mode padding for each item.
        ComboBoxHelper::OpenComboBox(comboBox, ComboBoxHelper::OpenMethod::Touch);
        RunOnUIThread([&]()
        {
            auto popup = TreeHelper::GetVisualChildByType<xaml_primitives::Popup>(comboBox);
            auto popupChild = safe_cast<FrameworkElement^>(popup->Child);
            scrollViewer = TreeHelper::GetVisualChildByType<xaml_controls::ScrollViewer>(popupChild);
        });
        ValidateComboBoxItemPadding(comboBox, ComboBoxHelper::OpenMethod::Touch);
        for (int i = 0; i < 10; ++i)
        {
            TestServices::InputHelper->Flick(comboBox, FlickDirection::North);
        }
        TestServices::WindowHelper->WaitForIdle();
        ValidateComboBoxItemPadding(comboBox, ComboBoxHelper::OpenMethod::Touch);
        for (int i = 0; i < 10; ++i)
        {
            TestServices::InputHelper->Flick(comboBox, FlickDirection::South);
        }
        TestServices::WindowHelper->WaitForIdle();
        ValidateComboBoxItemPadding(comboBox, ComboBoxHelper::OpenMethod::Touch);
        ComboBoxHelper::CloseComboBox(comboBox);
    }

    void ComboBoxIntegrationTests::ValidateCustomizedPaddingWithMouseAndKeyboard()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::ScrollViewer^ scrollViewer = nullptr;

        auto comboBox = SetupComboBoxCustomizedPaddingTest();

        // Validate the mouse input mode padding for each item.
        ComboBoxHelper::OpenComboBox(comboBox, ComboBoxHelper::OpenMethod::Mouse);
        RunOnUIThread([&]()
        {
            auto popup = TreeHelper::GetVisualChildByType<xaml_primitives::Popup>(comboBox);
            auto popupChild = safe_cast<FrameworkElement^>(popup->Child);
            scrollViewer = TreeHelper::GetVisualChildByType<xaml_controls::ScrollViewer>(popupChild);
        });
        ValidateComboBoxItemPadding(comboBox, ComboBoxHelper::OpenMethod::Mouse);
        TestServices::InputHelper->MoveMouse(scrollViewer);
        TestServices::WindowHelper->WaitForIdle();
        TestServices::InputHelper->ScrollMouseWheel(scrollViewer, 10 /* numberOfWheelClicks */);
        TestServices::WindowHelper->WaitForIdle();
        ValidateComboBoxItemPadding(comboBox, ComboBoxHelper::OpenMethod::Mouse);
        TestServices::InputHelper->ScrollMouseWheel(scrollViewer, -10 /* numberOfWheelClicks */);
        TestServices::WindowHelper->WaitForIdle();
        ValidateComboBoxItemPadding(comboBox, ComboBoxHelper::OpenMethod::Mouse);
        ComboBoxHelper::CloseComboBox(comboBox);

        // Validate the keyboard input mode padding for each item.
        ComboBoxHelper::OpenComboBox(comboBox, ComboBoxHelper::OpenMethod::Keyboard);
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_pagedown#$u$_pagedown");
        TestServices::WindowHelper->WaitForIdle();
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_pagedown#$u$_pagedown");
        TestServices::WindowHelper->WaitForIdle();
        ValidateComboBoxItemPadding(comboBox, ComboBoxHelper::OpenMethod::Keyboard);
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_pageup#$u$_pageup");
        TestServices::WindowHelper->WaitForIdle();
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_pageup#$u$_pageup");
        TestServices::WindowHelper->WaitForIdle();
        ValidateComboBoxItemPadding(comboBox, ComboBoxHelper::OpenMethod::Keyboard);
        ComboBoxHelper::CloseComboBox(comboBox);
    }

    xaml_controls::ComboBox^  ComboBoxIntegrationTests::SetupComboBoxCustomizedPaddingTest()
    {
        xaml_controls::ComboBox^ comboBox = nullptr;

        RunOnUIThread([&]()
        {
            auto rootPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                LR"(<Grid xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" VerticalAlignment="Center" >
                        <Grid.Resources>
                            <Style TargetType="ComboBoxItem">
                                <Setter Property="Background" Value="Red" />
                                <Setter Property="Padding" Value="21, 22, 23, 24" />
                            </Style>
                        </Grid.Resources>
                        <ComboBox x:Name='comboBoxCustomPaddingItem' Background='RoyalBlue' SelectedIndex='0'  Width='350' >
                            <ComboBoxItem Content='Apps and Games (1)' />
                            <ComboBoxItem Content='Files, Folders, and Online Storage (2)' />
                            <ComboBoxItem Content='Hardware, Devices, and Drivers (3)' />
                            <ComboBoxItem Content='Windows Installation and Setup (4)' />
                            <ComboBoxItem Content='Microsoft Edge and IE (5)' />
                            <ComboBoxItem Content='Networks (6)' />
                            <ComboBoxItem Content='Personalization and Ease of Access (7)' />
                            <ComboBoxItem Content='Power and Battery (8)' />
                            <ComboBoxItem Content='Windows Recovery (9)' />
                            <ComboBoxItem Content='Cortana and search (10)' />
                            <ComboBoxItem Content='Start (11)' />
                            <ComboBoxItem Content='Desktop (12)' />
                            <ComboBoxItem Content='Security, Privacy, and Accounts (13)' />
                            <ComboBoxItem Content='Input and Interaction Methods (14)' />
                            <ComboBoxItem Content='Store (15)' />
                            <ComboBoxItem Content='Developer Platform (16)' />
                            <ComboBoxItem Content='Internal Feedback Tools (17)' />
                            <ComboBoxItem Content='Preview Programs (18)' />
                        </ComboBox>
                    </Grid>)"));

            comboBox = safe_cast<xaml_controls::ComboBox^>(TreeHelper::GetVisualChildByName(rootPanel, L"comboBoxCustomPaddingItem"));
            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        return comboBox;
    }

    void ComboBoxIntegrationTests::ValidateComboBoxItemPadding(xaml_controls::ComboBox^ comboBox, ComboBoxHelper::OpenMethod openMethod)
    {
        RunOnUIThread([&]()
        {
            for (auto&& item : comboBox->Items)
            {
                auto comboBoxItem = safe_cast<xaml_controls::ComboBoxItem^>(static_cast<Platform::Object^>(item));
                auto contentPresenter = safe_cast<xaml_controls::ContentPresenter^>(TreeHelper::GetVisualChildByName(comboBoxItem, L"ContentPresenter"));

                if (contentPresenter)
                {
                    LOG_OUTPUT(L"Item padding left=%f top=%f right=%f bottom=%f", contentPresenter->Margin.Left, contentPresenter->Margin.Top, contentPresenter->Margin.Right, contentPresenter->Margin.Bottom);

                    if (openMethod == ComboBoxHelper::OpenMethod::Touch)
                    {
                        xaml::Thickness expectedThickness = xaml::Thickness({ 10,8,10,11 });
                        VERIFY_ARE_EQUAL(expectedThickness, contentPresenter->Margin);
                    }
                    else
                    {
                        VERIFY_ARE_EQUAL(comboBoxItem->Padding, contentPresenter->Margin);
                    }
                }
                else
                {
                    LOG_OUTPUT(L"Item isn't realized yet!");
                }
            }
        });
    }

    void ComboBoxIntegrationTests::ValidateItemTemplateSettingWithTouch()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::ComboBox^ comboBox = nullptr;
        xaml_controls::ScrollViewer^ scrollViewer = nullptr;

        RunOnUIThread([&]()
        {
            auto rootPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                LR"(<Grid xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" VerticalAlignment="Center" >
                        <Grid.Resources>
                            <Style x:Key="ContentTextStyle" TargetType="TextBlock">
                                <Setter Property="Foreground" Value="Blue"/>
                                <Setter Property="HorizontalAlignment" Value="Center"/>
                                <Setter Property="VerticalAlignment" Value="Center"/>
                                <Setter Property="FontSize" Value="40"/>
                                <Setter Property="TextWrapping" Value="NoWrap"/>
                                <Setter Property="FontFamily" Value="segoeuil.ttf#Segoe UI"/>
                            </Style>
                        </Grid.Resources>
                        <StackPanel Orientation="Horizontal">
                            <ComboBox x:Name='comboBoxNotificationSetting' Background='RoyalBlue' Width="200" >
                                <ComboBox.ItemTemplate>
                                    <DataTemplate>
                                        <Border Height="60" >
                                            <Border BorderBrush="Black" BorderThickness="0" >
                                                <TextBlock Text="{Binding}" Style="{StaticResource ContentTextStyle}" />
                                            </Border>
                                        </Border>
                                    </DataTemplate>
                                </ComboBox.ItemTemplate>
                            </ComboBox>
                        </StackPanel>
                    </Grid>)"));

            comboBox = safe_cast<xaml_controls::ComboBox^>(TreeHelper::GetVisualChildByName(rootPanel, L"comboBoxNotificationSetting"));

            // Add the notification setting items
            auto itemList = ref new Platform::Collections::Vector<Platform::String^>();
            itemList->Append("All settings(1)");
            itemList->Append("Connect (2)");
            itemList->Append("Battery saver (3)");
            itemList->Append("Flashlight (4)");
            itemList->Append("Note (5)");
            itemList->Append("VPN (6)");
            itemList->Append("Location (7)");
            itemList->Append("Airplane mode (8)");
            itemList->Append("Bluetooth (9)");
            itemList->Append("Camera (10)");
            itemList->Append("Cellular (11)");
            itemList->Append("Mobile hotspot (12)");
            itemList->Append("Quiet hours (13)");
            itemList->Append("Wi-Fi (14)");
            itemList->Append("Rotation lock (15)");
            itemList->Append("Brightness (16)");
            comboBox->ItemsSource = itemList;

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        // Validate the touch input mode padding for each item.
        ComboBoxHelper::OpenComboBox(comboBox, ComboBoxHelper::OpenMethod::Touch);

        RunOnUIThread([&]()
        {
            auto popup = TreeHelper::GetVisualChildByType<xaml_primitives::Popup>(comboBox);
            scrollViewer = TreeHelper::GetVisualChildByType<xaml_controls::ScrollViewer>(safe_cast<FrameworkElement^>(popup->Child));
        });

        LOG_OUTPUT(L"Panning to North.");

        for (int i = 0; i < 25; ++i)
        {
            TestServices::InputHelper->Flick(comboBox, FlickDirection::North);
        }
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Panning to South.");

        for (int i = 0; i < 25; ++i)
        {
            TestServices::InputHelper->Flick(comboBox, FlickDirection::South);
        }
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"No layout cycle reported!");

        ComboBoxHelper::CloseComboBox(comboBox);
    }

    void ComboBoxIntegrationTests::ValidateOpenedComboBoxPositionByTouchInput()
    {
        TestCleanupWrapper cleanup;

        DoValidatePosition(5, ComboBoxHelper::OpenMethod::Touch, false /* addMouseOpenMethod */);
        DoValidatePosition(50, ComboBoxHelper::OpenMethod::Touch, false /* addMouseOpenMethod */);
    }

    void ComboBoxIntegrationTests::ValidateOpenedComboBoxPositionWithDifferentInput()
    {
        TestCleanupWrapper cleanup;

        DoValidatePosition(5, ComboBoxHelper::OpenMethod::Touch, true /* addMouseOpenMethod */);
        DoValidatePosition(50, ComboBoxHelper::OpenMethod::Touch, true /* addMouseOpenMethod */);
    }

    void ComboBoxIntegrationTests::DoValidatePosition(int itemCount, ComboBoxHelper::OpenMethod openMethod, bool addMouseOpenMethod, bool isVerticalAlignment)
    {
        TestCleanupWrapper cleanup;

        auto comboBox = SetupBasicComboBoxTest(itemCount /* itemSize */);

        RunOnUIThread([&]()
        {
            comboBox->Margin = { 0,0,0,0 };
        });

        if (isVerticalAlignment)
        {
            LOG_OUTPUT(L"DoValidatePosition Horizontal:Left Vertical:Top.");
            ValidatePosition(xaml::HorizontalAlignment::Left, xaml::VerticalAlignment::Top, comboBox, openMethod);
            if (addMouseOpenMethod)
            {
                ValidatePosition(xaml::HorizontalAlignment::Left, xaml::VerticalAlignment::Top, comboBox, ComboBoxHelper::OpenMethod::Mouse);
            }

            LOG_OUTPUT(L"DoValidatePosition Horizontal:Left Vertical:Center.");
            ValidatePosition(xaml::HorizontalAlignment::Left, xaml::VerticalAlignment::Center, comboBox, openMethod);
            if (addMouseOpenMethod)
            {
                ValidatePosition(xaml::HorizontalAlignment::Left, xaml::VerticalAlignment::Center, comboBox, ComboBoxHelper::OpenMethod::Mouse);
            }

            LOG_OUTPUT(L"DoValidatePosition Horizontal:Left Vertical:Bottom.");
            ValidatePosition(xaml::HorizontalAlignment::Left, xaml::VerticalAlignment::Bottom, comboBox, openMethod);
            if (addMouseOpenMethod)
            {
                ValidatePosition(xaml::HorizontalAlignment::Left, xaml::VerticalAlignment::Bottom, comboBox, ComboBoxHelper::OpenMethod::Mouse);
            }
        }
        else
        {
            LOG_OUTPUT(L"DoValidatePosition Horizontal:Left Vertical:Top.");
            ValidatePosition(xaml::HorizontalAlignment::Left, xaml::VerticalAlignment::Top, comboBox, openMethod);
            if (addMouseOpenMethod)
            {
                ValidatePosition(xaml::HorizontalAlignment::Left, xaml::VerticalAlignment::Top, comboBox, ComboBoxHelper::OpenMethod::Mouse);
            }

            LOG_OUTPUT(L"DoValidatePosition Horizontal:Center Vertical:Top.");
            ValidatePosition(xaml::HorizontalAlignment::Center, xaml::VerticalAlignment::Top, comboBox, openMethod);
            if (addMouseOpenMethod)
            {
                ValidatePosition(xaml::HorizontalAlignment::Center, xaml::VerticalAlignment::Top, comboBox, ComboBoxHelper::OpenMethod::Mouse);
            }

            LOG_OUTPUT(L"DoValidatePosition Horizontal:Right Vertical:Top.");
            ValidatePosition(xaml::HorizontalAlignment::Right, xaml::VerticalAlignment::Top, comboBox, openMethod);
            if (addMouseOpenMethod)
            {
                ValidatePosition(xaml::HorizontalAlignment::Right, xaml::VerticalAlignment::Top, comboBox, ComboBoxHelper::OpenMethod::Mouse);
            }
        }
    }

    void ComboBoxIntegrationTests::ValidatePosition(xaml::HorizontalAlignment horizontalAlignment, xaml::VerticalAlignment verticalAlignment, xaml_controls::ComboBox^ comboBox, ComboBoxHelper::OpenMethod openMethod)
    {
        xaml_controls::ScrollViewer^ scrollViewer = nullptr;

        RunOnUIThread([&]()
        {
            auto rootPanel = safe_cast<xaml_controls::Grid^>(comboBox->Parent);
            rootPanel->HorizontalAlignment = horizontalAlignment;
            rootPanel->VerticalAlignment = verticalAlignment;
        });
        TestServices::WindowHelper->WaitForIdle();

        ComboBoxHelper::OpenComboBox(comboBox, openMethod);

        RunOnUIThread([&]()
        {
            auto popup = TreeHelper::GetVisualChildByType<xaml_primitives::Popup>(comboBox);
            auto popupChild = safe_cast<FrameworkElement^>(popup->Child);
            scrollViewer = safe_cast<xaml_controls::ScrollViewer^>(TreeHelper::GetVisualChildByName(popupChild, L"ScrollViewer"));

            wf::Rect scrollViewerBounds = ControlHelper::GetBounds(scrollViewer);
            wf::Rect visibleBounds = TestServices::WindowHelper->VisibleBounds;

            LOG_OUTPUT(L"scrollViewerBounds: (%f, %f, %f, %f)", scrollViewerBounds.X, scrollViewerBounds.Y, scrollViewerBounds.Width, scrollViewerBounds.Height);
            LOG_OUTPUT(L"visibleBounds:      (%f, %f, %f, %f)", visibleBounds.X, visibleBounds.Y, visibleBounds.Width, visibleBounds.Height);

            wf::Point topLeftCorner = { 0, 0 };

            // If we're on desktop with windowed popups enabled, then we expect the popup to be able to overlap with the window chrome.
            if (PopupHelper::AreWindowedPopupsEnabled())
            {
                wf::Rect windowBounds = TestServices::WindowHelper->WindowBounds;
                LOG_OUTPUT(L"windowBounds:       (%f, %f, %f, %f)", windowBounds.X, windowBounds.Y, windowBounds.Width, windowBounds.Height);

                topLeftCorner.X -= windowBounds.X;
                topLeftCorner.Y -= windowBounds.Y;
                visibleBounds.X += windowBounds.X;
                visibleBounds.Y += windowBounds.Y;
            }

            VERIFY_IS_TRUE(scrollViewerBounds.X >= topLeftCorner.X);
            VERIFY_IS_TRUE(scrollViewerBounds.X + scrollViewerBounds.Width <= topLeftCorner.X + visibleBounds.X + visibleBounds.Width);
            VERIFY_IS_TRUE(scrollViewerBounds.Y >= topLeftCorner.Y);
            VERIFY_IS_TRUE(scrollViewerBounds.Y + scrollViewerBounds.Height <= topLeftCorner.Y + visibleBounds.Y + visibleBounds.Height);
        });

        ComboBoxHelper::CloseComboBox(comboBox);
        TestServices::WindowHelper->WaitForIdle();
    }

    void ComboBoxIntegrationTests::ValidateLightDismissOverlayMode()
    {
        TestCleanupWrapper cleanup;

        auto comboBox = SetupBasicComboBoxTest();

        ComboBoxHelper::OpenComboBox(comboBox, ComboBoxHelper::OpenMethod::Programmatic);

        LOG_OUTPUT(L"Validate that the default is Auto and the ComboBox's popup is set to Off (or On if on Xbox)");
        {
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(comboBox->LightDismissOverlayMode, xaml_controls::LightDismissOverlayMode::Auto);
            });

            ValidateComboBoxPopupLightDismissOverlayMode(
                TestServices::Utilities->IsXBox ? xaml_controls::LightDismissOverlayMode::On : xaml_controls::LightDismissOverlayMode::Off);
        }

        LOG_OUTPUT(L"Validate that when set to On the ComboBox's popup is also set to On.");
        {
            RunOnUIThread([&]()
            {
                comboBox->LightDismissOverlayMode = xaml_controls::LightDismissOverlayMode::On;
            });

            ValidateComboBoxPopupLightDismissOverlayMode(xaml_controls::LightDismissOverlayMode::On);
        }

        LOG_OUTPUT(L"Validate that when set to Off the ComboBox's popup is also set to Off.");
        {
            RunOnUIThread([&]()
            {
                comboBox->LightDismissOverlayMode = xaml_controls::LightDismissOverlayMode::Off;
            });

            ValidateComboBoxPopupLightDismissOverlayMode(xaml_controls::LightDismissOverlayMode::Off);
        }

        ComboBoxHelper::CloseComboBox(comboBox);
    }

    void ComboBoxIntegrationTests::DoesAutoLightDismissOverlayModeCreateOverlayOnXbox()
    {
        TestCleanupWrapper cleanup;

        auto comboBox = SetupBasicComboBoxTest();

        RunOnUIThread([&]()
        {
            comboBox->LightDismissOverlayMode = xaml_controls::LightDismissOverlayMode::Auto;
        });

        ComboBoxHelper::OpenComboBox(comboBox, ComboBoxHelper::OpenMethod::Programmatic);

        ValidateComboBoxPopupLightDismissOverlayMode(xaml_controls::LightDismissOverlayMode::On);

        ComboBoxHelper::CloseComboBox(comboBox);
    }

    void ComboBoxIntegrationTests::ValidateOverlayBrush()
    {
        TestCleanupWrapper cleanup;

        auto comboBox = SetupBasicComboBoxTest();

        RunOnUIThread([&]()
        {
            comboBox->LightDismissOverlayMode = xaml_controls::LightDismissOverlayMode::On;
        });

        ComboBoxHelper::OpenComboBox(comboBox, ComboBoxHelper::OpenMethod::Programmatic);

        RunOnUIThread([&]()
        {
            auto expectedBrush = safe_cast<xaml_media::SolidColorBrush^>(xaml::Application::Current->Resources->Lookup(L"ComboBoxLightDismissOverlayBackground"));

            auto popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(comboBox->XamlRoot);
            WEX::Common::Throw::IfFalse(popups->Size == 1, E_FAIL, L"Expected exactly one open Popup.");

            auto popup = popups->GetAt(0);

            auto overlayElement = TestServices::Utilities->GetPopupOverlayElement(popup);
            THROW_IF_NULL_WITH_MSG(overlayElement, L"An overlay element should exist.");

            auto overlayRect = dynamic_cast<xaml_shapes::Rectangle^>(overlayElement);
            THROW_IF_NULL_WITH_MSG(overlayRect, L"The overlay element should be a rectangle.");

            auto overlayBrush = safe_cast<xaml_media::SolidColorBrush^>(overlayRect->Fill);
            VERIFY_IS_NOT_NULL(overlayBrush);
            VERIFY_IS_TRUE(overlayBrush->Equals(expectedBrush));
        });

        ComboBoxHelper::CloseComboBox(comboBox);
    }

    void ComboBoxIntegrationTests::ValidateComboBoxPopupLightDismissOverlayMode(xaml_controls::LightDismissOverlayMode expectedMode)
    {
        RunOnUIThread([&]()
        {
            auto popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(TestServices::WindowHelper->WindowContent->XamlRoot);
            WEX::Common::Throw::IfFalse(popups->Size == 1, E_FAIL, L"Expected exactly one open Popup.");

            auto popup = popups->GetAt(0);

            VERIFY_ARE_EQUAL(popup->LightDismissOverlayMode, expectedMode);

            if (expectedMode == xaml_controls::LightDismissOverlayMode::On)
            {
                auto overlayElement = TestServices::Utilities->GetPopupOverlayElement(popup);
                VERIFY_IS_NOT_NULL(overlayElement);
            }
        });
    }

    void ComboBoxIntegrationTests::ValidateFocusStateForComboBoxOpenedWithTouch()
    {
        ValidateFocusStateForComboBoxWorker(ComboBoxHelper::OpenMethod::Touch, xaml::FocusState::Pointer);
    }

    void ComboBoxIntegrationTests::ValidateFocusStateForComboBoxOpenedWithMouse()
    {
        ValidateFocusStateForComboBoxWorker(ComboBoxHelper::OpenMethod::Mouse, xaml::FocusState::Pointer);
    }

    void ComboBoxIntegrationTests::ValidateFocusStateForComboBoxOpenedWithKeyboard()
    {
        ValidateFocusStateForComboBoxWorker(ComboBoxHelper::OpenMethod::Keyboard, xaml::FocusState::Keyboard);
    }

    void ComboBoxIntegrationTests::ValidateFocusStateForComboBoxOpenedWithGamepad()
    {
        ValidateFocusStateForComboBoxWorker(ComboBoxHelper::OpenMethod::Gamepad, xaml::FocusState::Keyboard);
    }

    void ComboBoxIntegrationTests::ValidateFocusStateForComboBoxWorker(ComboBoxHelper::OpenMethod openMethod, xaml::FocusState expectedFocusState)
    {
        TestCleanupWrapper cleanup;

        xaml_controls::ComboBox^ comboBox = SetupBasicComboBoxTest();

        RunOnUIThread([&]()
        {
            comboBox->SelectedIndex = 0;
        });
        TestServices::WindowHelper->WaitForIdle();

        ComboBoxHelper::OpenComboBox(comboBox, openMethod);

        RunOnUIThread([&]()
        {
            auto selectedValue = dynamic_cast<xaml_controls::ComboBoxItem^>(comboBox->SelectedValue);
            VERIFY_IS_NOT_NULL(selectedValue);
            auto focusState = selectedValue->FocusState;
            VERIFY_ARE_EQUAL(expectedFocusState, focusState);
        });
        TestServices::WindowHelper->WaitForIdle();

        ComboBoxHelper::CloseComboBox(comboBox);
        TestServices::WindowHelper->WaitForIdle();
    }

    void ComboBoxIntegrationTests::ValidateFocusStateOfClosedComboBoxWhenOpenedAndClosedWithMouse()
    {
        ValidateFocusStateOfClosedComboBoxWorker(ComboBoxHelper::OpenMethod::Mouse, ComboBoxHelper::CloseMethod::Mouse, xaml::FocusState::Pointer);
    }

    void ComboBoxIntegrationTests::ValidateFocusStateOfClosedComboBoxWhenOpenedAndClosedWithTouch()
    {
        ValidateFocusStateOfClosedComboBoxWorker(ComboBoxHelper::OpenMethod::Touch, ComboBoxHelper::CloseMethod::Touch, xaml::FocusState::Pointer);
    }

    void ComboBoxIntegrationTests::ValidateFocusStateOfClosedComboBoxWhenOpenedAndClosedWithKeyboard()
    {
        ValidateFocusStateOfClosedComboBoxWorker(ComboBoxHelper::OpenMethod::Keyboard, ComboBoxHelper::CloseMethod::Keyboard, xaml::FocusState::Keyboard);
    }

    void ComboBoxIntegrationTests::ValidateFocusStateOfClosedComboBoxWhenOpenedAndClosedWithGamepad()
    {
        ValidateFocusStateOfClosedComboBoxWorker(ComboBoxHelper::OpenMethod::Gamepad, ComboBoxHelper::CloseMethod::Gamepad, xaml::FocusState::Keyboard);
    }

    void ComboBoxIntegrationTests::ValidateFocusStateOfClosedComboBoxWhenOpenedWithTouchAndClosedWithKeyboard()
    {
        ValidateFocusStateOfClosedComboBoxWorker(ComboBoxHelper::OpenMethod::Touch, ComboBoxHelper::CloseMethod::Keyboard, xaml::FocusState::Keyboard);
    }

    void ComboBoxIntegrationTests::ValidateFocusStateOfClosedComboBoxWhenOpenedWithTouchAndClosedWithGamepad()
    {
        ValidateFocusStateOfClosedComboBoxWorker(ComboBoxHelper::OpenMethod::Touch, ComboBoxHelper::CloseMethod::Gamepad, xaml::FocusState::Keyboard);
    }

    void ComboBoxIntegrationTests::ValidateFocusStateOfClosedComboBoxWhenOpenedWithMouseAndClosedWithKeyboard()
    {
        ValidateFocusStateOfClosedComboBoxWorker(ComboBoxHelper::OpenMethod::Mouse, ComboBoxHelper::CloseMethod::Keyboard, xaml::FocusState::Keyboard);
    }

    void ComboBoxIntegrationTests::ValidateFocusStateOfClosedComboBoxWhenOpenedWithMouseAndClosedWithGamepad()
    {
        ValidateFocusStateOfClosedComboBoxWorker(ComboBoxHelper::OpenMethod::Mouse, ComboBoxHelper::CloseMethod::Gamepad, xaml::FocusState::Keyboard);
    }

    void ComboBoxIntegrationTests::ValidateFocusStateOfClosedComboBoxWorker(ComboBoxHelper::OpenMethod openMethod, ComboBoxHelper::CloseMethod closeMethod, xaml::FocusState expectedFocusState)
    {
        TestCleanupWrapper cleanup;

        xaml_controls::ComboBox^ comboBox = SetupBasicComboBoxTest();

        RunOnUIThread([&]()
        {
            comboBox->SelectedIndex = 0;
        });
        TestServices::WindowHelper->WaitForIdle();

        ComboBoxHelper::OpenComboBox(comboBox, openMethod);

        ComboBoxHelper::CloseComboBox(comboBox, closeMethod);

        RunOnUIThread([&]()
        {
            auto focusState = comboBox->FocusState;
            VERIFY_ARE_EQUAL(expectedFocusState, focusState);
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void ComboBoxIntegrationTests::ValidatePagingKeyInteraction()
    {
        TestCleanupWrapper cleanup;

        int totalNumberOfItems = 20;
        int expectedNumberOfItemsToScrollWithPageKeys = 12;

        xaml_controls::ComboBox^ comboBox = SetupBasicComboBoxTest(totalNumberOfItems);

        RunOnUIThread([&]()
        {
            comboBox->SelectionChangedTrigger = xaml_controls::ComboBoxSelectionChangedTrigger::Always;
            comboBox->SelectedIndex = 0;
        });
        TestServices::WindowHelper->WaitForIdle();

        ComboBoxHelper::OpenComboBox(comboBox, ComboBoxHelper::OpenMethod::Programmatic);

        ComboBoxHelper::VerifySelectedIndex(comboBox, 0);

        LOG_OUTPUT(L"Changing selection with PageDown");
        TestServices::KeyboardHelper->PageDown();
        TestServices::WindowHelper->WaitForIdle();
        ComboBoxHelper::VerifySelectedIndex(comboBox, expectedNumberOfItemsToScrollWithPageKeys);

        LOG_OUTPUT(L"Changing selection with PageUp");
        TestServices::KeyboardHelper->PageUp();
        TestServices::WindowHelper->WaitForIdle();
        ComboBoxHelper::VerifySelectedIndex(comboBox, 0);

        LOG_OUTPUT(L"Changing selection with End");
        TestServices::KeyboardHelper->End();
        TestServices::WindowHelper->WaitForIdle();
        ComboBoxHelper::VerifySelectedIndex(comboBox, totalNumberOfItems - 1);

        LOG_OUTPUT(L"Changing selection with Home");
        TestServices::KeyboardHelper->Home();
        TestServices::WindowHelper->WaitForIdle();
        ComboBoxHelper::VerifySelectedIndex(comboBox, 0);

        ComboBoxHelper::CloseComboBox(comboBox, ComboBoxHelper::CloseMethod::Programmatic);
    }

    void ComboBoxIntegrationTests::OpenComboBoxWhileSIPIsUp()
    {
        // Regression coverage: ComboBox is blank when it opens while the SIP is up (once in this state, it is stuck, user cannot recover).
        //
        // This bug is hit in the scenario where all the ComboBoxItems fit on screen when the ComboBox is open, but they would NOT all fit in the space available
        // when the SIP is up.
        // Tapping on the ComboBox when the SIP is up causes the SIP to dismiss at the same time as the ComboBox is opening. This causes the size of the available space
        // to change while the ComboBox is in the process of opening, which was causing it to get into a bad state.
        //
        // Note: We cannot use SetWindowSizeOverride in this test, as this changes the behavior of what we are trying to test.

        wf::EventRegistrationToken inputPaneShowingToken = {};
        wf::EventRegistrationToken inputPaneHidingToken = {};

        // Since InputPane is not agile, we can't use SafeEventRegistration. We need to manage the SIP events manually.
        TestCleanupWrapper cleanup([&inputPaneShowingToken, &inputPaneHidingToken]()
        {
            RunOnUIThread([&inputPaneShowingToken, &inputPaneHidingToken]()
            {
                InputPane::GetForCurrentView()->Showing -= inputPaneShowingToken;
                InputPane::GetForCurrentView()->Hiding -= inputPaneHidingToken;
                inputPaneShowingToken = {};
                inputPaneHidingToken = {};
            });

            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        xaml_controls::ComboBox^ combobox;
        xaml_controls::TextBox^ textbox;
        xaml_controls::ComboBoxItem^ itemToSelect;

        RunOnUIThread([&]()
        {
            auto rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                LR"(<StackPanel
                            xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
                            xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" >
                        <ComboBox x:Name="combobox" Header="ComboBox Header" Margin="12" FontSize="40" SelectedIndex="0" >
                            <ComboBoxItem Content="ComboBoxItem A" />
                            <ComboBoxItem Content="ComboBoxItem B" />
                            <ComboBoxItem Content="ComboBoxItem C" x:Name="itemToSelect" />
                            <ComboBoxItem Content="ComboBoxItem D" />
                            <ComboBoxItem Content="ComboBoxItem E" />
                            <ComboBoxItem Content="ComboBoxItem F" />
                            <ComboBoxItem Content="ComboBoxItem G" />
                        </ComboBox>
                        <TextBox x:Name="textbox" Header="TextBox" Width="200" HorizontalAlignment="Left" Margin="12" />
                    </StackPanel>)"));

            combobox = safe_cast<xaml_controls::ComboBox^>(rootPanel->FindName(L"combobox"));
            textbox = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(L"textbox"));
            itemToSelect = safe_cast<xaml_controls::ComboBoxItem^>(rootPanel->FindName(L"itemToSelect"));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        Event selectionChangedEvent;
        auto selectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::ComboBox, SelectionChanged);
        selectionChangedRegistration.Attach(combobox, [&](){ selectionChangedEvent.Set(); });

        auto SIPShowingEvent = std::make_shared<Event>();
        auto SIPHidingEvent = std::make_shared<Event>();
        RunOnUIThread([&]()
        {
            InputPane^ inputPane = InputPane::GetForCurrentView();
            inputPaneShowingToken = inputPane->Showing += ref new wf::TypedEventHandler<InputPane^, InputPaneVisibilityEventArgs^>([&](InputPane^ pane, InputPaneVisibilityEventArgs^ e)
            {
                SIPShowingEvent->Set();
            });
            inputPaneHidingToken = inputPane->Hiding += ref new wf::TypedEventHandler<InputPane^, InputPaneVisibilityEventArgs^>([&](InputPane^ pane, InputPaneVisibilityEventArgs^ e)
            {
                SIPHidingEvent->Set();
            });
        });

        // We want to know the height of the open ComboBox popup (without the SIP being involved).
        double fullHeightOfComboBoxPopup = 0;
        ComboBoxHelper::OpenComboBox(combobox, ComboBoxHelper::OpenMethod::Touch);
        RunOnUIThread([&]()
        {
            auto popupBorder = TreeHelper::GetVisualChildByNameFromOpenPopups(L"PopupBorder", combobox);
            fullHeightOfComboBoxPopup = popupBorder->ActualHeight;
        });
        ComboBoxHelper::CloseComboBox(combobox);

        // Tap the TextBox to bring up the SIP.
        TestServices::InputHelper->Tap(textbox);
        SIPShowingEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        // We want to test the scenario where the full size of the ComboBox popup would not fit in the window space
        // available when the SIP is up.
        // We verify that this is actually the case (otherwise we are not testing what we want to test).
        RunOnUIThread([&]()
        {
            auto sipRect = InputPane::GetForCurrentView()->OccludedRect;
            auto windowBounds = xaml::Window::Current->Bounds;

            auto usableHeight = windowBounds.Height - sipRect.Height;
            VERIFY_IS_GREATER_THAN(fullHeightOfComboBoxPopup, usableHeight);
        });

        // Tap on the ComboBox when the SIP is up.
        // The ComboBox will open and the SIP will dismiss at the same time.
        ComboBoxHelper::OpenComboBox(combobox, ComboBoxHelper::OpenMethod::Touch);
        SIPHidingEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        // Select an item from the open ComboBox.
        TestServices::InputHelper->Tap(itemToSelect);
        selectionChangedEvent.WaitForDefault();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(itemToSelect->Equals(combobox->SelectedItem));
        });
    }

    // - On Xbox, ComboBox popup was cliped to the TV safe area due to some assumptions made
    // when arranging the popup. We can simulate this scenario on all platforms by nudging the ComboBox slightly
    // offscreen on the bottom. Before the fix to 7442479, the last ComboBox item would render outside the visible area
    // in order to remain in-line with the ComboBox while the popup would render within the visible area,
    // causing the popup to clip the last item. This fix ensures that the last item always renders within the
    // visible bounds so as to not get clipped.
    void ComboBoxIntegrationTests::ValidatePopupPlacementAtBottomOfScreen()
    {
        TestCleanupWrapper cleanup;

        UINT numberOfItems = 8;
        float heightOfWindow = 400;

        ::Windows::Foundation::Size size(400, heightOfWindow);
        TestServices::WindowHelper->SetWindowSizeOverride(size);

        xaml_controls::ComboBox^ comboBox = SetupBasicComboBoxTest(numberOfItems);

        RunOnUIThread([&]()
        {
            // Ensure that the combobox is partially off screen
            Microsoft::UI::Xaml::Media::TranslateTransform^ translateTransform = ref new Microsoft::UI::Xaml::Media::TranslateTransform();
            translateTransform->Y = 20;
            comboBox->RenderTransform = translateTransform;

            comboBox->Margin = xaml::ThicknessHelper::FromUniformLength(0);
            comboBox->HorizontalAlignment = HorizontalAlignment::Center;
            comboBox->VerticalAlignment = VerticalAlignment::Bottom;

            // Select the last item. We will get the bounds of this item and ensure it is rendered on-screen.
            comboBox->SelectedIndex = numberOfItems - 1;
        });
        TestServices::WindowHelper->WaitForIdle();

        ComboBoxHelper::OpenComboBox(comboBox, ComboBoxHelper::OpenMethod::Programmatic);

        xaml_controls::ComboBoxItem^ selectedItem;
        RunOnUIThread([&]()
        {
            // Get the selected item
            selectedItem = safe_cast<xaml_controls::ComboBoxItem^>(comboBox->SelectedItem);

            // Get the bounds of the item
            wf::Rect selectedItemBounds = ControlHelper::GetBounds(selectedItem);
            LOG_OUTPUT(L"selectedItemBounds: (%f, %f, %f, %f)", selectedItemBounds.X, selectedItemBounds.Y, selectedItemBounds.Width, selectedItemBounds.Height);

            // Ensure that the bounds of the item are rendered within the height of the window,
            // unless windowed popups are enabled, in which case we're fine with it appearing outside.
            if (PopupHelper::AreWindowedPopupsEnabled())
            {
                VERIFY_IS_GREATER_THAN(selectedItemBounds.Y + selectedItemBounds.Height, heightOfWindow);
            }
            else
            {
                VERIFY_IS_LESS_THAN_OR_EQUAL(selectedItemBounds.Y + selectedItemBounds.Height, heightOfWindow);
            }
        });

        ComboBoxHelper::CloseComboBox(comboBox, ComboBoxHelper::CloseMethod::Programmatic);
    }

    // - The bottom aligned ComboBox doesn't add the proper amount of margin
    // when the opened drop down bottom position is out of the available window height position.
    // ValidateOpenedDropDownHeightUsingMouse ensures the opened drop down height is always same
    // value for all different aligned ComboBox's opened drop down height.
    void ComboBoxIntegrationTests::ValidateOpenedDropDownHeightUsingMouse()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::ComboBox^ comboBoxTop;
        xaml_controls::ComboBox^ comboBoxCenter;
        xaml_controls::ComboBox^ comboBoxBottom;
        wf::Rect scrollViewerTopBounds = {};
        wf::Rect scrollViewerCenterBounds = {};
        wf::Rect scrollViewerBottomBounds = {};

        RunOnUIThread([&]()
        {
            auto rootPanel = safe_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                LR"(<Grid xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" >
                        <ComboBox x:Name="comboBoxTop" Width="300" HorizontalAlignment="Center" VerticalAlignment="Top" >
                            <ComboBoxItem Content="item one" />
                            <ComboBoxItem Content="item two" />
                            <ComboBoxItem Content="item three" />
                            <ComboBoxItem Content="item four" />
                            <ComboBoxItem Content="item five" />
                        </ComboBox>
                        <ComboBox x:Name="comboBoxCenter" Width="300" HorizontalAlignment="Center" VerticalAlignment="Center" >
                            <ComboBoxItem Content="item one" />
                            <ComboBoxItem Content="item two" />
                            <ComboBoxItem Content="item three" />
                            <ComboBoxItem Content="item four" />
                            <ComboBoxItem Content="item five" />
                        </ComboBox>
                        <ComboBox x:Name="comboBoxBottom" Width="300" HorizontalAlignment="Center" VerticalAlignment="Bottom" >
                            <ComboBoxItem Content="item one" />
                            <ComboBoxItem Content="item two" />
                            <ComboBoxItem Content="item three" />
                            <ComboBoxItem Content="item four" />
                            <ComboBoxItem Content="item five" />
                        </ComboBox>
                    </Grid>)"));

            comboBoxTop = safe_cast<xaml_controls::ComboBox^>(rootPanel->FindName(L"comboBoxTop"));
            comboBoxCenter = safe_cast<xaml_controls::ComboBox^>(rootPanel->FindName(L"comboBoxCenter"));
            comboBoxBottom = safe_cast<xaml_controls::ComboBox^>(rootPanel->FindName(L"comboBoxBottom"));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        ComboBoxHelper::OpenComboBox(comboBoxTop, ComboBoxHelper::OpenMethod::Mouse);
        RunOnUIThread([&]()
        {
            auto popup = TreeHelper::GetVisualChildByType<xaml_primitives::Popup>(comboBoxTop);
            auto popupChild = safe_cast<FrameworkElement^>(popup->Child);
            auto scrollViewer = TreeHelper::GetVisualChildByType<xaml_controls::ScrollViewer>(popupChild);
            scrollViewerTopBounds = ControlHelper::GetBounds(scrollViewer);
        });
        ComboBoxHelper::CloseComboBox(comboBoxTop);

        ComboBoxHelper::OpenComboBox(comboBoxCenter, ComboBoxHelper::OpenMethod::Mouse);
        RunOnUIThread([&]()
        {
            auto popup = TreeHelper::GetVisualChildByType<xaml_primitives::Popup>(comboBoxCenter);
            auto popupChild = safe_cast<FrameworkElement^>(popup->Child);
            auto scrollViewer = TreeHelper::GetVisualChildByType<xaml_controls::ScrollViewer>(popupChild);
            scrollViewerCenterBounds = ControlHelper::GetBounds(scrollViewer);
        });
        ComboBoxHelper::CloseComboBox(comboBoxCenter);

        ComboBoxHelper::OpenComboBox(comboBoxBottom, ComboBoxHelper::OpenMethod::Mouse);
        RunOnUIThread([&]()
        {
            auto popup = TreeHelper::GetVisualChildByType<xaml_primitives::Popup>(comboBoxBottom);
            auto popupChild = safe_cast<FrameworkElement^>(popup->Child);
            auto scrollViewer = TreeHelper::GetVisualChildByType<xaml_controls::ScrollViewer>(popupChild);
            scrollViewerBottomBounds = ControlHelper::GetBounds(scrollViewer);

            VERIFY_IS_TRUE(scrollViewerTopBounds.Height == scrollViewerCenterBounds.Height);
            VERIFY_IS_TRUE(scrollViewerCenterBounds.Height == scrollViewerBottomBounds.Height);
            VERIFY_IS_TRUE(scrollViewerBottomBounds.Height == scrollViewerTopBounds.Height);
        });
        ComboBoxHelper::CloseComboBox(comboBoxBottom);
    }

    void ComboBoxIntegrationTests::ValidateFocusTrappedWithNoSelectionPressingGamepadUp()
    {
        TestCleanupWrapper cleanup;

        ::Windows::Foundation::Size size(400, 400);
        TestServices::WindowHelper->SetWindowSizeOverride(size);

        xaml_controls::ComboBox^ comboBox = nullptr;
        xaml_controls::Button^ focusableButton = nullptr;

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"  <ScrollViewer>"
                L"    <StackPanel>"
                L"      <Button x:Name='focusableButton' Content='Focusable Button' />"
                L"      <Grid Height='1000' Background='LightBlue' />"
                L"      <ComboBox x:Name='comboBox'>"
                L"        <ComboBoxItem Content='item one' />"
                L"        <ComboBoxItem Content='item two' />"
                L"        <ComboBoxItem Content='item three' />"
                L"        <ComboBoxItem Content='item four' />"
                L"      </ComboBox>"
                L"    </StackPanel>"
                L"  </ScrollViewer>"
                L"</Grid>"));

            comboBox = dynamic_cast<xaml_controls::ComboBox^>(rootPanel->FindName(L"comboBox"));
            VERIFY_IS_NOT_NULL(comboBox);

            focusableButton = dynamic_cast<xaml_controls::Button^>(rootPanel->FindName(L"focusableButton"));
            VERIFY_IS_NOT_NULL(focusableButton);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        ComboBoxHelper::OpenComboBox(comboBox, ComboBoxHelper::OpenMethod::Programmatic);

        // Even though the ComboBox does not have a selected item, pressing up should /not/
        // go to the focusable button, but rather remain trapped in the ComboBox.
        TestServices::KeyboardHelper->GamepadDpadUp();
        TestServices::WindowHelper->WaitForIdle();

        // Verify that the focusable button does not have focus.
        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(focusableButton->FocusState == FocusState::Unfocused);
        });

        ComboBoxHelper::CloseComboBox(comboBox, ComboBoxHelper::CloseMethod::Programmatic);
    }

    void ComboBoxIntegrationTests::ValidateTriggerKeyFocusNavigation()
    {
        TestCleanupWrapper cleanup;

        int totalNumberOfItems = 9;
        int expectedNumberOfItemsToScrollWithTriggers = 8;

        xaml_controls::ComboBox^ comboBox = SetupBasicComboBoxTest(totalNumberOfItems, false /* adjustMargin */);

        RunOnUIThread([&]()
        {
            comboBox->SelectedIndex = 0;
        });
        TestServices::WindowHelper->WaitForIdle();

        ComboBoxHelper::OpenComboBox(comboBox, ComboBoxHelper::OpenMethod::Programmatic);

        // Pressing the trigger key should not move selection, only focus
        LOG_OUTPUT(L"Changing selection with RightTrigger");
        TestServices::KeyboardHelper->GamepadRightTrigger();
        TestServices::WindowHelper->WaitForIdle();
        ComboBoxHelper::VerifySelectedIndex(comboBox, 0);

        // Pressing Gamepad A "commits" the selection
        TestServices::KeyboardHelper->GamepadA();
        TestServices::WindowHelper->WaitForIdle();

        // Verify that selection has now changed and the proper item has been selected
        ComboBoxHelper::VerifySelectedIndex(comboBox, expectedNumberOfItemsToScrollWithTriggers);

        ComboBoxHelper::OpenComboBox(comboBox, ComboBoxHelper::OpenMethod::Programmatic);

        // Pressing the trigger will move focus to the first item, but not change selection
        LOG_OUTPUT(L"Changing selection with LeftTrigger");
        TestServices::KeyboardHelper->GamepadLeftTrigger();
        TestServices::WindowHelper->WaitForIdle();
        ComboBoxHelper::VerifySelectedIndex(comboBox, expectedNumberOfItemsToScrollWithTriggers);

        // Pressing Gamepad A "commits" the selection
        TestServices::KeyboardHelper->GamepadA();
        TestServices::WindowHelper->WaitForIdle();

        // The selection is now the first item
        ComboBoxHelper::VerifySelectedIndex(comboBox, 0);
    }

    void ComboBoxIntegrationTests::ValidateTriggerKeysDoNotChangeSelection()
    {
        TestCleanupWrapper cleanup;

        int totalNumberOfItems = 5;

        xaml_controls::ComboBox^ comboBox = SetupBasicComboBoxTest(totalNumberOfItems);

        RunOnUIThread([&]()
        {
            comboBox->SelectedIndex = 1;
        });
        TestServices::WindowHelper->WaitForIdle();

        ComboBoxHelper::OpenComboBox(comboBox, ComboBoxHelper::OpenMethod::Programmatic);

        // Selection starts on item at index 1
        ComboBoxHelper::VerifySelectedIndex(comboBox, 1);

        // Pressing the trigger keys moves focus to item 5, but not selection
        LOG_OUTPUT(L"Changing selection with RightTrigger");
        TestServices::KeyboardHelper->GamepadRightTrigger();
        TestServices::WindowHelper->WaitForIdle();
        ComboBoxHelper::VerifySelectedIndex(comboBox, 1);

        // Similarly, pressing the dpad should also not move selection
        LOG_OUTPUT(L"Changing selection with GamepadDpadUp");
        TestServices::KeyboardHelper->GamepadDpadUp();
        TestServices::WindowHelper->WaitForIdle();
        ComboBoxHelper::VerifySelectedIndex(comboBox, 1);

        LOG_OUTPUT(L"Changing selection with LeftTrigger");
        TestServices::KeyboardHelper->GamepadLeftTrigger();
        TestServices::WindowHelper->WaitForIdle();
        ComboBoxHelper::VerifySelectedIndex(comboBox, 1);

        ComboBoxHelper::CloseComboBox(comboBox, ComboBoxHelper::CloseMethod::Programmatic);

        // Finally, since we close the ComboBox Programmatically and not with A, selection remains on item 1
        ComboBoxHelper::VerifySelectedIndex(comboBox, 1);
    }

    void ComboBoxIntegrationTests::ValidateOpenedDropDownHeightWithMouseAfterTouchInput()
    {
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(600, 600));
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

        // Validate the ComboBox DComp output when ComboBox is opened with the mouse input
        // after opened/closed the ComboBox with the touch input.
        auto comboBox = SetupBasicComboBoxTest(20 /* itemSize */, false /* adjustMargin */);

        RunOnUIThread([&]()
        {
            comboBox->Height = 50;
            comboBox->VerticalAlignment = VerticalAlignment::Center;
        });
        TestServices::WindowHelper->WaitForIdle();

        ComboBoxHelper::OpenComboBox(comboBox, ComboBoxHelper::OpenMethod::Mouse);
        ComboBoxHelper::CloseComboBox(comboBox);

        ComboBoxHelper::OpenComboBox(comboBox, ComboBoxHelper::OpenMethod::Touch);
        ComboBoxHelper::CloseComboBox(comboBox);

        ComboBoxHelper::OpenComboBox(comboBox, ComboBoxHelper::OpenMethod::Mouse);
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
        ComboBoxHelper::CloseComboBox(comboBox);
    }

    void ComboBoxIntegrationTests::ValidateOpeningWithPendingClosedEventDoesNotCloseComboBox()
    {
        TestCleanupWrapper cleanup;

        auto comboBox = SetupBasicComboBoxTest();

        auto dropDownClosedEvent = std::make_shared<Event>();
        auto dropDownClosedRegistration = CreateSafeEventRegistration(xaml_controls::ComboBox, DropDownClosed);

        dropDownClosedRegistration.Attach(
            comboBox,
            [&]()
            {
                dropDownClosedRegistration.Detach();
                dropDownClosedEvent->Set();

                // In order to ensure that we've reopened the ComboBox's drop-down before
                // Popup.Closed is handled, we'll set IsDropDownOpen to true inside the handler
                // for DropDownClosed.  DropDownClosed is raised at the same time that we
                // initially close the Popup, meaning that Popup.Closed will have been raised
                // at this point, but not yet handled.
                RunOnUIThread([&]()
                {
                    comboBox->IsDropDownOpen = true;
                });
            });

        ComboBoxHelper::OpenComboBox(comboBox, ComboBoxHelper::OpenMethod::Programmatic);
        ComboBoxHelper::CloseComboBox(comboBox);

        dropDownClosedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(comboBox->IsDropDownOpen);
        });

        ComboBoxHelper::CloseComboBox(comboBox);
    }

    void ComboBoxIntegrationTests::CanSetIsDropDownOpenBeforeTemplateIsAppliedAndGotFocus()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::ComboBox^ comboBox = nullptr;
        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::ComboBox, Loaded);

        RunOnUIThread([&]()
        {
            comboBox = ref new xaml_controls::ComboBox();
            comboBox->PlaceholderText = "Placeholder Text";
            comboBox->Items->Append("Item #1");
            comboBox->Items->Append("Item #2");
            comboBox->Items->Append("Item #3");

            // We set IsDropDownOpen to true before the template is applied.
            // ComboBox's behavior is to close (IsDropDownOpen == false) when
            // the template gets eventually applied.
            // In doing so, it should correctly restore the "placeholder"
            // in the template. It can fail to do that correctly
            // and we crash if we were given focus.
            // The focus visual state targets the placeholder text and,
            // if it's not there, it will fail to resolve and we raise an
            // exception.
            comboBox->IsDropDownOpen = true;

            loadedRegistration.Attach(comboBox, ref new xaml::RoutedEventHandler([&](Platform::Object^ sender, xaml::RoutedEventArgs^)
            {
                LOG_OUTPUT(L"ComboBox loaded");
                comboBox->Focus(FocusState::Keyboard);
                loadedEvent->Set();
            }));

            TestServices::WindowHelper->WindowContent = comboBox;
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_FALSE(comboBox->IsDropDownOpen);
        });
    }

    void ComboBoxIntegrationTests::DoesNotChangeSelectedIndexWithHorizontalArrowKeys()
    {
        TestCleanupWrapper cleanup;

        // Set a KeyboardWaitKind of None to work around a known issue.
        KeyboardInjectionIgnoreEventWaitOverride keyboardEventsOverride(KeyboardWaitKind::None);

        const UINT numItems = 3;
        xaml_controls::ComboBox^ comboBox = SetupBasicComboBoxTest(numItems);

        Event selectionChangedEvent;
        auto selectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::ComboBox, SelectionChanged);
        selectionChangedRegistration.Attach(comboBox, [&]() { selectionChangedEvent.Set(); });

        RunOnUIThread([&]()
        {
            WEX::Common::Throw::If(comboBox->SelectedIndex != -1, E_FAIL, L"Nothing should be selected by default.");

            // Make sure the ComboBox has keyboard focus before trying to interact.
            comboBox->Focus(xaml::FocusState::Keyboard);
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Press Right %d times, once for each item -> SelectedIndex should not change.", numItems);
        for (size_t i = 0; i < numItems; ++i)
        {
            CommonInputHelper::Right(InputDevice::Keyboard);
            TestServices::WindowHelper->WaitForIdle();

            VERIFY_IS_FALSE(selectionChangedEvent.HasFired());
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(-1, comboBox->SelectedIndex);
            });
        }

        LOG_OUTPUT(L"Press Left %d times, once for each item -> SelectedIndex should not change.", numItems);
        for (size_t i = 0; i < numItems; ++i)
        {
            CommonInputHelper::Left(InputDevice::Keyboard);
            TestServices::WindowHelper->WaitForIdle();

            VERIFY_IS_FALSE(selectionChangedEvent.HasFired());
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(-1, comboBox->SelectedIndex);
            });
        }
    }

    void ComboBoxIntegrationTests::CanHorizontallyNavigatePastAComboBox()
    {
        TestCleanupWrapper cleanup;

        // Set a KeyboardWaitKind of None to work around a known issue.
        KeyboardInjectionIgnoreEventWaitOverride keyboardEventsOverride(KeyboardWaitKind::None);

        xaml_controls::ComboBox^ comboBox = nullptr;
        xaml_controls::Button^ firstButton = nullptr;
        xaml_controls::Button^ lastButton = nullptr;

        Event gotFocusEvent;
        auto gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::StackPanel, GotFocus);

        RunOnUIThread([&]()
        {
            comboBox = ref new xaml_controls::ComboBox();
            comboBox->Margin = xaml::ThicknessHelper::FromUniformLength(25);

            firstButton = ref new xaml_controls::Button();
            firstButton->Content = L"First Button";

            lastButton = ref new xaml_controls::Button();
            lastButton->Content = L"Last Button";

            auto root = ref new xaml_controls::StackPanel();
            root->HorizontalAlignment = xaml::HorizontalAlignment::Center;
            root->VerticalAlignment = xaml::VerticalAlignment::Center;
            root->Orientation = xaml_controls::Orientation::Horizontal;
            root->XYFocusKeyboardNavigation = xaml_input::XYFocusKeyboardNavigationMode::Enabled;

            root->Children->Append(firstButton);
            root->Children->Append(comboBox);
            root->Children->Append(lastButton);

            gotFocusRegistration.Attach(root, [&]() {gotFocusEvent.Set(); });

            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // Make sure the first button has keyboard focus before trying to interact.
            firstButton->Focus(xaml::FocusState::Keyboard);
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Move Right 2 times, which should move past the ComboBox and land on the last button.");
        CommonInputHelper::Right(InputDevice::Keyboard);
        TestServices::WindowHelper->WaitForIdle();
        gotFocusEvent.WaitForDefault();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(comboBox));
        });

        CommonInputHelper::Right(InputDevice::Keyboard);
        TestServices::WindowHelper->WaitForIdle();
        gotFocusEvent.WaitForDefault();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(lastButton));
        });

        LOG_OUTPUT(L"Move Left 2 times, which should move past the ComboBox and land back on the first button.");
        CommonInputHelper::Left(InputDevice::Keyboard);
        TestServices::WindowHelper->WaitForIdle();
        gotFocusEvent.WaitForDefault();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(comboBox));
        });

        CommonInputHelper::Left(InputDevice::Keyboard);
        TestServices::WindowHelper->WaitForIdle();
        gotFocusEvent.WaitForDefault();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(firstButton));
        });
    }

    void ComboBoxIntegrationTests::DoesRevertSelectionOnCancel()
    {
        TestCleanupWrapper cleanup;

        // Set a KeyboardWaitKind of None to work around a known issue.
        KeyboardInjectionIgnoreEventWaitOverride keyboardEventsOverride(KeyboardWaitKind::None);

        const UINT numItems = 3;
        xaml_controls::ComboBox^ comboBox = SetupBasicComboBoxTest(numItems);

        const int expectedSelectedIndex = 1;

        Event selectionChangedEvent;
        auto selectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::ComboBox, SelectionChanged);
        selectionChangedRegistration.Attach(comboBox, [&]() { selectionChangedEvent.Set(); });

        RunOnUIThread([&]()
        {
            // Set a selected index that we'll verify gets reverted back to.
            comboBox->SelectedIndex = expectedSelectedIndex;

            // Make sure SelectedIndex will change as we navigate through the items.
            comboBox->SelectionChangedTrigger = xaml_controls::ComboBoxSelectionChangedTrigger::Always;

            // Make sure the ComboBox has keyboard focus before trying to interact.
            comboBox->Focus(xaml::FocusState::Keyboard);
        });
        TestServices::WindowHelper->WaitForIdle();

        auto runScenario = [&](std::function<void()> cancelAction)
        {
            FocusTestHelper::EnsureFocus(comboBox, FocusState::Keyboard);

            RunOnUIThread([&]()
            {
                comboBox->IsDropDownOpen = true;
            });
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Press down to change the selection.");
            CommonInputHelper::Down(InputDevice::Keyboard);
            TestServices::WindowHelper->WaitForIdle();
            selectionChangedEvent.WaitForDefault();

            LOG_OUTPUT(L"Cancel the selection.");
            cancelAction();
            TestServices::WindowHelper->WaitForIdle();
            selectionChangedEvent.WaitForDefault();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(expectedSelectedIndex, comboBox->SelectedIndex);
            });
            TestServices::WindowHelper->WaitForIdle();
        };

        LOG_OUTPUT(L"Validate reverting selection on cancel using Escape key.");
        runScenario([]() { CommonInputHelper::Cancel(InputDevice::Keyboard); });

        LOG_OUTPUT(L"Validate reverting selection on cancel using GamepadB.");
        runScenario([]() { CommonInputHelper::Cancel(InputDevice::Gamepad); });

        LOG_OUTPUT(L"Validate reverting selection on cancel by clicking outside with mouse.");
        runScenario([&]() { ComboBoxHelper::CloseComboBox(comboBox, ComboBoxHelper::CloseMethod::Mouse); });

        LOG_OUTPUT(L"Validate reverting selection on cancel by tapping outside.");
        runScenario([&]() { ComboBoxHelper::CloseComboBox(comboBox, ComboBoxHelper::CloseMethod::Touch); });
    }

    void ComboBoxIntegrationTests::DoesRevertSelectionOnCancelWithManyItems()
    {
        TestCleanupWrapper cleanup;

        // Set a KeyboardWaitKind of None to work around a known issue.
        KeyboardInjectionIgnoreEventWaitOverride keyboardEventsOverride(KeyboardWaitKind::None);

        const UINT numItems = 500;
        xaml_controls::ComboBox^ comboBox = SetupBasicComboBoxTest(numItems);

        const int expectedSelectedIndex = 1;

        Event selectionChangedEvent;
        auto selectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::ComboBox, SelectionChanged);
        selectionChangedRegistration.Attach(comboBox, [&]() { selectionChangedEvent.Set(); });

        RunOnUIThread([&]()
        {
            // Set a selected index that we'll verify gets reverted back to.
            comboBox->SelectedIndex = expectedSelectedIndex;

            // Make sure SelectedIndex will change as we navigate through the items.
            comboBox->SelectionChangedTrigger = xaml_controls::ComboBoxSelectionChangedTrigger::Always;

            // Make sure the ComboBox has keyboard focus before trying to interact.
            comboBox->Focus(xaml::FocusState::Keyboard);

            comboBox->IsDropDownOpen = true;
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Press End to change the selection to the last item.");
        TestServices::KeyboardHelper->End();
        TestServices::WindowHelper->WaitForIdle();
        selectionChangedEvent.WaitForDefault();

        LOG_OUTPUT(L"Cancel the selection.");
        CommonInputHelper::Cancel(InputDevice::Keyboard);
        TestServices::WindowHelper->WaitForIdle();
        selectionChangedEvent.WaitForDefault();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(expectedSelectedIndex, comboBox->SelectedIndex);
        });
    }

    void ComboBoxIntegrationTests::ValidateSelectionChangedTrigger()
    {
        TestCleanupWrapper cleanup;

        // Set a KeyboardWaitKind of None to work around a known issue.
        KeyboardInjectionIgnoreEventWaitOverride keyboardEventsOverride(KeyboardWaitKind::None);

        const size_t numItems = 3;
        xaml_controls::ComboBox^ comboBox = SetupBasicComboBoxTest(numItems);

        Event selectionChangedEvent;
        auto selectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::ComboBox, SelectionChanged);
        selectionChangedRegistration.Attach(comboBox, [&]() { selectionChangedEvent.Set(); });

        RunOnUIThread([&]()
        {
            // Validate the default value for SelectionChangedTrigger is Committed.
            VERIFY_ARE_EQUAL(xaml_controls::ComboBoxSelectionChangedTrigger::Committed, comboBox->SelectionChangedTrigger);
        });
        TestServices::WindowHelper->WaitForIdle();

        auto runScenario = [&](InputDevice inputDevice, xaml_controls::ComboBoxSelectionChangedTrigger trigger)
        {
            FocusTestHelper::EnsureFocus(comboBox, FocusState::Keyboard);

            RunOnUIThread([&]()
            {
                comboBox->SelectedIndex = -1;
                comboBox->SelectionChangedTrigger = trigger;
            });

            selectionChangedEvent.Reset();

            // Open the ComboBox.
            CommonInputHelper::Accept(inputDevice);
            TestServices::WindowHelper->WaitForIdle();

            if (trigger == xaml_controls::ComboBoxSelectionChangedTrigger::Always)
            {
                LOG_OUTPUT(L"Wait for the SelectionChanged event on the first element to fire.");
                selectionChangedEvent.WaitForDefault();
            }

            LOG_OUTPUT(L"Navigate through the items.");
            for (size_t i = 1; i < numItems; ++i)
            {
                CommonInputHelper::Down(inputDevice);
                TestServices::WindowHelper->WaitForIdle();

                if (trigger == xaml_controls::ComboBoxSelectionChangedTrigger::Always)
                {
                    LOG_OUTPUT(L"Wait for the SelectionChanged event to fire.");
                    selectionChangedEvent.WaitForDefault();
                }
                else
                {
                    VERIFY_IS_FALSE(selectionChangedEvent.HasFired());
                }
            }

            selectionChangedEvent.Reset();

            LOG_OUTPUT(L"Accept the last item as the new selection");
            CommonInputHelper::Accept(inputDevice);
            TestServices::WindowHelper->WaitForIdle();

            if (trigger == xaml_controls::ComboBoxSelectionChangedTrigger::Committed)
            {
                LOG_OUTPUT(L"Wait for the SelectionChanged event to fire.");
                selectionChangedEvent.WaitForDefault();
            }
            else
            {
                VERIFY_IS_FALSE(selectionChangedEvent.HasFired());
            }

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(static_cast<int>(numItems - 1), comboBox->SelectedIndex);
            });
            TestServices::WindowHelper->WaitForIdle();
        };

        LOG_OUTPUT(L"Run scenario with keyboard input and SelectionChangedTrigger = Committed.");
        runScenario(InputDevice::Keyboard, xaml_controls::ComboBoxSelectionChangedTrigger::Committed);

        LOG_OUTPUT(L"Run scenario with keyboard input and SelectionChangedTrigger = Always.");
        runScenario(InputDevice::Keyboard, xaml_controls::ComboBoxSelectionChangedTrigger::Always);

        LOG_OUTPUT(L"Run scenario with gamepad input and SelectionChangedTrigger = Committed.");
        runScenario(InputDevice::Gamepad, xaml_controls::ComboBoxSelectionChangedTrigger::Committed);

        LOG_OUTPUT(L"Run scenario with gamepad input and SelectionChangedTrigger = Always.");
        runScenario(InputDevice::Gamepad, xaml_controls::ComboBoxSelectionChangedTrigger::Always);
    }

    void ComboBoxIntegrationTests::DoHomeAndEndChangeSelectionWhenClosed()
    {
        TestCleanupWrapper cleanup;

        // Set a KeyboardWaitKind of None to work around a known issue.
        KeyboardInjectionIgnoreEventWaitOverride keyboardEventsOverride(KeyboardWaitKind::None);

        const UINT numItems = 3;
        xaml_controls::ComboBox^ comboBox = SetupBasicComboBoxTest(numItems);

        Event selectionChangedEvent;
        auto selectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::ComboBox, SelectionChanged);
        selectionChangedRegistration.Attach(comboBox, [&]() { selectionChangedEvent.Set(); });

        RunOnUIThread([&]()
        {
            // Make sure the ComboBox has keyboard focus before trying to interact.
            comboBox->Focus(xaml::FocusState::Keyboard);
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Press the Home key -> the first item should get selected.");
        TestServices::KeyboardHelper->Home();
        TestServices::WindowHelper->WaitForIdle();
        selectionChangedEvent.WaitForDefault();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(0, comboBox->SelectedIndex);
        });

        LOG_OUTPUT(L"Press the End key -> the last item should get selected.");
        TestServices::KeyboardHelper->End();
        TestServices::WindowHelper->WaitForIdle();
        selectionChangedEvent.WaitForDefault();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(static_cast<int>(numItems - 1), comboBox->SelectedIndex);
        });
    }

    void ComboBoxIntegrationTests::ValidateSelectedInfoPropertiesStayInSync()
    {
        TestCleanupWrapper cleanup;

        // Set a KeyboardWaitKind of None to work around a known issue.
        KeyboardInjectionIgnoreEventWaitOverride keyboardEventsOverride(KeyboardWaitKind::None);

        xaml_controls::ComboBox^ comboBox = nullptr;

        Event selectionChangedEvent;
        auto selectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::ComboBox, SelectionChanged);

        std::array<PersonObject^, 3> personObjects = {
            ref new PersonObject(L"Roger", L"Federer"),
            ref new PersonObject(L"Cristiano", L"Ronaldo"),
            ref new PersonObject(L"LeBron", L"James")
        };

        RunOnUIThread([&]()
        {
            comboBox = ref new xaml_controls::ComboBox();

            for (auto personObject : personObjects)
            {
                comboBox->Items->Append(personObject);
            }

            comboBox->DisplayMemberPath = L"FirstName";
            comboBox->SelectedValuePath = L"LastName";

            selectionChangedRegistration.Attach(comboBox, [&]() { selectionChangedEvent.Set(); });

            TestServices::WindowHelper->WindowContent = comboBox;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(personObjects.size(), comboBox->Items->Size);

            LOG_OUTPUT(L"Validate that all Selected info properties stay in sync when changing SelectedIndex.");
            for (int i = 0; i < static_cast<int>(personObjects.size()); ++i)
            {
                comboBox->SelectedIndex = i;
                VERIFY_ARE_EQUAL(personObjects[i], comboBox->SelectedItem);
                VERIFY_ARE_EQUAL(safe_cast<PersonObject^>(comboBox->Items->GetAt(i))->LastName, safe_cast<Platform::String^>(comboBox->SelectedValue));
            }

            LOG_OUTPUT(L"Validate that all Selected info properties stay in sync when changing SelectedItem.");
            for (int i = 0; i < static_cast<int>(personObjects.size()); ++i)
            {
                comboBox->SelectedItem = personObjects[i];
                VERIFY_ARE_EQUAL(i, comboBox->SelectedIndex);
                VERIFY_ARE_EQUAL(safe_cast<PersonObject^>(comboBox->Items->GetAt(i))->LastName, safe_cast<Platform::String^>(comboBox->SelectedValue));
            }

            LOG_OUTPUT(L"Validate that all Selected info properties stay in sync when changing SelectedValue.");
            for (int i = 0; i < static_cast<int>(personObjects.size()); ++i)
            {
                comboBox->SelectedValue = personObjects[i]->LastName;
                VERIFY_ARE_EQUAL(i, comboBox->SelectedIndex);
                VERIFY_ARE_EQUAL(personObjects[i], comboBox->SelectedItem);
            }
        });
    }

    void ComboBoxIntegrationTests::DoesNotShowMulitpleSelectionVisuals()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::ComboBox^ comboBox = SetupBasicComboBoxTest(2);
        xaml_controls::ComboBoxItem^ firstComboBoxItem = nullptr;
        xaml_controls::ComboBoxItem^ secondComboBoxItem = nullptr;

        RunOnUIThread([&]()
        {
            WEX::Common::Throw::IfFalse(comboBox->SelectionChangedTrigger == xaml_controls::ComboBoxSelectionChangedTrigger::Committed, E_FAIL, L"Expected that the ComboBox is in commit mode.");

            comboBox->Focus(xaml::FocusState::Keyboard);
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Press SPACE to open the ComboBox.");
        TestServices::KeyboardHelper->Space();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            firstComboBoxItem = safe_cast<xaml_controls::ComboBoxItem^>(comboBox->Items->GetAt(0));
            secondComboBoxItem = safe_cast<xaml_controls::ComboBoxItem^>(comboBox->Items->GetAt(1));
        });

        LOG_OUTPUT(L"Press DOWN to select the first item.");
        TestServices::KeyboardHelper->Down();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Press SPACE to commit the selection.");
        TestServices::KeyboardHelper->Space();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Press DOWN to change the selected item to the second item.");
        TestServices::KeyboardHelper->Down();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Press SPACE to open the ComboBox again.");
        TestServices::KeyboardHelper->Space();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_FALSE(ControlHelper::IsInVisualState(firstComboBoxItem, L"CommonStates", L"Selected"));
            VERIFY_IS_TRUE(ControlHelper::IsInVisualState(secondComboBoxItem, L"CommonStates", L"Selected"));
        });

        ComboBoxHelper::CloseComboBox(comboBox);
    }

    void ComboBoxIntegrationTests::VerifyItemBeforeVisibleItemsInPopupIsRealized()
    {
        TestCleanupWrapper cleanup;

        // Regression coverage for:
        // Narrator says "No previous item" when item navigating backwards through "Add a braille display" combo box
        // We want to verify that when the ComboBox is opened in non-carouselling mode (e.g. with mouse) and scrolled to the bottom
        // (e.g. the last item is selected) then the item before the visible range has been realized.
        xaml_controls::ComboBox^ comboBox;

        RunOnUIThread([&]()
        {
            comboBox = ref new xaml_controls::ComboBox();
            comboBox->VerticalAlignment = xaml::VerticalAlignment::Center;

            auto rootPanel = ref new xaml_controls::Grid();
            rootPanel->Children->Append(comboBox);

            for (int i = 0; i < 100; i++)
            {
                auto item = ref new xaml_controls::ComboBoxItem();
                auto stringItem = ref new Platform::String(L"ComboBoxItem");
                stringItem += i;
                item->Content = stringItem;
                item->Height = 40;
                item->Name = stringItem;
                comboBox->Items->Append(item);
            }

            // We choose a MaxDropDownHeight so that 7 items will fit in the popup:
            comboBox->MaxDropDownHeight = 320;

            // Set SelectedIndex to the last item:
            comboBox->SelectedIndex = 99;

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        ComboBoxHelper::OpenComboBox(comboBox, ComboBoxHelper::OpenMethod::Mouse);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto carouselPanel = TreeHelper::GetVisualChildByTypeFromOpenPopups<xaml_primitives::CarouselPanel>(comboBox);

            // Verify that the item before the first visible item has been realized (99-7=92).
            auto item92 = safe_cast<xaml_controls::ComboBoxItem^>(TreeHelper::GetVisualChildByName(carouselPanel, L"ComboBoxItem92"));
            VERIFY_IS_NOT_NULL(item92, L"Expected the item to be realized");

            // Verify that this item is not visible in the popup:
            auto popupBorder = safe_cast<xaml::FrameworkElement^>(TreeHelper::GetVisualChildByNameFromOpenPopups(L"PopupBorder", comboBox));
            auto popupBorderBounds = ControlHelper::GetBounds(popupBorder);
            LOG_OUTPUT(L"popupBorder bounds: %f, %f, %f, %f", popupBorderBounds.Left, popupBorderBounds.Top, popupBorderBounds.Width, popupBorderBounds.Height);

            auto item92Bounds = ControlHelper::GetBounds(item92);
            LOG_OUTPUT(L"Item92 bounds: %f, %f, %f, %f", item92Bounds.Left, item92Bounds.Top, item92Bounds.Width, item92Bounds.Height);
            VERIFY_IS_FALSE(ControlHelper::IsContainedIn(item92Bounds, popupBorderBounds));
        });

        ComboBoxHelper::CloseComboBox(comboBox);
    }

    void ComboBoxIntegrationTests::CanSetEditableMode()
    {
        TestCleanupWrapper cleanup;

        ::Windows::Foundation::Size size(400, 400);
        TestServices::WindowHelper->SetWindowSizeOverride(size);

        xaml_controls::ComboBox^ comboBox = nullptr;

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"  <ComboBox x:Name='comboBox' IsEditable='true'>"
                L"      <ComboBoxItem Content='item one' />"
                L"      <ComboBoxItem Content='item two' />"
                L"      <ComboBoxItem Content='item three' />"
                L"      <ComboBoxItem Content='item four' />"
                L"  </ComboBox>"
                L"</Grid>"));

            comboBox = dynamic_cast<xaml_controls::ComboBox^>(rootPanel->FindName(L"comboBox"));
            VERIFY_IS_NOT_NULL(comboBox);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(comboBox->IsEditable, true);

            comboBox->IsEditable = false;
        });

        TestServices::WindowHelper->WaitForIdle();


        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(comboBox->IsEditable, false);
        });
    }

    void ComboBoxIntegrationTests::CanSetComboBoxTextOnNonEditableMode()
    {
        TestCleanupWrapper cleanup;

        ::Windows::Foundation::Size size(400, 400);
        TestServices::WindowHelper->SetWindowSizeOverride(size);

        xaml_controls::ComboBox^ comboBox = nullptr;
        xaml_controls::TextBox^ editableTextBox = nullptr;

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"  <ComboBox x:Name='comboBox'>"
                L"      <ComboBoxItem Content='item one' />"
                L"      <ComboBoxItem Content='item two' />"
                L"      <ComboBoxItem Content='item three' />"
                L"      <ComboBoxItem Content='item four' />"
                L"  </ComboBox>"
                L"</Grid>"));

            comboBox = dynamic_cast<xaml_controls::ComboBox^>(rootPanel->FindName(L"comboBox"));
            VERIFY_IS_NOT_NULL(comboBox);

            comboBox->Text = L"Test";

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            comboBox->IsEditable = true;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            editableTextBox = safe_cast<xaml_controls::TextBox^>(TreeHelper::GetVisualChildByName(comboBox, L"EditableText"));
            VERIFY_IS_NOT_NULL(editableTextBox);

            VERIFY_ARE_EQUAL(editableTextBox->Text, ref new Platform::String(L"Test"));
        });
    }

    void ComboBoxIntegrationTests::VerifyTabBehavior()
    {
        TestCleanupWrapper cleanup;

        ::Windows::Foundation::Size size(400, 400);
        TestServices::WindowHelper->SetWindowSizeOverride(size);

        xaml_controls::StackPanel^ stackPanel = nullptr;
        xaml_controls::Button^ button1 = nullptr;
        xaml_controls::ComboBox^ comboBox = nullptr;
        xaml_controls::TextBox^ textBox = nullptr;
        xaml_controls::Button^ button2 = nullptr;

        RunOnUIThread([&]()
        {
            button1 = ref new xaml_controls::Button();
            comboBox = dynamic_cast<xaml_controls::ComboBox^> (xaml_markup::XamlReader::Load(
                L"<ComboBox x:Name='comboBox' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"    <ComboBoxItem Content='item one' />"
                L"    <ComboBoxItem Content='item two' />"
                L"    <ComboBoxItem Content='item three' />"
                L"    <ComboBoxItem Content='item four' />"
                L"</ComboBox>"));
            VERIFY_IS_NOT_NULL(comboBox);
            button2 = ref new xaml_controls::Button();

            stackPanel = ref new xaml_controls::StackPanel();
            stackPanel->Children->Append(button1);
            stackPanel->Children->Append(comboBox);
            stackPanel->Children->Append(button2);

            TestServices::WindowHelper->WindowContent = stackPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            textBox = safe_cast<xaml_controls::TextBox^>(TreeHelper::GetVisualChildByName(comboBox, L"EditableText"));
            VERIFY_IS_NOT_NULL(textBox);

            button1->Focus(xaml::FocusState::Programmatic);
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Initial focus should be on Button 1.");
        VerifyFocusedElement(button1);

        LOG_OUTPUT(L"Pressing Tab should move focus to the ComboBox.");
        TestServices::KeyboardHelper->Tab();
        VerifyFocusedElement(comboBox);

        LOG_OUTPUT(L"Pressing Tab again should move focus to Button 2.");
        TestServices::KeyboardHelper->Tab();
        VerifyFocusedElement(button2);

        LOG_OUTPUT(L"Pressing Shift + Tab should move focus back to the ComboBox.");
        TestServices::KeyboardHelper->ShiftTab();
        VerifyFocusedElement(comboBox);

        LOG_OUTPUT(L"Pressing Shift + Tab once more should move focus back to Button1.");
        TestServices::KeyboardHelper->ShiftTab();
        VerifyFocusedElement(button1);

        RunOnUIThread([&]()
        {
            comboBox->IsEditable = true;
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Initial focus should be on Button 1.");
        VerifyFocusedElement(button1);

        LOG_OUTPUT(L"Pressing Tab should move focus to the TextBox inside the ComboBox.");
        TestServices::KeyboardHelper->Tab();
        VerifyFocusedElement(textBox);

        LOG_OUTPUT(L"Pressing Tab again should move focus to Button 2.");
        TestServices::KeyboardHelper->Tab();
        VerifyFocusedElement(button2);

        LOG_OUTPUT(L"Pressing Shift + Tab should move focus back to the TextBox inside the ComboBox.");
        TestServices::KeyboardHelper->ShiftTab();
        VerifyFocusedElement(textBox);

        LOG_OUTPUT(L"Pressing Shift + Tab once more should move focus back to Button1.");
        TestServices::KeyboardHelper->ShiftTab();
        VerifyFocusedElement(button1);
    }

    void ComboBoxIntegrationTests::VerifyFocusedElement(Platform::Object^ expectedFocusedElement)
    {
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(expectedFocusedElement));
        });
    }

    void ComboBoxIntegrationTests::InjectKeySequence(Platform::Collections::Vector<Platform::String^>^ keySequence, int timeBetweenKeyPressesInMs)
    {
        for (UINT i = 0; i < keySequence->Size; i++)
        {
            LOG_OUTPUT(L"ComboBox: Injecting key sequence");
            LOG_OUTPUT(L"%s", keySequence->GetAt(i)->Data());

            TestServices::KeyboardHelper->PressKeySequence(keySequence->GetAt(i));

            Sleep(timeBetweenKeyPressesInMs);
        }
    }

    void ComboBoxIntegrationTests::EnsureEditableTextBoxHasFocus(xaml_controls::ComboBox^ comboBox)
    {
        xaml_controls::TextBox^ textBox = nullptr;
        RunOnUIThread([&]()
        {
            textBox = safe_cast<xaml_controls::TextBox^>(TreeHelper::GetVisualChildByName(comboBox, L"EditableText"));
            VERIFY_IS_NOT_NULL(textBox);
        });
        FocusTestHelper::EnsureFocus(textBox, FocusState::Keyboard);
        TestServices::WindowHelper->WaitForIdle();
    }

    void ComboBoxIntegrationTests::CanRaiseTextSubmittedEventComboBoxClosed()
    {
        CanRaiseTextSubmittedEventComboBox(false);
    }

    void ComboBoxIntegrationTests::CanRaiseTextSubmittedEventComboBoxOpened()
    {
        CanRaiseTextSubmittedEventComboBox(true);
    }

    void ComboBoxIntegrationTests::ValidateItemAutoMatchOnTextSubmitted()
    {
        CanRaiseTextSubmittedEventComboBox(false, true);
    }

    void ComboBoxIntegrationTests::ValidateTextSubmittedHandledProperty()
    {
        CanRaiseTextSubmittedEventComboBox(false, false, true);
    }

    void ComboBoxIntegrationTests::CanRaiseTextSubmittedEventComboBox(bool open, bool addToItemSource, bool setHandled)
    {
        TestCleanupWrapper cleanup;

        auto comboBox = SetupBasicComboBoxTest(5, true, true);
        xaml_controls::ComboBoxItem^ comboBoxItem = nullptr;

        auto comboBoxTextSubmittedEvent = std::make_shared<Event>();
        auto textSubmittedRegistration = CreateSafeEventRegistration(xaml_controls::ComboBox, TextSubmitted);

        textSubmittedRegistration.Attach(comboBox, ref new wf::TypedEventHandler<xaml_controls::ComboBox^, xaml_controls::ComboBoxTextSubmittedEventArgs^>([comboBoxTextSubmittedEvent, comboBox, addToItemSource, &comboBoxItem, setHandled](xaml_controls::ComboBox^ sender, xaml_controls::ComboBoxTextSubmittedEventArgs^ args)
        {
            VERIFY_ARE_EQUAL(args->Text, ref new Platform::String(L"Custom Value"));

            if (addToItemSource)
            {
                RunOnUIThread([&]()
                {
                    comboBoxItem = ref new xaml_controls::ComboBoxItem();
                    comboBoxItem->Content = L"Custom Value";

                    comboBox->Items->Append(comboBoxItem);
                });
            }

            if (setHandled)
            {
                args->Handled = true;
            }

            comboBoxTextSubmittedEvent->Set();
        }));

        RunOnUIThread([&]()
        {
            if (setHandled)
            {
                // Set selection to something different than -1 so we can compare it stays the same.
                comboBox->SelectedIndex = 1;
            }

            if (open)
            {
                LOG_OUTPUT(L"OpenComboBox");
                comboBox->IsDropDownOpen = true;
            }
        });
        TestServices::WindowHelper->WaitForIdle();

        EnsureEditableTextBoxHasFocus(comboBox);

        TestServices::WindowHelper->WaitForIdle();

        auto keySequence = ref new Platform::Collections::Vector<Platform::String^>();
        keySequence->Append("C");
        keySequence->Append("u");
        keySequence->Append("s");
        keySequence->Append("t");
        keySequence->Append("o");
        keySequence->Append("m");
        keySequence->Append(" ");
        keySequence->Append("V");
        keySequence->Append("a");
        keySequence->Append("l");
        keySequence->Append("u");
        keySequence->Append("e");

        InjectKeySequence(keySequence, 100);
        TestServices::WindowHelper->WaitForIdle();

        TestServices::KeyboardHelper->Enter();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Waiting for ComboBox TextSubmitted Event");
        comboBoxTextSubmittedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            textSubmittedRegistration.Detach();
            if (addToItemSource)
            {
                VERIFY_ARE_EQUAL(safe_cast<xaml_controls::ComboBoxItem^>(comboBox->SelectedItem), comboBoxItem);
                VERIFY_ARE_EQUAL(safe_cast<xaml_controls::ComboBoxItem^>(comboBox->SelectedValue), comboBoxItem);
                VERIFY_ARE_EQUAL(comboBox->SelectedIndex, 5);
            }
            else if (setHandled)
            {
                auto testItem = comboBox->Items->GetAt(1);
                VERIFY_ARE_EQUAL(comboBox->SelectedItem, testItem);
                VERIFY_ARE_EQUAL(comboBox->SelectedValue, testItem);
                VERIFY_ARE_EQUAL(comboBox->SelectedIndex, 1);
            }
            else
            {
                VERIFY_ARE_EQUAL(safe_cast<Platform::String^>(comboBox->SelectedItem), ref new Platform::String(L"Custom Value"));
                VERIFY_ARE_EQUAL(safe_cast<Platform::String^>(comboBox->SelectedValue), ref new Platform::String(L"Custom Value"));
                VERIFY_ARE_EQUAL(comboBox->SelectedIndex, -1);
            }
        });
    }

    void ComboBoxIntegrationTests::ValidateEditableModeSearchAndSelectionComboBoxClosed()
    {
        ValidateEditableModeSearchAndSelection(false);
    }

    void ComboBoxIntegrationTests::ValidateEditableModeSearchAndSelectionComboBoxOpened()
    {
        ValidateEditableModeSearchAndSelection(true);
    }

    void ComboBoxIntegrationTests::ValidateEditableModeSearchAndSelection(bool open)
    {
        TestCleanupWrapper cleanup;

        ::Windows::Foundation::Size size(400, 400);
        TestServices::WindowHelper->SetWindowSizeOverride(size);

        xaml_controls::ComboBox^ comboBox = nullptr;

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"  <ComboBox x:Name='comboBox' IsEditable='true'>"
                L"      <ComboBoxItem Content='One' />"
                L"      <ComboBoxItem Content='Two' />"
                L"      <ComboBoxItem Content='Three' />"
                L"      <ComboBoxItem Content='Four' />"
                L"  </ComboBox>"
                L"</Grid>"));

            comboBox = dynamic_cast<xaml_controls::ComboBox^>(rootPanel->FindName(L"comboBox"));
            VERIFY_IS_NOT_NULL(comboBox);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        EnsureEditableTextBoxHasFocus(comboBox);

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            if (open)
            {
                LOG_OUTPUT(L"OpenComboBox");
                comboBox->IsDropDownOpen = true;
            }
        });

        TestServices::WindowHelper->WaitForIdle();

        auto keySequence = ref new Platform::Collections::Vector<Platform::String^>();
        keySequence->Append("T");
        keySequence->Append("h");

        InjectKeySequence(keySequence, 200);
        TestServices::WindowHelper->WaitForIdle();

        TestServices::KeyboardHelper->Enter();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto testItem = comboBox->Items->GetAt(2);
            VERIFY_ARE_EQUAL(comboBox->SelectedItem, testItem);
            VERIFY_ARE_EQUAL(comboBox->SelectedValue, testItem);
            VERIFY_ARE_EQUAL(comboBox->SelectedIndex, 2);
        });
    }

    void ComboBoxIntegrationTests::ValidateEditableModeSelectedItemAndValueAreSetToNull()
    {
        TestCleanupWrapper cleanup;

        auto comboBox = SetupBasicComboBoxTest(5, true, true);

        RunOnUIThread([&]()
        {
            comboBox->Focus(FocusState::Keyboard);
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            comboBox->Text = L"Custom Value";
        });

        TestServices::WindowHelper->WaitForIdle();

        TestServices::KeyboardHelper->Enter();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(safe_cast<Platform::String^>(comboBox->SelectedItem), ref new Platform::String(L"Custom Value"));
            VERIFY_ARE_EQUAL(safe_cast<Platform::String^>(comboBox->SelectedValue), ref new Platform::String(L"Custom Value"));
            VERIFY_ARE_EQUAL(comboBox->SelectedIndex, -1);

            comboBox->IsEditable = false;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(comboBox->SelectedItem, nullptr);
            VERIFY_ARE_EQUAL(comboBox->SelectedValue, nullptr);
            VERIFY_ARE_EQUAL(comboBox->SelectedIndex, -1);
        });
    }

    void ComboBoxIntegrationTests::ValidateEditableModeGamePadInteraction()
    {
        TestCleanupWrapper cleanup;

        ::Windows::Foundation::Size size(400, 400);
        TestServices::WindowHelper->SetWindowSizeOverride(size);

        xaml_controls::ComboBox^ comboBox = nullptr;
        xaml_controls::TextBox^ textBox = nullptr;
        xaml_controls::ComboBoxItem^ comboBoxItem1 = nullptr;
        xaml_controls::ComboBoxItem^ comboBoxItem2 = nullptr;

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"  <ComboBox x:Name='comboBox' IsEditable='true'>"
                L"      <ComboBoxItem x:Name='cbi1' Content='One' />"
                L"      <ComboBoxItem x:Name='cbi2' Content='Two' />"
                L"      <ComboBoxItem x:Name='cbi3' Content='Three' />"
                L"      <ComboBoxItem x:Name='cbi4' Content='Four' />"
                L"  </ComboBox>"
                L"</Grid>"));

            comboBox = dynamic_cast<xaml_controls::ComboBox^>(rootPanel->FindName(L"comboBox"));
            VERIFY_IS_NOT_NULL(comboBox);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            textBox = safe_cast<xaml_controls::TextBox^>(TreeHelper::GetVisualChildByName(comboBox, L"EditableText"));
            VERIFY_IS_NOT_NULL(textBox);

            // Programmatic/Keyboard/Pointer focus in Editable ComboBox moves the focus to the internal TextBox after the ComboBox is focused.
            // We later simulate a GamepadB press to return the focus to the ComboBox and open it.
            comboBox->Focus(FocusState::Programmatic);
        });

        TestServices::WindowHelper->WaitForIdle();

        TestServices::KeyboardHelper->GamepadB();
        TestServices::WindowHelper->WaitForIdle();

        TestServices::KeyboardHelper->GamepadA();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            comboBoxItem1 = safe_cast<xaml_controls::ComboBoxItem^>(TreeHelper::GetVisualChildByNameFromOpenPopups(L"cbi1", comboBox));
            comboBoxItem2 = safe_cast<xaml_controls::ComboBoxItem^>(TreeHelper::GetVisualChildByNameFromOpenPopups(L"cbi2", textBox));
            VERIFY_IS_NOT_NULL(comboBoxItem1);
            VERIFY_IS_NOT_NULL(comboBoxItem2);
        });

        TestServices::WindowHelper->WaitForIdle();

        CommonInputHelper::Down(InputDevice::Gamepad);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(comboBoxItem1->FocusState, FocusState::Keyboard);
        });

        CommonInputHelper::Up(InputDevice::Gamepad);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // Validate Focus can return to TextBox after moving up beyond first ComboBoxItem.
            VERIFY_ARE_EQUAL(textBox->FocusState, FocusState::Keyboard);
        });

        CommonInputHelper::Down(InputDevice::Gamepad);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(comboBoxItem2->FocusState, FocusState::Keyboard);
        });

        TestServices::KeyboardHelper->GamepadA();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(comboBox->SelectedIndex, 1);
        });
    }

    void ComboBoxIntegrationTests::ValidateEditableModePopupOpensUp()
    {
        TestCleanupWrapper cleanup;

        ::Windows::Foundation::Size size(400, 400);
        TestServices::WindowHelper->SetWindowSizeOverride(size);

        xaml_controls::ComboBox^ comboBox = nullptr;

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"  <ComboBox x:Name='comboBox' IsEditable='true' Margin='0,350,0,0'>"
                L"      <ComboBoxItem Content='One' />"
                L"      <ComboBoxItem Content='Two' />"
                L"      <ComboBoxItem Content='Three' />"
                L"  </ComboBox>"
                L"</Grid>"));

            comboBox = dynamic_cast<xaml_controls::ComboBox^>(rootPanel->FindName(L"comboBox"));
            VERIFY_IS_NOT_NULL(comboBox);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            comboBox->Focus(FocusState::Keyboard);
            comboBox->IsDropDownOpen = true;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // Verify popup verticalOffset
            auto popup = TreeHelper::GetVisualChildByType<xaml_primitives::Popup>(comboBox);
            VERIFY_ARE_EQUAL(popup->VerticalOffset, -105);

            // Verify correct margin is applied to canvas top.
            auto popupBorder = TreeHelper::GetVisualChildByNameFromOpenPopups(L"PopupBorder", comboBox);
            VERIFY_IS_NOT_NULL(popupBorder);
            VERIFY_ARE_EQUAL(xaml_controls::Canvas::GetTop(popupBorder), 1);
        });
    }

    void ComboBoxIntegrationTests::ValidateEditableModeComboBoxCanScrollToNewItems()
    {
        TestCleanupWrapper cleanup;

        ::Windows::Foundation::Size size(400, 400);
        TestServices::WindowHelper->SetWindowSizeOverride(size);

        xaml_controls::ComboBox^ comboBox = nullptr;

        RunOnUIThread([&]()
        {
            // Non latin characters are needed as this consistently virtualizes the last items.
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"  <ComboBox x:Name='comboBox' IsEditable='true'>"
                L"      <ComboBox.Items>"
                L"          <TextBlock>Text 1</TextBlock>"
                L"          <TextBlock>Text 2</TextBlock>"
                L"          <TextBlock>Text 3</TextBlock>"
                L"          <TextBlock>SuuuuuuperUltraLargeText</TextBlock>"
                L"          <TextBlock>Text 4</TextBlock>"
                L"          <TextBlock>Text 5</TextBlock>"
                L"          <TextBlock>Text 6</TextBlock>"
                L"          <TextBlock>SuuuuuuperUltraLargeText2</TextBlock>"
                L"          <TextBlock>Text 7</TextBlock>"
                L"          <TextBlock>Text 8</TextBlock>"
                L"          <TextBlock>Text 9</TextBlock>"
                L"          <TextBlock>????????????</TextBlock>"
                L"          <TextBlock>?????????</TextBlock>"
                L"          <TextBlock>??????</TextBlock>"
                L"          <TextBlock>???</TextBlock>"
                L"          <TextBlock>??????</TextBlock>"
                L"          <TextBlock>SuuuuuuperUltraLargeText3</TextBlock>"
                L"      </ComboBox.Items>"
                L"  </ComboBox>"
                L"</Grid>"));

            comboBox = dynamic_cast<xaml_controls::ComboBox^>(rootPanel->FindName(L"comboBox"));
            VERIFY_IS_NOT_NULL(comboBox);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        auto comboBoxTextSubmittedEvent = std::make_shared<Event>();
        auto textSubmittedRegistration = CreateSafeEventRegistration(xaml_controls::ComboBox, TextSubmitted);

        textSubmittedRegistration.Attach(comboBox,
            ref new wf::TypedEventHandler<xaml_controls::ComboBox^, xaml_controls::ComboBoxTextSubmittedEventArgs^>(
                [comboBoxTextSubmittedEvent, comboBox]
                (xaml_controls::ComboBox^ sender, xaml_controls::ComboBoxTextSubmittedEventArgs^ args)
        {
            VERIFY_ARE_EQUAL(args->Text, ref new Platform::String(L"Value"));

            RunOnUIThread([&]()
            {
                // Add custom value.
                auto comboBoxItem = ref new xaml_controls::ComboBoxItem();
                comboBoxItem->Content = L"Value";

                comboBox->Items->Append(comboBoxItem);
            });

            comboBoxTextSubmittedEvent->Set();
        }));

        RunOnUIThread([&]()
        {
            comboBox->Focus(FocusState::Keyboard);
            comboBox->Text = L"Value";
        });

        TestServices::WindowHelper->WaitForIdle();

        TestServices::KeyboardHelper->Enter();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Waiting for ComboBox TextSubmitted Event");
        comboBoxTextSubmittedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            textSubmittedRegistration.Detach();
            VERIFY_ARE_EQUAL(comboBox->SelectedIndex, 17);

            LOG_OUTPUT(L"OpenComboBox");
            comboBox->IsDropDownOpen = true;
        });

        TestServices::WindowHelper->WaitForIdle();

        // Selects first item of the list.
        TestServices::KeyboardHelper->PressKeySequence(L"T");
        TestServices::WindowHelper->WaitForIdle();

        TestServices::KeyboardHelper->Enter();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            comboBox->IsDropDownOpen = true;
        });

        TestServices::WindowHelper->WaitForIdle();

        // Typing the first character of the custom value, this should trigger a scroll to the searched item.
        TestServices::KeyboardHelper->PressKeySequence(L"V");
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto popup = TreeHelper::GetVisualChildByType<xaml_primitives::Popup>(comboBox);
            auto popupChild = safe_cast<FrameworkElement^>(popup->Child);
            auto scrollViewer = TreeHelper::GetVisualChildByType<xaml_controls::ScrollViewer>(popupChild);

            VERIFY_IS_NOT_NULL(scrollViewer);

            // Verify we have scrolled enough elements to have the last item visible. (Collection Size "18" - size of items visible in popup "15" = 3)
            VERIFY_ARE_EQUAL(scrollViewer->VerticalOffset, 3);
        });
    }

    void ComboBoxIntegrationTests::ValidateSettingSelectedIndexUpdatesTextBoxText()
    {
        TestCleanupWrapper cleanup;

        auto comboBox = SetupBasicComboBoxTest(5, true, true);

        xaml_controls::TextBox^ textBox = nullptr;

        RunOnUIThread([&]()
        {
            textBox = safe_cast<xaml_controls::TextBox^>(TreeHelper::GetVisualChildByName(comboBox, L"EditableText"));
            VERIFY_IS_NOT_NULL(textBox);
        });

        EnsureEditableTextBoxHasFocus(comboBox);

        TestServices::WindowHelper->WaitForIdle();

        TestServices::KeyboardHelper->PressKeySequence(L"C");
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(textBox->Text, ref new Platform::String(L"ComboBox Item 0"));

            comboBox->SelectedIndex = 2;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(textBox->Text, ref new Platform::String(L"ComboBox Item 2"));
        });
    }

    void ComboBoxIntegrationTests::ValidateSelectionTriggerAlwaysCanSetCustomValueAgain()
    {
        TestCleanupWrapper cleanup;

        ::Windows::Foundation::Size size(400, 400);
        TestServices::WindowHelper->SetWindowSizeOverride(size);

        xaml_controls::ComboBox^ comboBox = nullptr;

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"  <ComboBox x:Name='comboBox' IsEditable='true' SelectionChangedTrigger='Always'>"
                L"      <ComboBoxItem Content='One' />"
                L"      <ComboBoxItem Content='Two' />"
                L"      <ComboBoxItem Content='Three' />"
                L"      <ComboBoxItem Content='Four' />"
                L"  </ComboBox>"
                L"</Grid>"));

            comboBox = dynamic_cast<xaml_controls::ComboBox^>(rootPanel->FindName(L"comboBox"));
            VERIFY_IS_NOT_NULL(comboBox);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        EnsureEditableTextBoxHasFocus(comboBox);

        TestServices::WindowHelper->WaitForIdle();

        TestServices::KeyboardHelper->PressKeySequence(L"Hello");
        TestServices::WindowHelper->WaitForIdle();

        // Set Hello as the active custom value.
        TestServices::KeyboardHelper->Enter();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(safe_cast<Platform::String^>(comboBox->SelectedItem), ref new Platform::String(L"Hello"));
            VERIFY_ARE_EQUAL(comboBox->SelectedIndex, -1);

            comboBox->IsDropDownOpen = true;
        });

        TestServices::WindowHelper->WaitForIdle();

        // Arrow down through the ComboBox values, this will cause the SelectedIndex to change.
        TestServices::KeyboardHelper->Down();
        TestServices::KeyboardHelper->Down();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(comboBox->SelectedIndex, 1);
        });

        TestServices::KeyboardHelper->PressKeySequence(L"Hello");
        TestServices::WindowHelper->WaitForIdle();

        // Try to commit Hello again.
        TestServices::KeyboardHelper->Enter();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(safe_cast<Platform::String^>(comboBox->SelectedItem), ref new Platform::String(L"Hello"));
            VERIFY_ARE_EQUAL(comboBox->SelectedIndex, -1);
        });
    }

    void ComboBoxIntegrationTests::ValidateDropDownArrowClosesPopupOnEditableComboBox()
    {
        TestCleanupWrapper cleanup;

        auto comboBox = SetupBasicComboBoxTest(5, true, true);

        xaml_controls::Border^ dropDownOverlay = nullptr;

        RunOnUIThread([&]()
        {
            dropDownOverlay = safe_cast<xaml_controls::Border^>(TreeHelper::GetVisualChildByName(comboBox, L"DropDownOverlay"));
            VERIFY_IS_NOT_NULL(dropDownOverlay);

            comboBox->Focus(FocusState::Keyboard);
            comboBox->IsDropDownOpen = true;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(comboBox->IsDropDownOpen, true);
        });

        TestServices::KeyboardHelper->Down();
        TestServices::KeyboardHelper->Down();
        TestServices::WindowHelper->WaitForIdle();

        TestServices::InputHelper->Tap(dropDownOverlay);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(comboBox->IsDropDownOpen, false);
            VERIFY_ARE_EQUAL(comboBox->SelectedIndex, 1);
        });
    }

    void ComboBoxIntegrationTests::ValidateDropDownOverlayVisuals()
    {
        TestCleanupWrapper cleanup;

        ValidateDropDownOverlayVisualsHelper(xaml::ElementTheme::Dark, false /*useHighContrast*/, L"Dark");
        ValidateDropDownOverlayVisualsHelper(xaml::ElementTheme::Light, false /*useHighContrast*/, L"Light");
        ValidateDropDownOverlayVisualsHelper(xaml::ElementTheme::Light, true /*useHighContrast*/, L"HC");
    }

    void ComboBoxIntegrationTests::ValidateDropDownOverlayVisualsHelper(xaml::ElementTheme theme, bool useHighContrast, Platform::String ^ variation)
    {
        ::Windows::Foundation::Size size(400, 400);
        TestServices::WindowHelper->SetWindowSizeOverride(size);

        xaml_controls::ComboBox^ comboBox1 = nullptr;
        xaml_controls::ComboBox^ comboBox2 = nullptr;
        xaml_controls::ComboBox^ comboBox3 = nullptr;

        auto validationRules = ref new Platform::String(
            LR"(<?xml version='1.0' encoding='UTF-8'?>
                <Rules>
                    <Rule Applicability=\"//Element[@Type='Microsoft.UI.Xaml.Controls.ComboBox']\" Inclusion='Blacklist'>
                        <Property Name='FocusState'/>
                        <Property Name='IsSelectionBoxHighlighted'/>
                    </Rule>
                </Rules>)");

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"  <StackPanel>"
                L"      <ComboBox x:Name='comboBox1' Width='200' IsEditable='true' PlaceholderText='Select Option' />"
                L"      <ComboBox x:Name='comboBox2' Width='200' IsEditable='true' PlaceholderText='Select Option' />"
                L"      <ComboBox x:Name='comboBox3' Width='200' IsEditable='true' PlaceholderText='Select Option' />"
                L"  </StackPanel>"
                L"</Grid>"));

            comboBox1 = dynamic_cast<xaml_controls::ComboBox^>(rootPanel->FindName(L"comboBox1"));
            comboBox2 = dynamic_cast<xaml_controls::ComboBox^>(rootPanel->FindName(L"comboBox2"));
            comboBox3 = dynamic_cast<xaml_controls::ComboBox^>(rootPanel->FindName(L"comboBox3"));

            rootPanel->RequestedTheme = theme;

            if (useHighContrast)
            {
                TestServices::ThemingHelper->HighContrastTheme = HighContrastTheme::Black;
            }
            else
            {
                TestServices::ThemingHelper->HighContrastTheme = HighContrastTheme::None;
            }

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            comboBox1->Focus(FocusState::Keyboard);
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VisualStateManager::GoToState(comboBox2, "TextBoxOverlayPointerOver", false);
            VisualStateManager::GoToState(comboBox3, "TextBoxOverlayPressed", false);
        });

        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyUIElementTreeWithRulesInline(variation + L"1", validationRules);

        RunOnUIThread([&]()
        {
            VisualStateManager::GoToState(comboBox1, "TextBoxFocusedOverlayPointerOver", false);
        });

        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyUIElementTreeWithRulesInline(variation + L"2", validationRules);

        RunOnUIThread([&]()
        {
            VisualStateManager::GoToState(comboBox1, "TextBoxFocusedOverlayPressed", false);
        });

        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyUIElementTreeWithRulesInline(variation + L"3", validationRules);
    }

    void ComboBoxIntegrationTests::LightDismissLayerOnIslands()
    {
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

        // Validate the ComboBox DComp output when ComboBox is opened with the mouse input
        // after opened/closed the ComboBox with the touch input.
        auto comboBox = SetupBasicComboBoxTest(2 /* numberOfItems */, false /* adjustMargin */);

        RunOnUIThread([&]()
        {
            comboBox->Height = 50;
            comboBox->VerticalAlignment = VerticalAlignment::Center;
        });
        TestServices::WindowHelper->WaitForIdle();

        ComboBoxHelper::OpenComboBox(comboBox, ComboBoxHelper::OpenMethod::Mouse);

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"> There should be exactly one popup open...");
            XamlRoot^ xamlRoot = comboBox->XamlRoot;
            auto openPopups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(xamlRoot);
            VERIFY_ARE_EQUAL(openPopups->Size, 1u);

            LOG_OUTPUT(L"> ...with a Canvas as its Popup->Child...");
            auto popup = openPopups->GetAt(0);
            auto canvas = safe_cast<xaml_controls::Canvas^>(popup->Child);
            VERIFY_IS_NOT_NULL(canvas);

            LOG_OUTPUT(L"> ...and that Canvas should have only a Border inside it and no light dismiss children.");
            VERIFY_ARE_EQUAL(canvas->Children->Size, 1u);
            auto border = safe_cast<xaml_controls::Border^>(canvas->Children->GetAt(0));
            VERIFY_IS_NOT_NULL(border);
        });

        ComboBoxHelper::CloseComboBox(comboBox);
    }

    void ComboBoxIntegrationTests::CanDisableShadow()
    {
        TestCleanupWrapper cleanup;

        auto comboBox = SetupBasicComboBoxTest(5, true, true);

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Open the ComboBox");
            comboBox->IsDropDownOpen = true;
        });

        TestServices::WindowHelper->WaitForIdle();

        auto findShadow = [&]()
        {
            auto shadowTarget = TreeHelper::GetVisualChildByNameFromOpenPopups(L"PopupBorder", comboBox);
            return shadowTarget->Shadow;
        };

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Make sure it has a shadow");
            auto shadow = findShadow();
            VERIFY_IS_NOT_NULL(shadow);

            LOG_OUTPUT(L"Close the ComboBox");
            comboBox->IsDropDownOpen = false;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto dictionary = safe_cast<xaml::ResourceDictionary^>(xaml_markup::XamlReader::Load(
                LR"(<ResourceDictionary
                        xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
                        xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml">
                    <x:Boolean x:Key="IsDefaultShadowEnabled">False</x:Boolean>
                </ResourceDictionary>)"));

            xaml::Application::Current->Resources->MergedDictionaries->Append(dictionary);
        });

        // To remove the resource dictionary we added above
        auto resourceCleanup = wil::scope_exit([]()
        {
            RunOnUIThread([]()
            {
                auto mergedDictionaries = xaml::Application::Current->Resources->MergedDictionaries;
                mergedDictionaries->RemoveAt(mergedDictionaries->Size - 1);
            });
        });

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Open the ComboBox");
            comboBox->IsDropDownOpen = true;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Make sure it does not have a shadow");
            auto shadow = findShadow();
            VERIFY_IS_NULL(shadow);
        });

        ComboBoxHelper::CloseComboBox(comboBox);
    }

    void ComboBoxIntegrationTests::ValidateRestoreOnCancelIndexResetOnClose()
    {
        TestCleanupWrapper cleanup;

        auto comboBox = SetupBasicComboBoxTest();

        auto openedEvent = std::make_shared<Event>();
        auto openedRegistration = CreateSafeEventRegistration(xaml_controls::ComboBox, DropDownOpened);
        auto closedEvent = std::make_shared<Event>();
        auto closedRegistration = CreateSafeEventRegistration(xaml_controls::ComboBox, DropDownClosed);
        auto selectionChangedEvent = std::make_shared<Event>();
        auto selectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::ComboBox, SelectionChanged);

        openedRegistration.Attach(comboBox, [openedEvent]()
        {
            LOG_OUTPUT(L"ComboBox opened.");
            openedEvent->Set();
        });

        closedRegistration.Attach(comboBox, [closedEvent]()
        {
            LOG_OUTPUT(L"ComboBox closed.");
            closedEvent->Set();
        });

        selectionChangedRegistration.Attach(comboBox, [comboBox, selectionChangedEvent]()
        {
            LOG_OUTPUT(L"ComboBox selection changed. Selection is now %d.", comboBox->SelectedIndex);
            selectionChangedEvent->Set();
        });

        int originalSelectedIndex = 0;

        RunOnUIThread([&]()
        {
            comboBox->SelectedIndex = originalSelectedIndex;
        });

        selectionChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        FocusTestHelper::EnsureFocus(comboBox, FocusState::Keyboard);

        LOG_OUTPUT(L"Pressing space to open the ComboBox.");
        TestServices::KeyboardHelper->Space();

        openedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Pressing down and space to select a new item and close the ComboBox.");
        TestServices::KeyboardHelper->Down();
        TestServices::KeyboardHelper->Space();

        selectionChangedEvent->WaitForDefault();
        closedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        ComboBoxHelper::VerifySelectedIndex(comboBox, originalSelectedIndex + 1);

        selectionChangedEvent->Reset();

        LOG_OUTPUT(L"Pressing space to open the ComboBox again.");
        TestServices::KeyboardHelper->Space();

        openedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Pressing escape to close the ComboBox without selecting anything.");
        TestServices::KeyboardHelper->Escape();

        closedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        VERIFY_IS_FALSE(selectionChangedEvent->HasFired());
        ComboBoxHelper::VerifySelectedIndex(comboBox, originalSelectedIndex + 1);
    }

    void ComboBoxIntegrationTests::ValidateEditableComboBoxHasLightDismissLayer()
    {
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

        xaml_controls::Button^ button = nullptr;
        xaml_controls::ComboBox^ comboBox = nullptr;

        auto clickEvent = std::make_shared<Event>();
        auto clickRegistration = CreateSafeEventRegistration(xaml_controls::Button, Click);
        auto openedEvent = std::make_shared<Event>();
        auto openedRegistration = CreateSafeEventRegistration(xaml_controls::ComboBox, DropDownOpened);
        auto closedEvent = std::make_shared<Event>();
        auto closedRegistration = CreateSafeEventRegistration(xaml_controls::ComboBox, DropDownClosed);
        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::StackPanel, Loaded);

        RunOnUIThread([&]()
        {
            auto rootPanel = ref new xaml_controls::StackPanel();
            rootPanel->Orientation = xaml_controls::Orientation::Horizontal;

            button = ref new xaml_controls::Button();
            button->Content = ref new Platform::String(L"Click me");

            clickRegistration.Attach(button, [clickEvent]()
            {
                LOG_OUTPUT(L"Button clicked.");
                clickEvent->Set();
            });

            rootPanel->Children->Append(button);

            comboBox = ref new xaml_controls::ComboBox();
            comboBox->IsEditable = true;

            openedRegistration.Attach(comboBox, [openedEvent]()
            {
                LOG_OUTPUT(L"ComboBox opened.");
                openedEvent->Set();
            });

            closedRegistration.Attach(comboBox, [closedEvent]()
            {
                LOG_OUTPUT(L"ComboBox closed.");
                closedEvent->Set();
            });

            for (UINT i = 0; i < 5; i++)
            {
                auto item = ref new xaml_controls::ComboBoxItem();
                auto stringItem = ref new Platform::String(L"ComboBox Item ");
                stringItem += i;
                item->Content = stringItem;
                comboBox->Items->Append(item);
            }

            rootPanel->Children->Append(comboBox);

            loadedRegistration.Attach(rootPanel, [loadedEvent]()
            {
                LOG_OUTPUT(L"Root panel Loaded event raised.");
                loadedEvent->Set();
            });

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            comboBox->IsDropDownOpen = true;
        });

        openedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Comparing MockDComp to check the size of the light dismiss element.");
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

        LOG_OUTPUT(L"Clicking on the button.");
        TestServices::InputHelper->LeftMouseClick(button);

        LOG_OUTPUT(L"The combo box should close...");
        closedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"...but the button should not have been clicked.");
        VERIFY_IS_FALSE(clickEvent->HasFired());
    }

    void ComboBoxIntegrationTests::OpenTwiceThenPanComboBox()
    {
        TestCleanupWrapper cleanup;

        auto comboBox = SetupBasicComboBoxTest(50 /*numberOfItems*/);

        RunOnUIThread([&]()
        {
            comboBox->SelectionChangedTrigger = xaml_controls::ComboBoxSelectionChangedTrigger::Always;
            comboBox->Focus(xaml::FocusState::Programmatic);
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Open the ComboBox with the keyboard");
        ComboBoxHelper::OpenComboBox(comboBox, ComboBoxHelper::OpenMethod::Keyboard);

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(comboBox->IsDropDownOpen);
        });

        LOG_OUTPUT(L"Close the ComboBox");
        ComboBoxHelper::CloseComboBox(comboBox);

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"ComboBox SelectedIndex is %d", comboBox->SelectedIndex);
            VERIFY_IS_FALSE(comboBox->IsDropDownOpen);
        });

        ComboBoxHelper::VerifySelectedIndex(comboBox, 0);

        LOG_OUTPUT(L"Re-open the ComboBox with touch");
        ComboBoxHelper::OpenComboBox(comboBox, ComboBoxHelper::OpenMethod::Touch);

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(comboBox->IsDropDownOpen);
        });

        LOG_OUTPUT(L"Flick the ComboBox dropdown");
        TestServices::InputHelper->Flick(comboBox, FlickDirection::North);

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(comboBox->IsDropDownOpen);
        });

        LOG_OUTPUT(L"Click the ComboBox dropdown to select an item");
        TestServices::InputHelper->ClickMouseButton(MouseButton::Left, comboBox);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"ComboBox SelectedIndex is %d", comboBox->SelectedIndex);
            VERIFY_IS_GREATER_THAN(comboBox->SelectedIndex, 1);
            VERIFY_IS_FALSE(comboBox->IsDropDownOpen);
        });
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::ComboBox
