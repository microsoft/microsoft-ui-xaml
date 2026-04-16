// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "HyperlinkButtonIntegrationTests.h"

#include <generic\DependencyObjectTests.h>
#include <generic\FrameworkElementTests.h>
#include <generic\ButtonBaseTests.h>

#include <XamlTailored.h>
#include <ControlHelper.h>
#include <TestEvent.h>
#include <TestCleanupWrapper.h>
#include <WUCRenderingScopeGuard.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;
using namespace MockDComp;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace HyperlinkButton {

    bool HyperlinkButtonIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool HyperlinkButtonIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool HyperlinkButtonIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void HyperlinkButtonIntegrationTests::CanInstantiate()
    {
        Generic::DependencyObjectTests<xaml_controls::HyperlinkButton>::CanInstantiate();
    }

    void HyperlinkButtonIntegrationTests::CanEnterAndLeaveLiveTree()
    {
        Generic::FrameworkElementTests<xaml_controls::HyperlinkButton>::CanEnterAndLeaveLiveTree();
    }

    //
    // HyperlinkButton needs content to be set in order to have a tappable area,
    // so ButtonBaseTests::CanClickUsingTap won't work here.
    //
    void HyperlinkButtonIntegrationTests::CanClickUsingTap()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::HyperlinkButton^ hyperlinkButton = nullptr;

        auto clickEvent = std::make_shared<Event>();
        auto clickRegistration = CreateSafeEventRegistration(xaml_controls::HyperlinkButton, Click);

        RunOnUIThread([&]()
        {
            auto rootGrid = ref new xaml_controls::Grid();
            hyperlinkButton = ref new xaml_controls::HyperlinkButton();
            hyperlinkButton->Content = L"Hyperlink Button";

            clickRegistration.Attach(hyperlinkButton, ref new xaml::RoutedEventHandler([clickEvent](Platform::Object^, xaml::RoutedEventArgs^)
            {
                clickEvent->Set();
            }));

            hyperlinkButton->HorizontalAlignment = xaml::HorizontalAlignment::Center;
            hyperlinkButton->VerticalAlignment = xaml::VerticalAlignment::Center;

            rootGrid->Children->Append(hyperlinkButton);
            TestServices::WindowHelper->WindowContent = rootGrid;
        });

        TestServices::WindowHelper->WaitForIdle();

        TestServices::InputHelper->Tap(hyperlinkButton);
        clickEvent->WaitForDefault();
    }

    void HyperlinkButtonIntegrationTests::CanGetAndSetNavigationUri()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::HyperlinkButton^ hyperlinkButton = nullptr;

        RunOnUIThread([&]
        {
            hyperlinkButton = ref new xaml_controls::HyperlinkButton();
            VERIFY_IS_NULL(hyperlinkButton->NavigateUri);

            auto uri = ref new wf::Uri("http://www.bing.com");
            VERIFY_IS_NOT_NULL(uri);

            hyperlinkButton->NavigateUri = uri;
            VERIFY_IS_TRUE(hyperlinkButton->NavigateUri->Equals(uri));
        });
    }

    void HyperlinkButtonIntegrationTests::ValidateUIElementTree()
    {
        ControlHelper::ValidateUIElementTree(
            wf::Size(400, 600),
            1.f,
            []()
            {
                return ValidateTreeTestSetup();
            });
    }

    void HyperlinkButtonIntegrationTests::ValidateDCompTree()
    {
        ValidateDCompTree(false /*underlineInHighContrastOnly*/);
        ValidateDCompTree(true /*underlineInHighContrastOnly*/);
    }

    void HyperlinkButtonIntegrationTests::ValidateDCompTree(bool underlineInHighContrastOnly)
    {
        auto resourcesCleanup = wil::scope_exit([]
        {
            RunOnUIThread([]()
            {
                auto mergedDictionaries = Application::Current->Resources->MergedDictionaries;

                if (mergedDictionaries->Size > 0)
                {
                    LOG_OUTPUT(L"Removing dictionary with HyperlinkUnderlineVisible=False.");
                    mergedDictionaries->RemoveAt(mergedDictionaries->Size - 1);
                }

                TestServices::Utilities->DeleteResourceDictionaryCaches();
            });
        });

        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 600));

        RunOnUIThread([&]()
        {
            if (underlineInHighContrastOnly)
            {
                auto dictionary = safe_cast<xaml::ResourceDictionary^>(xaml_markup::XamlReader::Load(
                    LR"(<ResourceDictionary
                            xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                            xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <x:Boolean x:Key="HyperlinkUnderlineVisible">False</x:Boolean>
                        </ResourceDictionary>)"));

                LOG_OUTPUT(L"Adding dictionary with HyperlinkUnderlineVisible=False.");
                Application::Current->Resources->MergedDictionaries->Append(dictionary);
            }
        });

        auto root = ValidateTreeTestSetup();

        // Validate the dark theme of controls.
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, underlineInHighContrastOnly ? "DarkWithoutUnderline" : "Dark");

        // Validate the light theme of controls.
        RunOnUIThread([&]()
        {
            root->RequestedTheme = xaml::ElementTheme::Light;
            root->Background = ref new xaml_media::SolidColorBrush(mu::Colors::White);
        });
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, underlineInHighContrastOnly ? "LightWithoutUnderline" : "Light");

        // Validate the high-contrast theme of controls.
        RunOnUIThread([&]()
        {
            TestServices::ThemingHelper->HighContrastTheme = HighContrastTheme::Test;
            root->Background = ref new xaml_media::SolidColorBrush(mu::Colors::Black);
        });
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "HC");
    }

    xaml_controls::Panel^ HyperlinkButtonIntegrationTests::ValidateTreeTestSetup()
    {
        xaml_controls::HyperlinkButton^ restHyperlinkButton = nullptr;
        xaml_controls::HyperlinkButton^ pointerOverHyperlinkButton = nullptr;
        xaml_controls::HyperlinkButton^ pressedHyperlinkButton = nullptr;
        xaml_controls::HyperlinkButton^ disabledHyperlinkButton = nullptr;

        xaml_controls::HyperlinkButton^ focusedRestHyperlinkButton = nullptr;
        xaml_controls::HyperlinkButton^ focusedpointerOverHyperlinkButton = nullptr;
        xaml_controls::HyperlinkButton^ focusedPressedHyperlinkButton = nullptr;

        xaml_controls::HyperlinkButton^ contentChangedHyperlinkButton = nullptr;
        xaml_controls::HyperlinkButton^ customTextBlockHyperlinkButton = nullptr;

        xaml_controls::StackPanel^ rootPanel = nullptr;

        RunOnUIThread([&]()
        {
            rootPanel = ref new xaml_controls::StackPanel();
            rootPanel->HighContrastAdjustment = ElementHighContrastAdjustment::None;
            rootPanel->IsHitTestVisible = false;

            restHyperlinkButton = ref new xaml_controls::HyperlinkButton();
            restHyperlinkButton->Content = "HyperlinkButton";
            rootPanel->Children->Append(restHyperlinkButton);

            pointerOverHyperlinkButton = ref new xaml_controls::HyperlinkButton();
            pointerOverHyperlinkButton->Content = "PointerOver HyperlinkButton";
            rootPanel->Children->Append(pointerOverHyperlinkButton);

            pressedHyperlinkButton = ref new xaml_controls::HyperlinkButton();
            pressedHyperlinkButton->Content = "Pressed HyperlinkButton";
            rootPanel->Children->Append(pressedHyperlinkButton);

            disabledHyperlinkButton = ref new xaml_controls::HyperlinkButton();
            disabledHyperlinkButton->Content = "Disabled HyperlinkButton";
            disabledHyperlinkButton->IsEnabled = false;
            rootPanel->Children->Append(disabledHyperlinkButton);

            focusedRestHyperlinkButton = ref new xaml_controls::HyperlinkButton();
            focusedRestHyperlinkButton->Content = "Focused Rest HyperlinkButton";
            rootPanel->Children->Append(focusedRestHyperlinkButton);

            focusedpointerOverHyperlinkButton = ref new xaml_controls::HyperlinkButton();
            focusedpointerOverHyperlinkButton->Content = "Focused PointerOver HyperlinkButton";
            rootPanel->Children->Append(focusedpointerOverHyperlinkButton);

            focusedPressedHyperlinkButton = ref new xaml_controls::HyperlinkButton();
            focusedPressedHyperlinkButton->Content = "Focused Pressed HyperlinkButton";
            rootPanel->Children->Append(focusedPressedHyperlinkButton);

            contentChangedHyperlinkButton = ref new xaml_controls::HyperlinkButton();
            xaml_shapes::Rectangle^ rectangle = ref new xaml_shapes::Rectangle;
            rectangle->Fill = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Red);
            rectangle->Width = 50;
            rectangle->Height = 50;
            contentChangedHyperlinkButton->Content = rectangle;
            rootPanel->Children->Append(contentChangedHyperlinkButton);

            customTextBlockHyperlinkButton = ref new xaml_controls::HyperlinkButton();
            xaml_controls::TextBlock^ textBlock = ref new xaml_controls::TextBlock;
            textBlock->Text = "Custom TextBlock HyperlinkButton";
            customTextBlockHyperlinkButton->Content = textBlock;
            rootPanel->Children->Append(customTextBlockHyperlinkButton);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VisualStateManager::GoToState(pointerOverHyperlinkButton, "PointerOver", false);

            VisualStateManager::GoToState(pressedHyperlinkButton, "Pressed", false);

            VisualStateManager::GoToState(focusedRestHyperlinkButton, "Focused", false);

            VisualStateManager::GoToState(focusedpointerOverHyperlinkButton, "Focused", false);
            VisualStateManager::GoToState(focusedpointerOverHyperlinkButton, "PointerOver", false);

            VisualStateManager::GoToState(focusedPressedHyperlinkButton, "Focused", false);
            VisualStateManager::GoToState(focusedPressedHyperlinkButton, "Pressed", false);

            contentChangedHyperlinkButton->Content = "New Text HyperlinkButton";
        });
        TestServices::WindowHelper->WaitForIdle();

        return rootPanel;
    }

    void HyperlinkButtonIntegrationTests::ValidateFootprint()
    {
        TestCleanupWrapper cleanup;

        double const expectedHyperlinkButtonWidth_WithTextContent = 125;
        double const expectedHyperlinkButtonWidth_WithNoContent = 24;
        double const expectedHyperlinkButtonWidth_WithLargeContent = 100 + expectedHyperlinkButtonWidth_WithNoContent;

        double const expectedHyperlinkButtonHeight_WithTextContent = 32;
        double const expectedHyperlinkButtonHeight_WithNoContent = 13;
        double const expectedHyperlinkButtonHeight_WithLargeContent = 100 + expectedHyperlinkButtonHeight_WithNoContent;

        xaml_controls::HyperlinkButton^ hyperlinkButtonWithTextContent;
        xaml_controls::HyperlinkButton^ hyperlinkButtonWithNoContent;
        xaml_controls::HyperlinkButton^ hyperlinkButtonWithLargeContent;

        RunOnUIThread([&]()
        {
            auto rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                LR"(<StackPanel xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" >
                        <HyperlinkButton x:Name="hyperlinkButtonWithTextContent" Content="HyperlinkButton" />
                        <HyperlinkButton x:Name="hyperlinkButtonWithNoContent" />
                        <HyperlinkButton x:Name="hyperlinkButtonWithLargeContent" >
                            <Rectangle Height="100" Width="100" Fill="Red" />
                        </HyperlinkButton>
                    </StackPanel>)"));

            hyperlinkButtonWithTextContent = safe_cast<xaml_controls::HyperlinkButton^>(rootPanel->FindName(L"hyperlinkButtonWithTextContent"));
            hyperlinkButtonWithNoContent = safe_cast<xaml_controls::HyperlinkButton^>(rootPanel->FindName(L"hyperlinkButtonWithNoContent"));
            hyperlinkButtonWithLargeContent = safe_cast<xaml_controls::HyperlinkButton^>(rootPanel->FindName(L"hyperlinkButtonWithLargeContent"));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            // Verify Footprint of HyperlinkButton with text content:
            VERIFY_ARE_EQUAL(expectedHyperlinkButtonWidth_WithTextContent, hyperlinkButtonWithTextContent->ActualWidth);
            VERIFY_ARE_EQUAL(expectedHyperlinkButtonHeight_WithTextContent, hyperlinkButtonWithTextContent->ActualHeight);

            // Verify Footprint of HyperlinkButton with no content:
            VERIFY_ARE_EQUAL(expectedHyperlinkButtonWidth_WithNoContent, hyperlinkButtonWithNoContent->ActualWidth);
            VERIFY_ARE_EQUAL(expectedHyperlinkButtonHeight_WithNoContent, hyperlinkButtonWithNoContent->ActualHeight);

            // Verify Footprint of HyperlinkButton with large content:
            VERIFY_ARE_EQUAL(expectedHyperlinkButtonWidth_WithLargeContent, hyperlinkButtonWithLargeContent->ActualWidth);
            VERIFY_ARE_EQUAL(expectedHyperlinkButtonHeight_WithLargeContent, hyperlinkButtonWithLargeContent->ActualHeight);
        });
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::HyperlinkButton
