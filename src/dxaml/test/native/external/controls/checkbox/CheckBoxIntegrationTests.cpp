// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "CheckBoxIntegrationTests.h"

#include <generic\DependencyObjectTests.h>
#include <generic\FrameworkElementTests.h>
#include <generic\ButtonBaseTests.h>
#include <generic\ToggleButtonTests.h>

#include <XamlTailored.h>
#include <ControlHelper.h>
#include <TestEvent.h>
#include <TestCleanupWrapper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace CheckBox {

    bool CheckBoxIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool CheckBoxIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool CheckBoxIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void CheckBoxIntegrationTests::CanInstantiate()
    {
        Generic::DependencyObjectTests<xaml_controls::CheckBox>::CanInstantiate();
    }

    void CheckBoxIntegrationTests::CanEnterAndLeaveLiveTree()
    {
        Generic::FrameworkElementTests<xaml_controls::CheckBox>::CanEnterAndLeaveLiveTree();
    }

    void CheckBoxIntegrationTests::CanClickUsingTap()
    {
        Generic::ButtonBaseTests<xaml_controls::CheckBox>::CanClickUsingTap();
    }

    void CheckBoxIntegrationTests::CanChangeState()
    {
        Generic::ToggleButtonTests<xaml_controls::CheckBox>::CanToggle(Generic::ToggleAction::Tap);
    }

    void CheckBoxIntegrationTests::ValidateUIElementTree()
    {
        ControlHelper::ValidateUIElementTree(
            wf::Size(400, 600),
            1.f,
            []()
            {
                xaml_controls::CheckBox^ restUncheckedCheckBox = nullptr;
                xaml_controls::CheckBox^ pointerOverUncheckedCheckBox = nullptr;
                xaml_controls::CheckBox^ pressedUncheckedCheckBox = nullptr;
                xaml_controls::CheckBox^ disabledUncheckedCheckBox = nullptr;

                xaml_controls::CheckBox^ restCheckedCheckBox = nullptr;
                xaml_controls::CheckBox^ pointerOverCheckedCheckBox = nullptr;
                xaml_controls::CheckBox^ pressedCheckedCheckBox = nullptr;
                xaml_controls::CheckBox^ disabledCheckedCheckBox = nullptr;

                xaml_controls::CheckBox^ restIndeterminateCheckBox = nullptr;
                xaml_controls::CheckBox^ pointerOverIndeterminateCheckBox = nullptr;
                xaml_controls::CheckBox^ pressedIndeterminateCheckBox = nullptr;
                xaml_controls::CheckBox^ disabledIndeterminateCheckBox = nullptr;

                xaml_controls::CheckBox^ focusedRestUncheckedCheckBox = nullptr;
                xaml_controls::CheckBox^ focusedPointerOverUncheckedCheckBox = nullptr;
                xaml_controls::CheckBox^ focusedPressedUncheckedCheckBox = nullptr;

                xaml_controls::CheckBox^ focusedRestCheckedCheckBox = nullptr;
                xaml_controls::CheckBox^ focusedPointerOverCheckedCheckBox = nullptr;
                xaml_controls::CheckBox^ focusedPressedCheckedCheckBox = nullptr;

                xaml_controls::CheckBox^ focusedRestIndeterminateCheckBox = nullptr;
                xaml_controls::CheckBox^ focusedPointerOverIndeterminateCheckBox = nullptr;
                xaml_controls::CheckBox^ focusedPressedIndeterminateCheckBox = nullptr;

                xaml_controls::Grid^ rootGrid = nullptr;

                RunOnUIThread([&]()
                {
                    rootGrid = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                        L"<Grid IsHitTestVisible='False' xmlns = 'http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x = 'http://schemas.microsoft.com/winfx/2006/xaml'>"
                        L"  <Grid.ColumnDefinitions>"
                        L"      <ColumnDefinition Width='*'/>"
                        L"      <ColumnDefinition Width='*'/>"
                        L"  </Grid.ColumnDefinitions>"
                        L"</Grid>"));

                    auto panel1 = ref new xaml_controls::StackPanel();
                    auto panel2 = ref new xaml_controls::StackPanel();
                    rootGrid->Children->Append(panel1);
                    rootGrid->Children->Append(panel2);
                    rootGrid->SetColumn(panel2, 1);

                    // Panel 1
                    restUncheckedCheckBox = ref new xaml_controls::CheckBox();
                    restUncheckedCheckBox->Content = "Unchecked";
                    panel1->Children->Append(restUncheckedCheckBox);

                    pointerOverUncheckedCheckBox = ref new xaml_controls::CheckBox();
                    pointerOverUncheckedCheckBox->Content = "Unchecked PointerOver";
                    panel1->Children->Append(pointerOverUncheckedCheckBox);

                    pressedUncheckedCheckBox = ref new xaml_controls::CheckBox();
                    pressedUncheckedCheckBox->Content = "Unchecked Pressed";
                    panel1->Children->Append(pressedUncheckedCheckBox);

                    disabledUncheckedCheckBox = ref new xaml_controls::CheckBox();
                    disabledUncheckedCheckBox->Content = "Unchecked Disabled";
                    disabledUncheckedCheckBox->IsEnabled = false;
                    panel1->Children->Append(disabledUncheckedCheckBox);

                    restCheckedCheckBox = ref new xaml_controls::CheckBox();
                    restCheckedCheckBox->Content = "Checked";
                    restCheckedCheckBox->IsChecked = true;
                    panel1->Children->Append(restCheckedCheckBox);

                    pointerOverCheckedCheckBox = ref new xaml_controls::CheckBox();
                    pointerOverCheckedCheckBox->Content = "Checked PointerOver";
                    pointerOverCheckedCheckBox->IsChecked = true;
                    panel1->Children->Append(pointerOverCheckedCheckBox);

                    pressedCheckedCheckBox = ref new xaml_controls::CheckBox();
                    pressedCheckedCheckBox->Content = "Checked Pressed";
                    pressedCheckedCheckBox->IsChecked = true;
                    panel1->Children->Append(pressedCheckedCheckBox);

                    disabledCheckedCheckBox = ref new xaml_controls::CheckBox();
                    disabledCheckedCheckBox->Content = "Checked Disabled";
                    disabledCheckedCheckBox->IsChecked = true;
                    disabledCheckedCheckBox->IsEnabled = false;
                    panel1->Children->Append(disabledCheckedCheckBox);

                    restIndeterminateCheckBox = ref new xaml_controls::CheckBox();
                    restIndeterminateCheckBox->Content = "Indeterminate";
                    restIndeterminateCheckBox->IsThreeState = true;
                    restIndeterminateCheckBox->IsChecked = nullptr;
                    panel1->Children->Append(restIndeterminateCheckBox);

                    pointerOverIndeterminateCheckBox = ref new xaml_controls::CheckBox();
                    pointerOverIndeterminateCheckBox->Content = "Indeterminate PointerOver";
                    pointerOverIndeterminateCheckBox->IsThreeState = true;
                    pointerOverIndeterminateCheckBox->IsChecked = nullptr;
                    panel1->Children->Append(pointerOverIndeterminateCheckBox);

                    pressedIndeterminateCheckBox = ref new xaml_controls::CheckBox();
                    pressedIndeterminateCheckBox->Content = "Indeterminate Pressed";
                    pressedIndeterminateCheckBox->IsThreeState = true;
                    pressedIndeterminateCheckBox->IsChecked = nullptr;
                    panel1->Children->Append(pressedIndeterminateCheckBox);

                    disabledIndeterminateCheckBox = ref new xaml_controls::CheckBox();
                    disabledIndeterminateCheckBox->Content = "Indeterminate Disabled";
                    disabledIndeterminateCheckBox->IsThreeState = true;
                    disabledIndeterminateCheckBox->IsChecked = nullptr;
                    disabledIndeterminateCheckBox->IsEnabled = false;
                    panel1->Children->Append(disabledIndeterminateCheckBox);

                    // Panel 2
                    focusedRestUncheckedCheckBox = ref new xaml_controls::CheckBox();
                    focusedRestUncheckedCheckBox->Content = "Focused Unchecked";
                    panel2->Children->Append(focusedRestUncheckedCheckBox);

                    focusedPointerOverUncheckedCheckBox = ref new xaml_controls::CheckBox();
                    focusedPointerOverUncheckedCheckBox->Content = "Focused Unchecked PointerOver";
                    panel2->Children->Append(focusedPointerOverUncheckedCheckBox);

                    focusedPressedUncheckedCheckBox = ref new xaml_controls::CheckBox();
                    focusedPressedUncheckedCheckBox->Content = "Focused Unchecked Pressed";
                    panel2->Children->Append(focusedPressedUncheckedCheckBox);

                    focusedRestCheckedCheckBox = ref new xaml_controls::CheckBox();
                    focusedRestCheckedCheckBox->Content = "Focused Checked";
                    focusedRestCheckedCheckBox->IsChecked = true;
                    panel2->Children->Append(focusedRestCheckedCheckBox);

                    focusedPointerOverCheckedCheckBox = ref new xaml_controls::CheckBox();
                    focusedPointerOverCheckedCheckBox->Content = "Focused Checked PointerOver";
                    focusedPointerOverCheckedCheckBox->IsChecked = true;
                    panel2->Children->Append(focusedPointerOverCheckedCheckBox);

                    focusedPressedCheckedCheckBox = ref new xaml_controls::CheckBox();
                    focusedPressedCheckedCheckBox->Content = "Focused Checked Pressed";
                    focusedPressedCheckedCheckBox->IsChecked = true;
                    panel2->Children->Append(focusedPressedCheckedCheckBox);

                    focusedRestIndeterminateCheckBox = ref new xaml_controls::CheckBox();
                    focusedRestIndeterminateCheckBox->Content = "Focused Indeterminate";
                    focusedRestIndeterminateCheckBox->IsThreeState = true;
                    focusedRestIndeterminateCheckBox->IsChecked = nullptr;
                    panel2->Children->Append(focusedRestIndeterminateCheckBox);

                    focusedPointerOverIndeterminateCheckBox = ref new xaml_controls::CheckBox();
                    focusedPointerOverIndeterminateCheckBox->Content = "Focused Indeterminate PointerOver";
                    focusedPointerOverIndeterminateCheckBox->IsThreeState = true;
                    focusedPointerOverIndeterminateCheckBox->IsChecked = nullptr;
                    panel2->Children->Append(focusedPointerOverIndeterminateCheckBox);

                    focusedPressedIndeterminateCheckBox = ref new xaml_controls::CheckBox();
                    focusedPressedIndeterminateCheckBox->Content = "Focused Indeterminate Pressed";
                    focusedPressedIndeterminateCheckBox->IsThreeState = true;
                    focusedPressedIndeterminateCheckBox->IsChecked = nullptr;
                    panel2->Children->Append(focusedPressedIndeterminateCheckBox);

                    TestServices::WindowHelper->WindowContent = rootGrid;
                });
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    // not focused states, grouped by check states
                    VisualStateManager::GoToState(pointerOverUncheckedCheckBox, "UncheckedPointerOver", false);
                    VisualStateManager::GoToState(pressedUncheckedCheckBox, "UncheckedPressed", false);

                    VisualStateManager::GoToState(pointerOverCheckedCheckBox, "CheckedPointerOver", false);
                    VisualStateManager::GoToState(pressedCheckedCheckBox, "CheckedPressed", false);

                    VisualStateManager::GoToState(pointerOverIndeterminateCheckBox, "IndeterminatePointerOver", false);
                    VisualStateManager::GoToState(pressedIndeterminateCheckBox, "IndeterminatePressed", false);

                    // focused states, grouped by check states
                    VisualStateManager::GoToState(focusedRestUncheckedCheckBox, "Focused", false);
                    VisualStateManager::GoToState(focusedPointerOverUncheckedCheckBox, "UncheckedPointerOver", false);
                    VisualStateManager::GoToState(focusedPointerOverUncheckedCheckBox, "Focused", false);
                    VisualStateManager::GoToState(focusedPressedUncheckedCheckBox, "UncheckedPressed", false);
                    VisualStateManager::GoToState(focusedPressedUncheckedCheckBox, "Focused", false);

                    VisualStateManager::GoToState(focusedRestCheckedCheckBox, "Focused", false);
                    VisualStateManager::GoToState(focusedPointerOverCheckedCheckBox, "CheckedPointerOver", false);
                    VisualStateManager::GoToState(focusedPointerOverCheckedCheckBox, "Focused", false);
                    VisualStateManager::GoToState(focusedPressedCheckedCheckBox, "CheckedPressed", false);
                    VisualStateManager::GoToState(focusedPressedCheckedCheckBox, "Focused", false);

                    VisualStateManager::GoToState(focusedRestIndeterminateCheckBox, "Focused", false);
                    VisualStateManager::GoToState(focusedPointerOverIndeterminateCheckBox, "IndeterminatePointerOver", false);
                    VisualStateManager::GoToState(focusedPointerOverIndeterminateCheckBox, "Focused", false);
                    VisualStateManager::GoToState(focusedPressedIndeterminateCheckBox, "IndeterminatePressed", false);
                    VisualStateManager::GoToState(focusedPressedIndeterminateCheckBox, "Focused", false);
                });
                TestServices::WindowHelper->WaitForIdle();

                return rootGrid;
            }
        );
    }

    void CheckBoxIntegrationTests::ValidateFootprint()
    {
        TestCleanupWrapper cleanup;

        double const expectedCheckBoxWidth = 120;
        double const expectedCheckBoxWidth_WithLargeContent = 200 + 28;

        double const expectedCheckBoxHeight = 32;
        double const expectedCheckBoxHeight_WithLargeContent = 200 + 5;

        xaml_controls::CheckBox^ checkBox;
        xaml_controls::CheckBox^ checkBoxWithTextContent;
        xaml_controls::CheckBox^ checkBoxWithLargeContent;

        RunOnUIThread([&]()
        {
            auto rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                LR"(<StackPanel xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" >
                        <CheckBox x:Name="checkBox" />
                        <CheckBox x:Name="checkBoxWithTextContent" Content="CheckBox" />
                        <CheckBox x:Name="checkBoxWithLargeContent" >
                            <Rectangle Height="200" Width="200" Fill="Red" />
                        </CheckBox>
                    </StackPanel>)"));

            checkBox = safe_cast<xaml_controls::CheckBox^>(rootPanel->FindName(L"checkBox"));
            checkBoxWithTextContent = safe_cast<xaml_controls::CheckBox^>(rootPanel->FindName(L"checkBoxWithTextContent"));
            checkBoxWithLargeContent = safe_cast<xaml_controls::CheckBox^>(rootPanel->FindName(L"checkBoxWithLargeContent"));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            // Verify Footprint of CheckBox:
            VERIFY_ARE_EQUAL(expectedCheckBoxWidth, checkBox->ActualWidth);
            VERIFY_ARE_EQUAL(expectedCheckBoxHeight, checkBox->ActualHeight);

            // Verify Footprint of CheckBox with text Content:
            VERIFY_ARE_EQUAL(expectedCheckBoxWidth, checkBoxWithTextContent->ActualWidth);
            VERIFY_ARE_EQUAL(expectedCheckBoxHeight, checkBoxWithTextContent->ActualHeight);

            // Verify Footprint of CheckBox with large content:
            VERIFY_ARE_EQUAL(expectedCheckBoxWidth_WithLargeContent, checkBoxWithLargeContent->ActualWidth);
            VERIFY_ARE_EQUAL(expectedCheckBoxHeight_WithLargeContent, checkBoxWithLargeContent->ActualHeight);
        });
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::CheckBox
